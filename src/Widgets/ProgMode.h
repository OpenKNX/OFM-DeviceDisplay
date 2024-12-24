#pragma once
#include "../Widget.h"

class WidgetProgMode : public Widget
{
  public:
    const std::string logPrefix() { return "WidgetProgMode"; }

    WidgetProgMode(uint32_t displayTime = 0, WidgetsAction action = WidgetsAction::NoAction);
    ~WidgetProgMode();

    void setup() override;                                                // Widget setup
    void start() override;                                                // Start widget
    void stop() override;                                                 // Stop widget
    void pause() override;                                                // Pause widget
    void resume() override;                                               // Resume widget
    void loop() override;                                                 // Widget loop
    inline const WidgetState getState() const override { return _state; } // Get widget state
    inline uint32_t getDisplayTime() const override { return _displayTime; }
    inline WidgetsAction getAction() const override { return _action; }
    inline uint32_t setDisplayTime(uint32_t displayTime) override { return _displayTime = displayTime; }

    inline void setAction(uint8_t action) override { _action = static_cast<WidgetsAction>(action); }               // Set the widget action
    inline void addAction(uint8_t action) override { _action = static_cast<WidgetsAction>(_action | action); }     // Add an action to the widget
    inline void removeAction(uint8_t action) override { _action = static_cast<WidgetsAction>(_action & ~action); } // Remove an action from the widget

    inline void setDisplayModule(i2cDisplay *displayModule) override { _display = displayModule; }
    inline i2cDisplay *getDisplayModule() const override { return _display; }
    inline const std::string getName() const override { return _name; }
    inline void setName(const std::string &name) override { _name = name; }

  private:
    i2cDisplay *_display;           // Display module pointer
    WidgetsAction _action;          // Action flags
    uint32_t _displayTime;          // Time to display the widget
    WidgetState _state;             // Widget state
    std::string _name = "ProgMode"; // Widget name
    unsigned long _lastBlinkTime;   // Last blink time
    bool _showProgMode;             // Current blink state

    void draw(); // Draws the program mode content on the display
};