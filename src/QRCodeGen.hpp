#pragma once
/**
 * Display Size to QR Code Version Mapping:
 *
 * | Display Size | Max QR Code Version | Pixel Size (Low) | Max Characters (Low) | Pixel Size (High) | Max Characters (High) |
 * |--------------|---------------------|------------------|----------------------|-------------------|-----------------------|
 * | 128x32 Pixel | Version 1 (21x21)   | 32 / 21 ≈ 1.52   | 41 characters        | 32 / 21 ≈ 1.52    | 17 characters         |
 * | 128x64 Pixel | Version 3 (29x29)   | 64 / 29 ≈ 2.21   | 127 characters       | 64 / 29 ≈ 2.21    | 53 characters         |
 * | 96x16 Pixel  | Version 1 (21x21)   | 16 / 21 ≈ 0.76   | 41 characters        | 16 / 21 ≈ 0.76    | 17 characters         |
 * | 64x48 Pixel  | Version 2 (25x25)   | 48 / 25 ≈ 1.92   | 77 characters        | 48 / 25 ≈ 1.92    | 32 characters         |
 * | 128x128 Pixel| Version 6 (41x41)   | 128 / 41 ≈ 3.12  | 322 characters       | 128 / 41 ≈ 3.12   | 134 characters        |
 *
 * Explanation:
 * - Display Size:          The size of the I2C monochrome display in pixels.
 * - Max QR Code Version:   The highest QR Code version that fits on the display.
 * - Pixel Size (Low):      The size of a QR Code module in pixels using error correction level "L" (Low).
 * - Max Characters (Low):  The maximum number of characters that fit in the QR Code using error correction level "L" (Low).
 * - Pixel Size (High):     The size of a QR Code module in pixels using error correction level "H" (High).
 * - Max Characters (High): The maximum number of characters that fit in the QR Code using error correction level "H" (High).
 *
 * Possible QR Code Versions: 1 to 40
 * Possible Error Correction Levels: L (Low), M (Medium), Q (Quartile), H (High)
 * Possible Mask Patterns: 0 to 7 (Automatic mask pattern selection)
 * Possible Boost Mask: True or False (Boost the mask)
 *
 * Possible QR Code Alignment: Left, Right, Center
 * Possible Icon Alignment: Left, Right, Center (Center is default)
 *
 * The QR code is generated based on the display size and the URL. The QR code is centered on the display.
 * The icon is optional and can be placed in the center of the QR code. The background color can be set to white or black.
 * The QR code is generated only once to avoid unnecessary processing.
 *
 * The QR code is generated using the QR-Code library  (https://github.com/nayuki/QR-Code-generator).
 * The QR code is displayed on the I2C monochrome display.
 *
 * Following QR Codes are possible:
 * - URL QR Code ("https://www.openknx.de")
 * - WiFi QR Code ("WIFI:S:SSID;T:WPA;P:Password;;")
 * - Email QR Code ("mailto:<email>?subject=<subject>&body=<body>")
 * - Phone QR Code ("tel:<phone number>")
 * - SMS QR Code ("smsto:<phone number>:<message>")
 * - Contact QR Code ("MECARD:N:Name;ORG:Organization;TEL:Phone;EMAIL:Email;URL:URL;ADR:Address;NOTE:Note;;")
 * - Event QR Code ("BEGIN:VEVENT;SUMMARY:Summary;DTSTART:20210101T000000Z;DTEND:20210101T010000Z;LOCATION:Location;DESCRIPTION:Description;END:VEVENT")
 * - Geo Location QR Code ("geo:Latitude,Longitude")
 * - Bitcoin QR Code ("bitcoin:<address>?amount=<amount>")
 *
 * Attention: Maximum QR Code Version is set to 3! Minimum QR Code Version is set to 3!
 *            Max possible characters for the QR code is 127 characters!
 */

// #include "i2c-display.h"
#include "qrcodegen.h"
// extern "C" {
//   #include "qrcodegen.h" // QR-Code library (https://github.com/nayuki/QR-Code-generator)
// } // Not using here the #include <qrcodegen.hpp>

class QRCodeWidget
{
  public:
    struct Icon // Icon struct which holds the bitmap data and size for the icon in the QR code
    {
        const uint8_t* bitmapData; // Bitmap data for the icon
        int width;                 // Width of the icon
        int height;                // Height of the icon
    };

    enum QRCodeAlignPos
    {
        ALIGN_LEFT = 0x01,  // Align left
        ALIGN_RIGHT = 0x02, // Align right
        ALIGN_CENTER = 0x04 // Align center
        // ALIGN_TOP = 0x08,    // Align top
        // ALIGN_MIDDLE = 0x10, // Align middle
        // ALIGN_BOTTOM = 0x20,  // Align bottom
    } alignPos = ALIGN_CENTER; // Default alignment is center

    struct QRcodeAlignment
    {
        uint8_t alignment; // Alignment for the QR code. possible values: ALIGN_LEFT, ALIGN_RIGHT, ALIGN_CENTER
        int offsetX;       // X offset for the QR code
        int offsetY;       // Y offset for the QR code
    };

    // Constructor for QRCodeWidget
    QRCodeWidget(i2cDisplay* display, const std::string& url, bool backgroundWhite)
        : _display(display), _url(url), _backgroundWhite(backgroundWhite), _qrCodeGenerated(false) {}

#ifdef QRCODE_WIDGET_ICON
    // Constructor for QRCodeWidget with icon
    QRCodeWidget(i2cDisplay* display, const std::string& url, bool backgroundWhite, Icon iconBitmap)
        : _display(display), _url(url), _iconBitmap(iconBitmap), _backgroundWhite(backgroundWhite), _qrCodeGenerated(false) {}
#endif
    // Set the URL for the QR code
    void setUrl(const std::string& url) { _url = url; }
    // Get the URL for the QR code
    const std::string getUrl() { return _url; }

#ifdef QRCODE_WIDGET_ICON
    // Set the icon for the QR code
    void setIcon(const uint8_t* iconBitmap, int width, int height) { _iconBitmap = {iconBitmap, width, height}; }
    // Set the icon for the QR code
    void setIcon(const Icon& iconBitmap) { _iconBitmap = iconBitmap; }
#endif
    // Set the background color for the QR code
    void setBackgroundWhite(bool backgroundWhite) { _backgroundWhite = backgroundWhite; }
    // Set the display for the QR code
    void setDisplay(i2cDisplay* display) { _display = display; }
    // Set the alignment for the QR code
    void setAlign(QRCodeAlignPos alignPos) { _qrAlignment.alignment = alignPos; }

    // Draw the QR code on the display and optionally place the icon
    // The display and icon are centered on the display. The icon is optional. The background color can be set to white or black.
    // Before calling this method, the URL and icon should be set using the `setUrl` and `setIcon` methods. The display should be set using the `setDisplay` method.
    void generateQRCode()
    {
        uint8_t qrcode[qrcodegen_BUFFER_LEN_MAX];
        uint8_t tempBuffer[qrcodegen_BUFFER_LEN_MAX];

        // Generate QR code data
        if (!qrcodegen_encodeText(_url.c_str(), tempBuffer, qrcode,
                                  qrcodegen_Ecc_LOW, // Error correction level
                                  // qrcodegen_Ecc_HIGH,  // Error correction level
                                  3,                   // We will Fix is to 3! Minimum QR Code Version (Min: 1)
                                  3,                   // We will Fix is to 3! Maximum QR Code Version (Max: 40)
                                  qrcodegen_Mask_AUTO, // Automatic mask
                                  true                 // Boost the mask
                                  ))
        {
            return; // Error generating QR code
        }

        // Calculate QR code size and pixel size
        const int qrSize = qrcodegen_getSize(qrcode); // QR code size in modules (e.g., 21x21 moduiles for Version 1)

        // Calculate QR code size and pixel size based on display size
        const uint8_t pixelSize = std::min(_display->GetDisplayWidth() / qrSize, _display->GetDisplayHeight() / qrSize);
        const int qrPixelSize = qrSize * pixelSize; // Calculate QR code size in pixels

        // QR code alignment x offset left
        if (_qrAlignment.alignment & ALIGN_LEFT) // QR code alignment x offset left
        {
            _qrAlignment.offsetX = 0;
        }
        else if (_qrAlignment.alignment & ALIGN_RIGHT) // QR code alignment x offset right
        {
            _qrAlignment.offsetX = _display->GetDisplayWidth() - qrPixelSize; // QR code alignment x offset right
        }
        else
        {
            _qrAlignment.offsetX = (_display->GetDisplayWidth() - qrPixelSize) / 2; // QR code alignment centered. Default!
        }

        _qrAlignment.offsetY = (_display->GetDisplayHeight() - qrPixelSize) / 2; // QR code alignment y always centered

        _display->display->clearDisplay();                                                                   // Clear display buffer
        _display->display->setTextColor(_backgroundWhite ? BLACK : WHITE, _backgroundWhite ? WHITE : BLACK); // Set text color based on background color

        // Draw every QR code pixel on the display
        for (int y = 0; y < qrSize; y++)
        {
            for (int x = 0; x < qrSize; x++)
            {
                _display->display->drawRect(_qrAlignment.offsetX + x * pixelSize,               // X position
                                            _qrAlignment.offsetY + y * pixelSize,               // Y position
                                            pixelSize, pixelSize,                               // Width and height
                                            qrcodegen_getModule(qrcode, x, y) ? WHITE : BLACK); // Color of the pixel
            }
        }
#ifdef QRCODE_WIDGET_ICON
        // Draw the icon in the center of the QR code. The icon is optional. And not recommended for small displays!
        if (_iconBitmap.bitmapData != nullptr)
        {
            // Display the icon in the center of the QR code
            _display->display->drawBitmap((_qrAlignment.offsetX + (qrPixelSize - _iconBitmap.width) / 2),  // icon Offset X
                                          (_qrAlignment.offsetY + (qrPixelSize - _iconBitmap.height) / 2), // icon Offset Y,
                                          _iconBitmap.bitmapData, _iconBitmap.width, _iconBitmap.height, _backgroundWhite ? BLACK : WHITE);
        }
#endif

        // ToDo: Add text to the QR code. The text could be added below on the left or right or above or below the QR code.
        //      Calculate the QR code alignment and position and find out where to add Text. Makes only sense if the QR code is not full screen and
        //      there is space for additional text. The text could be added below on the left or right or above or below the QR code.

        // Show the QR code on the display
        //_display->display->display();
        _display->displayBuff(); // Display the buffer on the display

        // Set the flag that the QR code was generated and is displayed
        _qrCodeGenerated = true;
    } // End of generateQRCode

    void draw()
    {
        if (!_qrCodeGenerated) 
        {
            generateQRCode(); // Generate the QR code only once, to avoid unnecessary processing
            _display->displayBuff(); // Display the buffer on the display in the next loop
        }
    }

  private:
    QRcodeAlignment _qrAlignment = {ALIGN_CENTER, 0, 0}; // QR code alignment settings
    i2cDisplay* _display;                                // Display object
    std::string _url;                                    // URL for the QR code
    bool _backgroundWhite;                               // Background color for the QR code
    bool _qrCodeGenerated;                               // Flag to indicate if the QR code was generated and displayed
#ifdef QRCODE_WIDGET_ICON
    Icon _iconBitmap = {nullptr, 0, 0}; // Icon for the QR code
#endif
};