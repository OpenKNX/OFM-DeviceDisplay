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

    #include "ddc_console.h"
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
class DdcConsole;
class i2cDisplay;
class WidgetConsole;
class WidgetProgMode;
class WidgetGestureOverlay;
class WidgetAbout;
class WidgetOTA;
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
 * - Console commands (via DdcConsole)
 * - ProgMode handling
 */
class DeviceDisplay : public OpenKNX::Module
{
  private:
    // Core components
    i2cDisplay* _displayModule = nullptr;
    WidgetsManager* _widgetManager = nullptr;
    ButtonManager* _buttonManager = nullptr;
    DdcConsole* _ddcConsole = nullptr;

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
    WidgetOTA* _otaWidget = nullptr;                 // SYSTEM-priority OTA update overlay (over everything)

    // "Display" hardware tuning: live-previewed values NOT yet persisted. The store holds the saved
    // value; these hold the currently-applied one. disp_save copies pending -> store. Boot/apply/KONAMI
    // seed pending FROM the store. So the menu always shows what is live, save commits it.
    bool _pendDispRotate = false;
    uint8_t _pendDispPrechargeIdx = 5;
    uint8_t _pendDispRefreshIdx = 3;

    uint8_t _konamiPos = 0; // KONAMI unbrick-sequence match position (see handleButtonEvent)

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

    // --- Screenshot (RIGHT-hold gesture / "ddc screenshot") -------------------------------------
    // The gesture callback and console command only REQUEST; the SD write runs from loop() (a
    // blocking write in a button callback reboots the RP2040). The CLEAN framebuffer is frozen into
    // _shotBuf BEFORE the confirm overlay appears, so the saved image never shows the overlay/toast.
    enum class ShotState : uint8_t { Idle, Rows, Close, Done, Failed };
    static constexpr uint16_t SHOT_MAX_BYTES = 1024;  // 128 * 64 / 8
    static constexpr uint16_t SHOT_ROWS_PER_TICK = 8; // chunked write, well under the loop budget
    static constexpr uint32_t SHOT_TOAST_MS = 1800;   // on-screen confirmation hold time
    uint8_t _shotBuf[SHOT_MAX_BYTES] = {0};           // frozen framebuffer snapshot (no heap)
    ShotState _shotState = ShotState::Idle;
    bool _shotPending = false;                        // a request is latched (snapshot already frozen)
    bool _shotFrozen = false;                         // _shotBuf holds a valid frozen frame
    uint16_t _shotW = 0, _shotH = 0, _shotRow = 0;
    uint16_t _nextShot = 0;                           // cached next /screenshot_NNN.bmp index
    std::string _shotPath;                            // current target path (for the toast)
    std::string _toastMsg;                            // transient on-screen message
    uint32_t _toastUntil = 0;                         // millis() deadline for the toast (0 = none)

    void captureFramebuffer();              // freeze the live framebuffer into _shotBuf
    void screenshotTick();                  // loop-driven writer FSM
    void drawToast();                       // render the current toast

    // Internal functions
    void handleProgMode();
    void handleOTA(); // poll NetworkModule OTA status -> drive the OTA overlay
    // Push EVERY persisted display setting onto the live runtime (power-save, brightness, invert,
    // autoPaging, screensaver type, icon-menu, home-key map). Single canonical path used at boot AND
    // after a settings reset, so the two never diverge and no switch is left unapplied.
    void applyAllSettingsToRuntime();
    // KONAMI unbrick: advance the sequence matcher with one button PRESS; returns true (and restores +
    // saves defaults) when the full code is entered. Works blind (display off/misconfigured).
    bool matchKonami(ButtonType t);
    void triggerKonamiRestore();
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

    // Brief on-screen message (held SHOT_TOAST_MS). Public so other modules (e.g. OFM-SDCard format
    // feedback) can surface a status message on the display.
    void showToast(const std::string& msg);

    // True while a button is held (a gesture hold is armed). Lets a long background job (e.g. the SD
    // Low-Level format) yield for that moment so the gesture countdown overlay animates smoothly.
    inline bool isGestureActive() const { return _gestureButtonArmed; }

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

    // Console (ddc config) hooks: after mutating the store via getSettingsStore(), push it live +
    // persist + refresh the menu; or restore all defaults the same way (also the KONAMI action).
    void consoleApplyAndSave();
    void consoleResetToDefaults();

    // Read-only access to the RAM settings store (e.g. for the "ddc i" info dump).
    // Non-mutating callers should prefer this const overload so they cannot dirty the store.
    const DisplaySettingsStore& settingsStore() const { return _settingsStore; }

    // Access to the hold-to-confirm gesture engine.
    GestureEngine& getGestureEngine() { return _gestureEngine; }

    // Route a raw button event through the gesture engine BEFORE the widget (called
    // by the ButtonManager). Decides short-press (navigate) vs gesture (consume).
    void handleButtonEvent(const ButtonEvent& event);

    // Force the UI back to a known state: menu closed, About/overlays gone, gesture disarmed, all
    // the "swallow the next event" latches cleared, display awake. Exists because reaching the home
    // screen by injecting keys depends on where you currently are -- which makes automated tests
    // guess. This does not guess.
    void forceHome();

    // Let the menu release / retake the screen for an externally-managed modal background widget
    // (the SD file browser). Two background widgets carrying DisplayEnabled|ManagedExternally at
    // once make WidgetsManager::findActiveBackgroundWidget() return the FIRST one in list order —
    // the menu — so the modal gets drawn while the menu silently eats the navigation. The modal
    // must therefore call this with false on open and true when it hands control back.
    // The menu keeps its tree and cursor; only DisplayEnabled is toggled.
    void setMenuDisplayEnabled(bool on);

    // Capture the framebuffer and save it as a BMP to SD, driven from loop() (non-blocking).
    // captureNow=true freezes the live frame immediately (console "ddc screenshot"); the gesture path
    // pre-freezes the clean frame before the overlay and calls with captureNow=false.
    void requestScreenshot(bool captureNow = true);

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
