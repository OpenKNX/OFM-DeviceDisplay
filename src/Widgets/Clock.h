#pragma once
#include "../Widget.h"

// WidgetClock class: Displays an analog or digital clock on the display.
class WidgetClock : public Widget
{
  public:
    const std::string logPrefix() { return "WidgetClock"; }
    WidgetClock(uint32_t displayTime, WidgetsAction action, bool roundedClock);

    void start() override;                                                  // Start widget
    void stop() override;                                                   // Stop widget
    void pause() override;                                                  // Pause widget
    void resume() override;                                                 // Resume widget
    void setup() override;                                                  // Setup widget
    void loop() override;                                                   // Update and draw the widget
    inline const WidgetState getState() const override { return _state; }   // Get the current state of the widget
    inline const std::string getName() const override { return _name; }     // Return the name of the widget
    inline void setName(const std::string &name) override { _name = name; } // Set the name of the widget

    uint32_t getDisplayTime() const override; // Return display duration in ms
    WidgetsAction getAction() const override; // Return widget action

    void setDisplayModule(i2cDisplay *displayModule) override; // Set display module
    i2cDisplay *getDisplayModule() const override;             // Get display module

  private:
  
    uint32_t _displayTime;       // Display time in ms
    WidgetsAction _action;       // Widget action
    uint32_t _lastUpdateTime;    // Time of last update
    WidgetState _state;     // Clock state
    i2cDisplay *_display;        // Display object
    bool _roundedClock;          // Analog (true) or digital (false) clock
    std::string _name = "Clock"; // Name of the widget

    void drawClock();                                                      // Draw the clock on the display
    void fetchTime(uint16_t &hours, uint16_t &minutes, uint16_t &seconds); // Get current time
};