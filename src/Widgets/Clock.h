#pragma once
#include "../Widget.h"

// WidgetClock class: Displays an analog or digital clock on the display.
class WidgetClock : public Widget
{
  public:
    WidgetClock(uint32_t displayTime, WidgetsAction action, bool roundedClock);

    void start() override;  // Start the clock screensaver
    void stop() override;   // Stop the clock screensaver
    void pause() override;  // Pause the clock screensaver
    void resume() override; // Resume the clock screensaver
    void setup() override;  // Setup the clock
    void loop() override;   // Update the clock display periodically

    uint32_t getDisplayTime() const override; // Return display duration in ms
    WidgetsAction getAction() const override; // Return widget action

    void setDisplayModule(i2cDisplay *displayModule) override; // Set display module
    i2cDisplay *getDisplayModule() const override;             // Get display module

  private:
    enum WidgetClockState
    {
        STOPPED,
        RUNNING,
        PAUSED
    };

    uint32_t _displayTime;    // Display time in ms
    WidgetsAction _action;    // Widget action
    uint32_t _lastUpdateTime; // Time of last update
    WidgetClockState _state;  // Clock state
    i2cDisplay *_display;     // Display object
    bool _roundedClock;       // Analog (true) or digital (false) clock

    void drawClock();                                                      // Draw the clock on the display
    void fetchTime(uint16_t &hours, uint16_t &minutes, uint16_t &seconds); // Get current time
};