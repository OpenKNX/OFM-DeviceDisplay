#pragma once
#include "../Widget.h"

class WidgetOpenKNXLogo : public Widget
{
  public:
    const std::string logPrefix() { return "WidgetOpenKNXLogo"; }
    WidgetOpenKNXLogo(uint32_t displayTime, WidgetFlags action);

    void setup() override;
    void start() override;
    void stop() override;
    void pause() override;
    void resume() override;
    void loop() override;
    inline const WidgetState getState() const override { return _state; }   // Get the current state of the widget
    inline const std::string getName() const override { return _name; }     // Return the name of the widget
    inline void setName(const std::string &name) override { _name = name; } // Set the name of the widget

    uint32_t getDisplayTime() const override;                                                            // Return display time
    WidgetFlags getAction() const override;                                                            // Return widget action
    inline void setDisplayTime(uint32_t displayTime) override { _displayTime = displayTime; } // Set display time
    
    inline void setAction(uint8_t action) override { _action = static_cast<WidgetFlags>(action); }      // Set the widget action
    inline void addAction(uint8_t action) override { _action = static_cast<WidgetFlags>(_action | action); }     // Add an action to the widget
    inline void removeAction(uint8_t action) override { _action = static_cast<WidgetFlags>(_action & ~action); } // Remove an action from the widget


    void setDisplayModule(i2cDisplay *displayModule) override; // Set display module
    i2cDisplay *getDisplayModule() const override;             // Get display module

  private:
    void drawOpenKNXLogo();

    WidgetState _state;                  // OpenKNXLogo state
    uint32_t _displayTime;               // Display time in ms
    WidgetFlags _action;               // Widget action
    i2cDisplay *_display;                // Display object
    bool _needsRedraw;                   // Redraw flag
    std::string _name = "DefaultWidget"; // Name of the widget

    /** state of partial drawing:
     * 0    = no drawing / done,
     * 1..n = drawing step
     */
    uint8_t _drawStep = 0;

    /** the last rendered uptime */
    String _lastUptime = "";
};