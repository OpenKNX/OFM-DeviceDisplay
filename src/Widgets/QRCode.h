#pragma once
#include "../Widget.h"
#include "qrcodegen.h"

// WidgetQRCode class: Display a QR code on the screen
class WidgetQRCode : public Widget {
public:
    const std::string logPrefix() { return "WidgetQRCode"; }

    // Constructor
    WidgetQRCode(uint32_t displayTime, WidgetsAction action, const std::string &defaultText = "", bool backgroundWhite = true);

    // Overrides from Widget base class
    void start() override;    // Start the widget and display the QR code
    void stop() override;     // Stop the widget and clear the QR code
    void pause() override;    // Pause the widget
    void resume() override;   // Resume the widget
    void setup() override;    // Setup the widget
    void loop() override;     // Main loop to update the widget

    uint32_t getDisplayTime() const override; // Return the display time in ms
    WidgetsAction getAction() const override; // Return the widget action
    void setDisplayModule(i2cDisplay *displayModule) override; // Set the display module
    i2cDisplay *getDisplayModule() const override;             // Get the display module

    // Additional public methods
    void setQRCodeText(const std::string &text); // Update the text for the QR code
    void setBackgroundWhite(bool white);        // Set the background to white or black

private:

    uint32_t _displayTime;        // Time to display the widget in ms
    WidgetsAction _action;        // Action to perform after the widget
    WidgetState _state;     // Current state of the widget
    i2cDisplay *_display;         // Display object
    std::string _qrCodeText;      // Text for the QR code
    std::string _defaultText;     // Default text to display
    uint32_t _lastUpdateTime = 0; // Last update time

    bool _needsRedraw = true;     // Flag indicating if redraw is needed
    bool _backgroundWhite;        // Flag to set background color (true = white, false = black)

    // Private methods
    void drawQRCode(); // Draw the QR code on the display
};