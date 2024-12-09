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

    RUNTIME_MEASURE_BEGIN(_loopRuntimesDim);
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
    RUNTIME_MEASURE_END(_loopRuntimesDim);

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

    RUNTIME_MEASURE_BEGIN(_loopWidgets);
    LoopWidgets(); // Switch widgets based on timing
    RUNTIME_MEASURE_END(_loopWidgets);

#ifdef DEMO_WIDGET_CMD_TESTS
    RUNTIME_MEASURE_BEGIN(_loopDemoWidgets);
    if (_demoWidgetSysInfo) demoSysinfoWidgetLoop(); // Demo test widgets loop
    if (_demoWidgeConsoleWidget) demoConsoleWidgetLoop();
    RUNTIME_MEASURE_END(_loopDemoWidgets);

#endif

    RUNTIME_MEASURE_BEGIN(_loopDisplayModule);
    displayModule.loop();
    RUNTIME_MEASURE_END(_loopDisplayModule);
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
        else if (command.compare(4, 2, "m ") == 0) // Matrix Screensaver
        {
            if (command.compare(6, 1, "s") == 0) // Set Matrix Screensaver
            {
                Widget* srvMatrix = new Widget(Widget::DisplayMode::SCREEN_SAVER);
                addWidget(srvMatrix, 10000, "srvMatrix", DeviceDisplay::WidgetAction::StatusFlag |          // This is a status widget
                                                             DeviceDisplay::WidgetAction::InternalEnabled | // This widget is enabled
                                                             DeviceDisplay::WidgetAction::ExternalManaged); // This is external managed

                logInfoP("Sending Matrix Screensaver to display. Remove it with 'ddc m r'");
            }
            else if (command.compare(6, 1, "r") == 0) // Remove Screensaver
            {
                removeWidget("srvMatrix");
                logInfoP("Removing Matrix Screensaver from display...");
            }
            bRet = true;
        }
        else if (command.compare(4, 6, "clock ") == 0) // Clock Screensaver
        {
            if (command.compare(10, 1, "s") == 0) // Set Clock Screensaver
            {
                Widget* srvClock = new Widget(Widget::DisplayMode::SCREEN_SAVER_CLOCK);
                addWidget(srvClock, 10000, "srvClock", DeviceDisplay::WidgetAction::StatusFlag |          // This is a status widget
                                                           DeviceDisplay::WidgetAction::InternalEnabled | // This widget is enabled
                                                           DeviceDisplay::WidgetAction::ExternalManaged); // This is external managed

                logInfoP("Sending Clock Screensaver to display. Remove it with 'ddc clock r'");
            }
            else if (command.compare(10, 1, "r") == 0) // Remove Screensaver
            {
                removeWidget("srvClock");
                logInfoP("Removing Clock Screensaver from display...");
            }
            bRet = true;
        }
        else if (command.compare(4, 5, "pong ") == 0) // Pong Screensaver
        {
            if (command.compare(9, 1, "s") == 0) // Set Pong Screensaver
            {
                Widget* srvPong = new Widget(Widget::DisplayMode::SCREEN_SAVER_PONG);
                addWidget(srvPong, 10000, "srvPong", DeviceDisplay::WidgetAction::StatusFlag |          // This is a status widget
                                                         DeviceDisplay::WidgetAction::InternalEnabled | // This widget is enabled
                                                         DeviceDisplay::WidgetAction::ExternalManaged); // This is external managed

                logInfoP("Pong Screensaver is set to display. Remove it with 'ddc pong r'");
                bRet = true;
            }
            if (command.compare(9, 1, "r") == 0) // Remove Screensaver
            {
                removeWidget("srvPong");
                logInfoP("Removing Pong Screensaver from display...");
                bRet = true;
            }
        }
        else if (command.compare(4, 5, "rain ") == 0) // Rainfall Screensaver
        {
            if (command.compare(9, 1, "s") == 0) // Set Rainfall Screensaver
            {
                Widget* srvRain = new Widget(Widget::DisplayMode::SCREEN_SAVER_RAIN);
                addWidget(srvRain, 10000, "srvRain", DeviceDisplay::WidgetAction::StatusFlag |          // This is a status widget
                                                         DeviceDisplay::WidgetAction::InternalEnabled | // This widget is enabled
                                                         DeviceDisplay::WidgetAction::ExternalManaged); // This is external managed

                logInfoP("Rainfall Screensaver is set to display. Remove it with 'ddc rain r'");
                bRet = true;
            }
            if (command.compare(9, 1, "r") == 0) // Remove Screensaver
            {
                removeWidget("srvRain");
                logInfoP("Removing Rainfall Screensaver from display...");
                bRet = true;
            }
        }
        else if (command.compare(4, 7, "matrix ") == 0) // Rainfall Screensaver
        {
            if (command.compare(11, 1, "s") == 0) // Set Rainfall Screensaver
            {
                Widget* srvMatrixPixel = new Widget(Widget::DisplayMode::SCREEN_SAVER_MATRIX);
                addWidget(srvMatrixPixel, 10000, "srvMaxtrixPixel", DeviceDisplay::WidgetAction::StatusFlag |          // This is a status widget
                                                                        DeviceDisplay::WidgetAction::InternalEnabled | // This widget is enabled
                                                                        DeviceDisplay::WidgetAction::ExternalManaged); // This is external managed

                logInfoP("Matrix Screensaver is set to display. Remove it with 'ddc matrix r'");
                bRet = true;
            }
            if (command.compare(11, 1, "r") == 0) // Remove Screensaver
            {
                removeWidget("srvMaxtrixPixel");
                logInfoP("Removing Matrix Screensaver from display...");
                bRet = true;
            }
        }
        else if (command.compare(4, 10, "starfield ") == 0) // Starfield Screensaver
        {
            if (command.compare(14, 1, "s") == 0) // Set Starfield Screensaver
            {
                Widget* srvStarfield = new Widget(Widget::DisplayMode::SCREEN_SAVER_STARFIELD);
                addWidget(srvStarfield, 10000, "srvStarfield", DeviceDisplay::WidgetAction::StatusFlag |          // This is a status widget
                                                                   DeviceDisplay::WidgetAction::InternalEnabled | // This widget is enabled
                                                                   DeviceDisplay::WidgetAction::ExternalManaged); // This is external managed

                logInfoP("Starfield Screensaver is set to display. Remove it with 'ddc starfield r'");
                bRet = true;
            }
            if (command.compare(14, 1, "r") == 0) // Remove Screensaver
            {
                removeWidget("srvStarfield");
                logInfoP("Removing Starfield Screensaver from display...");
                bRet = true;
            }
        }
        else if (command.compare(4, 7, "3dcube ") == 0) // 3D Cube Screensaver
        {
            if (command.compare(11, 1, "s") == 0) // Set 3D Cube Screensaver
            {
                Widget* srv3DCube = new Widget(Widget::DisplayMode::SCREEN_SAVER_3DCUBE);
                addWidget(srv3DCube, 10000, "srv3DCube", DeviceDisplay::WidgetAction::StatusFlag |          // This is a status widget
                                                             DeviceDisplay::WidgetAction::InternalEnabled | // This widget is enabled
                                                             DeviceDisplay::WidgetAction::ExternalManaged); // This is external managed

                logInfoP("3D Cube Screensaver is set to display. Remove it with 'ddc 3dcube r'");
                bRet = true;
            }
            if (command.compare(11, 1, "r") == 0) // Remove Screensaver
            {
                removeWidget("srv3DCube");
                logInfoP("Removing 3D Cube Screensaver from display...");
                bRet = true;
            }
        }
        else if (command.compare(4, 5, "life ") == 0) // Life Screensaver
        {
            if (command.compare(9, 1, "s") == 0) // Set Life Screensaver
            {
                Widget* srvLife = new Widget(Widget::DisplayMode::SCREEN_SAVER_LIFE);
                addWidget(srvLife, 10000, "srvLife", DeviceDisplay::WidgetAction::StatusFlag |          // This is a status widget
                                                         DeviceDisplay::WidgetAction::InternalEnabled | // This widget is enabled
                                                         DeviceDisplay::WidgetAction::ExternalManaged); // This is external managed

                logInfoP("Life Screensaver is set to display. Remove it with 'ddc life r'");
                bRet = true;
            }
            if (command.compare(9, 1, "r") == 0) // Remove Screensaver
            {
                removeWidget("srvLife");
                logInfoP("Removing Life Screensaver from display...");
                bRet = true;
            }
        }
        else if (command.compare(4, 8, "openknx ") == 0) // OpenKNX Team Intro
        {
            if (command.compare(12, 1, "s") == 0) // Set OpenKNX Team Intro
            {
                Widget* srvOpenKNXTeam = new Widget(Widget::DisplayMode::OPENKNX_TEAM_INTRO);
                addWidget(srvOpenKNXTeam, 10000, "srvOpenKNXTeam", DeviceDisplay::WidgetAction::StatusFlag |          // This is a status widget
                                                                       DeviceDisplay::WidgetAction::InternalEnabled | // This widget is enabled
                                                                       DeviceDisplay::WidgetAction::ExternalManaged); // This is external managed

                logInfoP("OpenKNX Team Intro is set to display. Remove it with 'ddc openknx_team r'");
                bRet = true;
            }
            if (command.compare(12, 1, "r") == 0) // Remove Screensaver
            {
                removeWidget("srvOpenKNXTeam");
                logInfoP("Removing OpenKNX Team Intro from display...");
                bRet = true;
            }
        }
#endif // MATRIX_SCREENSAVER
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
                logInfoP(" - Attention: The console simulator widget is active for 30 seconds.");
                logInfoP(" - Please use 'ddc c <text>' to append text to the display console.");
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
        else if (command.compare(4, 1, "l") == 0 && command.size() < 6) // List all widgets
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
                displayModule.display->ssd1306_command(SSD1306_RIGHT_HORIZONTAL_SCROLL);
                displayModule.display->ssd1306_command(0x00); // Startkolonne
                displayModule.display->ssd1306_command(0x00); // Startseite
                displayModule.display->ssd1306_command(0x07); // Scroll-Dauer
                displayModule.display->ssd1306_command(0x00); // Scroll-Wiederholung
                displayModule.display->ssd1306_command(0xFF); // Ende der Seite
                displayModule.display->ssd1306_command(SSD1306_ACTIVATE_SCROLL);
                logInfoP("Right horizontal scroll started");
            }
            else if (command.compare(11, 1, "l") == 0) // Scrollen nach links
            {
                displayModule.display->ssd1306_command(SSD1306_LEFT_HORIZONTAL_SCROLL);
                displayModule.display->ssd1306_command(0x00); // Startkolonne
                displayModule.display->ssd1306_command(0x00); // Startseite
                displayModule.display->ssd1306_command(0x07); // Scroll-Dauer (7 Frames)
                displayModule.display->ssd1306_command(0x00); // Scroll-Wiederholung
                displayModule.display->ssd1306_command(0xFF); // Ende der Seite
                displayModule.display->ssd1306_command(SSD1306_ACTIVATE_SCROLL);
                logInfoP("Left horizontal scroll started");
            }
            else if (command.compare(11, 2, "dr") == 0) // Diagonales Scrollen nach rechts
            {
                displayModule.display->ssd1306_command(SSD1306_VERTICAL_AND_RIGHT_HORIZONTAL_SCROLL);
                displayModule.display->ssd1306_command(0x00); // Startkolonne
                displayModule.display->ssd1306_command(0x00); // Startseite
                displayModule.display->ssd1306_command(0x07); // Scroll-Dauer
                displayModule.display->ssd1306_command(0x00); // Scroll-Wiederholung
                displayModule.display->ssd1306_command(0xFF); // Ende der Seite
                displayModule.display->ssd1306_command(SSD1306_ACTIVATE_SCROLL);
                logInfoP("Diagonal scroll (right) started");
            }
            else if (command.compare(11, 2, "dl") == 0) // Diagonales Scrollen nach links
            {
                displayModule.display->ssd1306_command(SSD1306_VERTICAL_AND_LEFT_HORIZONTAL_SCROLL);
                displayModule.display->ssd1306_command(0x00); // Startkolonne
                displayModule.display->ssd1306_command(0x00); // Startseite
                displayModule.display->ssd1306_command(0x07); // Scroll-Dauer
                displayModule.display->ssd1306_command(0x00); // Scroll-Wiederholung
                displayModule.display->ssd1306_command(0xFF); // Ende der Seite
                displayModule.display->ssd1306_command(SSD1306_ACTIVATE_SCROLL);
                logInfoP("Diagonal scroll (left) started");
            }
            else if (command.compare(11, 5, "start") == 0) // Scrollen starten
            {
                displayModule.display->ssd1306_command(SSD1306_ACTIVATE_SCROLL);
                logInfoP("Scrolling activated");
            }
            else if (command.compare(11, 4, "stop") == 0) // Scrollen stoppen
            {
                displayModule.display->ssd1306_command(SSD1306_DEACTIVATE_SCROLL);
                logInfoP("Scrolling stopped");
            }
            else if (command.compare(11, 2, "sa") == 0) // Scrollbereich setzen
            {
                // Hier können wir den Bereich für das vertikale Scrollen definieren
                displayModule.display->ssd1306_command(SSD1306_SET_VERTICAL_SCROLL_AREA);
                displayModule.display->ssd1306_command(0x00); // Startseite
                displayModule.display->ssd1306_command(0x3F); // Endseite (64px für 64px Display)
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
                displayModule.display->ssd1306_command(SSD1306_SEGREMAP);
                displayModule.display->ssd1306_command(0xA1); // Umkehrung der Segmentzuordnung
                logInfoP("Segment remapping enabled");
            }
            else if (command.compare(13, 3, "off") == 0)
            {
                // Segmentzuordnung zurücksetzen
                displayModule.display->ssd1306_command(SSD1306_SEGREMAP);
                displayModule.display->ssd1306_command(0xA0); // Standard Segmentzuordnung
                logInfoP("Segment remapping disabled");
            }
        }
        else if (command.compare(4, 11, "displayall ") == 0) // ddc displayall <on|off>
        {
            if (command.compare(15, 2, "on") == 0)
            {
                // Alle Pixel auf dem Display einschalten
                displayModule.display->ssd1306_command(SSD1306_DISPLAYALLON);
                logInfoP("Display all-on mode enabled");
            }
            else if (command.compare(15, 3, "off") == 0)
            {
                // Alle Pixel wieder normal anzeigen
                displayModule.display->ssd1306_command(SSD1306_DISPLAYALLON_RESUME);
                logInfoP("Display all-on mode disabled, resumed normal display");
            }
        }
#endif // DD_CONSOLE_CMDS
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
#endif // QRCODE_WIDGET
#ifdef DEMO_WIDGET_CMD_TESTS
        else if (command.compare(4, 10, "test_start") == 0) // Show the boot logo
        {
            demoTestWidgetsSetup();
            bRet = true;
        }
        else if (command.compare(4, 9, "test_stop") == 0) // Show the boot logo
        {
            demoTestWidgetsStop();
            bRet = true;
        }
#endif // DEMO_WIDGET_CMD_TESTS
#ifdef OPENKNX_RUNTIME_STAT
        else if (command.compare(4, 8, "runtime ") == 0)
        {
            logInfoP("DeviceDisplay Runtime Statistics: (Uptime=%dms)", millis());
            logIndentUp();

            OpenKNX::Stat::RuntimeStat::showStatHeader();

            if (command.compare(12, 7, "widget ") == 0)
            {
                if (command.compare(19, 3, "all") == 0)
                {
                    for (size_t i = 0; i < widgetsQueue.size(); ++i)
                    {
                        WidgetInfo& widgetInfo = widgetsQueue[i];
                        widgetInfo.widget->_WidgetRutimeStat.showStat("widget_" + widgetInfo.name, 0, true, true);
                    }
                    bRet = true;
                }
                else
                {
                    std::string WidgetName = command.substr(19);
                    if (!WidgetName.empty())
                    {
                        WidgetInfo* widgetInfo = getWidgetInfo(WidgetName);
                        if (widgetInfo && widgetInfo->widget != nullptr)
                        {
                            widgetInfo->widget->_WidgetRutimeStat.showStat("widget_" + WidgetName, 0, true, true);
                        }
                        else
                        {
                            logErrorP("Widget '%s' not found!", WidgetName.c_str());
                        }
                    }
                }
            }
            else
            {
                if (command.compare(12, 7, "widgets") == 0)
                    _loopWidgets.showStat("widgets", 0, true, true);
                else if (command.compare(12, 3, "dim") == 0)
                    _loopRuntimesDim.showStat("dim", 0, true, true);
    #ifdef DEMO_WIDGET_CMD_TESTS
                else if (command.compare(12, 12, "demo_widgets") == 0)
                    _loopDemoWidgets.showStat("demo_widgets", 0, true, true);
    #endif
                else if (command.compare(12, 4, "loop") == 0)
                    _loopDisplayModule.showStat("loop_only", 0, true, true);
                else if (command.compare(12, 3, "all") == 0)
                {
                    _loopWidgets.showStat("widgets", 0, true, true);
                    _loopRuntimesDim.showStat("dim", 0, true, true);
                    _loopDemoWidgets.showStat("demo_widgets", 0, true, true);
                    _loopDisplayModule.showStat("loop_only", 0, true, true);
                }
                else
                    logErrorP("Invalid runtime command.");
            }

            logIndentDown();
            bRet = true;
            // return true;
        }
#endif // OPENKNX_RUNTIME_STAT
        else if (command.compare(4, 3, "__l") == 0)
        {
            displayModule.__setLoopColumnMethod(true);
            logInfoP("Set loop column method: Enabled");
        }
        else if (command.compare(4, 3, "l__") == 0)
        {
            displayModule.__setLoopColumnMethod(false);
            logInfoP("Set loop column method: Disabled");
        }
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
            openknx.console.printHelpLine("ddc contrast <value>", "Set the contrast value (0x00 to 0xFF)");
            openknx.console.printHelpLine("ddc chargepump <on|off>", "Enable or disable the charge pump");
            openknx.console.printHelpLine("ddc segremap <on|off>", "Enable or disable the segment remapping");
            openknx.console.printHelpLine("ddc displayall <on|off>", "Enable or disable the display all-on mode");
#endif // DD_CONSOLE_CMDS
#ifdef DEMO_WIDGET_CMD_TESTS
            openknx.console.printHelpLine("ddc test_start", "Start the demo test widgets");
            openknx.console.printHelpLine("ddc test_stop", "Stop the demo test widgets");
#endif // DEMO_WIDGET_CMD_TESTS
            openknx.console.printHelpLine("ddc l", "List all widgets");
            openknx.console.printHelpLine("ddc logo", "Show the boot logo");
#ifdef MATRIX_SCREENSAVER
            openknx.console.printHelpLine("ddc m <s|r>", "<s> set, <r> remove - Matrix Screensaver ");
            openknx.console.printHelpLine("ddc matrix <s|r>", "<s> set, <r> remove - Matrix Screensaver ");
            openknx.console.printHelpLine("ddc clock <s|r>", "<s> set, <r> remove - Clock Screensaver ");
            openknx.console.printHelpLine("ddc pong <s|r>", "<s> set, <r> remove - Pong Screensaver ");
            openknx.console.printHelpLine("ddc rain <s|r>", "<s> set, <r> remove - Rainfall Screensaver ");
            openknx.console.printHelpLine("ddc starfield <s|r>", "<s> set, <r> remove - Starfield Screensaver ");
            openknx.console.printHelpLine("ddc 3dcube <s|r>", "<s> set, <r> remove - 3D Cube Screensaver ");
            openknx.console.printHelpLine("ddc life <s|r>", "<s> set, <r> remove - Life Screensaver ");
            openknx.console.printHelpLine("ddc openknx <s|r>", "<s> set, <r> remove - OpenKNX Team Intro ");
#endif // MATRIX_SCREENSAVER
#ifdef QRCODE_WIDGET
            openknx.console.printHelpLine("ddc qr <URL>", "Show QR-Code");
#endif // QRCODE_WIDGET
            openknx.logger.color(CONSOLE_HEADLINE_COLOR);
            openknx.logger.log("Info: To test the progMode widget toogle the prog mode on the device.");
            openknx.logger.log("--------------------------------------------------------------------------------");
            openknx.logger.color(0);
#ifdef OPENKNX_RUNTIME_STAT
            openknx.logger.color(CONSOLE_HEADLINE_COLOR);
            openknx.logger.log("Runtime Statistics: Device Display Control");
            openknx.logger.log("--------------------------------------------------------------------------------");
            openknx.logger.color(0);
            openknx.console.printHelpLine("ddc runtime <all>", "Show all (dim, demo, loop widgets) runtime statistics");
            openknx.console.printHelpLine("ddc runtime <dim>", "Show display dim runtime statistics");
            openknx.console.printHelpLine("ddc runtime <demo_widgets>", "Show DEMO widgets runtime statistics");
            openknx.console.printHelpLine("ddc runtime <loop>", "Show display loop only runtime statistics");
            openknx.console.printHelpLine("ddc runtime <widgets>", "Show ALL widgets runtime statistics");
            openknx.console.printHelpLine("ddc runtime widget all", "Show ALL queue widgets runtime statistics");
            openknx.console.printHelpLine("ddc runtime widget <'widget_name'>", "Show Widget runtime statistics. Use 'ddc l' to list all widgets.");
            openknx.logger.color(CONSOLE_HEADLINE_COLOR);
            openknx.logger.log("--------------------------------------------------------------------------------");
#endif // OPENKNX_RUNTIME_STAT
            openknx.logger.color(0);
            openknx.logger.end();
            bRet = false;
        }
    }
    return bRet;
}

/**
 * @brief Initialize widgets with default settings or add widgets to queue
 */
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
    addWidget(defaultWidget, 3000, "defaultWidget");
}

/**
 * @brief Adds a widget to the queue with specified display duration.
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
    logErrorP("Widget not found in queue: %s", name.c_str());
    return false;
}

/**
 * @brief Will search for a widget by name and return the pointer to the widget.
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
                    // logDebugP("Displayed regular widget: %s (Duration: %d ms)", showWidget->name.c_str(), showWidget->duration);
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
        { // Now we are ready to draw the widget
            showWidget->widget->draw(&displayModule);
        }
    }
}

/**
 * @brief This section is for the demo test widgets. It is used to test the display and the widgets.
 *        The Command 'ddc test_start' will start the demo test widgets.
 *        The Command 'ddc test_stop' will stop the demo test widgets.
 *        This section can be enabled by defining the DEMO_WIDGET_CMD_TESTS in the platformio.ini file.
 */
#ifdef DEMO_WIDGET_CMD_TESTS
const char* _demoTestWidgets_conversationLines[] = {
    "1",
    "12",
    "123"
    "1234",
    "12345",
    "123456",
    "1234567",
    "12345678",
    "123456789",
    "1234567890",
    "12345678901",
    "123456789012",
    "1234567890123",
    "12345678901234",
    "123456789012345"};
const int _demoTestWidgets_numLines = sizeof(_demoTestWidgets_conversationLines) / sizeof(_demoTestWidgets_conversationLines[0]);
int _demoTestWidgets_currentLineIndex = 0;
uint32_t _demoTestWidgets_lastUpdateTime = 0;
uint32_t _demoTestWidgets_lastUpdateTime2 = 0;

void DeviceDisplay::demoTestWidgetsStop()
{
    removeWidget("SysInfo");
    removeWidget("NetInfo");
    removeWidget("UptimeLogo");
    removeWidget("DynamicText");
    removeWidget("DynamicText_Header_and_1_Line");
    removeWidget("DynamicText_LeftHeader_RightFooter");
    removeWidget("DynamicText_TopCenterBottom");
    removeWidget("DynamicText_DefaultStacking");
    removeWidget("DynamicText_ScrollingCentered");
    removeWidget("DynamicText_ScrollingCentered_skipLines");
    removeWidget("DynamicText_LeftTop_RightMiddle");
    removeWidget("DynamicText_AllChars");
    // removeWidget("QRCode"); Will be removed automatically
    removeWidget("consoleWidget");

    logInfoP("All test widgets removed from the display queue.");
    _demoWidgetSysInfo = false;
    _demoWidgeConsoleWidget = false;
}
void DeviceDisplay::demoTestWidgetsSetup()
{
    // This are the custom widgets for the display, which can be used in the own application modules
    // Example: addWidget(&textWidget, 5000); // Show TextWidget for 5 seconds

    // Example Widget: Show the OpenKNX System Information
    Widget* WidgetSysInfo = new Widget(Widget::DisplayMode::DYNAMIC_TEXT);                                // Initialize the display and widgets
    WidgetSysInfo->textLines[0].textSize = 2;                                                             // Set the text size for the header
    WidgetSysInfo->SetDynamicTextLines(                                                                   // Set the text lines and the information to display
        {"OpenKNX",                                                                                       // Header
         "",                                                                                              // Line 1
         String("Dev.: " + String(MAIN_OrderNumber)).c_str(),                                             // Line 2
         String("Ver.: " + String(openknx.info.humanFirmwareVersion().c_str())).c_str(),                  // Line 3
         String("Addr.: " + String(openknx.info.humanIndividualAddress().c_str())).c_str(),               // Line 4
         String("Free mem: " + String((float)freeMemory() / 1024) + " KiB").c_str(),                      // Line 5
         String("    (min. " + String((float)openknx.common.freeMemoryMin() / 1024) + " KiB)").c_str()}); // Line 6
    addWidget(WidgetSysInfo, 5000, "SysInfo", DeviceDisplay::WidgetAction::NoAction);                     // Add the widget to the display queue.
    logInfoP("Added System Information widget to the display queue.");

    //
    //  Example Widget: Show the Network Information
    //
    Widget* WidgetNetInfo = new Widget(Widget::DisplayMode::DYNAMIC_TEXT);
    {
        WidgetNetInfo->textLines[0].textSize = 1;                            // Set the text size for the header
        WidgetNetInfo->textLines[1].textSize = 1;                            // Set the text size for the Text line 1
        WidgetNetInfo->textLines[2].textSize = 1;                            // Set the text size for the Text line 2
        WidgetNetInfo->textLines[3].textSize = 1;                            // Set the text size for the Text line 3
        WidgetNetInfo->textLines[4].textSize = 1;                            // Set the text size for the Text line 4
        WidgetNetInfo->textLines[0].alignPos = TextDynamicAlign::ALIGN_LEFT; // Set the alignment for the header
        WidgetNetInfo->textLines[1].alignPos = TextDynamicAlign::ALIGN_LEFT; // Set the alignment for the Text line 1
        WidgetNetInfo->textLines[2].alignPos = TextDynamicAlign::ALIGN_LEFT; // Set the alignment for the Text line 2
        WidgetNetInfo->textLines[3].alignPos = TextDynamicAlign::ALIGN_LEFT; // Set the alignment for the Text line 3
        WidgetNetInfo->textLines[4].alignPos = TextDynamicAlign::ALIGN_LEFT; // Set the alignment for the Text line 4
        WidgetNetInfo->SetDynamicTextLines(                                  // Set the text lines and the information to display
            {
                "Network:",              // Header
                "IP: 11.11.0.123",       // Line 1 i.e.: String("IP: " + String(currentIpAddress.c_str())).c_str(),
                "Subnet: 255.255.255.0", // Line 2
                "Gateway: 11.11.0.1",    // Line 3
                "DNS: 11.11.0.1"         // Line 4
            });
    }
    addWidget(WidgetNetInfo, 3000, "NetInfo"); // Add the widget to the display queue.
                                               // Defaul Action: Regular Widget, wich is the action NoAction!
    logInfoP("Added Network Information widget to the display queue.");

    // Example Widget: Show the OpenKNX Logo
    // Since the OpenKNX logo is a part of the display module, it is not necessary to add it to the queue.
    Widget* WidgetOKNXlogo = new Widget(Widget::DisplayMode::OPENKNX_LOGO);
    addWidget(WidgetOKNXlogo, 3000, "UptimeLogo");
    logInfoP("Added OpenKNX Logo widget to the display queue.");

    // Example Widget: Text Widget with scrolling text
    Widget* dynTextWidgetFull = new Widget(Widget::DisplayMode::DYNAMIC_TEXT);
    dynTextWidgetFull->SetDynamicTextLines(
        {
            "H: Fixed!",                       // Test Line 1
            "L2: This is a scroll test Line2", // Test Line 2
            "L3: This is Line3",               // Test Line 3
            "L4: This is a scroll test Line4", // Test Line 4
            "L5: This is Line5",               // Test Line 5
            "L6: This is a scroll test Line6", // Test Line 6
            "L7: This is Line7",               // Test Line 7
            "L8: This is a scroll test Line8"  // Test Line 8
        });
    addWidget(dynTextWidgetFull, 3000, "DynamicText");
    logInfoP("Added Dynamic Text widget to the display queue.");

    // Example Widget: Header at the top, centered and middle-aligned line below
    Widget* dynTextWidget_Header_and_1_Line = new Widget(Widget::DisplayMode::DYNAMIC_TEXT);                                  // Create a new dynamic text widget
    dynTextWidget_Header_and_1_Line->textLines[1].textSize = 4;                                                               // Set the text size for the line below the header
    dynTextWidget_Header_and_1_Line->textLines[0].alignPos = TextDynamicAlign::ALIGN_CENTER | TextDynamicAlign::ALIGN_TOP;    // Set the alignment for the header
    dynTextWidget_Header_and_1_Line->textLines[1].alignPos = TextDynamicAlign::ALIGN_CENTER | TextDynamicAlign::ALIGN_MIDDLE; // Set the alignment for the line below the header
    dynTextWidget_Header_and_1_Line->SetDynamicTextLine(0, "H: Centered");                                                    // Set the header text
    dynTextWidget_Header_and_1_Line->SetDynamicTextLine(1, "L1: Middle Centered");                                            // Set the line below the header
    addWidget(dynTextWidget_Header_and_1_Line, 3000, "DynamicText_Header_and_1_Line");
    logInfoP("Added Dynamic Text widget with header and 1 line to the display queue.");

    // Example Widget:: Header aligned to the left, Footer aligned to the right
    Widget* dynTextWidget_LeftHeader_RightFooter = new Widget(Widget::DisplayMode::DYNAMIC_TEXT);                                 // Create a new dynamic text widget
    dynTextWidget_LeftHeader_RightFooter->textLines[0].textSize = 1;                                                              // Header text size
    dynTextWidget_LeftHeader_RightFooter->textLines[1].textSize = 1;                                                              // Footer text size
    dynTextWidget_LeftHeader_RightFooter->textLines[0].alignPos = TextDynamicAlign::ALIGN_LEFT | TextDynamicAlign::ALIGN_TOP;     // Header alignment
    dynTextWidget_LeftHeader_RightFooter->textLines[1].alignPos = TextDynamicAlign::ALIGN_RIGHT | TextDynamicAlign::ALIGN_BOTTOM; // Footer alignment
    dynTextWidget_LeftHeader_RightFooter->SetDynamicTextLines(
        {
            "H:Left Aligned", // Expected top-left of the display
            "F:Right Aligned" // Expected bottom-right of the display
        });
    addWidget(dynTextWidget_LeftHeader_RightFooter, 3000, "DynamicText_LeftHeader_RightFooter");
    logInfoP("Added Dynamic Text widget with left header and right footer to the display queue.");

    // Example Widget: Three lines - Top, Center, Bottom alignment (all centered horizontally)
    Widget* dynTextWidget_TopCenterBottom = new Widget(Widget::DisplayMode::DYNAMIC_TEXT); // Create a new dynamic text widget
    dynTextWidget_TopCenterBottom->textLines[0].textSize = 1;                              // Set the text size for the header. Default is 1
    dynTextWidget_TopCenterBottom->textLines[0].alignPos = TextDynamicAlign::ALIGN_CENTER | TextDynamicAlign::ALIGN_TOP;
    dynTextWidget_TopCenterBottom->textLines[1].alignPos = TextDynamicAlign::ALIGN_CENTER | TextDynamicAlign::ALIGN_MIDDLE;
    dynTextWidget_TopCenterBottom->textLines[2].alignPos = TextDynamicAlign::ALIGN_CENTER | TextDynamicAlign::ALIGN_BOTTOM;
    dynTextWidget_TopCenterBottom->SetDynamicTextLines( // Set the text lines and the information to display
        {
            "Top Centered",    // Expected top-center
            "Middle Centered", // Expected center of the display
            "Bottom Centered"  // Expected bottom-center
        });
    addWidget(dynTextWidget_TopCenterBottom, 3000, "DynamicText_TopCenterBottom");
    logInfoP("Added Dynamic Text widget with top, center and bottom alignment to the display queue.");

    // Example Widget: Multiple lines with stacked positioning (default without specific alignment flags)
    Widget* dynTextWidget_DefaultStacking = new Widget(Widget::DisplayMode::DYNAMIC_TEXT); // Create a new dynamic text widget
    dynTextWidget_DefaultStacking->textLines[1].textSize = 2;                              // Larger text for line 1
    dynTextWidget_DefaultStacking->textLines[2].textSize = 2;                              // Larger text for line 2
    dynTextWidget_DefaultStacking->SetDynamicTextLines(                                    // Set the text lines and the information to display
        {
            "L1: Top",    // Expected at the very top
            "L2: Line 1", // Below line 1
            "L3: Line 2"  // Below line 2, all stacked from the top
        });
    addWidget(dynTextWidget_DefaultStacking, 3000, "DynamicText_DefaultStacking");
    logInfoP("Added Dynamic Text widget with default stacking to the display queue.");

    // Example Widget: Center and middle alignment with scrollable text
    Widget* dynTextWidget_ScrollingCentered = new Widget(Widget::DisplayMode::DYNAMIC_TEXT);
    dynTextWidget_ScrollingCentered->textLines[0].alignPos = TextDynamicAlign::ALIGN_CENTER | TextDynamicAlign::ALIGN_TOP;
    dynTextWidget_ScrollingCentered->textLines[1].alignPos = TextDynamicAlign::ALIGN_CENTER | TextDynamicAlign::ALIGN_MIDDLE;
    dynTextWidget_ScrollingCentered->textLines[4].scrollText = false; // Do not scroll the Line 4! Default is True!
    dynTextWidget_ScrollingCentered->SetDynamicTextLines(
        {
            "H: Top Cententer",                                            // Centered at the top
            "",                                                            // Line 1 is empty
            "",                                                            // Line 2 is empty
            "",                                                            // Line 3 is empty
            "Not Scrollable. To long text for the display, do not scroll", // Centered in the middle, does not scroll
            "Scrollable: This line will scroll if too long."               // Centered in the middle, scrolls if it exceeds width
        });
    addWidget(dynTextWidget_ScrollingCentered, 3000, "DynamicText_ScrollingCentered");
    logInfoP("Added Dynamic Text widget with centered scrolling text to the display queue.");

    // Example Widget: Center and middle alignment with scrollable text
    Widget* dynTextWidget_ScrollingCentered_skipLines = new Widget(Widget::DisplayMode::DYNAMIC_TEXT);
    dynTextWidget_ScrollingCentered_skipLines->textLines[0].alignPos = TextDynamicAlign::ALIGN_CENTER | TextDynamicAlign::ALIGN_TOP;
    dynTextWidget_ScrollingCentered_skipLines->textLines[7].alignPos = TextDynamicAlign::ALIGN_LEFT; // Align the last line to the left
    dynTextWidget_ScrollingCentered_skipLines->textLines[1].skipLineIfEmpty = true;                  // Skip empty line 1
    dynTextWidget_ScrollingCentered_skipLines->textLines[2].skipLineIfEmpty = true;                  // Skip empty line 2
    dynTextWidget_ScrollingCentered_skipLines->textLines[3].skipLineIfEmpty = true;                  // Skip empty line 3
    dynTextWidget_ScrollingCentered_skipLines->textLines[4].skipLineIfEmpty = true;                  // Skip empty line 4
    dynTextWidget_ScrollingCentered_skipLines->textLines[5].skipLineIfEmpty = true;                  // Skip empty line 5
    dynTextWidget_ScrollingCentered_skipLines->textLines[6].skipLineIfEmpty = true;                  // Skip empty line 6
    dynTextWidget_ScrollingCentered_skipLines->SetDynamicTextLines(
        {
            "H: Top Cententer",                                              // Centered at the top
            "", "", "", "", "", "",                                          // Empty lines. Those will be skipped, since the flag is set to skip empty lines
            "NO SKIP LINES - Scrollable: This line will scroll if too long." // Centered in the middle, scrolls if it exceeds width
            // This is line 8, since the empty lines are skipped, this line will be displayed in place of the Line 1 !
        });
    addWidget(dynTextWidget_ScrollingCentered_skipLines, 3000, "DynamicText_ScrollingCentered_skipLines");
    logInfoP("Added Dynamic Text widget with centered scrolling text and skipped empty lines to the display queue.");

    // Example Widget: Left-aligned text at the top and right-aligned text in the middle
    Widget* dynTextWidget_LeftTop_RightMiddle = new Widget(Widget::DisplayMode::DYNAMIC_TEXT);
    dynTextWidget_LeftTop_RightMiddle->textLines[0].textSize = 2; // Larger text for header
    dynTextWidget_LeftTop_RightMiddle->textLines[0].alignPos = TextDynamicAlign::ALIGN_LEFT | TextDynamicAlign::ALIGN_TOP;
    dynTextWidget_LeftTop_RightMiddle->textLines[1].alignPos = TextDynamicAlign::ALIGN_RIGHT | TextDynamicAlign::ALIGN_MIDDLE;
    dynTextWidget_LeftTop_RightMiddle->SetDynamicTextLine(0, "H: Left Aligned");         // Set the header text
    dynTextWidget_LeftTop_RightMiddle->SetDynamicTextLine(1, "Right Aligned in Middle"); // Set the line below the header
    addWidget(dynTextWidget_LeftTop_RightMiddle, 3000, "DynamicText_LeftTop_RightMiddle");
    logInfoP("Added Dynamic Text widget with left top and right middle alignment to the display queue.");

    // Example Widget: Print all possible characters on the display
    Widget* dynTextWidget_AllChars = new Widget(Widget::DisplayMode::DYNAMIC_TEXT);
    dynTextWidget_AllChars->textLines[0].textSize = 1; // Set the text size for the header
    dynTextWidget_AllChars->SetDynamicTextLines(       // Set the text lines and the information to display
        {
            "All Characters:",                   // Header
            "0123456789",                        // Line 1
            "ABCDEFGHIJKLMNOPQRSTUVWXYZ",        // Line 2
            "abcdefghijklmnopqrstuvwxyz",        // Line 3
            "!\"#$%&'()*+,-./:;<=>?@[\\]^_`{|}~" // Line 4
        });
    addWidget(dynTextWidget_AllChars, 3000, "DynamicText_AllChars");
    logInfoP("Added Dynamic Text widget with all characters to the display queue.");

    #ifdef QRCODE_WIDGET
    // Example Widget: Show a QR code
    Widget* QRCodeWidget = new Widget(Widget::DisplayMode::QR_CODE);                 // Create a new QR code widget
    QRCodeWidget->qrCodeWidget.setUrl("https://www.openknx.de");                     // Set the URL for the QR code
    QRCodeWidget->qrCodeWidget.setAlign(QRCodeWidget::QRCodeAlignPos::ALIGN_CENTER); // Set the alignment for the QR code
    addWidget(QRCodeWidget, 15000, "QRCode",
              DeviceDisplay::WidgetAction::StatusFlag |          // This is a status widget. The status flag will be displayed immediately
                  DeviceDisplay::WidgetAction::InternalEnabled | // This widget is enabled
                  DeviceDisplay::WidgetAction::AutoRemoveFlag);  // Remove this widget after display of the set duration time. Here 10sec.
    logInfoP("Added QR-Code widget to the display queue. With link: https://www.openknx.de");
    #endif

    // Example Widget: Console Widget. This widget is used to display a console simulatted output.
    Widget* myConsoleWidget = new Widget(Widget::DisplayMode::DYNAMIC_TEXT);
    myConsoleWidget->setAllowEmptyTextLines(true); // Allow empty text lines
    myConsoleWidget->textLines[0].textSize = 1;    // Set the text size for the header
    myConsoleWidget->textLines[0].alignPos = TextDynamicAlign::ALIGN_LEFT;
    myConsoleWidget->textLines[0].textColor = SSD1306_WHITE;
    myConsoleWidget->textLines[0].bgColor = SSD1306_BLACK;
    addWidget(myConsoleWidget, 30000, "consoleWidget");
    logInfoP("Added Console Widget to the display queue.");

    _demoWidgetSysInfo = true;
    _demoWidgeConsoleWidget = true;
}

/**
 * @brief This function is used to update the demo test widget System Information "SysInfo" and the console widget "consoleWidget".
 *        The function will update the widgets every second.
 */
void DeviceDisplay::demoSysinfoWidgetLoop()
{
    if (delayCheck(_demoTestWidgets_lastUpdateTime2, 1000) && isWidgetCurrentlyDisplayed("SysInfo")) // Update the display every second!
    {
        Widget* sysInfoWidget = getWidgetInfo("SysInfo")->widget;
        // Do the update only if the widget is currently displayed

        if (sysInfoWidget != nullptr)
        {
            sysInfoWidget->SetDynamicTextLine(1, String("Uptime: " + String(openknx.logger.buildUptime().c_str())).c_str());
            sysInfoWidget->SetDynamicTextLine(4, String("Addr.: " + String(openknx.info.humanIndividualAddress().c_str())).c_str());
            sysInfoWidget->SetDynamicTextLine(5, String("Free mem: " + String((float)freeMemory() / 1024) + " KiB").c_str());
            sysInfoWidget->SetDynamicTextLine(6, String("    (min. " + String((float)openknx.common.freeMemoryMin() / 1024) + " KiB)").c_str());
        }
    }
}

void DeviceDisplay::demoConsoleWidgetLoop()
{
    if (delayCheck(_demoTestWidgets_lastUpdateTime, 1000) && isWidgetCurrentlyDisplayed("consoleWidget")) // Update the display every second!
    {
        if (_demoTestWidgets_currentLineIndex < _demoTestWidgets_numLines)
        {
            DeviceDisplay::WidgetInfo* consoleWidgetInfo = getWidgetInfo("consoleWidget");
            Widget* consoleWidget = consoleWidgetInfo->widget;
            if (consoleWidget != nullptr)
            {
                consoleWidget->appendLine(_demoTestWidgets_conversationLines[_demoTestWidgets_currentLineIndex]);
                _demoTestWidgets_currentLineIndex++;
            }
        }
        else
        {
            _demoTestWidgets_currentLineIndex = 0;
        }
        _demoTestWidgets_lastUpdateTime = millis();
    }
}
#endif // DEMO_WIDGET_CMD_TESTS