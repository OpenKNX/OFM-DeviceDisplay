#pragma once 

#include "Widget.h"

class DisplayFacade
{
  public:
    DisplayFacade(i2cDisplay* disp);
    void setWidget(Widget* widget);
    void updateDisplay();

  private:
    i2cDisplay* display;
    Widget* currentWidget;
};