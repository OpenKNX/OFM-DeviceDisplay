#pragma once
#include "../Widget.h"

// WidgetPong class. Is to show a Pong game on the display.

class WidgetPong : public Widget
{
  public:
    const std::string logPrefix() { return "WidgetPong"; }
    WidgetPong(uint32_t displayTime, WidgetFlags action);

    void start() override;                                                  // Start widget
    void stop() override;                                                   // Stop widget
    void pause() override;                                                  // Pause widget
    void resume() override;                                                 // Resume widget
    void setup() override;                                                  // Setup widget
    void loop() override;                                                   // Update and draw the widget
    inline const WidgetState getState() const override { return _state; }   // Get the current state of the widget
    inline const std::string getName() const override { return _name; }     // Return the name of the widget
    inline void setName(const std::string &name) override { _name = name; } // Set the name of the widget

    uint32_t getDisplayTime() const override;                                                            // Return the display time in ms
    WidgetFlags getAction() const override;                                                            // Return the widget action
    inline void setDisplayTime(uint32_t displayTime) override { _displayTime = displayTime; } // Set display time
    
    inline void setAction(uint8_t action) override { _action = static_cast<WidgetFlags>(action); }      // Set the widget action
    inline void addAction(uint8_t action) override { _action = static_cast<WidgetFlags>(_action | action); }     // Add an action to the widget
    inline void removeAction(uint8_t action) override { _action = static_cast<WidgetFlags>(_action & ~action); } // Remove an action from the widget



    void setDisplayModule(i2cDisplay *displayModule) override; // Set the display module
    i2cDisplay *getDisplayModule() const override;             // Get the display module

  private:
    uint32_t _displayTime;        // Time to display the widget in ms
    WidgetFlags _action;        // Widget action
    uint32_t _lastUpdateTime = 0; // Last update time
    WidgetState _state;           // Current state of the widget
    i2cDisplay *_display;         // Display object
    std::string _name = "Pong";   // Name of the widget

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