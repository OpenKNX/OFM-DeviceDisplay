#include "Clock.h"
#include "OpenKNX.h"

WidgetClock::WidgetClock(uint32_t displayTime, WidgetFlags action, bool roundedClock)
    : _displayTime(displayTime), _action(action), _state(WidgetState::STOPPED), _display(nullptr),
      _roundedClock(roundedClock), _lastUpdateTime(0) {}

void WidgetClock::setDisplayModule(i2cDisplay *displayModule)
{
    _display = displayModule;
}

i2cDisplay *WidgetClock::getDisplayModule() const { return _display; }

void WidgetClock::setup()
{
    if (_state == WidgetState::RUNNING) return;

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
WidgetFlags WidgetClock::getAction() const { return _action; }

void WidgetClock::fetchTime(uint16_t &days, uint16_t &hours, uint16_t &minutes, uint16_t &seconds)
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
    uptimeSecs /= 24;
    days = uptimeSecs;
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
    const uint16_t CENTER_Y = (SCREEN_HEIGHT / 2) + 10;
    const uint16_t CLOCK_RADIUS = min(SCREEN_WIDTH, SCREEN_HEIGHT) / 2 - 5;

    uint16_t days=0, hours=0, minutes=0, seconds=0;
    fetchTime(days, hours, minutes, seconds);

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

        /*
        // 12, 3, 6 und 9 Markierungen zeichnen
        int markLength = 3; // Länge der Markierungslinien

        // 12 Uhr (oben)
        _display->display->drawLine(CENTER_X, CENTER_Y - CLOCK_RADIUS,
                                    CENTER_X, CENTER_Y - CLOCK_RADIUS + markLength, WHITE);

        // 3 Uhr (rechts)
        _display->display->drawLine(CENTER_X + CLOCK_RADIUS, CENTER_Y,
                                    CENTER_X + CLOCK_RADIUS - markLength, CENTER_Y, WHITE);

        // 6 Uhr (unten)
        _display->display->drawLine(CENTER_X, CENTER_Y + CLOCK_RADIUS,
                                    CENTER_X, CENTER_Y + CLOCK_RADIUS - markLength, WHITE);

        // 9 Uhr (links)
        _display->display->drawLine(CENTER_X - CLOCK_RADIUS, CENTER_Y,
                                    CENTER_X - CLOCK_RADIUS + markLength, CENTER_Y, WHITE);
        */
    }
    else
    {
        // Draw digital clock
        uint8_t TextSize = 2;
        uint8_t FONT_HEIGHT = 8 * TextSize, FONT_WIDTH = 6 * TextSize;
        char timeString[9];
        if (days > 0)
            snprintf(timeString, sizeof(timeString), "%02u %02u:%02u:%02u", days, hours, minutes, seconds);
        else
            snprintf(timeString, sizeof(timeString), "%02u:%02u:%02u", hours, minutes, seconds);

        int16_t startX = (SCREEN_WIDTH - strlen(timeString) * FONT_WIDTH) / 2;
        int16_t startY = (SCREEN_HEIGHT - FONT_HEIGHT) / 2;

        _display->display->setCursor(startX, startY - 4);
        _display->display->setTextSize(TextSize);
        _display->display->setTextColor(WHITE);
        _display->display->print(timeString);

// Draw device ID on top
#ifdef DEVICE_ID
        TextSize = 1;
        const char *deviceId = DEVICE_ID;
        FONT_WIDTH = 6 * TextSize;
        _display->display->setTextWrap(false);
        _display->display->setCursor((SCREEN_WIDTH - strlen(deviceId) * FONT_WIDTH) / 2, 0);
        _display->display->setTextSize(TextSize);
        _display->display->print(deviceId);
#endif

        // Draw the KNX address with configuration status (vorletzte Zeile)
        const char *knxAddress = openknx.info.humanIndividualAddress().c_str();
        const char *knxStatus = knx.configured() ? "" : " [!]";
        char knxString[30]; // Buffer für die KNX-Adresse + Status
        snprintf(knxString, sizeof(knxString), "Addr.: %s%s", knxAddress, knxStatus);

        // Berechnung der tatsächlichen Pixelbreite für genauere Zentrierung
        startX = (SCREEN_WIDTH - (strlen(knxString)) * FONT_WIDTH) / 2;
        startY = SCREEN_HEIGHT - 18; // Vorletzte Zeile

        _display->display->setCursor(startX, startY);
        _display->display->print(knxString);

        // Draw Serial number on bottom (letzte Zeile)
        const char *deviceSN = openknx.info.humanSerialNumber().c_str();
        startX = (SCREEN_WIDTH - strlen(deviceSN) * FONT_WIDTH) / 2;
        startY = SCREEN_HEIGHT - 8; // Letzte Zeile
        _display->display->setCursor(startX, startY);
        _display->display->print(deviceSN);
    }

    _display->displayBuff();
    logDebugP("WidgetClock: Clock updated.");
}