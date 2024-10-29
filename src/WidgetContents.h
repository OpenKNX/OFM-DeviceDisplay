#pragma once
#include "Widget.h"

class WidgetContents : public Widget
{
  public:
    WidgetContents(DisplayMode mode = DisplayMode::FOUR_LINE);
    ~WidgetContents();
    void draw(i2cDisplay* display);

    void printText(i2cDisplay* display, std::string strText, uint8_t size, uint16_t color, uint8_t x, uint8_t y);

  private:
    void testdrawline(i2cDisplay* display);
    void testdrawrect(i2cDisplay* display);
    void testfillrect(i2cDisplay* display);
    void testdrawcircle(i2cDisplay* display);
    void testfillcircle(i2cDisplay* display);
    void testdrawroundrect(i2cDisplay* display);
    void testfillroundrect(i2cDisplay* display);
    void testdrawtriangle(i2cDisplay* display);
    void testfilltriangle(i2cDisplay* display);
    void testdrawchar(i2cDisplay* display);
    void testdrawstyles(i2cDisplay* display);
    void testscrolltext(i2cDisplay* display);
    void testdrawbitmap(i2cDisplay* display);

};