#pragma once
#include "../Widget.h"

// WidgetMatrixClassic class: Simulates a classic matrix character rain screensaver
class WidgetMatrixClassic : public Widget
{
  public:
    WidgetMatrixClassic(uint32_t displayTime, WidgetsAction action, uint8_t intensity);

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

    unsigned long _lastUpdateScreenSaver;
    uint16_t FallSpeed; // Speed of drops depending on intensity

    // Matrix drop positions
    static constexpr uint8_t maxColumns = 32;
    static constexpr uint8_t maxDrops = 6;
    static constexpr uint8_t columnWidth = 4;

    int _MatrixDropPos[maxColumns][maxDrops];
    static const char cp437[]; // Classic CP437 character set

    void initMatrix();
    void updateMatrix();
};