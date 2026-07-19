#ifdef DEVICE_DISPLAY_MODULE
    /**
     * @file        OTAUpdate.cpp
     * @brief       Full-screen OTA update progress overlay widget (WIDGET_PRIO_SYSTEM takeover)
     * @version     0.0.1
     * @date        2026-07-12
     * @copyright   Copyright (c) 2026, Erkan Çolak (erkan@colak.de)
     *              Licensed under GNU GPL v3.0
     */
    #include "OTAUpdate.h"
    #include "OpenKNX.h"
    #include <cstring> // strlen for centered percent text

WidgetOTA::WidgetOTA(uint32_t displayTime, WidgetFlags action)
    : _display(nullptr), _action(action), _displayTime(displayTime), _state(WidgetState::STOPPED)
{
}

WidgetOTA::~WidgetOTA()
{
}

void WidgetOTA::setup()
{
    if (!_display)
    {
        logErrorP("Display module is NULL.");
        return;
    }
}

void WidgetOTA::start()
{
    logDebugP("start...");
    _state = WidgetState::RUNNING;
    _lastBlinkTime = millis();
    _drawStep = 1; // initial draw next loop
}

void WidgetOTA::stop()
{
    logDebugP("stop...");
    _state = WidgetState::STOPPED;
    if (_display)
    {
        _display->display->clearDisplay();
        _display->displayBuff();
    }
}

void WidgetOTA::pause()
{
    logDebugP("paused...");
    _state = WidgetState::PAUSED;
}

void WidgetOTA::resume()
{
    logDebugP("resume...");
    _state = WidgetState::RUNNING;
    _lastBlinkTime = millis();
    _drawStep = 1; // redraw next loop
}

// Fed by DeviceDisplay each poll; only redraw when the value actually moves.
void WidgetOTA::setProgress(uint8_t percent)
{
    if (percent > 100) percent = 100;
    if (percent == _percent) return;
    _percent = percent;
    if (_state == WidgetState::RUNNING) _drawStep = 1;
}

void WidgetOTA::loop()
{
    if (!_display || _state != WidgetState::RUNNING) return;

    unsigned long currentTime = millis();

    // Toggle the "alive" dot so a stalled percent still looks like work in progress.
    if (currentTime - _lastBlinkTime >= OTA_BLINK_DELAY)
    {
        _lastBlinkTime = currentTime;
        _blinkOn = !_blinkOn;
        _drawStep = 1;
    }
    else if (_drawStep == 0)
    {
        return; // nothing pending
    }

    draw();
}

void WidgetOTA::draw()
{
    if (!_display) return;

    // Partial drawing, one step per loop()-call.
    switch (_drawStep++)
    {
        case 1:
            _display->display->clearDisplay();
            _display->display->setTextColor(SSD1306_WHITE);
            break;
        case 2:
            // Header + activity dot (top-right, blinks).
            _display->display->setTextWrap(false);
            _display->display->setTextSize(1);
            _display->display->setCursor(0, 0);
            _display->display->print("  www.OpenKNX.de");
            if (_blinkOn)
                _display->display->fillRect(120, 0, 6, 6, SSD1306_WHITE);
            break;
        case 3:
            // Title (big).
            _display->display->setTextSize(2);
            _display->display->setCursor(4, 16);
            _display->display->print("OTA UPDATE");
            break;
        case 4:
        {
            // Percent (big, centered).
            const int16_t sw = (int16_t)_display->GetDisplayWidth();
            char buf[8];
            snprintf(buf, sizeof(buf), "%u %%", (unsigned)_percent);
            const int16_t w = (int16_t)(strlen(buf) * 12); // size-2 glyph = 12 px
            _display->display->setTextSize(2);
            _display->display->setCursor((sw - w) / 2, 34);
            _display->display->print(buf);
            break;
        }
        case 5:
        {
            // Progress bar.
            const int16_t sw = (int16_t)_display->GetDisplayWidth();
            const int16_t barX = 14, barW = sw - 28;
            _display->display->drawRect(barX, 56, barW, 7, SSD1306_WHITE);
            if (_percent > 0)
                _display->display->fillRect(barX, 56, (int16_t)((uint32_t)barW * _percent / 100), 7, SSD1306_WHITE);
            break;
        }
        case 6:
            _display->displayBuff();
            // fall through
        default:
            _drawStep = 0; // completed
            break;
    }
}
#endif // DEVICE_DISPLAY_MODULE
