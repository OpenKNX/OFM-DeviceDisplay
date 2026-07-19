// The "ddc" command console is compiled in by default. Add build flag -D DDC_CONSOLE_DISABLE to strip
// it and save flash; the menu, settings persistence and the KONAMI restore keep working without it.
#if defined(DEVICE_DISPLAY_MODULE) && !defined(DDC_CONSOLE_DISABLE)
    #pragma once
/**
 * @file        ddc_console.h
 * @brief       Handler for the "ddc" (Device Display Control) console commands
 * @details     Parses and dispatches "ddc ..." commands: info dump, widget control,
 *              low-level display commands, scrolling, QR and help.
 * @version     0.0.1
 * @date        2024-11-27
 * @copyright   Copyright (c) 2024, Erkan Çolak (erkan@colak.de)
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

class DdcConsole
{
  public:
    DdcConsole(WidgetsManager* widgetManager, i2cDisplay* displayModule);
    ~DdcConsole();

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

    // "ddc i" extra sections (see processInfoCommand()).
    void logMenuSettings();     // every menu-set value from DisplaySettingsStore
    void logMenuConfig();       // menu registry tree + dynamic module items + live provider values
    void logFramebufferAscii();  // "ddc screenshot ascii": render the live framebuffer as half-blocks
    void logFramebufferBase64(); // "ddc screenshot b64": raw framebuffer as base64 (exact, for tooling)
    void logMemoryInfo();       // free heap now / min-ever / total (cross-platform)
    bool processQRCommand(const std::string& command);
    // "ddc config" family: show all settings (id/value), reset to defaults, or set one by id.
    bool processConfigCommand(const std::string& command);

    // "ddc key <up|down|left|right|ok> [long]": inject a front-plate button event, so the UI can be
    // driven from the console (headless menu tests, screenshot-based verification).
    bool processKeyCommand(const std::string& command);
    bool processConfigSet(const std::string& args); // args = "<id> <value>"
    void logConfig();                               // id | value table of every persisted setting
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
#endif // DEVICE_DISPLAY_MODULE
