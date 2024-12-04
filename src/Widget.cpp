#include "Widget.h"
#ifndef NOT_SUPPORT_UMALAUTS
// #include <Fonts/FreeMonoBold9pt7b
#endif

/**
 * @brief Construct a new Widget:: Widget object
 * @param mode is the display mode for the widget
 */
Widget::Widget(DisplayMode mode)
    : currentDisplayMode(mode), iconBitmap(nullptr)
#ifdef QRCODE_WIDGET
      ,
      qrCodeWidget(nullptr, "", false /*, {nullptr, 0, 0}*/) // Initialize the QR code widget
#endif
{
    // Initialize some default values
    InitDynamicTextLines();
}

/**
 * @brief Destroy the Widget:: Widget object
 */
Widget::~Widget() {}

/**
 * @brief Empty all text lines. This function will empty all text lines.
 */
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
    if (strcmp(sText->text, sText->_prevText) != 0)
    {
        strncpy(sText->_prevText, sText->text,
                sizeof(sText->_prevText)); // Update previous text
        sText->scrollPos = 0;              // Reset scroll position
        return true;                       // Indicate that a change has occurred
    }
    return false; // No change
}

/**
 * @brief Update the the display with the current display mode. The display mode
 * can be DYNAMIC_TEXT, OPENKNX_LOGO, BOOT_LOGO, PROG_MODE, SCREEN_SAVER, or
 * ICON_WITH_TEXT.
 * @param display is a pointer to the i2cDisplay object.
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
#ifdef MATRIX_SCREENSAVER
        case DisplayMode::SCREEN_SAVER:
            showMatrixScreensaver(display);
            break;
#endif
#ifdef QRCODE_WIDGET
        case DisplayMode::QR_CODE:
            showQRCode(display);
            break;
#endif
        case DisplayMode::ICON_WITH_TEXT:
            // displayIconWithText(display);
            break;

        default:
            break;
    }
}

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
    strncpy(textLines[lineIndex].text, text, sizeof(textLines[lineIndex].text) - 1);
    textLines[lineIndex].text[sizeof(textLines[lineIndex].text) - 1] = '\0'; // Null-terminate explicitly

    // Optionally set other properties for the line here if needed
    textLines[lineIndex].scrollPos = 0;             // Reset scroll position if this line scrolls
    textLines[lineIndex]._scrollTextPaused = false; // Unpause scroll if needed
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
    size_t lineCount = std::min(lines.size(), static_cast<size_t>(MAX_TEXT_LINES));

    // Set each line's text and properties
    for (size_t i = 0; i < lineCount; ++i)
    {
        // Copy text into the text buffer, ensuring no overflow
        strncpy(textLines[i].text, lines[i], sizeof(textLines[i].text) - 1);
        textLines[i].text[sizeof(textLines[i].text) - 1] = '\0'; // Null-terminate explicitly
    }

    // Clear remaining unused text lines, if any
    for (size_t i = lineCount; i < MAX_TEXT_LINES; ++i)
    {
        textLines[i].text[0] = '\0'; // Empty unused line
    }
}

/**
 * @brief Initialize the dynamic text lines with default settings. This function
 * is called in the constructor to set up the text lines with default values.
 */
void Widget::InitDynamicTextLines()
{
    for (int i = 0; i < MAX_TEXT_LINES; ++i) // Initialize all text lines with default values
    {
        textLines[i].scrollPos = 0;
        textLines[i].startPosY = 0;
        textLines[i].startPosX = 0;

        if (i == 0)
        {                                           // Setting the default values for the Header!
            textLines[i].textColor = SSD1306_BLACK; // Set default text color for the Header
            textLines[i].bgColor = SSD1306_WHITE;   // Set default background color for the Header
            textLines[i].alignPos = ALIGN_CENTER;   // Set default alignment for the Header
        }
        else
        {
            textLines[i].textColor = SSD1306_WHITE; // Set default text color
            textLines[i].bgColor = SSD1306_BLACK;   // Set default background color
            textLines[i].alignPos = ALIGN_LEFT;     // Set default alignment
        }
        textLines[i].textSize = 1;
        textLines[i].scrollText = true;
        textLines[i].pauseAtStart = true;
        textLines[i]._lastPauseTime = 0;
        textLines[i]._lastScrollTime = 0;
        textLines[i]._scrollTextPaused = false;
        textLines[i]._prevText[0] = '\0';
        textLines[i].text[0] = '\0';
    }
}

/**
 * @brief Update the dynamic text lines on the display. This function should be
 * called in the loop function to update the dynamic text lines on the display.
 *
 * @param display pointer to the i2cDisplay object.
 * @return true if any text lines have been updated or scrolled, false
 * otherwise.
 */
void Widget::UpdateDynamicTextLines(i2cDisplay *display)
{
    // Check if all lines are empty; if so, exit early
    bool allEmpty = true;
    if (_AllowEmtyTextLines) allEmpty = false; // Do not skip empty lines at the beginning
    else                                       // Check if all lines are empty then exit early and do not display anything!
    {
        for (uint8_t i = 0; i < MAX_TEXT_LINES; ++i)
        {
            if (strlen(textLines[i].text) != 0)
            {
                allEmpty = false;
                break;
            }
        }
    }

    if (!allEmpty) // Proceed only if at least one line is non-empty
    {
        bool changed = false;

        // Check if any line has changed or needs to scroll
        for (uint8_t i = 0; i < MAX_TEXT_LINES; ++i)
        {
            changed |= checkAndUpdateLcdText(&textLines[i]);
        }
        displayDynamicText(display, {&textLines[0], &textLines[1], &textLines[2],
                                     &textLines[3], &textLines[4], &textLines[5],
                                     &textLines[6], &textLines[7]});
    }
}

/**
 * @brief Append a new line of text to the widget. If the widget already has the maximum number of text lines,
 * the oldest line will be removed. Use case: Displaying log messages or other dynamic text.
 * IMPORTANT: This function is not checking the text size. It assumes
 *            that the text font is default and the size is 1.
 * @param newLine is the new line at the end of the text widget.
 * @example appendLine( "New Line of Text");
 */
void Widget::appendLine(std::string newLine)
{
    if (textLines[MAX_TEXT_LINES - 1].text[0] != '\0')
    {
        for (uint8_t i = 0; i < MAX_TEXT_LINES; i++)
        {
            strncpy(textLines[i].text, textLines[i + 1].text,
                    sizeof(textLines[i].text) - 1);
        }
        strncpy(textLines[MAX_TEXT_LINES - 1].text, newLine.c_str(),
                sizeof(textLines[MAX_TEXT_LINES - 1].text) - 1);
    }
    else
    {
        for (uint8_t i = 0; i < MAX_TEXT_LINES; i++)
        {
            if (textLines[i].text[0] == '\0')
            {
                SetDynamicTextLine(i, newLine.c_str());
                break;
            }
        }
    }
}

/**
 * @brief get the width of the text in pixels,
 *        considering the text size for the default font.
 *
 * @param display pointer to the i2cDisplay object.
 * @param text a charecter to get the width of. e.g. "X"
 * @param textSize the size of the text
 * @return uint16_t the width of the text in pixels
 */
uint16_t Widget::getTextWidth(i2cDisplay *display, const char *text, uint8_t textSize)
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
 * @param display pointer to the i2cDisplay object.
 * @param text a charecter to get the height of. e.g. "X"
 * @param textSize the size of the text
 * @return uint16_t the height of the text in pixels
 */
uint16_t Widget::getTextHeight(i2cDisplay *display, const char *text, uint8_t textSize)
{
    int16_t x1, y1;
    uint16_t w, h;
    display->display->setTextSize(
        textSize); // Set text size before calculating bounds
    display->display->getTextBounds(text, 0, 0, &x1, &y1, &w, &h);
    return h;
}

/**
 * @brief Calculate the maximum number of text lines that can be displayed on the
 *
 * @param display pointer to the i2cDisplay object.
 * @param font the font to use for the calculation. Default is nullptr.
 * @return uint16_t  the maximum number of text lines that can be displayed on the
 */
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
    uint16_t maxTextLines = display->GetDisplayHeight() / textHeight;
    return maxTextLines;
}

/**
 * @brief Display the dynamic text lines on the display. This function is called
 * in the loop function to display the dynamic text lines on the display.
 *
 * @param display pointer to the i2cDisplay object.
 * @param textLines is a vector of lcdText objects representing the text lines
 * to display.
 */
void Widget::displayDynamicText(i2cDisplay *display, const std::vector<lcdText *> &textLines)
{
    uint32_t currentTime = millis();

    // Clear the display before updating lines
    display->display->clearDisplay();
    display->display->cp437(true); // Use CP437 character encoding

    // Calculate the heights of the text sections
    uint16_t totalHeightTop, totalHeightBottom, totalMiddleHeight, middleLineCount;
    calculateTextHeights(display, textLines, totalHeightTop, totalHeightBottom, totalMiddleHeight, middleLineCount);

    // Calculate available height for middle-aligned lines
    int16_t availableMiddleHeight = display->display->height() - (totalHeightTop + totalHeightBottom);
    int16_t middleStartY = totalHeightTop + (availableMiddleHeight - totalMiddleHeight) / 2;

    // Draw each line of text
    drawTextLines(display, textLines, totalHeightTop, totalHeightBottom, middleStartY, availableMiddleHeight, currentTime);

    // Refresh display
    display->display->display();
}

/**
 * @brief Calculate the heights of the top, middle, and bottom text sections.
 *
 * @param display pointer to the i2cDisplay object.
 * @param textLines is a vector of lcdText objects representing the text lines to display.
 * @param totalHeightTop will store the total height of the top-aligned text.
 * @param totalHeightBottom will store the total height of the bottom-aligned text.
 * @param totalMiddleHeight will store the total height of the middle-aligned text.
 * @param middleLineCount will store the count of middle-aligned lines.
 */
void Widget::calculateTextHeights(i2cDisplay *display, const std::vector<lcdText *> &textLines, uint16_t &totalHeightTop, uint16_t &totalHeightBottom, uint16_t &totalMiddleHeight, uint16_t &middleLineCount)
{
    totalHeightTop = 0;
    totalHeightBottom = 0;
    totalMiddleHeight = 0;
    middleLineCount = 0;

    for (const auto &line : textLines)
    {
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
}

/**
 * @brief Draw each line of text on the display.
 *
 * @param display pointer to the i2cDisplay object.
 * @param textLines is a vector of lcdText objects representing the text lines to display.
 * @param totalHeightTop is the total height of the top-aligned text.
 * @param totalHeightBottom is the total height of the bottom-aligned text.
 * @param middleStartY is the starting Y position for middle-aligned text.
 * @param availableMiddleHeight is the available height for middle-aligned text.
 * @param currentTime is the current time in milliseconds.
 */
void Widget::drawTextLines(i2cDisplay *display, const std::vector<lcdText *> &textLines, uint16_t totalHeightTop, uint16_t totalHeightBottom, uint16_t middleStartY, uint16_t availableMiddleHeight, uint32_t currentTime)
{
    for (const auto &line : textLines)
    {
        if (line->skipLineIfEmpty && strlen(line->text) == 0)
            continue; // Skip empty lines!

        handleScrolling(display, line, currentTime); // Handles scrolling text!

        uint16_t cursorX = calculateCursorX(display, line);                                                                         // Calculate the X position of the cursor
        uint16_t cursorY = calculateCursorY(display, line, totalHeightTop, totalHeightBottom, middleStartY, availableMiddleHeight); // Calculate the Y position of the cursor

        display->display->setCursor(cursorX, cursorY);                                                                                   // Set the cursor positions
        display->display->setTextColor(line->textColor, line->bgColor);                                                                  // Set text and background colors
        writeScrolledText(display, line->text, line->scrollPos, display->display->width() / getTextWidth(display, "X", line->textSize)); // Write the (scrolled) text
    }
}

/**
 * @brief Handle the scrolling of a text line.
 *
 * @param line is a pointer to the lcdText object representing the text line.
 * @param currentTime is the current time in milliseconds.
 */
void Widget::handleScrolling(i2cDisplay *display, lcdText *line, uint32_t currentTime)
{
    // Pause scrolling at start if needed
    if (line->pauseAtStart && line->scrollPos == 0)
    {
        line->_scrollTextPaused = (currentTime - line->_lastPauseTime < line->scrollPauseTime);
    }

    // Scroll text if needed and pause scrolling at the beginning if it is set
    if (line->scrollText && strlen(line->text) > display->display->width() / getTextWidth(display, "X", line->textSize) && !line->_scrollTextPaused)
    {
        if (currentTime - line->_lastScrollTime > SCROLL_DELAY) // Scrolling speed
        {
            // Scroll text by one position!
            line->scrollPos = (line->scrollPos + 1) % (strlen(line->text) - display->display->width() / getTextWidth(display, "X", line->textSize) + 1);
            // Update the last scroll time
            line->_lastScrollTime = currentTime;
        }
    }
    else
    {
        line->scrollPos = 0; // Reset scroll position if text is not scrolling!
    }
}

/**
 * @brief Calculate the X position of the cursor for a text line.
 *
 * @param display pointer to the i2cDisplay object.
 * @param line is a pointer to the lcdText object representing the text line.
 * @return the X position of the cursor.
 */
uint16_t Widget::calculateCursorX(i2cDisplay *display, const lcdText *line)
{
    uint16_t cursorX = line->startPosX;

    if (line->alignPos & ALIGN_CENTER) // Try to center the text
    {
        cursorX = (display->display->width() - getTextWidth(display, line->text, line->textSize)) / 2;
    }
    else if (line->alignPos & ALIGN_RIGHT) // Align to the right
    {
        cursorX = display->display->width() - getTextWidth(display, line->text, line->textSize);
    }
    else if (line->alignPos & ALIGN_LEFT)
    {
        cursorX = 0; // Align to the left
    }

    return cursorX;
}

/**
 * @brief Calculate the Y position of the cursor for a text line.
 *
 * @param display pointer to the i2cDisplay object.
 * @param line is a pointer to the lcdText object representing the text line.
 * @param totalHeightTop is the total height of the top-aligned text.
 * @param totalHeightBottom is the total height of the bottom-aligned text.
 * @param middleStartY is the starting Y position for middle-aligned text.
 * @param availableMiddleHeight is the available height for middle-aligned text.
 * @return the Y position of the cursor.
 */
uint16_t Widget::calculateCursorY(i2cDisplay *display, const lcdText *line, uint16_t &totalHeightTop, uint16_t &totalHeightBottom, uint16_t &middleStartY, uint16_t availableMiddleHeight)
{
    uint16_t cursorY = 0;
    const uint16_t lineHeight = getTextHeight(display, "X", line->textSize);

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
    else if (line->alignPos & ALIGN_MIDDLE && availableMiddleHeight >= lineHeight)
    {
        cursorY = middleStartY;
        middleStartY += lineHeight;
    }
    else
    {
        cursorY = totalHeightTop; // Default to top alignment if no specific alignment is set
        totalHeightTop += lineHeight;
    }

    return cursorY;
}

/**
 * @brief Write a substring of the text for horizontal scrolling.
 *
 * @param display pointer to the i2cDisplay object.
 * @param text to write using the display write method
 * @param scrollPos the current scroll position
 * @param maxChars size of the text to write
 */
void Widget::writeScrolledText(i2cDisplay *display, const char *text, int scrollPos, int maxChars)
{
    // Find the actual length of the text
    uint16_t textLen = strlen(text);

    // Scrollable text starts from `scrollPos`
    for (uint16_t i = scrollPos; i < scrollPos + maxChars && i < textLen; i++)
    {
#ifdef SUPPORT_UMALAUTS
        display->display->write(convertCharToCP437(static_cast<uint8_t>(text[i])));
#else
        display->display->write(text[i]);
#endif
    }
}

/**
 * @brief DIsplay the OpenKNX logo on the screen. The logo is displayed in the
 * center of the screen. The
 *
 * @param display pointer to the i2cDisplay object.
 */
void Widget::OpenKNXLogo(i2cDisplay *display)
{
    static String lastUptime = ""; // Cache for the last rendered uptime
    String currentUptime = openknx.logger.buildUptime().c_str(); // Get the current uptime
    
    if (lastUptime != currentUptime) // Check if the uptime has changed
    {
        display->display->cp437(true); // Use CP437 character encoding
        display->display->clearDisplay(); // Clear the display for fresh rendering

        display->display->setCursor(0, 0);
        display->display->setTextSize(1);
        display->display->setTextColor(WHITE);

        display->display->println("Uptime: " + currentUptime);
        lastUptime = currentUptime; // Update the cache

        display->display->println("Dev.: " MAIN_OrderNumber);
        display->display->println(String("Addr.: ") + openknx.info.humanIndividualAddress().c_str());

        display->display->drawBitmap(
            (display->GetDisplayWidth() - LOGO_WIDTH_ICON_SMALL_OKNX) / 2,
            (display->GetDisplayHeight() - LOGO_HEIGHT_ICON_SMALL_OKNX + 20 /*SHIFT_TO_BOTTOM*/) / 2,
            logoICON_SMALL_OKNX, LOGO_WIDTH_ICON_SMALL_OKNX,
            LOGO_HEIGHT_ICON_SMALL_OKNX, 1);
    }
    display->display->display(); // Update the display with the rendered content
}

/**
 * @brief Shows the OpenKNX logo on the display.
 *
 * @param display pointer to the i2cDisplay object.
 */
void Widget::ShowBootLogo(i2cDisplay *display)
{
    static bool showLogo = false; // Flag to ensure the logo is drawn only once
    if(!showLogo)
    {
        display->display->clearDisplay();
        display->display->drawBitmap(
            (display->GetDisplayWidth() - logo_OpenKNX_WIDTH) / 2,
            (display->GetDisplayHeight() - logo_OpenKNX_HEIGHT) / 2, logo_OpenKNX,
            logo_OpenKNX_WIDTH, logo_OpenKNX_HEIGHT, 1);
        display->display->display();
        showLogo = true;
    }
}

/**
 * @brief Will show the "Prog Mode active" message on the display with a
 * blinking effect. The message will blink every 500ms.
 *
 * @param display pointer to the i2cDisplay object.
 */
void Widget::showProgrammingMode(i2cDisplay *display)
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
    _showProgrammingMode_showProgMode
        ? display->display->setTextColor(SSD1306_BLACK, SSD1306_WHITE)
        : display->display->setTextColor(SSD1306_WHITE);
    display->display->setTextSize(2);      // Set font size to large for the message
    display->display->setCursor(0, 20);    // Position the cursor for the message
    display->display->print(" ProgMode!"); // Print "Prog Mode"

    display->display->setTextColor(SSD1306_WHITE);
    display->display->setTextSize(1);                                         // Set font size to large for the message
    display->display->setCursor(0, 45);                                       // Position the cursor for the message
    display->display->println(" Ready to use ETS to  program the Device!  "); // Print "Prog Mode"

    // Update the display with the new content
    display->display->display();
}

#ifdef MATRIX_SCREENSAVER
/**
 * @brief Display a matrix-style screensaver on the screen. The screensaver
 * consists of falling characters that move down the screen.
 *
 * @param display pointer to the i2cDisplay object.
 */
void Widget::showMatrixScreensaver(i2cDisplay *display)
{
    // CP437 character set
    static const char cp437[] = {
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
                display->display->write(cp437[random(0, sizeof(cp437))]);
            }
        }
        display->display->display();
    }
}
#endif // MATRIX_SCREENSAVER

#ifdef QRCODE_WIDGET
/**
 * @brief Display a QR code on the screen. The QR code is generated from the.
 *        Be sure to set the qrCode settings before calling this function!!
 *
 * @param display pointer to the i2cDisplay object.
 */

void Widget::showQRCode(i2cDisplay *display)
{
    if (display == nullptr) return;

    // Fallbacks for the QR code URL and icon
    if (qrCodeWidget.getUrl().empty()) qrCodeWidget.setUrl("https://www.openknx.de"); // fallback to the OpenKNX website!

    #ifdef QRCODE_WIDGET_ICON
    // No icon for the QR code, since there is no space for it! For testing, we could use the OpenKNX icon on the left or right side of the QR code
    // qrCodeWidget.setIcon(qrCode.icon); // No icon for the QR code, since there is no space for it!
    QRCodeWidget::Icon icon = {logoICON_SMALL_OKNX, LOGO_WIDTH_ICON_SMALL_OKNX, LOGO_HEIGHT_ICON_SMALL_OKNX}; // Default icon for the QR code
    qrCodeWidget.setIcon(icon);                                                                               // Set the icon for the QR code
    #endif

    qrCodeWidget.setDisplay(display);
    qrCodeWidget.draw();
}
#endif // QRCODE_WIDGET