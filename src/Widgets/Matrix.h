#pragma once
#include "../Widget.h"

// WidgetMatrix class: Simulates a matrix-style "rainfall" screensaver
class WidgetMatrix : public Widget
{
  public:
    const std::string logPrefix() { return "WidgetMatrix"; }
    WidgetMatrix(uint32_t displayTime, WidgetsAction action, uint8_t intensity);

    void start() override;                                                  // Start widget
    void stop() override;                                                   // Stop widget
    void pause() override;                                                  // Pause widget
    void resume() override;                                                 // Resume widget
    void setup() override;                                                  // Setup widget
    void loop() override;                                                   // Update and draw the widget
    inline const WidgetState getState() const override { return _state; }   // Get the current state of the widget
    inline const std::string getName() const override { return _name; }     // Return the name of the widget
    inline void setName(const std::string &name) override { _name = name; } // Set the name of the widget

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
    std::string _name = "Matrix";  // Name of the widget
    WidgetState _state;            // Current state

    // Matrix screensaver configuration
    static constexpr uint8_t MAX_TAIL_LENGTH = 10; // Maximum length of the "tail" of the raindrops
    uint8_t _updateInterval;

    int8_t *_columnHeads;    // Head of the column (top)
    uint8_t *_columnLengths; // Length of the column (tail)

    void initMatrix();             // Initialize matrix settings
    void updateMatrix();           // Update and draw the matrix animation
    void resetColumn(uint8_t col); // Reset a column's parameters
};