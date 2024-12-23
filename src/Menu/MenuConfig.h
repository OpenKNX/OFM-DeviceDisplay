#pragma once

#include <nlohmann/json.hpp>
#include <optional>


using json = nlohmann::json;

class MenuValue {
public:
    enum class ValueType : uint8_t {
        None,
        Boolean,
        String,
        SizeT
    };

    MenuValue() : type(ValueType::None), boolValue(false) {}
    explicit MenuValue(bool b) : type(ValueType::Boolean), boolValue(b) {}
    explicit MenuValue(const std::string& s) : type(ValueType::String), stringValue(s) {}
    explicit MenuValue(size_t s) : type(ValueType::SizeT), sizeTValue(s) {}

    MenuValue(const MenuValue&) = default;
    MenuValue& operator=(const MenuValue&) = default;
    MenuValue(MenuValue&&) = default;
    MenuValue& operator=(MenuValue&&) = default;

    ValueType getType() const { return type; }
    bool getBool() const { return (type == ValueType::Boolean) ? boolValue : false; }
    const std::string& getString() const { 
        static const std::string empty;
        return (type == ValueType::String) ? stringValue : empty;
    }
    size_t getSizeT() const { return (type == ValueType::SizeT) ? sizeTValue : 0; }

    bool isBool() const { return type == ValueType::Boolean; }
    bool isString() const { return type == ValueType::String; }
    bool isSizeT() const { return type == ValueType::SizeT; }

private:
    ValueType type;
    union {
        bool boolValue;
        size_t sizeTValue;
    };
    std::string stringValue;
};

class MenuConfig {
public:
    enum class MenuElementType : uint8_t {
        Checkbox,
        TextInput,
        Dropdown,
        Action,
        Submenu,
        Back,
        Unknown
    };

    struct MenuOption {
        std::string label;
        MenuElementType type;
        std::string key;
        MenuValue defaultValue;
        std::vector<std::string> dropdownOptions;
        std::optional<std::pair<std::string, MenuValue>> visibleIf;
        std::vector<MenuOption> submenu;
        std::function<void()> action;
    };

    MenuConfig();
    ~MenuConfig() = default;

    MenuConfig(const MenuConfig&) = delete;
    MenuConfig& operator=(const MenuConfig&) = delete;
    MenuConfig(MenuConfig&&) = default;
    MenuConfig& operator=(MenuConfig&&) = default;

    void loadDefaultMenu(std::string_view jsonString);
    void loadExternalMenu(std::string_view jsonString);
    const MenuValue& getValue(std::string_view key) const;
    void setValue(const std::string& key, MenuValue value);
    bool isMenuOptionVisible(const MenuOption& option) const;
    const std::vector<MenuOption>& getMenu() const { return menu; }

private:
    void processMenuItems(const json& menuItems, std::vector<MenuOption>& targetMenu);
    void processSubmenu(const json& submenuItems, std::vector<MenuOption>& submenu);
    void setDefaultValue(const MenuOption& option, const json& item);
    MenuElementType parseMenuElementType(std::string_view typeStr) const;

    std::vector<MenuOption> menu;
    std::unordered_map<std::string, MenuValue> dataStore;
};