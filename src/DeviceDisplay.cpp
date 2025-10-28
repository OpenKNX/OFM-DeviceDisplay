#ifdef DEVICE_DISPLAY_MODULE
    #include "DDCLoggerHelp.h"
    #include "DeviceDisplay.h"
    #include "Devices/ButtonManager.h"
    #include "Devices/i2cDisplay.h"
    #include "WidgetsManager.h"
    // Widget includes
    #include "Menu/Menu.h"
    #include "Widgets/BootLogo.h"
    #include "Widgets/Clock.h"
    #include "Widgets/MatrixClassic.h"
    #include "Widgets/ProgMode.h"
    #ifdef WIDGET_CONSOLE
        #include "Widgets/Console.h"
    #endif

DeviceDisplay openknxDisplayModule;

// ============================================================================
// Constructor / Destructor
// ============================================================================

DeviceDisplay::DeviceDisplay()
{
}

DeviceDisplay::~DeviceDisplay()
{
    delete _buttonManager;
    delete _ddcLoggerHelp;
    delete _widgetManager;
    delete _displayModule;
}

// ============================================================================
// Initialization
// ============================================================================

void DeviceDisplay::init()
{
    logInfoP("Init started...");

    // Create core components
    _displayModule = new i2cDisplay();
    _widgetManager = new WidgetsManager();
    _buttonManager = new ButtonManager(_widgetManager);
    _ddcLoggerHelp = new DDCLoggerHelp(_widgetManager, _displayModule);

    if (!_displayModule || !_widgetManager || !_buttonManager || !_ddcLoggerHelp)
    {
        logErrorP("Failed to create components!");
        return;
    }

    _displayModule->lcdSettings.i2cInst = &OKNXHW_DEVICE_DISPLAY_I2C_INST;

    _displayModule->lcdSettings.sda = OKNXHW_DEVICE_DISPLAY_I2C_SDA;
    _displayModule->lcdSettings.scl = OKNXHW_DEVICE_DISPLAY_I2C_SCL;
    _displayModule->lcdSettings.i2cadress = OKNXHW_DEVICE_DISPLAY_I2C_ADDRESS;
    _displayModule->lcdSettings.width = OKNXHW_DEVICE_DISPLAY_WIDTH;
    _displayModule->lcdSettings.height = OKNXHW_DEVICE_DISPLAY_HEIGHT;
    _displayModule->lcdSettings.reset = -1;

    if (!_displayModule->InitDisplay(_displayModule->lcdSettings) || !_displayModule->display)
    {
        logErrorP("Display initialization failed!");
        return;
    }

    logInfoP("Display initialized (%dx%d @ 0x%02X)",
             _displayModule->lcdSettings.width,
             _displayModule->lcdSettings.height,
             _displayModule->lcdSettings.i2cadress);

    // Configure WidgetManager
    _widgetManager->setDisplayModule(_displayModule);
    _widgetManager->setIdleTimeout(10000);

    PowerSaveConfig config;
    config.enabled = true;
    config.dimTimeout = 30000;
    config.screenSaverTimeout = 60000;
    config.sleepTimeout = 300000;
    config.offTimeout = 0;
    config.dimBrightness = 30;
    config.normalBrightness = 100;
    _widgetManager->setPowerSaveConfig(config);

    // Optional callbacks
    _widgetManager->setStateTransitionCallback([this](WidgetManagerState from, WidgetManagerState to) {
        logInfoP("State %d -> %d", from, to);
    });

    _widgetManager->setPowerSaveCallback([this](PowerSaveMode from, PowerSaveMode to) {
        logInfoP("PowerSave %d -> %d", from, to);
    });
}

void DeviceDisplay::setup(bool configured)
{
    logDebugP("setup...");

    // Display hardware settings
    _displayModule->SetDisplayVCOMDetect(0x20);
    _displayModule->SetDisplayContrast(0xFF);

    // Initialize widgets
    initializeWidgets();

    // Setup button input
    if (_buttonManager->setup())
    {
        logInfoP("Buttons initialized");
    }

    // Setup help system for console commands
    _ddcLoggerHelp->setup();

    #ifdef WIDGET_CONSOLE
    _ddcLoggerHelp->setConsoleWidget(_consoleWidget);
    #endif
}

/**********************************************************************
 ********************** Initialize Default Widgets ********************
 **********************************************************************/
void DeviceDisplay::initializeWidgets()
{
    // Boot logo (first widget, auto-remove after 3s)
    _widgetManager->addWidget(new WidgetBootLogo(3000, WidgetFlags::AutoRemove));

    // Screensaver
    _widgetManager->setScreenSaverWidget(
        new WidgetMatrixClassic(5000, WidgetFlags::AutoRemove, 8));

    #ifdef WIDGET_CONSOLE
    // Console widget (persistent)
    _consoleWidget = new WidgetConsole(60000);
    _widgetManager->addWidget(_consoleWidget);
    #endif

    // Clock (default widget)
    _widgetManager->addWidget(
        new WidgetClock(5000, WidgetFlags::DefaultWidget, false));

    // Menu (background widget)
    _widgetManager->addWidget(
        new MenuWidget(10000, WidgetFlags::ManagedExternally));

    // ProgMode (status widget with CRITICAL priority)
    WidgetProgMode* progMode = new WidgetProgMode();
    progMode->setAction(WidgetFlags::ManagedExternally | WidgetFlags::StatusWidget);
    progMode->setPriority(WidgetPriority::WIDGET_PRIO_CRITICAL);
    _widgetManager->addWidget(progMode);

    _widgetManager->setup();
    _widgetManager->start();

    logInfoP("Widgets initialized");
}

void DeviceDisplay::processInputKo(GroupObject& obj)
{
    // TODO: Implement KO processing for display control
}

/**
 * @brief Main loop for device display
 */
void DeviceDisplay::loop(bool configured)
{
    static bool displayErrorShown = false;
    if (!_displayModule->display)
    {
        if (!displayErrorShown)
        {
            logErrorP("Display not initialized");
            displayErrorShown = true;
        }
        return;
    }

    // Process button input
    _buttonManager->loop();

    // Handle ProgMode widget
    handleProgMode();

    // Update display (only when CPU time available)
    if (openknx.freeLoopTime())
    {
        _widgetManager->loop();
    }
}

/**
 * @brief Handle ProgMode widget state changes based on KNX programming mode
 */
void DeviceDisplay::handleProgMode()
{
    static Widget* progModeWidget = nullptr;
    static bool wasActive = false;

    bool isActive = knx.progMode();

    if (isActive && !wasActive)
    {
        _widgetManager->userInteraction();
        if (!progModeWidget)
            progModeWidget = _widgetManager->getWidgetFromQueue("ProgMode");

        if (progModeWidget && progModeWidget->getState() != WidgetState::RUNNING)
        {
            progModeWidget->addAction(WidgetFlags::DisplayEnabled);
            logInfoP("ProgMode activated");
        }
        wasActive = true;
    }
    else if (!isActive && wasActive)
    {
        if (progModeWidget)
        {
            progModeWidget->removeAction(WidgetFlags::DisplayEnabled);
            logInfoP("ProgMode deactivated");
        }
        wasActive = false;
    }
}

/**********************************************************************
 ****************************** show HELP******************************
 **********************************************************************/
void DeviceDisplay::showHelp()
{
    openknx.console.printHelpLine("ddc", "Device Display Control. Use 'ddc ?' for help.");
}
bool DeviceDisplay::processCommand(const std::string command, bool diagnose)
{
    if (diagnose) return false;
    return _ddcLoggerHelp->processCommand(command);
}

#endif // DEVICE_DISPLAY_MODULE