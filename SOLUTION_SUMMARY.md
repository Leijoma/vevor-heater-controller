# Vevor Heater Bidirectional Communication - Solution Summary

## The Problem

**Goal**: Communicate with Vevor diesel heater control panel bidirectionally (both listen and respond)

**Challenge**: Using two separate GPIO pins (GPIO16 RX + GPIO17 TX) on the same wire causes electrical interference and garbled data.

**Evidence**:
- Sniffer (GPIO16 only) = Clean packets ✅
- Transmitter (GPIO16 + GPIO17) = Garbled data ❌

---

## The Solution: TX Gate Transistor

**Source**: BluetoothHeater project by Ray Jones
**URL**: https://gitlab.com/mrjones.id.au/bluetoothheater

### Key Insight:

Use a **separate GPIO pin (GPIO18) to control an NPN transistor** that gates TX on/off:

```
GPIO 18 = LOW  → Transistor OFF → TX blocked (high-Z) → RX can listen ✅
GPIO 18 = HIGH → Transistor ON  → TX enabled         → Can transmit ✅
```

### Hardware Required:

- 1x NPN transistor (2N2222 or similar) - **~$0.10**
- 2x 10kΩ resistors - **~$0.05**
- Breadboard for prototyping - **~$2.00**

**Total cost: ~$3**

---

## Wiring Diagram

```
ESP32                  TX Gate Circuit              Level Shifter
──────────────────────────────────────────────────────────────────

GPIO 16 (RX) ──────────────────────────┬─→ LV1 → HV1 ──→ Blue Wire
                                       │
GPIO 17 (TX) ───────────────────┬──────┘
                               │
GPIO 18 (Gate) ──[10kΩ]──┬─ Base (NPN)
                         │
                    +3.3V    Collector (connects RX & TX to bus)
                         │
                         └[10kΩ] (pull-up)

                          Emitter → GND

Level Shifter:
  LV VCC → 3.3V
  HV VCC → Red wire (+5V)
  GND → Black wire
  HV1 → Blue wire
```

---

## What We've Built

### ✅ Projects Created:

1. **[Vevor-Serial-Sniffer](Vevor-Serial-Sniffer/)** - RX only, fully working
   - Decodes controller packets
   - Shows power level, state, checksum
   - **Status**: Working perfectly ✅

2. **[Vevor-Serial-Transmitter](Vevor-Serial-Transmitter/)** - RX + TX with gate control
   - Heater impersonation mode
   - Auto-responds to controller
   - TX gate transistor control
   - **Status**: Ready to test (needs transistor circuit) ⏳

3. **Protocol Documentation**:
   - [PROTOCOL.md](Vevor-Serial-Sniffer/PROTOCOL.md) - Complete protocol spec
   - [TX_GATE_WIRING.md](Vevor-Serial-Transmitter/TX_GATE_WIRING.md) - Hardware guide

---

## Protocol Information Discovered

### From ESPHome Implementation:
- **Baud rate**: 4800
- **Signal**: Inverted UART
- **Packet size**: 16 bytes (controller → heater)
- **Response size**: 56 bytes (heater → controller)
- **Checksum**: Sum of bytes 2-14, mod 256

### Controller Packet Structure:
```
AA 66 06 0B 00 00 00 00 0A 06 00 00 00 00 00 21
│  │  │  │              │  │                 │
│  │  │  │              │  │                 └─ Checksum ✓
│  │  │  │              │  └─────────────────── State (06=START, 02=OFF, 08=RUNNING)
│  │  │  │              └────────────────────── Power level (1-10)
│  │  │  └───────────────────────────────────── Frame length (0x0B = 11 bytes)
│  │  └──────────────────────────────────────── Command (06=START/STOP, 02=STATUS)
│  └─────────────────────────────────────────── Controller ID (0x66)
└────────────────────────────────────────────── Frame start (0xAA)
```

---

## Current Status

### ✅ What Works:
- RX monitoring with clean packet decoding
- Protocol fully understood
- Checksum algorithm verified
- TX gate firmware ready

### ⏳ Next Steps:

1. **Build TX gate transistor circuit** (~30 minutes)
2. **Test RX still works with circuit added**
3. **Upload Transmitter firmware**
4. **Test bidirectional communication**
5. **Verify controller display reacts**

---

## Alternative Options (If Transistor Doesn't Work)

### Option 1: Two ESP32s
- ESP32 #1: Sniffer (RX only) - already working
- ESP32 #2: Transmitter (TX only)
- Both wired to same blue wire
- No additional hardware needed

### Option 2: Single GPIO for Both RX & TX
- Use GPIO16 for BOTH RX and TX (same pin)
- Configure as: `Serial2.begin(4800, SERIAL_8N1, 16, 16);`
- May not work on all ESP32 Arduino core versions
- Worth trying if transistor circuit fails

### Option 3: Monitor Only
- Keep using Sniffer (current working setup)
- Can capture all protocol data
- Cannot test controller reactions
- Cannot control real heater

---

## Testing Plan

### Phase 1: Verify RX (Already Complete ✅)
```bash
cd "/Users/magnus/Documents/PlatformIO/Projects/Vevor-Serial-Sniffer"
pio run --target upload
pio device monitor --baud 115200
```

**Expected**: Clean decoded packets from controller

### Phase 2: Build Transistor Circuit
See [TX_GATE_WIRING.md](Vevor-Serial-Transmitter/TX_GATE_WIRING.md) for detailed instructions

### Phase 3: Test Transmitter
```bash
cd "/Users/magnus/Documents/PlatformIO/Projects/Vevor-Serial-Transmitter"
pio run --target upload
pio device monitor --baud 115200
```

**Expected**:
- Clean RX packets from controller
- ESP32 sends heater responses
- Controller display reacts

---

## Success Indicators

### Level 1: RX Works with TX Circuit Present ✅
```
[RECEIVED] AA 66 06 0B 00 00 00 00 0A 06 00 00 00 00 00 21
  ┌─ Decoded:
  │ From: CONTROLLER
  │ Command: 0x06 (START/STOP)
  │ Power level: 10/10
  └─
```

### Level 2: TX Can Transmit ✅
```
→ Auto-responding as HEATER...
[HEATER RESPONSE] AA 77 06 33 ... (56 bytes)
  From: HEATER (0x77)
```

### Level 3: Controller Display Reacts 🎉
- **With auto-respond ON**: Display shows heater data (temperature, fan speed, etc.)
- **With auto-respond OFF** (press 'a'): Display shows error/timeout
- **Re-enable** (press 'a' again): Display returns to normal

This proves **you can control what the controller displays!**

---

## Future Integration

Once bidirectional works, integrate into main controller project:

1. Add serial communication to [Vevor remote control](Vevor%20remote%20control/)
2. Replace RF with serial (more reliable)
3. Get real heater status instead of estimates
4. Read actual temperature from heater
5. Detect errors and faults
6. Update web interface with real data

---

## Key Learnings

### ✅ What We Confirmed:
- Level shifting (5V ↔ 3.3V) is required
- UART signal inversion is required (RX confirmed)
- Two separate GPIO pins on same wire without gate = electrical conflict
- TX gate transistor = proven solution from BluetoothHeater project
- Protocol is well-documented and understood

### ❌ What Doesn't Work:
- Two separate GPIOs without isolation = garbled data
- Connecting both GPIOs to separate level shifter channels = electrical conflict
- Software-only half-duplex (Serial2 reconfiguration) = too complex/unreliable

### ✅ What Works:
- RX-only monitoring = perfect
- TX gate transistor control = proven solution (needs testing)
- Protocol decoding = fully functional

---

## References

1. **BluetoothHeater Project**: https://gitlab.com/mrjones.id.au/bluetoothheater
   - Source of TX gate solution
   - Complete ESP32 heater controller
   - Proven working implementation

2. **ESPHome Vevor Heater**: https://github.com/zatakon/esphome-vevor-heater
   - Complete protocol specification
   - Checksum algorithm
   - Packet structures

3. **Hackaday Protocol Analysis**: https://hackaday.io/project/195170
   - Original protocol reverse engineering
   - Single-wire serial discovery
   - Packet captures

---

## Support

For detailed wiring instructions, see:
- [TX_GATE_WIRING.md](Vevor-Serial-Transmitter/TX_GATE_WIRING.md)

For protocol details, see:
- [PROTOCOL.md](Vevor-Serial-Sniffer/PROTOCOL.md)

For testing without hardware, keep using:
- [Vevor-Serial-Sniffer](Vevor-Serial-Sniffer/) - proven working RX monitoring
