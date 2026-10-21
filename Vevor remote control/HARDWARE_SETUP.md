# Vevor Heater Controller v2.0 - Hardware Setup

## ESP32 Pin Assignment

### OLED Display (I2C)
- **GPIO22**: SDA (I2C data)
- **GPIO21**: SCL (I2C clock)
- **Display**: SH1106 1.3" 128x64 OLED
- **Address**: 0x3C
- **Note**: SDA/SCL pins are swapped from typical ESP32 default

### Rotary Encoder with Button
- **GPIO33**: CLK (encoder clock/A pin)
- **GPIO26**: DT (encoder data/B pin)
- **GPIO32**: SW (encoder switch/button)
- **Configuration**: Internal pull-ups enabled, interrupt-driven
- **Sensitivity**: 2 steps per detent for responsive feel

### Temperature Sensor (OneWire)
- **GPIO19**: DS18B20 OneWire bus
- **Configuration**: Internal pull-up enabled (45kΩ)
- **Resolution**: 12-bit (0.0625°C)
- **Update rate**: Every 2 seconds
- **Note**: External 4.7kΩ pull-up recommended for reliable operation

### Status LED
- **GPIO18**: Status indicator LED
- **Behavior**:
  - OFF: LED off (heater off)
  - RUNNING: Solid LED (heater running)
  - COOLING: Solid LED (heater cooling down)
  - ERROR: Fast blink at 200ms interval

### Serial Communication with Heater (Planned)
- **GPIO16**: RX (receive from heater)
- **GPIO17**: TX (transmit to heater)
- **Baud rate**: 25000
- **Format**: 8N1
- **Hardware**: Bidirectional level shifter (3.3V ↔ 5V) + 470Ω resistor on TX
- **Status**: Not yet implemented in v2.0 (TODO markers in code)

### Available Unused Pins
- GPIO2, GPIO4, GPIO5, GPIO13, GPIO14, GPIO15, GPIO23, GPIO25, GPIO27, GPIO34, GPIO35
- Can be used for future expansion (current sensor, additional sensors, etc.)

---

## I2C Devices

### SH1106 OLED Display (1.3" 128x64)
- **Address**: 0x3C
- **Driver**: Adafruit SH110X library
- **Voltage**: 3.3V
- **Pins**:
  - VCC/+ → 3.3V
  - GND → GND
  - SDA → GPIO22 (SWAPPED from typical pinout!)
  - SCL → GPIO21 (SWAPPED from typical pinout!)
- **Update rate**: 500ms (2 Hz)

**Important**: This project uses GPIO22 for SDA and GPIO21 for SCL, which is swapped from the typical ESP32 default (usually GPIO21=SDA, GPIO22=SCL).

---

## Rotary Encoder Wiring

```
Rotary Encoder    ESP32 Pin    Notes
--------------    ---------    -----
+    (VCC)     →  3.3V         Power
GND            →  GND          Ground
CLK  (A pin)   →  GPIO33       Rotation detection
DT   (B pin)   →  GPIO26       Rotation detection
SW   (Button)  →  GPIO32       Button press
```

**Configuration:**
- Internal pull-up resistors enabled on all three pins
- Interrupt-driven for reliable detection
- No external components required
- 2 steps per detent configured for better sensitivity
- Long press threshold: 1.5 seconds
- Debounce time: 50ms

---

## Power Supply

- **Input**: 5V via USB or VIN pin
- **ESP32 operates at**: 3.3V (internal regulator)
- **Total current draw**: ~150-250mA (depending on WiFi activity and display)
- **LED**: Adds ~20mA when ON

---

## OneWire Temperature Sensor Setup

### DS18B20 Wiring
```
DS18B20       ESP32
--------      -----
VDD (red)  →  3.3V
DTA (yel)  →  GPIO19
GND (blk)  →  GND
```

**Pull-up resistor**:
- Internal pull-up (~45kΩ) enabled in software
- External 4.7kΩ resistor recommended for reliable operation
- Connect between GPIO19 and 3.3V

### Sensor Configuration
- Resolution: 12-bit (0.0625°C precision)
- Read interval: 2 seconds
- Auto-detection on startup
- Displays temperature without decimal point in UI

---

## Complete System Block Diagram

```
                     ┌─────────────────┐
                     │   ESP32 Module  │
                     │                 │
DS18B20 (GPIO19) ────┤ GPIO22/21       │ ←─── I2C: OLED Display (0x3C, SH1106)
                     │                 │
LED (GPIO18) ────────┤ GPIO33/26/32    │ ←─── Rotary Encoder + Button
                     │                 │
                     │ GPIO16/17       │ ←─── Serial to Heater (TODO)
                     │                 │
WiFi Antenna ────────┤ Built-in        │
                     │                 │
USB ─────────────────┤ Power + Serial  │
                     └─────────────────┘
```

---

## Firmware Features

### v2.0 Features:
- ✅ SH1106 OLED display (1.3" 128x64) with two-row layout
- ✅ Rotary encoder with push button (interrupt-driven)
- ✅ DS18B20 temperature sensor on GPIO19
- ✅ Long press (1.5s) for heater ON/OFF
- ✅ Short press for config/diagnostic mode
- ✅ Status LED with multiple behaviors (off/solid/blink)
- ✅ Temperature display without decimals
- ✅ COOLING mode with automatic timeout
- ✅ Scrolling error message support
- ✅ Current/Target temperature labels
- ✅ Four operating modes: OFF, RUNNING, COOLING, CONFIG
- ⏳ Serial protocol communication (TODO - marked in code)

### Control Modes:
- **OFF Mode**: Shows current temperature and "OFF" text
- **RUNNING Mode**: Shows heater status, power level, current and target temperatures
- **COOLING Mode**: Shows "COOL" text during 10-second cooldown
- **CONFIG Mode**: Rotate through diagnostics (voltage, inner temp, fan speed, pump speed)

### User Interface:
- **Long Press (1.5s)**: Toggle heater ON/OFF
- **Short Press**: Enter/exit CONFIG mode (only when running)
- **Rotate**: Adjust target temperature (OFF/RUNNING) or cycle diagnostics (CONFIG)
- **Display**: Two-row layout with small font (row 1) and large font (row 2)

---

## Test Programs

Located in `test/` directory:

### 1. GPIO Input Test (`gpio_input_test`)
- Displays real-time state of GPIO 32, 33, 26
- Shows 0 for LOW, 1 for HIGH
- Useful for verifying encoder connections

**Upload**: `pio run -e gpio_input_test -t upload`

### 2. GPIO Diagnostic (`gpio_diagnostic`)
- Detailed monitoring with timing information
- Detects pin interference and crosstalk
- Logs all state changes with timestamps
- Identifies simultaneous changes indicating hardware issues

**Upload**: `pio run -e gpio_diagnostic -t upload`

### 3. Temperature Sensor Scanner (`temp_sensor_scan`)
- Scans GPIO 18 and 19 for DS18B20 sensors
- Shows sensor address and current temperature
- Useful for verifying sensor wiring

**Upload**: `pio run -e temp_sensor_scan -t upload`

---

## Initial Setup Procedure

1. **Wire all hardware** according to pinout above
   - OLED: SDA=GPIO22, SCL=GPIO21 (note: swapped!)
   - Encoder: CLK=33, DT=26, SW=32
   - Temperature: GPIO19 with pull-up
   - LED: GPIO18

2. **Verify connections** with test programs:
   - Run `gpio_input_test` to check encoder pins
   - Run `temp_sensor_scan` to verify temperature sensor

3. **Upload main firmware**: `pio run -e heater_ui_v2 -t upload`

4. **Open serial monitor** at 115200 baud to see system messages

5. **Test functionality**:
   - Check OLED display shows current temperature
   - Rotate encoder to adjust target temperature
   - Long press (1.5s) to start heater (enters RUNNING mode)
   - Short press to enter CONFIG mode
   - Rotate through diagnostics
   - Short press to exit CONFIG
   - Long press (1.5s) to stop (enters COOLING mode for 10 seconds)

---

## Troubleshooting

### OLED Not Working
- **Check wiring**: SDA→GPIO22, SCL→GPIO21 (SWAPPED from typical!)
- **Verify address**: Should be 0x3C (can run I2C scanner if needed)
- **Check power**: Display should have 3.3V on VCC
- **Serial output**: Look for "OLED initialized" message

### Encoder Not Responding
- **Run `gpio_input_test`**: Verify pins change state when rotated/pressed
- **Check wiring**: CLK→GPIO33, DT→GPIO26, SW→GPIO32
- **Look for interference**: Run `gpio_diagnostic` to check for crosstalk
- **Verify power**: Encoder should have 3.3V on +
- **Common issue**: Loose + wire can cause erratic behavior

### Temperature Sensor Not Found
- **Run `temp_sensor_scan`**: Check which pin has the sensor
- **Verify pull-up**: External 4.7kΩ resistor recommended (GPIO19 to 3.3V)
- **Check power**: Sensor should have 3.3V (NOT 5V)
- **Serial output**: Look for "Temperature sensors found: 1" message
- **Proper wiring**: Red→3.3V, Yellow→GPIO19, Black→GND

### LED Not Working
- **Check pin**: LED should be connected to GPIO18
- **Check polarity**: Anode (long leg) to GPIO18, cathode to GND (with resistor)
- **Test states**: LED off when OFF, solid when RUNNING/COOLING, blink when ERROR
- **Current limiting**: Use 220Ω-470Ω resistor in series

### Display Shows "No sensor"
- Temperature sensor not detected
- Check wiring and pull-up resistor
- Run `temp_sensor_scan` to diagnose

---

## Hardware Versions

### v2.0 (Current)
- SH1106 OLED display (1.3")
- Rotary encoder with button
- DS18B20 temperature sensor
- Status LED
- No serial communication yet

### v3.0 (Planned)
- Add serial communication to heater
- Integration with VevorHeater library
- Real-time heater status monitoring
- Power level adjustment
- Error detection and display

---

## Safety Notes

⚠️ **WARNING**: This device will interface with mains-powered equipment.

- Ensure proper isolation when adding current sensors
- Use optocouplers for AC mains signals
- Never work on live circuits
- Follow local electrical codes
- Consider using a qualified electrician for installation

---

## Repository Structure

```
.
├── src/
│   ├── heater_ui_v2.cpp      # Main UI firmware (v2.0)
│   └── encoder_raw_test.cpp  # Raw encoder test
├── test/
│   ├── gpio_input_test.cpp   # GPIO state display
│   ├── gpio_diagnostic.cpp   # Detailed GPIO monitoring
│   └── temp_sensor_scan.cpp  # Temperature sensor scanner
├── lib/
│   ├── RotaryEncoder/        # Encoder library
│   ├── DisplayFonts/         # Font library for OLED
│   └── VevorHeater/          # Heater control library (planned)
├── platformio.ini            # Build configuration
├── HARDWARE_SETUP.md         # This file
└── OLED_DISPLAY.md           # Display layout guide
```

---

**Firmware Version**: v2.0
**Last Updated**: 2026-04-06
**Board**: ESP32 DevKit v1
**Arduino Core**: espressif32@3.5.0 (Arduino 1.0.6)
