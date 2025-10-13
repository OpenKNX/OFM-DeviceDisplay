#ifdef DEVICE_DISPLAY_MODULE
    #pragma once
    #include "../Widget.h"
    #include "MenuConfig.h"

    class MenuWidget : public Widget
{
  public:
    // Constructor
    MenuWidget(uint32_t displayTime, WidgetFlags action);

    // Widget interface implementation
    void start() override;
    void stop() override;
    void pause() override;
    void resume() override;
    void setup() override;
    void loop() override;
    void background() override;

    // Getters & Setters
    inline const WidgetState getState() const override { return _state; }
    inline const std::string getName() const override { return _name; }
    inline void setName(const std::string& name) override { _name = name; }
    uint32_t getDisplayTime() const override;
    WidgetFlags getAction() const override;
    inline void setDisplayTime(uint32_t displayTime) override { _displayTime = displayTime; }

    inline void setAction(uint8_t action) override { _action = static_cast<WidgetFlags>(action); }               // Set the widget action
    inline void addAction(uint8_t action) override { _action = static_cast<WidgetFlags>(_action | action); }     // Add an action to the widget
    inline void removeAction(uint8_t action) override { _action = static_cast<WidgetFlags>(_action & ~action); } // Remove an action from the widget

    i2cDisplay* getDisplayModule() const override;
    void setDisplayModule(i2cDisplay* displayModule) override;

    // External navigation methods
    void externalNavigateUp();
    void externalNavigateDown();
    void externalSelectItem();
    void externalPause();
    void externalResume();
    void externalStop();
    // Front plate control
 
    bool setLED(uint16_t pin, bool state);

    // Logger prefix
    const std::string logPrefix() { return "MenuWidget"; }

    // Action registration
    void registerAction(const std::string& key, std::function<void()> action);
    void registerOnValueChanged(const std::string& key, std::function<void(const MenuConfig::MenuOption&, const MenuValue&)> callback);

    bool handleButtonEvent(const ButtonEvent& event) override;

  private:
    // UI Constants
    static constexpr uint8_t ITEM_HEIGHT = 10;
    static constexpr uint8_t ITEM_MARGIN = 2;
    static constexpr uint16_t BUTTON_CHECK_INTERVAL = 250;
    static constexpr uint16_t REDRAW_INTERVAL = 1000;

    // Core functionality
    void navigateUp();
    void navigateDown();
    void selectItem();
    void navigateLeft();
    void navigateRight();

    void addDefaultMenus();
    void addDefaultActions();
    void addDefaultOnValueChanged();

    void clearDisplay();
    void drawMenu();


    // Display properties
    i2cDisplay* _display = nullptr;
    uint16_t _screenWidth = 0;
    uint16_t _screenHeight = 0;


    uint32_t _lastRedrawTime = 0;
    uint32_t _lastButtonPressTime = 0;

    // Widget properties
    uint32_t _displayTime;
    WidgetFlags _action;
    std::string _name = "Menu";
    WidgetState _state = WidgetState::STOPPED;
    WidgetState _stateLast = WidgetState::STOPPED;

    // Menu
    std::vector<MenuConfig::MenuOption> _currentMenu;
    std::vector<std::vector<MenuConfig::MenuOption>> _menuStack;
    MenuConfig _menuConfig;
    size_t _selectedIndex = 0;
    bool _needsRedraw = false;
    bool _isPaused = false;
    bool _FrontPlateEnabled = false;

    std::unordered_map<std::string, std::function<void()>> actionRegistry;
    std::unordered_map<std::string, std::function<void(const MenuConfig::MenuOption&, const MenuValue&)>> onValueChangedRegistry;
    void assignRegisteredActions(std::vector<MenuConfig::MenuOption>& menuOptions);
    void assignOnValueChangedHandlers(std::vector<MenuConfig::MenuOption>& menuOptions);

    void showOverlay(std::function<void()> drawFn);
    bool _infoOverlayActive = false;
    const uint32_t _infoOverlayTimeout = 500;      // Prevent overlay from being dismissed too quickly (<1 second)
    const uint32_t _infoOverlayMaxTimeout = 60000; // Prevent overlay from being active for more than 60 seconds (auto close)
    std::function<void()> _overlayDrawFunction;
};
#endif // DEVICE_DISPLAY_MODULE