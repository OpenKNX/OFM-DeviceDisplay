#include "DeviceDisplay.h"

DeviceDisplay openknxDisplayModule;
i2cDisplay* displayModule = new i2cDisplay();

/**
 * @brief Construct a new Device Display:: Device Display object
 *
 */
DeviceDisplay::DeviceDisplay()
    : widget() // Initialize the widget
{
}

/**
 * @brief Initialize the display module.
 * This function is called within the OpenKNX
 */
void DeviceDisplay::init()
{
    logInfoP("Init started...");
    if (displayModule.InitDisplay() && displayModule.display != nullptr)
    {
        logInfoP("initialized!");
    }
    else
    {
        logErrorP("not initialized!");
    }
}

/**
 * @brief Setup the default widgets for the display.
 *
 * @param configured, will not be used
 */
void DeviceDisplay::setup(bool configured)
{

    logDebugP("setup...");
    initializeWidgets(); // Setup default widget queue
}

/**
 * @brief Process GroupObjects for the display module.
 *
 * @param obj, the GroupObject to process
 */
void DeviceDisplay::processInputKo(GroupObject& obj)
{
    // Could be a challenge to implement ;-)
}

/**
 * @brief Update the display in the loop. Will show the widgets based on their configuration.
 *
 * @param configured, will not be used
 */
void DeviceDisplay::loop(bool configured)
{
    if (displayModule.display == nullptr)
    {
        logErrorP("Display not initialized");
        return;
    }

    WidgetInfo* ProgMode = getWidgetInfo("ProgMode");
    if (knx.progMode() && (ProgMode->widget != nullptr))
    {
        ProgMode->addAction(WidgetAction::InternalEnabled);
    }
    else
    {
        ProgMode->removeAction(WidgetAction::InternalEnabled);
    }

    LoopWidgets(); // Switch widgets based on timing
}

/**
 * @brief Console commands to show the help for the display module.
 */
void DeviceDisplay::showHelp()
{
    openknx.console.printHelpLine("ddc", "Device Display Controll. Use 'disp ?' for more.");
}

/**
 * @brief Process the console commands for the display module.
 *
 * @param command, the command to process
 * @param diagnose, if true, will not process the command
 * @return true if the command was processed, false otherwise
 */
bool DeviceDisplay::processCommand(const std::string command, bool diagnose)
{
    if (diagnose) return false;
    // if (!knx.configured()) return true;
    if (command.compare(0, 4, "ddc ") == 0) // Display text on the display
    {
        if (command.compare(4, 4, "logo") == 0) // Show the boot logo
        {
            logInfoP("BootLogo requested and will be displayed for %d seconds...", BOOT_LOGO_TIMEOUT / 1000);
            Widget* bootLogo = new Widget(Widget::DisplayMode::BOOT_LOGO);
            addWidget(bootLogo, BOOT_LOGO_TIMEOUT, "BootLogo",
                      DeviceDisplay::WidgetAction::StatusFlag |          // This is a status widget
                          DeviceDisplay::WidgetAction::AutoRemoveFlag |  // Remove this widget after display
                          DeviceDisplay::WidgetAction::InternalEnabled); // This widget is enabled
        }
#ifdef MATRIX_SCREENSAVER
        else if (command.compare(4, 1, "m") == 0) // Matrix Screensaver
        {
            Widget* srvMatrix = new Widget(Widget::DisplayMode::SCREEN_SAVER);
            addWidget(srvMatrix, 10000, "srvMatrix", DeviceDisplay::WidgetAction::StatusFlag |          // This is a status widget
                                                         DeviceDisplay::WidgetAction::InternalEnabled | // This widget is enabled
                                                         DeviceDisplay::WidgetAction::AutoRemoveFlag);  // Remove this widget after display

            logInfoP("Sending Matrix Screensaver for 10 seconds to display...");
        }
#endif
        else if (command.compare(4, 1, "c") == 0) // Console simulation output widget
        {
            // Get the console widget
            WidgetInfo* consoleWidgetInfo_ = getWidgetInfo("consoleWidgetInfo_");
            Widget* consoleWidget_ = nullptr;
            if (consoleWidgetInfo_ != nullptr && consoleWidgetInfo_->widget != nullptr)
            {
                consoleWidget_ = consoleWidgetInfo_->widget;
                consoleWidgetInfo_->duration = 30000;                                       // Set the duration to 30 seconds
                consoleWidgetInfo_->action = DeviceDisplay::WidgetAction::StatusFlag |      // This is a status widget
                                             DeviceDisplay::WidgetAction::InternalEnabled | // This widget is enabled
                                             DeviceDisplay::WidgetAction::AutoRemoveFlag;   // Remove this widget after display
                logInfoP("Console Widget updated: avaiable for 30 seconds...");
            }
            else
            {
                consoleWidget_ = new Widget(Widget::DisplayMode::DYNAMIC_TEXT);
                addWidget(consoleWidget_, 30000, "consoleWidgetInfo_", DeviceDisplay::WidgetAction::StatusFlag |          // This is a status widget
                                                                           DeviceDisplay::WidgetAction::InternalEnabled | // This widget is enabled
                                                                           DeviceDisplay::WidgetAction::AutoRemoveFlag);  // Remove this widget after display
                logInfoP("NEW Console Widget created: Avaiable for for 30 seconds...");
            }
            if (consoleWidget_ != nullptr)
            {
                logInfoP("Got the console WidgetInfo and widget...");
                std::string text = command.substr(6);
                if (!text.empty())
                {
                    consoleWidget_->appendLine(text);
                    // logInfoP("Appending text to console widget: %s", text.c_str());
                    return true;
                }
            }
        }
        else if (command.compare(4, 1, "l") == 0 && command.compare(4, 4, "logo") != 0) // List all widgets
        {
            logInfoP("Total Widgets: %d:", widgetsQueue.size());
            for (size_t i = 0; i < widgetsQueue.size(); ++i)
            {
                WidgetInfo& widgetInfo = widgetsQueue[i];
                // Try to create a table with the widget information. The columns must be aligned.
                logInfoP("Order: %d | Name: %s | Action: %d | Duration: %d", i, widgetInfo.name.c_str(), widgetInfo.action, widgetInfo.duration);
            }
            logInfoP("---------------------------------------------------------");
        }
#ifdef QRCODE_WIDGET
        else if (command.compare(4, 2, "qr") == 0) // Show QR-Code
        {
            // grab the string after the qr command: e.g. "logdis qr https://www.openknx.de"
            std::string url = command.substr(7);
            if (url.empty())
            {
                logErrorP("No URL provided for QR-Code generation! Use 'logdis qr <URL>'");
                return false;
            }
            logInfoP("QR-Code requested and will be displayed and removed after %d seconds...", 15000 / 1000);
            Widget* QRCodeWidget = new Widget(Widget::DisplayMode::QR_CODE);                 // Create a new QR code widget
            QRCodeWidget->qrCodeWidget.setUrl(url);                                          // Set the URL for the QR code
            QRCodeWidget->qrCodeWidget.setAlign(QRCodeWidget::QRCodeAlignPos::ALIGN_CENTER); // Set the alignment for the QR code
            addWidget(QRCodeWidget, 15000, "Console-QRCode",
                      DeviceDisplay::WidgetAction::StatusFlag |          // This is a status widget. The status flag will be displayed immediately
                          DeviceDisplay::WidgetAction::InternalEnabled | // This widget is enabled
                          DeviceDisplay::WidgetAction::AutoRemoveFlag);  // Remove this widget after display of the set duration time. Here 10sec.
        }
#endif
        else
        { 
            openknx.logger.begin();
            openknx.logger.log("");
            openknx.logger.color(CONSOLE_HEADLINE_COLOR);
            openknx.logger.log("======================= Help: Device Display Control ===========================");
            openknx.logger.color(0);
            openknx.logger.log("Command(s)               Description");
            openknx.console.printHelpLine("ddc c <text>", "Print/Update Console Widget");
            openknx.console.printHelpLine("ddc l", "List all widgets");
            openknx.console.printHelpLine("ddc logo", "Show the boot logo");
#ifdef MATRIX_SCREENSAVER
            openknx.console.printHelpLine("ddc m", "Print Matrix Screensaver");
#endif
#ifdef QRCODE_WIDGET
            openknx.console.printHelpLine("ddc qr <URL>", "Show QR-Code");
#endif
            openknx.logger.color(CONSOLE_HEADLINE_COLOR);
            openknx.logger.log("Info: To test the progMode widget toogle the prog mode on the device.");
            openknx.logger.log("--------------------------------------------------------------------------------");
            openknx.logger.color(0);
            openknx.logger.end();
            return true;
        }
    }
    return true;
}

// Initialize widgets with default settings or add widgets to queue
void DeviceDisplay::initializeWidgets()
{
    // Add here the bootlogo widget
    Widget* bootLogo = new Widget(Widget::DisplayMode::BOOT_LOGO);
    addWidget(bootLogo, BOOT_LOGO_TIMEOUT, "BootLogo", DeviceDisplay::WidgetAction::StatusFlag |          // This is a status widget
                                                           DeviceDisplay::WidgetAction::InternalEnabled | // This widget is enabled
                                                           DeviceDisplay::WidgetAction::AutoRemoveFlag);  // Remove this widget after display

    Widget* progMode = new Widget(Widget::DisplayMode::PROG_MODE);
    addWidget(progMode, PROG_MODE_BLINK_DELAY, "ProgMode", DeviceDisplay::WidgetAction::StatusFlag |          // This is a status widget
                                                               DeviceDisplay::WidgetAction::ExternalManaged); // This widget is initially disabled
                                                                                                              // Add here more default widgets
}

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
    for (WidgetInfo& widgetInfo : widgetsQueue)
    {
        if (widgetInfo.name == name)
        {
            // Add unique suffix to the name to ensure it is unique. Just use a random kombination of numbers and letters
            name += "_" + std::to_string(random(0, 9)) + (char)random(65, 90);
            logDebugP("Widget name already in use. Added suffix to name: %s", name.c_str());
        }
    }
    logDebugP("Added widget to queue: %s", name.c_str());
    widgetsQueue.push_back({widget, duration, name, action});
}

/**
 * @brief Remove a widget from the queue by name. This will also free the memory of the widget.
 *
 * @param name the name of the widget to remove, must be unique
 * @return true if the widget was removed, false otherwise
 */
bool DeviceDisplay::removeWidget(const std::string& name)
{
    for (std::vector<WidgetInfo>::iterator it = widgetsQueue.begin(); it != widgetsQueue.end(); ++it)
    {
        if (it->name == name)
        {
            logDebugP("Removed widget from queue: %s", name.c_str());
            delete it->widget;      // Free memory, since the widget is created with new!
            widgetsQueue.erase(it); // Remove the widget from the queue list by name

            return true;
        }
    }
    return false;
}

/**
 * @brief Will search for a widget by name and return the pointer to the widget.
 *
 * @param name of the widget to search for
 * @return DeviceDisplay::WidgetInfo* pointer to the widget or nullptr if not found
 */
DeviceDisplay::WidgetInfo* DeviceDisplay::getWidgetInfo(const std::string& name)
{
    for (WidgetInfo& widgetInfo : widgetsQueue)
    {
        if (widgetInfo.name == name)
        {
            return &widgetInfo;
        }
    }
    return nullptr;
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

void DeviceDisplay::LoopWidgets()
{
    if (widgetsQueue.empty()) return; // Stop if no widgets are in the queue

    uint32_t currentTime = millis();
    WidgetInfo* showWidget = nullptr;
    bool statusWidgetsInProgress = false;

    // First loop to prioritize and manage status widgets
    for (WidgetInfo& widget : widgetsQueue)
    {
        showWidget = &widget;

        // Check if widget is a status widget with `InternalEnabled`
        if (showWidget->isActionSet(WidgetAction::StatusFlag) &&
            showWidget->isActionSet(WidgetAction::InternalEnabled))
        {
            if (showWidget->startDisplayTime == 0)
            {
                showWidget->startDisplayTime = currentTime; // Initialize start time for the widget
            }

            // logDebugP("Displaying status widget: %s", showWidget->name.c_str());
            // Auto-remove status widget after display duration
            bool durationPassed = (currentTime - showWidget->startDisplayTime >= showWidget->duration);

            if (showWidget->isActionSet(WidgetAction::AutoRemoveFlag))
            {
                showWidget->addAction(WidgetAction::MarkedForRemove);
            }

            if (showWidget->isActionSet(WidgetAction::MarkedForRemove) && durationPassed)
            {
                logDebugP("Removing status widget: %s", showWidget->name.c_str());
                removeWidget(showWidget->name);
                break; // Exit after removing the widget
            }

            // Disable status widget after its display duration if `ExternalManaged` is not set
            if (!showWidget->isActionSet(WidgetAction::ExternalManaged) && durationPassed)
            {
                // Clear start time after disabling the widget to reset for next activation
                showWidget->startDisplayTime = 0;
                logDebugP("Disabling status widget: %s", showWidget->name.c_str());
                showWidget->removeAction(WidgetAction::InternalEnabled);
                break;
            }
            statusWidgetsInProgress = true;
            break; // Status widget is active; skip to avoid switching to regular widgets
        }
    }

    if (!statusWidgetsInProgress) // Only proceed if no active status widget
    {
        showWidget = (&widgetsQueue[currentWidgetIndex]);
        bool durationPassed = (currentTime - lastWidgetSwitchTime >= widgetsQueue[currentWidgetIndex].duration);

        if (durationPassed)
        {
            if ((&widgetsQueue[currentWidgetIndex])->isActionSet(WidgetAction::StatusFlag) &&
                (&widgetsQueue[currentWidgetIndex])->isActionSet(WidgetAction::ExternalManaged))
            {
                // Skip `ExternalManaged` status widget without switching
                currentWidgetIndex = (currentWidgetIndex + 1) % widgetsQueue.size();
                showWidget = nullptr;
            }
            else
            {
                // Remove widget if marked for removal
                if ((&widgetsQueue[currentWidgetIndex])->isActionSet(WidgetAction::MarkedForRemove))
                {
                    removeWidget((&widgetsQueue[currentWidgetIndex])->name);
                    if (currentWidgetIndex >= widgetsQueue.size()) currentWidgetIndex = 0;
                    showWidget = &widgetsQueue[currentWidgetIndex];
                }
                else
                {
                    // Auto-remove non-status widget
                    if ((&widgetsQueue[currentWidgetIndex])->isActionSet(WidgetAction::AutoRemoveFlag))
                    {
                        (&widgetsQueue[currentWidgetIndex])->addAction(WidgetAction::MarkedForRemove);
                    }
                    // Display the widget
                    showWidget = &widgetsQueue[currentWidgetIndex];
                    currentWidgetIndex = (currentWidgetIndex + 1) % widgetsQueue.size();
                    logDebugP("Displayed regular widget: %s (Duration: %d ms)", showWidget->name.c_str(), showWidget->duration);
                }
            }
            lastWidgetSwitchTime = currentTime; // Only update switch time here to prevent pre-emptive skips
        }
    }

    // Final draw logic outside of conditionals for `showWidget`
    if (showWidget != nullptr && showWidget->duration > WIDGET_INACTIVE) // Skip inactive widgets.
    {
        if (showWidget->isActionSet(WidgetAction::StatusFlag) &&
            showWidget->isActionSet(WidgetAction::ExternalManaged) &&
            !showWidget->isActionSet(WidgetAction::InternalEnabled))
        {
            showWidget = nullptr; // Skip widget with conflicting flags
        }
        else
        {
            showWidget->widget->draw(&displayModule);
        }
    }
}