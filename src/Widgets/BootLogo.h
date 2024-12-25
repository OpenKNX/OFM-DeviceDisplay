#pragma once
#include "../Widget.h"

class WidgetBootLogo : public Widget
{
  public:
    const std::string logPrefix() { return "WidgetBootLogo"; }
    WidgetBootLogo(uint32_t displayTime, WidgetFlags action);

    void start() override;                                                  // Start widget
    void stop() override;                                                   // Stop widget
    void pause() override;                                                  // Pause widget
    void resume() override;                                                 // Resume widget
    void setup() override;                                                  // Setup widget
    void loop() override;                                                   // Update and draw the widget
    inline const WidgetState getState() const override { return _state; }   // Get the current state of the widget
    inline const std::string getName() const override { return _name; }     // Return the name of the widget
    inline void setName(const std::string &name) override { _name = name; } // Set the name of the widget

    uint32_t getDisplayTime() const override;                                                 // Return display duration in ms
    WidgetFlags getAction() const override;                                                   // Return widget action
    inline void setDisplayTime(uint32_t displayTime) override { _displayTime = displayTime; } // Set display time

    inline void setAction(uint8_t action) override { _action = static_cast<WidgetFlags>(action); }               // Set the widget action
    inline void addAction(uint8_t action) override { _action = static_cast<WidgetFlags>(_action | action); }     // Add an action to the widget
    inline void removeAction(uint8_t action) override { _action = static_cast<WidgetFlags>(_action & ~action); } // Remove an action from the widget

    void setDisplayModule(i2cDisplay *displayModule) override; // Set display module
    i2cDisplay *getDisplayModule() const override;             // Get display module

  private:
    void drawBootLogo(); // Draw the boot logo on the display

    WidgetState _state;             // BootLogo state
    uint32_t _displayTime;          // Display time in ms
    WidgetFlags _action;            // Widget action
    i2cDisplay *_display;           // Display object
    bool _needsRedraw;              // Redraw flag
    std::string _name = "BootLogo"; // Name of the widget

    /** state of partial drawing:
     * 0=no drawing / done,
     * 1=clear,
     * 2=partial draw image,
     * 3=start sending to display,
     */
    uint8_t _drawStep = 0; // TODO check using enum

    /** Number of rows to draw in one loop() call */
    const uint8_t _stepHeight = 8; // TODO find "best" size (note: 8 results in ~1.6ms max loop time)

    /** Row to start drawing in one loop() call */
    uint8_t _yStart = 0;
};