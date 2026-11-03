# Changelog

## [2.0.1](https://github.com/smazurov/esphome-ykh531e/releases/tag/v2.0.1)

### Fixed

- Fix compilation with ESPHome 2026.10+ after `ClimateIR::supports_heat_` is
  replaced by a mode mask. Query the base class's traits instead of its storage
  fields, retaining support for ESPHome 2025.11.0 and newer.
- Synchronize Python package metadata with the release version.

### Development

- Update the locked ESPHome dependency to 2026.10.2.
- Add an ESP8266 test configuration covering heat disabled and enabled.
- Document release pinning, external-component cache refresh and upgrade checks.

No climate YAML changes are required. For default-branch sources, temporarily set
`refresh: 0s` to fetch the fix before rebuilding, or pin `@v2.0.1`. See the
[upgrade instructions](README.md#compatibility-and-upgrades) and the
[upstream API migration](https://github.com/esphome/esphome/pull/19580).

## [2.0.0](https://github.com/smazurov/esphome-ykh531e/releases/tag/v2.0.0)

- Require ESPHome 2025.11.0 or newer.
- Replace deprecated climate capability setters with the feature-flags API.
- Add `fatfs-ng` and `littlefs-python` dependencies for ESP-IDF builds.
- Enable deprecation errors in the test build and non-blocking IR transmission.

## [1.0.0](https://github.com/smazurov/esphome-ykh531e/releases/tag/v1.0.0)

- Package the YKH531E protocol implementation as an external component.
- Advertise auto mode instead of combined heat/cool mode.
- Add Fahrenheit transmission and improve fan/dry mode handling.
- Address ESPHome schema deprecations.
