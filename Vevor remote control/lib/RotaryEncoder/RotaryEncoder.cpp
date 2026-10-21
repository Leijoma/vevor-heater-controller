/**
 * RotaryEncoder Library - Implementation
 */

#include "RotaryEncoder.h"

// Static instance pointer
RotaryEncoder* RotaryEncoder::_instance = nullptr;

RotaryEncoder::RotaryEncoder(int clkPin, int dtPin, int swPin, int stepsPerDetent)
  : _clkPin(clkPin),
    _dtPin(dtPin),
    _swPin(swPin),
    _rawPosition(0),
    _lastReportedPosition(0),
    _stepsPerDetent(stepsPerDetent),
    _lastCLK(HIGH),
    _buttonChanged(false),
    _buttonPressTime(0),
    _buttonReleaseTime(0),
    _lastClickTime(0),
    _waitingForDoubleClick(false),
    _lastButtonEvent(BTN_NONE),
    _longPressThreshold(500),
    _doubleClickWindow(400),
    _debounceTime(50),
    _accelerationEnabled(false),
    _lastDirection(DIR_NONE)
{
  _instance = this;
}

void RotaryEncoder::begin() {
  // Configure pins
  pinMode(_clkPin, INPUT_PULLUP);
  pinMode(_dtPin, INPUT_PULLUP);
  pinMode(_swPin, INPUT_PULLUP);

  // Read initial state
  _lastCLK = digitalRead(_clkPin);

  // DISABLED: Interrupts disabled - using polling only for I2C compatibility
  // attachInterrupt(digitalPinToInterrupt(_clkPin), isrCLK, CHANGE);
  // attachInterrupt(digitalPinToInterrupt(_swPin), isrButton, CHANGE);
}

void RotaryEncoder::isrCLK() {
  if (_instance) {
    int clk = digitalRead(_instance->_clkPin);
    int dt = digitalRead(_instance->_dtPin);

    if (clk != _instance->_lastCLK) {
      if (dt != clk) {
        _instance->_rawPosition++;
      } else {
        _instance->_rawPosition--;
      }
      _instance->_lastCLK = clk;
    }
  }
}

void RotaryEncoder::isrButton() {
  if (_instance) {
    unsigned long now = millis();
    bool pressed = (digitalRead(_instance->_swPin) == LOW);

    if (pressed) {
      _instance->_buttonPressTime = now;
    } else {
      if (_instance->_buttonPressTime > 0 &&
          (now - _instance->_buttonPressTime) >= _instance->_debounceTime) {
        _instance->_buttonReleaseTime = now;
        _instance->_buttonChanged = true;
      }
    }
  }
}

bool RotaryEncoder::update() {
  bool hasUpdate = false;

  // POLL rotation state (no interrupts - polling mode for I2C compatibility)
  int clk = digitalRead(_clkPin);
  int dt = digitalRead(_dtPin);

  if (clk != _lastCLK) {
    if (dt != clk) {
      _rawPosition++;
    } else {
      _rawPosition--;
    }
    _lastCLK = clk;
  }

  // POLL button state (interrupt disabled due to spurious triggers)
  static bool lastButtonState = HIGH;
  static unsigned long lastButtonChange = 0;
  bool currentButtonState = digitalRead(_swPin);
  unsigned long now = millis();

  if (currentButtonState != lastButtonState && (now - lastButtonChange) > _debounceTime) {
    lastButtonState = currentButtonState;
    lastButtonChange = now;

    if (currentButtonState == LOW) {
      // Button pressed
      _buttonPressTime = now;
    } else {
      // Button released
      _buttonReleaseTime = now;
      _buttonChanged = true;
    }
  }

  // Process rotation
  if (_rawPosition != _lastReportedPosition * _stepsPerDetent) {
    processRotation();
    hasUpdate = true;
  }

  // Process button
  if (_buttonChanged) {
    processButton();
    hasUpdate = true;
  }

  // Check for double-click timeout
  if (_waitingForDoubleClick) {
    if (millis() - _lastClickTime > _doubleClickWindow) {
      _waitingForDoubleClick = false;
      // Only set short press if no event is pending
      if (_lastButtonEvent == BTN_NONE) {
        _lastButtonEvent = BTN_SHORT_PRESS;
        hasUpdate = true;
      }
    }
  }

  // Check for long press (button still held)
  if (isPressed()) {
    unsigned long pressDuration = millis() - _buttonPressTime;
    if (pressDuration >= _longPressThreshold && _lastButtonEvent == BTN_NONE) {
      _lastButtonEvent = BTN_LONG_PRESS;
      _waitingForDoubleClick = false;
      hasUpdate = true;
    }
  }

  return hasUpdate;
}

void RotaryEncoder::processRotation() {
  long newPosition = _rawPosition / _stepsPerDetent;

  if (newPosition != _lastReportedPosition) {
    _lastDirection = (newPosition > _lastReportedPosition) ? DIR_CW : DIR_CCW;
    _lastReportedPosition = newPosition;
  }
}

void RotaryEncoder::processButton() {
  _buttonChanged = false;

  unsigned long pressDuration = _buttonReleaseTime - _buttonPressTime;

  // Ignore very short presses (likely noise)
  if (pressDuration < _debounceTime) {
    _buttonPressTime = 0;
    return;
  }

  // Check if it's a long press
  if (pressDuration >= _longPressThreshold) {
    // Only set if not already set (prevents double-triggering)
    if (_lastButtonEvent == BTN_NONE) {
      _lastButtonEvent = BTN_LONG_PRESS;
    }
    _waitingForDoubleClick = false;
    _buttonPressTime = 0;  // Clear to prevent reprocessing
    return;
  }

  // Short press - check for double click
  unsigned long timeSinceLastClick = _buttonReleaseTime - _lastClickTime;

  if (_waitingForDoubleClick && timeSinceLastClick <= _doubleClickWindow) {
    // Double click detected!
    // Only set if no event is pending
    if (_lastButtonEvent == BTN_NONE) {
      _lastButtonEvent = BTN_DOUBLE_CLICK;
    }
    _waitingForDoubleClick = false;
    _lastClickTime = 0;  // Clear to prevent false triggers
  } else {
    // First click - wait for potential double click
    // Clear any stale short press events before waiting
    if (_lastButtonEvent == BTN_SHORT_PRESS) {
      _lastButtonEvent = BTN_NONE;
    }
    _waitingForDoubleClick = true;
    _lastClickTime = _buttonReleaseTime;
  }

  _buttonPressTime = 0;
}

long RotaryEncoder::getPosition() {
  return _lastReportedPosition;
}

void RotaryEncoder::setPosition(long position) {
  _rawPosition = position * _stepsPerDetent;
  _lastReportedPosition = position;
}

int RotaryEncoder::getDelta() {
  static long lastPosition = 0;
  int delta = _lastReportedPosition - lastPosition;
  lastPosition = _lastReportedPosition;
  return delta;
}

EncoderDirection RotaryEncoder::getDirection() {
  EncoderDirection dir = _lastDirection;
  _lastDirection = DIR_NONE;
  return dir;
}

EncoderButton RotaryEncoder::getButton() {
  EncoderButton event = _lastButtonEvent;
  _lastButtonEvent = BTN_NONE;
  return event;
}

bool RotaryEncoder::isPressed() {
  return (digitalRead(_swPin) == LOW);
}

void RotaryEncoder::setLongPressThreshold(unsigned long ms) {
  _longPressThreshold = ms;
}

void RotaryEncoder::setDoubleClickWindow(unsigned long ms) {
  _doubleClickWindow = ms;
}

void RotaryEncoder::setDebounceTime(unsigned long ms) {
  _debounceTime = ms;
}

void RotaryEncoder::setAcceleration(bool enabled) {
  _accelerationEnabled = enabled;
}

long RotaryEncoder::getRawPosition() {
  return _rawPosition;
}
