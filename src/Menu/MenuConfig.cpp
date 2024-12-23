#include "MenuConfig.h"
#include "OpenKNX.h"

static constexpr char const* KEY_MENU = "menu";
static constexpr char const* KEY_LABEL = "label";
static constexpr char const* KEY_TYPE = "type";
static constexpr char const* KEY_KEY = "key";
static constexpr char const* KEY_DEFAULT_VALUE = "defaultValue";
static constexpr char const* KEY_OPTIONS = "options";
static constexpr char const* KEY_VISIBLE_IF = "visibleIf";
static constexpr char const* KEY_SUBMENU = "submenu";
static constexpr char const* KEY_VALUE = "value";
static constexpr char const* TYPE_CHECKBOX = "Checkbox";
static constexpr char const* TYPE_TEXTINPUT = "TextInput";
static constexpr char const* TYPE_DROPDOWN = "Dropdown";
static constexpr char const* TYPE_ACTION = "Action";
static constexpr char const* TYPE_SUBMENU = "Submenu";
static constexpr char const* TYPE_BACK = "Back";

MenuConfig::MenuConfig() = default;

void MenuConfig::loadDefaultMenu(std::string_view jsonString) {
    menu.clear();
    menu.shrink_to_fit();
    
    const nlohmann::json_abi_v3_11_3::json jsonData = json::parse(jsonString);
    if (jsonData.contains(KEY_MENU)) {
        processMenuItems(jsonData[KEY_MENU], menu);
    }
}

void MenuConfig::loadExternalMenu(std::string_view jsonString) {
    const nlohmann::json_abi_v3_11_3::json jsonData = json::parse(jsonString);
    if (jsonData.contains(KEY_MENU)) {
        processMenuItems(jsonData[KEY_MENU], menu);
    }
}

void MenuConfig::processMenuItems(const json& menuItems, std::vector<MenuOption>& targetMenu) {
    targetMenu.reserve(targetMenu.size() + menuItems.size());
    
    for (const nlohmann::json_abi_v3_11_3::json &item : menuItems) {
        MenuOption option;
        option.label = item[KEY_LABEL].get<std::string>();
        option.type = parseMenuElementType(item[KEY_TYPE].get<std::string>());

        if (item.contains(KEY_KEY)) {
            option.key = item[KEY_KEY].get<std::string>();
        }

        if (item.contains(KEY_VISIBLE_IF)) {
            const nlohmann::json_abi_v3_11_3::json &condition = item[KEY_VISIBLE_IF];
            const std::string &key = condition[KEY_KEY].get<std::string>();
            
            if (condition[KEY_VALUE].is_boolean()) {
                option.visibleIf.emplace(key, MenuValue(condition[KEY_VALUE].get<bool>()));
            } 
            else if (condition[KEY_VALUE].is_string()) {
                option.visibleIf.emplace(key, MenuValue(condition[KEY_VALUE].get<std::string>()));
            }
        }

        if (item.contains(KEY_OPTIONS)) {
            option.dropdownOptions = item[KEY_OPTIONS].get<std::vector<std::string>>();
        }

        if (item.contains(KEY_SUBMENU)) {
            processSubmenu(item[KEY_SUBMENU], option.submenu);
        }

        setDefaultValue(option, item);
        targetMenu.emplace_back(std::move(option));
    }
}

void MenuConfig::processSubmenu(const json& submenuItems, std::vector<MenuOption>& submenu) {
    submenu.reserve(submenuItems.size() + 1);
    
    MenuOption backOption;
    backOption.label = "Back";
    backOption.type = MenuElementType::Back;
    submenu.emplace_back(std::move(backOption));

    processMenuItems(submenuItems, submenu);
}

void MenuConfig::setDefaultValue(const MenuOption& option, const json& item) {
    if (!item.contains(KEY_DEFAULT_VALUE) || option.key.empty()) {
        return;
    }

    switch (option.type) {
        case MenuElementType::Checkbox:
            dataStore.emplace(option.key, MenuValue(item[KEY_DEFAULT_VALUE].get<bool>()));
            break;
        case MenuElementType::TextInput:
            dataStore.emplace(option.key, MenuValue(item[KEY_DEFAULT_VALUE].get<std::string>()));
            break;
        case MenuElementType::Dropdown:
            dataStore.emplace(option.key, MenuValue(item[KEY_DEFAULT_VALUE].get<size_t>()));
            break;
        default:
            break;
    }
}

MenuConfig::MenuElementType MenuConfig::parseMenuElementType(std::string_view typeStr) const {
    if (typeStr == TYPE_CHECKBOX) return MenuElementType::Checkbox;
    if (typeStr == TYPE_TEXTINPUT) return MenuElementType::TextInput;
    if (typeStr == TYPE_DROPDOWN) return MenuElementType::Dropdown;
    if (typeStr == TYPE_ACTION) return MenuElementType::Action;
    if (typeStr == TYPE_SUBMENU) return MenuElementType::Submenu;
    if (typeStr == TYPE_BACK) return MenuElementType::Back;
    return MenuElementType::Unknown;
}

const MenuValue& MenuConfig::getValue(std::string_view key) const {
    static const MenuValue empty;
    const std::unordered_map<std::string, MenuValue>::const_iterator it = dataStore.find(std::string(key));
    return it != dataStore.end() ? it->second : empty;
}

void MenuConfig::setValue(const std::string& key, MenuValue value) {
    dataStore[key] = std::move(value);
}

bool MenuConfig::isMenuOptionVisible(const MenuOption& option) const {
    if (!option.visibleIf) {
        return true;
    }

    const std::string& key = option.visibleIf->first;
    const MenuValue& expectedValue = option.visibleIf->second;
    const std::unordered_map<std::string, MenuValue>::const_iterator it = dataStore.find(key);
    if (it == dataStore.end()) {
        return false;
    }

    const MenuValue &currentValue = it->second;
    
    if (currentValue.isBool() && expectedValue.isBool()) {
        return currentValue.getBool() == expectedValue.getBool();
    }
    if (currentValue.isString() && expectedValue.isString()) {
        return currentValue.getString() == expectedValue.getString();
    }
    
    return false;
}