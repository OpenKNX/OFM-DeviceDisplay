
#include "DDCLoggerHelp.h"
#include "WidgetsManager.h"
#include "devices/i2cDisplay.h"

#include "Widget.h"
#include "Widgets/Clock.h"
#include "Widgets/Matrix.h"
#include "Widgets/Pong.h"
#include "Widgets/Starfield.h"
#include "Widgets/Cube3D.h"
#include "Widgets/Life.h"
#include "Widgets/Rain.h"
#include "Widgets/Fireworks.h"
#include "Widgets/SysInfoLite.h"
#include "Widgets/QRcode.h"
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
        }}
    };
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
 * @brief Process info command, shows widget manager settings
 */
bool DDCLoggerHelp::processInfoCommand()
{
    _widgetManager->logWidgetManagerSettings();
    return true;
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
        _consoleWidget->addAction(WidgetFlags::DisplayEnabled);
        logInfoP("Console activated");
    }
    else if (command.compare(pos, 1, "r") == 0)
    {
        _consoleWidget->removeAction(WidgetFlags::DisplayEnabled);
        _consoleWidget->removeAction(WidgetFlags::AutoRemove);
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
        if (level == "debug")       _consoleWidget->setLogLevel(WidgetConsole::DEBUG);
        else if (level == "info")   _consoleWidget->setLogLevel(WidgetConsole::INFO);
        else if (level == "warning") _consoleWidget->setLogLevel(WidgetConsole::WARNING);
        else if (level == "error")  _consoleWidget->setLogLevel(WidgetConsole::ERROR);
        else if (level == "fatal")  _consoleWidget->setLogLevel(WidgetConsole::FATAL);
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
        "dim ", "vcom ", "inv ", "scroll ", "contrast "
    };

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
            _displayModule->display->dim(true);
            logInfoP("Display dimmed (ON)");
        }
        else if (command.compare(8, 3, "off") == 0)
        {
            _displayModule->display->dim(false);
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
        _displayModule->display->ssd1306_command(SSD1306_ACTIVATE_SCROLL);
        logInfoP("Scroll started");
    }
    else if (command.compare(pos, 4, "stop") == 0)
    {
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
    openknx.console.printHelpLine("ddc i", "Widget manager info");
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