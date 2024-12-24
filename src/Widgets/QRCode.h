#pragma once
#include "../Widget.h"

// WidgetQRCode class: Display a QR code on the screen
class WidgetQRCode : public Widget
{
  public:
    const std::string logPrefix() { return "WidgetQRCode"; }

    // Constructor
    WidgetQRCode(uint32_t displayTime, WidgetFlags action, const std::string &defaultText = "", bool backgroundWhite = true);

    // Overrides from Widget base class
    void start() override;                                                  // Start widget
    void stop() override;                                                   // Stop widget
    void pause() override;                                                  // Pause widget
    void resume() override;                                                 // Resume widget
    void setup() override;                                                  // Setup widget
    void loop() override;                                                   // Update and draw the widget
    inline const WidgetState getState() const override { return _state; }   // Get the current state of the widget
    inline const std::string getName() const override { return _name; }     // Return the name of the widget
    inline void setName(const std::string &name) override { _name = name; } // Set the name of the widget

    uint32_t getDisplayTime() const override;                                                            // Return the display time in ms
    WidgetFlags getAction() const override;                                                            // Return the widget action
    inline void setDisplayTime(uint32_t displayTime) override { _displayTime = displayTime; } // Set display time
    
    inline void setAction(uint8_t action) override { _action = static_cast<WidgetFlags>(action); }      // Set the widget action
    inline void addAction(uint8_t action) override { _action = static_cast<WidgetFlags>(_action | action); }     // Add an action to the widget
    inline void removeAction(uint8_t action) override { _action = static_cast<WidgetFlags>(_action & ~action); } // Remove an action from the widget



    void setDisplayModule(i2cDisplay *displayModule) override; // Set the display module
    i2cDisplay *getDisplayModule() const override;             // Get the display module

    // Additional public methods
    void setQRCodeText(const std::string &text); // Update the text for the QR code
    void setBackgroundWhite(bool white);         // Set the background to white or black

  private:
    uint32_t _displayTime;        // Time to display the widget in ms
    WidgetFlags _action;        // Action to perform after the widget
    WidgetState _state;           // Current state of the widget
    i2cDisplay *_display;         // Display object
    std::string _qrCodeText;      // Text for the QR code
    std::string _defaultText;     // Default text to display
    uint32_t _lastUpdateTime = 0; // Last update time
    std::string _name = "QRCode"; // Name of the widget

    bool _needsRedraw = true; // Flag indicating if redraw is needed
    bool _backgroundWhite;    // Flag to set background color (true = white, false = black)

    // Private methods
    void drawQRCode(); // Draw the QR code on the display
};