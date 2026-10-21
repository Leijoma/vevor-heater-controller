# Vevor Heater Serial Protocol - Complete Specification

## Overview

The Vevor diesel heater uses **single-wire serial communication** at 4800 baud with **inverted UART signals**. This is NOT RS485! The communication occurs between the control panel and the heater unit on a single blue data wire.

**Protocol Source:** Confirmed via [ESPHome implementation](https://github.com/zatakon/esphome-vevor-heater) and live packet captures.

## Physical Layer

### Wiring (3-wire cable)
- **Black wire**: GND (ground reference)
- **Red wire**: +5V (powers control panel from heater)
- **Blue wire**: Serial data (single bidirectional UART)

### Serial Parameters
- **Baud rate**: 4800
- **Format**: 8N1 (8 data bits, no parity, 1 stop bit)
- **Signal inversion**: UART signals are inverted (logic 1 = low voltage)
- **Voltage level**: 5V (use level shifter for ESP32 3.3V compatibility)
- **Timing**: Controller sends packets every ~1 second (heartbeat)

## Packet Structure

### Controller → Heater (16 bytes)

| Byte | Value | Description |
|------|-------|-------------|
| 0 | `0xAA` | Frame start marker |
| 1 | `0x66` | Controller ID |
| 2 | `0x02` or `0x06` | Command byte |
| 3 | `0x0B` | Frame length (11 bytes payload) |
| 4-7 | `0x00` | Reserved (always zero) |
| 8 | `0x01`-`0x0A` | Power level (1-10) |
| 9 | varies | State byte |
| 10-14 | `0x00` | Reserved (always zero) |
| 15 | varies | Checksum |

### Heater → Controller (56+ bytes)

*Not yet fully captured - requires heater connected. Expected to contain:*
- Frame start: `0xAA`
- Heater ID: `0x77`
- Status information (temperature, runtime, errors, etc.)
- Checksum

## Command Bytes (Byte 2)

| Code | Name | Purpose |
|------|------|---------|
| `0x02` | STATUS_REQUEST | Poll heater for status |
| `0x06` | START_STOP | Control heater power state |

## State Bytes (Byte 9)

| Code | Name | Description |
|------|------|-------------|
| `0x02` | OFF | Heater is off |
| `0x05` | SET_OFF | Command to turn off |
| `0x06` | START | Starting/startup sequence |
| `0x08` | RUNNING | Stable combustion |

## Power Level (Byte 8)

- Range: `0x01` - `0x0A` (decimal 1-10)
- Controls fuel pump frequency and fan speed
- Level 10 = maximum heat output
- Level 1 = minimum heat output

## Checksum Algorithm (Byte 15)

**Simple additive checksum:**

```cpp
uint8_t calculateChecksum(uint8_t *packet) {
  uint8_t checksum = 0;
  for (int i = 2; i <= 14; i++) {  // Sum bytes 2 through 14
    checksum += packet[i];
  }
  return checksum & 0xFF;  // Keep lower 8 bits only
}
```

**Verification example:**
```
Packet: AA 66 06 0B 00 00 00 00 0A 06 00 00 00 00 00 21
Checksum calculation:
  06 + 0B + 00 + 00 + 00 + 00 + 0A + 06 + 00 + 00 + 00 + 00 + 00
  = 0x21 ✓
```

## Example Packets

### Power ON at level 10
```
AA 66 06 0B 00 00 00 00 0A 06 00 00 00 00 00 21
│  │  │  │              │  │                 │
│  │  │  │              │  │                 └─ Checksum (0x21)
│  │  │  │              │  └─────────────────── State: START (0x06)
│  │  │  │              └────────────────────── Power level: 10 (0x0A)
│  │  │  └───────────────────────────────────── Frame length: 11 bytes
│  │  └──────────────────────────────────────── Command: START_STOP
│  └─────────────────────────────────────────── Controller ID
└────────────────────────────────────────────── Frame start
```

### Power level 5, running
```
AA 66 06 0B 00 00 00 00 05 08 00 00 00 00 00 1E
                        │  │                 │
                        │  │                 └─ Checksum (0x1E)
                        │  └─────────────────── State: RUNNING (0x08)
                        └────────────────────── Power level: 5 (0x05)
```

### Power OFF
```
AA 66 06 0B 00 00 00 00 01 02 00 00 00 00 00 14
                        │  │                 │
                        │  │                 └─ Checksum (0x14)
                        │  └─────────────────── State: OFF (0x02)
                        └────────────────────── Power level: 1 (min)
```

## ESP32 Implementation

### Receiving (Sniffing)

```cpp
// Initialize Serial2 for receive
#define RX_PIN 16
Serial2.begin(4800, SERIAL_8N1, RX_PIN, -1);  // -1 = no TX pin

// IMPORTANT: May need signal inversion
Serial2.setRxInvert(true);  // Try if getting garbage data

// Read bytes
while (Serial2.available()) {
  uint8_t byte = Serial2.read();
  // Buffer and decode...
}
```

### Sending Commands

```cpp
// Initialize Serial2 for transmit
#define TX_PIN 17
Serial2.begin(4800, SERIAL_8N1, -1, TX_PIN);  // -1 = no RX pin

// IMPORTANT: May need signal inversion
Serial2.setTxInvert(true);  // Try if heater doesn't respond

// Build packet
uint8_t packet[16] = {
  0xAA,  // Start
  0x66,  // Controller ID
  0x06,  // Command: START_STOP
  0x0B,  // Frame length
  0x00, 0x00, 0x00, 0x00,  // Reserved
  0x0A,  // Power level: 10
  0x06,  // State: START
  0x00, 0x00, 0x00, 0x00, 0x00,  // Reserved
  0x00   // Checksum (calculate below)
};

// Calculate and set checksum
packet[15] = calculateChecksum(packet);

// Send packet
Serial2.write(packet, 16);
```

### Full Example: Turn heater ON at power level 8

```cpp
void sendHeaterCommand(uint8_t powerLevel, uint8_t state) {
  uint8_t packet[16] = {
    0xAA,              // Byte 0: Frame start
    0x66,              // Byte 1: Controller ID
    0x06,              // Byte 2: Command (START_STOP)
    0x0B,              // Byte 3: Frame length
    0x00, 0x00, 0x00, 0x00,  // Bytes 4-7: Reserved
    powerLevel,        // Byte 8: Power level (1-10)
    state,             // Byte 9: State (0x02=OFF, 0x06=START, 0x08=RUNNING)
    0x00, 0x00, 0x00, 0x00, 0x00,  // Bytes 10-14: Reserved
    0x00               // Byte 15: Checksum (calculated below)
  };

  // Calculate checksum (sum bytes 2-14)
  uint8_t checksum = 0;
  for (int i = 2; i <= 14; i++) {
    checksum += packet[i];
  }
  packet[15] = checksum & 0xFF;

  // Send to heater
  Serial2.write(packet, 16);
}

// Usage:
sendHeaterCommand(8, 0x06);  // Turn ON at level 8
delay(1000);
sendHeaterCommand(8, 0x08);  // Confirm RUNNING at level 8
delay(5000);
sendHeaterCommand(1, 0x02);  // Turn OFF
```

## Integration into Main Controller

### Option 1: Add Serial Control to Existing RF Controller

The `Vevor remote control` project currently uses RF 433MHz. You can add serial control as an alternative method:

**Benefits:**
- Bidirectional communication (read heater status)
- More reliable than RF
- Real power level confirmation
- Error codes from heater
- Actual temperature readings

**Implementation steps:**
1. Add Serial2 initialization in setup()
2. Create `sendSerialCommand()` function
3. Add mode selection (RF vs Serial)
4. Parse heater response packets for status

### Option 2: Pure Serial Controller

Replace RF entirely with serial communication:
- Remove rc-switch library
- Implement serial packet builder
- Add response parser for heater status (56-byte packets)
- More accurate state tracking

## Signal Inversion Details

**Why inverted?**
The Vevor protocol uses inverted UART signaling. This means:
- Logic 1 (idle/mark) = LOW voltage (~0V)
- Logic 0 (space) = HIGH voltage (~5V)

**Standard UART vs Inverted:**
```
Standard:  ___─────___─────___   (idle high)
Inverted:  ───_____───_____───   (idle low)
```

**ESP32 support:**
The ESP32 hardware UART supports signal inversion natively:
```cpp
Serial2.setRxInvert(true);  // Invert received data
Serial2.setTxInvert(true);  // Invert transmitted data
```

**Testing approach:**
1. Try without inversion first
2. If you see garbage/no data, enable inversion
3. If still problems, check level shifter and ground connections

## References

- [ESPHome Vevor Heater Component](https://github.com/zatakon/esphome-vevor-heater) - Complete working implementation
- [Hackaday Vevor Protocol Project](https://hackaday.io/project/195170-vevor-diesel-heater-protocol) - Original reverse engineering
- Live packet captures from control panel → heater

## Next Steps

1. ✅ **Protocol decoded** - We now have complete packet structure
2. ✅ **Sniffer working** - Can capture and decode packets
3. 🔄 **Test signal inversion** - Determine if `setRxInvert()` is needed
4. 🔄 **Capture heater responses** - Need to sniff heater→controller packets
5. 🔄 **Implement transmit** - Send commands to heater
6. 🔄 **Integrate into main controller** - Add serial control to `Vevor remote control` project
7. 🔄 **Add status parsing** - Decode heater response packets
8. 🔄 **Web interface updates** - Show real heater status instead of estimated

## Notes

- Control panel sends packets every ~1 second as heartbeat
- Heater expects regular communication or it will shut down (watchdog)
- Power level changes take effect immediately
- State transitions must follow valid sequences (OFF → START → RUNNING → OFF)
- Checksum validation is important - heater ignores invalid packets
