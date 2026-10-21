# Vevor Heater Serial Protocol - Complete Specification

## Overview

The Vevor diesel heater uses **single-wire serial communication** at 4800 baud with **inverted UART signals**. This is NOT RS485 - it's a simple UART protocol on a shared bus between the control panel and heater unit.

**Key Characteristics:**
- Bidirectional communication on single data wire (Blue wire)
- Controller polls heater every ~1 second (heartbeat)
- Heater responds with status packet
- Communication failure will trigger heater safety shutdown

## Physical Layer

### Wiring (3-wire cable)

| Wire Color | Function | Direction |
|------------|----------|-----------|
| Black | GND | Common ground reference |
| Red | +5V | Powers control panel from heater |
| Blue | Serial Data | Bidirectional UART (half-duplex) |

### Serial Parameters

| Parameter | Value |
|-----------|-------|
| Baud rate | 4800 |
| Data bits | 8 |
| Parity | None |
| Stop bits | 1 |
| Format | 8N1 |
| Signal type | **Inverted UART** |
| Voltage level | 5V (nominal) |
| Timing | Controller sends every ~1 second |

### Signal Inversion

**Critical:** The protocol uses inverted UART signaling.

- Logic 1 (idle/mark) = LOW voltage (~0V)
- Logic 0 (space) = HIGH voltage (~5V)

**ESP32 Support:**
```cpp
Serial2.setRxInvert(true);  // Invert received data
Serial2.setTxInvert(true);  // Invert transmitted data
```

## Packet Structure

### Controller → Heater (16 bytes)

| Byte | Hex | Description | Range/Values |
|------|-----|-------------|--------------|
| 0 | 0xAA | Frame start marker | Fixed |
| 1 | 0x66 | Controller device ID | Fixed |
| 2 | varies | Command byte | 0x02, 0x06 |
| 3 | 0x0B | Frame length (11 bytes payload) | Fixed |
| 4-7 | 0x00 | Reserved | Always zero |
| 8 | varies | Power level | 0x01-0x0A (1-10) |
| 9 | varies | State byte | 0x02, 0x05, 0x06, 0x08 |
| 10-14 | 0x00 | Reserved | Always zero |
| 15 | varies | Checksum | Calculated |

### Heater → Controller (56 bytes)

| Byte(s) | Description | Encoding | Example |
|---------|-------------|----------|---------|
| 0 | Frame start marker | Fixed 0xAA | 0xAA |
| 1 | Heater device ID | Fixed 0x77 | 0x77 |
| 2 | Response command | Fixed 0x06 | 0x06 |
| 3 | Frame length | Fixed 0x33 (51 bytes) | 0x33 |
| 4 | Status flag | 0-1 (last packet: 0) | 0x00 or 0x01 |
| 5 | Current state | State codes | 0x00-0x04 |
| 6 | Current power level | 1-10 | 0x01-0x0A |
| 7 | Error code | Error codes | 0x00-0x09 |
| 8-9 | Running status | Various flags | varies |
| 10 | Reserved | Always 0x00 | 0x00 |
| 11 | Input voltage | Volts × 10 | 132 = 13.2V |
| 12 | Reserved | Always 0x00 | 0x00 |
| 13 | Glow plug current | Amps (direct) | 0-12A |
| 14 | Cooling mode flag | 0 or 1 | 0x00 or 0x01 |
| 15 | Fan voltage/temp proxy | Various | 0-16 |
| 16-17 | Heat exchanger temp | °C × 100 (big-endian) | 0x0BB8 = 30°C |
| 18-19 | Reserved | Always 0x00 | 0x0000 |
| 20-21 | State duration | Seconds (big-endian) | varies |
| 22 | Reserved | Always 0x00 | 0x00 |
| 23 | Pump frequency | Hz × 10 | 51 = 5.1Hz |
| 24-27 | Glow plug telemetry | Various | varies |
| 28-29 | Fan speed | RPM (big-endian) | 0x09C4 = 2500 RPM |
| 30-45 | Reserved | Always 0x00 | 0x00... |
| 46-49 | Fixed constants | Unknown purpose | 35,4,17,35 |
| 50-53 | Reserved/telemetry | Various | varies |
| 54 | Reserved | Always 0x00 | 0x00 |
| 55 | Checksum | Calculated | varies |

## Command Codes (Byte 2)

| Code | Name | Purpose | Direction |
|------|------|---------|-----------|
| 0x02 | STATUS_REQUEST | Poll heater for status | Controller → Heater |
| 0x06 | START_STOP | Control heater power state | Controller → Heater |

## State Codes

### Controller States (Byte 9 - Requested State)

| Code | Name | Description |
|------|------|-------------|
| 0x02 | OFF | Heater is/should be off |
| 0x05 | SET_OFF | Command to turn off |
| 0x06 | START | Starting/startup sequence |
| 0x08 | RUNNING | Stable combustion |

### Heater States (Byte 5 - Current State)

| Code | Name | Description |
|------|------|-------------|
| 0x00 | OFF | Heater stopped |
| 0x01 | GLOW_PREHEAT | Glow plug pre-heating |
| 0x02 | IGNITION | Ignition sequence active |
| 0x03 | COMBUSTION | Stable combustion achieved |
| 0x04 | COOLDOWN | Shutdown/cooling mode |

## Error Codes (Byte 7)

| Code | Display | Description |
|------|---------|-------------|
| 0x00 | - | No error |
| 0x01 | E10 | Startup failure (no ignition) |
| 0x02 | E08 | Low fuel warning |
| 0x03 | E01 | Voltage overrun condition |
| 0x04 | E04 | Unspecified fault |
| 0x05 | E05 | Unspecified fault |
| 0x06 | - | Fuel pump failure |
| 0x07 | - | Fan motor failure |
| 0x08 | E03 | Ignition system fault |
| 0x09 | E05 | Overheat shutdown |

## Power Level (Byte 8 / Byte 6)

- **Range:** 0x01 - 0x0A (decimal 1-10)
- **Function:** Controls fuel pump frequency and fan speed
- **Level 10:** Maximum heat output (~8kW for 8kW model)
- **Level 1:** Minimum heat output (~1kW for 8kW model)
- **Default:** Heater typically starts at level 10, then can be adjusted

## Checksum Algorithms

### Controller Packet Checksum (Byte 15)

Simple additive checksum of payload bytes:

```cpp
uint8_t calculateControllerChecksum(uint8_t *packet) {
    uint8_t checksum = 0;
    for (int i = 2; i <= 14; i++) {  // Sum bytes 2 through 14
        checksum += packet[i];
    }
    return checksum & 0xFF;  // Keep lower 8 bits only
}
```

**Example verification:**
```
Packet: AA 66 06 0B 00 00 00 00 0A 06 00 00 00 00 00 21
Checksum calculation:
  06 + 0B + 00 + 00 + 00 + 00 + 0A + 06 + 00 + 00 + 00 + 00 + 00
  = 0x21 ✓
```

### Heater Packet Checksum (Byte 55)

Sum of all payload bytes (bytes 2 through 54):

```cpp
uint8_t calculateHeaterChecksum(uint8_t *packet) {
    uint8_t checksum = 0;
    for (int i = 2; i < 55; i++) {  // Sum bytes 2 through 54
        checksum += packet[i];
    }
    return checksum & 0xFF;
}
```

## Data Encoding

### Temperature (Bytes 16-17)

- **Format:** 16-bit signed integer, big-endian
- **Encoding:** °C × 100
- **Range:** -127.99°C to +327.67°C

**Examples:**
```cpp
// Encode temperature
int16_t temp_celsius_x100 = 4500;  // 45.00°C
packet[16] = (temp_celsius_x100 >> 8) & 0xFF;  // High byte
packet[17] = temp_celsius_x100 & 0xFF;         // Low byte

// Decode temperature
int16_t raw = (packet[16] << 8) | packet[17];
float temp_celsius = raw / 100.0;

// Examples:
// 0x0BB8 = 3000 = 30.00°C
// 0x1194 = 4500 = 45.00°C
// 0x2EE0 = 12000 = 120.00°C
```

### Voltage (Byte 11)

- **Format:** 8-bit unsigned integer
- **Encoding:** V × 10
- **Range:** 0V to 25.5V

**Examples:**
```cpp
uint8_t voltage_x10 = 132;  // 13.2V
float voltage = voltage_x10 / 10.0;

// 132 = 13.2V
// 120 = 12.0V
// 85 = 8.5V (low voltage)
```

### Current (Byte 13)

- **Format:** 8-bit unsigned integer
- **Encoding:** Direct amperes (0-25A range)
- **Range:** 0A to 25A

**Examples:**
```
25 = 2.5A (glow plug heating)
5 = 0.5A (running)
150 = 15.0A (error/overload)
```

### Fan Speed (Bytes 28-29)

- **Format:** 16-bit unsigned integer, big-endian
- **Encoding:** Direct RPM value
- **Range:** 0 to 6553 RPM

**Examples:**
```cpp
// Encode RPM
uint16_t fan_rpm = 2500;
packet[28] = (fan_rpm >> 8) & 0xFF;  // High byte
packet[29] = fan_rpm & 0xFF;         // Low byte

// Decode RPM
uint16_t fan_rpm = (packet[28] << 8) | packet[29];

// 0x09C4 = 2500 RPM
// 0x1388 = 5000 RPM (error condition)
```

### Pump Frequency (Byte 23)

- **Format:** 8-bit unsigned integer
- **Encoding:** Hz × 10
- **Range:** 0 to 25.5 Hz

**Examples:**
```
51 = 5.1 Hz (power level 5)
10 = 1.0 Hz (minimum)
100 = 10.0 Hz (maximum)
```

## Example Packets

### Example 1: Power ON at Level 10

**Controller → Heater:**
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

**Breakdown:**
- Command: START_STOP (0x06)
- Power Level: 10 (0x0A)
- State: START (0x06)
- Checksum: 0x06 + 0x0B + 0x0A + 0x06 = 0x21 ✓

### Example 2: Power Level 5, Running

**Controller → Heater:**
```
AA 66 06 0B 00 00 00 00 05 08 00 00 00 00 00 1E
                        │  │                 │
                        │  │                 └─ Checksum (0x1E)
                        │  └─────────────────── State: RUNNING (0x08)
                        └────────────────────── Power level: 5 (0x05)
```

**Breakdown:**
- Command: START_STOP (0x06)
- Power Level: 5 (0x05)
- State: RUNNING (0x08)
- Checksum: 0x06 + 0x0B + 0x05 + 0x08 = 0x1E ✓

### Example 3: Power OFF

**Controller → Heater:**
```
AA 66 06 0B 00 00 00 00 01 02 00 00 00 00 00 14
                        │  │                 │
                        │  │                 └─ Checksum (0x14)
                        │  └─────────────────── State: OFF (0x02)
                        └────────────────────── Power level: 1 (minimum)
```

**Breakdown:**
- Command: START_STOP (0x06)
- Power Level: 1 (0x01)
- State: OFF (0x02)
- Checksum: 0x06 + 0x0B + 0x01 + 0x02 = 0x14 ✓

### Example 4: Heater Response (Running at Level 10)

**Heater → Controller:**
```
AA 77 06 33 00 06 0A 00 00 00 00 84 00 19 00 00
11 94 00 00 00 00 00 0A 00 00 00 00 09 C4 00 00
00 00 00 00 00 00 00 00 00 00 00 00 00 00 00 00
00 00 00 00 00 00 00 00
```

**Decoded data:**
- Frame start: 0xAA
- Device ID: 0x77 (Heater)
- Command: 0x06 (Response)
- State: 0x06 (START/COMBUSTION)
- Power level: 10 (0x0A)
- Error: 0x00 (None)
- Voltage: 0x84 = 132 = 13.2V
- Glow current: 0x19 = 25 = 2.5A
- Temperature: 0x1194 = 4500 = 45.00°C
- Fan speed: 0x09C4 = 2500 RPM
- Pump freq: 0x0A = 10 = 1.0 Hz

## ESP32 Implementation

### Hardware Setup

```cpp
// Pin definitions
#define SERIAL_RX_PIN 16  // GPIO16
#define SERIAL_TX_PIN 17  // GPIO17
// Note: 470Ω resistor between GPIO17 and level shifter!

// Initialize Serial2
void setupSerial() {
    Serial2.begin(4800, SERIAL_8N1, SERIAL_RX_PIN, SERIAL_TX_PIN);

    // CRITICAL: Enable signal inversion
    Serial2.setRxInvert(true);
    Serial2.setTxInvert(true);
}
```

### Sending Commands

```cpp
// Build and send command packet
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

// Usage examples:
sendHeaterCommand(10, 0x06);  // Turn ON at level 10
delay(1000);
sendHeaterCommand(10, 0x08);  // Confirm RUNNING at level 10
delay(5000);
sendHeaterCommand(5, 0x08);   // Reduce to level 5
delay(5000);
sendHeaterCommand(1, 0x02);   // Turn OFF
```

### Receiving Status

```cpp
// Buffer for receiving heater packets
#define RX_BUFFER_SIZE 256
uint8_t rxBuffer[RX_BUFFER_SIZE];
int rxBufferIndex = 0;
unsigned long lastRxTime = 0;
#define RX_TIMEOUT 100  // ms to detect end of packet

void loop() {
    unsigned long currentTime = millis();

    // Read incoming bytes
    if (Serial2.available()) {
        uint8_t inByte = Serial2.read();
        lastRxTime = currentTime;

        if (rxBufferIndex < RX_BUFFER_SIZE) {
            rxBuffer[rxBufferIndex++] = inByte;
        }
    }

    // Check for packet timeout (100ms with no new bytes = end of packet)
    if (rxBufferIndex > 0 && (currentTime - lastRxTime) > RX_TIMEOUT) {
        processPacket(rxBuffer, rxBufferIndex);
        rxBufferIndex = 0;  // Reset for next packet
    }
}

void processPacket(uint8_t *buffer, int length) {
    // Controller packet (16 bytes)
    if (length == 16 && buffer[0] == 0xAA && buffer[1] == 0x66) {
        uint8_t command = buffer[2];
        uint8_t powerLevel = buffer[8];
        uint8_t state = buffer[9];
        uint8_t checksum = buffer[15];

        // Verify checksum
        uint8_t calculatedChecksum = 0;
        for (int i = 2; i <= 14; i++) {
            calculatedChecksum += buffer[i];
        }

        if ((calculatedChecksum & 0xFF) == checksum) {
            Serial.printf("Controller: Power=%d, State=0x%02X\n", powerLevel, state);
        }
    }

    // Heater packet (56 bytes)
    else if (length == 56 && buffer[0] == 0xAA && buffer[1] == 0x77) {
        uint8_t state = buffer[5];
        uint8_t powerLevel = buffer[6];
        uint8_t errorCode = buffer[7];
        uint8_t voltage_x10 = buffer[11];
        uint8_t glowCurrent = buffer[13];

        // Temperature (bytes 16-17, big-endian, × 100)
        int16_t temp_x100 = (buffer[16] << 8) | buffer[17];
        float temperature = temp_x100 / 100.0;

        // Fan speed (bytes 28-29, big-endian)
        uint16_t fanRPM = (buffer[28] << 8) | buffer[29];

        // Voltage
        float voltage = voltage_x10 / 10.0;

        Serial.printf("Heater: State=0x%02X, Power=%d, Error=0x%02X, Temp=%.1f°C, Fan=%d RPM, Voltage=%.1fV\n",
                      state, powerLevel, errorCode, temperature, fanRPM, voltage);
    }
}
```

### Complete Control Example

```cpp
// Turn heater on and monitor status
void heaterOnSequence() {
    // 1. Send START command at level 10
    sendHeaterCommand(10, 0x06);
    Serial.println("Sent: START at level 10");

    // 2. Wait for heater to respond
    delay(1000);

    // 3. Confirm RUNNING state
    sendHeaterCommand(10, 0x08);
    Serial.println("Sent: RUNNING at level 10");

    // 4. Monitor status (heater will respond to each command)
    // Response packets will be received and processed by processPacket()
}

// Turn heater off
void heaterOffSequence() {
    // 1. Send OFF command
    sendHeaterCommand(1, 0x02);
    Serial.println("Sent: OFF");

    // 2. Heater enters cooldown mode
    // Fan continues running for 3-5 minutes
    // Monitor with current sensor or status packets
}
```

## Communication Timing

### Heartbeat

- Controller sends packets every **~1 second** (heartbeat)
- Heater responds immediately to each packet
- If controller stops sending, heater will shut down after timeout (~3-5 seconds)

### Important Timing Rules

1. **Always maintain heartbeat** - Send status request every second even if no changes
2. **State transitions** - Allow heater time to transition (e.g., OFF → START takes 1-3 minutes)
3. **Cooldown period** - Heater must complete cooldown (3-5 min) before restart
4. **Response timeout** - Expect heater response within 100ms

## State Machine Sequences

### Startup Sequence

```
Controller: Send START (0x06) at desired power level
    ↓
Heater: State 0x01 (GLOW_PREHEAT) - Glow plug heating (30-180 seconds)
    ↓
Heater: State 0x02 (IGNITION) - Fuel pump starts, ignition (10-30 seconds)
    ↓
Heater: State 0x03 (COMBUSTION) - Stable combustion achieved
    ↓
Controller: Send RUNNING (0x08) to maintain operation
```

### Shutdown Sequence

```
Controller: Send OFF (0x02)
    ↓
Heater: Fuel pump stops immediately
    ↓
Heater: State 0x04 (COOLDOWN) - Fan continues running
    ↓
Heater: Fan gradually slows (3-5 minutes)
    ↓
Heater: State 0x00 (OFF) - Complete stop
```

### Power Level Change

```
Controller: Send new power level with RUNNING state (0x08)
    ↓
Heater: Adjusts pump frequency and fan speed
    ↓
Heater: Confirms new power level in response packet
```

## Troubleshooting

### No Response from Heater

**Check:**
- GND connection (Black wire) - CRITICAL
- Signal inversion enabled (`setRxInvert(true)`, `setTxInvert(true)`)
- Level shifter powered correctly (3.3V and 5V)
- 470Ω resistor present on TX line
- Baud rate correct (4800)

### Garbled Data

**Try:**
- Toggle signal inversion settings
- Verify level shifter wiring
- Check for loose connections
- Measure voltage levels on Blue wire

### Heater Doesn't Respond to Commands

**Check:**
- Checksum calculation correct
- Packet format exactly 16 bytes
- State transitions valid (can't go directly from OFF to RUNNING)
- Heartbeat maintained (send every second)

### Unexpected Shutdowns

**Causes:**
- Controller stopped sending heartbeat
- Communication loss detected
- Error condition triggered
- Low voltage condition

## References

- **ESPHome Vevor Heater:** [github.com/zatakon/esphome-vevor-heater](https://github.com/zatakon/esphome-vevor-heater)
- **Protocol Reverse Engineering:** [github.com/zatakon/vevor_heater_control](https://github.com/zatakon/vevor_heater_control)
- **BluetoothHeater Project:** [gitlab.com/mrjones.id.au/bluetoothheater](https://gitlab.com/mrjones.id.au/bluetoothheater)
- **Hackaday Project:** [hackaday.io/project/195170-vevor-diesel-heater-protocol](https://hackaday.io/project/195170-vevor-diesel-heater-protocol)

## License

This documentation is based on community reverse-engineering efforts. Use at your own risk. Vevor is a trademark of its respective owner.
