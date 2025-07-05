# CLAUDE.md

This file provides guidance to Claude Code (claude.ai/code) when working with code in this repository.

## Repository Overview

This is a ZMK (Zephyr Mechanical Keyboard) configuration repository for the "roBa" split keyboard, which is a custom ergonomic 38-key split keyboard with rotary encoders. The keyboard uses Seeed Studio XIAO BLE controllers and includes mouse functionality through a PMW3610 sensor driver.

## Architecture

- **Shield Definition**: Located in `config/boards/shields/Test/` with separate left (`roBa_L`) and right (`roBa_R`) configurations
- **Keymap**: Main layout defined in `config/roBa.keymap` with 8 layers including default, function, number, arrow, mouse, scroll, and system layers
- **Build Configuration**: Uses GitHub Actions with `build.yaml` defining board/shield combinations for automated firmware builds
- **Hardware Integration**: Custom PMW3610 mouse sensor driver included via west.yml dependency

## Key Components

- **Physical Layout**: Defined in `roBa.dtsi` with precise key positioning for the 38-key matrix
- **Sensor Integration**: Rotary encoders configured for different behaviors per layer (scroll, volume, arrow navigation)
- **Layer System**: 8 layers with Japanese input support (LANG1/LANG2 keys), mouse functionality, and Bluetooth management
- **Split Configuration**: Separate overlay files for left/right halves with different configurations (right side includes PMW3610 sensor)

## Build Commands

Firmware builds are handled automatically via GitHub Actions when pushing to the repository. The build matrix in `build.yaml` generates:
- `roBa_L` - Left half firmware
- `roBa_R` - Right half firmware with Studio RPC USB UART snippet
- `settings_reset` - Settings reset firmware

Manual builds would require ZMK development environment setup with west tool.

## Development Notes

- Layer definitions use custom behaviors like `lt_to_layer_0` for layer-tap-to-base functionality
- Combos are defined for common key combinations (tab, shift+tab, quotes, etc.)
- Sensor behaviors are layer-specific: scroll on default, volume on function, arrows on arrow layer
- Bluetooth management is handled in layer 6 with device selection and clearing functions

## Local Development Environment

When setting up test and build environments locally, always use virtual environments such as uv or venv for isolation and dependency management.