#pragma once
#include "i2c-Display.h"
#include <cstdint>

typedef enum : uint8_t
{
    NoAction = 0,          // Keine Aktion - Widget bleibt in der Queue
    StatusWidget = 1,      // Status-Widget (hat immer Vorrang)
    AutoRemove = 2,        // Automatische Entfernung nach Anzeige
    ManagedExternally = 4, // Extern verwaltet, bleibt aktiv, bis deaktiviert
    DisplayEnabled = 8,    // Intern aktiv, wird auf dem Display angezeigt
    Background = 16        // Widget läuft im Hintergrund
} WidgetFlags;

enum class WidgetState
{
    STOPPED,
    RUNNING,
    PAUSED,
    BACKGROUND
};

class Widget
{
  public:
    virtual void setup() = 0;                       // setup the widget
    virtual void start() = 0;                       // start the widget
    virtual void stop() = 0;                        // stop the widget
    virtual void pause() = 0;                       // pause the widget (will pause the current state and all internal timers, values, etc.)
    virtual void resume() = 0;                      // resume the widget (will continue the current state and all internal timers, values, etc.)
    virtual void loop() = 0;                        // loop the widget
    virtual const WidgetState getState() const = 0; // Get the current state of the widget

    virtual uint32_t getDisplayTime() const = 0;           // Time to display the widget in ms
    virtual WidgetFlags getAction() const = 0;             // Widget action flag
    virtual void setDisplayTime(uint32_t displayTime) = 0; // Set the display time
    virtual void setAction(uint8_t action) = 0;            // Set the widget action
    virtual void addAction(uint8_t action) = 0;            // Add an action to the widget
    virtual void removeAction(uint8_t action) = 0;         // Remove an action from the widget

    virtual ~Widget() = default;
    virtual void setDisplayModule(i2cDisplay *displayModule) = 0;
    virtual i2cDisplay *getDisplayModule() const = 0;
    virtual const std::string getName() const = 0;
    virtual void setName(const std::string &name) = 0;
};