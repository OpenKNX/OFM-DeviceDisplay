#pragma once
#include "../Widget.h"

struct MenuOption
{
    std::string label;   // ToDo - Submenu / Checkboxes / Dropdowns / Sliders / Buttons / Value etc.
    std::function<void()> action;
    bool isSubmenu;               
    std::vector<MenuOption> submenu;
    uint8_t type;
    void *value; 
};

// MenuWidget-Klasse
class MenuWidget : public Widget
{
  public:
    const std::string logPrefix() { return "MenuWidget"; }                                                              // Log-Präfix for the widget
    MenuWidget(uint32_t displayTime, WidgetsAction action, uint8_t buttonUp, uint8_t buttonDown, uint8_t buttonSelect); // Constructor

    void setup() override;  // Setup the widget
    void start() override;  // Start the widget
    void stop() override;   // Stop the widget
    void pause() override;  // Pause the widget
    void resume() override; // Resume the widget
    void loop() override;   // Loop the widget

    uint32_t getDisplayTime() const override; // Return the display time in ms
    WidgetsAction getAction() const override; // Return the widget action

    void setDisplayModule(i2cDisplay *displayModule) override; // Set the display module
    i2cDisplay *getDisplayModule() const override;             // Get the display module

    void addCustomMenu(const std::vector<MenuOption> &menu); // Add a custom menu. for external use (e.g. from the main or other libraries)

    void externalNavigateUp();   // External navigation up
    void externalNavigateDown(); // External navigation down
    void externalSelectItem();   // External select item
    void externalPause();        // External pause
    void externalResume();       // External resume
    void externalStop();         // External stop

  private:
    void navigateUp();      // Navigate up
    void navigateDown();    // Navigate down
    void selectItem();      // Select the current menu item
    void addDefaultMenus(); // Add default menus
    void drawMenu();        // Draw the menu

    uint32_t _displayTime; // Time to display the widget in ms
    WidgetsAction _action; // Widget action

    pin_size_t _buttonUp, _buttonDown, _buttonSelect; // Button pins ToDo: Assign the buttons to the pins!
    uint16_t _screenWidth, _screenHeight;             // Screen dimensions
    i2cDisplay *_display;                             // Display module

    std::vector<MenuOption> _currentMenu;            // Menu items
    std::vector<std::vector<MenuOption>> _menuStack; // Menu stack

    size_t _selectedIndex; // Selected menu index
    bool _needsRedraw;     // Redraw flag
    bool _isPaused;        // Pause flag
};