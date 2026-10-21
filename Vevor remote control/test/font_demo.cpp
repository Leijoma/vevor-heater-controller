/**
 * Font Demonstration Program
 *
 * Shows different font sizes and styles available in DisplayFonts library
 * Use rotary encoder to cycle through different font examples
 */

#include <Arduino.h>
#include <Wire.h>
#include <Adafruit_SH110X.h>
#include <DisplayFonts.h>
#include <RotaryEncoder.h>

// ========== HARDWARE PINS ==========
#define ENCODER_CLK 33
#define ENCODER_DT  26
#define ENCODER_SW  32
#define I2C_SDA_PIN 22
#define I2C_SCL_PIN 21

// ========== OLED DISPLAY ==========
#define OLED_WIDTH  128
#define OLED_HEIGHT 64
#define OLED_ADDR   0x3C
#define OLED_RESET  -1

Adafruit_SH1106G display(OLED_WIDTH, OLED_HEIGHT, &Wire, OLED_RESET);
DisplayFonts fonts(&display);
RotaryEncoder encoder(ENCODER_CLK, ENCODER_DT, ENCODER_SW, 4);

// ========== DEMO STATE ==========
int currentDemo = 0;
const int NUM_DEMOS = 12;
float testValue = 42.7;

// ========== DEMO SCREENS ==========

void showDemo0() {
  // Default font comparison
  display.clearDisplay();

  fonts.setDefaultFont();
  display.setCursor(0, 0);
  display.print("Default Font:");
  display.setCursor(0, 10);
  display.print("Size 1: ABC 123");
  display.setTextSize(2);
  display.setCursor(0, 20);
  display.print("Size 2");
  display.setTextSize(1);

  fonts.setFont(FONT_SMALL, FONT_SANS);
  fonts.printAligned("FreeSans 12pt", 0, 55, ALIGN_LEFT);

  display.display();
}

void showDemo1() {
  // Sans font sizes
  display.clearDisplay();

  fonts.setFont(FONT_TINY, FONT_SANS);
  fonts.printAligned("Sans 9pt", 64, 12, ALIGN_CENTER);

  fonts.setFont(FONT_SMALL, FONT_SANS);
  fonts.printAligned("Sans 12pt", 64, 28, ALIGN_CENTER);

  fonts.setFont(FONT_MEDIUM, FONT_SANS);
  fonts.printAligned("Sans 18pt", 64, 50, ALIGN_CENTER);

  display.display();
}

void showDemo2() {
  // Large font display
  display.clearDisplay();

  fonts.setFont(FONT_LARGE, FONT_SANS_BOLD);
  fonts.printCentered("BIG!", 0, 0, 128, 64);

  display.display();
}

void showDemo3() {
  // Bold fonts
  display.clearDisplay();

  fonts.setFont(FONT_TINY, FONT_SANS_BOLD);
  fonts.printAligned("Bold 9pt", 64, 12, ALIGN_CENTER);

  fonts.setFont(FONT_SMALL, FONT_SANS_BOLD);
  fonts.printAligned("Bold 12pt", 64, 28, ALIGN_CENTER);

  fonts.setFont(FONT_MEDIUM, FONT_SANS_BOLD);
  fonts.printAligned("Bold 18pt", 64, 50, ALIGN_CENTER);

  display.display();
}

void showDemo4() {
  // Serif fonts
  display.clearDisplay();

  fonts.setFont(FONT_TINY, FONT_SERIF);
  fonts.printAligned("Serif 9pt", 64, 12, ALIGN_CENTER);

  fonts.setFont(FONT_SMALL, FONT_SERIF);
  fonts.printAligned("Serif 12pt", 64, 28, ALIGN_CENTER);

  fonts.setFont(FONT_MEDIUM, FONT_SERIF);
  fonts.printAligned("Serif 18pt", 64, 50, ALIGN_CENTER);

  display.display();
}

void showDemo5() {
  // Monospace fonts
  display.clearDisplay();

  fonts.setFont(FONT_TINY, FONT_MONO);
  fonts.printAligned("Mono 9pt", 64, 12, ALIGN_CENTER);

  fonts.setFont(FONT_SMALL, FONT_MONO);
  fonts.printAligned("Mono 12pt", 64, 28, ALIGN_CENTER);

  fonts.setFont(FONT_MEDIUM, FONT_MONO);
  fonts.printAligned("Mono 18pt", 64, 50, ALIGN_CENTER);

  display.display();
}

void showDemo6() {
  // Number display - no decimals
  display.clearDisplay();

  fonts.setDefaultFont();
  display.setCursor(0, 0);
  display.print("Integer:");

  fonts.setFont(FONT_LARGE, FONT_SANS_BOLD);
  fonts.printCentered("42", 0, 10, 128, 54);

  display.display();
}

void showDemo7() {
  // Number display - 1 decimal
  display.clearDisplay();

  fonts.setDefaultFont();
  display.setCursor(0, 0);
  display.print("1 Decimal:");

  fonts.setFont(FONT_LARGE, FONT_SANS_BOLD);
  char buffer[16];
  snprintf(buffer, sizeof(buffer), "%.1f", testValue);
  fonts.printCentered(buffer, 0, 10, 128, 54);

  display.display();
}

void showDemo8() {
  // Number display - 2 decimals
  display.clearDisplay();

  fonts.setDefaultFont();
  display.setCursor(0, 0);
  display.print("2 Decimals:");

  fonts.setFont(FONT_LARGE, FONT_SANS_BOLD);
  char buffer[16];
  snprintf(buffer, sizeof(buffer), "%.2f", testValue);
  fonts.printCentered(buffer, 0, 10, 128, 54);

  display.display();
}

void showDemo9() {
  // Text alignment - left
  display.clearDisplay();

  fonts.setFont(FONT_SMALL, FONT_SANS);
  fonts.printAligned("Left Aligned", 0, 20, ALIGN_LEFT);
  fonts.printAligned("Text Example", 0, 35, ALIGN_LEFT);

  // Show alignment line
  display.drawLine(0, 0, 0, 64, SH110X_WHITE);

  display.display();
}

void showDemo10() {
  // Text alignment - center
  display.clearDisplay();

  fonts.setFont(FONT_SMALL, FONT_SANS);
  fonts.printAligned("Center Aligned", 64, 20, ALIGN_CENTER);
  fonts.printAligned("Text Example", 64, 35, ALIGN_CENTER);

  // Show alignment line
  display.drawLine(64, 0, 64, 64, SH110X_WHITE);

  display.display();
}

void showDemo11() {
  // Text alignment - right
  display.clearDisplay();

  fonts.setFont(FONT_SMALL, FONT_SANS);
  fonts.printAligned("Right Aligned", 128, 20, ALIGN_RIGHT);
  fonts.printAligned("Text Example", 128, 35, ALIGN_RIGHT);

  // Show alignment line
  display.drawLine(127, 0, 127, 64, SH110X_WHITE);

  display.display();
}

void showCurrentDemo() {
  switch (currentDemo) {
    case 0: showDemo0(); break;
    case 1: showDemo1(); break;
    case 2: showDemo2(); break;
    case 3: showDemo3(); break;
    case 4: showDemo4(); break;
    case 5: showDemo5(); break;
    case 6: showDemo6(); break;
    case 7: showDemo7(); break;
    case 8: showDemo8(); break;
    case 9: showDemo9(); break;
    case 10: showDemo10(); break;
    case 11: showDemo11(); break;
  }
}

// ========== SETUP ==========
void setup() {
  Serial.begin(115200);
  delay(1000);

  Serial.println("\n========================================");
  Serial.println("  Font Demonstration");
  Serial.println("========================================\n");

  // Initialize I2C and OLED
  Wire.begin(I2C_SDA_PIN, I2C_SCL_PIN);
  Wire.setClock(100000);
  delay(100);

  if (!display.begin(OLED_ADDR, true)) {
    Serial.println("ERROR: OLED initialization failed!");
    while (1) delay(100);
  }
  display.setTextColor(SH110X_WHITE);
  Serial.println("✓ OLED initialized");

  // Initialize encoder
  encoder.begin();
  Serial.println("✓ Encoder initialized");

  Serial.println("\n========================================");
  Serial.println("CONTROLS:");
  Serial.println("  Rotate: Change demo screen");
  Serial.println("  Press:  Change test value");
  Serial.println("========================================\n");
  Serial.println("Total demos: " + String(NUM_DEMOS));
  Serial.println();

  showCurrentDemo();
}

// ========== MAIN LOOP ==========
void loop() {
  if (encoder.update()) {

    // Check for rotation
    int delta = encoder.getDelta();
    if (delta != 0) {
      currentDemo += delta;

      // Wrap around
      if (currentDemo < 0) currentDemo = NUM_DEMOS - 1;
      if (currentDemo >= NUM_DEMOS) currentDemo = 0;

      Serial.print("Demo ");
      Serial.print(currentDemo + 1);
      Serial.print("/");
      Serial.println(NUM_DEMOS);

      showCurrentDemo();
    }

    // Check for button press
    EncoderButton btn = encoder.getButton();
    if (btn == BTN_SHORT_PRESS) {
      testValue += 1.1;
      if (testValue > 99.9) testValue = 0.0;

      Serial.print("Test value: ");
      Serial.println(testValue, 2);

      showCurrentDemo();
    }
  }

  delay(10);
}
