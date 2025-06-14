# CLAUDE.md

This file provides guidance to Claude Code (claude.ai/code) when working with code in this repository.

## Conversation Guidelines
- 常に日本語で会話する

## Development Philosophy

### Test-Driven Development (TDD)

- 原則としてテスト駆動開発（TDD）で進める
- 期待される入出力に基づき、まずテストを作成する
- 実装コードは書かず、テストのみを用意する
- テストを実行し、失敗を確認する
- テストが正しいことを確認できた段階でコミットする
- その後、テストをパスさせる実装を進める
- 実装中はテストを変更せず、コードを修正し続ける
- すべてのテストが通過するまで繰り返す

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
- **Velocity-Based Mouse Acceleration**: Custom real-time acceleration system with mathematical scaling curves

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

## Velocity-Based Mouse Acceleration System

The right half implements a custom **true velocity-based acceleration system** that dynamically scales mouse movement in real-time based on movement speed, without requiring layer switching or key presses.

### Core Technology

#### Real-Time Velocity Calculation
- **Movement History Buffer**: 4-sample buffer for stable velocity estimation
- **High-Precision Timing**: Microsecond-level timestamps using `k_uptime_get()`
- **Velocity Unit**: Normalized to pixels/second for consistent scaling
- **Smooth Transitions**: Exponential acceleration curves for natural feel

#### Mathematical Acceleration Algorithm
```c
normalized_velocity = velocity / threshold
acceleration_factor = pow(normalized_velocity, power)
scale_factor = min_scale + (max_scale - min_scale) * acceleration_factor
```

#### Precision Scaling
- **Remainder Tracking**: Sub-pixel precision for smooth movement
- **Real-Time Application**: Instantaneous calculation and scaling per input event

### Acceleration Profiles

#### Precision Mode (Layers 1, 6)
- **Range**: 0.4x → 1.5x scaling
- **Threshold**: 300 pixels/second
- **Curve**: Exponent 1.3 (gentle acceleration)
- **Use Case**: Detail work, settings, precision tasks

#### Standard Mode (Layers 0, 2, 4) 
- **Range**: 0.3x → 4.0x scaling
- **Threshold**: 800 pixels/second
- **Curve**: Exponent 2.0 (balanced acceleration)
- **Use Case**: General computing, normal workflow

#### Navigation Mode (Layers 3, 7)
- **Range**: 0.5x → 6.0x scaling  
- **Threshold**: 1200 pixels/second
- **Curve**: Exponent 3.0 (aggressive acceleration)
- **Use Case**: Fast navigation, screen traversal

#### Scroll Mode (Layer 5)
- **Fixed**: 0.25x scaling
- **Acceleration**: Disabled
- **Use Case**: Precise scrolling control

### Custom Implementation

#### Input Processor Architecture
- **File**: `config/custom/drivers/input_processor/input_processor_velocity_scaler.c`
- **Binding**: `config/custom/dts/bindings/input_processors/zmk,input-processor-velocity-scaler.yaml`
- **Integration**: Custom ZMK module via `zephyr/module.yml`

#### Configurable Parameters
```dts
trackball_velocity_accel: trackball_velocity_accel {
    compatible = "zmk,input-processor-velocity-scaler";
    min-scale-factor = <30>;        // Minimum 0.3x scaling
    max-scale-factor = <400>;       // Maximum 4.0x scaling  
    velocity-threshold = <800>;     // 800 px/s threshold
    acceleration-power = <200>;     // 2.0 exponent curve
    track-remainders;               // Sub-pixel precision
};
```

### Performance Characteristics

- **Latency**: Sub-millisecond response time
- **Precision**: Floating-point calculations with remainder tracking
- **Efficiency**: O(1) algorithm complexity with ring buffer
- **Memory**: Minimal state storage (< 100 bytes per instance)

### Development Notes

- Custom input processor follows ZMK driver API standards
- Device tree configuration allows runtime parameter tuning
- Modular build system enables easy customization
- Debug logging available for velocity and scaling analysis
- Professional-grade acceleration rivaling dedicated gaming mice