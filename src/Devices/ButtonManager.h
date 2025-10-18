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

// Forward declaration
class WidgetsManager;

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
    inline bool isEnabled() const { return _enabled; } // Return button enabled state
    inline void setCheckInterval(uint32_t intervalMs) { _checkInterval = intervalMs; } // Set button check interval

  private:
    WidgetsManager* _widgetManager = nullptr;

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