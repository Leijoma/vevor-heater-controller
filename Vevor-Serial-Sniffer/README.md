# Vevor Heater Serial Protocol Sniffer

Passive monitoring tool to capture and decode single-wire serial communication between control panel and heater.

## Key Discovery

**This is NOT RS485!** The Vevor heater uses **single-wire serial communication** at 4800 baud (confirmed by [Hackaday project](https://hackaday.io/project/195170)).

## Hardware Setup

### Cable Colors (from control panel connector)
- **Black** = GND (ground)
- **Red** = +5V (powers control panel - supplied by heater)
- **Blue** = Serial data (single wire, bidirectional)

### Wiring

```
Control Panel          ESP32
━━━━━━━━━━━━━━━━━━━━━━━━━━━━
Black (GND)       →    GND
Red (+5V)         →    NOT CONNECTED*
Blue (Data)       →    GPIO16 (RX2)

* Red wire powers the panel from heater - don't connect to ESP32
```

### Important Notes
- **Ground bridge is critical**: ESP32 GND must connect to Black wire for serial communication to work
- **Red wire**: Leave disconnected from ESP32 (it's power from heater to panel)
- **Blue wire**: This is where all the data flows (bidirectional)

## Protocol Information

**COMPLETE SPECIFICATION** (confirmed from ESPHome implementation + captures):

### Packet Structure (Controller → Heater, 16 bytes):
```
Byte  | Value | Description
------|-------|----------------------------------
0     | 0xAA  | Frame start marker
1     | 0x66  | Controller ID (0x77 for heater responses)
2     | 0x06  | Command (0x02=status request, 0x06=start/stop)
3     | 0x0B  | Frame length (11 bytes)
4-7   | 0x00  | Reserved
8     | 0x01-0x0A | Power level (1-10)
9     | varies | State (0x02=OFF, 0x05=SET OFF, 0x06=START, 0x08=RUNNING)
10-14 | 0x00  | Reserved
15    | varies | Checksum (sum of bytes 2-14, mod 256)
```

### Checksum Algorithm:
```cpp
uint8_t checksum = 0;
for (int i = 2; i <= 14; i++) {
  checksum += packet[i];
}
checksum &= 0xFF;  // Keep lower 8 bits
```

**Example packet (Power level 10, START):**
```
AA 66 06 0B 00 00 00 00 0A 06 00 00 00 00 00 21
Checksum: 06 + 0B + 0A + 06 = 0x21 ✓
```

### Communication Parameters:
- **Baud rate**: 4800
- **Signal**: Inverted UART (logic 1 = low voltage, logic 0 = high voltage)
- **Timing**: Controller sends every ~1 second

## Usage

1. Upload firmware to ESP32
2. Connect wiring as shown above
3. Open serial monitor at 115200 baud
4. Use control panel (press buttons)
5. Watch captured packets in serial monitor

## Expected Output

```
[    132850 ms] Packet (16 bytes): AA 66 06 0B 00 00 00 00 0A 06 00 00 00 00 00 21
  ┌─ Decoded:
  │ From: CONTROLLER
  │ Command: 0x06 (START/STOP)
  │ Frame length: 0x0B (11 bytes)
  │ Power level: 10/10
  │ State: 0x06 (START)
  │ Checksum: 0x21 (calculated: 0x21) ✓ VALID
  └─
```

## Troubleshooting

### No data or garbage characters?

The protocol uses **inverted UART signals**. Try uncommenting this line in [main.cpp:42](src/main.cpp#L42):
```cpp
Serial2.setRxInvert(true);  // Invert RX signal
```

### Still no data?
- Verify level shifter connections (HV side to blue wire, LV side to GPIO16)
- Check ground connection (critical!)
- Verify control panel is powered (red/black wires have +5V)
- Try swapping HV/LV sides on level shifter

## Next Steps

Once you've captured the protocol:
1. Decode packet structure
2. Implement command sending
3. Create ESP32-based controller replacement
