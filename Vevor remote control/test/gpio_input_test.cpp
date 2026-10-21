/**
 * GPIO Input Test Program
 *
 * Displays the digital state of GPIO pins 32, 33, and 26 on OLED display
 * Shows 0 for LOW, 1 for HIGH
 *
 * Hardware:
 * - OLED: SDA=GPIO22, SCL=GPIO21
 * - Inputs: GPIO32, GPIO33, GPIO26 (configured with internal pull-ups)
 */

#include <Arduino.h>
#include <Wire.h>
#include <Adafruit_SH110X.h>

// ========== HARDWARE PINS ==========
#define I2C_SDA_PIN 22
#define I2C_SCL_PIN 21

// GPIO pins to monitor
#define GPIO_PIN_32  32
#define GPIO_PIN_33  33
#define GPIO_PIN_26  26

// ========== DISPLAY ==========
#define OLED_WIDTH  128
#define OLED_HEIGHT 64
#define OLED_ADDR   0x3C
#define OLED_RESET  -1

Adafruit_SH1106G display(OLED_WIDTH, OLED_HEIGHT, &Wire, OLED_RESET);

void setup() {
  Serial.begin(115200);
  delay(1000);
  Serial.println("\n=== GPIO Input Test ===");

  // Configure input pins with internal pull-ups
  pinMode(GPIO_PIN_32, INPUT_PULLUP);
  pinMode(GPIO_PIN_33, INPUT_PULLUP);
  pinMode(GPIO_PIN_26, INPUT_PULLUP);

  Serial.println("GPIO pins configured as inputs with pull-ups");

  // Initialize I2C with swapped pins
  Wire.begin(I2C_SDA_PIN, I2C_SCL_PIN);
  Serial.println("I2C initialized (SDA=22, SCL=21)");

  // Initialize display
  if (!display.begin(OLED_ADDR, true)) {
    Serial.println("ERROR: OLED init failed!");
    while (1) delay(100);
  }

  Serial.println("OLED initialized successfully");

  display.clearDisplay();
  display.setTextColor(SH110X_WHITE);
  display.setTextSize(1);
  display.setCursor(0, 0);
  display.println("GPIO Input Test");
  display.display();
  delay(2000);

  Serial.println("Starting GPIO monitoring...");
}

void loop() {
  // Read GPIO states
  int state32 = digitalRead(GPIO_PIN_32);
  int state33 = digitalRead(GPIO_PIN_33);
  int state26 = digitalRead(GPIO_PIN_26);

  // Print to serial
  Serial.printf("GPIO32=%d  GPIO33=%d  GPIO26=%d\n", state32, state33, state26);

  // Clear display
  display.clearDisplay();

  // Display header
  display.setTextSize(1);
  display.setCursor(0, 0);
  display.println("GPIO Input States:");

  // Draw separator line
  display.drawLine(0, 10, 127, 10, SH110X_WHITE);

  // Display pin labels and values in large text
  int yPos = 20;
  int xLabel = 5;
  int xValue = 85;

  // GPIO 32
  display.setTextSize(1);
  display.setCursor(xLabel, yPos);
  display.print("GPIO 32:");
  display.setTextSize(2);
  display.setCursor(xValue, yPos - 2);
  display.print(state32);

  yPos += 16;

  // GPIO 33
  display.setTextSize(1);
  display.setCursor(xLabel, yPos);
  display.print("GPIO 33:");
  display.setTextSize(2);
  display.setCursor(xValue, yPos - 2);
  display.print(state33);

  yPos += 16;

  // GPIO 26
  display.setTextSize(1);
  display.setCursor(xLabel, yPos);
  display.print("GPIO 26:");
  display.setTextSize(2);
  display.setCursor(xValue, yPos - 2);
  display.print(state26);

  // Update display
  display.display();

  // Update rate
  delay(100);
}
