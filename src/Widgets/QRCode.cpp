#ifdef DEVICE_DISPLAY_MODULE
    #include "QRCode.h"
    #include "OpenKNX.h"
    #include "qrcodegen.h"

// Constructor
WidgetQRCode::WidgetQRCode(uint32_t displayTime, WidgetFlags action, const std::string &defaultText, bool backgroundWhite)
    : _displayTime(displayTime), _action(action), _state(WidgetState::STOPPED),
      _display(nullptr), _qrCodeText(defaultText), _defaultText(defaultText),
      _backgroundWhite(backgroundWhite) {}

// Start the widget
void WidgetQRCode::start()
{
    if (_state != WidgetState::STOPPED) return;

    logDebugP("Start...");
    _state = WidgetState::RUNNING;
    _qrCodeText = _defaultText; // Reset to default text when starting
    _needsRedraw = true;        // Mark as needing redraw
}

// Stop the widget
void WidgetQRCode::stop()
{
    if (_state == WidgetState::STOPPED) return;
    logDebugP("Stop...");
    _state = WidgetState::STOPPED;
    if (_display) _display->display->clearDisplay(); // Clear the display
}

// Pause the widget
void WidgetQRCode::pause()
{
    if (_state != WidgetState::RUNNING) return;
    logDebugP("Pause...");
    _state = WidgetState::PAUSED;
}

// Resume the widget
void WidgetQRCode::resume()
{
    if (_state != WidgetState::PAUSED) return;

    logDebugP("Resume...");
    _state = WidgetState::RUNNING;
    _needsRedraw = true; // Ensure it redraws on resume
}

// Setup the widget
void WidgetQRCode::setup()
{
    logDebugP("Setup...");
    if (_display == nullptr)
    {
        logDebugP("Display is NULL!");
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
WidgetFlags WidgetQRCode::getAction() const
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
    logDebugP("Setting QR code text...");
    if (_qrCodeText != text)
    { // Only trigger a redraw if the text changed
        _qrCodeText = text;
        _needsRedraw = true;
        logDebugP("QR code text set.");
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

    // version 3 at most: buffers sized for it, not for version 40
    static uint8_t qrcode[qrcodegen_BUFFER_LEN_FOR_VERSION(3)];
    static uint8_t tempBuffer[qrcodegen_BUFFER_LEN_FOR_VERSION(3)];

    // Generate QR Code
    if (!qrcodegen_encodeText(_qrCodeText.c_str(), tempBuffer, qrcode,
                              qrcodegen_Ecc_LOW, 3, 3, OPENKNX_QR_MASK, true))
    {
        logErrorP("Failed to generate QR code.");
        return;
    }

    _display->display->clearDisplay(); // Clear the display
    drawQr(_display, qrcode, 0, 0, _display->GetDisplayWidth(), _display->GetDisplayHeight(), 0, _backgroundWhite);
    _display->displayBuff(); // Send buffer to display
    logDebugP("QR code drawn.");
}

int16_t WidgetQRCode::drawQr(i2cDisplay *display, const uint8_t *qr, int16_t x, int16_t y, int16_t w, int16_t h, uint8_t quiet, bool darkOnLight)
{
    const int16_t n = (int16_t)qrcodegen_getSize(qr);
    const int16_t px = std::max<int16_t>(1, (std::min(w, h) - 2 * quiet) / n);
    const int16_t box = n * px + 2 * quiet;
    const int16_t x0 = x + (w - box) / 2;
    const int16_t y0 = y + (h - box) / 2;
    if (darkOnLight) display->display->fillRect(x0, y0, box, box, WHITE);
    const uint16_t color = darkOnLight ? BLACK : WHITE;
    for (int16_t my = 0; my < n; my++)
        for (int16_t mx = 0; mx < n; mx++)
            if (qrcodegen_getModule(qr, mx, my)) display->display->fillRect(x0 + quiet + mx * px, y0 + quiet + my * px, px, px, color);
    return x0 + box;
}
#endif // DEVICE_DISPLAY_MODULE