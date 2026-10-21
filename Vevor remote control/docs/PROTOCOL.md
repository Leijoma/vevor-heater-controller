# Vevor Diesel Heater Serial Protocol

Complete protocol documentation reverse-engineered from Vevor diesel heater controller.

## Table of Contents
- [Hardware Interface](#hardware-interface)
- [Serial Configuration](#serial-configuration)
- [Packet Structure](#packet-structure)
- [Controller Commands](#controller-commands)
- [Heater Responses](#heater-responses)
- [State Machine](#state-machine)
- [Error Codes](#error-codes)
- [Examples](#examples)

---

## Hardware Interface

### Physical Connection
- **Single-wire half-duplex serial** (blue wire on heater connector)
- **5V signal level** (requires level shifting for 3.3V microcontrollers)
- **Hardware setup:**
  - 4-channel bidirectional level shifter module (3.3V ↔ 5V)
  - 470Ω resistor on TX line for half-duplex protection
  - RX and TX share the same level shifter channel

### Wiring Diagram
```
ESP32 (3.3V)         Level Shifter         Heater (5V)
------------         -------------         -----------
GPIO16 (RX) ───────→ LV1 ↔ HV1 ─────────→ Blue wire
                      ↕     ↕
GPIO17 (TX) ─[470Ω]→ LV1   HV1

3.3V ──────────────→ LV    HV  ←────────── Red wire (+5V)
GND ───────────────→ GND   GND ←────────── Black wire (GND)
```

**Key Points:**
- RX connects directly to level shifter LV1 (no resistor)
- TX connects through 470Ω resistor to level shifter LV1
- Both share the same channel for half-duplex operation
- Level shifter handles bidirectional voltage translation automatically

---

## Serial Configuration

- **Baud Rate:** 4800
- **Data Bits:** 8
- **Parity:** None
- **Stop Bits:** 1
- **Flow Control:** None
- **Mode:** Half-duplex (transmit and receive on same wire)

**Note:** TX packets will echo back on RX due to half-duplex nature.

---

## Packet Structure

### Controller → Heater (16 bytes)

```
Byte    Value       Description
----    -----       -----------
0       0xAA        Header byte 1
1       0x66        Header byte 2
2       0x02/0x06   Command (0x02=STATUS, 0x06=START/STOP)
3       0x0B        Frame length
4-7     0x00        Reserved/Unknown
8       0x01-0x0A   Power level (1-10)
9       0x05/0x06/0x08  State command or status flag
10-14   0x00        Reserved/Unknown
15      CHECKSUM    Sum of bytes 2-14 (8-bit)
```

### Heater → Controller (57 bytes)

```
Byte    Value       Description
----    -----       -----------
0       0xAA        Header byte 1
1       0x77        Header byte 2
2       0x02        Response type
3       0x34        Frame length (52 bytes)
4       0x01        Unknown
5       0x00-0x04   State (OFF=0, PREHEAT=1, IGNITION=2, COMBUSTION=3, COOLDOWN=4)
6       0x01-0x0A   Current power level (1-10)
7       0x00        Error code
8       0x01        Unknown
9       0x32        Unknown
10      0x00        Unknown
11      0x00-0xFF   Voltage (×0.1V)
12-13   0x00        Unknown
14      0x80        Unknown
15      0x00        Unknown
16-17   0x0000      Temperature (big-endian, ×0.1°C)
18-27   ...         Unknown
28-29   0x0000      Fan speed (big-endian, RPM)
30-55   ...         Unknown/Reserved
56      CHECKSUM    Sum of bytes 2-55 (8-bit)
```

---

## Controller Commands

### 1. STATUS_REQUEST (0x02)

**Purpose:** Request heater status AND set power level

**Important:** This command serves dual purpose:
- Requests current status from heater
- Sets the desired power level (byte 8)

**Packet:**
```
AA 66 02 0B 00 00 00 00 [POWER] 08 00 00 00 00 00 [CHECKSUM]
```

**Fields:**
- Byte 2: `0x02` (STATUS_REQUEST command)
- Byte 8: `0x01-0x0A` (Power level 1-10)
- Byte 9: `0x08` (Status flag)
- Byte 15: Checksum

**Example (Power level 10):**
```
AA 66 02 0B 00 00 00 00 0A 08 00 00 00 00 00 1F
```

**Example (Power level 5):**
```
AA 66 02 0B 00 00 00 00 05 08 00 00 00 00 00 1A
```

**Timing:** Controller sends this every 1 second while heater is active

**Power Change:** To change power while running, simply send STATUS_REQUEST with new power level in byte 8. Heater responds within 1-2 seconds.

### 2. START Command (0x06)

**Purpose:** Start the heater

**Packet:**
```
AA 66 06 0B 00 00 00 00 [POWER] 06 00 00 00 00 00 [CHECKSUM]
```

**Fields:**
- Byte 2: `0x06` (START_STOP command)
- Byte 8: `0x01-0x0A` (Power level 1-10)
- Byte 9: `0x06` (START state)
- Byte 15: Checksum

**Example (Start at power 10):**
```
AA 66 06 0B 00 00 00 00 0A 06 00 00 00 00 00 21
```

**Example (Start at power 5):**
```
AA 66 06 0B 00 00 00 00 05 06 00 00 00 00 00 1C
```

**Protocol:** Send START command 3 times (once per second), then resume STATUS_REQUEST polling

### 3. STOP Command (0x06)

**Purpose:** Stop the heater (initiates cooldown)

**Packet:**
```
AA 66 06 0B 00 00 00 00 0A 05 00 00 00 00 00 20
```

**Fields:**
- Byte 2: `0x06` (START_STOP command)
- Byte 8: `0x0A` (Power level, typically kept at current)
- Byte 9: `0x05` (STOP state)
- Byte 15: Checksum

**Protocol:** Send STOP command 3 times (once per second), then resume STATUS_REQUEST polling

**Note:** Heater enters COOLDOWN mode, runs fan to cool down, then automatically goes to OFF state.

---

## Heater Responses

### Status Response (57 bytes)

**Format:**
```
AA 77 02 34 01 [STATE] [POWER] [ERROR] 01 32 00 [VOLTAGE] 00 00 80 00
[TEMP_HI] [TEMP_LO] ... [FAN_HI] [FAN_LO] ... [CHECKSUM]
```

**Key Fields:**
- Byte 5: Current state (see State Machine)
- Byte 6: Current power level (1-10)
- Byte 7: Error code (0 = no error)
- Byte 11: Voltage (×0.1V, e.g., 0x82 = 13.0V)
- Bytes 16-17: Temperature (big-endian, ×0.1°C)
- Bytes 28-29: Fan speed (big-endian, RPM)

**Example (OFF state):**
```
AA 77 02 34 01 00 0A 00 01 32 00 82 00 00 80 0C
02 58 00 00 00 54 00 00 00 00 00 00 00 00 00 00
00 00 00 00 00 00 00 00 00 00 00 00 00 00 24 09
20 10 00 FA 00 00 04 00 8C
```
Decoded: State=OFF, Power=10, Temp=60.0°C, Fan=0 RPM, Voltage=13.0V

**Example (COMBUSTION state):**
```
AA 77 02 34 01 03 0A 00 01 32 00 81 00 00 80 00
06 D6 00 00 00 00 00 00 06 1A 14 03 0F A0 00 00
00 00 00 00 00 00 00 00 00 00 00 00 00 00 24 09
20 10 01 22 00 00 11 00 XX
```
Decoded: State=COMBUSTION, Power=10, Temp=175.0°C, Fan=4000 RPM, Voltage=12.9V

---

## State Machine

### States

| Value | Name         | Description                                    |
|-------|--------------|------------------------------------------------|
| 0x00  | OFF          | Heater is off, safe to start                   |
| 0x01  | GLOW_PREHEAT | Glow plug heating, preparing for ignition      |
| 0x02  | IGNITION     | Fuel pump started, attempting ignition         |
| 0x03  | COMBUSTION   | Normal burning operation                       |
| 0x04  | COOLDOWN     | Stopping, fan running to cool down             |

### State Transitions

```
OFF → GLOW_PREHEAT → IGNITION → COMBUSTION
                                      ↓
                                   COOLDOWN → OFF
```

**Startup Sequence (typical):**
1. **OFF** (60°C, fan 0 RPM)
2. **GLOW_PREHEAT** (45-60°C, fan 0 RPM, ~30 seconds)
   - Glow plug heating combustion chamber
3. **IGNITION** (45-50°C, fan ~1500 RPM, ~30-60 seconds)
   - Fuel pump starts
   - Fan increases for air supply
   - Attempts to ignite fuel
4. **COMBUSTION** (rising to 150-220°C, fan ~4000 RPM)
   - Successful ignition
   - Normal operation
   - Power level controls fuel pump frequency and fan speed

**Shutdown Sequence:**
1. **COMBUSTION** → User sends STOP command
2. **COOLDOWN** (220°C → 60°C, fan high speed, ~4-5 minutes)
   - Fuel pump stops
   - Fan runs at high speed to cool down
   - Cannot restart until cooldown complete
3. **OFF** (60°C, fan 0 RPM)
   - Safe to restart

---

## Error Codes

Located in byte 7 of heater response.

| Code  | Description                          | Action Required              |
|-------|--------------------------------------|------------------------------|
| 0x00  | No error                             | Normal operation             |
| 0x01  | Voltage too low                      | Check battery/power supply   |
| 0x02  | Voltage too high                     | Check power supply regulation|
| 0x03  | Ignition failure                     | Check fuel, glow plug        |
| 0x04  | Flame out during operation           | Check fuel supply            |
| 0x05  | Temperature sensor error             | Check sensor connection      |
| 0x06  | Glow plug error                      | Check glow plug resistance   |
| 0x08  | Fan motor error                      | Check fan motor              |
| 0x09  | Fuel pump error                      | Check pump operation         |

**Error Handling:**
- Heater typically auto-stops on error and enters COOLDOWN
- Error code persists until heater reaches OFF state
- Log all non-zero error codes for diagnostics

---

## Checksum Calculation

**Algorithm:** Simple 8-bit sum

**For Controller Packets (16 bytes):**
```c
uint8_t calculateChecksum(uint8_t* packet) {
    uint8_t sum = 0;
    for (int i = 2; i <= 14; i++) {  // Sum bytes 2-14
        sum += packet[i];
    }
    return sum;  // Return 8-bit result (automatic truncation)
}
```

**For Heater Packets (57 bytes):**
```c
uint8_t calculateChecksum(uint8_t* packet) {
    uint8_t sum = 0;
    for (int i = 2; i <= 55; i++) {  // Sum bytes 2-55
        sum += packet[i];
    }
    return sum;  // Return 8-bit result
}
```

---

## Examples

### Example 1: Start Heater at Power 8

**Sequence:**
```
1. Send START command 3 times (1 second apart):
   AA 66 06 0B 00 00 00 00 08 06 00 00 00 00 00 1F
   AA 66 06 0B 00 00 00 00 08 06 00 00 00 00 00 1F
   AA 66 06 0B 00 00 00 00 08 06 00 00 00 00 00 1F

2. Resume STATUS_REQUEST with power 8:
   AA 66 02 0B 00 00 00 00 08 08 00 00 00 00 00 1D
   (Send every 1 second)

3. Monitor heater response:
   State: OFF → GLOW_PREHEAT → IGNITION → COMBUSTION
```

### Example 2: Change Power from 8 to 5 While Running

**Sequence:**
```
1. Heater running at power 8, in COMBUSTION state

2. Send STATUS_REQUEST with new power level 5:
   AA 66 02 0B 00 00 00 00 05 08 00 00 00 00 00 1A

3. Continue sending with power 5:
   AA 66 02 0B 00 00 00 00 05 08 00 00 00 00 00 1A
   (Every 1 second)

4. Heater response shows power change within 1-2 seconds:
   Byte 6: 0x05 (power level now 5)
   Fan speed decreases accordingly
```

### Example 3: Stop Heater

**Sequence:**
```
1. Send STOP command 3 times (1 second apart):
   AA 66 06 0B 00 00 00 00 0A 05 00 00 00 00 00 20
   AA 66 06 0B 00 00 00 00 0A 05 00 00 00 00 00 20
   AA 66 06 0B 00 00 00 00 0A 05 00 00 00 00 00 20

2. Resume STATUS_REQUEST:
   AA 66 02 0B 00 00 00 00 0A 08 00 00 00 00 00 1F
   (Send every 1 second)

3. Monitor cooldown:
   State: COMBUSTION → COOLDOWN (4-5 minutes) → OFF
   Temperature: 220°C → 60°C
   Fan: High speed throughout cooldown
```

---

## Implementation Notes

### Timing Requirements
- Send STATUS_REQUEST every 1 second (1000ms)
- Send START/STOP commands 3 times (1 second apart) before resuming STATUS_REQUEST
- Heater timeout: If no STATUS_REQUEST received for ~5 seconds, heater may stop

### Half-Duplex Considerations
- TX packets echo back on RX (expected behavior)
- Filter echo packets by checking if packet matches last transmitted packet
- Or ignore 16-byte packets when expecting 57-byte response

### Power Level Behavior
- Power level 1-10 controls:
  - Fuel pump frequency (Hz)
  - Fan speed (RPM)
  - Target temperature
- Higher power = more heat, more fuel, higher fan speed
- Lower power = less heat, less fuel, lower fan speed
- Power can be changed at any time during COMBUSTION

### Error Monitoring
- Check byte 7 of every heater response
- Log all non-zero error codes with timestamp
- Consider auto-stopping on critical errors (0x03, 0x04)

### State Machine Best Practices
- Only send START when state is OFF
- Only send STOP when state is COMBUSTION, IGNITION, or GLOW_PREHEAT
- Do not interrupt COOLDOWN (will complete automatically)
- Wait for OFF state before restarting after error

---

## Protocol Discovery Notes

This protocol was reverse-engineered through:
1. Passive sniffing of original controller communication
2. Analysis of packet structure and checksums
3. Testing command variations to identify byte functions
4. Monitoring state transitions during startup/shutdown
5. Experimenting with power level changes during operation

**Key discovery:** STATUS_REQUEST (0x02) also sets power level via byte 8, enabling power changes during operation without restarting.

---

## References

- Original reverse engineering: 2026-04-02
- Hardware: Vevor 5kW Diesel Air Heater
- Microcontroller: ESP32
- Tools: Logic analyzer, serial sniffer, test firmware

---

*Last Updated: 2026-04-02*
