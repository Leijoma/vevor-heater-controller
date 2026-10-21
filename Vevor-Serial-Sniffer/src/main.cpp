/*
 * Vevor Heater Serial Sniffer
 *
 * Purpose: Passive monitoring of single-wire serial communication
 *          between control panel and heater
 *
 * Hardware Setup:
 * Control Panel Cable → ESP32
 * - Black (GND)  → GND
 * - Red (+5V)    → NOT CONNECTED (powers panel from heater)
 * - Blue (Data)  → GPIO16 (RX2)
 *
 * Protocol Info (from Hackaday project):
 * - Baud rate: 4800
 * - Single wire bidirectional
 * - Heater requests: 0xAA 66 02 ...
 * - Remote commands: 06 = ON, 02 = OFF
 */

#include <Arduino.h>

#define SERIAL_RX_PIN 16  // GPIO16 - connect to Blue wire
#define SERIAL_TX_PIN 17  // GPIO17 - not used for sniffing

// Buffer for incoming data
#define BUFFER_SIZE 256
uint8_t rxBuffer[BUFFER_SIZE];
int bufferIndex = 0;
unsigned long lastByteTime = 0;
#define PACKET_TIMEOUT 100  // ms - gap between packets

void setup() {
  // USB serial for output
  Serial.begin(115200);
  delay(1000);

  // Serial2 for heater communication (4800 baud, single wire)
  // NOTE: Protocol uses INVERTED UART signals!
  // If you see garbage data, try adding: Serial2.setRxInvert(true);
  Serial2.begin(4800, SERIAL_8N1, SERIAL_RX_PIN, SERIAL_TX_PIN);
  // Uncomment next line if getting garbage/no data:
  // Serial2.setRxInvert(true);  // Invert RX signal

  Serial.println("\n╔══════════════════════════════════════════════════════╗");
  Serial.println("║     Vevor Heater Serial Protocol Sniffer            ║");
  Serial.println("╚══════════════════════════════════════════════════════╝\n");

  Serial.println("Configuration:");
  Serial.println("  - Baud rate: 4800");
  Serial.println("  - RX Pin: GPIO16 (Blue wire)");
  Serial.println("  - Mode: Passive listening");
  Serial.println();

  Serial.println("Wiring:");
  Serial.println("  Black (GND) → ESP32 GND");
  Serial.println("  Red (+5V)   → Not connected (panel powered by heater)");
  Serial.println("  Blue (Data) → ESP32 GPIO16");
  Serial.println();

  Serial.println("Expected packets:");
  Serial.println("  - Heater request: AA 66 02 ...");
  Serial.println("  - Remote ON: ... 06 ...");
  Serial.println("  - Remote OFF: ... 02 ...");
  Serial.println();

  Serial.println("Waiting for data...\n");
}

void printPacket() {
  if (bufferIndex == 0) return;

  // Print timestamp
  Serial.printf("[%10lu ms] ", millis());

  // Print packet in hex
  Serial.print("Packet (");
  Serial.print(bufferIndex);
  Serial.print(" bytes): ");

  for (int i = 0; i < bufferIndex; i++) {
    if (rxBuffer[i] < 0x10) Serial.print("0");
    Serial.print(rxBuffer[i], HEX);
    Serial.print(" ");
  }
  Serial.println();

  // Decode packet if it matches expected format
  if (bufferIndex == 16 && rxBuffer[0] == 0xAA) {
    Serial.println("  ┌─ Decoded:");

    // Identify sender
    if (rxBuffer[1] == 0x66) {
      Serial.println("  │ From: CONTROLLER");
    } else if (rxBuffer[1] == 0x77) {
      Serial.println("  │ From: HEATER");
    } else {
      Serial.printf("  │ From: UNKNOWN (0x%02X)\n", rxBuffer[1]);
    }

    // Decode controller packet (16 bytes from controller)
    if (rxBuffer[1] == 0x66 && bufferIndex == 16) {
      uint8_t command = rxBuffer[2];
      uint8_t frameLen = rxBuffer[3];
      uint8_t powerLevel = rxBuffer[8];
      uint8_t state = rxBuffer[9];
      uint8_t checksum = rxBuffer[15];

      // Calculate expected checksum (sum bytes 2-14)
      uint8_t calcChecksum = 0;
      for (int i = 2; i <= 14; i++) {
        calcChecksum += rxBuffer[i];
      }
      calcChecksum &= 0xFF;  // Keep lower 8 bits

      Serial.printf("  │ Command: 0x%02X ", command);
      if (command == 0x02) Serial.println("(STATUS REQUEST)");
      else if (command == 0x06) Serial.println("(START/STOP)");
      else Serial.println("(UNKNOWN)");

      Serial.printf("  │ Frame length: 0x%02X (%d bytes)\n", frameLen, frameLen);
      Serial.printf("  │ Power level: %d/10\n", powerLevel);

      Serial.printf("  │ State: 0x%02X ", state);
      if (state == 0x02) Serial.println("(OFF)");
      else if (state == 0x05) Serial.println("(SET OFF)");
      else if (state == 0x06) Serial.println("(START)");
      else if (state == 0x08) Serial.println("(RUNNING)");
      else Serial.println("(UNKNOWN)");

      Serial.printf("  │ Checksum: 0x%02X (calculated: 0x%02X) ", checksum, calcChecksum);
      if (checksum == calcChecksum) {
        Serial.println("✓ VALID");
      } else {
        Serial.println("✗ INVALID");
      }
    }

    Serial.println("  └─");
  } else {
    // Unknown packet format - just show as hex
    Serial.printf("  → Unknown format (%d bytes)\n", bufferIndex);
  }

  // Reset buffer
  bufferIndex = 0;
}

void loop() {
  unsigned long currentTime = millis();

  // Check for timeout (end of packet)
  if (bufferIndex > 0 && (currentTime - lastByteTime) > PACKET_TIMEOUT) {
    printPacket();
  }

  // Read available bytes
  while (Serial2.available()) {
    uint8_t inByte = Serial2.read();
    lastByteTime = currentTime;

    // Add to buffer if space available
    if (bufferIndex < BUFFER_SIZE) {
      rxBuffer[bufferIndex++] = inByte;
    } else {
      // Buffer overflow - print what we have
      Serial.println("WARNING: Buffer overflow!");
      printPacket();
      rxBuffer[0] = inByte;
      bufferIndex = 1;
    }
  }
}
