# TX Gate Transistor Wiring Guide

## The Solution (from BluetoothHeater Project)

The BluetoothHeater project (https://gitlab.com/mrjones.id.au/bluetoothheater) uses a **TX Gate transistor** to control half-duplex communication on the shared blue wire.

### How It Works:

- **TX Gate LOW**: TX blocked (high-Z), RX can listen cleanly ✅
- **TX Gate HIGH**: TX enabled, can send data ✅
- **No interference**: RX receives clean packets even with TX wired

---

## Hardware Requirements

### Components Needed:

1. **NPN Transistor**: 2N2222, 2N3904, or similar general-purpose NPN
2. **Resistors**:
   - 1x 10kΩ (base resistor)
   - 1x 10kΩ (pull-up, optional but recommended)
3. **4-channel Bidirectional Level Shifter** (you already have this)
4. **Breadboard** or prototype board for testing

---

## Wiring Diagram

```
ESP32                  TX Gate Circuit              Level Shifter    Heater
──────────────────────────────────────────────────────────────────────────

GPIO 16 (RX) ──────────────────────────┬─→ LV1 → HV1 ──┐
                                       │                │
                                       │                ├─→ Blue Wire
                                       │                │
GPIO 17 (TX) ───────────────────┬──────┘                │
                               │                        │
GPIO 18 (Gate) ──[10kΩ]──┬─ Base                        │
                         │   │                          │
                         │  NPN                         │
                         │   │                          │
                    +3.3V    Collector ─────────────────┘
                         │   │
                         └[10kΩ] (pull-up)
                             │
                          Emitter
                             │
                            GND ─────────→ GND → GND ──→ Black Wire
                                              │
                                          +3.3V → LV VCC
                                              │
                                          HV VCC ←─────── Red (+5V)
```

---

## Step-by-Step Wiring

### Step 1: Level Shifter Connections

```
Level Shifter LV side:
- LV (VCC) → ESP32 3.3V
- GND → ESP32 GND

Level Shifter HV side:
- HV (VCC) → Red wire (+5V from heater panel)
- GND → Black wire (heater GND)
- HV1 → Blue wire (data)
```

### Step 2: RX Connection (Simple)

```
ESP32 GPIO 16 → Level Shifter LV1
```

This is straightforward - direct connection.

### Step 3: TX Gate Transistor Circuit

**On breadboard:**

```
Row 1:  GPIO 18 ──[10kΩ resistor]── Row 2 (Base connection point)
Row 2:  Base (B) of NPN transistor
Row 3:  Collector (C) of NPN transistor
Row 4:  Emitter (E) of NPN transistor ── GND rail

Row 3:  Collector ──[10kΩ pull-up]── +3.3V rail
Row 3:  Collector ── Junction point
```

**Connections from junction point:**

```
Junction ─┬─ GPIO 17 (TX)
          │
          └─ Level Shifter LV1 (same as GPIO 16!)
```

---

## Detailed Component Placement

### NPN Transistor Pinout (TO-92 package, flat side facing you):

```
   ┌───┐
   │   │  ← Flat side
   │ T │
   └─┬─┘
     │
   ──┴────────
   E  B  C
   │  │  │
   │  │  └─ Collector (to pull-up & level shifter)
   │  └──── Base (to GPIO 18 via 10kΩ)
   └─────── Emitter (to GND)
```

### Breadboard Layout Example:

```
        ESP32          Breadboard                Level Shifter

GPIO 18 ────→ j10 ─[10kΩ]─ h10
                            h10 ─── Transistor Base (row 10)

GPIO 17 ────────────────→ g12 ─── Collector junction

              +3.3V ──[10kΩ]─ f12 ─── Collector junction

                            h12 ─── Transistor Collector (row 12)

                            h14 ─── Transistor Emitter (row 14)

              GND ──────────→ h14

GPIO 16 ────────────────────────→ j12 ───→ Level Shifter LV1

Collector junction (row 12) ─────────────→ Level Shifter LV1
```

---

## How the Circuit Works

### 1. Idle State (Listening):

```
GPIO 18 = LOW
  ↓
Transistor OFF (no base current)
  ↓
Collector floating (pulled HIGH by 10kΩ resistor)
  ↓
GPIO 17 (TX) cannot pull line LOW
  ↓
RX receives clean data ✅
```

### 2. Transmit State:

```
GPIO 18 = HIGH
  ↓
Transistor ON (base current flows)
  ↓
Collector connected to GND through emitter
  ↓
GPIO 17 (TX) can drive the line
  ↓
TX data sent to heater ✅
```

### 3. Why This Works:

- **RX always connected** to bus (via level shifter)
- **TX gated by transistor** - only active when GPIO 18 is HIGH
- **No electrical conflict** - TX is truly disconnected when gate is LOW
- **Software controlled** - enableTX()/disableTX() functions control timing

---

## Testing Procedure

### Phase 1: Test RX Only (No Transistor Yet)

1. Wire only GPIO 16 → Level Shifter LV1 → Blue wire
2. Upload **Sniffer** firmware
3. Verify clean packet reception
4. **If this doesn't work, fix wiring first!**

### Phase 2: Add TX Gate Circuit

1. Build transistor circuit on breadboard
2. Connect GPIO 17 through transistor to level shifter
3. Connect GPIO 18 to transistor base
4. **Don't connect to heater yet** - test circuit first

**Test the gate:**
```cpp
// Add to setup() for testing
pinMode(18, OUTPUT);
pinMode(17, OUTPUT);

digitalWrite(18, LOW);   // Gate off
digitalWrite(17, HIGH);  // Try to drive TX
// Measure voltage at collector - should be ~3.3V (pulled up)

digitalWrite(18, HIGH);  // Gate on
digitalWrite(17, HIGH);  // Drive TX high
// Measure voltage at collector - should be ~3.3V (driven by TX)

digitalWrite(18, HIGH);  // Gate on
digitalWrite(17, LOW);   // Drive TX low
// Measure voltage at collector - should be ~0V (pulled to GND via transistor)
```

### Phase 3: Test with Transmitter Firmware

1. Upload **Vevor-Serial-Transmitter** firmware
2. Connect to heater blue wire
3. Monitor serial output
4. Should see:
   ```
   [RECEIVED] AA 66 06 0B ... (clean packets) ✅
   → Auto-responding as HEATER...
   [HEATER RESPONSE] AA 77 06 33 ... ✅
   ```

### Phase 4: Verify Controller Display Reacts

1. Watch control panel display
2. With auto-respond ON: Display should show heater data
3. Press 'a' to disable auto-respond
4. Display should show communication error
5. Press 'a' to enable again
6. Display returns to normal

**This proves bidirectional communication works!** 🎉

---

## Troubleshooting

### RX Still Garbled with Transistor Added?

**Check:**
- Is transistor actually OFF when gate is LOW?
- Measure collector voltage with gate LOW - should be ~3.3V (pulled up)
- Is emitter truly connected to GND?
- Is base resistor correct (10kΩ)?

**Try:**
- Remove GPIO 17 connection temporarily
- Verify RX works with just GPIO 16 + transistor circuit present
- Transistor should be invisible to RX when gate is LOW

### TX Doesn't Transmit?

**Check:**
- Is GPIO 18 going HIGH when transmitting?
- Measure collector voltage when gate is HIGH and TX is LOW - should be ~0V
- Is transistor in correct orientation?
- Are all connections secure?

**Try:**
- Manually set GPIO 18 HIGH and toggle GPIO 17 - measure collector
- Should follow GPIO 17 when gate is HIGH

### Controller Display Doesn't React?

**Possible causes:**
1. **TX signal inversion** - NPN transistor inverts the signal, which might be what we need!
2. **Packet structure wrong** - heater might not accept our fake data
3. **Timing wrong** - might need to respond faster/slower

**Try:**
- Run sniffer on second ESP32 to verify packets are actually being sent
- Check if packets appear on the bus when transmitting

---

## Alternative: Software-Only Test (Without Transistor)

If you want to test the firmware first without building the transistor circuit:

### Option 1: Use GPIO 18 to Control Level Shifter OE Pin

Some level shifters have an Output Enable (OE) pin that can disable a channel:

```
GPIO 18 → Level Shifter OE pin for TX channel
When OE = LOW: Channel disabled (TX blocked)
When OE = HIGH: Channel enabled (TX active)
```

**Check your level shifter datasheet!**

### Option 2: Two Separate Level Shifter Channels

This **won't work** as you discovered - causes electrical conflicts. But documented here for completeness.

---

## Bill of Materials

| Component | Qty | Notes | Cost |
|-----------|-----|-------|------|
| 2N2222 NPN Transistor | 1 | Any general-purpose NPN works | $0.10 |
| 10kΩ Resistor | 2 | 1/4W, 5% tolerance | $0.05 |
| Breadboard | 1 | For prototyping | $2.00 |
| Jumper wires | ~10 | Male-to-male | $1.00 |
| **Total** | | | **~$3.15** |

---

## Success Criteria

✅ **RX works cleanly** - see decoded controller packets
✅ **TX can transmit** - ESP32 sends response packets
✅ **No interference** - RX not disrupted by TX presence
✅ **Controller reacts** - display shows data or error when disabled

---

## Next Steps After Success

1. **Solder prototype** - move from breadboard to permanent wiring
2. **Test with real heater** - capture actual heater response packets
3. **Refine packet structure** - match real heater data format
4. **Integrate into main controller** - add to Vevor remote control project
5. **Replace RF with serial** - more reliable bidirectional control

---

## References

- BluetoothHeater Project: https://gitlab.com/mrjones.id.au/bluetoothheater
- ESPHome Vevor Heater: https://github.com/zatakon/esphome-vevor-heater
- Hackaday Protocol Reverse Engineering: https://hackaday.io/project/195170

---

## Circuit Photos / Diagrams Needed

*TODO: Add photos of actual breadboard setup when built*
*TODO: Add oscilloscope traces showing gate control*
*TODO: Add photo of working setup with control panel*
