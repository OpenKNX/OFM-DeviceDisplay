#include "Cube3D.h"

// Define the cube vertices
const int8_t WidgetCube3D::cubeVertices[8][3] = {
    {-1, -1, -1}, {1, -1, -1}, {1, 1, -1}, {-1, 1, -1}, {-1, -1, 1}, {1, -1, 1}, {1, 1, 1}, {-1, 1, 1}};

// Define the cube edges
const uint8_t WidgetCube3D::cubeEdges[12][2] = {
    {0, 1}, {1, 2}, {2, 3}, {3, 0}, // Front face
    {4, 5}, {5, 6}, {6, 7}, {7, 4}, // Back face
    {0, 4}, {1, 5}, {2, 6}, {3, 7}  // Connections between front and back faces
};

WidgetCube3D::WidgetCube3D(uint32_t displayTime, WidgetsAction action)
    : _displayTime(displayTime), _action(action), _state(STOPPED), _lastUpdateTime(0), _display(nullptr) {}

void WidgetCube3D::setup() {}

void WidgetCube3D::start()
{
    if (_state == RUNNING) return;

    _state = RUNNING;
    _lastUpdateTime = millis();
    angleX = 0.0;
    angleY = 0.0;
}

void WidgetCube3D::stop()
{
    _state = STOPPED;
    if (_display)
    {
        _display->display->clearDisplay();
        _display->displayBuff();
    }
}

void WidgetCube3D::pause()
{
    if (_state == RUNNING)
    {
        _state = PAUSED;
    }
}

void WidgetCube3D::resume()
{
    if (_state == PAUSED)
    {
        _state = RUNNING;
    }
}

void WidgetCube3D::loop()
{
    if (_state != RUNNING || !_display) return;

    uint32_t currentTime = millis();
    if (currentTime - _lastUpdateTime < UPDATE_INTERVAL)
    {
        return; // Maintain frame rate
    }
    _lastUpdateTime = currentTime;

    updateCube();
}

uint32_t WidgetCube3D::getDisplayTime() const { return _displayTime; }
WidgetsAction WidgetCube3D::getAction() const { return _action; }

void WidgetCube3D::setDisplayModule(i2cDisplay *displayModule) { _display = displayModule; }
i2cDisplay *WidgetCube3D::getDisplayModule() const { return _display; }

void WidgetCube3D::updateCube()
{
    const uint16_t SCREEN_WIDTH = _display->GetDisplayWidth();
    const uint16_t SCREEN_HEIGHT = _display->GetDisplayHeight();
    const int CENTER_X = SCREEN_WIDTH / 2;
    const int CENTER_Y = SCREEN_HEIGHT / 2;

    // Clear display
    _display->display->clearDisplay();

    // Projected vertices
    float projectedVertices[8][2];

    for (int i = 0; i < 8; i++)
    {
        float x = cubeVertices[i][0] * CUBE_SIZE;
        float y = cubeVertices[i][1] * CUBE_SIZE;
        float z = cubeVertices[i][2] * CUBE_SIZE;

        // Rotation around the X-axis
        float tempY = y * cos(angleX) - z * sin(angleX);
        float tempZ = y * sin(angleX) + z * cos(angleX);
        y = tempY;
        z = tempZ;

        // Rotation around the Y-axis
        float tempX = x * cos(angleY) + z * sin(angleY);
        z = -x * sin(angleY) + z * cos(angleY);
        x = tempX;

        // Perspective projection
        float distance = 50.0f;
        float projectionFactor = distance / (distance - z);
        projectedVertices[i][0] = x * projectionFactor + CENTER_X;
        projectedVertices[i][1] = y * projectionFactor + CENTER_Y;
    }

    // Draw cube edges
    for (int i = 0; i < 12; i++)
    {
        int start = cubeEdges[i][0];
        int end = cubeEdges[i][1];
        _display->display->drawLine(
            projectedVertices[start][0], projectedVertices[start][1],
            projectedVertices[end][0], projectedVertices[end][1], WHITE);
    }

    _display->displayBuff();

    // Update rotation angles
    angleX += 0.05;
    angleY += 0.03;
}