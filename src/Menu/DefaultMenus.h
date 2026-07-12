#pragma once

#include "MenuConfig.h"
namespace DefaultMenus
{

    using MenuOption = MenuConfig::MenuOption;

    inline MenuOption buildMenu()
    {
        // Reusable "Back" entry.
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
        // rootMenu.submenu = {mainSettings, advancedSettings, progMode, showDeviceInfo};
        rootMenu.submenu = {advancedSettings, progMode, showDeviceInfo};

        return rootMenu;
    }

    // Display-owned root menu entries. About is pinned last by MenuRegistry::pinAboutLast();
    // sortOrder leaves gaps so module roots slot in between.
    inline std::vector<MenuOption> buildDisplayRootItems()
    {
        const MenuOption backOption = [] {
            MenuOption b;
            b.label = "Zurueck";
            b.type = MenuConfig::MenuElementType::Back;
            return b;
        }();

        std::vector<MenuOption> roots;

        // --- System-Info (read-only device facts) -----------------------------
        {
            // Each read-only row pulls its live value via valueProvider.
            MenuOption device;
            device.label = "Geraet";
            device.type = MenuConfig::MenuElementType::Readonly;
            device.valueProvider = []() -> std::string { return std::string(openknx.info.firmwareName()); };

            MenuOption firmware;
            firmware.label = "Firmware";
            firmware.type = MenuConfig::MenuElementType::Readonly;
            firmware.valueProvider = []() -> std::string { return std::string(openknx.info.humanFirmwareVersion()); };

            MenuOption serial;
            serial.label = "Seriennummer";
            serial.type = MenuConfig::MenuElementType::Readonly;
            serial.valueProvider = []() -> std::string { return std::string(openknx.info.humanSerialNumber()); };

            MenuOption knxType;
            knxType.label = "KNX-Typ";
            knxType.type = MenuConfig::MenuElementType::Readonly;
            knxType.valueProvider = []() -> std::string {
                switch (MASK_VERSION)
                {
                    case 0x091A: return "Router 091A";
                    case 0x07B0: return "TP Device 07B0";
                    case 0x57B0: return "Coupler 57B0";
                    default:
                    {
                        char b[12];
                        snprintf(b, sizeof(b), "0x%04X", (unsigned)MASK_VERSION);
                        return b;
                    }
                }
            };

            MenuOption bcuStatus;
            bcuStatus.label = "BCU-Status";
            bcuStatus.type = MenuConfig::MenuElementType::Readonly;
            bcuStatus.valueProvider = []() -> std::string { return knx.configured() ? std::string("OK") : std::string("-"); };

            MenuOption freeMem;
            freeMem.label = "Freier Speicher";
            freeMem.type = MenuConfig::MenuElementType::Readonly;
            freeMem.valueProvider = []() -> std::string {
                char b[16];
                snprintf(b, sizeof(b), "%.1f KiB", (float)freeMemory() / 1024.0f);
                return b;
            };

            MenuOption sysInfo;
            sysInfo.label = "System-Info";
            sysInfo.type = MenuConfig::MenuElementType::Submenu;
            sysInfo.sortOrder = 10;
            sysInfo.submenu = {device, firmware, serial, knxType, bcuStatus, freeMem, backOption};
            roots.push_back(std::move(sysInfo));
        }

        // Selectable "Beenden" exit near the bottom of the root menu; About stays pinned last.
        {
            MenuOption exitItem;
            exitItem.label = "< Beenden";
            exitItem.type = MenuConfig::MenuElementType::Back;
            exitItem.sortOrder = 900;
            roots.push_back(std::move(exitItem));
        }

        // --- Anzeige (display settings, device-only) --------------------------
        {
            // Keys must match the onValueChanged registrations in DeviceDisplay or the wiring no-ops.
            MenuOption brightness;
            brightness.label = "Helligkeit";
            brightness.type = MenuConfig::MenuElementType::Dropdown;
            brightness.key = "brightness_level";
            brightness.dropdownOptions = {"25%", "50%", "75%", "100%"};
            brightness.defaultValue = MenuValue(static_cast<size_t>(3));
            brightness.slider = true; // adjust via a horizontal slider (live)

            MenuOption autoDim;
            autoDim.label = "Auto-Dimmen";
            autoDim.type = MenuConfig::MenuElementType::Checkbox;
            autoDim.key = "auto_dimming";
            autoDim.defaultValue = MenuValue(true);

            MenuOption invert;
            invert.label = "Invertieren";
            invert.type = MenuConfig::MenuElementType::Checkbox;
            invert.key = "display_invert";
            invert.defaultValue = MenuValue(false);

            MenuOption fontSize;
            fontSize.label = "Schriftgroesse";
            fontSize.type = MenuConfig::MenuElementType::Dropdown;
            fontSize.key = "font_size";
            fontSize.dropdownOptions = {"Normal", "Gross", "Groesser"};
            fontSize.defaultValue = MenuValue(static_cast<size_t>(1));

            MenuOption autoPage;
            autoPage.label = "Seiten auto-blaettern";
            autoPage.type = MenuConfig::MenuElementType::Checkbox;
            autoPage.key = "auto_paging";
            autoPage.defaultValue = MenuValue(true);

            MenuOption iconMenuOpt;
            iconMenuOpt.label = "Icon-Menue";
            iconMenuOpt.type = MenuConfig::MenuElementType::Checkbox;
            iconMenuOpt.key = "icon_menu"; // root icon grid vs text list
            iconMenuOpt.defaultValue = MenuValue(false);

            MenuOption screensaver;
            screensaver.label = "Bildschirmschoner";
            screensaver.type = MenuConfig::MenuElementType::Dropdown;
            screensaver.key = "screensaver_type";
            // Order MUST match ScreenSaverType (value == index).
            screensaver.dropdownOptions = {"Clock", "Cube 3D", "Doom", "FireWorks", "Life", "Matrix", "Matrix Cl.", "Pong", "Rain", "Starfield", "Aus"};
            screensaver.defaultValue = MenuValue(static_cast<size_t>(5)); // Matrix
            screensaver.radioList = true;                                 // open as a full-screen single-select picker

            MenuOption ssAfter;
            ssAfter.label = "Screensaver nach";
            ssAfter.type = MenuConfig::MenuElementType::Dropdown;
            ssAfter.key = "screensaver_after";
            ssAfter.dropdownOptions = {"1 min", "2 min", "5 min", "10 min"};
            ssAfter.defaultValue = MenuValue(static_cast<size_t>(2));

            MenuOption sleepAfter;
            sleepAfter.label = "Schlafen nach";
            sleepAfter.type = MenuConfig::MenuElementType::Dropdown;
            sleepAfter.key = "sleep_after";
            sleepAfter.dropdownOptions = {"5 min", "10 min", "30 min", "nie"};
            sleepAfter.defaultValue = MenuValue(static_cast<size_t>(1));

            MenuOption anzeige;
            anzeige.label = "Anzeige";
            anzeige.type = MenuConfig::MenuElementType::Submenu;
            anzeige.devOnly = true;
            anzeige.sortOrder = 30;
            anzeige.submenu = {brightness, autoDim, invert, fontSize, autoPage, iconMenuOpt,
                               screensaver, ssAfter, sleepAfter, backOption};
            roots.push_back(std::move(anzeige));
        }

        // --- System (Prog-Mode, reboot, factory reset) ------------------------
        {
            MenuOption progMode;
            progMode.label = "Prog-Mode";
            progMode.type = MenuConfig::MenuElementType::ProgToggle;
            progMode.key = "prog_mode";

            MenuOption reboot;
            reboot.label = "Neustart";
            reboot.type = MenuConfig::MenuElementType::Reboot;
            reboot.key = "reboot_device";

            MenuOption factoryReset;
            factoryReset.label = "Werkseinstellungen";
            factoryReset.type = MenuConfig::MenuElementType::Toast;
            factoryReset.key = "factory_reset";
            factoryReset.toast = "Werksreset ...";

            MenuOption system;
            system.label = "System";
            system.type = MenuConfig::MenuElementType::Submenu;
            system.sortOrder = 50;
            system.submenu = {progMode, reboot, factoryReset, backOption};
            roots.push_back(std::move(system));
        }

        // --- Home-Tasten (assignable home-screen keys) ------------------------
        {
            // Index MUST match HomeKeyAction: 0=None 1=Pause 2=Reboot 3=Prog 4=DisplayOff.
            const std::vector<std::string> keyActions = {"-", "Pause", "Reboot", "Prog-Mode", "Display aus"};

            MenuOption keyUp;
            keyUp.label = "Oben";
            keyUp.type = MenuConfig::MenuElementType::Dropdown;
            keyUp.key = "homekey_up";
            keyUp.dropdownOptions = keyActions;
            keyUp.defaultValue = MenuValue(static_cast<size_t>(1)); // Pause

            MenuOption keyDown;
            keyDown.label = "Unten";
            keyDown.type = MenuConfig::MenuElementType::Dropdown;
            keyDown.key = "homekey_down";
            keyDown.dropdownOptions = keyActions;
            keyDown.defaultValue = MenuValue(static_cast<size_t>(2)); // Reboot

            MenuOption keyLeft;
            keyLeft.label = "Links";
            keyLeft.type = MenuConfig::MenuElementType::Dropdown;
            keyLeft.key = "homekey_left";
            keyLeft.dropdownOptions = keyActions;
            keyLeft.defaultValue = MenuValue(static_cast<size_t>(0)); // none

            MenuOption keyRight;
            keyRight.label = "Rechts";
            keyRight.type = MenuConfig::MenuElementType::Dropdown;
            keyRight.key = "homekey_right";
            keyRight.dropdownOptions = keyActions;
            keyRight.defaultValue = MenuValue(static_cast<size_t>(0)); // none

            MenuOption okHold;
            okHold.label = "OK halten";
            okHold.type = MenuConfig::MenuElementType::Readonly;

            MenuOption homeKeys;
            homeKeys.label = "Home-Tasten";
            homeKeys.type = MenuConfig::MenuElementType::Submenu;
            homeKeys.sortOrder = 60;
            homeKeys.submenu = {keyUp, keyDown, keyLeft, keyRight, okHold, backOption};
            roots.push_back(std::move(homeKeys));
        }

        return roots;
    }

    // Netzwerk submenu snapshot: initial IP/DHCP values are handed in by the caller (Menu.cpp);
    // the valueProvider lambdas re-pull live values lazily.
    struct NetSnapshot
    {
        bool dhcp = true;                 // true = DHCP, false = static (override)
        uint8_t ip[4] = {0, 0, 0, 0};     // host address
        uint8_t subnet[4] = {0, 0, 0, 0}; // subnet mask
        uint8_t gateway[4] = {0, 0, 0, 0};
        uint8_t dns[4] = {0, 0, 0, 0};
        uint8_t linkMode = 0; // 0=auto, 1=10 Mbit, 2=100 Mbit
        // Optional live provider (index 0..3 = ip/subnet/gw/dns).
        std::function<std::string(uint8_t)> liveValue;
    };

    inline MenuOption buildNetworkRootItem(const NetSnapshot& snap)
    {
        const MenuOption backOption = [] {
            MenuOption b;
            b.label = "Zurueck";
            b.type = MenuConfig::MenuElementType::Back;
            return b;
        }();

        MenuOption dhcp;
        dhcp.label = "DHCP";
        dhcp.type = MenuConfig::MenuElementType::Checkbox;
        dhcp.key = "net_dhcp";
        dhcp.defaultValue = MenuValue(snap.dhcp);

        auto makeIpRow = [&](const char* label, const char* key, const uint8_t src[4], uint8_t liveIdx) {
            MenuOption o;
            o.label = label;
            o.type = MenuConfig::MenuElementType::IpAddress;
            o.key = key;
            o.ip[0] = src[0];
            o.ip[1] = src[1];
            o.ip[2] = src[2];
            o.ip[3] = src[3];
            // IP rows stay always visible; when DHCP is on they render "auto" and are not editable.
            if (snap.liveValue)
            {
                auto fn = snap.liveValue;
                o.valueProvider = [fn, liveIdx]() { return fn(liveIdx); };
            }
            return o;
        };

        MenuOption ip = makeIpRow("IP-Adresse", "net_ip", snap.ip, 0);
        MenuOption subnet = makeIpRow("Subnetzmaske", "net_subnet", snap.subnet, 1);
        MenuOption gateway = makeIpRow("Gateway", "net_gw", snap.gateway, 2);
        MenuOption dns = makeIpRow("DNS-Server", "net_dns", snap.dns, 3);

        MenuOption link;
        link.label = "Verbindung";
        link.type = MenuConfig::MenuElementType::Dropdown;
        link.key = "net_link";
        link.dropdownOptions = {"Auto", "10 Mbit", "100 Mbit"};
        link.defaultValue = MenuValue(static_cast<size_t>(snap.linkMode < 3 ? snap.linkMode : 0));

        // "Uebernehmen": commits the staged DHCP/IP override live (net_apply action, wired in Menu.cpp).
        MenuOption apply;
        apply.label = "Uebernehmen";
        apply.type = MenuConfig::MenuElementType::Action;
        apply.key = "net_apply";

        MenuOption netz;
        netz.label = "Netzwerk";
        netz.type = MenuConfig::MenuElementType::Submenu;
        netz.devOnly = true;
        netz.sortOrder = 20;
        netz.submenu = {dhcp, ip, subnet, gateway, dns, link, apply, backOption};
        return netz;
    }

    // Widgets submenu: one submenu per rotation widget. Rows are keyed by a stable name-derived
    // suffix; the widget name travels in MenuOption::toast so the callback knows which widget to address.
    struct WidgetDesc
    {
        std::string name;       // widget name (used as toast payload + key suffix)
        bool enabled = true;    // current isEnabled() state
        size_t durationIdx = 2; // index into {5,8,10,15,20,30} s
    };

    // Canonical duration option set for Anzeigedauer.
    inline const std::vector<std::string>& widgetDurationOptions()
    {
        static const std::vector<std::string> opts = {"5 s", "8 s", "10 s", "15 s", "20 s", "30 s"};
        return opts;
    }

    // Map a widget name to a menu-safe key suffix (keep alnum, others -> '_').
    inline std::string widgetKeySuffix(const std::string& name)
    {
        std::string out;
        out.reserve(name.size());
        for (char c : name)
        {
            if ((c >= 'a' && c <= 'z') || (c >= 'A' && c <= 'Z') || (c >= '0' && c <= '9'))
                out.push_back(c);
            else
                out.push_back('_');
        }
        return out;
    }

    inline MenuOption buildWidgetsRootItem(const std::vector<WidgetDesc>& widgets)
    {
        const MenuOption backOption = [] {
            MenuOption b;
            b.label = "Zurueck";
            b.type = MenuConfig::MenuElementType::Back;
            return b;
        }();

        MenuOption widgetsRoot;
        widgetsRoot.label = "Widgets";
        widgetsRoot.type = MenuConfig::MenuElementType::Submenu;
        widgetsRoot.devOnly = true;
        widgetsRoot.sortOrder = 40;

        for (const auto& w : widgets)
        {
            const std::string suffix = widgetKeySuffix(w.name);

            MenuOption show;
            show.label = "Anzeigen";
            show.type = MenuConfig::MenuElementType::Checkbox;
            show.key = "widget_show_" + suffix;
            show.defaultValue = MenuValue(w.enabled);
            show.toast = w.name; // which widget this row addresses

            MenuOption dur;
            dur.label = "Anzeigedauer";
            dur.type = MenuConfig::MenuElementType::Dropdown;
            dur.key = "widget_dur_" + suffix;
            dur.dropdownOptions = widgetDurationOptions();
            dur.defaultValue = MenuValue(static_cast<size_t>(w.durationIdx < 6 ? w.durationIdx : 2));
            dur.toast = w.name; // which widget this row addresses

            MenuOption widgetSub;
            widgetSub.label = w.name;
            widgetSub.type = MenuConfig::MenuElementType::Submenu;
            widgetSub.submenu = {show, dur, backOption};
            widgetsRoot.submenu.push_back(std::move(widgetSub));
        }

        // Single reorder entry for the whole rotation order.
        MenuOption reorder;
        reorder.label = "Reihenfolge";
        reorder.type = MenuConfig::MenuElementType::Reorder;
        reorder.key = "widgets_reorder";
        widgetsRoot.submenu.push_back(std::move(reorder));

        widgetsRoot.submenu.push_back(backOption);
        return widgetsRoot;
    }

} // namespace DefaultMenus