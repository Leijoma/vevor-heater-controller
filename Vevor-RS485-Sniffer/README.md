# Vevor Heater RS485 Protocol Sniffer

Passive monitoring tool to capture and decode RS485 communication between Vevor diesel heater controller and heater unit.

## Purpose

- **Learn the protocol** - Capture real communication packets
- **Verify RS485 adapter** - Test your hardware setup
- **Debug issues** - See what commands are being sent
- **Reverse engineer** - Discover unknown commands and states

## Hardware Setup

### Required Components
1. **ESP32 Development Board**
2. **RS485 to TTL Converter Module** (3.3V/5V compatible)
3. **Wires** for tapping into heater communication

### Wiring

```
RS485 Module → ESP32
────────────────────
RX  → GPIO16 (RX2)
TX  → NOT CONNECTED (read-only mode!)
GND → GND
VCC → 3.3V or 5V

RS485 Module → Heater Cable
────────────────────────────
A/B terminals → Blue wire (parallel tap)
```

**IMPORTANT:** This is **READ-ONLY** monitoring. Do NOT connect TX pin - we're only listening!

### Tapping the Communication Line

**Option A: Parallel Tap (Non-Invasive)**
```
Controller Blue Wire ──┬──→ Heater Blue Wire
                        │
                        └──→ RS485 Module A or B

Controller Black Wire ──┬──→ Heater Black Wire
                        │
                        └──→ ESP32 GND (common ground)
```

**Option B: Series Connection (More Reliable)**
```
Controller Blue Wire → RS485 Module A → Heater Blue Wire
                              B → RS485 RX (GPIO16)
```

## Building and Running

```bash
cd "/Users/magnus/Documents/PlatformIO/Projects/Vevor-RS485-Sniffer"

# Upload to ESP32
pio run --target upload

# Open serial monitor
pio device monitor
```

## What You'll See

### Startup Message
```
╔═══════════════════════════════════════════════════════════╗
║         Vevor Heater RS485 Protocol Sniffer             ║
╚═══════════════════════════════════════════════════════════╝

Configuration:
  RX Pin:    GPIO16 (Serial2)
  Baud Rate: 4800
  Mode:      8N1

Listening for packets...
```

### Example Output - Controller → Heater

```
┌─────────────────────────────────────────────────────────┐
│ Packet #1 (15 bytes)                                    │
└─────────────────────────────────────────────────────────┘

  HEX:  AA 66 06 0B 00 00 00 00 0A 06 00 00 00 00 21

  📤 CONTROLLER → HEATER
     Device ID:       0x66
     Command:         0x06 (START_UP)
     Length:          0x0B (11 bytes)
     Power Level:     10/10
     Requested State: 0x06 (START)
     Checksum:        0x21 (calculated: 0x21) ✓
```

### Example Output - Heater → Controller

```
┌─────────────────────────────────────────────────────────┐
│ Packet #2 (55 bytes)                                    │
└─────────────────────────────────────────────────────────┘

  HEX:  AA 77 02 33 00 03 0A 00 ...

  📥 HEATER → CONTROLLER
     Device ID:        0x77
     Command:          0x02
     Length:           0x33 (51 bytes)
     Operating State:  0x03 (STABLE)
     Power Level:      10/10
     Error Code:       0x00
     Heat Exchanger:   68.5 °C
     Checksum:         0xXX (at byte 54)
```

## Protocol Details

### Controller → Heater (15 bytes total)

| Byte | Description | Example |
|------|-------------|---------|
| 0 | Header | 0xAA |
| 1 | Device ID (controller) | 0x66 |
| 2 | Command | 0x02=get state, 0x06=start |
| 3 | Length | 0x0B (11 bytes payload) |
| 4-7 | Unknown/Reserved | 0x00 |
| 8 | Power Level | 1-10 |
| 9 | Requested State | 0x02=off, 0x06=start, 0x08=run |
| 10-13 | Unknown/Reserved | 0x00 |
| 14 | Checksum | Sum of bytes 2+3+8+9 |

### Heater → Controller (55 bytes total)

| Byte | Description | Example |
|------|-------------|---------|
| 0 | Header | 0xAA |
| 1 | Device ID (heater) | 0x77 |
| 2 | Command | 0x02 |
| 3 | Length | 0x33 (51 bytes payload) |
| 4 | Unknown | - |
| 5 | Operating State | 0-4 (see below) |
| 6 | Power Level | 1-10 |
| 7 | Error Code | 0x00 = no error |
| 8-15 | Unknown | - |
| 16-17 | Heat Exchanger Temp | °C × 100 (big endian) |
| 18-53 | Unknown/Other data | - |
| 54 | Checksum | TBD |

### Operating States

| Value | State | Description |
|-------|-------|-------------|
| 0x00 | OFF | Heater is off |
| 0x01 | GLOW_PREHEAT | Glow plug heating |
| 0x02 | IGNITED | Fuel ignited, warming up |
| 0x03 | STABLE | Running normally |
| 0x04 | COOLING | Shutdown cooling phase |

### Commands (Controller → Heater)

| Value | Command | Description |
|-------|---------|-------------|
| 0x02 | GET_STATE | Request status |
| 0x06 | START_UP | Start heater |

### Requested States (Controller → Heater)

| Value | State | Description |
|-------|-------|-------------|
| 0x02 | OFF | Turn off |
| 0x05 | COOLING | Enter cooling mode |
| 0x06 | START | Start heater |
| 0x08 | RUNNING | Normal running |

## Troubleshooting

### No Packets Received

**Check hardware:**
- RS485 module powered? (LED should be on)
- GPIO16 connected correctly?
- Common ground between ESP32 and RS485 module?
- RS485 module connected to heater blue wire?

**Check heater:**
- Is heater powered on?
- Is original controller connected and working?
- Try turning heater on/off to generate traffic

### Garbled/Invalid Data

**Baud rate mismatch:**
- Verify 4800 baud is correct for your heater model
- Some models might use different rates

**Signal issues:**
- RS485 wiring reversed? (swap A/B)
- Long cable runs may need termination resistors
- Noise on the line - try shorter wires

**Inverted signals:**
- Some RS485 modules need inverted UART
- May need to configure Serial2 with SERIAL_8N1_INVERT

### Partial Packets

**Buffer overflow:**
- Increase MAX_PACKET_SIZE if needed
- Check for transmission errors

**Timing issues:**
- Adjust PACKET_TIMEOUT_MS (default 50ms)
- May need tuning for your system

## Next Steps

### After Successful Sniffing

1. **Analyze captured packets**
   - Note command sequences
   - Identify state transitions
   - Map power level changes

2. **Test sending commands**
   - Create transmitter version
   - Send captured commands
   - Verify heater responds

3. **Integrate with main controller**
   - Add RS485 send capability
   - Implement bidirectional communication
   - Create hybrid RF + RS485 control

## Technical Notes

### Checksum Algorithm (Controller → Heater)
```cpp
checksum = (byte[2] + byte[3] + byte[8] + byte[9]) & 0xFF;
```

### Temperature Decoding
```cpp
int16_t raw = (byte[16] << 8) | byte[17];  // Big endian
float tempC = raw / 100.0;                  // Scale by 100
```

### Protocol Reference
- **Baud:** 4800
- **Data bits:** 8
- **Parity:** None
- **Stop bits:** 1
- **Half-duplex:** Controller and heater take turns

## Safety Notes

⚠️ **This is passive monitoring only** - does not interfere with heater operation
⚠️ **Always maintain common ground** between all devices
⚠️ **Do not connect TX pin** during sniffing - read-only mode
⚠️ **Heater operates at high voltage** - keep electronics away from power lines

## Credits

Protocol information based on:
- [Hackaday Vevor Protocol Project](https://hackaday.io/project/195170-vevor-diesel-heater-protocol)
- [vevor_heater_control](https://github.com/zatakon/vevor_heater_control)
- [esphome-vevor-heater](https://github.com/zatakon/esphome-vevor-heater)
