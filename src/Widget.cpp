#include "Widget.h"
#include "DisplayIcons.h"

Widget::Widget(DisplayMode mode) : currentDisplayMode(mode), iconBitmap(nullptr)
{
    // Initialize some default values
}
Widget::~Widget()
{
    delete TextHeader;
    delete TextLine1;
    delete TextLine2;
    delete TextLine3;
}

// Set text lines for the display
void Widget::SetFourLines(const char* header, const char* line1, const char* line2, const char* line3)
{
    // Copy new strings into the current text variables
    strncpy(TextHeader->text, header, sizeof(TextHeader->text) - 1); // Allow longer lines for scrolling
    strncpy(TextLine1->text, line1, sizeof(TextLine1->text) - 1);
    strncpy(TextLine2->text, line2, sizeof(TextLine2->text) - 1);
    strncpy(TextLine3->text, line3, sizeof(TextLine3->text) - 1);

    // Header
    TextHeader->textColor = SSD1306_BLACK;
    TextHeader->bgColor = SSD1306_WHITE;
    TextHeader->align = CENTER;
    TextHeader->textSize = 1;
    TextHeader->startPosY = LCD_START_LINE_H; // 0; Start position for header
    TextHeader->pauseAtStart = true;

    TextLine1->textColor = SSD1306_WHITE;
    TextLine1->bgColor = SSD1306_BLACK;
    TextLine1->align = LEFT;
    TextLine1->textSize = 1;
    // TextLine1->startPosY = getTextHeight(display, "X"); // 8; Default Font! Depends on the font and text size
    TextLine1->pauseAtStart = true;

    // Line 2
    TextLine2->textColor = SSD1306_WHITE;
    TextLine2->bgColor = SSD1306_BLACK;
    TextLine2->align = LEFT;
    TextLine2->textSize = 1;
    // TextLine2->startPosY = getTextHeight(display, "X") * 2; // 16; Default Font! Depends on the font and text size
    TextLine2->pauseAtStart = true;

    // Line 3
    TextLine3->textColor = SSD1306_WHITE;
    TextLine3->bgColor = SSD1306_BLACK;
    TextLine3->align = LEFT;
    TextLine3->textSize = 1;
    // TextLine3->startPosY = getTextHeight(display, "X") * 3; // 24; Default Font! Depends on the font and text size
    TextLine3->pauseAtStart = true;
}
void Widget::EmptyLines()
{
    TextHeader->text[0] = '\0'; // Clear header
    TextLine1->text[0] = '\0';  // Clear line 1
    TextLine2->text[0] = '\0';  // Clear line 2
    TextLine3->text[0] = '\0';  // Clear line 3
}

// check if the text has changed
bool Widget::checkAndUpdateLcdText(lcdText* sText)
{
    if (strcmp(sText->text, sText->prevText) != 0)
    {
        strncpy(sText->prevText, sText->text, sizeof(sText->prevText)); // Update previous text
        sText->scrollPos = 0;                                           // Reset scroll position
        return true;                                                    // Indicate that a change has occurred
    }
    return false; // No change
}

// Update the display if any text has changed
void Widget::draw(i2cDisplay* display)
{
    if (display == nullptr) return;
    switch (currentDisplayMode)
    {
        case DisplayMode::FOUR_LINE:
            updateDisplay(display);
            break;
        case DisplayMode::OPENKNX_LOGO:
            OpenKNXLogo(display);
            break;
        case DisplayMode::BOOT_LOGO:
            ShowBootLogo(display);
            break;
        case DisplayMode::PROG_MODE:
            showProgrammingMode(display);
            break;
        case DisplayMode::SCREEN_SAVER:
            showMatrixScreensaver(display);
            break;
        case DisplayMode::SINGLE_LINE:
            // displaySingleLine(display);
            break;
        case DisplayMode::TWO_LINE:
            // displayTwoLine(display);
            break;
        case DisplayMode::THREE_LINE:
            // displayThreeLine(display);
            break;
        case DisplayMode::ICON_WITH_TEXT:
            // displayIconWithText(display);
            break;
    }
}

//
bool Widget::updateDisplay(i2cDisplay* display)
{

    // Do nothing if all lines and the header are empty
    if (strlen(TextHeader->text) == 0 && strlen(TextLine1->text) == 0 &&
        strlen(TextLine2->text) == 0 && strlen(TextLine3->text) == 0)
    {
        return false; // Stop updating if all lines are empty
    }

    // Get the current time for scrolling purposes
    ulong currentTime = millis();

    // Check if any lines have changed or need scrolling
    bool changed = false;

    // Detect changes in the header and each line
    changed |= checkAndUpdateLcdText(TextHeader);
    changed |= checkAndUpdateLcdText(TextLine1);
    changed |= checkAndUpdateLcdText(TextLine2);
    changed |= checkAndUpdateLcdText(TextLine3);

    // Only update the display if there's been a change or if it's time to scroll
    if (changed || (currentTime - lastScrollTime > SCROLL_DELAY))
    {
        lastScrollTime = currentTime; // Update scroll timer

        display->display->clearDisplay(); // Clear the display
        display->display->cp437(true);    // Use correct CP437 character codes

        // Header
        display->display->setTextSize(TextHeader->textSize); // Set the text size here, before calculating the text width/height
        TextLine1->startPosY = getTextHeight(display, "X");  // 8; Default Font! Depends on the font and text size
        displayText(display, TextHeader);

        // Line 1
        display->display->setTextSize(TextLine1->textSize); // Set the text size here, before calculating the text width/height
        TextLine1->startPosY = getTextHeight(display, "X"); // 8; Default Font! Depends on the font and text size
        displayText(display, TextLine1);

        // Line 2
        display->display->setTextSize(TextLine2->textSize);     // Set the text size here, before calculating the text width/height
        TextLine2->startPosY = getTextHeight(display, "X") * 2; // 16; Default Font! Depends on the font and text size
        displayText(display, TextLine2);

        // Line 3
        display->display->setTextSize(TextLine3->textSize);     // Set the text size here, before calculating the text width/height
        TextLine3->startPosY = getTextHeight(display, "X") * 3; // 24; Default Font! Depends on the font and text size
        displayText(display, TextLine3);

        display->display->display(); // Update the display
    }
    return true;
}

// Write a substring of the text for horizontal scrolling
void Widget::writeScrolledText(i2cDisplay* display, const char* text, int scrollPos, int maxChars)
{
    // Find the actual length of the text
    int textLen = strlen(text);

    // Scrollable text starts from `scrollPos`
    for (int i = scrollPos; i < scrollPos + maxChars && i < textLen; i++)
    {
        display->display->write(text[i]);
    }

    // If the text is shorter than the maximum characters, fill the rest with spaces
    // for (int i = strlen(text) - scrollPos; i < maxChars; i++) {
    //  display->write(' ');  // Fill the empty space if the text is short
    //}
}

// Get the width of the text in pixels.
uint16_t Widget::getTextWidth(i2cDisplay* display, const char* text)
{
    int16_t x1, y1;
    uint16_t w, h;
    display->display->getTextBounds(text, 0, 0, &x1, &y1, &w, &h);
    return w;
}

// Get the height of the text in pixels.
uint16_t Widget::getTextHeight(i2cDisplay* display, const char* text)
{
    int16_t x1, y1;
    uint16_t w, h;
    display->display->getTextBounds(text, 0, 0, &x1, &y1, &w, &h);
    return h;
}

// Display text on the screen with scrolling effect
void Widget::displayText(i2cDisplay* display, lcdText* sText)
{
    // Get the length of the text
    uint16_t textLength = strlen(sText->text);
    // Do nothing if the text is empty
    if (textLength == 0) return;

    // Check if the text should be paused from scrolling
    if (sText->pauseAtStart && sText->scrollPos == 0)
    {
        if (millis() - sText->lastPauseTime < 2000) sText->scrollTextPaused = true;
        else
            sText->scrollTextPaused = false;
    }

    // calculate the text width and chars can fit in one line. This depends on the text size and display width
    uint16_t maxCharsPerLine = display->display->width() / getTextWidth(display, "X");

    // Check if the text is longer than the display width
    if (sText->scrollText && textLength > maxCharsPerLine && !sText->scrollTextPaused)
    {
        display->display->setTextColor(sText->textColor, sText->bgColor);
        int16_t cursorX = 0;

        if (sText->align == CENTER)
        {
            cursorX = (display->display->width() - getTextWidth(display, sText->text)) / 2;
        }
        else if (sText->align == RIGHT)
        {
            cursorX = display->display->width() - getTextWidth(display, sText->text);
        }

        // Set the cursor position, scroll position and write the current text
        display->display->setCursor(cursorX, sText->startPosY);
        writeScrolledText(display, sText->text, sText->scrollPos, maxCharsPerLine);     // Scroll text
        sText->scrollPos = (sText->scrollPos + 1) % (textLength - maxCharsPerLine + 1); // Loop scroll position

        // Pause at the start of the text
        if (sText->scrollPos == 0)
        {
            sText->lastPauseTime = millis();
        }
    }
    else
    { // Display the text without scrolling
        display->display->setTextColor(sText->textColor, sText->bgColor);
        int16_t cursorX = 0;

        // Set the cursor position based on the alignment. Default is LEFT
        if (sText->align == CENTER)
        {
            cursorX = (display->display->width() - getTextWidth(display, sText->text)) / 2;
        }
        else if (sText->align == RIGHT)
        {
            cursorX = display->display->width() - getTextWidth(display, sText->text);
        }

        // Set the cursor position and write the current text
        display->display->setCursor(cursorX, sText->startPosY);
        writeScrolledText(display, sText->text, 0, maxCharsPerLine); // No scroll needed
    }
}

// Display the OpenKNX logo on the screen
void Widget::OpenKNXLogo(i2cDisplay* display)
{
    display->display->cp437(true); // Use correct CP437 character codes
    display->display->clearDisplay();
    display->display->setCursor(0, 0);
    display->display->setTextSize(1);
    display->display->setTextColor(WHITE);
    std::string uptimeStr = openknx.logger.buildUptime();

    // add a prefix "uptime: " to the uptime string
    uptimeStr = "Uptime: " + uptimeStr;
    display->display->print(uptimeStr.c_str());
    // display->print("   www.OpenKNX.de   ");

#define OPENKNX_LOGO_PRINT
#ifdef OPENKNX_LOGO_PRINT
    // Line 1: "Open" text and first block (■)
    display->display->setCursor(40, 10);
    display->display->print("O p e n ");
    display->display->write(0xDC); // Solid block (Code 219 for the square)

    // Line 2: Junction ┬, horizontal line ─, and ┴
    display->display->setCursor(40, 17);
    display->display->write(0xC2); // ┬ (Code 194)
    display->display->write(0xC4); // ─ (Code 196 for horizontal line)
    display->display->write(0xC4); // ─ (repeat for desired length)
    display->display->write(0xC4); // ─
    display->display->write(0xC4); // ─
    display->display->write(0xC4); // ─
    display->display->write(0xC4); // ─
    display->display->write(0xC4); // ─
    display->display->write(0xC1); // ┴ (Code 193)

    // Line 3: Block (■) and "KNX" text
    display->display->setCursor(40, 24);
    display->display->write(0xDF); // Solid block (Code 219 for the square)
    display->display->print(" K N X");
#else
    #define SHIFT_TO_BOTTOM 10
    display->display->drawBitmap((display->width() - LOGO_WIDTH_ICON_SMALL_OKNX) / 2, (display->height() - LOGO_HEIGHT_ICON_SMALL_OKNX + SHIFT_TO_BOTTOM) / 2,
                                 logoICON_SMALL_OKNX, LOGO_WIDTH_ICON_SMALL_OKNX, LOGO_HEIGHT_ICON_SMALL_OKNX, 1);
#endif
    //  display->setCursor(24,24);
    //  display->setTextSize(1);
    //  display->setTextColor(WHITE);
    //  std::string uptimeStr = openknx.logger.buildUptime();
    //  display->print(uptimeStr.c_str());

    display->display->display();
}

// Display the OpenKNX logo as boot logo!
void Widget::ShowBootLogo(i2cDisplay* display)
{
    display->display->clearDisplay();
    display->display->drawBitmap((display->display->width() - logo_OpenKNX_WIDTH) / 2, (display->display->height() - logo_OpenKNX_HEIGHT) / 2,
                                 logo_OpenKNX, logo_OpenKNX_WIDTH, logo_OpenKNX_HEIGHT, 1);
    display->display->display();
}

// Function to display "Prog Mode active" with blinking effect
void Widget::showProgrammingMode(i2cDisplay* display)
{
    ulong currentTime = millis();

    // Check if it's time to toggle the blink state
    if (currentTime - _showProgrammingMode_last_Blink >= PROG_MODE_BLINK_DELAY)
    {
        _showProgrammingMode_last_Blink = currentTime;                          // Update the last blink time
        _showProgrammingMode_showProgMode = !_showProgrammingMode_showProgMode; // Toggle the blink state (show/hide text)
    }

    display->display->clearDisplay(); // Clear the display
    display->display->setTextColor(SSD1306_WHITE);
    // Set the header: "OpenKNX" (always visible, not blinking)
    display->display->setTextSize(1);                // Set font size to normal for the header
    display->display->setCursor(0, 0);               // Position the cursor at the top
    display->display->print("   www.OpenKNX.de   "); // Print the header

    // Show the "Prog Mode active" message if the blink state is true
    _showProgrammingMode_showProgMode ? display->display->setTextColor(SSD1306_BLACK, SSD1306_WHITE) : display->display->setTextColor(SSD1306_WHITE);
    display->display->setTextSize(2);      // Set font size to large for the message
    display->display->setCursor(0, 20);    // Position the cursor for the message
    display->display->print(" ProgMode!"); // Print "Prog Mode"

    // Update the display with the new content
    display->display->display();
}

// Get a random CP437 character
char Widget::getRandomCP437Character()
{
    // CP437 character set
    const char cp437[] = {
        0x20, 0x21, 0x22, 0x23, 0x24, 0x25, 0x26, 0x27, 0x28, 0x29, 0x2A, 0x2B, 0x2C, 0x2D, 0x2E, 0x2F, // ' ', '!', '"', '#', '$', '%', '&', '\'', '(', ')', '*', '+', ',', '-', '.', '/'
        0x30, 0x31, 0x32, 0x33, 0x34, 0x35, 0x36, 0x37, 0x38, 0x39, 0x3A, 0x3B, 0x3C, 0x3D, 0x3E, 0x3F, // '0', '1', '2', '3', '4', '5', '6', '7', '8', '9', ':', ';', '<', '=', '>', '?'
        0x40, 0x41, 0x42, 0x43, 0x44, 0x45, 0x46, 0x47, 0x48, 0x49, 0x4A, 0x4B, 0x4C, 0x4D, 0x4E, 0x4F, // '@', 'A', 'B', 'C', 'D', 'E', 'F', 'G', 'H', 'I', 'J', 'K', 'L', 'M', 'N', 'O', 'P'
        0x50, 0x51, 0x52, 0x53, 0x54, 0x55, 0x56, 0x57, 0x58, 0x59, 0x5A, 0x5B, 0x5C, 0x5D, 0x5E, 0x5F, // 'Q', 'R', 'S', 'T', 'U', 'V', 'W', 'X', 'Y', 'Z', '[', '\\', ']', '^', '_', '`'
        0x61, 0x62, 0x63, 0x64, 0x65, 0x66, 0x67, 0x68, 0x69, 0x6A, 0x6B, 0x6C, 0x6D, 0x6E, 0x6F, 0x70, // 'a', 'b', 'c', 'd', 'e', 'f', 'g', 'h', 'i', 'j', 'k', 'l', 'm', 'n', 'o', 'p'
        0x71, 0x72, 0x73, 0x74, 0x75, 0x76, 0x77, 0x78, 0x79, 0x7A, 0x7B, 0x7C, 0x7D, 0x7E, 0xC4, 0xB1, // 'q', 'r', 's', 't', 'u', 'v', 'w', 'x', 'y', 'z', '{', '|', '}', '~', '█', '▓'
        0xB0, 0xB1, 0xB2, 0xB3, 0xC4, 0xC5, 0xC6, 0xC7, 0xC8, 0xC9,                                     // '▒', '░', '▌', '▐', '▄', '▔', '▕', '▁', '▏', '▎'
    };
    int index = random(0, sizeof(cp437));
    return cp437[index];
}

// Display a matrix-style screensaver on the screen
void Widget::showMatrixScreensaver(i2cDisplay* display)
{
    randomSeed(analogRead(0));
    static int _MatrixDropPos[16][6];
    static bool initialized = false;
    if (!initialized)
    {
        for (int col = 0; col < 16; col++)
        {
            for (int drop = 0; drop < 6; drop++)
            {
                _MatrixDropPos[col][drop] = random(0, 64);
            }
        }
        initialized = true;
    }

    unsigned long currentTime = millis();
    if (currentTime - _lastUpdate >= FALL_SPEED)
    {
        _lastUpdate = currentTime;
        display->display->clearDisplay();

        for (uint16_t x = 0; x < (SVR_SCREEN_WIDTH / COLUMN_WIDTH); x++)
        {
            for (uint16_t drop = 0; drop < MAX_DROPS; drop++)
            {
                if (_MatrixDropPos[x][drop] < SVR_SCREEN_HEIGHT)
                {
                    _MatrixDropPos[x][drop] += COLUMN_WIDTH;
                }
                else
                {
                    _MatrixDropPos[x][drop] = 0;
                }
                display->display->setCursor(x * COLUMN_WIDTH, _MatrixDropPos[x][drop]);
                display->display->setTextColor(SSD1306_WHITE);
                display->display->setTextSize(1);
                display->display->write(getRandomCP437Character());
            }
        }
        display->display->display();
    }
}