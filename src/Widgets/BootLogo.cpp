#ifdef DEVICE_DISPLAY_MODULE
#include "BootLogo.h"
#include "../icons/logo.h"
#include "OpenKNX.h"

WidgetBootLogo::WidgetBootLogo(uint32_t displayTime, WidgetFlags action, const LogoBitmap *logo)
    : _displayTime(displayTime), _action(action), _state(WidgetState::STOPPED),
      _display(nullptr), _needsRedraw(false), _logoBitmap(logo), _drawStep(0), _yStart(0)
{
    // Calculate optimal step height on construction
    // Default target loop time is 2000µs.
    calculateOptimalStepHeight(TARGET_LOOP_TIME_US);
}

/**
 * @brief Set the step height for incremental drawing
 * @param stepHeight Number of rows to draw per loop() call (1-64)
 */
void WidgetBootLogo::setStepHeight(uint8_t stepHeight)
{
    if (stepHeight >= 1 && stepHeight <= 64)
    {
        _stepHeight = stepHeight;
        logDebugP("Step height set to: %u rows", _stepHeight);
    }
    else
    {
        logWarningP("Invalid step height: %u (must be 1-64)", stepHeight);
    }
}

/**
 * @brief Calculate optimal step height based on available loop time
 * @param maxLoopTimeUs Maximum loop time in microseconds (default: 2000µs)
 */
void WidgetBootLogo::calculateOptimalStepHeight(uint32_t maxLoopTimeUs)
{
    // Calculate how many rows we can draw within the time budget
    uint32_t calculatedStepHeight = maxLoopTimeUs / TIME_PER_ROW_US;

    // For fullscreen logos (128x64), allow larger stripes
    if (_logoBitmap && _logoBitmap->width >= 100 && _logoBitmap->height >= 60)
    {
        // Fullscreen: Double the stripe size (less overhead)
        calculatedStepHeight = min(64, calculatedStepHeight * 2);
    }

    // Clamp to reasonable values
    if (calculatedStepHeight < 1)
        calculatedStepHeight = 1;
    else if (calculatedStepHeight > 64)
        calculatedStepHeight = 64;

    _stepHeight = static_cast<uint8_t>(calculatedStepHeight);

    logInfoP("Calculated optimal step height: %u rows (target: %uµs/loop)",
             _stepHeight, maxLoopTimeUs);
}

void WidgetBootLogo::start()
{
    if (_state == WidgetState::RUNNING)
    {
        // logDebugP("Already running.");
        return;
    }

    // logDebugP("Starting...");
    _state = WidgetState::RUNNING;
}

void WidgetBootLogo::stop()
{
    if (_state == WidgetState::STOPPED)
    {
        // logDebugP("Already stopped.");
        return;
    }

    // logDebugP("Stopping...");
    _state = WidgetState::STOPPED;
    _needsRedraw = false;
    _drawStep = 0;
}

void WidgetBootLogo::pause()
{
    if (_state == WidgetState::PAUSED)
    {
        // logDebugP("Already paused.");
        return;
    }

    // logDebugP("Pausing...");
    _state = WidgetState::PAUSED;
}

void WidgetBootLogo::resume()
{
    if (_state != WidgetState::PAUSED)
    {
        // logDebugP("Not paused.");
        return;
    }

    // logDebugP("Resuming...");
    _state = WidgetState::RUNNING;
}

void WidgetBootLogo::setup()
{
    if (_state == WidgetState::RUNNING)
        return;

    // logDebugP("Setup...");

    if (_display == nullptr)
    {
        // logErrorP("Display is NULL.");
        return;
    }

    // Set default logo if none provided
    if (!_logoBitmap)
    {
        //_logoBitmap = new LogoBitmap{logoICON_SMALL_OKNX, LOGO_WIDTH_ICON_SMALL_OKNX, LOGO_HEIGHT_ICON_SMALL_OKNX};
        _logoBitmap = new LogoBitmap{logo_OpenKNX, logo_OpenKNX_WIDTH, logo_OpenKNX_HEIGHT};
    }
    else
    {
        logInfoP("Using custom logo: %ux%u", _logoBitmap->width, _logoBitmap->height);
    }

    if (!_logoBitmap->data || _logoBitmap->width == 0 || _logoBitmap->height == 0)
    {
        logErrorP("Logo Bitmap is invalid.");
        return;
    }

    // ========== OPTIMIZATION: Pre-calculate everything ONCE ==========

    // Cache display dimensions
    _displayWidth = _display->GetDisplayWidth();
    _displayHeight = _display->GetDisplayHeight();

    // Cache centered position (calculated only once!)
    _xCenter = (_displayWidth - _logoBitmap->width) / 2;
    _yCenter = (_displayHeight - _logoBitmap->height) / 2;

    // Cache bytes per row for bitmap data offset calculation
    _bytesPerRow = (_logoBitmap->width + 7) / 8;

    // Adapt step height based on current system load
    uint32_t freeTime = openknx.freeLoopTime();
    if (freeTime < TARGET_LOOP_TIME_US)
    {
        logWarningP("Low loop time detected (%uµs), reducing step height", freeTime);
        calculateOptimalStepHeight(freeTime / 2); // Use half of available time
    }

    _needsRedraw = true;
    _drawStep = 1;
    _yStart = 0;

    logDebugP("Setup complete - Logo: %ux%u at (%u,%u), bytesPerRow: %u, stepHeight: %u",
              _logoBitmap->width, _logoBitmap->height, _xCenter, _yCenter, _bytesPerRow, _stepHeight);
}

void WidgetBootLogo::loop()
{
    if (_state != WidgetState::RUNNING)
        return;

    drawBootLogo();
}

/**
 * @brief Draw the boot logo on the display using incremental drawing
 *        to avoid long blocking times.
 */
void WidgetBootLogo::drawBootLogo()
{
    // Early exit if nothing to do
    if (!_display || !_display->display || !_needsRedraw)
        return;

    switch (_drawStep)
    {
        case 1:
            // ========== OPTIMIZATION: Combine clear + first stripe ==========
            // Step 1: Clear display AND draw first stripe in same loop!
            _display->display->clearDisplay();
            _yStart = 0;

            // Draw first stripe immediately (saves one loop iteration!)
            if (_logoBitmap->height > 0)
            {
                const uint16_t heightToDraw = MIN(_logoBitmap->height, _stepHeight);

                // Use cached values (no calculations!)
                _display->display->drawBitmap(
                    _xCenter,          // from cache
                    _yCenter,          // from cache
                    _logoBitmap->data, // first stripe at offset 0
                    _logoBitmap->width,
                    heightToDraw,
                    1);

                _yStart = _stepHeight;
            }

            _drawStep = 2;
            break;

        case 2:
            // Step 2: Draw remaining bitmap stripes
            if (_yStart < _logoBitmap->height)
            {
                // ========== OPTIMIZATION: All calculations from cache! ==========
                const uint16_t remainingHeight = _logoBitmap->height - _yStart;
                const uint16_t heightToDraw = MIN(remainingHeight, _stepHeight);

                // Calculate bitmap data pointer (using cached bytesPerRow)
                const uint8_t *dataPtr = _logoBitmap->data + (_yStart * _bytesPerRow);

                // Draw stripe (all positions from cache!)
                _display->display->drawBitmap(
                    _xCenter,           // from cache (no calculation!)
                    _yCenter + _yStart, // from cache + offset
                    dataPtr,            // pre-calculated pointer
                    _logoBitmap->width,
                    heightToDraw,
                    1);

                _yStart += _stepHeight;
                // Stay in case 2 until all stripes are drawn
            }
            else
            {
                // All stripes drawn, move to next step
                _drawStep = 3;
            }
            break;

        case 3:
            // Step 3: Send buffer to display
            _display->displayBuff();
            _drawStep = 0;
            _needsRedraw = false;
            break;

        default:
            _drawStep = 0;
            _needsRedraw = false;
            break;
    }
}
#endif // DEVICE_DISPLAY_MODULE