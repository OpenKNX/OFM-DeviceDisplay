#pragma once
#include "../Widget.h"

// WidgetStarfield class: Displays a starfield screensaver.
class WidgetStarfield : public Widget
{
  public:
    const std::string logPrefix() { return "WidgetStarfield"; }
    WidgetStarfield(uint32_t displayTime, WidgetsAction action, uint8_t intensity);

    void start() override;                                                  // Start widget
    void stop() override;                                                   // Stop widget
    void pause() override;                                                  // Pause widget
    void resume() override;                                                 // Resume widget
    void setup() override;                                                  // Setup widget
    void loop() override;                                                   // Update and draw the widget
    inline const WidgetState getState() const override { return _state; }   // Get the current state of the widget
    inline const std::string getName() const override { return _name; }     // Return the name of the widget
    inline void setName(const std::string &name) override { _name = name; } // Set the name of the widget

    uint32_t getDisplayTime() const override;                                                             // Return display time
    WidgetsAction getAction() const override;                                                             // Return widget action
    inline uint32_t setDisplayTime(uint32_t displayTime) override { return _displayTime = displayTime; }  // Set display time

    inline void setAction(uint8_t action) override { _action = static_cast<WidgetsAction>(action); }      // Set the widget action
    inline void addAction(uint8_t action) override { _action = static_cast<WidgetsAction>(_action | action); }     // Add an action to the widget
    inline void removeAction(uint8_t action) override { _action = static_cast<WidgetsAction>(_action & ~action); } // Remove an action from the widget


    void setDisplayModule(i2cDisplay *displayModule) override; // Set display module
    i2cDisplay *getDisplayModule() const override;

  private:
    struct Star
    {
        float x, y, z;
    };

    WidgetState _state;              // Current state
    uint32_t _displayTime;           // Display time
    WidgetsAction _action;           // Widget action
    i2cDisplay *_display;            // Display module
    uint32_t _lastUpdateTime;        // Last update time
    uint8_t _intensity;              // Intensity of the starfield
    std::string _name = "Starfield"; // Name of the widget

    static const int MAX_STARS = 100; // Maximum number of stars
    Star stars[MAX_STARS];            // Array of stars

    void initializeStars(); // Initialize the stars
    void updateStars();     // Update star positions
    void drawStars();       // Draw the stars on display
};