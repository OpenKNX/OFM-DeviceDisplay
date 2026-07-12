#ifdef DEVICE_DISPLAY_MODULE
#pragma once
#include "../Widget.h"

/**
 * @brief WidgetTime - Rotation home widget showing live date + time + seconds.
 *
 * Renders the current time as a large "HH:MM" with a smaller ":SS" appended,
 * a date line "Wochentag, DD.MM.YYYY" and a small uptime line. The visible
 * seconds are updated with a per-second partial redraw; the ticker keeps
 * running even while the widget is PAUSED (so the clock never freezes).
 *
 * If no valid time source is available it shows a centered
 * "Kein Datum / Uhrzeit gesetzt" fallback (without left labels) plus the
 * uptime and exposes a "keine Zeit" badge for the caller.
 *
 * There is intentionally NO mode switch (analog/digital) (decision 2026-07-07).
 * Time/date come from the available time source (OGM-Common DateTime /
 * NTP / KNX time via openknx.time); uptime comes from buildUptime.
 *
 * Screensaver reuse: this widget is self-contained and may be reused
 * as the "Uhr" screensaver. The distinction to the animated screensaver-clock
 * is purely which widget the WidgetsManager selects; WidgetTime carries no
 * screensaver-specific state, so a single instance can serve both roles.
 */
class WidgetTime : public Widget
{
  public:
    const std::string logPrefix() { return "WidgetTime"; } // Prefix for log output
    WidgetTime(uint32_t displayTime, WidgetFlags action);

    void setup() override;
    void start() override;
    void stop() override;
    void pause() override;
    void resume() override;
    void loop() override;
    inline const WidgetState getState() const override { return _state; }
    inline const std::string getName() const override { return _name; }
    inline void setName(const std::string &name) override { _name = name; }

    uint32_t getDisplayTime() const override;                                                 // Returns the display time in milliseconds
    WidgetFlags getAction() const override;                                                    // Returns the widget action
    inline void setDisplayTime(uint32_t displayTime) override { _displayTime = displayTime; }  // Set display time

    inline void setAction(uint8_t action) override { _action = static_cast<WidgetFlags>(action); }                // Set the widget action
    inline void addAction(uint8_t action) override { _action = static_cast<WidgetFlags>(_action | action); }      // Add an action to the widget
    inline void removeAction(uint8_t action) override { _action = static_cast<WidgetFlags>(_action & ~action); }  // Remove an action from the widget

    void setDisplayModule(i2cDisplay *displayModule) override; // Sets the display module
    i2cDisplay *getDisplayModule() const override;             // Retrieves the display module

    // True when no valid time source is available. Callers (badge/UI) can use
    // this to show the "keine Zeit" badge as specified in the mock.
    bool isTimeMissing() const;

  private:
    void drawTime();                                        // Renders the current frame into the display buffer
    void drawWithTime();                                    // Frame variant when a valid time is available
    void drawNoTime();                                      // Centered "keine Zeit" fallback (no left labels)
    void drawCenteredText(int16_t y, uint8_t textSize, const char *text); // Horizontally centered single line

    // Returns the current visible "second key" used to gate the per-second
    // redraw: the wall-clock second when time is valid, otherwise the uptime
    // second. Redrawing only when this key changes keeps the ticker cheap.
    uint32_t currentSecondKey() const;

    uint32_t _displayTime;         // Duration the widget is displayed in milliseconds
    WidgetFlags _action;           // Action assigned to the widget
    i2cDisplay *_display;          // Pointer to the display module
    WidgetState _state;            // Current state of the widget
    uint32_t _lastSecondKey;       // Last rendered second key (for per-second redraw)
    bool _forceRedraw;             // Force a full redraw on next tick (e.g. after start/resume)
    std::string _name = "Time";    // Name of the widget
};
#endif // DEVICE_DISPLAY_MODULE
