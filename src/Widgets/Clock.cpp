#include "Clock.h"

WidgetClock::WidgetClock(uint32_t displayTime, WidgetsAction action, bool roundedClock)
    : _displayTime(displayTime), _action(action), _state(WidgetState::STOPPED), _display(nullptr),
      _roundedClock(roundedClock), _lastUpdateTime(0) {}

void WidgetClock::setDisplayModule(i2cDisplay *displayModule)
{
    _display = displayModule;
}

i2cDisplay *WidgetClock::getDisplayModule() const { return _display; }

void WidgetClock::setup()
{
    if(_state == WidgetState::RUNNING) return;

    logInfoP("Setup...");
    if (_display == nullptr)
    {
        logErrorP("Display is NULL.");
        return;
    }
}

void WidgetClock::start()
{
    if (_state == WidgetState::STOPPED)
    {
        logInfoP("Start...");
        _state = WidgetState::RUNNING;
        _lastUpdateTime = millis();
    }
}

void WidgetClock::stop()
{
    if (_state != WidgetState::STOPPED)
    {
        logInfoP("Stop...");
        _state = WidgetState::STOPPED;
        _display->display->clearDisplay();
        _display->displayBuff();
    }
}

void WidgetClock::pause()
{
    if (_state == WidgetState::RUNNING)
    {
        logInfoP("Pause...");
        _state = WidgetState::PAUSED;
    }
}

void WidgetClock::resume()
{
    if (_state == WidgetState::PAUSED)
    {
        logInfoP("Resume...");
        _state = WidgetState::RUNNING;
        _lastUpdateTime = millis();
    }
}

void WidgetClock::loop()
{
    if (_state != WidgetState::RUNNING) return;

    uint32_t currentTime = millis();
    if (currentTime - _lastUpdateTime >= 1000)
    { // Update every second
        drawClock();
        _lastUpdateTime = currentTime;
    }
}

uint32_t WidgetClock::getDisplayTime() const { return _displayTime; }
WidgetsAction WidgetClock::getAction() const { return _action; }

void WidgetClock::fetchTime(uint16_t &hours, uint16_t &minutes, uint16_t &seconds)
{
    if (openknx.time.isValid())
    {
        auto time = openknx.time.getUtcTime();
        hours = time.hour;
        minutes = time.minute;
        seconds = time.second;
    }
    else
    {
        uint32_t uptimeSecs = uptime();
        seconds = uptimeSecs % 60;
        uptimeSecs /= 60;
        minutes = uptimeSecs % 60;
        uptimeSecs /= 60;
        hours = uptimeSecs % 24;
    }
}

void WidgetClock::drawClock()
{
    if (_display == nullptr)
    {
        logErrorP("WidgetClock: Display is NULL.");
        return;
    }

    const uint16_t SCREEN_WIDTH = _display->GetDisplayWidth();
    const uint16_t SCREEN_HEIGHT = _display->GetDisplayHeight();
    const uint16_t CENTER_X = SCREEN_WIDTH / 2;
    const uint16_t CENTER_Y = SCREEN_HEIGHT / 2;
    const uint16_t CLOCK_RADIUS = min(SCREEN_WIDTH, SCREEN_HEIGHT) / 2 - 5;

    uint16_t hours, minutes, seconds;
    fetchTime(hours, minutes, seconds);

    _display->display->clearDisplay();

    if (_roundedClock)
    {
        // Draw analog clock
        _display->display->drawCircle(CENTER_X, CENTER_Y, CLOCK_RADIUS, WHITE);

        float angleHour = -PI / 2 + (hours % 12 + minutes / 60.0) * (PI / 6);
        float angleMinute = -PI / 2 + (minutes + seconds / 60.0) * (PI / 30);
        float angleSecond = -PI / 2 + seconds * (PI / 30);

        _display->display->drawLine(CENTER_X, CENTER_Y,
                                    CENTER_X + CLOCK_RADIUS * 0.5 * cos(angleHour),
                                    CENTER_Y + CLOCK_RADIUS * 0.5 * sin(angleHour), WHITE);

        _display->display->drawLine(CENTER_X, CENTER_Y,
                                    CENTER_X + CLOCK_RADIUS * 0.8 * cos(angleMinute),
                                    CENTER_Y + CLOCK_RADIUS * 0.8 * sin(angleMinute), WHITE);

        _display->display->drawLine(CENTER_X, CENTER_Y,
                                    CENTER_X + CLOCK_RADIUS * cos(angleSecond),
                                    CENTER_Y + CLOCK_RADIUS * sin(angleSecond), WHITE);
    }
    else
    {
        // Draw digital clock
        const uint8_t FONT_HEIGHT = 8, FONT_WIDTH = 6;
        char timeString[9];
        snprintf(timeString, sizeof(timeString), "%02u:%02u:%02u", hours, minutes, seconds);
        int16_t startX = (SCREEN_WIDTH - strlen(timeString) * FONT_WIDTH) / 2;
        int16_t startY = (SCREEN_HEIGHT - FONT_HEIGHT) / 2;

        _display->display->setCursor(startX, startY);
        _display->display->setTextSize(1);
        _display->display->setTextColor(WHITE);
        _display->display->print(timeString);
    }

    _display->displayBuff();
    logDebugP("WidgetClock: Clock updated.");
}