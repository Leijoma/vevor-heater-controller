/**
 * VevorHeater Library Implementation
 */

#include "VevorHeater.h"

#define HEATER_BAUD 4800
#define PACKET_TIMEOUT_MS 100
#define COMMAND_REPEATS 3
#define POLL_INTERVAL_ACTIVE_MS 1000   // 1 second when active
#define POLL_INTERVAL_IDLE_MS 10000    // 10 seconds when OFF

VevorHeater::VevorHeater(HardwareSerial &serial, uint8_t rxPin, uint8_t txPin)
  : _serial(serial), _rxPin(rxPin), _txPin(txPin),
    _currentPowerLevel(5), _commandMode(CMD_NONE), _commandRepeatCount(0),
    _lastCommandTime(0), _lastPollTime(0), _rxIndex(0), _lastRxTime(0),
    _stateCallback(nullptr), _errorCallback(nullptr) {

  // Initialize status
  _status.state = STATE_UNKNOWN;
  _status.powerLevel = 0;
  _status.error = ERROR_NONE;
  _status.temperature = 0.0;
  _status.fanSpeed = 0;
  _status.pumpFrequency = 0.0;
  _status.voltage = 0.0;
  _status.timestamp = 0;
}

void VevorHeater::begin() {
  _serial.begin(HEATER_BAUD, SERIAL_8N1, _rxPin, _txPin);

  // Give heater a moment to be ready for communication
  delay(100);

  // Send immediate status request to detect heater's current state
  sendStatusRequest();
  _lastPollTime = millis();
}

void VevorHeater::update() {
  unsigned long now = millis();

  // If we're in UNKNOWN state and haven't received response in 5 seconds, assume heater is OFF
  // (Heater doesn't respond when powered off or in standby)
  if (_status.state == STATE_UNKNOWN && _status.timestamp == 0 && (now - _lastPollTime) > 5000) {
    _status.state = STATE_OFF;
  }

  // Determine poll interval based on state
  unsigned long pollInterval = (_status.state == STATE_OFF) ?
                                POLL_INTERVAL_IDLE_MS : POLL_INTERVAL_ACTIVE_MS;

  // Handle command repeat mode
  if (_commandMode != CMD_NONE && _commandRepeatCount > 0) {
    if (now - _lastCommandTime >= 1000) {  // 1 second between repeats
      if (_commandMode == CMD_START) {
        sendStartCommand(_currentPowerLevel);
      } else if (_commandMode == CMD_STOP) {
        sendStopCommand();
      }

      _commandRepeatCount--;
      _lastCommandTime = now;

      if (_commandRepeatCount == 0) {
        _commandMode = CMD_NONE;
      }
    }
  } else {
    // Normal polling
    if (now - _lastPollTime >= pollInterval) {
      sendStatusRequest();
      _lastPollTime = now;
    }
  }

  // Process incoming data
  while (_serial.available()) {
    uint8_t byte = _serial.read();

    // Check for packet timeout (new packet starting)
    if (_rxIndex > 0 && (now - _lastRxTime) > PACKET_TIMEOUT_MS) {
      processResponse();
      _rxIndex = 0;
    }

    // Store byte
    if (_rxIndex < sizeof(_rxBuffer)) {
      _rxBuffer[_rxIndex++] = byte;
      _lastRxTime = now;
    } else {
      // Buffer overflow, reset
      _rxIndex = 0;
    }
  }

  // Check for packet completion timeout
  if (_rxIndex > 0 && (now - _lastRxTime) > PACKET_TIMEOUT_MS) {
    processResponse();
    _rxIndex = 0;
  }
}

bool VevorHeater::start(uint8_t powerLevel) {
  if (powerLevel < 1 || powerLevel > 10) {
    return false;
  }

  _currentPowerLevel = powerLevel;
  _commandMode = CMD_START;
  _commandRepeatCount = COMMAND_REPEATS;
  _lastCommandTime = millis() - 1000;  // Send immediately

  return true;
}

bool VevorHeater::stop() {
  _commandMode = CMD_STOP;
  _commandRepeatCount = COMMAND_REPEATS;
  _lastCommandTime = millis() - 1000;  // Send immediately

  return true;
}

bool VevorHeater::setPower(uint8_t powerLevel) {
  if (powerLevel < 1 || powerLevel > 10) {
    return false;
  }

  _currentPowerLevel = powerLevel;
  return true;
}

void VevorHeater::sendStatusRequest() {
  uint8_t packet[16] = {
    0xAA, 0x66,           // Header
    0x02,                 // Command: STATUS_REQUEST
    0x0B,                 // Frame length
    0x00, 0x00, 0x00, 0x00,
    _currentPowerLevel,   // Power level (byte 8)
    0x08,                 // Status flag (byte 9)
    0x00, 0x00, 0x00, 0x00, 0x00,
    0x00                  // Checksum
  };

  packet[15] = calculateChecksum(packet, 2, 14);
  _serial.write(packet, 16);
}

void VevorHeater::sendStartCommand(uint8_t powerLevel) {
  uint8_t packet[16] = {
    0xAA, 0x66,           // Header
    0x06,                 // Command: START_STOP
    0x0B,                 // Frame length
    0x00, 0x00, 0x00, 0x00,
    powerLevel,           // Power level (byte 8)
    0x06,                 // State: START (byte 9)
    0x00, 0x00, 0x00, 0x00, 0x00,
    0x00                  // Checksum
  };

  packet[15] = calculateChecksum(packet, 2, 14);
  _serial.write(packet, 16);
}

void VevorHeater::sendStopCommand() {
  uint8_t packet[16] = {
    0xAA, 0x66,           // Header
    0x06,                 // Command: START_STOP
    0x0B,                 // Frame length
    0x00, 0x00, 0x00, 0x00,
    0x0A,                 // Power level (byte 8)
    0x05,                 // State: STOP (byte 9)
    0x00, 0x00, 0x00, 0x00, 0x00,
    0x00                  // Checksum
  };

  packet[15] = calculateChecksum(packet, 2, 14);
  _serial.write(packet, 16);
}

void VevorHeater::processResponse() {
  if (_rxIndex != 57 || _rxBuffer[0] != 0xAA || _rxBuffer[1] != 0x77) {
    return;  // Not a valid heater response packet
  }

  // Verify checksum
  uint8_t checksum = calculateChecksum(_rxBuffer, 2, 55);
  if (checksum != _rxBuffer[56]) {
    return;  // Invalid checksum
  }

  // Save old state for comparison
  HeaterState oldState = _status.state;
  HeaterError oldError = _status.error;

  // Parse response
  _status.state = (HeaterState)_rxBuffer[5];
  _status.powerLevel = _rxBuffer[6];
  _status.error = (HeaterError)_rxBuffer[7];
  _status.voltage = _rxBuffer[11] / 10.0;

  // Check cooling mode flag (byte 14) - if set, override state to COOLDOWN
  // The heater may report state=OFF while byte 14 indicates it's still cooling
  if (_rxBuffer[14] == 0x01 && _status.state == STATE_OFF) {
    _status.state = STATE_COOLDOWN;
  }

  // Temperature (big-endian, bytes 16-17)
  uint16_t temp_raw = (_rxBuffer[16] << 8) | _rxBuffer[17];
  _status.temperature = temp_raw / 10.0;

  // Fan speed (big-endian, bytes 28-29)
  _status.fanSpeed = (_rxBuffer[28] << 8) | _rxBuffer[29];

  // Pump frequency (byte 23, Hz × 10)
  _status.pumpFrequency = _rxBuffer[23] / 10.0;

  _status.timestamp = millis();

  // Notify callbacks
  // Don't trigger state change callback when transitioning FROM STATE_UNKNOWN
  // (initial state detection after boot is not a real state change)
  if (oldState != _status.state && oldState != STATE_UNKNOWN) {
    notifyStateChange(oldState, _status.state);
  }

  if (oldError != _status.error && _status.error != ERROR_NONE) {
    notifyError(_status.error);
  }
}

uint8_t VevorHeater::calculateChecksum(uint8_t* packet, uint8_t start, uint8_t end) {
  uint8_t sum = 0;
  for (uint8_t i = start; i <= end; i++) {
    sum += packet[i];
  }
  return sum;
}

void VevorHeater::notifyStateChange(HeaterState oldState, HeaterState newState) {
  if (_stateCallback) {
    _stateCallback(oldState, newState);
  }
}

void VevorHeater::notifyError(HeaterError error) {
  if (_errorCallback) {
    _errorCallback(error);
  }
}

const char* VevorHeater::getErrorString(HeaterError error) {
  switch (error) {
    case ERROR_NONE: return "No error";
    case ERROR_VOLTAGE_LOW: return "Voltage too low";
    case ERROR_VOLTAGE_HIGH: return "Voltage too high";
    case ERROR_IGNITION_FAIL: return "Ignition failure";
    case ERROR_FLAME_OUT: return "Flame out";
    case ERROR_TEMP_SENSOR: return "Temperature sensor error";
    case ERROR_GLOW_PLUG: return "Glow plug error";
    case ERROR_FAN_MOTOR: return "Fan motor error";
    case ERROR_FUEL_PUMP: return "Fuel pump error";
    default: return "Unknown error";
  }
}

const char* VevorHeater::getStateString(HeaterState state) {
  switch (state) {
    case STATE_OFF: return "OFF";
    case STATE_GLOW_PREHEAT: return "PREHEAT";
    case STATE_IGNITION: return "IGNITION";
    case STATE_COMBUSTION: return "Heating";
    case STATE_COOLDOWN: return "COOLDOWN";
    default: return "UNKNOWN";
  }
}
