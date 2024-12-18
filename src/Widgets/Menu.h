#pragma once
#include "../Widget.h"

enum class MenuElementType
{
    ACTION,
    CHECKBOX,
    DROPDOWN,
    SUBMENU,
    BACK // Neuer Typ für "Zurück"-Menüeintrag
};

struct MenuOption
{
    std::string label;                        // Name of the menu item
    std::function<void()> action;             // Function to call when the item is selected
    bool isSubmenu;                           // If true, the item is a submenu
    std::vector<MenuOption> submenu;          // Submenu items
    MenuElementType type;                     // Typ of the menu item
    void *value;                              // For Checkboxen, stores the value
    bool checkboxState;                       // For Checkboxen, stores the state
    size_t selectedOptionIndex;               // For Dropdown, stores the selected option
    std::vector<std::string> dropdownOptions; // For Dropdown, stores the options
};

// MenuWidget-Klasse
class MenuWidget : public Widget
{
  public:
    const std::string logPrefix() { return "MenuWidget"; }                                                              // Log-Präfix for the widget
    MenuWidget(uint32_t displayTime, WidgetsAction action, uint8_t buttonUp, uint8_t buttonDown, uint8_t buttonSelect); // Constructor

    void start() override;                                                  // Start widget
    void stop() override;                                                   // Stop widget
    void pause() override;                                                  // Pause widget
    void resume() override;                                                 // Resume widget
    void setup() override;                                                  // Setup widget
    void loop() override;                                                   // Update and draw the widget
    inline const WidgetState getState() const override { return _state; }   // Get the current state of the widget
    inline const std::string getName() const override { return _name; }     // Return the name of the widget
    inline void setName(const std::string &name) override { _name = name; } // Set the name of the widget

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
    std::string _name = "Menu";                       // Name of the widget
    WidgetState _state;                               // Current state of the widget

    std::vector<MenuOption> _currentMenu;            // Menu items
    std::vector<std::vector<MenuOption>> _menuStack; // Menu stack

    size_t _selectedIndex; // Selected menu index
    bool _needsRedraw;     // Redraw flag
    bool _isPaused;        // Pause flag
};