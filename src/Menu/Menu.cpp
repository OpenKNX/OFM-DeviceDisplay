#include "Menu.h"
// #include "DeviceDisplay.h"
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
i2cDisplay *MenuWidget::getDisplayModule() const { return _display; }

void MenuWidget::addDefaultMenus() // Add default menus
{
    // Öffne die Datei MenuConfig.json und lade das Standardmenü
    const char *defaultMenuJson = R"({
  "menu": [
    {
      "label": "Main Settings",
      "type": "Submenu",
      "submenu": [
        {
          "label": "Network Settings",
          "type": "Submenu",
          "submenu": [
            {
              "label": "DHCP",
              "type": "Checkbox",
              "defaultValue": true
            },
            {
              "label": "IP Address",
              "type": "Action",
              "isVisible": false
            },
            {
              "label": "Subnet Mask",
              "type": "Action",
              "isVisible": true
            },
            {
              "label": "DNS Settings",
              "type": "Submenu",
              "submenu": [
                {
                  "label": "Primary DNS",
                  "type": "Action"
                },
                {
                  "label": "Secondary DNS",
                  "type": "Action"
                },
                {
                  "label": "Back",
                  "type": "Back"
                }
              ]
            },
            {
              "label": "Back",
              "type": "Back"
            }
          ]
        },
        {
          "label": "System Settings",
          "type": "Submenu",
          "submenu": [
            {
              "label": "Time Zone",
              "type": "Dropdown",
              "defaultValue": 2,
              "options": ["UTC-12", "UTC-11", "UTC+0", "UTC+1", "UTC+2"]
            },
            {
              "label": "Date/Time",
              "type": "Action"
            },
            {
              "label": "Reboot Device",
              "type": "Action"
            },
            {
              "label": "Back",
              "type": "Back"
            }
          ]
        },
        {
          "label": "Device Info",
          "type": "Submenu",
          "submenu": [
            {
              "label": "Device Name",
              "type": "Action"
            },
            {
              "label": "Firmware Version",
              "type": "Action"
            },
            {
              "label": "Serial Number",
              "type": "Action"
            },
            {
              "label": "Back",
              "type": "Back"
            }
          ]
        },
        {
          "label": "Back",
          "type": "Back"
        }
      ]
    },
    {
      "label": "Advanced Settings",
      "type": "Submenu",
      "submenu": [
        {
          "label": "Security",
          "type": "Submenu",
          "submenu": [
            {
              "label": "Password",
              "type": "TextInput",
              "defaultValue": "defaultpassword"
            },
            {
              "label": "Enable Encryption",
              "type": "Checkbox",
              "defaultValue": false
            },
            {
              "label": "Back",
              "type": "Back"
            }
          ]
        },
        {
          "label": "Back",
          "type": "Back"
        }
      ]
    }, 
    {
      "label": "Brightness",
      "type": "Dropdown",
      "defaultValue": 2,
      "options": ["25%", "50%", "75%", "100%"]
    }, 
    {
      "label": "Auto Dimming",
      "type": "Checkbox",
      "defaultValue": true
    },
    {
      "label": "Screen Saver",
      "type": "Action"
    }
  ]
})";

    MenuConfig *menuConfig = new MenuConfig();
    menuConfig->loadDefaultMenu(defaultMenuJson);
    _currentMenu = menuConfig->getMenu();

    // Load the default menu from a JS
}

void MenuWidget::setDisplayModule(i2cDisplay *displayModule) // Set the display module
{
    _display = displayModule;
    logInfoP("Set display Module...");
    if (_display == nullptr)
    {
        logErrorP("Display ist NULL.");
        return;
    }
}

void MenuWidget::externalNavigateUp() // External navigation up
{
    logInfoP("External navigate up...");
    navigateUp();
}
void MenuWidget::externalNavigateDown() // External navigation down
{
    logInfoP("External navigate down...");
    navigateDown();
}
void MenuWidget::externalSelectItem() // External select item
{
    logInfoP("External select item...");
    selectItem();
}
void MenuWidget::externalPause() // External pause
{
    logInfoP("External pause...");
    pause();
}
void MenuWidget::externalResume() // External resume
{
    logInfoP("External resume...");
    resume();
}
void MenuWidget::externalStop() // External stop
{
    logInfoP("External stop...");
    stop();
}

void MenuWidget::setup() // Setup
{
    logInfoP("Setup...");
    if (_display == nullptr)
    {
        logErrorP("MenuWidget: Display ist NULL.");
        return;
    }
    _screenHeight = _display->GetDisplayHeight();
    _screenWidth = _display->GetDisplayWidth();
    addDefaultMenus();
}
void MenuWidget::start() // Start
{
    _state = WidgetState::RUNNING;
    logInfoP("Start...");
    _needsRedraw = true;
}
void MenuWidget::stop() // Stop
{
    logInfoP("Stop...");
    _state = WidgetState::STOPPED;
    _needsRedraw = true;
}
void MenuWidget::pause() // Pause
{
    logInfoP("Pause...");
    _state = WidgetState::PAUSED;
}
void MenuWidget::resume() // Resume
{
    logInfoP("Resume...");
    _state = WidgetState::RUNNING;
    _needsRedraw = true;
}
void MenuWidget::loop() // Loop
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

void MenuWidget::navigateUp() // Navigate up
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
void MenuWidget::navigateDown() // Navigate down
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
void MenuWidget::selectItem() // Select the current menu item
{
    auto &item = _currentMenu[_selectedIndex];
    logInfoP("Select item: %s", item.label.c_str());
    logInfoP("Item Type: %d", static_cast<int>(item.type));
    switch (item.type)
    {
        case MenuConfig::MenuElementType::Action:
        {
            logInfoP("Action...");
            if (item.action)
            {
                item.action(); // Do the action of the menu item
            }
        }
        break;
        case MenuConfig::MenuElementType::Submenu:
        {
            logInfoP("Submenu...");
            logInfoP("Submenu size: %d", item.submenu.size());
            if (!item.submenu.empty())
            {
                _menuStack.push_back(_currentMenu); // Store the current menu on the stack
                _currentMenu = item.submenu;        // Set the submenu as the current menu
                _selectedIndex = 0;
                logInfoP("Entering submenu...");
            }
            else
            {
                logInfoP("Submenu is empty!");
            }
        }
        break;
        case MenuConfig::MenuElementType::Back:
        {
            logInfoP("Back...");
            if (!_menuStack.empty())
            {
                _currentMenu = _menuStack.back(); // Get the last menu from the stack
                _menuStack.pop_back();
                _selectedIndex = 0;
            }
        }
        break;
        case MenuConfig::MenuElementType::Checkbox:
        {
            logInfoP("Checkbox...");
            item.defaultValue = !item.defaultValue.getBool();
        }
        break;
        case MenuConfig::MenuElementType::Dropdown:
        {
            logInfoP("Dropdown...");
            size_t currentIndex = item.defaultValue.getSizeT();
            item.defaultValue = (currentIndex + 1) % item.dropdownOptions.size();
        }
        break;
        case MenuConfig::MenuElementType::TextInput:
            break;
        default:
            logInfoP("Unknown...");
            break;
    }
    _needsRedraw = true;
}

void MenuWidget::drawMenu() // Draw the menu
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

        // If it is a Checkbox, display the state
        if (_currentMenu[i].type == MenuConfig::MenuElementType::Checkbox)
        {
            _display->display->setCursor(90, y + 2);
            _display->display->print(_currentMenu[i].defaultValue.getBool() ? "ON" : "OFF");
        }

        // If it is a Dropdown, display the current selection
        if (_currentMenu[i].type == MenuConfig::MenuElementType::Dropdown)
        {
            logInfoP("Dropdown Display...");
            _display->display->setCursor(90, y + 2);

            size_t selectedIndex = _currentMenu[i].defaultValue.getSizeT();
            if (selectedIndex < _currentMenu[i].dropdownOptions.size())
            {
                logInfoP("Dropdown Option: %s", _currentMenu[i].dropdownOptions[selectedIndex].c_str());
                _display->display->print(_currentMenu[i].dropdownOptions[selectedIndex].c_str());
            }
            else
            {
                logErrorP("Dropdown Index out of range: %zu", selectedIndex);
            }
        }

        // If it is a Submenu, indicate that there is a submenu
        if (_currentMenu[i].type == MenuConfig::MenuElementType::Submenu)
        {
            _display->display->setCursor(_screenWidth - 12, y + 2); // Show an arrow indicating submenus
            _display->display->print(">");
        }

        // Handle Back option
        if (_currentMenu[i].type == MenuConfig::MenuElementType::Back)
        {
            _display->display->setCursor(90, y + 2);
            _display->display->print("Back");
        }
    }

    _display->displayBuff();
    logInfoP("Menu drawn.");
}