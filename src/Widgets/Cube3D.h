#pragma once
#include "../Widget.h"

// WidgetCube3D class: Displays a 3D rotating cube on the display
class WidgetCube3D : public Widget
{
  public:
    WidgetCube3D(uint32_t displayTime, WidgetsAction action);

    void start() override;  // Start the 3D cube animation
    void stop() override;   // Stop the animation and clear display
    void pause() override;  // Pause the cube rotation
    void resume() override; // Resume the animation
    void setup() override;  // Setup the widget
    void loop() override;   // Update and draw the cube

    uint32_t getDisplayTime() const override;
    WidgetsAction getAction() const override;

    void setDisplayModule(i2cDisplay *displayModule) override;
    i2cDisplay *getDisplayModule() const override;

  private:
    enum WidgetCube3DState
    {
        STOPPED,
        RUNNING,
        PAUSED
    };

    WidgetCube3DState _state; // State of the widget
    uint32_t _displayTime;    // Display duration
    WidgetsAction _action;    // Action associated with widget
    i2cDisplay *_display;     // Display object
    uint32_t _lastUpdateTime; // Last frame time

    static constexpr unsigned long UPDATE_INTERVAL = 50; // Frame rate (50ms per frame)
    static constexpr int CUBE_SIZE = 20;                // Cube size
    float angleX = 0.0, angleY = 0.0;                   // Rotation angles

    // Cube vertices and edges
    static const int8_t cubeVertices[8][3];
    static const uint8_t cubeEdges[12][2];

    void updateCube(); // Update and draw the cube
};