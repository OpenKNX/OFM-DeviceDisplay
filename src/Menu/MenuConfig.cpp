#include "MenuConfig.h"
#include "OpenKNX.h"

MenuConfig::MenuConfig() = default;

void MenuConfig::loadMenu(const MenuOption& rootMenu)
{
    menu.clear();
    menu = rootMenu.submenu;
}

const MenuValue& MenuConfig::getValue(std::string_view key) const
{
    static const MenuValue empty;
    const auto it = dataStore.find(std::string(key));
    return it != dataStore.end() ? it->second : empty;
}

bool MenuConfig::hasValue(std::string_view key) const
{
    return dataStore.find(std::string(key)) != dataStore.end();
}

void MenuConfig::seedValue(const std::string& key, MenuValue value)
{
    dataStore[key] = std::move(value);
}

void MenuConfig::setValue(const std::string& key, MenuValue value)
{
    dataStore[key] = std::move(value);

    MenuOption* option = findOptionByKey(menu, key);
    if (option && option->onValueChanged)
    {
        option->onValueChanged(*option, value);
    }
}

bool MenuConfig::isMenuOptionVisible(const MenuOption& option) const
{
    if (!option.visibleIf) return true;

    const std::string& key = option.visibleIf->first;
    const MenuValue& expectedValue = option.visibleIf->second;
    const auto it = dataStore.find(key);
    if (it == dataStore.end()) return false;

    const MenuValue& currentValue = it->second;

    if (currentValue.isBool() && expectedValue.isBool())
    {
        return currentValue.getBool() == expectedValue.getBool();
    }
    if (currentValue.isString() && expectedValue.isString())
    {
        return currentValue.getString() == expectedValue.getString();
    }

    return false;
}

MenuConfig::MenuElementType MenuConfig::parseMenuElementType(std::string_view typeStr) const
{
    if (typeStr == "Checkbox") return MenuElementType::Checkbox;
    if (typeStr == "TextInput") return MenuElementType::TextInput;
    if (typeStr == "Dropdown") return MenuElementType::Dropdown;
    if (typeStr == "Action") return MenuElementType::Action;
    if (typeStr == "Submenu") return MenuElementType::Submenu;
    if (typeStr == "Back") return MenuElementType::Back;

    // Accept both the mockup's lowercase spellings and the PascalCase enum names.
    if (typeStr == "Readonly" || typeStr == "ro") return MenuElementType::Readonly;
    if (typeStr == "IpEdit") return MenuElementType::IpEdit;
    if (typeStr == "IpAddress" || typeStr == "ip") return MenuElementType::IpAddress;
    if (typeStr == "Reorder" || typeStr == "reorder") return MenuElementType::Reorder;
    if (typeStr == "Toast" || typeStr == "act") return MenuElementType::Toast;
    if (typeStr == "ProgToggle" || typeStr == "progtoggle") return MenuElementType::ProgToggle;
    if (typeStr == "Reboot" || typeStr == "reboot") return MenuElementType::Reboot;
    if (typeStr == "Files" || typeStr == "files") return MenuElementType::Files;
    if (typeStr == "About" || typeStr == "about") return MenuElementType::About;

    return MenuElementType::Unknown;
}