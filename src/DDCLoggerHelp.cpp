
#ifdef DEVICE_DISPLAY_MODULE
    #include "DDCLoggerHelp.h"
    #include "DeviceDisplay.h"            // openknxDisplayModule + settingsStore() (ddc i "Menu Settings")
    #include "OpenKNX/Helper.h"           // freeMemory() (cross-platform, ddc i "Memory")
    #include "OpenKNX/I2C/Wire1Lock.h"    // OPENKNX_WIRE1_LOCK for the raw scroll ssd1306_command block (no-op on RP2040)
    #include "Settings/DisplaySettings.h" // DisplaySettings / WidgetSetting / HomeKey* enums
    #include "Settings/DisplaySettingsStore.h"
    #include "WidgetsManager.h"
    #include "devices/i2cDisplay.h"

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

DDCLoggerHelp::DDCLoggerHelp(WidgetsManager* widgetManager, i2cDisplay* displayModule)
    : _widgetManager(widgetManager), _displayModule(displayModule)
{
}

DDCLoggerHelp::~DDCLoggerHelp()
{
}

void DDCLoggerHelp::setup()
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
void DDCLoggerHelp::registerWidgetCommands()
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
bool DDCLoggerHelp::processCommand(const std::string& command)
{
    if (command.compare(0, 4, "ddc ") != 0) return false;

    // Route to sub-handlers
    if (command.compare(4, 1, "l") == 0 && command.size() < 6)
        return processListCommand();

    if (command.compare(4, 1, "i") == 0 && command.size() < 6)
        return processInfoCommand();

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
 * @brief Show help for DDC commands
 */
bool DDCLoggerHelp::processListCommand()
{
    _widgetManager->logWidgetQueue();
    return true;
}

/**
 * @brief Process info command: widget-manager settings + persisted menu settings + memory.
 */
bool DDCLoggerHelp::processInfoCommand()
{
    _widgetManager->logWidgetManagerSettings();
    logMenuSettings();
    logMemoryInfo();
    return true;
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
            default: return "?";
        }
    }
} // namespace

/**
 * @brief "ddc i" section: dump every menu-set value from the DisplaySettingsStore.
 */
void DDCLoggerHelp::logMenuSettings()
{
    const DisplaySettingsStore& store = openknxDisplayModule.settingsStore();

    openknx.logger.begin();
    openknx.logger.log("");
    openknx.logger.color(CONSOLE_HEADLINE_COLOR);
    openknx.logger.log("=======================================================");
    openknx.logger.log("=                   Menu Settings                     =");
    openknx.logger.log("=======================================================");
    openknx.logger.color(0);
    openknx.logger.logWithValues(" Brightness idx       : %u", store.brightnessIdx());
    openknx.logger.logWithValues(" Auto-Dim             : %-3s", store.autoDim() ? "yes" : "no");
    openknx.logger.logWithValues(" Invert               : %-3s", store.invert() ? "yes" : "no");
    openknx.logger.logWithValues(" Font size idx        : %u", store.fontSizeIdx());
    openknx.logger.logWithValues(" Auto-Paging          : %-3s", store.autoPaging() ? "yes" : "no");
    openknx.logger.logWithValues(" Screensaver type     : %u (%s)", store.screenSaverType(),
                                 screenSaverTypeLabel(store.screenSaverType()));
    openknx.logger.logWithValues(" Screensaver t/o idx  : %u", store.screenSaverTimeoutIdx());
    openknx.logger.logWithValues(" Sleep t/o idx        : %u", store.sleepTimeoutIdx());
    openknx.logger.logWithValues(" Icon menu            : %-3s", store.iconMenu() ? "yes" : "no");
    openknx.logger.logWithValues(" Screensaver custom   : %u min", store.screenSaverCustomMin());
    openknx.logger.logWithValues(" Sleep custom         : %u min", store.sleepCustomMin());
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
 * @brief "ddc i" section: free heap now / min-ever / total, cross-platform.
 */
void DDCLoggerHelp::logMemoryInfo()
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
bool DDCLoggerHelp::processQRCommand(const std::string& command)
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
bool DDCLoggerHelp::processConsoleCommand(const std::string& command)
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
bool DDCLoggerHelp::isWidgetCommand(const std::string& command)
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
bool DDCLoggerHelp::processWidgetCommand(const std::string& command)
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
bool DDCLoggerHelp::isLowLevelCommand(const std::string& command)
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
bool DDCLoggerHelp::processLowLevelCommand(const std::string& command)
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
bool DDCLoggerHelp::processScrollCommand(const std::string& command)
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
void DDCLoggerHelp::showHelp()
{
    openknx.logger.begin();
    openknx.logger.log("");
    openknx.logger.color(CONSOLE_HEADLINE_COLOR);
    openknx.logger.logHeader("Device Display Control (ddc)");
    openknx.logger.color(0);

    openknx.logger.log("--- General Commands ---");
    openknx.console.printHelpLine("ddc l", "List all widgets");
    openknx.console.printHelpLine("ddc i", "Widget mgr + menu settings + memory");
    openknx.console.printHelpLine("ddc qr <url>", "Show QR-Code");

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