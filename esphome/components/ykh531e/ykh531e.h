#pragma once

#include "esphome/components/climate_ir/climate_ir.h"

/***********************************************************************************
 * SOURCE
 ***********************************************************************************
 * The IR codes and the functional description below were taken from
 * "Reverse engineering the YK-H/531E AC remote control IR protocol"
 * https://blog.spans.fi/2024/04/16/reverse-engineering-the-yk-h531e-ac-remote-control-ir-protocol.html
 * As well as IRremoteESP8266 library
 * https://github.com/crankyoldgit/IRremoteESP8266/blob/master/src/ir_Electra.h
 * and Iverasp's ESPHome component
 * https://github.com/iverasp/esphome/tree/ykh531e
 *
 ************************************************************************************
 *  Air condition remote control encoder for:
 *
 *  YK-H/531E Remote control https://www.google.com/search?q=yk-h/531e+remote+control
 *
 *  The YK-H/531E remote control is used for different airconditioner brands.
 *
 * Tested on:
 *  Frigidaire FHPC102AC1
 ***********************************************************************************/

namespace esphome
{
  namespace ykh531e
  {

    /********************************************************************************
     *  Protocol constants
     *******************************************************************************/
    static const uint32_t YKH531E_IR_FREQUENCY = 38000;

    // Timings in microseconds
    static const uint32_t YKH531E_HEADER_MARK = 9100;
    static const uint32_t YKH531E_HEADER_SPACE = 4500;
    static const uint32_t YKH531E_BIT_MARK = 600;
    static const uint32_t YKH531E_ZERO_SPACE = 600;
    static const uint32_t YKH531E_ONE_SPACE = 1700;

    /********************************************************************************
     *
     * YK-H/531E codes
     *
     *******************************************************************************/
    static const uint8_t YKH531E_FAN_SPEED_LOW = 0b011;
    static const uint8_t YKH531E_FAN_SPEED_MID = 0b010;
    static const uint8_t YKH531E_FAN_SPEED_HIGH = 0b001;
    static const uint8_t YKH531E_FAN_SPEED_AUTO = 0b101;

    static const uint8_t YKH531E_SWING_ON = 0b000;
    static const uint8_t YKH531E_SWING_OFF = 0b111;

    static const uint8_t YKH531E_MODE_AUTO = 0b000;
    static const uint8_t YKH531E_MODE_COOL = 0b001;
    static const uint8_t YKH531E_MODE_DRY = 0b010;
    static const uint8_t YKH531E_MODE_HEAT = 0b100; // Experimental - some units support this
    static const uint8_t YKH531E_MODE_FAN = 0b110;

    // Temperature range
    static const float YKH531E_TEMP_MIN = 16.0f;
    static const float YKH531E_TEMP_MAX = 32.0f;
    static const float YKH531E_TEMP_INC = 1.0f;

    /// YKH531E Climate IR remote control
    /// Note: Only vertical swing is supported by the hardware
    /// Heat mode is experimental and may not work on all units
    class YKH531EClimate : public climate_ir::ClimateIR
    {
    public:
      YKH531EClimate()
          : climate_ir::ClimateIR(YKH531E_TEMP_MIN, YKH531E_TEMP_MAX, YKH531E_TEMP_INC, true, true,
                                  {climate::CLIMATE_FAN_AUTO, climate::CLIMATE_FAN_LOW, climate::CLIMATE_FAN_MEDIUM,
                                   climate::CLIMATE_FAN_HIGH},
                                  {climate::CLIMATE_SWING_OFF, climate::CLIMATE_SWING_VERTICAL},
                                  {climate::CLIMATE_PRESET_NONE, climate::CLIMATE_PRESET_SLEEP})
      {
        // Heat mode is automatically supported by ClimateIR base class
      }

      /// Set use of Fahrenheit units
      void set_fahrenheit(bool fahrenheit) { this->transmit_fahrenheit_ = fahrenheit; }

    protected:
      /// Override traits to provide AUTO mode instead of HEAT_COOL mode
      climate::ClimateTraits traits() override;
      void transmit_state() override;
      bool on_receive(remote_base::RemoteReceiveData data) override;

      bool transmit_fahrenheit_{false};
    };

  } // namespace ykh531e
} // namespace esphome
