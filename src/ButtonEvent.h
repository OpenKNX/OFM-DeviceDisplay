#pragma once

#include <cstdint>
#include <Arduino.h>

/**
 * @brief Button types available on the front plate
 */
enum class ButtonType : uint8_t
{
    UP,
    DOWN,
    SELECT,
    LEFT,
    RIGHT
};

/**
 * @brief Button actions that can be detected
 */
enum class ButtonAction : uint8_t
{
    PRESS,           // Short press
    LONG_PRESS,      // Long press (>500ms)
    VERY_LONG_PRESS, // Very long press (>5000ms)
    RELEASE          // Button released
};

/**
 * @brief Button event structure
 */
struct ButtonEvent
{
    ButtonType type;
    ButtonAction action;
    uint32_t timestamp;

    ButtonEvent(ButtonType t, ButtonAction a)
        : type(t), action(a), timestamp(millis()) {}
};