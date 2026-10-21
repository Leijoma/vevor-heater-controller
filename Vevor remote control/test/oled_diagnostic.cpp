#include <Arduino.h>
#include <Wire.h>
#include <Adafruit_SH110X.h>
#include <Adafruit_SSD1306.h>

// ========== PINS ==========
#define I2C_SDA_PIN 21
#define I2C_SCL_PIN 22
#define LED_PIN 18
#define OLED_WIDTH  128
#define OLED_HEIGHT 64
#define OLED_RESET  -1

// Test both possible addresses
const uint8_t test_addresses[] = {0x3C, 0x3D};
const char* driver_names[] = {"SH1106", "SSD1306"};

// ========== SETUP ==========
void setup() {
  Serial.begin(115200);
  delay(1000);

  pinMode(LED_PIN, OUTPUT);
  digitalWrite(LED_PIN, LOW);

  Serial.println("\n========================================");
  Serial.println("  OLED DIAGNOSTIC TEST");
  Serial.println("========================================\n");

  // Blink LED to show we're alive
  for (int i = 0; i < 5; i++) {
    digitalWrite(LED_PIN, HIGH);
    delay(100);
    digitalWrite(LED_PIN, LOW);
    delay(100);
  }

  Serial.println("LED should have blinked 5 times on GPIO18");
  Serial.println("If LED blinked: ESP32 is running OK");
  Serial.println("If LED didn't blink: Check LED wiring\n");

  // Initialize I2C
  Wire.begin(I2C_SDA_PIN, I2C_SCL_PIN);
  Wire.setClock(100000);
  delay(100);

  Serial.println("Scanning I2C bus...");
  Serial.println("-------------------");

  bool found = false;
  for (uint8_t addr = 1; addr < 127; addr++) {
    Wire.beginTransmission(addr);
    uint8_t error = Wire.endTransmission();

    if (error == 0) {
      Serial.print("I2C device found at 0x");
      if (addr < 16) Serial.print("0");
      Serial.println(addr, HEX);
      found = true;

      // Blink LED for each device found
      digitalWrite(LED_PIN, HIGH);
      delay(300);
      digitalWrite(LED_PIN, LOW);
      delay(200);
    }
  }

  if (!found) {
    Serial.println("ERROR: No I2C devices found!");
    Serial.println("Check wiring:");
    Serial.println("  OLED SDA -> GPIO21");
    Serial.println("  OLED SCL -> GPIO22");
    Serial.println("  OLED VCC -> 3.3V");
    Serial.println("  OLED GND -> GND");
    // Fast blink to indicate error
    while (1) {
      digitalWrite(LED_PIN, !digitalRead(LED_PIN));
      delay(100);
    }
  }

  Serial.println("-------------------\n");

  // Try SH1106 driver first (most common for 1.3" displays)
  Serial.println("Testing SH1106 driver...");
  for (uint8_t addr : test_addresses) {
    Serial.print("  Trying address 0x");
    Serial.print(addr, HEX);
    Serial.print("... ");

    Adafruit_SH1106G display(OLED_WIDTH, OLED_HEIGHT, &Wire, OLED_RESET);

    if (display.begin(addr, true)) {
      Serial.println("SUCCESS!");
      Serial.println("\n*** DISPLAY WORKING! ***");
      Serial.print("Driver: SH1106, Address: 0x");
      Serial.println(addr, HEX);

      // Update code to use this configuration
      Serial.println("\nUpdate your ui_test.cpp:");
      Serial.print("#define OLED_ADDR 0x");
      Serial.println(addr, HEX);
      Serial.println("Use: Adafruit_SH1106G driver");

      // Show test pattern
      display.clearDisplay();
      display.setTextColor(SH110X_WHITE);
      display.setTextSize(2);
      display.setCursor(10, 10);
      display.println("DISPLAY");
      display.println("  WORKS!");
      display.setTextSize(1);
      display.setCursor(0, 50);
      display.print("Addr:0x");
      display.print(addr, HEX);
      display.print(" SH1106");
      display.display();

      // Blink LED continuously to show success
      while (1) {
        digitalWrite(LED_PIN, HIGH);
        delay(500);
        digitalWrite(LED_PIN, LOW);
        delay(500);
      }
    } else {
      Serial.println("Failed");
    }
  }

  // Try SSD1306 driver if SH1106 failed
  Serial.println("\nTesting SSD1306 driver...");
  for (uint8_t addr : test_addresses) {
    Serial.print("  Trying address 0x");
    Serial.print(addr, HEX);
    Serial.print("... ");

    Adafruit_SSD1306 display(OLED_WIDTH, OLED_HEIGHT, &Wire, OLED_RESET);

    if (display.begin(SSD1306_SWITCHCAPVCC, addr)) {
      Serial.println("SUCCESS!");
      Serial.println("\n*** DISPLAY WORKING! ***");
      Serial.print("Driver: SSD1306, Address: 0x");
      Serial.println(addr, HEX);

      // Update code to use this configuration
      Serial.println("\nUpdate your ui_test.cpp:");
      Serial.print("#define OLED_ADDR 0x");
      Serial.println(addr, HEX);
      Serial.println("Replace: Adafruit_SH1106G with Adafruit_SSD1306");
      Serial.println("Update begin() call to: display.begin(SSD1306_SWITCHCAPVCC, OLED_ADDR)");

      // Show test pattern
      display.clearDisplay();
      display.setTextColor(SSD1306_WHITE);
      display.setTextSize(2);
      display.setCursor(10, 10);
      display.println("DISPLAY");
      display.println("  WORKS!");
      display.setTextSize(1);
      display.setCursor(0, 50);
      display.print("Addr:0x");
      display.print(addr, HEX);
      display.print(" SSD1306");
      display.display();

      // Blink LED continuously to show success
      while (1) {
        digitalWrite(LED_PIN, HIGH);
        delay(500);
        digitalWrite(LED_PIN, LOW);
        delay(500);
      }
    } else {
      Serial.println("Failed");
    }
  }

  Serial.println("\n========================================");
  Serial.println("ERROR: Could not initialize display!");
  Serial.println("Possible issues:");
  Serial.println("1. Wrong display type (not SH1106 or SSD1306)");
  Serial.println("2. Faulty display");
  Serial.println("3. Missing pull-up resistors on I2C");
  Serial.println("========================================");

  // Very fast blink to show failure
  while (1) {
    digitalWrite(LED_PIN, !digitalRead(LED_PIN));
    delay(50);
  }
}

void loop() {
  // Everything happens in setup
}
