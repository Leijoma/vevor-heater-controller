# WCS1800 Current Sensor Test with Voltage Divider

Quick test project to verify WCS1800 current sensor with voltage divider protection for ESP32.

## Purpose

Test and calibrate the WCS1800 Hall-effect current sensor with the required voltage divider circuit **before** integrating it into the main heater controller.

## Hardware Required

1. **ESP32 Development Board**
2. **WCS1800 Hall Current Sensor** (0-35A range)
3. **2x 10kΩ Resistors** (for voltage divider - CRITICAL!)
4. **Breadboard and jumper wires**
5. **Power cable to test** (heater power cable or similar)

## ⚠️ CRITICAL: Voltage Divider Circuit

**YOU MUST BUILD THIS CIRCUIT!** Without it, you'll damage your ESP32!

```
WCS1800 Pinout:
┌─────────────────┐
│                 │
│   WCS1800       │
│                 │
├─────────────────┤
│ VCC  GND  OUT   │
└──┬───┬────┬─────┘
   │   │    │
   │   │    └─────→ [10kΩ resistor] → ESP32 GPIO34
   │   │                               └─ [10kΩ resistor] → GND
   │   │
   5V  GND
```

### Why Voltage Divider is Needed:
- **WCS1800 output:** 0.5V - 4.5V (at 5V supply)
- **ESP32 ADC max:** 3.3V
- **Voltage divider:** Reduces output by 50% → 0.25V - 2.25V (safe!)

### Wiring Steps:

1. **Power WCS1800:**
   - VCC → 5V (ESP32 VIN or USB power)
   - GND → GND

2. **Build voltage divider:**
   - WCS1800 OUT → First 10kΩ resistor → Junction point
   - Junction point → ESP32 GPIO34
   - Junction point → Second 10kΩ resistor → GND

3. **Verify:**
   - Measure voltage at GPIO34 with multimeter
   - With no current: Should be ~1.25V (half of 2.5V zero point)
   - Should NEVER exceed 3.3V

## Building and Running

```bash
cd "WCS1800-CurrentSensor-Test"
pio run --target upload
pio device monitor
```

## Test Procedure

### Step 1: Zero Current Test (No Load)
```
1. Upload code to ESP32
2. Open serial monitor (115200 baud)
3. Do NOT pass any cable through sensor yet
4. Observe readings - should be close to 0A
5. Note: Small offset (±0.1-0.3A) is normal sensor noise
```

**Expected output:**
```
Current: 0.02 A  │  Raw ADC: 2047  │  ESP32 V: 1.250 V  │  WCS1800 V: 2.500 V
```

### Step 2: Heater Current Test
```
1. Pass heater power cable through WCS1800 sensor opening
   - Only ONE wire should pass through (not entire cable bundle)
   - Direction matters - if reading is negative, flip cable direction

2. Turn heater OFF - verify reading is still close to 0A

3. Turn heater ON (using original remote)

4. Watch current readings:
   - Startup (glow plug): 8-12A expected
   - Normal running: 1-6A depending on power level
   - Shutdown (fan only): 0.5-1.5A
   - OFF: 0A
```

### Step 3: Verify Calibration
```
1. Press 'c' in serial monitor to show calibration table
2. Compare measured current to expected values
3. If readings are consistently off by same percentage:
   - Measure WCS1800 supply voltage (should be 5.0V)
   - Measure resistor values (should be 10kΩ each)
   - Adjust CURRENT_SENSOR_SENSITIVITY if needed
```

## Serial Commands

| Command | Function |
|---------|----------|
| `z` | **Zero calibration** - Remove all cables, press 'z' to calibrate zero point |
| `s` | **Sensitivity calibration** - With known current source (PSU/multimeter) |
| `l` | **Start/Stop logging mode** - Profile heater current over time (CSV output) |
| `r` | Reset min/max statistics |
| `c` | Show calibration table and troubleshooting |

## Understanding the Display

### Normal Mode (scrolling output):
```
Current:    2.45 A │ ADC: 1876 │ ESP32: 1.514 V │ WCS1800: 3.028 V │ Min:    0.12 A │ Max:   12.34 A
Current:    2.47 A │ ADC: 1880 │ ESP32: 1.517 V │ WCS1800: 3.034 V │ Min:    0.12 A │ Max:   12.34 A
```

| Field | Description |
|-------|-------------|
| **Current** | Calculated current in Amps (calibrated) |
| **ADC** | ESP32 ADC reading (0-4095) |
| **ESP32** | Voltage at GPIO34 (after divider, 0-3.3V) |
| **WCS1800** | Calculated WCS1800 output voltage (0-5V) |
| **Min/Max** | Statistics since last reset |

### Logging Mode (CSV format):
```
Time(s),Current(A),WCS1800_V,ESP32_V,ADC_Raw
0,0.05,1.934,0.967,1200
1,0.03,1.932,0.966,1198
2,10.52,2.636,1.318,1635
120,3.24,2.150,1.075,1334
```

Easy to import to Excel/spreadsheet for analysis and graphing.

## Calibration Workflow

### Two-Point Calibration (Recommended)

For accurate current readings, perform both calibrations:

**1. Zero Calibration ('z' command)**
```
Purpose:  Correct sensor offset when no current is flowing
When:     First time setup, or if readings show constant offset
Steps:
  1. Remove ALL cables from sensor opening (no current flowing)
  2. Heater must be OFF
  3. Press 'z' in serial monitor
  4. Wait for calibration to complete (~1 second)
  5. Verify current now reads close to 0.00 A
```

**2. Sensitivity Calibration ('s' command)**
```
Purpose:  Correct sensor scale/gain for accurate measurements
When:     After zero calibration, if readings are consistently wrong by %
Steps:
  1. Connect a known current source:
     - Bench power supply (best - shows exact current)
     - OR use multimeter to measure actual current
  2. Pass power cable through WCS1800 sensor
  3. Note the ACTUAL current (from PSU display or multimeter)
  4. Press 's' in serial monitor
  5. Enter actual current value when prompted (e.g., "3.18")
  6. Sensor will calculate new sensitivity
  7. Verify readings now match your reference
```

**Example Calibration Session:**
```
1. Remove cables → Press 'z' → Reads 0.02 A ✓
2. Connect 3.18 A load → Press 's' → Enter "3.18" → Reads 3.19 A ✓
3. Calibration complete! Accuracy: ±1-2%
```

### When to Recalibrate

- **Zero calibration:** If sensor shows constant offset (e.g., always reads -8A with no load)
- **Sensitivity calibration:** If readings are consistently high or low by same percentage
- **Both:** After replacing sensor, changing supply voltage, or moving to new location

**Note:** The hardware potentiometer on WCS1800 sets overflow alarm threshold, NOT zero calibration. Use software 'z' command instead.

## Heater Profiling Mode

### Purpose
Capture current consumption profile of your heater through a complete cycle to identify operating states and set accurate thresholds.

### How to Use Logging Mode

**Step 1: Start Logging**
```
Press 'l' → Logging starts
```

**Step 2: Run Heater Through Full Cycle**
```
1. Turn heater ON (glow plug starts)
2. Wait for heater to reach steady running state
3. Let it run for a few minutes
4. Turn heater OFF (cooling fan runs)
5. Wait until completely off
Total time: 10-15 minutes typical
```

**Step 3: Stop Logging**
```
Press 'l' → Logging stops, shows summary
```

**Step 4: Analyze Data**
```
Copy CSV output from serial monitor
Paste into Excel/Google Sheets
Create time-series graph
Identify state transitions
```

### Expected Heater Current Profile

Typical diesel heater states you'll observe:

```
State           Current     Duration        What's happening
─────────────────────────────────────────────────────────────
OFF             0-0.2 A     -               Standby/idle
STARTING        8-12 A      30-120 sec      Glow plug heating
RUNNING LOW     1-3 A       continuous      Low power output
RUNNING HIGH    3-6 A       continuous      High power output
COOLING         0.5-1.5 A   5-10 min        Fan only, purging
OFF             0-0.2 A     -               Complete shutdown
```

### Example Logging Output

```
╔══════════════════════════════════════════════════════════╗
║              Heater Profile Logging Mode                ║
╚══════════════════════════════════════════════════════════╝

CSV Data Logging Started
Time(s),Current(A),WCS1800_V,ESP32_V,ADC_Raw
------------------------------------------------------------
0,0.05,2.503,1.252,1552
1,0.03,2.501,1.251,1550
2,10.52,3.204,1.602,1987    ← Glow plug ON!
3,10.48,3.201,1.601,1985
...
120,3.24,2.716,1.358,1684   ← Steady running
...
600,1.05,2.573,1.287,1596   ← Cooling phase
602,0.02,2.501,1.251,1550   ← OFF
------------------------------------------------------------

✓ Logging stopped

Summary:
  Duration:     602 seconds (10.0 minutes)
  Samples:      1204
  Min current:  0.02 A
  Max current:  11.45 A

Suggested heater state thresholds:
  OFF:          < 0.3 A
  STARTING:     > 9.2 A
  RUNNING LOW:  1.0 - 4.6 A
  RUNNING HIGH: 4.6 - 9.2 A
  COOLING:      0.3 - 2.0 A
```

### Using Profile Data

**1. Import to spreadsheet:**
   - Copy CSV data from serial monitor
   - Paste into Excel/Google Sheets
   - Column A = Time, Column B = Current, etc.

**2. Create graph:**
   - Select Time and Current columns
   - Insert → Line chart
   - Visualize state transitions

**3. Set thresholds in main controller:**
   - Use suggested values from logging summary
   - OR identify exact values from your graph
   - Update threshold constants in heater controller code

**Memory Usage:** Logging mode uses **ZERO additional RAM** - data streams directly to serial output, not stored on ESP32.

## Troubleshooting

### Constant Offset (e.g., reads -8A with no current)
**Most common issue!** WCS1800 sensors often have a voltage offset.

**Solution:**
- Press **'z'** to perform zero calibration
- Takes 1 second, samples sensor at zero current
- Automatically corrects offset
- Current should read 0.00 A ± 0.05 A after calibration

**Note:** Hardware potentiometer on WCS1800 board sets alarm threshold, NOT zero point. Use software 'z' command.

### Readings Consistently Wrong by Same Percentage
Example: Reads 2.7A when actual is 3.18A (always ~15% low)

**Solution:**
- Press **'s'** to perform sensitivity calibration
- Requires known current source (bench PSU or multimeter)
- Enter actual current value when prompted
- Sensor calculates correct sensitivity for your specific unit

### Always Reads 0A
- Check WCS1800 power: VCC = 5V, GND connected
- Verify voltage divider resistors are connected
- Ensure cable passes through sensor opening
- Try passing cable through opposite direction

### Negative Values (with current flowing)
- Flip cable direction through WCS1800 sensor
- Current direction matters for Hall-effect sensors
- Or use `abs()` in code if direction doesn't matter for your application

### Unstable/Noisy Readings
- Add 0.1µF ceramic capacitor between GPIO34 and GND
- Keep sensor wires short
- Route wires away from power cables
- Increase `SAMPLES_PER_READING` to 20 or 50

### ADC Reading Stuck at 0 or 4095
- Voltage divider may be wrong or missing
- Check for short circuits
- Verify GPIO34 is analog-capable pin

## Calibration Values

### WCS1800 Specifications:
- **Range:** 0-35A
- **Sensitivity:** 66.7 mV/A
- **Zero current output:** 2.5V (at 5V supply)
- **Max output:** 4.83V (at 35A)

### Expected Voltage Divider Output:
| Current | WCS1800 Out | After Divider | ESP32 ADC |
|---------|-------------|---------------|-----------|
| 0A      | 2.50V       | 1.25V         | ~1550     |
| 5A      | 2.83V       | 1.42V         | ~1760     |
| 10A     | 3.17V       | 1.58V         | ~1970     |
| 15A     | 3.50V       | 1.75V         | ~2180     |
| 20A     | 3.83V       | 1.92V         | ~2390     |

Press `c` in serial monitor to see full calibration table.

## Integration with Main Heater Controller

### Complete Workflow (On the Boat)

**Phase 1: Hardware Setup**
1. Build voltage divider circuit (10kΩ/10kΩ)
2. Connect WCS1800 to ESP32 (with voltage divider protection!)
3. Pass heater power cable through sensor opening
4. Upload this test code to ESP32

**Phase 2: Calibration**
1. Press **'z'** - Zero calibration (no current)
2. Press **'s'** - Sensitivity calibration (use multimeter or known load)
3. Verify readings are accurate (±1-2%)

**Phase 3: Profiling**
1. Press **'l'** - Start logging mode
2. Turn heater ON, let run through full cycle (10-15 min)
3. Press **'l'** - Stop logging
4. Copy CSV data, analyze in spreadsheet
5. Note threshold values from summary

**Phase 4: Integration**
1. Transfer calibration values to main controller code:
   - `zeroCurrentVoltage` (from 'z' calibration)
   - `currentSensitivity` (from 's' calibration)
2. Set heater state thresholds based on profile data:
   - `CURRENT_THRESHOLD_OFF` = 0.3 A
   - `CURRENT_THRESHOLD_STARTING` = (from your max glow plug current)
   - `CURRENT_THRESHOLD_RUNNING_LOW` = (from your profile)
   - `CURRENT_THRESHOLD_RUNNING_HIGH` = (from your profile)
3. Transfer voltage divider circuit to permanent installation
4. Test with actual heater on/off cycles

## Safety Notes

- ⚠️ Current sensor must be properly insulated (high voltage!)
- ⚠️ Never exceed 35A through WCS1800
- ⚠️ Double-check voltage divider before connecting to ESP32
- ⚠️ Use proper wire gauge for heater power (not thin jumper wires)

## Next Steps

After successful testing:
1. Copy voltage divider circuit to main project
2. Test current threshold detection with heater on/off
3. Calibrate threshold value (typically 0.5A)
4. Integrate into heater controller for automatic operation detection
