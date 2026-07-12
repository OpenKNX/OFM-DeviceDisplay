#ifdef DEVICE_DISPLAY_MODULE
    #include "DeviceDisplay.h"
    #include "DDCLoggerHelp.h"
    #include "Devices/ButtonManager.h"
    #include "Devices/i2cDisplay.h"
    #include "WidgetsManager.h"
    #include "Menu/MenuRegistry.h"
    #include "Settings/DisplaySettingsStore.h"
    #include "Menu/Menu.h"
    #include "Widgets/About.h"
    #include "Widgets/BootLogo.h"
    #include "Widgets/Clock.h"
    #include "Widgets/GestureOverlay.h"
    #include "Widgets/ProgMode.h"
    #include "Widgets/WidgetTime.h"
    #include "Widgets/Cube3D.h"
    #include "Widgets/Doom.h"
    #include "Widgets/FireWorks.h"
    #include "Widgets/Life.h"
    #include "Widgets/Matrix.h"
    #include "Widgets/MatrixClassic.h"
    #include "Widgets/Pong.h"
    #include "Widgets/Rain.h"
    #include "Widgets/Starfield.h"
    #ifdef WIDGET_CONSOLE
        #include "Widgets/Console.h"
    #endif

    #include <algorithm> // std::min/std::max for index clamping

DeviceDisplay openknxDisplayModule;

#if defined(ARDUINO_ARCH_ESP32)
// Enlarge the ESP32 loopTask stack (default 8 KB): the eager display-root menu build overflows it.
// Lives here (always-linked TU) so the requirement stays a DeviceDisplay concern.
SET_LOOP_TASK_STACK_SIZE(32 * 1024);
#endif

// ============================================================================
// Constructor / Destructor
// ============================================================================

DeviceDisplay::DeviceDisplay()
{
}

DeviceDisplay::~DeviceDisplay()
{
    delete _buttonManager;
    delete _ddcLoggerHelp;
    delete _menuRegistry;
    delete _widgetManager;
    delete _displayModule;
    // DeviceDisplay owns the screensaver instance; free it after the manager is gone.
    delete _screenSaverOwned;
    // _progModeWidget / _gestureOverlay are owned by the WidgetsManager queue - do not double-free.
}

// ============================================================================
// Initialization
// ============================================================================

void DeviceDisplay::init()
{
    logInfoP("Init started...");

    // Create core components
    _displayModule = new i2cDisplay();
    _widgetManager = new WidgetsManager();
    _buttonManager = new ButtonManager(_widgetManager);
    _ddcLoggerHelp = new DDCLoggerHelp(_widgetManager, _displayModule);

    // Central menu-item registry, valid once init() has run so modules can register.
    _menuRegistry = new MenuRegistry();

    if (!_displayModule || !_widgetManager || !_buttonManager || !_ddcLoggerHelp || !_menuRegistry)
    {
        logErrorP("Failed to create components!");
        return;
    }

    _displayModule->lcdSettings.i2cInst = &OKNXHW_DEVICE_DISPLAY_I2C_INST;

    _displayModule->lcdSettings.sda = OKNXHW_DEVICE_DISPLAY_I2C_SDA;
    _displayModule->lcdSettings.scl = OKNXHW_DEVICE_DISPLAY_I2C_SCL;
    _displayModule->lcdSettings.i2cadress = OKNXHW_DEVICE_DISPLAY_I2C_ADDRESS;
    _displayModule->lcdSettings.width = OKNXHW_DEVICE_DISPLAY_WIDTH;
    _displayModule->lcdSettings.height = OKNXHW_DEVICE_DISPLAY_HEIGHT;
    _displayModule->lcdSettings.reset = -1;

    if (!_displayModule->InitDisplay(_displayModule->lcdSettings) || !_displayModule->display)
    {
        logErrorP("Display initialization failed!");
        return;
    }

    logInfoP("Display initialized (%dx%d @ 0x%02X)",
             _displayModule->lcdSettings.width,
             _displayModule->lcdSettings.height,
             _displayModule->lcdSettings.i2cadress);

    // Configure WidgetManager
    _widgetManager->setDisplayModule(_displayModule);
    _widgetManager->setIdleTimeout(10000);

    PowerSaveConfig config;
    config.enabled = true;
    config.dimTimeout = 30000;
    config.screenSaverTimeout = 60000;
    config.sleepTimeout = 300000;
    config.offTimeout = 0;
    config.dimBrightness = 30;
    config.normalBrightness = 100;
    _widgetManager->setPowerSaveConfig(config);

    // Optional callbacks
    _widgetManager->setStateTransitionCallback([this](WidgetManagerState from, WidgetManagerState to) {
        logDebugP("State %d -> %d", from, to);
    });

    _widgetManager->setPowerSaveCallback([this](PowerSaveMode from, PowerSaveMode to) {
        logDebugP("PowerSave %d -> %d", from, to);
    });

    // Gesture engine action callbacks (fired once on Counting -> Firing).
    _gestureEngine.setOnProgToggle([this]() {
        logInfoP("Gesture: toggle ProgMode");
        knx.toggleProgMode();
    });
    _gestureEngine.setOnReboot([this]() {
        logInfoP("Gesture: reboot");
        openknx.restart();
    });
    _gestureEngine.setOnPauseToggle([this]() {
        logInfoP("Gesture: toggle rotation pause");
        if (_widgetManager) _widgetManager->toggleRotationPause();
    });
    _gestureEngine.setOnDisplayOff([this]() { // hold the DisplayOff key -> display off immediately
        logInfoP("Gesture: display off");
        if (_widgetManager) _widgetManager->forceDisplayOff();
        // The mapped button is still held; ignore it until RELEASE so it can't re-wake the panel.
        _displayOffSwallow = true;
        _displayOffButton = _heldGestureButton;
    });

    // Seed the engine key map from the display settings; setup() re-applies after readFlash().
    seedGestureKeyMapFromSettings();

    // Route raw events through DeviceDisplay so the gesture engine sees them before the widget.
    if (_buttonManager)
        _buttonManager->setGestureRouter(this);
}

void DeviceDisplay::seedGestureKeyMapFromSettings()
{
    GestureKeyMap km;
    km.up = static_cast<GestureAction>(_settingsStore.keyAction(HOME_KEY_UP));
    km.down = static_cast<GestureAction>(_settingsStore.keyAction(HOME_KEY_DOWN));
    km.left = static_cast<GestureAction>(_settingsStore.keyAction(HOME_KEY_LEFT));
    km.right = static_cast<GestureAction>(_settingsStore.keyAction(HOME_KEY_RIGHT));
    _gestureEngine.setKeyMap(km);
}

void DeviceDisplay::setup(bool configured)
{
    logDebugP("setup...");

    // Display hardware settings
    _displayModule->SetDisplayVCOMDetect(0x20);
    _displayModule->SetDisplayContrast(0xFF);

    // Initialize widgets
    initializeWidgets();

    // Hand the MenuRegistry to the MenuWidget (consumed lazily, so late registrations
    // still appear on the next rebuild).
    if (_menuWidget && _menuRegistry)
    {
        _menuWidget->setMenuRegistry(_menuRegistry);
    }

    wireMenuCallbacks();

    // Apply the persisted display settings to the runtime (readFlash() already ran):
    // derives the PowerSaveConfig and pushes invert/fontSize to the display.
    _settingsStore.applyToRuntime(_widgetManager, _displayModule);

    // Select the persisted screensaver type (replaces the default seeded in initializeWidgets()).
    setScreenSaverType(static_cast<ScreenSaverType>(_settingsStore.screenSaverType()));

    // Apply the persisted root-menu style (text list vs icon grid) to the menu widget.
    if (_menuWidget) _menuWidget->setIconMenu(_settingsStore.iconMenu());

    // (Re)seed the gesture key map from the restored settings after readFlash().
    seedGestureKeyMapFromSettings();

    // Setup button input
    if (_buttonManager->setup())
    {
        logInfoP("Buttons initialized");
    }

    // Setup help system for console commands
    _ddcLoggerHelp->setup();

    #ifdef WIDGET_CONSOLE
    _ddcLoggerHelp->setConsoleWidget(_consoleWidget);
    #endif
}

/**
 * @brief Register the display-submenu onValueChanged callbacks and wire the MenuWidget
 *        screen/gesture hooks. No-op without a MenuWidget.
 *
 * Each "Anzeige" entry pushes its new value onto the live runtime AND into _settingsStore, then
 * requests a debounced save. Keys mirror DefaultMenus.h; registerOnValueChanged() overwrites by
 * key, replacing the demo defaults from MenuWidget::addDefaultOnValueChanged().
 */
void DeviceDisplay::wireMenuCallbacks()
{
    if (!_menuWidget)
    {
        logDebugP("wireMenuCallbacks: no MenuWidget - hooks/callbacks skipped");
        return;
    }

    // Seed each display option's live defaultValue from the persisted store, so the menu shows saved
    // state after reboot instead of hardcoded defaults (and re-confirm can't silently revert it).
    _menuWidget->setValueSeeder([this](MenuConfig::MenuOption& o) {
        if (o.key == "brightness_level") o.defaultValue = MenuValue(static_cast<size_t>(_settingsStore.brightnessIdx()));
        else if (o.key == "auto_dimming") o.defaultValue = MenuValue(_settingsStore.autoDim());
        else if (o.key == "display_invert") o.defaultValue = MenuValue(_settingsStore.invert());
        else if (o.key == "font_size") o.defaultValue = MenuValue(static_cast<size_t>(_settingsStore.fontSizeIdx()));
        else if (o.key == "auto_paging") o.defaultValue = MenuValue(_settingsStore.autoPaging());
        else if (o.key == "icon_menu") o.defaultValue = MenuValue(_settingsStore.iconMenu());
        else if (o.key == "screensaver_type") o.defaultValue = MenuValue(static_cast<size_t>(_settingsStore.screenSaverType()));
        else if (o.key == "screensaver_after") o.defaultValue = MenuValue(static_cast<size_t>(_settingsStore.screenSaverTimeoutIdx()));
        else if (o.key == "sleep_after") o.defaultValue = MenuValue(static_cast<size_t>(_settingsStore.sleepTimeoutIdx()));
        else if (o.key == "homekey_up") o.defaultValue = MenuValue(static_cast<size_t>(_settingsStore.keyAction(HOME_KEY_UP)));
        else if (o.key == "homekey_down") o.defaultValue = MenuValue(static_cast<size_t>(_settingsStore.keyAction(HOME_KEY_DOWN)));
        else if (o.key == "homekey_left") o.defaultValue = MenuValue(static_cast<size_t>(_settingsStore.keyAction(HOME_KEY_LEFT)));
        else if (o.key == "homekey_right") o.defaultValue = MenuValue(static_cast<size_t>(_settingsStore.keyAction(HOME_KEY_RIGHT)));
    });

    // Helligkeit: Dropdown index 0..3; brightnessIdx maps to (idx+1)*25 % (25/50/75/100).
    _menuWidget->registerOnValueChanged("brightness_level", [this](const MenuConfig::MenuOption&, const MenuValue& val) {
        const size_t idx = std::min(val.getSizeT(), static_cast<size_t>(3));
        _settingsStore.setBrightnessIdx(static_cast<uint8_t>(idx));
        if (_displayModule)
            _displayModule->setBrightness(static_cast<uint8_t>((idx + 1) * 25));
        _settingsStore.requestSave();
        logDebugP("brightness -> idx %u", static_cast<unsigned>(idx));
    });

    // Auto-Dimmen: Checkbox. applyToRuntime() owns the dim behaviour.
    _menuWidget->registerOnValueChanged("auto_dimming", [this](const MenuConfig::MenuOption&, const MenuValue& val) {
        _settingsStore.setAutoDim(val.getBool());
        _settingsStore.applyToRuntime(_widgetManager, _displayModule);
        _settingsStore.requestSave();
        logDebugP("autoDim -> %u", val.getBool() ? 1u : 0u);
    });

    // Invertieren: Checkbox -> i2cDisplay::SetInvertDisplay + persist.
    _menuWidget->registerOnValueChanged("display_invert", [this](const MenuConfig::MenuOption&, const MenuValue& val) {
        _settingsStore.setInvert(val.getBool());
        if (_displayModule)
            _displayModule->SetInvertDisplay(val.getBool());
        _settingsStore.requestSave();
        logDebugP("invert -> %u", val.getBool() ? 1u : 0u);
    });

    // Schriftgroesse: Dropdown 0..2 ("Normal"/"Gross"/"Groesser") -> i2cDisplay::setFontSize.
    _menuWidget->registerOnValueChanged("font_size", [this](const MenuConfig::MenuOption&, const MenuValue& val) {
        const size_t idx = std::min(val.getSizeT(), static_cast<size_t>(2));
        _settingsStore.setFontSizeIdx(static_cast<uint8_t>(idx));
        if (_displayModule)
            _displayModule->setFontSize(static_cast<uint8_t>(idx));
        _settingsStore.requestSave();
        logDebugP("fontSize -> idx %u", static_cast<unsigned>(idx));
    });

    // Home-Tasten: dropdown 0..4 (HomeKeyAction) per direction -> persist + re-seed the gesture engine.
    const auto homeKeyFromIdx = [](size_t idx) -> HomeKeyAction {
        // clamp out-of-range/future index to None
        return (idx <= static_cast<size_t>(HomeKeyAction::DisplayOff))
                   ? static_cast<HomeKeyAction>(idx)
                   : HomeKeyAction::None;
    };
    _menuWidget->registerOnValueChanged("homekey_up", [this, homeKeyFromIdx](const MenuConfig::MenuOption&, const MenuValue& val) {
        _settingsStore.setKeyAction(HOME_KEY_UP, homeKeyFromIdx(val.getSizeT()));
        seedGestureKeyMapFromSettings();
        _settingsStore.requestSave();
        logDebugP("homekey up -> %u", static_cast<unsigned>(val.getSizeT()));
    });
    _menuWidget->registerOnValueChanged("homekey_down", [this, homeKeyFromIdx](const MenuConfig::MenuOption&, const MenuValue& val) {
        _settingsStore.setKeyAction(HOME_KEY_DOWN, homeKeyFromIdx(val.getSizeT()));
        seedGestureKeyMapFromSettings();
        _settingsStore.requestSave();
        logDebugP("homekey down -> %u", static_cast<unsigned>(val.getSizeT()));
    });
    _menuWidget->registerOnValueChanged("homekey_left", [this, homeKeyFromIdx](const MenuConfig::MenuOption&, const MenuValue& val) {
        _settingsStore.setKeyAction(HOME_KEY_LEFT, homeKeyFromIdx(val.getSizeT()));
        seedGestureKeyMapFromSettings();
        _settingsStore.requestSave();
        logDebugP("homekey left -> %u", static_cast<unsigned>(val.getSizeT()));
    });
    _menuWidget->registerOnValueChanged("homekey_right", [this, homeKeyFromIdx](const MenuConfig::MenuOption&, const MenuValue& val) {
        _settingsStore.setKeyAction(HOME_KEY_RIGHT, homeKeyFromIdx(val.getSizeT()));
        seedGestureKeyMapFromSettings();
        _settingsStore.requestSave();
        logDebugP("homekey right -> %u", static_cast<unsigned>(val.getSizeT()));
    });

    // Seiten auto-blaettern: Checkbox -> WidgetsManager::setAutoPaging + persist.
    _menuWidget->registerOnValueChanged("auto_paging", [this](const MenuConfig::MenuOption&, const MenuValue& val) {
        _settingsStore.setAutoPaging(val.getBool());
        if (_widgetManager)
            _widgetManager->setAutoPaging(val.getBool());
        _settingsStore.requestSave();
        logDebugP("autoPaging -> %u", val.getBool() ? 1u : 0u);
    });

    // Icon-Menue: toggle the root icon grid; persist + apply live to the menu widget.
    _menuWidget->registerOnValueChanged("icon_menu", [this](const MenuConfig::MenuOption&, const MenuValue& val) {
        _settingsStore.setIconMenu(val.getBool());
        _menuWidget->setIconMenu(val.getBool());
        _settingsStore.requestSave();
        logDebugP("iconMenu -> %u", val.getBool() ? 1u : 0u);
    });

    // Bildschirmschoner: Dropdown index -> setScreenSaverType; store the raw index.
    _menuWidget->registerOnValueChanged("screensaver_type", [this](const MenuConfig::MenuOption&, const MenuValue& val) {
        const size_t idx = std::min(val.getSizeT(), static_cast<size_t>(SCREENSAVER_TYPE_COUNT - 1));
        _settingsStore.setScreenSaverType(static_cast<uint8_t>(idx));
        setScreenSaverType(static_cast<ScreenSaverType>(idx));
        _settingsStore.requestSave();
        logDebugP("screensaver -> idx %u", static_cast<unsigned>(idx));
    });

    // The screensaver radio picker opens on the value actually in effect (persisted
    // screenSaverType), not the static dropdown default, and the collapsed row reflects it too.
    _menuWidget->registerRadioIndexProvider("screensaver_type", [this]() -> size_t {
        return static_cast<size_t>(_settingsStore.screenSaverType());
    });

    // The brightness slider opens on (and its collapsed row reflects) the persisted level.
    _menuWidget->registerRadioIndexProvider("brightness_level", [this]() -> size_t {
        return static_cast<size_t>(_settingsStore.brightnessIdx());
    });

    // Screensaver nach: Dropdown index -> store timeout index, re-apply.
    _menuWidget->registerOnValueChanged("screensaver_after", [this](const MenuConfig::MenuOption&, const MenuValue& val) {
        _settingsStore.setScreenSaverTimeoutIdx(static_cast<uint8_t>(val.getSizeT()));
        _settingsStore.applyToRuntime(_widgetManager, _displayModule);
        _settingsStore.requestSave();
        logDebugP("screensaverAfter -> idx %u", static_cast<unsigned>(val.getSizeT()));
    });

    // Schlafen nach: Dropdown index -> store sleep timeout index, re-apply, persist.
    _menuWidget->registerOnValueChanged("sleep_after", [this](const MenuConfig::MenuOption&, const MenuValue& val) {
        _settingsStore.setSleepTimeoutIdx(static_cast<uint8_t>(val.getSizeT()));
        _settingsStore.applyToRuntime(_widgetManager, _displayModule);
        _settingsStore.requestSave();
        logDebugP("sleepAfter -> idx %u", static_cast<unsigned>(val.getSizeT()));
    });

    // Gesture hook (prog/reboot).
    _menuWidget->setOnGestureAction([this](const std::string& action, const MenuConfig::MenuOption&) {
        if (action == "prog")
        {
            logInfoP("Menu gesture: toggle ProgMode");
            knx.toggleProgMode();
        }
        else if (action == "reboot")
        {
            logInfoP("Menu gesture: reboot");
            openknx.restart(); // may not return
        }
        else
        {
            logDebugP("Menu gesture: unknown action '%s'", action.c_str());
        }
    });

    // File-browser hook. The SD-card module binds the item's action to the file browser,
    // so forward to item.action() (else the browser never opens).
    _menuWidget->setOnFilesRequested([this](const MenuConfig::MenuOption& item) {
        if (item.action)
        {
            logInfoP("Files requested ('%s') -> activating SD file browser", item.label.c_str());
            item.action();
        }
        else
        {
            logInfoP("Files requested ('%s') - no action bound (SD-card module present?)", item.label.c_str());
        }
    });

    // Screen hooks. About opens the real HUD; the rest remain safe stubs.
    _menuWidget->setOnAboutRequested([this]() {
        logInfoP("About requested -> show HUD");
        showAbout();
    });
    _menuWidget->setOnIpEditRequested([this](const MenuConfig::MenuOption& item) {
        logDebugP("IP edit requested ('%s') (Phase-3c stub)", item.label.c_str());
    });
    _menuWidget->setOnReorderRequested([this](const MenuConfig::MenuOption& item) {
        logDebugP("Reorder requested ('%s') (Phase-3c stub)", item.label.c_str());
    });

    logInfoP("Menu callbacks wired");
}

/**********************************************************************
 ********************** Initialize Default Widgets ********************
 **********************************************************************/
void DeviceDisplay::initializeWidgets()
{
    // Boot logo (first widget, auto-remove after 3s)
    _widgetManager->addWidget(new WidgetBootLogo(3000, WidgetFlags::AutoRemove));

    // Screensaver (default Matrix). Tracked in _screenSaverOwned so setScreenSaverType()
    // can free it (the WidgetsManager only borrows the pointer).
    _screenSaverOwned = new WidgetMatrixClassic(5000, WidgetFlags::AutoRemove, 8);
    _widgetManager->setScreenSaverWidget(_screenSaverOwned);

    #ifdef WIDGET_CONSOLE
    // Console widget (persistent)
    _consoleWidget = new WidgetConsole(60000);
    _widgetManager->addWidget(_consoleWidget);
    #endif

    // Clock (default widget)
    _widgetManager->addWidget(
        new WidgetClock(5000, WidgetFlags::DefaultWidget, false));

    // WidgetTime - live date/time rotation widget. Ticks per-second even while paused.
    _widgetManager->addWidget(
        new WidgetTime(4500, WidgetFlags::DefaultWidget));

    // Menu (background widget). Kept as a member so setup() can hand it the MenuRegistry.
    _menuWidget = new MenuWidget(10000, WidgetFlags::ManagedExternally);
    _widgetManager->addWidget(_menuWidget);

    // ProgMode (CRITICAL status widget). Kept as a member for handleProgMode().
    _progModeWidget = new WidgetProgMode();
    _progModeWidget->setAction(WidgetFlags::ManagedExternally | WidgetFlags::StatusWidget);
    _progModeWidget->setPriority(WidgetPriority::WIDGET_PRIO_CRITICAL);
    _widgetManager->addWidget(_progModeWidget);

    // Hold/auto confirm overlay. CRITICAL StatusWidget shown on demand while a gesture is live.
    _gestureOverlay = new WidgetGestureOverlay();
    _gestureOverlay->setEngine(&_gestureEngine);
    _gestureOverlay->setMode(GestureOverlayMode::Hold);
    _gestureOverlay->setAction(WidgetFlags::ManagedExternally | WidgetFlags::StatusWidget | WidgetFlags::WantsButtonInput);
    _gestureOverlay->setPriority(WidgetPriority::WIDGET_PRIO_CRITICAL);
    _widgetManager->addWidget(_gestureOverlay);

    // About: "Über" HUD shown on demand from the menu; any button or an auto-timeout dismisses it.
    // CRITICAL StatusWidget so it draws over the menu/rotation.
    _aboutWidget = new WidgetAbout(0, WidgetFlags::ManagedExternally);
    _aboutWidget->setAction(WidgetFlags::ManagedExternally | WidgetFlags::StatusWidget | WidgetFlags::WantsButtonInput);
    _aboutWidget->setPriority(WidgetPriority::WIDGET_PRIO_CRITICAL);
    _widgetManager->addWidget(_aboutWidget);

    _widgetManager->setup();
    _widgetManager->start();

    logInfoP("Widgets initialized");
}

    #if (MASK_VERSION & 0x0900) != 0x0900 // Couplers (e.g. IP-Router 0x091A) have no GroupObjects
void DeviceDisplay::processInputKo(GroupObject& obj)
{
    // TODO: Implement KO processing for display control
}
    #endif

/**
 * @brief Main loop for device display
 */
void DeviceDisplay::loop(bool configured)
{
    static bool displayErrorShown = false;
    if (!_displayModule->display)
    {
        if (!displayErrorShown)
        {
            logErrorP("Display not initialized");
            displayErrorShown = true;
        }
        return;
    }

    // Commit any settled settings change here, at a shallow loop-level stack point - NOT
    // from the button callback, where a flash write reboots the RP2040.
    _settingsStore.tickSave();

    // Process button input (routes raw events through DeviceDisplay::handleButtonEvent()).
    _buttonManager->loop();

    // Advance the gesture state machine so a held button progresses between PRESS/RELEASE.
    _gestureEngine.tick(millis());

    // Latch once the armed gesture reaches the confirm bar (Counting) or fires, so the
    // release is consumed even if the FSM already folded back to Idle while the button was held.
    if (_gestureButtonArmed && !_gestureReachedBar)
    {
        const GesturePhase p = _gestureEngine.getPhase();
        if (p == GesturePhase::Counting || p == GesturePhase::Firing ||
            p == GesturePhase::Done)
        {
            _gestureReachedBar = true;
        }
    }

    // Show/hide the confirm overlay while a gesture is live (PreRoll..Done).
    updateGestureOverlay();

    // About HUD: auto-dismiss after a while so it never gets stuck if the user walks away.
    if (_aboutActive && (millis() - _aboutShownAt) > ABOUT_TIMEOUT_MS) hideAbout();

    // Handle ProgMode widget (ETS path + prog-exclusive display)
    handleProgMode();

    // Update display (only when CPU time available)
    if (openknx.freeLoopTime())
    {
        _widgetManager->loop();
    }
}

/**
 * @brief Route a raw button event through the gesture engine BEFORE the widget.
 *
 * A PRESS arms a hold for the resolved action (advanced in loop()::tick()). On release we decide:
 * gesture reached Counting/Firing/Done -> consumed, no navigation; otherwise a short press ->
 * forward a single PRESS to the active widget. Any input wakes the display and resets timers.
 */
void DeviceDisplay::handleButtonEvent(const ButtonEvent& event)
{
    if (!_widgetManager) return;

    // A gesture forced the display OFF and its button is still held. Swallow every event from
    // that button until RELEASE (before userInteraction()) so it cannot re-wake the panel.
    if (_displayOffSwallow && event.type == _displayOffButton)
    {
        if (event.action != ButtonAction::PRESS) // RELEASE / LONG_PRESS / VERY_LONG_PRESS = button let go
            _displayOffSwallow = false;
        return;
    }

    // Any input wakes the display / resets the timers -- EXCEPT the RELEASE of a button that armed a
    // gesture: a DisplayOff gesture would be instantly undone by its own release. Defer
    // userInteraction() for such releases until we know the gesture was NOT consumed.
    const bool releaseOfArmedGesture = (event.action != ButtonAction::PRESS) &&
                                       _gestureButtonArmed && event.type == _heldGestureButton;
    if (!releaseOfArmedGesture)
        _widgetManager->userInteraction();

    // About screen: any button dismisses it and returns to the menu. Dismiss on the PRESS, then
    // swallow the paired RELEASE so it neither navigates the menu nor arms a gesture.
    if (_aboutActive)
    {
        if (event.action == ButtonAction::PRESS)
        {
            hideAbout();
            _aboutSwallowRelease = true;
        }
        return;
    }
    if (_aboutSwallowRelease && event.action != ButtonAction::PRESS)
    {
        _aboutSwallowRelease = false;
        return;
    }
    // Swallow the RELEASE (and any later hold ticks) that end a LEFT-long-press menu exit, so the
    // paired release does not step the widget rotation after the menu already closed.
    if (_menuExitSwallowRelease)
    {
        if (event.action == ButtonAction::RELEASE) _menuExitSwallowRelease = false;
        return;
    }

    const bool isPress = (event.action == ButtonAction::PRESS);

    if (isPress)
    {
        // Arm a hold for the action this button maps to (OK -> ProgMode; directional -> keyMap on
        // the Home screen). None -> nothing to arm.
        const GestureAction action = _gestureEngine.resolveAction(event.type, isHomeScreen());
        if (action != GestureAction::None)
        {
            _gestureEngine.startHold(action);
            _gestureButtonArmed = true;
            _gestureReachedBar = false; // fresh hold: bar not reached yet
            _heldGestureButton = event.type;
        }
        else
        {
            _gestureButtonArmed = false;
            _gestureReachedBar = false;
        }

        // Wake the display on any press even if nothing is armed / no widget is active.
        if (!_widgetManager->getActiveButtonWidget())
            _widgetManager->wakeUpDisplay();

        // A PRESS never navigates on its own: navigation happens on the release of a SHORT press.
        return;
    }

    // Release event (RELEASE / LONG_PRESS / VERY_LONG_PRESS).
    // Only the button that armed the gesture ends it; a different button's release cannot.
    bool gestureConsumed = false;
    if (_gestureButtonArmed && event.type == _heldGestureButton)
    {
        const GesturePhase phase = _gestureEngine.getPhase();
        // Consumed when the confirm bar ran / the action fired (observed live or latched via
        // _gestureReachedBar). PreRoll released before the bar started is a short press -> navigate.
        gestureConsumed = _gestureReachedBar ||
                          (phase == GesturePhase::Counting ||
                           phase == GesturePhase::Firing ||
                           phase == GesturePhase::Done);

        _gestureEngine.endHold(); // PreRoll/Counting -> Aborted; Firing/Done stays fired
        _gestureButtonArmed = false;
        _gestureReachedBar = false;
    }

    if (gestureConsumed)
    {
        // Consumed gesture: NO userInteraction() so a DisplayOff gesture is not re-woken by its own release.
        logDebugP("Gesture consumed release of button %d (no navigation)", static_cast<int>(event.type));
        return;
    }

    // Genuine short press on a gesture-armed button (bar never ran): do the deferred wake now.
    if (releaseOfArmedGesture)
        _widgetManager->userInteraction();

    // HOLDING LEFT closes the whole menu from anywhere; a short LEFT still navigates.
    if ((event.action == ButtonAction::LONG_PRESS || event.action == ButtonAction::VERY_LONG_PRESS) &&
        event.type == ButtonType::LEFT && _menuWidget &&
        _widgetManager->getActiveButtonWidget() == static_cast<Widget*>(_menuWidget))
    {
        logInfoP("Menu: LEFT long-press -> exit menu");
        _menuWidget->externalClose();
        _menuExitSwallowRelease = true; // eat the paired RELEASE so it does not step the rotation
        return;
    }

    // Short press: forward a single synthetic PRESS to the active widget.
    Widget* activeWidget = _widgetManager->getActiveButtonWidget();
    ButtonEvent shortPress(event.type, ButtonAction::PRESS);

    // Forward to the focused widget first. Open menu consumes navigation; closed consumes only a short OK.
    if (activeWidget && activeWidget->handleButtonEvent(shortPress))
    {
        _widgetManager->userInteraction();
        return;
    }

    // Not consumed -> Home screen (menu closed): directional presses step the widget rotation.
    if (isHomeScreen())
        handleHomeNavigation(event.type);
    else
        _widgetManager->wakeUpDisplay();
}

/**
 * @brief Manual widget-rotation navigation on the Home screen (menu closed).
 *
 * LEFT/RIGHT step through the DefaultWidget rotation; UP/DOWN page within the current widget.
 */
void DeviceDisplay::handleHomeNavigation(ButtonType type)
{
    if (!_widgetManager) return;
    switch (type)
    {
        case ButtonType::LEFT:
            _widgetManager->prevWidgetManual();
            break;
        case ButtonType::RIGHT:
            _widgetManager->nextWidgetManual();
            break;
        // Up/Down page within the current widget's internal pages (no-op on single-page widgets).
        case ButtonType::UP:
            _widgetManager->pageUp();
            break;
        case ButtonType::DOWN:
            _widgetManager->pageDown();
            break;
        default:
            _widgetManager->wakeUpDisplay();
            return;
    }
    _widgetManager->userInteraction();
}

/**
 * @brief True while the Home screen is shown, i.e. the WidgetsManager is NOT in its
 *        Background (menu) state. Directional gestures only arm here.
 */
bool DeviceDisplay::isHomeScreen() const
{
    if (!_widgetManager) return false;
    return _widgetManager->getState() != WidgetManagerState::Background;
}

/**
 * @brief Drive the confirm overlay widget from the gesture engine phase (Counting..Done
 *        visible). Uses the same DisplayEnabled toggle as the ProgMode widget.
 */
void DeviceDisplay::updateGestureOverlay()
{
    if (!_gestureOverlay || !_widgetManager) return;

    // Show the overlay from Counting through Done, NOT during PreRoll: a short tap must not flash it
    // before it is resolved as navigation.
    const GesturePhase phase = _gestureEngine.getPhase();
    // DisplayOff hides the panel on fire; keep its overlay OUT of Firing/Done or the priority-wake
    // (it's a StatusWidget) turns the display right back on. Its countdown still shows during Counting.
    const bool ack = (phase == GesturePhase::Firing || phase == GesturePhase::Done);
    const bool shouldShow = (phase == GesturePhase::Counting) ||
                            (ack && _gestureEngine.getCurrentAction() != GestureAction::DisplayOff);

    Widget* w = static_cast<Widget*>(_gestureOverlay);
    const bool isShown = (static_cast<uint8_t>(w->getAction()) & DisplayEnabled) != 0;

    if (shouldShow && !isShown)
    {
        // In prog mode WidgetProgMode (also CRITICAL) would hide the overlay -> suppress it for
        // the overlay's lifetime, and hand it the prog state so its label reads correctly.
        const bool progActive = knx.progMode();
        _gestureOverlay->setProgActive(progActive);
        _gestureOverlay->setPauseActive(_widgetManager->isRotationPaused()); // Pause->FORTSETZEN
        if (progActive && _progModeWidget)
            _progModeWidget->removeAction(WidgetFlags::DisplayEnabled);
        w->addAction(DisplayEnabled);
    }
    else if (!shouldShow && isShown)
    {
        w->removeAction(DisplayEnabled);
        // Restore the prog widget if prog mode is STILL active (an aborted deactivate).
        if (knx.progMode() && _progModeWidget)
            _progModeWidget->addAction(WidgetFlags::DisplayEnabled);
    }
}

/**
 * @brief Show the About HUD (menu "Über"). CRITICAL StatusWidget drawn over the menu/rotation; the
 * ProgMode widget is suppressed while shown (restored on hide). Dismissed on any button / timeout.
 */
void DeviceDisplay::showAbout()
{
    if (!_aboutWidget) return;
    _aboutActive = true;
    _aboutSwallowRelease = false;
    _aboutShownAt = millis();
    _aboutWidget->restart(); // fresh credit cycle each time it opens
    if (knx.progMode() && _progModeWidget)
        _progModeWidget->removeAction(WidgetFlags::DisplayEnabled);
    _aboutWidget->addAction(DisplayEnabled);
    logInfoP("About HUD shown");
}

void DeviceDisplay::hideAbout()
{
    if (!_aboutWidget || !_aboutActive) return;
    _aboutActive = false;
    _aboutWidget->removeAction(DisplayEnabled);
    if (knx.progMode() && _progModeWidget)
        _progModeWidget->addAction(WidgetFlags::DisplayEnabled);
    logInfoP("About HUD dismissed");
}

/**
 * @brief Mirror the KNX programming mode onto the display.
 *
 * Reacts to knx.progMode() regardless of HOW it was entered (hold gesture or ETS). On enter force
 * the prog-exclusive display (only the ProgMode widget); on exit restore normal operation.
 */
void DeviceDisplay::handleProgMode()
{
    static bool wasActive = false;

    const bool isActive = knx.progMode();

    if (isActive && !wasActive)
    {
        _widgetManager->userInteraction();
        if (_progModeWidget && _progModeWidget->getState() != WidgetState::RUNNING)
            _progModeWidget->addAction(WidgetFlags::DisplayEnabled);

        enterProgExclusive(); // suppress rotation/menu/screensaver
        logInfoP("ProgMode activated");
        wasActive = true;
    }
    else if (!isActive && wasActive)
    {
        if (_progModeWidget)
            _progModeWidget->removeAction(WidgetFlags::DisplayEnabled);

        leaveProgExclusive(); // restore rotation + screensaver
        logInfoP("ProgMode deactivated");
        wasActive = false;
    }
}

/**
 * @brief Enter prog-exclusive display - show ONLY the ProgMode widget. Pauses the rotation
 * and detaches the screensaver (remembered for leaveProgExclusive() to restore).
 */
void DeviceDisplay::enterProgExclusive()
{
    if (_progExclusiveActive || !_widgetManager) return;
    _progExclusiveActive = true;

    // Freeze the auto-rotation so no DefaultWidget switch happens while prog is shown.
    _widgetManager->pauseRotation();

    // Detach the screensaver (remember it) so power-save can never overlay the prog widget.
    _savedScreenSaverWidget = _screenSaverOwned;
    _widgetManager->setScreenSaverWidget(nullptr);
}

/**
 * @brief Leave prog-exclusive display - restore rotation + screensaver.
 */
void DeviceDisplay::leaveProgExclusive()
{
    if (!_progExclusiveActive || !_widgetManager) return;
    _progExclusiveActive = false;

    // Restore the screensaver we detached on enter (still owned by DeviceDisplay).
    if (_savedScreenSaverWidget)
        _widgetManager->setScreenSaverWidget(_savedScreenSaverWidget);
    _savedScreenSaverWidget = nullptr;

    // Resume normal auto-rotation.
    _widgetManager->resumeRotation();
}

/**
 * @brief Select the active screensaver widget from the full family, or nullptr for Off
 * (manager then drops straight to SLEEP). The previous instance is freed before installing the new
 * one (DeviceDisplay owns it; the WidgetsManager only borrows it).
 */
void DeviceDisplay::setScreenSaverType(ScreenSaverType type)
{
    if (!_widgetManager) return;

    // Every screensaver is AutoRemove so it is transient (shown while idle, torn down on wake).
    Widget* newWidget = nullptr;
    switch (type)
    {
        case ScreenSaverType::Clock:
            newWidget = new WidgetClock(5000, WidgetFlags::AutoRemove, false);
            break;
        case ScreenSaverType::Cube3D:
            newWidget = new WidgetCube3D(5000, WidgetFlags::AutoRemove);
            break;
        case ScreenSaverType::Doom:
            newWidget = new WidgetDoom(5000, WidgetFlags::AutoRemove);
            break;
        case ScreenSaverType::Fireworks:
            newWidget = new WidgetFireworks(10000, WidgetFlags::AutoRemove, 10);
            break;
        case ScreenSaverType::Life:
            newWidget = new WidgetLife(5000, WidgetFlags::AutoRemove);
            break;
        case ScreenSaverType::Matrix:
            newWidget = new WidgetMatrix(5000, WidgetFlags::AutoRemove, 7);
            break;
        case ScreenSaverType::MatrixClassic:
            newWidget = new WidgetMatrixClassic(5000, WidgetFlags::AutoRemove, 8);
            break;
        case ScreenSaverType::Pong:
            newWidget = new WidgetPong(5000, WidgetFlags::AutoRemove);
            break;
        case ScreenSaverType::Rain:
            newWidget = new WidgetRain(5000, WidgetFlags::AutoRemove, 6);
            break;
        case ScreenSaverType::Starfield:
            newWidget = new WidgetStarfield(5000, WidgetFlags::AutoRemove, 10);
            break;
        case ScreenSaverType::Off:
        default:
            newWidget = nullptr; // "Aus" -> no screensaver, manager goes straight to SLEEP
            break;
    }

    // Install the NEW widget BEFORE freeing the old one: setScreenSaverWidget() reads the previous
    // pointer, so deleting first would be a use-after-free. During prog-exclusive the live
    // screensaver is detached, so only update the remembered pointer.
    Widget* previous = _screenSaverOwned;
    _screenSaverOwned = newWidget;

    if (_progExclusiveActive)
    {
        _savedScreenSaverWidget = _screenSaverOwned; // detached for prog: remember for the restore
    }
    else
    {
        _widgetManager->setScreenSaverWidget(_screenSaverOwned);
    }

    // Manager no longer borrows `previous` -> safe to free.
    if (previous) delete previous;

    logInfoP("Screensaver type set: %d", static_cast<int>(type));
}

/**********************************************************************
 ****************************** show HELP******************************
 **********************************************************************/
void DeviceDisplay::showHelp()
{
    openknx.console.printHelpLine("ddc", "Device Display Control. Use 'ddc ?' for help.");
}
bool DeviceDisplay::processCommand(const std::string command, bool diagnose)
{
    if (diagnose) return false;
    return _ddcLoggerHelp->processCommand(command);
}

/**
 * @brief Fixed, constant flash reservation for the DeviceDisplay module (must stay in sync with
 * writeFlash()/readFlash()): 1 version byte + DisplaySettings + 1 count byte + fixed widget array.
 */
uint16_t DeviceDisplay::flashSize()
{
    return static_cast<uint16_t>(
        1u                                              // format version byte
        + sizeof(DisplaySettings)                       // display settings blob
        + 1u                                            // widget-settings count byte
        + WIDGET_SETTINGS_MAX * sizeof(WidgetSetting)); // fixed widget array
}

namespace
{
    // Bumped when the serialized layout changes so an older/newer image is rejected (readFlash
    // falls back to defaults). Stored as the first flash byte.
    constexpr uint8_t DD_SETTINGS_FORMAT_VERSION = 2; // v2: removed the bogus ssd1315 field

    // On-flash blob = 1 version byte + the store's serialized size; must not exceed flashSize().
    constexpr size_t DD_FLASH_BLOB_SIZE = 1u + DisplaySettingsStore::SERIALIZED_SIZE;
    static_assert(
        DD_FLASH_BLOB_SIZE <=
            1u + sizeof(DisplaySettings) + 1u + WIDGET_SETTINGS_MAX * sizeof(WidgetSetting),
        "DeviceDisplay flash blob exceeds flashSize() reservation");
} // namespace

/**
 * @brief Serialize the DisplaySettingsStore into the module-flash stream.
 *        Layout: [DD_SETTINGS_FORMAT_VERSION][serialize() blob]; the driver pads the rest.
 */
void DeviceDisplay::writeFlash()
{
    uint8_t buf[DisplaySettingsStore::SERIALIZED_SIZE];
    const size_t written = _settingsStore.serialize(buf);

    openknx.flash.writeByte(DD_SETTINGS_FORMAT_VERSION);
    for (size_t i = 0; i < written; ++i)
        openknx.flash.writeByte(buf[i]);

    _settingsStore.markSaved();
}

/**
 * @brief Restore the DisplaySettingsStore from the module-flash blob. Robust against
 * first boot (size==0) and format changes (unknown version byte) -> loadDefaults() in both cases.
 */
void DeviceDisplay::readFlash(const uint8_t* data, const uint16_t size)
{
    if (size == 0 || data == nullptr)
    {
        // First boot / never persisted: seed the mock defaults.
        _settingsStore.loadDefaults();
        return;
    }

    const uint8_t version = data[0];
    if (version != DD_SETTINGS_FORMAT_VERSION)
    {
        logInfoP("readFlash: unknown settings version %u (expected %u) - using defaults",
                 static_cast<unsigned>(version),
                 static_cast<unsigned>(DD_SETTINGS_FORMAT_VERSION));
        _settingsStore.loadDefaults();
        return;
    }

    // Hand the payload after the version byte to the store; deserialize() falls back to defaults on
    // a short buffer.
    if (!_settingsStore.deserialize(data + 1, static_cast<size_t>(size) - 1))
    {
        logInfoP("readFlash: settings payload rejected - using defaults");
        // deserialize() already applied defaults on the false path.
    }
}

/**
 * @brief Add a widget only if the WidgetsManager exists; otherwise no-op. Takes ownership of
 * @p widget (deleted on the no-op path so a caller-side `new` never leaks).
 * @return true if handed to the WidgetsManager, false on no-op.
 */
bool DeviceDisplay::tryAddWidget(Widget* widget)
{
    if (widget == nullptr) return false;

    if (_widgetManager == nullptr)
    {
        logDebugP("tryAddWidget: no WidgetsManager (yet) - widget discarded");
        delete widget; // avoid leaking the caller's `new`
        return false;
    }

    _widgetManager->addWidget(widget);
    return true;
}

/**
 * @brief Register a batch of root menu items only if the MenuRegistry exists.
 * @return true if forwarded to the registry, false on no-op.
 */
bool DeviceDisplay::tryRegisterRootItems(const std::vector<MenuConfig::MenuOption>& items)
{
    if (_menuRegistry == nullptr)
    {
        logDebugP("tryRegisterRootItems: no MenuRegistry (yet) - %u item(s) skipped",
                  static_cast<unsigned>(items.size()));
        return false;
    }

    _menuRegistry->registerRootItems(items);
    return true;
}

/**
 * @brief Register a single root menu item only if the MenuRegistry exists.
 * @return true if forwarded to the registry, false on no-op.
 */
bool DeviceDisplay::tryRegisterRootItem(const MenuConfig::MenuOption& item)
{
    if (_menuRegistry == nullptr)
    {
        logDebugP("tryRegisterRootItem: no MenuRegistry (yet) - item skipped");
        return false;
    }

    _menuRegistry->registerRootItem(item);
    return true;
}

/**
 * @brief Register an action callback by key only if the MenuRegistry exists.
 * @return true if forwarded to the registry, false on no-op.
 */
bool DeviceDisplay::tryRegisterAction(const std::string& key, std::function<void()> fn)
{
    if (_menuRegistry == nullptr)
    {
        logDebugP("tryRegisterAction: no MenuRegistry (yet) - action '%s' skipped", key.c_str());
        return false;
    }

    _menuRegistry->registerAction(key, std::move(fn));
    return true;
}

#endif // DEVICE_DISPLAY_MODULE