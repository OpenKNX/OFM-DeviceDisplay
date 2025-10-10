#pragma once
#include "widget.h"
#ifdef ARDUINO_ARCH_ESP32
    #include <deque>
#endif

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
    std::deque<Widget*> getWidgetQueue() { return _widgetQueue; }

    void clearWidgetQueue() { _widgetQueue.clear(); }

  private:
    std::deque<Widget*> _widgetQueue;     // Queue of widgets
    Widget* _currentWidget = nullptr;     // Pointer to the current widget
    uint32_t _currentTime = 0;            // Time when the widget should be removed
    i2cDisplay* _displayModule = nullptr; // Pointer to the display manager

    uint32_t _lastInteractionTime = 0; // Timestamp of the last interaction
    uint32_t _idleTimeout = 500;       // Timeout before the default widget is shown

    void removeWidgetFromQueue(const char* widgetName);
    void removeWidgetFromQueue(Widget* widget);
    Widget* getNextPriorityWidget();


    void handleCurrentWidget(uint32_t currentTime);
    bool activatePriorityWidget(uint32_t currentTime);
    void activateNormalWidget(uint32_t currentTime);
    void handleBackgroundAndDefaultWidgets(uint32_t currentTime);
};
