# Vevor Diesel Heater Controller

ESP32-based controller for Vevor diesel heaters with automatic temperature control, WiFi connectivity, and SignalK integration.

## Archive Contents

This archive contains the complete source code for the Vevor Heater Controller project as of April 13, 2026.

**Latest commit:** `67a20b9` - Add pump frequency monitoring and auto ON/OFF temperature control

## Key Features

### Temperature Control
- **Auto ON/OFF Control**: Heater automatically turns off when temperature reaches target+1°C and restarts at target-1°C
- **10-minute minimum cycles**: Prevents excessive wear on ignition system
- **Smart power adjustment**: Automatically adjusts power level (1-10) based on temperature error
- Temperature smoothing and trend tracking

### Hardware Interface
- **OLED Display** (128x64, SH1106): Shows temperature, target, heater state
- **Rotary Encoder**: Adjust target temperature, navigate diagnostics
- **DS18B20 Temperature Sensor**: Cabin temperature monitoring
- **Serial Communication**: Direct control of Vevor heater via RS485

### WiFi & API
- **REST API**: Full control and monitoring via HTTP
- **WebSocket**: Real-time status updates
- **Token Authentication**: Secure remote control
- **WiFi Manager**: Easy credential configuration

### SignalK Integration
- Includes complete SignalK plugin (`signalk-vevor-heater`)
- Publishes all heater data to SignalK paths
- Polls heater every 2 seconds
- Publishes: temperature, power, voltage, fan speed, pump frequency, error codes

### Monitoring
- Heater voltage
- Heat exchanger temperature
- Fan speed (RPM)
- **Pump frequency (Hz)** - NEW! Direct fuel consumption monitoring
- Error codes with descriptions
- All 9 Vevor heater error codes exposed

## Hardware Requirements

- ESP32 development board
- SH1106 OLED display (128x64, I2C)
- Rotary encoder with button
- DS18B20 temperature sensor
- RS485 to TTL converter (or BSS138 level shifter)
- 10kΩ resistor for level shifting

See `HARDWARE_SETUP.md` for complete wiring diagrams.

## Software Setup

### PlatformIO
```bash
# Extract archive
tar -xzf vevor-heater-controller.tar.gz
cd vevor-heater-controller

# Build
pio run

# Upload
pio run --target upload
```

### SignalK Plugin
```bash
cd "SignalK plugins/signalk-vevor-heater"
npm pack
# Copy .tgz to SignalK server and install
```

## Configuration

### WiFi Setup (via Serial Monitor)
```
wifi ssid <your-network>
wifi pass <your-password>
wifi connect
```

### PIN Authentication
```
pin set 1234
```

### Temperature Control
- **Serial 'a'**: Enable auto control
- **Serial 'd'**: Disable auto control
- **Rotary encoder**: Adjust target temperature
- **Long press (1.5s)**: Turn heater ON/OFF
- **Short click**: Enter diagnostics mode

## Auto Control Parameters

Located in `src/heater_ui_v2_web.cpp`:

```cpp
const float THRESH_AUTO_OFF = 1.0;          // Turn OFF at target + 1°C
const float THRESH_AUTO_RESTART = -1.0;     // Turn ON at target - 1°C
const unsigned long MIN_CYCLE_TIME = 600000; // 10 minutes (600,000ms)
```

Adjust these values to tune the heating cycle behavior.

## API Endpoints

- `GET /api/status` - Full system status
- `POST /api/authenticate` - Get auth token (PIN required)
- `POST /api/start` - Start heater (auth required)
- `POST /api/stop` - Stop heater (auth required)
- `POST /api/target` - Set target temperature (auth required)
- `POST /api/power` - Set power level (auth required)

See `API_DOCUMENTATION.md` for complete API reference.

## Documentation

- `README.md` - Main project documentation
- `HARDWARE_SETUP.md` - Wiring and hardware setup
- `SERIAL_PROTOCOL.md` - Vevor heater communication protocol
- `API_DOCUMENTATION.md` - REST API reference
- `PROJECT_COMPONENTS.md` - Software architecture
- `ENCLOSURE_DESIGN.md` - Physical enclosure plans

## Recent Changes (This Archive)

**Auto ON/OFF Control:**
- Added `UI_STATE_AUTO_OFF` mode
- Automatic cycling with ±1°C hysteresis
- 10-minute minimum ON/OFF times
- Countdown timer on display

**Pump Frequency Monitoring:**
- Added `pumpFrequency` field to HeaterStatus
- Exposed in REST API (`/api/status`)
- Added to OLED diagnostics (4th screen)
- Shows fuel pump frequency in Hz

**Display Updates:**
- "AUTO OFF" screen with countdown
- Pump frequency in diagnostic rotation
- Improved state visualization

## License

This is a personal project. Feel free to use and modify for your own needs.

## Safety Warning

Diesel heaters can be dangerous if not properly installed and maintained. This controller is provided as-is with no warranty. Ensure proper ventilation, fuel connections, and electrical safety before use.

## Support

For questions about this specific build, contact Magnus.

For general Vevor heater protocol information, see `SERIAL_PROTOCOL.md`.

---

**Build Date:** April 13, 2026
**Commit:** 67a20b9
**ESP32 Firmware:** v2.0 with auto ON/OFF control
**SignalK Plugin:** v1.0.3
