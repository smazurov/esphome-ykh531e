# ESPHome YKH531E Climate Component

ESPHome custom component for YKH531E IR remote control protocol. Controls compatible AC units via infrared signals.

Tested with: Frigidaire FHPC102AC1

## Installation

Add to your ESPHome configuration:

```yaml
external_components:
  - source: github://smazurov/esphome-ykh531e@v2.0.1
    components: [ykh531e]
```

See [ESPHome External Components](https://esphome.io/components/external_components/) for more installation options.

## Compatibility and upgrades

Requires ESPHome 2025.11.0 or newer. See [CHANGELOG.md](CHANGELOG.md) for releases.

Version 2.0.1 fixes compilation on ESPHome 2026.10+, which replaces the protected
`ClimateIR::supports_heat_` field with a mode mask. The component reads heat support
through the base class's traits API so earlier supported ESPHome versions continue
to work. See the [upstream migration](https://esphome.io/blog/2026/10/21/esphome-2026-10/#climate-ir-mode-storage).

To upgrade, use the release tag above and rebuild. If you follow the default branch
instead, ESPHome caches Git sources for one day by default. Temporarily add
`refresh: 0s` to fetch the fix immediately, then remove it after a successful build:

```yaml
external_components:
  - source: github://smazurov/esphome-ykh531e
    components: [ykh531e]
    refresh: 0s
```

Keep your existing `supports_heat` setting; no climate YAML changes are needed.
After installing, verify cooling, off and fan controls, and that a cooling-only
configuration does not advertise heat.

## Configuration

```yaml
remote_transmitter:
  pin: GPIO14
  carrier_duty_percent: 50%

climate:
  - platform: ykh531e
    name: "AC Unit"
    supports_heat: false
    supports_cool: true
    use_fahrenheit: true # Optional: use Fahrenheit instead of Celsius
```

## Features

- **Temperature**: 16-32°C (60-90°F)
- **Modes**: Auto, Cool, Dry, Fan, Heat (untested)
- **Fan Speed**: Low, Medium, High, Auto
- **Swing**: Vertical only
- **Presets**: Sleep mode
- **Units**: Celsius or Fahrenheit

## Hardware Setup

Connect IR transmitter to GPIO14 (or your preferred pin).

Optional: Connect IR receiver to GPIO27 to update climate state when using the physical remote control.

```yaml
remote_receiver:
  pin: GPIO27
```

## Development

To modify and test:

```bash
# Install dependencies
uv sync

# Test ESP32 / ESP-IDF compilation
uv run esphome compile test_device.yaml

# Test ESP8266 compilation with heat disabled and enabled
uv run esphome compile test_device_esp8266.yaml

# Check the minimum supported version without changing the project lockfile
uv run --python 3.13 --no-project --with esphome==2025.11.0 esphome compile test_device_esp8266.yaml
```
