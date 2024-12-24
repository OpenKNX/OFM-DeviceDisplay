#pragma once
/**
 * @file        DeviceDisplay.h
 * @brief       This module offers a i2c display for the OpenKNX ecosystem
 * @version     0.0.1
 * @date        2024-11-27
 * @copyright   Copyright (c) 2024, Erkan Çolak (erkan@çolak.de)
 *              Licensed under GNU GPL v3.0
 */
#include "OpenKNX/Stat/RuntimeStat.h"
#include "Widgets.h"
#define WIDGET_MANAGER // Enable the widget manager
#ifdef WIDGET_MANAGER
    #include "Widget.h"
    #include "Widgets/BootLogo.h"
    #include "Widgets/Clock.h"
    #include "Widgets/Cube3D.h"
    #include "Widgets/Life.h"
    #include "Widgets/Matrix.h"
    #include "Widgets/MatrixClassic.h"
    #include "Widgets/Pong.h"
    #include "Widgets/QRcode.h"
    #include "Widgets/Rain.h"
    #include "Widgets/Starfield.h"
    #include "Widgets/SysInfoLite.h"
    #include "Widgets/OpenKNXLogo.h" // --> This is the new SysInfoLite widget
    #include "Menu/Menu.h"
    //#include "Widgets/Fireworks.h"
    #include "Widgets/ProgMode.h"

    #include "WidgetsManager.h"
    #include "i2c-Display.h"
#endif

#define DeviceDisplay_Display_Name "DeviceDisplay"
#define DeviceDisplay_Display_Version "0.0.1"

#define DISPLAY_DIM_TIMER 60000 // DIm the display after 60 seconds of inactivity
#define DEMO_WIDGET_CMD_TESTS   // Enable the demo widget command tests

class DeviceDisplay : public OpenKNX::Module
{
  public:
#ifdef WIDGET_MANAGER
    inline void setMenuWidget(MenuWidget* menuWidget) { _menuWidget = menuWidget; }
    WidgetsManager widgetManager;
#endif

  private:
#ifdef WIDGET_MANAGER
    MenuWidget* _menuWidget = nullptr;
#endif
#ifdef OPENKNX_RUNTIME_STAT
    OpenKNX::Stat::RuntimeStat _loopRuntimesDim;
    OpenKNX::Stat::RuntimeStat _loopWidgets;
    #ifdef DEMO_WIDGET_CMD_TESTS
    OpenKNX::Stat::RuntimeStat _loopDemoWidgets;
    #endif
    OpenKNX::Stat::RuntimeStat _loopDisplayModule;
#endif

  public:
    // Regular widget is part of the widget queue and will be displayed in sequence
    // Status widget is a special widget that can be displayed at any time. It can be used for error messages or status information
    // Status widgets are not part of the default widget queue and will be shown immediately
    // Auto-remove is a action flag to show the widget only once and remove it after display. It can be used for notifications or warnings
    // Auto-remove widgets are not part of the widget queue and will be shown immediately and removed after display
    typedef enum : uint8_t
    {
        NoAction = 0,         // 0b00000000: No action. Default value. Just display the widget.
        StatusFlag = 1,       // 0b00000001: Status widget. Display immediately and only once for the given duration
        AutoRemoveFlag = 2,   // 0b00000010: Auto-remove. Display once and remove after display
        InternalEnabled = 4,  // 0b00000100: Enabled or Disabled by internal action
        ExternalManaged = 8,  // 0b00001000: External managed. Should be enabled by external action. Internal enabled must be set to display
        MarkedForRemove = 16, // 0b00010000: Marked for remove. Remove the widget after display
    } WidgetAction;
    DeviceDisplay();

    void init();                                   // Initialize the display module
    void setup(bool configured) override;          // Initialize display and widgets
    void loop(bool configured) override;           // Update display in the loop
    void processInputKo(GroupObject& ko) override; // Process GroupObjects

    inline const std::string name() { return DeviceDisplay_Display_Name; }       // Library name
    inline const std::string version() { return DeviceDisplay_Display_Version; } // Library version

    void addWidget(Widgets* widget, uint32_t duration, std::string name = "", uint8_t action = NoAction); // Add a widget to the queue
    bool removeWidget(const std::string& name);                                                           // Remove a widget from the queue
    inline void clearWidgets() { widgetsQueue.clear(); }                                                  // Clear all widgets from the queue

    void showHelp() override;                                               // Show help for console commands
    bool processCommand(const std::string command, bool diagnose) override; // Process console commands

    struct WidgetInfo
    {
        Widgets* widget;                     // Pointer to the widget
        uint32_t duration = WIDGET_INACTIVE; // Duration to display this widget in milliseconds. 0 = inactive
        std::string name;                    // Optional name for the widget
        uint8_t action = NoAction;           // Action flags for the widget
        uint32_t startDisplayTime = 0;

        inline void setDuration(uint32_t duration_ms) { duration = duration_ms; }  // Set the duration of the widget
        inline uint32_t getDuration() { return duration; }                         // Get the duration of the widget
        inline void addDuration(uint32_t duration_ms) { duration += duration_ms; } // Add the duration to the widget
        inline void disable() { duration = WIDGET_INACTIVE; }                      // Disable the widget by setting the duration to 0

        inline void setName(const std::string& WidgetName) { name = WidgetName; }                // Set the new name for the widget. Must be unique!
        inline std::string getName() { return name; }                                            // Get the name of the widget
        inline void setAction(uint8_t newAction) { action = newAction; }                         // Set new action to the widget
        inline uint8_t getAction() { return action; }                                            // Get the action of the widget
        inline bool isActionSet(uint8_t actionToCheck) { return (action & actionToCheck) != 0; } // Check if the action is set
        inline void addAction(uint8_t actionToAdd) { action |= actionToAdd; }                    // Add an additional action to the widget
        inline void removeAction(uint8_t actiontoRemove) { action &= ~actiontoRemove; }          // Remove the action from the widget
        inline void clearAction() { action = NoAction; }                                         // Clear all actions of the widget
    };
    i2cDisplay displayModule; // The hardware display instance

    inline bool isWidgetCurrentlyDisplayed(const std::string& name)
    {
        // Returns true if the widget is currently displayed on display
        return !widgetsQueue.empty() && widgetsQueue[currentWidgetIndex].name == name;
    }

    std::vector<WidgetInfo> widgetsQueue; // Queue of widgets to display
    uint32_t lastWidgetSwitchTime = 0;    // Last time the widget was switched
    size_t currentWidgetIndex = 0;        // Current widget index in the queue

    bool progModeActive = false; // Tracks Programming Mode status

    Widgets widget; // Widget instance

    void initializeWidgets(); // Initialize widgets with default settings or add widgets to queue
    void LoopWidgets();       // Switches widgets based on timing

    WidgetInfo* getWidgetInfo(const std::string& name); // Get widget info by name
#ifdef DEMO_WIDGET_CMD_TESTS
    // Example console conversation lines
    void demoTestWidgetsSetup(); // Demo test widgets setup
    void demoTestWidgetsStop();  // Demo test widgets remove

    // Loop fpr the demo test widgets, to update their content. System info and console widget!
    bool _demoWidgetSysInfo = false; // Flag to enable the demo widget commands
    bool _demoWidgeConsoleWidget = false;
    void demoSysinfoWidgetLoop(); // Demo test widgets loop
    void demoConsoleWidgetLoop(); // Demo test widgets
#endif
};

extern DeviceDisplay openknxDisplayModule; // Display module instance