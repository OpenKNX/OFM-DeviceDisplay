#include "Menu.h"

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
    if (displayModule == nullptr) {
        //logErrorP("MenuWidget: Display ist NULL.");
        openknx.logger.log("MenuWidget: Display ist NULL.");
    } return;

    //logDebugP("WidgetPong: Display-Modul gesetzt.");
    openknx.logger.log("WidgetPong: Display-Modul gesetzt.");
}
i2cDisplay *MenuWidget::getDisplayModule() const { return _display; }

void MenuWidget::externalNavigateUp()
{
    navigateUp();
    _needsRedraw = true; // Menü neu zeichnen
}

void MenuWidget::externalNavigateDown()
{
    navigateDown();
    _needsRedraw = true; // Menü neu zeichnen
}

void MenuWidget::externalSelectItem()
{
    selectItem();
    _needsRedraw = true; // Menü neu zeichnen
}
void MenuWidget::externalPause()
{
    pause();
    _needsRedraw = true; // Eventuell visueller Hinweis beim Pausieren
}

void MenuWidget::externalResume()
{
    resume();
    _needsRedraw = true; // Menü nach dem Fortsetzen neu zeichnen
}

void MenuWidget::externalStop()
{
    stop();
    _needsRedraw = true; // Menü nach dem Stoppen leeren oder resetten
}

// Setup
void MenuWidget::setup()
{
    logInfoP("MenuWidget: Setup...");
    if(_display == nullptr) {
        //logErrorP("MenuWidget: Display ist NULL.");
        openknx.logger.log("MenuWidget: Display ist NULL.");
        return;
    }
    _screenHeight = _display->GetDisplayHeight();
    _screenWidth = _display->GetDisplayWidth();
    addDefaultMenus();
}

// Start
void MenuWidget::start()
{
    _isPaused = false;
    _needsRedraw = true;
}

// Stop
void MenuWidget::stop()
{
    _isPaused = true;
    _needsRedraw = true;
}

// Pause
void MenuWidget::pause()
{
    _isPaused = true;
}

// Resume
void MenuWidget::resume()
{
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
        drawMenu();
        _needsRedraw = false;
    }
}

// Add default menus
void MenuWidget::addDefaultMenus()
{
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
    _currentMenu.insert(_currentMenu.end(), menu.begin(), menu.end());
}

// Navigate up
void MenuWidget::navigateUp()
{
    if (_selectedIndex > 0)
    {
        _selectedIndex--;
        _needsRedraw = true;
    }
}

// Navigate down
void MenuWidget::navigateDown()
{
    if (_selectedIndex < _currentMenu.size() - 1)
    {
        _selectedIndex++;
        _needsRedraw = true;
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
    logDebugP("MenuWidget: Draw menu...");

    _display->display->clearDisplay();

    const uint8_t ITEM_HEIGHT = 10;
    const uint8_t ITEM_MARGIN = 2;
    const uint8_t MAX_VISIBLE_ITEMS = _screenHeight / (ITEM_HEIGHT + ITEM_MARGIN);
    uint8_t startIdx = (_selectedIndex >= MAX_VISIBLE_ITEMS)
                           ? _selectedIndex - (MAX_VISIBLE_ITEMS - 1)
                           : 0;

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
    }

    _display->displayBuff();
    logDebugP("MenuWidget: Menu drawn.");
}