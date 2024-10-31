#include "Widget.h"
#include "DisplayIcons.h"
#ifndef NOT_SUPPORT_UMALAUTS
// #include <Fonts/FreeMonoBold9pt7b
#endif

/**
 * @brief Construct a new Widget:: Widget object
 *
 * @param mode is the display mode for the widget
 */
Widget::Widget(DisplayMode mode)
    : currentDisplayMode(mode), iconBitmap(nullptr)
{
    // Initialize some default values
    InitDynamicTextLines();
}

/**
 * @brief Destroy the Widget:: Widget object
 *
 */
Widget::~Widget() {}

void Widget::EmptyLines()
{
    for (int i = 0; i < MAX_TEXT_LINES; ++i)
    {
        textLines[i].text[0] = '\0'; // Empty the text buffer
    }
}

/**
 * @brief Check if the text has changed
 *
 * @param sText to check
 * @return true if the text has changed
 * @return false if the text has not changed
 */
bool Widget::checkAndUpdateLcdText(lcdText *sText)
{
    if (strcmp(sText->text, sText->prevText) != 0)
    {
        strncpy(sText->prevText, sText->text,
                sizeof(sText->prevText)); // Update previous text
        sText->scrollPos = 0;             // Reset scroll position
        return true;                      // Indicate that a change has occurred
    }
    return false; // No change
}

/**
 * @brief Update the the display with the current display mode. The display mode
 * can be DYNAMIC_TEXT, OPENKNX_LOGO, BOOT_LOGO, PROG_MODE, SCREEN_SAVER, or
 * ICON_WITH_TEXT.
 * @param display is the display to update
 *
 */
void Widget::draw(i2cDisplay *display)
{
    if (display == nullptr)
        return;
    switch (currentDisplayMode)
    {
        case DisplayMode::DYNAMIC_TEXT:
            UpdateDynamicTextLines(display);
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
        case DisplayMode::ICON_WITH_TEXT:
            // displayIconWithText(display);
            break;
    }
}

// Example so set a line of text
// widget.SetLine(2, "Updated Line 3 Text");
// widget.SetLine(3, "Updated Line 4 Text");

/**
 * @brief Set the text for a specific line in the widget. The line index is
 * zero-based. and there is a maximum of 8 lines. The text will be copied into
 * the line's buffer, ensuring no overflow. YOu can set the text color,
 * background color, alignment, and text size for each line. Depending on the
 * settings, the text can be scrolled, paused at the start, or paused at the
 * end. The size of the text buffer is limited to 21 characters. All text lines
 * with more than 21 characters will be displayed as scrolling text, but only if
 * the scrollText flag is set to true. The line size depends on the display size
 * and the text size. For a 128x64 display, the maximum number lines for 1 text
 * size is 7, for text size 2 is 4, for text size 3 is 3, and for text size 4
 * is 2.
 * @param lineIndex is the index of the line to set the text for. The index is
 * zero-based.
 * @param text is the text to set for the line. The text will be copied into the
 * line's buffer, ensuring no overflow.
 * @example SerLine(0, "Updated Header Text");
 *          SetLine(1, "Updated Line 2 Text");
 */
void Widget::SetDynamicTextLine(size_t lineIndex, const char *text)
{
    // Ensure the line index is within bounds
    if (lineIndex >= MAX_TEXT_LINES)
    {
        return; // Do nothing if index is out of bounds
    }

    // Copy new text into the specified line's buffer, ensuring no overflow
    strncpy(textLines[lineIndex].text, text,
            sizeof(textLines[lineIndex].text) - 1);
    textLines[lineIndex].text[sizeof(textLines[lineIndex].text) - 1] =
        '\0'; // Null-terminate explicitly

    // Optionally set other properties for the line here if needed
    textLines[lineIndex].scrollPos =
        0;                                         // Reset scroll position if this line scrolls
    textLines[lineIndex].scrollTextPaused = false; // Unpause scroll if needed
}

/**
 * @brief  Set the text for all lines in the widget. The text will be copied
 * into the line's buffer, ensuring no overflow.
 *
 * @param lines is a vector of text lines to set for the widget. The text will
 * be copied into the line's buffer, ensuring no overflow.
 * @example SetLines({"Header Text", "Line 1 Text", "Line 2 Text"});
 */
void Widget::SetDynamicTextLines(const std::vector<const char *> &lines)
{
    // Limit the number of lines to the pre-allocated textLines capacity
    size_t lineCount =
        std::min(lines.size(), static_cast<size_t>(MAX_TEXT_LINES));

    // Set each line's text and properties
    for (size_t i = 0; i < lineCount; ++i)
    {
        // Copy text into the text buffer, ensuring no overflow
        strncpy(textLines[i].text, lines[i], sizeof(textLines[i].text) - 1);

        // Default properties for all lines,  for the header line set the text color
        // to black and background color to white and the alignment to center
        if (i == 0) // Spezific settings for the header line
        {
            textLines[i].textColor = SSD1306_BLACK;
            textLines[i].bgColor = SSD1306_WHITE;
            textLines[i].alignPos = ALIGN_CENTER;
            // textLines[i].textSize = 1;
            // textLines[i].pauseAtStart = true;
        }
        else // Default settings for all other lines. Since they are allready set
             // in the InitDynamicTextLines function, we don't need to set them
             // here again
        {
            // textLines[i].textColor = SSD1306_WHITE;
            // textLines[i].bgColor = SSD1306_BLACK;
            // textLines[i].align = LEFT;
            // textLines[i].textSize = 1;
            // textLines[i].pauseAtStart = false;
        }
    }

    // Clear remaining unused text lines, if any
    for (size_t i = lineCount; i < MAX_TEXT_LINES; ++i)
    {
        textLines[i].text[0] = '\0'; // Empty unused line
    }
}

// Dynamic text lines depending on the lines of the display and their position
// or settings
/**
 * @brief Initialize the dynamic text lines with default settings. This function
 * is called in the constructor to set up the text lines with default values.
 */
void Widget::InitDynamicTextLines()
{
    // Initialize all text lines
    for (int i = 0; i < MAX_TEXT_LINES; ++i)
    {
        textLines[i].scrollPos = 0;
        textLines[i].startPosY = 0;
        textLines[i].startPosX = 0;
        textLines[i].textColor = SSD1306_WHITE;
        textLines[i].bgColor = SSD1306_BLACK;
        textLines[i].alignPos = ALIGN_LEFT;
        textLines[i].textSize = 1;
        textLines[i].scrollText = true;
        textLines[i].pauseAtStart = true;
        textLines[i].lastPauseTime = 0;
        textLines[i].lastScrollTime = 0;
        textLines[i].scrollTextPaused = false;
        textLines[i].prevText[0] = '\0';
        textLines[i].text[0] = '\0';
    }
}

/**
 * @brief Update the dynamic text lines on the display. This function should be
 * called in the loop function to update the dynamic text lines on the display.
 *
 * @param display is a pointer to the i2cDisplay object to update the text on.
 * @return true if any text lines have been updated or scrolled, false
 * otherwise.
 */
bool Widget::UpdateDynamicTextLines(i2cDisplay *display)
{
    // Check if all lines are empty; if so, exit early
    bool allEmpty = true;
    for (int i = 0; i < MAX_TEXT_LINES; ++i)
    {
        if (strlen(textLines[i].text) != 0)
        {
            allEmpty = false;
            break;
        }
    }
    if (allEmpty)
        return false;
    bool changed = false;

    // Check if any line has changed or needs to scroll
    for (int i = 0; i < MAX_TEXT_LINES; ++i)
    {
        changed |= checkAndUpdateLcdText(&textLines[i]);
    }

    displayDynamicText(display, {&textLines[0], &textLines[1], &textLines[2],
                                 &textLines[3], &textLines[4], &textLines[5],
                                 &textLines[6], &textLines[7]});

    return true;
}

/**
 * @brief Append a new line of text to the widget. If the widget already has the
 maximum number of text lines,
 *        the oldest line will be removed. Use case: Displaying log messages or
 other dynamic text.
 *        IMPORTANT: This function is not checking the text size. It assumes
 that the text font is default and the size is 1.
 *
 * @param display object
 * @param newLine is the new line of text to append to the widget.
 * @example appendLine(display, "New Line of Text");

 */
void Widget::appendLine(Widget *Widget, std::string newLine)
{
    if (Widget->textLines[MAX_TEXT_LINES - 1].text[0] != '\0')
    {
        for (int i = 0; i < MAX_TEXT_LINES; i++)
        {
            strncpy(Widget->textLines[i].text, Widget->textLines[i + 1].text,
                    sizeof(Widget->textLines[i].text) - 1);
        }
        strncpy(Widget->textLines[MAX_TEXT_LINES - 1].text, newLine.c_str(),
                sizeof(Widget->textLines[MAX_TEXT_LINES - 1].text) - 1);
    }
    else
    {
        for (int i = 0; i < MAX_TEXT_LINES; i++)
        {
            if (Widget->textLines[i].text[0] == '\0')
            {
                SetDynamicTextLine(i, newLine.c_str());
                break;
            }
        }
    }
}

// Get the width of the text in pixels, considering the text size.
/**
 * @brief get the width of the text in pixels, considering the text size for the
 * default font.
 *
 * @param display  object
 * @param text a charecter to get the width of. e.g. "X"
 * @param textSize the size of the text
 * @return uint16_t the width of the text in pixels
 */
uint16_t Widget::getTextWidth(i2cDisplay *display, const char *text,
                              uint8_t textSize)
{
    int16_t x1, y1;
    uint16_t w, h;
    display->display->setTextSize(
        textSize); // Set text size before calculating bounds
    display->display->getTextBounds(text, 0, 0, &x1, &y1, &w, &h);
    return w;
}

/**
 * @brief get the height of the text in pixels, considering the text size for
 * the default font.
 *
 * @param display object
 * @param text a charecter to get the height of. e.g. "X"
 * @param textSize the size of the text
 * @return uint16_t the height of the text in pixels
 */
uint16_t Widget::getTextHeight(i2cDisplay *display, const char *text,
                               uint8_t textSize)
{
    int16_t x1, y1;
    uint16_t w, h;
    display->display->setTextSize(
        textSize); // Set text size before calculating bounds
    display->display->getTextBounds(text, 0, 0, &x1, &y1, &w, &h);
    return h;
}

uint16_t Widget::calculateMaxTextLines(i2cDisplay *display, const GFXfont *font)
{

    if (font != nullptr) // Optional font
    {
        display->display->setFont(font);
    }

    int16_t x, y;
    uint16_t textWidth, textHeight;

    // Berechne die Höhe einer Beispielzeile
    display->display->getTextBounds("X", 0, 0, &x, &y, &textWidth, &textHeight);

    // Berechne die maximalen Zeilen
    int maxTextLines = display->GetDisplayHeight() / textHeight;
    return maxTextLines;
}

/**
 * @brief Display the dynamic text lines on the display. This function is called
 * in the loop function to display the dynamic text lines on the display.
 *
 * @param display is a pointer to the i2cDisplay object to display the text on.
 * @param textLines is a vector of lcdText objects representing the text lines
 * to display.
 */
void Widget::displayDynamicText(i2cDisplay *display,
                                const std::vector<lcdText *> &textLines)
{
    uint32_t currentTime = millis();

    // Clear the display before updating lines
    display->display->clearDisplay();
    display->display->cp437(true); // Use CP437 character encoding
    // display->display->setFont(&FreeMonoBold9pt7b);

    int16_t totalHeightTop = 0, totalHeightBottom = 0;
    int middleLineCount = 0;
    int16_t totalMiddleHeight = 0;

    // First pass: Calculate the height of top, middle, and bottom sections
    for (size_t i = 0; i < textLines.size(); ++i)
    {
        lcdText *line = textLines[i];
        int16_t lineHeight = getTextHeight(display, "X", line->textSize);

        if (line->alignPos & ALIGN_TOP)
        {
            totalHeightTop += lineHeight;
        }
        else if (line->alignPos & ALIGN_BOTTOM)
        {
            totalHeightBottom += lineHeight;
        }
        else if (line->alignPos & ALIGN_MIDDLE)
        {
            totalMiddleHeight += lineHeight;
            ++middleLineCount;
        }
    }

    // Calculate available height for middle-aligned lines
    int16_t availableMiddleHeight =
        display->display->height() - (totalHeightTop + totalHeightBottom);
    int16_t middleStartY =
        totalHeightTop + (availableMiddleHeight - totalMiddleHeight) / 2;

    // Second pass: Calculate cursor positions and display each line
    for (size_t i = 0; i < textLines.size(); ++i)
    {
        lcdText *line = textLines[i];
        uint16_t textLength = strlen(line->text);

        // Skip if the text is empty
        if (textLength == 0)
            continue;

        // Handle pause at the start of scrolling
        if (line->pauseAtStart && line->scrollPos == 0)
        {
            if (millis() - line->lastPauseTime < 2000)
                line->scrollTextPaused = true;
            else
                line->scrollTextPaused = false;
        }

        // Calculate maximum characters per line
        uint16_t maxCharsPerLine =
            display->display->width() / getTextWidth(display, "X", line->textSize);
        int16_t cursorX = 0;

        // Determine horizontal alignment
        if (line->alignPos & ALIGN_CENTER)
        {
            cursorX = (display->display->width() -
                       getTextWidth(display, line->text, line->textSize)) /
                      2;
        }
        else if (line->alignPos & ALIGN_RIGHT)
        {
            cursorX = display->display->width() -
                      getTextWidth(display, line->text, line->textSize);
        }

        // Calculate Y position based on alignment
        int16_t cursorY = 0;
        int16_t lineHeight = getTextHeight(display, "X", line->textSize);

        if (line->alignPos & ALIGN_TOP)
        {
            cursorY = totalHeightTop;
            totalHeightTop += lineHeight;
        }
        else if (line->alignPos & ALIGN_BOTTOM)
        {
            totalHeightBottom += lineHeight;
            cursorY = display->display->height() - totalHeightBottom;
        }
        else if (line->alignPos & ALIGN_MIDDLE &&
                 availableMiddleHeight >= totalMiddleHeight)
        {
            cursorY = middleStartY;
            middleStartY += lineHeight;
        }
        else
        {
            // Default stacking if no alignment flag is set
            cursorY = i * lineHeight;
        }

        // Check if scrolling is needed
        if (line->scrollText && textLength > maxCharsPerLine &&
            !line->scrollTextPaused)
        {
            // Update scroll position if enough time has passed
            if (currentTime - line->lastScrollTime > SCROLL_DELAY)
            {
                line->scrollPos =
                    (line->scrollPos + 1) % (textLength - maxCharsPerLine + 1);
                line->lastScrollTime = currentTime;
            }
        }
        else
        {
            // Reset scroll position if scrolling is disabled or text is shorter than
            // the display width
            line->scrollPos = 0;
        }

        // Set the cursor position and write the current text
        display->display->setCursor(cursorX, cursorY);
        display->display->setTextColor(line->textColor, line->bgColor);
        writeScrolledText(display, line->text, line->scrollPos, maxCharsPerLine);
    }

    // Refresh display
    display->display->display();
}

/**
 * @brief Write a substring of the text for horizontal scrolling.
 *
 * @param display object
 * @param text to write using the display write method
 * @param scrollPos the current scroll position
 * @param maxChars size of the text to write
 */
void Widget::writeScrolledText(i2cDisplay *display, const char *text,
                               int scrollPos, int maxChars)
{
    // Find the actual length of the text
    int textLen = strlen(text);

    // Scrollable text starts from `scrollPos`
    for (int i = scrollPos; i < scrollPos + maxChars && i < textLen; i++)
    {
#ifdef NOT_SUPPORT_UMALAUTS
        display->display->write(text[i]);
#else
        display->display->write(convertCharToCP437(static_cast<uint8_t>(text[i])));
#endif
    }

    // If the text is shorter than the maximum characters, fill the rest with
    // spaces for (int i = strlen(text) - scrollPos; i < maxChars; i++) {
    //  display->write(' ');  // Fill the empty space if the text is short
    //}
}

// Display the OpenKNX logo on the screen
/**
 * @brief DIsplay the OpenKNX logo on the screen. The logo is displayed in the
 * center of the screen. The
 *
 * @param display
 */
void Widget::OpenKNXLogo(i2cDisplay *display)
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

//#define OPENKNX_LOGO_PRINT
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
    display->display->drawBitmap(
        (display->GetDisplayWidth() - LOGO_WIDTH_ICON_SMALL_OKNX) / 2,
        (display->GetDisplayHeight() - LOGO_HEIGHT_ICON_SMALL_OKNX + SHIFT_TO_BOTTOM) / 2,
        logoICON_SMALL_OKNX, LOGO_WIDTH_ICON_SMALL_OKNX,
        LOGO_HEIGHT_ICON_SMALL_OKNX, 1);
#endif
    //  display->setCursor(24,24);
    //  display->setTextSize(1);
    //  display->setTextColor(WHITE);
    //  std::string uptimeStr = openknx.logger.buildUptime();
    //  display->print(uptimeStr.c_str());

    display->display->display();
}

/**
 * @brief Shows the OpenKNX logo on the display.
 *
 * @param display object
 */
void Widget::ShowBootLogo(i2cDisplay *display)
{
    display->display->clearDisplay();
    display->display->drawBitmap(
        (display->GetDisplayWidth() - logo_OpenKNX_WIDTH) / 2,
        (display->GetDisplayHeight() - logo_OpenKNX_HEIGHT) / 2, logo_OpenKNX,
        logo_OpenKNX_WIDTH, logo_OpenKNX_HEIGHT, 1);
    display->display->display();
}

/**
 * @brief Will show the "Prog Mode active" message on the display with a
 * blinking effect. The message will blink every 500ms.
 *
 * @param display object
 */
void Widget::showProgrammingMode(i2cDisplay *display)
{
    ulong currentTime = millis();

    // Check if it's time to toggle the blink state
    if (currentTime - _showProgrammingMode_last_Blink >= PROG_MODE_BLINK_DELAY)
    {
        _showProgrammingMode_last_Blink = currentTime; // Update the last blink time
        _showProgrammingMode_showProgMode =
            !_showProgrammingMode_showProgMode; // Toggle the blink state (show/hide
                                                // text)
    }

    display->display->clearDisplay(); // Clear the display
    display->display->setTextColor(SSD1306_WHITE);
    // Set the header: "OpenKNX" (always visible, not blinking)
    display->display->setTextSize(1);                // Set font size to normal for the header
    display->display->setCursor(0, 0);               // Position the cursor at the top
    display->display->print("   www.OpenKNX.de   "); // Print the header

    // Show the "Prog Mode active" message if the blink state is true
    _showProgrammingMode_showProgMode
        ? display->display->setTextColor(SSD1306_BLACK, SSD1306_WHITE)
        : display->display->setTextColor(SSD1306_WHITE);
    display->display->setTextSize(2);      // Set font size to large for the message
    display->display->setCursor(0, 20);    // Position the cursor for the message
    display->display->print(" ProgMode!"); // Print "Prog Mode"

    display->display->setTextColor(SSD1306_WHITE);
    display->display->setTextSize(1);      // Set font size to large for the message
    display->display->setCursor(0, 45);    // Position the cursor for the message
    display->display->println(" Ready to use ETS to  program the Device!  "); // Print "Prog Mode"

    // Update the display with the new content
    display->display->display();

}

/**
 * @brief Generates a random CP437 character from the CP437 character set.
 *
 * @return char
 */
char Widget::getRandomCP437Character()
{
    // CP437 character set
    const char cp437[] = {
        0x20, 0x21, 0x22, 0x23, 0x24, 0x25, 0x26, 0x27, 0x28, 0x29, 0x2A, 0x2B, 0x2C, 0x2D, 0x2E, 0x2F,
        // ' ', '!', '"', '#', '$', '%', '&', '\'', '(', ')', '*', '+', ',', '-', '.', '/'
        0x30, 0x31, 0x32, 0x33, 0x34, 0x35, 0x36, 0x37, 0x38, 0x39, 0x3A, 0x3B, 0x3C, 0x3D, 0x3E, 0x3F,
        // '0', '1', '2', '3', '4', '5', '6', '7', '8', '9', ':', ';', '<', '=', '>', '?'
        0x40, 0x41, 0x42, 0x43, 0x44, 0x45, 0x46, 0x47, 0x48, 0x49, 0x4A, 0x4B, 0x4C, 0x4D, 0x4E, 0x4F,
        // '@', 'A', 'B', 'C', 'D', 'E', 'F', 'G', 'H', 'I', 'J', 'K', 'L', 'M', 'N', 'O', 'P'
        0x50, 0x51, 0x52, 0x53, 0x54, 0x55, 0x56, 0x57, 0x58, 0x59, 0x5A, 0x5B, 0x5C, 0x5D, 0x5E, 0x5F,
        // 'Q', 'R', 'S', 'T', 'U', 'V', 'W', 'X', 'Y', 'Z', '[', '\\', ']', '^', '_', '`'
        0x61, 0x62, 0x63, 0x64, 0x65, 0x66, 0x67, 0x68, 0x69, 0x6A, 0x6B, 0x6C, 0x6D, 0x6E, 0x6F, 0x70,
        // 'a', 'b', 'c', 'd', 'e', 'f', 'g', 'h', 'i', 'j', 'k', 'l', 'm', 'n', 'o', 'p'
        0x71, 0x72, 0x73, 0x74, 0x75, 0x76, 0x77, 0x78, 0x79, 0x7A, 0x7B, 0x7C, 0x7D, 0x7E, 0xC4, 0xB1,
        // 'q', 'r', 's', 't', 'u', 'v', 'w', 'x', 'y', 'z', '{', '|', '}', '~', '█', '▓'
        0xB0, 0xB1, 0xB2, 0xB3, 0xC4, 0xC5, 0xC6, 0xC7, 0xC8, 0xC9,
        // '▒', '░', '▌', '▐', '▄', '▔', '▕', '▁', '▏', '▎'
    };
    int index = random(0, sizeof(cp437));
    return cp437[index];
}

/**
 * @brief Display a matrix-style screensaver on the screen. The screensaver
 * consists of falling characters that move down the screen.
 *
 * @param display object
 */
void Widget::showMatrixScreensaver(i2cDisplay *display)
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
    if (currentTime - _lastUpdateScreenSaver >= FALL_SPEED)
    {
        _lastUpdateScreenSaver = currentTime;
        display->display->clearDisplay();

        for (uint16_t x = 0; x < (display->GetDisplayWidth() / COLUMN_WIDTH); x++)
        {
            for (uint16_t drop = 0; drop < MAX_DROPS; drop++)
            {
                if (_MatrixDropPos[x][drop] < display->GetDisplayHeight())
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