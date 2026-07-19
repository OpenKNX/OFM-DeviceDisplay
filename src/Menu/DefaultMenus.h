#pragma once

#include "MenuConfig.h"

// Rows are built with MenuBuild::addRow() straight into their final vector storage, never as
// stack locals — see the rationale above MenuBuild in MenuConfig.h. Shared option lists are
// handed out by reference for the same reason (an initializer list would land on the stack).
namespace DefaultMenus
{

    using MenuOption = MenuConfig::MenuOption;
    using MenuBuild::addRow;

    // Shared "Zurueck" row: built once, copied in where a submenu needs it.
    inline const MenuOption& backRow()
    {
        static const MenuOption back = [] {
            MenuOption b;
            b.label = "Zurueck";
            b.type = MenuConfig::MenuElementType::Back;
            return b;
        }();
        return back;
    }

    // --- Shared dropdown option lists -----------------------------------------
    // Built once and handed out by reference. As initializer lists they would land on the stack
    // (one std::string per entry) and re-allocate on every menu open.

    inline const std::vector<std::string>& brightnessOptions()
    {
        static const std::vector<std::string> opts = {"10%", "20%", "30%", "40%", "50%", "60%", "70%", "80%", "90%", "100%"};
        return opts;
    }

    // idx 0 = "nie" (no dim), 1..9 -> 10..90 %.
    inline const std::vector<std::string>& dimLevelOptions()
    {
        static const std::vector<std::string> opts = {"nie", "10%", "20%", "30%", "40%", "50%", "60%", "70%", "80%", "90%"};
        return opts;
    }

    // Order MUST match ScreenSaverType (value == index).
    inline const std::vector<std::string>& screensaverOptions()
    {
        static const std::vector<std::string> opts = {"Clock", "Cube 3D", "Doom", "FireWorks", "Life", "Matrix", "Matrix Cl.", "Pong", "Rain", "Starfield", "Aus"};
        return opts;
    }

    // Precharge / Refresh steps.
    inline const std::vector<std::string>& sixStepOptions()
    {
        static const std::vector<std::string> opts = {"1", "2", "3", "4", "5", "6"};
        return opts;
    }

    // Human-readable labels for the two SSD1315 tuning sliders (DeviceDisplay kRefreshBytes /
    // kPreChargeBytes). Refresh -> approx frame rate (~, internal RC osc has tolerance; ordering is
    // exact). Precharge -> pixel pre-charge time as a short..long scale (longer = brighter/sharper).
    inline const std::vector<std::string>& refreshOptions()
    {
        static const std::vector<std::string> opts = {"~70 Hz", "~80 Hz", "~90 Hz", "~100 Hz", "~120 Hz", "~150 Hz"};
        return opts;
    }
    inline const std::vector<std::string>& prechargeOptions()
    {
        static const std::vector<std::string> opts = {"1 kurz", "2", "3", "4", "5", "6 lang"};
        return opts;
    }

    // NumberEdit label for the 0 case.
    inline const std::vector<std::string>& offLabel()
    {
        static const std::vector<std::string> opts = {"aus"};
        return opts;
    }

    inline const std::vector<std::string>& neverLabel()
    {
        static const std::vector<std::string> opts = {"nie"};
        return opts;
    }

    // Index MUST match HomeKeyAction: 0=None 1=Pause 2=Reboot 3=Prog 4=DisplayOff 5=Screenshot.
    inline const std::vector<std::string>& homeKeyActions()
    {
        static const std::vector<std::string> opts = {"-", "Pause", "Reboot", "Prog-Mode", "Display aus", "Screenshot"};
        return opts;
    }

    inline const std::vector<std::string>& linkModeOptions()
    {
        static const std::vector<std::string> opts = {"Auto", "10 Mbit", "100 Mbit"};
        return opts;
    }

    // Canonical duration option set for Anzeigedauer.
    inline const std::vector<std::string>& widgetDurationOptions()
    {
        static const std::vector<std::string> opts = {"5 s", "8 s", "10 s", "15 s", "20 s", "30 s"};
        return opts;
    }

    // --- Legacy standalone menu (fallback when no MenuRegistry is handed in) ---

    // Shared "Back" row for the legacy English tree.
    inline const MenuOption& backRowEn()
    {
        static const MenuOption back = [] {
            MenuOption b;
            b.label = "Back";
            b.type = MenuConfig::MenuElementType::Back;
            return b;
        }();
        return back;
    }

    // Currently unreachable: the root below lists only advancedSettings/progMode/showDeviceInfo.
    // Kept (and never called, so the compiler emits nothing) for whoever re-enables the branch.
    MENU_NOINLINE inline void addMainSettingsRow(std::vector<MenuOption>& dst)
    {
        addRow(dst, [](MenuOption& mainSettings) {
            mainSettings.label = "Main Settings";
            mainSettings.type = MenuConfig::MenuElementType::Submenu;

            auto& sub = mainSettings.submenu;
            sub.reserve(4);

            // --- Network Settings ---
            addRow(sub, [](MenuOption& networkSettings) {
                networkSettings.label = "Network Settings";
                networkSettings.type = MenuConfig::MenuElementType::Submenu;

                auto& net = networkSettings.submenu;
                net.reserve(5);
                addRow(net, [](MenuOption& o) {
                    o.label = "DHCP";
                    o.type = MenuConfig::MenuElementType::Checkbox;
                    o.key = "dhcp_enabled";
                    o.defaultValue = MenuValue(true);
                });
                addRow(net, [](MenuOption& o) {
                    o.label = "IP Address";
                    o.type = MenuConfig::MenuElementType::Action;
                    o.visibleIf = std::make_pair("dhcp_enabled", MenuValue(false));
                });
                addRow(net, [](MenuOption& o) {
                    o.label = "Subnet Mask";
                    o.type = MenuConfig::MenuElementType::Action;
                    o.visibleIf = std::make_pair("dhcp_enabled", MenuValue(true));
                });
                addRow(net, [](MenuOption& dnsSettings) {
                    dnsSettings.label = "DNS Settings";
                    dnsSettings.type = MenuConfig::MenuElementType::Submenu;

                    auto& dns = dnsSettings.submenu;
                    dns.reserve(3);
                    addRow(dns, [](MenuOption& o) {
                        o.label = "Primary DNS";
                        o.type = MenuConfig::MenuElementType::Action;
                    });
                    addRow(dns, [](MenuOption& o) {
                        o.label = "Secondary DNS";
                        o.type = MenuConfig::MenuElementType::Action;
                    });
                    dns.push_back(backRowEn());
                });
                net.push_back(backRowEn());
            });

            // --- System Settings ---
            addRow(sub, [](MenuOption& systemSettings) {
                systemSettings.label = "System Settings";
                systemSettings.type = MenuConfig::MenuElementType::Submenu;

                auto& sys = systemSettings.submenu;
                sys.reserve(4);
                addRow(sys, [](MenuOption& o) {
                    o.label = "Time Zone";
                    o.type = MenuConfig::MenuElementType::Dropdown;
                    o.key = "timezone";
                    o.dropdownOptions = {"UTC-12", "UTC-11", "UTC+0", "UTC+1", "UTC+2"};
                    o.defaultValue = MenuValue(static_cast<size_t>(2));
                });
                addRow(sys, [](MenuOption& o) {
                    o.label = "Date/Time";
                    o.type = MenuConfig::MenuElementType::Action;
                });
                addRow(sys, [](MenuOption& o) {
                    o.label = "Reboot Device";
                    o.type = MenuConfig::MenuElementType::Action;
                    o.key = "reboot_device";
                });
                sys.push_back(backRowEn());
            });

            // --- Device Info ---
            addRow(sub, [](MenuOption& deviceInfo) {
                deviceInfo.label = "Device Info";
                deviceInfo.type = MenuConfig::MenuElementType::Submenu;

                auto& info = deviceInfo.submenu;
                info.reserve(4);
                addRow(info, [](MenuOption& o) {
                    o.label = "Device Name";
                    o.type = MenuConfig::MenuElementType::Action;
                });
                addRow(info, [](MenuOption& o) {
                    o.label = "Firmware Version";
                    o.type = MenuConfig::MenuElementType::Action;
                });
                addRow(info, [](MenuOption& o) {
                    o.label = "Serial Number";
                    o.type = MenuConfig::MenuElementType::Action;
                });
                info.push_back(backRowEn());
            });

            sub.push_back(backRowEn());
        });
    }

    MENU_NOINLINE inline void addAdvancedSettingsRow(std::vector<MenuOption>& dst)
    {
        addRow(dst, [](MenuOption& advancedSettings) {
            advancedSettings.label = "Advanced Settings";
            advancedSettings.type = MenuConfig::MenuElementType::Submenu;

            auto& sub = advancedSettings.submenu;
            sub.reserve(3);

            addRow(sub, [](MenuOption& display) {
                display.label = "Display";
                display.type = MenuConfig::MenuElementType::Submenu;

                auto& disp = display.submenu;
                disp.reserve(3);
                addRow(disp, [](MenuOption& o) {
                    o.label = "Brightness";
                    o.type = MenuConfig::MenuElementType::Dropdown;
                    o.key = "brightness_level";
                    o.dropdownOptions = {"25%", "50%", "75%", "100%"};
                    o.defaultValue = MenuValue(static_cast<size_t>(2));
                });
                // Minute value (0 = aus); dropdownOptions[0] labels the 0 case.
                addRow(disp, [](MenuOption& o) {
                    o.label = "Auto-Dim nach";
                    o.type = MenuConfig::MenuElementType::NumberEdit;
                    o.key = "dim_after";
                    o.dropdownOptions = offLabel();
                    o.defaultValue = MenuValue(static_cast<size_t>(1));
                });
                disp.push_back(backRowEn());
            });

            addRow(sub, [](MenuOption& o) {
                o.label = "Reboot";
                o.type = MenuConfig::MenuElementType::Action;
                o.key = "reboot_device";
            });

            sub.push_back(backRowEn());
        });
    }

    inline MenuOption buildMenu()
    {
        MenuOption rootMenu;
        rootMenu.label = "Root";
        rootMenu.type = MenuConfig::MenuElementType::Submenu;

        auto& sub = rootMenu.submenu;
        sub.reserve(3);

        // addMainSettingsRow(sub); // disabled: legacy Main Settings branch
        addAdvancedSettingsRow(sub);

        // --- Prog Mode ---
        addRow(sub, [](MenuOption& o) {
            o.label = "Prog-Mode";
            o.type = MenuConfig::MenuElementType::Action;
            o.key = "prog_mode";
        });

        // --- Show Device Info Overlay ---
        addRow(sub, [](MenuOption& o) {
            o.label = "Show Device Info";
            o.type = MenuConfig::MenuElementType::Action;
            o.key = "show_device_info_overlay";
        });

        return rootMenu;
    }

    // --- Display-owned root menu entries --------------------------------------
    // About is pinned last by MenuRegistry::pinAboutLast(); sortOrder leaves gaps so module
    // roots slot in between.

    MENU_NOINLINE inline void addSysInfoRoot(std::vector<MenuOption>& roots)
    {
        addRow(roots, [](MenuOption& sysInfo) {
            sysInfo.label = "System-Info";
            sysInfo.type = MenuConfig::MenuElementType::Submenu;
            sysInfo.sortOrder = 10;

            auto& sub = sysInfo.submenu;
            sub.reserve(7);

            // Each read-only row pulls its live value via valueProvider.
            addRow(sub, [](MenuOption& o) {
                o.label = "Geraet";
                o.type = MenuConfig::MenuElementType::Readonly;
                o.valueProvider = []() -> std::string { return std::string(openknx.info.firmwareName()); };
            });
            addRow(sub, [](MenuOption& o) {
                o.label = "Firmware";
                o.type = MenuConfig::MenuElementType::Readonly;
                o.valueProvider = []() -> std::string { return std::string(openknx.info.humanFirmwareVersion()); };
            });
            addRow(sub, [](MenuOption& o) {
                o.label = "Seriennummer";
                o.type = MenuConfig::MenuElementType::Readonly;
                o.valueProvider = []() -> std::string { return std::string(openknx.info.humanSerialNumber()); };
            });
            addRow(sub, [](MenuOption& o) {
                o.label = "KNX-Typ";
                o.type = MenuConfig::MenuElementType::Readonly;
                o.valueProvider = []() -> std::string {
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
            });
            addRow(sub, [](MenuOption& o) {
                o.label = "BCU-Status";
                o.type = MenuConfig::MenuElementType::Readonly;
                o.valueProvider = []() -> std::string { return knx.configured() ? std::string("OK") : std::string("-"); };
            });
            addRow(sub, [](MenuOption& o) {
                o.label = "Freier Speicher";
                o.type = MenuConfig::MenuElementType::Readonly;
                o.valueProvider = []() -> std::string {
                    char b[16];
                    snprintf(b, sizeof(b), "%.1f KiB", (float)freeMemory() / 1024.0f);
                    return b;
                };
            });
            sub.push_back(backRow());
        });
    }

    // Selectable "Beenden" exit near the bottom of the root menu; About stays pinned last.
    MENU_NOINLINE inline void addExitRoot(std::vector<MenuOption>& roots)
    {
        addRow(roots, [](MenuOption& o) {
            o.label = "< Beenden";
            o.type = MenuConfig::MenuElementType::Back;
            o.sortOrder = 900;
        });
    }

    // Display (hardware tuning) submenu: LIVE preview, saved ONLY via "Speichern". A bad value
    // never auto-persists (would brick the panel); the KONAMI code restores defaults.
    MENU_NOINLINE inline void addDisplayTuningRow(std::vector<MenuOption>& dst)
    {
        addRow(dst, [](MenuOption& displaySub) {
            displaySub.label = "Display";
            displaySub.type = MenuConfig::MenuElementType::Submenu;

            auto& sub = displaySub.submenu;
            sub.reserve(5);
            addRow(sub, [](MenuOption& o) {
                o.label = "180 Grad drehen";
                o.type = MenuConfig::MenuElementType::Checkbox;
                o.key = "disp_rotate";
                o.defaultValue = MenuValue(false);
            });
            addRow(sub, [](MenuOption& o) {
                o.label = "Precharge";
                o.type = MenuConfig::MenuElementType::Dropdown;
                o.key = "disp_precharge";
                o.dropdownOptions = sixStepOptions();
                o.defaultValue = MenuValue(static_cast<size_t>(5)); // 0xF1
                o.slider = true;
            });
            addRow(sub, [](MenuOption& o) {
                o.label = "Refresh";
                o.type = MenuConfig::MenuElementType::Dropdown;
                o.key = "disp_refresh";
                o.dropdownOptions = sixStepOptions();
                o.defaultValue = MenuValue(static_cast<size_t>(3)); // 0x80
                o.slider = true;
            });
            addRow(sub, [](MenuOption& o) {
                o.label = "Speichern";
                o.type = MenuConfig::MenuElementType::Action;
                o.key = "disp_save";
                o.confirmText = "Anzeige OK? Speichern?";
            });
            sub.push_back(backRow());
        });
    }

    // --- Anzeige (display settings) -------------------------------------------
    // Keys must match the onValueChanged registrations in DeviceDisplay or the wiring no-ops.
    MENU_NOINLINE inline void addAnzeigeRoot(std::vector<MenuOption>& roots)
    {
        addRow(roots, [](MenuOption& anzeige) {
            anzeige.label = "Anzeige";
            anzeige.type = MenuConfig::MenuElementType::Submenu;
            anzeige.sortOrder = 30; // visible for all (reset + KONAMI + manual-save protect the tunings)

            auto& sub = anzeige.submenu;
            sub.reserve(12);

            addRow(sub, [](MenuOption& o) {
                o.label = "Helligkeit";
                o.type = MenuConfig::MenuElementType::Dropdown;
                o.key = "brightness_level";
                o.dropdownOptions = brightnessOptions();
                o.defaultValue = MenuValue(static_cast<size_t>(9)); // 100%
                o.slider = true;                                    // adjust via a horizontal slider (live)
            });
            // Timeouts are 3-digit minute values (0 = aus/nie); dropdownOptions[0] is the label for 0.
            addRow(sub, [](MenuOption& o) {
                o.label = "Auto-Dim nach";
                o.type = MenuConfig::MenuElementType::NumberEdit;
                o.key = "dim_after";
                o.dropdownOptions = offLabel();
                o.defaultValue = MenuValue(static_cast<size_t>(15));
            });
            // Effective dim = min(level, Helligkeit).
            addRow(sub, [](MenuOption& o) {
                o.label = "Dim-Level";
                o.type = MenuConfig::MenuElementType::Dropdown;
                o.key = "dim_level";
                o.dropdownOptions = dimLevelOptions();
                o.defaultValue = MenuValue(static_cast<size_t>(3)); // 30%
                o.slider = true;
            });
            addRow(sub, [](MenuOption& o) {
                o.label = "Invertieren";
                o.type = MenuConfig::MenuElementType::Checkbox;
                o.key = "display_invert";
                o.defaultValue = MenuValue(false);
            });
            addRow(sub, [](MenuOption& o) {
                o.label = "Seiten auto-blaettern";
                o.type = MenuConfig::MenuElementType::Checkbox;
                o.key = "auto_paging";
                o.defaultValue = MenuValue(true);
            });
            addRow(sub, [](MenuOption& o) {
                o.label = "Icon-Menue";
                o.type = MenuConfig::MenuElementType::Checkbox;
                o.key = "icon_menu"; // root icon grid vs text list
                o.defaultValue = MenuValue(false);
            });
            addRow(sub, [](MenuOption& o) {
                o.label = "Bildschirmschoner";
                o.type = MenuConfig::MenuElementType::Dropdown;
                o.key = "screensaver_type";
                o.dropdownOptions = screensaverOptions();
                o.defaultValue = MenuValue(static_cast<size_t>(5)); // Matrix
                o.radioList = true;                                 // open as a full-screen single-select picker
            });
            addRow(sub, [](MenuOption& o) {
                o.label = "Screensaver nach";
                o.type = MenuConfig::MenuElementType::NumberEdit;
                o.key = "screensaver_after";
                o.dropdownOptions = offLabel();
                o.defaultValue = MenuValue(static_cast<size_t>(30));
            });
            addRow(sub, [](MenuOption& o) {
                o.label = "Display aus nach";
                o.type = MenuConfig::MenuElementType::NumberEdit;
                o.key = "sleep_after";
                o.dropdownOptions = neverLabel();
                o.defaultValue = MenuValue(static_cast<size_t>(60));
            });

            addDisplayTuningRow(sub);

            // Reset all display settings to defaults (Nein/Ja guard). Wired in DeviceDisplay.
            addRow(sub, [](MenuOption& o) {
                o.label = "Zuruecksetzen";
                o.type = MenuConfig::MenuElementType::Action;
                o.key = "display_reset";
                o.confirmText = "Alle Anzeige-Werte auf Standard?";
            });

            sub.push_back(backRow());
        });
    }

    // --- System (Prog-Mode, reboot, factory reset) ----------------------------
    MENU_NOINLINE inline void addSystemRoot(std::vector<MenuOption>& roots)
    {
        addRow(roots, [](MenuOption& system) {
            system.label = "System";
            system.type = MenuConfig::MenuElementType::Submenu;
            system.sortOrder = 50;

            auto& sub = system.submenu;
            sub.reserve(4);
            addRow(sub, [](MenuOption& o) {
                o.label = "Prog-Mode";
                o.type = MenuConfig::MenuElementType::ProgToggle;
                o.key = "prog_mode";
            });
            addRow(sub, [](MenuOption& o) {
                o.label = "Neustart";
                o.type = MenuConfig::MenuElementType::Reboot;
                o.key = "reboot_device";
            });
            addRow(sub, [](MenuOption& o) {
                o.label = "Werkseinstellungen";
                o.type = MenuConfig::MenuElementType::Toast;
                o.key = "factory_reset";
                o.toast = "Werksreset ...";
            });
            sub.push_back(backRow());
        });
    }

    // --- Screenshot (RIGHT-hold / "ddc screenshot" -> 1-bit BMP on SD) ---------
    MENU_NOINLINE inline void addScreenshotRoot(std::vector<MenuOption>& roots)
    {
        addRow(roots, [](MenuOption& screenshot) {
            screenshot.label = "Screenshot";
            screenshot.type = MenuConfig::MenuElementType::Submenu;
            screenshot.devOnly = true;
            screenshot.sortOrder = 55;

            auto& sub = screenshot.submenu;
            sub.reserve(2);
            addRow(sub, [](MenuOption& o) {
                o.label = "Invertieren";
                o.type = MenuConfig::MenuElementType::Checkbox;
                o.key = "screenshot_invert";
                o.defaultValue = MenuValue(false); // OLED look by default (lit pixels white)
            });
            sub.push_back(backRow());
        });
    }

    // --- Home-Tasten (assignable home-screen keys) ----------------------------
    MENU_NOINLINE inline void addHomeKeysRoot(std::vector<MenuOption>& roots)
    {
        addRow(roots, [](MenuOption& homeKeys) {
            homeKeys.label = "Home-Tasten";
            homeKeys.type = MenuConfig::MenuElementType::Submenu;
            homeKeys.sortOrder = 60;

            auto& sub = homeKeys.submenu;
            sub.reserve(6);

            // Defaults: Pause / Reboot / none / Screenshot.
            const struct
            {
                const char* label;
                const char* key;
                size_t defaultIdx;
            } keys[] = {
                {"Oben", "homekey_up", 1},
                {"Unten", "homekey_down", 2},
                {"Links", "homekey_left", 0},
                {"Rechts", "homekey_right", 5},
            };

            for (const auto& k : keys)
            {
                addRow(sub, [&k](MenuOption& o) {
                    o.label = k.label;
                    o.type = MenuConfig::MenuElementType::Dropdown;
                    o.key = k.key;
                    o.dropdownOptions = homeKeyActions();
                    o.defaultValue = MenuValue(k.defaultIdx);
                });
            }

            addRow(sub, [](MenuOption& o) {
                o.label = "OK halten";
                o.type = MenuConfig::MenuElementType::Readonly;
            });
            sub.push_back(backRow());
        });
    }

    inline std::vector<MenuOption> buildDisplayRootItems()
    {
        std::vector<MenuOption> roots;
        // 6 built here + Netzwerk/Widgets appended by the caller. MenuOption move is not
        // guaranteed noexcept, so a regrow would deep-copy the whole tree.
        roots.reserve(8);

        addSysInfoRoot(roots);
        addExitRoot(roots);
        addAnzeigeRoot(roots);
        addSystemRoot(roots);
        addScreenshotRoot(roots);
        addHomeKeysRoot(roots);

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

    // IP rows stay always visible; when DHCP is on they render "auto" and are not editable.
    MENU_NOINLINE inline void addIpRow(std::vector<MenuOption>& dst, const NetSnapshot& snap,
                                     const char* label, const char* key, const uint8_t src[4], uint8_t liveIdx)
    {
        addRow(dst, [&](MenuOption& o) {
            o.label = label;
            o.type = MenuConfig::MenuElementType::IpAddress;
            o.key = key;
            o.ip[0] = src[0];
            o.ip[1] = src[1];
            o.ip[2] = src[2];
            o.ip[3] = src[3];
            if (snap.liveValue)
            {
                auto fn = snap.liveValue;
                o.valueProvider = [fn, liveIdx]() { return fn(liveIdx); };
            }
        });
    }

    inline MenuOption buildNetworkRootItem(const NetSnapshot& snap)
    {
        MenuOption netz;
        netz.label = "Netzwerk";
        netz.type = MenuConfig::MenuElementType::Submenu;
        netz.devOnly = true;
        netz.sortOrder = 20;

        auto& sub = netz.submenu;
        sub.reserve(8);

        addRow(sub, [&snap](MenuOption& o) {
            o.label = "DHCP";
            o.type = MenuConfig::MenuElementType::Checkbox;
            o.key = "net_dhcp";
            o.defaultValue = MenuValue(snap.dhcp);
        });

        addIpRow(sub, snap, "IP-Adresse", "net_ip", snap.ip, 0);
        addIpRow(sub, snap, "Subnetzmaske", "net_subnet", snap.subnet, 1);
        addIpRow(sub, snap, "Gateway", "net_gw", snap.gateway, 2);
        addIpRow(sub, snap, "DNS-Server", "net_dns", snap.dns, 3);

        addRow(sub, [&snap](MenuOption& o) {
            o.label = "Verbindung";
            o.type = MenuConfig::MenuElementType::Dropdown;
            o.key = "net_link";
            o.dropdownOptions = linkModeOptions();
            o.defaultValue = MenuValue(static_cast<size_t>(snap.linkMode < 3 ? snap.linkMode : 0));
        });

        // "Uebernehmen": commits the staged DHCP/IP override live (net_apply action, wired in Menu.cpp).
        addRow(sub, [](MenuOption& o) {
            o.label = "Uebernehmen";
            o.type = MenuConfig::MenuElementType::Action;
            o.key = "net_apply";
        });

        sub.push_back(backRow());
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

    MENU_NOINLINE inline void addWidgetRow(std::vector<MenuOption>& dst, const WidgetDesc& w)
    {
        addRow(dst, [&w](MenuOption& widgetSub) {
            widgetSub.label = w.name;
            widgetSub.type = MenuConfig::MenuElementType::Submenu;

            const std::string suffix = widgetKeySuffix(w.name);

            auto& sub = widgetSub.submenu;
            sub.reserve(3);
            addRow(sub, [&w, &suffix](MenuOption& o) {
                o.label = "Anzeigen";
                o.type = MenuConfig::MenuElementType::Checkbox;
                o.key = "widget_show_" + suffix;
                o.defaultValue = MenuValue(w.enabled);
                o.toast = w.name; // which widget this row addresses
            });
            addRow(sub, [&w, &suffix](MenuOption& o) {
                o.label = "Anzeigedauer";
                o.type = MenuConfig::MenuElementType::Dropdown;
                o.key = "widget_dur_" + suffix;
                o.dropdownOptions = widgetDurationOptions();
                o.defaultValue = MenuValue(static_cast<size_t>(w.durationIdx < 6 ? w.durationIdx : 2));
                o.toast = w.name; // which widget this row addresses
            });
            sub.push_back(backRow());
        });
    }

    inline MenuOption buildWidgetsRootItem(const std::vector<WidgetDesc>& widgets)
    {
        MenuOption widgetsRoot;
        widgetsRoot.label = "Widgets";
        widgetsRoot.type = MenuConfig::MenuElementType::Submenu;
        widgetsRoot.devOnly = true;
        widgetsRoot.sortOrder = 40;

        auto& sub = widgetsRoot.submenu;
        sub.reserve(widgets.size() + 2); // + Reihenfolge + Zurueck

        for (const auto& w : widgets)
            addWidgetRow(sub, w);

        // Single reorder entry for the whole rotation order.
        addRow(sub, [](MenuOption& o) {
            o.label = "Reihenfolge";
            o.type = MenuConfig::MenuElementType::Reorder;
            o.key = "widgets_reorder";
        });

        sub.push_back(backRow());
        return widgetsRoot;
    }

} // namespace DefaultMenus
