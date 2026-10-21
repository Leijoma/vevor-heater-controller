#include <Arduino.h>

// Test all three encoder pins
#define PIN_A 26
#define PIN_B 32
#define PIN_C 33

volatile bool pin_a_changed = false;
volatile bool pin_b_changed = false;
volatile bool pin_c_changed = false;

volatile int pin_a_count = 0;
volatile int pin_b_count = 0;
volatile int pin_c_count = 0;

void IRAM_ATTR isrPinA() {
  pin_a_changed = true;
  pin_a_count++;
}

void IRAM_ATTR isrPinB() {
  pin_b_changed = true;
  pin_b_count++;
}

void IRAM_ATTR isrPinC() {
  pin_c_changed = true;
  pin_c_count++;
}

void setup() {
  Serial.begin(115200);
  delay(2000);

  Serial.println("\n\n");
  Serial.println("╔════════════════════════════════════════╗");
  Serial.println("║   ENCODER PIN IDENTIFICATION TEST      ║");
  Serial.println("╔════════════════════════════════════════╗");
  Serial.println();
  Serial.println("This test monitors pins 26, 32, and 33");
  Serial.println("to identify which one does what.");
  Serial.println();
  Serial.println("INSTRUCTIONS:");
  Serial.println("-------------");
  Serial.println("1. First, ROTATE the encoder a few clicks");
  Serial.println("   (you'll see which pins trigger)");
  Serial.println();
  Serial.println("2. Then, PRESS the encoder button");
  Serial.println("   (you'll see which pin triggers)");
  Serial.println();
  Serial.println("Starting monitor...");
  Serial.println();

  // Setup all pins with pull-ups
  pinMode(PIN_A, INPUT_PULLUP);
  pinMode(PIN_B, INPUT_PULLUP);
  pinMode(PIN_C, INPUT_PULLUP);

  // Attach interrupts to all pins
  attachInterrupt(digitalPinToInterrupt(PIN_A), isrPinA, CHANGE);
  attachInterrupt(digitalPinToInterrupt(PIN_B), isrPinB, CHANGE);
  attachInterrupt(digitalPinToInterrupt(PIN_C), isrPinC, CHANGE);

  Serial.println("Monitoring all pins... (waiting for input)");
  Serial.println();
}

void loop() {
  static unsigned long lastReport = 0;
  static int last_a = 0, last_b = 0, last_c = 0;

  // Report immediately when any pin changes
  if (pin_a_changed) {
    pin_a_changed = false;
    Serial.print(">>> GPIO26 changed! State: ");
    Serial.print(digitalRead(PIN_A) ? "HIGH" : "LOW");
    Serial.print(" | Total: ");
    Serial.println(pin_a_count);
  }

  if (pin_b_changed) {
    pin_b_changed = false;
    Serial.print(">>> GPIO32 changed! State: ");
    Serial.print(digitalRead(PIN_B) ? "HIGH" : "LOW");
    Serial.print(" | Total: ");
    Serial.println(pin_b_count);
  }

  if (pin_c_changed) {
    pin_c_changed = false;
    Serial.print(">>> GPIO33 changed! State: ");
    Serial.print(digitalRead(PIN_C) ? "HIGH" : "LOW");
    Serial.print(" | Total: ");
    Serial.println(pin_c_count);
  }

  // Periodic summary every 5 seconds
  if (millis() - lastReport > 5000) {
    lastReport = millis();

    if (pin_a_count > 0 || pin_b_count > 0 || pin_c_count > 0) {
      Serial.println();
      Serial.println("--- 5-Second Summary ---");
      Serial.print("GPIO26: ");
      Serial.print(pin_a_count - last_a);
      Serial.println(" changes");
      Serial.print("GPIO32: ");
      Serial.print(pin_b_count - last_b);
      Serial.println(" changes");
      Serial.print("GPIO33: ");
      Serial.print(pin_c_count - last_c);
      Serial.println(" changes");
      Serial.println();

      last_a = pin_a_count;
      last_b = pin_b_count;
      last_c = pin_c_count;

      // Give guidance
      if (pin_a_count == 0 && pin_b_count == 0 && pin_c_count == 0) {
        Serial.println("No activity detected. Try rotating or pressing the encoder.");
      } else {
        Serial.println("ANALYSIS:");
        Serial.println("---------");
        Serial.println("• Rotation pins change MULTIPLE times per click");
        Serial.println("• Button pin changes ONCE when pressed (then once when released)");
        Serial.println();
      }
    }
  }

  delay(10);
}
