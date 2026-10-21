#include <Arduino.h>
#include <Wire.h>
#include <Adafruit_SH110X.h>

// ========== ROTARY ENCODER PINS ==========
#define ENCODER_CLK 25    // GPIO25 - Encoder clock pin (A)
#define ENCODER_DT  26    // GPIO26 - Encoder data pin (B)
#define ENCODER_SW  27    // GPIO27 - Encoder switch/button pin

// ========== OLED DISPLAY PINS ==========
#define I2C_SDA_PIN 21
#define I2C_SCL_PIN 22
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

// ========== OTHER VARIABLES ==========
float temperature = 20.0;  // Starting temperature
long lastDisplayedPosition = 0;
bool buttonPressed = false;
unsigned long lastButtonPress = 0;
const unsigned long DEBOUNCE_MS = 200;

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
  display.print("Rotary Encoder Test");

  // Large temperature number
  display.setTextSize(4);
  display.setCursor(10, 25);
  display.print(temperature, 1);
  display.print("C");

  // Instructions
  display.setTextSize(1);
  display.setCursor(0, 56);
  display.print("Turn=Temp Press=Reset");

  display.display();
}

// ========== SETUP ==========
void setup() {
  Serial.begin(115200);
  delay(1000);

  Serial.println("\n========================================");
  Serial.println("  Rotary Encoder Test with OLED");
  Serial.println("========================================\n");

  // Initialize I2C
  Wire.begin(I2C_SDA_PIN, I2C_SCL_PIN);
  Wire.setClock(100000);
  delay(100);

  // Initialize OLED
  Serial.println("Initializing OLED...");
  if (!display.begin(OLED_ADDR, true)) {
    Serial.println("ERROR: OLED initialization failed!");
    while (1) delay(100);
  }
  Serial.println("✓ OLED initialized");

  display.setTextColor(SH110X_WHITE);
  display.clearDisplay();
  display.display();

  // Initialize rotary encoder
  Serial.println("Initializing encoder...");
  Serial.print("  CLK (A): GPIO");
  Serial.println(ENCODER_CLK);
  Serial.print("  DT  (B): GPIO");
  Serial.println(ENCODER_DT);
  Serial.print("  SW:      GPIO");
  Serial.println(ENCODER_SW);

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
  Serial.println("Rotate encoder to change temperature");
  Serial.println("Press button to reset to 20.0°C");
  Serial.println("========================================\n");

  updateDisplay();
}

// ========== MAIN LOOP ==========
void loop() {
  // Check for encoder rotation
  if (encoderChanged) {
    encoderChanged = false;

    // Calculate temperature change (0.5°C per click)
    long delta = encoderPosition - lastDisplayedPosition;
    temperature += delta * 0.5;

    // Limit temperature range
    if (temperature < -40.0) temperature = -40.0;
    if (temperature > 125.0) temperature = 125.0;

    Serial.print("Encoder: ");
    Serial.print(encoderPosition);
    Serial.print("  Temperature: ");
    Serial.print(temperature, 1);
    Serial.println("°C");

    lastDisplayedPosition = encoderPosition;
    updateDisplay();
  }

  // Check for button press
  if (buttonPressed) {
    buttonPressed = false;

    Serial.println("\n*** BUTTON PRESSED - RESET TO 20.0°C ***\n");

    temperature = 20.0;
    encoderPosition = 0;
    lastDisplayedPosition = 0;

    updateDisplay();
  }

  delay(10);
}
