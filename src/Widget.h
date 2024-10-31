#pragma once

#include "i2c-Display.h" // Assuming i2c-Display has display object definitions

#define MAX_CHARS_PER_LINE 20 // Maximum 20 characters per line
#define MAX_CHARS_PER_LINE_SCROLL \
    100                  // Maximum 100 characters per line for scrolling
#define SCROLL_DELAY 250 // Scrolling speed (in milliseconds)
#define BLINK_DELAY 500  // Blinking speed (in milliseconds)

// Y positions for each line, starting from the top of the screen, in pixels
// (0-63) to be used with setCursor() function for each line
#define LCD_START_LINE_H 0  // Y Start position for header
#define LCD_START_LINE_1 9  // Y Start position for line 1
#define LCD_START_LINE_2 16 // Y Start position for line 2
#define LCD_START_LINE_3 23 // Y Start position for line 3

#define SSD1306_NO_SPLASH // Suppress the internal display splash screen

#define PROG_MODE_BLINK_DELAY 500 // Blink delay for "Prog Mode active" text
#define BOOT_LOGO_TIMEOUT 3000    // Timeout for showing the boot logo
#define MAX_TEXT_LINES 8          // 8 Maximum number of text lines on a 128x64 display. This Setting can be
                                  // adjusted for other display sizes
#define SVR_SCREEN_WIDTH 128
#define SVR_SCREEN_HEIGHT 64
#define COLUMN_WIDTH 8
#define FALL_SPEED 50 // Geschwindigkeit des Falls in Millisekunden
#define MAX_DROPS 5   // Maximale Anzahl an fallenden Zeichen pro Spalte

#define WIDGET_INACTIVE 0 // Widget is inactive

enum TextAlign
{
    LEFT,
    CENTER,
    RIGHT
}; // Top, Middle, Bottom to be added later
enum TextDynamicAlign : uint8_t
{
    ALIGN_LEFT = 0x01,
    ALIGN_CENTER = 0x02,
    ALIGN_RIGHT = 0x04,
    ALIGN_TOP = 0x10,
    ALIGN_MIDDLE = 0x20,
    ALIGN_BOTTOM = 0x40
};
// Overload | operator for TextDynamicAlign
inline TextDynamicAlign operator|(TextDynamicAlign lhs, TextDynamicAlign rhs)
{
    return static_cast<TextDynamicAlign>(static_cast<uint8_t>(lhs) | static_cast<uint8_t>(rhs));
}

struct lcdText
{
    int16_t scrollPos = 0;              // Current scroll position
    uint16_t startPosY = 0;             // Y Start position for header
    uint16_t startPosX = 0;             // X Start position for header
    uint16_t textColor = SSD1306_WHITE; // Text color is either white or black
    uint16_t bgColor = SSD1306_BLACK;   // Background color is either white or black
    TextAlign align =
        LEFT; // Text alignment. Default is left. Can be LEFT, CENTER, RIGHT
    TextDynamicAlign alignPos = ALIGN_LEFT;
    uint8_t textSize = 1;      // Text size. Default is 1. Can be 1, 2, 3, 4
    bool scrollText = true;    // Flag to enable scrolling text
    bool pauseAtStart = false; // Flag to pause scrolling text at start
    ulong lastPauseTime = 0;   // Time tracking for scrolling
    ulong lastScrollTime = 0;  // Time tracking for scrolling

    bool scrollTextPaused = false; // Flag to pause scrolling text
    char prevText[MAX_CHARS_PER_LINE_SCROLL + 1] = "";
    char text[MAX_CHARS_PER_LINE_SCROLL + 1] = "";
};

class Widget
{
  public:
    enum class DisplayMode
    { // Modes for displaying the widget
        DYNAMIC_TEXT,
        ICON_WITH_TEXT,
        OPENKNX_LOGO,
        BOOT_LOGO,
        PROG_MODE,
        SCREEN_SAVER
    };

  private:
    DisplayMode currentDisplayMode; // Current display mode
    const uint8_t *iconBitmap;       // Bitmap for icon with text mode
    ulong _lastUpdate = 0;
    ulong _showProgrammingMode_last_Blink = 0; // Last time the blink state was updated
    bool _showProgrammingMode_showProgMode = true; // Toggle between showing/hiding "Prog Mode active"

    uint16_t getTextWidth(i2cDisplay *display, const char *text, uint8_t textSize);
    uint16_t getTextHeight(i2cDisplay *display, const char *text, uint8_t textSize);
    void writeScrolledText(i2cDisplay *display, const char *text, int scrollPos, int maxChars);
    bool checkAndUpdateLcdText(lcdText *sText);
    void displayDynamicText(i2cDisplay *display, const std::vector<lcdText *> &lines);
    void InitDynamicTextLines();
    bool UpdateDynamicTextLines(i2cDisplay *display);
    void OpenKNXLogo(i2cDisplay *display);
    void ShowBootLogo(i2cDisplay *display);
    void showProgrammingMode(i2cDisplay *display);
    void showMatrixScreensaver(i2cDisplay *display);
    void setUpMatrixScreensaver();
    char getRandomCP437Character();

  public:
    void appendLine( Widget* Widget, std::string newLine);

    // Constructor to initialize the widget with a display mode
    Widget(DisplayMode mode = DisplayMode::DYNAMIC_TEXT);
    ~Widget();
    void draw(i2cDisplay *display);
    void SetDynamicTextLines(const std::vector<const char *> &lines);
    void SetDynamicTextLine(size_t lineIndex, const char *text);
    lcdText textLines[MAX_TEXT_LINES]; // Fixed array for text lines
    void EmptyLines(); // Clear all lines
};