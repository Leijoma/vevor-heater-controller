#include <Arduino.h>

// ========== SERIAL CONFIGURATION ==========
#define HEATER_RX_PIN 16       // GPIO16 - Receive from heater
#define HEATER_TX_PIN 17       // GPIO17 - Transmit to heater
#define HEATER_BAUD 4800       // Vevor protocol baud rate

// ========== PACKET CONFIGURATION ==========
#define PACKET_TIMEOUT_MS 100  // Timeout for receiving response
#define MAX_PACKET_SIZE 256

// ========== GLOBAL STATE ==========
uint8_t currentPowerLevel = 5;
bool heaterRunning = false;
unsigned long lastStatusRequest = 0;
#define STATUS_INTERVAL_ACTIVE_MS 1000   // Poll every 1 second when active
#define STATUS_INTERVAL_IDLE_MS 10000    // Poll every 10 seconds when OFF
uint8_t currentState = 0xFF;  // Current heater state
uint8_t lastState = 0xFF;     // Track state changes
uint8_t lastError = 0x00;     // Track error changes
bool verboseResponse = false; // Control response verbosity

// Command repeat logic
enum CommandType { CMD_NONE, CMD_START, CMD_STOP };
CommandType activeCommand = CMD_NONE;
uint8_t commandRepeatCount = 0;
#define COMMAND_REPEATS 3  // Send START/STOP commands this many times

// ========== PACKET BUFFER ==========
uint8_t rxBuffer[MAX_PACKET_SIZE];
int rxIndex = 0;
unsigned long lastRxTime = 0;

// ========== CALCULATE CHECKSUM ==========
uint8_t calculateChecksum(uint8_t* packet, int start, int end) {
  uint8_t sum = 0;
  for (int i = start; i <= end; i++) {
    sum += packet[i];
  }
  return sum;
}

// ========== SEND STATUS REQUEST ==========
void sendStatusRequest(bool verbose = false) {
  uint8_t packet[16] = {
    0xAA, 0x66,  // Header
    0x02,        // Command: STATUS_REQUEST
    0x0B,        // Frame length
    0x00, 0x00, 0x00, 0x00,
    currentPowerLevel, 0x08,  // Byte 8: Current power level, Byte 9: 0x08
    0x00, 0x00, 0x00, 0x00, 0x00,
    0x00         // Checksum (calculated below)
  };

  // Calculate checksum (sum of bytes 2-14)
  packet[15] = calculateChecksum(packet, 2, 14);

  if (verbose) {
    Serial.println("\n→ Sending STATUS_REQUEST");
    Serial.print("   TX HEX: ");
    for (int i = 0; i < 16; i++) {
      if (packet[i] < 0x10) Serial.print("0");
      Serial.print(packet[i], HEX);
      Serial.print(" ");
    }
    Serial.println();
  }
  Serial2.write(packet, 16);
}

// ========== SEND START COMMAND ==========
void sendStartCommand(uint8_t powerLevel) {
  if (powerLevel < 1) powerLevel = 1;
  if (powerLevel > 10) powerLevel = 10;

  uint8_t packet[16] = {
    0xAA, 0x66,  // Header
    0x06,        // Command: START_STOP
    0x0B,        // Frame length
    0x00, 0x00, 0x00, 0x00,
    powerLevel,  // Power level (byte 8)
    0x06,        // State: START (byte 9)
    0x00, 0x00, 0x00, 0x00, 0x00,
    0x00         // Checksum
  };

  packet[15] = calculateChecksum(packet, 2, 14);

  Serial.print("\n→ Sending START command (Power: ");
  Serial.print(powerLevel);
  Serial.println("/10)");

  // Show hex for debugging
  Serial.print("   HEX: ");
  for (int i = 0; i < 16; i++) {
    if (packet[i] < 0x10) Serial.print("0");
    Serial.print(packet[i], HEX);
    Serial.print(" ");
  }
  Serial.println();

  Serial2.write(packet, 16);
  currentPowerLevel = powerLevel;
  heaterRunning = true;
}

// ========== SEND STOP COMMAND ==========
void sendStopCommand() {
  uint8_t packet[16] = {
    0xAA, 0x66,  // Header
    0x06,        // Command: START_STOP
    0x0B,        // Frame length
    0x00, 0x00, 0x00, 0x00,
    0x0A,        // Power level (kept at current)
    0x05,        // State: SET_OFF (byte 9)
    0x00, 0x00, 0x00, 0x00, 0x00,
    0x00         // Checksum
  };

  packet[15] = calculateChecksum(packet, 2, 14);

  Serial.println("\n→ Sending STOP command");

  // Show hex for debugging
  Serial.print("   HEX: ");
  for (int i = 0; i < 16; i++) {
    if (packet[i] < 0x10) Serial.print("0");
    Serial.print(packet[i], HEX);
    Serial.print(" ");
  }
  Serial.println();

  Serial2.write(packet, 16);
  heaterRunning = false;
}

// ========== DECODE HEATER RESPONSE ==========
void decodeResponse() {
  if (rxIndex == 0) return;

  if (rxIndex == 57 && rxBuffer[0] == 0xAA && rxBuffer[1] == 0x77) {
    // Heater status packet
    uint8_t state = rxBuffer[5];
    uint8_t power = rxBuffer[6];
    uint8_t error = rxBuffer[7];
    uint16_t temp_raw = (rxBuffer[16] << 8) | rxBuffer[17];
    float temp = temp_raw / 10.0;
    uint16_t fan = (rxBuffer[28] << 8) | rxBuffer[29];
    float voltage = rxBuffer[11] / 10.0;

    // Update current state
    currentState = state;

    // Check if state or error changed
    bool stateChanged = (state != lastState);
    bool errorChanged = (error != lastError);
    lastState = state;
    lastError = error;

    // Show detailed if verbose, state changed, error changed, or error present
    bool showDetailed = verboseResponse || stateChanged || errorChanged || (error != 0);

    if (showDetailed) {
      Serial.println("\n← HEATER RESPONSE:");
      Serial.print("   State: ");
      switch(state) {
        case 0x00: Serial.println("OFF"); break;
        case 0x01: Serial.println("GLOW_PREHEAT"); break;
        case 0x02: Serial.println("IGNITION"); break;
        case 0x03: Serial.println("COMBUSTION"); break;
        case 0x04: Serial.println("COOLDOWN"); break;
        default: Serial.print("UNKNOWN (0x"); Serial.print(state, HEX); Serial.println(")");
      }
      Serial.print("   Power: "); Serial.print(power); Serial.println("/10");
      Serial.print("   Temp: "); Serial.print(temp, 1); Serial.println(" °C");
      Serial.print("   Fan: "); Serial.print(fan); Serial.println(" RPM");
      Serial.print("   Voltage: "); Serial.print(voltage, 1); Serial.println(" V");

      if (error != 0) {
        Serial.print("   ERROR: 0x"); Serial.print(error, HEX); Serial.print(" - ");
        switch(error) {
          case 0x01: Serial.println("Voltage too low"); break;
          case 0x02: Serial.println("Voltage too high"); break;
          case 0x03: Serial.println("Ignition failure"); break;
          case 0x04: Serial.println("Flame out"); break;
          case 0x05: Serial.println("Temperature sensor error"); break;
          case 0x06: Serial.println("Glow plug error"); break;
          case 0x08: Serial.println("Fan motor error"); break;
          case 0x09: Serial.println("Fuel pump error"); break;
          default: Serial.println("Unknown error"); break;
        }
      }
    } else {
      // Compact one-line output
      Serial.print("[");
      switch(state) {
        case 0x00: Serial.print("OFF"); break;
        case 0x01: Serial.print("PREHEAT"); break;
        case 0x02: Serial.print("IGNIT"); break;
        case 0x03: Serial.print("BURN"); break;
        case 0x04: Serial.print("COOL"); break;
        default: Serial.print("?"); break;
      }
      Serial.print(" T:");
      Serial.print(temp, 0);
      Serial.print("C F:");
      Serial.print(fan);
      Serial.print(" V:");
      Serial.print(voltage, 1);
      Serial.println("]");
    }
  } else {
    // Unknown packet - show hex dump
    Serial.print("\n← Received ");
    Serial.print(rxIndex);
    Serial.println(" bytes (unknown packet)");
    Serial.print("   HEX: ");
    for (int i = 0; i < rxIndex && i < 64; i++) {
      if (rxBuffer[i] < 0x10) Serial.print("0");
      Serial.print(rxBuffer[i], HEX);
      Serial.print(" ");
      if ((i + 1) % 16 == 0) Serial.print("\n        ");
    }
    Serial.println();
  }

  rxIndex = 0;
  verboseResponse = false;  // Reset after each response
}

// ========== SETUP ==========
void setup() {
  Serial.begin(115200);
  delay(2000);

  Serial.println("\n\n========================================");
  Serial.println("  Vevor Heater Control Test Interface");
  Serial.println("========================================");
  Serial.println("\nConfiguration:");
  Serial.print("  Heater Serial: GPIO");
  Serial.print(HEATER_RX_PIN);
  Serial.print(" (RX), GPIO");
  Serial.print(HEATER_TX_PIN);
  Serial.print(" (TX), ");
  Serial.print(HEATER_BAUD);
  Serial.println(" baud");
  Serial.println("\nCommands:");
  Serial.println("  status           - Request heater status");
  Serial.println("  start [1-10]     - Start heater at power level");
  Serial.println("  stop             - Stop heater");
  Serial.println("  power [1-10]     - Set power level (works while running!)");
  Serial.println();
  Serial.println("========================================\n");

  // Initialize heater serial (TX enabled!)
  Serial2.begin(HEATER_BAUD, SERIAL_8N1, HEATER_RX_PIN, HEATER_TX_PIN);

  Serial.println("Ready! Type a command...");
  Serial.print("> ");
}

// ========== PROCESS COMMAND ==========
void processCommand(String cmd) {
  cmd.trim();
  cmd.toLowerCase();

  if (cmd == "status") {
    verboseResponse = true;
    sendStatusRequest(true);  // Verbose when manually requested
  }
  else if (cmd.startsWith("start")) {
    int power = currentPowerLevel;
    if (cmd.length() > 6) {
      power = cmd.substring(6).toInt();
    }
    verboseResponse = true;
    currentPowerLevel = power;
    activeCommand = CMD_START;
    commandRepeatCount = COMMAND_REPEATS;  // Send START 3 times
    heaterRunning = true;
    Serial.print("Starting heater - will send START command ");
    Serial.print(COMMAND_REPEATS);
    Serial.println(" times...");
  }
  else if (cmd == "stop") {
    verboseResponse = true;
    activeCommand = CMD_STOP;
    commandRepeatCount = COMMAND_REPEATS;  // Send STOP 3 times
    Serial.print("Stopping heater - will send STOP command ");
    Serial.print(COMMAND_REPEATS);
    Serial.println(" times...");
  }
  else if (cmd.startsWith("power")) {
    if (cmd.length() > 6) {
      int power = cmd.substring(6).toInt();
      if (power >= 1 && power <= 10) {
        currentPowerLevel = power;
        Serial.print("Power level set to ");
        Serial.print(power);
        Serial.println("/10");
        if (heaterRunning) {
          Serial.println("Power change will take effect on next status poll (~1 second)");
        } else {
          Serial.println("(Will be applied when heater starts)");
        }
      } else {
        Serial.println("Error: Power must be 1-10");
      }
    } else {
      Serial.println("Error: Usage: power [1-10]");
    }
  }
  else if (cmd == "help" || cmd == "?") {
    Serial.println("\nCommands:");
    Serial.println("  status       - Request heater status");
    Serial.println("  start [1-10] - Start heater at power level");
    Serial.println("  stop         - Stop heater");
    Serial.println("  power [1-10] - Set power level (works while running!)");
  }
  else if (cmd.length() > 0) {
    Serial.print("Unknown command: ");
    Serial.println(cmd);
    Serial.println("Type 'help' for command list");
  }
}

// ========== MAIN LOOP ==========
void loop() {
  unsigned long now = millis();

  // Determine polling interval based on heater state
  unsigned long pollInterval;
  if (currentState == 0x00) {
    // OFF state - poll slowly (every 10 seconds)
    pollInterval = STATUS_INTERVAL_IDLE_MS;
  } else {
    // Active states (PREHEAT, IGNITION, COMBUSTION, COOLDOWN) - poll fast
    pollInterval = STATUS_INTERVAL_ACTIVE_MS;
  }

  // If we have an active command (START or STOP), prioritize sending those
  if (activeCommand != CMD_NONE && commandRepeatCount > 0) {
    if (now - lastStatusRequest >= STATUS_INTERVAL_ACTIVE_MS) {
      if (activeCommand == CMD_START) {
        sendStartCommand(currentPowerLevel);
      } else if (activeCommand == CMD_STOP) {
        sendStopCommand();
      }

      commandRepeatCount--;
      lastStatusRequest = now;

      if (commandRepeatCount == 0) {
        Serial.print(activeCommand == CMD_START ? "START" : "STOP");
        Serial.println(" command sequence complete. Resuming auto-polling...");
        activeCommand = CMD_NONE;
      }
    }
  } else {
    // Normal auto-polling when not sending command sequences
    if (now - lastStatusRequest >= pollInterval) {
      sendStatusRequest();
      lastStatusRequest = now;
    }
  }

  // Read commands from USB serial
  static String commandBuffer = "";
  while (Serial.available()) {
    char c = Serial.read();
    if (c == '\n' || c == '\r') {
      if (commandBuffer.length() > 0) {
        processCommand(commandBuffer);
        commandBuffer = "";
        Serial.print("\n> ");
      }
    } else if (c >= 32 && c <= 126) {
      commandBuffer += c;
      Serial.print(c);  // Echo
    }
  }

  // Read responses from heater
  while (Serial2.available()) {
    uint8_t byte = Serial2.read();
    unsigned long now = millis();

    // Check for packet timeout (new packet starting)
    if (rxIndex > 0 && (now - lastRxTime) > PACKET_TIMEOUT_MS) {
      decodeResponse();
    }

    // Store byte
    if (rxIndex < MAX_PACKET_SIZE) {
      rxBuffer[rxIndex++] = byte;
      lastRxTime = now;
    } else {
      Serial.println("\n!!! RX Buffer overflow !!!");
      rxIndex = 0;
    }
  }

  // Check for packet timeout
  if (rxIndex > 0 && (millis() - lastRxTime) > PACKET_TIMEOUT_MS) {
    decodeResponse();
  }

  delay(1);
}
