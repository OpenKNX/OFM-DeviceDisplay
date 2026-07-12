#ifdef DEVICE_DISPLAY_MODULE
#pragma once
/**
 * @file        ButtonManager.h
 * @brief       Button input handler for 5-way navigation
 * @details     Manages hardware button input (UP/DOWN/LEFT/RIGHT/OK) with
 *              press/release detection, long-press, debouncing and widget forwarding
 * @version     0.0.1
 * @date        2025-02-15
 * @copyright   Copyright (c) 2025, Erkan Çolak
 *              Licensed under GNU GPL v3.0
 **/

#include "OpenKNX.h"
#include "ButtonEvent.h"
#include "../WidgetsManager.h"
#include <stdint.h>
#include <functional>

// Forward declaration
class WidgetsManager;
class DeviceDisplay;

// ButtonManager class
class ButtonManager
{
  public:
    ButtonManager(WidgetsManager* widgetManager);
    ~ButtonManager();

    const std::string logPrefix() { return "ButtonManager"; }

    bool setup();
    void loop();
    void setEnabled(bool enabled); // Enable or disable button input

    // Route raw button events through the gesture engine (in DeviceDisplay) BEFORE they reach the
    // active widget. When a router is set, processButton() hands each event to it and does NOT
    // forward to the widget itself: DeviceDisplay decides short-press (navigate) vs gesture
    // (consume) and calls the widget when appropriate.
    void setGestureRouter(DeviceDisplay* router) { _gestureRouter = router; }
    inline bool isEnabled() const { return _enabled; } // Return button enabled state
    inline void setCheckInterval(uint32_t intervalMs) { _checkInterval = intervalMs; } // Set button check interval

    // Continuous hold-duration query API. Mirrors the debounced press-state that loop() already
    // tracks so a gesture layer can poll "who is held and for how long" WITHOUT consuming the
    // existing PRESS/RELEASE/LONG_PRESS events. Reads reflect the last debounced sample (<=50ms old).
    static constexpr size_t ButtonCount = 5; // UP/DOWN/SELECT/LEFT/RIGHT (index order matches loop())

    // True while the button at index is currently held down (debounced, LEFT already de-inverted).
    inline bool isButtonDown(size_t index) const { return (index < ButtonCount) && _buttonPressed[index]; }

    // Milliseconds the button at index has been held; 0 if not currently held or index invalid.
    inline uint32_t getHeldDurationMs(size_t index) const
    {
        if (index >= ButtonCount || !_buttonPressed[index]) return 0;
        return millis() - _buttonPressTime[index];
    }

    // ButtonType of the currently held button; if several are held, the lowest index wins.
    // Returns false when nothing is held (out-param left untouched).
    bool getHeldButton(ButtonType& outType) const;

    // Convenience: index (0..4) of the currently held button, or -1 when nothing is held.
    int getHeldButtonIndex() const;

    // True while any navigation button is currently held down.
    bool isAnyButtonDown() const;

  private:
    WidgetsManager* _widgetManager = nullptr;

    // Optional gesture router (DeviceDisplay). When non-null, raw events are handed to it instead
    // of being forwarded to the active widget directly (see processButton()).
    DeviceDisplay* _gestureRouter = nullptr;

    // Hardware pins
    uint16_t _buttonUp = 0;
    uint16_t _buttonDown = 0;
    uint16_t _buttonSelect = 0;
    uint16_t _buttonLeft = 0;
    uint16_t _buttonRight = 0;

    // State tracking
    bool _enabled = false;
    uint32_t _lastCheck = 0;
    uint32_t _checkInterval = 50;  // 50ms debounce

    // Button press state (5 buttons: UP/DOWN/SELECT/LEFT/RIGHT)
    bool _buttonPressed[5] = {false};
    uint32_t _buttonPressTime[5] = {0};

    // Button processing
    ButtonEvent* checkButton(uint16_t pin, ButtonType type, size_t index);
    bool readButton(uint16_t pin);
    void processButton(uint16_t pin, ButtonType type, size_t index);
};
#endif // DEVICE_DISPLAY_MODULE