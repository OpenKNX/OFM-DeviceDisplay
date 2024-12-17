#pragma once

#include <cstdint>
#include "i2c-Display.h"
#include "OpenKNX.h"

typedef enum : uint8_t {
    NoAction = 0,           // No action
    StatusFlag = 1,         // Status widget
    AutoRemoveFlag = 2,     // Auto-remove
    InternalEnabled = 4,    // Internal enabled
    ExternalManaged = 8,    // External managed
    MarkedForRemove = 16    // Marked for remove
} WidgetsAction;

class Widget {
public:
    const std::string logPrefix() { return "DeviceDisplay: "; }
    virtual void setup() = 0;    // setup the widget
    virtual void start() = 0;    // start the widget
    virtual void stop() = 0;     // stop the widget
    virtual void pause() = 0;    // pause the widget (will pause the current state and all internal timers, values, etc.)
    virtual void resume() = 0;   // resume the widget (will continue the current state and all internal timers, values, etc.)
    virtual void loop() = 0;     // loop the widget
    virtual uint32_t getDisplayTime() const = 0; // Time to display the widget in ms
    virtual WidgetsAction getAction() const = 0;   // Widget action flag
    virtual ~Widget() = default;
    virtual void setDisplayModule(i2cDisplay *displayModule) = 0;
    virtual i2cDisplay *getDisplayModule() const = 0;
};