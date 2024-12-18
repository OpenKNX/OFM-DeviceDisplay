#include "Rain.h"
#include "OpenKNX.h"

WidgetRain::WidgetRain(uint32_t displayTime, WidgetsAction action, uint8_t intensity)
    : _displayTime(displayTime), _action(action), _intensity(intensity), _display(nullptr), _lastUpdateScreenSaver(0), _initialized(false)
{
    UPDATE_INTERVAL = map(_intensity, 1, 10, 50, 10);        // Speed: higher intensity = faster rain
    _dropCount = map(_intensity, 1, 10, 10, MAX_RAIN_DROPS); // More drops with higher intensity
}

void WidgetRain::setup()
{
    logInfoP("Setting up Rain Widget...");
    //openknx.logger.log(logPrefix() + ": Setting up Rain Widget...");
    if (_display == nullptr)
    {
        // logErrorP("WidgetRain: Display is NULL.");
        openknx.logger.log(logPrefix() + ": Display is NULL.");
        return;
    }
    initRain();
}

void WidgetRain::start()
{
    _state = WidgetState::RUNNING;
    logInfoP("Starting Rain Widget...");
    initRain();
}

void WidgetRain::stop()
{
    _state = WidgetState::STOPPED;
    logInfoP("Stopping Rain Widget...");
    if (_display)
    {
        _display->display->clearDisplay();
        _display->displayBuff();
    }
}

void WidgetRain::pause()
{
    _state = WidgetState::PAUSED;
    logInfoP("Pausing Rain Widget...");
}

void WidgetRain::resume()
{
    _state = WidgetState::RUNNING;
    logInfoP("Resuming Rain Widget...");
}

void WidgetRain::loop()
{
    if (!_display || _state != WidgetState::RUNNING)
        return;

    unsigned long currentTime = millis();
    if (currentTime - _lastUpdateScreenSaver < UPDATE_INTERVAL)
        return; // Wait for the update interval

    _lastUpdateScreenSaver = currentTime; // Reset timer
    updateRain();
}

uint32_t WidgetRain::getDisplayTime() const { return _displayTime; }
WidgetsAction WidgetRain::getAction() const { return _action; }

void WidgetRain::setDisplayModule(i2cDisplay *displayModule) { _display = displayModule; }
i2cDisplay *WidgetRain::getDisplayModule() const { return _display; }

void WidgetRain::initRain()
{
    logInfoP("Initializing Rain Widget...");
    for (uint8_t i = 0; i < MAX_RAIN_DROPS; i++)
    {
        _dropsX[i] = random(_display->GetDisplayWidth());
        _dropsY[i] = random(_display->GetDisplayHeight());
    }
    _initialized = true;
}

void WidgetRain::updateRain()
{
    _display->display->clearDisplay();

    for (uint8_t i = 0; i < _dropCount; i++)
    {
        // Draw the raindrop
        _display->display->drawPixel(_dropsX[i], _dropsY[i], WHITE);

        // Move the raindrop down
        _dropsY[i]++;

        // Reset the raindrop if it reaches the bottom of the screen
        if (_dropsY[i] >= _display->GetDisplayHeight())
        {
            _dropsX[i] = random(_display->GetDisplayWidth());
            _dropsY[i] = 0;
        }
    }

    _display->displayBuff(); // Refresh display
}