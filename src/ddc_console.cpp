
#if defined(DEVICE_DISPLAY_MODULE) && !defined(DDC_CONSOLE_DISABLE)
    #include "ddc_console.h"
    #include "DeviceDisplay.h"            // openknxDisplayModule + settingsStore() (ddc i "Menu Settings")
    #include "Menu/MenuRegistry.h"        // ddc i "Menu Config": registry tree + dynamic module items
    #include "OpenKNX/Helper.h"           // freeMemory() (cross-platform, ddc i "Memory")
    #include "OpenKNX/I2C/Wire1Lock.h"    // OPENKNX_WIRE1_LOCK for the raw scroll ssd1306_command block (no-op on RP2040)
    #include "Settings/DisplaySettings.h" // DisplaySettings / WidgetSetting / HomeKey* enums
    #include "Settings/DisplaySettingsStore.h"
    #include "WidgetsManager.h"
    #include "devices/i2cDisplay.h"
    #include <cstdlib> // strtol for "ddc config set"

    #include "Widget.h"
    #include "Widgets/Clock.h"
    #include "Widgets/Cube3D.h"
    #include "Widgets/Fireworks.h"
    #include "Widgets/Life.h"
    #include "Widgets/Matrix.h"
    #include "Widgets/Pong.h"
    #include "Widgets/QRcode.h"
    #include "Widgets/Rain.h"
    #include "Widgets/Starfield.h"
    #include "Widgets/SysInfoLite.h"
    #ifdef WIDGET_CONSOLE
        #include "Widgets/Console.h"
    #endif

// Approximate frame rate (Hz) per refresh preset idx (0..5), see DeviceDisplay kRefreshBytes.
// "~" on purpose: the SSD1315 oscillator is an internal RC (chip tolerance); only the ordering
// slow->fast is exact. Purely for a human-readable readout.
static inline uint16_t ddcRefreshHz(uint8_t idx)
{
    static const uint16_t hz[6] = {70, 80, 90, 100, 120, 150};
    return hz[idx < 6 ? idx : 5];
}

DdcConsole::DdcConsole(WidgetsManager* widgetManager, i2cDisplay* displayModule)
    : _widgetManager(widgetManager), _displayModule(displayModule)
{
}

DdcConsole::~DdcConsole()
{
}

void DdcConsole::setup()
{
    registerWidgetCommands();
    logInfoP("Initialized (%d widgets)", _widgetCommands.size());
}

/**********************************************************************
 *********************** WIDGET REGISTRY ******************************
 **********************************************************************/
/**
 * @brief Register available widget commands
 */
void DdcConsole::registerWidgetCommands()
{
    _widgetCommands = {
        {"clock", "Clock", []() -> Widget* {
             return new WidgetClock(5000, WidgetFlags::DefaultWidget, false);
         }},
        {"matrix", "Matrix", []() -> Widget* {
             return new WidgetMatrix(5000, WidgetFlags::DefaultWidget, 7);
         }},
        {"pong", "Pong", []() -> Widget* {
             return new WidgetPong(5000, WidgetFlags::DefaultWidget);
         }},
        {"starfield", "Starfield", []() -> Widget* {
             return new WidgetStarfield(5000, WidgetFlags::DefaultWidget, 10);
         }},
        {"3dcube", "Cube3D", []() -> Widget* {
             return new WidgetCube3D(5000, WidgetFlags::DefaultWidget);
         }},
        {"life", "Life", []() -> Widget* {
             return new WidgetLife(5000, WidgetFlags::DefaultWidget);
         }},
        {"rain", "Rain", []() -> Widget* {
             return new WidgetRain(5000, WidgetFlags::DefaultWidget, 6);
         }},
        {"fireworks", "Fireworks", []() -> Widget* {
             return new WidgetFireworks(10000, WidgetFlags::DefaultWidget, 10);
         }},
        {"sysinfo", "SysInfoLite", []() -> Widget* {
             return new WidgetSysInfoLite(5000, WidgetFlags::DefaultWidget);
         }}};
}

/**********************************************************************
 ************************ COMMAND PROCESSING **************************
 **********************************************************************/
/**
 * @brief Process a console command
 */
bool DdcConsole::processCommand(const std::string& command)
{
    if (command.compare(0, 4, "ddc ") != 0) return false;

    // Route to sub-handlers
    if (command.compare(4, 1, "l") == 0 && command.size() < 6)
        return processListCommand();

    if (command.compare(4, 1, "i") == 0 && command.size() < 6)
        return processInfoCommand();

    // "ddc screenshot": capture the CURRENT frame and save a BMP to SD (test hook for the gesture
    // path). requestScreenshot() only latches; the write runs from DeviceDisplay::loop().
    // "ddc screenshot ascii": render the live framebuffer to the console instead (no SD).
    if (command.compare(4, 10, "screenshot") == 0)
    {
        if (command.find("ascii") != std::string::npos)
        {
            logFramebufferAscii();
            return true;
        }
        if (command.find("b64") != std::string::npos || command.find("base64") != std::string::npos)
        {
            logFramebufferBase64();
            return true;
        }
        openknxDisplayModule.requestScreenshot(true);
        logInfoP("Screenshot requested (writing to SD from loop)");
        return true;
    }

    // "ddc home": force a known UI state. Test harnesses need a reset that does not depend on
    // where the UI currently is.
    if (command.compare(4, 4, "home") == 0)
    {
        openknxDisplayModule.forceHome();
        return true;
    }

    if (command.compare(4, 3, "key") == 0)
        return processKeyCommand(command);

    if (command.compare(4, 6, "config") == 0)
        return processConfigCommand(command);

    if (command.compare(4, 3, "qr ") == 0)
        return processQRCommand(command);

    #ifdef WIDGET_CONSOLE
    if (command.compare(4, 2, "c ") == 0)
        return processConsoleCommand(command);
    #endif

    if (isWidgetCommand(command))
        return processWidgetCommand(command);

    #ifdef DISPLAY_LOW_LEVEL_COMMANDS
    if (isLowLevelCommand(command))
        return processLowLevelCommand(command);
    #endif

    // Unknown command → show help
    showHelp();
    return true;
}

/**********************************************************************
 *********************** COMMAND HANDLERS *****************************
 **********************************************************************/
/**
 * @brief List all widgets currently in the manager queue.
 */
bool DdcConsole::processListCommand()
{
    _widgetManager->logWidgetQueue();
    return true;
}

/**
 * @brief Process info command: widget-manager settings + persisted menu settings + memory.
 */
bool DdcConsole::processInfoCommand()
{
    _widgetManager->logWidgetManagerSettings();
    logMenuSettings();
    logMenuConfig();
    logMemoryInfo();
    return true;
}

// "ddc config" -> show table; "ddc config reset" -> defaults; "ddc config set <id> <value>".
bool DdcConsole::processConfigCommand(const std::string& command)
{
    std::string rest = command.size() > 10 ? command.substr(10) : ""; // after "ddc config"
    const size_t s = rest.find_first_not_of(' ');
    if (s == std::string::npos)
    {
        logConfig();
        return true;
    }
    rest = rest.substr(s);

    if (rest == "reset")
    {
        openknxDisplayModule.consoleResetToDefaults();
        logInfoP("config reset -> defaults (applied + saved)");
        return true;
    }
    if (rest.compare(0, 4, "set ") == 0)
        return processConfigSet(rest.substr(4));

    logConfig();
    return true;
}

// Parse "<id> <value>" and apply+persist a single setting. Values clamp to their valid range.
bool DdcConsole::processConfigSet(const std::string& args)
{
    const size_t sp = args.find(' ');
    if (sp == std::string::npos)
    {
        logInfoP("Usage: ddc config set <id> <value>   (see 'ddc config' for ids)");
        return true;
    }
    const std::string id = args.substr(0, sp);
    const long v = strtol(args.c_str() + sp + 1, nullptr, 0);
    auto clamp = [](long x, long lo, long hi) -> long { return x < lo ? lo : (x > hi ? hi : x); };
    DisplaySettingsStore& store = openknxDisplayModule.getSettingsStore();

    bool ok = true;
    if (id == "brightness") store.setBrightnessIdx(static_cast<uint8_t>(clamp(v, 0, 9)));
    else if (id == "dim_after") store.setDimMin(static_cast<uint16_t>(clamp(v, 0, 999)));
    else if (id == "dim_level") store.setDimLevelIdx(static_cast<uint8_t>(clamp(v, 0, 9)));
    else if (id == "invert") store.setInvert(v != 0);
    else if (id == "auto_paging") store.setAutoPaging(v != 0);
    else if (id == "icon_menu") store.setIconMenu(v != 0);
    else if (id == "screensaver_type") store.setScreenSaverType(static_cast<uint8_t>(clamp(v, 0, 10)));
    else if (id == "screensaver_after") store.setScreenSaverMin(static_cast<uint16_t>(clamp(v, 0, 999)));
    else if (id == "sleep_after") store.setSleepMin(static_cast<uint16_t>(clamp(v, 0, 999)));
    else if (id == "rotate") store.setDisplayRotate(v != 0);
    else if (id == "precharge") store.setPreChargeIdx(static_cast<uint8_t>(clamp(v, 0, 5)));
    else if (id == "refresh") store.setRefreshIdx(static_cast<uint8_t>(clamp(v, 0, 5)));
    else if (id == "screenshot_invert") store.setScreenshotInvert(v != 0);
    else ok = false;

    if (!ok)
    {
        logErrorP("unknown config id '%s' (see 'ddc config')", id.c_str());
        return true;
    }
    openknxDisplayModule.consoleApplyAndSave();
    logInfoP("config set %s = %ld (applied + saved)", id.c_str(), v);
    return true;
}

// id | value table of every persisted display setting (ids match 'ddc config set').
void DdcConsole::logConfig()
{
    const DisplaySettingsStore& store = openknxDisplayModule.settingsStore();
    openknx.logger.log("");
    openknx.logger.color(CONSOLE_HEADLINE_COLOR);
    openknx.logger.log("=======================================================");
    openknx.logger.log("=              Display Config (id | value)            =");
    openknx.logger.log("=======================================================");
    openknx.logger.color(0);
    openknx.logger.logWithValues(" brightness         : %u   (0..9 = 10..100)", store.brightnessIdx());
    openknx.logger.logWithValues(" dim_after          : %u   min (0 = aus)", store.dimMin());
    openknx.logger.logWithValues(" dim_level          : %u   (0 = nie, 1..9 = 10..90)", store.dimLevelIdx());
    openknx.logger.logWithValues(" invert             : %u", store.invert() ? 1 : 0);
    openknx.logger.logWithValues(" auto_paging        : %u", store.autoPaging() ? 1 : 0);
    openknx.logger.logWithValues(" icon_menu          : %u", store.iconMenu() ? 1 : 0);
    openknx.logger.logWithValues(" screensaver_type   : %u   (0..10)", store.screenSaverType());
    openknx.logger.logWithValues(" screensaver_after  : %u   min (0 = aus)", store.screenSaverMin());
    openknx.logger.logWithValues(" sleep_after        : %u   min (0 = nie)", store.sleepMin());
    openknx.logger.logWithValues(" rotate             : %u   (180 deg)", store.displayRotate() ? 1 : 0);
    openknx.logger.logWithValues(" precharge          : %u   (0..5, kurz->lang)", store.preChargeIdx());
    openknx.logger.logWithValues(" refresh            : %u   (~%u Hz, 0..5 langsam->schnell)",
                                 store.refreshIdx(), ddcRefreshHz(store.refreshIdx()));
    openknx.logger.logWithValues(" screenshot_invert  : %u", store.screenshotInvert() ? 1 : 0);
    openknx.logger.color(CONSOLE_HEADLINE_COLOR);
    openknx.logger.log("-------------------------------------------------------");
    openknx.logger.color(0);
    openknx.logger.log(" ddc config set <id> <value>   |   ddc config reset");
    openknx.logger.log("=======================================================");
}

namespace
{
    // Label for the persisted screensaver type. Order MUST match the enum / DefaultMenus dropdown.
    const char* screenSaverTypeLabel(uint8_t type)
    {
        static const char* const kLabels[] = {
            "Clock", "Cube3D", "Doom", "FireWorks", "Life", "Matrix",
            "MatrixCl.", "Pong", "Rain", "Starfield", "Aus"};
        if (type < (sizeof(kLabels) / sizeof(kLabels[0]))) return kLabels[type];
        return "?";
    }

    // Label for a Home-key action (HomeKeyAction enum).
    const char* homeKeyActionLabel(HomeKeyAction action)
    {
        switch (action)
        {
            case HomeKeyAction::None: return "-";
            case HomeKeyAction::Pause: return "Pause";
            case HomeKeyAction::Reboot: return "Reboot";
            case HomeKeyAction::Prog: return "Prog-Mode";
            case HomeKeyAction::DisplayOff: return "Display aus";
            case HomeKeyAction::Screenshot: return "Screenshot";
            default: return "?";
        }
    }

    // Short label for a menu element type (ddc i "Menu Config").
    const char* menuTypeLabel(MenuConfig::MenuElementType t)
    {
        switch (t)
        {
            case MenuConfig::Checkbox: return "Checkbox";
            case MenuConfig::TextInput: return "Text";
            case MenuConfig::Dropdown: return "Dropdown";
            case MenuConfig::Action: return "Action";
            case MenuConfig::Submenu: return "Submenu";
            case MenuConfig::Back: return "Back";
            case MenuConfig::Readonly: return "Readonly";
            case MenuConfig::IpEdit: return "IpEdit";
            case MenuConfig::IpAddress: return "IpAddr";
            case MenuConfig::Reorder: return "Reorder";
            case MenuConfig::Toast: return "Toast";
            case MenuConfig::ProgToggle: return "ProgTgl";
            case MenuConfig::Reboot: return "Reboot";
            case MenuConfig::Files: return "Files";
            case MenuConfig::About: return "About";
            default: return "?";
        }
    }

    // Current value of an option, provider-first so dynamic/live items show their runtime state.
    // GCC 14 emits a bogus -Wfree-nonheap-object when this SSO-string helper is inlined into
    // dumpMenuOptions (known optimizer false positive; the code is correct). noinline breaks the
    // triggering inline, and the pragma below suppresses the diagnostic for good measure.
#if defined(__GNUC__) && !defined(__clang__)
    #pragma GCC diagnostic push
    #pragma GCC diagnostic ignored "-Wfree-nonheap-object"
#endif
    __attribute__((noinline)) std::string menuOptionValue(const MenuConfig::MenuOption& o)
    {
        if (o.type == MenuConfig::Readonly && o.valueProvider) return o.valueProvider();
        if (o.type == MenuConfig::IpEdit || o.type == MenuConfig::IpAddress)
        {
            char buf[16];
            snprintf(buf, sizeof(buf), "%u.%u.%u.%u", o.ip[0], o.ip[1], o.ip[2], o.ip[3]);
            return buf;
        }
        if (o.type == MenuConfig::Dropdown)
        {
            const size_t idx = o.radioIndexProvider ? o.radioIndexProvider() : o.defaultValue.getSizeT();
            std::string s = "idx=";
            s += std::to_string(idx);
            if (idx < o.dropdownOptions.size())
            {
                s += " (";
                s += o.dropdownOptions[idx];
                s += ")";
            }
            return s;
        }
        switch (o.defaultValue.getType())
        {
            case MenuValue::Boolean: return o.defaultValue.getBool() ? "[x]" : "[ ]";
            case MenuValue::SizeT: return std::to_string(o.defaultValue.getSizeT());
            case MenuValue::String: return o.defaultValue.getString();
            default: return "";
        }
    }

    // Recursively dump the menu tree. Eager submenus recurse; a lazy submenuBuilder is expanded once
    // so dynamically-built children (per-widget subs, live network IPs) are visible too.
    void dumpMenuOptions(const std::vector<MenuConfig::MenuOption>& opts, uint8_t depth)
    {
        if (depth > 6) return; // guard against a pathological tree
        const std::string indent(static_cast<size_t>(depth) * 2, ' ');
        for (const MenuConfig::MenuOption& o : opts)
        {
            std::string line = " " + indent + "[" + menuTypeLabel(o.type) + "] " + o.label;
            if (!o.key.empty()) line += " {" + o.key + "}";
            const std::string val = menuOptionValue(o);
            if (!val.empty()) line += " = " + val;
            if (o.devOnly) line += "  (dev)";
            openknx.logger.log(line);

            if (!o.submenu.empty())
                dumpMenuOptions(o.submenu, depth + 1);
            else if (o.submenuBuilder)
            {
                openknx.logger.log(" " + indent + "  (lazy submenu:)");
                dumpMenuOptions(o.submenuBuilder(), depth + 1);
            }
        }
    }
#if defined(__GNUC__) && !defined(__clang__)
    #pragma GCC diagnostic pop
#endif
} // namespace

/**
 * @brief "ddc i" section: dump every menu-set value from the DisplaySettingsStore.
 */
void DdcConsole::logMenuSettings()
{
    const DisplaySettingsStore& store = openknxDisplayModule.settingsStore();

    openknx.logger.begin();
    openknx.logger.log("");
    openknx.logger.color(CONSOLE_HEADLINE_COLOR);
    openknx.logger.log("=======================================================");
    openknx.logger.log("=                   Menu Settings                     =");
    openknx.logger.log("=======================================================");
    openknx.logger.color(0);
    openknx.logger.logWithValues(" Brightness idx       : %u (-> %u%%)", store.brightnessIdx(),
                                 (unsigned)((store.brightnessIdx() + 1) * 10));
    openknx.logger.logWithValues(" Dim after (min)      : %u", store.dimMin());
    openknx.logger.logWithValues(" Dim level idx        : %u (0=nie)", store.dimLevelIdx());
    openknx.logger.logWithValues(" Invert               : %-3s", store.invert() ? "yes" : "no");
    openknx.logger.logWithValues(" Auto-Paging          : %-3s", store.autoPaging() ? "yes" : "no");
    openknx.logger.logWithValues(" Screensaver type     : %u (%s)", store.screenSaverType(),
                                 screenSaverTypeLabel(store.screenSaverType()));
    openknx.logger.logWithValues(" Screensaver (min)    : %u", store.screenSaverMin());
    openknx.logger.logWithValues(" Display off (min)    : %u", store.sleepMin());
    openknx.logger.logWithValues(" Icon menu            : %-3s", store.iconMenu() ? "yes" : "no");
    openknx.logger.logWithValues(" Display rotate 180   : %-3s", store.displayRotate() ? "yes" : "no");
    openknx.logger.logWithValues(" Precharge idx        : %u (0..5, kurz->lang)", store.preChargeIdx());
    openknx.logger.logWithValues(" Refresh idx          : %u (~%u Hz, langsam->schnell)",
                                 store.refreshIdx(), ddcRefreshHz(store.refreshIdx()));
    openknx.logger.color(CONSOLE_HEADLINE_COLOR);
    openknx.logger.log("-------------------------------------------------------");
    openknx.logger.color(0);
    openknx.logger.log(" Home-key actions:");
    openknx.logger.logWithValues("   - Up          : %u (%s)", (unsigned)store.keyAction(HOME_KEY_UP),
                                 homeKeyActionLabel(store.keyAction(HOME_KEY_UP)));
    openknx.logger.logWithValues("   - Down        : %u (%s)", (unsigned)store.keyAction(HOME_KEY_DOWN),
                                 homeKeyActionLabel(store.keyAction(HOME_KEY_DOWN)));
    openknx.logger.logWithValues("   - Left        : %u (%s)", (unsigned)store.keyAction(HOME_KEY_LEFT),
                                 homeKeyActionLabel(store.keyAction(HOME_KEY_LEFT)));
    openknx.logger.logWithValues("   - Right       : %u (%s)", (unsigned)store.keyAction(HOME_KEY_RIGHT),
                                 homeKeyActionLabel(store.keyAction(HOME_KEY_RIGHT)));
    openknx.logger.color(CONSOLE_HEADLINE_COLOR);
    openknx.logger.log("-------------------------------------------------------");
    openknx.logger.color(0);
    openknx.logger.logWithValues(" Widget records       : %u / %u", store.widgetCount(),
                                 (unsigned)DisplaySettingsStore::widgetCapacity());
    if (store.widgetCount() > 0)
    {
        openknx.logger.log(" Idx | NameHash    | En | Ord | Dur(ds)");
        openknx.logger.log(" ----+-------------+----+-----+--------");
        for (uint8_t i = 0; i < store.widgetCount(); ++i)
        {
            const WidgetSetting* ws = store.widgetAt(i);
            if (!ws) continue;
            openknx.logger.logWithValues(" %3u | 0x%08lX  | %-2s | %3u | %u",
                                         i,
                                         (unsigned long)ws->nameHash,
                                         ws->enabled ? "on" : "of",
                                         ws->orderIndex,
                                         ws->durationDs);
        }
    }
    openknx.logger.color(CONSOLE_HEADLINE_COLOR);
    openknx.logger.log("=======================================================");
    openknx.logger.log("");
    openknx.logger.color(0);
    openknx.logger.end();
}

/**
 * @brief "ddc i" section: menu registry tree + dynamic module items + live provider values.
 */
void DdcConsole::logMenuConfig()
{
    MenuRegistry* reg = openknxDisplayModule.getMenuRegistry();

    openknx.logger.begin();
    openknx.logger.log("");
    openknx.logger.color(CONSOLE_HEADLINE_COLOR);
    openknx.logger.log("=======================================================");
    openknx.logger.log("=                    Menu Config                      =");
    openknx.logger.log("=======================================================");
    openknx.logger.color(0);

    if (reg == nullptr)
    {
        openknx.logger.log(" (menu registry not available)");
        openknx.logger.color(CONSOLE_HEADLINE_COLOR);
        openknx.logger.log("=======================================================");
        openknx.logger.color(0);
        openknx.logger.end();
        return;
    }

    openknx.logger.logWithValues(" Root items (module)  : %u", (unsigned)reg->rootItemCount());
    openknx.logger.logWithValues(" onValueChanged cbs   : %u", (unsigned)reg->getOnValueChanged().size());
    openknx.logger.logWithValues(" action cbs           : %u", (unsigned)reg->getActions().size());
    openknx.logger.color(CONSOLE_HEADLINE_COLOR);
    openknx.logger.log("-------------------------------------------------------");
    openknx.logger.color(0);

    // build() returns the deduplicated, sorted registry tree (dynamic module roots + About);
    // dumpMenuOptions expands lazy submenus so per-widget / live children are visible too.
    dumpMenuOptions(reg->build(), 0);

    openknx.logger.color(CONSOLE_HEADLINE_COLOR);
    openknx.logger.log("=======================================================");
    openknx.logger.log("");
    openknx.logger.color(0);
    openknx.logger.end();
}

namespace
{
    // Raw line to the log device (no timestamp/prefix) so base64/ASCII dumps copy-paste cleanly.
    inline void rawLine(const char* s)
    {
#ifdef OPENKNX_LOGGER_DEVICE
        OPENKNX_LOGGER_DEVICE.println(s);
#else
        openknx.logger.log(s);
#endif
    }

    // Append `len` bytes of `data` to `out` as standard base64.
    void appendBase64(std::string& out, const uint8_t* data, size_t len)
    {
        static const char T[] = "ABCDEFGHIJKLMNOPQRSTUVWXYZabcdefghijklmnopqrstuvwxyz0123456789+/";
        size_t i = 0;
        for (; i + 3 <= len; i += 3)
        {
            const uint32_t n = (uint32_t(data[i]) << 16) | (uint32_t(data[i + 1]) << 8) | data[i + 2];
            out += T[(n >> 18) & 63];
            out += T[(n >> 12) & 63];
            out += T[(n >> 6) & 63];
            out += T[n & 63];
        }
        if (len - i == 1)
        {
            const uint32_t n = uint32_t(data[i]) << 16;
            out += T[(n >> 18) & 63];
            out += T[(n >> 12) & 63];
            out += "==";
        }
        else if (len - i == 2)
        {
            const uint32_t n = (uint32_t(data[i]) << 16) | (uint32_t(data[i + 1]) << 8);
            out += T[(n >> 18) & 63];
            out += T[(n >> 12) & 63];
            out += T[(n >> 6) & 63];
            out += '=';
        }
    }
} // namespace

/**
 * @brief "ddc key <up|down|left|right|ok> [long]": inject a front-plate button event.
 *
 * Feeds DeviceDisplay::handleButtonEvent() exactly like the real buttons do, so screensaver wake,
 * gesture arming and the menu routing all behave identically — this drives the UI headlessly.
 *
 * A real short press is PRESS followed by RELEASE, and only the RELEASE navigates (the PRESS just
 * arms a hold). Both are emitted back-to-back here, so no loop() tick separates them and the
 * gesture confirm bar can never run: an injected key therefore cannot fire a hold action
 * (Reboot/Prog/DisplayOff on the Home screen). "long" sends PRESS + LONG_PRESS instead, which is
 * what the LEFT-hold menu exit listens for; gestures needing the bar still will not fire, as that
 * requires real time passing between the events.
 */
bool DdcConsole::processKeyCommand(const std::string& command)
{
    const size_t argStart = 8; // strlen("ddc key ")
    if (command.size() <= argStart)
    {
        logInfoP("Usage: ddc key <up|down|left|right|ok> [long|press|release]");
        return true;
    }

    std::string arg = command.substr(argStart);
    for (char& c : arg) // lowercase in place (no <cctype> locale detour)
        if (c >= 'A' && c <= 'Z') c += 32;

    // Split "<name> [modifier]".
    std::string name = arg;
    std::string mod;
    const size_t sp = arg.find(' ');
    if (sp != std::string::npos)
    {
        name = arg.substr(0, sp);
        mod = arg.substr(sp + 1);
        while (!mod.empty() && mod.front() == ' ')
            mod.erase(mod.begin());
    }
    while (!name.empty() && (name.back() == ' ' || name.back() == '\r'))
        name.pop_back();

    ButtonType type;
    if (name == "up") type = ButtonType::UP;
    else if (name == "down") type = ButtonType::DOWN;
    else if (name == "left") type = ButtonType::LEFT;
    else if (name == "right") type = ButtonType::RIGHT;
    else if (name == "ok" || name == "select") type = ButtonType::SELECT;
    else
    {
        logInfoP("Unknown key '%s' (use up|down|left|right|ok)", name.c_str());
        return true;
    }

    // "press"/"release" emit a single half of the pair, so a gesture can be held across real
    // loop() ticks (the confirm bar only fills while time passes between the two).
    if (mod.compare(0, 5, "press") == 0)
    {
        openknxDisplayModule.handleButtonEvent(ButtonEvent(type, ButtonAction::PRESS));
        logInfoP("Key '%s' PRESS (held)", name.c_str());
        return true;
    }
    if (mod.compare(0, 7, "release") == 0)
    {
        openknxDisplayModule.handleButtonEvent(ButtonEvent(type, ButtonAction::RELEASE));
        logInfoP("Key '%s' RELEASE", name.c_str());
        return true;
    }

    const bool isLong = (mod.compare(0, 4, "long") == 0);
    openknxDisplayModule.handleButtonEvent(ButtonEvent(type, ButtonAction::PRESS));
    if (isLong)
    {
        // A real long press still ends with the button being let go. Without that RELEASE the
        // LEFT-hold menu exit leaves _menuExitSwallowRelease armed, and it then eats the NEXT
        // press entirely -- which silently breaks whatever runs after "ddc key left long".
        openknxDisplayModule.handleButtonEvent(ButtonEvent(type, ButtonAction::LONG_PRESS));
        openknxDisplayModule.handleButtonEvent(ButtonEvent(type, ButtonAction::RELEASE));
    }
    else
    {
        openknxDisplayModule.handleButtonEvent(ButtonEvent(type, ButtonAction::RELEASE));
    }

    logInfoP("Key '%s'%s injected", name.c_str(), isLong ? " (long)" : "");
    return true;
}

/**
 * @brief "ddc screenshot ascii": render the live framebuffer as half-block glyphs (2 rows/char),
 *        one row per line, prefix-free.
 */
void DdcConsole::logFramebufferAscii()
{
    if (_displayModule == nullptr) { logInfoP("Framebuffer: no display"); return; }
    const uint8_t* fb = _displayModule->getFramebuffer();
    const uint8_t w = _displayModule->GetDisplayWidth();
    const uint8_t h = _displayModule->GetDisplayHeight();
    if (fb == nullptr || w == 0 || h == 0 || w > 128)
    {
        logInfoP("Framebuffer unavailable (w=%u h=%u)", (unsigned)w, (unsigned)h);
        return;
    }

    const auto lit = [&](uint16_t x, uint16_t y) -> bool {
        return (fb[static_cast<uint32_t>(y >> 3) * w + x] >> (y & 7)) & 1;
    };

    logInfoP("Framebuffer %ux%u (half-block):", (unsigned)w, (unsigned)h);
    std::string line;
    line.reserve(static_cast<size_t>(w) * 3 + 1);
    for (uint16_t y = 0; y + 1 < h; y += 2)
    {
        line.clear();
        for (uint16_t x = 0; x < w; ++x)
        {
            const bool t = lit(x, y);
            const bool b = lit(x, y + 1);
            if (t && b) line += "\xE2\x96\x88";      // U+2588 full block
            else if (t) line += "\xE2\x96\x80";      // U+2580 upper half
            else if (b) line += "\xE2\x96\x84";      // U+2584 lower half
            else line += ' ';
        }
        rawLine(line.c_str());
    }
}

/**
 * @brief "ddc screenshot b64": raw framebuffer (SSD1306 vertical packing, LSB=top) as base64 in
 *        prefix-free lines, so a tool can reconstruct the exact 1-bit w x h image.
 */
void DdcConsole::logFramebufferBase64()
{
    if (_displayModule == nullptr) { logInfoP("Framebuffer: no display"); return; }
    const uint8_t* fb = _displayModule->getFramebuffer();
    const uint8_t w = _displayModule->GetDisplayWidth();
    const uint8_t h = _displayModule->GetDisplayHeight();
    const size_t bytes = static_cast<size_t>(w / 8) * h;
    if (fb == nullptr || bytes == 0 || bytes > 1024)
    {
        logInfoP("Framebuffer unavailable (w=%u h=%u)", (unsigned)w, (unsigned)h);
        return;
    }

    logInfoP("Framebuffer %ux%u base64 (%u bytes, SSD1306 vertical packing):",
             (unsigned)w, (unsigned)h, (unsigned)bytes);
    std::string b64;
    b64.reserve(((bytes + 2) / 3) * 4);
    appendBase64(b64, fb, bytes);
    for (size_t i = 0; i < b64.size(); i += 120)
        rawLine(b64.substr(i, 120).c_str());
}

/**
 * @brief "ddc i" section: free heap now / min-ever / total, cross-platform.
 */
void DdcConsole::logMemoryInfo()
{
    openknx.logger.begin();
    openknx.logger.log("");
    openknx.logger.color(CONSOLE_HEADLINE_COLOR);
    openknx.logger.log("=======================================================");
    openknx.logger.log("=                      Memory                         =");
    openknx.logger.log("=======================================================");
    openknx.logger.color(0);

    // Cross-platform: freeMemory() (OGM-Common Helper) + common.freeMemoryMin().
    openknx.logger.logWithValues(" Free memory          : %.3f KiB (min. %.3f KiB)",
                                 ((float)freeMemory() / 1024), ((float)openknx.common.freeMemoryMin() / 1024));

    #ifdef ARDUINO_ARCH_ESP32
    size_t heapFree = heap_caps_get_free_size(MALLOC_CAP_DEFAULT);
    size_t heapMin = heap_caps_get_minimum_free_size(MALLOC_CAP_DEFAULT);
    size_t heapTotal = ESP.getHeapSize();
    openknx.logger.logWithValues(" Heap free / min      : %.3f / %.3f KiB",
                                 ((float)heapFree / 1024), ((float)heapMin / 1024));
    openknx.logger.logWithValues(" Heap total           : %.3f KiB", ((float)heapTotal / 1024));
    #elif defined(ARDUINO_ARCH_RP2040)
    size_t heapFree = rp2040.getFreeHeap();
    size_t heapTotal = rp2040.getTotalHeap();
    openknx.logger.logWithValues(" Heap free            : %.3f KiB", ((float)heapFree / 1024));
    openknx.logger.logWithValues(" Heap total           : %.3f KiB", ((float)heapTotal / 1024));
    openknx.logger.logWithValues(" Heap used            : %.3f KiB",
                                 ((float)(heapTotal - heapFree) / 1024));
    #endif

    openknx.logger.color(CONSOLE_HEADLINE_COLOR);
    openknx.logger.log("=======================================================");
    openknx.logger.log("");
    openknx.logger.color(0);
    openknx.logger.end();
}

/**
 * @brief Process QR code command, adds a Test-QR code widget with the given String
 */
bool DdcConsole::processQRCommand(const std::string& command)
{
    std::string url = command.substr(7);
    if (url.empty() || url.length() > 127)
    {
        logErrorP("Invalid URL length (1-127 characters)");
        return false;
    }

    _widgetManager->addWidget(
        new WidgetQRCode(10000, WidgetFlags::AutoRemove, url, false));
    logInfoP("QR-Code added (%s)", url.c_str());
    return true;
}

    #ifdef WIDGET_CONSOLE
/**
 * @brief Process console widget commands, this requires the console widget to be set.
 */
bool DdcConsole::processConsoleCommand(const std::string& command)
{
    if (!_consoleWidget)
    {
        logErrorP("Console widget not available");
        return false;
    }

    size_t pos = 6; // Skip "ddc c "

    if (command.compare(pos, 1, "s") == 0)
    {
        _consoleWidget->addAction(WidgetFlags::StatusWidget);
        _consoleWidget->addAction(WidgetFlags::ManagedExternally);
        _consoleWidget->addAction(WidgetFlags::DisplayEnabled);
        _consoleWidget->setPriority(WidgetPriority::WIDGET_PRIO_HIGH);
        logInfoP("Console activated");
    }
    else if (command.compare(pos, 1, "r") == 0)
    {
        _consoleWidget->removeAction(WidgetFlags::DisplayEnabled);
        logInfoP("Console deactivated");
    }
    else if (command.compare(pos, 1, "c") == 0)
    {
        _consoleWidget->clear();
        logInfoP("Console cleared");
    }
    else if (command.compare(pos, 2, "l ") == 0)
    {
        std::string text = command.substr(pos + 2);
        if (text.empty() || text.length() > 127)
        {
            logErrorP("Invalid text length (1-127)");
            return false;
        }
        _consoleWidget->addLine(text, WidgetConsole::INFO);
        logInfoP("Line added");
    }
    else if (command.compare(pos, 6, "level ") == 0)
    {
        std::string level = command.substr(pos + 6);
        if (level == "debug") _consoleWidget->setLogLevel(WidgetConsole::DEBUG);
        else if (level == "info")
            _consoleWidget->setLogLevel(WidgetConsole::INFO);
        else if (level == "warning")
            _consoleWidget->setLogLevel(WidgetConsole::WARNING);
        else if (level == "error")
            _consoleWidget->setLogLevel(WidgetConsole::ERROR);
        else if (level == "fatal")
            _consoleWidget->setLogLevel(WidgetConsole::FATAL);
        else
        {
            logErrorP("Unknown log level (%s)", level.c_str());
            return false;
        }
        logInfoP("Log level set (%s)", level.c_str());
    }
    else if (command.compare(pos, 10, "timestamps") == 0)
    {
        static bool enabled = true;
        enabled = !enabled;
        _consoleWidget->toggleTimestamps(enabled);
        logInfoP("Timestamps %s", enabled ? "ON" : "OFF");
    }
    else if (command.compare(pos, 10, "autoscroll") == 0)
    {
        static bool enabled = true;
        enabled = !enabled;
        _consoleWidget->setAutoScroll(enabled);
        logInfoP("Autoscroll %s", enabled ? "ON" : "OFF");
    }
    else if (command.compare(pos, 5, "size ") == 0)
    {
        uint8_t size = atoi(command.substr(pos + 5).c_str());
        if (size == 1 || size == 2)
        {
            _consoleWidget->setTextSize(size);
            logInfoP("Text size %u", size);
        }
        else
        {
            logErrorP("Invalid text size (1 or 2)");
            return false;
        }
    }
    else if (command.compare(pos, 4, "test") == 0)
    {
        _consoleWidget->addLine("Test DEBUG", WidgetConsole::DEBUG);
        _consoleWidget->addLine("Test INFO", WidgetConsole::INFO);
        _consoleWidget->addLine("Test WARNING", WidgetConsole::WARNING);
        _consoleWidget->addLine("Test ERROR", WidgetConsole::ERROR);
        _consoleWidget->addLine("Test FATAL", WidgetConsole::FATAL);
        logInfoP("Test messages added");
    }
    else
    {
        logErrorP("Invalid console command");
        return false;
    }

    return true;
}
    #endif

/**********************************************************************
 *********************** WIDGET COMMAND HANDLERS **********************
 **********************************************************************/
/**
 * @brief Check if the command is a widget command
 */
bool DdcConsole::isWidgetCommand(const std::string& command)
{
    for (const auto& wc : _widgetCommands)
    {
        if (command.compare(4, wc.name.length() + 1, wc.name + " ") == 0)
            return true;
    }
    return false;
}

/**
 * @brief Process widget command (add/remove)
 */
bool DdcConsole::processWidgetCommand(const std::string& command)
{
    for (const auto& wc : _widgetCommands)
    {
        size_t pos = 4; // "ddc "
        if (command.compare(pos, wc.name.length() + 1, wc.name + " ") == 0)
        {
            pos += wc.name.length() + 1;

            if (command.compare(pos, 1, "s") == 0) // Set
            {
                Widget* widget = wc.create();
                _widgetManager->addWidget(widget);
                logInfoP("%s widget added", wc.displayName.c_str());
                return true;
            }
            else if (command.compare(pos, 1, "r") == 0) // Remove
            {
                Widget* widget = _widgetManager->getWidgetFromQueue(wc.displayName);
                if (widget)
                {
                    widget->addAction(WidgetFlags::AutoRemove);
                    logInfoP("%s widget removed", wc.displayName.c_str());
                }
                else
                {
                    logErrorP("%s widget not found", wc.displayName.c_str());
                }
                return true;
            }
        }
    }
    return false;
}

    /**********************************************************************
     ************************ SSD1306 LOW-LEVEL COMMANDS ******************
     **********************************************************************/
    #ifdef DISPLAY_LOW_LEVEL_COMMANDS
/**
 * @brief Check if the command is a low-level display command
 */
bool DdcConsole::isLowLevelCommand(const std::string& command)
{
    const std::vector<std::string> cmds = {
        "dim ", "vcom ", "inv ", "scroll ", "contrast "};

    for (const auto& cmd : cmds)
    {
        if (command.compare(4, cmd.length(), cmd) == 0)
            return true;
    }
    return false;
}

/**
 * @brief Process low-level display commands
 */
bool DdcConsole::processLowLevelCommand(const std::string& command)
{
    // Dim command
    if (command.compare(4, 4, "dim ") == 0)
    {
        if (command.compare(8, 2, "on") == 0)
        {
            _displayModule->SetDim(true); // locked Wire1 wrapper (raw display->dim() bypasses the bus mutex)
            logInfoP("Display dimmed (ON)");
        }
        else if (command.compare(8, 3, "off") == 0)
        {
            _displayModule->SetDim(false);
            logInfoP("Display not dimmed (OFF)");
        }
        else
        {
            int value = std::stoi(command.substr(8));
            if (value >= 0 && value <= 255)
            {
                _displayModule->SetDisplayContrast(value);
                logInfoP("Contrast %d", value);
            }
            else
            {
                logErrorP("Invalid contrast (0-255)");
                return false;
            }
        }
        return true;
    }

    // VCOM command
    if (command.compare(4, 5, "vcom ") == 0)
    {
        if (command.compare(9, 2, "on") == 0)
        {
            _displayModule->SetDisplayVCOMDetect(0x00);
            logInfoP("VCOM enabled");
        }
        else if (command.compare(9, 3, "off") == 0)
        {
            _displayModule->SetDisplayVCOMDetect(0x20);
            logInfoP("VCOM disabled");
        }
        else
        {
            int value = std::stoi(command.substr(9), nullptr, 16);
            if (value >= 0 && value <= 0xFF)
            {
                _displayModule->SetDisplayVCOMDetect(value);
                logInfoP("VCOM 0x%02X", value);
            }
            else
            {
                logErrorP("Invalid VCOM (0x00-0xFF)");
                return false;
            }
        }
        return true;
    }

    // Invert command
    if (command.compare(4, 4, "inv ") == 0)
    {
        bool invert = (command.compare(8, 1, "1") == 0);
        _displayModule->SetInvertDisplay(invert);
        logInfoP("Display %s", invert ? "inverted" : "normal");
        return true;
    }

    // Scroll commands
    if (command.compare(4, 7, "scroll ") == 0)
    {
        return processScrollCommand(command);
    }

    logErrorP("Invalid low-level command");

    return false;
}

/**
 * @brief Process scroll commands
 */
bool DdcConsole::processScrollCommand(const std::string& command)
{
    size_t pos = 11; // "ddc scroll "

    if (command.compare(pos, 1, "r") == 0)
    {
        OPENKNX_WIRE1_LOCK(); // one lock for the whole scroll-setup sequence (shared Wire1 mutex; no-op on RP2040)
        _displayModule->display->ssd1306_command(SSD1306_RIGHT_HORIZONTAL_SCROLL);
        _displayModule->display->ssd1306_command(0x00);
        _displayModule->display->ssd1306_command(0x00);
        _displayModule->display->ssd1306_command(0x07);
        _displayModule->display->ssd1306_command(0x00);
        _displayModule->display->ssd1306_command(0xFF);
        _displayModule->display->ssd1306_command(SSD1306_ACTIVATE_SCROLL);
        logInfoP("Scroll right");
    }
    else if (command.compare(pos, 1, "l") == 0)
    {
        OPENKNX_WIRE1_LOCK();
        _displayModule->display->ssd1306_command(SSD1306_LEFT_HORIZONTAL_SCROLL);
        _displayModule->display->ssd1306_command(0x00);
        _displayModule->display->ssd1306_command(0x00);
        _displayModule->display->ssd1306_command(0x07);
        _displayModule->display->ssd1306_command(0x00);
        _displayModule->display->ssd1306_command(0xFF);
        _displayModule->display->ssd1306_command(SSD1306_ACTIVATE_SCROLL);
        logInfoP("Scroll left");
    }
    else if (command.compare(pos, 5, "start") == 0)
    {
        OPENKNX_WIRE1_LOCK();
        _displayModule->display->ssd1306_command(SSD1306_ACTIVATE_SCROLL);
        logInfoP("Scroll started");
    }
    else if (command.compare(pos, 4, "stop") == 0)
    {
        OPENKNX_WIRE1_LOCK();
        _displayModule->display->ssd1306_command(SSD1306_DEACTIVATE_SCROLL);
        logInfoP("Scroll stopped");
    }
    else
    {
        logErrorP("Invalid scroll command");
        return false;
    }

    return true;
}
    #endif

/**********************************************************************
 ************************ HELP DISPLAY ********************************
 **********************************************************************/
/**
 * @brief Show help information for device display control commands
 */
void DdcConsole::showHelp()
{
    openknx.logger.begin();
    openknx.logger.log("");
    openknx.logger.color(CONSOLE_HEADLINE_COLOR);
    openknx.logger.logHeader("Device Display Control (ddc)");
    openknx.logger.color(0);

    openknx.logger.log("--- General Commands ---");
    openknx.console.printHelpLine("ddc l", "List all widgets");
    openknx.console.printHelpLine("ddc i", "Widget mgr + menu settings + memory");
    openknx.console.printHelpLine("ddc config", "Show display config (id | value)");
    openknx.console.printHelpLine("ddc config reset", "Restore all display defaults");
    openknx.console.printHelpLine("ddc config set <id> <val>", "Set one setting (see 'ddc config')");
    openknx.console.printHelpLine("ddc qr <url>", "Show QR-Code");
    openknx.console.printHelpLine("ddc key <dir> [long]", "Press a button: up|down|left|right|ok");
    openknx.console.printHelpLine("ddc key <dir> press|release", "Hold a button (gestures)");
    openknx.console.printHelpLine("ddc home", "Force home: close menu/overlays, reset latches");

    openknx.logger.log("--- Widget Commands (ddc <widget> s|r) ---");
    for (const auto& wc : _widgetCommands)
    {
        std::string cmdLine = "ddc " + wc.name + " s|r";
        std::string desc = wc.displayName + " widget (s=set, r=remove)";
        openknx.console.printHelpLine(cmdLine.c_str(), desc.c_str());
    }

    #ifdef WIDGET_CONSOLE
    openknx.logger.log("--- Console Widget Commands ---");
    openknx.console.printHelpLine("ddc c s|r|c", "Show/Hide/Clear console");
    openknx.console.printHelpLine("ddc c l <text>", "Log to console");
    openknx.console.printHelpLine("ddc c level <level>", "Set log level");
    openknx.console.printHelpLine("ddc c timestamps", "Toggle timestamps");
    openknx.console.printHelpLine("ddc c autoscroll", "Toggle autoscroll");
    openknx.console.printHelpLine("ddc c size <1|2>", "Set text size");
    openknx.console.printHelpLine("ddc c test", "Add test messages");
    #endif

    #ifdef DISPLAY_LOW_LEVEL_COMMANDS
    openknx.logger.log("--- Display Configuration ---");
    openknx.console.printHelpLine("ddc dim <on|off|0-255>", "Dim/contrast");
    openknx.console.printHelpLine("ddc vcom <on|off|value>", "VCOM detect");
    openknx.console.printHelpLine("ddc inv <0|1>", "Invert display");
    openknx.console.printHelpLine("ddc scroll <r|l|start|stop>", "Scroll control");
    #endif

    openknx.logger.logDividingLine();
    openknx.logger.end();
}
#endif // DEVICE_DISPLAY_MODULE
