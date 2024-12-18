#pragma once
#include "../Widget.h"

class WidgetSysInfoLite : public Widget
{
  public:
    const std::string logPrefix() { return "WidgetSysInfoLite"; } // Prefix for log output
    WidgetSysInfoLite(uint32_t displayTime, WidgetsAction action);

    void setup() override;
    void start() override;
    void stop() override;
    void pause() override;
    void resume() override;
    void loop() override;

    uint32_t getDisplayTime() const override; // Returns the display time in milliseconds
    WidgetsAction getAction() const override; // Returns the widget action

    void setDisplayModule(i2cDisplay *displayModule) override; // Sets the display module
    i2cDisplay *getDisplayModule() const override;             // Retrieves the display module

  private:
    void drawSysInfo();                              // Draws the system information on the screen
    void invertBitmap( uint8_t *bitmap, size_t width, size_t height); // Inverts the bitmap

    uint32_t _displayTime; // Duration the widget is displayed in milliseconds
    WidgetsAction _action; // Action assigned to the widget
    i2cDisplay *_display;  // Pointer to the display module
    WidgetState _state;    // Current state of the widget
    bool _invertBitmap;    // Tracks if the bitmap should be inverted
    uint32_t _lastUpdate;  // Last update time
};