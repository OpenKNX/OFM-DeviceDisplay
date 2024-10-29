#include "DeviceDisplay.h"

DeviceDisplay openknxDisplayModule;
i2cDisplay* displayModule = new i2cDisplay();

// Initialize the display and widgets
DeviceDisplay::DeviceDisplay()
    : displayManager(&displayModule), // Connect displayModule to displayManager
      widget()                        // Initialize the widget
{
}

/**
 * @brief Initialize the display module
 */
void DeviceDisplay::init()
{
#ifdef ENABLE_DISPLAY_DEBUG_LOGS
    logInfoP("Init DeviceDisplay started...");
#endif
    if (displayModule.InitDisplay() && displayModule.display)
    {
        logInfoP("Display initialized!");
    }
    else
    {
        logErrorP("Display not initialized");
    }
}

// Setup method for initialization
void DeviceDisplay::setup(bool configured)
{
#ifdef ENABLE_DISPLAY_DEBUG_LOGS
    logInfoP("setup...");
#endif
    initializeWidgets(); // Setup default widget queue
}

// Process the input GroupObject
void DeviceDisplay::processInputKo(GroupObject& obj)
{
    // Could be a challenge to implement ;-)
}

// Main loop for display updates
void DeviceDisplay::loop(bool configured)
{
    if (displayModule.display == nullptr)
    {
        logErrorP("displayModule - Display not initialized");
        return;
    }

    WidgetInfo* ProgMode = getWidgetInfo("ProgMode");
    if (knx.progMode() && (ProgMode->widget != nullptr))
    {
        setWidgetFlag(ProgMode->action, WidgetAction::InternalEnabled);
    }
    else
    {
        clearWidgetFlag(ProgMode->action, WidgetAction::InternalEnabled);
    }

    LoopWidgets(); // Switch widgets based on timing

    displayManager.updateDisplay(); // Render the current widget on the display
}

// Show the help console commands
void DeviceDisplay::showHelp()
{
    openknx.console.printHelpLine("dis", "display text on display");
    openknx.console.printHelpLine("dist", "Several tests on the display. ? to show more");
    openknx.console.printHelpLine("logdis", "print text to given lines. ? to show more");
}
// Process the console command
bool DeviceDisplay::processCommand(const std::string command, bool diagnose)
{
    if (diagnose) return false;
    // if (!knx.configured()) return true;

    if (command.compare(0, 4, "dis ") == 0)
    {
        Widget* cmdWidget = new Widget(Widget::DisplayMode::FOUR_LINE);
        addWidget(cmdWidget, 5000, "cmdWidget", DeviceDisplay::WidgetAction::StatusFlag |          // This is a status widget
                                                    DeviceDisplay::WidgetAction::InternalEnabled | // This widget is enabled
                                                    DeviceDisplay::WidgetAction::AutoRemoveFlag);  // Remove this widget after display

        std::string text = command.substr(4);
        cmdWidget->SetFourLines("Commandline", "", text.c_str(), "");
        cmdWidget->TextHeader->textSize = 2;
        cmdWidget->TextLine2->textSize = 2;
        logInfoP("Send text to display! Will display for 5 seconds: %s", text.c_str());
    }
    else if (command.compare(0, 7, "logdis ") == 0)
    {
        if (command.compare(7, 1, "?") == 0)
        {
            logInfoP("---------------------------------------");
            logInfoP("Commands | Description");
            logInfoP("---------------------------------------");
            logInfoP("logdis h | Print Text to Header       ");
            logInfoP("logdis 1 | Line 1 Print/Update Text   ");
            logInfoP("logdis 2 | Line 2 Print/Update Text   ");
            logInfoP("logdis 3 | Line 3 Print/Update Text   ");
            logInfoP("logdis l | Print logo to all lines    ");
            logInfoP("logdis r | Reset display              ");
            logInfoP("logdis s | Scroll text on display     ");
        }
        else if (command.compare(7, 1, "m") == 0)
        {
          // ScreenSaver Matrix
            std::string text = command.substr(8);
            Widget* srvMatrix = new Widget(Widget::DisplayMode::SCREEN_SAVER);
            addWidget(srvMatrix, 10000, "srvMatrix", DeviceDisplay::WidgetAction::StatusFlag |          // This is a status widget
                                                    DeviceDisplay::WidgetAction::InternalEnabled | // This widget is enabled
                                                    DeviceDisplay::WidgetAction::AutoRemoveFlag);  // Remove this widget after display

            logInfoP("Sending Matrix Screensaver for 10 seconds to display...");
        }
        else if (command.compare(7, 1, "l") == 0)
        {
            std::string text = command.substr(8);
            // LogLCD("www.OpenKNX.de", "Open ■", "┬────┴", "■ KNX");
            logInfoP("Sending display OpenKNX Logo. %s", text.c_str());
        }
        else if (command.compare(7, 1, "h") == 0)
        {
            std::string text = command.substr(8);
            // LogLCDHeader(text.c_str());
            logInfoP("Sending text to display Header: %s", text.c_str());
        }
        else if (command.compare(7, 1, "1") == 0)
        {
            std::string text = command.substr(8);
            // LogLCDLine1(text.c_str());
            logInfoP("Sending text to display line 1: %s", text.c_str());
        }
        else if (command.compare(7, 1, "2") == 0)
        {
            std::string text = command.substr(8);
            // LogLCDLine2(text.c_str());
            logInfoP("Sending text to display line 2: %s", text.c_str());
        }
        else if (command.compare(7, 1, "3") == 0)
        {
            std::string text = command.substr(8);
            // LogLCDLine3(text.c_str());
            logInfoP("Sending text to display line 3: %s", text.c_str());
        }
        else if (command.compare(7, 1, "r") == 0)
        {
            // LogLCDCLear();
            logInfoP("Resetting display...");
        }
        else if (command.compare(7, 1, "s") == 0)
        {
            // LogLCD("WWW.OPENKNX.DE",
            //        "LINE 1: THIS IS A SCROLLING TEXT. CHARACTERS WILL MOVE FROM RIGHT TO LEFT.",
            //        "LINE 2: THIS IS A SCROLLING TEXT. CHARACTERS WILL MOVE FROM RIGHT TO LEFT.",
            //        "LINE 3: THIS IS A SCROLLING TEXT. CHARACTERS WILL MOVE FROM RIGHT TO LEFT.");
            logInfoP("Sending scrolling text to display...");
        }
    }
    else if (command.compare(0, 5, "dist ") == 0)
    {
        if (command.compare(5, 1, "?") == 0)
        {
            logInfoP("Command   | Description");
            logInfoP("---------------------------------------");
            logInfoP("dist line | Draw lines                 ");
            logInfoP("dist rect | Draw rectangles            ");
            logInfoP("dist filr | Fill rectangles            ");
            logInfoP("dist circ | Draw circles               ");
            logInfoP("dist filc | Fill circles               ");
            logInfoP("dist rrec | Draw rounded rectangles    ");
            logInfoP("dist frec | Fill rounded rectangles    ");
            logInfoP("dist tria | Draw triangles             ");
            logInfoP("dist ftri | Fill triangles             ");
            logInfoP("dist char | Draw characters            ");
            logInfoP("dist styl | Draw styles                ");
            logInfoP("dist scro | Scroll text                ");
            logInfoP("dist bitm | Draw OpenKNX bitmap logo   ");
            logInfoP("dist logo | Show OpenKNX logo          ");
            logInfoP("---------------------------------------");
        }
        if (command.compare(5, 4, "line") == 0)
        {
            // testdrawline();
        }
        else if (command.compare(5, 4, "rect") == 0)
        {
            // testdrawrect();
        }
        else if (command.compare(5, 4, "filr") == 0)
        {
            // testfillrect();
        }
        else if (command.compare(5, 4, "circ") == 0)
        {
            // testdrawcircle();
        }
        else if (command.compare(5, 4, "filc") == 0)
        {
            // testfillcircle();
        }
        else if (command.compare(5, 4, "rrec") == 0)
        {
            // testdrawroundrect();
        }
        else if (command.compare(5, 4, "frec") == 0)
        {
            // testfillroundrect();
        }
        else if (command.compare(5, 4, "tria") == 0)
        {
            // testdrawtriangle();
        }
        else if (command.compare(5, 4, "ftri") == 0)
        {
            // testfilltriangle();
        }
        else if (command.compare(5, 4, "char") == 0)
        {
            // testdrawchar();
        }
        else if (command.compare(5, 4, "styl") == 0)
        {
            // testdrawstyles();
        }
        else if (command.compare(5, 4, "scro") == 0)
        {
            // testscrolltext();
        }
        else if (command.compare(5, 4, "bitm") == 0)
        {
            // testdrawbitmap();
        }
        else if (command.compare(5, 4, "logo") == 0)
        {
            // OpenKNXLogo();
        }
    }
    else
        return false;
    return true;
}

// Initialize widgets with default settings or add widgets to queue
void DeviceDisplay::initializeWidgets()
{
    // Add here the bootlogo widget
    Widget* bootLogo = new Widget(Widget::DisplayMode::BOOT_LOGO);
    addWidget(bootLogo, BOOT_LOGO_TIMEOUT, "BootLogo", DeviceDisplay::WidgetAction::StatusFlag | DeviceDisplay::WidgetAction::InternalEnabled | DeviceDisplay::WidgetAction::AutoRemoveFlag);

    Widget* progMode = new Widget(Widget::DisplayMode::PROG_MODE);
    addWidget(progMode, PROG_MODE_BLINK_DELAY, "ProgMode", DeviceDisplay::WidgetAction::StatusFlag |          // This is a status widget
                                                               DeviceDisplay::WidgetAction::ExternalManaged); // This widget is initially disabled
                                                                                                              // Add here more default widgets
}

// Adds a widget to the queue with specified display duration
// Widget name must be unique!

/**
 * @brief Adds a widget to the queue with specified display duration.
 *
 * @param widget Pointer to the widget to be added.
 * @param duration Duration for which the widget should be displayed (in milliseconds).
 * @param name Unique name for the widget. If empty, a default name will be generated.
 * @param action Action flag for the widget. Default: 0 = No action,
 *        1 = Status widget: This action flag is used to display a status widget immediately and only once for the given duration. To show it again, the action flag for the widget must be set again.
 *        2 = Auto-remove: This action flag is used to display a widget only once for the given duration and then it will be removed Automatically from the queue.
 *        4 = (Internal) Internal disable: This action flag is used to disable a status widget after one display. It is used internally and should not be set manually.
 *        8 = (Internal) Marked for remove: This action flag is used to mark a widget for removal after display. It is used internally and should not be set manually.
 */
void DeviceDisplay::addWidget(Widget* widget, uint32_t duration, std::string name, uint8_t action)
{
    // Check if the widget name is already in use in the queue, if so, then add a suffix to the name
    if (name.empty())
    {
        name = "Widget" + std::to_string(widgetsQueue.size());
    }
    for (auto& widgetInfo : widgetsQueue)
    {
        if (widgetInfo.name == name)
        {
            // Add unique suffix to the name to ensure it is unique. Just use a random kombination of numbers and letters
            name += "_" + std::to_string(random(0, 9)) + (char)random(65, 90);
#ifdef ENABLE_DISPLAY_DEBUG_LOGS
            logInfoP("Widget name already in use. Added suffix to name: %s", name.c_str());
#endif
        }
    }
#ifdef ENABLE_DISPLAY_DEBUG_LOGS
    logInfoP("Added widget to queue: %s", name.c_str());
#endif
    widgetsQueue.push_back({widget, duration, name, action});
}

// Remove a widget from the queue by name
bool DeviceDisplay::removeWidget(const std::string& name)
{
    for (auto it = widgetsQueue.begin(); it != widgetsQueue.end(); ++it)
    {
        if (it->name == name)
        {
#ifdef ENABLE_DISPLAY_DEBUG_LOGS
            logInfoP("Removed widget from queue: %s", name.c_str());
#endif
            widgetsQueue.erase(it);
            return true;
        }
    }
    return false;
}

// Get a widget by name
DeviceDisplay::WidgetInfo* DeviceDisplay::getWidgetInfo(const std::string& name)
{
    for (auto& widgetInfo : widgetsQueue)
    {
        if (widgetInfo.name == name)
        {
            return &widgetInfo;
        }
    }
    return {};
}

// Clear all widgets from the queue
void DeviceDisplay::clearWidgets()
{
    widgetsQueue.clear();
}

// Switches to the next widget in the queue based on duration and priority
/**
 * @brief Switches widgets based on timing.
 *      Status widgets are displayed immediately and only once for the given duration
 *      Regular widgets are displayed based on the duration and order in the queue
 *      Auto-remove widgets are displayed immediately and removed after display
 *      Widgets can be disabled by internal or external actions
 *      Widgets can be marked for removal after display
 *      Widgets can be controlled externally to disable or enable them
 */

/**/
void DeviceDisplay::LoopWidgets()
{
    if (widgetsQueue.empty()) return; // Stop if no widgets are in the queue

    uint32_t currentTime = millis();
    WidgetInfo* showWidget = nullptr;
    bool statusWidgetsInProgress = false;
    // Search for an active status widget
    for (auto& widget : widgetsQueue)
    {
        showWidget = &widget;
        // Check if it’s a status widget and should be displayed based on duration
        if (isWidgetFlagSet(showWidget->action, WidgetAction::StatusFlag) &&
            isWidgetFlagSet(showWidget->action, WidgetAction::InternalEnabled))
        {
#ifdef ENABLE_DISPLAY_DEBUG_LOGS
            logInfoP("Displaying status widget: %s", showWidget->name.c_str());
#endif
            // If AutoRemoveFlag is set, mark the widget for removal
            if (isWidgetFlagSet(showWidget->action, WidgetAction::AutoRemoveFlag))
            {
                setWidgetFlag(showWidget->action, WidgetAction::MarkedForRemove);
            }

            // Check if the widget should be removed or disabled based on duration
            bool durationPassed = (currentTime - lastWidgetSwitchTime >= showWidget->duration);

            if (isWidgetFlagSet(showWidget->action, WidgetAction::MarkedForRemove) && durationPassed)
            {
#ifdef ENABLE_DISPLAY_DEBUG_LOGS
                logInfoP("Removing status widget: %s", showWidget->name.c_str());
#endif
                removeWidget(showWidget->name);
            }

            if (!isWidgetFlagSet(showWidget->action, WidgetAction::ExternalManaged) && durationPassed)
            {
                lastWidgetSwitchTime = currentTime;
#ifdef ENABLE_DISPLAY_DEBUG_LOGS
                logInfoP("Disabling status widget: %s", showWidget->name.c_str());
#endif
                clearWidgetFlag(showWidget->action, WidgetAction::InternalEnabled);
                break;
            }
            statusWidgetsInProgress = true;
            break;
        }
    }

    if (!statusWidgetsInProgress)
    {
        showWidget = (&widgetsQueue[currentWidgetIndex]);
        // If no status widget is active, proceed to display regular widgets
        if (currentTime - lastWidgetSwitchTime >= widgetsQueue[currentWidgetIndex].duration)
        {
            // Skip any status widget marked with ExternalManaged
            if (isWidgetFlagSet((&widgetsQueue[currentWidgetIndex])->action, WidgetAction::StatusFlag) &&
                isWidgetFlagSet((&widgetsQueue[currentWidgetIndex])->action, WidgetAction::ExternalManaged))
            {
                // Skip the widget and move to the next one
                currentWidgetIndex = (currentWidgetIndex + 1) % widgetsQueue.size();
                showWidget = nullptr;
            }
            else
            {
                // Remove widget if marked for removal
                if (isWidgetFlagSet((&widgetsQueue[currentWidgetIndex])->action, WidgetAction::MarkedForRemove))
                {
                    removeWidget((&widgetsQueue[currentWidgetIndex])->name);
                    if (currentWidgetIndex >= widgetsQueue.size()) currentWidgetIndex = 0;
                    showWidget = &widgetsQueue[currentWidgetIndex];
                }
                else
                {
                    if (isWidgetFlagSet((&widgetsQueue[currentWidgetIndex])->action, WidgetAction::AutoRemoveFlag))
                    {
                        setWidgetFlag((&widgetsQueue[currentWidgetIndex])->action, WidgetAction::MarkedForRemove);
                    }
                    // Display the regular widget
                    showWidget = &widgetsQueue[currentWidgetIndex];
                    currentWidgetIndex = (currentWidgetIndex + 1) % widgetsQueue.size();
#ifdef ENABLE_DISPLAY_DEBUG_LOGS
                    logInfoP("Displaying regular widget: %s", showWidget->name.c_str());
#endif
                }
            }
            lastWidgetSwitchTime = currentTime;
        }
    }
    if (showWidget != nullptr)
    {
        if (isWidgetFlagSet(showWidget->action, WidgetAction::StatusFlag) &&
            isWidgetFlagSet(showWidget->action, WidgetAction::ExternalManaged) &&
            !isWidgetFlagSet(showWidget->action, WidgetAction::InternalEnabled))
        {
            // Skip the widget and move to the next one
            showWidget = nullptr;
        }
        else
        {
            showWidget->widget->draw(&displayModule);
        }
    }
}