/*
 * RS485 TX Test - Verify RS485 board receives data
 *
 * Purpose: Send test data continuously to verify RS485 RX LED blinks
 *
 * Hardware Setup:
 * ESP32 → RS485 TTL Module
 * - GPIO17 (TX2) → DI (Data In / TX)
 * - GPIO16 (RX2) → RO (Receive Out / RX)
 * - GND → GND
 * - 3.3V or 5V → VCC
 *
 * Expected behavior:
 * - RX LED on RS485 board should blink when ESP32 sends data
 * - Serial monitor shows what's being sent
 */

#include <Arduino.h>

// Serial2 pins for RS485 communication
#define RS485_TX_PIN 17  // GPIO17 - connects to RS485 DI (Data In)
#define RS485_RX_PIN 16  // GPIO16 - connects to RS485 RO (Receive Out)

void setup() {
  // USB serial for debug output
  Serial.begin(115200);
  delay(1000);

  // Initialize Serial2 for RS485 communication
  // Baud rate: 4800 (typical for Vevor heater)
  Serial2.begin(4800, SERIAL_8N1, RS485_RX_PIN, RS485_TX_PIN);

  Serial.println("\n╔══════════════════════════════════════════════════════╗");
  Serial.println("║         RS485 TX Test - LED Verification            ║");
  Serial.println("╚══════════════════════════════════════════════════════╝\n");

  Serial.println("Configuration:");
  Serial.println("  - Baud rate: 4800");
  Serial.println("  - TX Pin: GPIO17 → RS485 DI");
  Serial.println("  - RX Pin: GPIO16 → RS485 RO");
  Serial.println();

  Serial.println("Expected: RS485 board RX LED should blink");
  Serial.println("Sending test data every second...\n");

  delay(2000);
}

void loop() {
  static uint32_t counter = 0;

  // Send test data
  Serial2.print("TEST_DATA_");
  Serial2.println(counter);

  // Also send some binary data to make LED more visible
  for (int i = 0; i < 10; i++) {
    Serial2.write(0x55);  // Alternating bits pattern
  }

  // Debug output to USB serial
  Serial.printf("Sent: TEST_DATA_%lu + binary pattern (RX LED should blink now)\n", counter);

  counter++;
  delay(1000);  // Send every second
}
