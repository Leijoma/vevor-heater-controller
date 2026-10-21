/*
 * Vevor Heater RS485 Protocol Sniffer
 *
 * Hardware Setup:
 * - RS485 to TTL converter module
 * - Module RX → ESP32 GPIO16 (RX2)
 * - Module TX → NOT CONNECTED (read-only mode)
 * - Module GND → ESP32 GND
 * - Tap into blue wire from heater (parallel connection)
 *
 * Protocol: 4800 baud, 8N1
 *
 * Packet Types:
 * - 0xAA 0x66 = Controller → Heater (15 bytes total)
 * - 0xAA 0x77 = Heater → Controller (55 bytes total)
 */

#include <Arduino.h>

// UART configuration
#define RX_PIN 16          // GPIO16 = RX2
#define BAUD_RATE 4800     // Vevor protocol baud rate

// Buffer for packet capture
#define MAX_PACKET_SIZE 100
uint8_t packetBuffer[MAX_PACKET_SIZE];
int packetIndex = 0;
bool capturingPacket = false;
unsigned long lastByteTime = 0;
unsigned long packetCount = 0;

// Packet timeout (if no data for 50ms, packet is complete)
#define PACKET_TIMEOUT_MS 50

// State names
const char* getStateName(uint8_t state) {
  switch(state) {
    case 0x00: return "OFF";
    case 0x01: return "GLOW_PREHEAT";
    case 0x02: return "IGNITED";
    case 0x03: return "STABLE";
    case 0x04: return "COOLING";
    default: return "UNKNOWN";
  }
}

// Command names
const char* getCommandName(uint8_t cmd) {
  switch(cmd) {
    case 0x02: return "GET_STATE";
    case 0x06: return "START_UP";
    default: return "UNKNOWN";
  }
}

// Requested state names
const char* getRequestedStateName(uint8_t state) {
  switch(state) {
    case 0x02: return "OFF";
    case 0x05: return "COOLING";
    case 0x06: return "START";
    case 0x08: return "RUNNING";
    default: return "UNKNOWN";
  }
}

void setup() {
  Serial.begin(115200);
  delay(1000);

  // Initialize Serial2 for RS485 (RX only)
  Serial2.begin(BAUD_RATE, SERIAL_8N1, RX_PIN, -1);  // TX pin = -1 (not used)

  Serial.println("\n╔═══════════════════════════════════════════════════════════╗");
  Serial.println("║         Vevor Heater RS485 Protocol Sniffer             ║");
  Serial.println("╚═══════════════════════════════════════════════════════════╝\n");

  Serial.println("Configuration:");
  Serial.printf("  RX Pin:    GPIO%d (Serial2)\n", RX_PIN);
  Serial.printf("  Baud Rate: %d\n", BAUD_RATE);
  Serial.printf("  Mode:      8N1 (8 data bits, no parity, 1 stop bit)\n");
  Serial.println();

  Serial.println("Wiring:");
  Serial.println("  RS485 Module RX  → ESP32 GPIO16");
  Serial.println("  RS485 Module GND → ESP32 GND");
  Serial.println("  RS485 Module A/B → Heater blue wire (parallel tap)");
  Serial.println();

  Serial.println("Packet Format:");
  Serial.println("  Controller → Heater: 0xAA 0x66 ... (15 bytes)");
  Serial.println("  Heater → Controller: 0xAA 0x77 ... (55 bytes)");
  Serial.println();
  Serial.println("Listening for packets...\n");
  Serial.println("────────────────────────────────────────────────────────────\n");
}

void printHexDump(uint8_t* data, int length) {
  for (int i = 0; i < length; i++) {
    if (i > 0 && i % 16 == 0) {
      Serial.println();
      Serial.print("       ");
    }
    Serial.printf("%02X ", data[i]);
  }
  Serial.println();
}

void decodeControllerPacket(uint8_t* data, int length) {
  if (length < 15) {
    Serial.println("  ⚠️  Packet too short for controller format");
    return;
  }

  Serial.println("\n  📤 CONTROLLER → HEATER");
  Serial.printf("     Device ID:       0x%02X\n", data[1]);
  Serial.printf("     Command:         0x%02X (%s)\n", data[2], getCommandName(data[2]));
  Serial.printf("     Length:          0x%02X (%d bytes)\n", data[3], data[3]);
  Serial.printf("     Power Level:     %d/10\n", data[8]);
  Serial.printf("     Requested State: 0x%02X (%s)\n", data[9], getRequestedStateName(data[9]));

  // Calculate checksum
  uint8_t calculatedChecksum = (data[2] + data[3] + data[8] + data[9]) & 0xFF;
  uint8_t packetChecksum = data[14];
  Serial.printf("     Checksum:        0x%02X (calculated: 0x%02X) %s\n",
                packetChecksum, calculatedChecksum,
                (packetChecksum == calculatedChecksum) ? "✓" : "✗ MISMATCH");
}

void decodeHeaterPacket(uint8_t* data, int length) {
  if (length < 55) {
    Serial.println("  ⚠️  Packet too short for heater format");
    return;
  }

  Serial.println("\n  📥 HEATER → CONTROLLER");
  Serial.printf("     Device ID:        0x%02X\n", data[1]);
  Serial.printf("     Command:          0x%02X\n", data[2]);
  Serial.printf("     Length:           0x%02X (%d bytes)\n", data[3], data[3]);
  Serial.printf("     Operating State:  0x%02X (%s)\n", data[5], getStateName(data[5]));
  Serial.printf("     Power Level:      %d/10\n", data[6]);
  Serial.printf("     Error Code:       0x%02X\n", data[7]);

  // Heat exchanger temperature (bytes 16-17, big endian, °C × 100)
  int16_t tempRaw = (data[16] << 8) | data[17];
  float temperature = tempRaw / 100.0;
  Serial.printf("     Heat Exchanger:   %.1f °C\n", temperature);

  Serial.printf("     Checksum:         0x%02X (at byte 54)\n", data[54]);
}

void processPacket() {
  if (packetIndex < 2) return;  // Need at least header

  packetCount++;

  Serial.println("\n┌─────────────────────────────────────────────────────────┐");
  Serial.printf("│ Packet #%lu (%d bytes)                                   \n",
                packetCount, packetIndex);
  Serial.println("└─────────────────────────────────────────────────────────┘");

  // Print hex dump
  Serial.print("  HEX:  ");
  printHexDump(packetBuffer, packetIndex);

  // Decode based on packet type
  if (packetBuffer[0] == 0xAA) {
    if (packetBuffer[1] == 0x66) {
      // Controller → Heater
      decodeControllerPacket(packetBuffer, packetIndex);
    } else if (packetBuffer[1] == 0x77) {
      // Heater → Controller
      decodeHeaterPacket(packetBuffer, packetIndex);
    } else {
      Serial.printf("\n  ⚠️  Unknown device ID: 0x%02X\n", packetBuffer[1]);
    }
  } else {
    Serial.println("\n  ⚠️  Invalid header (expected 0xAA)");
  }

  Serial.println("\n────────────────────────────────────────────────────────────\n");
}

void loop() {
  // Check for packet timeout
  if (capturingPacket && (millis() - lastByteTime > PACKET_TIMEOUT_MS)) {
    processPacket();
    capturingPacket = false;
    packetIndex = 0;
  }

  // Read available bytes
  while (Serial2.available()) {
    uint8_t incomingByte = Serial2.read();
    lastByteTime = millis();

    // Start of new packet (0xAA header)
    if (incomingByte == 0xAA && !capturingPacket) {
      capturingPacket = true;
      packetIndex = 0;
    }

    // Store byte in buffer
    if (capturingPacket) {
      if (packetIndex < MAX_PACKET_SIZE) {
        packetBuffer[packetIndex++] = incomingByte;
      } else {
        // Buffer overflow - reset
        Serial.println("⚠️  Buffer overflow! Packet too large.");
        capturingPacket = false;
        packetIndex = 0;
      }
    }
  }

  // Small delay to prevent busy-waiting
  delay(1);
}
