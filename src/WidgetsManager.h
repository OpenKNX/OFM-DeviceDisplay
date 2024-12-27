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
    inline void setDisplayModule(i2cDisplay* displayModule) { _displayModule = displayModule; }
    inline i2cDisplay* getDisplayModule() { return _displayModule; }
    Widget* getCurrentWidget() { return _currentWidget; }
    Widget* getWidgetFromQueue(const char* widgetName);
    Widget* getWidgetFromQueue(Widget* widget);
    std::queue<Widget*> getWidgetQueue() { return _widgetQueue; }
    void clearWidgetQueue()
    {
        while (!_widgetQueue.empty())
            _widgetQueue.pop();
    }

  private:
    std::queue<Widget*> _widgetQueue;     // Queue of widgets
    Widget* _currentWidget = nullptr;     // Pointer to the current widget
    uint32_t _currentTime = 0;            // Time when the widget should be removed
    i2cDisplay* _displayModule = nullptr; // Pointer to the display manager

    uint32_t _lastInteractionTime = 0; // Timestamp of the last interaction
    uint32_t _idleTimeout = 1000;      // Timeout before the default widget is shown (Start immediately - 1s Wait)!

    void removeWidgetFromQueue(const char* widgetName);
    void removeWidgetFromQueue(Widget* widget);
    Widget* getNextPriorityWidget();
};
