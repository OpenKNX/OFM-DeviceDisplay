#pragma once

#include <nlohmann/json.hpp> // JSON parsing library
#include <optional>
#include <variant>

using json = nlohmann::json;

class MenuValue
{
  public:
    enum class ValueType { None, Boolean, String, SizeT }; // Supported value types
    // Constructors for each type
    MenuValue() : type(ValueType::None), boolValue(false), stringValue(""), sizeTValue(0) {}
    MenuValue(bool b) : type(ValueType::Boolean), boolValue(b), sizeTValue(0), stringValue("") {}
    MenuValue(const std::string &s) : type(ValueType::String), boolValue(false), sizeTValue(0), stringValue(s) {}
    MenuValue(size_t s) : type(ValueType::SizeT), boolValue(false), sizeTValue(s), stringValue("") {}
    ValueType getType() const { return type; }

    // Accessors
    bool getBool() const
    {
        if (type == ValueType::Boolean) return boolValue;
        else return false;
    }

    std::string getString() const
    {
        if (type == ValueType::String) return stringValue;
        else return "";
    }

    size_t getSizeT() const
    {
        if (type == ValueType::SizeT) return sizeTValue;
        else return 0;
    }

    inline bool isBool() const { return type == ValueType::Boolean; }
    inline bool isString() const { return type == ValueType::String; }
    inline bool isSizeT() const { return type == ValueType::SizeT; }

  private:
    ValueType type;

    // Internal storage
    bool boolValue;
    std::string stringValue;
    size_t sizeTValue;
};

class MenuConfig
{
  public:
    // Supported menu element types
    enum class MenuElementType
    {
        Checkbox,
        TextInput,
        Dropdown,
        Action,
        Submenu,
        Back,
        Unknown
    };

    // Structure for a menu option
    struct MenuOption
    {
        std::string label;                                          // Name of the menu item
        MenuElementType type;                                       // Type of the menu item
        std::string key;                                            // Unique key for the menu option
        MenuValue defaultValue;                                     // Default value (Checkbox: bool, TextInput: string, Dropdown: size_t)
        std::function<void()> action;                               // Function to call for Action type
        std::vector<std::string> dropdownOptions;                   // Dropdown options
        std::optional<std::pair<std::string, MenuValue>> visibleIf; // Visibility condition (key and value)
        std::vector<MenuOption> submenu;
    };

    const std::string logPrefix() { return "MenuConfig"; }

    using MenuValues = std::variant<bool, int, std::string, size_t>; // Supported value types

    MenuConfig(); // Constructor

    // Load the default menu from JSON
    void loadDefaultMenu(const std::string &jsonString);

    // Load additional menu options from external JSON
    void loadExternalMenu(const std::string &jsonString);

    // Retrieve the value of a menu item
    MenuValue getValue(const std::string &key) const;

    // Set the value of a menu item
    void setValue(const std::string &key, const MenuValue &value);

    // Check visibility of a menu option
    bool isMenuOptionVisible(const MenuOption &option) const;

    // Get the full menu structure
    const std::vector<MenuOption> &getMenu() const;

  private:
    void processMenuItems(const json &menuItems, std::vector<MenuOption> &targetMenu); // Shared logic for processing menu items
    void processSubmenu(const json &submenuItems, std::vector<MenuOption> &submenu);   // Process submenu items
    void addBackOptionIfNeeded(std::vector<MenuOption> &submenu);                      // Add "Back" option if missing

    std::vector<MenuOption> menu;                         // List of menu options
    std::unordered_map<std::string, MenuValue> dataStore; // Centralized data storage

    // Helper to parse the menu element type from string
    MenuElementType parseMenuElementType(const std::string &typeStr) const;

    // Helper to add default values to the data store
    void setDefaultValue(const MenuOption &option, const json &item);
};