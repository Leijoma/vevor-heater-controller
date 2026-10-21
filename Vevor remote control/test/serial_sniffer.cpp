#include <Arduino.h>

// ========== SERIAL CONFIGURATION ==========
#define SERIAL_RX_PIN 16       // GPIO16 - Receive from heater
#define SERIAL_TX_PIN 17       // GPIO17 - Transmit to heater (not used in sniffer mode)
#define SERIAL_BAUD 4800       // Vevor protocol baud rate

// ========== PACKET DETECTION ==========
#define PACKET_TIMEOUT_MS 50   // Time to wait for more bytes before packet end
#define MAX_PACKET_SIZE 256    // Maximum expected packet size

// ========== DISPLAY OPTIONS ==========
bool showTimestamps = true;
bool showHex = true;
bool showAscii = true;
bool detectPackets = true;

// ========== BUFFER ==========
uint8_t packetBuffer[MAX_PACKET_SIZE];
int packetIndex = 0;
unsigned long lastByteTime = 0;
unsigned long packetCount = 0;
uint8_t lastState = 0xFF;  // Track state changes

// ========== KNOWN PROTOCOL VALUES ==========
// From Hackaday research and documentation
#define PACKET_HEADER_1 0xAA
#define PACKET_HEADER_2 0x66

// ========== FUNCTION DECLARATIONS ==========
void decodeHeaterPacket();
void decodeHeaterPacketCompact();
void decodeControllerPacket();
void printPacketDetailsAndDecode();
const char* getStateName(uint8_t state);

// ========== SETUP ==========
void setup() {
  Serial.begin(115200);
  delay(2000);

  Serial.println("\n\n========================================");
  Serial.println("  Vevor Heater Serial Protocol Sniffer");
  Serial.println("========================================");
  Serial.println("\nConfiguration:");
  Serial.print("  Baud Rate: ");
  Serial.println(SERIAL_BAUD);
  Serial.print("  RX Pin: GPIO");
  Serial.println(SERIAL_RX_PIN);
  Serial.print("  TX Pin: GPIO");
  Serial.print(SERIAL_TX_PIN);
  Serial.println(" (not used in sniffer mode)");
  Serial.println("\nMode: PASSIVE SNIFFING");
  Serial.println("  - Only listening (RX)");
  Serial.println("  - Not transmitting (TX)");
  Serial.println("  - Will not interfere with controller");
  Serial.println();

  // Initialize serial port to heater (RX only, TX not used)
  Serial2.begin(SERIAL_BAUD, SERIAL_8N1, SERIAL_RX_PIN, SERIAL_TX_PIN);

  Serial.println("Waiting for serial data...");
  Serial.println("Commands:");
  Serial.println("  h - Toggle hex display");
  Serial.println("  a - Toggle ASCII display");
  Serial.println("  t - Toggle timestamps");
  Serial.println("  p - Toggle packet detection");
  Serial.println("  c - Clear screen");
  Serial.println("  s - Show statistics");
  Serial.println();
  Serial.println("========================================\n");
}

// ========== PRINT PACKET ==========
void printPacket() {
  if (packetIndex == 0) return;

  packetCount++;

  // Check if this is a heater status packet - decide compact vs detailed FIRST
  if ((packetIndex == 56 || packetIndex == 57) && packetBuffer[0] == 0xAA && packetBuffer[1] == 0x77) {
    // Check if state changed
    uint8_t currentState = packetBuffer[5];
    bool stateChanged = (currentState != lastState);
    lastState = currentState;

    // Show detailed every 10 packets or on state change
    bool showDetailed = (packetCount % 10 == 0) || stateChanged;

    if (showDetailed) {
      // Show full detailed packet info
      Serial.println("\n--- PACKET START ---");
      if (showTimestamps) {
        Serial.print("Time: ");
        Serial.print(millis());
        Serial.println(" ms");
      }
      Serial.print("Length: ");
      Serial.print(packetIndex);
      Serial.println(" bytes");

      printPacketDetailsAndDecode();
      decodeHeaterPacket();
      Serial.println("--- PACKET END ---\n");
    } else {
      // Compact mode - just one line
      decodeHeaterPacketCompact();
    }
  } else {
    // Controller commands and unknown packets - always show detailed
    Serial.println("\n--- PACKET START ---");
    if (showTimestamps) {
      Serial.print("Time: ");
      Serial.print(millis());
      Serial.println(" ms");
    }
    Serial.print("Length: ");
    Serial.print(packetIndex);
    Serial.println(" bytes");

    printPacketDetailsAndDecode();

    if (packetIndex == 16 && packetBuffer[0] == 0xAA && packetBuffer[1] == 0x66) {
      decodeControllerPacket();
    }
    Serial.println("--- PACKET END ---\n");
  }

  // Reset for next packet
  packetIndex = 0;
}

// ========== PRINT PACKET DETAILS ==========
void printPacketDetailsAndDecode() {
  // Check for known header
  if (packetIndex >= 2 && packetBuffer[0] == PACKET_HEADER_1 && packetBuffer[1] == PACKET_HEADER_2) {
    Serial.println("Header: 0xAA 0x66 ✓ (Known Vevor header)");
  }

  // Display in hex
  if (showHex) {
    Serial.print("\nHEX: ");
    for (int i = 0; i < packetIndex; i++) {
      if (packetBuffer[i] < 0x10) Serial.print("0");
      Serial.print(packetBuffer[i], HEX);
      Serial.print(" ");

      // Add line break every 16 bytes for readability
      if ((i + 1) % 16 == 0 && i < packetIndex - 1) {
        Serial.print("\n     ");
      }
    }
    Serial.println();
  }

  // Display in ASCII (if printable)
  if (showAscii) {
    Serial.print("ASCII: ");
    bool hasPrintable = false;
    for (int i = 0; i < packetIndex; i++) {
      if (packetBuffer[i] >= 32 && packetBuffer[i] <= 126) {
        Serial.print((char)packetBuffer[i]);
        hasPrintable = true;
      } else {
        Serial.print(".");
      }
    }
    if (!hasPrintable) {
      Serial.print(" (no printable characters)");
    }
    Serial.println();
  }

  // Calculate simple checksum (sum of all bytes)
  uint16_t checksum = 0;
  for (int i = 0; i < packetIndex; i++) {
    checksum += packetBuffer[i];
  }
  Serial.print("Checksum (sum): 0x");
  Serial.print(checksum, HEX);
  Serial.print(" (8-bit: 0x");
  Serial.print(checksum & 0xFF, HEX);
  Serial.println(")");

  // Try to identify packet type
  Serial.print("Type: ");
  if (packetIndex == 16) {
    Serial.println("16-byte packet (controller command)");
  } else if (packetIndex == 56 || packetIndex == 57) {
    Serial.println("56/57-byte packet (heater status)");
  } else if (packetIndex == 72) {
    Serial.println("72-byte packet (possibly extended status)");
  } else {
    Serial.println("Unknown");
  }
}

// ========== SHOW STATISTICS ==========
void showStatistics() {
  Serial.println("\n========== STATISTICS ==========");
  Serial.print("Total packets: ");
  Serial.println(packetCount);
  Serial.print("Uptime: ");
  Serial.print(millis() / 1000);
  Serial.println(" seconds");
  Serial.print("Packets/minute: ");
  if (millis() > 60000) {
    Serial.println(packetCount * 60000 / millis());
  } else {
    Serial.println("N/A (< 1 minute)");
  }
  Serial.println("================================\n");
}

// ========== GET STATE NAME ==========
const char* getStateName(uint8_t state) {
  switch (state) {
    case 0x00: return "OFF";
    case 0x01: return "GLOW_PREHEAT";
    case 0x02: return "IGNITION";
    case 0x03: return "COMBUSTION";
    case 0x04: return "COOLDOWN";
    default: return "UNKNOWN";
  }
}

// ========== DECODE HEATER PACKET (COMPACT) ==========
void decodeHeaterPacketCompact() {
  if (packetIndex != 56 && packetIndex != 57) return;
  if (packetBuffer[0] != 0xAA || packetBuffer[1] != 0x77) return;

  uint8_t state = packetBuffer[5];
  uint8_t power = packetBuffer[6];
  uint16_t temp_raw = (packetBuffer[16] << 8) | packetBuffer[17];
  float temp = temp_raw / 10.0;
  uint16_t fan_rpm = (packetBuffer[28] << 8) | packetBuffer[29];
  float voltage = packetBuffer[11] / 10.0;

  // One-line format: [#count] State | Pwr | Temp | Fan | Voltage
  Serial.print("[");
  Serial.print(packetCount);
  Serial.print("] ");
  Serial.print(getStateName(state));
  Serial.print(" | P:");
  Serial.print(power);
  Serial.print(" | T:");
  Serial.print(temp, 1);
  Serial.print("C | F:");
  Serial.print(fan_rpm);
  Serial.print(" | V:");
  Serial.print(voltage, 1);
  Serial.println("V");
}

// ========== DECODE HEATER STATUS PACKET (56 or 57 bytes) ==========
void decodeHeaterPacket() {
  if (packetIndex != 56 && packetIndex != 57) return;
  if (packetBuffer[0] != 0xAA || packetBuffer[1] != 0x77) return;

  Serial.println("\n=== HEATER STATUS DECODED ===");

  // Byte 5: State
  Serial.print("State: ");
  Serial.println(getStateName(packetBuffer[5]));

  // Byte 6: Power level (1-10)
  Serial.print("Power Level: ");
  Serial.print(packetBuffer[6]);
  Serial.println("/10");

  // Byte 7: Error code
  Serial.print("Error Code: ");
  if (packetBuffer[7] == 0x00) {
    Serial.println("None");
  } else {
    Serial.print("0x");
    Serial.print(packetBuffer[7], HEX);
    Serial.print(" (");
    switch (packetBuffer[7]) {
      case 0x01: Serial.print("E10 - Startup failure"); break;
      case 0x02: Serial.print("E08 - Low fuel"); break;
      case 0x03: Serial.print("E01 - Voltage overrun"); break;
      case 0x08: Serial.print("E03 - Ignition fault"); break;
      case 0x09: Serial.print("E05 - Overheat"); break;
      default: Serial.print("Unknown error"); break;
    }
    Serial.println(")");
  }

  // Bytes 16-17: Internal Temperature (big-endian, ÷ 10 for °C)
  uint16_t temp_raw = (packetBuffer[16] << 8) | packetBuffer[17];
  float temperature = temp_raw / 10.0;
  Serial.print("Internal Temperature: ");
  Serial.print(temperature, 1);
  Serial.println(" °C");

  // Byte 11: Voltage (V × 10)
  float voltage = packetBuffer[11] / 10.0;
  Serial.print("Voltage: ");
  Serial.print(voltage, 1);
  Serial.println(" V");

  // Byte 13: Glow plug current (direct amps)
  Serial.print("Glow Plug Current: ");
  Serial.print(packetBuffer[13]);
  Serial.println(" A");

  // Bytes 28-29: Fan speed (big-endian, direct RPM)
  uint16_t fan_rpm = (packetBuffer[28] << 8) | packetBuffer[29];
  Serial.print("Fan Speed: ");
  Serial.print(fan_rpm);
  Serial.println(" RPM");

  // Byte 23: Pump frequency (Hz × 10)
  float pump_freq = packetBuffer[23] / 10.0;
  Serial.print("Pump Frequency: ");
  Serial.print(pump_freq, 1);
  Serial.println(" Hz");

  // Checksum: last byte (should be sum of bytes from 2 to second-to-last)
  int checksumIndex = packetIndex - 1;
  uint8_t calculated_checksum = 0;
  for (int i = 2; i < checksumIndex; i++) {
    calculated_checksum += packetBuffer[i];
  }
  Serial.print("Checksum: 0x");
  Serial.print(packetBuffer[checksumIndex], HEX);
  Serial.print(" (calculated: 0x");
  Serial.print(calculated_checksum, HEX);
  Serial.print(") ");
  if (packetBuffer[checksumIndex] == calculated_checksum) {
    Serial.println("✓ VALID");
  } else {
    Serial.println("✗ INVALID");
  }

  Serial.println("=============================");
}

// ========== DECODE CONTROLLER COMMAND PACKET (16 bytes) ==========
void decodeControllerPacket() {
  if (packetIndex != 16) return;
  if (packetBuffer[0] != 0xAA || packetBuffer[1] != 0x66) return;

  Serial.println("\n=== CONTROLLER COMMAND DECODED ===");

  // Byte 2: Command type
  Serial.print("Command: 0x");
  Serial.println(packetBuffer[2], HEX);

  // Byte 5: Requested state
  Serial.print("Requested State: ");
  switch (packetBuffer[5]) {
    case 0x00: Serial.println("OFF"); break;
    case 0x01: Serial.println("START"); break;
    case 0x02: Serial.println("RUNNING"); break;
    default:
      Serial.print("UNKNOWN (0x");
      Serial.print(packetBuffer[5], HEX);
      Serial.println(")");
      break;
  }

  // Byte 6: Power level (1-10)
  Serial.print("Power Level: ");
  Serial.print(packetBuffer[6]);
  Serial.println("/10");

  // Byte 15: Checksum
  uint8_t calculated_checksum = 0;
  for (int i = 2; i < 15; i++) {
    calculated_checksum += packetBuffer[i];
  }
  Serial.print("Checksum: 0x");
  Serial.print(packetBuffer[15], HEX);
  Serial.print(" (calculated: 0x");
  Serial.print(calculated_checksum, HEX);
  Serial.print(") ");
  if (packetBuffer[15] == calculated_checksum) {
    Serial.println("✓ VALID");
  } else {
    Serial.println("✗ INVALID");
  }

  Serial.println("==================================");
}

// ========== MAIN LOOP ==========
void loop() {
  // Check for user commands
  if (Serial.available()) {
    char cmd = Serial.read();
    switch (cmd) {
      case 'h':
      case 'H':
        showHex = !showHex;
        Serial.print("Hex display: ");
        Serial.println(showHex ? "ON" : "OFF");
        break;
      case 'a':
      case 'A':
        showAscii = !showAscii;
        Serial.print("ASCII display: ");
        Serial.println(showAscii ? "ON" : "OFF");
        break;
      case 't':
      case 'T':
        showTimestamps = !showTimestamps;
        Serial.print("Timestamps: ");
        Serial.println(showTimestamps ? "ON" : "OFF");
        break;
      case 'p':
      case 'P':
        detectPackets = !detectPackets;
        Serial.print("Packet detection: ");
        Serial.println(detectPackets ? "ON" : "OFF");
        if (!detectPackets && packetIndex > 0) {
          // Print pending packet before disabling
          printPacket();
        }
        break;
      case 'c':
      case 'C':
        Serial.write(27);       // ESC
        Serial.print("[2J");    // Clear screen
        Serial.write(27);       // ESC
        Serial.print("[H");     // Home cursor
        Serial.println("Screen cleared");
        break;
      case 's':
      case 'S':
        showStatistics();
        break;
      case '\n':
      case '\r':
        // Ignore newlines
        break;
      default:
        Serial.println("Unknown command. Press 'h' for hex, 'a' for ASCII, 't' for time, 'p' for packets, 'c' to clear, 's' for stats");
        break;
    }
  }

  // Read serial data from heater
  while (Serial2.available()) {
    uint8_t incomingByte = Serial2.read();
    unsigned long currentTime = millis();

    // Check if this is a new packet (timeout since last byte)
    if (detectPackets && packetIndex > 0 && (currentTime - lastByteTime) > PACKET_TIMEOUT_MS) {
      // Timeout - previous packet is complete
      printPacket();
    }

    // Store byte in buffer
    if (packetIndex < MAX_PACKET_SIZE) {
      packetBuffer[packetIndex++] = incomingByte;
      lastByteTime = currentTime;
    } else {
      // Buffer overflow - print what we have and start fresh
      Serial.println("\n!!! BUFFER OVERFLOW - Packet too large !!!");
      printPacket();
      packetBuffer[0] = incomingByte;
      packetIndex = 1;
      lastByteTime = currentTime;
    }

    // If not using packet detection, print byte immediately
    if (!detectPackets) {
      if (showHex) {
        if (incomingByte < 0x10) Serial.print("0");
        Serial.print(incomingByte, HEX);
        Serial.print(" ");
      }
      if (showAscii) {
        if (incomingByte >= 32 && incomingByte <= 126) {
          Serial.print((char)incomingByte);
        } else {
          Serial.print(".");
        }
      }
      if (showTimestamps && (incomingByte == 0x0A || incomingByte == 0x0D)) {
        Serial.print(" [");
        Serial.print(currentTime);
        Serial.print("ms]");
      }
    }
  }

  // Check if we have a pending packet that hasn't received data in a while
  if (detectPackets && packetIndex > 0 && (millis() - lastByteTime) > PACKET_TIMEOUT_MS) {
    printPacket();
  }

  delay(1);  // Small delay to prevent watchdog issues
}
