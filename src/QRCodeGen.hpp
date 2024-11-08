#pragma once

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
    QRCodeWidget(i2cDisplay* display, const std::string& url, Icon iconBitmap, bool backgroundWhite)
        : display_(display), url_(url), iconBitmap_(iconBitmap), backgroundWhite_(backgroundWhite) {}

    // Constructor without icon, if no icon is set
    QRCodeWidget(i2cDisplay* display, const std::string& url, bool backgroundWhite)
        : display_(display), url_(url), backgroundWhite_(backgroundWhite) {}

    // Set the URL for the QR code
    void setUrl(const std::string& url) { url_ = url; }
    // Get the URL for the QR code
    const std::string getUrl() { return url_; }

    // Set the icon for the QR code
    void setIcon(const uint8_t* iconBitmap, int width, int height) { iconBitmap_ = {iconBitmap, width, height}; }
    // Set the icon for the QR code
    void setIcon(const Icon& iconBitmap) { iconBitmap_ = iconBitmap; }
    // Set the background color for the QR code
    void setBackgroundWhite(bool backgroundWhite) { backgroundWhite_ = backgroundWhite; }
    // Set the display for the QR code
    void setDisplay(i2cDisplay* display) { display_ = display; }
    // Set the alignment for the QR code
    void setAlign(QRCodeAlignPos alignPos) { qrAlignment_.alignment = alignPos; }

    // Draw the QR code on the display and optionally place the icon
    // The display and icon are centered on the display. The icon is optional. The background color can be set to white or black.
    // Before calling this method, the URL and icon should be set using the `setUrl` and `setIcon` methods. The display should be set using the `setDisplay` method.
    void draw()
    {
        uint8_t qrcode[qrcodegen_BUFFER_LEN_MAX];
        uint8_t tempBuffer[qrcodegen_BUFFER_LEN_MAX];

        // Generate QR code data
        if (!qrcodegen_encodeText(url_.c_str(), tempBuffer, qrcode,
                                  // qrcodegen_Ecc_LOW, // Error correction level
                                  qrcodegen_Ecc_HIGH,    // Error correction level
                                  qrcodegen_VERSION_MIN, // Minimum version
                                  qrcodegen_VERSION_MAX, // Maximum version
                                  qrcodegen_Mask_AUTO,   // Automatic mask
                                  true                   // Boost the mask
                                  ))
        {
            return; // Error generating QR code
        }

        // Calculate QR code size and pixel size
        int qrSize = qrcodegen_getSize(qrcode);                                                   // QR code size in modules (e.g., 21x21 moduiles for Version 1)
        int maxDisplaySize = std::min(display_->GetDisplayWidth(), display_->GetDisplayHeight()); // Minimum of display height and width

        // Calculate QR code size and pixel size based on display size
        int pixelSize = maxDisplaySize / qrSize; // Pixel size based on display size
        int qrPixelSize = qrSize * pixelSize;    // Calculate QR code size in pixels

        // QR code alignment x offset left
        if (qrAlignment_.alignment & ALIGN_LEFT) // QR code alignment x offset left
        {
            qrAlignment_.offsetX = 0;
        }
        else if (qrAlignment_.alignment & ALIGN_RIGHT) // QR code alignment x offset right
        {
            qrAlignment_.offsetX = display_->GetDisplayWidth() - qrPixelSize; // QR code alignment x offset right
        }
        else
        {
            qrAlignment_.offsetX = (display_->GetDisplayWidth() - qrPixelSize) / 2; // QR code alignment centered. Default!
        }

        qrAlignment_.offsetY = (display_->GetDisplayHeight() - qrPixelSize) / 2; // QR code alignment y always centered

        display_->display->clearDisplay();                                                                   // Clear display buffer
        display_->display->setTextColor(backgroundWhite_ ? BLACK : WHITE, backgroundWhite_ ? WHITE : BLACK); // Set text color based on background color

        // Draw every QR code pixel on the display
        for (int y = 0; y < qrSize; y++)
        {
            for (int x = 0; x < qrSize; x++)
            {
                bool module = qrcodegen_getModule(qrcode, x, y);
                display_->display->drawRect(qrAlignment_.offsetX + x * pixelSize, qrAlignment_.offsetY + y * pixelSize, pixelSize, pixelSize, module ? BLACK : WHITE);
            }
        }

        // Draw the icon in the center of the QR code. The icon is optional. And not recommended for small displays!
        if (iconBitmap_.bitmapData != nullptr)
        {
            // Display the icon in the center of the QR code
            display_->display->drawBitmap((qrAlignment_.offsetX + (qrPixelSize - iconBitmap_.width) / 2),  // icon Offset X
                                          (qrAlignment_.offsetY + (qrPixelSize - iconBitmap_.height) / 2), // icon Offset Y,
                                          iconBitmap_.bitmapData, iconBitmap_.width, iconBitmap_.height, backgroundWhite_ ? BLACK : WHITE);
        }

        // ToDo: Add text to the QR code. The text could be added below on the left or right or above or below the QR code.
        //      Calculate the QR code alignment and position and find out where to add Text. Makes only sense if the QR code is not full screen and
        //      there is space for additional text. The text could be added below on the left or right or above or below the QR code.

        // Show the QR code on the display
        display_->display->display();
    }

  private:
    QRcodeAlignment qrAlignment_ = {ALIGN_CENTER, 0, 0}; // QR code alignment settings
    i2cDisplay* display_;                                // Display object
    std::string url_;                                    // URL for the QR code
    Icon iconBitmap_ = {nullptr, 0, 0};                  // Icon for the QR code
    bool backgroundWhite_;                               // Background color for the QR code
};