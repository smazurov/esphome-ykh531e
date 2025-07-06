# CLAUDE.md

This file provides guidance to Claude Code (claude.ai/code) when working with code in this repository.

## Project Overview

This is an ESPHome custom component for the YKH531E air conditioner remote control. The component implements IR communication to control compatible AC units (tested on Frigidaire FHPC102AC1).

## Development Commands

### Python Environment
- Use `uv` for all Python package management
- Install dependencies: `uv sync`
- Run Python commands: `uv run <command>`

### ESPHome Testing
- Test the component: `uv run esphome compile test_device.yaml`
- Upload to device: `uv run esphome upload test_device.yaml`
- View logs: `uv run esphome logs test_device.yaml`

### Type Checking
- Run type checking: `uv run pyright` (configured with relaxed rules in pyproject.toml)

## Architecture

### Component Structure
The component follows ESPHome's standard custom component pattern:

- `esphome/components/ykh531e/` - Main component directory
  - `__init__.py` - Empty file (component auto-discovery)
  - `climate.py` - ESPHome Python integration code (config schema, code generation)
  - `ykh531e.h` - C++ header with protocol constants and class definition
  - `ykh531e.cpp` - C++ implementation with IR encoding/decoding logic

### IR Protocol Implementation
The component extends `climate_ir::ClimateIR` base class and implements the YKH531E IR protocol:

- Protocol constants defined in header (timings, codes, temperature ranges)
- Temperature encoding supports both Celsius and Fahrenheit
- Supports fan speeds (low/medium/high/auto), swing (vertical only), and multiple modes
- Protocol constraints enforced (e.g., auto mode requires auto fan speed)

### Test Configuration
`test_device.yaml` provides a complete ESPHome configuration for testing:
- Uses ESP32 with ESP-IDF framework
- Configures IR transmitter (GPIO14) and receiver (GPIO27)
- Includes the custom component via `external_components` with local path
- Sets up climate entity with component platform

### Build System
- Uses Hatchling build backend
- Packages the component for distribution
- Includes C++ source files in wheel
- Configured for Python 3.12+
- ESPHome 2025.6.1+ dependency

## Component Usage

The component supports standard ESPHome climate features:
- Temperature control (16-32°C range)
- Mode selection (auto/cool/dry/fan/heat*)
- Fan speed control
- Vertical swing toggle
- Sleep preset mode
- Optional Fahrenheit unit configuration

*Heat mode is experimental and may not work on all units.