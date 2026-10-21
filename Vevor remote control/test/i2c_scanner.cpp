/*
 * I2C Scanner Test Program
 *
 * Simple program to scan I2C bus and find connected devices
 * This will help diagnose I2C issues
 */

#include <Arduino.h>
#include <Wire.h>

// I2C pins
#define SDA_PIN 21
#define SCL_PIN 22

// Test different I2C speeds
uint32_t i2c_speeds[] = {100000, 400000, 50000};  // 100kHz, 400kHz, 50kHz
const char* speed_names[] = {"100kHz", "400kHz", "50kHz"};

void scanI2C(uint32_t speed) {
  Serial.println("\n========================================");
  Serial.print("Scanning I2C at ");
  Serial.print(speed / 1000);
  Serial.println(" kHz...");
  Serial.println("========================================");

  byte count = 0;
  byte error, address;

  for (address = 1; address < 127; address++) {
    Wire.beginTransmission(address);
    error = Wire.endTransmission();

    if (error == 0) {
      Serial.print("Device found at address 0x");
      if (address < 16) Serial.print("0");
      Serial.print(address, HEX);
      Serial.print(" (");
      Serial.print(address);
      Serial.print(") - ");

      // Identify common devices
      switch(address) {
        case 0x3C: Serial.println("Likely: SSD1306 OLED or SH1106 OLED"); break;
        case 0x3D: Serial.println("Likely: SSD1306 OLED or SH1106 OLED"); break;
        case 0x76: Serial.println("Likely: BMP280 or BME280"); break;
        case 0x77: Serial.println("Likely: BMP280 or BME280"); break;
        case 0x68: Serial.println("Likely: DS3231 RTC or MPU6050"); break;
        case 0x27: Serial.println("Likely: LCD with I2C backpack"); break;
        case 0x50: Serial.println("Likely: EEPROM"); break;
        default: Serial.println("Unknown device");
      }
      count++;
    }
    else if (error == 4) {
      Serial.print("Unknown error at address 0x");
      if (address < 16) Serial.print("0");
      Serial.println(address, HEX);
    }
  }

  if (count == 0) {
    Serial.println("No I2C devices found!\n");
  } else {
    Serial.print("\nFound ");
    Serial.print(count);
    Serial.println(" device(s)\n");
  }

  delay(1000);
}

void setup() {
  Serial.begin(115200);
  delay(2000);

  Serial.println("\n\n");
  Serial.println("========================================");
  Serial.println("       I2C Scanner Test Program");
  Serial.println("========================================");
  Serial.print("SDA Pin: GPIO");
  Serial.println(SDA_PIN);
  Serial.print("SCL Pin: GPIO");
  Serial.println(SCL_PIN);
  Serial.println();

  // Enable internal pull-ups
  pinMode(SDA_PIN, INPUT_PULLUP);
  pinMode(SCL_PIN, INPUT_PULLUP);
  delay(100);

  Serial.println("Internal pull-ups enabled on SDA and SCL");
  Serial.println();

  // Try different I2C speeds
  for (int i = 0; i < 3; i++) {
    Wire.begin(SDA_PIN, SCL_PIN);
    Wire.setClock(i2c_speeds[i]);
    delay(100);

    scanI2C(i2c_speeds[i]);

    // Wire.end() not available in old Arduino core
    delay(500);
  }

  Serial.println("========================================");
  Serial.println("Scan complete. Will scan again every 5 seconds.");
  Serial.println("Reset ESP32 to change settings.");
  Serial.println("========================================\n");

  // Reinitialize at 100kHz for continuous scanning
  Wire.begin(SDA_PIN, SCL_PIN);
  Wire.setClock(100000);
}

void loop() {
  delay(5000);

  Serial.println("\n--- Quick Rescan (100kHz) ---");

  byte count = 0;
  for (byte address = 1; address < 127; address++) {
    Wire.beginTransmission(address);
    if (Wire.endTransmission() == 0) {
      Serial.print("0x");
      if (address < 16) Serial.print("0");
      Serial.print(address, HEX);
      Serial.print(" ");
      count++;
    }
  }

  if (count == 0) {
    Serial.println("No devices found");
  } else {
    Serial.print("\nFound ");
    Serial.print(count);
    Serial.println(" device(s)");
  }
}
