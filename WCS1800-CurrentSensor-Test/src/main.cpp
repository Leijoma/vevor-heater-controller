/*
 * WCS1800 Current Sensor Test with Voltage Divider + Temperature Sensors
 *
 * Hardware Setup:
 * - WCS1800 VCC → 5V (ESP32 VIN or external supply)
 * - WCS1800 GND → GND
 * - WCS1800 OUT → [10kΩ resistor] → ESP32 GPIO34
 *                                    └─ [10kΩ resistor] → GND
 * - DS18B20 sensors → GPIO18 (with 4.7kΩ pull-up to 3.3V)
 *
 * CRITICAL: Voltage divider is REQUIRED!
 * WCS1800 outputs 0.5V-4.5V (at 5V supply)
 * ESP32 ADC max is 3.3V - without divider you'll damage the ESP32!
 */

#include <Arduino.h>
#include <Preferences.h>
#include <OneWire.h>
#include <DallasTemperature.h>

// Pin Configuration
#define CURRENT_SENSOR_PIN 34  // GPIO34 (ADC1_CH6) - must be ADC-capable
#define ONEWIRE_BUS_PIN 18     // GPIO18 for DS18B20 temperature sensors

// WCS1800 Specifications
#define CURRENT_SENSOR_SENSITIVITY 0.0667  // 66.7mV/A for WCS1800
#define CURRENT_SENSOR_VREF 5.0            // WCS1800 supply voltage
#define VOLTAGE_DIVIDER_RATIO 0.5          // Using 10kΩ/10kΩ divider (output / 2)
#define ESP32_ADC_VREF 3.3                 // ESP32 ADC reference voltage
#define ADC_RESOLUTION 4095.0              // 12-bit ADC

// Sampling Configuration
#define SAMPLES_PER_READING 10             // Average multiple samples for stability
#define UPDATE_INTERVAL 500                // Update display every 500ms
#define LOGGING_INTERVAL 10000             // Log data every 10 seconds
#define MOVING_AVG_SIZE 20                 // Moving average window (smooth noisy readings)

// Heater State Detection Thresholds (based on actual cycle data)
#define THRESHOLD_OFF 0.3              // Below this = OFF
#define THRESHOLD_GLOW_PLUG 8.0        // Above this = glow plug heating
#define THRESHOLD_RUNNING_MIN 0.8      // Minimum current for RUNNING
#define THRESHOLD_RUNNING_MAX 8.0      // Maximum current for RUNNING
#define THRESHOLD_COOLING_MAX 1.5      // COOLING fan-only current

// Temperature delta thresholds (degrees per second)
#define TEMP_DELTA_WINDOW 10000        // Calculate delta over 10 second window
#define TEMP_DELTA_RISING 0.1          // Duct temp rising > 0.1°C/sec = combustion active
#define TEMP_DELTA_FALLING -0.05       // Duct temp falling < -0.05°C/sec = combustion stopped

// Heater states with proper transitions
enum HeaterState {
  STATE_OFF,           // < 0.3A
  STATE_STARTING,      // Initial startup: 0.3-8A (before glow plug)
  STATE_HEATING,       // Glow plug active: > 8A
  STATE_RUNNING,       // Normal operation: 0.8-8A + duct temp rising
  STATE_COOLING,       // Shutdown: 0.3-1.5A (fan only, from RUNNING)
  STATE_UNKNOWN
};

const char* stateToString(HeaterState state) {
  switch (state) {
    case STATE_OFF: return "OFF";
    case STATE_STARTING: return "STARTING";
    case STATE_HEATING: return "HEATING";
    case STATE_RUNNING: return "RUNNING";
    case STATE_COOLING: return "COOLING";
    default: return "UNKNOWN";
  }
}

// Global variables for statistics
float minCurrent = 999.0;
float maxCurrent = -999.0;
unsigned long sampleCount = 0;

// Calibration variables
float zeroCurrentVoltage = 2.5;  // Default zero point (will be calibrated)
float currentSensitivity = CURRENT_SENSOR_SENSITIVITY;  // Will be calibrated

// Moving average filter
float movingAvgBuffer[MOVING_AVG_SIZE];
int movingAvgIndex = 0;
bool movingAvgFilled = false;
float movingAvgSum = 0;

// Logging mode (streams to serial, no RAM storage)
bool loggingEnabled = false;
unsigned long loggingStartTime = 0;
unsigned long lastLogTime = 0;

// Heater state tracking
HeaterState currentHeaterState = STATE_OFF;
HeaterState previousHeaterState = STATE_OFF;
float previousDuctTemp = 0.0;
unsigned long stateChangeTime = 0;  // Track when state last changed

// Temperature delta tracking (for duct temp rate of change)
float ductTempAtWindowStart = 0.0;
unsigned long deltaWindowStartTime = 0;
float currentDuctTempDelta = 0.0;  // °C per second

// Persistent storage for calibration
Preferences preferences;

// Temperature sensors
OneWire oneWire(ONEWIRE_BUS_PIN);
DallasTemperature tempSensors(&oneWire);
int numTempSensors = 0;
float tempCabin = 0.0;
float tempDuct = 0.0;

void setup() {
  Serial.begin(115200);
  delay(1000);

  // Configure ADC pin
  pinMode(CURRENT_SENSOR_PIN, INPUT);

  // ESP32 ADC configuration for better accuracy
  analogSetAttenuation(ADC_11db);  // 0-3.3V range (default)

  // Initialize moving average buffer
  for (int i = 0; i < MOVING_AVG_SIZE; i++) {
    movingAvgBuffer[i] = 0;
  }

  // Load calibration from persistent storage
  preferences.begin("wcs1800", false);
  zeroCurrentVoltage = preferences.getFloat("zeroVoltage", 2.5);
  currentSensitivity = preferences.getFloat("sensitivity", CURRENT_SENSOR_SENSITIVITY);
  preferences.end();

  // Initialize temperature sensors
  tempSensors.begin();
  numTempSensors = tempSensors.getDeviceCount();

  Serial.println("\n╔══════════════════════════════════════════════════════════╗");
  Serial.println("║ WCS1800 + Temperature Sensor Test with Voltage Divider  ║");
  Serial.println("╚══════════════════════════════════════════════════════════╝\n");

  Serial.println("Configuration:");
  Serial.println("  - Sensor: WCS1800 (0-35A range)");
  Serial.println("  - Sensitivity: 66.7mV/A");
  Serial.println("  - Supply voltage: 5V");
  Serial.println("  - Voltage divider: 10kΩ/10kΩ (50%)");
  Serial.println("  - ADC pin: GPIO34");
  Serial.println("  - Samples per reading: " + String(SAMPLES_PER_READING));
  Serial.println("  - Moving average window: " + String(MOVING_AVG_SIZE) + " readings");
  Serial.println();

  Serial.println("Calibration (from persistent storage):");
  Serial.printf("  - Zero voltage: %.3f V\n", zeroCurrentVoltage);
  Serial.printf("  - Sensitivity: %.4f V/A\n", currentSensitivity);
  Serial.println();

  Serial.println("Temperature Sensors:");
  Serial.printf("  - Found %d DS18B20 sensor(s) on GPIO%d\n", numTempSensors, ONEWIRE_BUS_PIN);
  if (numTempSensors >= 2) {
    Serial.println("  - Sensor 0: Cabin temperature");
    Serial.println("  - Sensor 1: Duct temperature");
  } else if (numTempSensors == 1) {
    Serial.println("  ⚠️  Only 1 sensor found (need 2 for cabin + duct)");
  } else {
    Serial.println("  ⚠️  No temperature sensors found!");
  }
  Serial.println();

  Serial.println("Voltage Divider Circuit:");
  Serial.println("  WCS1800 OUT → [10kΩ] → GPIO34");
  Serial.println("                           └─ [10kΩ] → GND");
  Serial.println();

  Serial.println("Expected Voltage Ranges:");
  Serial.printf("  - WCS1800 output: 0.5V (no current) to 4.5V (max current)\n");
  Serial.printf("  - After divider: 0.25V to 2.25V (safe for ESP32)\n");
  Serial.printf("  - ESP32 ADC max: 3.3V\n\n");

  Serial.println("Instructions:");
  Serial.println("  1. Verify voltage divider is connected correctly");
  Serial.println("  2. With NO current: Should read close to 0A");
  Serial.println("  3. Pass heater power cable through WCS1800 sensor");
  Serial.println("  4. Turn heater ON and observe current readings");
  Serial.println();
  Serial.println("Commands:");
  Serial.println("  'z' - Zero calibration (remove all cables first!)");
  Serial.println("  's' - Sensitivity calibration (with known current source)");
  Serial.println("  'l' - Start/stop logging mode (profile heater states)");
  Serial.println("  'r' - Reset min/max statistics");
  Serial.println("  'c' - Show calibration info");
  Serial.println("\n" + String('─', 60) + "\n");

  delay(2000);
}

HeaterState detectHeaterState(float current, float ductTemp) {
  // Use absolute value to handle negative readings (cable backwards)
  float absCurrent = abs(current);
  float ductTempChange = ductTemp - previousDuctTemp;

  // State machine with valid transitions only
  HeaterState newState = currentHeaterState;

  switch (currentHeaterState) {
    case STATE_OFF:
      // OFF → STARTING: Current rises above threshold
      if (absCurrent >= THRESHOLD_OFF && absCurrent < THRESHOLD_GLOW_PLUG) {
        newState = STATE_STARTING;
      }
      // OFF → HEATING: Direct to glow plug (if we missed STARTING)
      else if (absCurrent >= THRESHOLD_GLOW_PLUG) {
        newState = STATE_HEATING;
      }
      break;

    case STATE_STARTING:
      // STARTING → HEATING: Glow plug kicks in
      if (absCurrent >= THRESHOLD_GLOW_PLUG) {
        newState = STATE_HEATING;
      }
      // STARTING → OFF: Startup aborted
      else if (absCurrent < THRESHOLD_OFF) {
        newState = STATE_OFF;
      }
      break;

    case STATE_HEATING:
      // HEATING → RUNNING: Current drops + duct temp rising (combustion confirmed)
      if (absCurrent < THRESHOLD_GLOW_PLUG && absCurrent >= THRESHOLD_RUNNING_MIN) {
        unsigned long timeInHeating = millis() - stateChangeTime;

        // Ignition confirmed by:
        // 1. Duct temp rising (positive delta = combustion active), OR
        // 2. Been in HEATING for at least 10 seconds (fallback for edge cases)
        if (currentDuctTempDelta > TEMP_DELTA_RISING || timeInHeating >= 10000) {
          newState = STATE_RUNNING;
        }
      }
      // HEATING → STARTING: Glow plug turns off but not yet running
      else if (absCurrent < THRESHOLD_GLOW_PLUG && absCurrent >= THRESHOLD_OFF) {
        newState = STATE_STARTING;  // Back to waiting
      }
      // HEATING → OFF: Startup failed
      else if (absCurrent < THRESHOLD_OFF) {
        newState = STATE_OFF;
      }
      break;

    case STATE_RUNNING:
      // RUNNING → COOLING: Current drops to fan-only range + temp falling/stable
      if (absCurrent < THRESHOLD_COOLING_MAX && absCurrent >= THRESHOLD_OFF) {
        // Cooling mode confirmed: low current + temp no longer rising
        // (combustion stopped, fan blowing residual heat)
        if (currentDuctTempDelta < TEMP_DELTA_RISING) {
          newState = STATE_COOLING;
        }
      }
      // RUNNING → OFF: Sudden shutdown
      else if (absCurrent < THRESHOLD_OFF) {
        newState = STATE_OFF;
      }
      // Stay in RUNNING if current is in normal operating range
      break;

    case STATE_COOLING:
      // COOLING → OFF: Current drops below threshold
      if (absCurrent < THRESHOLD_OFF) {
        newState = STATE_OFF;
      }
      // COOLING → RUNNING: Heater restarted (turned back on)
      else if (absCurrent >= THRESHOLD_RUNNING_MIN) {
        newState = STATE_RUNNING;
      }
      break;

    default:
      // Unknown state - reset based on current only
      if (absCurrent < THRESHOLD_OFF) {
        newState = STATE_OFF;
      }
      break;
  }

  // Track state changes
  if (newState != currentHeaterState) {
    previousHeaterState = currentHeaterState;
    stateChangeTime = millis();
  }

  return newState;
}

float readCurrent() {
  // Read multiple samples and average
  long sumRaw = 0;

  for (int i = 0; i < SAMPLES_PER_READING; i++) {
    sumRaw += analogRead(CURRENT_SENSOR_PIN);
    delayMicroseconds(100);  // Small delay between samples
  }

  float rawValue = sumRaw / (float)SAMPLES_PER_READING;

  // Convert ADC reading to voltage (0-3.3V)
  float measuredVoltage = (rawValue / ADC_RESOLUTION) * ESP32_ADC_VREF;

  // Apply voltage divider correction to get actual WCS1800 output
  float actualVoltage = measuredVoltage / VOLTAGE_DIVIDER_RATIO;

  // Calculate current from WCS1800
  // Zero current voltage is calibrated (press 'z' to zero)
  // Current (A) = (Vout - ZeroVoltage) / Sensitivity
  float current = (actualVoltage - zeroCurrentVoltage) / currentSensitivity;

  return current;
}

float getAverageVoltage() {
  // Get average voltage reading (used for calibration)
  long sumRaw = 0;

  for (int i = 0; i < 100; i++) {  // More samples for calibration
    sumRaw += analogRead(CURRENT_SENSOR_PIN);
    delay(10);
  }

  float rawValue = sumRaw / 100.0;
  float measuredVoltage = (rawValue / ADC_RESOLUTION) * ESP32_ADC_VREF;
  float actualVoltage = measuredVoltage / VOLTAGE_DIVIDER_RATIO;

  return actualVoltage;
}

void performZeroCalibration() {
  Serial.println("\n\n╔══════════════════════════════════════════════════════════╗");
  Serial.println("║              Zero Current Calibration                   ║");
  Serial.println("╚══════════════════════════════════════════════════════════╝\n");

  Serial.println("⚠️  IMPORTANT: Make sure NO current is flowing!");
  Serial.println("   - Remove all cables from sensor opening");
  Serial.println("   - Heater should be OFF");
  Serial.println("   - Only WCS1800 supply current should be present");
  Serial.println();
  Serial.println("Calibrating in 3 seconds...");
  delay(3000);

  Serial.print("Taking calibration readings");
  float newZeroVoltage = getAverageVoltage();
  Serial.println(" DONE!");

  Serial.println();
  Serial.println("Calibration Results:");
  Serial.printf("  Old zero voltage: %.3f V\n", zeroCurrentVoltage);
  Serial.printf("  New zero voltage: %.3f V\n", newZeroVoltage);
  Serial.printf("  Offset correction: %.3f V (%.2f A equivalent)\n",
                newZeroVoltage - zeroCurrentVoltage,
                (newZeroVoltage - zeroCurrentVoltage) / CURRENT_SENSOR_SENSITIVITY);

  zeroCurrentVoltage = newZeroVoltage;

  // Save to persistent storage
  preferences.begin("wcs1800", false);
  preferences.putFloat("zeroVoltage", zeroCurrentVoltage);
  preferences.end();
  Serial.println("  ✓ Saved to persistent storage");

  // Reset statistics after calibration
  minCurrent = 999.0;
  maxCurrent = -999.0;
  sampleCount = 0;

  Serial.println();
  Serial.println("✓ Zero calibration complete!");
  Serial.println("  Current readings should now be close to 0A");
  Serial.println("\n" + String('-', 60) + "\n");

  delay(2000);
}

void performSensitivityCalibration() {
  Serial.println("\n\n╔══════════════════════════════════════════════════════════╗");
  Serial.println("║           Sensitivity Calibration (Scale)               ║");
  Serial.println("╚══════════════════════════════════════════════════════════╝\n");

  Serial.println("This calibration adjusts the sensitivity (scale) of readings.");
  Serial.println("You need a known current source for reference.");
  Serial.println();
  Serial.println("Setup:");
  Serial.println("  1. Connect a load with known current (e.g., bench power supply)");
  Serial.println("  2. Pass the power cable through WCS1800 sensor");
  Serial.println("  3. Measure actual current with your reference (PSU display/multimeter)");
  Serial.println("  4. Enter that value when prompted");
  Serial.println();

  // Get current sensor reading
  Serial.println("Taking current sensor readings...");
  delay(1000);
  float measuredVoltage = getAverageVoltage();
  float currentReading = (measuredVoltage - zeroCurrentVoltage) / currentSensitivity;

  Serial.println();
  Serial.printf("Sensor currently reads: %.2f A\n", currentReading);
  Serial.printf("Sensor voltage: %.3f V (zero: %.3f V, delta: %.3f V)\n\n",
                measuredVoltage, zeroCurrentVoltage, measuredVoltage - zeroCurrentVoltage);

  // Get actual current from user
  Serial.println("Enter the ACTUAL current from your reference meter:");
  Serial.println("(Format: 3.18 for 3.18 amps, then press Enter)");

  // Wait for user input
  while (!Serial.available()) {
    delay(100);
  }

  String input = Serial.readStringUntil('\n');
  input.trim();
  float actualCurrent = input.toFloat();

  if (actualCurrent <= 0.1) {
    Serial.println("\n❌ Invalid current value. Calibration cancelled.");
    Serial.println("   Current must be greater than 0.1A for accurate calibration.\n");
    delay(2000);
    return;
  }

  // Calculate new sensitivity
  // Sensitivity = (Voltage - ZeroVoltage) / Current
  float voltageDelta = measuredVoltage - zeroCurrentVoltage;
  float newSensitivity = voltageDelta / actualCurrent;

  Serial.println();
  Serial.println("Calibration Results:");
  Serial.printf("  Actual current (reference):  %.2f A\n", actualCurrent);
  Serial.printf("  Sensor reading (before):     %.2f A\n", currentReading);
  Serial.printf("  Error:                       %.2f A (%.1f%%)\n",
                currentReading - actualCurrent,
                ((currentReading - actualCurrent) / actualCurrent) * 100.0);
  Serial.println();
  Serial.printf("  Old sensitivity: %.4f V/A\n", currentSensitivity);
  Serial.printf("  New sensitivity: %.4f V/A\n", newSensitivity);
  Serial.printf("  Adjustment:      %.1f%%\n",
                ((newSensitivity - currentSensitivity) / currentSensitivity) * 100.0);

  currentSensitivity = newSensitivity;

  // Save to persistent storage
  preferences.begin("wcs1800", false);
  preferences.putFloat("sensitivity", currentSensitivity);
  preferences.end();

  // Verify with new reading
  float newReading = (measuredVoltage - zeroCurrentVoltage) / currentSensitivity;
  Serial.println();
  Serial.printf("  Sensor reading (after):      %.2f A\n", newReading);
  Serial.printf("  New error:                   %.2f A (%.1f%%)\n",
                newReading - actualCurrent,
                ((newReading - actualCurrent) / actualCurrent) * 100.0);

  // Reset statistics
  minCurrent = 999.0;
  maxCurrent = -999.0;
  sampleCount = 0;

  Serial.println();
  Serial.println("✓ Sensitivity calibration complete!");
  Serial.println("  Future readings should now match your reference meter.");
  Serial.println("\n" + String('-', 60) + "\n");

  delay(2000);
}

void displayReading() {
  // Read sensor
  float current = readCurrent();

  // Read temperature sensors
  if (numTempSensors > 0) {
    tempSensors.requestTemperatures();
    tempCabin = tempSensors.getTempCByIndex(0);
    if (numTempSensors > 1) {
      tempDuct = tempSensors.getTempCByIndex(1);
    }
  }

  // Update moving average filter
  movingAvgSum -= movingAvgBuffer[movingAvgIndex];  // Subtract oldest value
  movingAvgBuffer[movingAvgIndex] = current;         // Store new value
  movingAvgSum += current;                           // Add new value to sum
  movingAvgIndex = (movingAvgIndex + 1) % MOVING_AVG_SIZE;  // Circular buffer

  if (!movingAvgFilled && movingAvgIndex == 0) {
    movingAvgFilled = true;  // Buffer is now full
  }

  // Calculate average (use actual count if buffer not yet full)
  int avgCount = movingAvgFilled ? MOVING_AVG_SIZE : (movingAvgIndex + 1);
  float avgCurrent = movingAvgSum / avgCount;

  // Calculate duct temperature delta (rate of change over time window)
  unsigned long currentTime = millis();
  unsigned long timeSinceWindowStart = currentTime - deltaWindowStartTime;

  if (timeSinceWindowStart >= TEMP_DELTA_WINDOW || deltaWindowStartTime == 0) {
    // Reset window - calculate delta and start new window
    if (deltaWindowStartTime != 0) {
      float tempChange = tempDuct - ductTempAtWindowStart;
      float timeSeconds = timeSinceWindowStart / 1000.0;
      currentDuctTempDelta = tempChange / timeSeconds;  // °C per second
    }
    // Start new window
    ductTempAtWindowStart = tempDuct;
    deltaWindowStartTime = currentTime;
  }

  // Detect heater state based on averaged current and duct temperature
  HeaterState newState = detectHeaterState(avgCurrent, tempDuct);

  // Show state transition notification (only in normal display mode)
  if (newState != currentHeaterState && !loggingEnabled) {
    Serial.printf("\n>>> STATE CHANGE: %s → %s <<<\n",
                  stateToString(currentHeaterState), stateToString(newState));
    Serial.printf("    Current: %.2f A | Duct: %.1f°C | Delta: %+.3f°C/sec\n\n",
                  avgCurrent, tempDuct, currentDuctTempDelta);
  }

  currentHeaterState = newState;

  // Update previous duct temperature for next cycle
  previousDuctTemp = tempDuct;

  // Update statistics (use averaged value for better min/max)
  if (avgCurrent < minCurrent) minCurrent = avgCurrent;
  if (avgCurrent > maxCurrent) maxCurrent = avgCurrent;
  sampleCount++;

  // Get raw values for detailed display
  int rawADC = analogRead(CURRENT_SENSOR_PIN);
  float measuredV = (rawADC / ADC_RESOLUTION) * ESP32_ADC_VREF;
  float actualV = measuredV / VOLTAGE_DIVIDER_RATIO;

  // Check if in logging mode
  if (loggingEnabled) {
    // Only log every LOGGING_INTERVAL (10 seconds)
    if (currentTime - lastLogTime >= LOGGING_INTERVAL) {
      // CSV format - use averaged current for cleaner data
      // Time(s), State, Current_Avg(A), Current_Instant(A), Cabin_Temp(C), Duct_Temp(C), Duct_Delta(C/s), WCS1800_V, ESP32_V, ADC_Raw
      unsigned long elapsedSeconds = (currentTime - loggingStartTime) / 1000;
      Serial.printf("%lu,%s,%.2f,%.2f,%.1f,%.1f,%.4f,%.3f,%.3f,%d\n",
                    elapsedSeconds, stateToString(currentHeaterState),
                    avgCurrent, current, tempCabin, tempDuct, currentDuctTempDelta,
                    actualV, measuredV, rawADC);
      lastLogTime = currentTime;
    }
  } else {
    // Normal display mode - show both instant and averaged with temperatures, delta, and state
    if (numTempSensors >= 2) {
      Serial.printf("[%8s] Avg: %5.2f A │ Cabin: %5.1f°C │ Duct: %5.1f°C (%+.3f°C/s) │ Min: %5.2f A │ Max: %5.2f A\n",
                    stateToString(currentHeaterState), avgCurrent, tempCabin, tempDuct, currentDuctTempDelta, minCurrent, maxCurrent);
    } else if (numTempSensors == 1) {
      Serial.printf("[%8s] Avg: %5.2f A │ Temp: %5.1f°C │ Min: %5.2f A │ Max: %5.2f A\n",
                    stateToString(currentHeaterState), avgCurrent, tempCabin, minCurrent, maxCurrent);
    } else {
      Serial.printf("[%8s] Avg: %5.2f A │ ADC: %4d │ Min: %5.2f A │ Max: %5.2f A\n",
                    stateToString(currentHeaterState), avgCurrent, rawADC, minCurrent, maxCurrent);
    }
  }
}

void resetStatistics() {
  minCurrent = 999.0;
  maxCurrent = -999.0;
  sampleCount = 0;
  Serial.println("\n✓ Statistics reset\n");
}

void startLogging() {
  if (loggingEnabled) {
    Serial.println("\n⚠️  Logging is already running!\n");
    return;
  }

  Serial.println("\n╔══════════════════════════════════════════════════════════╗");
  Serial.println("║              Heater Profile Logging Mode                ║");
  Serial.println("╚══════════════════════════════════════════════════════════╝\n");

  Serial.println("CSV Data Logging Started");
  Serial.println("This data streams to serial (no RAM usage)");
  Serial.println("Copy/paste output to text file or spreadsheet\n");
  Serial.println("Instructions:");
  Serial.println("  1. Turn heater ON now");
  Serial.println("  2. Let it run through full cycle (start → run → cool → off)");
  Serial.println("  3. Press 'l' again to stop logging when done\n");
  Serial.println("CSV Header:");
  Serial.println("Time(s),State,Current_Avg(A),Current_Instant(A),Cabin_Temp(C),Duct_Temp(C),Duct_Delta(C/s),WCS1800_V,ESP32_V,ADC_Raw");
  Serial.println("------------------------------------------------------------");

  loggingEnabled = true;
  loggingStartTime = millis();
  lastLogTime = 0;  // Log immediately on first cycle

  // Reset statistics for logging session
  minCurrent = 999.0;
  maxCurrent = -999.0;
  sampleCount = 0;
}

void stopLogging() {
  if (!loggingEnabled) {
    Serial.println("\n⚠️  Logging is not running!\n");
    return;
  }

  loggingEnabled = false;
  unsigned long totalTime = (millis() - loggingStartTime) / 1000;

  Serial.println("------------------------------------------------------------");
  Serial.println("\n✓ Logging stopped\n");
  Serial.println("Summary:");
  Serial.printf("  Duration:     %lu seconds (%.1f minutes)\n", totalTime, totalTime / 60.0);
  Serial.printf("  Samples:      %lu\n", sampleCount);
  Serial.printf("  Min current:  %.2f A\n", minCurrent);
  Serial.printf("  Max current:  %.2f A\n", maxCurrent);
  Serial.println();
  Serial.println("Suggested heater state thresholds:");
  Serial.println("  OFF:          < 0.3 A");
  Serial.printf("  STARTING:     > %.1f A (adjust based on your max)\n", maxCurrent * 0.8);
  Serial.printf("  RUNNING LOW:  %.1f - %.1f A\n", 1.0, maxCurrent * 0.4);
  Serial.printf("  RUNNING HIGH: %.1f - %.1f A\n", maxCurrent * 0.4, maxCurrent * 0.8);
  Serial.printf("  COOLING:      0.3 - %.1f A\n", 2.0);
  Serial.println("\n" + String('-', 60) + "\n");
}

void showCalibration() {
  Serial.println("\n\n╔══════════════════════════════════════════════════════════╗");
  Serial.println("║                  Calibration Information                 ║");
  Serial.println("╚══════════════════════════════════════════════════════════╝\n");

  Serial.println("WCS1800 Output Voltage vs Current:");
  Serial.println("┌──────────┬──────────────┬────────────────┬──────────────┐");
  Serial.println("│ Current  │ WCS1800 Out  │ After Divider  │  ESP32 ADC   │");
  Serial.println("├──────────┼──────────────┼────────────────┼──────────────┤");

  for (float amps = 0; amps <= 35; amps += 5) {
    float wcsOut = 2.5 + (amps * CURRENT_SENSOR_SENSITIVITY);
    float afterDiv = wcsOut * VOLTAGE_DIVIDER_RATIO;
    int adcValue = (int)((afterDiv / ESP32_ADC_VREF) * ADC_RESOLUTION);

    Serial.printf("│  %5.1f A │   %5.2f V    │    %5.2f V     │    %4d      │\n",
                  amps, wcsOut, afterDiv, adcValue);
  }
  Serial.println("└──────────┴──────────────┴────────────────┴──────────────┘\n");

  Serial.println("Typical Diesel Heater Current Draw:");
  Serial.println("  - Glow plug heating (startup): 8-12 A");
  Serial.println("  - Running (low power):          1-3 A");
  Serial.println("  - Running (high power):         3-6 A");
  Serial.println("  - Shutdown (fan only):          0.5-1.5 A");
  Serial.println("  - OFF (standby):                0-0.2 A");
  Serial.println();

  Serial.println("Troubleshooting:");
  Serial.println("  - Constant offset (reads -8A or similar with no current):");
  Serial.println("    → BEST: Use hardware potentiometer on WCS1800 board");
  Serial.println("    → Turn pot with small screwdriver while watching display");
  Serial.println("    → Adjust until reading shows 0.00A");
  Serial.println("    → Alternative: Use software 'z' calibration command");
  Serial.println();
  Serial.println("  - Always reads 0A:");
  Serial.println("    → Check WCS1800 power (VCC=5V, GND connected)");
  Serial.println("    → Verify voltage divider resistors");
  Serial.println("    → Ensure cable passes through sensor opening");
  Serial.println();
  Serial.println("  - Negative values:");
  Serial.println("    → Reverse cable direction through WCS1800");
  Serial.println();
  Serial.println("  - Readings too high/low:");
  Serial.println("    → Verify 10kΩ resistors (measure with multimeter)");
  Serial.println("    → Check WCS1800 supply is exactly 5V");
  Serial.println("    → Try different SENSITIVITY value if using different sensor");
  Serial.println();
  Serial.println("  - Unstable/noisy readings:");
  Serial.println("    → Add 0.1µF capacitor between GPIO34 and GND");
  Serial.println("    → Keep sensor wires short and away from power cables");
  Serial.println("    → Increase SAMPLES_PER_READING to 20 or 50");
  Serial.println("\n" + String('─', 60) + "\n");
}

void loop() {
  static unsigned long lastUpdate = 0;

  // Update display at regular interval
  if (millis() - lastUpdate >= UPDATE_INTERVAL) {
    displayReading();
    lastUpdate = millis();
  }

  // Check for serial commands
  if (Serial.available()) {
    char cmd = Serial.read();
    while (Serial.available()) Serial.read();  // Flush buffer

    switch (cmd) {
      case 'z':
      case 'Z':
        performZeroCalibration();
        break;

      case 's':
      case 'S':
        performSensitivityCalibration();
        break;

      case 'l':
      case 'L':
        if (loggingEnabled) {
          stopLogging();
        } else {
          startLogging();
        }
        break;

      case 'r':
      case 'R':
        resetStatistics();
        break;

      case 'c':
      case 'C':
        showCalibration();
        break;

      default:
        Serial.println("\nCommands:");
        Serial.println("  z - Zero calibration (no current flowing!)");
        Serial.println("  s - Sensitivity calibration (with known current)");
        Serial.println("  l - Start/stop logging mode (profile heater)");
        Serial.println("  r - Reset min/max statistics");
        Serial.println("  c - Show calibration info\n");
        break;
    }
  }
}
