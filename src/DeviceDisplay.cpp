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
 *
 */
void DeviceDisplay::init()
{
    logInfoP("Init started...");

// Setup the display module with the default settings from the selected hardware
// Ensure all necessary hardware configuration macros are defined
#ifndef OKNXHW_DEVICE_DISPLAY_I2C_0_1
    ERROR_REQUIRED_DEFINE(OKNXHW_DEVICE_DISPLAY_I2C_0_1);
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

    displayModule.lcdSettings.i2cInst = OKNXHW_DEVICE_DISPLAY_I2C_INST; // Set here the i2c instance to use. i2c0 or i2c1
    displayModule.lcdSettings.sda = OKNXHW_DEVICE_DISPLAY_I2C_SDA;      // Set the Hardware specific SDA pin for the display
    displayModule.lcdSettings.scl = OKNXHW_DEVICE_DISPLAY_I2C_SCL;      // Set the Hardware specific SCL pin for the display

    displayModule.lcdSettings.i2cadress = OKNXHW_DEVICE_DISPLAY_I2C_ADDRESS; // Set here the i2c address of the display. I.e. 0x3C
    displayModule.lcdSettings.width = OKNXHW_DEVICE_DISPLAY_WIDTH;           // Set here the width of the display. I.e. 128
    displayModule.lcdSettings.height = OKNXHW_DEVICE_DISPLAY_HEIGHT;         // Set here the height of the display. I.e. 64

    displayModule.lcdSettings.reset = -1; // We are not using a reset pin and set it to -1, which use the internal reset

    if (displayModule.InitDisplay(displayModule.lcdSettings) && displayModule.display != nullptr)
    {
        logInfoP("Display initialized.");
        logDebugP("Display i2c Settings - i2cInt: %p, SDA: %d, SCL: %d, Address: 0x%02X, Width: %d, Height: %d",
                  displayModule.lcdSettings.i2cInst, displayModule.lcdSettings.sda, displayModule.lcdSettings.scl,
                  displayModule.lcdSettings.i2cadress, displayModule.lcdSettings.width, displayModule.lcdSettings.height);
    }
    else
    {
        logErrorP("Display initialization failed!");
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
    displayModule.SetDisplayVCOMDetect(0x20); // Set the VCOMH regulator output
    displayModule.SetDisplayContrast(0xFF);   // Set the contrast of the display
    initializeWidgets();                      // Setup default widget queue
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
    static uint32_t lastDisplayDimTimer_ = millis();
    static bool isDimmed = false;

    if (millis() - lastDisplayDimTimer_ > DISPLAY_DIM_TIMER)
    {
        if (!isDimmed)
        {
            displayModule.SetDisplayContrast(0x00);   // Set the contrast of the display
            displayModule.SetDisplayVCOMDetect(0x00); // Set the VCOMH regulator output
            isDimmed = true;
        }
    }
    else
    {
        if (isDimmed)
        {
            displayModule.SetDisplayContrast(0xFF);   // Set the contrast of the display
            displayModule.SetDisplayVCOMDetect(0x20); // Set the VCOMH regulator output
            isDimmed = false;
        }
    }

    static bool wasInProgMode = false;
    if (knx.progMode())
    {
        wasInProgMode = true;

        lastDisplayDimTimer_ = millis(); // Reset the display dim timer if prog mode is active

        WidgetInfo* ProgMode = getWidgetInfo("ProgMode");
        if (ProgMode && ProgMode->widget != nullptr)
        {
            ProgMode->addAction(WidgetAction::InternalEnabled);
        }
    }
    else if (wasInProgMode)
    {
        WidgetInfo* ProgMode = getWidgetInfo("ProgMode");
        if (ProgMode && ProgMode->widget != nullptr)
        {
            ProgMode->removeAction(WidgetAction::InternalEnabled);
            wasInProgMode = false;
        }
    }
    
    LoopWidgets(); // Switch widgets based on timing

    displayModule.loop();
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
    bool bRet = false;
    if ((!diagnose) && command.compare(0, 4, "ddc ") == 0) // Display text on the display
    {
        if (command.compare(4, 4, "logo") == 0) // Show the boot logo
        {
            logInfoP("BootLogo requested and will be displayed for %d seconds...", BOOT_LOGO_TIMEOUT / 1000);
            Widget* bootLogo = new Widget(Widget::DisplayMode::BOOT_LOGO);
            addWidget(bootLogo, BOOT_LOGO_TIMEOUT, "BootLogo",
                      DeviceDisplay::WidgetAction::StatusFlag |          // This is a status widget
                          DeviceDisplay::WidgetAction::AutoRemoveFlag |  // Remove this widget after display
                          DeviceDisplay::WidgetAction::InternalEnabled); // This widget is enabled
            bRet = true;
        }
#ifdef MATRIX_SCREENSAVER
        else if (command.compare(4, 1, "m") == 0) // Matrix Screensaver
        {
            Widget* srvMatrix = new Widget(Widget::DisplayMode::SCREEN_SAVER);
            addWidget(srvMatrix, 10000, "srvMatrix", DeviceDisplay::WidgetAction::StatusFlag |          // This is a status widget
                                                         DeviceDisplay::WidgetAction::InternalEnabled | // This widget is enabled
                                                         DeviceDisplay::WidgetAction::AutoRemoveFlag);  // Remove this widget after display

            logInfoP("Sending Matrix Screensaver for 10 seconds to display...");
            bRet = true;
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
                    bRet = true;
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
            bRet = true;
        }
#ifdef DD_CONSOLE_CMDS
        else if (command.compare(4, 4, "dim ") == 0) // ddc dim <on|off|0-255>
        {
            if (command.compare(8, 2, "on") == 0)
            {
                // Display dimmen aktivieren
                displayModule.display->dim(true);
                logInfoP("Display dimmed (ON)");
            }
            else if (command.compare(8, 3, "off") == 0)
            {
                // Display dimmen deaktivieren
                displayModule.display->dim(false);
                logInfoP("Display not dimmed (OFF)");
            }
            else
            {
                int contrastValue = std::stoi(command.substr(8));
                if (contrastValue >= 0 && contrastValue <= 255)
                {
                    displayModule.SetDisplayContrast(contrastValue);
                    logInfoP("Display contrast set to " + std::to_string(contrastValue));
                }
                else
                {
                    logErrorP("Invalid contrast value. Please provide a value between 0 and 255.");
                }
            }
            bRet = true;
        }
        else if (command.compare(4, 5, "vcom ") == 0) // ddc vcom <on|off|value> VCOM detect
        {
            if (command.compare(9, 2, "on") == 0)
            {
                // Aktiviert VCOM Detect
                displayModule.SetDisplayVCOMDetect(0x00);
                logInfoP("VCOM detect enabled");
            }
            else if (command.compare(9, 3, "off") == 0)
            {
                // Deaktiviert VCOM Detect
                displayModule.SetDisplayVCOMDetect(0x20); // Set VCOMH to the default value
                logInfoP("VCOM detect disabled");
            }
            else
            {
                // Extract the possible VCOM value from the command
                int vcomValue = std::stoi(command.substr(9), nullptr, 16); // Convert to hex

                // Check if the VCOM value is in the valid range (0x00 to 0xFF)
                if (vcomValue >= 0 && vcomValue <= 0xFF)
                {
                    // Set VCOM detect value
                    displayModule.SetDisplayVCOMDetect(vcomValue);
                    logInfoP("VCOM detect set to value 0x" + std::to_string(vcomValue));
                }
                else
                {
                    logErrorP("Invalid VCOM detect value. Please provide a value between 0x00 and 0xFF.");
                }
            }
            bRet = true;
        }
        else if (command.compare(4, 4, "inv ") == 0) // ddc inv <0|1> Invert the display
        {
            if (command.compare(8, 1, "1") == 0)
            {
                displayModule.SetInvertDisplay(true);
                logInfoP("Display inverted");
            }
            else
            {
                displayModule.SetInvertDisplay(false);
                logInfoP("Display not inverted");
            }
            bRet = true;
        }
        else if (command.compare(4, 7, "scroll ") == 0) // ddc scroll <right|left|diag_right|diag_left|start|stop|set_area>
        {
            if (command.compare(11, 1, "r") == 0) // Scrollen nach rechts
            {
                openknxDisplayModule.displayModule.display->ssd1306_command(SSD1306_RIGHT_HORIZONTAL_SCROLL);
                openknxDisplayModule.displayModule.display->ssd1306_command(0x00); // Startkolonne
                openknxDisplayModule.displayModule.display->ssd1306_command(0x00); // Startseite
                openknxDisplayModule.displayModule.display->ssd1306_command(0x07); // Scroll-Dauer
                openknxDisplayModule.displayModule.display->ssd1306_command(0x00); // Scroll-Wiederholung
                openknxDisplayModule.displayModule.display->ssd1306_command(0xFF); // Ende der Seite
                openknxDisplayModule.displayModule.display->ssd1306_command(SSD1306_ACTIVATE_SCROLL);
                logInfoP("Right horizontal scroll started");
            }
            else if (command.compare(11, 1, "l") == 0) // Scrollen nach links
            {
                openknxDisplayModule.displayModule.display->ssd1306_command(SSD1306_LEFT_HORIZONTAL_SCROLL);
                openknxDisplayModule.displayModule.display->ssd1306_command(0x00); // Startkolonne
                openknxDisplayModule.displayModule.display->ssd1306_command(0x00); // Startseite
                openknxDisplayModule.displayModule.display->ssd1306_command(0x07); // Scroll-Dauer (7 Frames)
                openknxDisplayModule.displayModule.display->ssd1306_command(0x00); // Scroll-Wiederholung
                openknxDisplayModule.displayModule.display->ssd1306_command(0xFF); // Ende der Seite
                openknxDisplayModule.displayModule.display->ssd1306_command(SSD1306_ACTIVATE_SCROLL);
                logInfoP("Left horizontal scroll started");
            }
            else if (command.compare(11, 2, "dr") == 0) // Diagonales Scrollen nach rechts
            {
                openknxDisplayModule.displayModule.display->ssd1306_command(SSD1306_VERTICAL_AND_RIGHT_HORIZONTAL_SCROLL);
                openknxDisplayModule.displayModule.display->ssd1306_command(0x00); // Startkolonne
                openknxDisplayModule.displayModule.display->ssd1306_command(0x00); // Startseite
                openknxDisplayModule.displayModule.display->ssd1306_command(0x07); // Scroll-Dauer
                openknxDisplayModule.displayModule.display->ssd1306_command(0x00); // Scroll-Wiederholung
                openknxDisplayModule.displayModule.display->ssd1306_command(0xFF); // Ende der Seite
                openknxDisplayModule.displayModule.display->ssd1306_command(SSD1306_ACTIVATE_SCROLL);
                logInfoP("Diagonal scroll (right) started");
            }
            else if (command.compare(11, 2, "dl") == 0) // Diagonales Scrollen nach links
            {
                openknxDisplayModule.displayModule.display->ssd1306_command(SSD1306_VERTICAL_AND_LEFT_HORIZONTAL_SCROLL);
                openknxDisplayModule.displayModule.display->ssd1306_command(0x00); // Startkolonne
                openknxDisplayModule.displayModule.display->ssd1306_command(0x00); // Startseite
                openknxDisplayModule.displayModule.display->ssd1306_command(0x07); // Scroll-Dauer
                openknxDisplayModule.displayModule.display->ssd1306_command(0x00); // Scroll-Wiederholung
                openknxDisplayModule.displayModule.display->ssd1306_command(0xFF); // Ende der Seite
                openknxDisplayModule.displayModule.display->ssd1306_command(SSD1306_ACTIVATE_SCROLL);
                logInfoP("Diagonal scroll (left) started");
            }
            else if (command.compare(11, 5, "start") == 0) // Scrollen starten
            {
                openknxDisplayModule.displayModule.display->ssd1306_command(SSD1306_ACTIVATE_SCROLL);
                logInfoP("Scrolling activated");
            }
            else if (command.compare(11, 4, "stop") == 0) // Scrollen stoppen
            {
                openknxDisplayModule.displayModule.display->ssd1306_command(SSD1306_DEACTIVATE_SCROLL);
                logInfoP("Scrolling stopped");
            }
            else if (command.compare(11, 2, "sa") == 0) // Scrollbereich setzen
            {
                // Hier können wir den Bereich für das vertikale Scrollen definieren
                openknxDisplayModule.displayModule.display->ssd1306_command(SSD1306_SET_VERTICAL_SCROLL_AREA);
                openknxDisplayModule.displayModule.display->ssd1306_command(0x00); // Startseite
                openknxDisplayModule.displayModule.display->ssd1306_command(0x3F); // Endseite (64px für 64px Display)
                logInfoP("Vertical scroll area set");
            }
            else
            {
                logErrorP("Invalid scroll command.");
            }
            bRet = true;
        }
        else if (command.compare(4, 9, "contrast ") == 0) // ddc contrast <value>
        {
            // Extrahiere den Wert für den Kontrast (0x00 bis 0xFF)
            int contrastValue = std::stoi(command.substr(13), nullptr, 16); // Wandelt Hex-Wert um

            // Prüfe, ob der Wert im gültigen Bereich (0x00 bis 0xFF) liegt
            if (contrastValue >= 0 && contrastValue <= 0xFF)
            {
                displayModule.SetDisplayContrast(contrastValue);
                logInfoP("Display contrast set to 0x" + std::to_string(contrastValue));
            }
            else
            {
                logErrorP("Invalid contrast value. Please provide a value between 0x00 and 0xFF.");
            }
        }
        else if (command.compare(4, 11, "chargepump ") == 0) // ddc chargepump <on|off>
        {
            if (command.compare(15, 2, "on") == 0)
            {
                // Aktiviert die Ladepumpe
                displayModule.SetDisplayPreCharge(0xF1);
                logInfoP("Charge pump enabled");
            }
            else if (command.compare(15, 3, "off") == 0)
            {
                displayModule.SetDisplayPreCharge(0x10);
                logInfoP("Charge pump disabled");
            }
        }
        else if (command.compare(4, 9, "segremap ") == 0) // ddc segremap <on|off>
        {
            if (command.compare(13, 2, "on") == 0)
            {
                // Segmentzuordnung umkehren (Segment Mapping)
                openknxDisplayModule.displayModule.display->ssd1306_command(SSD1306_SEGREMAP);
                openknxDisplayModule.displayModule.display->ssd1306_command(0xA1); // Umkehrung der Segmentzuordnung
                logInfoP("Segment remapping enabled");
            }
            else if (command.compare(13, 3, "off") == 0)
            {
                // Segmentzuordnung zurücksetzen
                openknxDisplayModule.displayModule.display->ssd1306_command(SSD1306_SEGREMAP);
                openknxDisplayModule.displayModule.display->ssd1306_command(0xA0); // Standard Segmentzuordnung
                logInfoP("Segment remapping disabled");
            }
        }
        else if (command.compare(4, 11, "displayall ") == 0) // ddc displayall <on|off>
        {
            if (command.compare(15, 2, "on") == 0)
            {
                // Alle Pixel auf dem Display einschalten
                openknxDisplayModule.displayModule.display->ssd1306_command(SSD1306_DISPLAYALLON);
                logInfoP("Display all-on mode enabled");
            }
            else if (command.compare(15, 3, "off") == 0)
            {
                // Alle Pixel wieder normal anzeigen
                openknxDisplayModule.displayModule.display->ssd1306_command(SSD1306_DISPLAYALLON_RESUME);
                logInfoP("Display all-on mode disabled, resumed normal display");
            }
        }
#endif
#ifdef QRCODE_WIDGET
        else if (command.compare(4, 2, "qr") == 0) // Show QR-Code
        {
            // grab the string after the qr command: e.g. "ddc qr https://www.openknx.de"
            if (command.length() <= 7 || command.substr(7).empty())
            {
                logErrorP("No URL provided for QR-Code generation! Use 'ddc qr <URL>'");
                bRet = false;
            }

            logInfoP("QR-Code requested and will be displayed and removed after %d seconds...", 15000 / 1000);
            Widget* QRCodeWidget = new Widget(Widget::DisplayMode::QR_CODE);                 // Create a new QR code widget
            QRCodeWidget->qrCodeWidget.setUrl(command.substr(7));                            // Set the URL for the QR code
            QRCodeWidget->qrCodeWidget.setAlign(QRCodeWidget::QRCodeAlignPos::ALIGN_CENTER); // Set the alignment for the QR code
            addWidget(QRCodeWidget, 15000, "Console-QRCode",
                      DeviceDisplay::WidgetAction::StatusFlag |          // This is a status widget. The status flag will be displayed immediately
                          DeviceDisplay::WidgetAction::InternalEnabled | // This widget is enabled
                          DeviceDisplay::WidgetAction::AutoRemoveFlag);  // Remove this widget after display of the set duration time. Here 10sec.
            bRet = true;
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
#ifdef DD_CONSOLE_CMDS
            openknx.console.printHelpLine("ddc scroll <cmd>", "<r|l|dr|dl|start|stop|sa> Scroll the display");
            openknx.console.printHelpLine("ddc vcom <on|off|value>", "Enable or disable VCOM detect or set the value");
            openknx.console.printHelpLine("ddc dim <on|off|0-255>", "Dim the display to on, off or set the contrast value");
            openknx.console.printHelpLine("ddc inv <0|1>", "Invert the display to 0 or 1");
#endif
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
            bRet = false;
        }
    }
    return bRet;
}

// Initialize widgets with default settings or add widgets to queue
void DeviceDisplay::initializeWidgets()
{
    // Bootlogo widget! The boot logo will be displayed immediately and removed after the set duration
    Widget* bootLogo = new Widget(Widget::DisplayMode::BOOT_LOGO);
    addWidget(bootLogo, BOOT_LOGO_TIMEOUT, "BootLogo", DeviceDisplay::WidgetAction::StatusFlag |          // This is a status widget
                                                           DeviceDisplay::WidgetAction::InternalEnabled | // This widget is enabled
                                                           DeviceDisplay::WidgetAction::AutoRemoveFlag);  // Remove this widget after display

    // ProgMode widget! The ProgMode widget will be displayed if the device is in programming mode.
    Widget* progMode = new Widget(Widget::DisplayMode::PROG_MODE);
    addWidget(progMode, PROG_MODE_BLINK_DELAY, "ProgMode", DeviceDisplay::WidgetAction::StatusFlag |          // This is a status widget
                                                               DeviceDisplay::WidgetAction::ExternalManaged); // This widget is initially disabled
                                                                                                              // Add here more default widgets
    Widget* defaultWidget = new Widget(Widget::DisplayMode::OPENKNX_LOGO);
    openknxDisplayModule.addWidget(defaultWidget, 3000, "defaultWidget");
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
    if (widgetsQueue.empty()) return; // Keine Widgets -> frühzeitiger Abbruch

    uint32_t currentTime = millis();
    WidgetInfo* showWidget = nullptr;
    bool statusWidgetsInProgress = false;

    // Status-Widgets priorisieren
    for (WidgetInfo& widget : widgetsQueue)
    {
        if (widget.isActionSet(WidgetAction::StatusFlag) &&
            widget.isActionSet(WidgetAction::InternalEnabled))
        {
            if (widget.startDisplayTime == 0)
                widget.startDisplayTime = currentTime; // Startzeit setzen

            uint32_t elapsedTime = currentTime - widget.startDisplayTime;
            bool durationPassed = elapsedTime >= widget.duration;

            if (widget.isActionSet(WidgetAction::AutoRemoveFlag))
                widget.addAction(WidgetAction::MarkedForRemove);

            if (widget.isActionSet(WidgetAction::MarkedForRemove) && durationPassed)
            {
                logDebugP("Removing status widget: %s", widget.name.c_str());
                removeWidget(widget.name);
                return; // Nach dem Entfernen sofort zurückkehren
            }

            if (!widget.isActionSet(WidgetAction::ExternalManaged) && durationPassed)
            {
                logDebugP("Disabling status widget: %s", widget.name.c_str());
                widget.startDisplayTime = 0; // Reset der Startzeit
                widget.removeAction(WidgetAction::InternalEnabled);
                return; // Widget-Status geändert, zurückkehren
            }

            statusWidgetsInProgress = true;
            showWidget = &widget; // Status-Widget gefunden
            break;                // Status-Widgets blockieren reguläre Widgets
        }
    }

    // Normale Widgets nur verarbeiten, wenn keine aktiven Status-Widgets
    if (!statusWidgetsInProgress)
    {
        WidgetInfo& currentWidget = widgetsQueue[currentWidgetIndex];
        uint32_t elapsedTime = currentTime - lastWidgetSwitchTime;

        if (elapsedTime >= currentWidget.duration)
        {
            if (currentWidget.isActionSet(WidgetAction::StatusFlag) &&
                currentWidget.isActionSet(WidgetAction::ExternalManaged))
            {
                // Überspringen, wenn ExternalManaged
                currentWidgetIndex = (currentWidgetIndex + 1) % widgetsQueue.size();
            }
            else
            {
                if (currentWidget.isActionSet(WidgetAction::MarkedForRemove))
                {
                    removeWidget(currentWidget.name);
                    if (currentWidgetIndex >= widgetsQueue.size())
                        currentWidgetIndex = 0; // Index neu setzen, falls notwendig
                }
                else
                {
                    if (currentWidget.isActionSet(WidgetAction::AutoRemoveFlag))
                        currentWidget.addAction(WidgetAction::MarkedForRemove);

                    showWidget = &currentWidget; // Aktuelles Widget anzeigen
                    currentWidgetIndex = (currentWidgetIndex + 1) % widgetsQueue.size();
                }
                lastWidgetSwitchTime = currentTime; // Switch-Zeit aktualisieren
            }
        }
        else
        {
            showWidget = &currentWidget; // Fortfahren mit aktuellem Widget
        }
    }

    // Endgültiges Zeichnen des Widgets
    if (showWidget && showWidget->duration > WIDGET_INACTIVE)
    {
        if (!(showWidget->isActionSet(WidgetAction::StatusFlag) &&
              showWidget->isActionSet(WidgetAction::ExternalManaged) &&
              !showWidget->isActionSet(WidgetAction::InternalEnabled)))
        {
            showWidget->widget->draw(&displayModule); // Zeichnen, wenn keine Konflikte
        }
    }
}