#include <Arduino.h>
#include <Wire.h>

// Pin combinations to test
struct PinCombo {
  int sda;
  int scl;
  const char* name;
};

PinCombo pinCombos[] = {
  {21, 22, "SDA=21, SCL=22 (CORRECT)"},
  {22, 21, "SDA=22, SCL=21 (SWAPPED)"},
  {21, 23, "SDA=21, SCL=23"},
  {23, 22, "SDA=23, SCL=22"}
};

void testPinCombo(int sda, int scl, const char* name) {
  Serial.println("\n========================================");
  Serial.print("Testing: ");
  Serial.println(name);
  Serial.print("  SDA on GPIO");
  Serial.println(sda);
  Serial.print("  SCL on GPIO");
  Serial.println(scl);
  Serial.println("========================================");

  // Initialize I2C with these pins
  delay(100);
  Wire.begin(sda, scl);
  Wire.setClock(100000);  // 100kHz
  delay(200);

  // Scan for devices
  Serial.println("Scanning I2C bus...");
  bool found = false;

  for (uint8_t addr = 1; addr < 127; addr++) {
    Wire.beginTransmission(addr);
    uint8_t error = Wire.endTransmission();

    if (error == 0) {
      Serial.print("  ✓ FOUND device at 0x");
      if (addr < 16) Serial.print("0");
      Serial.print(addr, HEX);

      if (addr == 0x3C || addr == 0x3D) {
        Serial.println(" ← OLED DISPLAY!");
      } else {
        Serial.println();
      }
      found = true;
    }
  }

  if (!found) {
    Serial.println("  ✗ No devices found");
  }

  delay(1000);
}

void setup() {
  Serial.begin(115200);
  delay(2000);

  Serial.println("\n\n");
  Serial.println("╔════════════════════════════════════════╗");
  Serial.println("║   I2C PIN IDENTIFICATION TEST          ║");
  Serial.println("╔════════════════════════════════════════╗");
  Serial.println();
  Serial.println("This test will try different pin combinations");
  Serial.println("to find your OLED display.");
  Serial.println();
  Serial.println("Current wiring:");
  Serial.println("  OLED pin labeled 'SDA' → ESP32 GPIO ??");
  Serial.println("  OLED pin labeled 'SCL' → ESP32 GPIO ??");
  Serial.println();
  Serial.println("Testing now...");

  // Test all combinations
  for (int i = 0; i < 4; i++) {
    testPinCombo(pinCombos[i].sda, pinCombos[i].scl, pinCombos[i].name);
  }

  Serial.println("\n\n");
  Serial.println("╔════════════════════════════════════════╗");
  Serial.println("║   TEST COMPLETE                        ║");
  Serial.println("╔════════════════════════════════════════╗");
  Serial.println();
  Serial.println("RESULTS:");
  Serial.println("--------");
  Serial.println("Look above for any line with '✓ FOUND'");
  Serial.println();
  Serial.println("If you see 'OLED DISPLAY!' then:");
  Serial.println("  → Note which pin combo worked");
  Serial.println("  → That's your correct wiring!");
  Serial.println();
  Serial.println("If NO devices found at all:");
  Serial.println("  → Check power (try 5V instead of 3.3V)");
  Serial.println("  → Check all wire connections");
  Serial.println("  → Make sure OLED is powered on");
  Serial.println();
}

void loop() {
  // Nothing to do - all testing done in setup()
  delay(1000);
}
