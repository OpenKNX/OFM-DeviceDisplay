#include "DeviceDisplay.h"
#include "OpenKNX.h"

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
    // ERROR_REQUIRED_DEFINE(OKNXHW_DEVICE_DISPLAY_I2C_0_1);
#endif

#ifndef OKNXHW_DEVICE_DISPLAY_I2C_SDA
    // ERROR_REQUIRED_DEFINE(OKNXHW_DEVICE_DISPLAY_I2C_SDA);
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
#ifndef WIDGET_MANAGER
    initializeWidgets(); // Setup default widget queue
#else
    widgetManager.setDisplayModule(&displayModule); // The display module for the widgets

    WidgetLife* lifeWidget = new WidgetLife(2000, WidgetFlags::AutoRemove); // Create a new Life widget
    widgetManager.addWidget(lifeWidget);

    WidgetStarfield* starfieldWidget = new WidgetStarfield(2000, WidgetFlags::AutoRemove, 10); // Create a new Starfield widget
    widgetManager.addWidget(starfieldWidget);

    WidgetQRCode* qrcodeWidget = new WidgetQRCode(2000, WidgetFlags::AutoRemove, "https://www.openknx.de", false); // Create a new QRcode widget
    widgetManager.addWidget(qrcodeWidget);

    WidgetCube3D* cube3DWidget = new WidgetCube3D(2000, WidgetFlags::AutoRemove); // Create a new 3D Cube widget
    widgetManager.addWidget(cube3DWidget);

    WidgetPong* pongWidget = new WidgetPong(2000, WidgetFlags::AutoRemove); // Create a new Pong widget
    widgetManager.addWidget(pongWidget);

    WidgetRain* rainWidget = new WidgetRain(2000, WidgetFlags::AutoRemove, 6); // Create a new Rain widget
    widgetManager.addWidget(rainWidget);

    WidgetMatrix* matrixWidget = new WidgetMatrix(5000, WidgetFlags::AutoRemove, 7); // Create a new Matrix widget
    widgetManager.addWidget(matrixWidget);

    WidgetMatrixClassic* matrixClassicWidget = new WidgetMatrixClassic(5000, WidgetFlags::AutoRemove, 8); // Create a new MatrixClassic widget
    widgetManager.addWidget(matrixClassicWidget);

    WidgetClock* clockWidget = new WidgetClock(3000, WidgetFlags::DefaultWidget, true); // Create a new Clock widget
    widgetManager.addWidget(clockWidget);

    WidgetSysInfoLite* sysInfoLiteWidget = new WidgetSysInfoLite(5000, WidgetFlags::AutoRemove); // Create a new SysInfoLite widget
    widgetManager.addWidget(sysInfoLiteWidget);

    WidgetOpenKNXLogo* openknxLogoWidget = new WidgetOpenKNXLogo(5000, WidgetFlags::AutoRemove); // Create a new OpenKNXLogo widget
    widgetManager.addWidget(openknxLogoWidget);

    WidgetFireworks* fireworksWidget = new WidgetFireworks(10000, WidgetFlags::AutoRemove, 10); // Create a new Fireworks widget
    widgetManager.addWidget(fireworksWidget);

    WidgetBootLogo* bootLogoWidget = new WidgetBootLogo(5000, WidgetFlags::AutoRemove); // Create a new BootLogo widget
    widgetManager.addWidget(bootLogoWidget);

    MenuWidget* menuWidget = new MenuWidget(20000, WidgetFlags::ManagedExternally,  // Is managed externally
    0x107,  // DD_CTRL_PIN7_UP_BUTTON,     // But will stopped for testing after 20 seconds
    0x104,  // DD_CTRL_PIN4_DOWN_BUTTON,
    0x106,  // DD_CTRL_PIN6_OK_BUTTON,
    0x100,  // DD_CTRL_PIN0_LEFT_BUTTON, Optional: 0x103 can be used for left button
    0x105   // DD_CTRL_PIN5_RIGHT_BUTTON 
    );                                                                        // Create a new Menu widget

    menuWidget->setAction(WidgetFlags::ManagedExternally | WidgetFlags::Background);
    setMenuWidget(menuWidget);                                                // For Internal use in this class
    widgetManager.addWidget(menuWidget);

    WidgetProgMode* progModeWidget = new WidgetProgMode(); // Create a new ProgMode widget
    progModeWidget->setAction(WidgetFlags::ManagedExternally | WidgetFlags::StatusWidget);
    widgetManager.addWidget(progModeWidget);

    //widgetManager.setup(); // Setup the widgets ToDo! CHeck whats going wrong here
    widgetManager.start(); // Start the widgets 
#endif
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
    static Widget* progMode = nullptr;
    if (knx.progMode())
    {
        lastDisplayDimTimer_ = millis(); // Reset the display dim timer if prog mode is active
        if (!wasInProgMode &&
            (progMode = widgetManager.getWidgetFromQueue("ProgMode")) != nullptr &&
            progMode->getState() != WidgetState::RUNNING)
        {
            logInfoP("ProgMode requested and will be displayed...");
            progMode->addAction(WidgetFlags::DisplayEnabled);
            wasInProgMode = true;
            logInfoP(" Current Action: %d", progMode->getAction());
        }
    }
    else if (wasInProgMode && progMode != nullptr)
    {
        logInfoP("ProgMode requested and will be removed...");
        progMode->removeAction(WidgetFlags::DisplayEnabled);
        wasInProgMode = false;
        logInfoP(" Current Action: %d", progMode->getAction());
    }

    RUNTIME_MEASURE_BEGIN(_loopWidgets);
    widgetManager.loop();
    RUNTIME_MEASURE_END(_loopWidgets);

    //RUNTIME_MEASURE_BEGIN(_loopDisplayModule);
    //displayModule.loop();
    //RUNTIME_MEASURE_END(_loopDisplayModule);
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
        }
        else if (command.compare(4, 6, "press ") == 0) // Simuliere Button-Press
        {
            if (_menuWidget) // Prüfen, ob MenuWidget referenziert wurde
            {
                if (command.compare(10, 1, "w") == 0) // "w" = Up
                {
                    _menuWidget->externalNavigateUp();
                    bRet = true;
                }
                else if (command.compare(10, 1, "s") == 0) // "s" = Down
                {
                    _menuWidget->externalNavigateDown();
                    bRet = true;
                }
                else if (command.compare(10, 1, "e") == 0) // "e" = Select
                {
                    _menuWidget->externalSelectItem();
                    bRet = true;
                }
                else if (command.compare(10, 1, "p") == 0) // "p" = Pause
                {
                    widgetManager.getCurrentWidget()->pause();
                    bRet = true;
                }
                else if (command.compare(10, 1, "r") == 0) // "r" = Resume
                {
                    widgetManager.getCurrentWidget()->resume();
                    bRet = true;
                }
                else if (command.compare(10, 1, "x") == 0) // "x" = Stop
                {
                    widgetManager.getCurrentWidget()->stop();
                    bRet = true;
                }
            }
        }
        else if (command.compare(4, 2, "m ") == 0) // Matrix Screensaver
        {
            if (command.compare(6, 1, "s") == 0) // Set Matrix Screensaver
            {
                logInfoP("Sending Matrix Screensaver to display. Remove it with 'ddc m r'");
            }
            else if (command.compare(6, 1, "r") == 0) // Remove Screensaver
            {
                logInfoP("Removing Matrix Screensaver from display...");
            }
            bRet = true;
        }
        else if (command.compare(4, 6, "clock ") == 0) // Clock Screensaver
        {
            if (command.compare(10, 1, "s") == 0) // Set Clock Screensaver
            {
                logInfoP("Sending Clock Screensaver to display. Remove it with 'ddc clock r'");
            }
            else if (command.compare(10, 1, "r") == 0) // Remove Screensaver
            {
                logInfoP("Removing Clock Screensaver from display...");
            }
            bRet = true;
        }
        else if (command.compare(4, 5, "pong ") == 0) // Pong Screensaver
        {
            if (command.compare(9, 1, "s") == 0) // Set Pong Screensaver
            {
                logInfoP("Pong Screensaver is set to display. Remove it with 'ddc pong r'");
                bRet = true;
            }
            if (command.compare(9, 1, "r") == 0) // Remove Screensaver
            {
                logInfoP("Removing Pong Screensaver from display...");
                bRet = true;
            }
        }
        else if (command.compare(4, 5, "rain ") == 0) // Rainfall Screensaver
        {
            if (command.compare(9, 1, "s") == 0) // Set Rainfall Screensaver
            {
                logInfoP("Rainfall Screensaver is set to display. Remove it with 'ddc rain r'");
                bRet = true;
            }
            if (command.compare(9, 1, "r") == 0) // Remove Screensaver
            {
                logInfoP("Removing Rainfall Screensaver from display...");
                bRet = true;
            }
        }
        else if (command.compare(4, 7, "matrix ") == 0) // Rainfall Screensaver
        {
            if (command.compare(11, 1, "s") == 0) // Set Rainfall Screensaver
            {
                logInfoP("Matrix Screensaver is set to display. Remove it with 'ddc matrix r'");
                bRet = true;
            }
            if (command.compare(11, 1, "r") == 0) // Remove Screensaver
            {
                logInfoP("Removing Matrix Screensaver from display...");
                bRet = true;
            }
        }
        else if (command.compare(4, 10, "starfield ") == 0) // Starfield Screensaver
        {
            if (command.compare(14, 1, "s") == 0) // Set Starfield Screensaver
            {
                logInfoP("Starfield Screensaver is set to display. Remove it with 'ddc starfield r'");
                bRet = true;
            }
            if (command.compare(14, 1, "r") == 0) // Remove Screensaver
            {
                logInfoP("Removing Starfield Screensaver from display...");
                bRet = true;
            }
        }
        else if (command.compare(4, 7, "3dcube ") == 0) // 3D Cube Screensaver
        {
            if (command.compare(11, 1, "s") == 0) // Set 3D Cube Screensaver
            {
                logInfoP("3D Cube Screensaver is set to display. Remove it with 'ddc 3dcube r'");
                bRet = true;
            }
            if (command.compare(11, 1, "r") == 0) // Remove Screensaver
            {
                logInfoP("Removing 3D Cube Screensaver from display...");
                bRet = true;
            }
        }
        else if (command.compare(4, 5, "life ") == 0) // Life Screensaver
        {
            if (command.compare(9, 1, "s") == 0) // Set Life Screensaver
            {
                logInfoP("Life Screensaver is set to display. Remove it with 'ddc life r'");
                bRet = true;
            }
            if (command.compare(9, 1, "r") == 0) // Remove Screensaver
            {
                logInfoP("Removing Life Screensaver from display...");
                bRet = true;
            }
        }
        else if (command.compare(4, 8, "openknx ") == 0) // OpenKNX Team Intro
        {
            if (command.compare(12, 1, "s") == 0) // Set OpenKNX Team Intro
            {
                logInfoP("OpenKNX Team Intro is set to display. Remove it with 'ddc openknx_team r'");
                bRet = true;
            }
            if (command.compare(12, 1, "r") == 0) // Remove Screensaver
            {
                logInfoP("Removing OpenKNX Team Intro from display...");
                bRet = true;
            }
        }
        else if (command.compare(4, 1, "c") == 0) // Console simulation output widget
        {
        }
        else if (command.compare(4, 1, "l") == 0 && command.size() < 6) // List all widgets
        {
            //logInfoP("Total Widgets: %d:", widgetsQueue.size());
            //for (size_t i = 0; i < widgetsQueue.size(); ++i)
            //{
            //    WidgetInfo& widgetInfo = widgetsQueue[i];
            //    // Try to create a table with the widget information. The columns must be aligned.
            //    logInfoP("Order: %d | Name: %s | Action: %d | Duration: %d", i, widgetInfo.name.c_str(), widgetInfo.action, widgetInfo.duration);
            //}
            //logInfoP("---------------------------------------------------------");
            //bRet = true;
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
        else if (command.compare(4, 2, "qr") == 0) // Show QR-Code
        {
        }
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
                    //for (size_t i = 0; i < widgetsQueue.size(); ++i)
                    //{
                    //    //WidgetInfo& widgetInfo = widgetsQueue[i];
                    //    //widgetInfo.widget->_WidgetRutimeStat.showStat("widget_" + widgetInfo.name, 0, true, true);
                    //}
                    bRet = true;
                }
                else
                {
                    //std::string WidgetName = command.substr(19);
                    //if (!WidgetName.empty())
                    //{
                    //    WidgetInfo* widgetInfo = getWidgetInfo(WidgetName);
                    //    if (widgetInfo && widgetInfo->widget != nullptr)
                    //    {
                    //        widgetInfo->widget->_WidgetRutimeStat.showStat("widget_" + WidgetName, 0, true, true);
                    //    }
                    //    else
                    //    {
                    //        logErrorP("Widgets '%s' not found!", WidgetName.c_str());
                    //    }
                    //}
                }
            }
            else
            {
                if (command.compare(12, 7, "widgets") == 0) {}
                    //_loopWidgets.showStat("widgets", 0, true, true);
                else if (command.compare(12, 3, "dim") == 0) {}
                    //_loopRuntimesDim.showStat("dim", 0, true, true);
    #ifdef DEMO_WIDGET_CMD_TESTS
                else if (command.compare(12, 12, "demo_widgets") == 0) {}
                    //_loopDemoWidgets.showStat("demo_widgets", 0, true, true);
    #endif
                else if (command.compare(12, 4, "loop") == 0) {}
                    //_loopDisplayModule.showStat("loop_only", 0, true, true);
                else if (command.compare(12, 3, "all") == 0) 
                {
                    //_loopWidgets.showStat("widgets", 0, true, true);
                    //_loopRuntimesDim.showStat("dim", 0, true, true);
                    //_loopDemoWidgets.showStat("demo_widgets", 0, true, true);
                    //_loopDisplayModule.showStat("loop_only", 0, true, true);
                }
                else
                    logErrorP("Invalid runtime command.");
            }

            logIndentDown();
            bRet = true;
            // return true;
        }
#endif // OPENKNX_RUNTIME_STAT
        else
        {
            openknx.logger.begin();
            openknx.logger.log("");
            openknx.logger.color(CONSOLE_HEADLINE_COLOR);
            openknx.logger.log("======================= Help: Device Display Control ===========================");
            openknx.logger.color(0);
            openknx.logger.log("Command(s)               Description");
            openknx.console.printHelpLine("ddc c <text>", "Print/Update Console Widgets");
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
#ifdef WIDGET_MANAGER
            openknx.console.printHelpLine("ddc press <KEY>", "Simulates the menu widget button press");
            openknx.console.printHelpLine("ddc press w ", "UP - Button press (W)");
            openknx.console.printHelpLine("ddc press s ", "DOWN - Button press (S)");
            openknx.console.printHelpLine("ddc press e ", "ENTER - Button press (E)");
            openknx.console.printHelpLine("ddc press p ", "PAUSE the current widget");
            openknx.console.printHelpLine("ddc press r ", "RESUME the current widget");
            openknx.console.printHelpLine("ddc press x ", "STOP the current widget");
#endif // WIDGET_MANAGER

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
            openknx.console.printHelpLine("ddc runtime widget <'widget_name'>", "Show Widgets runtime statistics. Use 'ddc l' to list all widgets.");
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