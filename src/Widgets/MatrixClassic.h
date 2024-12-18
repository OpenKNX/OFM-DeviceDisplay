#pragma once
#include "../Widget.h"

// WidgetMatrixClassic class: Simulates a classic matrix character rain screensaver
class WidgetMatrixClassic : public Widget
{
  public:
    const std::string logPrefix() { return "WidgetMatrixClassic"; }
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
    static constexpr uint8_t maxColumns = 16; // Maximum number of columns
    static constexpr uint8_t maxDrops = 5; // Maximum number of drops per column
    static constexpr uint8_t columnWidth = 8; // Width of a column in pixels

    int _MatrixDropPos[maxColumns][maxDrops];
    static const char cp437[]; // Classic CP437 character set

    void initMatrix();
    void updateMatrix();
};