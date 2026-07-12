#ifdef DEVICE_DISPLAY_MODULE
    #pragma once
    #include "../Widget.h"
    #include "MenuConfig.h"

class MenuRegistry;

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
    // Close the menu from an external router (LEFT long-press exit); safe when already closed.
    void externalClose() { closeMenu(); }

    // Logger prefix
    const std::string logPrefix() { return "MenuWidget"; }

    // Action registration
    void registerAction(const std::string& key, std::function<void()> action);
    void registerOnValueChanged(const std::string& key, std::function<void(const MenuConfig::MenuOption&, const MenuValue&)> callback);
    // Bind a live current-index provider to a radio-list dropdown so its picker opens on the value in
    // effect (e.g. the persisted screensaver), not the static defaultValue.
    void registerRadioIndexProvider(const std::string& key, std::function<size_t()> provider);

    // Root renders as an icon grid (true) or text list (false); toggled live from the Anzeige menu.
    void setIconMenu(bool on)
    {
        if (_iconMenu != on)
        {
            _iconMenu = on;
            _needsRedraw = true;
        }
    }
    bool iconMenu() const { return _iconMenu; }

    void setMenuRegistry(MenuRegistry* registry) { _menuRegistry = registry; }

    // Screen/gesture hooks. The MenuWidget only renders + routes; the actual screens live elsewhere.
    // Until a hook is wired the corresponding select just logs + no-ops.
    //   onAboutRequested()            -> open the "Über" screen
    //   onFilesRequested(item)        -> open the SD-card file browser
    //   onGestureAction(action, item) -> start a hold-to-confirm gesture ("prog" | "reboot")
    //   onIpEditRequested(item)       -> open the 4-octet IP editor for item
    //   onReorderRequested(item)      -> open the widget reorder editor
    void setOnAboutRequested(std::function<void()> cb) { _onAboutRequested = std::move(cb); }
    void setOnFilesRequested(std::function<void(const MenuConfig::MenuOption&)> cb) { _onFilesRequested = std::move(cb); }
    void setOnGestureAction(std::function<void(const std::string&, const MenuConfig::MenuOption&)> cb) { _onGestureAction = std::move(cb); }
    void setOnIpEditRequested(std::function<void(const MenuConfig::MenuOption&)> cb) { _onIpEditRequested = std::move(cb); }
    void setOnReorderRequested(std::function<void(const MenuConfig::MenuOption&)> cb) { _onReorderRequested = std::move(cb); }

    // Developer-mode global flag. devOnly items are only reachable when dev mode is active (default
    // true: shown on the physical device). Applied in drawMenu() and navigateUp/Down().
    void setDevMode(bool on) { _devMode = on; }
    bool getDevMode() const { return _devMode; }

    bool handleButtonEvent(const ButtonEvent& event) override;

  private:
    // UI Constants
    static constexpr uint8_t ITEM_HEIGHT = 10;
    static constexpr uint8_t ITEM_MARGIN = 2;
    static constexpr uint16_t BUTTON_CHECK_INTERVAL = 250;
    static constexpr uint16_t REDRAW_INTERVAL = 1000;

    // Interactive editor sub-modes. While _mode != Normal the MenuWidget takes over button + draw
    // handling and the normal menu list is suppressed.
    enum class MenuMode : uint8_t
    {
        Normal,
        IpEdit,      // 4-octet IP editor
        Reorder,     // widget rotation reorder editor
        RadioSelect, // full-screen single-select picker (e.g. screensaver family)
        Slider       // horizontal slider over a stepped dropdown (e.g. brightness)
    };
    MenuMode _mode = MenuMode::Normal;

    // openMenu() builds a fresh tree from the registry and shows the menu; closeMenu() hides it and
    // releases the built tree (frees the per-open RAM).
    void openMenu();
    void closeMenu();

    // Apply a dynamic widget_show_/widget_dur_ setting directly; returns true if it was one.
    bool applyWidgetSetting(const MenuConfig::MenuOption& item);

    // IP editor state.
    uint8_t _ipEdit[4] = {0, 0, 0, 0}; // working copy of the octets being edited
    uint8_t _ipOctet = 0;              // active octet (0..3)
    size_t _ipEditIndex = 0;           // index into _currentMenu of the item being edited
    // ~500ms blink toggle for the active octet (shared by both editors' redraw).
    bool _editBlinkOn = true;
    uint32_t _editBlinkLast = 0;
    static constexpr uint32_t EDIT_BLINK_INTERVAL = 500;

    // IP editor sub-mode entry/handling/render.
    void enterIpEdit(size_t itemIndex); // copy item.ip -> _ipEdit, _mode = IpEdit
    void commitIpEdit();                // write _ipEdit back, fire onValueChanged, leave
    void cancelIpEdit();                // discard, leave
    bool handleIpEditButton(const ButtonEvent& event);
    void drawIpEditor();

    // Reorder editor sub-mode entry/handling/render.
    void enterReorder(); // _mode = Reorder (drops any prior grab)
    void leaveReorder(); // drop if grabbing, _mode = Normal
    bool handleReorderButton(const ButtonEvent& event);
    void drawReorder();
    size_t _reorderSel = 0; // selection cursor while not grabbing (grab index tracked by WidgetsManager)

    // Radio-select sub-mode: full-screen single-select picker for Dropdown items flagged radioList.
    // Initial highlight reflects the live value; OK commits like an in-place cycle.
    size_t _radioIndex = 0; // index into _currentMenu of the item being edited
    size_t _radioSel = 0;   // highlighted option (0..dropdownOptions.size()-1)
    size_t _radioTop = 0;   // first visible option (scroll offset when the list exceeds the screen)
    void enterRadioSelect(size_t itemIndex);
    void commitRadioSelect(); // write _radioSel back, fire onValueChanged, leave
    bool handleRadioSelectButton(const ButtonEvent& event);
    void drawRadioSelect();

    // Slider sub-mode: a horizontal bar over a stepped Dropdown (item flagged slider).
    // LEFT/RIGHT (and DOWN/UP) move between stops and apply live on each step; OK just leaves.
    size_t _sliderIndex = 0; // index into _currentMenu of the item being edited
    size_t _sliderSel = 0;   // current stop (0..dropdownOptions.size()-1)
    void enterSlider(size_t itemIndex);
    void applySlider(); // write _sliderSel into the item + fire onValueChanged (live apply)
    bool handleSliderButton(const ButtonEvent& event);
    void drawSlider();

    // Icon main menu (user-toggleable): render the root categories as an icon grid instead of a
    // text list. Arrows move within the grid (2D), OK enters. Submenus keep the normal text list.
    static constexpr uint8_t ICON_COLS = 3;
    static constexpr uint8_t ICON_ROWS = 2;
    bool _iconMenu = false;
    bool iconGridActive() const { return _iconMenu && _menuStack.empty(); }
    void gridMove(int dCol, int dRow);                                                     // move the cursor within the icon grid
    void drawIconMenu();                                                                   // render the root as an icon grid
    void drawCategoryIcon(const std::string& label, int16_t cx, int16_t cy, uint16_t col); // ~18x18 GFX glyph by category

    // Common: return to Normal mode and force a redraw.
    void leaveEditor();

    // Core functionality
    void navigateUp();
    void navigateDown();
    void selectItem();
    void navigateLeft();
    void navigateRight();

    void addDefaultMenus();
    void addDefaultActions();
    void addDefaultOnValueChanged();

    // (re)build the root menu from the shared MenuRegistry, merging its callbacks into the local
    // registries. Built lazily (first open / after the startup delay) so late registrations are still
    // picked up. No-op when no registry was handed in.
    void buildMenuFromRegistry();

    // Build the device-owned Netzwerk / Widgets submenu roots from the live NetworkModule /
    // WidgetsManager. Presence-guarded: a missing module yields a plain default tree instead of a crash.
    MenuConfig::MenuOption buildNetworkRootItem();
    MenuConfig::MenuOption buildWidgetsRootItem();

    bool _displayRootsRegistered = false; // display roots pushed into registry once
    bool _registryMenuBuilt = false;      // lazy-build guard (first open)

    void clearDisplay();
    void drawMenu();

    // Graphic on/off toggle switch (rounded pill + knob) for Checkbox/ProgToggle rows.
    void drawToggleSwitch(int16_t x, int16_t y, int16_t w, int16_t h, bool on, uint16_t fg, uint16_t bg);

    // Visibility filter — combines visibleIf with the devOnly/_devMode flag.
    bool isItemVisible(const MenuConfig::MenuOption& item) const;
    // First visible item at/after `from`, or `from` when none is found.
    size_t firstVisibleFrom(size_t from) const;

    // Right-aligned value/indicator string for an item.
    std::string itemValueText(const MenuConfig::MenuOption& item) const;

    // True when `item` is an IP row currently locked by DHCP (net_dhcp present and true). Such rows
    // stay visible but render "auto" and must NOT enter the octet editor on select.
    bool isDhcpLocked(const MenuConfig::MenuOption& item) const;

    // `text` truncated to fit `maxWidth` px at the current font, with an ASCII "..." when it
    // overflows; returns `text` unchanged when it already fits.
    std::string truncateToWidth(const std::string& text, int16_t maxWidth) const;

    // Display properties
    i2cDisplay* _display = nullptr;
    uint16_t _screenWidth = 0;
    uint16_t _screenHeight = 0;

    uint32_t _lastRedrawTime = 0;
    uint32_t _lastButtonPressTime = 0;

    // Keep redrawing for a short window after (re)activation so the menu wins the outgoing widget's
    // stop()/clearDisplay(), which runs after the menu already drew this frame (else blank until the
    // next button press).
    uint32_t _activationRedrawUntil = 0;
    static constexpr uint32_t ACTIVATION_REDRAW_MS = 400;

    // Widget properties
    uint32_t _displayTime;
    WidgetFlags _action;
    std::string _name = "Menu";
    WidgetState _state = WidgetState::STOPPED;
    WidgetState _stateLast = WidgetState::STOPPED;

    // Menu
    std::vector<MenuConfig::MenuOption> _currentMenu;
    std::vector<std::vector<MenuConfig::MenuOption>> _menuStack;
    // Parallel to _menuStack — the parent's _selectedIndex, restored on Back.
    std::vector<size_t> _selectedIndexStack;
    MenuConfig _menuConfig;
    MenuRegistry* _menuRegistry = nullptr; // set by DeviceDisplay
    size_t _selectedIndex = 0;
    bool _needsRedraw = false;
    bool _isPaused = false;
    bool _FrontPlateEnabled = false;
    bool _devMode = true; // developer/device-only items visible by default

    // Screen/gesture hooks (see setters above). Empty until wired.
    std::function<void()> _onAboutRequested;
    std::function<void(const MenuConfig::MenuOption&)> _onFilesRequested;
    std::function<void(const std::string&, const MenuConfig::MenuOption&)> _onGestureAction;
    std::function<void(const MenuConfig::MenuOption&)> _onIpEditRequested;
    std::function<void(const MenuConfig::MenuOption&)> _onReorderRequested;

    std::unordered_map<std::string, std::function<void()>> actionRegistry;
    std::unordered_map<std::string, std::function<void(const MenuConfig::MenuOption&, const MenuValue&)>> onValueChangedRegistry;
    // Current-index providers for radio-list dropdowns.
    std::unordered_map<std::string, std::function<size_t()>> radioIndexProviderRegistry;
    void assignRegisteredActions(std::vector<MenuConfig::MenuOption>& menuOptions);
    void assignOnValueChangedHandlers(std::vector<MenuConfig::MenuOption>& menuOptions);
    void assignRadioIndexProviders(std::vector<MenuConfig::MenuOption>& menuOptions);

    // Seed each keyed option's defaultValue (crucially net_dhcp) so visibleIf resolves against real
    // state on first entry. Only seeds keys still absent (never overwrites a change).
    void seedDefaultValues(const std::vector<MenuConfig::MenuOption>& menuOptions);

    void showOverlay(std::function<void()> drawFn);
    bool _infoOverlayActive = false;
    const uint32_t _infoOverlayTimeout = 500;      // Prevent overlay from being dismissed too quickly (<1 second)
    const uint32_t _infoOverlayMaxTimeout = 60000; // Prevent overlay from being active for more than 60 seconds (auto close)
    std::function<void()> _overlayDrawFunction;
};
#endif // DEVICE_DISPLAY_MODULE