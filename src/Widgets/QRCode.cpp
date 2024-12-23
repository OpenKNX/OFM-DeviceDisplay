#include "QRCode.h"
#include "OpenKNX.h"
#include "qrcodegen.h"

// Constructor
WidgetQRCode::WidgetQRCode(uint32_t displayTime, WidgetsAction action, const std::string &defaultText, bool backgroundWhite)
    : _displayTime(displayTime), _action(action), _state(WidgetState::STOPPED),
      _display(nullptr), _qrCodeText(defaultText), _defaultText(defaultText),
      _backgroundWhite(backgroundWhite) {}

// Start the widget
void WidgetQRCode::start()
{
    if (_state != WidgetState::STOPPED) return;

    logInfoP("Start...");
    _state = WidgetState::RUNNING;
    _qrCodeText = _defaultText; // Reset to default text when starting
    _needsRedraw = true;        // Mark as needing redraw
}

// Stop the widget
void WidgetQRCode::stop()
{
    if (_state == WidgetState::STOPPED) return;
    logInfoP("Stop...");
    _state = WidgetState::STOPPED;
    if (_display) _display->display->clearDisplay(); // Clear the display
}

// Pause the widget
void WidgetQRCode::pause()
{
    if (_state != WidgetState::RUNNING) return;
    logInfoP("Pause...");
    _state = WidgetState::PAUSED;
}

// Resume the widget
void WidgetQRCode::resume()
{
    if (_state != WidgetState::PAUSED) return;

    logInfoP("Resume...");
    _state = WidgetState::RUNNING;
    _needsRedraw = true; // Ensure it redraws on resume
}

// Setup the widget
void WidgetQRCode::setup()
{
    logInfoP("Setup...");
    if (_display == nullptr)
    {
        logInfoP("Display is NULL!");
        return;
    }
}

// Main loop for the widget
void WidgetQRCode::loop()
{
    if (_state != WidgetState::RUNNING) return;

    if (_needsRedraw)
    {
        drawQRCode();         // Only draw when a redraw is needed
        _needsRedraw = false; // Mark as updated
    }
}

// Return the display time
uint32_t WidgetQRCode::getDisplayTime() const
{
    return _displayTime;
}

// Return the widget action
WidgetsAction WidgetQRCode::getAction() const
{
    return _action;
}

// Set the display module
void WidgetQRCode::setDisplayModule(i2cDisplay *displayModule)
{
    _display = displayModule;
}

// Get the display module
i2cDisplay *WidgetQRCode::getDisplayModule() const
{
    return _display;
}

// Set the QR code text
void WidgetQRCode::setQRCodeText(const std::string &text)
{
    // Set external QR code text
    logInfoP("Setting QR code text...");
    if (_qrCodeText != text)
    { // Only trigger a redraw if the text changed
        _qrCodeText = text;
        _needsRedraw = true;
        logInfoP("QR code text set.");
    }

}

// Set the background color
void WidgetQRCode::setBackgroundWhite(bool white)
{
    if (_backgroundWhite != white)
    { // Only trigger a redraw if the background color changed
        _backgroundWhite = white;
        _needsRedraw = true;
    }
}

// Draw the QR code
void WidgetQRCode::drawQRCode()
{
    if (_qrCodeText.empty() || !_display) return;

    static uint8_t qrcode[qrcodegen_BUFFER_LEN_MAX];
    static uint8_t tempBuffer[qrcodegen_BUFFER_LEN_MAX];

    // Generate QR Code
    if (!qrcodegen_encodeText(_qrCodeText.c_str(), tempBuffer, qrcode,
                              qrcodegen_Ecc_LOW, 3, 3, qrcodegen_Mask_AUTO, true))
    {
        logErrorP("Failed to generate QR code.");
        return;
    }

    // Get QR Code size and pixel size
    int qrSize = qrcodegen_getSize(qrcode);
    int pixelSize = std::min(_display->GetDisplayWidth() / qrSize, _display->GetDisplayHeight() / qrSize);
    int xOffset = (_display->GetDisplayWidth() - qrSize * pixelSize) / 2;
    int yOffset = (_display->GetDisplayHeight() - qrSize * pixelSize) / 2;

    _display->display->clearDisplay(); // Clear the display

    for (int y = 0; y < qrSize; y++)
    {
        for (int x = 0; x < qrSize; x++)
        {
            if (qrcodegen_getModule(qrcode, x, y))
            {
                _display->display->drawRect(xOffset + x * pixelSize, yOffset + y * pixelSize,
                                            pixelSize, pixelSize, _backgroundWhite ? BLACK : WHITE);
            }
        }
    }
    _display->displayBuff(); // Send buffer to display
    openknx.logger.log(logPrefix() + ": QR code drawn.");
}