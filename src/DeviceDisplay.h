#pragma once
/**
 * @file        DeviceDisplay.h
 * @brief       This module offers a i2c display for the OpenKNX ecosystem
 * @version     0.0.1
 * @date        2024-11-27
 * @copyright   Copyright (c) 2024, Erkan Çolak (erkan@çolak.de)
 *              Licensed under GNU GPL v3.0
 */
#ifdef DEVICE_DISPLAY_MODULE
    #define USE_GPIO_MODULE
    #define WIDGET_MANAGER

    #include "ButtonEvent.h"
    #include "OpenKNX.h"
    #include "OpenKNX/Stat/RuntimeStat.h"
    #include "Widgets.h"
    #include "WidgetsManager.h"
    #ifdef WIDGET_MANAGER
        #include "Menu/Menu.h"
        #include "Widget.h"
        #include "Widgets/BootLogo.h"
        #include "Widgets/Clock.h"
        #include "Widgets/Cube3D.h"
        #include "Widgets/Fireworks.h"
        #include "Widgets/Life.h"
        #include "Widgets/Matrix.h"
        #include "Widgets/MatrixClassic.h"
        #include "Widgets/OpenKNXLogo.h"
        #include "Widgets/Pong.h"
        #include "Widgets/ProgMode.h"
        #include "Widgets/QRcode.h"
        #include "Widgets/Rain.h"
        #include "Widgets/Starfield.h"
        #include "Widgets/SysInfoLite.h"

        #include "WidgetsManager.h"
        #include "i2c-Display.h"
    #endif

// Setup the display module with the default settings from the selected hardware
// Ensure all necessary hardware configuration macros are defined

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

    #define DeviceDisplay_Display_Name "DeviceDisplay"

class DeviceDisplay : public OpenKNX::Module
{
  private:
    #ifdef OPENKNX_RUNTIME_STAT
        // OpenKNX::Stat::RuntimeStat _loopRuntimesDim;
        // OpenKNX::Stat::RuntimeStat _loopDisplayModule;
    #endif
    i2cDisplay* _displayModule = nullptr;
    WidgetsManager* _widgetManager = nullptr;

    // Button hardware pins
    uint16_t _buttonUp;
    uint16_t _buttonDown;
    uint16_t _buttonSelect;
    uint16_t _buttonLeft;
    uint16_t _buttonRight;

    // Button state tracking
    uint32_t _lastButtonCheck = 0;
    uint32_t _buttonCheckInterval = 50; // 50ms check interval

    // Button press timestamps (for LongPress detection)
    uint32_t _buttonPressTime[5] = {0};
    bool _buttonPressed[5] = {false};

    bool _frontPlateEnabled = false;

    // Button methods
    void setupButtons();
    void processButtons();
    bool readButton(uint16_t pin);
    ButtonEvent* checkButton(uint16_t pin, ButtonType type, size_t index);

  public:
    DeviceDisplay();
    ~DeviceDisplay();

    void init();
    void setup(bool configured) override;
    void loop(bool configured) override;
    void processInputKo(GroupObject& ko); // override;

    inline const std::string name() { return DeviceDisplay_Display_Name; }      // Library name
    inline const std::string version() { return MODULE_DeviceDisplay_Version; } // Library version

    void showHelp() override;                                               // Show help for console commands
    bool processCommand(const std::string command, bool diagnose) override; // Process console commands
    void initializeWidgets();                                               // Initialize widgets with default settings or add widgets to queue

    i2cDisplay* getDisplayModule() { return _displayModule; }
    WidgetsManager* getWidgetManager() { return _widgetManager; }

    // void setPowerSaveCallback(PowerSaveCallback callback);

}; // class DeviceDisplay
extern DeviceDisplay openknxDisplayModule; // Display module instance
#endif