#include "Matrix.h"

WidgetMatrix::WidgetMatrix(uint32_t displayTime, WidgetsAction action, uint8_t intensity)
    : _displayTime(displayTime), _action(action), _intensity(intensity), 
      _display(nullptr), _lastUpdateTime(0), _state(WidgetState::STOPPED)
{
    _updateInterval = map(_intensity, 1, 10, 150, 30); // Map intensity to update speed
    _columnHeads = new int8_t[_display->GetDisplayWidth()];
    _columnLengths = new uint8_t[_display->GetDisplayWidth()];
    memset(_columnHeads, 0, _display->GetDisplayWidth() * sizeof(int8_t));
    memset(_columnLengths, 0, _display->GetDisplayWidth() * sizeof(uint8_t));
}

void WidgetMatrix::setup()
{
    logInfoP("Setup...");
    if (_display == nullptr)
    {
        logErrorP("Display is NULL.");
        return;
    }
    initMatrix();
}

void WidgetMatrix::start()
{
    logInfoP("Start...");
    _state = WidgetState::RUNNING;
    _lastUpdateTime = millis();
    logDebugP("WidgetMatrix: Started.");
}

void WidgetMatrix::stop()
{
    logInfoP("Stop...");
    _state = WidgetState::STOPPED;
    if (_display)
    {
        _display->display->clearDisplay();
        _display->displayBuff();
    }
}

void WidgetMatrix::pause()
{
    logInfoP("Pause...");
    _state = WidgetState::PAUSED;
    _lastUpdateTime = millis();
}

void WidgetMatrix::resume()
{
    logInfoP("Resume...");
    _state = WidgetState::RUNNING;
    _lastUpdateTime = millis();
}

void WidgetMatrix::loop()
{
    if (!_display || _state != WidgetState::RUNNING) return;

    unsigned long currentTime = millis();
    if (currentTime - _lastUpdateTime < _updateInterval)
    {
        return; // Control the frame rate
    }
    _lastUpdateTime = currentTime;

    updateMatrix();
}

uint32_t WidgetMatrix::getDisplayTime() const { return _displayTime; }
WidgetsAction WidgetMatrix::getAction() const { return _action; }

void WidgetMatrix::setDisplayModule(i2cDisplay *displayModule) { _display = displayModule; }
i2cDisplay *WidgetMatrix::getDisplayModule() const { return _display; }

void WidgetMatrix::initMatrix()
{
    for (uint8_t col = 0; col < _display->GetDisplayWidth(); col++)
    {
        resetColumn(col); // Initialize each column
    }
}

void WidgetMatrix::updateMatrix()
{
    _display->display->clearDisplay(); // Clear display before drawing

    for (uint8_t col = 0; col < _display->GetDisplayWidth(); col++)
    {
        int8_t head = _columnHeads[col];
        uint8_t length = _columnLengths[col];

        // Draw the "tail" for this column
        for (uint8_t offset = 0; offset < length; offset++)
        {
            int8_t y = head - offset;
            if (y >= 0 && y < _display->GetDisplayHeight())
            {
                uint8_t brightness = 255 - (offset * (255 / length)); // Brightness fades with offset
                _display->display->drawPixel(col, y, brightness > 128 ? WHITE : BLACK);
            }
        }

        // Move the column head downward
        _columnHeads[col]++;
        if (_columnHeads[col] - length >= _display->GetDisplayHeight())
        {
            resetColumn(col); // Reset the column when it moves off-screen
        }
    }

    _display->displayBuff(); // Update the display
}

void WidgetMatrix::resetColumn(uint8_t col)
{
    _columnHeads[col] = 0;                            // Reset head to top
    _columnLengths[col] = random(3, MAX_TAIL_LENGTH); // Randomize tail length
}