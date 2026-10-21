/*
 * OLED Display Test Program
 *
 * Tests OLED display at 0x3C with multiple drivers and initialization methods
 * For 1.3" 128x64 OLED displays (SH1106 or SSD1306)
 */

#include <Arduino.h>
#include <Wire.h>
#include <Adafruit_GFX.h>
#include <Adafruit_SSD1306.h>
#include <Adafruit_SH110X.h>

// I2C pins
#define SDA_PIN 21
#define SCL_PIN 22

// Display settings
#define SCREEN_WIDTH 128
#define SCREEN_HEIGHT 64
#define OLED_RESET -1
#define OLED_ADDR 0x3C

// Display objects
Adafruit_SSD1306 *display1306 = nullptr;
Adafruit_SH1106G *display1106 = nullptr;

bool testSSD1306() {
  Serial.println("\n=== Testing SSD1306 Driver ===");

  display1306 = new Adafruit_SSD1306(SCREEN_WIDTH, SCREEN_HEIGHT, &Wire, OLED_RESET);

  Serial.print("Attempting begin() at 0x");
  Serial.print(OLED_ADDR, HEX);
  Serial.println("...");

  if (!display1306->begin(SSD1306_SWITCHCAPVCC, OLED_ADDR)) {
    Serial.println("❌ SSD1306 begin() failed");
    delete display1306;
    display1306 = nullptr;
    return false;
  }

  Serial.println("✓ SSD1306 begin() succeeded!");
  Serial.println("Attempting to draw test pattern...");

  // Clear display
  display1306->clearDisplay();

  // Test 1: Simple text
  display1306->setTextSize(2);
  display1306->setTextColor(SSD1306_WHITE);
  display1306->setCursor(0, 0);
  display1306->println("SSD1306");
  display1306->println("TEST");
  display1306->setTextSize(1);
  display1306->println();
  display1306->println("If you can");
  display1306->println("read this,");
  display1306->println("it works!");

  display1306->display();
  Serial.println("✓ Display updated");
  Serial.println("Check your OLED screen!");

  delay(5000);

  // Test 2: Patterns
  display1306->clearDisplay();
  display1306->drawRect(0, 0, SCREEN_WIDTH, SCREEN_HEIGHT, SSD1306_WHITE);
  display1306->drawLine(0, 0, SCREEN_WIDTH-1, SCREEN_HEIGHT-1, SSD1306_WHITE);
  display1306->drawLine(SCREEN_WIDTH-1, 0, 0, SCREEN_HEIGHT-1, SSD1306_WHITE);
  display1306->display();

  Serial.println("✓ Pattern test complete");
  delay(3000);

  return true;
}

bool testSH1106() {
  Serial.println("\n=== Testing SH1106 Driver ===");

  display1106 = new Adafruit_SH1106G(SCREEN_WIDTH, SCREEN_HEIGHT, &Wire, OLED_RESET);

  Serial.print("Attempting begin() at 0x");
  Serial.print(OLED_ADDR, HEX);
  Serial.println("...");

  if (!display1106->begin(OLED_ADDR, true)) {  // true = reset
    Serial.println("❌ SH1106 begin() failed");
    delete display1106;
    display1106 = nullptr;
    return false;
  }

  Serial.println("✓ SH1106 begin() succeeded!");
  Serial.println("Attempting to draw test pattern...");

  // Clear display
  display1106->clearDisplay();

  // Test 1: Simple text
  display1106->setTextSize(2);
  display1106->setTextColor(SH110X_WHITE);
  display1106->setCursor(0, 0);
  display1106->println("SH1106");
  display1106->println("TEST");
  display1106->setTextSize(1);
  display1106->println();
  display1106->println("If you can");
  display1106->println("read this,");
  display1106->println("it works!");

  display1106->display();
  Serial.println("✓ Display updated");
  Serial.println("Check your OLED screen!");

  delay(5000);

  // Test 2: Patterns
  display1106->clearDisplay();
  display1106->drawRect(0, 0, SCREEN_WIDTH, SCREEN_HEIGHT, SH110X_WHITE);
  display1106->drawLine(0, 0, SCREEN_WIDTH-1, SCREEN_HEIGHT-1, SH110X_WHITE);
  display1106->drawLine(SCREEN_WIDTH-1, 0, 0, SCREEN_HEIGHT-1, SH110X_WHITE);
  display1106->display();

  Serial.println("✓ Pattern test complete");
  delay(3000);

  return true;
}

void animateDisplay() {
  Serial.println("\n=== Running Animation Test ===");

  if (display1306) {
    for (int i = 0; i < 64; i += 4) {
      display1306->clearDisplay();
      display1306->fillCircle(64, 32, i/2, SSD1306_WHITE);
      display1306->display();
      delay(50);
    }
  } else if (display1106) {
    for (int i = 0; i < 64; i += 4) {
      display1106->clearDisplay();
      display1106->fillCircle(64, 32, i/2, SH110X_WHITE);
      display1106->display();
      delay(50);
    }
  }

  Serial.println("✓ Animation complete");
}

void setup() {
  Serial.begin(115200);
  delay(2000);

  Serial.println("\n\n");
  Serial.println("========================================");
  Serial.println("       OLED Display Test Program");
  Serial.println("========================================");
  Serial.print("SDA Pin: GPIO");
  Serial.println(SDA_PIN);
  Serial.print("SCL Pin: GPIO");
  Serial.println(SCL_PIN);
  Serial.print("I2C Address: 0x");
  Serial.println(OLED_ADDR, HEX);
  Serial.print("Resolution: ");
  Serial.print(SCREEN_WIDTH);
  Serial.print("x");
  Serial.println(SCREEN_HEIGHT);
  Serial.println();

  // Enable internal pull-ups
  pinMode(SDA_PIN, INPUT_PULLUP);
  pinMode(SCL_PIN, INPUT_PULLUP);
  delay(10);

  // Initialize I2C
  Wire.begin(SDA_PIN, SCL_PIN);
  Wire.setClock(100000);  // 100kHz
  delay(100);

  Serial.println("I2C initialized at 100kHz");
  Serial.println();

  // Scan for device first
  Serial.println("Scanning for device at 0x3C...");
  Wire.beginTransmission(0x3C);
  byte error = Wire.endTransmission();

  if (error == 0) {
    Serial.println("✓ Device found at 0x3C");
  } else {
    Serial.println("❌ No device found at 0x3C!");
    Serial.println("Check your wiring!");
    while(1) delay(1000);
  }
  Serial.println();

  // Try SH1106 first (most common for 1.3" displays)
  bool sh1106_works = testSH1106();

  if (sh1106_works) {
    Serial.println("\n========================================");
    Serial.println("SUCCESS! SH1106 driver works!");
    Serial.println("Your display uses the SH1106 driver");
    Serial.println("========================================\n");

    animateDisplay();

    // Show success screen
    display1106->clearDisplay();
    display1106->setTextSize(2);
    display1106->setCursor(0, 0);
    display1106->println("SUCCESS!");
    display1106->setTextSize(1);
    display1106->println();
    display1106->println("SH1106 works!");
    display1106->println("Use this driver");
    display1106->display();

    Serial.println("\n✓ Test complete! SH1106 driver confirmed working.");
    return;
  }

  // If SH1106 didn't work, try SSD1306
  Serial.println("\nSH1106 didn't work, trying SSD1306...");
  delay(1000);

  bool ssd1306_works = testSSD1306();

  if (ssd1306_works) {
    Serial.println("\n========================================");
    Serial.println("SUCCESS! SSD1306 driver works!");
    Serial.println("Your display uses the SSD1306 driver");
    Serial.println("========================================\n");

    animateDisplay();

    // Show success screen
    display1306->clearDisplay();
    display1306->setTextSize(2);
    display1306->setCursor(0, 0);
    display1306->println("SUCCESS!");
    display1306->setTextSize(1);
    display1306->println();
    display1306->println("SSD1306 works");
    display1306->println("Use this driver");
    display1306->display();

    Serial.println("\n✓ Test complete! SSD1306 driver confirmed working.");
    return;
  }

  // Neither worked
  Serial.println("\n========================================");
  Serial.println("ERROR: Neither driver worked!");
  Serial.println("========================================");
  Serial.println("The display is detected on I2C but");
  Serial.println("cannot be initialized.");
  Serial.println();
  Serial.println("Possible issues:");
  Serial.println("1. Display needs 5V power (try VCC to 5V pin)");
  Serial.println("2. Display is defective");
  Serial.println("3. Display uses different driver");
  Serial.println("4. Display needs hardware reset");
  Serial.println("========================================\n");
}

void loop() {
  // Alternate between two screens every 3 seconds
  static unsigned long lastUpdate = 0;
  static bool showScreen1 = true;

  if (millis() - lastUpdate > 3000) {
    lastUpdate = millis();
    showScreen1 = !showScreen1;

    if (display1306) {
      display1306->clearDisplay();
      display1306->setTextSize(1);
      display1306->setCursor(0, 0);

      if (showScreen1) {
        display1306->setTextSize(2);
        display1306->println("SSD1306");
        display1306->setTextSize(1);
        display1306->println();
        display1306->println("Working!");
        display1306->println();
        display1306->print("Uptime: ");
        display1306->print(millis() / 1000);
        display1306->println("s");
      } else {
        display1306->println("Screen 2");
        display1306->println();
        for (int i = 0; i < 5; i++) {
          display1306->print("Line ");
          display1306->println(i + 1);
        }
      }

      display1306->display();
    } else if (display1106) {
      display1106->clearDisplay();
      display1106->setTextSize(1);
      display1106->setCursor(0, 0);

      if (showScreen1) {
        display1106->setTextSize(2);
        display1106->println("SH1106");
        display1106->setTextSize(1);
        display1106->println();
        display1106->println("Working!");
        display1106->println();
        display1106->print("Uptime: ");
        display1106->print(millis() / 1000);
        display1106->println("s");
      } else {
        display1106->println("Screen 2");
        display1106->println();
        for (int i = 0; i < 5; i++) {
          display1106->print("Line ");
          display1106->println(i + 1);
        }
      }

      display1106->display();
    }
  }
}
