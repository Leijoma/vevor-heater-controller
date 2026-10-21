/**
 * RotaryEncoder Library Test Program
 *
 * Demonstrates all features:
 * - Rotation detection with configurable resolution
 * - Short press
 * - Long press
 * - Double click
 */

#include <Arduino.h>
#include <Wire.h>
#include <Adafruit_SH110X.h>
#include <RotaryEncoder.h>

// ========== HARDWARE PINS ==========
#define ENCODER_CLK 33
#define ENCODER_DT  26
#define ENCODER_SW  32
#define LED_PIN     18
#define I2C_SDA_PIN 22
#define I2C_SCL_PIN 21

// ========== OLED DISPLAY ==========
#define OLED_WIDTH  128
#define OLED_HEIGHT 64
#define OLED_ADDR   0x3C
#define OLED_RESET  -1

Adafruit_SH1106G display(OLED_WIDTH, OLED_HEIGHT, &Wire, OLED_RESET);

// ========== ROTARY ENCODER ==========
// Resolution: 4 steps per detent (most common for mechanical encoders)
RotaryEncoder encoder(ENCODER_CLK, ENCODER_DT, ENCODER_SW, 4);

// ========== STATE VARIABLES ==========
int value = 0;
int minValue = 0;
int maxValue = 100;
int step = 1;

// For display
String lastEvent = "Ready";
unsigned long lastEventTime = 0;

// ========== LED PATTERNS ==========
void ledBlink(int times, int delayMs = 100) {
  for (int i = 0; i < times; i++) {
    digitalWrite(LED_PIN, HIGH);
    delay(delayMs);
    digitalWrite(LED_PIN, LOW);
    if (i < times - 1) delay(delayMs);
  }
}

void ledPulse() {
  digitalWrite(LED_PIN, HIGH);
  delay(30);
  digitalWrite(LED_PIN, LOW);
}

// ========== DISPLAY UPDATE ==========
void updateDisplay() {
  display.clearDisplay();

  // Title
  display.setTextSize(1);
  display.setCursor(0, 0);
  display.print("Encoder Library Test");

  // Current value (large)
  display.setTextSize(3);
  display.setCursor(20, 20);
  display.print(value);

  // Step size
  display.setTextSize(1);
  display.setCursor(0, 48);
  display.print("Step: ");
  display.print(step);

  // Last event
  display.setCursor(60, 48);
  display.print(lastEvent);

  // Instructions
  display.setCursor(0, 56);
  display.print("Turn:+/- Btn:Events");

  display.display();
}

// ========== SETUP ==========
void setup() {
  Serial.begin(115200);
  delay(1000);

  Serial.println("\n========================================");
  Serial.println("  RotaryEncoder Library Test");
  Serial.println("========================================\n");

  // Initialize LED
  pinMode(LED_PIN, OUTPUT);
  digitalWrite(LED_PIN, LOW);
  Serial.println("✓ LED initialized");

  // Initialize I2C and OLED
  Wire.begin(I2C_SDA_PIN, I2C_SCL_PIN);
  Wire.setClock(100000);
  delay(100);

  if (!display.begin(OLED_ADDR, true)) {
    Serial.println("ERROR: OLED initialization failed!");
    while (1) {
      ledBlink(5, 50);
      delay(1000);
    }
  }
  display.setTextColor(SH110X_WHITE);
  Serial.println("✓ OLED initialized");

  // Initialize encoder
  encoder.begin();

  // Configure encoder timing
  encoder.setLongPressThreshold(1000);  // 1 second for long press
  encoder.setDoubleClickWindow(400);     // 400ms for double click
  encoder.setDebounceTime(50);           // 50ms debounce

  Serial.println("✓ Encoder initialized");
  Serial.println("\n========================================");
  Serial.println("FEATURES:");
  Serial.println("--------");
  Serial.println("Rotate:        Change value");
  Serial.println("Short Press:   Reset to 0");
  Serial.println("Long Press:    Change to step=10");
  Serial.println("Double Click:  Change to step=1");
  Serial.println("========================================\n");

  // Blink LED 3 times to show ready
  ledBlink(3, 100);

  updateDisplay();
}

// ========== MAIN LOOP ==========
void loop() {
  // Update encoder state
  if (encoder.update()) {

    // Check for rotation
    int delta = encoder.getDelta();
    if (delta != 0) {
      value += delta * step;

      // Constrain to range
      if (value < minValue) value = minValue;
      if (value > maxValue) value = maxValue;

      // Show direction
      EncoderDirection dir = encoder.getDirection();
      if (dir == DIR_CW) {
        Serial.print("↑ Clockwise:     ");
        lastEvent = "CW";
      } else if (dir == DIR_CCW) {
        Serial.print("↓ Counter-CW:    ");
        lastEvent = "CCW";
      }

      Serial.print("Value = ");
      Serial.print(value);
      Serial.print(" (delta: ");
      Serial.print(delta * step);
      Serial.println(")");

      ledPulse();
      updateDisplay();
    }

    // Check for button events
    EncoderButton btn = encoder.getButton();
    if (btn != BTN_NONE) {

      switch (btn) {
        case BTN_SHORT_PRESS:
          Serial.println("\n*** SHORT PRESS - Reset to 0 ***\n");
          value = 0;
          lastEvent = "Short";
          ledBlink(1, 100);
          break;

        case BTN_LONG_PRESS:
          Serial.println("\n*** LONG PRESS - Step = 10 ***\n");
          step = 10;
          lastEvent = "Long";
          ledBlink(2, 200);
          break;

        case BTN_DOUBLE_CLICK:
          Serial.println("\n*** DOUBLE CLICK - Step = 1 ***\n");
          step = 1;
          lastEvent = "2xClk";
          ledBlink(3, 100);
          break;

        default:
          break;
      }

      lastEventTime = millis();
      updateDisplay();
    }
  }

  // Clear event label after 2 seconds
  if (millis() - lastEventTime > 2000 && lastEvent != "Ready") {
    lastEvent = "";
    updateDisplay();
  }

  delay(10);
}
