#pragma once
#include "../Widget.h"

// WidgetMatrix class: Simulates a matrix-style "rainfall" screensaver
class WidgetMatrix : public Widget
{
  public:
    const std::string logPrefix() { return "WidgetMatrix"; }
    WidgetMatrix(uint32_t displayTime, WidgetsAction action, uint8_t intensity);

    void start() override;  // Start the screensaver
    void stop() override;   // Stop the screensaver
    void pause() override;  // Pause the screensaver
    void resume() override; // Resume the screensaver
    void setup() override;  // Setup widget resources
    void loop() override;   // Update the screensaver animation

    uint32_t getDisplayTime() const override;
    WidgetsAction getAction() const override;

    void setDisplayModule(i2cDisplay *displayModule) override;
    i2cDisplay *getDisplayModule() const override;

  private:
    uint32_t _displayTime;         // Duration to display the widget
    WidgetsAction _action;         // Widget action
    uint8_t _intensity;            // Speed/intensity of the animation
    i2cDisplay *_display;          // Display module
    unsigned long _lastUpdateTime; // Last update time for frame rate control

    // Matrix screensaver configuration
    static constexpr uint8_t MAX_TAIL_LENGTH = 10; // Maximum length of the "tail" of the raindrops
    uint8_t _updateInterval;

    int8_t *_columnHeads;  // Head of the column (top)
    uint8_t *_columnLengths; // Length of the column (tail)

    void initMatrix();     // Initialize matrix settings
    void updateMatrix();   // Update and draw the matrix animation
    void resetColumn(uint8_t col); // Reset a column's parameters
};