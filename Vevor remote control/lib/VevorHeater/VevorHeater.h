/**
 * VevorHeater Library
 *
 * Arduino library for controlling Vevor diesel heaters via serial protocol
 *
 * Hardware Requirements:
 * - 5V to 3.3V level shifter (e.g., BSS138 MOSFET)
 * - 10kΩ pull-up resistor
 *
 * Author: Reverse engineered protocol
 * Date: 2026-04-02
 */

#ifndef VEVOR_HEATER_H
#define VEVOR_HEATER_H

#include <Arduino.h>

// Heater states
enum HeaterState {
  STATE_OFF = 0x00,
  STATE_GLOW_PREHEAT = 0x01,
  STATE_IGNITION = 0x02,
  STATE_COMBUSTION = 0x03,
  STATE_COOLDOWN = 0x04,
  STATE_UNKNOWN = 0xFF
};

// Error codes
enum HeaterError {
  ERROR_NONE = 0x00,
  ERROR_VOLTAGE_LOW = 0x01,
  ERROR_VOLTAGE_HIGH = 0x02,
  ERROR_IGNITION_FAIL = 0x03,
  ERROR_FLAME_OUT = 0x04,
  ERROR_TEMP_SENSOR = 0x05,
  ERROR_GLOW_PLUG = 0x06,
  ERROR_FAN_MOTOR = 0x08,
  ERROR_FUEL_PUMP = 0x09
};

// Heater status structure
struct HeaterStatus {
  HeaterState state;
  uint8_t powerLevel;      // 1-10
  HeaterError error;
  float temperature;       // °C
  uint16_t fanSpeed;       // RPM
  float pumpFrequency;     // Hz
  float voltage;           // V
  unsigned long timestamp; // millis() when received
};

// Callback function types
typedef void (*HeaterStateCallback)(HeaterState oldState, HeaterState newState);
typedef void (*HeaterErrorCallback)(HeaterError error);

class VevorHeater {
public:
  /**
   * Constructor
   * @param serial HardwareSerial port (e.g., Serial2)
   * @param rxPin RX pin number
   * @param txPin TX pin number
   */
  VevorHeater(HardwareSerial &serial, uint8_t rxPin, uint8_t txPin);

  /**
   * Initialize the heater communication
   */
  void begin();

  /**
   * Update loop - call this frequently in main loop()
   */
  void update();

  /**
   * Start the heater at specified power level
   * @param powerLevel Power level 1-10
   * @return true if command sent successfully
   */
  bool start(uint8_t powerLevel);

  /**
   * Stop the heater (initiates cooldown)
   * @return true if command sent successfully
   */
  bool stop();

  /**
   * Set power level (works while running)
   * @param powerLevel Power level 1-10
   * @return true if valid power level
   */
  bool setPower(uint8_t powerLevel);

  /**
   * Get current power level setting
   */
  uint8_t getPower() const { return _currentPowerLevel; }

  /**
   * Get current heater state
   */
  HeaterState getState() const { return _status.state; }

  /**
   * Get current error code
   */
  HeaterError getError() const { return _status.error; }

  /**
   * Get current temperature
   */
  float getTemperature() const { return _status.temperature; }

  /**
   * Get current fan speed
   */
  uint16_t getFanSpeed() const { return _status.fanSpeed; }

  /**
   * Get current pump frequency
   */
  float getPumpFrequency() const { return _status.pumpFrequency; }

  /**
   * Get current voltage
   */
  float getVoltage() const { return _status.voltage; }

  /**
   * Get full status structure
   */
  HeaterStatus getStatus() const { return _status; }

  /**
   * Check if heater is active (not OFF)
   */
  bool isActive() const { return _status.state != STATE_OFF; }

  /**
   * Check if there's an error
   */
  bool hasError() const { return _status.error != ERROR_NONE; }

  /**
   * Set callback for state changes
   */
  void onStateChange(HeaterStateCallback callback) { _stateCallback = callback; }

  /**
   * Set callback for errors
   */
  void onError(HeaterErrorCallback callback) { _errorCallback = callback; }

  /**
   * Get error description string
   */
  static const char* getErrorString(HeaterError error);

  /**
   * Get state description string
   */
  static const char* getStateString(HeaterState state);

private:
  // Serial communication
  HardwareSerial &_serial;
  uint8_t _rxPin;
  uint8_t _txPin;

  // Current state
  HeaterStatus _status;
  uint8_t _currentPowerLevel;

  // Command state machine
  enum CommandMode { CMD_NONE, CMD_START, CMD_STOP };
  CommandMode _commandMode;
  uint8_t _commandRepeatCount;
  unsigned long _lastCommandTime;

  // Polling
  unsigned long _lastPollTime;

  // RX buffer
  uint8_t _rxBuffer[64];
  uint8_t _rxIndex;
  unsigned long _lastRxTime;

  // Callbacks
  HeaterStateCallback _stateCallback;
  HeaterErrorCallback _errorCallback;

  // Private methods
  void sendStatusRequest();
  void sendStartCommand(uint8_t powerLevel);
  void sendStopCommand();
  void processResponse();
  uint8_t calculateChecksum(uint8_t* packet, uint8_t start, uint8_t end);
  void notifyStateChange(HeaterState oldState, HeaterState newState);
  void notifyError(HeaterError error);
};

#endif // VEVOR_HEATER_H
