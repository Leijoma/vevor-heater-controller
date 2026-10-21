/**
 * Temperature Sensor Scanner
 * Scans GPIO 18 and 19 for DS18B20 sensors
 */

#include <Arduino.h>
#include <OneWire.h>
#include <DallasTemperature.h>

void scanPin(int pin) {
  Serial.printf("\n=== Scanning GPIO %d ===\n", pin);

  pinMode(pin, INPUT_PULLUP);
  delay(100);

  OneWire oneWire(pin);
  DallasTemperature sensors(&oneWire);

  sensors.begin();
  int count = sensors.getDeviceCount();

  Serial.printf("Sensors found: %d\n", count);

  if (count > 0) {
    sensors.requestTemperatures();
    float temp = sensors.getTempCByIndex(0);
    Serial.printf("Temperature: %.2f°C\n", temp);

    // Get device address
    DeviceAddress addr;
    if (sensors.getAddress(addr, 0)) {
      Serial.print("Address: ");
      for (int i = 0; i < 8; i++) {
        Serial.printf("%02X", addr[i]);
        if (i < 7) Serial.print(":");
      }
      Serial.println();
    }
  }
}

void setup() {
  Serial.begin(115200);
  delay(2000);

  Serial.println("\n========================================");
  Serial.println("  DS18B20 Temperature Sensor Scanner");
  Serial.println("========================================");

  scanPin(18);
  scanPin(19);

  Serial.println("\n========================================");
  Serial.println("Scan complete!");
  Serial.println("========================================\n");
}

void loop() {
  delay(1000);
}
