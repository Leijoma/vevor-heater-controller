#include <Arduino.h>
#include <Wire.h>
#include <Adafruit_SH110X.h>

// ========== ROTARY ENCODER PINS (USER CONFIGURATION) ==========
// User connected: pins 32, 33, 26 (order to be verified)
// Testing different pin mapping based on observed behavior:
// Button press was causing temp change, so button pin is likely on rotation pin
#define ENCODER_CLK 33    // GPIO33 - Encoder clock pin (A) - TESTING
#define ENCODER_DT  26    // GPIO26 - Encoder data pin (B) - TESTING
#define ENCODER_SW  32    // GPIO32 - Encoder switch/button pin - TESTING

// ========== LED PIN ==========
#define LED_PIN 18        // GPIO18 - Status LED

// ========== OLED DISPLAY PINS (CORRECTED - WIRES WERE SWAPPED) ==========
#define I2C_SDA_PIN 22    // GPIO22 - Your OLED SDA wire is connected here
#define I2C_SCL_PIN 21    // GPIO21 - Your OLED SCL wire is connected here
#define OLED_WIDTH  128
#define OLED_HEIGHT 64
#define OLED_ADDR   0x3C
#define OLED_RESET  -1

// ========== OBJECTS ==========
Adafruit_SH1106G display(OLED_WIDTH, OLED_HEIGHT, &Wire, OLED_RESET);

// ========== ENCODER VARIABLES ==========
volatile long encoderPosition = 0;
volatile bool encoderChanged = false;
volatile int lastCLK = HIGH;
volatile unsigned long lastEncoderTime = 0;

// ========== OTHER VARIABLES ==========
float temperature = 20.0;  // Starting temperature
long lastDisplayedPosition = 0;
bool buttonPressed = false;
unsigned long lastButtonPress = 0;
unsigned long lastLedToggle = 0;
bool ledState = false;
const unsigned long DEBOUNCE_MS = 200;
const unsigned long LED_BLINK_MS = 500;

// ========== INTERRUPT HANDLER FOR ENCODER ==========
void IRAM_ATTR encoderISR() {
  int clk = digitalRead(ENCODER_CLK);
  int dt = digitalRead(ENCODER_DT);

  if (clk != lastCLK) {
    if (dt != clk) {
      encoderPosition++;
    } else {
      encoderPosition--;
    }
    encoderChanged = true;
    lastCLK = clk;
    lastEncoderTime = millis();
  }
}

// ========== INTERRUPT HANDLER FOR BUTTON ==========
void IRAM_ATTR buttonISR() {
  unsigned long now = millis();
  if (now - lastButtonPress > DEBOUNCE_MS) {
    buttonPressed = true;
    lastButtonPress = now;
  }
}

// ========== UPDATE DISPLAY ==========
void updateDisplay() {
  display.clearDisplay();

  // Title
  display.setTextSize(1);
  display.setCursor(0, 0);
  display.print("UI Test - All OK!");

  // Large temperature number
  display.setTextSize(3);
  display.setCursor(15, 20);
  display.print(temperature, 1);
  display.print("C");

  // LED status indicator
  display.setTextSize(1);
  display.setCursor(0, 48);
  display.print("LED: ");
  display.print(ledState ? "ON " : "OFF");

  // Instructions
  display.setCursor(0, 56);
  display.print("Turn:+/- Press:Reset");

  display.display();
}

// ========== SETUP ==========
void setup() {
  Serial.begin(115200);
  delay(1000);

  Serial.println("\n========================================");
  Serial.println("  COMPREHENSIVE UI TEST");
  Serial.println("  OLED + LED + Rotary Encoder");
  Serial.println("========================================\n");

  // Initialize LED pin
  pinMode(LED_PIN, OUTPUT);
  digitalWrite(LED_PIN, LOW);
  Serial.print("✓ LED initialized on GPIO");
  Serial.println(LED_PIN);

  // Initialize I2C
  Wire.begin(I2C_SDA_PIN, I2C_SCL_PIN);
  Wire.setClock(100000);
  delay(100);

  // Initialize OLED
  Serial.println("Initializing OLED...");
  if (!display.begin(OLED_ADDR, true)) {
    Serial.println("ERROR: OLED initialization failed!");
    Serial.println("Check connections:");
    Serial.println("  SDA -> GPIO21");
    Serial.println("  SCL -> GPIO22");
    // Blink LED rapidly to indicate error
    while (1) {
      digitalWrite(LED_PIN, !digitalRead(LED_PIN));
      delay(100);
    }
  }
  Serial.println("✓ OLED initialized");

  display.setTextColor(SH110X_WHITE);
  display.clearDisplay();
  display.display();

  // Blink LED 3 times to confirm startup
  for (int i = 0; i < 3; i++) {
    digitalWrite(LED_PIN, HIGH);
    delay(200);
    digitalWrite(LED_PIN, LOW);
    delay(200);
  }

  // Initialize rotary encoder
  Serial.println("\nInitializing rotary encoder...");
  Serial.println("Pin configuration:");
  Serial.print("  CLK (A): GPIO");
  Serial.println(ENCODER_CLK);
  Serial.print("  DT  (B): GPIO");
  Serial.println(ENCODER_DT);
  Serial.print("  SW:      GPIO");
  Serial.println(ENCODER_SW);
  Serial.println("\nIf encoder direction is reversed:");
  Serial.println("  Swap ENCODER_CLK and ENCODER_DT in code");

  // Setup encoder pins with internal pull-ups
  pinMode(ENCODER_CLK, INPUT_PULLUP);
  pinMode(ENCODER_DT, INPUT_PULLUP);
  pinMode(ENCODER_SW, INPUT_PULLUP);

  // Read initial state
  lastCLK = digitalRead(ENCODER_CLK);

  // Attach interrupts for encoder rotation (both edges on CLK pin)
  attachInterrupt(digitalPinToInterrupt(ENCODER_CLK), encoderISR, CHANGE);

  // Attach interrupt for button press
  attachInterrupt(digitalPinToInterrupt(ENCODER_SW), buttonISR, FALLING);

  Serial.println("✓ Encoder initialized");
  Serial.println("\n========================================");
  Serial.println("TEST INSTRUCTIONS:");
  Serial.println("1. LED should blink every 500ms");
  Serial.println("2. OLED should show temperature");
  Serial.println("3. Rotate encoder -> temp changes");
  Serial.println("4. Press button -> reset to 20.0°C");
  Serial.println("========================================\n");
  Serial.println("Monitoring encoder activity...\n");

  updateDisplay();
}

// ========== MAIN LOOP ==========
void loop() {
  unsigned long now = millis();

  // Blink LED periodically
  if (now - lastLedToggle >= LED_BLINK_MS) {
    ledState = !ledState;
    digitalWrite(LED_PIN, ledState);
    lastLedToggle = now;
    updateDisplay();  // Update LED status on display
  }

  // Check for encoder rotation
  if (encoderChanged) {
    encoderChanged = false;

    // Calculate temperature change (0.5°C per click)
    long delta = encoderPosition - lastDisplayedPosition;
    temperature += delta * 0.5;

    // Limit temperature range
    if (temperature < -40.0) temperature = -40.0;
    if (temperature > 125.0) temperature = 125.0;

    // Flash LED on encoder activity
    digitalWrite(LED_PIN, HIGH);
    ledState = true;
    lastLedToggle = now;  // Reset blink timer

    Serial.print("Encoder: ");
    Serial.print(delta > 0 ? "CW " : "CCW");
    Serial.print(" | Position: ");
    Serial.print(encoderPosition);
    Serial.print(" | Temperature: ");
    Serial.print(temperature, 1);
    Serial.println("°C");

    lastDisplayedPosition = encoderPosition;
    updateDisplay();
  }

  // Check for button press
  if (buttonPressed) {
    buttonPressed = false;

    Serial.println("\n*** BUTTON PRESSED - RESET TO 20.0°C ***\n");

    // Flash LED 3 times on button press
    for (int i = 0; i < 3; i++) {
      digitalWrite(LED_PIN, HIGH);
      delay(100);
      digitalWrite(LED_PIN, LOW);
      delay(100);
    }

    temperature = 20.0;
    encoderPosition = 0;
    lastDisplayedPosition = 0;

    updateDisplay();
  }

  delay(10);
}
