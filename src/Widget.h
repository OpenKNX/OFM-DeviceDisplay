#pragma once

#include "i2c-Display.h"  // Include 1st
#include "DisplayIcons.h" // Include 2nd
#include "QRCodeGen.hpp" // Include 3rd

// Maximum 100 characters per line for scrolling text
#define MAX_CHARS_PER_LINE_SCROLL 100
#define SCROLL_DELAY 250 // Scrolling speed (in milliseconds)

// Default widget settings
#define PROG_MODE_BLINK_DELAY 500 // Blink delay for "Prog Mode active" text
#define BOOT_LOGO_TIMEOUT 5000    // Timeout for showing the boot logo

// Default settings for the display
#define MAX_TEXT_LINES 8 // Maximum number of text lines on a 128x64 display with default font

// WIDGETS
#define QRCODE_WIDGET      // Enable the QR code widget
#define MATRIX_SCREENSAVER // Enable the matrix screensaver

// Matrix screensaver settings
#ifdef MATRIX_SCREENSAVER
    #define COLUMN_WIDTH 8 // Width of a column in pixels
    #define FALL_SPEED 50  // Falling speed in milliseconds
    #define MAX_DROPS 5    // Maximum number of falling characters per column
#endif

#define WIDGET_INACTIVE 0 // Widget is inactive

// Text alignment options
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

// lcdText struct for dynamic text lines
struct lcdText
{
    int16_t scrollPos = 0;              // Current scroll position
    uint16_t startPosY = 0;             // Y Start position for header
    uint16_t startPosX = 0;             // X Start position for header
    uint16_t textColor = SSD1306_WHITE; // Text color is either white or black
    uint16_t bgColor = SSD1306_BLACK;   // Background color is either white or black
    TextDynamicAlign alignPos = ALIGN_LEFT;
    uint8_t textSize = 1;         // Text size. Default is 1. Can be 1, 2, 3, 4
    bool skipLineIfEmpty = false; // Skip line if empty. Next non-empty line will be displayed in the same position

    // Scrolling text settings
    bool scrollText = true;          // Flag to enable scrolling text
    bool pauseAtStart = false;       // Flag to pause scrolling text at start
    uint16_t scrollPauseTime = 2000; // Time to pause scrolling text at start
    bool _scrollTextPaused = false;  // Flag to pause scrolling text
    ulong _lastPauseTime = 0;        // Time tracking for scrolling
    ulong _lastScrollTime = 0;       // Time tracking for scrolling

    // Text for scrolling
    char _prevText[MAX_CHARS_PER_LINE_SCROLL + 1] = ""; // Previous text for scrolling
    char text[MAX_CHARS_PER_LINE_SCROLL + 1] = "";      // Default text for the line
};

struct lcdQRCode
{
    std::string qrText;                                                                                       // Text for the QR code
    bool useIcon = true;                                                                                      // Flag to use an icon for the QR code
    QRCodeWidget::Icon icon = {logoICON_SMALL_OKNX, LOGO_WIDTH_ICON_SMALL_OKNX, LOGO_HEIGHT_ICON_SMALL_OKNX}; // Default icon for the QR code
};

// Overload | operator for TextDynamicAlign
inline TextDynamicAlign operator|(TextDynamicAlign lhs, TextDynamicAlign rhs)
{
    return static_cast<TextDynamicAlign>(static_cast<uint8_t>(lhs) | static_cast<uint8_t>(rhs));
}

class Widget
{
  public:
    enum class DisplayMode
    {
        DYNAMIC_TEXT,   // Dynamic text lines depending on the lines of the display and their position or settings
        ICON_WITH_TEXT, // Icon with text mode
        OPENKNX_LOGO,   // OpenKNX logo
        PROG_MODE,      // Programming mode
#ifdef QRCODE_WIDGET
        QR_CODE, // QR Code
#endif
#ifdef MATRIX_SCREENSAVER
        SCREEN_SAVER, // Matrix screensaver
#endif
        BOOT_LOGO // Boot logo

    };

  private:
    DisplayMode currentDisplayMode; // Current display mode for the widgets. Default is dynamic text mode
    const uint8_t *iconBitmap;      // Bitmap for icon with text mode

    // Text lines for dynamic text mode
    uint16_t getTextWidth(i2cDisplay *display, const char *text, uint8_t textSize);             // Get the width of the text in pixels
    uint16_t getTextHeight(i2cDisplay *display, const char *text, uint8_t textSize);            // Get the height of the text in pixels
    uint16_t calculateMaxTextLines(i2cDisplay *display, const GFXfont *font = nullptr);         // Calculate the maximum number of text lines
    void writeScrolledText(i2cDisplay *display, const char *text, int scrollPos, int maxChars); // Write the scrolled text to the display
    bool checkAndUpdateLcdText(lcdText *sText);                                                 // Check and update the text on the display
    void displayDynamicText(i2cDisplay *display, const std::vector<lcdText *> &lines);          // Display the dynamic text on the display
    void InitDynamicTextLines();                                                                // Initialize the dynamic text lines with default settings
    void UpdateDynamicTextLines(i2cDisplay *display);                                           // Update the dynamic text lines on the display

    // Helper functions for displayDynamicText
    void calculateTextHeights(i2cDisplay *display, const std::vector<lcdText *> &textLines, uint16_t &totalHeightTop, uint16_t &totalHeightBottom, uint16_t &totalMiddleHeight, uint16_t &middleLineCount);             // Calculate the heights of the text sections
    void drawTextLines(i2cDisplay *display, const std::vector<lcdText *> &textLines, uint16_t totalHeightTop, uint16_t totalHeightBottom, uint16_t middleStartY, uint16_t availableMiddleHeight, uint32_t currentTime); // Draw each line of text
    void handleScrolling(i2cDisplay *display, lcdText *line, uint32_t currentTime);                                                                                                                                     // Handle the scrolling of a text line
    uint16_t calculateCursorX(i2cDisplay *display, const lcdText *line);                                                                                                                                                // Calculate the X position of the cursor for a text line
    uint16_t calculateCursorY(i2cDisplay *display, const lcdText *line, uint16_t &totalHeightTop, uint16_t &totalHeightBottom, uint16_t &middleStartY, uint16_t availableMiddleHeight);                                 // Calculate the Y position of the cursor for a text line

    // Boot logo and OpenKNX logo
    void OpenKNXLogo(i2cDisplay *display);  // Show the OpenKNX logo on the display
    void ShowBootLogo(i2cDisplay *display); // Show the boot logo on the display

    // Programming mode
    ulong _showProgrammingMode_last_Blink = 0;     // Last time the blink state was updated
    bool _showProgrammingMode_showProgMode = true; // Toggle between showing/hiding "Prog Mode active"
    void showProgrammingMode(i2cDisplay *display); // Show the programming mode on the display

#ifdef MATRIX_SCREENSAVER
    // Matrix screensaver
    ulong _lastUpdateScreenSaver = 0;                // Last time the screensaver was updated
    void showMatrixScreensaver(i2cDisplay *display); // Show the matrix screensaver on the display
#endif
#ifdef QRCODE_WIDGET
    QRCodeWidget qrCodeWidget; // QR Code Widget
    void showQRCode(i2cDisplay *display);
#endif // QRCODE_WIDGET

    // Convert special characters to CP437. Supports only with another font!
    // inline uint8_t convertCharToCP437(uint8_t ch) {
    //    switch (ch) {
    //        case 0xC4: return 0x8E; // Ä
    //        case 0xD6: return 0x99; // Ö
    //        case 0xDC: return 0x9A; // Ü
    //        case 0xE4: return 0x84; // ä
    //        case 0xF6: return 0x94; // ö
    //        case 0xFC: return 0x81; // ü
    //        case 0xDF: return 0xE1; // ß
    //        case 0xC7: return 0x80; // Ç
    //        case 0xE7: return 0x87; // ç
    //        default: return ch;     // All other characters are returned as is
    //    }
    //}

  public:
    lcdQRCode qrCode;                                     // QR Code settings
    void appendLine(std::string newLine); // Append a new line to the widget. Only works for dynamic text mode and use case is console output

    Widget(DisplayMode mode = DisplayMode::DYNAMIC_TEXT);             // Constructor
    ~Widget();                                                        // Destructor
    void draw(i2cDisplay *display);                                   // Update the display with the current display mode
    void SetDynamicTextLines(const std::vector<const char *> &lines); // Set the text for multiple lines in the widget
    void SetDynamicTextLine(size_t lineIndex, const char *text);      // Set the text for a specific line in the widget
    lcdText textLines[MAX_TEXT_LINES];                                // Fixed array for text lines
    void EmptyLines();                                                // Clear all lines
}; // End of class Widget