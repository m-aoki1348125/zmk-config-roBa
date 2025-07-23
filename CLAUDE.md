# CLAUDE.md

This file provides comprehensive guidance to Claude Code (claude.ai/code) when working with ZMK firmware configuration in this repository.

## Repository Overview

This is a ZMK (Zephyr Mechanical Keyboard) configuration repository for the "roBa" split keyboard, a custom ergonomic 38-key split keyboard with rotary encoders and trackball functionality. The keyboard uses Seeed Studio XIAO BLE controllers and integrates pointing device support through PAW3222/PMW3610 sensor drivers.

ZMK is a modern, open-source keyboard firmware built on the Zephyr™ Project RTOS, designed for power efficiency, wireless connectivity, and flexible configuration through Devicetree and Kconfig systems.

## ZMK Firmware Overview

**Official Repository**: https://github.com/zmkfirmware/zmk  
**Documentation**: https://zmk.dev/  
**License**: MIT License (less restrictive than QMK's GPL)  
**Foundation**: Built on Zephyr™ Project RTOS for robust hardware support  
**Focus**: Wireless-first design with Bluetooth Low Energy native support

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
- **Trackball**: PMW3610 sensor on right half (SPI)
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
- **Layer 4**: Auto mouse layer (activated by trackball movement via PMW3610 driver)
- **Layer 5**: Scroll layer (activated by &mo 5 from auto mouse layer)
- **Layer 6**: Bluetooth management layer

### Trackball Implementation
Current configuration uses PMW3610 driver-specific auto mouse functionality:

```dts
// In roBa_R.overlay - Hardware configuration
&spi0 {
    trackball: trackball@0 {
        status = "okay";
        compatible = "pixart,pmw3610";
        reg = <0>;
        spi-max-frequency = <2000000>;
        irq-gpios = <&gpio0 2 (GPIO_ACTIVE_LOW | GPIO_PULL_UP)>;
    };
};

/ {
    trackball_listener: trackball_listener {
        status = "okay";
        compatible = "zmk,input-listener";
        device = <&trackball>;
    };
};

// In roBa.keymap - Auto mouse layer configuration
&trackball {
    automouse-layer = <4>;  // Layer 4 auto-activated by trackball movement
    scroll-layers = <5>;    // Layer 5 for scroll functionality
};
```

### Build System
- **Automated Builds**: GitHub Actions triggered on push
- **Build Targets**: `roBa_L`, `roBa_R`, `settings_reset`
- **Dependencies**: PMW3610 driver from kumamuk-git/zmk-pmw3610-driver

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
- PMW3610 Driver: https://github.com/kumamuk-git/zmk-pmw3610-driver

### Hardware References  
- Seeed Studio XIAO BLE: nRF52840 MCU
- PMW3610: Optical tracking sensor
- Standard GPIO matrix scanning for switches

## ZMK Repository Structure and Implementation

### Main Directory Structure
```
app/                         # ZMK application core
├── boards/                  # Hardware definitions
│   ├── arm/                 # ARM architecture boards (MCU-integrated)
│   └── shields/             # Shield definitions (keyboard PCBs)
├── dts/                     # Devicetree source files
├── include/                 # Header files
├── module/                  # ZMK modules
├── src/                     # Core C source files
│   ├── behaviors/           # Behavior implementations
│   ├── events/              # Event processing
│   └── split/               # Split keyboard functionality
docs/                        # Documentation
```

### Core Implementation Details

#### Kscan System
- **Purpose**: Physical key input detection and debouncing
- **Configuration**: `CONFIG_ZMK_KSCAN=y` enables kscan integration
- **Debouncing**: Configurable via `CONFIG_ZMK_KSCAN_DEBOUNCE_PRESS_MS` and `CONFIG_ZMK_KSCAN_DEBOUNCE_RELEASE_MS`
- **Driver Types**: Direct GPIO, Matrix, Demux, Composite, Mock (testing)
- **Event Generation**: Creates `zmk_position_state_changed` events

#### Event System Architecture
ZMK uses event-driven architecture for loose coupling between components:

**Key Event Types**:
- `position_state_changed`: Key press/release with position and timestamp
- `keycode_state_changed`: Keycode events with modifiers
- `modifiers_state_changed`: Modifier state changes
- `layer_state_changed`: Layer activation/deactivation
- `sensor_event`: Encoder and sensor data
- `endpoint_changed`: Connection state changes

**Event Management**:
- Declared with `ZMK_EVENT_DECLARE` macro
- Listeners registered with `ZMK_LISTENER`
- Subscriptions via `ZMK_SUBSCRIPTION`
- Return values: `ZMK_EV_EVENT_BUBBLE`, `ZMK_EV_EVENT_HANDLED`, `ZMK_EV_EVENT_CAPTURED`

#### HID Report Generation
- **Report Types**: Keyboard (NKRO/HKRO), Consumer (media keys), Mouse (pointing devices)
- **Configuration Options**: 
  - `CONFIG_ZMK_HID_REPORT_TYPE_NKRO`: N-Key Rollover support
  - `CONFIG_ZMK_HID_CONSUMER_REPORT_USAGES_FULL`: Extended consumer keys
- **Processing**: HID listener processes key events and calls appropriate HID functions
- **Transmission**: Reports sent via `zmk_endpoints_send_report()`

#### Power Management
- **Idle Control**: `CONFIG_ZMK_IDLE_TIMEOUT` for idle state timing
- **Sleep Support**: `CONFIG_ZMK_SLEEP` enables deep sleep functionality
- **Sleep Timeout**: `CONFIG_ZMK_IDLE_SLEEP_TIMEOUT` for deep sleep timing
- **Soft Off**: `CONFIG_ZMK_PM_SOFT_OFF` for keymap-triggered power off
- **Battery Reporting**: `CONFIG_ZMK_BATTERY_REPORTING` for BLE battery status

#### Advanced Behavior System

##### Macro System
- **Definition**: `ZMK_MACRO` macro for custom action sequences
- **Timing Control**: 
  - `wait-ms`: Delay between actions (default 15ms)
  - `tap-ms`: Hold duration for tap actions (default 30ms)
- **Control Actions**: `&macro_tap`, `&macro_press`, `&macro_release`, `&macro_pause_for_release`
- **Dynamic Timing**: `&macro_wait_time`, `&macro_tap_time` for runtime adjustment
- **Parameterization**: Support for 1-2 parameter macros for modularity

##### Combo System
- **Configuration**: Defined in `combos` node with `key-positions`, `bindings`, `timeout-ms`
- **Structure**: Managed by `struct combo_cfg` 
- **Timing Options**: `require_prior_idle_ms` for preventing accidental triggers
- **Release Behavior**: `slow_release` property controls when combo releases
- **Limits**: Configurable via `CONFIG_ZMK_COMBO_MAX_*` options

##### Hold-Tap Behaviors
- **Flavors**:
  - `hold-preferred`: Triggers hold on other key press or timer
  - `balanced`: Similar to hold-preferred with release cancellation
  - `tap-preferred`: Only triggers hold on timer expiry
  - `tap-unless-interrupted`: Triggers hold only on interruption
- **Timing**: `tapping-term-ms` for hold threshold
- **Quick Tap**: `quick-tap-ms` for repeat tap behavior

##### Layer Management
- **Core Behaviors**: 
  - `&mo`: Momentary layer activation
  - `&lt`: Layer-tap (hold for layer, tap for key)
  - `&to`: Switch to layer exclusively
  - `&tog`: Toggle layer on/off
  - `&sl`: Sticky layer (one-shot)
- **Reordering**: `CONFIG_ZMK_KEYMAP_LAYER_REORDERING` enables dynamic layer priority

##### RGB/LED System
- **RGB Underglow**: `CONFIG_ZMK_RGB_UNDERGLOW` with `&rgb_ug` behavior
- **Configuration Options**:
  - `ZMK_RGB_UNDERGLOW_EXT_POWER`: External power control
  - `ZMK_RGB_UNDERGLOW_BRT_MIN/MAX`: Brightness limits
  - `ZMK_RGB_UNDERGLOW_HUE/SAT_STEP`: Color adjustment steps
- **Split Synchronization**: Global execution across split halves
- **Backlight**: `&bl` behavior for key backlighting

##### Encoder Support
- **Hardware**: EC11 rotary encoder support via `CONFIG_EC11=y`
- **Threading**: `CONFIG_EC11_TRIGGER_GLOBAL_THREAD=y` for proper operation
- **Configuration**:
  - Push button: Part of keyboard matrix
  - Rotation: Sensor with `sensor-bindings` for CW/CCW actions
- **Behavior Evolution**: From `&inc_dec_kp` limitation to arbitrary behavior support

### Build System and Development

#### Build Process
- **Foundation**: Uses Zephyr's West build tool
- **GitHub Actions**: Automated firmware building
- **Configuration**: `zmk-config` repository pattern with `.conf` and `.overlay` files
- **Split Builds**: Separate firmware for each split half

#### Development Tools
- **ZMK Tools**: VS Code extension for configuration editing
- **Keymap Editor**: Web-based graphical editor
- **ZMK Locale Generator**: International keyboard layout support
- **Debugging**: USB logging via `CONFIG_ZMK_USB_LOGGING`

#### Hardware Support Matrix
- **MCU Support**: 32/64-bit microcontrollers only (no AVR support)
- **Wireless**: Native BLE 4.2+ with secure connections
- **Sensors**: Encoders, pointing devices, environmental sensors
- **Displays**: LVGL-based display system
- **Power Features**: Low-power design optimized for battery operation

## ZMK vs QMK Comparison

| Aspect | ZMK | QMK |
|--------|-----|-----|
| **Foundation** | Zephyr RTOS | Custom framework |
| **License** | MIT | GPL |
| **Wireless** | Native BLE support | Limited/aftermarket |
| **Architecture** | 32/64-bit only | AVR + ARM support |
| **Power Management** | Built-in low-power design | Retrofitted power features |
| **Configuration** | Devicetree + Kconfig | C headers + make |
| **Behavior System** | Object-oriented behaviors | Function-based behaviors |
| **Split Communication** | BLE-native | Various protocols |
| **Development Model** | GitHub Actions builds | Local compilation |

This comprehensive knowledge base should guide all future development and troubleshooting for this ZMK configuration repository.