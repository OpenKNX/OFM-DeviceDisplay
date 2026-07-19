#ifdef DEVICE_DISPLAY_MODULE
    #define USE_GPIO_MODULE
    #include "Menu.h"
    #include "DefaultMenus.h"
    #include "MenuRegistry.h"
    #include "OpenKNX.h"

    // Network include is optional so a build without OFM-Network still compiles
    // (the Netzwerk root then falls back to plain defaults).
    #include "../DeviceDisplay.h"
    #include "../Widget.h"
    #include "../WidgetsManager.h"
    #if defined(__has_include)
        #if __has_include("NetworkModule.h")
            #include "NetworkModule.h"
            #define MENU_HAS_NETWORK_MODULE 1
        #endif
    #endif

MenuWidget::MenuWidget(uint32_t displayTime, WidgetFlags action)
    : _displayTime(displayTime),
      _action(static_cast<WidgetFlags>(action | WidgetFlags::WantsButtonInput | WidgetFlags::ManagedExternally | WidgetFlags::Background)),
      _selectedIndex(0),
      _lastButtonPressTime(0),
      _lastRedrawTime(0),
      _FrontPlateEnabled(false)
{
}

uint32_t MenuWidget::getDisplayTime() const
{
    return _displayTime;
}

WidgetFlags MenuWidget::getAction() const
{
    return _action;
}

i2cDisplay* MenuWidget::getDisplayModule() const
{
    return _display;
}

void MenuWidget::setDisplayModule(i2cDisplay* displayModule)
{
    _display = displayModule;
    if (!_display)
    {
        logErrorP("Display is NULL");
    }
}

void MenuWidget::setup()
{
    logInfoP("Setup...");
    if (!_display)
    {
        logErrorP("Display is NULL");
        return;
    }

    _screenHeight = _display->GetDisplayHeight();
    _screenWidth = _display->GetDisplayWidth();

    // Menu is built LAZILY on first open, NOT here: building the full tree at setup() would hold
    // ~19 KB from boot and overflow the ESP32 loopTask stack (many MenuOption locals at once).

    _state = WidgetState::BACKGROUND; // Start Menu in background! Will be started by button press.
}

void MenuWidget::loop()
{
    uint32_t currentTime = millis();
    if (_state != WidgetState::RUNNING && _state != WidgetState::BACKGROUND)
    {
        return;
    }

    // If the menu is in the background but has set DisplayEnabled, it waits to be activated by the widget manager.
    if (_state == WidgetState::BACKGROUND && (getAction() & WidgetFlags::DisplayEnabled))
    {
        logDebugP("MenuWidget requested activation, waiting for manager...");
        // The button checks continue, but the menu will only become active when the manager activates it.
    }

    // Are we the active button widget (front), or parked behind another ManagedExternally widget
    // (e.g. the SD file browser)? Only meaningful while we want to be shown (DisplayEnabled). While
    // parked we must NOT auto-close — that frees the tree and drops to the home screen when the
    // other widget hands back. On regaining focus, repaint + reset the idle timer so we neither
    // stay blank nor instantly auto-close on the stale timestamp.
    bool isFront = false;
    if (getAction() & WidgetFlags::DisplayEnabled)
    {
        isFront = true; // shown -> front unless the manager reports another widget owns button input
        if (WidgetsManager* wm = openknxDisplayModule.getWidgetManager())
            isFront = (wm->getActiveButtonWidget() == static_cast<Widget*>(this));
        if (isFront && !_wasFront)
        {
            _needsRedraw = true;
            _activationRedrawUntil = currentTime + ACTIVATION_REDRAW_MS;
            _lastButtonPressTime = currentTime;
        }
        _wasFront = isFront;
    }
    else
        _wasFront = false;

    // Close overlay automatically after max timeout
    if (_infoOverlayActive &&
        (currentTime - _lastButtonPressTime) > _infoOverlayMaxTimeout)
    {
        logDebugP("Overlay closed automatically after max timeout of %d ms", _infoOverlayMaxTimeout);
        _infoOverlayActive = false;
        _needsRedraw = true;
        return;
    }

    // Automatic activation on button interaction
    if (_lastButtonPressTime > 0 && (currentTime - _lastButtonPressTime) < _displayTime)
    {
        if (_state != WidgetState::RUNNING)
        {
            logDebugP("Menu is running due to button press.");
            resume(); // Will set _needsRedraw = true;
        }
    }
    // Automatic background setting on inactivity — only when we actually own the screen. Parked
    // behind another widget (browser) we hold position instead of freeing the tree.
    else if ((currentTime - _lastButtonPressTime) >= _displayTime && !_infoOverlayActive)
    {
        if (_state == WidgetState::RUNNING && isFront)
        {
            closeMenu(); // 10 s inactivity closes AND frees the built tree
            return;
        }
    }

    // While an editor is active, drive the ~500ms blink toggle for the active octet / grabbed row
    // and redraw on each toggle so it blinks without any button press.
    if (_mode != MenuMode::Normal && _state == WidgetState::RUNNING)
    {
        if (currentTime - _editBlinkLast >= EDIT_BLINK_INTERVAL)
        {
            _editBlinkOn = !_editBlinkOn;
            _editBlinkLast = currentTime;
            _needsRedraw = true;
        }
    }

    // Keep redrawing for the short activation window even once _needsRedraw was consumed, so the
    // menu re-asserts itself over the outgoing widget's stop()/clearDisplay() (see resume()).
    const bool activationRedraw = (_activationRedrawUntil != 0 && currentTime < _activationRedrawUntil);
    if (_state == WidgetState::RUNNING && (_needsRedraw || activationRedraw))
    {
        // Fold in late module registrations before drawing, but ONLY at the root level
        // (empty stack) so an in-progress submenu navigation is never disrupted.
        if (_menuStack.empty() && _menuRegistry &&
            (!_registryMenuBuilt || _menuRegistry->isDirty()))
        {
            buildMenuFromRegistry();
        }

        drawMenu();
        _needsRedraw = false;
        _lastRedrawTime = currentTime;
    }

    // Close the activation window once it elapses.
    if (_activationRedrawUntil != 0 && currentTime >= _activationRedrawUntil)
        _activationRedrawUntil = 0;
}

void MenuWidget::start()
{
    logDebugP("Start...");
    _stateLast = _state;
    _state = WidgetState::RUNNING;
    _needsRedraw = true;

    // addAction(WidgetFlags::DisplayEnabled);
    // removeAction(WidgetFlags::Background);
}

void MenuWidget::stop()
{
    _stateLast = _state;
    _state = WidgetState::STOPPED;
    _infoOverlayActive = false; // Ensure overlay is reseted
    _needsRedraw = false;
    // Never resume into an editor after a stop/start; drop any live grab first.
    if (_mode == MenuMode::Reorder)
    {
        WidgetsManager* wm = openknxDisplayModule.getWidgetManager();
        if (wm && wm->isGrabbing()) wm->dropWidget();
    }
    _mode = MenuMode::Normal;
    // Clean up the memory
    _menuStack.clear();
    _selectedIndexStack.clear(); // keep parent-index stack in sync
    _currentMenu.clear();
    _selectedIndex = 0;
    // Force a fresh registry build on the next interaction (menu was just cleared).
    _registryMenuBuilt = false;
    clearDisplay();
    logDebugP("Stop...");
}

void MenuWidget::pause()
{
    _stateLast = _state;
    _state = WidgetState::PAUSED; // No pausing for menu, just go to background
    _needsRedraw = false;

    removeAction(WidgetFlags::DisplayEnabled);
    _infoOverlayActive = false; // Ensure overlay is reseted

    logDebugP("Pause requested, going to background...");
    // background();
}

void MenuWidget::resume()
{
    _state = _stateLast;
    _state = WidgetState::RUNNING;
    _needsRedraw = true;
    // Redraw for a short window so we survive the outgoing widget's stop()/clearDisplay().
    _activationRedrawUntil = millis() + ACTIVATION_REDRAW_MS;
    addAction(WidgetFlags::DisplayEnabled);

    logDebugP("Resume...");
}

void MenuWidget::background()
{
    _stateLast = _state;
    _state = WidgetState::BACKGROUND;
    _infoOverlayActive = false; // Ensure overlay is reseted
    _needsRedraw = false;

    removeAction(WidgetFlags::DisplayEnabled);
    clearDisplay();
    logDebugP("Menu is running in background due to inactivity.");
}

void MenuWidget::addDefaultMenus()
{
    _menuConfig.setMenu(DefaultMenus::buildMenu().submenu);
    _currentMenu = _menuConfig.getMenu();
    if (!_currentMenu.empty())
    {
        _selectedIndex = 0;
        _needsRedraw = true;
        logDebugP("Default menus loaded successfully");

        addDefaultActions();
        addDefaultOnValueChanged();

        // Seed keyed option defaults for the static default tree too, so
        // visibleIf rows are resolved against real state on first entry.
        seedDefaultValues(_currentMenu);
    }
    else
    {
        logErrorP("Failed to load default menus");
    }
}

void MenuWidget::addDefaultActions()
{
    // Register default actions here if needed
    registerAction("reboot_device", []() {
        openknx.common.restart();
    });

    registerAction("prog_mode", []() {
        knx.toggleProgMode();
    });

    registerAction("show_device_info_overlay", [this]() {
        showOverlay([this]() {
            _display->display->clearDisplay();
            _display->display->setTextSize(1);
            _display->display->setTextColor(WHITE);
            _display->display->setCursor(0, 0);
            _display->display->print("Device Info:");
            _display->display->setCursor(0, 10);
            _display->display->print("Name: MyDevice123");
            _display->display->setCursor(0, 20);
            _display->display->print("FW: 1.2.3");
            _display->display->setCursor(0, 30);
            _display->display->print("SN: 456789");
            _display->displayBuff();
            logDebugP("Device Info overlay displayed");
        });
    });

#ifdef MENU_HAS_NETWORK_MODULE
    // Network "Uebernehmen": commit the staged DHCP/IP override live (no reboot). The net_* onValueChanged
    // handlers stage edits into the module's in-RAM override; this applies + persists them.
    registerAction("net_apply", [this]() {
        openknxNetwork.applyLocalNetworkConfig(false); // false = live re-init (resetNetwork), no reboot
        logInfoP("Network override applied (live)");
    });
#endif

    assignRegisteredActions(_currentMenu);
}

void MenuWidget::addDefaultOnValueChanged()
{
#ifdef MENU_HAS_NETWORK_MODULE
    // Network settings: stage edits into the module's in-RAM override (setters do NOT apply yet; the
    // "Uebernehmen"/net_apply action commits + re-inits live). DHCP checkbox: checked == DHCP.
    registerOnValueChanged("net_dhcp", [this](const MenuConfig::MenuOption&, const MenuValue& val) {
        if (val.isBool()) openknxNetwork.setLocalStaticIpEnabled(!val.getBool());
    });
    registerOnValueChanged("net_ip", [this](const MenuConfig::MenuOption&, const MenuValue& val) {
        const size_t p = val.getSizeT();
        openknxNetwork.setLocalIp(IPAddress((uint8_t)(p >> 24), (uint8_t)(p >> 16), (uint8_t)(p >> 8), (uint8_t)p));
    });
    registerOnValueChanged("net_subnet", [this](const MenuConfig::MenuOption&, const MenuValue& val) {
        const size_t p = val.getSizeT();
        openknxNetwork.setLocalSubnet(IPAddress((uint8_t)(p >> 24), (uint8_t)(p >> 16), (uint8_t)(p >> 8), (uint8_t)p));
    });
    registerOnValueChanged("net_gw", [this](const MenuConfig::MenuOption&, const MenuValue& val) {
        const size_t p = val.getSizeT();
        openknxNetwork.setLocalGateway(IPAddress((uint8_t)(p >> 24), (uint8_t)(p >> 16), (uint8_t)(p >> 8), (uint8_t)p));
    });
    registerOnValueChanged("net_dns", [this](const MenuConfig::MenuOption&, const MenuValue& val) {
        const size_t p = val.getSizeT();
        openknxNetwork.setLocalDns(IPAddress((uint8_t)(p >> 24), (uint8_t)(p >> 16), (uint8_t)(p >> 8), (uint8_t)p));
    });
#endif
    registerOnValueChanged("dhcp_enabled", [this](const MenuConfig::MenuOption& opt, const MenuValue& val) {
        if (val.isBool())
        {
            // std::cout << "DHCP geändert auf: " << (val.getBool() ? "Enabled" : "Disabled") << std::endl;
            if (val.getBool())
            {
                // openknx.network.enableDhcp();
                logInfoP("DHCP enabled");
            }
            else
            {
                // openknx.network.disableDhcp();
                logInfoP("DHCP disabled");
            }
        }
    });
    registerOnValueChanged("timezone", [this](const MenuConfig::MenuOption& opt, const MenuValue& val) {
        if (val.isSizeT())
        {
            // openknx.common.setTimeZone(opt.dropdownOptions[val.getSizeT()]);
            logDebugP("Zeitzone geändert auf: %s", opt.dropdownOptions[val.getSizeT()].c_str());
        }
    });
    registerOnValueChanged("brightness_level", [this](const MenuConfig::MenuOption& opt, const MenuValue& val) {
        if (val.isSizeT())
        {
            // Auswahl ist 0-4, also 5%, 25%, 50%, 75%, 100%
            // const int selectedOption = static_cast<int>(std::clamp(val.getSizeT(), static_cast<size_t>(0), static_cast<size_t>(4)));
            const int selectedOption = static_cast<int>(std::min(static_cast<size_t>(4), std::max(static_cast<size_t>(0), val.getSizeT())));
            const int percentages[] = {5, 25, 50, 75, 100};
            const int percentage = percentages[selectedOption];
            int contrast = static_cast<int>(std::round(percentage * 255.0 / 100.0));
            _display->SetDisplayContrast(contrast);
            logDebugP("Helligkeit geändert auf: %s%% (%d)", opt.dropdownOptions[val.getSizeT()].c_str(), contrast);
        }
    });
    // Dimming is driven by the PowerSave state machine (dimMin -> dimTimeout); no manual toggle.

    assignOnValueChangedHandlers(_currentMenu);
}

// Assemble the Netzwerk submenu from the live NetworkModule. When OFM-Network is absent the
// snapshot stays at defaults and a plain tree is rendered. valueProvider re-pulls live dotted values.
MenuConfig::MenuOption MenuWidget::buildNetworkRootItem()
{
    DefaultMenus::NetSnapshot snap;

    #ifdef MENU_HAS_NETWORK_MODULE
    auto fill = [](uint8_t out[4], IPAddress a) {
        out[0] = a[0];
        out[1] = a[1];
        out[2] = a[2];
        out[3] = a[3];
    };

    // localUsesStaticIp() == false -> DHCP mode (checkbox checked).
    snap.dhcp = !openknxNetwork.localUsesStaticIp();
    fill(snap.ip, openknxNetwork.localIP());
    fill(snap.subnet, openknxNetwork.subnetMask());
    fill(snap.gateway, openknxNetwork.gatewayIP());
    fill(snap.dns, openknxNetwork.nameServerIP());

    // Live read-only value provider for the four IP rows (0=ip 1=subnet 2=gw 3=dns).
    snap.liveValue = [](uint8_t idx) -> std::string {
        IPAddress a;
        switch (idx)
        {
            case 0: a = openknxNetwork.localIP(); break;
            case 1: a = openknxNetwork.subnetMask(); break;
            case 2: a = openknxNetwork.gatewayIP(); break;
            default: a = openknxNetwork.nameServerIP(); break;
        }
        char buf[16];
        snprintf(buf, sizeof(buf), "%u.%u.%u.%u",
                 (unsigned)a[0], (unsigned)a[1], (unsigned)a[2], (unsigned)a[3]);
        return std::string(buf);
    };
    #endif

    return DefaultMenus::buildNetworkRootItem(snap);
}

// Assemble the Widgets submenu from the live WidgetsManager rotation set.
// When no WidgetsManager is available yet the widget list is empty and only
// the Reorder + Back entries remain.
MenuConfig::MenuOption MenuWidget::buildWidgetsRootItem()
{
    std::vector<DefaultMenus::WidgetDesc> descs;

    WidgetsManager* wm = openknxDisplayModule.getWidgetManager();
    if (wm)
    {
        // getReorderableWidgets() is the canonical rotation set in user order;
        // fall back to getDefaultWidgetsInOrder() if the former is empty.
        std::vector<Widget*> widgets = wm->getReorderableWidgets();
        if (widgets.empty()) widgets = wm->getDefaultWidgetsInOrder();

        static const uint32_t durMs[6] = {5000, 8000, 10000, 15000, 20000, 30000};
        for (Widget* w : widgets)
        {
            if (!w) continue;
            DefaultMenus::WidgetDesc d;
            d.name = w->getName();
            d.enabled = w->isEnabled();

            // Map the widget's current display time to the nearest option index.
            uint32_t cur = wm->getWidgetDisplayTime(d.name);
            size_t idx = 2; // default 10 s
            for (size_t i = 0; i < 6; ++i)
            {
                if (cur == durMs[i])
                {
                    idx = i;
                    break;
                }
            }
            d.durationIdx = idx;
            descs.push_back(std::move(d));
        }
    }

    return DefaultMenus::buildWidgetsRootItem(descs);
}

void MenuWidget::buildMenuFromRegistry()
{
    if (!_menuRegistry)
    {
        // No registry handed in: keep/restore the static DefaultMenus tree so the
        // widget still works standalone (e.g. after a stop() cleared _currentMenu).
        if (_currentMenu.empty())
        {
            logDebugP("buildMenuFromRegistry: no registry, reloading default menu");
            addDefaultMenus();
        }
        return;
    }

    // Build the display-owned roots FRESH on every open and combine them with the registry's
    // persistent module roots via buildWith(). The heavy display tree (~17 KiB) is never stored in
    // the registry — it lives only in _currentMenu and is released again by closeMenu().
    std::vector<MenuConfig::MenuOption> displayRoots = DefaultMenus::buildDisplayRootItems();

    // LAZY roots: Netzwerk and Widgets are the heaviest branches, so add only lightweight root
    // placeholders here and build their children on ENTRY (Submenu case), not on every menu open.
    {
        MenuConfig::MenuOption netRoot;
        netRoot.label = "Netzwerk";
        netRoot.type = MenuConfig::MenuElementType::Submenu;
        netRoot.devOnly = true;
        netRoot.sortOrder = 20;
        netRoot.submenuBuilder = [this]() {
            MenuConfig::MenuOption full = buildNetworkRootItem(); // live values, built on entry
            return std::move(full.submenu);
        };
        displayRoots.push_back(std::move(netRoot));

        MenuConfig::MenuOption widgetsRoot;
        widgetsRoot.label = "Widgets";
        widgetsRoot.type = MenuConfig::MenuElementType::Submenu;
        widgetsRoot.devOnly = true;
        widgetsRoot.sortOrder = 40;
        widgetsRoot.submenuBuilder = [this]() {
            MenuConfig::MenuOption full = buildWidgetsRootItem(); // per-widget subs, built on entry
            return std::move(full.submenu);
        };
        displayRoots.push_back(std::move(widgetsRoot));
    }

    _currentMenu = _menuRegistry->buildWith(displayRoots);

    // Merge the registry's callbacks into the local registries; a registry entry with the same key
    // overrides the local default (last wins).
    for (const auto& kv : _menuRegistry->getActions())
    {
        actionRegistry[kv.first] = kv.second;
    }
    for (const auto& kv : _menuRegistry->getOnValueChanged())
    {
        onValueChangedRegistry[kv.first] = kv.second;
    }

    // Bind the merged handlers onto the freshly built tree (recurses submenus).
    assignRegisteredActions(_currentMenu);
    assignOnValueChangedHandlers(_currentMenu);
    assignRadioIndexProviders(_currentMenu); // live current-index for radio-list dropdowns
    seedOptionValuesFromStore(_currentMenu); // set each option's defaultValue from the persisted store

    // Seed keyed option defaults (incl. net_dhcp) so visibleIf resolves against real state
    // on first entry. Only seeds keys still absent, so a user's prior change is preserved.
    seedDefaultValues(_currentMenu);

    // Reset navigation to a clean root view.
    _menuStack.clear();
    _selectedIndexStack.clear();
    _selectedIndex = 0;
    _registryMenuBuilt = true;
    _needsRedraw = true;

    // Land the cursor on the first visible root (devOnly/visibleIf may
    // hide index 0). No-op when everything is visible.
    _selectedIndex = firstVisibleFrom(0);

    _menuRegistry->markClean();

    logDebugP("buildMenuFromRegistry: built %u root item(s)", static_cast<unsigned>(_currentMenu.size()));
}

void MenuWidget::registerAction(const std::string& key, std::function<void()> action)
{
    actionRegistry[key] = std::move(action);
}

void MenuWidget::assignRegisteredActions(std::vector<MenuConfig::MenuOption>& menuOptions)
{
    for (auto& option : menuOptions)
    {
        if (!option.key.empty())
        {
            const auto it = actionRegistry.find(option.key);
            if (it != actionRegistry.end())
            {
                option.action = it->second;
            }
        }

        if (!option.submenu.empty())
        {
            assignRegisteredActions(option.submenu);
        }
    }
}

void MenuWidget::registerOnValueChanged(const std::string& key, std::function<void(const MenuConfig::MenuOption&, const MenuValue&)> handler)
{
    onValueChangedRegistry[key] = std::move(handler);
}

void MenuWidget::registerRadioIndexProvider(const std::string& key, std::function<size_t()> provider)
{
    radioIndexProviderRegistry[key] = std::move(provider);
}

void MenuWidget::assignOnValueChangedHandlers(std::vector<MenuConfig::MenuOption>& menuOptions)
{
    for (auto& option : menuOptions)
    {
        if (!option.key.empty())
        {
            logDebugP("assign OnValueChanged for key: %s", option.key.c_str());
            const auto it = onValueChangedRegistry.find(option.key);
            if (it != onValueChangedRegistry.end())
            {
                option.onValueChanged = it->second;
                logDebugP("OnValueChanged handler assigned for key: %s", option.key.c_str());
            }
            else
            {
                logDebugP("No OnValueChanged handler found for key: %s", option.key.c_str());
            }
        }

        if (!option.submenu.empty())
        {
            assignOnValueChangedHandlers(option.submenu);
        }
    }
}

void MenuWidget::seedOptionValuesFromStore(std::vector<MenuConfig::MenuOption>& menuOptions)
{
    if (!_valueSeeder) return;
    for (auto& option : menuOptions)
    {
        if (!option.key.empty()) _valueSeeder(option);
        if (!option.submenu.empty()) seedOptionValuesFromStore(option.submenu);
    }
}

void MenuWidget::assignRadioIndexProviders(std::vector<MenuConfig::MenuOption>& menuOptions)
{
    for (auto& option : menuOptions)
    {
        if (!option.key.empty())
        {
            const auto it = radioIndexProviderRegistry.find(option.key);
            if (it != radioIndexProviderRegistry.end()) option.radioIndexProvider = it->second;
        }

        if (!option.submenu.empty())
        {
            assignRadioIndexProviders(option.submenu);
        }
    }
}

// Recursively seed keyed option defaults into _menuConfig so visibleIf (which treats an
// absent key as "hidden") resolves against real state on first entry. Seeds only absent keys with a
// non-None defaultValue. Crucial for net_dhcp, else a static-IP user's net_dhcp=false is never set.
void MenuWidget::seedDefaultValues(const std::vector<MenuConfig::MenuOption>& menuOptions)
{
    for (const auto& option : menuOptions)
    {
        if (!option.key.empty() &&
            option.defaultValue.getType() != MenuValue::None &&
            !_menuConfig.hasValue(option.key))
        {
            // seedValue (not setValue): prime visibleIf state only, never fire onValueChanged.
            _menuConfig.seedValue(option.key, option.defaultValue);
            logDebugP("Seeded default value for key: %s", option.key.c_str());
        }

        if (!option.submenu.empty())
        {
            seedDefaultValues(option.submenu);
        }
    }
}

void MenuWidget::showOverlay(std::function<void()> drawFn)
{
    _overlayDrawFunction = std::move(drawFn);
    _infoOverlayActive = true;
    _needsRedraw = true;
    logDebugP("Showing overlay");
}

/************************************************************
 ********************** MENU NAVIGATION *********************
 ************************************************************/

bool MenuWidget::handleButtonEvent(const ButtonEvent& event)
{
    if (event.action != ButtonAction::PRESS) // only handle short press for now
    {
        return false;
    }

    // While an editor is active, dispatch to its handler first and consume the
    // event, bypassing normal navigation.
    if (_mode == MenuMode::IpEdit)
    {
        _lastButtonPressTime = event.timestamp;
        if (_state == WidgetState::BACKGROUND) resume();
        return handleIpEditButton(event);
    }
    if (_mode == MenuMode::Reorder)
    {
        _lastButtonPressTime = event.timestamp;
        if (_state == WidgetState::BACKGROUND) resume();
        return handleReorderButton(event);
    }
    if (_mode == MenuMode::RadioSelect)
    {
        _lastButtonPressTime = event.timestamp;
        if (_state == WidgetState::BACKGROUND) resume();
        return handleRadioSelectButton(event);
    }
    if (_mode == MenuMode::Slider)
    {
        _lastButtonPressTime = event.timestamp;
        if (_state == WidgetState::BACKGROUND) resume();
        return handleSliderButton(event);
    }
    if (_mode == MenuMode::TextEdit)
    {
        _lastButtonPressTime = event.timestamp;
        if (_state == WidgetState::BACKGROUND) resume();
        return handleTextEditButton(event);
    }
    if (_mode == MenuMode::ConfirmDialog)
    {
        _lastButtonPressTime = event.timestamp;
        if (_state == WidgetState::BACKGROUND) resume();
        return handleConfirmButton(event);
    }
    if (_mode == MenuMode::NumberEdit)
    {
        _lastButtonPressTime = event.timestamp;
        if (_state == WidgetState::BACKGROUND) resume();
        return handleNumberEditButton(event);
    }

    // The menu is "open" only while displayed (DisplayEnabled). While CLOSED only a deliberate
    // OK opens it; every other button is left to the home rotation / screensaver wake.
    const bool isOpen = (getAction() & WidgetFlags::DisplayEnabled) != 0;
    if (!isOpen)
    {
        if (event.type != ButtonType::SELECT)
            return false; // not OK -> ignore; home rotation / screensaver keeps control
        openMenu();       // OK on the home screen -> build fresh + show
        return true;
    }

    // Keep the tree fresh at the root for late module registrations / dirty rebuilds.
    if (_menuStack.empty() &&
        ((_menuRegistry && (!_registryMenuBuilt || _menuRegistry->isDirty())) || _currentMenu.empty()))
    {
        buildMenuFromRegistry();
    }

    if (_infoOverlayActive) // an open info overlay is dismissed by the next press
    {
        logDebugP("Overlay closed by button press");
        _infoOverlayActive = false;
        _needsRedraw = true;
        _lastButtonPressTime = event.timestamp;
        return true;
    }

    bool handled = false;
    bool closed = false;

    switch (event.type)
    {
        case ButtonType::UP:
            navigateUp();
            handled = true;
            break;

        case ButtonType::DOWN:
            navigateDown();
            handled = true;
            break;

        case ButtonType::SELECT:
            selectItem();
            handled = true;
            break;

        case ButtonType::LEFT:
            if (iconGridActive())
            {
                gridMove(-1, 0); // icon grid: LEFT moves the cursor; exit via the "Beenden" tile
            }
            else if (_menuStack.empty())
            {
                closeMenu(); // text-list root: LEFT closes + frees the built tree
                closed = true;
            }
            else
            {
                navigateLeft(); // submenu: one level up (Back)
            }
            handled = true;
            break;

        case ButtonType::RIGHT:
            navigateRight();
            handled = true;
            break;
    }

    if (handled && !closed)
    {
        _lastButtonPressTime = event.timestamp;
        _needsRedraw = true;

        // Keep the menu shown while navigating (never after a close).
        if (_state == WidgetState::BACKGROUND)
        {
            resume();
        }
    }

    return handled;
}

// Open the menu from the home screen. Build a fresh tree from the registry and show it.
void MenuWidget::openMenu()
{
    _mode = MenuMode::Normal;
    _menuStack.clear();
    _selectedIndexStack.clear();
    _selectedIndex = 0;
    _registryMenuBuilt = false; // force a fresh build (also picks up late registrations)
    buildMenuFromRegistry();
    _needsRedraw = true;
    // Stamp the interaction time so loop()'s auto-background does not immediately hide the menu.
    _lastButtonPressTime = millis();
    resume(); // RUNNING + DisplayEnabled -> the WidgetsManager shows the menu
    logDebugP("Menu opened");
}

// Close the menu, RELEASE the built tree (tens of KiB) and hand the display back to the
// widget rotation. The MenuRegistry keeps the authoritative copy; the next open rebuilds from it.
void MenuWidget::closeMenu()
{
    if (_mode == MenuMode::Reorder)
    {
        WidgetsManager* wm = openknxDisplayModule.getWidgetManager();
        if (wm && wm->isGrabbing()) wm->dropWidget();
    }
    _mode = MenuMode::Normal;
    _confirmOnYes = nullptr; // drop any ConfirmDialog callback if the menu closes while it is open
    // swap-with-empty actually returns the capacity to the heap (clear() alone keeps it).
    std::vector<MenuConfig::MenuOption>().swap(_currentMenu);
    std::vector<std::vector<MenuConfig::MenuOption>>().swap(_menuStack);
    std::vector<size_t>().swap(_selectedIndexStack);
    _selectedIndex = 0;
    _registryMenuBuilt = false; // next open rebuilds fresh
    // Clear the interaction time so loop()'s auto-activation does not immediately re-open the menu.
    _lastButtonPressTime = 0;
    background(); // removeAction(DisplayEnabled) + BACKGROUND -> rotation resumes
    logDebugP("Menu closed, tree freed");
}

// An item is visible when its visibleIf holds AND it is not devOnly while dev mode is off.
bool MenuWidget::isItemVisible(const MenuConfig::MenuOption& item) const
{
    if (item.devOnly && !_devMode) return false;
    return _menuConfig.isMenuOptionVisible(item);
}

// First visible index at/after `from` (forward, then backward); `from` if all hidden.
size_t MenuWidget::firstVisibleFrom(size_t from) const
{
    if (_currentMenu.empty()) return 0;
    if (from >= _currentMenu.size()) from = _currentMenu.size() - 1;

    for (size_t i = from; i < _currentMenu.size(); ++i)
    {
        if (isItemVisible(_currentMenu[i])) return i;
    }
    // Nothing visible forward — search backward from `from`.
    for (size_t i = from; i-- > 0;)
    {
        if (isItemVisible(_currentMenu[i])) return i;
    }
    return from; // everything hidden: keep caller in-bounds
}

// An IP row is DHCP-locked when it is IpAddress/IpEdit and net_dhcp is present and true.
// Locked rows stay visible but render "auto" and are not editable.
bool MenuWidget::isDhcpLocked(const MenuConfig::MenuOption& item) const
{
    if (item.type != MenuConfig::MenuElementType::IpAddress &&
        item.type != MenuConfig::MenuElementType::IpEdit)
        return false;
    if (!_menuConfig.hasValue("net_dhcp")) return false;
    return _menuConfig.getValue("net_dhcp").getBool();
}

// Right-aligned text/indicator for an item; empty when it has no trailing value.
//   Readonly -> valueProvider   ProgToggle/Checkbox -> "[x]"/"[ ]"   Dropdown -> selected option
//   IpEdit/IpAddress -> "a.b.c.d"   Submenu/About/Files -> ">"   Back -> "<"
std::string MenuWidget::itemValueText(const MenuConfig::MenuOption& item) const
{
    switch (item.type)
    {
        case MenuConfig::MenuElementType::Readonly:
            if (item.valueProvider) return item.valueProvider();
            return std::string();

        case MenuConfig::MenuElementType::ProgToggle:
            return knx.progMode() ? "[x]" : "[ ]";

        case MenuConfig::MenuElementType::Checkbox:
            return item.defaultValue.getBool() ? "[x]" : "[ ]";

        case MenuConfig::MenuElementType::Dropdown:
        {
            // A radio-list dropdown reflects its live value (provider); plain ones the default.
            const size_t idx = item.radioIndexProvider ? item.radioIndexProvider() : item.defaultValue.getSizeT();
            if (idx < item.dropdownOptions.size()) return item.dropdownOptions[idx];
            return std::string();
        }

        case MenuConfig::MenuElementType::TextInput:
            if (item.valueProvider) return item.valueProvider();
            return item.defaultValue.isString() ? item.defaultValue.getString() : std::string();

        case MenuConfig::MenuElementType::NumberEdit:
        {
            // Minutes value; 0 shows the item's zero-label (dropdownOptions[0], e.g. "nie"/"aus").
            const size_t m = item.defaultValue.isSizeT() ? item.defaultValue.getSizeT() : 0;
            if (m == 0)
                return item.dropdownOptions.empty() ? std::string("aus") : item.dropdownOptions[0];
            char buf[12];
            snprintf(buf, sizeof(buf), "%u min", static_cast<unsigned>(m));
            return std::string(buf);
        }

        case MenuConfig::MenuElementType::IpEdit:
        case MenuConfig::MenuElementType::IpAddress:
        {
            // DHCP-locked rows show "auto" instead of dotted octets; the selected-row form
            // "auto (DHCP)" is produced in drawMenu(), not here.
            if (isDhcpLocked(item)) return "auto";

            char buf[16];
            snprintf(buf, sizeof(buf), "%u.%u.%u.%u",
                     static_cast<unsigned>(item.ip[0]), static_cast<unsigned>(item.ip[1]),
                     static_cast<unsigned>(item.ip[2]), static_cast<unsigned>(item.ip[3]));
            return std::string(buf);
        }

        case MenuConfig::MenuElementType::Submenu:
        case MenuConfig::MenuElementType::About:
        case MenuConfig::MenuElementType::Files:
            return ">";

        case MenuConfig::MenuElementType::Back:
            return "<";

        default:
            return std::string();
    }
}

void MenuWidget::navigateUp()
{
    logDebugP("Navigate up");
    if (iconGridActive())
    {
        gridMove(0, -1);
        return;
    }
    // Step upward to the nearest visible item, skipping hidden ones.
    for (size_t i = _selectedIndex; i-- > 0;)
    {
        if (isItemVisible(_currentMenu[i]))
        {
            _selectedIndex = i;
            logDebugP("Navigated up to index %d", _selectedIndex);
            return;
        }
    }
    // No visible item above: stay put (already at the first visible row).
}

void MenuWidget::navigateDown()
{
    logDebugP("Navigate down");
    if (iconGridActive())
    {
        gridMove(0, 1);
        return;
    }
    // Step downward to the nearest visible item, skipping hidden ones.
    for (size_t i = _selectedIndex + 1; i < _currentMenu.size(); ++i)
    {
        if (isItemVisible(_currentMenu[i]))
        {
            _selectedIndex = i;
            logDebugP("Navigated down to index %d", _selectedIndex);
            return;
        }
    }
    // No visible item below: stay put (already at the last visible row).
}

// LEFT = go up one level (Back). Pops the stack and restores the parent's _selectedIndex.
// At the root the stack is empty and LEFT is a no-op.
void MenuWidget::navigateLeft()
{
    logDebugP("Navigate left (back)");
    if (iconGridActive())
    {
        gridMove(-1, 0);
        return;
    }
    if (_menuStack.empty())
    {
        logDebugP("Already at root, LEFT is a no-op");
        return;
    }

    _currentMenu = std::move(_menuStack.back());
    _menuStack.pop_back();

    // Restore the parent selection captured on descent.
    size_t restored = 0;
    if (!_selectedIndexStack.empty())
    {
        restored = _selectedIndexStack.back();
        _selectedIndexStack.pop_back();
    }
    if (restored >= _currentMenu.size())
    {
        restored = _currentMenu.empty() ? 0 : _currentMenu.size() - 1;
    }
    _selectedIndex = restored;
    logDebugP("Navigated back to parent, restored index %d", _selectedIndex);
}

// RIGHT = enter the selected item. RIGHT and OK both activate, so this delegates to
// selectItem(). Kept distinct so the button mapping stays explicit.
void MenuWidget::navigateRight()
{
    logDebugP("Navigate right (enter)");
    if (iconGridActive())
    {
        gridMove(1, 0); // in the icon grid RIGHT moves the cursor; OK (selectItem) enters
        return;
    }
    if (_currentMenu.empty()) return;
    selectItem();
}

// Apply a dynamic per-widget setting (widget_show_<name> / widget_dur_<name>) via the
// WidgetsManager; the widget name is stashed in MenuOption::toast. Returns true if handled.
bool MenuWidget::applyWidgetSetting(const MenuConfig::MenuOption& item)
{
    WidgetsManager* wm = openknxDisplayModule.getWidgetManager();
    if (!wm || item.toast.empty()) return false;

    if (item.key.rfind("widget_show_", 0) == 0)
    {
        Widget* w = wm->getWidgetFromQueue(item.toast);
        if (w) w->setEnabled(item.defaultValue.getBool());
        openknxDisplayModule.getSettingsStore().requestSave();
        logDebugP("Widget '%s' enabled=%d", item.toast.c_str(), item.defaultValue.getBool() ? 1 : 0);
        return true;
    }
    if (item.key.rfind("widget_dur_", 0) == 0)
    {
        static const uint32_t DUR_S[] = {5, 8, 10, 15, 20, 30}; // matches the mock duration options
        size_t idx = item.defaultValue.getSizeT();
        if (idx >= (sizeof(DUR_S) / sizeof(DUR_S[0]))) idx = 2; // default 10 s
        wm->setWidgetDisplayTime(item.toast, DUR_S[idx] * 1000);
        openknxDisplayModule.getSettingsStore().requestSave();
        logDebugP("Widget '%s' duration=%lus", item.toast.c_str(), (unsigned long)DUR_S[idx]);
        return true;
    }
    return false;
}

void MenuWidget::selectItem()
{
    if (_currentMenu.empty()) return;

    auto& item = _currentMenu[_selectedIndex];
    logDebugP("Select item: %s (type: %d) at index %d", item.label.c_str(), static_cast<int>(item.type), _selectedIndex);
    switch (item.type)
    {
        case MenuConfig::MenuElementType::Action:
            if (item.action)
            {
                if (!item.confirmText.empty()) // destructive -> Nein/Ja guard first
                {
                    auto act = item.action;
                    enterConfirmDialog(item.label, item.confirmText, [act]() { if (act) act(); });
                }
                else
                {
                    item.action();
                    logDebugP("Action (%s) executed for item: %s", item.key.c_str(), item.label.c_str());
                }
            }
            else
                logErrorP("No action assigned to this item: %s", item.label.c_str());
            break;

        case MenuConfig::MenuElementType::Submenu:
            if (!item.submenu.empty() || item.submenuBuilder)
            {
                logDebugP("Navigating to submenu: %s", item.label.c_str());
                // `item` is a reference INTO _currentMenu, so copy/build the child out FIRST, then
                // MOVE the parent onto the stack; `_currentMenu = item.submenu` would be
                // self-referential UB (source freed while _currentMenu is cleared).
                std::vector<MenuConfig::MenuOption> child;
                if (!item.submenu.empty())
                {
                    child = item.submenu; // eager submenu -> copy it out
                }
                else
                {
                    // Lazy submenu: build the children now and bind callbacks/defaults as
                    // buildMenuFromRegistry does (heavy subs pay on entry, not on every open).
                    child = item.submenuBuilder();
                    assignRegisteredActions(child);
                    assignOnValueChangedHandlers(child);
                    assignRadioIndexProviders(child);
                    seedDefaultValues(child);
                }
                _selectedIndexStack.push_back(_selectedIndex);
                _menuStack.push_back(std::move(_currentMenu));
                _currentMenu = std::move(child);
                // Land on the first visible child, not blindly index 0.
                _selectedIndex = firstVisibleFrom(0);
            }
            else
            {
                logErrorP("Submenu is empty for item: %s", item.label.c_str());
            }
            break;

        case MenuConfig::MenuElementType::Back:
            // One level up; at the root it closes the menu (selectable "Beenden").
            if (_menuStack.empty())
                closeMenu();
            else
                navigateLeft();
            break;

        case MenuConfig::MenuElementType::Files:
            // Open the SD-card file browser via the wired hook; falls back to item.action().
            logDebugP("Files item selected: %s", item.label.c_str());
            if (_onFilesRequested)
                _onFilesRequested(item);
            else if (item.action)
                item.action();
            else
                logDebugP("Files: no hook wired yet (no-op)");
            break;

        case MenuConfig::MenuElementType::About:
            // Open the "Über" screen via the wired hook; falls back to item.action().
            logDebugP("About item selected: %s", item.label.c_str());
            if (_onAboutRequested)
                _onAboutRequested();
            else if (item.action)
                item.action();
            else
                logDebugP("About: no hook wired yet (no-op)");
            break;

        case MenuConfig::MenuElementType::Checkbox:
            item.defaultValue = MenuValue(!item.defaultValue.getBool());
            if (!item.key.empty())
            {
                _menuConfig.setValue(item.key, item.defaultValue);
                // Dynamic widget_show_/widget_dur_ keys have no static handler -> apply directly.
                if (applyWidgetSetting(item))
                { /* handled */
                }
                else if (item.onValueChanged)
                    item.onValueChanged(item, item.defaultValue);
                else
                    logDebugP("No OnValueChanged handler for key: %s", item.key.c_str());
            }
            break;

        case MenuConfig::MenuElementType::Dropdown:
        {
            if (item.dropdownOptions.empty()) break; // no options -> nothing to cycle (avoid % 0)
            // Radio-flagged dropdowns open a full-screen picker instead of cycling in place.
            if (item.radioList)
            {
                enterRadioSelect(_selectedIndex);
                break;
            }
            // Slider-flagged dropdowns (e.g. brightness) open a horizontal slider.
            if (item.slider)
            {
                enterSlider(_selectedIndex);
                break;
            }
            size_t nextIndex = (item.defaultValue.getSizeT() + 1) % item.dropdownOptions.size();
            item.defaultValue = MenuValue(nextIndex);
            if (!item.key.empty())
            {
                _menuConfig.setValue(item.key, item.defaultValue);
                if (applyWidgetSetting(item))
                { /* handled */
                }
                else if (item.onValueChanged)
                    item.onValueChanged(item, item.defaultValue);
                else
                    logDebugP("No OnValueChanged handler for key: %s", item.key.c_str());
            }
            break;
        }
        case MenuConfig::MenuElementType::TextInput:
            logDebugP("Text editor selected: %s", item.label.c_str());
            enterTextEdit(_selectedIndex);
            break;

        case MenuConfig::MenuElementType::NumberEdit:
            logDebugP("Number editor selected: %s", item.label.c_str());
            enterNumberEdit(_selectedIndex);
            break;

        // Read-only rows are non-interactive (value shown via valueProvider()).
        case MenuConfig::MenuElementType::Readonly:
            logDebugP("Readonly item selected (no action): %s", item.label.c_str());
            break;

        // Prog-Mode toggle -> launch the hold-to-confirm gesture ("prog"); falls back to the
        // bound action (key prog_mode) when the hook is unwired.
        case MenuConfig::MenuElementType::ProgToggle:
            logDebugP("ProgToggle selected: %s", item.label.c_str());
            if (_onGestureAction)
                _onGestureAction("prog", item);
            else if (item.action)
                item.action();
            else
                logDebugP("ProgToggle: no hook/action wired yet (no-op)");
            break;

        // Reboot -> hold-to-confirm gesture ("reboot"); falls back to the bound action
        // (key reboot_device) when unwired.
        case MenuConfig::MenuElementType::Reboot:
            logDebugP("Reboot selected: %s", item.label.c_str());
            if (_onGestureAction)
                _onGestureAction("reboot", item);
            else if (item.action)
                item.action();
            else
                logDebugP("Reboot: no hook/action wired yet (no-op)");
            break;

        // Show the item's toast message as a transient overlay, then run its optional action.
        // A non-empty confirmText gates the whole thing behind a Nein/Ja guard first.
        case MenuConfig::MenuElementType::Toast:
        {
            const std::string message = item.toast;
            auto act = item.action;
            auto perform = [this, message, act]() {
                if (!message.empty())
                    showOverlay([this, message]() {
                        _display->display->clearDisplay();
                        _display->display->setTextSize(1);
                        _display->display->setTextColor(WHITE);
                        _display->display->setCursor(0, 0);
                        _display->display->print(message.c_str());
                        _display->displayBuff();
                    });
                if (act) act();
            };
            if (!item.confirmText.empty())
                enterConfirmDialog(item.label, item.confirmText, perform);
            else
                perform();
            break;
        }

        // 4-octet IP editor (IpEdit / IpAddress). Rows are always visible; DHCP-on
        // rows are locked and select is a no-op, DHCP-off enters the built-in IpEdit sub-mode.
        case MenuConfig::MenuElementType::IpEdit:
        case MenuConfig::MenuElementType::IpAddress:
            logDebugP("IP editor selected: %s", item.label.c_str());
            if (isDhcpLocked(item))
            {
                logDebugP("IpEdit: item DHCP-locked, not entering edit (no-op)");
                break;
            }
            if (_onIpEditRequested) _onIpEditRequested(item);
            enterIpEdit(_selectedIndex);
            break;

        // Enter the built-in Reorder sub-mode (drives the WidgetsManager grab/move/drop API).
        case MenuConfig::MenuElementType::Reorder:
            logDebugP("Reorder selected: %s", item.label.c_str());
            if (_onReorderRequested) _onReorderRequested(item);
            enterReorder();
            break;

        default:
            // Unknown/unhandled item types log and ignore (never crash).
            logDebugP("No select handler for item type %d (%s)", static_cast<int>(item.type), item.label.c_str());
            break;
    }
}

/************************************************************
 ********************* EDITOR SUB-MODES ********************
 ************************************************************/

// Common: return to Normal mode and force a redraw.
void MenuWidget::leaveEditor()
{
    _mode = MenuMode::Normal;
    _confirmOnYes = nullptr; // release any ConfirmDialog callback (captures) on exit
    _needsRedraw = true;
    _editBlinkOn = true;
    _editBlinkLast = millis();
}

// ---------------- IP-octet editor ----------------

// Enter the IP editor for _currentMenu[itemIndex]. Copies ip[4] into a working buffer so a
// discarded edit leaves the stored value untouched. Caller verified the item is editable.
void MenuWidget::enterIpEdit(size_t itemIndex)
{
    if (itemIndex >= _currentMenu.size()) return;
    const auto& item = _currentMenu[itemIndex];

    _ipEditIndex = itemIndex;
    _ipEdit[0] = item.ip[0];
    _ipEdit[1] = item.ip[1];
    _ipEdit[2] = item.ip[2];
    _ipEdit[3] = item.ip[3];
    _ipOctet = 0;
    _mode = MenuMode::IpEdit;
    _editBlinkOn = true;
    _editBlinkLast = millis();
    _needsRedraw = true;
    logDebugP("Enter IP editor: %s (%u.%u.%u.%u)", item.label.c_str(),
              _ipEdit[0], _ipEdit[1], _ipEdit[2], _ipEdit[3]);
}

// Commit the working octets back into the item's ip[4] and fire onValueChanged, then leave. The
// MenuValue packs the four octets as a big-endian size_t; the authoritative value stays ip[4].
void MenuWidget::commitIpEdit()
{
    if (_ipEditIndex < _currentMenu.size())
    {
        auto& item = _currentMenu[_ipEditIndex];
        item.ip[0] = _ipEdit[0];
        item.ip[1] = _ipEdit[1];
        item.ip[2] = _ipEdit[2];
        item.ip[3] = _ipEdit[3];
        logDebugP("Commit IP editor: %s (%u.%u.%u.%u)", item.label.c_str(),
                  item.ip[0], item.ip[1], item.ip[2], item.ip[3]);

        if (item.onValueChanged)
        {
            const size_t packed = (static_cast<size_t>(item.ip[0]) << 24) |
                                  (static_cast<size_t>(item.ip[1]) << 16) |
                                  (static_cast<size_t>(item.ip[2]) << 8) |
                                  (static_cast<size_t>(item.ip[3]));
            item.onValueChanged(item, MenuValue(packed));
            logDebugP("IpEdit onValueChanged fired for key: %s", item.key.c_str());
        }
    }
    leaveEditor();
}

// Discard the working octets (item.ip untouched) and leave to Normal.
void MenuWidget::cancelIpEdit()
{
    logDebugP("Cancel IP editor (no save)");
    leaveEditor();
}

// PRESS handling for the IP editor.
//   LEFT  : octet-- (wrap), EXCEPT LEFT at octet 0 cancels (back)
//   RIGHT : octet++ (wrap)   UP: value++ mod 256   DOWN: value-- mod 256   OK: commit + leave
bool MenuWidget::handleIpEditButton(const ButtonEvent& event)
{
    switch (event.type)
    {
        case ButtonType::UP:
            _ipEdit[_ipOctet] = static_cast<uint8_t>((_ipEdit[_ipOctet] + 1) % 256);
            break;

        case ButtonType::DOWN:
            _ipEdit[_ipOctet] = static_cast<uint8_t>((_ipEdit[_ipOctet] + 255) % 256);
            break;

        case ButtonType::LEFT:
            if (_ipOctet == 0)
            {
                // LEFT at the first octet is "back": cancel without saving.
                cancelIpEdit();
                return true;
            }
            _ipOctet = static_cast<uint8_t>((_ipOctet + 3) % 4);
            break;

        case ButtonType::RIGHT:
            _ipOctet = static_cast<uint8_t>((_ipOctet + 1) % 4);
            break;

        case ButtonType::SELECT:
            commitIpEdit();
            return true;

        default:
            return true; // consume anything else while editing
    }

    // Reset the blink so the newly-active octet is shown immediately.
    _editBlinkOn = true;
    _editBlinkLast = millis();
    _needsRedraw = true;
    return true;
}

// Render the IP editor: label on top, "a.b.c.d" large + centered; the active
// octet blinks (hidden on the "off" phase of the ~500ms toggle).
void MenuWidget::drawIpEditor()
{
    if (!_display || !_display->display) return;

    _display->display->clearDisplay();
    _display->display->setTextWrap(false);

    // Title / label row (size 1).
    _display->display->setTextSize(1);
    _display->display->setTextColor(WHITE, BLACK);
    _display->display->setCursor(2, 0);
    const char* label = (_ipEditIndex < _currentMenu.size())
                            ? _currentMenu[_ipEditIndex].label.c_str()
                            : "IP";
    _display->display->print(label);

    // Build the "a.b.c.d" string and measure it at size 2 to center horizontally.
    char full[16];
    snprintf(full, sizeof(full), "%u.%u.%u.%u",
             static_cast<unsigned>(_ipEdit[0]), static_cast<unsigned>(_ipEdit[1]),
             static_cast<unsigned>(_ipEdit[2]), static_cast<unsigned>(_ipEdit[3]));

    _display->display->setTextSize(2);
    int16_t bx, by;
    uint16_t bw, bh;
    _display->display->getTextBounds(full, 0, 0, &bx, &by, &bw, &bh);
    int16_t x = (static_cast<int16_t>(_screenWidth) - static_cast<int16_t>(bw)) / 2;
    if (x < 0) x = 0;
    const int16_t y = static_cast<int16_t>((_screenHeight - bh) / 2);

    // Draw octet-by-octet so the active octet can blink while the dots/others stay put.
    int16_t cx = x;
    _display->display->setCursor(cx, y);
    for (uint8_t i = 0; i < 4; ++i)
    {
        char oct[4];
        snprintf(oct, sizeof(oct), "%u", static_cast<unsigned>(_ipEdit[i]));

        const bool active = (i == _ipOctet);
        // Measure this octet's width to advance the cursor and size the highlight box.
        int16_t obx, oby;
        uint16_t obw, obh;
        _display->display->getTextBounds(oct, cx, y, &obx, &oby, &obw, &obh);

        if (active)
        {
            if (_editBlinkOn)
            {
                // Highlight the active octet (inverted box) on the "on" phase.
                _display->display->fillRect(cx - 1, y - 1, static_cast<int16_t>(obw) + 2,
                                            static_cast<int16_t>(bh) + 1, WHITE);
                _display->display->setTextColor(BLACK, WHITE);
                _display->display->setCursor(cx, y);
                _display->display->print(oct);
                _display->display->setTextColor(WHITE, BLACK);
            }
            // On the "off" phase, leave the active octet blank (blink effect).
        }
        else
        {
            _display->display->setTextColor(WHITE, BLACK);
            _display->display->setCursor(cx, y);
            _display->display->print(oct);
        }

        cx = static_cast<int16_t>(cx + obw);

        // Draw the separating dot after octets 0..2.
        if (i < 3)
        {
            int16_t dbx, dby;
            uint16_t dbw, dbh;
            _display->display->getTextBounds(".", cx, y, &dbx, &dby, &dbw, &dbh);
            _display->display->setTextColor(WHITE, BLACK);
            _display->display->setCursor(cx, y);
            _display->display->print(".");
            cx = static_cast<int16_t>(cx + dbw);
        }
    }

    // Footer hint (size 1).
    _display->display->setTextSize(1);
    _display->display->setTextColor(WHITE, BLACK);
    _display->display->setCursor(2, static_cast<int16_t>(_screenHeight - 9));
    _display->display->print("OK=ok  <=zurueck");

    _display->displayBuff();
}

// ---------------- text editor (char-scroll) ----------------

// Enter the text editor for _currentMenu[itemIndex]. Copies the item's string value into a working
// buffer so a cancel leaves the original untouched.
void MenuWidget::enterTextEdit(size_t itemIndex)
{
    if (itemIndex >= _currentMenu.size()) return;
    const auto& item = _currentMenu[itemIndex];

    _textEditIndex = itemIndex;
    // valueProvider (live, e.g. the SD volume label) wins over the static defaultValue.
    _textEdit = item.valueProvider ? item.valueProvider()
                                   : (item.defaultValue.isString() ? item.defaultValue.getString() : std::string());
    if (_textEdit.empty()) _textEdit = " ";                                    // seed one editable cell
    if (_textEdit.size() > TEXT_EDIT_MAX) _textEdit.resize(TEXT_EDIT_MAX);
    _textCursor = 0;
    _mode = MenuMode::TextEdit;
    _editBlinkOn = true;
    _editBlinkLast = millis();
    _needsRedraw = true;
    logDebugP("Enter text editor: %s (\"%s\")", item.label.c_str(), _textEdit.c_str());
}

// Commit the working text back into the item (trailing spaces trimmed) and fire onValueChanged.
void MenuWidget::commitTextEdit()
{
    if (_textEditIndex < _currentMenu.size())
    {
        auto& item = _currentMenu[_textEditIndex];
        const size_t end = _textEdit.find_last_not_of(' ');
        const std::string result = (end == std::string::npos) ? std::string() : _textEdit.substr(0, end + 1);
        item.defaultValue = MenuValue(result);
        logDebugP("Commit text editor: %s (\"%s\")", item.label.c_str(), result.c_str());
        if (item.onValueChanged) item.onValueChanged(item, MenuValue(result));
    }
    leaveEditor();
}

// Discard the working text (item untouched) and leave to Normal.
void MenuWidget::cancelTextEdit()
{
    logDebugP("Cancel text editor (no save)");
    leaveEditor();
}

// PRESS handling for the text editor.
//   UP/DOWN : cycle the char at the cursor through CHARSET
//   LEFT    : cursor-- , EXCEPT LEFT at cursor 0 cancels (back)
//   RIGHT   : cursor++ , or grow by one char (space) past the end (up to TEXT_EDIT_MAX)
//   OK      : commit + leave
bool MenuWidget::handleTextEditButton(const ButtonEvent& event)
{
    static const std::string CHARSET =
        " ABCDEFGHIJKLMNOPQRSTUVWXYZabcdefghijklmnopqrstuvwxyz0123456789._-";
    const size_t N = CHARSET.size();

    switch (event.type)
    {
        case ButtonType::UP:
        case ButtonType::DOWN:
        {
            if (_textCursor >= _textEdit.size()) break;
            size_t idx = CHARSET.find(_textEdit[_textCursor]);
            if (idx == std::string::npos) idx = 0;
            idx = (event.type == ButtonType::UP) ? (idx + 1) % N : (idx + N - 1) % N;
            _textEdit[_textCursor] = CHARSET[idx];
            break;
        }

        case ButtonType::LEFT:
            if (_textCursor == 0)
            {
                cancelTextEdit();
                return true;
            }
            _textCursor--;
            break;

        case ButtonType::RIGHT:
            if (static_cast<size_t>(_textCursor) + 1 < _textEdit.size())
                _textCursor++;
            else if (_textEdit.size() < TEXT_EDIT_MAX)
            {
                _textEdit.push_back(' ');
                _textCursor = static_cast<uint8_t>(_textEdit.size() - 1);
            }
            break;

        case ButtonType::SELECT:
            commitTextEdit();
            return true;

        default:
            return true; // consume anything else while editing
    }

    _editBlinkOn = true;
    _editBlinkLast = millis();
    _needsRedraw = true;
    return true;
}

// Render the text editor: label on top, the string large + windowed (cursor stays visible); the
// active character blinks (hidden on the "off" phase of the ~500ms toggle).
void MenuWidget::drawTextEditor()
{
    if (!_display || !_display->display) return;
    auto* d = _display->display;

    d->clearDisplay();
    d->setTextWrap(false);

    // Title / label row (size 1).
    d->setTextSize(1);
    d->setTextColor(WHITE, BLACK);
    d->setCursor(2, 0);
    const char* label = (_textEditIndex < _currentMenu.size()) ? _currentMenu[_textEditIndex].label.c_str() : "Text";
    d->print(label);

    // Characters at size 2, windowed so the cursor is always visible (~10 chars fit at 128px).
    d->setTextSize(2);
    const int16_t cellW = 12;
    const int16_t cellH = 16;
    const uint8_t visible = static_cast<uint8_t>((_screenWidth - 4) / cellW);
    uint8_t start = 0;
    if (visible > 0 && _textCursor >= visible) start = static_cast<uint8_t>(_textCursor - visible + 1);
    const int16_t y = static_cast<int16_t>((_screenHeight - cellH) / 2);

    int16_t cx = 2;
    for (uint8_t i = start; i < _textEdit.size() && i < start + visible; ++i)
    {
        const bool active = (i == _textCursor);
        const char c = _textEdit[i];
        if (active)
        {
            if (_editBlinkOn)
            {
                d->fillRect(cx - 1, y - 1, cellW + 1, cellH + 1, WHITE);
                d->setTextColor(BLACK, WHITE);
                d->setCursor(cx, y);
                d->write(static_cast<uint8_t>(c));
                d->setTextColor(WHITE, BLACK);
            }
            // "off" phase: leave the active cell blank (blink).
        }
        else
        {
            d->setTextColor(WHITE, BLACK);
            d->setCursor(cx, y);
            d->write(static_cast<uint8_t>(c));
        }
        cx = static_cast<int16_t>(cx + cellW);
    }

    // Footer hint (size 1).
    d->setTextSize(1);
    d->setTextColor(WHITE, BLACK);
    d->setCursor(2, static_cast<int16_t>(_screenHeight - 9));
    d->print("OK=ok  <=zurueck");

    _display->displayBuff();
}

// ---------------- confirm dialog ----------------

// Enter the modal Nein/Ja guard. Selection defaults to Nein (safe); onYes runs only on commit-Ja.
void MenuWidget::enterConfirmDialog(const std::string& title, const std::string& message, std::function<void()> onYes)
{
    _confirmTitle = title;
    _confirmMessage = message;
    _confirmOnYes = std::move(onYes);
    _confirmSel = 0; // Nein
    _mode = MenuMode::ConfirmDialog;
    _needsRedraw = true;
    logDebugP("Confirm dialog: %s", title.c_str());
}

// Horizontal [Nein][Ja] layout: LEFT selects Nein, RIGHT selects Ja (LEFT does NOT exit — cancel is
// "Nein + OK"). UP/DOWN toggle. OK commits the selection (Ja runs the captured action).
bool MenuWidget::handleConfirmButton(const ButtonEvent& event)
{
    switch (event.type)
    {
        case ButtonType::LEFT:
            _confirmSel = 0; // Nein (left button)
            _needsRedraw = true;
            return true;

        case ButtonType::RIGHT:
            _confirmSel = 1; // Ja (right button)
            _needsRedraw = true;
            return true;

        case ButtonType::UP:
        case ButtonType::DOWN:
            _confirmSel = _confirmSel ? 0 : 1; // toggle
            _needsRedraw = true;
            return true;

        case ButtonType::SELECT:
        {
            const bool yes = (_confirmSel == 1);
            auto cb = _confirmOnYes;
            leaveEditor(); // back to Normal BEFORE running (onYes may showOverlay / block)
            if (yes && cb) cb();
            return true;
        }

        default:
            return true; // consume anything else while modal
    }
}

// Render: title + wrapped message + [Nein] [Ja] (selected filled).
void MenuWidget::drawConfirmDialog()
{
    if (!_display || !_display->display) return;
    auto* d = _display->display;

    d->clearDisplay();
    d->setTextWrap(false);
    d->setTextSize(1);
    d->setTextColor(WHITE, BLACK);

    d->setCursor(2, 0);
    d->print(_confirmTitle.c_str());
    d->drawLine(0, 10, _screenWidth, 10, WHITE);

    // Message word-wrapped to full-width lines (6px font), up to 3 lines at y=14/23/32. Break at the
    // last space that fits so words are not split mid-letter; fall back to a hard cut for a word that
    // is longer than one line.
    const size_t perLine = (_screenWidth > 12) ? (size_t)((_screenWidth - 4) / 6) : 20;
    size_t pos = 0;
    for (int16_t my = 14; pos < _confirmMessage.size() && my <= 32; my = (int16_t)(my + 9))
    {
        size_t len = _confirmMessage.size() - pos;
        if (len > perLine)
        {
            len = perLine; // hard cut fallback
            const size_t sp = _confirmMessage.rfind(' ', pos + perLine);
            if (sp != std::string::npos && sp > pos)
                len = sp - pos; // break at the last space within the line
        }
        const std::string ln = _confirmMessage.substr(pos, len);
        d->setCursor(2, my);
        d->print(ln.c_str());
        pos += len;
        while (pos < _confirmMessage.size() && _confirmMessage[pos] == ' ')
            pos++; // swallow the break space(s)
    }

    // Buttons [Nein] [Ja]; the selected one is filled, the other outlined.
    static const char* const OPTS[2] = {"Nein", "Ja"};
    const int16_t by = 46, bh = 14, bw = 50;
    for (uint8_t i = 0; i < 2; i++)
    {
        const int16_t bx = (i == 0) ? 8 : (int16_t)(_screenWidth - bw - 8);
        if (i == _confirmSel)
        {
            d->fillRect(bx, by, bw, bh, WHITE);
            d->setTextColor(BLACK);
        }
        else
        {
            d->drawRect(bx, by, bw, bh, WHITE);
            d->setTextColor(WHITE);
        }
        const std::string s = OPTS[i];
        const int16_t tw = (int16_t)(s.size() * 6);
        d->setCursor((int16_t)(bx + (bw - tw) / 2), (int16_t)(by + 4));
        d->print(s.c_str());
        d->setTextColor(WHITE, BLACK);
    }

    _display->displayBuff();
}

// ---------------- number editor ----------------

// Enter the 3-digit number editor for _currentMenu[itemIndex] (minutes 0..999). Reads the current
// value from defaultValue (SizeT); the zero-label comes from dropdownOptions[0] ("nie"/"aus").
void MenuWidget::enterNumberEdit(size_t itemIndex)
{
    if (itemIndex >= _currentMenu.size()) return;
    const auto& item = _currentMenu[itemIndex];

    _numberEditIndex = itemIndex;
    const size_t v = item.defaultValue.isSizeT() ? item.defaultValue.getSizeT() : 0;
    _numberEdit = (v > NUMBER_EDIT_MAX) ? NUMBER_EDIT_MAX : static_cast<uint16_t>(v);
    _numberZeroLabel = item.dropdownOptions.empty() ? std::string("aus") : item.dropdownOptions[0];
    _numberCursor = 0; // start on the hundreds digit
    _mode = MenuMode::NumberEdit;
    _editBlinkOn = true;
    _editBlinkLast = millis();
    _needsRedraw = true;
    logDebugP("Enter number editor: %s (%u)", item.label.c_str(), (unsigned)_numberEdit);
}

// Write the value back (as SizeT minutes) + fire onValueChanged.
void MenuWidget::commitNumberEdit()
{
    if (_numberEditIndex < _currentMenu.size())
    {
        auto& item = _currentMenu[_numberEditIndex];
        item.defaultValue = MenuValue(static_cast<size_t>(_numberEdit));
        logDebugP("Commit number editor: %s (%u)", item.label.c_str(), (unsigned)_numberEdit);
        if (item.onValueChanged) item.onValueChanged(item, MenuValue(static_cast<size_t>(_numberEdit)));
    }
    leaveEditor();
}

void MenuWidget::cancelNumberEdit()
{
    logDebugP("Cancel number editor (no save)");
    leaveEditor();
}

// PRESS handling: UP/DOWN cycle the active digit (0..9), LEFT/RIGHT move the cursor (LEFT at the
// leftmost digit cancels), OK commits.
bool MenuWidget::handleNumberEditButton(const ButtonEvent& event)
{
    uint16_t h = _numberEdit / 100, t = (_numberEdit / 10) % 10, o = _numberEdit % 10;
    switch (event.type)
    {
        case ButtonType::UP:
        case ButtonType::DOWN:
        {
            const uint16_t delta = (event.type == ButtonType::UP) ? 1u : 9u; // +1 / -1 (mod 10)
            if (_numberCursor == 0)      h = (h + delta) % 10;
            else if (_numberCursor == 1) t = (t + delta) % 10;
            else                         o = (o + delta) % 10;
            _numberEdit = static_cast<uint16_t>(h * 100 + t * 10 + o);
            break;
        }
        case ButtonType::LEFT:
            if (_numberCursor == 0) { cancelNumberEdit(); return true; }
            _numberCursor--;
            break;
        case ButtonType::RIGHT:
            if (_numberCursor < 2) _numberCursor++;
            break;
        case ButtonType::SELECT:
            commitNumberEdit();
            return true;
        default:
            return true;
    }
    _editBlinkOn = true;
    _editBlinkLast = millis();
    _needsRedraw = true;
    return true;
}

// Render: label on top, 3 big digits (active blinks) + "min", plus the zero-label hint when 0.
void MenuWidget::drawNumberEditor()
{
    if (!_display || !_display->display) return;
    auto* d = _display->display;

    d->clearDisplay();
    d->setTextWrap(false);

    d->setTextSize(1);
    d->setTextColor(WHITE, BLACK);
    d->setCursor(2, 0);
    const char* label = (_numberEditIndex < _currentMenu.size()) ? _currentMenu[_numberEditIndex].label.c_str() : "Wert";
    d->print(label);

    // 3 digits at size 2, the active one blinks; "min" at size 1 after them.
    const int16_t cellW = 12, cellH = 16;
    const int16_t y = static_cast<int16_t>((_screenHeight - cellH) / 2);
    const uint16_t dig[3] = {static_cast<uint16_t>(_numberEdit / 100),
                             static_cast<uint16_t>((_numberEdit / 10) % 10),
                             static_cast<uint16_t>(_numberEdit % 10)};
    int16_t cx = 30;
    d->setTextSize(2);
    for (uint8_t i = 0; i < 3; ++i, cx = static_cast<int16_t>(cx + cellW))
    {
        const bool active = (i == _numberCursor);
        const char c = static_cast<char>('0' + dig[i]);
        if (active)
        {
            if (_editBlinkOn)
            {
                d->fillRect(cx - 1, y - 1, cellW + 1, cellH + 1, WHITE);
                d->setTextColor(BLACK, WHITE);
                d->setCursor(cx, y);
                d->write(static_cast<uint8_t>(c));
                d->setTextColor(WHITE, BLACK);
            }
            // blink-off: leave the active digit blank
        }
        else
        {
            d->setTextColor(WHITE, BLACK);
            d->setCursor(cx, y);
            d->write(static_cast<uint8_t>(c));
        }
    }
    d->setTextSize(1);
    d->setTextColor(WHITE, BLACK);
    d->setCursor(static_cast<int16_t>(cx + 3), static_cast<int16_t>(y + 4));
    d->print("min");

    // Zero-label hint + control hint (footer).
    if (_numberEdit == 0)
    {
        const std::string z = std::string("0 = ") + _numberZeroLabel;
        d->setCursor(2, static_cast<int16_t>(_screenHeight - 18));
        d->print(z.c_str());
    }
    d->setCursor(2, static_cast<int16_t>(_screenHeight - 9));
    d->print("OK=ok  <=zurueck");

    _display->displayBuff();
}

// ---------------- reorder editor ----------------

// Enter the reorder sub-mode. Any prior grab is released; the cursor starts at index 0.
void MenuWidget::enterReorder()
{
    WidgetsManager* wm = openknxDisplayModule.getWidgetManager();
    if (!wm)
    {
        // No manager available -> nothing to reorder, stay in Normal.
        logDebugP("Reorder: no WidgetsManager available (no-op)");
        return;
    }
    if (wm->isGrabbing()) wm->dropWidget();
    _reorderSel = 0;
    _mode = MenuMode::Reorder;
    _editBlinkOn = true;
    _editBlinkLast = millis();
    _needsRedraw = true;
    logDebugP("Enter reorder editor");
}

// Leave the reorder editor: drop any grab first, then return to Normal.
void MenuWidget::leaveReorder()
{
    WidgetsManager* wm = openknxDisplayModule.getWidgetManager();
    if (wm && wm->isGrabbing()) wm->dropWidget();
    logDebugP("Leave reorder editor");
    leaveEditor();
}

// PRESS handling for the reorder editor (mockup: input()/lvl.reorder).
//   not grabbing: UP/DOWN move selection (wrap), OK grabs sel, LEFT leaves
//   grabbing    : UP/DOWN move grabbed widget (no wrap), OK/LEFT drop
// After any order change, request a debounced settings save.
bool MenuWidget::handleReorderButton(const ButtonEvent& event)
{
    WidgetsManager* wm = openknxDisplayModule.getWidgetManager();
    if (!wm)
    {
        leaveReorder();
        return true;
    }

    const size_t count = wm->getReorderableWidgets().size();
    if (count == 0)
    {
        // Nothing to reorder — LEFT/OK leaves, others no-op.
        if (event.type == ButtonType::LEFT || event.type == ButtonType::SELECT)
            leaveReorder();
        return true;
    }

    if (wm->isGrabbing())
    {
        switch (event.type)
        {
            case ButtonType::UP:
                if (wm->moveGrabbedUp())
                {
                    _reorderSel = static_cast<size_t>(wm->getGrabIndex());
                    openknxDisplayModule.getSettingsStore().requestSave();
                }
                break;
            case ButtonType::DOWN:
                if (wm->moveGrabbedDown())
                {
                    _reorderSel = static_cast<size_t>(wm->getGrabIndex());
                    openknxDisplayModule.getSettingsStore().requestSave();
                }
                break;
            case ButtonType::SELECT:
                wm->dropWidget(); // drop in place, stay in the editor
                break;
            case ButtonType::LEFT:
                wm->dropWidget();
                break;
            default:
                break;
        }
    }
    else
    {
        switch (event.type)
        {
            case ButtonType::UP:
                _reorderSel = (_reorderSel + count - 1) % count;
                break;
            case ButtonType::DOWN:
                _reorderSel = (_reorderSel + 1) % count;
                break;
            case ButtonType::SELECT:
                wm->grabWidget(_reorderSel);
                break;
            case ButtonType::LEFT:
                leaveReorder();
                return true;
            default:
                break;
        }
    }

    _editBlinkOn = true;
    _editBlinkLast = millis();
    _needsRedraw = true;
    return true;
}

// Render the reorder editor (mockup: renderMenu()/lvl.reorder):
//   header "Reihenfolge" + status ("OK=greifen" / "⇕ verschieben")
//   one row per widget: [grab-prefix] name .................... position (1..n)
//   grabbed row inverted/framed; selected row inverted.
void MenuWidget::drawReorder()
{
    if (!_display || !_display->display) return;

    _display->display->clearDisplay();
    _display->display->setTextSize(1);
    _display->display->setTextWrap(false);

    WidgetsManager* wm = openknxDisplayModule.getWidgetManager();
    std::vector<Widget*> widgets;
    int grabIdx = -1;
    if (wm)
    {
        widgets = wm->getReorderableWidgets();
        grabIdx = wm->getGrabIndex();
    }

    // Header: title + right-aligned status.
    const char* status = (grabIdx >= 0) ? "verschieben" : "OK=greifen";
    _display->display->setTextColor(WHITE, BLACK);
    _display->display->setCursor(2, 0);
    _display->display->print("Reihenfolge");
    {
        int16_t bx, by;
        uint16_t bw, bh;
        _display->display->getTextBounds(status, 0, 0, &bx, &by, &bw, &bh);
        int16_t xr = static_cast<int16_t>(_screenWidth) - static_cast<int16_t>(bw) - 2;
        if (xr < 0) xr = 0;
        _display->display->setCursor(xr, 0);
        _display->display->print(status);
    }

    if (widgets.empty())
    {
        _display->display->setCursor(2, 14);
        _display->display->print("(keine Widgets)");
        _display->displayBuff();
        return;
    }

    // Rows below the header; page so the selected row stays visible.
    const uint8_t ROW_H = 10;
    const uint8_t HEADER_H = 12;
    const uint8_t rowsFit = static_cast<uint8_t>((_screenHeight - HEADER_H) / ROW_H);
    const size_t sel = (grabIdx >= 0) ? static_cast<size_t>(grabIdx) : _reorderSel;

    size_t start = 0;
    if (rowsFit > 0 && sel >= rowsFit) start = sel - (rowsFit - 1);

    for (size_t i = start; i < widgets.size() && i < start + rowsFit; ++i)
    {
        const uint8_t yPos = static_cast<uint8_t>(HEADER_H + (i - start) * ROW_H);
        const bool isSel = (i == sel);
        const bool isGrab = (grabIdx >= 0 && static_cast<size_t>(grabIdx) == i);

        if (isSel || isGrab)
        {
            _display->display->fillRect(0, yPos, _screenWidth, ROW_H, WHITE);
            _display->display->setTextColor(BLACK, WHITE);
        }
        else
        {
            _display->display->setTextColor(WHITE, BLACK);
        }

        // Name (with a grab-prefix marker when this row is grabbed).
        _display->display->setCursor(2, yPos + 1);
        if (isGrab) _display->display->print("^ "); // "⇕ " marker (ASCII-safe)
        _display->display->print(widgets[i] ? widgets[i]->getName().c_str() : "?");

        // Right-aligned 1-based position number.
        char num[6];
        snprintf(num, sizeof(num), "%u", static_cast<unsigned>(i + 1));
        int16_t bx, by;
        uint16_t bw, bh;
        _display->display->getTextBounds(num, 0, 0, &bx, &by, &bw, &bh);
        int16_t xr = static_cast<int16_t>(_screenWidth) - static_cast<int16_t>(bw) - 2;
        if (xr < 0) xr = 0;
        _display->display->setCursor(xr, yPos + 1);
        _display->display->print(num);
    }

    _display->displayBuff();
}

// ---------------- radio-select (single-choice) picker ----------------

// Enter the single-select picker for the Dropdown at _currentMenu[itemIndex]. Initial highlight
// reflects the live value (radioIndexProvider if wired, else defaultValue).
void MenuWidget::enterRadioSelect(size_t itemIndex)
{
    if (itemIndex >= _currentMenu.size()) return;
    const auto& item = _currentMenu[itemIndex];
    const size_t count = item.dropdownOptions.size();
    if (count == 0) return;

    size_t current = item.radioIndexProvider ? item.radioIndexProvider() : item.defaultValue.getSizeT();
    if (current >= count) current = 0; // clamp a stale / out-of-range value

    _radioIndex = itemIndex;
    _radioSel = current;
    _radioTop = 0;
    _mode = MenuMode::RadioSelect;
    _needsRedraw = true;
    logDebugP("Enter radio picker: %s (current %u/%u)", item.label.c_str(),
              static_cast<unsigned>(current), static_cast<unsigned>(count));
}

// Commit the highlighted option into the item's defaultValue and fire onValueChanged (same contract
// as an in-place Dropdown cycle).
void MenuWidget::commitRadioSelect()
{
    if (_radioIndex < _currentMenu.size())
    {
        auto& item = _currentMenu[_radioIndex];
        item.defaultValue = MenuValue(_radioSel);
        if (!item.key.empty())
        {
            _menuConfig.setValue(item.key, item.defaultValue);
            if (!applyWidgetSetting(item) && item.onValueChanged)
                item.onValueChanged(item, item.defaultValue);
        }
        logDebugP("Commit radio picker: %s -> idx %u", item.label.c_str(), static_cast<unsigned>(_radioSel));
    }
    leaveEditor();
}

// PRESS handling for the radio picker:
//   UP         : previous option (wrap)
//   DOWN       : next option (wrap)
//   LEFT       : cancel (leave without applying)
//   RIGHT / OK : commit the highlighted option + leave
bool MenuWidget::handleRadioSelectButton(const ButtonEvent& event)
{
    const size_t count = (_radioIndex < _currentMenu.size()) ? _currentMenu[_radioIndex].dropdownOptions.size() : 0;
    if (count == 0)
    {
        leaveEditor();
        return true;
    }

    switch (event.type)
    {
        case ButtonType::UP:
            _radioSel = (_radioSel + count - 1) % count;
            break;
        case ButtonType::DOWN:
            _radioSel = (_radioSel + 1) % count;
            break;
        case ButtonType::LEFT:
            leaveEditor(); // back == cancel, no change
            return true;
        case ButtonType::RIGHT:
        case ButtonType::SELECT:
            commitRadioSelect();
            return true;
        default:
            return true; // consume anything else while picking
    }

    _needsRedraw = true;
    return true;
}

// Render the radio picker: title + a scrollable option list; the highlighted row is inverted with a
// filled bullet, others hollow. Up/down carets hint at more items when the list scrolls.
void MenuWidget::drawRadioSelect()
{
    if (!_display || !_display->display) return;
    if (_radioIndex >= _currentMenu.size())
    {
        leaveEditor();
        return;
    }
    Adafruit_SSD1306* d = _display->display;
    const auto& item = _currentMenu[_radioIndex];
    const size_t count = item.dropdownOptions.size();

    d->clearDisplay();
    d->setTextWrap(false);
    d->setTextSize(1);

    // Title row (item label) + hairline separator.
    d->setTextColor(WHITE, BLACK);
    d->setCursor(2, 0);
    d->print(truncateToWidth(item.label, static_cast<int16_t>(_screenWidth) - 4).c_str());
    d->drawFastHLine(0, 10, static_cast<int16_t>(_screenWidth), WHITE);

    const int16_t listTop = 12;
    const uint8_t ROW_H = 11;
    const uint8_t visibleRows = static_cast<uint8_t>((static_cast<int16_t>(_screenHeight) - listTop) / ROW_H);
    if (visibleRows == 0 || count == 0)
    {
        _display->displayBuff();
        return;
    }

    // Keep the highlighted option in view (scroll offset).
    if (_radioSel < _radioTop) _radioTop = _radioSel;
    else if (_radioSel >= _radioTop + visibleRows)
        _radioTop = _radioSel - visibleRows + 1;
    if (count >= visibleRows && _radioTop + visibleRows > count) _radioTop = count - visibleRows;

    for (size_t row = 0; row < visibleRows && (_radioTop + row) < count; ++row)
    {
        const size_t idx = _radioTop + row;
        const int16_t y = static_cast<int16_t>(listTop + row * ROW_H);
        const bool sel = (idx == _radioSel);
        const uint16_t fg = sel ? BLACK : WHITE;
        const uint16_t bg = sel ? WHITE : BLACK;

        if (sel) d->fillRect(0, static_cast<int16_t>(y - 1), static_cast<int16_t>(_screenWidth), ROW_H, WHITE);

        // Radio bullet: filled dot when selected, hollow ring otherwise.
        const int16_t cx = 6, cy = static_cast<int16_t>(y + 4);
        d->drawCircle(cx, cy, 3, fg);
        if (sel) d->fillCircle(cx, cy, 1, fg);

        d->setTextColor(fg, bg);
        d->setCursor(14, static_cast<int16_t>(y + 1));
        d->print(truncateToWidth(item.dropdownOptions[idx], static_cast<int16_t>(_screenWidth) - 16).c_str());
    }

    // Scroll carets when there is more above / below the visible window.
    const int16_t sw = static_cast<int16_t>(_screenWidth);
    const int16_t sh = static_cast<int16_t>(_screenHeight);
    if (_radioTop > 0)
        d->fillTriangle(static_cast<int16_t>(sw - 6), 12, static_cast<int16_t>(sw - 10), 16, static_cast<int16_t>(sw - 2), 16, WHITE);
    if (_radioTop + visibleRows < count)
        d->fillTriangle(static_cast<int16_t>(sw - 6), static_cast<int16_t>(sh - 1), static_cast<int16_t>(sw - 10), static_cast<int16_t>(sh - 5), static_cast<int16_t>(sw - 2), static_cast<int16_t>(sh - 5), WHITE);

    _display->displayBuff();
}

// ---------------- slider (stepped value bar) editor ----------------

// Enter the slider for the Dropdown at _currentMenu[itemIndex]. Initial stop reflects the live value
// (radioIndexProvider if wired, else defaultValue).
void MenuWidget::enterSlider(size_t itemIndex)
{
    if (itemIndex >= _currentMenu.size()) return;
    const auto& item = _currentMenu[itemIndex];
    const size_t count = item.dropdownOptions.size();
    if (count == 0) return;

    size_t current = item.radioIndexProvider ? item.radioIndexProvider() : item.defaultValue.getSizeT();
    if (current >= count) current = count - 1;

    _sliderIndex = itemIndex;
    _sliderSel = current;
    _mode = MenuMode::Slider;
    _needsRedraw = true;
    logDebugP("Enter slider: %s (stop %u/%u)", item.label.c_str(), static_cast<unsigned>(current), static_cast<unsigned>(count));
}

// Write the current stop into the item + fire onValueChanged. Called live on each adjust so the
// effect (e.g. brightness) applies immediately.
void MenuWidget::applySlider()
{
    if (_sliderIndex >= _currentMenu.size()) return;
    auto& item = _currentMenu[_sliderIndex];
    item.defaultValue = MenuValue(_sliderSel);
    if (item.key.empty()) return;
    _menuConfig.setValue(item.key, item.defaultValue);
    if (!applyWidgetSetting(item) && item.onValueChanged)
        item.onValueChanged(item, item.defaultValue);
}

// PRESS handling for the slider:
//   LEFT / DOWN : previous stop (clamped), apply live
//   RIGHT / UP  : next stop (clamped), apply live
//   OK          : leave (value already applied live)
bool MenuWidget::handleSliderButton(const ButtonEvent& event)
{
    const size_t count = (_sliderIndex < _currentMenu.size()) ? _currentMenu[_sliderIndex].dropdownOptions.size() : 0;
    if (count == 0)
    {
        leaveEditor();
        return true;
    }

    switch (event.type)
    {
        case ButtonType::LEFT:
        case ButtonType::DOWN:
            if (_sliderSel > 0)
            {
                _sliderSel--;
                applySlider();
            }
            break;
        case ButtonType::RIGHT:
        case ButtonType::UP:
            if (_sliderSel + 1 < count)
            {
                _sliderSel++;
                applySlider();
            }
            break;
        case ButtonType::SELECT:
            leaveEditor();
            return true;
        default:
            return true; // consume anything else while sliding
    }

    _needsRedraw = true;
    return true;
}

// Render the slider: title + a bar filled to (stop+1)/count, a knob at the fill edge, and the
// current option label centred below.
void MenuWidget::drawSlider()
{
    if (!_display || !_display->display) return;
    if (_sliderIndex >= _currentMenu.size())
    {
        leaveEditor();
        return;
    }
    Adafruit_SSD1306* d = _display->display;
    const auto& item = _currentMenu[_sliderIndex];
    const size_t count = item.dropdownOptions.size();
    if (count == 0)
    {
        _display->displayBuff();
        return;
    }

    d->clearDisplay();
    d->setTextWrap(false);
    d->setTextSize(1);

    // Title row + hairline separator.
    d->setTextColor(WHITE, BLACK);
    d->setCursor(2, 0);
    d->print(truncateToWidth(item.label, static_cast<int16_t>(_screenWidth) - 4).c_str());
    d->drawFastHLine(0, 10, static_cast<int16_t>(_screenWidth), WHITE);

    // Slider track (rounded rect); fill proportional to the current stop.
    const int16_t margin = 8;
    const int16_t barX = margin;
    const int16_t barW = static_cast<int16_t>(_screenWidth) - 2 * margin;
    const int16_t barH = 10;
    const int16_t barY = 26;
    d->drawRoundRect(barX, barY, barW, barH, 3, WHITE);

    // Stop 0 -> 1/count .. stop count-1 -> count/count (min stop still shows a sliver).
    const int16_t fillW = static_cast<int16_t>((static_cast<int32_t>(barW - 2) * static_cast<int32_t>(_sliderSel + 1)) / static_cast<int32_t>(count));
    if (fillW > 0) d->fillRoundRect(static_cast<int16_t>(barX + 1), static_cast<int16_t>(barY + 1), fillW, static_cast<int16_t>(barH - 2), 2, WHITE);

    // Knob at the fill edge (white disc with a dark rim so it reads over the filled track).
    const int16_t knobX = static_cast<int16_t>(barX + 1 + fillW);
    const int16_t knobY = static_cast<int16_t>(barY + barH / 2);
    d->fillCircle(knobX, knobY, 4, WHITE);
    d->drawCircle(knobX, knobY, 4, BLACK);

    // Current value label, centred below the bar.
    const std::string& val = item.dropdownOptions[_sliderSel];
    int16_t bx, by;
    uint16_t bw, bh;
    d->getTextBounds(val.c_str(), 0, 0, &bx, &by, &bw, &bh);
    int16_t vx = static_cast<int16_t>((static_cast<int16_t>(_screenWidth) - static_cast<int16_t>(bw)) / 2);
    if (vx < 0) vx = 0;
    d->setTextColor(WHITE, BLACK);
    d->setCursor(vx, static_cast<int16_t>(barY + barH + 6));
    d->print(val.c_str());

    _display->displayBuff();
}

// ---------------- Icon main menu (root icon grid) ----------------

// Move the root cursor within the ICON_COLS x ICON_ROWS grid (dCol/dRow -1/0/+1), clamped at the
// edges; DOWN into a partial last row lands on the last item.
void MenuWidget::gridMove(int dCol, int dRow)
{
    std::vector<size_t> vis;
    vis.reserve(_currentMenu.size());
    for (size_t i = 0; i < _currentMenu.size(); ++i)
        if (isItemVisible(_currentMenu[i])) vis.push_back(i);
    if (vis.empty()) return;

    size_t pos = 0;
    for (size_t v = 0; v < vis.size(); ++v)
        if (vis[v] == _selectedIndex)
        {
            pos = v;
            break;
        }

    const int cols = ICON_COLS;
    const int col = static_cast<int>(pos) % cols;
    int np = static_cast<int>(pos);
    if (dCol > 0 && col < cols - 1 && pos + 1 < vis.size())
        np = static_cast<int>(pos) + 1;
    else if (dCol < 0 && col > 0)
        np = static_cast<int>(pos) - 1;
    else if (dRow > 0)
    {
        if (pos + static_cast<size_t>(cols) < vis.size())
            np = static_cast<int>(pos) + cols;
        else if (pos / cols < (vis.size() - 1) / cols) // partial last row -> last item
            np = static_cast<int>(vis.size()) - 1;
    }
    else if (dRow < 0 && pos >= static_cast<size_t>(cols))
        np = static_cast<int>(pos) - cols;

    if (np >= 0 && np < static_cast<int>(vis.size()) && vis[static_cast<size_t>(np)] != _selectedIndex)
    {
        _selectedIndex = vis[static_cast<size_t>(np)];
        _needsRedraw = true;
    }
}

// Render the root as a paged ICON_COLS x ICON_ROWS icon grid; the selected cell is inverted. Page
// dots along the bottom when > 1 page.
void MenuWidget::drawIconMenu()
{
    if (!_display || !_display->display) return;
    Adafruit_SSD1306* d = _display->display;
    d->clearDisplay();
    d->setTextSize(1);
    d->setTextWrap(false);

    std::vector<size_t> vis;
    vis.reserve(_currentMenu.size());
    for (size_t i = 0; i < _currentMenu.size(); ++i)
        if (isItemVisible(_currentMenu[i])) vis.push_back(i);
    if (vis.empty())
    {
        _display->displayBuff();
        return;
    }

    size_t sel = 0;
    for (size_t v = 0; v < vis.size(); ++v)
        if (vis[v] == _selectedIndex)
        {
            sel = v;
            break;
        }

    // Top header line: the selected item's full label (instead of a tiny caption under every icon).
    const int16_t HEADER_H = 11;
    {
        const std::string title = truncateToWidth(_currentMenu[vis[sel]].label, static_cast<int16_t>(_screenWidth - 2));
        int16_t tbx, tby;
        uint16_t tbw, tbh;
        d->getTextBounds(title.c_str(), 0, 0, &tbx, &tby, &tbw, &tbh);
        int16_t tx = static_cast<int16_t>((static_cast<int16_t>(_screenWidth) - static_cast<int16_t>(tbw)) / 2);
        if (tx < 1) tx = 1;
        d->setTextColor(WHITE, BLACK);
        d->setCursor(tx, 1);
        d->print(title.c_str());
        d->drawFastHLine(0, static_cast<int16_t>(HEADER_H - 1), static_cast<int16_t>(_screenWidth), WHITE);
    }

    const int cols = ICON_COLS, rows = ICON_ROWS, perPage = cols * rows;
    const size_t page = sel / static_cast<size_t>(perPage);
    const size_t start = page * static_cast<size_t>(perPage);
    const size_t pages = (vis.size() + perPage - 1) / static_cast<size_t>(perPage);

    const int16_t sw = static_cast<int16_t>(_screenWidth);
    const int16_t footer = (pages > 1) ? 4 : 0; // room for page dots
    const int16_t cellW = static_cast<int16_t>(sw / cols);
    const int16_t cellH = static_cast<int16_t>((static_cast<int16_t>(_screenHeight) - HEADER_H - footer) / rows);

    for (size_t k = 0; k < static_cast<size_t>(perPage) && (start + k) < vis.size(); ++k)
    {
        const size_t idx = vis[start + k];
        const int16_t c = static_cast<int16_t>(k % cols), r = static_cast<int16_t>(k / cols);
        const int16_t x0 = static_cast<int16_t>(c * cellW), y0 = static_cast<int16_t>(HEADER_H + r * cellH);
        const bool selCell = (start + k == sel);
        const uint16_t fg = selCell ? BLACK : WHITE;
        if (selCell) d->fillRoundRect(static_cast<int16_t>(x0 + 1), static_cast<int16_t>(y0 + 1), static_cast<int16_t>(cellW - 2), static_cast<int16_t>(cellH - 2), 3, WHITE);

        // Icon centred in the cell; label is the top header, no per-icon caption.
        drawCategoryIcon(_currentMenu[idx].label, static_cast<int16_t>(x0 + cellW / 2), static_cast<int16_t>(y0 + cellH / 2), fg);
    }

    if (pages > 1)
    {
        const int16_t totalW = static_cast<int16_t>(pages * 4 - 1);
        int16_t dx = static_cast<int16_t>((sw - totalW) / 2);
        for (size_t pd = 0; pd < pages; ++pd)
        {
            d->fillRect(dx, static_cast<int16_t>(_screenHeight - 2), (pd == page) ? 3 : 2, 2, WHITE);
            dx = static_cast<int16_t>(dx + 4);
        }
    }

    _display->displayBuff();
}

// Draw an ~18x18 category glyph at (cx,cy) in colour `col`, chosen by the item label (pure GFX
// primitives). More-specific matches are tested before shorter ones.
void MenuWidget::drawCategoryIcon(const std::string& label, int16_t cx, int16_t cy, uint16_t col)
{
    Adafruit_SSD1306* d = _display->display;
    const auto has = [&](const char* s) { return label.find(s) != std::string::npos; };

    if (has("System-Info") || has("Info"))
    {
        // "i" in a circle
        d->drawCircle(cx, cy, 8, col);
        d->fillRect(static_cast<int16_t>(cx - 1), static_cast<int16_t>(cy - 4), 2, 2, col); // dot
        d->fillRect(static_cast<int16_t>(cx - 1), static_cast<int16_t>(cy - 1), 2, 5, col); // stem
    }
    else if (has("Anzeige") || has("Display"))
    {
        // monitor + stand
        d->drawRoundRect(static_cast<int16_t>(cx - 9), static_cast<int16_t>(cy - 7), 18, 12, 2, col);
        d->drawFastHLine(static_cast<int16_t>(cx - 4), static_cast<int16_t>(cy + 7), 8, col);
        d->drawFastVLine(cx, static_cast<int16_t>(cy + 5), 2, col);
    }
    else if (has("Netzwerk") || has("Netz") || has("LAN"))
    {
        // 3-node topology (coupler + two devices)
        d->drawLine(cx, static_cast<int16_t>(cy - 5), static_cast<int16_t>(cx - 7), static_cast<int16_t>(cy + 5), col);
        d->drawLine(cx, static_cast<int16_t>(cy - 5), static_cast<int16_t>(cx + 7), static_cast<int16_t>(cy + 5), col);
        d->fillCircle(cx, static_cast<int16_t>(cy - 6), 2, col);
        d->fillCircle(static_cast<int16_t>(cx - 7), static_cast<int16_t>(cy + 6), 2, col);
        d->fillCircle(static_cast<int16_t>(cx + 7), static_cast<int16_t>(cy + 6), 2, col);
    }
    else if (has("Widget"))
    {
        // 2x2 tiles
        d->drawRect(static_cast<int16_t>(cx - 8), static_cast<int16_t>(cy - 8), 7, 7, col);
        d->drawRect(static_cast<int16_t>(cx + 1), static_cast<int16_t>(cy - 8), 7, 7, col);
        d->drawRect(static_cast<int16_t>(cx - 8), static_cast<int16_t>(cy + 1), 7, 7, col);
        d->drawRect(static_cast<int16_t>(cx + 1), static_cast<int16_t>(cy + 1), 7, 7, col);
    }
    else if (has("Home") || has("Tasten"))
    {
        // D-pad (plus)
        d->fillRect(static_cast<int16_t>(cx - 2), static_cast<int16_t>(cy - 8), 5, 17, col);
        d->fillRect(static_cast<int16_t>(cx - 8), static_cast<int16_t>(cy - 2), 17, 5, col);
    }
    else if (has("Ueber") || has("About"))
    {
        // developer: head + shoulders silhouette
        d->fillCircle(cx, static_cast<int16_t>(cy - 4), 3, col);
        d->fillTriangle(static_cast<int16_t>(cx - 7), static_cast<int16_t>(cy + 8), static_cast<int16_t>(cx + 7), static_cast<int16_t>(cy + 8), cx, static_cast<int16_t>(cy + 1), col);
    }
    else if (has("SD") || has("Karte") || has("Card"))
    {
        // SD card (rect with a cut top-right corner)
        d->drawLine(static_cast<int16_t>(cx - 6), static_cast<int16_t>(cy - 8), static_cast<int16_t>(cx + 2), static_cast<int16_t>(cy - 8), col);
        d->drawLine(static_cast<int16_t>(cx + 2), static_cast<int16_t>(cy - 8), static_cast<int16_t>(cx + 6), static_cast<int16_t>(cy - 4), col);
        d->drawLine(static_cast<int16_t>(cx + 6), static_cast<int16_t>(cy - 4), static_cast<int16_t>(cx + 6), static_cast<int16_t>(cy + 8), col);
        d->drawLine(static_cast<int16_t>(cx + 6), static_cast<int16_t>(cy + 8), static_cast<int16_t>(cx - 6), static_cast<int16_t>(cy + 8), col);
        d->drawLine(static_cast<int16_t>(cx - 6), static_cast<int16_t>(cy + 8), static_cast<int16_t>(cx - 6), static_cast<int16_t>(cy - 8), col);
    }
    else if (has("System"))
    {
        // gear: circle + 4 teeth + hub
        d->drawCircle(cx, cy, 6, col);
        d->fillCircle(cx, cy, 2, col);
        d->fillRect(static_cast<int16_t>(cx - 1), static_cast<int16_t>(cy - 9), 2, 3, col);
        d->fillRect(static_cast<int16_t>(cx - 1), static_cast<int16_t>(cy + 6), 2, 3, col);
        d->fillRect(static_cast<int16_t>(cx - 9), static_cast<int16_t>(cy - 1), 3, 2, col);
        d->fillRect(static_cast<int16_t>(cx + 6), static_cast<int16_t>(cy - 1), 3, 2, col);
    }
    else
    {
        // generic: rounded tile with a dot
        d->drawRoundRect(static_cast<int16_t>(cx - 8), static_cast<int16_t>(cy - 8), 16, 16, 3, col);
        d->fillCircle(cx, cy, 2, col);
    }
}

// External navigation methods (just delegate to internal ones)
void MenuWidget::externalNavigateUp() { navigateUp(); }
void MenuWidget::externalNavigateDown() { navigateDown(); }
void MenuWidget::externalSelectItem() { selectItem(); }
void MenuWidget::externalPause() { pause(); }
void MenuWidget::externalResume() { resume(); }
void MenuWidget::externalStop() { stop(); }

/****************************************************
 ****************** DISPLAY HANDLING ****************
 ****************************************************/
void MenuWidget::clearDisplay()
{
    if (!_display || !_display->display) return;
    _display->display->clearDisplay();
    _display->displayBuff();
}

// `text` truncated with an ASCII "..." if it exceeds `maxWidth` px. Caller must have set the
// text size first (drawMenu uses size 1).
std::string MenuWidget::truncateToWidth(const std::string& text, int16_t maxWidth) const
{
    if (!_display || !_display->display || maxWidth <= 0) return text;

    auto widthOf = [this](const std::string& s) -> int16_t {
        int16_t bx, by;
        uint16_t bw, bh;
        _display->display->getTextBounds(s.c_str(), 0, 0, &bx, &by, &bw, &bh);
        return static_cast<int16_t>(bw);
    };

    if (widthOf(text) <= maxWidth) return text;

    // Overflow: find the longest prefix that fits together with the "..." suffix.
    static const char* kEllipsis = "...";
    for (size_t len = text.size(); len-- > 0;)
    {
        const std::string candidate = text.substr(0, len) + kEllipsis;
        if (widthOf(candidate) <= maxWidth) return candidate;
    }
    // Not even "..." fits: return it anyway (hard-clipped) rather than nothing.
    return kEllipsis;
}

// Small graphic on/off toggle switch (rounded pill + knob) for a bool row's value area. `color` is
// the row foreground. Replaces the old "[x]"/"[ ]" text for Checkbox/ProgToggle.
void MenuWidget::drawToggleSwitch(int16_t x, int16_t y, int16_t w, int16_t h, bool on, uint16_t fg, uint16_t bg)
{
    if (!_display || !_display->display) return;
    auto* d = _display->display;
    const int16_t r = static_cast<int16_t>(h / 2);        // pill corner radius (stadium shape)
    const int16_t knobR = static_cast<int16_t>(h / 2 - 2); // knob radius (leaves a 1px gap inside the pill)
    const int16_t cy = static_cast<int16_t>(y + h / 2);
    if (on)
    {
        // ON: solid filled pill with the knob to the RIGHT in the background colour -> reads as a clear switch.
        d->fillRoundRect(x, y, w, h, r, fg);
        d->fillCircle(static_cast<int16_t>(x + w - 1 - r), cy, knobR, bg);
    }
    else
    {
        // OFF: outlined pill with the knob to the LEFT (empty track).
        d->drawRoundRect(x, y, w, h, r, fg);
        d->fillCircle(static_cast<int16_t>(x + r), cy, knobR, fg);
    }
}

void MenuWidget::drawMenu()
{
    if (!_display || !_display->display) return;

    // Editor sub-modes draw their own full-screen editor.
    if (_mode == MenuMode::IpEdit)
    {
        drawIpEditor();
        return;
    }
    if (_mode == MenuMode::Reorder)
    {
        drawReorder();
        return;
    }
    if (_mode == MenuMode::RadioSelect)
    {
        drawRadioSelect();
        return;
    }
    if (_mode == MenuMode::Slider)
    {
        drawSlider();
        return;
    }
    if (_mode == MenuMode::TextEdit)
    {
        drawTextEditor();
        return;
    }
    if (_mode == MenuMode::ConfirmDialog)
    {
        drawConfirmDialog();
        return;
    }
    if (_mode == MenuMode::NumberEdit)
    {
        drawNumberEditor();
        return;
    }

    _display->display->clearDisplay();
    _display->display->setTextSize(1); // Normal 1:1 pixel scale
    _display->display->setTextWrap(false);

    if (_infoOverlayActive && _overlayDrawFunction)
    {
        _overlayDrawFunction(); // We draw the info overlay
        return;                 // Skip the rest of the menu drawing
    }

    // Icon main menu: at the root, render the categories as an icon grid instead of a text list.
    if (iconGridActive())
    {
        drawIconMenu();
        return;
    }

    const uint8_t ITEM_HEIGHT = 10;
    const uint8_t ITEM_MARGIN = 2;
    const uint8_t MAX_VISIBLE_ITEMS = _screenHeight / (ITEM_HEIGHT + ITEM_MARGIN);

    // Build the list of VISIBLE row indices (visibleIf + devOnly); filtered rows are neither
    // drawn nor counted toward paging.
    std::vector<size_t> visible;
    visible.reserve(_currentMenu.size());
    for (size_t i = 0; i < _currentMenu.size(); ++i)
    {
        if (isItemVisible(_currentMenu[i])) visible.push_back(i);
    }
    if (visible.empty())
    {
        _display->displayBuff();
        return;
    }

    // Locate the selected item within the visible list (fallback to first).
    size_t selVis = 0;
    for (size_t v = 0; v < visible.size(); ++v)
    {
        if (visible[v] == _selectedIndex)
        {
            selVis = v;
            break;
        }
    }

    const size_t startVis = (selVis >= MAX_VISIBLE_ITEMS) ? selVis - (MAX_VISIBLE_ITEMS - 1) : 0;

    for (size_t v = startVis; v < visible.size() && v < startVis + MAX_VISIBLE_ITEMS; ++v)
    {
        const uint8_t yPos = static_cast<uint8_t>((v - startVis) * (ITEM_HEIGHT + ITEM_MARGIN));
        const size_t i = visible[v];
        const bool isSelected = (i == _selectedIndex);
        const auto& item = _currentMenu[i];

        if (isSelected)
        {
            _display->display->fillRect(0, yPos, _screenWidth, ITEM_HEIGHT, WHITE);
            _display->display->setTextColor(BLACK, WHITE);
        }
        else
        {
            _display->display->setTextColor(WHITE, BLACK);
        }

        // DHCP-locked IP rows show "auto (DHCP)" on the selected row ("auto" on others,
        // already from itemValueText()).
        std::string valueText = itemValueText(item);
        if (isSelected && isDhcpLocked(item)) valueText = "auto (DHCP)";

        // bool rows render a graphic toggle SWITCH instead of "[x]"/"[ ]" text.
        if (item.type == MenuConfig::MenuElementType::Checkbox ||
            item.type == MenuConfig::MenuElementType::ProgToggle)
        {
            const bool on = (item.type == MenuConfig::MenuElementType::ProgToggle)
                                ? knx.progMode()
                                : item.defaultValue.getBool();
            const uint16_t fg = isSelected ? BLACK : WHITE;
            const uint16_t bg = isSelected ? WHITE : BLACK;
            const int16_t sw = 16, sh = 8;
            const int16_t sx = static_cast<int16_t>(_screenWidth) - sw - 2;
            drawToggleSwitch(sx, static_cast<int16_t>(yPos + 1), sw, sh, on, fg, bg);
            _display->display->setTextColor(fg, bg);
            _display->display->setCursor(2, yPos + 2);
            _display->display->print(truncateToWidth(item.label, sx - 4).c_str());
            continue; // switch + label drawn; skip the text-value paths below
        }

        if (isSelected)
        {
            // Same layout as the non-selected row: value RIGHT-aligned first, label truncated to fit — so
            // the VALUE is always visible and only the LABEL shrinks on overflow (the reported bug was the
            // value collapsing to "<..." on long rows). A plain Dropdown wraps its value in "< value >"
            // (OK cycles); a radio-list/slider Dropdown shows "value >" (OK opens a picker).
            const bool opensEditor = (item.radioList || item.slider);
            std::string valueStr;
            if (!valueText.empty())
            {
                if (item.type == MenuConfig::MenuElementType::Dropdown && !opensEditor)
                    valueStr = "< " + valueText + " >";
                else if (item.type == MenuConfig::MenuElementType::Dropdown && opensEditor)
                    valueStr = valueText + " >";
                else
                    valueStr = valueText;
            }

            int16_t availW = static_cast<int16_t>(_screenWidth) - 4;
            if (!valueStr.empty())
            {
                int16_t bx, by;
                uint16_t bw, bh;
                _display->display->getTextBounds(valueStr.c_str(), 0, yPos + 2, &bx, &by, &bw, &bh);
                int16_t xRight = static_cast<int16_t>(_screenWidth) - static_cast<int16_t>(bw) - 2;
                if (xRight < 0) xRight = 0;
                _display->display->setCursor(xRight, yPos + 2);
                _display->display->print(valueStr.c_str());
                availW = xRight - 2 - 2; // label runs up to just before the value
            }
            if (availW < 0) availW = 0;

            const std::string shownLabel = truncateToWidth(item.label, availW);
            _display->display->setCursor(2, yPos + 2);
            _display->display->print(shownLabel.c_str());
        }
        else
        {
            // Non-selected row = label (left) + right-aligned value; the label is truncated
            // with "..." when it would overflow into the value.
            int16_t availW = static_cast<int16_t>(_screenWidth) - 4; // 2px margins
            if (!valueText.empty())
            {
                int16_t bx, by;
                uint16_t bw, bh;
                _display->display->getTextBounds(valueText.c_str(), 0, yPos + 2, &bx, &by, &bw, &bh);
                int16_t xRight = static_cast<int16_t>(_screenWidth) - static_cast<int16_t>(bw) - 2;
                if (xRight < 0) xRight = 0;
                _display->display->setCursor(xRight, yPos + 2);
                _display->display->print(valueText.c_str());

                // Label may run up to just before the value (leave a small gap).
                availW = xRight - 2 - 2;
            }
            if (availW < 0) availW = 0;

            const std::string shownLabel = truncateToWidth(item.label, availW);
            _display->display->setCursor(2, yPos + 2);
            _display->display->print(shownLabel.c_str());
        }
    }
    _display->displayBuff();
}
#endif // DEVICE_DISPLAY_MODULE
