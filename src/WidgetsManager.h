/**
 * @file        WidgetsManager.h
 * @version     1.0
 * @date        2024-06-10
 * @brief       Manages the widget queue and state machine for display widgets.
 * @copyright   Copyright (c) 2024, Erkan Çolak (erkan@colak.de)
 *              Licensed under GNU GPL v3.0
 * @details     This class handles the lifecycle of widgets, including their setup,
 *              activation, deactivation, and transitions between different power save modes.
 *              It supports priority widgets, background widgets, and default widgets.
 *              The power save modes include ACTIVE, DIMMED, SCREENSAVER, SLEEP, and OFF.
 *              Callbacks can be registered for state transitions and power save mode changes.
 *              User interactions can reset power save timers and wake up the display.
 **/

/**
 * @section State Machine Overview
 *
 * The WidgetsManager operates as a state machine for managing and displaying widgets.
 *
 * Visualization of widget types and states:
 *
 *   +-------------------+      +-------------------+      +-------------------+      +-------------------+
 *   |     STARTUP       | ---> |       IDLE        | ---> |     PRIORITY      | ---> |    BACKGROUND     |
 *   |-------------------|      |-------------------|      |-------------------|      |-------------------|
 *   | BootLogoWidget    |      |                   |      | StatusWidget      |      |  MenuWidget       |
 *   | AutoRemoveWidget  |      |                   |      | ProgModeWidget    |      |  SettingsWidget   |
 *   +-------------------+      +-------------------+      +-------------------+      +-------------------+
 * *                                      |                                                      |
 *                                        |                                                      |
 *                                        v                                                      v
 *                                +-------------------+                               +-------------------+
 *                                |     DEFAULT       | <-----------------------------|     DEFAULT       |
 *                                |-------------------|                               |-------------------|
 *                                | ClockWidget       |                               | All widgets       |
 *                                | Cube3DWidget      |                               | rotate here       |
 *                                | PongWidget        |                               |                   |
 *                                | ConsoleWidget     |                               |                   |
 *                                | (rotation)        |                               |                   |
 *                                +-------------------+                               +-------------------+
 *
 * Legend of widget types::
 *   - BootLogoWidget, AutoRemoveWidget: Displayed at startup (STARTUP).
 *   - StatusWidget, ProgModeWidget: Highest priority (PRIORITY).
 *   - MenuWidget, SettingsWidget: Background widgets, e.g., menus (BACKGROUND).
 *   - ClockWidget, Cube3DWidget, PongWidget, ConsoleWidget: All are DefaultWidgets (DEFAULT state)
 *     → Permanent DefaultWidgets (i.e. Clock): Always available
 *     → Temporary DefaultWidgets (i.e. Pong, Console): Can be auto-removed (AutoRemove flag)
 *     → All DefaultWidgets rotate in the same DEFAULT state!
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
 *   - While DEFAULT is active, a StatusWidget is activated.
 *   - Immediate switch to PRIORITY, after deactivation back to DEFAULT.
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
 **/

#pragma once
#ifdef DEVICE_DISPLAY_MODULE
    #include "Widget.h"
    #include <algorithm>
    #include <deque>
    #include <functional>
    #include <string>
    #include <vector>

// Forward declaration
class Widget;
class i2cDisplay;
struct DisplaySettings; // mapped onto PowerSaveConfig via applyDisplaySettings()

enum class WidgetManagerState : uint8_t // State of the WidgetManager, see state machine below
{
    Startup = 0,    // Boot sequence (BootLogo, AutoRemove widgets)
    Idle = 1,       // No widget active
    Priority = 2,   // StatusWidget (e.g., ProgMode) active - highest priority
    Background = 3, // Background widget (e.g., Menu) active
    Default = 4     // DefaultWidget (e.g., Clock, QRCode) active - fallback
};

enum class PowerSaveMode : uint8_t // Power save mode states are used for display: brightness and power management
{
    Active = 0,      // Display is fully active
    Dimmed = 1,      // Display is dimmed (e.g., 30% brightness)
    Screensaver = 2, // Screensaver is active (e.g., Matrix, Clock)
    Sleep = 3,       // Display is off, but can be reactivated
    Off = 4          // Display is completely off
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
    void logWidgetManagerSettings();
    void userInteraction(); // Call this on user input (button press, etc.)

    WidgetManagerState getState() const { return _state; }
    const char* getStateName() const;
    Widget* getCurrentWidget() const { return _currentWidget; }

    PowerSaveMode getPowerSaveMode() const { return _powerSaveMode; }
    const char* getPowerSaveModeName(PowerSaveMode mode) const;

    PowerSaveConfig& getPowerSaveConfig() { return _powerSaveConfig; }

    Widget* getActiveButtonWidget();
    void wakeUpDisplay();

    // Force the display OFF immediately, independent of the inactivity timers. Back-dates the
    // interaction time and transitions to the deepest configured off/sleep mode so it STAYS off
    // until a real interaction (any button -> userInteraction/wakeUpDisplay).
    void forceDisplayOff();

    // Linear 0.0f..1.0f rotation progress for the currently rotating DefaultWidget; returns
    // ROTATION_PROGRESS_NONE when there is no time-based rotation (background/priority/screensaver
    // widgets, or rotation disabled). millis() overflow-safe.
    static constexpr float ROTATION_PROGRESS_NONE = -1.0f;
    float getRotationProgress() const;

    // Pause / manual mode for the DefaultWidget rotation: no time-based auto switch, only manual
    // switching (next/prevWidgetManual). Resuming restarts the display time from now.
    void pauseRotation();
    void resumeRotation();
    void toggleRotationPause();
    bool isRotationPaused() const { return _rotationPaused; }

    // Manually switch the rotation by one DefaultWidget in persistent user order, skipping
    // disabled widgets. Always switch even while paused and reset the display timer. Wraps around.
    void nextWidgetManual();
    void prevWidgetManual();

    // Page/rotation status text for the Home overlay: "n/N" (1-based) for a multi-page current
    // widget, "manuell" when paused and single-page, otherwise empty.
    std::string getPageStatusText() const;

    // Per-widget display duration (ms), by widget name. Takes effect on the next switch to that
    // widget. getWidgetDisplayTime() returns 0 for an unknown name.
    void setWidgetDisplayTime(const std::string& widgetName, uint32_t displayTimeMs);
    uint32_t getWidgetDisplayTime(const std::string& widgetName) const;

    // Persistent DefaultWidget display order (_defaultOrder), decoupled from the rotate-to-end
    // queue housekeeping, so a full rotation round leaves the user order intact.
    std::vector<Widget*> getDefaultWidgetsInOrder() const;

    // Auto-paging: a multi-page current DefaultWidget splits its display duration evenly across its
    // pages (dur/N per page) and advances during the loop, rotating on only after the last page.
    // Manual page flipping and rotation-pause both suppress the automatic page advance.
    void setAutoPaging(bool enabled);
    bool isAutoPagingEnabled() const { return _autoPagingEnabled; }

    // Manual page flipping of the current widget. Wraps around and rescales the rotation/paging
    // timer so the chosen page gets its full per-page slice. No-op for single-page/non-rotating.
    void pageUp();
    void pageDown();

    // Reorder / grab API for the persistent DefaultWidget order. grabWidget(i) picks up the widget
    // at display index i; moveGrabbedUp/Down() swap it with its neighbour (no wrap); the grabbed
    // index follows the move; dropWidget() releases. Mutations edit _defaultOrder in place.
    std::vector<Widget*> getReorderableWidgets() const;
    bool grabWidget(size_t index);
    bool isGrabbing() const { return _grabIndex >= 0; }
    int getGrabIndex() const { return _grabIndex; }
    bool moveGrabbedUp();
    bool moveGrabbedDown();
    void dropWidget();

    // Derive PowerSaveConfig from persisted DisplaySettings (screensaver/sleep timeout indices,
    // normal brightness). autoDim == false disables the DIMMED stage (dimTimeout = 0 -> "never").
    void applyDisplaySettings(const DisplaySettings& settings);

  private:
    i2cDisplay* _displayModule = nullptr;
    std::vector<Widget*> _widgetQueue;
    Widget* _currentWidget = nullptr;

    uint32_t _currentTime = 0;            // Scheduled end time (millis) of the current widget; UINT32_MAX = infinite/no rotation
    uint32_t _currentWidgetStartTime = 0; // millis() when the current widget was switched in (rotation progress base)
    uint32_t _lastInteractionTime = 0;
    uint32_t _idleTimeout = 10000; // 10 seconds default idle timeout

    WidgetManagerState _state = WidgetManagerState::Startup;
    WidgetManagerState _previousState = WidgetManagerState::Startup;
    StateTransitionCallback _stateTransitionCallback;
    bool _startupComplete = false;
    bool _isInitialized = false; // Indicates if setup() has been called

    // Manual/paused rotation mode: no time-based auto switch, only manual next/prevWidgetManual.
    bool _rotationPaused = false;

    // Persistent DefaultWidget display order, independent of the rotateWidgetToEnd housekeeping
    // that shuffles _widgetQueue; kept in sync lazily with the queue (see syncDefaultOrder).
    std::vector<Widget*> _defaultOrder;

    bool _autoPagingEnabled = true;

    // Index (into _defaultOrder) of the currently grabbed widget in reorder mode, -1 if none.
    int _grabIndex = -1;

    // Power save
    PowerSaveConfig _powerSaveConfig;
    PowerSaveMode _powerSaveMode = PowerSaveMode::Active;
    PowerSaveCallback _powerSaveCallback;
    bool _forcedOff = false; // display manually forced off; stays off until a real interaction

    // Screensaver widget
    Widget* _screenSaverWidget = nullptr;
    // uint32_t _screenSaverFallbackStartTime = 0; // ToDo: Fallback handling
    // bool isScreenSaverValid() const;    // Idea: Internal Widget for fallback display
    // void displayScreenSaverWarning();   // With text "Screensaver Widget not set!" --> going off

    void updateState(uint32_t currentTime);
    void transitionTo(WidgetManagerState newState);

    void handleStartupState(uint32_t currentTime);
    void handleIdleState(uint32_t currentTime);
    void handlePriorityState(uint32_t currentTime);
    void handleBackgroundState(uint32_t currentTime);
    void handleDefaultState(uint32_t currentTime);

    // Shared pause indicator, drawn over the current DefaultWidget view.
    void drawPauseOverlay();

    void handleCurrentWidget(uint32_t currentTime);

    void loopBackgroundWidgets();

    void updatePowerSaveMode(uint32_t currentTime);
    void transitionToPowerSaveMode(PowerSaveMode newMode);

    // Widget finders
    Widget* findNextPriorityWidget();
    Widget* findNextStartupWidget();
    Widget* findNextDefaultWidget();
    Widget* findActiveBackgroundWidget();

    void switchToWidget(Widget* widget, uint32_t currentTime, const char* reason);
    void rotateWidgetToEnd(Widget* widget);
    bool shouldRotateWidgets() const;
    bool isIdleTimeoutReached(uint32_t currentTime) const;
    bool hasOnlyDefaultWidgets() const;

    // Keep _defaultOrder consistent with the DefaultWidgets currently in the queue: append
    // newly-seen ones (preserving queue order) and drop any that are gone. No-op once stable.
    void syncDefaultOrder();
    // DefaultWidget adjacent to the currently shown one in persistent order, skipping disabled
    // widgets. dir = +1 next, -1 previous. Wraps.
    Widget* findAdjacentDefaultWidget(int dir);
    void switchToDefaultWidgetManual(Widget* widget, uint32_t currentTime, const char* reason);

    // Advance the current DefaultWidget's page to the slice matching the elapsed display time
    // (dur/N per page) when auto-paging is enabled. No-op for single-page/non-rotating or paused.
    void updateAutoPaging(uint32_t currentTime);

    std::vector<Widget*> _backgroundWidgets; // Cache for background widgets
    void rebuildBackgroundCache();           // Rebuilds the cache only when widgets are added/removed
};
#endif // DEVICE_DISPLAY_MODULE