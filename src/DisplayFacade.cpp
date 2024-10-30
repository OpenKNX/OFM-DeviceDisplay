#include "DisplayFacade.h"

//  DisplayFacade::DisplayFacade(i2cDisplay* disp) : display(disp), currentWidget(nullptr) {}
//  
//  void DisplayFacade::setWidget(Widget* widget)
//  {
//      currentWidget = widget;           // Set the current widget
//      display->display->clearDisplay(); // Clear the display
//  }
//  
//  void DisplayFacade::updateDisplay()
//  {
//      if (currentWidget != nullptr)
//      {
//          currentWidget->draw(display); // Call the widget's draw method
//      }
//  }