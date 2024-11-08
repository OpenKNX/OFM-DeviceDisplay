#pragma once
#include "Widget.h"
#include "i2c-Display.h"

#define ENABLE_DISPLAY_DEBUG_LOGS // Enable debug console logs for display

#define DeviceDisplay_Display_Name "DeviceDisplay"
#define DeviceDisplay_Display_Version "0.0.1"

class DeviceDisplay : public OpenKNX::Module
{
  public:
    typedef enum : uint8_t
    {
        // Regular widget is part of the widget queue and will be displayed in sequence
        // Status widget is a special widget that can be displayed at any time. It can be used for error messages or status information
        // Status widgets are not part of the widget queue and will be shown immediately
        // Auto-remove is a action flag to show the widget only once and remove it after display. It can be used for notifications or warnings
        // Auto-remove widgets are not part of the widget queue and will be shown immediately and removed after display

        NoAction = 0,         // 0b00000000: No action
        StatusFlag = 1,       // 0b00000001: Regular (0) or Status Widget (1)
        AutoRemoveFlag = 2,   // 0b00000010: No auto-remove (0) or Auto-remove after duration (1)
        InternalEnabled = 4,  // 0b00000100: Enabled (0) or Disabled by internal action (1)
        ExternalManaged = 8,  // 0b00001000: Enabled (0) or Disabled by external action (1)
        MarkedForRemove = 16, // 0b00010000: Not marked for remove (0) or Marked for remove (1) after display
    } WidgetAction;
    DeviceDisplay();

    void init();                                   // Initialize the display module
    void setup(bool configured) override;          // Initialize display and widgets
    void loop(bool configured) override;           // Update display in the loop
    void processInputKo(GroupObject& ko) override; // Process GroupObjects

    // Switch display modes
    inline const std::string name() { return DeviceDisplay_Display_Name; }
    inline const std::string version() { return DeviceDisplay_Display_Version; }

    void addWidget(Widget* widget, uint32_t duration, std::string name = "", uint8_t action = NoAction); // Add a widget to the queue
    bool removeWidget(const std::string& name);                                                          // Remove a widget from the queue
    void clearWidgets();                                                                                 // Clear all widgets from the queue

    void showHelp() override; // Show help for console commands
    bool processCommand(const std::string command, bool diagnose) override;

    struct WidgetInfo
    {
        Widget* widget;
        uint32_t duration = WIDGET_INACTIVE; // Duration to display this widget in milliseconds. 0 = inactive
        std::string name;                    // Optional name for the widget
        uint8_t action = NoAction;           // Action flags for the widget
        uint32_t startDisplayTime = 0;

        inline void setDuration(uint32_t duration_ms) { duration = duration_ms; }  // Set the duration of the widget
        inline uint32_t getDuration() { return duration; }                              // Get the duration of the widget
        inline void addDuration(uint32_t duration_ms) { duration += duration_ms; } // Add the duration to the widget
        inline void disable() { duration = WIDGET_INACTIVE; }                // Disable the widget by setting the duration to 0

        inline void setName(const std::string& WidgetName) { name = WidgetName; }   // Set the new name for the widget. Must be unique!
        inline std::string getName() { return name; }                                 // Get the name of the widget
        inline void setAction(uint8_t newAction) { action = newAction; }      // Set new action to the widget
        inline uint8_t getAction() { return action; }                             // Get the action of the widget
        inline bool isActionSet(uint8_t actionToCheck) { return (action & actionToCheck) != 0; } // Check if the action is set
        inline void addAction(uint8_t actionToAdd) { action |= actionToAdd; }     // Add an additional action to the widget
        inline void removeAction(uint8_t actiontoRemove) { action &= ~actiontoRemove; } // Remove the action from the widget
        inline void clearAction() { action = NoAction; }                // Clear all actions of the widget
    };
    i2cDisplay displayModule; // The hardware display instance

    inline bool isWidgetCurrentlyDisplayed(const std::string& name) { // Returns true if the widget is currently displayed on display
      return !widgetsQueue.empty() && widgetsQueue[currentWidgetIndex].name == name;
    }


    std::vector<WidgetInfo> widgetsQueue; // Queue of widgets to display
    uint32_t lastWidgetSwitchTime = 0;    // Last time the widget was switched
    size_t currentWidgetIndex = 0;        // Current widget index in the queue

    bool progModeActive = false; // Tracks Programming Mode status

    Widget widget; // Widget instance

    void initializeWidgets(); // Initialize widgets with default settings or add widgets to queue
    void LoopWidgets();       // Switches widgets based on timing

    WidgetInfo* getWidgetInfo(const std::string& name); // Get widget info by name
};

extern DeviceDisplay openknxDisplayModule;