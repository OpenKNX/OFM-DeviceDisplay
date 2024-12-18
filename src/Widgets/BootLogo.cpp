#include "BootLogo.h"
#include "../icons/logo.h"
#include "openknx.h"

WidgetBootLogo::WidgetBootLogo(uint32_t displayTime, WidgetsAction action)
    : _displayTime(displayTime), _action(action), _display(nullptr), _needsRedraw(true) {}

void WidgetBootLogo::setup()
{
    logInfoP("Setup...");
    if (_display == nullptr)
    {
        logErrorP("Display ist NULL.");
        return;
    }
}

void WidgetBootLogo::start()
{
    logInfoP("Start...");
    _needsRedraw = true;
}

void WidgetBootLogo::stop()
{
    logInfoP("Stop...");
    _needsRedraw = false;
}

void WidgetBootLogo::pause()
{
    logInfoP("Pause...");
    _needsRedraw = false;
}

void WidgetBootLogo::resume()
{
    logInfoP("Resume...");
    _needsRedraw = true;
}

void WidgetBootLogo::loop()
{
    if (!_needsRedraw) return;

    drawBootLogo();
    _needsRedraw = false;
}

void WidgetBootLogo::setDisplayModule(i2cDisplay *displayModule)
{
    _display = displayModule;
    if (_display == nullptr)
    {
        logErrorP("Display ist NULL.");
        return;
    }
}

i2cDisplay *WidgetBootLogo::getDisplayModule() const
{
    return _display;
}

uint32_t WidgetBootLogo::getDisplayTime() const
{
    return _displayTime;
}

WidgetsAction WidgetBootLogo::getAction() const
{
    return _action;
}

void WidgetBootLogo::drawBootLogo()
{
    if (!_display || !_display->display)
    {
        logErrorP("Display ist NULL.");
        return;
    }

    // Zentriertes Zeichnen des Logos
    _display->display->clearDisplay();
    _display->display->drawBitmap(
        (_display->GetDisplayWidth() - logo_OpenKNX_WIDTH) / 2,
        (_display->GetDisplayHeight() - logo_OpenKNX_HEIGHT) / 2,
        logo_OpenKNX,
        logo_OpenKNX_WIDTH,
        logo_OpenKNX_HEIGHT,
        1);
    _display->displayBuff();

    logInfoP("Boot logo drawn.");
}