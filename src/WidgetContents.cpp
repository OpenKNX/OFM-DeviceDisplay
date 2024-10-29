#include "WidgetContents.h"
#include "i2c-Display.h"


//WidgetContents::WidgetContents() {
//  // Constructor implementation
//}
//
//WidgetContents::~WidgetContents() {
//  // Destructor implementation
//}
//
//void WidgetContents::display() {
//  // Display method implementation
//}


// Print text on the display
void WidgetContents::printText(i2cDisplay* display, std::string strText, uint8_t size, uint16_t color, uint8_t x, uint8_t y)
{
    if (strText.length())
    {
        display->display->clearDisplay();           // Clear the buffer
        display->display->setTextSize(size);        // Normal 1:1 pixel scale
        display->display->setTextColor(color);      // Draw white text
        display->display->setCursor(x, y);          // Start on given position
        display->display->println(strText.c_str()); // Print the text

        display->display->display(); // Show the text
    }
}

void WidgetContents::testdrawline( i2cDisplay* display)
{
    int16_t i;

    display->display->clearDisplay(); // Clear display buffer

    for (i = 0; i < display->display->width(); i += 4)
    {
        display->display->drawLine(0, 0, i, display->display->height() - 1, SSD1306_WHITE);
        display->display->display(); // Update screen with each newly-drawn line
        sleep_ms(1);
    }
    for (i = 0; i < display->display->height(); i += 4)
    {
        display->display->drawLine(0, 0, display->display->width() - 1, i, SSD1306_WHITE);
        display->display->display();
        sleep_ms(1);
    }
    sleep_ms(250);

    display->display->clearDisplay();

    for (i = 0; i < display->display->width(); i += 4)
    {
        display->display->drawLine(0, display->display->height() - 1, i, 0, SSD1306_WHITE);
        display->display->display();
        sleep_ms(1);
    }
    for (i = display->display->height() - 1; i >= 0; i -= 4)
    {
        display->display->drawLine(0, display->display->height() - 1, display->display->width() - 1, i, SSD1306_WHITE);
        display->display->display();
        sleep_ms(1);
    }
    sleep_ms(250);

    display->display->clearDisplay();

    for (i = display->display->width() - 1; i >= 0; i -= 4)
    {
        display->display->drawLine(display->display->width() - 1, display->display->height() - 1, i, 0, SSD1306_WHITE);
        display->display->display();
        sleep_ms(1);
    }
    for (i = display->display->height() - 1; i >= 0; i -= 4)
    {
        display->display->drawLine(display->display->width() - 1, display->display->height() - 1, 0, i, SSD1306_WHITE);
        display->display->display();
        sleep_ms(1);
    }
    sleep_ms(250);

    display->display->clearDisplay();

    for (i = 0; i < display->display->height(); i += 4)
    {
        display->display->drawLine(display->display->width() - 1, 0, 0, i, SSD1306_WHITE);
        display->display->display();
        sleep_ms(1);
    }
    for (i = 0; i < display->display->width(); i += 4)
    {
        display->display->drawLine(display->display->width() - 1, 0, i, display->display->height() - 1, SSD1306_WHITE);
        display->display->display();
        sleep_ms(1);
    }
}

void WidgetContents::testdrawrect( i2cDisplay* display)
{
    display->display->clearDisplay();

    for (int16_t i = 0; i < display->display->height() / 2; i += 2)
    {
        display->display->drawRect(i, i, display->display->width() - 2 * i, display->display->height() - 2 * i, SSD1306_WHITE);
        display->display->display(); // Update screen with each newly-drawn rectangle
        sleep_ms(1);
    }
}

void WidgetContents::testfillrect(i2cDisplay* display)
{
    display->display->clearDisplay();
    for (int16_t i = 0; i < display->display->height() / 2; i += 3)
    {
        // The INVERSE color is used so rectangles alternate white/black
        display->display->fillRect(i, i, display->display->width() - i * 2, display->display->height() - i * 2, SSD1306_INVERSE);
        display->display->display(); // Update screen with each newly-drawn rectangle
        sleep_ms(1);
    }
}

void WidgetContents::testdrawcircle(i2cDisplay* display)
{
    display->display->clearDisplay();

    for (int16_t i = 0; i < max(display->display->width(), display->display->height()) / 2; i += 2)
    {
        display->display->drawCircle(display->display->width() / 2, display->display->height() / 2, i, SSD1306_WHITE);
        display->display->display();
        sleep_ms(1);
    }
}

void WidgetContents::testfillcircle(i2cDisplay* display)
{
    display->display->clearDisplay();

    for (int16_t i = max(display->display->width(), display->display->height()) / 2; i > 0; i -= 3)
    {
        // The INVERSE color is used so circles alternate white/black
        display->display->fillCircle(display->display->width() / 2, display->display->height() / 2, i, SSD1306_INVERSE);
        display->display->display(); // Update screen with each newly-drawn circle
        sleep_ms(1);
    }
}

void WidgetContents::testdrawroundrect(i2cDisplay* display)
{
    display->display->clearDisplay();

    for (int16_t i = 0; i < display->display->height() / 2 - 2; i += 2)
    {
        display->display->drawRoundRect(i, i, display->display->width() - 2 * i, display->display->height() - 2 * i,
                               display->display->height() / 4, SSD1306_WHITE);
        display->display->display();
        sleep_ms(1);
    }
}

void WidgetContents::testfillroundrect(i2cDisplay* display)
{
    display->display->clearDisplay();

    for (int16_t i = 0; i < display->display->height() / 2 - 2; i += 2)
    {
        // The INVERSE color is used so round-rects alternate white/black
        display->display->fillRoundRect(i, i, display->display->width() - 2 * i, display->display->height() - 2 * i,
                               display->display->height() / 4, SSD1306_INVERSE);
        display->display->display();
        sleep_ms(1);
    }
}

void WidgetContents::testdrawtriangle(i2cDisplay* display)
{
    display->display->clearDisplay();

    for (int16_t i = 0; i < max(display->display->width(), display->display->height()) / 2; i += 5)
    {
        display->display->drawTriangle(
            display->display->width() / 2, display->display->height() / 2 - i,
            display->display->width() / 2 - i, display->display->height() / 2 + i,
            display->display->width() / 2 + i, display->display->height() / 2 + i, SSD1306_WHITE);
        display->display->display();
        sleep_ms(1);
    }
}

void WidgetContents::testfilltriangle(i2cDisplay* display)
{
    display->display->clearDisplay();

    for (int16_t i = max(display->display->width(), display->display->height()) / 2; i > 0; i -= 5)
    {
        // The INVERSE color is used so triangles alternate white/black
        display->display->fillTriangle(
            display->display->width() / 2, display->display->height() / 2 - i,
            display->display->width() / 2 - i, display->display->height() / 2 + i,
            display->display->width() / 2 + i, display->display->height() / 2 + i, SSD1306_INVERSE);
        display->display->display();
        sleep_ms(1);
    }
}

void WidgetContents::testdrawchar(i2cDisplay* display)
{
    display->display->clearDisplay();

    display->display->setTextSize(1);              // Normal 1:1 pixel scale
    display->display->setTextColor(SSD1306_WHITE); // Draw white text
    display->display->setCursor(0, 0);             // Start at top-left corner
    display->display->cp437(true);                 // Use full 256 char 'Code Page 437' font

    // Not all the characters will fit on the display->display-> This is normal.
    // Library will draw what it can and the rest will be clipped.
    for (int16_t i = 0; i < 256; i++)
    {
        if (i == '\n') display->display->write(' ');
        else
            display->display->write(i);
    }

    display->display->display();
    sleep_ms(2000); // Pause for 2 seconds
}

void WidgetContents::testdrawstyles(i2cDisplay* display)
{
    display->display->clearDisplay();

    display->display->setTextSize(1);              // Normal 1:1 pixel scale
    display->display->setTextColor(SSD1306_WHITE); // Draw white text
    display->display->setCursor(0, 0);             // Start at top-left corner
    display->display->println(F("Hello, world!"));

    display->display->setTextColor(SSD1306_BLACK, SSD1306_WHITE); // Draw 'inverse' text
    display->display->println(3.141592);

    display->display->setTextSize(2); // Draw 2X-scale text
    display->display->setTextColor(SSD1306_WHITE);
    display->display->setCursor(16, 0); // Start at top-left corner
    display->display->print(F("0x"));
    display->display->println(0xDEADBEEF, HEX);
    display->display->display();
}

void WidgetContents::testscrolltext(i2cDisplay* display)
{
    display->display->clearDisplay();

    display->display->setTextSize(2); // Draw 2X-scale text
    display->display->setTextColor(SSD1306_WHITE);
    display->display->setCursor(10, 0);
    display->display->println(F("scroll"));
    display->display->display(); // Show initial text
    sleep_ms(100);

    // Scroll in various directions, pausing in-between:
    display->display->startscrollright(0x00, 0x0F);
    sleep_ms(2000);
    display->display->stopscroll();
    sleep_ms(1000);
    display->display->startscrollleft(0x00, 0x0F);
    sleep_ms(2000);
    display->display->stopscroll();
    sleep_ms(1000);
    display->display->startscrolldiagright(0x00, 0x07);
    sleep_ms(2000);
    display->display->startscrolldiagleft(0x00, 0x07);
    sleep_ms(2000);
    display->display->stopscroll();
}

void WidgetContents::testdrawbitmap(i2cDisplay* display)
{
    display->display->clearDisplay();

    //display->display->drawBitmap( (display->display->width()  - logo_OpenKNX_WIDTH ) / 2, (display->display->height() - logo_OpenKNX_HEIGHT) / 2,
    //                     logo_OpenKNX, logo_OpenKNX_WIDTH, logo_OpenKNX_HEIGHT, 1);

    //  display->display->drawBitmap( (display->display->width()  - LOGO_WIDTH_OKNX ) / 2, (display->display->height() - LOGO_HEIGHT_OKNX) / 2,
    //                        logo_OKNX, LOGO_WIDTH_OKNX , LOGO_HEIGHT_OKNX, 1);
    //
    // display->display->drawBitmap( (display->display->width()  - LOGO_WIDTH_ICON_OKNX ) / 2, (display->display->height() - LOGO_HEIGHT_ICON_OKNX) / 2,
    //                      logoICON_OKNX, LOGO_WIDTH_ICON_OKNX , LOGO_HEIGHT_ICON_OKNX, 1);

    //display->display->drawBitmap((display->display->width() - LOGO_WIDTH_ICON_SMALL_OKNX) / 2, (display->display->height() - LOGO_HEIGHT_ICON_SMALL_OKNX) / 2,
    //                    logoICON_SMALL_OKNX, LOGO_WIDTH_ICON_SMALL_OKNX, LOGO_HEIGHT_ICON_SMALL_OKNX, 1);
    display->display->display();
}
