#pragma once
#include "../Widget.h"

// Delay for blinking the "ProgMode" message
#define PROG_MODE_BLINK_DELAY 500

class WidgetProgMode : public Widget
{
  public:
    const std::string logPrefix() { return "WidgetProgMode"; }

    WidgetProgMode(uint32_t displayTime = 0, WidgetFlags action = WidgetFlags::NoAction);
    ~WidgetProgMode();

    void setup() override;                                                // Widget setup
    void start() override;                                                // Start widget
    void stop() override;                                                 // Stop widget
    void pause() override;                                                // Pause widget
    void resume() override;                                               // Resume widget
    void loop() override;                                                 // Widget loop
    inline const WidgetState getState() const override { return _state; } // Get widget state
    inline uint32_t getDisplayTime() const override { return _displayTime; }
    inline WidgetFlags getAction() const override { return _action; }
    inline void setDisplayTime(uint32_t displayTime) override { _displayTime = displayTime; }

    inline void setAction(uint8_t action) override { _action = static_cast<WidgetFlags>(action); }               // Set the widget action
    inline void addAction(uint8_t action) override { _action = static_cast<WidgetFlags>(_action | action); }     // Add an action to the widget
    inline void removeAction(uint8_t action) override { _action = static_cast<WidgetFlags>(_action & ~action); } // Remove an action from the widget

    inline void setDisplayModule(i2cDisplay *displayModule) override { _display = displayModule; }
    inline i2cDisplay *getDisplayModule() const override { return _display; }
    inline const std::string getName() const override { return _name; }
    inline void setName(const std::string &name) override { _name = name; }

  private:
    i2cDisplay *_display;           // Display module pointer
    WidgetFlags _action;            // Action flags
    uint32_t _displayTime;          // Time to display the widget
    WidgetState _state;             // Widget state
    std::string _name = "ProgMode"; // Widget name
    unsigned long _lastBlinkTime;   // Last blink time
    bool _showProgMode;             // Current blink state

    void draw(); // Draws the program mode content on the display
};