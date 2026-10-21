/**
 * Simple Vevor Heater Control Example
 *
 * This example shows basic usage of the VevorHeater library
 */

#include <VevorHeater.h>

// Pin definitions
#define HEATER_RX_PIN 16  // GPIO16
#define HEATER_TX_PIN 17  // GPIO17

// Create heater object
VevorHeater heater(Serial2, HEATER_RX_PIN, HEATER_TX_PIN);

// Last known state
HeaterState lastState = STATE_UNKNOWN;

void setup() {
  Serial.begin(115200);
  delay(2000);

  Serial.println("Vevor Heater Control Example");
  Serial.println("=============================\n");

  // Initialize heater communication
  heater.begin();

  // Register callback for state changes
  heater.onStateChange([](HeaterState oldState, HeaterState newState) {
    Serial.print("State changed: ");
    Serial.print(VevorHeater::getStateString(oldState));
    Serial.print(" -> ");
    Serial.println(VevorHeater::getStateString(newState));
  });

  // Register callback for errors
  heater.onError([](HeaterError error) {
    Serial.print("ERROR: ");
    Serial.println(VevorHeater::getErrorString(error));
  });

  Serial.println("Commands:");
  Serial.println("  s  - Start heater at power 5");
  Serial.println("  x  - Stop heater");
  Serial.println("  +  - Increase power");
  Serial.println("  -  - Decrease power");
  Serial.println("  ?  - Show status\n");
}

void loop() {
  // Update heater communication (call frequently!)
  heater.update();

  // Handle user commands
  if (Serial.available()) {
    char cmd = Serial.read();

    switch (cmd) {
      case 's':
        Serial.println("Starting heater at power 5...");
        heater.start(5);
        break;

      case 'x':
        Serial.println("Stopping heater...");
        heater.stop();
        break;

      case '+':
        {
          uint8_t power = heater.getPower();
          if (power < 10) {
            power++;
            heater.setPower(power);
            Serial.print("Power increased to ");
            Serial.println(power);
          } else {
            Serial.println("Already at max power");
          }
        }
        break;

      case '-':
        {
          uint8_t power = heater.getPower();
          if (power > 1) {
            power--;
            heater.setPower(power);
            Serial.print("Power decreased to ");
            Serial.println(power);
          } else {
            Serial.println("Already at min power");
          }
        }
        break;

      case '?':
        printStatus();
        break;
    }
  }

  // Print status updates every 5 seconds
  static unsigned long lastPrint = 0;
  if (millis() - lastPrint >= 5000) {
    HeaterState currentState = heater.getState();
    if (currentState != STATE_OFF && currentState != STATE_UNKNOWN) {
      printCompactStatus();
    }
    lastPrint = millis();
  }
}

void printStatus() {
  Serial.println("\n=== Heater Status ===");
  Serial.print("State: ");
  Serial.println(VevorHeater::getStateString(heater.getState()));
  Serial.print("Power: ");
  Serial.print(heater.getPower());
  Serial.println("/10");
  Serial.print("Temperature: ");
  Serial.print(heater.getTemperature(), 1);
  Serial.println(" °C");
  Serial.print("Fan: ");
  Serial.print(heater.getFanSpeed());
  Serial.println(" RPM");
  Serial.print("Voltage: ");
  Serial.print(heater.getVoltage(), 1);
  Serial.println(" V");

  if (heater.hasError()) {
    Serial.print("ERROR: ");
    Serial.println(VevorHeater::getErrorString(heater.getError()));
  }

  Serial.println("====================\n");
}

void printCompactStatus() {
  Serial.print("[");
  Serial.print(VevorHeater::getStateString(heater.getState()));
  Serial.print(" T:");
  Serial.print(heater.getTemperature(), 0);
  Serial.print("C F:");
  Serial.print(heater.getFanSpeed());
  Serial.print(" P:");
  Serial.print(heater.getPower());
  Serial.println("]");
}
