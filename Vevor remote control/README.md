# Vevor Diesel Heater ESP32 Control

Complete ESP32-based controller for Vevor diesel heaters with Arduino library.

## Features

- ✅ **Full heater control** - Start, stop, and adjust power (1-10) remotely
- ✅ **Real-time monitoring** - Temperature, fan speed, voltage, and state
- ✅ **Error detection** - Automatic error detection with descriptive messages
- ✅ **Power adjustment while running** - Change power level without restarting
- ✅ **State machine tracking** - OFF, PREHEAT, IGNITION, COMBUSTION, COOLDOWN
- ✅ **Optimized polling** - Smart polling intervals (1s active, 10s idle)
- ✅ **Event callbacks** - State change and error notifications
- ✅ **Clean API** - Easy-to-use Arduino library

## Hardware Requirements

### Components

- ESP32 development board
- 4-channel bidirectional level shifter (3.3V ↔ 5V)
- 470Ω resistor (1/4W)
- Jumper wires

**Total cost: ~$8-10**

### Wiring Diagram

```
ESP32                              Level Shifter           Heater Cable
─────────────────────────────────────────────────────────────────────────

GPIO 16 (RX) ─────────┬──────────→  LV1  ↔  HV1  ────────→  Blue (Data)
                      │
GPIO 17 (TX) ─[470Ω]──┘

3.3V ─────────────────────────────→  LV (VCC)
GND ──────────────────────────────→  LV GND

                                      HV (VCC)  ←───────────  Red (+5V)
                                      HV GND    ←───────────  Black (GND)

GND (ESP32) ──────────────────────────────────────────────→  Black (GND)
```

**Key Points:**
- **RX (GPIO16)**: Connects directly to level shifter LV1 (no resistor)
- **TX (GPIO17)**: Connects through 470Ω resistor to level shifter LV1
- **Both RX and TX** share the same level shifter channel (half-duplex)
- **Level shifter module** provides bidirectional voltage translation (3.3V ↔ 5V)

### Level Shifter Circuit

The heater uses 5V logic while ESP32 uses 3.3V. A 4-channel bidirectional level shifter module (based on BSS138 MOSFETs) provides voltage translation:

```
ESP32 (3.3V Side)         Level Shifter         Heater (5V Side)
─────────────────         ─────────────         ────────────────

GPIO16 (RX) ───────────→ LV1 ↔ HV1 ───────────→ Blue Wire
                          ↕     ↕
GPIO17 (TX) ──[470Ω]────→ LV1   HV1

3.3V ──────────────────→ LV    HV  ←──────────── Red Wire (+5V)
GND ───────────────────→ GND   GND ←──────────── Black Wire (GND)
```

**Why this works:**
- Bidirectional level shifter handles half-duplex serial automatically
- 470Ω resistor limits current on TX line for safe half-duplex operation
- Both directions (3.3V → 5V and 5V → 3.3V) work simultaneously
- Simple, reliable, and proven design

**Why 470Ω on TX?**
- Protects GPIO pins during simultaneous RX/TX on half-duplex bus
- Allows RX to "win" when both are active
- Limits current without affecting signal quality at 4800 baud

### Pin Connections

| Component       | Pin    | Connection                              |
|-----------------|--------|-----------------------------------------|
| ESP32 GPIO16    | RX     | Level shifter LV1 (direct)              |
| ESP32 GPIO17    | TX     | Level shifter LV1 (via 470Ω resistor)   |
| ESP32 3.3V      | Power  | Level shifter LV (VCC)                  |
| ESP32 GND       | Ground | Level shifter LV GND + Heater Black     |
| Heater Blue     | Data   | Level shifter HV1                       |
| Heater Red      | +5V    | Level shifter HV (VCC)                  |
| Heater Black    | Ground | Level shifter HV GND + ESP32 GND        |

## Software Installation

### Using PlatformIO

1. Clone this repository
2. Copy `lib/VevorHeater` to your project's `lib` directory
3. Add to `platformio.ini`:

```ini
[env:esp32dev]
platform = espressif32
board = esp32dev
framework = arduino
lib_deps =
    VevorHeater
```

### Using Arduino IDE

1. Copy `lib/VevorHeater` folder to your Arduino libraries directory
2. Restart Arduino IDE
3. Include the library: `#include <VevorHeater.h>`

## Quick Start

### Basic Example

```cpp
#include <VevorHeater.h>

#define HEATER_RX_PIN 16
#define HEATER_TX_PIN 17

VevorHeater heater(Serial2, HEATER_RX_PIN, HEATER_TX_PIN);

void setup() {
  Serial.begin(115200);
  heater.begin();

  // Start heater at power level 5
  heater.start(5);
}

void loop() {
  heater.update();  // Call frequently!

  // Check status
  if (heater.getState() == STATE_COMBUSTION) {
    Serial.print("Temp: ");
    Serial.print(heater.getTemperature());
    Serial.println("°C");
  }
}
```

### With Callbacks

```cpp
void setup() {
  Serial.begin(115200);
  heater.begin();

  // State change callback
  heater.onStateChange([](HeaterState oldState, HeaterState newState) {
    Serial.print("State: ");
    Serial.print(VevorHeater::getStateString(oldState));
    Serial.print(" -> ");
    Serial.println(VevorHeater::getStateString(newState));
  });

  // Error callback
  heater.onError([](HeaterError error) {
    Serial.print("ERROR: ");
    Serial.println(VevorHeater::getErrorString(error));
  });
}
```

## Library API Reference

### Initialization

```cpp
VevorHeater heater(Serial2, RX_PIN, TX_PIN);
heater.begin();
```

### Control Methods

| Method | Description | Returns |
|--------|-------------|---------|
| `start(powerLevel)` | Start heater at power 1-10 | `bool` |
| `stop()` | Stop heater (initiates cooldown) | `bool` |
| `setPower(powerLevel)` | Change power 1-10 (works while running!) | `bool` |
| `update()` | Update communication (call in loop) | `void` |

### Status Methods

| Method | Description | Returns |
|--------|-------------|---------|
| `getState()` | Get current state | `HeaterState` |
| `getError()` | Get current error code | `HeaterError` |
| `getPower()` | Get current power level | `uint8_t` |
| `getTemperature()` | Get temperature in °C | `float` |
| `getFanSpeed()` | Get fan speed in RPM | `uint16_t` |
| `getVoltage()` | Get voltage in V | `float` |
| `isActive()` | Check if heater is running | `bool` |
| `hasError()` | Check if error present | `bool` |

### Callback Methods

```cpp
void onStateChange(HeaterStateCallback callback);
void onError(HeaterErrorCallback callback);
```

### Static Helper Methods

```cpp
const char* VevorHeater::getStateString(HeaterState state);
const char* VevorHeater::getErrorString(HeaterError error);
```

## Heater States

| State | Value | Description |
|-------|-------|-------------|
| `STATE_OFF` | 0x00 | Heater off, safe to start |
| `STATE_GLOW_PREHEAT` | 0x01 | Glow plug heating (~30s) |
| `STATE_IGNITION` | 0x02 | Attempting ignition (~30-60s) |
| `STATE_COMBUSTION` | 0x03 | Normal burning operation |
| `STATE_COOLDOWN` | 0x04 | Cooling down after stop (~4-5 min) |

### State Transition Flow

```
OFF → GLOW_PREHEAT → IGNITION → COMBUSTION
                                     ↓
                                 COOLDOWN → OFF
```

## Error Codes

| Error | Value | Description | Action |
|-------|-------|-------------|--------|
| `ERROR_NONE` | 0x00 | No error | - |
| `ERROR_VOLTAGE_LOW` | 0x01 | Voltage too low | Check battery |
| `ERROR_VOLTAGE_HIGH` | 0x02 | Voltage too high | Check power supply |
| `ERROR_IGNITION_FAIL` | 0x03 | Ignition failure | Check fuel, glow plug |
| `ERROR_FLAME_OUT` | 0x04 | Flame out during operation | Check fuel supply |
| `ERROR_TEMP_SENSOR` | 0x05 | Temperature sensor error | Check sensor |
| `ERROR_GLOW_PLUG` | 0x06 | Glow plug error | Check glow plug |
| `ERROR_FAN_MOTOR` | 0x08 | Fan motor error | Check fan |
| `ERROR_FUEL_PUMP` | 0x09 | Fuel pump error | Check pump |

## Usage Examples

### Example 1: Simple Start/Stop

```cpp
void loop() {
  heater.update();

  if (digitalRead(START_BUTTON) == LOW) {
    heater.start(5);  // Start at power 5
  }

  if (digitalRead(STOP_BUTTON) == LOW) {
    heater.stop();
  }
}
```

### Example 2: Temperature Control

```cpp
void loop() {
  heater.update();

  float temp = heater.getTemperature();

  if (temp > 200.0 && heater.getPower() > 1) {
    heater.setPower(heater.getPower() - 1);  // Reduce power
  } else if (temp < 150.0 && heater.getPower() < 10) {
    heater.setPower(heater.getPower() + 1);  // Increase power
  }
}
```

### Example 3: WiFi Control

```cpp
#include <WebServer.h>
#include <VevorHeater.h>

WebServer server(80);
VevorHeater heater(Serial2, 16, 17);

void handleStart() {
  int power = server.arg("power").toInt();
  if (power < 1 || power > 10) power = 5;

  heater.start(power);
  server.send(200, "text/plain", "Started");
}

void handleStop() {
  heater.stop();
  server.send(200, "text/plain", "Stopped");
}

void handleStatus() {
  String json = "{";
  json += "\"state\":\"" + String(VevorHeater::getStateString(heater.getState())) + "\",";
  json += "\"temp\":" + String(heater.getTemperature()) + ",";
  json += "\"fan\":" + String(heater.getFanSpeed()) + ",";
  json += "\"power\":" + String(heater.getPower());
  json += "}";

  server.send(200, "application/json", json);
}

void setup() {
  heater.begin();

  WiFi.begin("SSID", "PASSWORD");
  while (WiFi.status() != WL_CONNECTED) delay(500);

  server.on("/start", handleStart);
  server.on("/stop", handleStop);
  server.on("/status", handleStatus);
  server.begin();
}

void loop() {
  heater.update();
  server.handleClient();
}
```

## Protocol Documentation

For detailed protocol information, see [docs/PROTOCOL.md](docs/PROTOCOL.md).

**Key protocol features:**
- 4800 baud, 8N1, half-duplex serial
- 16-byte controller packets
- 57-byte heater response packets
- Simple 8-bit checksum
- Power level in STATUS_REQUEST packet enables runtime power changes

## Troubleshooting

### Heater doesn't start

1. Check level shifter wiring
2. Verify 5V and 3.3V connections
3. Test with original controller first
4. Check serial output for error messages
5. Verify GPIO pins (16=RX, 17=TX)

### Communication errors

1. Check baud rate (must be 4800)
2. Verify level shifter circuit
3. Check for loose connections
4. Test voltage levels with multimeter
5. Ensure 10kΩ pull-up resistor is in place

### Heater stops unexpectedly

1. Check battery voltage (must be 11-15V)
2. Monitor for error codes
3. Ensure fuel supply is adequate
4. Check that `heater.update()` is called frequently

### Power changes don't work

1. Ensure heater is in COMBUSTION state
2. Check that power level is 1-10
3. Wait 1-2 seconds for heater to respond
4. Verify STATUS_REQUEST packets include power level

## Advanced Topics

### Custom Polling Intervals

Modify these constants in VevorHeater.cpp:

```cpp
#define POLL_INTERVAL_ACTIVE_MS 1000   // Poll when active
#define POLL_INTERVAL_IDLE_MS 10000    // Poll when OFF
```

### Custom Serial Port

```cpp
// Use Serial1 instead of Serial2
VevorHeater heater(Serial1, RX_PIN, TX_PIN);
```

### Multiple Heaters

```cpp
VevorHeater heater1(Serial1, 16, 17);
VevorHeater heater2(Serial2, 18, 19);

void loop() {
  heater1.update();
  heater2.update();
}
```

## Project Structure

```
Vevor-remote-control/
├── lib/
│   └── VevorHeater/
│       ├── VevorHeater.h         # Library header
│       └── VevorHeater.cpp       # Library implementation
├── examples/
│   └── SimpleHeaterControl/
│       └── SimpleHeaterControl.ino
├── test/
│   ├── heater_control.cpp        # Test/debug interface
│   └── serial_sniffer.cpp        # Protocol analysis tool
├── docs/
│   └── PROTOCOL.md               # Detailed protocol documentation
└── README.md                     # This file
```

## Contributing

Protocol reverse-engineered through passive sniffing and analysis. Contributions welcome!

**Key discovery:** The STATUS_REQUEST packet (0x02) also sets the power level via byte 8, enabling power changes during operation without restarting.

## License

MIT License - Free to use for personal and commercial projects

## Credits

- Protocol reverse engineering: 2026-04-02
- Hardware: Vevor 5kW Diesel Air Heater
- Microcontroller: ESP32
- Development: PlatformIO

## Safety Warning

⚠️ **IMPORTANT:**
- Diesel heaters produce heat and exhaust fumes
- Ensure proper ventilation
- Install carbon monoxide detector
- Follow manufacturer safety guidelines
- Test thoroughly before unattended operation
- Monitor fuel levels and battery voltage

## Support

For issues, questions, or contributions, please open an issue on GitHub.

---

**Status:** Working and tested
**Last Updated:** 2026-04-02
