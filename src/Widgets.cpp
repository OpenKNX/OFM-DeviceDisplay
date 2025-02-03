#include "Widgets.h"
#include "Widgets/Clock.h"
#ifndef NOT_SUPPORT_UMALAUTS
// #include <Fonts/FreeMonoBold9pt7b
#endif

/**
 * @brief Construct a new Widgets:: Widgets object
 * @param mode is the display mode for the widget
 */
Widgets::Widgets(DisplayMode mode)
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
 * @brief Destroy the Widgets:: Widgets object
 */
Widgets::~Widgets() {}

/**
 * @brief Empty all text lines. This function will empty all text lines.
 */
void Widgets::EmptyLines()
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
bool Widgets::checkAndUpdateLcdText(lcdText *sText)
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
void Widgets::draw(i2cDisplay *display)
{
    RUNTIME_MEASURE_BEGIN(_WidgetRutimeStat);
    if (display == nullptr)
        return;
    switch (currentDisplayMode)
    {
        case DisplayMode::DYNAMIC_TEXT:
            UpdateDynamicTextLines(display);
            break;
        default:
            break;
    }
    RUNTIME_MEASURE_END(_WidgetRutimeStat);
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
void Widgets::SetDynamicTextLine(size_t lineIndex, const char *text)
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
void Widgets::SetDynamicTextLines(const std::vector<const char *> &lines)
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
void Widgets::InitDynamicTextLines()
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
void Widgets::UpdateDynamicTextLines(i2cDisplay *display)
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
        // ToDo EC: "changed" is only detecting the text changes. But the scrolling needs also a redraw! 
        /*if (changed)*/ displayDynamicText(display, {&textLines[0], &textLines[1], &textLines[2],
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
void Widgets::appendLine(std::string newLine)
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
uint16_t Widgets::getTextWidth(i2cDisplay *display, const char *text, uint8_t textSize)
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
uint16_t Widgets::getTextHeight(i2cDisplay *display, const char *text, uint8_t textSize)
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
uint16_t Widgets::calculateMaxTextLines(i2cDisplay *display, const GFXfont *font)
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
void Widgets::displayDynamicText(i2cDisplay *display, const std::vector<lcdText *> &textLines)
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
    // display->display->display();
    display->displayBuff();
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
void Widgets::calculateTextHeights(i2cDisplay *display, const std::vector<lcdText *> &textLines, uint16_t &totalHeightTop, uint16_t &totalHeightBottom, uint16_t &totalMiddleHeight, uint16_t &middleLineCount)
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
void Widgets::drawTextLines(i2cDisplay *display, const std::vector<lcdText *> &textLines, uint16_t totalHeightTop, uint16_t totalHeightBottom, uint16_t middleStartY, uint16_t availableMiddleHeight, uint32_t currentTime)
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
void Widgets::handleScrolling(i2cDisplay *display, lcdText *line, uint32_t currentTime)
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
uint16_t Widgets::calculateCursorX(i2cDisplay *display, const lcdText *line)
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
uint16_t Widgets::calculateCursorY(i2cDisplay *display, const lcdText *line, uint16_t &totalHeightTop, uint16_t &totalHeightBottom, uint16_t &middleStartY, uint16_t availableMiddleHeight)
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
void Widgets::writeScrolledText(i2cDisplay *display, const char *text, int scrollPos, int maxChars)
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

void Widgets::showOpenKNXTeamIntro(i2cDisplay *display, const std::vector<std::string> &names, const std::string &endText)
{
    static uint8_t state = 0;            // Current state of the animation
    static unsigned long lastUpdate = 0; // Last update time of the animation
    static int step = 0;                 // Step within a state
    static uint8_t currentNameIndex = 0; // Current index of the names

    const uint16_t SCREEN_WIDTH = display->GetDisplayWidth();
    const uint16_t SCREEN_HEIGHT = display->GetDisplayHeight();
    const uint16_t LOGO_DISPLAY_TIME = 2000;  // Time to display the logo in full size (ms)
    const uint16_t ZOOM_OUT_SPEED = 50;       // Delay between zoom-out steps (ms)
    const uint16_t SCROLL_SPEED = 20;         // Speed of scrolling (ms)
    const uint16_t FONT_SIZE = 1;             // Base font size
    const uint8_t MAX_FONT_SIZE = 2;          // Maximum font size
    const uint8_t END_TEXT_MAX_FONT_SIZE = 1; // Smaller end text size

    unsigned long currentTime = millis();

    switch (state)
    {
        case 0: // Display the logo in full size
            if (currentTime - lastUpdate >= LOGO_DISPLAY_TIME)
            {
                lastUpdate = currentTime;
                step = 100; // Start zoom-out at 100%
                state = 1;
            }
            else
            {
                display->display->clearDisplay();
                display->display->drawBitmap(
                    (SCREEN_WIDTH - logo_OpenKNX_WIDTH) / 2,
                    (SCREEN_HEIGHT - logo_OpenKNX_HEIGHT) / 2,
                    logo_OpenKNX,
                    logo_OpenKNX_WIDTH,
                    logo_OpenKNX_HEIGHT,
                    WHITE);
                display->displayBuff();
            }
            break;

        case 1: // Zoom out the logo
            if (currentTime - lastUpdate >= ZOOM_OUT_SPEED)
            {
                lastUpdate = currentTime;

                if (step >= 10)
                {
                    display->display->clearDisplay();
                    int scaledWidth = (logo_OpenKNX_WIDTH * step) / 100;
                    int scaledHeight = (logo_OpenKNX_HEIGHT * step) / 100;

                    display->display->drawBitmap(
                        (SCREEN_WIDTH - scaledWidth) / 2,
                        (SCREEN_HEIGHT - scaledHeight) / 2,
                        logo_OpenKNX,
                        scaledWidth,
                        scaledHeight,
                        WHITE);
                    display->displayBuff();
                    step -= 5;
                }
                else
                {
                    step = SCREEN_HEIGHT; // Set scroll start
                    state = 2;            // Move to the next state
                }
            }
            break;

        case 2: // Scroll names from the bottom
            if (currentNameIndex < names.size())
            {
                if (currentTime - lastUpdate >= SCROLL_SPEED)
                {
                    lastUpdate = currentTime;

                    if (step > 0)
                    {
                        display->display->clearDisplay();
                        display->display->setTextSize(FONT_SIZE);
                        display->display->setCursor(SCREEN_WIDTH / 2 - (names[currentNameIndex].length() * 3), step);
                        display->display->print(names[currentNameIndex].c_str());
                        display->displayBuff();
                        step--;
                    }
                    else
                    {
                        state = 3; // Zoom effect for names
                        step = FONT_SIZE;
                        lastUpdate = currentTime; // Prevent immediate transition
                    }
                }
            }
            else
            {
                step = 1;
                state = 5; // Move to end text display
            }
            break;

        case 3: // Zoom effect for the current name
            if (currentTime - lastUpdate >= ZOOM_OUT_SPEED)
            {
                lastUpdate = currentTime;

                if (step <= MAX_FONT_SIZE)
                {
                    display->display->clearDisplay();
                    display->display->setTextSize(step);
                    display->display->setCursor(SCREEN_WIDTH / 2 - (names[currentNameIndex].length() * 3 * step), SCREEN_HEIGHT / 2 - (6 * step));
                    display->display->print(names[currentNameIndex].c_str());
                    display->displayBuff();
                    step++;
                }
                else
                {
                    state = 4;  // Move to fade-out
                    step = 255; // Start value for fade-out
                    lastUpdate = currentTime;
                }
            }
            break;

        case 4: // Fade-out for the current name
            if (currentTime - lastUpdate >= ZOOM_OUT_SPEED)
            {
                lastUpdate = currentTime;

                if (step > 0)
                {
                    display->display->clearDisplay();
                    display->display->setTextColor(WHITE);
                    display->display->setTextSize(MAX_FONT_SIZE);
                    display->display->setCursor(SCREEN_WIDTH / 2 - (names[currentNameIndex].length() * 3 * MAX_FONT_SIZE), SCREEN_HEIGHT / 2 - (6 * MAX_FONT_SIZE));
                    display->display->print(names[currentNameIndex].c_str());
                    display->displayBuff();
                    step -= 15;
                }
                else
                {
                    step = SCREEN_HEIGHT; // Reset for the next name
                    currentNameIndex++;
                    state = 2; // Back to scrolling
                }
            }
            break;

        case 5: // Display end text with slow zoom
            if (currentTime - lastUpdate >= 300)
            {
                lastUpdate = currentTime;

                if (step <= END_TEXT_MAX_FONT_SIZE)
                { // Smaller maximum size
                    display->display->clearDisplay();
                    display->display->setTextSize(step);
                    display->display->setCursor(SCREEN_WIDTH / 2 - (endText.length() * 3 * step), SCREEN_HEIGHT / 2 - (6 * step));
                    display->display->print(endText.c_str());
                    display->displayBuff();
                    step += 0.5; // Finer zoom
                }
                else
                {
                    state = 6; // Final state, show end text permanently
                }
            }
            break;

        case 6: // End text remains visible
            display->display->clearDisplay();
            display->display->setTextSize(3);
            display->display->setCursor(SCREEN_WIDTH / 2 - (endText.length() * 9), SCREEN_HEIGHT / 2 - 9);
            display->display->print(endText.c_str());
            display->displayBuff();
            break;
    }
}