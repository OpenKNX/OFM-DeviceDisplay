#pragma once
#pragma once
/**
 * @file        WidgetsManager.h
 * @brief       This module manages the widgets for displaying on the i2c display for the OpenKNX ecosystem
 * @version     0.0.1
 * @date        2025-10-10
 * @copyright   Copyright (c) 2025, Erkan Çolak (erkan@çolak.de)
 *              Licensed under GNU GPL v3.0
 */
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

    Widget* getCurrentWidget() { return _currentWidget; } // Get the current widget
    Widget* getWidgetFromQueue(const std::string& widgetName); // Get a widget from the queue by name
    Widget* getWidgetFromQueue(Widget* widget); // Get a widget from the queue by pointer
    std::deque<Widget*> getWidgetQueue() { return _widgetQueue; } // Get the widget queue

    void clearWidgetQueue() { _widgetQueue.clear(); } // Clear the widget queue
    void logWidgetQueue();

  private:
    std::deque<Widget*> _widgetQueue;     // Queue of widgets
    Widget* _currentWidget = nullptr;     // Pointer to the current widget
    uint32_t _currentTime = 0;            // Time when the widget should be removed
    i2cDisplay* _displayModule = nullptr; // Pointer to the display manager

    uint32_t _lastInteractionTime = 0; // Timestamp of the last interaction
    uint32_t _idleTimeout = 500;       // Timeout before the default widget is shown

    void removeWidgetFromQueue(const char* widgetName); // Remove a widget from the queue by name
    void removeWidgetFromQueue(Widget* widget); // Remove a widget from the queue by pointer
    Widget* getNextPriorityWidget(); // Get the next priority widget

    void handleCurrentWidget(uint32_t currentTime); // Handle the current widget
    bool activatePriorityWidget(uint32_t currentTime); // Activate a priority widget if available
    void activateNormalWidget(uint32_t currentTime); // Activate a normal widget if no priority widget is active
    void handleBackgroundAndDefaultWidgets(uint32_t currentTime); // Handle background and default widgets
};
