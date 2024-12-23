#include "OpenKNXLogo.h"
#include "../icons/logo.h"
#include "openknx.h"

WidgetOpenKNXLogo::WidgetOpenKNXLogo(uint32_t displayTime, WidgetsAction action)
    : _displayTime(displayTime), _action(action), _display(nullptr), _needsRedraw(true), _drawStep(0) {}

void WidgetOpenKNXLogo::setup()
{
    logInfoP("Setup...");
    if (_display == nullptr)
    {
        logErrorP("Display ist NULL.");
        return;
    }
}

void WidgetOpenKNXLogo::start()
{
    logInfoP("Start...");
    _drawStep = 1;
    _needsRedraw = true;
}

void WidgetOpenKNXLogo::stop()
{
    logInfoP("Stop...");
    _drawStep = 0;
    _needsRedraw = false;
}

void WidgetOpenKNXLogo::pause()
{
    logInfoP("Pause...");
    _needsRedraw = false;
}

void WidgetOpenKNXLogo::resume()
{
    logInfoP("Resume...");
    _drawStep = 1; // TODO check
    _needsRedraw = true;
}

void WidgetOpenKNXLogo::loop()
{
    if (!_needsRedraw) return;

    const String _currentUptime = openknx.logger.buildUptime().c_str(); // Get the current uptime

    // TODO compare update only, without formatting
    if (_lastUptime != _currentUptime)
    {
        _drawStep = 1; // restart drawing as uptime has changed
        _lastUptime = _currentUptime;
    }
    drawOpenKNXLogo();
}

void WidgetOpenKNXLogo::setDisplayModule(i2cDisplay *displayModule)
{
    _display = displayModule;
    if (_display == nullptr)
    {
        logErrorP("Display ist NULL.");
        return;
    }
}

i2cDisplay *WidgetOpenKNXLogo::getDisplayModule() const
{
    return _display;
}

uint32_t WidgetOpenKNXLogo::getDisplayTime() const
{
    return _displayTime;
}

WidgetsAction WidgetOpenKNXLogo::getAction() const
{
    return _action;
}

/*
void OpenKNXLogo::drawBootLogo()
{
    if (!_display || !_display->display)
    {
        logErrorP("Display ist NULL.");
        return;
    }

    // TODO/looptime: check direct usage of full-display buffer as alternative

    // TODO cleanup after extraction to own widget class...

    / ** Number of rows to draw in one loop() call * /
    const uint8_t stepHeight = 8; // TODO find "best" size (note: 8 results in ~1.6ms max loop time)

    if (_needsRedraw)
    {
        // reduce loop-time by split into multiple loop-calls, as drawing the bitmap is slow
        switch (_drawBootlogo)
        {
            case 1:
                _display->display->clearDisplay();
                _drawBootlogo = 2;
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
                        MIN(logo_OpenKNX_HEIGHT - _yStart, stepHeight),
                        1
                    );
                    _yStart += stepHeight;
                    break;
                }
                _drawBootlogo = 3;
                // fall through, when full height was drawn in last loop()
            case 3:
                _display->displayBuff();
                // fall through, to stop drawing
            default:
                _drawBootlogo = 0;
                _needsRedraw = false;

        }
    }
}
*/

/**
 * @brief Display the OpenKNX logo on the screen. The logo is displayed in the
 * center of the screen. The
 */
void WidgetOpenKNXLogo::drawOpenKNXLogo()
{
    if (!_display || !_display->display)
    {
        logErrorP("Display ist NULL.");
        return;
    }

    if (_drawStep)
    {
        _display->display->cp437(true); // Use CP437 character encoding (for all steps)

        switch (_drawStep)
        {
            case 1:
                _display->display->clearDisplay(); // Clear the display for fresh rendering
                _drawStep++;
                break;
            case 2:
                _display->display->setCursor(0, 0);
                _display->display->setTextSize(1);
                _display->display->setTextColor(WHITE);

                // _lastUptime :== _currentUptime
                _display->display->println("Uptime: " + _lastUptime);

                _display->display->println("Dev.: " MAIN_OrderNumber);
                _display->display->println(String("Addr.: ") + openknx.info.humanIndividualAddress().c_str());

                _drawStep++;
                break;
            case 3:
                _display->display->drawBitmap(
                    (_display->GetDisplayWidth() - LOGO_WIDTH_ICON_SMALL_OKNX) / 2,
                    (_display->GetDisplayHeight() - LOGO_HEIGHT_ICON_SMALL_OKNX + 20 /*SHIFT_TO_BOTTOM*/) / 2,
                    logoICON_SMALL_OKNX,
                    LOGO_WIDTH_ICON_SMALL_OKNX,
                    LOGO_HEIGHT_ICON_SMALL_OKNX,
                    1);

                _drawStep++;
                break;
            case 4:
                _display->displayBuff(); // Update the display with the rendered content
                // display->display->display(); // Update the display with the rendered content

                // fall trough
            default:
                _drawStep = 0;
                break;
        }
    }
}