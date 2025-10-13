/**
 * @file        WidgetsManager.h
 * @version     1.0
 * @date        2024-06-10
 * @brief       Manages the widget queue and state machine for display widgets.
 * @copyright   Copyright (c) 2024, Erkan Çolak (erkan@çolak.de)
 *              Licensed under GNU GPL v3.0
 * @details     This class handles the lifecycle of widgets, including their setup,
 *              activation, deactivation, and transitions between different power save modes.
 *              It supports priority widgets, background widgets, default widgets, and normal widgets.
 *              The power save modes include ACTIVE, DIMMED, SCREENSAVER, SLEEP, and OFF.
 *              Callbacks can be registered for state transitions and power save mode changes.
 *              User interactions can reset power save timers and wake up the display.
 */

/**
 * @section State Machine Overview
 * 
 * The WidgetsManager operates as a state machine for managing and displaying widgets.
 * 
 * Visualization of widget types and states:
 * 
 *   +-------------------+      +-------------------+      +-------------------+      +-------------------+      +-------------------+
 *   |     STARTUP       | ---> |       IDLE        | ---> |     PRIORITY      | ---> |    BACKGROUND     | ---> |      NORMAL       |
 *   |-------------------|      |-------------------|      |-------------------|      |-------------------|      |-------------------|
 *   | BootLogoWidget    |      |                   |      | StatusWidget      |      |  MenuWidget       |      |  InfoWidget       |
 *   | AutoRemoveWidget  |      |                   |      | ProgModeWidget    |      |  SettingsWidget   |      |  DataWidget       |
 *   +-------------------+      +-------------------+      +-------------------+      +-------------------+      +-------------------+
 *                                                                                                                        |
 *                                                                                                                        v
 *                                                                                                               +-------------------+
 *                                                                                                               |     DEFAULT       |
 *                                                                                                               |-------------------|
 *                                                                                                               | ClockWidget       |
 *                                                                                                               | QRCodeWidget      |
 *                                                                                                               +-------------------+
 * 
 * Legend of widget types::
 *   - BootLogoWidget, AutoRemoveWidget: Displayed at startup (STARTUP).
 *   - StatusWidget, ProgModeWidget: Highest priority (PRIORITY).
 *   - MenuWidget, SettingsWidget: Background widgets, e.g., menus (BACKGROUND).
 *   - InfoWidget, DataWidget: Normal widgets (NORMAL).
 *   - ClockWidget, QRCodeWidget: Default display when nothing else is active (DEFAULT).
 * 
 * Power Save Modes:
 *   +-----------+    +---------+    +--------------+    +-------+    +-----+
 *   |  ACTIVE   | -> | DIMMED  | -> | SCREENSAVER  | -> | SLEEP | -> | OFF |
 *   +-----------+    +---------+    +--------------+    +-------+    +-----+
 * 
 * Transitions occur after timeouts or user interaction.
 * 
 * @section Example Scenarios
 * 
 * Example 1: Startup and Default Widget
 *   - On startup: STARTUP → IDLE → DEFAULT (e.g., ClockWidget).
 * 
 * Example 2: Priority Widget Interrupt
 *   - While NORMAL or DEFAULT is active, a StatusWidget is activated.
 *   - Immediate switch to PRIORITY, after deactivation back to NORMAL or DEFAULT.
 * 
 * Example 3: Power Save Sequence
 *   - After dimTimeout: DIMMED.
 *   - After screenSaverTimeout: SCREENSAVER (e.g., MatrixWidget).
 *   - After sleepTimeout: SLEEP.
 *   - After offTimeout: OFF.
 *   - Any user interaction resets everything to ACTIVE.
 * 
 * Example 4: Background Widget
 *   - User opens menu by pressing a button (MenuWidget): switch to BACKGROUND.
 *   - After a predefined idle the menu closes, return to previous state.
 */

#pragma once

#include "Widget.h"
#include <algorithm>
#include <deque>
#include <functional>

enum class WidgetManagerState : uint8_t // State of the WidgetManager, see state machine below
{
    STARTUP = 0,    // Boot sequence (BootLogo, AutoRemove widgets)
    IDLE = 1,       // No widget active
    PRIORITY = 2,   // StatusWidget (e.g., ProgMode) active - highest priority
    BACKGROUND = 3, // Background widget (e.g., Menu) active
    NORMAL = 4,     // Normal widget active
    DEFAULT = 5     // DefaultWidget (e.g., Clock, QRCode) active - fallback
};

enum class PowerSaveMode : uint8_t // Power save mode states are used for display: brightness and power management
{
    ACTIVE = 0,      // Display voll aktiv, normale Helligkeit
    DIMMED = 1,      // Display gedimmt (z.B. 30% Helligkeit)
    SCREENSAVER = 2, // Screensaver aktiv (z.B. Matrix, Uhr)
    SLEEP = 3,       // Display aus, aber reaktivierbar
    OFF = 4          // Display komplett aus
};

struct PowerSaveConfig // Configuration for power save modes and timeouts
{
    bool enabled = true;                  // Activates power-save functionality
    uint32_t dimTimeout = 120000;         // After 2 minutes dim
    uint32_t screenSaverTimeout = 300000; // After 5 minutes screensaver
    uint32_t sleepTimeout = 600000;       // After 10 minutes sleep
    uint32_t offTimeout = 0;              // 0 = never turn off
    uint8_t dimBrightness = 30;           // 30% brightness when dimmed (ToDo: dimming needs to be improved on i2cDisplay)
    uint8_t normalBrightness = 100;       // 100% normal brightness
};

class WidgetsManager // Manages the widget queue and state machine
{
  public:
    WidgetsManager() = default;
    ~WidgetsManager() = default;
    const std::string logPrefix() { return "WidgetsManager"; }

    using StateTransitionCallback = std::function<void(WidgetManagerState from, WidgetManagerState to)>;
    using PowerSaveCallback = std::function<void(PowerSaveMode from, PowerSaveMode to)>;

    void setDisplayModule(i2cDisplay* displayModule) { _displayModule = displayModule; }
    void setIdleTimeout(uint32_t timeout) { _idleTimeout = timeout; }
    void setStateTransitionCallback(StateTransitionCallback callback) { _stateTransitionCallback = callback; }
    void setPowerSaveCallback(PowerSaveCallback callback) { _powerSaveCallback = callback; }
    void setPowerSaveConfig(const PowerSaveConfig& config) { _powerSaveConfig = config; }
    void setScreenSaverWidget(Widget* widget);

    void addWidget(Widget* widget);
    void setup();
    void start();
    void loop();

    Widget* getWidgetFromQueue(const std::string& widgetName);
    Widget* getWidgetFromQueue(Widget* widget);
    void removeWidgetFromQueue(const char* widgetName);
    void removeWidgetFromQueue(Widget* widget);

    void logWidgetQueue();
    void userInteraction(); // Call this on user input (button press, etc.)

    WidgetManagerState getState() const { return _state; }
    const char* getStateName() const;
    Widget* getCurrentWidget() const { return _currentWidget; }

    PowerSaveMode getPowerSaveMode() const { return _powerSaveMode; }
    const char* getPowerSaveModeName() const;
    PowerSaveConfig& getPowerSaveConfig() { return _powerSaveConfig; }

  private:
    i2cDisplay* _displayModule = nullptr;
    std::deque<Widget*> _widgetQueue;
    Widget* _currentWidget = nullptr;

    uint32_t _currentTime = 0;
    uint32_t _lastInteractionTime = 0;
    uint32_t _idleTimeout = 10000; // 10 seconds default idle timeout

    WidgetManagerState _state = WidgetManagerState::STARTUP;
    WidgetManagerState _previousState = WidgetManagerState::STARTUP;
    StateTransitionCallback _stateTransitionCallback;
    bool _startupComplete = false;

    // Power save
    PowerSaveConfig _powerSaveConfig;
    PowerSaveMode _powerSaveMode = PowerSaveMode::ACTIVE;
    PowerSaveCallback _powerSaveCallback;
    Widget* _screenSaverWidget = nullptr;

    void updateState(uint32_t currentTime);
    void transitionTo(WidgetManagerState newState);

    void handleStartupState(uint32_t currentTime);
    void handleIdleState(uint32_t currentTime);
    void handlePriorityState(uint32_t currentTime);
    void handleBackgroundState(uint32_t currentTime);
    void handleNormalState(uint32_t currentTime);
    void handleDefaultState(uint32_t currentTime);

    void handleCurrentWidget(uint32_t currentTime);

    void loopBackgroundWidgets();

    void updatePowerSaveMode(uint32_t currentTime);
    void transitionToPowerSaveMode(PowerSaveMode newMode);
    void wakeUpDisplay();

    // Widget finders
    Widget* findNextPriorityWidget();
    Widget* findNextStartupWidget();
    Widget* findNextNormalWidget();
    Widget* findNextDefaultWidget();
    Widget* findActiveBackgroundWidget();

    void switchToWidget(Widget* widget, uint32_t currentTime, const char* reason);
    void rotateWidgetToEnd(Widget* widget);
    bool isIdleTimeoutReached(uint32_t currentTime) const;
    bool hasOnlyDefaultWidgets() const;
};