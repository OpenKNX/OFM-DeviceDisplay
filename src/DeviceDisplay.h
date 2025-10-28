#pragma once
/**
 * @file        DeviceDisplay.h
 * @brief       Main display module for OpenKNX ecosystem
 * @version     0.0.1
 * @date        2024-12-17
 * @copyright   Copyright (c) 2024, Erkan Çolak
 *              Licensed under GNU GPL v3.0
 **/

#ifdef DEVICE_DISPLAY_MODULE
    #define WIDGET_MANAGER

    #define WIDGET_CONSOLE             // Enable console widget
    #define DISPLAY_LOW_LEVEL_COMMANDS // Enable low-level commands

    #define DeviceDisplay_Display_Name "DeviceDisplay"

    #include "DDCLoggerHelp.h"
    #include "Devices/ButtonManager.h"
    #include "Devices/i2cDisplay.h"
    #include "OpenKNX.h"
    #include "WidgetsManager.h"

    class WidgetsManager;
    class ButtonManager;
    class DDCLoggerHelp;
    class i2cDisplay;
    class WidgetConsole;

    // Hardware validation
    #ifndef OKNXHW_DEVICE_DISPLAY_I2C_INST
ERROR_REQUIRED_DEFINE(OKNXHW_DEVICE_DISPLAY_I2C_INST);
    #endif
    #ifndef OKNXHW_DEVICE_DISPLAY_I2C_SDA
ERROR_REQUIRED_DEFINE(OKNXHW_DEVICE_DISPLAY_I2C_SDA);
    #endif
    #ifndef OKNXHW_DEVICE_DISPLAY_I2C_SCL
ERROR_REQUIRED_DEFINE(OKNXHW_DEVICE_DISPLAY_I2C_SCL);
    #endif
    #ifndef OKNXHW_DEVICE_DISPLAY_I2C_ADDRESS
ERROR_REQUIRED_DEFINE(OKNXHW_DEVICE_DISPLAY_I2C_ADDRESS);
    #endif
    #ifndef OKNXHW_DEVICE_DISPLAY_WIDTH
ERROR_REQUIRED_DEFINE(OKNXHW_DEVICE_DISPLAY_WIDTH);
    #endif
    #ifndef OKNXHW_DEVICE_DISPLAY_HEIGHT
ERROR_REQUIRED_DEFINE(OKNXHW_DEVICE_DISPLAY_HEIGHT);
    #endif

/**
 * @brief Main Display Module
 *
 * Orchestrates:
 * - Display hardware (I2C SSD1306)
 * - Widget system (via WidgetsManager)
 * - Button input (via ButtonManager)
 * - Console commands (via DDCLoggerHelp)
 * - ProgMode handling
 */
class DeviceDisplay : public OpenKNX::Module
{
  private:
    // Core components
    i2cDisplay* _displayModule = nullptr;
    WidgetsManager* _widgetManager = nullptr;
    ButtonManager* _buttonManager = nullptr;
    DDCLoggerHelp* _ddcLoggerHelp = nullptr;

    // Widget references
    WidgetConsole* _consoleWidget = nullptr;

    // Internal functions
    void handleProgMode();
    void initializeWidgets();

  public:
    DeviceDisplay();
    ~DeviceDisplay();

    // OpenKNX::Module interface
    const std::string logPrefix() { return DeviceDisplay_Display_Name; } // Logger prefix

    inline const std::string name() { return DeviceDisplay_Display_Name; }      // Library name
    inline const std::string version() { return MODULE_DeviceDisplay_Version; } // Library version

    void init() override;
    void setup(bool configured) override;
    void loop(bool configured) override;
    void processInputKo(GroupObject& ko) override;
    void showHelp() override;
    bool processCommand(const std::string command, bool diagnose) override;

    // Public accessors (for testing/debugging)
    i2cDisplay* getDisplayModule() { return _displayModule; }
    WidgetsManager* getWidgetManager() { return _widgetManager; }
    ButtonManager* getButtonManager() { return _buttonManager; }

    #ifdef WIDGET_CONSOLE
    WidgetConsole* getConsoleWidget() { return _consoleWidget; }
    #endif
};

extern DeviceDisplay openknxDisplayModule;

#endif // DEVICE_DISPLAY_MODULE