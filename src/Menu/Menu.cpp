#include "Menu.h"
#include "GPIO_PCA9557.h"
#include "MenuConfig_json.h"
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

    if (!openknxGPIOModule.initialized(1))
    {
        logErrorP("GPIO Module not initialized");
    }
    else
    {
        _FrontPlateEnabled = true;
        logInfoP("GPIO Module initialized");
        // Initialize buttons
        const uint16_t pins[] = {_buttonUp, _buttonDown, _buttonSelect, _buttonLeft, _buttonRight, 0x0103};
        for (auto pin : pins)
        {
            openknxGPIOModule.pinMode(pin, INPUT, true, 0);
        }

        // Initialize LED outputs
        const struct
        {
            uint16_t pin;
            bool state;
        } outputs[] = {
            {0x0101, HIGH},  // Prog LED
            {0x0102, HIGH}}; // Info LED

        for (const auto& out : outputs)
        {
            openknxGPIOModule.pinMode(out.pin, OUTPUT, false, 0);
            openknxGPIOModule.digitalWrite(out.pin, out.state);
        }
    }
    _state = WidgetState::BACKGROUND; // Start Menu in background! Will be started by button press.
}

void MenuWidget::addDefaultMenus()
{
    _menuConfig.loadDefaultMenu(defaultMenuJson);
    _currentMenu = _menuConfig.getMenu();
    if (!_currentMenu.empty())
    {
        _selectedIndex = 0;
        _needsRedraw = true;
        logInfoP("Default menus loaded successfully");
    }
    else
    {
        // logErrorP("Failed to load default menus");
    }
}

bool MenuWidget::readButton(uint16_t pin)
{
    //openknxGPIOModule.pinMode(pin, OUTPUT);
    //openknxGPIOModule.digitalWrite(pin, HIGH);
    //openknxGPIOModule.pinMode(pin, INPUT);
    //bool state = !openknxGPIOModule.digitalRead(pin);
    //openknxGPIOModule.digitalWrite(pin, HIGH);
    //return !state;
    return openknxGPIOModule.digitalRead(pin);
}

void MenuWidget::loop()
{
    uint32_t currentTime = millis();
    if (_state != WidgetState::RUNNING && _state != WidgetState::BACKGROUND) return;

    if (_lastButtonPressTime > 0 && (currentTime - _lastButtonPressTime) < _displayTime)
    {
        if (_state != WidgetState::RUNNING)
        {
            logInfoP("Menu is running due to button press.");
            start();
        }
    }
    else if ((currentTime - _lastButtonPressTime) >= _displayTime)
    {
        if (_state == WidgetState::RUNNING && _state != WidgetState::BACKGROUND)
        {
            background();
            return;
        }
    }

    if (currentTime - _lastButtonCheck >= BUTTON_CHECK_INTERVAL)
    {
        _lastButtonCheck = currentTime;

        if (_FrontPlateEnabled && readButton(_buttonUp)) { 
          navigateUp();
          openknxGPIOModule.digitalWrite(0x0101, LOW); // Prog LED
        }
        if (_FrontPlateEnabled && readButton(_buttonDown)) { 
          navigateDown();
          openknxGPIOModule.digitalWrite(0x0102, LOW); // Info LED
        }
        if (_FrontPlateEnabled && readButton(_buttonSelect)) selectItem();
        if (_FrontPlateEnabled && !readButton(_buttonLeft)) {
          navigateLeft();
          openknxGPIOModule.digitalWrite(0x0101, HIGH); // Prog LED
        }
        if (_FrontPlateEnabled && readButton(_buttonRight)) { 
          navigateRight();
          openknxGPIOModule.digitalWrite(0x0102, HIGH); // Info LED
        }
    }

    if (_state != WidgetState::RUNNING) return; // Skip the rest if not running
    if (_needsRedraw /*|| (currentTime - _lastRedrawTime >= REDRAW_INTERVAL)*/)
    {
        drawMenu();
        _needsRedraw = false;
        _lastRedrawTime = currentTime;
    }
}

// State management methods
void MenuWidget::start()
{
    logInfoP("Start...");
    _stateLast = _state;
    _state = WidgetState::RUNNING;
    _needsRedraw = true;

    addAction(WidgetFlags::DisplayEnabled);
    removeAction(WidgetFlags::Background);
}

void MenuWidget::stop()
{
    _stateLast = _state;
    _state = WidgetState::STOPPED;
    clearDisplay();
    logInfoP("Stop...");
}

void MenuWidget::pause()
{
    _stateLast = _state;
    _state = WidgetState::PAUSED;
    logInfoP("Pause...");
}

void MenuWidget::resume()
{
    _state = _stateLast;
    _needsRedraw = true;
    logInfoP("Resume...");
}

void MenuWidget::background()
{
    _stateLast = _state;
    _state = WidgetState::BACKGROUND;
    _needsRedraw = false;

    addAction(WidgetFlags::Background);
    removeAction(WidgetFlags::DisplayEnabled);
    clearDisplay();
    logInfoP("Menu is running in background due to inactivity.");
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
    logInfoP("Navigate up");
    _lastButtonPressTime = millis();
    if (_selectedIndex > 0)
    {
        --_selectedIndex;
        _needsRedraw = true;
        logInfoP("Navigated up to index %d", _selectedIndex);
    }
}

void MenuWidget::navigateDown()
{
    logInfoP("Navigate down");
    _lastButtonPressTime = millis();
    if (_selectedIndex < _currentMenu.size() - 1)
    {
        ++_selectedIndex;
        _needsRedraw = true;
        logInfoP("Navigated down to index %d", _selectedIndex);
    }
}

void MenuWidget::navigateLeft()
{
    logInfoP("Navigate left");
    _lastButtonPressTime = millis();
    if (_selectedIndex > 0)
    {
        //--_selectedIndex;
        //_needsRedraw = true;
        //logInfoP("Navigated left to index %d", _selectedIndex);
    }
}

void MenuWidget::navigateRight()
{
    logInfoP("Navigate right");
    _lastButtonPressTime = millis();
    if (_selectedIndex < _currentMenu.size() - 1)
    {
        //++_selectedIndex;
        //_needsRedraw = true;
        //logInfoP("Navigated right to index %d", _selectedIndex);
    }
}

void MenuWidget::selectItem()
{
    _lastButtonPressTime = millis();

    if (_currentMenu.empty()) return;

    logInfoP("Select item");
    auto& item = _currentMenu[_selectedIndex];
    switch (item.type)
    {
        case MenuConfig::MenuElementType::Action:
            if (item.action) item.action();
            break;

        case MenuConfig::MenuElementType::Submenu:
            if (!item.submenu.empty())
            {
                _menuStack.push_back(std::move(_currentMenu));
                _currentMenu = std::move(item.submenu);
                _selectedIndex = 0;
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
            }
            break;

        case MenuConfig::MenuElementType::Dropdown:
        {
            size_t nextIndex = (item.defaultValue.getSizeT() + 1) % item.dropdownOptions.size();
            item.defaultValue = MenuValue(nextIndex);
            if (!item.key.empty())
            {
                _menuConfig.setValue(item.key, item.defaultValue);
            }
            break;
        }
    }

    _needsRedraw = true;
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