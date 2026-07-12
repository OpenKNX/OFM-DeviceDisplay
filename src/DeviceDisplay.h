#pragma once
/**
 * @file        DeviceDisplay.h
 * @brief       Main display module for OpenKNX ecosystem
 * @version     0.0.1
 * @date        2024-12-17
 * @copyright   Copyright (c) 2024, Erkan Çolak
 *              Licensed under GNU GPL v3.0
 **/

#ifdef DEVICE_DISPLAY_MODULE
    #define WIDGET_MANAGER

    #define WIDGET_CONSOLE             // Enable console widget
    #define DISPLAY_LOW_LEVEL_COMMANDS // Enable low-level commands

    #define DeviceDisplay_Display_Name "DeviceDisplay"

    #include "DDCLoggerHelp.h"
    #include "Devices/ButtonManager.h"
    #include "Devices/i2cDisplay.h"
    #include "Gesture/GestureEngine.h"
    #include "Menu/MenuConfig.h"
    #include "Menu/MenuRegistry.h"
    #include "OpenKNX.h"
    #include "Settings/DisplaySettingsStore.h"
    #include "WidgetsManager.h"

class WidgetsManager;
class ButtonManager;
class DDCLoggerHelp;
class i2cDisplay;
class WidgetConsole;
class WidgetProgMode;
class WidgetGestureOverlay;
class WidgetAbout;
class MenuWidget;
class MenuRegistry;
class Widget;

// Screensaver selection. setScreenSaverType() owns the value->widget mapping.
// The enum value == dropdown option index AND is the persisted field (screenSaverType),
// so only APPEND new screensavers at the end (before Off) once shipped, or existing saves break.
enum class ScreenSaverType : uint8_t
{
    Clock = 0,         // WidgetClock (analog)
    Cube3D = 1,        // WidgetCube3D
    Doom = 2,          // WidgetDoom (raycaster, SciFi easter egg)
    Fireworks = 3,     // WidgetFireworks
    Life = 4,          // WidgetLife (Conway)
    Matrix = 5,        // WidgetMatrix
    MatrixClassic = 6, // WidgetMatrixClassic
    Pong = 7,          // WidgetPong
    Rain = 8,          // WidgetRain
    Starfield = 9,     // WidgetStarfield
    Off = 10           // no screensaver widget -> WidgetsManager goes straight to SLEEP
};

// Number of selectable screensaver entries. Keep in sync with the enum + the dropdown options.
static constexpr uint8_t SCREENSAVER_TYPE_COUNT = 11;

    // Hardware validation
    #ifndef OKNXHW_DEVICE_DISPLAY_I2C_INST
ERROR_REQUIRED_DEFINE(OKNXHW_DEVICE_DISPLAY_I2C_INST);
    #endif
    #ifndef OKNXHW_DEVICE_DISPLAY_I2C_SDA
ERROR_REQUIRED_DEFINE(OKNXHW_DEVICE_DISPLAY_I2C_SDA);
    #endif
    #ifndef OKNXHW_DEVICE_DISPLAY_I2C_SCL
ERROR_REQUIRED_DEFINE(OKNXHW_DEVICE_DISPLAY_I2C_SCL);
    #endif
    #ifndef OKNXHW_DEVICE_DISPLAY_I2C_ADDRESS
ERROR_REQUIRED_DEFINE(OKNXHW_DEVICE_DISPLAY_I2C_ADDRESS);
    #endif
    #ifndef OKNXHW_DEVICE_DISPLAY_WIDTH
ERROR_REQUIRED_DEFINE(OKNXHW_DEVICE_DISPLAY_WIDTH);
    #endif
    #ifndef OKNXHW_DEVICE_DISPLAY_HEIGHT
ERROR_REQUIRED_DEFINE(OKNXHW_DEVICE_DISPLAY_HEIGHT);
    #endif

/**
 * @brief Main Display Module
 *
 * Orchestrates:
 * - Display hardware (I2C SSD1306)
 * - Widget system (via WidgetsManager)
 * - Button input (via ButtonManager)
 * - Console commands (via DDCLoggerHelp)
 * - ProgMode handling
 */
class DeviceDisplay : public OpenKNX::Module
{
  private:
    // Core components
    i2cDisplay* _displayModule = nullptr;
    WidgetsManager* _widgetManager = nullptr;
    ButtonManager* _buttonManager = nullptr;
    DDCLoggerHelp* _ddcLoggerHelp = nullptr;

    // Central menu-item registry (module member, not a global singleton)
    MenuRegistry* _menuRegistry = nullptr;

    // RAM-side persisted display + widget settings (no flash access here)
    DisplaySettingsStore _settingsStore;

    // Widget references
    WidgetConsole* _consoleWidget = nullptr;
    MenuWidget* _menuWidget = nullptr;               // kept so setup() can hand it the registry
    WidgetProgMode* _progModeWidget = nullptr;       // exclusive prog widget
    WidgetGestureOverlay* _gestureOverlay = nullptr; // hold/auto confirm overlay
    WidgetAbout* _aboutWidget = nullptr;             // menu "Über" SciFi HUD (shown on demand)

    // About HUD state: shown from the menu, dismissed by any button or an auto-timeout.
    bool _aboutActive = false;            // About HUD currently displayed
    bool _aboutSwallowRelease = false;    // eat the RELEASE paired with a dismissing PRESS
    bool _menuExitSwallowRelease = false; // eat the RELEASE that ends a LEFT-long-press menu exit
    // After a gesture forces the display OFF, swallow every event from its still-held button
    // until RELEASE, so the held button cannot re-wake the panel; a fresh press afterwards reacts.
    bool _displayOffSwallow = false;
    ButtonType _displayOffButton = ButtonType::SELECT;
    uint32_t _aboutShownAt = 0;                         // millis() when shown (for the auto-timeout)
    static constexpr uint32_t ABOUT_TIMEOUT_MS = 20000; // auto-dismiss after ~8 credit cycles

    // Hold-to-confirm gesture engine. Owned by the module (value member so it
    // lives as long as DeviceDisplay). Wired to the WidgetsManager/knx callbacks in init().
    GestureEngine _gestureEngine;

    // Per-button gesture bookkeeping. When a directional/OK PRESS arrives we arm the
    // engine; on RELEASE we decide short-press (navigate) vs gesture-consumed (no navigation).
    ButtonType _heldGestureButton = ButtonType::SELECT; // which button armed the current hold
    bool _gestureButtonArmed = false;                   // a hold is currently armed for _heldGestureButton
    // Latched once the armed gesture reaches Counting/fires, so the eventual release is
    // consumed even if the engine already folded back to Idle while the button was still held.
    bool _gestureReachedBar = false;

    // Remember whether we forced the prog-exclusive display so we can restore normal
    // operation (rotation + screensaver) on exit.
    bool _progExclusiveActive = false;
    Widget* _savedScreenSaverWidget = nullptr; // screensaver suppressed during prog, restored after

    // The screensaver instance owned by DeviceDisplay (the WidgetsManager only borrows it),
    // so we delete the old one ourselves when switching the screensaver type.
    Widget* _screenSaverOwned = nullptr;

    // Internal functions
    void handleProgMode();
    void initializeWidgets();

    // Register the display-submenu onValueChanged callbacks and wire the
    // screen/gesture hooks the MenuWidget declared but left unset. Called once from setup().
    void wireMenuCallbacks();

    // Re-seed the GestureEngine's Home-key map from the persisted display settings (used at
    // init/setup and after a "Home-Tasten" menu change).
    void seedGestureKeyMapFromSettings();

    // Is the Home screen currently shown? Directional gestures only arm there.
    bool isHomeScreen() const;

    // On the Home screen a short directional press steps through the widget rotation manually.
    void handleHomeNavigation(ButtonType type);

    void updateGestureOverlay();

    // Show / dismiss the About HUD (menu "Über"). See the implementation for the display handoff.
    void showAbout();
    void hideAbout();

    // Enter/leave the prog-exclusive display mode (suppress rotation/menu/screensaver).
    void enterProgExclusive();
    void leaveProgExclusive();

  public:
    DeviceDisplay();
    ~DeviceDisplay();

    // OpenKNX::Module interface
    const std::string logPrefix() { return DeviceDisplay_Display_Name; } // Logger prefix

    inline const std::string name() { return DeviceDisplay_Display_Name; }      // Library name
    inline const std::string version() { return MODULE_DeviceDisplay_Version; } // Library version

    void init() override;
    void setup(bool configured) override;
    void loop(bool configured) override;
    #if (MASK_VERSION & 0x0900) != 0x0900 // Couplers (e.g. IP-Router 0x091A) have no GroupObjects
    void processInputKo(GroupObject& ko) override;
    #endif
    void showHelp() override;
    bool processCommand(const std::string command, bool diagnose) override;

    // Fixed, constant flash reservation for the DeviceDisplay module.
    // = 1(format version) + sizeof(DisplaySettings) + 1(widget count)
    //   + WIDGET_SETTINGS_MAX * sizeof(WidgetSetting).
    uint16_t flashSize() override;

    // Persist / restore the DisplaySettingsStore blob (1-byte format version +
    // serialize()); readFlash() validates the version or falls back to defaults.
    void writeFlash() override;
    void readFlash(const uint8_t* data, const uint16_t size) override;

    // Public accessors (for testing/debugging)
    i2cDisplay* getDisplayModule() { return _displayModule; }
    WidgetsManager* getWidgetManager() { return _widgetManager; }
    ButtonManager* getButtonManager() { return _buttonManager; }

    // Access point for modules to register their own menu items.
    // Mirrors getWidgetManager(); may be nullptr before init() has run.
    MenuRegistry* getMenuRegistry() { return _menuRegistry; }

    // RAM settings store (display + per-widget settings)
    DisplaySettingsStore& getSettingsStore() { return _settingsStore; }

    // Read-only access to the RAM settings store (e.g. for the "ddc i" info dump).
    // Non-mutating callers should prefer this const overload so they cannot dirty the store.
    const DisplaySettingsStore& settingsStore() const { return _settingsStore; }

    // Access to the hold-to-confirm gesture engine.
    GestureEngine& getGestureEngine() { return _gestureEngine; }

    // Route a raw button event through the gesture engine BEFORE the widget (called
    // by the ButtonManager). Decides short-press (navigate) vs gesture (consume).
    void handleButtonEvent(const ButtonEvent& event);

    // Select the active screensaver from the full family. Off -> nullptr (manager goes
    // straight to SLEEP). The previous widget is freed before the new one is installed.
    void setScreenSaverType(ScreenSaverType type);

    // Presence-guarded registration helpers, no-ops when the manager/registry is absent.
    // tryAddWidget takes ownership of `widget` (deleted on the no-op path to avoid a leak).
    bool tryAddWidget(Widget* widget);
    bool tryRegisterRootItems(const std::vector<MenuConfig::MenuOption>& items);
    bool tryRegisterRootItem(const MenuConfig::MenuOption& item);
    bool tryRegisterAction(const std::string& key, std::function<void()> fn);

    #ifdef WIDGET_CONSOLE
    WidgetConsole* getConsoleWidget() { return _consoleWidget; }
    #endif
};

extern DeviceDisplay openknxDisplayModule;

#endif // DEVICE_DISPLAY_MODULE