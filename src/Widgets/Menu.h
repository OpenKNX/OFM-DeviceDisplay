#pragma once
#include "../Widget.h"

struct MenuOption {
    std::string label;
    std::function<void()> action; // Aktion für den Menüpunkt
    bool isSubmenu;               // Hat der Menüpunkt ein Untermenü?
    std::vector<MenuOption> submenu;
    uint8_t type;
    void *value;                  // Optionaler Wert (für TYPE_VALUE etc.)
};

// MenuWidget-Klasse
class MenuWidget : public Widget {
public:
    MenuWidget(uint32_t displayTime, WidgetsAction action, uint8_t buttonUp, uint8_t buttonDown, uint8_t buttonSelect);

    void setup() override;
    void start() override;
    void stop() override;
    void pause() override;
    void resume() override;
    void loop() override;

    uint32_t getDisplayTime() const override; // Return the display time in ms
    WidgetsAction getAction() const override; // Return the widget action

    void setDisplayModule(i2cDisplay *displayModule) override; // Set the display module
    i2cDisplay *getDisplayModule() const override;             // Get the display module

    void addCustomMenu(const std::vector<MenuOption> &menu);

    void externalNavigateUp();     // Externes Navigieren nach oben
    void externalNavigateDown();   // Externes Navigieren nach unten
    void externalSelectItem();     // Externes Auswählen
    void externalPause();          // Externes Pausieren
    void externalResume();         // Externes Fortsetzen
    void externalStop();           // Externes Stoppen
    
private:
    void navigateUp();
    void navigateDown();
    void selectItem();
    void addDefaultMenus();
    void drawMenu();

    uint32_t _displayTime;        // Time to display the widget in ms
    WidgetsAction _action;        // Widget action
    
    pin_size_t _buttonUp, _buttonDown, _buttonSelect;
    uint16_t _screenWidth, _screenHeight;
    i2cDisplay *_display;

    std::vector<MenuOption> _currentMenu; // Aktuelles Menü
    std::vector<std::vector<MenuOption>> _menuStack;

    size_t _selectedIndex;
    bool _needsRedraw;
    bool _isPaused;
};