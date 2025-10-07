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

        std::function<void()> action;
        std::function<void(const MenuOption&, const MenuValue&)> onValueChanged;
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