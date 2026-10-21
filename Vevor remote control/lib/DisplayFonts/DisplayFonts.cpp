/**
 * DisplayFonts Library - Implementation
 */

#include "DisplayFonts.h"
#include <stdio.h>

// Font lookup tables
const GFXfont *DisplayFonts::_sansFonts[4] = {
  &FreeSans9pt7b,
  &FreeSans12pt7b,
  &FreeSans18pt7b,
  &FreeSans24pt7b
};

const GFXfont *DisplayFonts::_sansBoldFonts[4] = {
  &FreeSansBold9pt7b,
  &FreeSansBold12pt7b,
  &FreeSansBold18pt7b,
  &FreeSansBold24pt7b
};

const GFXfont *DisplayFonts::_serifFonts[4] = {
  &FreeSerif9pt7b,
  &FreeSerif12pt7b,
  &FreeSerif18pt7b,
  &FreeSerif24pt7b
};

const GFXfont *DisplayFonts::_monoFonts[4] = {
  &FreeMono9pt7b,
  &FreeMono12pt7b,
  &FreeMono18pt7b,
  &FreeMono24pt7b
};

DisplayFonts::DisplayFonts(Adafruit_GFX *display)
  : _display(display)
{
}

void DisplayFonts::setFont(FontSize size, FontStyle style) {
  const GFXfont *font = nullptr;

  switch (style) {
    case FONT_SANS:
      font = _sansFonts[size];
      break;
    case FONT_SANS_BOLD:
      font = _sansBoldFonts[size];
      break;
    case FONT_SERIF:
      font = _serifFonts[size];
      break;
    case FONT_MONO:
      font = _monoFonts[size];
      break;
  }

  if (font) {
    _display->setFont(font);
  }
}

void DisplayFonts::setDefaultFont() {
  _display->setFont();  // nullptr = default font
}

void DisplayFonts::getTextBounds(const char *text, int16_t x, int16_t y,
                                 int16_t *w, int16_t *h) {
  int16_t x1, y1;
  uint16_t width, height;
  _display->getTextBounds(text, x, y, &x1, &y1, &width, &height);
  *w = width;
  *h = height;
}

void DisplayFonts::printAligned(const char *text, int16_t x, int16_t y, TextAlign align) {
  int16_t width, height;
  getTextBounds(text, x, y, &width, &height);

  int16_t printX = x;

  switch (align) {
    case ALIGN_LEFT:
      // x is already correct
      break;

    case ALIGN_CENTER:
      printX = x - (width / 2);
      break;

    case ALIGN_RIGHT:
      printX = x - width;
      break;
  }

  _display->setCursor(printX, y);
  _display->print(text);
}

void DisplayFonts::printCentered(const char *text, int16_t boxX, int16_t boxY,
                                 int16_t boxW, int16_t boxH) {
  int16_t width, height;
  getTextBounds(text, 0, 0, &width, &height);

  int16_t x = boxX + (boxW - width) / 2;
  int16_t y = boxY + (boxH + height) / 2;  // Baseline adjustment for FreeFonts

  _display->setCursor(x, y);
  _display->print(text);
}

void DisplayFonts::printNumber(float value, int16_t x, int16_t y, int decimals) {
  char buffer[32];

  if (decimals == 0) {
    snprintf(buffer, sizeof(buffer), "%d", (int)value);
  } else {
    // Create format string like "%.2f" for 2 decimals
    char format[16];
    snprintf(format, sizeof(format), "%%.%df", decimals);
    snprintf(buffer, sizeof(buffer), format, value);
  }

  _display->setCursor(x, y);
  _display->print(buffer);
}

int16_t DisplayFonts::getFontHeight() {
  int16_t x1, y1;
  uint16_t w, h;
  _display->getTextBounds("Ay", 0, 0, &x1, &y1, &w, &h);
  return h;
}

int16_t DisplayFonts::getTextWidth(const char *text) {
  int16_t width, height;
  getTextBounds(text, 0, 0, &width, &height);
  return width;
}
