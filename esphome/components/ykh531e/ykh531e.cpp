#include "ykh531e.h"
#include "esphome/core/log.h"

namespace esphome
{
  namespace ykh531e
  {

    static const char *const TAG = "ykh531e.climate";

    uint8_t encode_temperature_celsius(float temperature)
    {
      if (temperature > YKH531E_TEMP_MAX)
      {
        return YKH531E_TEMP_MAX - 8;
      }
      if (temperature < YKH531E_TEMP_MIN)
      {
        return YKH531E_TEMP_MIN - 8;
      }
      return temperature - 8;
    }

    float decode_temperature_celsius(uint8_t encoded_temperature) { return encoded_temperature + 8.0f; }

    uint8_t encode_temperature_fahrenheit(float temperature_c)
    {
      // Convert Celsius to Fahrenheit and apply offset for protocol
      float temp_f = temperature_c * 9.0 / 5.0 + 32.0;
      // Clamp to valid Fahrenheit range (60-90°F)
      if (temp_f > 90.0f)
        temp_f = 90.0f;
      if (temp_f < 60.0f)
        temp_f = 60.0f;
      return (uint8_t)temp_f + 8; // Apply +8 offset for protocol encoding
    }

    void YKH531EClimate::transmit_state()
    {
      // Protocol constraints:
      // - Auto mode: fan speed should be auto
      // - Dry mode: fan speed should be low
      // - Fan mode: fan speed cannot be auto, temperature is ignored
      uint8_t ir_message[13] = {0};

      // byte 0: preamble
      ir_message[0] = 0b11000011;

      // byte1: temperature and vertical swing
      // Note: Only vertical swing is supported by hardware
      switch (this->swing_mode)
      {
      case climate::CLIMATE_SWING_VERTICAL:
        ir_message[1] |= YKH531E_SWING_ON;
        break;
      case climate::CLIMATE_SWING_OFF:
      default:
        ir_message[1] |= YKH531E_SWING_OFF;
        break;
      }

      // Set temperature based on mode
      // Fan-only and dry modes don't use temperature, so skip temperature encoding
      if (this->mode != climate::CLIMATE_MODE_FAN_ONLY && 
          this->mode != climate::CLIMATE_MODE_DRY)
      {
        if (this->transmit_fahrenheit_)
        {
          ESP_LOGD(TAG, "Transmitting in Fahrenheit mode");
          // Fahrenheit mode - only use Fahrenheit field, leave Celsius field empty

          // Set Fahrenheit temperature (bits 81-87 = byte 10, bits 1-7)
          uint8_t temp_f_encoded = encode_temperature_fahrenheit(this->target_temperature);
          float temp_f = this->target_temperature * 9.0 / 5.0 + 32.0;
          ESP_LOGD(TAG, "Target %.1f°C = %.1f°F, encoded as %d", this->target_temperature, temp_f, temp_f_encoded);
          ir_message[10] |= (temp_f_encoded << 1) & 0b11111110;
        }
        else
        {
          ESP_LOGD(TAG, "Transmitting in Celsius mode");
          // Celsius mode - use Celsius field only
          ir_message[1] |= encode_temperature_celsius(this->target_temperature) << 3;
        }
      }
      else
      {
        ESP_LOGD(TAG, "Mode %s doesn't use temperature - skipping temperature encoding", 
                 this->mode == climate::CLIMATE_MODE_FAN_ONLY ? "FAN_ONLY" : "DRY");
      }

      // byte4: 5 unknown bits and fan speed
      switch (this->fan_mode.value())
      {
      case climate::CLIMATE_FAN_LOW:
        ir_message[4] |= YKH531E_FAN_SPEED_LOW << 5;
        break;
      case climate::CLIMATE_FAN_MEDIUM:
        ir_message[4] |= YKH531E_FAN_SPEED_MID << 5;
        break;
      case climate::CLIMATE_FAN_HIGH:
        ir_message[4] |= YKH531E_FAN_SPEED_HIGH << 5;
        break;
      case climate::CLIMATE_FAN_AUTO:
        ir_message[4] |= YKH531E_FAN_SPEED_AUTO << 5;
        break;
      default:
        break;
      }

      // byte6: 2 unknown bits, sleep, 2 unknown bits and mode
      ir_message[6] = this->preset == climate::CLIMATE_PRESET_SLEEP ? 1 << 2 : 0;

      // Set Fahrenheit flag (bit 49 = byte 6, bit 1)
      if (this->transmit_fahrenheit_)
      {
        ir_message[6] |= 0b00000010;
      }

      switch (this->mode)
      {
      case climate::CLIMATE_MODE_AUTO:
        ir_message[6] |= YKH531E_MODE_AUTO << 5;
        break;
      case climate::CLIMATE_MODE_COOL:
        ir_message[6] |= YKH531E_MODE_COOL << 5;
        break;
      case climate::CLIMATE_MODE_DRY:
        ir_message[6] |= YKH531E_MODE_DRY << 5;
        break;
      case climate::CLIMATE_MODE_HEAT:
        ESP_LOGI(TAG, "Heat mode is experimental and may not work on all units, log an issue to confirm support");
        ir_message[6] |= YKH531E_MODE_HEAT << 5;
        break;
      case climate::CLIMATE_MODE_FAN_ONLY:
        ir_message[6] |= YKH531E_MODE_FAN << 5;
        break;
      default:
        break;
      }

      // byte9: 5 unknown bits, power and 2 unknown bits
      ir_message[9] = this->mode == climate::CLIMATE_MODE_OFF ? 0 : 1 << 5;

      // byte12: checksum
      uint8_t checksum = 0;
      for (int i = 0; i < 12; i++)
      {
        checksum += ir_message[i];
      }
      ir_message[12] = checksum;
      ESP_LOGD(TAG, "Transmitting: %02X %02X %02X %02X %02X %02X %02X %02X %02X %02X %02X %02X %02X",
               ir_message[0], ir_message[1], ir_message[2], ir_message[3], ir_message[4],
               ir_message[5], ir_message[6], ir_message[7], ir_message[8], ir_message[9],
               ir_message[10], ir_message[11], ir_message[12]);
      auto transmit = this->transmitter_->transmit();
      auto *data = transmit.get_data();

      data->set_carrier_frequency(YKH531E_IR_FREQUENCY);

      // header
      data->item(YKH531E_HEADER_MARK, YKH531E_HEADER_SPACE);
      // data
      for (uint8_t i : ir_message)
      {
        for (uint8_t j = 0; j < 8; j++)
        {
          bool bit = i & (1 << j);
          data->item(YKH531E_BIT_MARK, bit ? YKH531E_ONE_SPACE : YKH531E_ZERO_SPACE);
        }
      }

      // footer
      data->item(YKH531E_BIT_MARK, 0);

      transmit.perform();
    }

    bool YKH531EClimate::on_receive(remote_base::RemoteReceiveData data)
    {
      // validate header
      if (!data.expect_item(YKH531E_HEADER_MARK, YKH531E_HEADER_SPACE))
      {
        // Check if signal might be inverted
        int32_t raw_mark = data.peek(0);
        int32_t raw_space = data.peek(1);

        if (raw_mark < 0 && raw_space > 0)
        {
          ESP_LOGW(TAG, "Header fail - received inverted signal. "
                        "Try adding 'inverted: true' to your remote_receiver pin configuration.");
        }

        ESP_LOGV(TAG, "Header fail");
        return false;
      }
      ESP_LOGD(TAG, "Header received");

      uint8_t ir_message[13] = {0};
      for (int i = 0; i < 13; i++)
      {
        // read all bits
        for (int j = 0; j < 8; j++)
        {
          if (data.expect_item(YKH531E_BIT_MARK, YKH531E_ONE_SPACE))
          {
            ir_message[i] |= 1 << j;
          }
          else if (!data.expect_item(YKH531E_BIT_MARK, YKH531E_ZERO_SPACE))
          {
            ESP_LOGV(TAG, "Byte %d bit %d fail", i, j);
            return false;
          }
        }
        ESP_LOGVV(TAG, "Byte %d %02X", i, ir_message[i]);
      }

      // validate footer
      if (!data.expect_mark(YKH531E_BIT_MARK))
      {
        ESP_LOGV(TAG, "Footer fail");
        return false;
      }

      // validate checksum
      uint8_t checksum = 0;
      for (int i = 0; i < 12; i++)
      {
        checksum += ir_message[i];
      }
      if (ir_message[12] != checksum)
      {
        ESP_LOGV(TAG, "Checksum fail");
        return false;
      }

      bool power = (ir_message[9] & 0b00100000) >> 5;
      if (!power)
      {
        this->mode = climate::CLIMATE_MODE_OFF;
      }
      else
      {
        uint8_t vertical_swing = ir_message[1] & 0b00000111;

        if (vertical_swing == YKH531E_SWING_ON)
        {
          this->swing_mode = climate::CLIMATE_SWING_VERTICAL;
        }
        else
        {
          this->swing_mode = climate::CLIMATE_SWING_OFF;
        }

        uint8_t fan_speed = (ir_message[4] & 0b11100000) >> 5;
        switch (fan_speed)
        {
        case YKH531E_FAN_SPEED_LOW:
          this->fan_mode = climate::CLIMATE_FAN_LOW;
          break;
        case YKH531E_FAN_SPEED_MID:
          this->fan_mode = climate::CLIMATE_FAN_MEDIUM;
          break;
        case YKH531E_FAN_SPEED_HIGH:
          this->fan_mode = climate::CLIMATE_FAN_HIGH;
          break;
        case YKH531E_FAN_SPEED_AUTO:
          this->fan_mode = climate::CLIMATE_FAN_AUTO;
          break;
        }

        this->preset = ir_message[6] & 0b00000100 ? climate::CLIMATE_PRESET_SLEEP : climate::CLIMATE_PRESET_NONE;

        // Parse mode first to determine if temperature should be updated
        uint8_t mode = (ir_message[6] & 0b11100000) >> 5;
        switch (mode)
        {
        case YKH531E_MODE_AUTO:
          this->mode = climate::CLIMATE_MODE_AUTO;
          break;
        case YKH531E_MODE_COOL:
          this->mode = climate::CLIMATE_MODE_COOL;
          break;
        case YKH531E_MODE_DRY:
          this->mode = climate::CLIMATE_MODE_DRY;
          break;
        case YKH531E_MODE_HEAT:
          this->mode = climate::CLIMATE_MODE_HEAT;
          break;
        case YKH531E_MODE_FAN:
          this->mode = climate::CLIMATE_MODE_FAN_ONLY;
          break;
        }

        // Only update temperature for modes that use temperature control
        // Fan-only and dry modes don't use temperature
        if (this->mode != climate::CLIMATE_MODE_FAN_ONLY && 
            this->mode != climate::CLIMATE_MODE_DRY)
        {
          // Check if temperature is in Fahrenheit (bit 49 = byte 6, bit 1)
          bool fahrenheit = (ir_message[6] & 0b00000010) >> 1;

          if (fahrenheit)
          {
            // Fahrenheit temperature is in bits 81-87 (byte 10, bits 1-7)
            uint8_t temp_f = (ir_message[10] & 0b11111110) >> 1;
            // Protocol stores F temp with -8 offset (not +8 as documented)
            temp_f -= 8; // Convert from protocol value to actual Fahrenheit
            // Convert to Celsius for ESPHome
            this->target_temperature = (temp_f - 32) * 5.0 / 9.0;
            ESP_LOGV(TAG, "Temperature: %d°F = %.1f°C", temp_f, this->target_temperature);
          }
          else
          {
            // Celsius temperature is in bits 11-15 (byte 1, bits 3-7)
            uint8_t temp_c = decode_temperature_celsius((ir_message[1] & 0b11111000) >> 3);
            this->target_temperature = temp_c;
            ESP_LOGV(TAG, "Temperature: %d°C", temp_c);
          }
        }
        else
        {
          ESP_LOGV(TAG, "Mode %s doesn't use temperature - preserving current setting", 
                   this->mode == climate::CLIMATE_MODE_FAN_ONLY ? "FAN_ONLY" : "DRY");
        }
      }

      this->publish_state();
      return true;
    }

    climate::ClimateTraits YKH531EClimate::traits()
    {
      auto traits = climate::ClimateTraits();
      traits.set_supports_current_temperature(this->sensor_ != nullptr);
      traits.set_supports_action(false);
      traits.set_visual_min_temperature(YKH531E_TEMP_MIN);
      traits.set_visual_max_temperature(YKH531E_TEMP_MAX);
      traits.set_visual_temperature_step(YKH531E_TEMP_INC);

      // Always support these modes
      traits.set_supported_modes({climate::CLIMATE_MODE_OFF, climate::CLIMATE_MODE_AUTO});
      
      // Always add cool, dry, and fan_only as the hardware supports them
      traits.add_supported_mode(climate::CLIMATE_MODE_COOL);
      traits.add_supported_mode(climate::CLIMATE_MODE_DRY);
      traits.add_supported_mode(climate::CLIMATE_MODE_FAN_ONLY);
      
      // Only add heat if configured in YAML
      if (this->supports_heat_)
        traits.add_supported_mode(climate::CLIMATE_MODE_HEAT);

      traits.set_supported_fan_modes(
          {climate::CLIMATE_FAN_AUTO, climate::CLIMATE_FAN_LOW, climate::CLIMATE_FAN_MEDIUM,
           climate::CLIMATE_FAN_HIGH});

      traits.set_supported_swing_modes({climate::CLIMATE_SWING_OFF, climate::CLIMATE_SWING_VERTICAL});

      traits.set_supported_presets({climate::CLIMATE_PRESET_NONE, climate::CLIMATE_PRESET_SLEEP});

      return traits;
    }

  } // namespace ykh531e
} // namespace esphome
