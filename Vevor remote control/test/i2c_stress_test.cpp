/**
 * I2C Stress Test with Encoder Interrupts
 *
 * This test reproduces the "Bus Invalid State, sda=0, scl=1" error
 * by combining:
 * - OLED display on swapped I2C pins (SDA=22, SCL=21)
 * - Encoder with CLK interrupt
 * - Continuous display updates
 *
 * Purpose: Diagnose I2C bus conflicts with encoder interrupts
 */

#include <Arduino.h>
#include <Wire.h>
#include <Adafruit_SH110X.h>
#include <RotaryEncoder.h>

// ========== HARDWARE PINS ==========
#define I2C_SDA_PIN     22  // OLED SDA (SWAPPED!)
#define I2C_SCL_PIN     21  // OLED SCL (SWAPPED!)
#define ENCODER_CLK     33  // Rotary encoder CLK
#define ENCODER_DT      26  // Rotary encoder DT
#define ENCODER_SW      32  // Rotary encoder button

// ========== DISPLAY ==========
#define OLED_WIDTH  128
#define OLED_HEIGHT 64
#define OLED_ADDR   0x3C
#define OLED_RESET  -1

Adafruit_SH1106G display(OLED_WIDTH, OLED_HEIGHT, &Wire, OLED_RESET);

// ========== ENCODER ==========
RotaryEncoder encoder(ENCODER_CLK, ENCODER_DT, ENCODER_SW, 2);

// ========== TEST CONFIGURATION ==========
enum TestMode {
  TEST_NO_INTERRUPTS = 0,     // Polling only, no interrupts
  TEST_WITH_INTERRUPTS = 1,   // With CLK interrupt
  TEST_FAST_UPDATES = 2,      // Fast display updates + interrupts
  TEST_MODE_COUNT
};

TestMode currentMode = TEST_NO_INTERRUPTS;
const char* modeNames[] = {
  "NO INTERRUPTS (polling)",
  "WITH INTERRUPTS",
  "FAST UPDATES + INTERRUPTS"
};

// ========== STATISTICS ==========
unsigned long displayUpdateCount = 0;
unsigned long encoderEventCount = 0;
unsigned long lastStatsTime = 0;
unsigned long i2cErrorCount = 0;
unsigned long lastDisplayUpdate = 0;

// ========== DISPLAY UPDATE INTERVALS ==========
unsigned long updateIntervals[] = {
  500,  // TEST_NO_INTERRUPTS: 500ms
  500,  // TEST_WITH_INTERRUPTS: 500ms
  50    // TEST_FAST_UPDATES: 50ms (20 Hz)
};

// ========== FUNCTION DECLARATIONS ==========
void updateDisplay();
void showStats();
void switchMode(TestMode newMode);

// ========== SETUP ==========
void setup() {
  Serial.begin(115200);
  delay(2000);

  Serial.println("\n\n========================================");
  Serial.println("  I2C Stress Test - 400kHz");
  Serial.println("========================================");
  Serial.printf("OLED: SDA=GPIO%d, SCL=GPIO%d @ 400kHz\n", I2C_SDA_PIN, I2C_SCL_PIN);
  Serial.printf("Encoder: CLK=GPIO%d, DT=GPIO%d, SW=GPIO%d\n", ENCODER_CLK, ENCODER_DT, ENCODER_SW);
  Serial.println("\nCommands:");
  Serial.println("  0 - No interrupts (polling only)");
  Serial.println("  1 - With interrupts");
  Serial.println("  2 - Fast updates + interrupts");
  Serial.println("  s - Show statistics");
  Serial.println("========================================\n");

  // Initialize I2C with swapped pins at 400kHz (fast mode)
  Serial.println("Initializing I2C at 400kHz...");
  Wire.begin(I2C_SDA_PIN, I2C_SCL_PIN);
  Wire.setClock(400000);  // 400kHz instead of 100kHz
  delay(100);

  // Initialize OLED with false (don't reset Wire)
  Serial.println("Initializing OLED...");
  if (!display.begin(OLED_ADDR, false)) {
    Serial.println("ERROR: OLED initialization failed!");
    while (1) {
      delay(1000);
      Serial.println("OLED not found!");
    }
  }
  display.setTextColor(SH110X_WHITE);
  display.setTextSize(1);
  Serial.println("OLED initialized OK");

  // Initialize encoder (no interrupts yet)
  encoder.begin();
  Serial.println("Encoder initialized");

  // Start in mode 0 (no interrupts)
  Serial.println("\nStarting in mode 0: NO INTERRUPTS (polling)");
  Serial.println("Rotate encoder to test...\n");

  updateDisplay();
}

// ========== UPDATE DISPLAY ==========
void updateDisplay() {
  display.clearDisplay();

  // Title
  display.setTextSize(1);
  display.setCursor(0, 0);
  display.print("I2C Stress Test");

  // Mode
  display.setCursor(0, 12);
  display.print("Mode: ");
  display.print(currentMode);

  display.setCursor(0, 24);
  display.setTextSize(1);
  display.print(modeNames[currentMode]);

  // Encoder position
  display.setCursor(0, 40);
  display.setTextSize(2);
  display.print("Pos: ");
  display.print(encoder.getPosition());

  // Stats
  display.setTextSize(1);
  display.setCursor(0, 56);
  display.print("D:");
  display.print(displayUpdateCount);
  display.print(" E:");
  display.print(encoderEventCount);

  // CRITICAL: Disable encoder interrupt during I2C transaction to prevent bus corruption
  // Only disable if interrupt is actually enabled
  bool wasInterruptEnabled = (currentMode == TEST_WITH_INTERRUPTS || currentMode == TEST_FAST_UPDATES);
  if (wasInterruptEnabled) {
    detachInterrupt(digitalPinToInterrupt(ENCODER_CLK));
  }

  display.display();

  if (wasInterruptEnabled) {
    attachInterrupt(digitalPinToInterrupt(ENCODER_CLK), RotaryEncoder::isrCLK, CHANGE);
  }

  displayUpdateCount++;
}

// ========== SHOW STATS ==========
void showStats() {
  Serial.println("\n========== STATISTICS ==========");
  Serial.printf("Current mode: %d - %s\n", currentMode, modeNames[currentMode]);
  Serial.printf("Display updates: %lu\n", displayUpdateCount);
  Serial.printf("Encoder events: %lu\n", encoderEventCount);
  Serial.printf("I2C errors: %lu\n", i2cErrorCount);
  Serial.printf("Update interval: %lu ms\n", updateIntervals[currentMode]);
  Serial.printf("Uptime: %lu seconds\n", millis() / 1000);

  // Calculate rates
  if (millis() > 60000) {
    Serial.printf("Display update rate: %.1f/min\n", displayUpdateCount * 60000.0 / millis());
    Serial.printf("Encoder event rate: %.1f/min\n", encoderEventCount * 60000.0 / millis());
  }
  Serial.println("================================\n");
}

// ========== SWITCH MODE ==========
void switchMode(TestMode newMode) {
  if (newMode >= TEST_MODE_COUNT) return;

  Serial.printf("\n--- Switching to mode %d: %s ---\n", newMode, modeNames[newMode]);

  // Disable old interrupts if needed
  if (currentMode == TEST_WITH_INTERRUPTS || currentMode == TEST_FAST_UPDATES) {
    detachInterrupt(digitalPinToInterrupt(ENCODER_CLK));
    Serial.println("Encoder interrupt DISABLED");
  }

  currentMode = newMode;

  // Enable interrupts if needed
  if (currentMode == TEST_WITH_INTERRUPTS || currentMode == TEST_FAST_UPDATES) {
    attachInterrupt(digitalPinToInterrupt(ENCODER_CLK), []() {
      // ISR will be called by RotaryEncoder library
    }, CHANGE);
    Serial.println("Encoder interrupt ENABLED");
  }

  Serial.printf("Update interval: %lu ms\n", updateIntervals[currentMode]);
  Serial.println("Rotate encoder to test...\n");

  updateDisplay();
}

// ========== MAIN LOOP ==========
void loop() {
  unsigned long now = millis();

  // Always update encoder (polling in mode 0, interrupt handling in mode 1/2)
  if (encoder.update()) {
    encoderEventCount++;

    // In polling mode, print each event
    if (currentMode == TEST_NO_INTERRUPTS) {
      int delta = encoder.getDelta();
      if (delta != 0) {
        Serial.printf("Encoder: %+d (pos=%ld)\n", delta, encoder.getPosition());
      }
    }
  }

  // Handle button
  EncoderButton btn = encoder.getButton();
  if (btn == BTN_SHORT_PRESS) {
    Serial.println("Button pressed - switching mode...");
    switchMode((TestMode)((currentMode + 1) % TEST_MODE_COUNT));
  }

  // Update display periodically
  unsigned long interval = updateIntervals[currentMode];
  if (now - lastDisplayUpdate >= interval) {
    lastDisplayUpdate = now;
    updateDisplay();

    // Print summary every 10 seconds
    if (now - lastStatsTime >= 10000) {
      lastStatsTime = now;
      Serial.printf("[%6lu s] Mode=%d Updates=%lu Events=%lu\n",
                    now/1000, currentMode, displayUpdateCount, encoderEventCount);
    }
  }

  // Handle serial commands
  if (Serial.available()) {
    char cmd = Serial.read();
    switch (cmd) {
      case '0':
        switchMode(TEST_NO_INTERRUPTS);
        break;
      case '1':
        switchMode(TEST_WITH_INTERRUPTS);
        break;
      case '2':
        switchMode(TEST_FAST_UPDATES);
        break;
      case 's':
      case 'S':
        showStats();
        break;
      case '\n':
      case '\r':
        // Ignore newlines
        break;
      default:
        Serial.println("Unknown command. Press 0, 1, 2, or 's'");
        break;
    }
  }

  delay(1);
}
