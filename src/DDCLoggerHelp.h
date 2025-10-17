#pragma once
/**
 * @file        DDCLoggerHelp.h
 * @brief       Device Display Control - Logger & Help System
 * @details     Console command handler for "ddc" commands with integrated
 *              logging and help system (extens the OpenKNX logger)
 * @version     0.0.1
 * @date        2024-11-27
 * @copyright   Copyright (c) 2024, Erkan Çolak (erkan@çolak.de)
 *              Licensed under GNU GPL v3.0
 **/

#include "OpenKNX.h"

#include <functional>
#include <string>
#include <vector>

#define DISPLAY_LOW_LEVEL_COMMANDS
#define WIDGET_CONSOLE

// Forward declarations
class Widget;
class WidgetConsole;
class WidgetsManager;
class i2cDisplay;

struct WidgetCommandInfo
{
    std::string name;                // "clock", "matrix", etc.
    std::string displayName;         // "Clock", "Matrix", etc.
    std::function<Widget*()> create; // Factory function
};

class DDCLoggerHelp
{
  public:
    DDCLoggerHelp(WidgetsManager* widgetManager, i2cDisplay* displayModule);
    ~DDCLoggerHelp();

    const std::string logPrefix() { return "DeviceDisplay"; } // Logger prefix

    void setup();
    bool processCommand(const std::string& command);
    void showHelp();
    void setConsoleWidget(WidgetConsole* consoleWidget) { _consoleWidget = consoleWidget; }

  private:
    WidgetsManager* _widgetManager = nullptr;
    i2cDisplay* _displayModule = nullptr;
    WidgetConsole* _consoleWidget = nullptr;

    std::vector<WidgetCommandInfo> _widgetCommands;

    // Command routing
    bool processListCommand();
    bool processInfoCommand();
    bool processQRCommand(const std::string& command);
    bool processWidgetCommand(const std::string& command);
    bool isWidgetCommand(const std::string& command);

#ifdef WIDGET_CONSOLE
    bool processConsoleCommand(const std::string& command);
#endif

#ifdef DISPLAY_LOW_LEVEL_COMMANDS
    bool processLowLevelCommand(const std::string& command);
    bool isLowLevelCommand(const std::string& command);
    bool processScrollCommand(const std::string& command);
#endif

    void registerWidgetCommands();
};