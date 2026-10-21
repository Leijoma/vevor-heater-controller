/**
 * Vevor Heater Controller - UI v2.0 Web
 *
 * Complete temperature control interface with WiFi and web access
 *
 * Features:
 * - Read temperature from DS18B20 sensor
 * - Long press (1.5s): Turn heater on/off
 * - Short click (when on): Enter/exit config mode
 * - Rotate: Adjust target temp or config parameters
 * - Display: Current temp, target temp, status, config
 * - WiFi connectivity with AP fallback
 * - PIN authentication for remote control
 * - REST API for SignalK integration
 * - WebSocket for real-time updates
 *
 * Hardware:
 * - OLED: SDA=GPIO22, SCL=GPIO21
 * - Encoder: CLK=33, DT=26, SW=32
 * - Temperature: DS18B20 on GPIO19 (1-wire)
 * - LED: GPIO18
 * - Heater: Serial2 RX=16, TX=17
 *
 * Serial Commands:
 * - wifi ssid <name>    : Set WiFi SSID
 * - wifi pass <password>: Set WiFi password
 * - wifi connect        : Connect to WiFi
 * - wifi status         : Show WiFi status
 * - pin set <4-digit>   : Set new PIN code
 * - pin <4-digit>       : Verify PIN code
 * - a                   : Enable auto mode
 * - d                   : Disable auto mode
 */

#include <Arduino.h>
#include <Wire.h>
#include <Adafruit_SH110X.h>
#include <DisplayFonts.h>
#include <RotaryEncoder.h>
#include <OneWire.h>
#include <DallasTemperature.h>
#include <VevorHeater.h>
#include <WiFiManager.h>
#include <AuthManager.h>
#include <WebServer.h>
#include <ArduinoOTA.h>

// ========== HARDWARE PINS ==========
#define I2C_SDA_PIN     22  // OLED SDA
#define I2C_SCL_PIN     21  // OLED SCL
#define ENCODER_CLK     33  // Rotary encoder CLK
#define ENCODER_DT      26  // Rotary encoder DT
#define ENCODER_SW      32  // Rotary encoder button
#define TEMP_SENSOR_PIN 19  // DS18B20 temperature sensor
#define LED_PIN         18  // Status LED
#define HEATER_RX_PIN   16  // Heater serial RX
#define HEATER_TX_PIN   17  // Heater serial TX

// ========== DISPLAY ==========
#define OLED_WIDTH  128
#define OLED_HEIGHT 64
#define OLED_ADDR   0x3C
#define OLED_RESET  -1

Adafruit_SH1106G display(OLED_WIDTH, OLED_HEIGHT, &Wire, OLED_RESET);
DisplayFonts fonts(&display);

// ========== COMPONENTS ==========
RotaryEncoder encoder(ENCODER_CLK, ENCODER_DT, ENCODER_SW, 2);
OneWire oneWire(TEMP_SENSOR_PIN);
DallasTemperature tempSensor(&oneWire);
VevorHeater heater(Serial2, HEATER_RX_PIN, HEATER_TX_PIN);
WiFiManager wifiManager;
AuthManager authManager;
WebServer webServer(authManager, 80);

// ========== UI STATES ==========
enum UIState {
  UI_STATE_OFF,      // Heater is off (manual)
  UI_STATE_RUNNING,  // Heater is running
  UI_STATE_COOLING,  // Heater is cooling down before off
  UI_STATE_CONFIG,   // Configuration mode (can only enter when running)
  UI_STATE_AUTO_OFF  // Heater is off due to auto temperature control
};

UIState uiState = UI_STATE_OFF;
UIState previousUIState = UI_STATE_OFF;

// ========== CONFIGURATION PARAMETERS ==========
int targetTemp = 22;    // Target temperature (8-36°C)
int powerLevel = 5;     // Power level (1-10)

// Diagnostic values (TODO: Get from heater)
float heaterVoltage = 12.5;     // Heater voltage
float heaterInnerTemp = 25.0;   // Heater internal temperature
int fanSpeed = 0;               // Fan speed (RPM)
int pumpSpeed = 0;              // Pump speed (%)

// Diagnostic display items
enum DiagnosticItem {
  DIAG_VOLTAGE = 0,
  DIAG_INNER_TEMP,
  DIAG_FAN_SPEED,
  DIAG_PUMP_FREQ,
  DIAG_COUNT
};

int currentDiagnostic = DIAG_VOLTAGE;

// ========== TEMPERATURE ==========
float currentTemp = -127.0;  // -127 indicates no reading yet
unsigned long lastTempRead = 0;
const unsigned long TEMP_READ_INTERVAL = 2000;  // Read every 2 seconds
bool tempSensorFound = false;

// ========== TIMING ==========
unsigned long lastDisplayUpdate = 0;
const unsigned long DISPLAY_UPDATE_INTERVAL = 200;  // Update display every 200ms for responsive UI

// ========== LED ==========
bool ledState = false;
bool hasError = false;
String errorMessage = "";
unsigned long lastLedBlink = 0;
const unsigned long LED_BLINK_INTERVAL = 200;  // Fast blink for errors

// ========== COOLING ==========
unsigned long coolingStartTime = 0;
const unsigned long COOLING_TIMEOUT = 180000;  // 3 minutes max cooling time

// ========== AUTO TEMPERATURE CONTROL ==========
bool autoControlEnabled = true;  // Default: AUTO mode ON

// Temperature history for smoothing and trend tracking
const int TEMP_HISTORY_SIZE = 150;  // 5 minutes at 2s interval
float tempHistory[TEMP_HISTORY_SIZE];
int tempHistoryIndex = 0;
int tempHistoryCount = 0;

float smoothedTemp = 0.0;     // 30-second smoothed temperature
float tempTrend = 0.0;        // Temperature trend (°C/min)

// Control thresholds (tunable)
const float THRESH_FAR_BELOW = 2.0;      // >2°C below target
const float THRESH_BELOW = 1.0;          // >1°C below target
const float THRESH_APPROACHING = 0.5;    // >0.5°C below target
const float THRESH_FINE_TUNE = 0.2;      // >0.2°C below target
const float THRESH_AT_TARGET = 0.5;      // Within ±0.5°C
const float THRESH_ABOVE = 2.0;          // >2°C above target -> turn OFF

// Trend thresholds for predictive control
const float TREND_FAST = 0.4;      // Fast heating (°C/min)
const float TREND_MEDIUM = 0.2;    // Medium heating (°C/min)
const float TREND_SLOW = 0.1;      // Slow heating (°C/min)

// Lookahead time for temperature prediction (minutes)
const float LOOKAHEAD_TIME = 3.0;  // Predict temperature 3 minutes ahead

// Power levels for each zone
const int POWER_FAR_BELOW = 10;
const int POWER_BELOW = 7;
const int POWER_APPROACHING = 5;
const int POWER_FINE_TUNE = 3;
const int POWER_AT_TARGET = 2;    // Maintenance (compensates ~0.2°C/min cooling)
const int POWER_ABOVE = 1;        // Minimal but never OFF

// Auto ON/OFF control thresholds (HYBRID: Rare OFF cycles only when far over target)
const float THRESH_AUTO_OFF = 2.0;        // Turn OFF when >2°C above target
const float THRESH_AUTO_RESTART = -1.0;   // Turn ON when >1°C below target
const int POWER_RESTART = 3;              // Restart at low-medium power
const unsigned long MIN_CYCLE_TIME = 600000;  // 10 minutes minimum ON or OFF time

// Auto cycle timing
unsigned long lastAutoStateChange = 0;  // Timestamp of last auto ON/OFF transition

// ========== ERROR MESSAGE SCROLLING ==========
int scrollOffset = 0;
unsigned long lastScrollTime = 0;
const unsigned long SCROLL_INTERVAL = 200;  // Scroll speed

// ========== BUTTON HANDLING ==========
bool buttonLongPressHandled = false;  // Prevent multiple triggers during long press

// ========== WEB SERVER STATE ==========
bool webServerStarted = false;  // Track if web server has been initialized

// ========== LIMITS ==========
const int MIN_TEMP = 8;
const int MAX_TEMP = 36;
const int MIN_POWER = 1;
const int MAX_POWER = 10;
const int MIN_HYSTERESIS = 1;
const int MAX_HYSTERESIS = 5;
const int MIN_FAN_SPEED = 20;
const int MAX_FAN_SPEED = 100;

// ========== TEMPERATURE READING ==========
void readTemperature() {
  if (millis() - lastTempRead >= TEMP_READ_INTERVAL) {
    lastTempRead = millis();

    tempSensor.requestTemperatures();
    float temp = tempSensor.getTempCByIndex(0);

    // Check if reading is valid
    if (temp != DEVICE_DISCONNECTED_C && temp > -50 && temp < 100) {
      currentTemp = temp;
      if (!tempSensorFound) {
        tempSensorFound = true;
        Serial.println("Temperature sensor found!");
      }

      // Add to history for smoothing and trend tracking
      tempHistory[tempHistoryIndex] = temp;
      tempHistoryIndex = (tempHistoryIndex + 1) % TEMP_HISTORY_SIZE;
      if (tempHistoryCount < TEMP_HISTORY_SIZE) {
        tempHistoryCount++;
      }

      // Calculate smoothed temperature (30 seconds = 15 samples at 2s interval)
      if (tempHistoryCount >= 15) {
        float sum = 0;
        for (int i = 0; i < 15; i++) {
          int idx = (tempHistoryIndex - 1 - i + TEMP_HISTORY_SIZE) % TEMP_HISTORY_SIZE;
          sum += tempHistory[idx];
        }
        smoothedTemp = sum / 15.0;
      } else {
        smoothedTemp = currentTemp;
      }

      // Calculate temperature trend (5 minutes)
      if (tempHistoryCount >= TEMP_HISTORY_SIZE) {
        // Compare current smoothed temp with temp 5 minutes ago
        int oldIdx = tempHistoryIndex;  // Oldest sample
        float temp5MinAgo = tempHistory[oldIdx];
        float tempChange = currentTemp - temp5MinAgo;
        tempTrend = tempChange / 5.0;  // °C per minute
      } else {
        tempTrend = 0.0;
      }

    } else {
      if (tempSensorFound) {
        Serial.println("WARNING: Temperature sensor disconnected!");
        tempSensorFound = false;
      }
      currentTemp = -127.0;
      smoothedTemp = 0.0;
      tempTrend = 0.0;
    }

    // Temperature logging removed to reduce serial spam
    // Enable for debugging: uncomment the lines below
    // if (tempSensorFound) {
    //   HeaterState state = heater.getState();
    //   const char* stateStr = VevorHeater::getStateString(state);
    //   const char* ctrlMode = autoControlEnabled ? "AUTO" : "MANUAL";
    //   Serial.printf("Temp: %.1f°C, Smoothed: %.1f°C, Trend: %.2f°C/min, Power: %d, Mode: %s, Ctrl: %s\n",
    //                 currentTemp, smoothedTemp, tempTrend, heater.getPower(), stateStr, ctrlMode);
    // }
  }
}

// ========== FORWARD DECLARATIONS ==========
void updateDisplay();

// ========== LED CONTROL ==========
void setLED(bool on) {
  digitalWrite(LED_PIN, on ? HIGH : LOW);
  ledState = on;
}

void updateLED() {
  if (hasError) {
    // Fast blink on error
    if (millis() - lastLedBlink >= LED_BLINK_INTERVAL) {
      lastLedBlink = millis();
      setLED(!ledState);
    }
  } else if (uiState == UI_STATE_RUNNING) {
    // Solid LED when running
    setLED(true);
  } else if (uiState == UI_STATE_COOLING) {
    // Solid LED when cooling
    setLED(true);
  } else if (uiState == UI_STATE_AUTO_OFF) {
    // LED off when in auto-off mode
    setLED(false);
  } else {
    // LED off when off
    setLED(false);
  }
}

// ========== HEATER CONTROL ==========
void startHeater() {
  Serial.println("\n=== STARTING HEATER ===");
  Serial.printf("Target: %d°C, Power: %d\n", targetTemp, powerLevel);

  // Enable auto control when starting heater
  if (tempSensorFound) {
    autoControlEnabled = true;
    Serial.println("Auto control: ENABLED");
  }

  // Send start command to heater
  if (heater.start(powerLevel)) {
    uiState = UI_STATE_RUNNING;
    lastAutoStateChange = millis();  // Track state change for min cycle time
    setLED(true);
    Serial.println("Heater start command sent");
  } else {
    Serial.println("ERROR: Failed to start heater");
    hasError = true;
  }

  // Update display immediately
  updateDisplay();
}

void stopHeater() {
  Serial.println("\n=== STOPPING HEATER ===");
  Serial.println("Entering COOLING mode...");

  // Disable auto control when user manually stops
  autoControlEnabled = false;

  // Send stop command to heater
  if (heater.stop()) {
    uiState = UI_STATE_COOLING;
    coolingStartTime = millis();
    Serial.println("Heater stop command sent");
  } else {
    Serial.println("ERROR: Failed to stop heater");
  }

  // Update display immediately
  updateDisplay();
}

void autoStopHeater() {
  Serial.println("\n=== AUTO STOP (Over-temperature) ===");
  Serial.printf("Temp: %.1f°C, Target: %d°C (%.1f°C above)\n",
                smoothedTemp, targetTemp, smoothedTemp - targetTemp);

  // Send stop command to heater
  if (heater.stop()) {
    uiState = UI_STATE_COOLING;  // Will transition to AUTO_OFF after cooling
    coolingStartTime = millis();
    lastAutoStateChange = millis();
    Serial.println("Heater stop command sent (auto)");
    Serial.printf("Will wait %.1f minutes before next state change\n", MIN_CYCLE_TIME / 60000.0);
  } else {
    Serial.println("ERROR: Failed to auto-stop heater");
  }

  updateDisplay();
}

void autoStartHeater() {
  Serial.println("\n=== AUTO START (Under-temperature) ===");
  Serial.printf("Temp: %.1f°C, Target: %d°C (%.1f°C below)\n",
                smoothedTemp, targetTemp, targetTemp - smoothedTemp);
  Serial.printf("Restarting at power level %d\n", POWER_RESTART);

  // Send start command to heater
  if (heater.start(POWER_RESTART)) {
    uiState = UI_STATE_RUNNING;
    powerLevel = POWER_RESTART;
    lastAutoStateChange = millis();
    setLED(true);
    Serial.println("Heater auto-start command sent");
    Serial.printf("Will wait %.1f minutes before next state change\n", MIN_CYCLE_TIME / 60000.0);
  } else {
    Serial.println("ERROR: Failed to auto-start heater");
    hasError = true;
  }

  updateDisplay();
}

// ========== AUTO TEMPERATURE CONTROL ==========
void autoTemperatureControl() {
  if (!autoControlEnabled || !tempSensorFound) return;

  // Use smoothed temperature for control decisions (reduces sensor noise)
  float T = smoothedTemp;
  float Tset = targetTemp;
  float trend = tempTrend;  // °C/min
  unsigned long now = millis();
  unsigned long timeSinceLastChange = now - lastAutoStateChange;

  // PREDICTIVE CONTROL: Estimate temperature N minutes ahead
  float predictedTemp = T + (trend * LOOKAHEAD_TIME);
  float delta = T - Tset;  // Current temperature error
  float predictedDelta = predictedTemp - Tset;  // Predicted error

  // ========== WHEN HEATER IS RUNNING ==========
  if (uiState == UI_STATE_RUNNING) {
    // Check if we should auto-stop (>2°C over-temperature)
    if (T > Tset + THRESH_AUTO_OFF) {
      // Only stop if minimum ON time has elapsed
      if (timeSinceLastChange >= MIN_CYCLE_TIME) {
        autoStopHeater();
        return;
      } else {
        // Still in minimum ON time, just log
        if ((timeSinceLastChange % 60000) < 2000) {  // Log once per minute
          float minutesRemaining = (MIN_CYCLE_TIME - timeSinceLastChange) / 60000.0;
          Serial.printf("[AUTO] Over-temp (%.1f°C), but waiting %.1f more minutes before stop\n",
                        T - Tset, minutesRemaining);
        }
      }
    }

    // ========== TREND-BASED PREDICTIVE POWER CONTROL ==========
    int targetPower;

    // Zone 1: FAR BELOW TARGET (< -2°C)
    if (delta < -THRESH_FAR_BELOW) {
      targetPower = POWER_FAR_BELOW;
    }
    // Zone 2: BELOW TARGET (-2°C to -1°C)
    else if (delta < -THRESH_BELOW) {
      if (trend > TREND_FAST) {
        targetPower = POWER_APPROACHING;  // Fast heating -> reduce early
      } else if (trend > TREND_MEDIUM) {
        targetPower = POWER_BELOW - 1;  // Medium -> power 6
      } else {
        targetPower = POWER_BELOW;  // Slow -> maintain strong power
      }
    }
    // Zone 3: APPROACHING TARGET (-1°C to -0.5°C)
    else if (delta < -THRESH_APPROACHING) {
      if (predictedDelta > 0) {
        targetPower = POWER_ABOVE;  // Predicted overshoot -> minimal
      } else if (trend > TREND_FAST) {
        targetPower = POWER_FINE_TUNE;  // Fast trend -> fine tune
      } else {
        targetPower = POWER_APPROACHING;  // Normal approach
      }
    }
    // Zone 4: FINE TUNING (-0.5°C to -0.2°C)
    else if (delta < -THRESH_FINE_TUNE) {
      if (trend > TREND_MEDIUM) {
        targetPower = POWER_ABOVE;  // Still heating fast -> minimal
      } else {
        targetPower = POWER_FINE_TUNE;  // Normal fine tuning
      }
    }
    // Zone 5: AT TARGET (±0.5°C)
    else if (fabs(delta) <= THRESH_AT_TARGET) {
      if (trend < -TREND_SLOW) {
        targetPower = POWER_AT_TARGET + 1;  // Cooling -> maintenance+
      } else if (trend > TREND_SLOW) {
        targetPower = POWER_ABOVE;  // Still heating -> maintenance-
      } else {
        targetPower = POWER_AT_TARGET;  // Stable -> normal maintenance
      }
    }
    // Zone 6: ABOVE TARGET (+0.5°C to +2°C)
    else if (delta < THRESH_AUTO_OFF) {
      targetPower = POWER_ABOVE;  // Above target -> minimal
    }
    // Zone 7: FAR ABOVE (>+2°C) - handled by auto-stop above
    else {
      targetPower = POWER_ABOVE;  // Over-temp, waiting for min cycle
    }

    // Apply power change if needed (only log on actual change)
    uint8_t currentPower = heater.getPower();
    if (targetPower != currentPower) {
      heater.setPower(targetPower);
      powerLevel = targetPower;  // Update UI variable
      Serial.printf("[AUTO] Power: %d->%d | T=%.1f°C, Target=%.1f°C, Trend=%.2f°C/min, Predicted=%.1f°C\n",
                    currentPower, targetPower, T, Tset, trend, predictedTemp);
    }
  }

  // ========== WHEN HEATER IS IN AUTO-OFF MODE ==========
  else if (uiState == UI_STATE_AUTO_OFF) {
    // Check if we should auto-restart (under-temperature)
    if (T < Tset + THRESH_AUTO_RESTART) {
      // Only restart if minimum OFF time has elapsed
      if (timeSinceLastChange >= MIN_CYCLE_TIME) {
        autoStartHeater();
        return;
      } else {
        // Still in minimum OFF time, just log
        if ((timeSinceLastChange % 60000) < 2000) {  // Log once per minute
          float minutesRemaining = (MIN_CYCLE_TIME - timeSinceLastChange) / 60000.0;
          Serial.printf("[AUTO] Under-temp (%.1f°C below target), but waiting %.1f more minutes before restart\n",
                        Tset - T, minutesRemaining);
        }
      }
    }
  }
}

// ========== CONFIGURATION/DIAGNOSTIC ADJUSTMENT ==========
void cycleDiagnostic(int delta) {
  currentDiagnostic += delta;
  if (currentDiagnostic < 0) currentDiagnostic = DIAG_COUNT - 1;
  if (currentDiagnostic >= DIAG_COUNT) currentDiagnostic = 0;

  const char* diagNames[] = {"Voltage", "Inner Temp", "Fan Speed", "Pump Freq"};
  Serial.printf("Diagnostic: %s\n", diagNames[currentDiagnostic]);
}

// ========== DISPLAY FUNCTIONS ==========
void displayOffMode() {
  // Row 1: Current temperature
  display.setFont();
  display.setTextSize(1);
  display.setCursor(0, 0);

  if (tempSensorFound && currentTemp > -100) {
    int tempInt = (int)(currentTemp + 0.5);
    display.printf("Current: %d", tempInt);
    display.write(247);  // Degree symbol
    display.print("C");
  } else {
    display.print("No sensor");
  }

  // Row 2: "OFF" or error message (large font)
  if (hasError && errorMessage.length() > 0) {
    // Display scrolling error message
    display.setFont();
    display.setTextSize(1);
    display.setCursor(-scrollOffset, 30);
    display.print(errorMessage);
  } else {
    fonts.setFont(FONT_LARGE, FONT_SANS_BOLD);
    fonts.printCentered("OFF", 0, 20, 128, 44);
  }
}

void displayRunningMode() {
  // Row 1: Left = Heater state, Right = "P:X"
  display.setFont();
  display.setTextSize(1);
  display.setCursor(0, 0);

  // Show real heater state
  HeaterState heaterState = heater.getState();
  const char* stateStr = VevorHeater::getStateString(heaterState);
  display.print(stateStr);

  // Power level on right (show 0 when OFF or COOLDOWN, otherwise actual heater power)
  uint8_t actualPower = (heaterState == STATE_OFF || heaterState == STATE_COOLDOWN) ? 0 : heater.getPower();
  char powerStr[8];
  snprintf(powerStr, sizeof(powerStr), "P:%d", actualPower);
  int16_t textWidth = strlen(powerStr) * 6;
  display.setCursor(128 - textWidth, 0);
  display.print(powerStr);

  // Check for error
  if (hasError && errorMessage.length() > 0) {
    // Display scrolling error message in row 2
    display.setFont();
    display.setTextSize(1);
    display.setCursor(-scrollOffset, 30);
    display.print(errorMessage);
  } else {
    // Row 2: Small labels "Current" and "Target" above temperatures
    display.setFont();
    display.setTextSize(1);
    display.setCursor(10, 14);
    display.print("Current");
    display.setCursor(74, 14);
    display.print("Target");

    // Row 2: Current temp (left) and Target temp (right) - large font
    // Positioned closer to labels
    fonts.setFont(FONT_LARGE, FONT_SANS_BOLD);

    // Current temp on the left
    if (tempSensorFound && currentTemp > -100) {
      int currentTempInt = (int)(currentTemp + 0.5);
      char currentStr[8];
      snprintf(currentStr, sizeof(currentStr), "%d", currentTempInt);
      fonts.printCentered(currentStr, 0, 26, 64, 38);  // Left half, moved up
    } else {
      fonts.printCentered("--", 0, 26, 64, 38);  // Left half, moved up
    }

    // Target temp on the right
    char targetStr[8];
    snprintf(targetStr, sizeof(targetStr), "%d", targetTemp);
    fonts.printCentered(targetStr, 64, 26, 64, 38);  // Right half, moved up
  }
}

void displayCoolingMode() {
  // Row 1: Empty (no state text to avoid confusion)
  display.setFont();
  display.setTextSize(1);
  display.setCursor(0, 0);
  display.print("Shutting down...");

  // Row 2: "COOLING" message
  display.setFont();
  display.setTextSize(2);

  // Center "COOLING"
  int textWidth = 7 * 12;  // 7 chars at size 2
  int x = (128 - textWidth) / 2;
  display.setCursor(x, 28);
  display.print("COOLING");
}

void displayAutoOffMode() {
  // Row 1: Current temperature and AUTO indicator
  display.setFont();
  display.setTextSize(1);
  display.setCursor(0, 0);

  if (tempSensorFound && currentTemp > -100) {
    int tempInt = (int)(currentTemp + 0.5);
    display.printf("Current: %d", tempInt);
    display.write(247);  // Degree symbol
    display.print("C");
  } else {
    display.print("No sensor");
  }

  // "AUTO" on the right
  display.setCursor(95, 0);
  display.print("AUTO");

  // Row 2: "AUTO OFF" message
  display.setFont();
  display.setTextSize(2);

  // Center "AUTO OFF"
  int textWidth = 8 * 12;  // 8 chars at size 2
  int x = (128 - textWidth) / 2;
  display.setCursor(x, 24);
  display.print("AUTO OFF");

  // Row 3: Time until restart consideration (small font)
  unsigned long timeSinceChange = millis() - lastAutoStateChange;
  if (timeSinceChange < MIN_CYCLE_TIME) {
    display.setFont();
    display.setTextSize(1);
    float minutesRemaining = (MIN_CYCLE_TIME - timeSinceChange) / 60000.0;
    char timeStr[32];
    snprintf(timeStr, sizeof(timeStr), "Wait %.0fm", minutesRemaining);
    int textW = strlen(timeStr) * 6;
    display.setCursor((128 - textW) / 2, 52);
    display.print(timeStr);
  }
}

void displayConfigMode() {
  // Row 1: Diagnostic label (small font)
  display.setFont();
  display.setTextSize(1);
  display.setCursor(0, 0);

  const char* labels[] = {"Voltage", "Inner Temp", "Fan Speed", "Pump Freq"};
  display.print(labels[currentDiagnostic]);

  // Row 2: Diagnostic value (large font)
  char valueStr[16];

  switch (currentDiagnostic) {
    case DIAG_VOLTAGE:
      snprintf(valueStr, sizeof(valueStr), "%.1fV", heater.getVoltage());
      break;

    case DIAG_INNER_TEMP:
      snprintf(valueStr, sizeof(valueStr), "%.0fC", heater.getTemperature());
      break;

    case DIAG_FAN_SPEED:
      snprintf(valueStr, sizeof(valueStr), "%drpm", heater.getFanSpeed());
      break;

    case DIAG_PUMP_FREQ:
      snprintf(valueStr, sizeof(valueStr), "%.1fHz", heater.getPumpFrequency());
      break;
  }

  fonts.setFont(FONT_LARGE, FONT_SANS_BOLD);
  fonts.printCentered(valueStr, 0, 20, 128, 44);
}

void updateDisplay() {
  display.clearDisplay();

  switch (uiState) {
    case UI_STATE_OFF:
      displayOffMode();
      break;

    case UI_STATE_RUNNING:
      displayRunningMode();
      break;

    case UI_STATE_COOLING:
      displayCoolingMode();
      break;

    case UI_STATE_CONFIG:
      displayConfigMode();
      break;

    case UI_STATE_AUTO_OFF:
      displayAutoOffMode();
      break;
  }

  display.display();
}

// ========== ERROR MESSAGE SCROLLING ==========
void updateScrolling() {
  if (hasError && errorMessage.length() > 0) {
    if (millis() - lastScrollTime >= SCROLL_INTERVAL) {
      lastScrollTime = millis();

      // Calculate message width in pixels (approx 6 pixels per character)
      int messageWidth = errorMessage.length() * 6;

      // Scroll if message is wider than screen
      if (messageWidth > 128) {
        scrollOffset++;
        // Reset when message has scrolled off screen
        if (scrollOffset > messageWidth) {
          scrollOffset = -128;  // Start from right edge
        }
      } else {
        scrollOffset = 0;  // No scrolling needed
      }
    }
  } else {
    scrollOffset = 0;
  }
}

// ========== WEB API CALLBACKS ==========
void apiStartHeater() {
  Serial.println("[API] Start command received");
  if (uiState == UI_STATE_OFF) {
    startHeater();
  } else {
    Serial.println("[API] Heater already running");
  }
}

void apiStopHeater() {
  Serial.println("[API] Stop command received");
  if (uiState == UI_STATE_RUNNING || uiState == UI_STATE_CONFIG) {
    if (uiState == UI_STATE_CONFIG) {
      uiState = UI_STATE_RUNNING;  // Exit config first
    }
    stopHeater();
  } else {
    Serial.println("[API] Heater not running");
  }
}

void apiSetTarget(int temp) {
  Serial.printf("[API] Set target to %d°C\n", temp);
  targetTemp = constrain(temp, MIN_TEMP, MAX_TEMP);
  updateDisplay();
}

void apiSetPower(int power) {
  Serial.printf("[API] Set power to %d\n", power);
  if (uiState == UI_STATE_RUNNING) {
    powerLevel = constrain(power, MIN_POWER, MAX_POWER);
    heater.setPower(powerLevel);
    // Disable auto control when manually setting power via API
    autoControlEnabled = false;
    Serial.println("[API] Auto control disabled (manual power set)");
    updateDisplay();
  } else {
    Serial.println("[API] Heater not running, power change ignored");
  }
}

// ========== WEB SERVER INITIALIZATION ==========
void startWebServer() {
  if (webServerStarted) {
    Serial.println("Web server already running");
    return;
  }

  webServer.setHeater(&heater);
  webServer.setTemperatureData(&currentTemp, &smoothedTemp, &tempTrend, &targetTemp);
  webServer.setAutoControlEnabled(&autoControlEnabled);
  webServer.setStartCallback(apiStartHeater);
  webServer.setStopCallback(apiStopHeater);
  webServer.setTargetCallback(apiSetTarget);
  webServer.setPowerCallback(apiSetPower);
  webServer.begin();
  webServerStarted = true;

  Serial.printf("\nWeb server available at: http://%s\n", wifiManager.getIP().c_str());
  Serial.println("REST API ENDPOINTS:");
  Serial.println("  GET  /api/status        : Full status");
  Serial.println("  GET  /api/temperature   : Temperature data");
  Serial.println("  POST /api/authenticate  : Get auth token");
  Serial.println("  POST /api/start         : Start heater*");
  Serial.println("  POST /api/stop          : Stop heater*");
  Serial.println("  POST /api/target        : Set target temp*");
  Serial.println("  POST /api/power         : Set power level*");
  Serial.println("  GET  /api/signalk/values: SignalK format");
  Serial.println("  (* requires Authorization: Bearer <token>)\n");
}

// ========== SETUP ==========
void setup() {
  Serial.begin(115200);
  delay(1000);

  Serial.println("\n========================================");
  Serial.println("  Vevor Heater Controller v2.0");
  Serial.println("  OTA UPDATE TEST - Build 3 (VIA OTA!)");
  Serial.println("========================================\n");

  // Initialize LED
  pinMode(LED_PIN, OUTPUT);
  setLED(false);
  Serial.println("LED initialized");

  // Initialize I2C FIRST with correct pins
  Wire.begin(I2C_SDA_PIN, I2C_SCL_PIN);
  Wire.setClock(100000);
  delay(100);

  // Initialize OLED (false = don't reset Wire, we already configured it)
  if (!display.begin(OLED_ADDR, false)) {
    Serial.println("ERROR: OLED initialization failed!");
    while (1) {
      setLED(!ledState);
      delay(200);
    }
  }
  display.setTextColor(SH110X_WHITE);
  Serial.println("OLED initialized");

  // Initialize encoder
  encoder.begin();
  encoder.setLongPressThreshold(1500);  // 1.5 seconds for long press
  Serial.println("Encoder initialized");

  // Initialize temperature sensor
  pinMode(TEMP_SENSOR_PIN, INPUT_PULLUP);  // Enable internal pull-up
  delay(100);  // Give pull-up time to stabilize

  tempSensor.begin();
  int sensorCount = tempSensor.getDeviceCount();
  Serial.printf("Temperature sensors found: %d\n", sensorCount);

  if (sensorCount > 0) {
    tempSensor.setResolution(12);  // 12-bit resolution
    tempSensorFound = true;
  } else {
    Serial.println("WARNING: No temperature sensor detected!");
  }

  // Initialize heater communication
  heater.begin();
  Serial.println("Heater communication initialized");

  // Initialize WiFi manager
  wifiManager.begin();

  // Initialize authentication manager
  authManager.begin();

  // Attempt WiFi connection if credentials are saved
  if (wifiManager.hasCredentials()) {
    Serial.println("\nAttempting WiFi connection...");
    if (wifiManager.connect(10000)) {  // 10 second timeout
      Serial.println("WiFi connected successfully!");
    } else {
      Serial.println("WiFi connection failed, starting AP mode...");
      wifiManager.startAP();
    }
  } else {
    Serial.println("\nNo WiFi credentials saved");
    Serial.println("Use 'wifi ssid' and 'wifi pass' commands to configure");
  }

  // Debug: Print WiFi status before web server init
  Serial.println("\n========== WiFi Status Check ==========");
  Serial.printf("WiFi.status(): %d (WL_CONNECTED=%d)\n", WiFi.status(), WL_CONNECTED);
  Serial.printf("WiFi.getMode(): %d (WIFI_AP=%d)\n", WiFi.getMode(), WIFI_AP);
  Serial.printf("isConnected(): %s\n", wifiManager.isConnected() ? "true" : "false");
  Serial.printf("Local IP: %s\n", WiFi.localIP().toString().c_str());
  Serial.println("=======================================\n");

  // Initialize web server (if WiFi is connected or in AP mode)
  if (wifiManager.isConnected() || WiFi.getMode() == WIFI_AP) {
    startWebServer();
    Serial.println("[WEB] Server started");
  } else {
    Serial.println("[WEB] Server NOT started - no WiFi connection");
  }

  // Initialize OTA (Over-The-Air) updates - Always try if any WiFi mode is active
  Serial.println("\n[OTA] Initializing ArduinoOTA...");

  ArduinoOTA.setHostname("vevor-heater");
  ArduinoOTA.setPassword("1234");  // Same as authentication PIN
  ArduinoOTA.setPort(3232);  // Explicit port setting

  ArduinoOTA.onStart([]() {
    String type;
    if (ArduinoOTA.getCommand() == U_FLASH) {
      type = "firmware";
    } else {  // U_SPIFFS
      type = "filesystem";
    }
    Serial.println("\n[OTA] Starting update: " + type);
    // Stop heater during OTA update for safety
    if (uiState == UI_STATE_RUNNING || uiState == UI_STATE_CONFIG) {
      heater.stop();
    }
  });

  ArduinoOTA.onEnd([]() {
    Serial.println("\n[OTA] Update complete!");
  });

  ArduinoOTA.onProgress([](unsigned int progress, unsigned int total) {
    static unsigned int lastPercent = 0;
    unsigned int percent = (progress / (total / 100));
    if (percent != lastPercent && percent % 10 == 0) {
      Serial.printf("[OTA] Progress: %u%%\n", percent);
      lastPercent = percent;
    }
  });

  ArduinoOTA.onError([](ota_error_t error) {
    Serial.printf("[OTA] Error[%u]: ", error);
    if (error == OTA_AUTH_ERROR) Serial.println("Auth Failed");
    else if (error == OTA_BEGIN_ERROR) Serial.println("Begin Failed");
    else if (error == OTA_CONNECT_ERROR) Serial.println("Connect Failed");
    else if (error == OTA_RECEIVE_ERROR) Serial.println("Receive Failed");
    else if (error == OTA_END_ERROR) Serial.println("End Failed");
  });

  ArduinoOTA.begin();
  Serial.printf("[OTA] ✓ ArduinoOTA.begin() called successfully\n");
  Serial.printf("[OTA] ✓ Hostname: vevor-heater, Port: 3232\n");
  Serial.printf("[OTA] ✓ IP: %s\n", WiFi.localIP().toString().c_str());
  Serial.printf("[OTA] ✓ Upload command: pio run -t upload --upload-port %s\n\n", WiFi.localIP().toString().c_str());

  Serial.println("\n========================================");
  Serial.println("CONTROLS:");
  Serial.println("  Long Press (1.5s): Turn heater ON/OFF");
  Serial.println("  Short Click:       Enter/exit CONFIG");
  Serial.println("  Rotate:            Adjust target temp");
  Serial.println("\nSERIAL COMMANDS:");
  Serial.println("  wifi ssid <name>  : Set WiFi SSID");
  Serial.println("  wifi pass <pass>  : Set WiFi password");
  Serial.println("  wifi connect      : Connect to WiFi");
  Serial.println("  wifi status       : Show WiFi status");
  Serial.println("  pin set <4-digit> : Set new PIN");
  Serial.println("  pin <4-digit>     : Verify PIN");
  Serial.println("  a                 : Enable AUTO mode");
  Serial.println("  d                 : Disable AUTO mode");
  Serial.println("========================================\n");

  // Initial display
  updateDisplay();

  // Blink LED to show ready
  for (int i = 0; i < 3; i++) {
    setLED(true);
    delay(100);
    setLED(false);
    delay(100);
  }

  Serial.println("System ready!\n");
}

// ========== MAIN LOOP ==========
void loop() {
  // Handle OTA updates (must be called frequently)
  ArduinoOTA.handle();

  // Update encoder
  encoder.update();

  // Update heater communication
  heater.update();

  // Read temperature periodically
  readTemperature();

  // Handle rotation
  int delta = encoder.getDelta();
  if (delta != 0) {
    Serial.printf("Rotation: %d\n", delta);

    switch (uiState) {
      case UI_STATE_OFF:
        // In OFF mode, rotation adjusts target temp
        targetTemp = constrain(targetTemp + delta, MIN_TEMP, MAX_TEMP);
        Serial.printf("Target temp: %d°C\n", targetTemp);
        break;

      case UI_STATE_RUNNING:
        // In RUNNING mode, rotation adjusts target temp
        targetTemp = constrain(targetTemp + delta, MIN_TEMP, MAX_TEMP);
        Serial.printf("Target temp: %d°C\n", targetTemp);
        break;

      case UI_STATE_COOLING:
        // Ignore rotation during cooling
        break;

      case UI_STATE_AUTO_OFF:
        // In AUTO_OFF mode, rotation adjusts target temp
        targetTemp = constrain(targetTemp + delta, MIN_TEMP, MAX_TEMP);
        Serial.printf("Target temp: %d°C\n", targetTemp);
        break;

      case UI_STATE_CONFIG:
        // In CONFIG mode, rotation cycles through diagnostics
        cycleDiagnostic(delta);
        break;
    }

    // Update display immediately on rotation (safe with polling mode)
    updateDisplay();
  }

  // Handle button events
  EncoderButton btn = encoder.getButton();

  if (btn == BTN_LONG_PRESS && !buttonLongPressHandled) {
    Serial.println("Button: LONG PRESS");
    buttonLongPressHandled = true;  // Mark as handled to prevent retriggering

    if (uiState == UI_STATE_CONFIG) {
      // Exit config mode first
      uiState = UI_STATE_RUNNING;
      Serial.println("Exited config mode");
    }

    // Toggle heater on/off
    if (uiState == UI_STATE_OFF || uiState == UI_STATE_AUTO_OFF) {
      startHeater();
    } else if (uiState == UI_STATE_RUNNING) {
      stopHeater();
    }

    updateDisplay();

  } else if (btn == BTN_SHORT_PRESS) {
    Serial.println("Button: SHORT PRESS");
    buttonLongPressHandled = false;  // Reset long press flag on short press

    // Short press only works when heater is running
    if (uiState == UI_STATE_RUNNING) {
      // Enter config/diagnostic mode
      uiState = UI_STATE_CONFIG;
      currentDiagnostic = DIAG_VOLTAGE;
      Serial.println("Entered diagnostic mode");
      updateDisplay();

    } else if (uiState == UI_STATE_CONFIG) {
      // Exit config/diagnostic mode
      uiState = UI_STATE_RUNNING;
      Serial.println("Exited diagnostic mode");
      updateDisplay();
    }
  }

  // Reset long press flag when button is released
  if (!encoder.isPressed() && buttonLongPressHandled) {
    buttonLongPressHandled = false;
  }

  // Check if heater has finished cooling down
  if (uiState == UI_STATE_COOLING) {
    HeaterState heaterState = heater.getState();
    unsigned long coolingTime = millis() - coolingStartTime;

    // Exit cooling if heater is OFF, or timeout reached (3 min)
    bool heaterIsOff = (heaterState == STATE_OFF || heaterState == STATE_UNKNOWN);
    bool timeoutReached = (coolingTime >= COOLING_TIMEOUT);

    if (heaterIsOff || timeoutReached) {
      if (heaterIsOff) {
        Serial.println("\n=== COOLING COMPLETE ===");
        Serial.printf("Heater state: %s\n", VevorHeater::getStateString(heaterState));
      } else {
        Serial.println("\n=== COOLING TIMEOUT ===");
        Serial.printf("Timeout after %.1f minutes\n", coolingTime / 60000.0);
      }

      // Transition to AUTO_OFF if auto control is enabled, otherwise manual OFF
      if (autoControlEnabled && tempSensorFound) {
        Serial.println("Entering AUTO_OFF mode (auto control active)");
        uiState = UI_STATE_AUTO_OFF;
      } else {
        Serial.println("Heater now OFF (manual)");
        uiState = UI_STATE_OFF;
      }
      updateDisplay();
    }
  }

  // Run auto temperature control
  autoTemperatureControl();

  // Update LED state
  updateLED();

  // Update error message scrolling
  updateScrolling();

  // Update display periodically (in all modes)
  if (millis() - lastDisplayUpdate >= DISPLAY_UPDATE_INTERVAL) {
    lastDisplayUpdate = millis();
    updateDisplay();
  }

  // Handle serial commands
  if (Serial.available()) {
    String command = Serial.readStringUntil('\n');
    command.trim();

    if (command.length() == 0) {
      // Ignore empty lines
    }
    else if (command.startsWith("wifi ssid ")) {
      String ssid = command.substring(10);
      wifiManager.setSSID(ssid);
    }
    else if (command.startsWith("wifi pass ")) {
      String password = command.substring(10);
      wifiManager.setPassword(password);
    }
    else if (command == "wifi connect") {
      Serial.println("\nConnecting to WiFi...");
      if (wifiManager.connect(30000)) {
        Serial.println("SUCCESS!");
        // Start web server after successful connection
        startWebServer();
        // Start OTA after successful connection
        ArduinoOTA.setHostname("vevor-heater");
        ArduinoOTA.setPassword("1234");
        ArduinoOTA.begin();
        Serial.printf("[OTA] Ready! IP: %s\n", wifiManager.getIP().c_str());
      } else {
        Serial.println("FAILED - check credentials");
      }
    }
    else if (command == "wifi status") {
      Serial.println("\n========== WiFi Status ==========");
      Serial.printf("Status: %s\n", wifiManager.getStatusString().c_str());
      Serial.printf("SSID: %s\n", wifiManager.getSSID().c_str());
      if (wifiManager.isConnected()) {
        Serial.printf("IP: %s\n", wifiManager.getIP().c_str());
      }
      Serial.println("==================================\n");
    }
    else if (command.startsWith("pin set ")) {
      String pin = command.substring(8);
      if (authManager.setPIN(pin)) {
        Serial.println("PIN set successfully");
      }
    }
    else if (command.startsWith("pin ")) {
      String pin = command.substring(4);
      if (authManager.verifyPIN(pin)) {
        Serial.println("PIN verified - access granted");
      } else {
        Serial.println("PIN incorrect - access denied");
      }
    }
    else if (command == "a" || command == "A") {
      if (tempSensorFound) {
        autoControlEnabled = true;
        Serial.println("\n=== AUTO MODE ENABLED ===");
        Serial.printf("Target: %d°C\n", targetTemp);
        Serial.println("Heater will automatically adjust power");
        updateDisplay();
      } else {
        Serial.println("\nERROR: Cannot enable auto mode - no temperature sensor!");
      }
    }
    else if (command == "d" || command == "D") {
      autoControlEnabled = false;
      Serial.println("\n=== AUTO MODE DISABLED ===");
      Serial.println("Manual control active");
      updateDisplay();
    }
    else {
      Serial.println("Unknown command. Available commands:");
      Serial.println("  wifi ssid <name>, wifi pass <password>, wifi connect, wifi status");
      Serial.println("  pin set <4-digit>, pin <4-digit>");
      Serial.println("  a (auto on), d (auto off)");
    }
  }

  // Update WiFi connection
  wifiManager.update();

  // Small delay to avoid overwhelming the system
  // 10ms gives ~100 updates/sec which is fast enough for responsive encoder
  delay(10);
}
