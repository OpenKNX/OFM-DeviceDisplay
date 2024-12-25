#include "Starfield.h"
#include "OpenKNX.h"

// Constructor
WidgetStarfield::WidgetStarfield(uint32_t displayTime, WidgetFlags action, uint8_t intensity)
    : _displayTime(displayTime), _action(action), _intensity(intensity), _state(WidgetState::STOPPED), _lastUpdateTime(0), _display(nullptr)
{
    memset(stars, 0, sizeof(stars));
}

void WidgetStarfield::setup()
{
    logInfoP("Setting up Starfield Widget...");

    if (_display == nullptr)
    {
        // logErrorP("WidgetStarfield: Display is NULL.");
        logInfoP("Display is NULL!");

        return;
    }
    // logDebugP("WidgetStarfield: Setup completed.");
    logInfoP("Setup completed.");
}

void WidgetStarfield::start()
{
    if (_state == WidgetState::RUNNING) return;
    // logDebugP("WidgetStarfield: Starting...");
    logInfoP("Starting...");

    _state = WidgetState::RUNNING;
    initializeStars();
    _lastUpdateTime = millis();
}

void WidgetStarfield::stop()
{
    _state =WidgetState::STOPPED;
    if (_display)
    {
        // logDebugP("WidgetStarfield: Stop...");
        logInfoP("Stopping...");
        _display->display->clearDisplay();
        _display->displayBuff();
    }
}

void WidgetStarfield::pause()
{
    if (_state == WidgetState::RUNNING)
    {
        // logDebugP("WidgetStarfield: Pausing...");
        logInfoP("Pausing...");
        _state = WidgetState::PAUSED;
    }
}

void WidgetStarfield::resume()
{
    if (_state == WidgetState::PAUSED)
    {
        // logDebugP("WidgetStarfield: Resuming...");
        logInfoP("Resuming...");
        _state = WidgetState::RUNNING;
    }
}

void WidgetStarfield::loop()
{
    if (_state != WidgetState::RUNNING || !_display) return;

    const uint32_t UPDATE_INTERVAL = map(_intensity, 1, 10, 100, 20); // Control speed. The _intensity value is between 1 and 10.
    uint32_t currentTime = millis();

    if (currentTime - _lastUpdateTime >= UPDATE_INTERVAL)
    {
        updateStars();
        drawStars();
        _lastUpdateTime = currentTime;
    }
}

uint32_t WidgetStarfield::getDisplayTime() const { return _displayTime; }
WidgetFlags WidgetStarfield::getAction() const { return _action; }

void WidgetStarfield::setDisplayModule(i2cDisplay *displayModule) { _display = displayModule; }
i2cDisplay *WidgetStarfield::getDisplayModule() const { return _display; }

void WidgetStarfield::initializeStars()
{
    logInfoP("Initializing stars...");
    for (int i = 0; i < MAX_STARS; i++)
    {
        stars[i] = {static_cast<float>(random(-100, 100)), static_cast<float>(random(-100, 100)), static_cast<float>(random(1, 100))};
    }
}

void WidgetStarfield::updateStars()
{
    for (int i = 0; i < MAX_STARS; i++)
    {
        stars[i].z -= 2; // Move the stars closer

        if (stars[i].z <= 0)
        {
            // Reset star to "far away"
            stars[i].z = 100;
            stars[i].x = random(-100, 100);
            stars[i].y = random(-100, 100);
        }
    }
}

void WidgetStarfield::drawStars()
{
    const uint16_t SCREEN_WIDTH = _display->GetDisplayWidth();
    const uint16_t SCREEN_HEIGHT = _display->GetDisplayHeight();
    const uint16_t CENTER_X = SCREEN_WIDTH / 2;
    const uint16_t CENTER_Y = SCREEN_HEIGHT / 2;

    _display->display->clearDisplay();

    for (int i = 0; i < MAX_STARS; i++)
    {
        float projectionFactor = 100.0 / stars[i].z;
        int16_t screenX = stars[i].x * projectionFactor + CENTER_X;
        int16_t screenY = stars[i].y * projectionFactor + CENTER_Y;

        if (screenX >= 0 && screenX < SCREEN_WIDTH && screenY >= 0 && screenY < SCREEN_HEIGHT)
        {
            _display->display->drawPixel(screenX, screenY, WHITE);
        }
    }

    _display->displayBuff();
}