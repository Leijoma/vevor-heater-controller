# Testing Guide - Heater Impersonation Mode

## Quick Start

You have the control panel but not the heater. This firmware lets the ESP32 **pretend to be the heater** so you can test if the controller reacts!

## Wiring

```
ESP32                  Level Shifter       Control Panel Cable
─────────────────────────────────────────────────────────────
GPIO 16 (RX) ─┐
              ├──→  LV side  →  HV side  →  Blue (data)
GPIO 17 (TX) ─┘
GND  ─────────→  GND  ──────→  GND  ─────→  Black (GND)
3.3V ─────────→  LV VCC
                 HV VCC  ←─────────────────  Red (+5V)
```

**Both GPIO16 and GPIO17 connect to the SAME blue wire through level shifter!**

## What It Does

1. **Listens** for controller packets on GPIO16 (RX)
2. **Auto-responds** with fake heater status on GPIO17 (TX)
3. **Simulates:**
   - Temperature: 45°C
   - Fan speed: 2500 RPM
   - Voltage: 13.2V
   - Power level: Whatever controller requested
   - State: Whatever controller requested

## Expected Behavior

### Test 1: Check Communication
1. Upload firmware
2. Open serial monitor (115200 baud)
3. Wait for controller to send packets (~1 second intervals)
4. You should see:
   ```
   [RECEIVED] AA 66 06 0B 00 00 00 00 0A 06 ...
   → Auto-responding as HEATER...
   [HEATER RESPONSE] AA 77 06 33 ...
   ```

### Test 2: Watch Control Panel Display
**With ESP32 responding:**
- Display should show NORMAL operation
- Temperature reading (45°C)
- Fan speed, power level
- NO error messages

### Test 3: Disable Auto-Respond
1. Press `a` in serial monitor
2. Auto-respond turns OFF
3. Controller keeps sending, but gets no responses
4. **Watch control panel display:**
   - Should show communication error
   - Timeout
   - Error code
   - Maybe flashing or beeping

### Test 4: Re-Enable
1. Press `a` again
2. Auto-respond turns ON
3. Display should return to normal

## Limitations

### TX Signal Inversion
The ESP32 Arduino core used doesn't support `setTxInvert()`. This means transmitted data might not be inverted correctly.

**What to try:**
1. **Test as-is first** - Some level shifters handle inversion
2. **If display doesn't react:**
   - Add hardware inverter (NPN transistor)
   - Or upgrade ESP32 core version
   - Or level shifter might need adjustment

### Packet Structure
The 56-byte heater response is approximated from ESPHome research. Some fields might be wrong:
- Byte positions guessed
- Temperature encoding might be off
- Fan speed encoding might be off

**But:** Even if not perfect, controller should at least acknowledge receiving SOMETHING, which proves TX works!

## Success Criteria

✅ **RX working** - You see `[RECEIVED]` packets in serial monitor
✅ **TX working** - Control panel display reacts (shows data OR shows error when disabled)
❓ **TX inversion** - May need hardware fix if display doesn't react

## Next Steps After Testing

Once you confirm TX works (display reacts):

1. **Capture real heater responses** - Connect with actual heater to see real 56-byte packets
2. **Refine packet structure** - Match exact byte positions from real data
3. **Integrate into main controller** - Add to [Vevor remote control](../Vevor%20remote%20control/) project
4. **Bidirectional control** - Replace RF with serial for better reliability
5. **Web interface updates** - Show real heater status instead of estimated

## Troubleshooting

**See packets received but controller doesn't react?**
- TX signal inversion issue - see README.md for hardware inverter circuit
- Try different level shifter
- Check TX wire connection

**Don't see any packets?**
- Check wiring (especially GND!)
- Verify control panel is powered
- Verify RX connection to GPIO16

**Packets look garbled?**
- Signal inversion already enabled for RX (working)
- Check level shifter orientation

**Control panel shows error immediately?**
- Good! This means it's trying to communicate
- TX signal might be inverted wrong, but controller sees SOMETHING
- Try hardware inverter on TX line
