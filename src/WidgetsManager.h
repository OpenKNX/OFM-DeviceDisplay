#pragma once
#include "widget.h"

class WidgetsManager
{
  public:
    const std::string logPrefix() { return "WidgetsManager"; }
    void addWidget(Widget* widget); // Add a widget to the queue
    void setup();                   // Setup all widgets
    void start();                   // Start the first widget
    void loop();                    // Main loop for the widgets
    void stopCurrent();             // Stop the current widget
    inline void setDisplayModule(i2cDisplay* displayModule) { _displayModule = displayModule; }
    inline i2cDisplay* getDisplayModule() { return _displayModule; }
    Widget *getCurrentWidget() { return _currentWidget; }

  private:
    std::queue<Widget*> _widgetQueue; // Queue for the widgets
    Widget* _currentWidget = nullptr; // Pointer to the current widget
    uint32_t _currentTime = 0;        // Time when the widget should be removed
    i2cDisplay* _displayModule = nullptr; // Pointer to the display manager
};
