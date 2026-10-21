/**
 * RotaryEncoder Library
 *
 * Advanced rotary encoder handler with:
 * - Rotation detection with configurable resolution
 * - Short press detection
 * - Long press detection
 * - Double click detection
 * - Interrupt-driven for reliability
 *
 * Author: Claude
 * License: MIT
 */

#ifndef ROTARY_ENCODER_H
#define ROTARY_ENCODER_H

#include <Arduino.h>

// Button event types
enum EncoderButton {
  BTN_NONE = 0,
  BTN_SHORT_PRESS,
  BTN_LONG_PRESS,
  BTN_DOUBLE_CLICK
};

// Rotation direction
enum EncoderDirection {
  DIR_NONE = 0,
  DIR_CW = 1,   // Clockwise
  DIR_CCW = -1  // Counter-clockwise
};

class RotaryEncoder {
public:
  /**
   * Constructor
   * @param clkPin - Clock/A pin (interrupt capable)
   * @param dtPin - Data/B pin (interrupt capable)
   * @param swPin - Switch/Button pin (interrupt capable)
   * @param stepsPerDetent - Steps per physical click (1, 2, or 4)
   */
  RotaryEncoder(int clkPin, int dtPin, int swPin, int stepsPerDetent = 4);

  /**
   * Initialize the encoder (must be called in setup())
   */
  void begin();

  /**
   * Update function - call this in loop() to process events
   * @return true if there's new data to process
   */
  bool update();

  /**
   * Get current position (in detents/clicks)
   */
  long getPosition();

  /**
   * Set position
   */
  void setPosition(long position);

  /**
   * Get position change since last read
   * @return number of detents moved (positive = CW, negative = CCW)
   */
  int getDelta();

  /**
   * Get rotation direction
   */
  EncoderDirection getDirection();

  /**
   * Check if button event occurred
   * @return Button event type
   */
  EncoderButton getButton();

  /**
   * Check if button is currently pressed
   */
  bool isPressed();

  /**
   * Configure long press threshold (milliseconds)
   * Default: 500ms
   */
  void setLongPressThreshold(unsigned long ms);

  /**
   * Configure double click window (milliseconds)
   * Default: 400ms
   */
  void setDoubleClickWindow(unsigned long ms);

  /**
   * Configure debounce time (milliseconds)
   * Default: 50ms
   */
  void setDebounceTime(unsigned long ms);

  /**
   * Enable/disable acceleration
   * When enabled, faster rotation = larger steps
   */
  void setAcceleration(bool enabled);

  /**
   * Get raw step count (before resolution division)
   */
  long getRawPosition();

  // Static ISR handlers (public so they can be attached)
  static void isrCLK();
  static void isrButton();

private:
  // Pin configuration
  int _clkPin;
  int _dtPin;
  int _swPin;

  // Rotation state
  volatile long _rawPosition;
  long _lastReportedPosition;
  int _stepsPerDetent;
  volatile int _lastCLK;

  // Button state
  volatile bool _buttonChanged;
  volatile unsigned long _buttonPressTime;
  volatile unsigned long _buttonReleaseTime;
  unsigned long _lastClickTime;
  bool _waitingForDoubleClick;
  EncoderButton _lastButtonEvent;

  // Configuration
  unsigned long _longPressThreshold;
  unsigned long _doubleClickWindow;
  unsigned long _debounceTime;
  bool _accelerationEnabled;

  // Direction tracking
  EncoderDirection _lastDirection;

  // Static instance pointer for ISR
  static RotaryEncoder* _instance;

  // Internal methods
  void processRotation();
  void processButton();
};

#endif // ROTARY_ENCODER_H
