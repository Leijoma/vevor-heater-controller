/**
 * DisplayFonts Library
 *
 * Provides easy access to high-quality fonts in multiple sizes
 * with helper functions for common display tasks.
 *
 * Includes FreeFonts for better text rendering on OLED displays
 */

#ifndef DISPLAY_FONTS_H
#define DISPLAY_FONTS_H

#include <Adafruit_GFX.h>

// FreeFonts - better quality than default bitmap fonts
#include <Fonts/FreeSans9pt7b.h>
#include <Fonts/FreeSans12pt7b.h>
#include <Fonts/FreeSans18pt7b.h>
#include <Fonts/FreeSans24pt7b.h>

#include <Fonts/FreeSansBold9pt7b.h>
#include <Fonts/FreeSansBold12pt7b.h>
#include <Fonts/FreeSansBold18pt7b.h>
#include <Fonts/FreeSansBold24pt7b.h>

#include <Fonts/FreeSerif9pt7b.h>
#include <Fonts/FreeSerif12pt7b.h>
#include <Fonts/FreeSerif18pt7b.h>
#include <Fonts/FreeSerif24pt7b.h>

#include <Fonts/FreeMono9pt7b.h>
#include <Fonts/FreeMono12pt7b.h>
#include <Fonts/FreeMono18pt7b.h>
#include <Fonts/FreeMono24pt7b.h>

// Font size enum for easy selection
enum FontSize {
  FONT_TINY = 0,      // 9pt
  FONT_SMALL = 1,     // 12pt
  FONT_MEDIUM = 2,    // 18pt
  FONT_LARGE = 3      // 24pt
};

// Font style enum
enum FontStyle {
  FONT_SANS = 0,
  FONT_SANS_BOLD = 1,
  FONT_SERIF = 2,
  FONT_MONO = 3
};

// Text alignment
enum TextAlign {
  ALIGN_LEFT = 0,
  ALIGN_CENTER = 1,
  ALIGN_RIGHT = 2
};

class DisplayFonts {
public:
  /**
   * Constructor
   * @param display - pointer to Adafruit_GFX display object
   */
  DisplayFonts(Adafruit_GFX *display);

  /**
   * Set font by size and style
   * @param size - Font size (FONT_TINY, FONT_SMALL, FONT_MEDIUM, FONT_LARGE)
   * @param style - Font style (FONT_SANS, FONT_SANS_BOLD, FONT_SERIF, FONT_MONO)
   */
  void setFont(FontSize size, FontStyle style = FONT_SANS);

  /**
   * Reset to default system font (classic 5x7 bitmap)
   */
  void setDefaultFont();

  /**
   * Get text bounds (width and height)
   * @param text - Text to measure
   * @param x - X position
   * @param y - Y position (baseline for FreeFonts)
   * @param w - Output: width
   * @param h - Output: height
   */
  void getTextBounds(const char *text, int16_t x, int16_t y,
                     int16_t *w, int16_t *h);

  /**
   * Print text with alignment
   * @param text - Text to print
   * @param x - X position (meaning depends on alignment)
   * @param y - Y position (baseline)
   * @param align - Text alignment
   */
  void printAligned(const char *text, int16_t x, int16_t y, TextAlign align = ALIGN_LEFT);

  /**
   * Print text centered in a box
   * @param text - Text to print
   * @param boxX - Box X position
   * @param boxY - Box Y position
   * @param boxW - Box width
   * @param boxH - Box height
   */
  void printCentered(const char *text, int16_t boxX, int16_t boxY,
                     int16_t boxW, int16_t boxH);

  /**
   * Print large number (optimized for displaying values)
   * @param value - Number to display
   * @param x - X position
   * @param y - Y position
   * @param decimals - Number of decimal places (default: 0)
   */
  void printNumber(float value, int16_t x, int16_t y, int decimals = 0);

  /**
   * Get current font height (in pixels)
   */
  int16_t getFontHeight();

  /**
   * Get text width in pixels
   */
  int16_t getTextWidth(const char *text);

private:
  Adafruit_GFX *_display;

  // Font arrays for quick lookup
  static const GFXfont *_sansFonts[4];
  static const GFXfont *_sansBoldFonts[4];
  static const GFXfont *_serifFonts[4];
  static const GFXfont *_monoFonts[4];
};

#endif // DISPLAY_FONTS_H
