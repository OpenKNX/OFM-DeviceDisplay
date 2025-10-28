#ifdef DEVICE_DISPLAY_MODULE
    #include "WidgetsManager.h"
    #include "OpenKNX.h"

/**
 * @brief Calls setup() for all widgets in the queue
 */
void WidgetsManager::setup()
{
    for (auto& widget : _widgetQueue)
    {
        if (widget != nullptr)
        {
            widget->setup();
        }
    }

    rebuildBackgroundCache();
    _isInitialized = true; // setup is initialized
}

/**
 * @brief Starts all background widgets in background mode
 */
void WidgetsManager::start()
{
    // We use the background widget cache here and start only the background widgets
    for (auto& widget : _backgroundWidgets)
    {
        if (widget)
        {
            logDebugP("Initial starting background widget: %s", widget->getName().c_str());
            widget->background();
        }
    }

    _lastInteractionTime = millis(); // ← Startet den Timer
    logDebugP("Power-Save timer started at %lu", _lastInteractionTime);
}

/**
 * @brief Main loop to manage widgets and power save modes
 */
void WidgetsManager::loop()
{
    uint32_t currentTime = millis();

    // 1. Handle state changes of current widget
    handleCurrentWidget(currentTime);

    // 2. ALWAYS loop background widgets
    loopBackgroundWidgets();

    // 3. PRIORITY CHECK FIRST - overrides EVERYTHING!
    Widget* priorityWidget = findNextPriorityWidget();
    if (priorityWidget)
    {
        // Wake up display if in any power save mode
        if (_powerSaveMode != PowerSaveMode::ACTIVE)
        {
            logDebugP("Priority widget detected, forcing wake from %s", getPowerSaveModeName(_powerSaveMode));
            wakeUpDisplay();
        }

        // Force state to PRIORITY
        if (_state != WidgetManagerState::PRIORITY)
        {
            transitionTo(WidgetManagerState::PRIORITY);
        }

        // Handle priority state immediately
        handlePriorityState(currentTime);

        // Update display and return - skip everything else!
        if (_displayModule) _displayModule->loop();
        return;
    }

    // 4. Update power save mode (only if no priority widget)
    updatePowerSaveMode(currentTime);

    // 5. Screensaver mode
    if (_powerSaveMode == PowerSaveMode::SCREENSAVER)
    {
        if (_currentWidget == _screenSaverWidget &&
            _currentWidget->getState() == WidgetState::RUNNING)
        {
            _currentWidget->loop();
        }

        if (_displayModule) _displayModule->loop();
        return;
    }

    // 6. Sleep/Off mode
    if (_powerSaveMode == PowerSaveMode::SLEEP || _powerSaveMode == PowerSaveMode::OFF)
    {
        if (_displayModule) _displayModule->loop();
        return;
    }

    // 7. Normal operation: Update state machine
    updateState(currentTime);

    // 8. Execute state-specific logic
    switch (_state)
    {
        case WidgetManagerState::STARTUP:
            handleStartupState(currentTime);
            break;

        case WidgetManagerState::IDLE:
            handleIdleState(currentTime);
            break;

        case WidgetManagerState::PRIORITY:
            handlePriorityState(currentTime);
            break;

        case WidgetManagerState::BACKGROUND:
            handleBackgroundState(currentTime);
            break;

        case WidgetManagerState::DEFAULT:
            handleDefaultState(currentTime);
            break;
    }

    // 9. Always update display
    if (_displayModule) _displayModule->loop();
}

/**********************************************************************
 *********************** WIDGET QUEUE MANAGEMENT **********************
 **********************************************************************/
/**
 * @brief Adds a widget to the queue with unique name validation
 */
void WidgetsManager::addWidget(Widget* widget)
{
    if (widget == nullptr || _displayModule == nullptr) return;

    if (!_widgetQueue.empty() && getWidgetFromQueue(widget) != nullptr)
    {
        std::string widgetName = widget->getName();
        widgetName += "_";
        widgetName += std::to_string(random(0, 9));
        widgetName += (char)random(65, 90);
        widget->setName(widgetName);

        logDebugP("Widget name already in use. Added suffix to name: %s", widgetName.c_str());
    }

    widget->setDisplayModule(_displayModule);
    logDebugP("Widget added to queue: %s", widget->getName().c_str());

    // Initialize widget if manager is initialized, else will be done in setup()
    if (_isInitialized)
    {
        widget->setup();
    }
    _widgetQueue.push_back(widget);

    // Update background cache
    if (widget->getAction() & Background)
    {
        _backgroundWidgets.push_back(widget);
        logDebugP("Background widget added to cache: %s", widget->getName().c_str());
    }
}

/**
 * @brief Sets the screensaver widget
 * @param widget to use as screensaver
 */
void WidgetsManager::setScreenSaverWidget(Widget* widget)
{
    _screenSaverWidget = widget;
    if (widget)
    {
        if (_displayModule)
        {
            widget->setDisplayModule(_displayModule);
            widget->setup();
        }
        logDebugP("Screensaver widget set: %s", widget->getName().c_str());
    }
}

/**
 * @brief Retrieves a widget from the queue by name
 * @param widgetName to find in queue
 * @return nullptr if not found
 */
Widget* WidgetsManager::getWidgetFromQueue(const std::string& widgetName)
{
    for (auto& widget : _widgetQueue)
    {
        if (widget && widget->getName() == widgetName)
        {
            return widget;
        }
    }
    return nullptr;
}

/**
 * @brief Retrieves a widget from the queue by pointer
 * @param widget to find in queue, may be nullptr
 * @return nullptr if not found
 */
Widget* WidgetsManager::getWidgetFromQueue(Widget* widget)
{
    if (widget != nullptr) return getWidgetFromQueue(widget->getName());
    return nullptr;
}

/**
 * @brief Removes a widget from the queue by name
 * @param widgetName to remove from queue
 */
void WidgetsManager::removeWidgetFromQueue(const char* widgetName)
{
    if (widgetName[0] == '\0' || _widgetQueue.empty()) return;
    for (auto it = _widgetQueue.begin(); it != _widgetQueue.end(); ++it)
    {
        if (*it && (*it)->getName().compare(widgetName) == 0)
        {
            logDebugP("Removing widget from queue: %s", widgetName);
            Widget* widget = *it;

            if (widget->getAction() & Background)
            {
                auto cacheIt = std::find(_backgroundWidgets.begin(), _backgroundWidgets.end(), widget);
                if (cacheIt != _backgroundWidgets.end())
                {
                    _backgroundWidgets.erase(cacheIt);
                    logDebugP("Background widget removed from cache: %s", widgetName);
                }
            }

            _widgetQueue.erase(it);
            delete widget;
            return;
        }
    }
}

/**
 * @brief Removes a widget from the queue by pointer
 * @param widget to remove from queue
 */
void WidgetsManager::removeWidgetFromQueue(Widget* widget)
{
    if (widget != nullptr) removeWidgetFromQueue(widget->getName().c_str());
}

/**
 * @brief Rebuilds the background widget cache for O(1) access
 * Called only when widgets are added/removed
 */
void WidgetsManager::rebuildBackgroundCache()
{
    _backgroundWidgets.clear();

    for (auto& widget : _widgetQueue)
    {
        if (widget && (widget->getAction() & Background))
        {
            _backgroundWidgets.push_back(widget);
        }
    }

    logDebugP("Background cache rebuilt: %d widgets", _backgroundWidgets.size());
}

/**
 * @brief Logs the current state of the widget queue in to the debug output
 */
void WidgetsManager::logWidgetQueue()
{
    openknx.logger.begin();
    openknx.logger.log("");
    openknx.logger.color(CONSOLE_HEADLINE_COLOR);
    openknx.logger.log("=============================================================");
    openknx.logger.log("=               WidgetsManager - Widget Queue               =");
    openknx.logger.log("=============================================================");
    openknx.logger.log("");
    openknx.logger.color(0);
    openknx.logger.logWithValues("Manager State      : %s", getStateName());
    openknx.logger.logWithValues("Power Save Mode    : %s", getPowerSaveModeName(_powerSaveMode));
    openknx.logger.logWithValues("Current Widget     : %s", _currentWidget ? _currentWidget->getName().c_str() : "none");
    openknx.logger.logWithValues("Startup Complete   : %s", _startupComplete ? "yes" : "no");
    openknx.logger.log("");
    openknx.logger.log("-------------------------------------------------------------");

    if (_widgetQueue.empty())
    {
        openknx.logger.log("Queue is empty");
        openknx.logger.log("-------------------------------------------------------------");
        return;
    }

    openknx.logger.log("Idx | Name                | Flags | State |   Prio.   | Cur. ");
    openknx.logger.log("----+---------------------+-------+-------+-----------+------");
    for (size_t i = 0; i < _widgetQueue.size(); ++i)
    {
        Widget* widget = _widgetQueue[i];
        if (widget)
        {
            const char* priorityName = "N/A";
            if (widget->getAction() & StatusWidget)
            {
                switch (widget->getPriority())
                {
                    case WidgetPriority::WIDGET_PRIO_LOW: priorityName = "LOW"; break;
                    case WidgetPriority::WIDGET_PRIO_NORMAL: priorityName = "NORMAL"; break;
                    case WidgetPriority::WIDGET_PRIO_HIGH: priorityName = "HIGH"; break;
                    case WidgetPriority::WIDGET_PRIO_CRITICAL: priorityName = "CRITICAL"; break;
                }
            }

            openknx.logger.logWithValues("%3d | %-19s | %5d | %5d | %-9s | %s",
                                         (int)i,
                                         widget->getName().c_str(),
                                         widget->getAction(),
                                         (int)widget->getState(),
                                         priorityName,
                                         (_currentWidget == widget) ? "YES" : "NO");
        }
        else
        {
            openknx.logger.logWithValues("%3d | nullptr", (int)i);
        }
    }

    openknx.logger.log("-------------------------------------------------------------");
    openknx.logger.log("");
    openknx.logger.color(CONSOLE_HEADLINE_COLOR);
    openknx.logger.log("=============================================================");
    openknx.logger.color(0);
    openknx.logger.end();
}

/**
 * @brief Logs the current settings of the WidgetsManager to the debug output
 */
void WidgetsManager::logWidgetManagerSettings()
{
    openknx.logger.begin();
    openknx.logger.log("");
    openknx.logger.color(CONSOLE_HEADLINE_COLOR);
    openknx.logger.log("=======================================================");
    openknx.logger.log("=               WidgetsManager SETTINGS               =");
    openknx.logger.log("=======================================================");
    openknx.logger.color(0);
    openknx.logger.logWithValues(" Idle Timeout         : %-8lu ms (Min. %lu)", _idleTimeout, _idleTimeout / 60000);
    openknx.logger.logWithValues(" Startup Complete     : %-3s", _startupComplete ? "yes" : "no");
    openknx.logger.logWithValues(" Power Save Enabled   : %-3s", _powerSaveConfig.enabled ? "yes" : "no");
    openknx.logger.color(CONSOLE_HEADLINE_COLOR);
    openknx.logger.log("-------------------------------------------------------");
    openknx.logger.color(0);
    openknx.logger.log(" Power Save Timings (ms):");
    openknx.logger.logWithValues("   - Dim         : %8lu ms (Min. %lu)", _powerSaveConfig.dimTimeout, _powerSaveConfig.dimTimeout / 60000);
    openknx.logger.logWithValues("   - Screensaver : %8lu ms (Min. %lu)", _powerSaveConfig.screenSaverTimeout, _powerSaveConfig.screenSaverTimeout / 60000);
    openknx.logger.logWithValues("   - Sleep       : %8lu ms (Min. %lu)", _powerSaveConfig.sleepTimeout, _powerSaveConfig.sleepTimeout / 60000);
    openknx.logger.logWithValues("   - Off         : %8lu ms (Min. %lu)", _powerSaveConfig.offTimeout, _powerSaveConfig.offTimeout / 60000);
    openknx.logger.color(CONSOLE_HEADLINE_COLOR);
    openknx.logger.log("-------------------------------------------------------");
    openknx.logger.color(0);
    openknx.logger.log(" Brightness:");
    openknx.logger.logWithValues("   - Normal      : %3d%%", _powerSaveConfig.normalBrightness);
    openknx.logger.logWithValues("   - Dim         : %3d%%", _powerSaveConfig.dimBrightness);
    openknx.logger.color(CONSOLE_HEADLINE_COLOR);
    openknx.logger.log("-------------------------------------------------------");
    openknx.logger.color(0);
    openknx.logger.logWithValues(" Current State         : %-12s", getStateName());
    openknx.logger.logWithValues(" Power Save Mode       : %-12s", getPowerSaveModeName(_powerSaveMode));
    openknx.logger.logWithValues(" Current Widget        : %-20s", _currentWidget ? _currentWidget->getName().c_str() : "none");
    openknx.logger.logWithValues(" Screensaver Widget    : %-20s", _screenSaverWidget ? _screenSaverWidget->getName().c_str() : "none");
    openknx.logger.color(CONSOLE_HEADLINE_COLOR);
    openknx.logger.log("=======================================================");
    openknx.logger.log("");
    openknx.logger.color(0);
    openknx.logger.end();
}

/**
 * @brief Retrieves the current state name of the widget manager
 * @return const char*, fallback "UNKNOWN"
 */
const char* WidgetsManager::getStateName() const
{
    switch (_state)
    {
        case WidgetManagerState::STARTUP: return "STARTUP";
        case WidgetManagerState::IDLE: return "IDLE";
        case WidgetManagerState::PRIORITY: return "PRIORITY";
        case WidgetManagerState::BACKGROUND: return "BACKGROUND";
        case WidgetManagerState::DEFAULT: return "DEFAULT";
        default: return "UNKNOWN";
    }
}

/**********************************************************************
 *********************** STATE MACHINE ********************************
 **********************************************************************/
/**
 * @brief Updates the state of the widget manager based on the current time
 * @param currentTime The current time in milliseconds
 */
void WidgetsManager::updateState(uint32_t currentTime)
{
    // Priority 0: STARTUP
    if (!_startupComplete && findNextStartupWidget())
    {
        transitionTo(WidgetManagerState::STARTUP);
        return;
    }
    else if (!_startupComplete)
    {
        _startupComplete = true;
        logDebugP("Startup sequence complete");
    }

    // Priority 1: StatusWidget (PRIORITY)
    if (findNextPriorityWidget())
    {
        transitionTo(WidgetManagerState::PRIORITY);
        return;
    }

    // Priority 2: Background widgets
    if (findActiveBackgroundWidget())
    {
        transitionTo(WidgetManagerState::BACKGROUND);
        return;
    }

    // Priority 3: DefaultWidgets
    if (findNextDefaultWidget())
    {
        bool showDefault = isIdleTimeoutReached(currentTime) ||
                           !_currentWidget ||
                           currentTime >= _currentTime ||
                           hasOnlyDefaultWidgets();

        if (showDefault)
        {
            transitionTo(WidgetManagerState::DEFAULT);
            return;
        }
    }

    transitionTo(WidgetManagerState::IDLE);
}

/**
 * @brief Handles the transition between states and invokes callbacks
 * @param newState The new state to transition to
 */
void WidgetsManager::transitionTo(WidgetManagerState newState)
{
    if (_state == newState) return;

    logDebugP("State transition: %s -> %s", getStateName(),
              newState == WidgetManagerState::STARTUP ? "STARTUP" : newState == WidgetManagerState::IDLE     ? "IDLE"
                                                                : newState == WidgetManagerState::PRIORITY   ? "PRIORITY"
                                                                : newState == WidgetManagerState::BACKGROUND ? "BACKGROUND"
                                                                : newState == WidgetManagerState::DEFAULT    ? "DEFAULT"
                                                                                                             : "UNKNOWN");

    if (_stateTransitionCallback)
    {
        _stateTransitionCallback(_state, newState);
    }

    _previousState = _state;
    _state = newState;
}

/**********************************************************************
 *********************** STATE HANDLERS *******************************
 **********************************************************************/
/**
 * @brief Handles the startup state of the widget manager
 * @param currentTime The current time in milliseconds
 */
void WidgetsManager::handleStartupState(uint32_t currentTime)
{
    if (_currentWidget && currentTime < _currentTime)
    {
        if (_currentWidget->getState() == WidgetState::RUNNING &&
            !(_currentWidget->getAction() & Background))
        {
            _currentWidget->loop();
        }
        return;
    }

    Widget* startupWidget = findNextStartupWidget();
    if (!startupWidget) return;

    if (_currentWidget != startupWidget)
    {
        switchToWidget(startupWidget, currentTime, "startup widget");
    }

    if (_currentWidget &&
        _currentWidget->getState() == WidgetState::RUNNING &&
        !(_currentWidget->getAction() & Background))
    {
        _currentWidget->loop();
    }
}

/**
 * @brief Handles the idle state of the widget manager
 * @param currentTime The current time in milliseconds
 */
void WidgetsManager::handleIdleState(uint32_t currentTime)
{
    // Background widgets already looped in loopBackgroundWidgets()
    // Nothing else to do in IDLE state
    return;
}

/**
 * @brief Handles the priority state of the widget manager
 * @param currentTime The current time in milliseconds
 */
void WidgetsManager::handlePriorityState(uint32_t currentTime)
{
    Widget* priorityWidget = findNextPriorityWidget();
    if (!priorityWidget) return;

    if (_currentWidget != priorityWidget)
    {
        for (auto& widget : _backgroundWidgets)
        {
            if (widget && widget != priorityWidget &&
                widget->getState() == WidgetState::RUNNING)
            {
                logDebugP("Pausing background widget for priority: %s", widget->getName().c_str());
                widget->pause();
            }
        }

        // Handle current widget
        if (_currentWidget)
        {
            if (_currentWidget->getAction() & DefaultWidget)
            {
                logDebugP("Stopping DefaultWidget for priority: %s", _currentWidget->getName().c_str());
                _currentWidget->stop();
            }
            else if (!(_currentWidget->getAction() & Background))
            {
                // Pause only non-background widgets
                logDebugP("Pausing widget for priority: %s", _currentWidget->getName().c_str());
                if (_currentWidget->getState() == WidgetState::RUNNING)
                {
                    _currentWidget->pause();
                }
            }
        }

        switchToWidget(priorityWidget, currentTime, "priority widget");
    }

    if (_currentWidget &&
        _currentWidget->getState() == WidgetState::RUNNING &&
        !(_currentWidget->getAction() & Background))
    {
        _currentWidget->loop();
    }
}

/**
 * @brief Handles the background state of the widget manager
 * @param currentTime The current time in milliseconds
 */
void WidgetsManager::handleBackgroundState(uint32_t currentTime)
{
    Widget* backgroundWidget = findActiveBackgroundWidget();
    if (!backgroundWidget) return;

    // Resume background widget if it was paused (by PRIORITY)
    if (backgroundWidget->getState() == WidgetState::PAUSED)
    {
        logDebugP("Resuming paused background widget: %s", backgroundWidget->getName().c_str());
        backgroundWidget->resume();
    }

    if (_currentWidget != backgroundWidget)
    {
        switchToWidget(backgroundWidget, currentTime, "background widget");
    }
}

/**
 * @brief Handles the default state of the widget manager
 * @param currentTime The current time in milliseconds
 */
void WidgetsManager::handleDefaultState(uint32_t currentTime)
{
    Widget* nextDefaultWidget = findNextDefaultWidget();
    if (!nextDefaultWidget)
    {
        logDebugP("No DefaultWidget found!");
        return;
    }

    // Check if we need to switch widgets
    bool shouldSwitch = false;

    // Case 1: No current widget
    if (!_currentWidget)
    {
        shouldSwitch = true;
        logDebugP("No current widget, switching to DefaultWidget");
    }
    // Case 2: Current widget is not a DefaultWidget (e.g., Menu in background)
    else if (!(_currentWidget->getAction() & DefaultWidget))
    {
        shouldSwitch = true;
        logDebugP("Current widget (%s) is not a DefaultWidget, switching", _currentWidget->getName().c_str());
    }
    // Case 2b: Current widget is a DefaultWidget but rotation is disabled
    else if (_currentTime == UINT32_MAX && shouldRotateWidgets())
    {
        // Rotation is now enabled, so we need to switch
        shouldSwitch = true;
        logDebugP("Rotation activated (new widget added), starting rotation");
    }
    // Case 3: Display time expired (Important!)
    else if (currentTime >= _currentTime)
    {
        shouldSwitch = true;
        logDebugP("Display time expired, switching to next DefaultWidget");
    }

    if (shouldSwitch)
    {
        switchToWidget(nextDefaultWidget, currentTime, "DefaultWidget");

        if (shouldRotateWidgets()) // default rotation behavior
        {
            _currentTime = currentTime + nextDefaultWidget->getDisplayTime();
            rotateWidgetToEnd(nextDefaultWidget);
        }
        else
        {
            // If rotation is disabled, just set a long display time
            _currentTime = UINT32_MAX; // effectively infinite
            logDebugP("Widget rotation disabled, setting long display time for DefaultWidget");
        }
    }

    // Loop current DefaultWidget
    if (_currentWidget &&
        (_currentWidget->getAction() & DefaultWidget) &&
        (_currentWidget->getState() == WidgetState::RUNNING))
    {
        _currentWidget->loop();
    }
}

/**********************************************************************
 ********************** CURRENT WIDGET MANAGEMENT *********************
 **********************************************************************/
/**
 * @brief Handles the current widget based on its flags and state
 * @param currentTime The current time in milliseconds
 */
void WidgetsManager::handleCurrentWidget(uint32_t currentTime)
{
    if (!_currentWidget) return;

    const WidgetFlags flags = _currentWidget->getAction();
    const WidgetState state = _currentWidget->getState();

    // AutoRemove widget expired
    if ((flags & AutoRemove) && currentTime >= _currentTime)
    {
        logDebugP("AutoRemove widget expired: %s", _currentWidget->getName().c_str());
        removeWidgetFromQueue(_currentWidget);
        _currentWidget = nullptr;
        return;
    }

    // StatusWidget with DisplayEnabled
    if ((flags & StatusWidget) && (flags & DisplayEnabled))
    {
        if (state == WidgetState::PAUSED)
        {
            _currentWidget->resume();
            logDebugP("Resuming paused StatusWidget: %s", _currentWidget->getName().c_str());
        }
        if (state == WidgetState::STOPPED)
        {
            _currentWidget->start();
            logDebugP("Starting stopped StatusWidget: %s", _currentWidget->getName().c_str());
        }
        _lastInteractionTime = currentTime;
        return;
    }

    // StatusWidget loses DisplayEnabled
    if ((flags & StatusWidget) && state == WidgetState::RUNNING && !(flags & DisplayEnabled))
    {
        logDebugP("StatusWidget no longer DisplayEnabled: %s", _currentWidget->getName().c_str());
        _currentWidget->stop();
        _currentWidget = nullptr;

        for (auto& widget : _backgroundWidgets)
        {
            if (widget && widget->getState() == WidgetState::PAUSED)
            {
                logDebugP("Resuming paused background widget after priority: %s", widget->getName().c_str());
                widget->resume();
            }
        }
        return;
    }

    // ManagedExternally widget with DisplayEnabled
    if ((flags & ManagedExternally) && (flags & DisplayEnabled))
    {
        if (state == WidgetState::PAUSED)
        {
            logDebugP("Resuming paused widget: %s", _currentWidget->getName().c_str());
            _currentWidget->resume();
        }
        if (state == WidgetState::STOPPED)
        {
            logDebugP("Starting stopped widget: %s", _currentWidget->getName().c_str());
            _currentWidget->start();
        }
        _lastInteractionTime = currentTime;
        return; // Return here to avoid further checks
    }

    // ManagedExternally widget loses DisplayEnabled (non-background)
    if ((flags & ManagedExternally) && !(flags & DisplayEnabled) && !(flags & Background))
    {
        logDebugP("Widget no longer DisplayEnabled: %s", _currentWidget->getName().c_str());
        _currentWidget = nullptr;
        return;
    }

    // Background widget with DisplayEnabled but without ManagedExternally
    if ((flags & Background) && (flags & DisplayEnabled) && !(flags & ManagedExternally))
    {
        if (state == WidgetState::PAUSED)
        {
            logDebugP("Resuming paused background widget: %s", _currentWidget->getName().c_str());
            _currentWidget->resume();
        }
        if (state == WidgetState::STOPPED)
        {
            logDebugP("Starting stopped background widget: %s", _currentWidget->getName().c_str());
            _currentWidget->start();
        }
        _lastInteractionTime = currentTime;
        return;
    }

    // Background widget loses DisplayEnabled
    if ((flags & Background) && !(flags & DisplayEnabled))
    {
        logDebugP("Background widget no longer DisplayEnabled: %s", _currentWidget->getName().c_str());

        // Clear current widget so DEFAULT state can activate a DefaultWidget
        _currentWidget = nullptr;

        // Reset last interaction time to avoid immediate power-save
        _lastInteractionTime = currentTime;
        logDebugP("Power-Save timer restarted after menu timeout");

        return;
    }
}

/**
 * @brief Loops all background widgets regardless of the current state
 */
void WidgetsManager::loopBackgroundWidgets()
{
    for (auto& widget : _backgroundWidgets)
    {
        // Use the background widget cache here!
        if (widget && (widget->getState() == WidgetState::BACKGROUND ||
                       widget->getState() == WidgetState::RUNNING))
        {
            widget->loop();
        }
    }
}

/**********************************************************************
 ********************** POWER SAVE MANAGEMENT *************************
 **********************************************************************/
/**
 * @brief Retrieves the current power save mode name
 * @return const char*, fallback "UNKNOWN"
 */
const char* WidgetsManager::getPowerSaveModeName(PowerSaveMode mode) const
{
    switch (mode)
    {
        case PowerSaveMode::ACTIVE: return "ACTIVE";
        case PowerSaveMode::DIMMED: return "DIMMED";
        case PowerSaveMode::SCREENSAVER: return "SCREENSAVER";
        case PowerSaveMode::SLEEP: return "SLEEP";
        case PowerSaveMode::OFF: return "OFF";
        default: return "UNKNOWN";
    }
}

/**
 * @brief Updates the power save mode based on inactivity and user interaction
 * @param currentTime The current time in milliseconds
 */
void WidgetsManager::updatePowerSaveMode(uint32_t currentTime)
{
    if (!_powerSaveConfig.enabled)
    {
        logDebugP("Power-Save disabled");
        return;
    }

    uint32_t inactiveTime = currentTime - _lastInteractionTime;

    static uint32_t lastPowerSaveCheck = 0;
    if (currentTime - lastPowerSaveCheck < 1000) // Check every 1000ms
    {
        return;
    }

    lastPowerSaveCheck = currentTime;

    // Debug: Log inactivity time every 10 seconds
    static uint32_t lastDebugLog = 0;
    if (currentTime - lastDebugLog > 10000)
    {
        logDebugP("Power-Save check: inactiveTime=%lums, mode=%s",
                  inactiveTime, getPowerSaveModeName(_powerSaveMode));
        lastDebugLog = currentTime;
    }

    // User interaction detected (Priority or Background ACTIVE)
    if (findActiveBackgroundWidget() ||
        findNextPriorityWidget())
    {
        if (_powerSaveMode != PowerSaveMode::ACTIVE)
        {
            wakeUpDisplay();
        }
        _lastInteractionTime = currentTime;
        // logDebugP("User interaction detected, timer reset");
        return;
    }

    // Determine power save mode based on inactivity
    if (_powerSaveConfig.offTimeout > 0 && inactiveTime >= _powerSaveConfig.offTimeout)
    {
        transitionToPowerSaveMode(PowerSaveMode::OFF);
    }
    else if (_powerSaveConfig.sleepTimeout > 0 && inactiveTime >= _powerSaveConfig.sleepTimeout)
    {
        transitionToPowerSaveMode(PowerSaveMode::SLEEP);
    }
    else if (_powerSaveConfig.screenSaverTimeout > 0 && inactiveTime >= _powerSaveConfig.screenSaverTimeout)
    {
        transitionToPowerSaveMode(PowerSaveMode::SCREENSAVER);
    }
    else if (_powerSaveConfig.dimTimeout > 0 && inactiveTime >= _powerSaveConfig.dimTimeout)
    {
        transitionToPowerSaveMode(PowerSaveMode::DIMMED);
    }
    else
    {
        transitionToPowerSaveMode(PowerSaveMode::ACTIVE);
    }
}

/**
 * @brief Handles the transition between power save modes and invokes callbacks
 * @param newMode The new power save mode to transition to
 */
void WidgetsManager::transitionToPowerSaveMode(PowerSaveMode newMode)
{
    if (_powerSaveMode == newMode) return;

    PowerSaveMode oldMode = _powerSaveMode;
    _powerSaveMode = newMode;

    logDebugP("Power save mode transition: %s -> %s", getPowerSaveModeName(oldMode), getPowerSaveModeName(_powerSaveMode));

    if (_powerSaveCallback)
    {
        _powerSaveCallback(oldMode, newMode);
    }

    if (!_displayModule) return;

    switch (newMode)
    {
        case PowerSaveMode::ACTIVE:
        {
            logDebugP("Display: ACTIVE mode (%d%%)", _powerSaveConfig.normalBrightness);
            _displayModule->setBrightness(_powerSaveConfig.normalBrightness);
            _displayModule->displayOn();

            // Resume current widget if paused
            if (_currentWidget && _currentWidget->getState() == WidgetState::PAUSED)
            {
                logDebugP("Resuming paused widget: %s", _currentWidget->getName().c_str());
                _currentWidget->resume();
            }

            for (auto& widget : _backgroundWidgets)
            {
                if (widget && widget->getState() == WidgetState::PAUSED)
                {
                    logDebugP("Resuming paused background widget: %s", widget->getName().c_str());
                    widget->resume();
                }
            }
        }
        break;

        case PowerSaveMode::DIMMED:
        {
            logDebugP("Display: DIMMED mode (%d%%)", _powerSaveConfig.dimBrightness);
            _displayModule->setBrightness(_powerSaveConfig.dimBrightness);
        }
        break;

        case PowerSaveMode::SCREENSAVER:
        {
            logDebugP("Display: SCREENSAVER mode");
            _displayModule->setBrightness(50);

            if (!_screenSaverWidget) // No screensaver widget set, fallback to SLEEP
            {
                logWarningP("No screensaver widget set! Entering SLEEP mode instead.");
                transitionToPowerSaveMode(PowerSaveMode::SLEEP);
                return;
            }
            if (_currentWidget && _currentWidget->getState() == WidgetState::RUNNING)
            {
                _currentWidget->stop();
            }
            _currentWidget = _screenSaverWidget;
            _currentWidget->start();
            _currentTime = UINT32_MAX;
            logDebugP("Starting screensaver widget: %s", _currentWidget->getName().c_str());
        }
        break;

        case PowerSaveMode::SLEEP:
        {
            logDebugP("Display: SLEEP mode");
            if (_currentWidget && _currentWidget->getState() == WidgetState::RUNNING)
            {
                _currentWidget->pause();
            }
            _displayModule->displayOff();
        }
        break;

        case PowerSaveMode::OFF:
        {
            logDebugP("Display: OFF mode");
            if (_currentWidget && _currentWidget->getState() == WidgetState::RUNNING)
            {
                _currentWidget->stop();
            }
            _displayModule->displayOff();
        }
        break;
    }
}

/**
 * @brief Wakes up the display from any power save mode
 */
void WidgetsManager::wakeUpDisplay()
{
    logDebugP("Waking up display");

    if (_powerSaveMode == PowerSaveMode::SCREENSAVER &&
        _currentWidget == _screenSaverWidget)
    {
        _currentWidget->stop();
        _currentWidget = nullptr;
    }

    transitionToPowerSaveMode(PowerSaveMode::ACTIVE);
}

/**
 * @brief Call this on user interaction to reset power save timers
 */
void WidgetsManager::userInteraction()
{
    _lastInteractionTime = millis();

    // Wake up display if in power save mode
    if (_powerSaveMode != PowerSaveMode::ACTIVE)
    {
        wakeUpDisplay();
    }
}

/**********************************************************************
 ********************** WIDGET FINDERS ********************************
 **********************************************************************/
/**
 * @brief Finds the next priority widget in the queue
 * @return Pointer to the next priority widget or nullptr if none found
 */
Widget* WidgetsManager::findNextPriorityWidget()
{
    Widget* highestPriorityWidget = nullptr;
    uint8_t highestPriority = 0;

    for (auto& widget : _widgetQueue)
    {
        if (!widget) continue;

        WidgetFlags flags = widget->getAction();
        if ((flags & StatusWidget) && (flags & DisplayEnabled))
        {
            uint8_t priority = static_cast<uint8_t>(widget->getPriority());

            // Select widget with highest priority
            // If same priority, keep first widget in queue (FIFO)
            if (priority > highestPriority)
            {
                highestPriority = priority;
                highestPriorityWidget = widget;
            }
        }
    }
    return highestPriorityWidget;
}

/**
 * @brief Finds the next startup widget in the queue
 * @return Pointer to the next startup widget or nullptr if none found
 */
Widget* WidgetsManager::findNextStartupWidget()
{
    for (auto& widget : _widgetQueue)
    {
        if (!widget) continue;

        const WidgetFlags flags = widget->getAction();

        if ((flags & AutoRemove) &&
            !(flags & DefaultWidget) &&
            !(flags & Background) &&
            !(flags & ManagedExternally) &&
            !(flags & StatusWidget))
        {
            return widget;
        }
    }
    return nullptr;
}

/**
 * @brief Finds an active background widget in the queue
 * @return Pointer to the active background widget or nullptr if none found
 */
Widget* WidgetsManager::findActiveBackgroundWidget()
{
    for (auto& widget : _backgroundWidgets)
    {
        if (widget && (widget->getAction() & DisplayEnabled))
        {
            return widget;
        }
    }
    return nullptr;
}

/**
 * @brief Finds the next default widget in the queue
 * @return Pointer to the next default widget or nullptr if none found
 */
Widget* WidgetsManager::findNextDefaultWidget()
{
    for (auto& widget : _widgetQueue)
    {
        if (widget && (widget->getAction() & DefaultWidget))
        {
            return widget;
        }
    }
    return nullptr;
}

/**********************************************************************
 *************** HELPER FUNCTIONS FOR WIDGET SWITCHING ****************
 **********************************************************************/
/**
 * @brief Switches to the specified widget, handling stopping/pausing of the current widget
 * @param widget The widget to switch to
 * @param currentTime The current time in milliseconds
 * @param reason A string describing the reason for the switch (for logging)
 */
void WidgetsManager::switchToWidget(Widget* widget, uint32_t currentTime, const char* reason)
{
    if (_currentWidget)
    {
        if (!(_currentWidget->getAction() & Background))
        {
            logDebugP("Stopping current widget for %s: %s", reason, _currentWidget->getName().c_str());
            _currentWidget->stop();
        }
        else
        {
            logDebugP("Background widget loses focus for %s: %s", reason, _currentWidget->getName().c_str());
        }
    }

    if (widget->getAction() & StatusWidget)
    {
        const char* priorityName = "UNKNOWN";
        switch (widget->getPriority())
        {
            case WidgetPriority::WIDGET_PRIO_LOW: priorityName = "LOW"; break;
            case WidgetPriority::WIDGET_PRIO_NORMAL: priorityName = "NORMAL"; break;
            case WidgetPriority::WIDGET_PRIO_HIGH: priorityName = "HIGH"; break;
            case WidgetPriority::WIDGET_PRIO_CRITICAL: priorityName = "CRITICAL"; break;
        }
        logDebugP("Activating %s (Priority: %s): %s", reason, priorityName, widget->getName().c_str());
    }
    else
    {
        logDebugP("Activating %s: %s", reason, widget->getName().c_str());
    }

    _currentWidget = widget;

    if (_currentWidget->getState() != WidgetState::RUNNING)
    {
        _currentWidget->start();
    }

    if (widget->getAction() & Background)
    {
        _currentTime = UINT32_MAX;
    }
    else
    {
        _currentTime = currentTime + _currentWidget->getDisplayTime();
    }

    // DefaultWidgets should not reset the idle timer
    // _lastInteractionTime = currentTime;
}

/**
 * @brief Moves the specified widget to the end of the widget queue
 * @param widget The widget to rotate
 */
void WidgetsManager::rotateWidgetToEnd(Widget* widget)
{
    auto it = std::find(_widgetQueue.begin(), _widgetQueue.end(), widget);
    if (it != _widgetQueue.end())
    {
        _widgetQueue.erase(it);
        _widgetQueue.push_back(widget);
    }
}

/**
 * @brief Checks if widget rotation is needed
 * @return true if Normal > 0 OR Default > 1 (rotation needed)
 */
bool WidgetsManager::shouldRotateWidgets() const
{
    int defaultCount = 0;
    bool hasAutoRemove = false;

    for (auto& widget : _widgetQueue)
    {
        if (!widget) continue;

        const WidgetFlags flags = widget->getAction();

        // Count DefaultWidgets
        if (flags & DefaultWidget)
        {
            defaultCount++;
        }

        // Check for AutoRemove widgets
        if (flags & AutoRemove)
        {
            hasAutoRemove = true;
        }
    }

    // Rotation needed if:
    // - AutoRemove-Widgets exist (must rotate to expire)
    // - Multiple DefaultWidgets (rotation between Defaults)
    return (defaultCount > 1) || hasAutoRemove;
}

/**********************************************************************
 ********************** UTILITY FUNCTIONS *****************************
 **********************************************************************/
/**
 * @brief Checks if the idle timeout has been reached
 * @param currentTime The current time in milliseconds
 * @return true if the idle timeout has been reached, false otherwise
 */
bool WidgetsManager::isIdleTimeoutReached(uint32_t currentTime) const
{
    return (currentTime - _lastInteractionTime >= _idleTimeout);
}

/**
 * @brief Checks if the widget queue contains only DefaultWidgets (and no other types)
 * @return true if only DefaultWidgets are present, false otherwise
 */
bool WidgetsManager::hasOnlyDefaultWidgets() const
{
    bool hasDefault = false;

    for (auto& widget : _widgetQueue)
    {
        if (!widget) continue;

        const WidgetFlags flags = widget->getAction();

        if (flags & DefaultWidget)
        {
            hasDefault = true;
        }
        else if (!(flags & Background) &&
                 !(flags & StatusWidget) &&
                 !(flags & AutoRemove) &&
                 !(flags & ManagedExternally) &&
                 widget->getState() != WidgetState::STOPPED)
        {
            // This could be happen if a valid flaged widget is in the queue but stopped
            return false; // Exit directly if a non-default, non-background widget is found
        }
    }

    return hasDefault;
}

/**
 * @brief Gets the widget that should receive button events
 * Priority: PRIORITY > BACKGROUND (active) > BACKGROUND (inactive) > NORMAL
 * @return Pointer to the active button widget or nullptr
 */
Widget* WidgetsManager::getActiveButtonWidget()
{
    // Priority 1: PRIORITY-Widgets (ProgMode, StatusWidgets)
    if (_state == WidgetManagerState::PRIORITY &&
        _currentWidget &&
        _currentWidget->wantsButtonInput())
    {
        return _currentWidget;
    }

    // Priority 2: BACKGROUND-Widgets (Menu aktiv mit DisplayEnabled)
    if (_state == WidgetManagerState::BACKGROUND &&
        _currentWidget &&
        _currentWidget->wantsButtonInput())
    {
        return _currentWidget;
    }

    // Priority 3: Background-Widgets im Hintergrund (Menu inaktiv, aber RUNNING)
    Widget* backgroundWidget = findActiveBackgroundWidget();
    if (!backgroundWidget)
    {
        // Kein aktives Background-Widget, suche nach RUNNING Background-Widgets
        for (auto& widget : _backgroundWidgets)
        {
            if (widget &&
                widget->wantsButtonInput() &&
                (widget->getState() == WidgetState::RUNNING ||
                 widget->getState() == WidgetState::BACKGROUND))
            {
                return widget;
            }
        }
    }
    else if (backgroundWidget->wantsButtonInput())
    {
        return backgroundWidget;
    }

    return nullptr;
}
#endif