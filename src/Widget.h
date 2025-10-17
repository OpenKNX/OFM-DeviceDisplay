#pragma once
/**
 * @file        Widget.h
 * @brief       This module offers a widget manager for displays on the OpenKNX ecosystem
 * @version     0.0.1
 * @date        2024-12-28
 * @copyright   Copyright (c) 2024, Erkan Çolak (erkan@çolak.de)
 *              Licensed under GNU GPL v3.0
 */

#include "ButtonEvent.h"
#include "devices/i2cDisplay.h"
#include <cstdint>

typedef enum : uint8_t
{
    NoAction = 0,          // Keine Aktion - Widget bleibt in der Queue
    StatusWidget = 1,      // Status-Widget (hat immer Vorrang)
    AutoRemove = 2,        // Automatische Entfernung nach Anzeige
    ManagedExternally = 4, // Extern verwaltet, bleibt aktiv, bis deaktiviert
    DisplayEnabled = 8,    // Intern aktiv, wird auf dem Display angezeigt
    Background = 16,       // Widget läuft im Hintergrund
    DefaultWidget = 32,    // Standard-Widget, wird angezeigt, wenn kein anderes Widget aktiv ist
    WantsButtonInput = 64  // Widget möchte Button-Eingaben erhalten
} WidgetFlags;

enum class WidgetState
{
    STOPPED,   // Widget is not running
    RUNNING,   // Widget is actively displayed and looping
    PAUSED,    // Widget is paused (e.g. ProgMode)
    BACKGROUND // Widget is running in the background
};

enum class WidgetPriority : uint8_t
{
    LOW = 0,     // Default widgets, animations (not used for StatusWidgets)
    NORMAL = 1,  // Menu activation
    HIGH = 2,    // Warnings (UseCase specific warnings - e.g., no Screensaver configured)
    CRITICAL = 3 // Critical errors (ProgMode, System failure, etc.)
};

class Widget
{
  protected:
    WidgetPriority _priority = WidgetPriority::NORMAL;

  public:

    // Core functions
    virtual void setup() = 0;                       // setup the widget
    virtual void start() = 0;                       // start the widget
    virtual void stop() = 0;                        // stop the widget
    virtual void pause() = 0;                       // pause the widget (will pause the current state and all internal timers, values, etc.)
    virtual void resume() = 0;                      // resume the widget (will continue the current state and all internal timers, values, etc.)
    virtual void loop() = 0;                        // loop the widget
    virtual void background() {}                    // Optional - put the widget in background mode (will continue to run, but not be displayed)
    
    // Getters / Setters
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

    virtual bool handleButtonEvent(const ButtonEvent &event) { return false; }
    virtual bool wantsButtonInput() const
    {
        return (static_cast<uint8_t>(getAction()) & WantsButtonInput) != 0;
    }

    virtual WidgetPriority getPriority() const { return _priority; }
    virtual void setPriority(WidgetPriority priority) { _priority = priority; }
};