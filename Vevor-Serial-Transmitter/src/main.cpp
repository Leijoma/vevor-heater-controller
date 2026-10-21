#include <Arduino.h>

// Pin configuration
#define SERIAL_TX_PIN 17  // GPIO17 - TX data (through gate transistor)
#define SERIAL_RX_PIN 16  // GPIO16 - RX directly to level shifter
#define TX_GATE_PIN 18    // GPIO18 - Controls TX gate transistor (HIGH = TX enabled)

// Protocol constants
#define FRAME_START 0xAA
#define CONTROLLER_ID 0x66
#define HEATER_ID 0x77
#define CMD_STATUS_REQUEST 0x02
#define CMD_START_STOP 0x06
#define FRAME_LENGTH 0x0B

// State constants
#define STATE_OFF 0x02
#define STATE_SET_OFF 0x05
#define STATE_START 0x06
#define STATE_RUNNING 0x08

// Global state
uint8_t currentPowerLevel = 10;
uint8_t currentState = STATE_OFF;
bool heaterOn = false;
bool autoRespondMode = true;  // Automatically respond to controller packets
bool sendErrorMode = false;    // Send error/extreme values to test panel reaction

// Buffer for receiving controller packets
#define RX_BUFFER_SIZE 256
uint8_t rxBuffer[RX_BUFFER_SIZE];
int rxBufferIndex = 0;
unsigned long lastRxTime = 0;
#define RX_TIMEOUT 100  // ms between packets

// Periodic unsolicited transmission (heater sends updates every ~800ms)
unsigned long lastTxTime = 0;
#define TX_INTERVAL 800  // ms between unsolicited heater updates

// Software TX gate control - no transistor needed!
// Dynamically switch GPIO 17 between OUTPUT (can transmit) and INPUT (high-Z, invisible to RX)
// This prevents GPIO 17 from interfering with RX when not transmitting

void enableTX() {
  // Switch GPIO 17 to OUTPUT mode so it can drive the TX line
  Serial2.end();  // Stop Serial2 first
  delay(5);
  pinMode(SERIAL_TX_PIN, OUTPUT);  // Enable TX - can now drive the bus
  Serial2.begin(4800, SERIAL_8N1, SERIAL_RX_PIN, SERIAL_TX_PIN);  // Restart Serial2
  Serial2.setRxInvert(false);  // Maintain RX inversion setting
  delay(5);
}

void disableTX() {
  // Switch GPIO 17 to INPUT mode (high-impedance) so it doesn't interfere with RX
  Serial2.flush();  // Wait for all data to be transmitted
  delay(5);
  Serial2.end();  // Stop Serial2
  pinMode(SERIAL_TX_PIN, INPUT);  // Disable TX - high-Z, electrically invisible!
  Serial2.begin(4800, SERIAL_8N1, SERIAL_RX_PIN, SERIAL_TX_PIN);  // Restart Serial2
  Serial2.setRxInvert(false);  // Maintain RX inversion setting
  delay(5);
}

// Calculate checksum (sum of bytes 2-14, mod 256) for 16-byte controller packets
uint8_t calculateChecksum(uint8_t *packet) {
  uint8_t checksum = 0;
  for (int i = 2; i <= 14; i++) {
    checksum += packet[i];
  }
  return checksum & 0xFF;
}

// Calculate checksum for heater response packets (56 bytes: sum bytes 2 to second-to-last)
uint8_t calculateHeaterChecksum(uint8_t *packet, int length) {
  uint8_t checksum = 0;
  for (int i = 2; i < length - 1; i++) {  // Sum from byte 2 to second-to-last
    checksum += packet[i];
  }
  return checksum & 0xFF;
}

// Send heater status response packet (56 bytes)
void sendHeaterResponse(uint8_t requestedPower, uint8_t requestedState) {
  // Build a basic 56-byte heater status packet
  uint8_t packet[56];

  // Initialize all to zero
  memset(packet, 0, 56);

  // Header (from ESPHome research)
  packet[0] = FRAME_START;     // 0xAA
  packet[1] = HEATER_ID;        // 0x77 (heater ID)
  packet[2] = 0x06;             // Response type (guessing based on command)
  packet[3] = 0x33;             // Frame length (51 bytes = 0x33)

  // Status information - echo back controller's request
  packet[5] = requestedState;   // Echo the state controller requested (will implement state machine later)
  packet[6] = requestedPower;   // Current power level

  if (sendErrorMode) {
    // ERROR MODE - Send extreme/error values to test panel reaction

    // Voltage (byte 11) - CRITICAL LOW VOLTAGE (8.5V)
    packet[11] = 85;              // 8.5V * 10 (should trigger low voltage warning)

    // Glow plug current (byte 13) - OVERHEAT (15A)
    packet[13] = 150;             // 15.0A * 10 (way too high, should trigger error)

    // Heat exchanger temperature (bytes 16-17) - EXTREME TEMP (120°C)
    int16_t temp = 12000;         // 120.0°C × 100 (correct Vevor scaling!)
    packet[16] = (temp >> 8) & 0xFF;  // High byte
    packet[17] = temp & 0xFF;         // Low byte

    // Fan speed (bytes 28-29) - VERY HIGH (5000 RPM)
    uint16_t fanSpeed = 5000;
    packet[28] = (fanSpeed >> 8) & 0xFF;  // High byte
    packet[29] = fanSpeed & 0xFF;         // Low byte

    // Pump frequency (byte 23) - MAX
    packet[23] = 15;              // Hz, very high

    Serial.println("  ⚠️  ERROR MODE: Sending extreme values!");

  } else {
    // NORMAL MODE - Send realistic values

    // Voltage (byte 11) - simulate 13.2V
    packet[11] = 132;             // 13.2V * 10

    // Glow plug current (byte 13) - simulate 2.5A during start
    if (requestedState == STATE_START) {
      packet[13] = 25;            // 2.5A * 10
    } else if (requestedState == STATE_RUNNING) {
      packet[13] = 5;             // 0.5A during running
    } else {
      packet[13] = 0;             // 0A when off
    }

    // Heat exchanger temperature (bytes 16-17, signed int16, big-endian)
    // CORRECT VEVOR SCALING: °C × 100 (not × 10!)
    // Always send temperature - varies by state
    int16_t heatTemp = 2000;  // Default 20°C × 100
    if (requestedState == STATE_START || requestedState == 0x06) {
      heatTemp = 2500;  // 25°C × 100 - starting up
    } else if (requestedState == STATE_RUNNING || requestedState == 0x08) {
      heatTemp = 4500;  // 45°C × 100 - running
    }
    packet[16] = (heatTemp >> 8) & 0xFF;  // High byte
    packet[17] = heatTemp & 0xFF;         // Low byte

    // Fan speed (bytes 28-29) - simulate 2500 RPM
    if (requestedState == STATE_RUNNING || requestedState == STATE_START) {
      uint16_t fanSpeed = 2500;
      packet[28] = (fanSpeed >> 8) & 0xFF;  // High byte
      packet[29] = fanSpeed & 0xFF;         // Low byte
    }

    // Pump frequency (byte 23) - varies with power level
    packet[23] = requestedPower;  // Hz, roughly matches power level
  }

  // Calculate and set checksum (last byte)
  packet[55] = calculateHeaterChecksum(packet, 56);

  // Enable TX, send packet, disable TX
  enableTX();
  Serial2.write(packet, 56);
  disableTX();

  // Print what we sent
  Serial.print("\n[HEATER RESPONSE] ");
  for (int i = 0; i < 56; i++) {
    if (packet[i] < 0x10) Serial.print("0");
    Serial.print(packet[i], HEX);
    Serial.print(" ");
    if ((i + 1) % 16 == 0) Serial.print("\n                  ");
  }
  Serial.printf("\n  From: HEATER (0x77)\n");
  Serial.printf("  State: 0x%02X\n", requestedState);
  Serial.printf("  Power: %d/10\n", requestedPower);
  Serial.printf("  Checksum: 0x%02X\n", packet[55]);
}

// Send command packet to heater
void sendHeaterCommand(uint8_t powerLevel, uint8_t state) {
  uint8_t packet[16] = {
    FRAME_START,        // Byte 0: Frame start marker
    CONTROLLER_ID,      // Byte 1: Controller ID
    CMD_START_STOP,     // Byte 2: Command (START/STOP)
    FRAME_LENGTH,       // Byte 3: Frame length
    0x00, 0x00, 0x00, 0x00,  // Bytes 4-7: Reserved
    powerLevel,         // Byte 8: Power level (1-10)
    state,              // Byte 9: State
    0x00, 0x00, 0x00, 0x00, 0x00,  // Bytes 10-14: Reserved
    0x00                // Byte 15: Checksum (calculated below)
  };

  // Calculate and set checksum
  packet[15] = calculateChecksum(packet);

  // Enable TX, send packet, disable TX
  enableTX();
  Serial2.write(packet, 16);
  disableTX();

  // Print what we sent
  Serial.print("\n[SENT] ");
  for (int i = 0; i < 16; i++) {
    if (packet[i] < 0x10) Serial.print("0");
    Serial.print(packet[i], HEX);
    Serial.print(" ");
  }

  Serial.printf("\n  Power level: %d/10\n", powerLevel);
  Serial.print("  State: 0x");
  Serial.print(state, HEX);
  Serial.print(" (");
  if (state == STATE_OFF) Serial.print("OFF");
  else if (state == STATE_SET_OFF) Serial.print("SET_OFF");
  else if (state == STATE_START) Serial.print("START");
  else if (state == STATE_RUNNING) Serial.print("RUNNING");
  else Serial.print("UNKNOWN");
  Serial.println(")");
  Serial.printf("  Checksum: 0x%02X\n", packet[15]);
}

void turnHeaterOn() {
  if (!heaterOn) {
    heaterOn = true;
    currentPowerLevel = 10;  // Always start at max power
    currentState = STATE_START;

    Serial.println("\n=== TURNING HEATER ON ===");
    sendHeaterCommand(currentPowerLevel, currentState);

    delay(1000);  // Wait a bit

    // Follow up with RUNNING state
    currentState = STATE_RUNNING;
    Serial.println("\n=== CONFIRMING RUNNING STATE ===");
    sendHeaterCommand(currentPowerLevel, currentState);
  } else {
    Serial.println("Heater already ON");
  }
}

void turnHeaterOff() {
  if (heaterOn) {
    heaterOn = false;
    currentState = STATE_OFF;
    currentPowerLevel = 1;

    Serial.println("\n=== TURNING HEATER OFF ===");
    sendHeaterCommand(currentPowerLevel, currentState);
  } else {
    Serial.println("Heater already OFF");
  }
}

void setPowerLevel(uint8_t level) {
  if (level < 1) level = 1;
  if (level > 10) level = 10;

  currentPowerLevel = level;

  Serial.printf("\n=== SETTING POWER LEVEL: %d/10 ===\n", level);
  sendHeaterCommand(currentPowerLevel, heaterOn ? STATE_RUNNING : STATE_OFF);
}

void increasePower() {
  if (currentPowerLevel < 10) {
    setPowerLevel(currentPowerLevel + 1);
  } else {
    Serial.println("Already at maximum power (10)");
  }
}

void decreasePower() {
  if (currentPowerLevel > 1) {
    setPowerLevel(currentPowerLevel - 1);
  } else {
    Serial.println("Already at minimum power (1)");
  }
}

void printMenu() {
  Serial.println("\n╔══════════════════════════════════════════════════════╗");
  Serial.println("║     Vevor Heater Serial Transmitter                 ║");
  Serial.println("║          (Heater Impersonation Mode)                 ║");
  Serial.println("╚══════════════════════════════════════════════════════╝");
  Serial.println("\n📡 MODE: Impersonate heater, respond to controller\n");
  Serial.println("Commands:");
  Serial.println("  a      - Toggle auto-respond mode");
  Serial.println("  e      - Toggle ERROR mode (send extreme values)");
  Serial.println("  r      - Manually send heater response");
  Serial.println("  s      - Show current status");
  Serial.println("  h      - Show this help menu");
  Serial.println();
  Serial.println("Send commands to heater (if heater present):");
  Serial.println("  1      - Turn heater ON (power level 10)");
  Serial.println("  0      - Turn heater OFF");
  Serial.println("  +      - Increase power level");
  Serial.println("  -      - Decrease power level");
  Serial.println("  2-9    - Set specific power level (2-9)");
  Serial.println();
  Serial.println("Current state:");
  Serial.printf("  Auto-respond: %s\n", autoRespondMode ? "ENABLED" : "DISABLED");
  Serial.printf("  Simulated power: %d/10\n", currentPowerLevel);
  Serial.printf("  Simulated state: 0x%02X\n", currentState);
  Serial.println("════════════════════════════════════════════════════════\n");
}

void setup() {
  // USB serial for commands and output
  Serial.begin(115200);
  delay(1000);

  // Serial2 for heater communication
  Serial2.begin(4800, SERIAL_8N1, SERIAL_RX_PIN, SERIAL_TX_PIN);
  Serial2.setRxInvert(false);  // RX inversion that works (confirmed!)

  // Initialize GPIO 17 as INPUT (high-impedance) - TX starts disabled
  // This prevents GPIO 17 from interfering with RX until we need to transmit
  pinMode(SERIAL_TX_PIN, INPUT);  // Start in high-Z mode

  // Note: No TX inversion available in this ESP32 core version
  // If panel doesn't react, we may need to add external signal inversion

  Serial.println("\n\n");
  Serial.println("╔══════════════════════════════════════════════════════╗");
  Serial.println("║     Vevor Heater Serial Transmitter                 ║");
  Serial.println("║      (Software TX Gating - No Transistor!)          ║");
  Serial.println("╚══════════════════════════════════════════════════════╝\n");
  Serial.println("Hardware: ESP32 + 4-channel level shifter");
  Serial.println("RX/TX:    GPIO 16 & 17 → SAME level shifter channel");
  Serial.println("Baud:     4800 (8N1)");
  Serial.println("RX Invert: FALSE (confirmed working!)");
  Serial.println("TX Gating: Software (pinMode switching)");
  Serial.println();
  Serial.println("⚠️  SIMPLE WIRING (No transistor needed!):");
  Serial.println("   ESP32          Level Shifter        Heater Panel");
  Serial.println("   ─────────────────────────────────────────────────");
  Serial.println("   GPIO 16 (RX) ┐");
  Serial.println("                ├──→ LV4 ──→ HV4 ──→ Blue wire");
  Serial.println("   GPIO 17 (TX) ┘     (SAME channel!)");
  Serial.println();
  Serial.println("   3.3V ────────→ LV VCC");
  Serial.println("   GND ─────────→ LV GND");
  Serial.println("                  HV VCC ←───── Red wire (+5V)");
  Serial.println("                  HV GND ←───── Black wire (GND)");
  Serial.println();
  Serial.println("📡 MODE: Auto-respond to controller packets");
  Serial.println("   GPIO 17 switches between INPUT (high-Z) and OUTPUT");
  Serial.println("   This prevents TX from interfering with RX!");
  Serial.println("   Watch the controller display for reactions!");
  Serial.println();
  Serial.println("Ready!\n");

  printMenu();
}

void loop() {
  unsigned long currentTime = millis();

  // Listen for controller packets on the bus
  if (Serial2.available()) {
    uint8_t inByte = Serial2.read();
    lastRxTime = currentTime;

    if (rxBufferIndex < RX_BUFFER_SIZE) {
      rxBuffer[rxBufferIndex++] = inByte;
    }
  }

  // Check for packet timeout (end of packet)
  if (rxBufferIndex > 0 && (currentTime - lastRxTime) > RX_TIMEOUT) {
    // Print received packet
    Serial.print("\n[RECEIVED] ");
    for (int i = 0; i < rxBufferIndex; i++) {
      if (rxBuffer[i] < 0x10) Serial.print("0");
      Serial.print(rxBuffer[i], HEX);
      Serial.print(" ");
    }
    Serial.println();

    // Check if this is a controller packet (16 bytes, starts with AA 66)
    if (rxBufferIndex == 16 && rxBuffer[0] == FRAME_START && rxBuffer[1] == CONTROLLER_ID) {
      uint8_t command = rxBuffer[2];
      uint8_t powerLevel = rxBuffer[8];
      uint8_t state = rxBuffer[9];
      uint8_t checksum = rxBuffer[15];

      Serial.println("  ┌─ Controller Command:");
      Serial.printf("  │ Command: 0x%02X\n", command);
      Serial.printf("  │ Power: %d/10\n", powerLevel);
      Serial.printf("  │ State: 0x%02X\n", state);
      Serial.printf("  │ Checksum: 0x%02X\n", checksum);
      Serial.println("  └─");

      // Auto-respond if enabled
      if (autoRespondMode) {
        Serial.println("  → Auto-responding as HEATER...");
        delay(50);  // Small delay before responding
        sendHeaterResponse(powerLevel, state);
        lastTxTime = currentTime;  // Reset periodic timer after responding
      }
    }

    // Reset buffer
    rxBufferIndex = 0;
  }

  // NOTE: Periodic updates REMOVED - Vevor protocol is REQUEST-RESPONSE
  // Controller continuously polls, we only respond when asked

  // Handle serial commands
  if (Serial.available()) {
    char cmd = Serial.read();

    switch (cmd) {
      case '1':
        turnHeaterOn();
        break;

      case '0':
        turnHeaterOff();
        break;

      case '+':
      case '=':
        increasePower();
        break;

      case '-':
      case '_':
        decreasePower();
        break;

      case '2':
      case '3':
      case '4':
      case '5':
      case '6':
      case '7':
      case '8':
      case '9':
        setPowerLevel(cmd - '0');
        break;

      case 'a':
      case 'A':
        autoRespondMode = !autoRespondMode;
        Serial.printf("\nAuto-respond mode: %s\n", autoRespondMode ? "ENABLED" : "DISABLED");
        break;

      case 'e':
      case 'E':
        sendErrorMode = !sendErrorMode;
        Serial.printf("\n⚠️  ERROR MODE: %s\n", sendErrorMode ? "ENABLED (sending extreme values)" : "DISABLED (sending normal values)");
        if (sendErrorMode) {
          Serial.println("   - Low voltage: 8.5V");
          Serial.println("   - High current: 15A");
          Serial.println("   - Overheat: 120°C");
          Serial.println("   - High fan speed: 5000 RPM");
        }
        break;

      case 'r':
      case 'R':
        // Manually send heater response
        Serial.println("\n=== SENDING MANUAL HEATER RESPONSE ===");
        sendHeaterResponse(currentPowerLevel, currentState);
        break;

      case 's':
      case 'S':
        Serial.println("\n=== CURRENT STATUS ===");
        Serial.printf("Heater: %s\n", heaterOn ? "ON" : "OFF");
        Serial.printf("Power level: %d/10\n", currentPowerLevel);
        Serial.printf("State: 0x%02X\n", currentState);
        Serial.printf("Auto-respond: %s\n", autoRespondMode ? "ENABLED" : "DISABLED");
        Serial.printf("Error mode: %s\n", sendErrorMode ? "ENABLED ⚠️" : "DISABLED");
        Serial.println("======================\n");
        break;

      case 'h':
      case 'H':
        printMenu();
        break;

      case '\n':
      case '\r':
        // Ignore newlines
        break;

      default:
        Serial.println("Unknown command. Press 'h' for help.");
        break;
    }
  }

  delay(10);
}
