#include "MenuConfig.h"
#include "openknx.h"
#include <iostream>
#include <stdexcept>

MenuConfig::MenuConfig() {}

void MenuConfig::loadDefaultMenu(const std::string &jsonString) // Load the default menu from JSON
{
    json jsonData = json::parse(jsonString); // Parse the JSON string
    menu.clear();                            // Clear the current menu

    if (jsonData.contains("menu")) // Process the top-level menu items
    {
        processMenuItems(jsonData["menu"], menu);
    }
    logDebugP("Menu loaded successfully");
}
void MenuConfig::loadExternalMenu(const std::string &jsonString) // Load additional menu options from external JSON
{
    json jsonData = json::parse(jsonString); // Parse the JSON string
    if (jsonData.contains("menu"))           // Process and append additional menu items
    {
        processMenuItems(jsonData["menu"], menu); // Process the menu items
    }
}
const std::vector<MenuConfig::MenuOption> &MenuConfig::getMenu() const // Get the full menu structure
{
    return menu;
}

void MenuConfig::processMenuItems(const json &menuItems, std::vector<MenuOption> &targetMenu)
{
    for (const json::value_type &item : menuItems) // Iterate through the given menu items
    {
        MenuOption option;
        option.label = item["label"].get<std::string>();       // Parse the label of the menu item
        std::string typeStr = item["type"].get<std::string>(); // Parse the type of the menu item
        option.type = parseMenuElementType(typeStr);

        if (item.contains("key")) // If a key exists, set it
        {
            option.key = item["key"].get<std::string>();
        }

        if (item.contains("visibleIf")) // If visibility conditions exist, set them (ToDo EC: Check if the key exists)
        {
            const json &condition = item["visibleIf"];             // Parse the visibility condition
            std::string key = condition["key"].get<std::string>(); // Parse the key of the condition
            if (condition["value"].is_boolean())
            {
                option.visibleIf = std::make_pair(key, condition["value"].get<bool>());
            }
            else if (condition["value"].is_string())
            {
                option.visibleIf = std::make_pair(key, condition["value"].get<std::string>());
            }
        }

        if (item.contains("options")) // If dropdown options exist, set them
        {
            option.dropdownOptions = item["options"].get<std::vector<std::string>>();
        }

        if (item.contains("submenu")) // If submenu exists, process submenu
        {
            processSubmenu(item["submenu"], option.submenu);
        }

        setDefaultValue(option, item); // Set the default value for the menu item
        targetMenu.push_back(option);  // Add the menu option to the target menu
    }
}
void MenuConfig::setDefaultValue(const MenuOption &option, const json &item) // Helper to add default values to the data store
{
    if (!item.contains("defaultValue") || option.key.empty()) return; // If no default value exists or the key is empty, return

    if (option.type == MenuElementType::Checkbox) // If the type is "Checkbox"
    {
        dataStore[option.key] = MenuValue(item["defaultValue"].get<bool>()); // Set the default value as a boolean
    }
    else if (option.type == MenuElementType::TextInput) // If the type is "TextInput"
    {
        dataStore[option.key] = MenuValue(item["defaultValue"].get<std::string>()); // Set the default value as a string
    }
    else if (option.type == MenuElementType::Dropdown) // If the type is "Dropdown"
    {
        dataStore[option.key] = MenuValue(item["defaultValue"].get<size_t>()); // Set the default value as a size_t
    }
}
void MenuConfig::processSubmenu(const json &submenuItems, std::vector<MenuOption> &submenu)
{
    for (const json::value_type &subItem : submenuItems) // Iterate through submenu items
    {
        MenuOption subOption;                                  // Create a submenu option
        subOption.label = subItem["label"].get<std::string>(); // Parse the label of the submenu item

        std::string subTypeStr = subItem["type"].get<std::string>(); // Parse the type of the submenu item
        subOption.type = parseMenuElementType(subTypeStr);           // Parse the type of the submenu item

        if (subItem.contains("key")) // If a key exists, set it
        {
            subOption.key = subItem["key"].get<std::string>();
        }

        if (subItem.contains("visibleIf")) // If visibility conditions exist, set them
        {
            const json &subCondition = subItem["visibleIf"];
            std::string subKey = subCondition["key"].get<std::string>();
            if (subCondition["value"].is_boolean())
            {
                subOption.visibleIf = std::make_pair(subKey, subCondition["value"].get<bool>());
            }
            else if (subCondition["value"].is_string())
            {
                subOption.visibleIf = std::make_pair(subKey, subCondition["value"].get<std::string>());
            }
        }

        if (subItem.contains("options")) // If dropdown options exist, set them
        {
            subOption.dropdownOptions = subItem["options"].get<std::vector<std::string>>();
        }

        if (subItem.contains("submenu")) // If submenu exists, process submenu
        {
            processSubmenu(subItem["submenu"], subOption.submenu);
        }

        setDefaultValue(subOption, subItem); // Set the default value for the submenu item

        submenu.push_back(subOption); // Add the submenu option
    }

    addBackOptionIfNeeded(submenu); // Add a "Back" option if it is missing
}
MenuConfig::MenuElementType MenuConfig::parseMenuElementType(const std::string &typeStr) const // Helper to parse the menu element type from string
{
    if (typeStr == "Checkbox") // If the type is "Checkbox", return the corresponding enum value
        return MenuElementType::Checkbox;
    if (typeStr == "TextInput") // If the type is "TextInput", return the corresponding enum value
        return MenuElementType::TextInput;
    if (typeStr == "Dropdown") // If the type is "Dropdown", return the corresponding enum value
        return MenuElementType::Dropdown;
    if (typeStr == "Action") // If the type is "Action", return the corresponding enum value
        return MenuElementType::Action;
    if (typeStr == "Submenu") // If the type is "Submenu", return the corresponding enum value
        return MenuElementType::Submenu;
    if (typeStr == "Back") // If the type is "Back", return the corresponding enum value
        return MenuElementType::Back;

    return MenuElementType::Unknown; // For all unknown types, return "Unknown"
}
void MenuConfig::addBackOptionIfNeeded(std::vector<MenuOption> &submenu)
{
    bool hasBack = false;
    for (const MenuOption &opt : submenu) // Check if a "Back" option already exists
    {
        if (opt.type == MenuConfig::MenuElementType::Back) // If a "Back" option exists, set the flag
        {
            hasBack = true;
            break;
        }
    }

    if (!hasBack) // If no "Back" option exists, add it at the beginning
    {
        MenuOption backOption;
        backOption.label = "Back";
        backOption.type = MenuConfig::MenuElementType::Back;
        submenu.insert(submenu.begin(), backOption);
    }
}

MenuValue MenuConfig::getValue(const std::string &key) const // Retrieve the value of a menu item
{
    std::unordered_map<std::string, MenuValue>::const_iterator it = dataStore.find(key); // Find the key in the data store
    if (it != dataStore.end())                                                           // If the key exists, return the value
    {
        return it->second;
    }
    else
        return MenuValue(); // If the key does not exist, return an empty value
}
void MenuConfig::setValue(const std::string &key, const MenuValue &value) // Set the value of a menu item
{
    dataStore[key] = value;
}

// TODO: Show/Hide menu items based on visibility/config conditions
bool MenuConfig::isMenuOptionVisible(const MenuOption &option) const // Check visibility of a menu option
{
    if (option.visibleIf.has_value()) // If visibility conditions exist
    {
        const std::pair<std::string, MenuValue> &condition = option.visibleIf.value();
        std::string key = condition.first;
        MenuValue expectedValue = condition.second;

        if (dataStore.find(key) != dataStore.end()) // Check if the key exists in the data store
        {
            MenuValue currentValue = dataStore.at(key);
            if (currentValue.isBool() && expectedValue.isBool()) // If both values are boolean
            {
                return currentValue.getBool() == expectedValue.getBool();
            }
            else if (currentValue.isString() && expectedValue.isString()) // If both values are strings
            {
                return currentValue.getString() == expectedValue.getString();
            }
        }
        return false; // If condition is not met, hide the menu option
    }
    return true; // Default: visible
}
