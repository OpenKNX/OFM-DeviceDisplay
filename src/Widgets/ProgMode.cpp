#include "ProgMode.h"
#include "OpenKNX.h"

WidgetProgMode::WidgetProgMode(uint32_t displayTime, WidgetFlags action)
    : _display(nullptr), _action(action), _displayTime(displayTime), _state(WidgetState::STOPPED),
      _lastBlinkTime(0), _showProgMode(true)
{
}

WidgetProgMode::~WidgetProgMode()
{
}

void WidgetProgMode::setup()
{
    if (!_display)
    {
        logErrorP("Display module is NULL.");
        return;
    }
}

void WidgetProgMode::start()
{
    logInfoP("start...");
    _state = WidgetState::RUNNING;
    _lastBlinkTime = millis();
    draw(); // Initial draw
}

void WidgetProgMode::stop()
{
    logInfoP("stop...");
    _state = WidgetState::STOPPED;
    if (_display)
    {
        _display->display->clearDisplay();
        _display->displayBuff();
    }
}

void WidgetProgMode::pause()
{
    logInfoP("paused...");
    _state = WidgetState::PAUSED;
}

void WidgetProgMode::resume()
{
    logInfoP("resume...");
    _state = WidgetState::RUNNING;
    _lastBlinkTime = millis();
    draw(); // Redraw after resuming
}

void WidgetProgMode::loop()
{
    if (!_display || _state != WidgetState::RUNNING) return;

    unsigned long currentTime = millis();

    // Check if it's time to toggle the blink state
    if (currentTime - _lastBlinkTime >= PROG_MODE_BLINK_DELAY)
    {
        _lastBlinkTime = currentTime;
        _showProgMode = !_showProgMode; // Toggle the blink state
        draw();                         // Update the display
    }
}

void WidgetProgMode::draw()
{
    if (!_display) return;

    _display->display->clearDisplay(); // Clear the display
    _display->display->setTextColor(SSD1306_WHITE);

    // Header: "OpenKNX" (always visible)
    _display->display->setTextSize(1);
    _display->display->setCursor(0, 0);
    _display->display->print("   www.OpenKNX.de   ");

    // "ProgMode" message (blinking)
    if (_showProgMode)
    {
        _display->display->setTextColor(SSD1306_BLACK, SSD1306_WHITE);
    }
    else
    {
        _display->display->setTextColor(SSD1306_WHITE);
    }

    _display->display->setTextSize(2);
    _display->display->setCursor(0, 20);
    _display->display->print(" ProgMode!");

    // Footer: "Ready to use ETS..."
    _display->display->setTextColor(SSD1306_WHITE);
    _display->display->setTextSize(1);
    _display->display->setCursor(0, 45);
    _display->display->println(" Ready to use ETS to  program the Device! ");

    // Update the display
    _display->displayBuff();
}