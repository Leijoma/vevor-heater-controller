# Vevor Remote Control - Project Components Documentation

**Version:** 4.0
**Date:** 2026-04-05
**Status:** Heater UI v1.0 Complete and Working

---

## Table of Contents

1. [Hardware Configuration](#hardware-configuration)
2. [RotaryEncoder Library](#rotaryencoder-library)
3. [DisplayFonts Library](#displayfonts-library)
4. [VevorHeater Library](#vevorheater-library)
5. [Complete Integration Example](#complete-integration-example)
6. [Heater UI v1.0 - Production Implementation](#heater-ui-v10---production-implementation)

---

## Hardware Configuration

### Verified Working Pinout

```
Component           ESP32 Pin    Notes
---------           ---------    -----
OLED SDA         →  GPIO22       ⚠️ SWAPPED from standard (normally 21)
OLED SCL         →  GPIO21       ⚠️ SWAPPED from standard (normally 22)
OLED VCC         →  3.3V or 5V
OLED GND         →  GND

LED              →  GPIO18       Status indicator

Encoder CLK (A)  →  GPIO33       Rotation detection
Encoder DT  (B)  →  GPIO26       Rotation detection
Encoder SW       →  GPIO32       Button press detection
Encoder GND      →  GND
Encoder VCC      →  3.3V
```

### Critical Notes

⚠️ **I2C Pins are Swapped!**
The OLED SDA/SCL wires are physically swapped. Always use:
```cpp
#define I2C_SDA_PIN 22  // NOT 21!
#define I2C_SCL_PIN 21  // NOT 22!
```

⚠️ **Encoder Pin Mapping**
The encoder pins are NOT in the order marked on the module. Use the verified pins above.

### OLED Display Specifications

- **Model:** SH1106 (1.3" 128x64)
- **I2C Address:** 0x3C
- **Driver:** Adafruit SH110X library
- **Voltage:** 3.3V or 5V (both work)

### Encoder Specifications

- **Type:** Rotary encoder with push button
- **Resolution:** 4 steps per detent (mechanical click)
- **Pull-ups:** Internal (built into ESP32)
- **Interrupt-capable:** All pins support interrupts

---

## RotaryEncoder Library

### Overview

Advanced rotary encoder library with intelligent button handling.

**Location:** `lib/RotaryEncoder/`

### Features

- ✅ Rotation detection (CW/CCW)
- ✅ Short press detection
- ✅ Long press detection (configurable threshold)
- ✅ Double click detection (configurable window)
- ✅ Configurable resolution (1, 2, or 4 steps per detent)
- ✅ Interrupt-driven for reliability
- ✅ Built-in debouncing

### Basic Usage

```cpp
#include <RotaryEncoder.h>

// Initialize (CLK, DT, SW pins, steps per detent)
RotaryEncoder encoder(33, 26, 32, 4);

void setup() {
  encoder.begin();

  // Optional: Configure timing
  encoder.setLongPressThreshold(1000);  // 1 second
  encoder.setDoubleClickWindow(400);     // 400ms
  encoder.setDebounceTime(50);           // 50ms
}

void loop() {
  if (encoder.update()) {

    // Check rotation
    int delta = encoder.getDelta();
    if (delta != 0) {
      Serial.print("Rotated: ");
      Serial.println(delta);
    }

    // Check button
    EncoderButton btn = encoder.getButton();
    if (btn == BTN_SHORT_PRESS) {
      Serial.println("Short press");
    } else if (btn == BTN_LONG_PRESS) {
      Serial.println("Long press");
    } else if (btn == BTN_DOUBLE_CLICK) {
      Serial.println("Double click");
    }
  }
}
```

### API Reference

#### Initialization

```cpp
RotaryEncoder(int clkPin, int dtPin, int swPin, int stepsPerDetent = 4);
void begin();
```

#### Reading State

```cpp
bool update();                    // Call in loop(), returns true if new data
long getPosition();               // Get absolute position
int getDelta();                   // Get change since last read
EncoderDirection getDirection();  // Get last direction (DIR_CW, DIR_CCW, DIR_NONE)
EncoderButton getButton();        // Get button event (BTN_NONE, BTN_SHORT_PRESS, etc.)
bool isPressed();                 // Check if button currently held
```

#### Configuration

```cpp
void setPosition(long position);           // Set current position
void setLongPressThreshold(unsigned long ms);  // Default: 500ms
void setDoubleClickWindow(unsigned long ms);   // Default: 400ms
void setDebounceTime(unsigned long ms);        // Default: 50ms
```

#### Button Event Types

```cpp
enum EncoderButton {
  BTN_NONE = 0,
  BTN_SHORT_PRESS,
  BTN_LONG_PRESS,
  BTN_DOUBLE_CLICK
};
```

#### Direction Types

```cpp
enum EncoderDirection {
  DIR_NONE = 0,
  DIR_CW = 1,   // Clockwise
  DIR_CCW = -1  // Counter-clockwise
};
```

### Example: Menu System

```cpp
RotaryEncoder encoder(33, 26, 32, 4);
int menuIndex = 0;
int value = 0;

void loop() {
  if (encoder.update()) {
    int delta = encoder.getDelta();

    EncoderButton btn = encoder.getButton();
    if (btn == BTN_SHORT_PRESS) {
      // Enter menu item
      Serial.print("Selected: ");
      Serial.println(menuIndex);
    } else if (btn == BTN_LONG_PRESS) {
      // Exit to main menu
      Serial.println("Exit to main");
    } else if (btn == BTN_DOUBLE_CLICK) {
      // Quick action
      Serial.println("Quick action");
    }

    // Navigate menu
    if (delta != 0) {
      menuIndex += delta;
      if (menuIndex < 0) menuIndex = 0;
      if (menuIndex > 9) menuIndex = 9;
    }
  }
}
```

---

## DisplayFonts Library

### Overview

High-quality font rendering library with multiple sizes and styles.

**Location:** `lib/DisplayFonts/`

### Features

- ✅ 4 font families (Sans, Sans Bold, Serif, Mono)
- ✅ 4 sizes (9pt, 12pt, 18pt, 24pt)
- ✅ Text alignment (left, center, right)
- ✅ Centered text in box
- ✅ Number formatting with decimals
- ✅ Text measurement utilities

### Basic Usage

```cpp
#include <Wire.h>
#include <Adafruit_SH110X.h>
#include <DisplayFonts.h>

Adafruit_SH1106G display(128, 64, &Wire, -1);
DisplayFonts fonts(&display);

void setup() {
  Wire.begin(22, 21);  // SDA=22, SCL=21 (SWAPPED!)
  display.begin(0x3C, true);
  display.setTextColor(SH110X_WHITE);

  // Use fonts
  fonts.setFont(FONT_LARGE, FONT_SANS_BOLD);
  fonts.printCentered("Hello!", 0, 0, 128, 64);
  display.display();
}
```

### Font Families and Sizes

#### Font Families

```cpp
enum FontStyle {
  FONT_SANS = 0,       // Clean sans-serif
  FONT_SANS_BOLD = 1,  // Bold sans-serif
  FONT_SERIF = 2,      // Classic serif
  FONT_MONO = 3        // Fixed-width monospace
};
```

#### Font Sizes

```cpp
enum FontSize {
  FONT_TINY = 0,    // 9pt  - small labels
  FONT_SMALL = 1,   // 12pt - regular text
  FONT_MEDIUM = 2,  // 18pt - headings
  FONT_LARGE = 3    // 24pt - big numbers
};
```

### API Reference

#### Font Selection

```cpp
void setFont(FontSize size, FontStyle style = FONT_SANS);
void setDefaultFont();  // Reset to classic 5x7 bitmap font
```

#### Text Rendering

```cpp
// Aligned text
void printAligned(const char *text, int16_t x, int16_t y,
                  TextAlign align = ALIGN_LEFT);

// Centered in box
void printCentered(const char *text, int16_t boxX, int16_t boxY,
                   int16_t boxW, int16_t boxH);

// Number with decimals
void printNumber(float value, int16_t x, int16_t y, int decimals = 0);
```

#### Text Measurement

```cpp
void getTextBounds(const char *text, int16_t x, int16_t y,
                   int16_t *w, int16_t *h);
int16_t getTextWidth(const char *text);
int16_t getFontHeight();
```

### Examples

#### Temperature Display

```cpp
void showTemperature(float temp) {
  display.clearDisplay();

  // Title
  fonts.setFont(FONT_SMALL, FONT_SANS);
  fonts.printAligned("Temperature", 64, 15, ALIGN_CENTER);

  // Large value
  fonts.setFont(FONT_LARGE, FONT_SANS_BOLD);
  char buffer[16];
  snprintf(buffer, sizeof(buffer), "%.1f°C", temp);
  fonts.printCentered(buffer, 0, 20, 128, 44);

  display.display();
}
```

#### Status Screen

```cpp
void showStatus() {
  display.clearDisplay();

  fonts.setFont(FONT_SMALL, FONT_SANS);

  // Left-aligned labels
  fonts.printAligned("Mode:", 0, 15, ALIGN_LEFT);
  fonts.printAligned("Status:", 0, 30, ALIGN_LEFT);
  fonts.printAligned("Temp:", 0, 45, ALIGN_LEFT);

  // Right-aligned values
  fonts.printAligned("Auto", 128, 15, ALIGN_RIGHT);
  fonts.printAligned("ON", 128, 30, ALIGN_RIGHT);
  fonts.printAligned("22.5°C", 128, 45, ALIGN_RIGHT);

  display.display();
}
```

#### Multi-line Menu

```cpp
void showMenu(int selected) {
  const char* items[] = {"Settings", "Temperature", "Timer", "About"};

  display.clearDisplay();
  fonts.setFont(FONT_SMALL, FONT_SANS);

  for (int i = 0; i < 4; i++) {
    if (i == selected) {
      fonts.setFont(FONT_SMALL, FONT_SANS_BOLD);  // Bold for selected
    } else {
      fonts.setFont(FONT_SMALL, FONT_SANS);
    }
    fonts.printAligned(items[i], 10, 15 + (i * 15), ALIGN_LEFT);
  }

  display.display();
}
```

---

## VevorHeater Library

### Overview

Communication library for Vevor diesel heaters.

**Location:** `lib/VevorHeater/`

### Features

- ✅ Serial protocol communication (25000 baud, inverted)
- ✅ Power on/off control
- ✅ Temperature adjustment
- ✅ Real-time status monitoring
- ✅ Error detection
- ✅ RF remote emulation (433MHz)

### Basic Usage

```cpp
#include <VevorHeater.h>

VevorHeater heater(16, 17);  // RX=16, TX=17

void setup() {
  heater.begin();
  heater.powerOn();
  heater.setTemperature(22);
}

void loop() {
  heater.update();

  if (heater.isRunning()) {
    float temp = heater.getCurrentTemp();
    Serial.println(temp);
  }
}
```

### API Reference

**Note:** Full API documentation available in `lib/VevorHeater/VevorHeater.h`

#### Control Commands

```cpp
void powerOn();
void powerOff();
void setTemperature(int temp);  // 8-36°C
void setPower(int level);       // 1-6
```

#### Status Reading

```cpp
bool isRunning();
float getCurrentTemp();
int getTargetTemp();
int getPowerLevel();
HeaterState getState();
```

---

## Complete Integration Example

### Heater Controller with OLED UI

```cpp
#include <Arduino.h>
#include <Wire.h>
#include <Adafruit_SH110X.h>
#include <DisplayFonts.h>
#include <RotaryEncoder.h>
#include <VevorHeater.h>

// Hardware pins (VERIFIED WORKING)
#define I2C_SDA_PIN 22  // SWAPPED!
#define I2C_SCL_PIN 21  // SWAPPED!
#define ENCODER_CLK 33
#define ENCODER_DT  26
#define ENCODER_SW  32
#define LED_PIN     18

// Components
Adafruit_SH1106G display(128, 64, &Wire, -1);
DisplayFonts fonts(&display);
RotaryEncoder encoder(ENCODER_CLK, ENCODER_DT, ENCODER_SW, 4);
VevorHeater heater(16, 17);

// State
int targetTemp = 22;
bool heaterOn = false;

void updateDisplay() {
  display.clearDisplay();

  // Title
  fonts.setFont(FONT_SMALL, FONT_SANS);
  fonts.printAligned("Vevor Controller", 64, 10, ALIGN_CENTER);

  // Current temp (large)
  fonts.setFont(FONT_LARGE, FONT_SANS_BOLD);
  char tempStr[16];
  snprintf(tempStr, sizeof(tempStr), "%.1f°C", heater.getCurrentTemp());
  fonts.printCentered(tempStr, 0, 20, 128, 30);

  // Target temp
  fonts.setFont(FONT_SMALL, FONT_SANS);
  snprintf(tempStr, sizeof(tempStr), "Target: %d°C", targetTemp);
  fonts.printAligned(tempStr, 64, 55, ALIGN_CENTER);

  display.display();
}

void setup() {
  // Initialize I2C (SWAPPED PINS!)
  Wire.begin(I2C_SDA_PIN, I2C_SCL_PIN);

  // Initialize display
  display.begin(0x3C, true);
  display.setTextColor(SH110X_WHITE);

  // Initialize encoder
  encoder.begin();
  encoder.setLongPressThreshold(1000);

  // Initialize heater
  heater.begin();

  // LED
  pinMode(LED_PIN, OUTPUT);

  updateDisplay();
}

void loop() {
  heater.update();

  if (encoder.update()) {
    // Adjust temperature with rotation
    int delta = encoder.getDelta();
    if (delta != 0) {
      targetTemp += delta;
      if (targetTemp < 8) targetTemp = 8;
      if (targetTemp > 36) targetTemp = 36;

      if (heaterOn) {
        heater.setTemperature(targetTemp);
      }

      updateDisplay();
    }

    // Button controls
    EncoderButton btn = encoder.getButton();
    if (btn == BTN_SHORT_PRESS) {
      // Toggle power
      heaterOn = !heaterOn;
      if (heaterOn) {
        heater.powerOn();
        heater.setTemperature(targetTemp);
      } else {
        heater.powerOff();
      }
      digitalWrite(LED_PIN, heaterOn ? HIGH : LOW);
      updateDisplay();

    } else if (btn == BTN_LONG_PRESS) {
      // Emergency stop
      heater.powerOff();
      heaterOn = false;
      digitalWrite(LED_PIN, LOW);
      updateDisplay();
    }
  }

  delay(10);
}
```

---

## Heater UI v1.0 - Production Implementation

### Overview

Complete heater controller implementation with OLED display, rotary encoder control, and cooldown phase management.

**Location:** `src/heater_ui_v1.cpp`
**Status:** ✅ Working and Tested
**Date:** 2026-04-05

### Features

- ✅ Main display: Temperature control with current/target display
- ✅ Diagnostics mode: Fan, Internal Temp, Voltage, Power monitoring
- ✅ Long press (1s): Toggle heater ON/OFF with 2-second lockout
- ✅ Double-click: Toggle diagnostics mode
- ✅ Rotate: Adjust target temperature (8-36°C) or cycle diagnostics
- ✅ Cooldown phase: Shows "COOL" for 10 seconds after stop (simulated)
- ✅ LED indicator: ON when running/cooling, OFF when standby
- ✅ Error handling: LED blinks on heater errors
- ✅ Font corruption prevention: Always resets to default font for Row 1

### Hardware Configuration

```cpp
// Display
#define I2C_SDA_PIN 22  // SWAPPED!
#define I2C_SCL_PIN 21  // SWAPPED!

// Encoder
#define ENCODER_CLK 33
#define ENCODER_DT  26
#define ENCODER_SW  32

// LED
#define LED_PIN     18

// Heater serial
#define HEATER_RX   16
#define HEATER_TX   17
```

### Display Modes

#### Main Screen (MODE_MAIN)

**When OFF/Standby:**
- Row 1: `"Standby"` (small default font)
- Row 2: `"OFF"` (large bold font)
- LED: OFF

**When Cooling Down:**
- Row 1: `"Cooldown"` (small default font)
- Row 2: `"COOL"` (large bold font)
- LED: ON
- Duration: 10 seconds (simulated, will use real heater state later)

**When Running:**
- Row 1: Heater state (e.g., "Combustion", "Ignition")
- Row 2: `"22 20"` format (current temperature, space, target temperature)
- LED: ON
- Example: Display shows `"24 22"` = current 24°C, target 22°C

#### Diagnostics Screen (MODE_DIAGNOSTICS)

Access via double-click. Displays 4 different values:

1. **Fan**
   - Row 1: `"Fan"`
   - Row 2: Fan speed in RPM (e.g., `"2450"`)

2. **Internal Temp**
   - Row 1: `"Internal Temp"`
   - Row 2: Temperature as integer (e.g., `"68"`)

3. **Voltage**
   - Row 1: `"Voltage"`
   - Row 2: Battery voltage (e.g., `"12.4V"`)

4. **Power Mode**
   - Row 1: `"Power Mode"`
   - Row 2: Current power level 1-10 (e.g., `"5"`)

Rotate encoder to cycle through these values.

### Controls

| Action | Function | Notes |
|--------|----------|-------|
| **Rotate** | Adjust target temp (8-36°C) | Main mode only |
| **Rotate** | Cycle diagnostics | Diagnostics mode |
| **Long Press (1s)** | Start heater | When OFF |
| **Long Press (1s)** | Stop heater | When ON |
| **Long Press (1s)** | Ignored | During cooldown or 2s lockout |
| **Double-Click** | Enter diagnostics | From main mode |
| **Double-Click** | Exit diagnostics | From diagnostics mode |

### State Machine

```
Standby (OFF)
    │
    │ (long press)
    ↓
Running (ON) ←──┐
    │           │ (rotate: adjust temp)
    │ (long press)
    ↓
Cooldown (COOL)
    │
    │ (10 seconds)
    ↓
Standby (OFF)
```

### Button Lockout System

To prevent flickering and accidental retriggering:

1. **2-Second Lockout After Toggle**
   - After starting or stopping heater, button is locked for 2 seconds
   - Any long press during lockout is ignored with debug message
   - Prevents retriggering while button is still held

2. **Cooldown Lockout**
   - Cannot restart heater during 10-second cooldown phase
   - Long press is ignored with countdown message
   - Ensures proper heater shutdown sequence

### Temperature to Power Mapping

The heater uses power levels (1-10), not temperature directly:

```cpp
int tempToPower(int temp) {
  // Map 8-36°C to power 1-10
  int power = map(temp, 8, 36, 1, 10);
  return constrain(power, 1, 10);
}
```

Examples:
- 8°C  → Power 1 (minimum)
- 22°C → Power 5 (default)
- 36°C → Power 10 (maximum)

### Font Rendering Strategy

**Critical for preventing display corruption:**

```cpp
// Row 1: ALWAYS use default font
display.setFont();  // Reset to default
display.setTextSize(1);
display.setTextColor(SH110X_WHITE);
display.setTextWrap(false);
display.setCursor(0, 0);
display.print("Status text");

// Row 2: Use custom large fonts
fonts.setFont(FONT_LARGE, FONT_SANS_BOLD);
fonts.printCentered("Value", 0, 20, 128, 44);
```

**Why this matters:** The DisplayFonts library changes font settings that persist. Always reset to default font before Row 1 to prevent text overlay corruption.

### Serial Debug Output

The UI provides detailed debug information:

```
========================================
  Vevor Heater Controller v1.0
========================================

✓ LED initialized
✓ OLED initialized
✓ Encoder initialized
✓ Heater initialized

========================================
CONTROLS:
  Rotate:       Adjust temp / Cycle diagnostics
  Long Press:   Start/Stop heater
  Double-Click: Toggle diagnostics screen
========================================

Default target: 20°C

Button event detected: 2
-> Long press: Toggle heater
   Starting heater...
Target temperature: 20°C
Power level: 5

*** STARTING HEATER ***
   Button locked for 2 seconds

State: Combustion | Current: 22.3°C | Target: 20°C | Power: 5

Button event detected: 2
-> Long press: Toggle heater
   Stopping heater...

*** STOPPING HEATER ***
Sending stop command to heater...
Heater entering COOLDOWN phase (10 seconds simulated)

Cooldown: 9 seconds remaining
Cooldown: 8 seconds remaining
...

*** COOLDOWN COMPLETE ***
Heater now fully OFF
```

### Error Handling

1. **Heater Errors**
   - Detected via `heater.hasError()`
   - LED blinks rapidly (200ms on/off)
   - Error message displayed in Row 1
   - Serial output shows error details

2. **Display Errors**
   - Font corruption prevented by always resetting to default before Row 1
   - Single update point with `needsDisplayUpdate` flag

### Cooldown Implementation

**Current (Simulated):**
```cpp
const unsigned long COOLDOWN_DISPLAY_MS = 10000;  // 10 seconds
```

**Future (Real Heater State):**
When integrated with real heater:
```cpp
if (heater.getState() == STATE_COOLDOWN) {
  heaterCooling = true;
} else if (heater.getState() == STATE_OFF) {
  heaterCooling = false;
  setLED(false);
}
```

### Upload and Run

```bash
# Build and upload
pio run -e heater_ui_v1 --target upload

# Monitor serial output
pio device monitor -e heater_ui_v1
```

### Configuration Options

```cpp
// Temperature limits
const int MIN_TEMP = 8;
const int MAX_TEMP = 36;

// Power limits
const int MIN_POWER = 1;
const int MAX_POWER = 10;

// Button timing
const unsigned long BUTTON_LOCKOUT_MS = 2000;  // 2 seconds
encoder.setLongPressThreshold(1000);  // 1 second
encoder.setDoubleClickWindow(400);    // 400ms

// Cooldown
const unsigned long COOLDOWN_DISPLAY_MS = 10000;  // 10 seconds
```

### Known Issues and Solutions

#### Issue: Text Overlaying on Display
**Cause:** DisplayFonts library leaves font settings in wrong state
**Solution:** Always call `display.setFont()` before Row 1 text

#### Issue: Button Flickering After Long Press
**Cause:** Encoder library triggers multiple events while button held
**Solution:** 2-second button lockout after heater toggle

#### Issue: Immediate Restart After Stop
**Cause:** User can immediately restart during heater cooldown
**Solution:** Block long press during 10-second cooldown phase

### Future Enhancements

1. **Real Cooldown Detection**
   - Use `heater.getState()` to detect actual STATE_COOLDOWN
   - Transition to OFF when heater reports STATE_OFF
   - Remove 10-second simulation

2. **Temperature PID Control**
   - Instead of fixed power levels
   - Maintain target temperature automatically

3. **Additional Diagnostics**
   - Pump frequency (when available in VevorHeater API)
   - Fuel consumption estimate
   - Run time counter

4. **Settings Menu**
   - Adjust min/max temperature
   - Configure button timing
   - Calibration options

---

## Testing Programs

All test programs are located in `test/` directory:

### Hardware Tests

- **`i2c_test`** - Scan I2C bus
- **`oled_test`** - Test OLED display (both SSD1306 and SH1106)
- **`pin_test`** - Identify correct I2C pin mapping
- **`encoder_pin_test`** - Identify encoder pin mapping

### Library Tests

- **`ui_test`** - Simple UI test (OLED + LED + Encoder)
- **`rotary_library_test`** - RotaryEncoder library demo
- **`font_demo`** - DisplayFonts library demo (12 screens)

### Upload Commands

```bash
# Test hardware
pio run -e i2c_test --target upload
pio run -e oled_test --target upload

# Test libraries
pio run -e ui_test --target upload
pio run -e rotary_library_test --target upload
pio run -e font_demo --target upload
```

---

## Library Files

### Project Structure

```
lib/
├── RotaryEncoder/
│   ├── RotaryEncoder.h
│   └── RotaryEncoder.cpp
├── DisplayFonts/
│   ├── DisplayFonts.h
│   └── DisplayFonts.cpp
└── VevorHeater/
    ├── VevorHeater.h
    └── VevorHeater.cpp

test/
├── ui_test.cpp
├── rotary_library_test.cpp
├── font_demo.cpp
├── i2c_scanner.cpp
├── oled_test.cpp
└── pin_test.cpp
```

---

## Known Issues & Workarounds

### I2C Pins are Swapped

**Problem:** Physical wiring has SDA/SCL reversed
**Solution:** Always use `SDA=22, SCL=21` in code

### Encoder Pin Order

**Problem:** Encoder pins not in marked order
**Solution:** Use verified mapping: `CLK=33, DT=26, SW=32`

### OLED May Need 5V

**Problem:** Some OLED modules don't work reliably on 3.3V
**Solution:** Try connecting VCC to 5V pin if display is blank

---

## Next Steps

1. ✅ All hardware verified and documented
2. ✅ All libraries tested and working
3. ✅ Heater UI v1.0 complete and tested
4. ⏭️ Integrate real heater state detection (replace 10s cooldown simulation)
5. ⏭️ Add settings menu (temperature limits, button timing)
6. ⏭️ Add web interface
7. ⏭️ Add WiFi configuration
8. ⏭️ Add OLED brightness control

---

**Last Updated:** 2026-04-05
**Version:** 4.0
**Heater UI v1.0:** ✅ Complete and Working
