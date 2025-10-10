#ifdef DEVICE_DISPLAY_MODULE
    #define USE_GPIO_MODULE
    #include "Menu.h"
    #include "DefaultMenus.h"
    #include "OpenKNX.h"

MenuWidget::MenuWidget(uint32_t displayTime, WidgetFlags action, uint16_t buttonUp, uint16_t buttonDown, uint16_t buttonSelect, uint16_t buttonLeft, uint16_t buttonRight)
    : _displayTime(displayTime),
      _action(action),
      _buttonUp(buttonUp),
      _buttonDown(buttonDown),
      _buttonSelect(buttonSelect),
      _buttonLeft(buttonLeft),
      _buttonRight(buttonRight),
      _selectedIndex(0),
      _lastButtonPressTime(0),
      _lastButtonCheck(0),
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

    addDefaultMenus();

    #ifdef USE_GPIO_MODULE
    if (!openknx.gpio.isInitialized(1))
    {
        logErrorP("GPIO Module not initialized");
        return;
    }
    else
    {
        _FrontPlateEnabled = true;
        logDebugP("GPIO Module initialized");

        // Initialize buttons
        const uint16_t pins[] = {_buttonUp, _buttonDown, _buttonSelect, _buttonLeft, _buttonRight, 0x0103};
        for (auto pin : pins)
        {
            openknx.gpio.pinMode(pin, INPUT, true, 0);
        }

        // Initialize LEDs
        openknx.gpio.pinMode(FRONTPLATE_LED_RED, OUTPUT, false, 0);
        openknx.gpio.pinMode(FRONTPLATE_LED_GREEN, OUTPUT, false, 0);

        // Turn on both LEDs to indicate that the MenuWidget is active
        setLED(FRONTPLATE_LED_RED, HIGH);   // Switch on the Prog LED (RED)
        setLED(FRONTPLATE_LED_GREEN, HIGH); // Switch on the Info LED
    }
    #endif
    _state = WidgetState::BACKGROUND; // Start Menu in background! Will be started by button press.
}

bool MenuWidget::readButton(uint16_t pin)
{
    // openknx.gpio.pinMode(pin, OUTPUT);
    // openknx.gpio.digitalWrite(pin, HIGH);
    // openknx.gpio.pinMode(pin, INPUT);
    // bool state = !openknx.gpio.digitalRead(pin);
    // openknx.gpio.digitalWrite(pin, HIGH);
    // return !state;
    #ifdef USE_GPIO_MODULE
    return openknx.gpio.digitalRead(pin);
    #else
    return false;
    #endif
}

bool MenuWidget::setLED(uint16_t pin, bool state)
{
    #ifdef USE_GPIO_MODULE
    openknx.gpio.digitalWrite(pin, state ? HIGH : LOW);
    return true;
    #else
    return false;
    #endif
}

bool MenuWidget::isAnyButtonPressed()
{
    // Info: _buttonLeft is active LOW. We need to invert the reading.
    return readButton(_buttonUp) || readButton(_buttonDown) || readButton(_buttonSelect) || !readButton(_buttonLeft) || readButton(_buttonRight);
}

bool MenuWidget::processButtonPress() // We need to call this periodically in loop(), to check for button presses
{
    bool bRet = false;
    if (_FrontPlateEnabled) // Only process if front plate is present (Buttons available)
        if (readButton(_buttonUp))
        {
            navigateUp();
            bRet = true;
            setLED(FRONTPLATE_LED_RED, LOW); // Switch off Prog LED (RED)
        }
        else if (readButton(_buttonDown))
        {
            navigateDown();
            bRet = true;
            setLED(FRONTPLATE_LED_GREEN, LOW); // Switch off Info LED (GREEN)
        }
        else if (readButton(_buttonSelect))
        {
            selectItem();
            bRet = true;
        }
        else if (!readButton(_buttonLeft))
        {
            navigateLeft();
            bRet = true;
            setLED(FRONTPLATE_LED_RED, HIGH); // Switch on Prog LED (RED)
        }
        else if (readButton(_buttonRight))
        {
            navigateRight();
            bRet = true;
            setLED(FRONTPLATE_LED_GREEN, HIGH); // Switch on Info LED (GREEN)
        }
    return bRet;
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

    if ((_infoOverlayActive && isAnyButtonPressed() && // Close overlay on any button press
         (currentTime - _lastButtonPressTime) > _infoOverlayTimeout) ||
        (_infoOverlayActive && (currentTime - _lastButtonPressTime) > _infoOverlayMaxTimeout))
    {
        _lastButtonPressTime = currentTime;
        _infoOverlayActive = false;
        _needsRedraw = true;

        if ((_infoOverlayActive && (currentTime - _lastButtonPressTime) > _infoOverlayMaxTimeout))
            logDebugP("Overlay closed automatically after infoOverlayMaxTimeout of %d ms", _infoOverlayMaxTimeout);
        else
            logDebugP("Overlay closed by button press");
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
    // Automatic background setting on inactivity
    else if ((currentTime - _lastButtonPressTime) >= _displayTime && !_infoOverlayActive)
    {
        if (_state == WidgetState::RUNNING)
        {
            background(); // Removes DisplayEnabled, manager recognizes this
            return;
        }
    }

    // Button checks
    if (currentTime - _lastButtonCheck >= BUTTON_CHECK_INTERVAL)
    {
        _lastButtonCheck = currentTime;
        if (processButtonPress())
        {
            _lastButtonPressTime = currentTime;
            _needsRedraw = true;
        }
    }

    if (_state == WidgetState::RUNNING && _needsRedraw)
    {
        drawMenu();
        _needsRedraw = false;
        _lastRedrawTime = currentTime;
    }
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
    // Clean up the memory
    _menuStack.clear();
    _currentMenu.clear();
    _selectedIndex = 0;
    clearDisplay();
    logDebugP("Stop...");
}

void MenuWidget::pause()
{
    /*
      _stateLast = _state;
      _state = WidgetState::PAUSED; // No pausing for menu, just go to background
      _infoOverlayActive = false; // Ensure overlay is reseted
    */

    logDebugP("Pause requested, going to background...");
    background();
}

void MenuWidget::resume()
{
    _state = _stateLast;
    _state = WidgetState::RUNNING;
    _needsRedraw = true;
    addAction(WidgetFlags::DisplayEnabled);
    // removeAction(WidgetFlags::Background);
    logDebugP("Resume...");
}

void MenuWidget::background()
{
    _stateLast = _state;
    _state = WidgetState::BACKGROUND;
    _infoOverlayActive = false; // Ensure overlay is reseted
    _needsRedraw = false;

    // addAction(WidgetFlags::Background);
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
    assignRegisteredActions(_currentMenu);
}

void MenuWidget::addDefaultOnValueChanged()
{
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
    registerOnValueChanged("auto_dimming", [this](const MenuConfig::MenuOption& opt, const MenuValue& val) {
        if (val.isBool())
        {
            if (val.getBool())
            {
                _display->display->dim(true);
                logDebugP("Auto Dimming enabled");
            }
            else
            {
                _display->display->dim(false);
                logDebugP("Auto Dimming disabled");
            }
        }
    });

    assignOnValueChangedHandlers(_currentMenu);
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

void MenuWidget::showOverlay(std::function<void()> drawFn)
{
    _overlayDrawFunction = std::move(drawFn);
    _infoOverlayActive = true;
    _needsRedraw = true;
    logDebugP("Showing overlay");
}

// External navigation methods (just delegate to internal ones)
void MenuWidget::externalNavigateUp() { navigateUp(); }
void MenuWidget::externalNavigateDown() { navigateDown(); }
void MenuWidget::externalSelectItem() { selectItem(); }
void MenuWidget::externalPause() { pause(); }
void MenuWidget::externalResume() { resume(); }
void MenuWidget::externalStop() { stop(); }

// Core functionality
void MenuWidget::navigateUp()
{
    logDebugP("Navigate up");
    if (_selectedIndex > 0)
    {
        --_selectedIndex;
        logDebugP("Navigated up to index %d", _selectedIndex);
    }
}

void MenuWidget::navigateDown()
{
    logDebugP("Navigate down");
    if (_selectedIndex < _currentMenu.size() - 1)
    {
        ++_selectedIndex;
        logDebugP("Navigated down to index %d", _selectedIndex);
    }
}

void MenuWidget::navigateLeft()
{
    logDebugP("Navigate left");
    if (_selectedIndex > 0)
    {
        //--_selectedIndex;
        // logDebugP("Navigated left to index %d", _selectedIndex);
    }
}

void MenuWidget::navigateRight()
{
    logDebugP("Navigate right");
    if (_selectedIndex < _currentMenu.size() - 1)
    {
        //++_selectedIndex;
        // logDebugP("Navigated right to index %d", _selectedIndex);
    }
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
                item.action();
                logDebugP("Action (%s) executed for item: %s", item.key.c_str(), item.label.c_str());
            }
            else
                logErrorP("No action assigned to this item: %s", item.label.c_str());
            break;

        case MenuConfig::MenuElementType::Submenu:
            if (!item.submenu.empty())
            {
                logDebugP("Navigating to submenu: %s", item.label.c_str());
                _menuStack.push_back(_currentMenu);
                _currentMenu = item.submenu;
                _selectedIndex = 0;
            }
            else
            {
                logErrorP("Submenu is empty for item: %s", item.label.c_str());
            }
            break;

        case MenuConfig::MenuElementType::Back:
            if (!_menuStack.empty())
            {
                _currentMenu = std::move(_menuStack.back());
                _menuStack.pop_back();
                _selectedIndex = 0;
            }
            break;

        case MenuConfig::MenuElementType::Checkbox:
            item.defaultValue = MenuValue(!item.defaultValue.getBool());
            if (!item.key.empty())
            {
                _menuConfig.setValue(item.key, item.defaultValue);
                if (item.onValueChanged)
                {
                    item.onValueChanged(item, item.defaultValue);
                    logDebugP("OnValueChanged called for key: %s", item.key.c_str());
                }
                else
                {
                    logErrorP("No OnValueChanged handler for key: %s", item.key.c_str());
                }
            }
            break;

        case MenuConfig::MenuElementType::Dropdown:
        {
            size_t nextIndex = (item.defaultValue.getSizeT() + 1) % item.dropdownOptions.size();
            item.defaultValue = MenuValue(nextIndex);
            if (!item.key.empty())
            {
                _menuConfig.setValue(item.key, item.defaultValue);
                if (item.onValueChanged)
                {
                    item.onValueChanged(item, item.defaultValue);
                    logDebugP("OnValueChanged called for key: %s", item.key.c_str());
                }
                else
                {
                    logErrorP("No OnValueChanged handler for key: %s", item.key.c_str());
                }
            }
            break;
        }
        case MenuConfig::MenuElementType::TextInput:
            // Not implemented in this example
            logDebugP("TextInput not implemented");
            break;
        default:
            logErrorP("Unknown menu item type");
            break;
    }
}

void MenuWidget::clearDisplay()
{
    if (!_display || !_display->display) return;
    _display->display->clearDisplay();
    _display->displayBuff();
}

void MenuWidget::drawMenu()
{
    if (!_display || !_display->display) return;

    _display->display->clearDisplay();
    _display->display->setTextSize(1); // Normal 1:1 pixel scale
    _display->display->setTextWrap(false);

    if (_infoOverlayActive && _overlayDrawFunction)
    {
        _overlayDrawFunction(); // We draw the info overlay
        return;                 // Skip the rest of the menu drawing
    }

    const uint8_t ITEM_HEIGHT = 10;
    const uint8_t ITEM_MARGIN = 2;
    const uint8_t MAX_VISIBLE_ITEMS = _screenHeight / (ITEM_HEIGHT + ITEM_MARGIN);
    const uint8_t startIdx = (_selectedIndex >= MAX_VISIBLE_ITEMS) ? _selectedIndex - (MAX_VISIBLE_ITEMS - 1) : 0;

    for (size_t i = startIdx; i < _currentMenu.size() && i < startIdx + MAX_VISIBLE_ITEMS; ++i)
    {
        const uint8_t yPos = (i - startIdx) * (ITEM_HEIGHT + ITEM_MARGIN);
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

        // Draw label
        _display->display->setCursor(2, yPos + 2);
        _display->display->print(item.label.c_str());

        // Draw item-specific content
        switch (item.type)
        {
            case MenuConfig::MenuElementType::Checkbox:
                _display->display->setCursor(_screenWidth - 12, yPos + 2);
                _display->display->print(item.defaultValue.getBool() ? "ON" : "OFF");
                break;

            case MenuConfig::MenuElementType::Dropdown:
                if (size_t idx = item.defaultValue.getSizeT(); idx < item.dropdownOptions.size())
                {
                    _display->display->setCursor(_screenWidth - 12, yPos + 2);
                    _display->display->print(item.dropdownOptions[idx].c_str());
                }
                break;

            case MenuConfig::MenuElementType::Submenu:
                _display->display->setCursor(_screenWidth - 12, yPos + 2);
                _display->display->print(">");
                break;

            case MenuConfig::MenuElementType::Back:
                _display->display->setCursor(_screenWidth - 12, yPos + 2);
                _display->display->print("<");
                break;
        }
    }
    _display->displayBuff();
}
#endif // DEVICE_DISPLAY_MODULE