#include "Menu.h"
#include "openknx.h"

// Constructor
MenuWidget::MenuWidget(uint32_t displayTime, WidgetsAction action, pin_size_t buttonUp, pin_size_t buttonDown, pin_size_t buttonSelect)
    : _displayTime(displayTime), _action(action), _buttonUp(buttonUp), _buttonDown(buttonDown), _buttonSelect(buttonSelect),
      _selectedIndex(0), _needsRedraw(true), _isPaused(false)
{
    //pinMode(buttonUp, INPUT_PULLUP);
    //pinMode(buttonDown, INPUT_PULLUP);
    //pinMode(buttonSelect, INPUT_PULLUP);
}

uint32_t MenuWidget::getDisplayTime() const { return 0; }
WidgetsAction MenuWidget::getAction() const { return NoAction; }

void MenuWidget::setDisplayModule(i2cDisplay *displayModule)
{
    _display = displayModule;

    logInfoP("Set display Module...");
    if (_display == nullptr)
    {
        logErrorP("Display ist NULL.");
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
    if(_display == nullptr) {
        logErrorP("MenuWidget: Display ist NULL.");
        return;
    }
    _screenHeight = _display->GetDisplayHeight();
    _screenWidth = _display->GetDisplayWidth();
    addDefaultMenus();
}

// Start
void MenuWidget::start()
{   
    logInfoP("Start...");
    _isPaused = false;
    _needsRedraw = true;
}

// Stop
void MenuWidget::stop()
{
    logInfoP("Stop...");
    _isPaused = true;
    _needsRedraw = true;
}

// Pause
void MenuWidget::pause()
{
    logInfoP("Pause...");
    _isPaused = true;
}

// Resume
void MenuWidget::resume()
{
    logInfoP("Resume...");
    _isPaused = false;
    _needsRedraw = true;
}

// Loop
void MenuWidget::loop()
{
    if (_isPaused) return;

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
    _currentMenu = {
        {"Reboot", []() { /* Reboot-Aktion */ }, false, {}, 0, nullptr},
        {"Brightness", nullptr, false, {}, 0, nullptr},
        {"Enable Screensaver", nullptr, false, {}, 0, nullptr},
        {"Uptime", []() { /* Uptime-Anzeige */ }, false, {}, 0, nullptr},
        {"Back", [this]() {
             if (!_menuStack.empty())
             {
                 _currentMenu = _menuStack.back();
                 _menuStack.pop_back();
                 _selectedIndex = 0;
             }
         },
         false,
         {},
         0,
         nullptr}};
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

// Select the current menu item
void MenuWidget::selectItem()
{
    auto &item = _currentMenu[_selectedIndex];
    if (item.isSubmenu)
    {
        _menuStack.push_back(_currentMenu);
        _currentMenu = item.submenu;
        _selectedIndex = 0;
    }
    else if (item.action)
    {
        item.action();
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
    }

    _display->displayBuff();
    logInfoP("Menu drawn.");
}