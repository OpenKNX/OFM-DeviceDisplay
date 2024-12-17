#pragma once
#include "../Widget.h"

// WidgetRain class: Simulates falling rain drops as a screensaver
class WidgetRain : public Widget
{
  public:
    WidgetRain(uint32_t displayTime, WidgetsAction action, uint8_t intensity);

    void start() override;
    void stop() override;
    void pause() override;
    void resume() override;
    void setup() override;
    void loop() override;

    uint32_t getDisplayTime() const override;
    WidgetsAction getAction() const override;

    void setDisplayModule(i2cDisplay *displayModule) override;
    i2cDisplay *getDisplayModule() const override;

  private:
    uint32_t _displayTime;
    WidgetsAction _action;
    uint8_t _intensity;
    i2cDisplay *_display;

    static const uint8_t MAX_RAIN_DROPS = 100;
    unsigned long _lastUpdateScreenSaver;
    unsigned long UPDATE_INTERVAL; // Update speed based on intensity
    uint8_t _dropCount;            // Active raindrop count
    uint8_t _dropsX[MAX_RAIN_DROPS];
    uint8_t _dropsY[MAX_RAIN_DROPS];
    bool _initialized;

    void initRain();
    void updateRain();
};