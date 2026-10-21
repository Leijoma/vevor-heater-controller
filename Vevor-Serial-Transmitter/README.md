# Vevor Heater Serial Transmitter

**Heater Impersonation Mode** - Make the control panel think it's talking to the heater!

## Features

- ✅ **Auto-respond mode** - Automatically respond to controller packets as if you were the heater
- ✅ **Heater impersonation** - Send 56-byte heater status packets
- ✅ **Test controller reactions** - See if control panel display reacts to fake heater data
- ✅ **Bidirectional communication** - Listen and respond on same wire
- ✅ Turn heater ON/OFF (if real heater present)
- ✅ Set power level (1-10)
- ✅ Automatic checksum calculation
- ✅ Signal inversion (RX confirmed working)

## Hardware Setup

### Wiring

```
ESP32                    Level Shifter         Heater Cable
─────────────────────────────────────────────────────────
3.3V    →  LV (VCC)
GND     →  GND (both sides)  →  GND        →   Black (GND)
GPIO17  →  LV side
           HV side          →  Data out    →   Blue (Serial)
           HV (VCC)         ←  +5V         ←   Red (+5V)
GPIO16  ←  LV side (optional for monitoring)
```

**CRITICAL:**
- **Use bidirectional level shifter** (3.3V ↔ 5V)
- **Ground is mandatory** - ESP32 GND must connect to Black wire
- **GPIO17 = TX** (sends commands to heater)
- **GPIO16 = RX** (optional, for monitoring heater responses)

## Serial Commands

### Primary Commands (Heater Impersonation)

| Key | Action |
|-----|--------|
| `a` | Toggle auto-respond mode (ON by default) |
| `r` | Manually send heater response packet |
| `s` | Show current status |
| `h` | Show help menu |

### Optional Commands (Send to Real Heater)

| Key | Action |
|-----|--------|
| `1` | Turn heater ON (starts at power level 10) |
| `0` | Turn heater OFF |
| `+` | Increase power level (+1) |
| `-` | Decrease power level (-1) |
| `2-9` | Set specific power level |

## Usage

### 1. Build and Upload

```bash
cd "/Users/magnus/Documents/PlatformIO/Projects/Vevor-Serial-Transmitter"
pio run --target upload
```

### 2. Open Serial Monitor

```bash
pio device monitor --baud 115200
```

### 3. Send Commands

Press keys to control the heater:
- Press `1` to turn ON
- Press `+` or `-` to adjust power
- Press `0` to turn OFF

### Example Session (Heater Impersonation Mode)

```
╔══════════════════════════════════════════════════════╗
║     Vevor Heater Serial Transmitter                 ║
║          (Heater Impersonation Mode)                 ║
╚══════════════════════════════════════════════════════╝

📡 MODE: Auto-respond to controller packets

Ready!

[RECEIVED] AA 66 06 0B 00 00 00 00 0A 06 00 00 00 00 00 21
  ┌─ Controller Command:
  │ Command: 0x06
  │ Power: 10/10
  │ State: 0x06
  │ Checksum: 0x21
  └─
  → Auto-responding as HEATER...

[HEATER RESPONSE] AA 77 06 33 00 06 0A 00 00 00 00 84 00 19 00 00
                  00 00 00 00 00 00 00 0A 00 00 00 00 09 C4 00 00
                  00 00 00 00 00 00 00 00 00 00 00 00 00 00 00 00
                  00 00 00 00 00 00 00 00
  From: HEATER (0x77)
  State: 0x06
  Power: 10/10
  Checksum: 0xE4

> s

=== CURRENT STATUS ===
Heater: OFF
Power level: 10/10
State: 0x06
Auto-respond: ENABLED
======================

> a
Auto-respond mode: DISABLED

(Now controller packets won't get responses - watch for errors on display!)
```

### What You Should See

**With auto-respond ENABLED:**
1. Controller sends packets every ~1 second
2. ESP32 automatically responds as heater
3. **Control panel display should show:**
   - Temperature readings (simulated 45°C)
   - Fan speed (simulated 2500 RPM)
   - Power level
   - Voltage (simulated 13.2V)
   - NO communication errors!

**With auto-respond DISABLED:**
4. Controller sends packets but gets no response
5. **Control panel display should show:**
   - Communication error
   - Timeout message
   - Maybe error code or flashing display

This proves you can control what the panel shows!

## Troubleshooting

### Controller display doesn't react to responses?

The protocol uses **inverted UART signals**. RX inversion is enabled, but TX inversion is not available in this ESP32 Arduino core version.

**Options:**

1. **Try it first!** The level shifter might handle inversion automatically
2. **Hardware inverter:** Add a simple NPN transistor inverter on TX line:
   ```
   GPIO17 → 10kΩ → Base (NPN transistor)
   Collector → Level shifter (pulled up to 3.3V via 10kΩ)
   Emitter → GND
   ```
3. **Upgrade ESP32 core:** Use newer platformio.ini platform version with `setTxInvert()` support
4. **Use different level shifter:** Some automatically invert signals

### How do I know if it's working?

**Without heater connected:**
- You'll see `[SENT]` messages showing packets being transmitted
- No response is expected (heater not connected)

**With heater connected:**
- If heater responds to commands (turns on/off, changes power), it's working!
- You can also run the **Vevor-Serial-Sniffer** on another ESP32 to monitor the bus
- If RX (GPIO16) is connected, you might see `[HEATER]` responses in serial monitor

### Still not working?

1. **Check wiring:**
   - GND connected to Black wire? (CRITICAL)
   - GPIO17 connected through level shifter to Blue wire?
   - Level shifter powered correctly (3.3V on LV, 5V on HV)?

2. **Test with sniffer:**
   - Run Vevor-Serial-Sniffer on another ESP32
   - Send commands from transmitter
   - Verify packets appear correctly on sniffer

3. **Try signal inversion:**
   - Uncomment `Serial2.setTxInvert(true);` in setup()

## Safety Notes

- **Test without fuel first** - make sure you can turn heater off!
- **Have CO detector** - diesel heaters produce carbon monoxide
- **Proper ventilation** required for safe operation
- **Monitor temperature** - ensure heater doesn't overheat
- **Emergency shutoff** - know how to manually shut down heater

## Next Steps

Once transmitter works:

1. **Test basic control** - verify heater responds to ON/OFF
2. **Test power levels** - verify heater adjusts heat output
3. **Monitor with sniffer** - capture heater response packets
4. **Integrate into main controller** - add to [Vevor remote control](../Vevor%20remote%20control/) project
5. **Add status parsing** - decode heater response packets (56 bytes)
6. **Web interface** - expose serial control via WiFi REST API

## Protocol Documentation

See [PROTOCOL.md](../Vevor-Serial-Sniffer/PROTOCOL.md) for complete protocol specification.

## Related Projects

- [Vevor-Serial-Sniffer](../Vevor-Serial-Sniffer/) - Monitor protocol traffic
- [Vevor remote control](../Vevor%20remote%20control/) - Main RF-based controller (v2.0)
- [WCS1800-CurrentSensor-Test](../WCS1800-CurrentSensor-Test/) - Current sensor testing
