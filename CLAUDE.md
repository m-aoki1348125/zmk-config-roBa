# CLAUDE.md

This file provides comprehensive guidance to Claude Code (claude.ai/code) when working with ZMK firmware configuration in this repository.

## Repository Overview

This is a ZMK (Zephyr Mechanical Keyboard) configuration repository for the "roBa" split keyboard, a custom ergonomic 38-key split keyboard with rotary encoders and trackball functionality. The keyboard uses Seeed Studio XIAO BLE controllers and integrates pointing device support through PAW3222/PMW3610 sensor drivers.

ZMK is a modern, open-source keyboard firmware built on the Zephyr™ Project RTOS, designed for power efficiency, wireless connectivity, and flexible configuration through Devicetree and Kconfig systems.

## ZMK Firmware Architecture

### Core Systems Overview

ZMK's architecture consists of several interconnected systems:

```
┌─────────────────┐    ┌─────────────────┐    ┌─────────────────┐
│   Key Scanner   │    │  Input Devices  │    │  Sensor System  │
│   (Kscan)       │    │  (Trackball)    │    │  (Encoders)     │
└─────────┬───────┘    └─────────┬───────┘    └─────────┬───────┘
          │                      │                      │
          └──────────────────────┼──────────────────────┘
                                 │
                    ┌─────────────┴─────────────┐
                    │     Event Manager         │
                    │   (Event Distribution)    │
                    └─────────────┬─────────────┘
                                 │
                    ┌─────────────┴─────────────┐
                    │     Keymap System         │
                    │  (Layer Management &      │
                    │   Behavior Resolution)    │
                    └─────────────┬─────────────┘
                                 │
                    ┌─────────────┴─────────────┐
                    │     Behavior System       │
                    │   (Action Processing)     │
                    └─────────────┬─────────────┘
                                 │
                    ┌─────────────┴─────────────┐
                    │      HID System           │
                    │   (Report Generation)     │
                    └─────────────┬─────────────┘
                                 │
                    ┌─────────────┴─────────────┐
                    │    Endpoints System       │
                    │  (USB/BLE Transport)      │
                    └───────────────────────────┘
```

### Keymap System Architecture

The Keymap System is central to ZMK's input processing:

- **Layer Management**: Maintains active layer bitmap (`_zmk_keymap_layer_state`), default layer, and per-key active layers
- **Event Processing**: Processes `zmk_position_state_changed` and `zmk_sensor_event` events
- **Behavior Resolution**: Maps physical positions to behavior bindings based on active layers
- **Layer Priority**: Iterates through active layers from highest to lowest priority

**Key Functions**:
- `zmk_keymap_position_state_changed()`: Handles key press/release events
- `zmk_keymap_sensor_event()`: Handles encoder/sensor events  
- `zmk_keymap_layer_activate()/deactivate()/toggle()`: Layer management

### Behavior System Architecture

Behaviors define actions triggered by input events and implement the `behavior_driver_api`:

**Core Behavior Types**:
- **Key Press Behaviors**: `&kp` (keycode), `&mt` (mod-tap), `&lt` (layer-tap)
- **Layer Behaviors**: `&mo` (momentary), `&to` (to-layer), `&tog` (toggle), `&sl` (sticky)
- **Mouse Behaviors**: `&mkp` (mouse button), `&mmv` (mouse move), `&msc` (mouse scroll)
- **Bluetooth Behaviors**: `&bt` (profile management), `&out` (output selection)
- **Utility Behaviors**: `&trans` (transparent), `&none` (no-op), `&macro` (sequences)

**Behavior Locality**:
- **Central**: Processed on central device (default)
- **Global**: Affects all split parts (RGB, power management)
- **Source**: Affects originating device only (reset behaviors)

### Split Keyboard Architecture

#### Communication Protocol
ZMK split keyboards communicate via custom BLE GATT service with characteristics:

- **Position State**: Key press/release events as bitmap
- **Run Behavior**: Central-to-peripheral behavior execution
- **Sensor State**: Encoder/sensor data transmission
- **HID Indicators**: LED state synchronization (Caps/Num/Scroll Lock)
- **Input Event**: Pointing device events from peripherals

#### Role Definitions
- **Central Device**: 
  - Receives events from peripherals
  - Processes keymap logic and layer states
  - Generates HID reports
  - Communicates with host (USB/BLE)
  - Higher power consumption

- **Peripheral Device**:
  - Scans key matrix and reads sensors
  - Sends events to central via BLE
  - Executes behaviors when instructed by central
  - Lower power consumption
  - Cannot communicate directly with host

### Input and Pointing Device System

#### Input Device Architecture
```
Physical Device → Input Driver → Input Listener → Input Processors → HID System
```

**Components**:
- **Input Devices**: Physical hardware (trackballs, trackpads) defined in Devicetree
- **Input Listeners**: Bridge between input devices and ZMK system (`zmk,input-listener`)
- **Input Processors**: Transform/modify input events before HID processing

**Input Processors**:
- `&zip_xy_scaler`: Scale X/Y movement values
- `&zip_xy_transform`: Transform coordinates (invert, swap axes)
- `&zip_xy_to_scroll_mapper`: Convert movement to scroll events
- `&zip_temp_layer`: Auto-activate layers on input (key for auto-mouse functionality)

#### Auto Mouse Layer Implementation
Official ZMK pattern for auto mouse layer switching:

```dts
trackball_listener: trackball_listener {
    compatible = "zmk,input-listener";
    device = <&trackball>;
    input-processors = <&zip_temp_layer 4 600>;  // Layer 4, 600ms timeout
};
```

### Configuration Systems

#### Devicetree Configuration
ZMK uses Devicetree for hardware definition and keymap configuration:

**File Types**:
- `.dts`: Base hardware definition
- `.overlay`: Hardware additions/overrides for shields
- `.keymap`: Keymap and user hardware configuration
- `.dtsi`: Reusable includes for shared definitions

**Key Devicetree Nodes**:
- `/chosen`: Global references (`zmk,kscan`, `zmk,matrix-transform`)
- `/keymap`: Layer definitions with behavior bindings
- `/behaviors`: Custom behavior instances
- Hardware nodes: GPIO, SPI, I2C configurations

**Devicetree Syntax**:
```dts
node_label: node_name {
    compatible = "binding-compatible-string";
    property = <value>;
    string-property = "text";
    array-property = <1 2 3>;
    phandle-property = <&other_node>;
};
```

#### Kconfig Configuration
Compile-time configuration using `.conf` files:

**Essential Options**:
- `CONFIG_ZMK_POINTING=y`: Enable pointing device support
- `CONFIG_ZMK_SPLIT=y`: Enable split keyboard functionality
- `CONFIG_ZMK_SPLIT_ROLE_CENTRAL=y`: Configure as central device
- `CONFIG_ZMK_BLE=y`: Enable Bluetooth connectivity
- `CONFIG_ZMK_USB=y`: Enable USB connectivity

**Deprecated Options**:
- `CONFIG_ZMK_MOUSE=y`: Use `CONFIG_ZMK_POINTING=y` instead

### HID Implementation

ZMK generates HID reports for communication with hosts:

**HID Report Types**:
- **Keyboard Reports**: Standard keyboard input with NKRO/HKRO support
- **Consumer Reports**: Media keys (volume, playback control)
- **Mouse Reports**: Mouse movement, buttons, scroll (requires `CONFIG_ZMK_POINTING=y`)

**Configuration Options**:
- `CONFIG_ZMK_HID_REPORT_TYPE_NKRO`: N-Key Rollover support
- `CONFIG_ZMK_HID_CONSUMER_REPORT_USAGES_FULL`: Full consumer key support

## Project-Specific Architecture

### Hardware Configuration
- **MCU**: Seeed Studio XIAO BLE (nRF52840)
- **Split Communication**: BLE between halves
- **Trackball**: PAW3222 sensor on right half (SPI)
- **Encoders**: Rotary encoders on both halves
- **Matrix**: 38-key split (19 keys per half)

### File Structure
```
config/
├── roBa.keymap              # Main keymap definition
├── west.yml                 # Module dependencies
└── boards/shields/Test/
    ├── roBa.dtsi           # Shared hardware definitions
    ├── roBa_L.overlay      # Left half configuration
    ├── roBa_R.overlay      # Right half configuration
    ├── roBa_L.conf         # Left half Kconfig
    └── roBa_R.conf         # Right half Kconfig
```

### Layer Architecture
- **Layer 0**: Default typing layer
- **Layer 1**: Function layer (F-keys, media)
- **Layer 2**: Number/symbol layer
- **Layer 3**: Arrow navigation layer
- **Layer 4**: Auto mouse layer (activated by trackball movement)
- **Layer 5**: Scroll layer (for scroll mode)
- **Layer 6**: Bluetooth management layer

### Trackball Implementation
Current configuration uses official ZMK pattern:

```dts
// In roBa_R.overlay
&spi0 {
    trackball: trackball@0 {
        compatible = "pixart,paw3222";
        reg = <0>;
        spi-max-frequency = <2000000>;
        irq-gpios = <&gpio0 2 (GPIO_ACTIVE_LOW | GPIO_PULL_UP)>;
    };
};

/ {
    trackball_listener: trackball_listener {
        compatible = "zmk,input-listener";
        device = <&trackball>;
        input-processors = <&zip_temp_layer 4 600>;  // Auto mouse layer
    };
};
```

### Build System
- **Automated Builds**: GitHub Actions triggered on push
- **Build Targets**: `roBa_L`, `roBa_R`, `settings_reset`
- **Dependencies**: PAW3222 driver from m-aoki1348125/zmk-driver-paw3222

## Development Guidelines

### Configuration Best Practices

1. **Split Keyboard Development**:
   - Always configure central/peripheral roles correctly
   - Test both halves independently
   - Remember that keymap changes typically only require central reflashing

2. **Pointing Device Development**:
   - Use official `&zip_temp_layer` for auto mouse layer
   - Prefer `CONFIG_ZMK_POINTING=y` over deprecated `CONFIG_ZMK_MOUSE=y`
   - Define input listeners for all input devices
   - Use input processors for coordinate transformations

3. **Devicetree Development**:
   - Always include proper `#include` statements
   - Use labels for node references across files
   - Validate compatible strings match driver bindings
   - Organize shared definitions in `.dtsi` files

4. **Behavior Configuration**:
   - Understand behavior locality for split keyboards
   - Use appropriate behavior types for intended actions
   - Test layer interactions thoroughly
   - Consider power consumption implications

### Debugging Guidelines

1. **Build Issues**:
   - Check Devicetree syntax and node references
   - Verify Kconfig option validity
   - Ensure all required includes are present
   - Validate compatible strings match available drivers

2. **Split Communication Issues**:
   - Verify BLE pairing between halves
   - Check central/peripheral role configuration
   - Monitor battery levels on both halves
   - Test individual half functionality

3. **Pointing Device Issues**:
   - Verify SPI/I2C pin configurations
   - Check driver compatibility and version
   - Test basic input listener functionality before adding processors
   - Monitor power consumption with sensors enabled

### Testing Strategies

1. **Incremental Development**:
   - Start with basic key matrix functionality
   - Add layers and behaviors incrementally
   - Test pointing devices with minimal configuration first
   - Add auto mouse layer functionality last

2. **Split Testing**:
   - Test each half independently
   - Verify central processes all events correctly
   - Test behavior locality (central/global/source)
   - Monitor BLE connection stability

3. **Input Device Testing**:
   - Test basic cursor movement before auto layer
   - Verify input processors work individually
   - Test layer switching with various timeouts
   - Validate HID report generation

## Technical References

### ZMK Official Documentation
- Repository: https://github.com/zmkfirmware/zmk
- Documentation: https://zmk.dev/
- Behavior Reference: https://zmk.dev/docs/behaviors/
- Configuration Reference: https://zmk.dev/docs/config/

### Module Dependencies
- PAW3222 Driver: https://github.com/m-aoki1348125/zmk-driver-paw3222
- Auto Layer Module: https://github.com/urob/zmk-auto-layer

### Hardware References  
- Seeed Studio XIAO BLE: nRF52840 MCU
- PAW3222: Optical tracking sensor
- Standard GPIO matrix scanning for switches

This comprehensive knowledge base should guide all future development and troubleshooting for this ZMK configuration repository.