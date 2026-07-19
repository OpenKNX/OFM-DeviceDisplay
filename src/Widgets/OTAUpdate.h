#ifdef DEVICE_DISPLAY_MODULE
#pragma once
/**
 * @file        OTAUpdate.h
 * @brief       Full-screen OTA update progress overlay widget (WIDGET_PRIO_SYSTEM takeover)
 * @version     0.0.1
 * @date        2026-07-12
 * @copyright   Copyright (c) 2026, Erkan Çolak (erkan@colak.de)
 *              Licensed under GNU GPL v3.0
 */
#include "../Widget.h"

// Blink cadence of the "alive" activity dot while an OTA update runs.
#define OTA_BLINK_DELAY 400

// Full-screen OTA progress overlay. Driven externally by DeviceDisplay (polls the NetworkModule's
// otaActive()/otaProgress()); this widget only owns the drawing. Runs at WIDGET_PRIO_SYSTEM so it
// takes over the display above ProgMode and every other status overlay.
class WidgetOTA : public Widget
{
  public:
    const std::string logPrefix() { return "WidgetOTA"; }

    WidgetOTA(uint32_t displayTime = 0, WidgetFlags action = WidgetFlags::NoAction);
    ~WidgetOTA();

    void setup() override;
    void start() override;
    void stop() override;
    void pause() override;
    void resume() override;
    void loop() override;
    inline const WidgetState getState() const override { return _state; }
    inline uint32_t getDisplayTime() const override { return _displayTime; }
    inline WidgetFlags getAction() const override { return _action; }
    inline void setDisplayTime(uint32_t displayTime) override { _displayTime = displayTime; }

    inline void setAction(uint8_t action) override { _action = static_cast<WidgetFlags>(action); }
    inline void addAction(uint8_t action) override { _action = static_cast<WidgetFlags>(_action | action); }
    inline void removeAction(uint8_t action) override { _action = static_cast<WidgetFlags>(_action & ~action); }

    inline void setDisplayModule(i2cDisplay *displayModule) override { _display = displayModule; }
    inline i2cDisplay *getDisplayModule() const override { return _display; }
    inline const std::string getName() const override { return _name; }
    inline void setName(const std::string &name) override { _name = name; }

    // Live progress fed by DeviceDisplay each poll (0..100). Forces a redraw when the value changes.
    void setProgress(uint8_t percent);

  private:
    i2cDisplay *_display;
    WidgetFlags _action;
    uint32_t _displayTime;
    WidgetState _state;
    std::string _name = "OTAUpdate";
    unsigned long _lastBlinkTime = 0;
    bool _blinkOn = true;
    uint8_t _percent = 0;

    void draw();

    /** partial-draw state: 0 = done, 1..n = drawing step */
    uint8_t _drawStep = 0;
};
#endif // DEVICE_DISPLAY_MODULE
