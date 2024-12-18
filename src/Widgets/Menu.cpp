#include "Menu.h"
//#include "DeviceDisplay.h"
#include "openknx.h"

// Constructor
MenuWidget::MenuWidget(uint32_t displayTime, WidgetsAction action, pin_size_t buttonUp, pin_size_t buttonDown, pin_size_t buttonSelect)
    : _displayTime(displayTime), _action(action), _buttonUp(buttonUp), _buttonDown(buttonDown), _buttonSelect(buttonSelect),
      _selectedIndex(0), _needsRedraw(true), _isPaused(false), _state(WidgetState::STOPPED), _display(nullptr)
{
    // pinMode(buttonUp, INPUT_PULLUP);
    // pinMode(buttonDown, INPUT_PULLUP);
    // pinMode(buttonSelect, INPUT_PULLUP);
}

uint32_t MenuWidget::getDisplayTime() const { return 0; }
WidgetsAction MenuWidget::getAction() const { return NoAction; }

void MenuWidget::setDisplayModule(i2cDisplay *displayModule)
{
    _display = displayModule;

    logInfoP("Set display Module...");
    if (_display == nullptr)
    {
        //logErrorP("Display ist NULL.");
        return;
    }
}
i2cDisplay *MenuWidget::getDisplayModule() const { return _display; }

void MenuWidget::externalNavigateUp()
{
    logInfoP("External navigate up...");
    navigateUp();
}

void MenuWidget::externalNavigateDown()
{
    logInfoP("External navigate down...");
    navigateDown();
}

void MenuWidget::externalSelectItem()
{
    logInfoP("External select item...");
    selectItem();
}
void MenuWidget::externalPause()
{
    logInfoP("External pause...");
    pause();
}

void MenuWidget::externalResume()
{
    logInfoP("External resume...");
    resume();
}

void MenuWidget::externalStop()
{
    logInfoP("External stop...");
    stop();
}

// Setup
void MenuWidget::setup()
{
    logInfoP("Setup...");
    if (_display == nullptr)
    {
        //logErrorP("MenuWidget: Display ist NULL.");
        return;
    }
    _screenHeight = _display->GetDisplayHeight();
    _screenWidth = _display->GetDisplayWidth();
    addDefaultMenus();
}

// Start
void MenuWidget::start()
{
    _state = WidgetState::RUNNING;
    logInfoP("Start...");
    _needsRedraw = true;
}

// Stop
void MenuWidget::stop()
{
    logInfoP("Stop...");
    _state = WidgetState::STOPPED;
    _needsRedraw = true;
}

// Pause
void MenuWidget::pause()
{
    logInfoP("Pause...");
    _state = WidgetState::PAUSED;
}

// Resume
void MenuWidget::resume()
{
    logInfoP("Resume...");
    _state = WidgetState::RUNNING;
    _needsRedraw = true;
}

// Loop
void MenuWidget::loop()
{
    if (_state != WidgetState::RUNNING) return;

    // Button-Handling
    /*
    if (!digitalRead(_buttonUp))
    {
        navigateUp();
        delay(150); // Debouncing
    }
    if (!digitalRead(_buttonDown))
    {
        navigateDown();
        delay(150); // Debouncing
    }
    if (!digitalRead(_buttonSelect))
    {
        selectItem();
        delay(150); // Debouncing
    }
    */

    if (_needsRedraw) // Redraw the menu if needed
    {
        logInfoP("Redraw menu...");
        drawMenu();
        _needsRedraw = false;
    }
}

// Add default menus
void MenuWidget::addDefaultMenus()
{
    logInfoP("Add default menus...");

    std::vector<MenuOption> settingsMenu(5);
    settingsMenu[0].label = "WiFi";
    settingsMenu[0].type = MenuElementType::CHECKBOX;
    settingsMenu[0].checkboxState = false;
    
    settingsMenu[1].label = "Bluetooth";
    settingsMenu[1].type = MenuElementType::CHECKBOX;
    settingsMenu[1].checkboxState = true;

    settingsMenu[2].label = "Brightness";
    settingsMenu[2].type = MenuElementType::DROPDOWN;
    settingsMenu[2].selectedOptionIndex = 0;
    settingsMenu[2].dropdownOptions = {"25%", "50%", "75%", "100%"};

    settingsMenu[3].label = "Language";
    settingsMenu[3].type = MenuElementType::DROPDOWN;
    settingsMenu[3].selectedOptionIndex = 0;
    settingsMenu[3].dropdownOptions = {"English", "Deutsch", "Français"};

    settingsMenu[4].label = "Back";
    settingsMenu[4].type = MenuElementType::BACK;

    _currentMenu.resize(6);

    _currentMenu[0].label = "Reboot";
    _currentMenu[0].type = MenuElementType::ACTION;
    _currentMenu[0].action = []() { 
      /* Reboot the system */
      openknx.restart();
    };
    _currentMenu[0].isSubmenu = false;

    _currentMenu[1].label = "Matrix";
    _currentMenu[1].type = MenuElementType::ACTION;
    _currentMenu[1].action = []() {
      /* Matrix Screensaver */
      //WidgetMatrixClassic* matrixClassicWidget = new WidgetMatrixClassic(5000, WidgetsAction::AutoRemoveFlag, 8);
      //openknxDisplayModule.widgetManager.addWidget(matrixClassicWidget);
      //matrixClassicWidget->start();
      //Widget *matrixClassicWidget = openknxDisplayModule.widgetManager.getWidgetFromQueue("MatrixClassic");
      //if(matrixClassicWidget != nullptr && matrixClassicWidget->getState() == WidgetState::PAUSED) {
      //  matrixClassicWidget->resume();
      //} else {
      //  //logErrorP("MatrixClassic widget not found or not paused.");
      //}
      
    };
    _currentMenu[1].isSubmenu = false;

    _currentMenu[2].label = "Enable Screensaver";
    _currentMenu[2].type = MenuElementType::CHECKBOX;
    _currentMenu[2].checkboxState = true;
    _currentMenu[2].isSubmenu = false;

    _currentMenu[3].label = "Uptime";
    _currentMenu[3].type = MenuElementType::ACTION;
    _currentMenu[3].action = []() { /* Uptime-Anzeige */ };
    _currentMenu[3].isSubmenu = false;

    _currentMenu[4].label = "Settings";
    _currentMenu[4].type = MenuElementType::SUBMENU;
    _currentMenu[4].isSubmenu = true;
    _currentMenu[4].submenu = settingsMenu;

    _currentMenu[5].label = "Exit";
    _currentMenu[5].type = MenuElementType::ACTION;
    _currentMenu[5].action = []() { /* Exit-Aktion */ };
    _currentMenu[5].isSubmenu = false;
}

// Add custom menu
void MenuWidget::addCustomMenu(const std::vector<MenuOption> &menu)
{
    logInfoP("Add custom menu...");
    _currentMenu.insert(_currentMenu.end(), menu.begin(), menu.end());
}

// Navigate up
void MenuWidget::navigateUp()
{
    logInfoP("Navigate up...");
    logInfoP("Selected Index: %d", _selectedIndex);
    logInfoP("Current Menu Size: %d", _currentMenu.size());
    if (_selectedIndex > 0)
    {
        _selectedIndex--;
        logInfoP("Selected Index New: %d", _selectedIndex);
        _needsRedraw = true;
    }
}

// Navigate down
void MenuWidget::navigateDown()
{
    logInfoP("Navigate down...");
    logInfoP("Selected Index: %d", _selectedIndex);
    logInfoP("Current Menu Size: %d", _currentMenu.size());
    if (_selectedIndex < _currentMenu.size() - 1)
    {
        _selectedIndex++;
        _needsRedraw = true;
        logInfoP("Selected Index New: %d", _selectedIndex);
    }
}

void MenuWidget::selectItem()
{
    auto &item = _currentMenu[_selectedIndex];
    switch (item.type)
    {
        case MenuElementType::ACTION:
            if (item.action)
            {
                item.action(); // Do the action of the menu item
            }
            break;
        case MenuElementType::SUBMENU:
            if (item.submenu.size() > 0)
            {
                _menuStack.push_back(_currentMenu); // Store the current menu on the stack
                _currentMenu = item.submenu;        // Set the submenu as the current menu
                _selectedIndex = 0;
            }
            break;
        case MenuElementType::BACK:
            if (!_menuStack.empty())
            {
                _currentMenu = _menuStack.back(); // Get the last menu from the stack
                _menuStack.pop_back();
                _selectedIndex = 0;
            }
            break;
        case MenuElementType::CHECKBOX:
            item.checkboxState = !item.checkboxState; // Toggle the state of the checkbox
            break;
        case MenuElementType::DROPDOWN:
            item.selectedOptionIndex = (item.selectedOptionIndex + 1) % item.dropdownOptions.size(); // Switch to the next option in the dropdown
            break;
        default:
            break;
    }
    _needsRedraw = true;
}

// Draw the menu
void MenuWidget::drawMenu()
{
    if (!_display || !_display->display) return;

    logInfoP("Draw menu...");

    _display->display->clearDisplay();

    const uint8_t ITEM_HEIGHT = 10;
    const uint8_t ITEM_MARGIN = 2;
    const uint8_t MAX_VISIBLE_ITEMS = _screenHeight / (ITEM_HEIGHT + ITEM_MARGIN);
    uint8_t startIdx = (_selectedIndex >= MAX_VISIBLE_ITEMS)
                           ? _selectedIndex - (MAX_VISIBLE_ITEMS - 1)
                           : 0;

    logInfoP("Menu item: %d", _currentMenu.size());
    for (size_t i = startIdx; i < _currentMenu.size() && i < startIdx + MAX_VISIBLE_ITEMS; ++i)
    {
        uint8_t y = (i - startIdx) * (ITEM_HEIGHT + ITEM_MARGIN);
        if (i == _selectedIndex)
        {
            _display->display->fillRect(0, y, _screenWidth, ITEM_HEIGHT, WHITE);
            _display->display->setTextColor(BLACK, WHITE);
        }
        else
        {
            _display->display->setTextColor(WHITE, BLACK);
        }

        _display->display->setCursor(2, y + 2);
        _display->display->print(_currentMenu[i].label.c_str());
        logInfoP("Menu item: %s", _currentMenu[i].label.c_str());

        // Wenn es sich um eine Checkbox handelt, zeige den Zustand
        if (_currentMenu[i].type == MenuElementType::CHECKBOX)
        {
            _display->display->setCursor(90, y + 2);
            _display->display->print(_currentMenu[i].checkboxState ? "ON" : "OFF");
        }

        // Wenn es sich um ein Dropdown handelt, zeige die aktuelle Auswahl
        if (_currentMenu[i].type == MenuElementType::DROPDOWN)
        {
            _display->display->setCursor(90, y + 2);
            _display->display->print(_currentMenu[i].dropdownOptions[_currentMenu[i].selectedOptionIndex].c_str());
        }
    }

    _display->displayBuff();
    logInfoP("Menu drawn.");
}