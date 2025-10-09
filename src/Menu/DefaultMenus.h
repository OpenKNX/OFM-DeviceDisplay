#pragma once

#include "MenuConfig.h"
namespace DefaultMenus {

using MenuOption = MenuConfig::MenuOption;

inline MenuOption buildMenu() {
    // --- Wiederverwendbarer "Back"-Eintrag ---
    static const MenuOption backOption = [] {
        MenuOption b;
        b.label = "Back";
        b.type = MenuConfig::MenuElementType::Back;
        return b;
    }();

    // --- Network Settings ---
    MenuOption dhcp;
    dhcp.label = "DHCP";
    dhcp.type = MenuConfig::MenuElementType::Checkbox;
    dhcp.key = "dhcp_enabled";
    dhcp.defaultValue = MenuValue(true);

    MenuOption ipAddress;
    ipAddress.label = "IP Address";
    ipAddress.type = MenuConfig::MenuElementType::Action;
    ipAddress.visibleIf = std::make_pair("dhcp_enabled", MenuValue(false));

    MenuOption subnetMask;
    subnetMask.label = "Subnet Mask";
    subnetMask.type = MenuConfig::MenuElementType::Action;
    subnetMask.visibleIf = std::make_pair("dhcp_enabled", MenuValue(true));

    MenuOption dnsPrimary;
    dnsPrimary.label = "Primary DNS";
    dnsPrimary.type = MenuConfig::MenuElementType::Action;

    MenuOption dnsSecondary;
    dnsSecondary.label = "Secondary DNS";
    dnsSecondary.type = MenuConfig::MenuElementType::Action;

    MenuOption dnsSettings;
    dnsSettings.label = "DNS Settings";
    dnsSettings.type = MenuConfig::MenuElementType::Submenu;
    dnsSettings.submenu = {dnsPrimary, dnsSecondary, backOption};

    MenuOption networkSettings;
    networkSettings.label = "Network Settings";
    networkSettings.type = MenuConfig::MenuElementType::Submenu;
    networkSettings.submenu = {dhcp, ipAddress, subnetMask, dnsSettings, backOption};

    // --- System Settings ---
    MenuOption timeZone;
    timeZone.label = "Time Zone";
    timeZone.type = MenuConfig::MenuElementType::Dropdown;
    timeZone.key = "timezone";
    timeZone.dropdownOptions = {"UTC-12", "UTC-11", "UTC+0", "UTC+1", "UTC+2"};
    timeZone.defaultValue = MenuValue(static_cast<size_t>(2));

    MenuOption dateTime;
    dateTime.label = "Date/Time";
    dateTime.type = MenuConfig::MenuElementType::Action;

    MenuOption rebootDevice;
    rebootDevice.label = "Reboot Device";
    rebootDevice.type = MenuConfig::MenuElementType::Action;
    rebootDevice.key = "reboot_device";

    MenuOption systemSettings;
    systemSettings.label = "System Settings";
    systemSettings.type = MenuConfig::MenuElementType::Submenu;
    systemSettings.submenu = {timeZone, dateTime, rebootDevice, backOption};

    // --- Device Info ---
    MenuOption devName;
    devName.label = "Device Name";
    devName.type = MenuConfig::MenuElementType::Action;

    MenuOption fwVersion;
    fwVersion.label = "Firmware Version";
    fwVersion.type = MenuConfig::MenuElementType::Action;

    MenuOption serialNum;
    serialNum.label = "Serial Number";
    serialNum.type = MenuConfig::MenuElementType::Action;

    MenuOption deviceInfo;
    deviceInfo.label = "Device Info";
    deviceInfo.type = MenuConfig::MenuElementType::Submenu;
    deviceInfo.submenu = {devName, fwVersion, serialNum, backOption};

    // --- Main Settings ---
    MenuOption mainSettings;
    mainSettings.label = "Main Settings";
    mainSettings.type = MenuConfig::MenuElementType::Submenu;
    mainSettings.submenu = {networkSettings, systemSettings, deviceInfo, backOption};

    // --- Advanced Settings ---
    MenuOption brightness;
    brightness.label = "Brightness";
    brightness.type = MenuConfig::MenuElementType::Dropdown;
    brightness.key = "brightness_level";
    brightness.dropdownOptions = {"25%", "50%", "75%", "100%"};
    brightness.defaultValue = MenuValue(static_cast<size_t>(2));
    
    MenuOption autoDimming;
    autoDimming.label = "Auto Dimming";
    autoDimming.type = MenuConfig::MenuElementType::Checkbox;
    autoDimming.key = "auto_dimming";
    autoDimming.defaultValue = MenuValue(true);

    MenuOption display;
    display.label = "Display";
    display.type = MenuConfig::MenuElementType::Submenu;
    display.submenu = {brightness, autoDimming, backOption};

    MenuOption advReboot;
    advReboot.label = "Reboot";
    advReboot.type = MenuConfig::MenuElementType::Action;
    advReboot.key = "reboot_device";

    MenuOption advancedSettings;
    advancedSettings.label = "Advanced Settings";
    advancedSettings.type = MenuConfig::MenuElementType::Submenu;
    advancedSettings.submenu = {display, advReboot, backOption};

    // --- Prog Mode ---
    MenuOption progMode;
    progMode.label = "Prog-Mode";
    progMode.type = MenuConfig::MenuElementType::Action;
    progMode.key = "prog_mode";

    // --- Show Device Info Overlay ---
    MenuOption showDeviceInfo;
    showDeviceInfo.label = "Show Device Info";
    showDeviceInfo.type = MenuConfig::MenuElementType::Action;
    showDeviceInfo.key = "show_device_info_overlay";

    // --- Root Menu ---
    MenuOption rootMenu;
    rootMenu.label = "Root";
    rootMenu.type = MenuConfig::MenuElementType::Submenu;
    //rootMenu.submenu = {mainSettings, advancedSettings, progMode, showDeviceInfo};
    rootMenu.submenu = { advancedSettings, progMode, showDeviceInfo};

    return rootMenu;
}

} // namespace DefaultMenus