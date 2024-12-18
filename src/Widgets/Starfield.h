#pragma once
#include "../Widget.h"

// WidgetStarfield class: Displays a starfield screensaver.
class WidgetStarfield : public Widget
{
  public:
    const std::string logPrefix() { return "WidgetStarfield"; }
    WidgetStarfield(uint32_t displayTime, WidgetsAction action, uint8_t intensity);

    void start() override;  // Start the starfield widget
    void stop() override;   // Stop the starfield screensaver
    void pause() override;  // Pause the starfield
    void resume() override; // Resume the starfield
    void setup() override;  // Setup the widget
    void loop() override;   // Update and draw the starfield

    uint32_t getDisplayTime() const override; // Return display time
    WidgetsAction getAction() const override; // Return widget action

    void setDisplayModule(i2cDisplay *displayModule) override; // Set display module
    i2cDisplay *getDisplayModule() const override;

  private:
    struct Star
    {
        float x, y, z;
    };

    WidgetState _state; // Current state
    uint32_t _displayTime;       // Display time
    WidgetsAction _action;       // Widget action
    i2cDisplay *_display;        // Display module
    uint32_t _lastUpdateTime;    // Last update time
    uint8_t _intensity;          // Intensity of the starfield

    static const int MAX_STARS = 100; // Maximum number of stars
    Star stars[MAX_STARS];            // Array of stars

    void initializeStars(); // Initialize the stars
    void updateStars();     // Update star positions
    void drawStars();       // Draw the stars on display
};