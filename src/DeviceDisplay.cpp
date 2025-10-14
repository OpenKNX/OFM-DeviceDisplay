#ifdef DEVICE_DISPLAY_MODULE
#include "DeviceDisplay.h"


DeviceDisplay openknxDisplayModule;

/**
 * @brief Construct a new Device Display:: Device Display object
 */
DeviceDisplay::DeviceDisplay()
{
    // Constructor
}

DeviceDisplay::~DeviceDisplay()
{
    // Destructor
    #ifdef WIDGET_MANAGER
    if (_widgetManager)
    {
        delete _widgetManager;
        _widgetManager = nullptr;
    }
    #endif
}

/**
 * @brief Initialize the display module.
 * This function is called within the OpenKNX
 */
void DeviceDisplay::init()
{
    logInfoP("Init started...");
    _widgetManager = new WidgetsManager();
    _displayModule = new i2cDisplay();
    if (_displayModule == nullptr)
    {
        logErrorP("Display module not created!");
        return;
    }
    if (_widgetManager == nullptr)
    {
        logErrorP("Widget manager not created!");
        return;
    }
    
    #ifdef ARDUINO_ARCH_ESP32
    _displayModule->lcdSettings.i2cInst = &OKNXHW_DEVICE_DISPLAY_I2C_INST; // Set here the i2c instance to use. i2c0 or i2c1
    #else
    _displayModule->lcdSettings.i2cInst = OKNXHW_DEVICE_DISPLAY_I2C_INST; // Set here the i2c instance to use. i2c0 or i2c1
    #endif
    _displayModule->lcdSettings.sda = OKNXHW_DEVICE_DISPLAY_I2C_SDA; // Set the Hardware specific SDA pin for the display
    _displayModule->lcdSettings.scl = OKNXHW_DEVICE_DISPLAY_I2C_SCL; // Set the Hardware specific SCL pin for the display

    _displayModule->lcdSettings.i2cadress = OKNXHW_DEVICE_DISPLAY_I2C_ADDRESS; // Set here the i2c address of the display. I.e. 0x3C
    _displayModule->lcdSettings.width = OKNXHW_DEVICE_DISPLAY_WIDTH;           // Set here the width of the display. I.e. 128
    _displayModule->lcdSettings.height = OKNXHW_DEVICE_DISPLAY_HEIGHT;         // Set here the height of the display. I.e. 64

    _displayModule->lcdSettings.reset = -1; // We are not using a reset pin and set it to -1, which use the internal reset

    if (_displayModule->InitDisplay(_displayModule->lcdSettings) && _displayModule->display != nullptr)
    {
        logInfoP("Display initialized.");
        logInfoP("Display i2c Settings - i2cInt: %p, SDA: %d, SCL: %d, Address: 0x%02X, Width: %d, Height: %d",
                 _displayModule->lcdSettings.i2cInst, _displayModule->lcdSettings.sda, _displayModule->lcdSettings.scl,
                 _displayModule->lcdSettings.i2cadress, _displayModule->lcdSettings.width, _displayModule->lcdSettings.height);
    }
    else
    {
        logErrorP("Display initialization failed!");
    }

    #ifdef WIDGET_MANAGER
    
    _widgetManager->setDisplayModule(_displayModule); // Important: The display module for the widgets
    _widgetManager->setIdleTimeout(10000);

    // Power-Save-Konfiguration
    PowerSaveConfig powerSaveConfig;
    powerSaveConfig.enabled = true;
    powerSaveConfig.dimTimeout = 30000;         // 30s
    powerSaveConfig.screenSaverTimeout = 60000; // 1min
    powerSaveConfig.sleepTimeout = 300000;      // 5min
    powerSaveConfig.offTimeout = 0;             // nie
    powerSaveConfig.dimBrightness = 30;         // 30%
    powerSaveConfig.normalBrightness = 100;     // 100%
    _widgetManager->setPowerSaveConfig(powerSaveConfig);

    // Optional: Callbacks
    _widgetManager->setStateTransitionCallback([this](WidgetManagerState from, WidgetManagerState to) {
        logInfoP("State: %d -> %d", from, to);
    });

    _widgetManager->setPowerSaveCallback([this](PowerSaveMode from, PowerSaveMode to) {
        logInfoP("PowerSave: %d -> %d", from, to);
    });
    #endif
}

/**
 * @brief Initialize and add the default widgets to the widget manager.
 */
void DeviceDisplay::initializeWidgets()
{
    #ifdef WIDGET_MANAGER
    // Set the boot logo widget (only shown once at startup - must be added as first widget)
    WidgetBootLogo* bootLogoWidget = new WidgetBootLogo(3000, WidgetFlags::AutoRemove); // Create a new BootLogo widget
    _widgetManager->addWidget(bootLogoWidget);
    // ToDo: _widgetManager->setBootlogoWidget(bootLogoWidget); // Set the boot logo widget

    // Set the screensaver widget
    WidgetMatrixClassic* matrixClassicWidget = new WidgetMatrixClassic(5000, WidgetFlags::AutoRemove, 8); // Create a new MatrixClassic widget
    _widgetManager->setScreenSaverWidget(matrixClassicWidget);

    // WidgetLife* lifeWidget = new WidgetLife(2000, WidgetFlags::AutoRemove); // Create a new Life widget
    // _widgetManager->addWidget(lifeWidget);

    // WidgetStarfield* starfieldWidget = new WidgetStarfield(2000, WidgetFlags::AutoRemove, 10); // Create a new Starfield widget
    // _widgetManager->addWidget(starfieldWidget);

    // WidgetCube3D* cube3DWidget = new WidgetCube3D(2000, WidgetFlags::AutoRemove); // Create a new 3D Cube widget
    // _widgetManager->addWidget(cube3DWidget);

    // WidgetPong* pongWidget = new WidgetPong(2000, WidgetFlags::AutoRemove); // Create a new Pong widget
    // _widgetManager->addWidget(pongWidget);

    // WidgetRain* rainWidget = new WidgetRain(2000, WidgetFlags::AutoRemove, 6); // Create a new Rain widget
    // _widgetManager->addWidget(rainWidget);

    // WidgetMatrix* matrixWidget = new WidgetMatrix(5000, WidgetFlags::AutoRemove, 7); // Create a new Matrix widget
    // _widgetManager->addWidget(matrixWidget);

    // WidgetSysInfoLite* sysInfoLiteWidget = new WidgetSysInfoLite(5000, WidgetFlags::AutoRemove); // Create a new SysInfoLite widget
    // _widgetManager->addWidget(sysInfoLiteWidget);

    // WidgetOpenKNXLogo* openknxLogoWidget = new WidgetOpenKNXLogo(5000, WidgetFlags::AutoRemove); // Create a new OpenKNXLogo widget
    // _widgetManager->addWidget(openknxLogoWidget);

    // WidgetFireworks* fireworksWidget = new WidgetFireworks(10000, WidgetFlags::AutoRemove, 10); // Create a new Fireworks widget
    // _widgetManager->addWidget(fireworksWidget);

    // WidgetQRCode* qrcodeWidget = new WidgetQRCode(2000, WidgetFlags::DefaultWidget, "https://www.openknx.de", false); // Create a new QRcode widget
    // _widgetManager->addWidget(qrcodeWidget);

    // Default Widgets

    // Clock Widget, which will be displayed for 5 seconds, if there is no other widget in the queue, infitely.
    WidgetClock* clockWidget = new WidgetClock(5000, WidgetFlags::DefaultWidget, false); // Create a new Clock widget
    _widgetManager->addWidget(clockWidget);

    // Menu Widget, which will be displayed Initially, if there is no other widget in the queue, infitely.
    MenuWidget* menuWidget = new MenuWidget(10000, WidgetFlags::ManagedExternally);                                                                             // Create a new Menu widget

    // Info: Those actions are default for the MenuWidget - ManagedExternally, Background and WantsButtonInput!
    //menuWidget->setAction(WidgetFlags::ManagedExternally | WidgetFlags::Background | WidgetFlags::WantsButtonInput);
    _widgetManager->addWidget(menuWidget);

    WidgetProgMode* progModeWidget = new WidgetProgMode(); // Create a new ProgMode widget
    progModeWidget->setAction(WidgetFlags::ManagedExternally | WidgetFlags::StatusWidget);
    _widgetManager->addWidget(progModeWidget);

    _widgetManager->setup(); // Setup the widget manager
    _widgetManager->start(); // Start the widget manager
    #endif
}

/**
 * @brief Setup the default widgets for the display.
 * @param configured, will not be used
 */
void DeviceDisplay::setup(bool configured)
{
    logDebugP("setup...");
    _displayModule->SetDisplayVCOMDetect(0x20); // Set the VCOMH regulator output
    _displayModule->SetDisplayContrast(0xFF);   // Set the contrast of the display
    logDebugP("Initialize widgets...");
    initializeWidgets();
    setupButtons();

}

/**
 * @brief Process GroupObjects for the display module.
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
    //if(!configured) return;

    if (_displayModule->display == nullptr)
    {
        logErrorP("Display not initialized");
        return;
    }

    processButtons();

    static bool wasInProgMode = false;
    static Widget* progMode = nullptr;
    if (knx.progMode())
    {
        // ← NEU: Reset Power-Save-Timer bei ProgMode
        _widgetManager->userInteraction();

        if (!wasInProgMode)
        {
            if ((progMode = _widgetManager->getWidgetFromQueue("ProgMode")) != nullptr &&
                progMode->getState() != WidgetState::RUNNING)
            {
                logInfoP("ProgMode requested and will be displayed...");
                progMode->addAction(WidgetFlags::DisplayEnabled);
                wasInProgMode = true;
                logInfoP(" Current Action: %d", progMode->getAction());
            }
            else
            {
                logErrorP("ProgMode widget not found in queue!");
            }
        }
    }
    else if (wasInProgMode && progMode != nullptr)
    {
        logInfoP("ProgMode requested and will be removed...");
        progMode->removeAction(WidgetFlags::DisplayEnabled);
        wasInProgMode = false;
        logInfoP(" Current Action: %d", progMode->getAction());
    }

    _widgetManager->loop();

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
        if (command.compare(4, 1, "m") == 0) // Matrix Screensaver
        {
            logInfoP("Sending Matrix Screensaver to display.");
            WidgetMatrixClassic* matrixClassicWidget = new WidgetMatrixClassic(5000, WidgetFlags::NoAction, 8);
            matrixClassicWidget->setAction(WidgetFlags::AutoRemove | WidgetFlags::DefaultWidget);
            _widgetManager->addWidget(matrixClassicWidget);
            bRet = true;
        }
        else if (command.compare(4, 5, "clock") == 0) // Clock Screensaver
        {
            logInfoP("Sending Clock Screensaver to display.");
            WidgetClock* clockWidget = new WidgetClock(5000, WidgetFlags::NoAction, true);
            clockWidget->setAction(WidgetFlags::AutoRemove | WidgetFlags::DefaultWidget);
            _widgetManager->addWidget(clockWidget);
            bRet = true;
        }
        else if (command.compare(4, 5, "pong ") == 0) // Pong Screensaver
        {
            if (command.compare(9, 1, "s") == 0) // Set Pong Screensaver
            {
                logInfoP("Pong Screensaver is set to display. Remove it with 'ddc pong r'");
                WidgetPong* pongWidget = new WidgetPong(5000, WidgetFlags::NoAction);
                pongWidget->setAction(WidgetFlags::DefaultWidget);
                _widgetManager->addWidget(pongWidget);
                bRet = true;
            }
            if (command.compare(9, 1, "r") == 0) // Remove Screensaver
            {
                logInfoP("Removing Pong Screensaver from display...");
                Widget* widget = _widgetManager->getWidgetFromQueue("Pong");
                if (widget)
                {
                    widget->addAction(WidgetFlags::AutoRemove);
                    logInfoP("AutoRemove action added to Pong widget. Will be removed shortly.");
                }
                bRet = true;
            }
        }
        else if (command.compare(4, 10, "starfield ") == 0) // Starfield Screensaver
        {
            if (command.compare(14, 1, "s") == 0) // Set Starfield Screensaver
            {
                logInfoP("Starfield Screensaver is set to display. Remove it with 'ddc starfield r'");
                WidgetStarfield* starfieldWidget = new WidgetStarfield(5000, WidgetFlags::NoAction, 10);
                starfieldWidget->setAction(WidgetFlags::DefaultWidget);
                _widgetManager->addWidget(starfieldWidget);
                bRet = true;
            }
            if (command.compare(14, 1, "r") == 0) // Remove Screensaver
            {
                logInfoP("Removing Starfield Screensaver from display...");
                Widget* widget = _widgetManager->getWidgetFromQueue("Starfield");
                if (widget)
                {
                    logInfoP("Retrieved widget at %p with name %s", widget, widget->getName().c_str());
                    widget->addAction(WidgetFlags::AutoRemove);
                    logInfoP("AutoRemove action added to Starfield widget. Will be removed shortly.");
                }
                bRet = true;
            }
        }
        else if (command.compare(4, 7, "3dcube ") == 0) // 3D Cube Screensaver
        {
            if (command.compare(11, 1, "s") == 0) // Set 3D Cube Screensaver
            {
                logInfoP("3D Cube Screensaver is set to display. Remove it with 'ddc 3dcube r'");
                WidgetCube3D* cube3DWidget = new WidgetCube3D(5000, WidgetFlags::NoAction);
                cube3DWidget->setAction(WidgetFlags::DefaultWidget);
                _widgetManager->addWidget(cube3DWidget);
                bRet = true;
            }
            if (command.compare(11, 1, "r") == 0) // Remove Screensaver
            {
                logInfoP("Removing 3D Cube Screensaver from display...");
                Widget* widget = _widgetManager->getWidgetFromQueue("WidgetCube3D");
                if (widget)
                {
                    widget->addAction(WidgetFlags::AutoRemove);
                    logInfoP("AutoRemove action added to 3D Cube widget. Will be removed shortly.");
                }
                bRet = true;
            }
        }
        else if (command.compare(4, 1, "l") == 0 && command.size() < 6) // List all widgets
        {
            _widgetManager->logWidgetQueue();
            bRet = true;
        }
        else if (command.compare(4, 1, "i") == 0 && command.size() < 6) // Info about the widget manager
        {
            _widgetManager->logWidgetManagerSettings();
            bRet = true;
        }
    #ifdef DD_CONSOLE_CMDS
        else if (command.compare(4, 4, "dim ") == 0) // ddc dim <on|off|0-255>
        {
            if (command.compare(8, 2, "on") == 0)
            {
                // Display dimmen aktivieren
                _displayModule->display->dim(true);
                logInfoP("Display dimmed (ON)");
            }
            else if (command.compare(8, 3, "off") == 0)
            {
                // Display dimmen deaktivieren
                _displayModule->display->dim(false);
                logInfoP("Display not dimmed (OFF)");
            }
            else
            {
                int contrastValue = std::stoi(command.substr(8));
                if (contrastValue >= 0 && contrastValue <= 255)
                {
                    _displayModule->SetDisplayContrast(contrastValue);
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
                _displayModule->SetDisplayVCOMDetect(0x00);
                logInfoP("VCOM detect enabled");
            }
            else if (command.compare(9, 3, "off") == 0)
            {
                // Deaktiviert VCOM Detect
                _displayModule->SetDisplayVCOMDetect(0x20); // Set VCOMH to the default value
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
                    _displayModule->SetDisplayVCOMDetect(vcomValue);
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
                _displayModule->SetInvertDisplay(true);
                logInfoP("Display inverted");
            }
            else
            {
                _displayModule->SetInvertDisplay(false);
                logInfoP("Display not inverted");
            }
            bRet = true;
        }
        else if (command.compare(4, 7, "scroll ") == 0) // ddc scroll <right|left|diag_right|diag_left|start|stop|set_area>
        {
            if (command.compare(11, 1, "r") == 0) // Scrollen nach rechts
            {
                _displayModule->display->ssd1306_command(SSD1306_RIGHT_HORIZONTAL_SCROLL);
                _displayModule->display->ssd1306_command(0x00); // Startkolonne
                _displayModule->display->ssd1306_command(0x00); // Startseite
                _displayModule->display->ssd1306_command(0x07); // Scroll-Dauer
                _displayModule->display->ssd1306_command(0x00); // Scroll-Wiederholung
                _displayModule->display->ssd1306_command(0xFF); // Ende der Seite
                _displayModule->display->ssd1306_command(SSD1306_ACTIVATE_SCROLL);
                logInfoP("Right horizontal scroll started");
            }
            else if (command.compare(11, 1, "l") == 0) // Scrollen nach links
            {
                _displayModule->display->ssd1306_command(SSD1306_LEFT_HORIZONTAL_SCROLL);
                _displayModule->display->ssd1306_command(0x00); // Startkolonne
                _displayModule->display->ssd1306_command(0x00); // Startseite
                _displayModule->display->ssd1306_command(0x07); // Scroll-Dauer (7 Frames)
                _displayModule->display->ssd1306_command(0x00); // Scroll-Wiederholung
                _displayModule->display->ssd1306_command(0xFF); // Ende der Seite
                _displayModule->display->ssd1306_command(SSD1306_ACTIVATE_SCROLL);
                logInfoP("Left horizontal scroll started");
            }
            else if (command.compare(11, 2, "dr") == 0) // Diagonales Scrollen nach rechts
            {
                _displayModule->display->ssd1306_command(SSD1306_VERTICAL_AND_RIGHT_HORIZONTAL_SCROLL);
                _displayModule->display->ssd1306_command(0x00); // Startkolonne
                _displayModule->display->ssd1306_command(0x00); // Startseite
                _displayModule->display->ssd1306_command(0x07); // Scroll-Dauer
                _displayModule->display->ssd1306_command(0x00); // Scroll-Wiederholung
                _displayModule->display->ssd1306_command(0xFF); // Ende der Seite
                _displayModule->display->ssd1306_command(SSD1306_ACTIVATE_SCROLL);
                logInfoP("Diagonal scroll (right) started");
            }
            else if (command.compare(11, 2, "dl") == 0) // Diagonales Scrollen nach links
            {
                _displayModule->display->ssd1306_command(SSD1306_VERTICAL_AND_LEFT_HORIZONTAL_SCROLL);
                _displayModule->display->ssd1306_command(0x00); // Startkolonne
                _displayModule->display->ssd1306_command(0x00); // Startseite
                _displayModule->display->ssd1306_command(0x07); // Scroll-Dauer
                _displayModule->display->ssd1306_command(0x00); // Scroll-Wiederholung
                _displayModule->display->ssd1306_command(0xFF); // Ende der Seite
                _displayModule->display->ssd1306_command(SSD1306_ACTIVATE_SCROLL);
                logInfoP("Diagonal scroll (left) started");
            }
            else if (command.compare(11, 5, "start") == 0) // Scrollen starten
            {
                _displayModule->display->ssd1306_command(SSD1306_ACTIVATE_SCROLL);
                logInfoP("Scrolling activated");
            }
            else if (command.compare(11, 4, "stop") == 0) // Scrollen stoppen
            {
                _displayModule->display->ssd1306_command(SSD1306_DEACTIVATE_SCROLL);
                logInfoP("Scrolling stopped");
            }
            else if (command.compare(11, 2, "sa") == 0) // Scrollbereich setzen
            {
                // Hier können wir den Bereich für das vertikale Scrollen definieren
                _displayModule->display->ssd1306_command(SSD1306_SET_VERTICAL_SCROLL_AREA);
                _displayModule->display->ssd1306_command(0x00); // Startseite
                _displayModule->display->ssd1306_command(0x3F); // Endseite (64px für 64px Display)
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
                _displayModule->SetDisplayContrast(contrastValue);
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
                _displayModule->SetDisplayPreCharge(0xF1);
                logInfoP("Charge pump enabled");
            }
            else if (command.compare(15, 3, "off") == 0)
            {
                _displayModule->SetDisplayPreCharge(0x10);
                logInfoP("Charge pump disabled");
            }
        }
        else if (command.compare(4, 9, "segremap ") == 0) // ddc segremap <on|off>
        {
            if (command.compare(13, 2, "on") == 0)
            {
                // Segmentzuordnung umkehren (Segment Mapping)
                _displayModule->display->ssd1306_command(SSD1306_SEGREMAP);
                _displayModule->display->ssd1306_command(0xA1); // Umkehrung der Segmentzuordnung
                logInfoP("Segment remapping enabled");
            }
            else if (command.compare(13, 3, "off") == 0)
            {
                // Segmentzuordnung zurücksetzen
                _displayModule->display->ssd1306_command(SSD1306_SEGREMAP);
                _displayModule->display->ssd1306_command(0xA0); // Standard Segmentzuordnung
                logInfoP("Segment remapping disabled");
            }
        }
        else if (command.compare(4, 11, "displayall ") == 0) // ddc displayall <on|off>
        {
            if (command.compare(15, 2, "on") == 0)
            {
                // Alle Pixel auf dem Display einschalten
                _displayModule->display->ssd1306_command(SSD1306_DISPLAYALLON);
                logInfoP("Display all-on mode enabled");
            }
            else if (command.compare(15, 3, "off") == 0)
            {
                // Alle Pixel wieder normal anzeigen
                _displayModule->display->ssd1306_command(SSD1306_DISPLAYALLON_RESUME);
                logInfoP("Display all-on mode disabled, resumed normal display");
            }
        }
    #endif // DD_CONSOLE_CMDS
    #ifdef OPENKNX_RUNTIME_STAT
        else if (command.compare(4, 8, "runtime ") == 0)
        {
            logInfoP("DeviceDisplay Runtime Statistics: (Uptime=%dms)", millis());
            logIndentUp();

            OpenKNX::Stat::RuntimeStat::showStatHeader();
            //_loop_DisplayModule->showStat("loop_only", 0, true, true);

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
            openknx.console.printHelpLine("ddc l", "List all widgets");
            openknx.console.printHelpLine("ddc i", "Info about the widget manager");
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


/**
 * @brief Setup button pins
 */
void DeviceDisplay::setupButtons()
{
#ifdef USE_GPIO_MODULE
    if (!openknx.gpio.isInitialized(1))
    {
        logErrorP("GPIO Module not initialized - buttons disabled");
        return;
    }

    _buttonUp = FRONT_CTRL_UP;
    _buttonDown = FRONT_CTRL_DOWN;
    _buttonSelect = FRONT_CTRL_OK;
    _buttonLeft = FRONT_CTRL_LEFT;
    _buttonRight = FRONT_CTRL_RIGHT;

    const uint16_t pins[] = {
        _buttonUp, _buttonDown, _buttonSelect,
        _buttonLeft, _buttonRight
    };

    for (auto pin : pins)
    {
        openknx.gpio.pinMode(pin, INPUT, true, 0);
    }

    _frontPlateEnabled = true;
    logInfoP("Front plate buttons initialized");
#else
    logInfoP("GPIO module not available - buttons disabled");
#endif
}

/**
 * @brief Process button inputs and forward to active widget
 */
void DeviceDisplay::processButtons()
{
    if (!_frontPlateEnabled) return;

    uint32_t currentTime = millis();
    if (currentTime - _lastButtonCheck < _buttonCheckInterval) return;

    _lastButtonCheck = currentTime;

    // Get active widget that wants buttons
    Widget* activeWidget = _widgetManager->getActiveButtonWidget();
    // Check all buttons
    ButtonEvent* event = nullptr;

    if ((event = checkButton(_buttonUp, ButtonType::UP, 0)) != nullptr)
    {
        logDebugP("Button UP event detected");
        // If no widget wants buttons, just wake up display

        if (!activeWidget)
        {
            _widgetManager->wakeUpDisplay();
            logDebugP("Button UP pressed - waking display");
            delete event;
            return;
        }
        
        if (activeWidget->handleButtonEvent(*event))
        {
            _widgetManager->userInteraction();  // Wake-Up Display
        }
        delete event;
    }

    if ((event = checkButton(_buttonDown, ButtonType::DOWN, 1)) != nullptr)
    {
        logDebugP("Button DOWN event detected");
        if (!activeWidget)
        {
            _widgetManager->wakeUpDisplay();
            logDebugP("Button DOWN pressed - waking display");
            delete event;
            return;
        }
        
        if (activeWidget->handleButtonEvent(*event))
        {
            _widgetManager->userInteraction();
        }
        delete event;
    }

    if ((event = checkButton(_buttonSelect, ButtonType::SELECT, 2)) != nullptr)
    {
        logDebugP("Button SELECT event detected");
        if (!activeWidget)
        {
            _widgetManager->wakeUpDisplay();
            logDebugP("Button SELECT pressed - waking display and activating menu");
            delete event;
            return;
        }
        
        if (activeWidget->handleButtonEvent(*event))
        {
            _widgetManager->userInteraction();
        }
        delete event;
    }

    if ((event = checkButton(_buttonLeft, ButtonType::LEFT, 3)) != nullptr)
    {
        logDebugP("Button LEFT event detected");
        if (!activeWidget)
        {
            _widgetManager->wakeUpDisplay();
            logDebugP("Button LEFT pressed - waking display");
            delete event;
            return;
        }
        
        if (activeWidget->handleButtonEvent(*event))
        {
            _widgetManager->userInteraction();
        }
        delete event;
    }

    if ((event = checkButton(_buttonRight, ButtonType::RIGHT, 4)) != nullptr)
    {
        logDebugP("Button RIGHT event detected");
        if (!activeWidget)
        {
            _widgetManager->wakeUpDisplay();
            logDebugP("Button RIGHT pressed - waking display");
            delete event;
            return;
        }
        
        if (activeWidget->handleButtonEvent(*event))
        {
            _widgetManager->userInteraction();
        }
        delete event;
    }
}
/**
 * @brief Check a single button for state changes
 * @param pin GPIO pin to check
 * @param type Button type
 * @param index Array index for state tracking
 * @return ButtonEvent* if state changed, nullptr otherwise
 */
ButtonEvent* DeviceDisplay::checkButton(uint16_t pin, ButtonType type, size_t index)
{
    bool isPressed = readButton(pin);

    // LEFT button is active-LOW, invert
    if (type == ButtonType::LEFT)
    {
        isPressed = !isPressed;
    }

    uint32_t currentTime = millis();
    ButtonEvent* event = nullptr;

    // Button was pressed
    if (isPressed && !_buttonPressed[index])
    {
        _buttonPressed[index] = true;
        _buttonPressTime[index] = currentTime;
        event = new ButtonEvent(type, ButtonAction::PRESS);
    }
    // Button was released
    else if (!isPressed && _buttonPressed[index])
    {
        _buttonPressed[index] = false;

        uint32_t pressDuration = currentTime - _buttonPressTime[index];

        // Check press duration
        if (pressDuration > 5000)
        {
            event = new ButtonEvent(type, ButtonAction::VERY_LONG_PRESS);
        }
        else if (pressDuration > 500)
        {
            event = new ButtonEvent(type, ButtonAction::LONG_PRESS);
        }
        else
        {
            event = new ButtonEvent(type, ButtonAction::RELEASE);
        }
    }

    return event;
}

/**
 * @brief Read button state from GPIO
 * @param pin GPIO pin to read
 * @return true if button is pressed
 */
bool DeviceDisplay::readButton(uint16_t pin)
{
#ifdef USE_GPIO_MODULE
    return openknx.gpio.digitalRead(pin);
#else
    return false;
#endif
}

#endif