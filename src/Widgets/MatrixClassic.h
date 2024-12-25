#pragma once
#include "../Widget.h"

// WidgetMatrixClassic class: Simulates a classic matrix character rain screensaver
class WidgetMatrixClassic : public Widget
{
  public:
    const std::string logPrefix() { return "WidgetMatrixClassic"; }
    WidgetMatrixClassic(uint32_t displayTime, WidgetFlags action, uint8_t intensity);

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
    WidgetFlags getAction() const override;                                                            // Return widget action
    inline void setDisplayTime(uint32_t displayTime) override { _displayTime = displayTime; } // Set display time
    
    inline void setAction(uint8_t action) override { _action = static_cast<WidgetFlags>(action); }      // Set the widget action
    inline void addAction(uint8_t action) override { _action = static_cast<WidgetFlags>(_action | action); }     // Add an action to the widget
    inline void removeAction(uint8_t action) override { _action = static_cast<WidgetFlags>(_action & ~action); } // Remove an action from the widget


    void setDisplayModule(i2cDisplay *displayModule) override;
    i2cDisplay *getDisplayModule() const override;

  private:
    uint32_t _displayTime;
    WidgetFlags _action;
    uint8_t _intensity;
    i2cDisplay *_display;
    std::string _name = "MatrixClassic";
    WidgetState _state; // Current state

    unsigned long _lastUpdateScreenSaver;
    uint16_t FallSpeed; // Speed of drops depending on intensity

    // Matrix drop positions
    static constexpr uint8_t maxColumns = 16; // Maximum number of columns
    static constexpr uint8_t maxDrops = 5;    // Maximum number of drops per column
    static constexpr uint8_t columnWidth = 8; // Width of a column in pixels

    int _MatrixDropPos[maxColumns][maxDrops];
    static const char cp437[]; // Classic CP437 character set

    void initMatrix();
    void updateMatrix();
};