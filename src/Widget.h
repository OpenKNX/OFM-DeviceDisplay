#pragma once
#include <cstdint>
#include "i2c-Display.h"

typedef enum : uint8_t {
    NoAction = 0,           // No action
    StatusFlag = 1,         // Status widget
    AutoRemoveFlag = 2,     // Auto-remove
    InternalEnabled = 4,    // Internal enabled
    ExternalManaged = 8,    // External managed
    MarkedForRemove = 16    // Marked for remove
} WidgetsAction;

enum class WidgetState {
    STOPPED,
    RUNNING,
    PAUSED
};

class Widget {
public:
    virtual void setup() = 0;    // setup the widget
    virtual void start() = 0;    // start the widget
    virtual void stop() = 0;     // stop the widget
    virtual void pause() = 0;    // pause the widget (will pause the current state and all internal timers, values, etc.)
    virtual void resume() = 0;   // resume the widget (will continue the current state and all internal timers, values, etc.)
    virtual void loop() = 0;     // loop the widget
    virtual const WidgetState getState() const = 0;  // Get the current state of the widget
    
    virtual uint32_t getDisplayTime() const = 0; // Time to display the widget in ms
    virtual WidgetsAction getAction() const = 0;   // Widget action flag
    virtual uint32_t setDisplayTime(uint32_t displayTime) = 0; // Set the display time
    virtual void setAction(uint8_t action) = 0; // Set the widget action
    virtual void addAction(uint8_t action) = 0; // Add an action to the widget
    virtual void removeAction(uint8_t action) = 0; // Remove an action from the widget

    virtual ~Widget() = default;
    virtual void setDisplayModule(i2cDisplay *displayModule) = 0;
    virtual i2cDisplay *getDisplayModule() const = 0;
    virtual const std::string getName() const = 0;
    virtual void setName(const std::string &name) = 0;
};