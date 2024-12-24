#include "Matrix.h"
#include "OpenKNX.h"

WidgetMatrix::WidgetMatrix(uint32_t displayTime, WidgetFlags action, uint8_t intensity)
    : _displayTime(displayTime), _action(action), _intensity(intensity),
      _display(nullptr), _lastUpdateTime(0), _state(WidgetState::STOPPED)
{
    _updateInterval = map(_intensity, 1, 10, 150, 30);
}

WidgetMatrix::~WidgetMatrix()
{
    logInfoP("Test - descructor called");
    delete[] _columnHeads;
    delete[] _columnLengths;
    delete[] _columnSpeeds;
}

void WidgetMatrix::setup()
{
    logInfoP("Setting up Matrix widget...");
    if (!_display)
    {
        logErrorP("Display is NULL.");
        return;
    }

    uint8_t displayWidth = _display->GetDisplayWidth();
    _columnHeads = new int8_t[displayWidth];
    _columnLengths = new uint8_t[displayWidth];
    _columnSpeeds = new uint8_t[displayWidth];
    memset(_columnHeads, 0, displayWidth * sizeof(int8_t));
    memset(_columnLengths, 0, displayWidth * sizeof(uint8_t));
    memset(_columnSpeeds, 0, displayWidth * sizeof(uint8_t));

    initMatrix();
}

void WidgetMatrix::start()
{
    logInfoP("Starting Matrix widget...");
    _state = WidgetState::RUNNING;
    _lastUpdateTime = millis();
}

void WidgetMatrix::stop()
{
    logInfoP("Stopping Matrix widget...");
    _state = WidgetState::STOPPED;
    if (_display)
    {
        _display->display->clearDisplay();
        _display->displayBuff();
    }
    //delete[] _columnHeads;
    //delete[] _columnLengths;
    //delete[] _columnSpeeds;
}

void WidgetMatrix::pause()
{
    logInfoP("Pausing Matrix widget...");
    _state = WidgetState::PAUSED;
    _lastUpdateTime = millis();
}

void WidgetMatrix::resume()
{
    logInfoP("Resuming Matrix widget...");
    _state = WidgetState::RUNNING;
    _lastUpdateTime = millis();
}

void WidgetMatrix::loop()
{
    if (!_display || _state != WidgetState::RUNNING) return;

    unsigned long currentTime = millis();
    if (currentTime - _lastUpdateTime < _updateInterval) return;

    _lastUpdateTime = currentTime;

    updateMatrix();
}

uint32_t WidgetMatrix::getDisplayTime() const { return _displayTime; }
WidgetFlags WidgetMatrix::getAction() const { return _action; }

void WidgetMatrix::setDisplayModule(i2cDisplay* displayModule) { _display = displayModule; }
i2cDisplay* WidgetMatrix::getDisplayModule() const { return _display; }

void WidgetMatrix::initMatrix()
{
    uint8_t displayWidth = _display->GetDisplayWidth();
    for (uint8_t col = 0; col < displayWidth; col++)
    {
        resetColumn(col);
    }
}

void WidgetMatrix::updateMatrix()
{
    _display->display->clearDisplay();

    uint8_t displayHeight = _display->GetDisplayHeight();
    uint8_t displayWidth = _display->GetDisplayWidth();

    for (uint8_t col = 0; col < displayWidth; col++)
    {
        int8_t head = _columnHeads[col];
        uint8_t length = _columnLengths[col];
        uint8_t speed = _columnSpeeds[col];

        if (millis() % speed == 0)
        {
            _columnHeads[col]++;
        }

        for (uint8_t offset = 0; offset < length; offset++)
        {
            int8_t y = head - offset;
            if (y < 0) y += displayHeight; // Loop nach oben
            if (y >= 0 && y < displayHeight)
            {
                if (offset % 2 == 0)
                {
                    _display->display->drawPixel(col, y, WHITE);
                }
            }
        }

        if (_columnHeads[col] >= displayHeight)
        {
            _columnHeads[col] = 0;
            resetColumn(col);
        }
    }

    _display->displayBuff();
}

void WidgetMatrix::resetColumn(uint8_t col)
{
    _columnHeads[col] = random(0, _display->GetDisplayHeight() / 4); // Zufällige Startposition
    _columnLengths[col] = random(3, MAX_TAIL_LENGTH);                // Zufällige Tropfenlänge
    _columnSpeeds[col] = random(1, 4);                               // Zufällige Geschwindigkeit
}