#pragma once

#include <cstdint> // Für uint8_t
#include <functional>
#include <optional>
#include <string>
#include <unordered_map>
#include <vector>

class MenuValue
{
  public:
    enum ValueType : uint8_t
    {
        None,
        Boolean,
        String,
        SizeT
    };

    MenuValue() : type(None), boolValue(false) {}
    explicit MenuValue(bool b) : type(Boolean), boolValue(b) {}
    explicit MenuValue(const std::string& s) : type(String), stringValue(s) {}
    explicit MenuValue(size_t s) : type(SizeT), sizeTValue(s) {}

    MenuValue(const MenuValue&) = default;
    MenuValue& operator=(const MenuValue&) = default;
    MenuValue(MenuValue&&) = default;
    MenuValue& operator=(MenuValue&&) = default;

    ValueType getType() const { return type; }
    bool getBool() const { return (type == Boolean) ? boolValue : false; }
    const std::string& getString() const
    {
        static const std::string empty;
        return (type == String) ? stringValue : empty;
    }
    size_t getSizeT() const { return (type == SizeT) ? sizeTValue : 0; }

    bool isBool() const { return type == Boolean; }
    bool isString() const { return type == String; }
    bool isSizeT() const { return type == SizeT; }

  private:
    ValueType type;
    union
    {
        bool boolValue;
        size_t sizeTValue;
    };
    std::string stringValue;
};

class MenuConfig
{
  public:
    enum MenuElementType : uint8_t
    {
        Checkbox,
        TextInput,
        Dropdown,
        Action,
        Submenu,
        Back,
        Readonly, // right-aligned value via valueProvider(), no action on select
        IpEdit,   // 4-octet IP editor, backed by MenuOption::ip[4]
        // Semantically identical to IpEdit (both back MenuOption::ip[4]); the separate
        // name mirrors the mockup's "ip" element. parseMenuElementType maps both spellings.
        IpAddress,  // 4-octet IP address editor, backed by MenuOption::ip[4]
        Reorder,    // reorder screen (e.g. widget rotation order)
        Toast,      // action that shows an overlay toast (MenuOption::toast)
        ProgToggle, // KNX programming-mode toggle ([x]/[ ])
        Reboot,     // trigger a device reboot
        Files,      // open the SD-card file browser
        About,      // open the "About" screen
        Unknown
    };

    struct MenuOption
    {
        std::string label;
        MenuElementType type;
        std::string key;
        MenuValue defaultValue;
        std::vector<std::string> dropdownOptions;
        std::optional<std::pair<std::string, MenuValue>> visibleIf;
        std::vector<MenuOption> submenu;
        // Lazy submenu: when `submenu` is empty but this is set, the children are BUILT on entry.
        // Keeps the menu-open tree small — heavy submenus (Widgets per-widget subs, Netzwerk live
        // IPs) pay their build cost only when actually opened, not on every menu open.
        std::function<std::vector<MenuOption>()> submenuBuilder;

        std::function<void()> action;
        std::function<void(const MenuOption&, const MenuValue&)> onValueChanged;

        std::string toast;                          // message shown when a Toast item is selected
        std::function<std::string()> valueProvider; // right-aligned live value for Readonly items
        uint8_t ip[4] = {0, 0, 0, 0};               // octet carrier for IpEdit items
        bool devOnly = false;                       // item only visible when developer mode is active
        int sortOrder = 0;                          // stable root ordering (0 = registration order)

        // Richer editors for Dropdown items (else the item cycles in place on select).
        bool radioList = false;                     // open a full-screen single-select picker
        bool slider = false;                        // open a horizontal slider over the options (live-applied)
        std::function<size_t()> radioIndexProvider; // optional: live current index (reflects runtime/store state)
    };

    MenuConfig();
    ~MenuConfig() = default;

    MenuConfig(const MenuConfig&) = delete;
    MenuConfig& operator=(const MenuConfig&) = delete;
    MenuConfig(MenuConfig&&) = default;
    MenuConfig& operator=(MenuConfig&&) = default;

    void loadMenu(const MenuOption& rootMenu);

    const MenuValue& getValue(std::string_view key) const;
    void setValue(const std::string& key, MenuValue value);

    // True when `key` already has a stored value; seed defaultValues on menu
    // (re)build only when absent, so a user-changed value is not overwritten.
    bool hasValue(std::string_view key) const;

    // Store `value` for `key` WITHOUT firing onValueChanged. Pure state seed used
    // on menu (re)build to prime visibleIf state (e.g. net_dhcp); must not trigger
    // side effects (contrast changes, DHCP enable/disable, ...) merely on load.
    void seedValue(const std::string& key, MenuValue value);

    bool isMenuOptionVisible(const MenuOption& option) const;
    inline const std::vector<MenuOption>& getMenu() const { return menu; }
    inline void setMenu(std::vector<MenuOption> newMenu) { menu = std::move(newMenu); }

  private:
    MenuElementType parseMenuElementType(std::string_view typeStr) const;
    std::vector<MenuOption> menu;
    std::unordered_map<std::string, MenuValue> dataStore;

    MenuOption* findOptionByKey(std::vector<MenuOption>& list, const std::string& key)
    {
        for (auto& opt : list)
        {
            if (opt.key == key) return &opt;
            if (auto* sub = findOptionByKey(opt.submenu, key)) return sub;
        }
        return nullptr;
    }
};