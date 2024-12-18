#include "SysInfoLite.h"
#include "../icons/logo.h"
#include "openknx.h"

// Constructor
WidgetSysInfoLite::WidgetSysInfoLite(uint32_t displayTime, WidgetsAction action)
    : _displayTime(displayTime), _action(action), _display(nullptr),
      _state(WidgetState::STOPPED), _invertBitmap(false), _lastUpdate(0) {}

// Sets the display module
void WidgetSysInfoLite::setDisplayModule(i2cDisplay *displayModule)
{
    _display = displayModule;

    logInfoP("Set display module...");
    if (_display == nullptr)
    {
        logErrorP("Display is NULL.");
    }
}

// Retrieves the display module
i2cDisplay *WidgetSysInfoLite::getDisplayModule() const
{
    return _display;
}

// Returns the display time in milliseconds
uint32_t WidgetSysInfoLite::getDisplayTime() const
{
    return _displayTime;
}

// Returns the widget action
WidgetsAction WidgetSysInfoLite::getAction() const
{
    return _action;
}

// Setup method for widget initialization
void WidgetSysInfoLite::setup()
{
    logInfoP("Setup...");
    if (_display == nullptr)
    {
        logErrorP("Display is NULL.");
        return;
    }
}

// Starts the widget
void WidgetSysInfoLite::start()
{
    logInfoP("Start...");
    _state = WidgetState::RUNNING;
    _invertBitmap = false; // Reset inversion state
}

// Stops the widget
void WidgetSysInfoLite::stop()
{
    logInfoP("Stop...");
    _state = WidgetState::STOPPED;
}

// Pauses the widget
void WidgetSysInfoLite::pause()
{
    logInfoP("Pause...");
    _state = WidgetState::PAUSED;
}

// Resumes the widget
void WidgetSysInfoLite::resume()
{
    logInfoP("Resume...");
    if (_state == WidgetState::PAUSED)
    {
        _state = WidgetState::RUNNING;
    }
}

// Main loop for widget operation
void WidgetSysInfoLite::loop()
{
    if (_state != WidgetState::RUNNING)
    {
        return; // Do nothing if the widget is not running
    }

    // Check if at least 1 second has passed since the last update
    uint32_t currentMillis = millis();
    if (currentMillis - _lastUpdate >= 1000)
    {                                   // 1000 ms = 1 second
        _lastUpdate = currentMillis;    // Update the last update time
        _invertBitmap = !_invertBitmap; // Toggle the inversion state
        drawSysInfo();                  // Redraw the screen with the updated state
    }
}

// Draws the system information on the screen
void WidgetSysInfoLite::drawSysInfo()
{
    if (_display == nullptr || _display->display == nullptr)
    {
        logErrorP("Display or display driver is NULL.");
        return;
    }

    // Retrieve the current time or fallback to uptime
    char currentDisplayText[50];
    if (openknx.time.isValid())
        snprintf(currentDisplayText, sizeof(currentDisplayText), "Time: %04d-%02d-%02d %02d:%02d:%02d",
                 openknx.time.getUtcTime().year, openknx.time.getUtcTime().month, openknx.time.getUtcTime().day,
                 openknx.time.getUtcTime().hour, openknx.time.getUtcTime().minute, openknx.time.getUtcTime().second);
    else
        snprintf(currentDisplayText, sizeof(currentDisplayText), "Uptime: %s", openknx.logger.buildUptime().c_str());

    // Clear the display
    _display->display->clearDisplay();

    // Draw text content
    _display->display->setCursor(0, 0);
    _display->display->setTextSize(1);
    _display->display->setTextColor(WHITE);
    _display->display->println("System Info:");
    _display->display->println(currentDisplayText);
    _display->display->println(String("Addr.: ") + openknx.info.humanIndividualAddress().c_str());

    // Load and draw the bitmap
    uint8_t logoBitmap[((LOGO_WIDTH_ICON_SMALL_OKNX + 7) / 8) * LOGO_HEIGHT_ICON_SMALL_OKNX];
    memcpy(logoBitmap, logoICON_SMALL_OKNX,
           sizeof(logoBitmap)); // Copy the original bitmap to a local buffer

    if (_invertBitmap) invertBitmap(logoBitmap, LOGO_WIDTH_ICON_SMALL_OKNX, LOGO_HEIGHT_ICON_SMALL_OKNX); // Invert the bitmap pixels


    _display->display->drawBitmap(
        (int16_t)((_display->GetDisplayWidth() - LOGO_WIDTH_ICON_SMALL_OKNX) / 2),
        (int16_t)((_display->GetDisplayHeight() - LOGO_HEIGHT_ICON_SMALL_OKNX + 20 /*SHIFT_TO_BOTTOM*/) / 2),
        logoBitmap, LOGO_WIDTH_ICON_SMALL_OKNX, LOGO_HEIGHT_ICON_SMALL_OKNX, WHITE);

    _display->displayBuff(); // Send everything to the display
    logInfoP("System info drawn.");
}

// Inverts the pixels of a bitmap
void WidgetSysInfoLite::invertBitmap(uint8_t *bitmap, size_t width, size_t height) {
    size_t bytesPerRow = (width + 7) / 8; // Correctly round up to the nearest byte
    for (size_t row = 0; row < height; row++) {
        uint8_t *rowStart = bitmap + row * bytesPerRow;
        for (size_t col = 0; col < bytesPerRow; col++) {
            rowStart[col] = ~rowStart[col]; // Invert each byte
        }
    }
}