#pragma once

#include "i2c-Display.h" // Assuming i2c-Display has display object definitions

#define MAX_CHARS_PER_LINE 20         // Maximum 20 characters per line
#define MAX_CHARS_PER_LINE_SCROLL 100 // Maximum 100 characters per line for scrolling
#define SCROLL_DELAY 250              // Scrolling speed (in milliseconds)
#define BLINK_DELAY 500               // Blinking speed (in milliseconds)

// Y positions for each line, starting from the top of the screen, in pixels (0-63) to be used with setCursor() function for each line
#define LCD_START_LINE_H 0  // Y Start position for header
#define LCD_START_LINE_1 9  // Y Start position for line 1
#define LCD_START_LINE_2 16 // Y Start position for line 2
#define LCD_START_LINE_3 23 // Y Start position for line 3

#define SSD1306_NO_SPLASH // Suppress the internal display splash screen

enum TextAlign
{
    LEFT,
    CENTER,
    RIGHT
}; // Top, Middle, Bottom to be added later

struct lcdText
{
    int16_t scrollPos = 0;              // Current scroll position
    uint16_t startPosY = 0;             // Y Start position for header
    uint16_t startPosX = 0;             // X Start position for header
    uint16_t textColor = SSD1306_WHITE; // Text color is either white or black
    uint16_t bgColor = SSD1306_BLACK;   // Background color is either white or black
    TextAlign align = LEFT;             // Text alignment. Default is left. Can be LEFT, CENTER, RIGHT
    uint8_t textSize = 1;               // Text size. Default is 1. Can be 1, 2, 3, 4
    bool scrollText = true;             // Flag to enable scrolling text
    bool pauseAtStart = false;          // Flag to pause scrolling text at start
    ulong lastPauseTime = 0;            // Time tracking for scrolling
    bool scrollTextPaused = false;      // Flag to pause scrolling text
    char prevText[MAX_CHARS_PER_LINE_SCROLL + 1] = "";
    char text[MAX_CHARS_PER_LINE_SCROLL + 1] = "";
};

class Widget
{
  public:
    enum class DisplayMode
    { // Modes for displaying the widget
        SINGLE_LINE,
        TWO_LINE,
        THREE_LINE,
        FOUR_LINE,
        ICON_WITH_TEXT,
        OPENKNX_LOGO,
        BOOT_LOGO,
        PROG_MODE,
        SCREEN_SAVER
    };

  private:
    ulong lastScrollTime = 0; // Time tracking for scrolling
    uint16_t getTextWidth(i2cDisplay* display, const char* text);
    uint16_t getTextHeight(i2cDisplay* display, const char* text);

    std::string singleLineText;      // Text for single line mode
    std::string line1, line2, line3; // Text for multi-line modes
    const uint8_t* iconBitmap;       // Bitmap for icon with text mode
    std::string iconText;            // Text for icon with text mode

    void displayText(i2cDisplay* display, lcdText* sText);
    void writeScrolledText(i2cDisplay* display, const char* text, int scrollPos, int maxChars);
    bool checkAndUpdateLcdText(lcdText* sText);
    bool updateDisplay(i2cDisplay* display);
    void OpenKNXLogo(i2cDisplay* display);

#define BOOT_LOGO_TIMEOUT 3000 // Timeout for showing the boot logo
    void ShowBootLogo(i2cDisplay* display);

#define PROG_MODE_BLINK_DELAY 500 // Blink delay for "Prog Mode active" text
    void showProgrammingMode(i2cDisplay* display);
    ulong _showProgrammingMode_last_Blink = 0;     // Last time the blink state was updated
    bool _showProgrammingMode_showProgMode = true; // Toggle between showing/hiding "Prog Mode active"

    void showMatrixScreensaver(i2cDisplay* display);
    void setUpMatrixScreensaver();
#define SVR_SCREEN_WIDTH 128
#define SVR_SCREEN_HEIGHT 64
#define COLUMN_WIDTH 8
#define FALL_SPEED 50 // Geschwindigkeit des Falls in Millisekunden
    ulong _lastUpdate = 0;
    char getRandomCP437Character();
#define MAX_DROPS 5 // Maximale Anzahl an fallenden Zeichen pro Spalte

    DisplayMode currentDisplayMode; // Current display mode

  public:
    // Constructor to initialize the widget with a display mode
    Widget(DisplayMode mode = DisplayMode::FOUR_LINE);
    ~Widget();
    void draw(i2cDisplay* display);

    // Setters for the Four lines of text
    // lcdText TextHeader, TextLine1, TextLine2, TextLine3;
    lcdText* TextHeader = new lcdText();
    lcdText* TextLine1 = new lcdText();
    lcdText* TextLine2 = new lcdText();
    lcdText* TextLine3 = new lcdText();
    void SetFourLines(const char* header, const char* line1, const char* line2, const char* line3);
    inline void SetFourHeader(const char* header) { strcpy(TextHeader->text, header); }
    inline void SetFourLine1(const char* line1) { strcpy(TextLine1->text, line1); }
    inline void SetFourLine2(const char* line2) { strcpy(TextLine2->text, line2); }
    inline void SetFourLine3(const char* line3) { strcpy(TextLine3->text, line3); }

    void EmptyLines(); // Clear all lines
};