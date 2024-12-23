#include "BootLogo.h"
#include "../icons/logo.h"
#include "OpenKNX.h"

WidgetBootLogo::WidgetBootLogo(uint32_t displayTime, WidgetsAction action)
    : _displayTime(displayTime), _action(action), _display(nullptr), _needsRedraw(true), _drawStep(0), _yStart(0) {}

void WidgetBootLogo::setup()
{
    if(_state == WidgetState::RUNNING) return;
    //logInfoP("Setup...");
    if (_display == nullptr)
    {
        //logErrorP("Display ist NULL.");
        return;
    }
}

void WidgetBootLogo::start()
{
    //logInfoP("Start...");
    _state = WidgetState::RUNNING;
    _drawStep = 1;
    _needsRedraw = true;
}

void WidgetBootLogo::stop()
{
    //logInfoP("Stop...");
    _state = WidgetState::STOPPED;
    _drawStep = 0;
    _needsRedraw = false;
}

void WidgetBootLogo::pause()
{
    //logInfoP("Pause...");
    _state = WidgetState::PAUSED;
    _needsRedraw = false;
}

void WidgetBootLogo::resume()
{
    //logInfoP("Resume...");
    _state = WidgetState::RUNNING;
    _drawStep = 1; // TODO check
    _needsRedraw = true;
}

void WidgetBootLogo::loop()
{
    if (!_needsRedraw || _state != WidgetState::RUNNING) return;

    drawBootLogo();
}

void WidgetBootLogo::setDisplayModule(i2cDisplay *displayModule)
{
    _display = displayModule;
    if (_display == nullptr)
    {
        //logErrorP("Display ist NULL.");
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

/**
 * @brief Shows the OpenKNX logo on the display.
 */
void WidgetBootLogo::drawBootLogo()
{
    if (!_display || !_display->display)
    {
        //logErrorP("Display ist NULL.");
        return;
    }

    // TODO/looptime: check direct usage of full-display buffer as alternative

    if (_needsRedraw)
    {
        // reduce loop-time by split into multiple loop-calls, as drawing the bitmap is slow
        switch (_drawStep)
        {
            case 1:
                _display->display->clearDisplay();
                _drawStep = 2;
                break;
            case 2:
                // TODO check generalization and extraction of partial image drawing
                if (_yStart < logo_OpenKNX_HEIGHT)
                {
                    _display->display->drawBitmap(
                        (_display->GetDisplayWidth() - logo_OpenKNX_WIDTH) / 2,
                        (_display->GetDisplayHeight() - logo_OpenKNX_HEIGHT) / 2 + _yStart,
                        logo_OpenKNX + _yStart * ((logo_OpenKNX_WIDTH + 7) / 8),
                        logo_OpenKNX_WIDTH,
                        MIN(logo_OpenKNX_HEIGHT - _yStart, _stepHeight),
                        1
                    );
                    _yStart += _stepHeight;
                    break;
                }
                _drawStep = 3;
                // fall through, when full height was drawn in last loop()
            case 3:
                _display->displayBuff();
                // fall through, to stop drawing
            default:
                _drawStep = 0;
                _needsRedraw = false;

        }
    }
}