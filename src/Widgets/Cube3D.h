#pragma once
#include "../Widget.h"

// WidgetCube3D class: Displays a 3D rotating cube on the display
class WidgetCube3D : public Widget
{
  public:
    const std::string logPrefix() { return "WidgetCube3D"; }
    WidgetCube3D(uint32_t displayTime, WidgetsAction action);

    void start() override;                                                  // Start widget
    void stop() override;                                                   // Stop widget
    void pause() override;                                                  // Pause widget
    void resume() override;                                                 // Resume widget
    void setup() override;                                                  // Setup widget
    void loop() override;                                                   // Update and draw the widget
    inline const WidgetState getState() const override { return _state; }   // Get the current state of the widget
    inline const std::string getName() const override { return _name; }     // Return the name of the widget
    inline void setName(const std::string &name) override { _name = name; } // Set the name of the widget

    uint32_t getDisplayTime() const override;                                                            // Return display duration in ms
    WidgetsAction getAction() const override;                                                            // Return widget action
    inline uint32_t setDisplayTime(uint32_t displayTime) override { return _displayTime = displayTime; } // Set display time

    inline void setAction(uint8_t action) override { _action = static_cast<WidgetsAction>(action); }      // Set the widget action
    inline void addAction(uint8_t action) override { _action = static_cast<WidgetsAction>(_action | action); }     // Add an action to the widget
    inline void removeAction(uint8_t action) override { _action = static_cast<WidgetsAction>(_action & ~action); } // Remove an action from the widget


    void setDisplayModule(i2cDisplay *displayModule) override;
    i2cDisplay *getDisplayModule() const override;

  private:
    WidgetState _state;           // State of the widget
    uint32_t _displayTime;        // Display duration
    WidgetsAction _action;        // Action associated with widget
    i2cDisplay *_display;         // Display object
    uint32_t _lastUpdateTime;     // Last frame time
    std::string _name = "Cube3D"; // Name of the widget

    static constexpr unsigned long UPDATE_INTERVAL = 50; // Frame rate (50ms per frame)
    static constexpr int CUBE_SIZE = 20;                 // Cube size
    float angleX = 0.0, angleY = 0.0;                    // Rotation angles

    // Cube vertices and edges
    static const int8_t cubeVertices[8][3];
    static const uint8_t cubeEdges[12][2];

    void updateCube(); // Update and draw the cube
};