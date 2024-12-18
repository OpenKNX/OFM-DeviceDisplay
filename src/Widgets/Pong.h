#pragma once
#include "../Widget.h"

// WidgetPong class. Is to show a Pong game on the display.

class WidgetPong : public Widget
{
  public:
    const std::string logPrefix() { return "WidgetPong"; }
    WidgetPong(uint32_t displayTime, WidgetsAction action);

    void start() override;  // Start the widget and display the WidgetPong screensaver
    void stop() override;   // Stop the WidgetPong screensaver and clear the display
    void pause() override;  // Pause the WidgetPong screensaver
    void resume() override; // Resume the WidgetPong screensaver
    void setup() override;  // Setup the widget
    void loop() override;   // Loop is called every second to update the screensaver

    uint32_t getDisplayTime() const override; // Return the display time in ms
    WidgetsAction getAction() const override; // Return the widget action

    void setDisplayModule(i2cDisplay *displayModule) override; // Set the display module
    i2cDisplay *getDisplayModule() const override;             // Get the display module

  private:

    uint32_t _displayTime;        // Time to display the widget in ms
    WidgetsAction _action;        // Widget action
    uint32_t _lastUpdateTime = 0; // Last update time
    WidgetState _state;       // Current state of the widget
    i2cDisplay *_display;         // Display object

    // WidgetPong screensaver data
    uint16_t paddleLeftY;  // Paddle left Y position
    uint16_t paddleRightY; // Paddle right Y position
    uint16_t ballX;        // Ball X position
    uint16_t ballY;        // Ball Y position
    int ballSpeedX;        // Ball speed X
    int ballSpeedY;        // Ball speed Y

    // Initialize the default settings
    void initSettings();

    // Draw the WidgetPong screensaver ToDo: Just draw the buffer, sending to display will managed seperately
    void drawScreensaver();
};