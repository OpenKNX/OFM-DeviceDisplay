#pragma once
#include "../Widget.h"

// WidgetRain class: Simulates falling rain drops as a screensaver
class WidgetRain : public Widget
{
  public:
    const std::string logPrefix() { return "WidgetRain"; }
    WidgetRain(uint32_t displayTime, WidgetsAction action, uint8_t intensity);

    void start() override;                                                  // Start widget
    void stop() override;                                                   // Stop widget
    void pause() override;                                                  // Pause widget
    void resume() override;                                                 // Resume widget
    void setup() override;                                                  // Setup widget
    void loop() override;                                                   // Update and draw the widget
    inline const WidgetState getState() const override { return _state; }   // Get the current state of the widget
    inline const std::string getName() const override { return _name; }     // Return the name of the widget
    inline void setName(const std::string &name) override { _name = name; } // Set the name of the widget

    uint32_t getDisplayTime() const override;                                                            // Return display time
    WidgetsAction getAction() const override;                                                            // Return widget action
    inline uint32_t setDisplayTime(uint32_t displayTime) override { return _displayTime = displayTime; } // Set display time
    
    inline void setAction(uint8_t action) override { _action = static_cast<WidgetsAction>(action); }      // Set the widget action
    inline void addAction(uint8_t action) override { _action = static_cast<WidgetsAction>(_action | action); }     // Add an action to the widget
    inline void removeAction(uint8_t action) override { _action = static_cast<WidgetsAction>(_action & ~action); } // Remove an action from the widget



    void setDisplayModule(i2cDisplay *displayModule) override;
    i2cDisplay *getDisplayModule() const override;

  private:
    WidgetState _state; // Current state
    uint32_t _displayTime;
    WidgetsAction _action;
    uint8_t _intensity;
    i2cDisplay *_display;

    static const uint8_t MAX_RAIN_DROPS = 100;
    unsigned long _lastUpdateScreenSaver;
    unsigned long UPDATE_INTERVAL; // Update speed based on intensity
    uint8_t _dropCount;            // Active raindrop count
    uint8_t _dropsX[MAX_RAIN_DROPS];
    uint8_t _dropsY[MAX_RAIN_DROPS];
    bool _initialized;
    std::string _name = "Rain";

    void initRain();
    void updateRain();
};