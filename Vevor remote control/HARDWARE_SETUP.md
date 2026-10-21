# Hardware Setup

> ## ⚠️ Disclaimer: test setup, use at your own risk
>
> This hardware description documents a **breadboard / prototype test setup**. It is **not a finished or certified product** and it **lacks many safety features** that a device controlling a fuel-burning heater should have. See [Safety and limitations](#safety-and-limitations) before building anything. You are solely responsible for what you build, connect and run.

This is the wiring for the current firmware, `heater_ui_v2_web` (OLED + rotary encoder UI, DS18B20 room sensor, WiFi web UI / REST API / OTA, and serial link to the heater). The heater-side connection is covered in more detail in [HARDWARE_SERIAL.md](HARDWARE_SERIAL.md).

**Board:** ESP32 DevKit v1 (`esp32dev`), Arduino core via `espressif32@3.5.0`

---

## Parts

| Part | Notes |
|---|---|
| ESP32 DevKit v1 | Needs GPIO16/17 (UART2) |
| SH1106 1.3" 128x64 OLED (I2C, 0x3C) | 3.3V |
| Rotary encoder with push button | Internal pull-ups used |
| DS18B20 temperature sensor | 3.3V, with external 4.7 kΩ pull-up |
| Status LED + 220-470 Ω resistor | |
| Bidirectional 3.3V ↔ 5V level shifter | Required, the heater data line is ~5V |
| 470 Ω resistor | In series with ESP32 TX |
| 3-wire cable to the heater | Black = GND, Red = +5V, Blue = data |

Full BOM with optional power-protection parts: [HARDWARE_SERIAL.md](HARDWARE_SERIAL.md#bill-of-materials).

---

## Pin assignment

| Function | ESP32 pin | Notes |
|---|---|---|
| OLED SDA | GPIO22 | **Swapped** vs. the usual ESP32 default (21 = SDA) |
| OLED SCL | GPIO21 | **Swapped** vs. the usual ESP32 default (22 = SCL) |
| Encoder CLK (A) | GPIO33 | Internal pull-up, interrupt driven |
| Encoder DT (B) | GPIO26 | Internal pull-up, interrupt driven |
| Encoder SW (button) | GPIO32 | Internal pull-up |
| DS18B20 data | GPIO19 | External 4.7 kΩ pull-up to 3.3V recommended |
| Status LED | GPIO18 | Via 220-470 Ω resistor to GND |
| Heater serial RX | GPIO16 | UART2 (`Serial2`), via level shifter |
| Heater serial TX | GPIO17 | UART2 (`Serial2`), via 470 Ω then level shifter |

These match the `#define`s in [src/heater_ui_v2_web.cpp](src/heater_ui_v2_web.cpp).

Unused pins: GPIO2, 4, 5, 13, 14, 15, 23, 25, 27, 34, 35.

---

## Wiring overview

```
                         ┌────────────────────┐
  OLED SH1106 ───────────┤ GPIO22 SDA         │
  (I2C 0x3C)             │ GPIO21 SCL         │
                         │                    │
  Encoder CLK/DT/SW ─────┤ GPIO33 / 26 / 32   │
                         │                    │
  DS18B20 (+4.7k) ───────┤ GPIO19             │
                         │         ESP32      │
  Status LED (+R) ───────┤ GPIO18             │
                         │                    │
                         │ GPIO16 (RX) ───────┼──┬──► Level shifter LV1 ─► HV1 ─► Heater BLUE (data)
                         │ GPIO17 (TX) ─[470Ω]┼──┘
                         │                    │
                         │ 3.3V ──────────────┼──► Level shifter LV
                         │ GND  ──────────────┼──► Level shifter GND ─► Heater BLACK (GND)
                         │                    │    Level shifter HV  ◄─ Heater RED (+5V)
                         └────────────────────┘
```

RX and TX share one level-shifter channel (half-duplex). TX goes through the 470 Ω resistor so the ESP32 and the heater's own controller don't fight each other when both drive the line. A common ground between the ESP32 and the heater is required.

Per-part wiring:

```
OLED          ESP32          Encoder       ESP32          DS18B20       ESP32
VCC      →    3.3V           +        →    3.3V           VDD (red) →   3.3V
GND      →    GND            GND      →    GND            DQ  (yel) →   GPIO19 (+4.7k to 3.3V)
SDA      →    GPIO22         CLK      →    GPIO33         GND (blk) →   GND
SCL      →    GPIO21         DT       →    GPIO26
                             SW       →    GPIO32
```

### Heater serial link

- Baud 25000, 8N1, half-duplex on the blue wire
- Signal inversion is handled in the VevorHeater library, see [HARDWARE_SERIAL.md](HARDWARE_SERIAL.md#signal-inversion-configuration)
- Protocol: [SERIAL_PROTOCOL.md](SERIAL_PROTOCOL.md) and [docs/PROTOCOL.md](docs/PROTOCOL.md)

### Power

Two ways have been used, neither is "production grade":

- **USB** to the ESP32 during development (ESP32 and heater share only GND and the data line)
- **Heater 5V (red wire)** to power the ESP32 via VIN. Check the available current and ripple first, see [Power supply options](HARDWARE_SERIAL.md#power-supply-options)

Typical draw is ~150-250 mA depending on WiFi activity and display.

---

## Firmware behavior (hardware-facing)

- **Long press (1.5 s)**: heater on / off (off enters a cooldown state)
- **Short press (heater running)**: enter / leave config mode (diagnostics: voltage, inner temp, fan, pump)
- **Rotate**: adjust target temperature, or cycle diagnostics in config mode
- **LED**: off = heater off, solid = running or cooling down, fast blink (200 ms) = error
- **Web / API**: HTTP on port 80 with PIN authentication, see [API_DOCUMENTATION.md](API_DOCUMENTATION.md)
- **WiFi**: configured over the USB serial console (`wifi ssid <name>`, `wifi pass <password>`, `wifi connect`). Falls back to an access point if the connection fails
- **OTA**: ArduinoOTA, hostname `vevor-heater`

Build and flash: `pio run -e heater_ui_v2_web -t upload` (USB), or over the network with the OTA settings in [platformio.ini](platformio.ini). **Edit `upload_port`, `monitor_port` and the OTA password there for your own setup** before using them.

---

## Bring-up checklist

1. Wire the OLED, encoder, DS18B20 and LED first. **Leave the heater disconnected.**
2. Verify with the test programs in `test/`:
   - `pio run -e i2c_test -t upload`: finds the OLED at 0x3C
   - `pio run -e gpio_input_test -t upload`: shows encoder CLK/DT/SW state
   - `pio run -e temp_sensor_scan -t upload`: finds the DS18B20
3. Wire the level shifter and 470 Ω resistor, still without the heater. Check voltages with a multimeter.
4. Test receive-only against the heater (sniffer firmware: [Vevor-Serial-Sniffer](../Vevor-Serial-Sniffer/)) before ever transmitting
5. Only then flash `heater_ui_v2_web` and test control, with the heater supervised, outdoors or in a safe, ventilated place

---

## Troubleshooting

| Symptom | Check |
|---|---|
| OLED blank | SDA/SCL are swapped from the default (SDA=22, SCL=21); 3.3V on VCC; address 0x3C (`i2c_test`) |
| Encoder erratic / unresponsive | `gpio_input_test`, `gpio_diagnostic`; loose `+` wire is a common cause |
| "No sensor" | `temp_sensor_scan`; 4.7 kΩ pull-up GPIO19 → 3.3V; sensor on 3.3V, **not 5V** |
| LED never lights | Polarity, series resistor, GPIO18 |
| No data from heater | Common ground, level shifter orientation (LV/HV), blue wire, [HARDWARE_SERIAL.md troubleshooting](HARDWARE_SERIAL.md#troubleshooting) |
| Heater ignores commands | TX path, 470 Ω resistor, signal inversion |
| ESP32 resets | Power supply sag, especially when WiFi transmits |

---

## Safety and limitations

This setup is a **test / hobby prototype**. In particular:

- **It is not a safety device.** It only talks to the heater's serial control line. Any protection (overheat, flame-out, fuel, voltage, fault handling) is whatever the heater's own controller provides. This project adds **no independent safety layer**: no separate over-temperature cutoff, no flame or CO monitoring, no hardware watchdog that shuts the heater off if the ESP32 hangs or loses WiFi.
- **Breadboard wiring.** Jumper wires and breadboards can work loose, short or disconnect. There is no strain relief, no fuse by default, no reverse-polarity or over-voltage protection unless you add the optional parts, and no galvanic isolation.
- **Not in an enclosure.** [ENCLOSURE_DESIGN.md](ENCLOSURE_DESIGN.md) is a design idea, not a tested or rated housing. Do not leave electronics exposed near the heater, fuel or exhaust.
- **Weak network security.** The web UI is plain HTTP with a 4-digit PIN (default `1234`) and the OTA password defaults to the same. Anyone on your network who guesses or sniffs it can start the heater or flash the device. Change the PIN and OTA password, and **never expose the device to the internet**.
- **Reverse-engineered protocol.** The heater protocol was worked out by sniffing, not from a manufacturer specification. Behavior may differ between heater models and firmware versions. Wrong commands could cause unexpected behavior.
- **Fire and CO risk.** Diesel heaters involve fuel, high temperatures and exhaust gases (carbon monoxide). Never run one unattended in a closed or poorly ventilated space, and install a CO alarm.
- **Mains.** If you add any current sensing or mains-connected parts, use proper isolation, never work on live circuits and follow local electrical rules, ideally with a qualified electrician.
- **Warranty.** Connecting to the heater may void its warranty.

**Do not rely on this project to keep anyone or anything safe.** Always keep the heater's original controller or switch available as a way to turn it off.

---

## Hardware versions

| Version | Hardware |
|---|---|
| v2.0 | OLED, encoder, DS18B20, status LED (no heater link) |
| v2.0 web (current) | v2.0 + serial link to heater (level shifter, 470 Ω), WiFi, web UI, REST API, OTA |

---

**Last updated**: 2026-10-21
