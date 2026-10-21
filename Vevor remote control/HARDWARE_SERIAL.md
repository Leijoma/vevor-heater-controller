# Hardware Guide: Serial Connection

Complete hardware setup guide for connecting ESP32 to Vevor heater via serial protocol.

## Overview

This guide documents the proven hardware setup using:
- ESP32 development board
- Bidirectional level shifter (3.3V ↔ 5V)
- 470Ω resistor for half-duplex TX protection
- Simple wiring, no transistors required

**Tested and verified** in the [Vevor-Serial-Transmitter](../Vevor-Serial-Transmitter/) project.

## Bill of Materials

| Component | Specification | Quantity | Purpose | Approx Cost |
|-----------|---------------|----------|---------|-------------|
| ESP32 Dev Board | Any variant with GPIO 16/17 | 1 | Main controller | $5-10 |
| Bidirectional Level Shifter | 4-channel, 3.3V ↔ 5V | 1 | Voltage translation | $1-2 |
| 470Ω Resistor | 1/4W, 5% tolerance | 1 | TX current limiting | $0.05 |
| Jumper Wires | Male-to-male | ~10 | Connections | $1 |
| Breadboard | (for prototyping) | 1 | Testing | $2 |
| 3-wire Cable | Or use heater cable | 1 | Heater connection | Included |
| **Power Supply Components (Optional):** | | | | |
| 100µF Electrolytic Capacitor | 16V or higher | 1 | Power filtering | $0.10 |
| 1N5819 Schottky Diode | 40V, 1A | 1-2 | Reverse protection / OR-ing | $0.20 |
| Fuse Holder + Fuse | 500mA slow-blow | 1 | Overcurrent protection | $0.50 |

**Total (without power components):** ~$9-15
**Total (with power protection):** ~$10-16

## Heater Cable

The heater uses a **3-wire cable** with the following connections:

| Wire Color | Function | Voltage | Direction |
|------------|----------|---------|-----------|
| **Black** | Ground (GND) | 0V | Reference |
| **Red** | Power (+5V) | 5V | Powers control panel |
| **Blue** | Serial Data | 0-5V | Bidirectional |

**Note:** Voltage levels are ~5V but noisy. Level shifter is **required** to protect ESP32 (3.3V max).

## Wiring Diagram

### Complete Connection

```
ESP32                                  Level Shifter           Heater Cable
─────────────────────────────────────────────────────────────────────────

GPIO 16 (RX) ─────────┬──────────→  LV1  →  HV1  ────────→  Blue (Data)
                      │
GPIO 17 (TX) ─[470Ω]──┘

3.3V ─────────────────────────────→  LV (VCC)
GND ──────────────────────────────→  LV GND

                                      HV (VCC)  ←───────────  Red (+5V)
                                      HV GND    ←───────────  Black (GND)

GND (ESP32) ──────────────────────────────────────────────→  Black (GND)
```

### Key Points

1. **RX (GPIO16)** connects directly to level shifter LV1
2. **TX (GPIO17)** connects through **470Ω resistor** to level shifter LV1 (same channel as RX!)
3. **Both RX and TX** share the same level shifter channel (bidirectional half-duplex)
4. **ESP32 GND** must connect to heater Black wire (common ground reference)
5. **Level shifter HV side** is powered by heater Red wire (+5V)

## Detailed Wiring Steps

### Step 1: Level Shifter LV (Low Voltage) Side

Connect to ESP32 (3.3V side):

```
Level Shifter LV Side → ESP32
─────────────────────────────
LV (VCC) → 3.3V pin
GND      → GND pin
LV1      → GPIO 16 (RX) directly
LV1      → GPIO 17 (TX) via 470Ω resistor
```

**Important:**
- LV1 channel is used for both RX and TX
- 470Ω resistor goes in series with TX line BEFORE level shifter
- RX has no resistor (direct connection)

### Step 2: Level Shifter HV (High Voltage) Side

Connect to heater cable (5V side):

```
Level Shifter HV Side → Heater Cable
────────────────────────────────────
HV (VCC) → Red wire (+5V)
GND      → Black wire (GND)
HV1      → Blue wire (Data)
```

### Step 3: Ground Reference

**CRITICAL:** ESP32 ground must be connected to heater ground.

```
ESP32 GND ──────→ Level Shifter LV GND
                        ↓
                  Level Shifter HV GND
                        ↓
                  Black wire (Heater GND)
```

Without proper ground reference, communication will fail!

### Step 4: 470Ω Resistor Placement

The resistor goes between GPIO17 and the level shifter:

```
ESP32                     Level Shifter
─────                     ─────────────
GPIO 17 ──[470Ω]────────→ LV1
```

**Why 470Ω?**
- Limits current when both RX and TX are active
- Protects GPIO pins from conflicts on half-duplex bus
- Allows RX to "win" during simultaneous activity
- Doesn't affect signal quality at 4800 baud

## Breadboard Layout Example

For prototyping, here's a suggested breadboard layout:

```
       ESP32              Breadboard               Level Shifter

 3.3V ────────────────→ Red rail (+)
 GND  ────────────────→ Blue rail (-)

GPIO 16 ──────────────→ Row 10 ───────────────→ LV1
GPIO 17 ──────────────→ Row 12
                        Row 12 ─[470Ω]─ Row 10

                        Red rail (+) ──────────→ LV (VCC)
                        Blue rail (-) ─────────→ LV GND

                        HV1 ←─────────────────── Blue wire
                        HV (VCC) ←────────────── Red wire (+5V)
                        HV GND ←──────────────── Black wire (GND)
                        Blue rail (-) ←─────────Black wire (GND)
```

## Level Shifter Selection

### Recommended Module

**4-Channel Bidirectional Level Shifter**
- Based on BSS138 MOSFETs
- Voltage range: 1.8V-5.5V
- Common models: Adafruit, SparkFun, generic Chinese modules
- Auto-direction sensing (no OE pin needed)

### Features Needed

✅ **Bidirectional** - Must support data flow in both directions
✅ **3.3V ↔ 5V** - Voltage translation range
✅ **4 channels** - Only need 1 for serial, others for future expansion
❌ **NOT** - TXS0108E (has issues with UART at 4800 baud)
❌ **NOT** - Simple resistor divider (RX works, but TX won't work reliably)

## Signal Inversion Configuration

The Vevor protocol uses **inverted UART signals**. ESP32 hardware UART supports this natively.

### In Code

```cpp
#define SERIAL_RX_PIN 16
#define SERIAL_TX_PIN 17

void setupSerial() {
    // Initialize Serial2
    Serial2.begin(4800, SERIAL_8N1, SERIAL_RX_PIN, SERIAL_TX_PIN);

    // CRITICAL: Enable signal inversion
    Serial2.setRxInvert(true);   // Invert received data
    Serial2.setTxInvert(true);   // Invert transmitted data

    Serial.println("Serial initialized: 4800 baud, inverted signals");
}
```

### What Signal Inversion Means

**Standard UART:**
- Idle = HIGH (~3.3V or ~5V)
- Start bit = LOW
- Data bits = HIGH or LOW
- Stop bit = HIGH

**Inverted UART (Vevor protocol):**
- Idle = LOW (~0V)
- Start bit = HIGH
- Data bits = inverted (HIGH = 0, LOW = 1)
- Stop bit = LOW

The ESP32 handles this automatically when inversion is enabled!

## Testing Procedure

### Phase 1: Visual Inspection

Before powering on:

1. ✓ Level shifter LV side connected to 3.3V and GND
2. ✓ Level shifter HV side connected to Red (+5V) and Black (GND)
3. ✓ GPIO16 connected to LV1 (no resistor)
4. ✓ GPIO17 connected through 470Ω to LV1
5. ✓ LV1 connected to HV1
6. ✓ HV1 connected to Blue wire
7. ✓ Common ground between ESP32 and heater

### Phase 2: Power-On Test

1. Connect heater cable (or power level shifter HV side from separate 5V source)
2. Power ESP32 via USB
3. Measure voltages:
   - Level shifter LV: ~3.3V
   - Level shifter HV: ~5V
   - Blue wire (idle): ~0V or ~5V (depends on what's transmitting)

### Phase 3: RX Test (Listen Mode)

Upload test firmware:

```cpp
void setup() {
    Serial.begin(115200);
    Serial2.begin(4800, SERIAL_8N1, 16, 17);
    Serial2.setRxInvert(true);
    Serial2.setTxInvert(true);
    Serial.println("Listening for packets...");
}

void loop() {
    if (Serial2.available()) {
        uint8_t byte = Serial2.read();
        Serial.printf("%02X ", byte);
    }
}
```

**Expected result:**
- With control panel connected and sending: See hex bytes streaming
- Should see patterns like `AA 66 06 0B...` every second
- If garbage or nothing: Check signal inversion, GND connection, wiring

### Phase 4: TX Test (Transmit Mode)

Upload transmit test firmware from [Vevor-Serial-Transmitter](../Vevor-Serial-Transmitter/) project.

**Expected result:**
- Control panel display should react to ESP32 responses
- Or use second ESP32 running sniffer to verify packets sent

### Phase 5: Full Bidirectional Test

Run full heater control:
1. Send START command
2. Receive heater status responses
3. Verify bidirectional communication working

## Troubleshooting

### No Data Received

**Check:**
- ✓ GND connection between ESP32 and heater (CRITICAL!)
- ✓ Level shifter powered on both sides
- ✓ GPIO16 connected to LV1
- ✓ HV1 connected to Blue wire
- ✓ Signal inversion enabled: `Serial2.setRxInvert(true)`

**Try:**
- Swap signal inversion (try `false` instead of `true`)
- Check level shifter with multimeter (should see ~3.3V on LV, ~5V on HV)
- Verify heater is powered and control panel is active

### Garbled Data / Random Bytes

**Likely causes:**
- Signal inversion wrong (try toggling)
- Baud rate incorrect (must be exactly 4800)
- Level shifter not working (replace or check wiring)
- Ground loop or noise (add 0.1µF capacitor between Blue wire and GND)

**Try:**
- Toggle `setRxInvert()` between true/false
- Measure Blue wire with oscilloscope (should see ~5V square waves at 4800 baud)
- Move ESP32 further from heater (electrical noise)

### Commands Sent But Heater Doesn't Respond

**Check:**
- ✓ TX signal inversion: `Serial2.setTxInvert(true)`
- ✓ 470Ω resistor present on TX line
- ✓ GPIO17 connected through resistor to LV1
- ✓ Checksum calculated correctly
- ✓ Packet exactly 16 bytes

**Try:**
- Run sniffer on second ESP32 to verify TX packets appear on bus
- Toggle `setTxInvert()` between true/false
- Verify level shifter is bidirectional (not unidirectional)
- Check 470Ω resistor value with multimeter

### Heater Shuts Down Unexpectedly

**Causes:**
- Heartbeat not maintained (must send packet every ~1 second)
- Communication lost (check wiring)
- Error condition triggered (check error codes in status response)
- Power supply issue (check voltage on Red wire)

**Solution:**
- Implement continuous heartbeat in code
- Add watchdog to detect communication loss
- Monitor heater error codes

### ESP32 Keeps Resetting

**Causes:**
- Brown-out from power supply (ESP32 drawing too much current)
- Level shifter shorted or miswired
- No ground connection causing voltage imbalance

**Solution:**
- Use USB power supply with adequate current (500mA+)
- Check for shorts on breadboard
- Verify all grounds connected properly

## Alternative Approaches

### Option 1: TX Gate Transistor (From BluetoothHeater Project)

Instead of 470Ω resistor, use NPN transistor to completely disconnect TX when not transmitting.

See [TX_GATE_WIRING.md](../Vevor-Serial-Transmitter/TX_GATE_WIRING.md) in Vevor-Serial-Transmitter project for detailed schematic.

**Pros:**
- Complete electrical isolation when RX
- Cleaner signal

**Cons:**
- More complex (adds transistor + resistors)
- More components
- 470Ω approach works fine

### Option 2: Software TX Gating

Switch GPIO17 pinMode between INPUT (high-Z) and OUTPUT.

**Pros:**
- No resistor needed
- Software controlled

**Cons:**
- More complex code
- Must reinitialize Serial2 each time
- 470Ω approach is simpler

### Recommendation

**Use the 470Ω resistor approach** (as documented above):
- Simplest hardware
- Proven to work
- Reliable
- Easy to troubleshoot

## Permanent Installation

Once tested on breadboard, create permanent wiring:

### Option 1: Solder to Protoboard

1. Transfer components from breadboard to protoboard
2. Solder all connections
3. Use heat shrink tubing for insulation
4. Label wires clearly
5. Mount in enclosure with strain relief

### Option 2: Custom PCB

For clean installation, consider designing a simple PCB with:
- ESP32 module
- Level shifter circuit
- 470Ω resistor
- Screw terminals for heater cable
- LED indicators
- Button footprints for future UI

### Mounting Considerations

- Keep away from heater hot air outlet
- Allow ventilation for ESP32 (gets warm during WiFi use)
- Protect from moisture (diesel heaters produce condensation)
- Cable strain relief (heater vibrates)
- Access to USB port for programming/debugging

## Power Supply Options

### Option 1: USB Power (Development/Testing)

**Recommended for:**
- Initial testing and development
- Programming/debugging
- Troubleshooting

**Advantages:**
- Easy to connect/disconnect
- Stable, clean power
- Convenient for development

**Disadvantages:**
- Requires USB cable to computer or power adapter
- Not suitable for permanent installation
- Extra cable management

### Option 2: Heater 5V Supply (Production)

**Recommended for:**
- Permanent installation
- Standalone operation
- Clean integration

The heater provides **5V on the Red wire** which can power the ESP32 directly.

#### Power Specifications

**Heater 5V Output:**
- Voltage: +5V (typically 4.8-5.2V)
- Current capacity: ~500mA (varies by model)
- Noise: Moderate (switching regulator noise)
- Load: Control panel (~50-100mA) + ESP32

**ESP32 Power Requirements:**
- Operating voltage: 5V input to dev board (regulated to 3.3V internally)
- Current draw:
  - Idle: ~80mA
  - WiFi active: 160-260mA
  - WiFi transmitting: Up to 400mA (short bursts)
  - Peak: ~500mA
- **Recommended supply:** 500mA minimum, 1A preferred

#### Power Connection

**Simple Connection (if heater has capacity):**

```
Heater Red Wire (+5V) ──────→ ESP32 VIN or 5V pin
Heater Black Wire (GND) ─────→ ESP32 GND
```

**Important Notes:**
- Most ESP32 dev boards have onboard 5V→3.3V regulator
- Connect heater 5V to **VIN** or **5V** pin (NOT 3.3V pin!)
- Ensure common ground connection

#### Power Supply Validation

**Before permanent connection, test the heater 5V supply:**

1. **Measure voltage with multimeter:**
   - Red wire to Black wire
   - Should read 4.8-5.2V
   - Check with heater both ON and OFF

2. **Check voltage stability:**
   - Monitor during heater startup (high current draw)
   - Should not drop below 4.5V
   - Large voltage drops indicate insufficient capacity

3. **Test with ESP32:**
   - Connect temporarily
   - Monitor ESP32 brownout
   - Test WiFi functionality
   - Run for 1 hour minimum

#### Recommended Power Setup

**For Most Reliable Operation:**

```
Heater Cable                    Protection              ESP32
─────────────────────────────────────────────────────────────
Red (+5V)  ────→ [Fuse 500mA] ─→ [Schottky Diode] ─→ VIN
                                  ↓
                            [100µF Cap]
                                  ↓
Black (GND) ────────────────────────────────────────→ GND
```

**Components:**
- **Fuse:** 500mA slow-blow (protects heater supply)
- **Schottky Diode:** 1N5819 (reverse polarity protection)
- **Capacitor:** 100µF electrolytic (filter noise)

#### Power-On Sequencing

**Heater Power-Up:**
1. Heater powers on → 5V supply activates
2. ESP32 receives power and boots (~2-3 seconds)
3. ESP32 connects to WiFi (~3-5 seconds)
4. Serial communication starts
5. Total boot time: ~5-10 seconds

**Considerations:**
- ESP32 may reboot if heater has voltage dips during startup
- Add capacitor to help with voltage stability
- Consider delayed start for ESP32 if needed

#### Alternative: Separate 5V Supply

**If heater 5V supply is insufficient or unstable:**

```
Options:
1. USB power adapter (5V 1A) powered separately
2. Buck converter from 12V vehicle/boat power
3. Dedicated 5V regulator from higher voltage
```

**Advantages:**
- Independent power (no heater interference)
- Cleaner, more stable voltage
- Higher current capacity

**Disadvantages:**
- Requires additional power source
- More complex wiring
- Extra components

### Option 3: Dual Power (Best of Both)

**Schottky diode OR-ing for automatic fallback:**

```
                  Diode 1N5819
USB 5V ───────→ [|>|] ───┐
                          ├──→ ESP32 VIN
Heater 5V ─────→ [|>|] ───┘
                  Diode 1N5819
```

**Benefits:**
- USB power during development
- Heater power in production
- Automatic switching (higher voltage wins)
- No manual changes needed

**How it works:**
- Schottky diodes prevent backfeeding
- Whichever source has higher voltage powers ESP32
- ~0.3V drop per diode (acceptable)

### Power Monitoring

**Add to code for diagnostics:**

```cpp
// Read ESP32 supply voltage (if available on your board)
#ifdef ESP32
uint32_t getVoltage() {
    // Some ESP32 boards don't have this feature
    return 0;  // Placeholder
}
#endif

// Monitor brownout events
void setup() {
    // Enable brownout detector
    // ESP32 will reset if voltage drops below ~2.8V (on 3.3V rail)

    Serial.println("Monitoring power supply...");
}

void loop() {
    // Check for brownouts in boot count
    if (esp_reset_reason() == ESP_RST_BROWNOUT) {
        Serial.println("⚠️  Warning: Brownout detected - check power supply!");
    }
}
```

### Power Budget

**Total current draw estimation:**

| Component | Current (mA) | Notes |
|-----------|-------------|-------|
| ESP32 (idle) | 80 | Baseline |
| ESP32 (WiFi active) | 160-260 | Average |
| ESP32 (WiFi TX peak) | 400-500 | Short bursts |
| Level shifter | <1 | Negligible |
| LEDs (4x @ 10mA each) | 40 | If using physical UI |
| OLED display | 20-30 | If added later |
| **Total (no UI)** | **~300mA avg** | **500mA peak** |
| **Total (with UI)** | **~400mA avg** | **600mA peak** |

**Heater 5V supply must provide at least 500mA continuous for v3.0, 700mA for v4.0 with UI.**

### Power Supply Troubleshooting

**ESP32 keeps resetting:**
- Brownout due to voltage drops
- Add 100µF+ capacitor
- Check fuse not limiting current
- Consider separate power supply

**WiFi won't connect or is unstable:**
- Insufficient current during WiFi TX
- Add capacitor
- Reduce WiFi TX power in code:
  ```cpp
  WiFi.setTxPower(WIFI_POWER_15dBm);  // Reduce from 20dBm default
  ```

**Voltage drops during heater startup:**
- Normal - heater draws high current initially
- Add larger capacitor (470µF or 1000µF)
- Consider separate power supply
- Delay ESP32 serial communication until heater stable

### Recommendations

**For v3.0 (Serial Communication):**
- ✅ **Use heater 5V supply** if testing shows it's stable
- ✅ **Add 100µF capacitor** minimum for filtering
- ✅ **Add fuse** for protection
- ⚠️ **Test thoroughly** before permanent installation

**For v4.0+ (Physical UI with LEDs):**
- Consider separate 5V supply if heater supply limited
- Budget for additional ~100mA for LEDs/buzzer
- May need 1A supply total

**For Development:**
- Use USB power for convenience
- Switch to heater power for final testing
- Or use dual-power setup with diodes

## Pin Summary

| Function | ESP32 Pin | Connection | Notes |
|----------|-----------|------------|-------|
| Serial RX | GPIO 16 | Level shifter LV1 | Direct connection |
| Serial TX | GPIO 17 | Level shifter LV1 via 470Ω | Current limiting |
| Power | 3.3V | Level shifter LV VCC | From ESP32 regulator |
| Ground | GND | Level shifter LV GND + Heater Black | Common reference |
| OneWire (temp sensors) | GPIO 18 | DS18B20 data | Existing v2.0 |
| Current sensor | GPIO 34 | WCS1800 analog out | Existing v2.0 |
| RF (optional backup) | GPIO 4 | RF transmitter | Existing v2.0 |

**GPIO pins remaining:**
- GPIO 2, 5, 13, 14, 15, 19, 21, 22, 23, 25, 26, 27, 32, 33
- Available for physical UI (buttons, LEDs, display, encoder)

## Safety Notes

⚠️ **Electrical Safety:**
- Do not connect ESP32 3.3V pins directly to 5V heater signals!
- Always use level shifter for voltage translation
- Verify all connections before applying power
- Use proper gauge wire for current capacity

⚠️ **Heater Safety:**
- Test control thoroughly before unattended operation
- Diesel heaters produce CO - ensure proper ventilation
- Have CO detector installed
- Understand emergency shutdown procedures
- Heater safety features (flame sensor, overheat) remain active

⚠️ **Installation:**
- Marine/boat installations: Use marine-grade components
- Secure all connections (vibration resistant)
- Waterproof enclosure if needed
- Keep away from hot surfaces

## Next Steps

Once hardware is built and tested:

1. ✅ **Verify RX** - Can receive controller packets
2. ✅ **Verify TX** - Can send commands to heater
3. ✅ **Bidirectional** - Both work simultaneously
4. → **Integration** - Add serial control to main controller (see [INTEGRATION_PLAN.md](INTEGRATION_PLAN.md))
5. → **Web Interface** - Update with real heater data
6. → **Physical UI** - Design and add buttons/display (see [HARDWARE_UI.md](HARDWARE_UI.md))

## References

- **Protocol Specification:** [SERIAL_PROTOCOL.md](SERIAL_PROTOCOL.md)
- **Testing Project:** [Vevor-Serial-Transmitter](../Vevor-Serial-Transmitter/)
- **Sniffer Project:** [Vevor-Serial-Sniffer](../Vevor-Serial-Sniffer/)
- **Level Shifter Guide:** [SparkFun Tutorial](https://learn.sparkfun.com/tutorials/bi-directional-logic-level-converter-hookup-guide)

## Revisions

- **v1.0** - Initial documentation with 470Ω resistor approach
- **Hardware verified** - Tested in Vevor-Serial-Transmitter project
