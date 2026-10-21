/**
 * GPIO Diagnostic Test - Detailed pin monitoring
 *
 * Monitors GPIO 32, 33, 26 with detailed logging to identify interference
 * Reports any state changes with timestamps and duration
 *
 * Hardware:
 * - OLED: SDA=GPIO22, SCL=GPIO21
 * - Encoder: CLK=33, DT=26, SW=32
 */

#include <Arduino.h>
#include <Wire.h>
#include <Adafruit_SH110X.h>

// ========== HARDWARE PINS ==========
#define I2C_SDA_PIN 22
#define I2C_SCL_PIN 21

#define GPIO_PIN_32  32  // Button/Switch
#define GPIO_PIN_33  33  // Encoder CLK
#define GPIO_PIN_26  26  // Encoder DT

// ========== DISPLAY ==========
#define OLED_WIDTH  128
#define OLED_HEIGHT 64
#define OLED_ADDR   0x3C
#define OLED_RESET  -1

Adafruit_SH1106G display(OLED_WIDTH, OLED_HEIGHT, &Wire, OLED_RESET);

// State tracking
int lastState32 = HIGH;
int lastState33 = HIGH;
int lastState26 = HIGH;

unsigned long lastChange32 = 0;
unsigned long lastChange33 = 0;
unsigned long lastChange26 = 0;

int changeCount32 = 0;
int changeCount33 = 0;
int changeCount26 = 0;

// Debounce time in microseconds
#define DEBOUNCE_US 1000

void setup() {
  Serial.begin(115200);
  delay(1000);
  Serial.println("\n=== GPIO Diagnostic Test ===");
  Serial.println("Monitoring for interference and state changes");
  Serial.println("Pin 32 = Button, Pins 33/26 = Encoder");
  Serial.println("================================================");

  // Configure pins with pull-ups (standard for encoders)
  pinMode(GPIO_PIN_32, INPUT_PULLUP);
  pinMode(GPIO_PIN_33, INPUT_PULLUP);
  pinMode(GPIO_PIN_26, INPUT_PULLUP);

  // Read initial states
  lastState32 = digitalRead(GPIO_PIN_32);
  lastState33 = digitalRead(GPIO_PIN_33);
  lastState26 = digitalRead(GPIO_PIN_26);

  Serial.printf("Initial states - 32:%d  33:%d  26:%d\n",
                lastState32, lastState33, lastState26);

  // Initialize I2C
  Wire.begin(I2C_SDA_PIN, I2C_SCL_PIN);

  // Initialize display
  if (!display.begin(OLED_ADDR, true)) {
    Serial.println("ERROR: OLED init failed!");
    while (1) delay(100);
  }

  display.clearDisplay();
  display.setTextColor(SH110X_WHITE);
  display.setTextSize(1);
  display.setCursor(0, 0);
  display.println("GPIO Diagnostic");
  display.println("Monitoring...");
  display.display();

  Serial.println("\nMonitoring started. Rotate encoder and press button.");
  Serial.println("Watch for simultaneous changes (indicates interference)");
  Serial.println();
}

void updateDisplay() {
  display.clearDisplay();
  display.setTextSize(1);
  display.setCursor(0, 0);
  display.println("GPIO States:");
  display.drawLine(0, 9, 127, 9, SH110X_WHITE);

  int y = 14;

  // Pin 32 - Button
  display.setCursor(0, y);
  display.print("SW32:");
  display.setTextSize(2);
  display.setCursor(50, y-2);
  display.print(lastState32);
  display.setTextSize(1);
  display.setCursor(80, y);
  display.print("n=");
  display.print(changeCount32);

  y += 16;

  // Pin 33 - Encoder CLK
  display.setCursor(0, y);
  display.print("CL33:");
  display.setTextSize(2);
  display.setCursor(50, y-2);
  display.print(lastState33);
  display.setTextSize(1);
  display.setCursor(80, y);
  display.print("n=");
  display.print(changeCount33);

  y += 16;

  // Pin 26 - Encoder DT
  display.setCursor(0, y);
  display.print("DT26:");
  display.setTextSize(2);
  display.setCursor(50, y-2);
  display.print(lastState26);
  display.setTextSize(1);
  display.setCursor(80, y);
  display.print("n=");
  display.print(changeCount26);

  display.display();
}

void loop() {
  unsigned long now = micros();
  bool displayUpdate = false;

  // Read current states
  int state32 = digitalRead(GPIO_PIN_32);
  int state33 = digitalRead(GPIO_PIN_33);
  int state26 = digitalRead(GPIO_PIN_26);

  // Check for state changes with debouncing
  if (state32 != lastState32 && (now - lastChange32) > DEBOUNCE_US) {
    unsigned long duration = now - lastChange32;
    lastState32 = state32;
    lastChange32 = now;
    changeCount32++;
    displayUpdate = true;

    Serial.printf("[%10lu] PIN 32 (SW): %d -> %d (duration: %lu us)\n",
                  now/1000, !state32, state32, duration);
  }

  if (state33 != lastState33 && (now - lastChange33) > DEBOUNCE_US) {
    unsigned long duration = now - lastChange33;
    lastState33 = state33;
    lastChange33 = now;
    changeCount33++;
    displayUpdate = true;

    Serial.printf("[%10lu] PIN 33 (CLK): %d -> %d (duration: %lu us)\n",
                  now/1000, !state33, state33, duration);
  }

  if (state26 != lastState26 && (now - lastChange26) > DEBOUNCE_US) {
    unsigned long duration = now - lastChange26;
    lastState26 = state26;
    lastChange26 = now;
    changeCount26++;
    displayUpdate = true;

    Serial.printf("[%10lu] PIN 26 (DT): %d -> %d (duration: %lu us)\n",
                  now/1000, !state26, state26, duration);
  }

  // Check for simultaneous changes (possible interference)
  unsigned long timeDiff_32_33 = abs((long)(lastChange32 - lastChange33));
  unsigned long timeDiff_32_26 = abs((long)(lastChange32 - lastChange26));

  if (timeDiff_32_33 < 5000 && changeCount32 > 0 && changeCount33 > 0) {
    Serial.println("*** WARNING: Pin 32 and 33 changed within 5ms - possible interference! ***");
  }

  if (timeDiff_32_26 < 5000 && changeCount32 > 0 && changeCount26 > 0) {
    Serial.println("*** WARNING: Pin 32 and 26 changed within 5ms - possible interference! ***");
  }

  if (displayUpdate) {
    updateDisplay();
  }

  // Small delay to reduce CPU usage
  delayMicroseconds(100);
}
