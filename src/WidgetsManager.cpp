#ifdef DEVICE_DISPLAY_MODULE
    #include "WidgetsManager.h"
    #include "OpenKNX.h"
    #include "Settings/DisplaySettings.h"

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
        if (_powerSaveMode != PowerSaveMode::Active)
        {
            logDebugP("Priority widget detected, forcing wake from %s", getPowerSaveModeName(_powerSaveMode));
            wakeUpDisplay();
        }

        // Force state to PRIORITY
        if (_state != WidgetManagerState::Priority)
        {
            transitionTo(WidgetManagerState::Priority);
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
    if (_powerSaveMode == PowerSaveMode::Screensaver)
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
    if (_powerSaveMode == PowerSaveMode::Sleep || _powerSaveMode == PowerSaveMode::Off)
    {
        if (_displayModule) _displayModule->loop();
        return;
    }

    // 7. Normal operation: Update state machine
    updateState(currentTime);

    // 8. Execute state-specific logic
    switch (_state)
    {
        case WidgetManagerState::Startup:
            handleStartupState(currentTime);
            break;

        case WidgetManagerState::Idle:
            handleIdleState(currentTime);
            break;

        case WidgetManagerState::Priority:
            handlePriorityState(currentTime);
            break;

        case WidgetManagerState::Background:
            handleBackgroundState(currentTime);
            break;

        case WidgetManagerState::Default:
            handleDefaultState(currentTime);
            break;
    }

    // Shared PAUSE overlay for all widgets: drawn on top of the current widget's buffer
    // before the incremental flush below pushes the changed columns to the OLED.
    drawPauseOverlay();

    // 9. Always update display
    if (_displayModule) _displayModule->loop();
}

/**
 * @brief Draw the shared "rotation paused" glyph (top-right) over the current
 *        DefaultWidget view. No-op unless paused and showing a rotation widget
 *        (not the menu, prog-exclusive or screensaver views). Draws directly into the
 *        Adafruit buffer; the caller flushes right after.
 */
void WidgetsManager::drawPauseOverlay()
{
    if (!_rotationPaused || !_displayModule || _displayModule->display == nullptr) return;
    if (!_currentWidget || !(_currentWidget->getAction() & DefaultWidget)) return;

    Adafruit_SSD1306* d = _displayModule->display;
    const int16_t w = static_cast<int16_t>(_displayModule->GetDisplayWidth());

    // Cleared box so the two bars stay legible over the widget content underneath.
    const int16_t bx = w - 10;
    d->fillRect(bx, 0, 10, 9, BLACK);
    d->fillRect(bx + 2, 1, 2, 7, WHITE);
    d->fillRect(bx + 6, 1, 2, 7, WHITE);

    // The progressive column flush (_displayModule->loop()) does not reliably push this tiny
    // top-right region, so flush here. displayBuff() is incremental, so once the glyph is on
    // screen subsequent frames diff to nothing (no flicker).
    _displayModule->displayBuff();
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
 * @brief Sets (or swaps / clears) the screensaver widget
 * @details The WidgetsManager does NOT own the screensaver widget: ownership stays with the
 *          caller (the DeviceDisplay module, which picks Matrix/Clock/off). This method
 *          therefore never deletes the old or new widget - it only swaps the pointer. It is
 *          safe to call at any time, including while the screensaver is currently on screen:
 *            - Same pointer  -> no-op.
 *            - New widget     -> if the old one is live, stop it and switch the live view to
 *                                the new widget so the change is visible immediately.
 *            - nullptr ("off")-> if the old one is live, stop it and drop straight to SLEEP;
 *                                a later Screensaver transition with no widget also falls
 *                                through to SLEEP (see transitionToPowerSaveMode).
 * @param widget to use as screensaver, or nullptr to disable the screensaver stage
 */
void WidgetsManager::setScreenSaverWidget(Widget* widget)
{
    if (_screenSaverWidget == widget) return; // idempotent, avoids re-setup/flicker

    Widget* previous = _screenSaverWidget;
    const bool wasLive = (_powerSaveMode == PowerSaveMode::Screensaver) &&
                         (_currentWidget == previous) && (previous != nullptr);

    // Prepare the new widget (ownership stays with the caller; we only wire it up).
    if (widget && _displayModule)
    {
        widget->setDisplayModule(_displayModule);
        widget->setup();
    }

    _screenSaverWidget = widget;

    // Tear down the previously running screensaver if it was on screen. We must NOT delete
    // it - the caller owns it - only stop and detach it from the live view.
    if (wasLive)
    {
        if (previous->getState() == WidgetState::RUNNING) previous->stop();
        _currentWidget = nullptr;

        if (widget)
        {
            // Swap the live screensaver to the new one so the change shows immediately.
            _currentWidget = widget;
            widget->start();
            _currentTime = UINT32_MAX;
            logDebugP("Screensaver widget swapped live: %s", widget->getName().c_str());
        }
        else
        {
            // Screensaver cleared to "Aus" while live: do NOT force SLEEP (that conflated "no
            // animation" with "off"). Wake to the normal display; Dim/Sleep timeouts then decide.
            logDebugP("Screensaver widget cleared while live -> wake to active");
            wakeUpDisplay();
        }
        return;
    }

    if (widget)
        logDebugP("Screensaver widget set: %s", widget->getName().c_str());
    else
        logDebugP("Screensaver widget cleared (off)");
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
                    case WidgetPriority::WIDGET_PRIO_SYSTEM: priorityName = "SYSTEM"; break;
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
        case WidgetManagerState::Startup: return "STARTUP";
        case WidgetManagerState::Idle: return "IDLE";
        case WidgetManagerState::Priority: return "PRIORITY";
        case WidgetManagerState::Background: return "BACKGROUND";
        case WidgetManagerState::Default: return "DEFAULT";
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
        transitionTo(WidgetManagerState::Startup);
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
        transitionTo(WidgetManagerState::Priority);
        return;
    }

    // Priority 2: Background widgets
    if (findActiveBackgroundWidget())
    {
        transitionTo(WidgetManagerState::Background);
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
            transitionTo(WidgetManagerState::Default);
            return;
        }
    }

    transitionTo(WidgetManagerState::Idle);
}

/**
 * @brief Handles the transition between states and invokes callbacks
 * @param newState The new state to transition to
 */
void WidgetsManager::transitionTo(WidgetManagerState newState)
{
    if (_state == newState) return;

    logDebugP("State transition: %s -> %s", getStateName(),
              newState == WidgetManagerState::Startup ? "STARTUP" : newState == WidgetManagerState::Idle     ? "IDLE"
                                                                : newState == WidgetManagerState::Priority   ? "PRIORITY"
                                                                : newState == WidgetManagerState::Background ? "BACKGROUND"
                                                                : newState == WidgetManagerState::Default    ? "DEFAULT"
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
    // Background widgets already looped in loopBackgroundWidgets(); nothing else to do.
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
    bool advanceRotation = false; // true only for a time-based step to the *next* widget
    Widget* switchTarget = nextDefaultWidget;

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
    else if (_currentTime == UINT32_MAX && shouldRotateWidgets() && !_rotationPaused)
    {
        // Rotation is now enabled, so we advance to the next widget in user order.
        shouldSwitch = true;
        advanceRotation = true;
        logDebugP("Rotation activated (new widget added), starting rotation");
    }
    // Case 3: Display time expired. While paused there is no time-based switch - the
    // current DefaultWidget stays put until the user manually switches or resumes.
    else if (!_rotationPaused && currentTime >= _currentTime)
    {
        shouldSwitch = true;
        advanceRotation = true;
        logDebugP("Display time expired, switching to next DefaultWidget");
    }

    // While paused, never switch AWAY from a live DefaultWidget - not even when the manager
    // briefly cycles Default<->Background/Idle (Case 1/2 are otherwise NOT gated by
    // _rotationPaused). The current widget is held until the user resumes or manually steps.
    // If there is no live DefaultWidget yet, the switch below still runs once to establish one.
    if (shouldSwitch && _rotationPaused && _currentWidget &&
        (_currentWidget->getAction() & DefaultWidget) &&
        _currentWidget->getState() == WidgetState::RUNNING)
    {
        shouldSwitch = false;
        advanceRotation = false;
    }

    // For a rotation step, advance to the DefaultWidget that follows the current one in the
    // persistent user order (skipping disabled). For Case 1/2 (re-entering the DEFAULT state)
    // we resume on the first enabled widget in order (nextDefaultWidget).
    if (advanceRotation)
    {
        Widget* adjacent = findAdjacentDefaultWidget(+1);
        if (adjacent) switchTarget = adjacent;
    }

    if (shouldSwitch)
    {
        switchToWidget(switchTarget, currentTime, "DefaultWidget");

        if (_rotationPaused)
        {
            // In manual/paused mode hold the widget indefinitely (no auto switch).
            _currentTime = UINT32_MAX;
            logDebugP("Rotation paused, holding DefaultWidget: %s", switchTarget->getName().c_str());
        }
        else if (shouldRotateWidgets()) // default rotation behavior
        {
            _currentTime = currentTime + switchTarget->getDisplayTime();
            // rotateWidgetToEnd only reorders the queue for AutoRemove housekeeping;
            // it no longer drives (nor destroys) the persistent DefaultWidget order.
            rotateWidgetToEnd(switchTarget);
        }
        else
        {
            // If rotation is disabled, just set a long display time
            _currentTime = UINT32_MAX; // effectively infinite
            logDebugP("Widget rotation disabled, setting long display time for DefaultWidget");
        }
    }

    // Auto-page through a multi-page DefaultWidget within its display duration, before the
    // rotation (handled above via _currentTime) moves on to the next widget.
    updateAutoPaging(currentTime);

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
        case PowerSaveMode::Active: return "ACTIVE";
        case PowerSaveMode::Dimmed: return "DIMMED";
        case PowerSaveMode::Screensaver: return "SCREENSAVER";
        case PowerSaveMode::Sleep: return "SLEEP";
        case PowerSaveMode::Off: return "OFF";
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

    // Display was manually forced off (Left-hold gesture). Stay OFF regardless of any active
    // status/overlay widget until a real interaction clears the flag via wakeUpDisplay();
    // otherwise the wake-on-active-widget check below would flicker it back on immediately.
    if (_forcedOff)
    {
        if (_powerSaveMode != PowerSaveMode::Off)
            transitionToPowerSaveMode(PowerSaveMode::Off);
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
        if (_powerSaveMode != PowerSaveMode::Active)
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
        transitionToPowerSaveMode(PowerSaveMode::Off);
    }
    else if (_powerSaveConfig.sleepTimeout > 0 && inactiveTime >= _powerSaveConfig.sleepTimeout)
    {
        transitionToPowerSaveMode(PowerSaveMode::Sleep);
    }
    else if (_powerSaveConfig.screenSaverTimeout > 0 && inactiveTime >= _powerSaveConfig.screenSaverTimeout &&
             _screenSaverWidget)
    {
        // Only enter the Screensaver stage when a screensaver actually exists. With type "Aus"
        // (no widget) the stage is skipped -> the chain falls through to Dim/Sleep, so whether the
        // display turns off is decided solely by "Schlafen nach" (sleepTimeout 0 = nie -> stays on).
        transitionToPowerSaveMode(PowerSaveMode::Screensaver);
    }
    else if (_powerSaveConfig.dimTimeout > 0 && inactiveTime >= _powerSaveConfig.dimTimeout)
    {
        transitionToPowerSaveMode(PowerSaveMode::Dimmed);
    }
    else
    {
        transitionToPowerSaveMode(PowerSaveMode::Active);
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
        case PowerSaveMode::Active:
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

        case PowerSaveMode::Dimmed:
        {
            logDebugP("Display: DIMMED mode (%d%%)", _powerSaveConfig.dimBrightness);
            _displayModule->setBrightness(_powerSaveConfig.dimBrightness);
        }
        break;

        case PowerSaveMode::Screensaver:
        {
            logDebugP("Display: SCREENSAVER mode");
            _displayModule->setBrightness(50);

            if (!_screenSaverWidget) // No screensaver widget (type "Aus") -> skip stage, don't force SLEEP
            {
                logDebugP("No screensaver widget -> wake to active (sleep governed by sleepTimeout).");
                wakeUpDisplay();
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

        case PowerSaveMode::Sleep:
        {
            logDebugP("Display: SLEEP mode");
            if (_currentWidget && _currentWidget->getState() == WidgetState::RUNNING)
            {
                _currentWidget->pause();
            }
            _displayModule->displayOff();
        }
        break;

        case PowerSaveMode::Off:
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

    _forcedOff = false; // a real wake clears the manual display-off latch

    // Null-guard: setScreenSaverWidget(nullptr) can clear both _currentWidget and _screenSaverWidget
    // before waking, which would make this branch match as nullptr==nullptr and deref a null widget.
    if (_powerSaveMode == PowerSaveMode::Screensaver &&
        _currentWidget != nullptr &&
        _currentWidget == _screenSaverWidget)
    {
        _currentWidget->stop();
        _currentWidget = nullptr;
    }

    transitionToPowerSaveMode(PowerSaveMode::Active);
}

/**
 * @brief Force the display OFF immediately (Left-hold gesture), independent of the
 *        inactivity timers. Latches _forcedOff so updatePowerSaveMode() keeps it OFF until
 *        wakeUpDisplay() clears it on the next real interaction (any button press).
 */
void WidgetsManager::forceDisplayOff()
{
    _forcedOff = true;
    // Back-date the interaction time so the inactivity machine also agrees we are past the
    // off/sleep timeout, in case the latch is ever cleared without a fresh interaction.
    const uint32_t now = millis();
    const uint32_t back = _powerSaveConfig.offTimeout > 0 ? _powerSaveConfig.offTimeout
                                                          : (_powerSaveConfig.sleepTimeout > 0 ? _powerSaveConfig.sleepTimeout : 1000);
    _lastInteractionTime = now - back - 1;
    transitionToPowerSaveMode(PowerSaveMode::Off);
    logDebugP("forceDisplayOff: display forced OFF (any button wakes)");
}

/**
 * @brief Call this on user interaction to reset power save timers
 */
void WidgetsManager::userInteraction()
{
    _lastInteractionTime = millis();

    // Wake up display if in power save mode
    if (_powerSaveMode != PowerSaveMode::Active)
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
 *
 * A ManagedExternally widget (e.g. the SD file browser) is a MODAL takeover: when it and a plain
 * background widget (the menu) BOTH request the screen (DisplayEnabled), the modal must win — else
 * the first-added widget (usually the menu) would keep focus while the browser only draws on top,
 * so the browser never receives navigation. Preferring the modal makes it the active button widget
 * (and puts the manager into Background state, so directional buttons navigate instead of arming
 * Home-key gestures). When the modal drops DisplayEnabled, the plain widget below is returned again.
 */
Widget* WidgetsManager::findActiveBackgroundWidget()
{
    Widget* fallback = nullptr;
    for (auto& widget : _backgroundWidgets)
    {
        if (widget && (widget->getAction() & DisplayEnabled))
        {
            if (widget->getAction() & ManagedExternally)
                return widget; // modal takeover wins over a plain background widget
            if (!fallback)
                fallback = widget; // first plain background widget (e.g. the menu)
        }
    }
    return fallback;
}

/**
 * @brief Finds the next default widget in the queue
 * @details Disabled DefaultWidgets are skipped so they drop out of the rotation.
 *          To guarantee we never end up on a blank screen, if every DefaultWidget is
 *          disabled we fall back to the first DefaultWidget in the queue regardless of
 *          its enabled flag. So there is always at least one active DefaultWidget as long
 *          as one exists at all.
 * @return Pointer to the next (enabled) default widget, a disabled fallback, or nullptr
 *         if the queue holds no DefaultWidget at all
 */
Widget* WidgetsManager::findNextDefaultWidget()
{
    // Iterate the persistent user order, not the raw queue. rotateWidgetToEnd() shuffles
    // _widgetQueue for AutoRemove housekeeping, but the display order of DefaultWidgets
    // must stay in the user-defined _defaultOrder.
    syncDefaultOrder();

    Widget* fallbackDefault = nullptr; // First DefaultWidget seen, used only if all are disabled

    for (auto& widget : _defaultOrder)
    {
        if (widget && (widget->getAction() & DefaultWidget))
        {
            if (widget->isEnabled())
            {
                return widget; // Preferred: first enabled DefaultWidget in user order
            }
            if (!fallbackDefault)
            {
                fallbackDefault = widget; // Remember first (disabled) DefaultWidget as fallback
            }
        }
    }

    // No enabled DefaultWidget found; fall back to a disabled one to avoid a blank screen.
    return fallbackDefault;
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
            case WidgetPriority::WIDGET_PRIO_SYSTEM: priorityName = "SYSTEM"; break;
        }
        logDebugP("Activating %s (Priority: %s): %s", reason, priorityName, widget->getName().c_str());
    }
    else
    {
        logDebugP("Activating %s: %s", reason, widget->getName().c_str());
    }

    _currentWidget = widget;

    // Record the switch time for every widget switch (incl. AutoRemove/Background/Priority).
    // This is the base for getRotationProgress() and resets it to 0 on each switch.
    _currentWidgetStartTime = currentTime;

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

        // Count only ENABLED DefaultWidgets: disabled ones are skipped by the rotation,
        // so a single enabled Default surrounded by disabled ones must not rotate.
        if ((flags & DefaultWidget) && widget->isEnabled())
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
    // - Multiple enabled DefaultWidgets (rotation between Defaults)
    return (defaultCount > 1) || hasAutoRemove;
}

/**********************************************************************
 ******************** ROTATION ORDER & CONTROL ***********************
 **********************************************************************/
/**
 * @brief Keep _defaultOrder in sync with the DefaultWidgets in the queue
 * @details Appends any DefaultWidget that is in the queue but not yet tracked (in queue
 *          order, so the first-ever order matches insertion order), and drops any tracked
 *          widget that is no longer in the queue (removed/AutoRemoved). The user-defined
 *          order of the surviving widgets is preserved. Cheap once the set is stable.
 */
void WidgetsManager::syncDefaultOrder()
{
    // Drop stale entries (widgets no longer in the queue).
    _defaultOrder.erase(
        std::remove_if(_defaultOrder.begin(), _defaultOrder.end(),
                       [this](Widget* w) {
                           if (!w) return true;
                           return std::find(_widgetQueue.begin(), _widgetQueue.end(), w) == _widgetQueue.end();
                       }),
        _defaultOrder.end());

    // Append newly-seen DefaultWidgets in queue order.
    for (auto& widget : _widgetQueue)
    {
        if (widget && (widget->getAction() & DefaultWidget))
        {
            if (std::find(_defaultOrder.begin(), _defaultOrder.end(), widget) == _defaultOrder.end())
            {
                _defaultOrder.push_back(widget);
            }
        }
    }
}

/**
 * @brief Returns the DefaultWidgets in their persistent user order
 * @return Vector of DefaultWidget pointers (enabled and disabled), user order
 */
std::vector<Widget*> WidgetsManager::getDefaultWidgetsInOrder() const
{
    // const method: work on a local copy so the cached order stays untouched here.
    // We still fold in any not-yet-tracked queue DefaultWidgets so callers see a
    // complete, up-to-date list even before the next rotation tick runs syncDefaultOrder().
    std::vector<Widget*> order;
    order.reserve(_defaultOrder.size());

    for (Widget* w : _defaultOrder)
    {
        if (w && (w->getAction() & DefaultWidget) &&
            std::find(_widgetQueue.begin(), _widgetQueue.end(), w) != _widgetQueue.end())
        {
            order.push_back(w);
        }
    }
    for (Widget* widget : _widgetQueue)
    {
        if (widget && (widget->getAction() & DefaultWidget) &&
            std::find(order.begin(), order.end(), widget) == order.end())
        {
            order.push_back(widget);
        }
    }
    return order;
}

/**
 * @brief DefaultWidget adjacent to the current one in user order
 * @details Steps dir (+1 next / -1 previous) from the current widget's position in
 *          _defaultOrder, skipping disabled DefaultWidgets, and wraps around. If the
 *          current widget is not a tracked DefaultWidget, returns the first enabled
 *          DefaultWidget in order. Returns nullptr only if no enabled DefaultWidget
 *          exists at all.
 * @param dir +1 to move forward, -1 to move backward
 * @return the adjacent enabled DefaultWidget, or nullptr
 */
Widget* WidgetsManager::findAdjacentDefaultWidget(int dir)
{
    syncDefaultOrder();

    const size_t n = _defaultOrder.size();
    if (n == 0) return nullptr;

    // Locate the current widget in the order.
    size_t startIdx = 0;
    bool found = false;
    for (size_t i = 0; i < n; ++i)
    {
        if (_defaultOrder[i] == _currentWidget)
        {
            startIdx = i;
            found = true;
            break;
        }
    }

    const int step = (dir >= 0) ? 1 : -1;

    // Walk up to n neighbours, skipping disabled widgets, wrapping around.
    // If the current widget is not in the order (found == false), begin the search at
    // the first element (i == 0) so we land on the first enabled DefaultWidget.
    for (size_t k = 1; k <= n; ++k)
    {
        size_t idx;
        if (found)
        {
            idx = (startIdx + static_cast<size_t>(step) * k) % n; // step*k wraps via modulo
        }
        else
        {
            idx = (k - 1) % n;
        }
        Widget* candidate = _defaultOrder[idx];
        if (candidate && (candidate->getAction() & DefaultWidget) && candidate->isEnabled())
        {
            return candidate;
        }
    }

    // No enabled neighbour: fall back to any DefaultWidget to avoid a blank screen.
    return findNextDefaultWidget();
}

/**
 * @brief Pause the DefaultWidget auto-rotation (enter manual mode)
 */
void WidgetsManager::pauseRotation()
{
    if (_rotationPaused) return;
    _rotationPaused = true;
    // Freeze the current DefaultWidget: no scheduled end time -> no auto switch.
    if (_currentWidget && (_currentWidget->getAction() & DefaultWidget))
    {
        _currentTime = UINT32_MAX;
    }
    logDebugP("Rotation paused (manual mode)");
}

/**
 * @brief Resume the DefaultWidget auto-rotation with a fresh display duration
 */
void WidgetsManager::resumeRotation()
{
    if (!_rotationPaused) return;
    _rotationPaused = false;

    // Give the currently shown DefaultWidget a fresh full duration before the next
    // auto switch (mirrors the mock resetting homeStart on resume).
    const uint32_t now = millis();
    if (_currentWidget && (_currentWidget->getAction() & DefaultWidget) && shouldRotateWidgets())
    {
        _currentWidgetStartTime = now;
        _currentTime = now + _currentWidget->getDisplayTime();
    }
    logDebugP("Rotation resumed");
}

/**
 * @brief Toggle between paused (manual) and running (auto) rotation
 */
void WidgetsManager::toggleRotationPause()
{
    if (_rotationPaused) resumeRotation();
    else
        pauseRotation();
}

/**
 * @brief Common manual-switch path (next/prevWidgetManual)
 * @details Switches to the given DefaultWidget and resets its display timer so the
 *          freshly shown widget is fully visible. In paused mode the widget is held
 *          indefinitely; otherwise a fresh full duration is scheduled.
 */
void WidgetsManager::switchToDefaultWidgetManual(Widget* widget, uint32_t currentTime, const char* reason)
{
    switchToWidget(widget, currentTime, reason);

    if (_rotationPaused || !shouldRotateWidgets())
    {
        _currentTime = UINT32_MAX; // held until next manual switch / resume
    }
    else
    {
        _currentTime = currentTime + widget->getDisplayTime();
    }
}

/**
 * @brief Manually switch to the next DefaultWidget in user order
 */
void WidgetsManager::nextWidgetManual()
{
    Widget* target = findAdjacentDefaultWidget(+1);
    if (!target) return;
    logDebugP("Manual switch (next): %s", target->getName().c_str());
    switchToDefaultWidgetManual(target, millis(), "manual next");
}

/**
 * @brief Manually switch to the previous DefaultWidget in user order
 */
void WidgetsManager::prevWidgetManual()
{
    Widget* target = findAdjacentDefaultWidget(-1);
    if (!target) return;
    logDebugP("Manual switch (prev): %s", target->getName().c_str());
    switchToDefaultWidgetManual(target, millis(), "manual prev");
}

/**********************************************************************
 ****************** AUTO-PAGING & PAGE FLIP **************************
 **********************************************************************/
/**
 * @brief Enable/disable automatic paging of multi-page DefaultWidgets
 * @details When enabled, a multi-page current DefaultWidget advances its pages evenly
 *          over its display duration (see updateAutoPaging). Disabling it leaves the
 *          current widget on whatever page it is on; the next full duration then simply
 *          keeps page 0 (the auto advance no longer runs). Does not touch the rotation
 *          deadline (_currentTime), so the per-widget duration is unchanged either way.
 * @param enabled true to auto-page multi-page widgets, false to hold a single page
 */
void WidgetsManager::setAutoPaging(bool enabled)
{
    if (_autoPagingEnabled == enabled) return;
    _autoPagingEnabled = enabled;
    logDebugP("Auto-paging %s", enabled ? "enabled" : "disabled");
}

/**
 * @brief Advance the current DefaultWidget's page to match elapsed display time
 * @details Mirrors the mock: page = min(N-1, floor(elapsed / (dur/N))), where dur is the
 *          scheduled display duration (_currentTime - _currentWidgetStartTime) and N is the
 *          page count. Only runs for a rotating (finite _currentTime), enabled, non-paused
 *          multi-page DefaultWidget. The rotation itself still fires from the _currentTime
 *          deadline in handleDefaultState, i.e. only after the last page has been shown.
 *          millis() overflow-safe: uses unsigned differences throughout.
 * @param currentTime The current time in milliseconds
 */
void WidgetsManager::updateAutoPaging(uint32_t currentTime)
{
    if (!_autoPagingEnabled || _rotationPaused) return;
    if (!_currentWidget || !(_currentWidget->getAction() & DefaultWidget)) return;

    const size_t pageCount = _currentWidget->getPageCount();
    if (pageCount <= 1) return;

    // Cycle length: the scheduled rotation duration when rotating, else the widget's own
    // display time (a lone multi-page DefaultWidget shown with rotation disabled still pages
    // through its content, matching the mock which always pages a multi-page widget).
    uint32_t total;
    if (_currentTime != UINT32_MAX)
    {
        total = _currentTime - _currentWidgetStartTime;
    }
    else
    {
        total = _currentWidget->getDisplayTime();
    }
    if (total == 0) return; // degenerate zero-duration widget: nothing to slice

    // Elapsed since the widget was switched in, overflow-safe.
    uint32_t elapsed = currentTime - _currentWidgetStartTime;
    // For a finite rotation clamp to the last page until the switch fires; for the infinite
    // (non-rotating) case wrap through the pages so a lone widget keeps cycling.
    if (_currentTime != UINT32_MAX)
    {
        if (elapsed >= total) elapsed = total - 1;
    }
    else
    {
        elapsed %= total;
    }

    const uint32_t slice = total / static_cast<uint32_t>(pageCount);
    size_t targetPage = (slice == 0) ? (pageCount - 1)
                                     : static_cast<size_t>(elapsed / slice);
    if (targetPage >= pageCount) targetPage = pageCount - 1;

    if (targetPage != _currentWidget->getCurrentPage())
    {
        _currentWidget->setPage(targetPage);
    }
}

// Page index one step (dir +1 next / -1 previous) with wrap. count >= 1 by callers.
static inline size_t wrapPage(size_t page, int dir, size_t count)
{
    if (dir >= 0) return (page + 1) % count;
    return (page + count - 1) % count;
}

/**
 * @brief Manually flip the current widget's page by one, with wrap
 * @details Moves the current (multi-page) widget's page by one (up = -1, down = +1),
 *          wrapping around, and rescales _currentWidgetStartTime so the chosen page keeps
 *          its full per-page slice before the next automatic advance/rotation (mirrors the
 *          mock resetting homeStart to now - page*slice). No-op for single-page current
 *          widgets. Manual paging works even while auto-paging is off.
 */
void WidgetsManager::pageUp()
{
    Widget* current = _currentWidget;
    if (!current) return;
    const size_t pageCount = current->getPageCount();
    if (pageCount <= 1) return;

    const size_t newPage = wrapPage(current->getCurrentPage(), -1, pageCount);
    current->setPage(newPage);

    // Rescale the timer so the shown page gets its full slice before the next step.
    if (_currentTime != UINT32_MAX)
    {
        const uint32_t now = millis();
        const uint32_t total = _currentTime - _currentWidgetStartTime;
        const uint32_t slice = total / static_cast<uint32_t>(pageCount);
        _currentWidgetStartTime = now - static_cast<uint32_t>(newPage) * slice;
        _currentTime = _currentWidgetStartTime + total;
    }
    logDebugP("Manual page up: %s -> page %u", current->getName().c_str(), (unsigned)newPage);
}

void WidgetsManager::pageDown()
{
    Widget* current = _currentWidget;
    if (!current) return;
    const size_t pageCount = current->getPageCount();
    if (pageCount <= 1) return;

    const size_t newPage = wrapPage(current->getCurrentPage(), +1, pageCount);
    current->setPage(newPage);

    if (_currentTime != UINT32_MAX)
    {
        const uint32_t now = millis();
        const uint32_t total = _currentTime - _currentWidgetStartTime;
        const uint32_t slice = total / static_cast<uint32_t>(pageCount);
        _currentWidgetStartTime = now - static_cast<uint32_t>(newPage) * slice;
        _currentTime = _currentWidgetStartTime + total;
    }
    logDebugP("Manual page down: %s -> page %u", current->getName().c_str(), (unsigned)newPage);
}

/**********************************************************************
 ********************** REORDER / GRAB API ***************************
 **********************************************************************/
/**
 * @brief The reorderable DefaultWidgets in their persistent user order
 * @details Same order/content as getDefaultWidgetsInOrder(); named separately so the
 *          reorder UI has an intent-revealing entry point. Indices into this list are
 *          the ones grabWidget()/getGrabIndex() operate on.
 * @return Vector of DefaultWidget pointers in user order
 */
std::vector<Widget*> WidgetsManager::getReorderableWidgets() const
{
    return getDefaultWidgetsInOrder();
}

/**
 * @brief Grab the widget at display index for reordering
 * @details Syncs the persistent order, then records the grabbed index. Subsequent
 *          moveGrabbedUp/Down() swap it with its neighbour in _defaultOrder. Index refers
 *          to the position in the synced _defaultOrder (== getReorderableWidgets()).
 * @param index position of the widget to grab
 * @return true if the index was valid and the widget is now grabbed
 */
bool WidgetsManager::grabWidget(size_t index)
{
    syncDefaultOrder();
    if (index >= _defaultOrder.size())
    {
        logDebugP("grabWidget: index %u out of range (%u)", (unsigned)index, (unsigned)_defaultOrder.size());
        return false;
    }
    _grabIndex = static_cast<int>(index);
    logDebugP("Grabbed widget at index %u: %s", (unsigned)index,
              _defaultOrder[index] ? _defaultOrder[index]->getName().c_str() : "nullptr");
    return true;
}

/**
 * @brief Move the grabbed widget one position up (towards the front)
 * @details Swaps the grabbed widget with its predecessor in _defaultOrder and follows the
 *          move (grab index decremented), mirroring the mock. No wrap: a no-op at the top.
 *          The change is immediately visible to the rotation (it reads _defaultOrder).
 * @return true if a swap happened, false if nothing grabbed or already at the top
 */
bool WidgetsManager::moveGrabbedUp()
{
    if (_grabIndex < 0) return false;
    // Guard against a stale grab index after a concurrent queue change.
    if (static_cast<size_t>(_grabIndex) >= _defaultOrder.size())
    {
        _grabIndex = -1;
        return false;
    }
    if (_grabIndex == 0) return false;

    std::swap(_defaultOrder[_grabIndex], _defaultOrder[_grabIndex - 1]);
    _grabIndex--;
    logDebugP("Moved grabbed widget up to index %d", _grabIndex);
    return true;
}

/**
 * @brief Move the grabbed widget one position down (towards the back)
 * @details Swaps the grabbed widget with its successor in _defaultOrder and follows the
 *          move (grab index incremented). No wrap: a no-op at the bottom.
 * @return true if a swap happened, false if nothing grabbed or already at the bottom
 */
bool WidgetsManager::moveGrabbedDown()
{
    if (_grabIndex < 0) return false;
    if (static_cast<size_t>(_grabIndex) >= _defaultOrder.size())
    {
        _grabIndex = -1;
        return false;
    }
    if (static_cast<size_t>(_grabIndex) + 1 >= _defaultOrder.size()) return false;

    std::swap(_defaultOrder[_grabIndex], _defaultOrder[_grabIndex + 1]);
    _grabIndex++;
    logDebugP("Moved grabbed widget down to index %d", _grabIndex);
    return true;
}

/**
 * @brief Release the currently grabbed widget (end the grab)
 */
void WidgetsManager::dropWidget()
{
    if (_grabIndex < 0) return;
    logDebugP("Dropped widget at index %d", _grabIndex);
    _grabIndex = -1;
}

/**
 * @brief Page/rotation status text for the Home overlay ("2/4" / "manuell")
 * @details - Multi-page current widget: "n/N" (1-based current page / page count).
 *          - Paused + single-page current widget: "manuell".
 *          - Otherwise (single-page, actively rotating, or no current widget): "".
 *          Pure text provider; performs no state changes.
 * @return status text (may be empty)
 */
std::string WidgetsManager::getPageStatusText() const
{
    Widget* current = getCurrentWidget();
    if (!current) return std::string();

    const size_t pageCount = current->getPageCount();
    if (pageCount > 1)
    {
        // 1-based "current/total"; clamp the page index defensively.
        size_t page = current->getCurrentPage();
        if (page >= pageCount) page = pageCount - 1;
        return std::to_string(page + 1) + "/" + std::to_string(pageCount);
    }

    // Single-page widget: only the paused/manual mode carries a status label.
    if (_rotationPaused) return std::string("manuell");

    return std::string();
}

/**
 * @brief Set a widget's display duration by name (effective next switch)
 * @details Updates the widget's own display time. The change takes effect the next time
 *          the rotation switches to that widget; the currently running widget keeps its
 *          already-scheduled end time. No-op for an unknown name.
 * @param widgetName name of the target widget
 * @param displayTimeMs new display duration in milliseconds
 */
void WidgetsManager::setWidgetDisplayTime(const std::string& widgetName, uint32_t displayTimeMs)
{
    Widget* widget = getWidgetFromQueue(widgetName);
    if (!widget)
    {
        logDebugP("setWidgetDisplayTime: widget not found: %s", widgetName.c_str());
        return;
    }
    widget->setDisplayTime(displayTimeMs);
    logDebugP("Widget display time set: %s -> %lu ms", widgetName.c_str(), (unsigned long)displayTimeMs);
    // Intentionally does NOT reschedule _currentTime: the new duration applies from the
    // next switch onward, so an in-flight widget is not cut short or extended abruptly.
}

/**
 * @brief Read a widget's display duration by name
 * @param widgetName name of the target widget
 * @return display duration in milliseconds, or 0 if the widget is not found
 */
uint32_t WidgetsManager::getWidgetDisplayTime(const std::string& widgetName) const
{
    // getWidgetFromQueue is non-const; do a local const lookup instead.
    for (Widget* widget : _widgetQueue)
    {
        if (widget && widget->getName() == widgetName)
        {
            return widget->getDisplayTime();
        }
    }
    return 0;
}

/**
 * @brief Map persisted DisplaySettings onto the PowerSaveConfig
 * @details Replaces the hardcoded init() defaults with the on-device settings:
 *          - normalBrightness  <- brightnessIdx  (idx 0..9 -> 10..100 %)
 *          - dimBrightness      <- dimLevelIdx   (0 = nie -> = normal; 1..9 -> min(idx*10, normal))
 *          - dimTimeout        <- dimMin         (minutes; 0 OR dimLevel "nie" -> no DIMMED stage)
 *          - screenSaverTimeout<- screenSaverMin (direct minutes, 0 -> no screensaver)
 *          - sleepTimeout      <- sleepMin       (direct minutes, 0 -> "nie")
 *          Timeouts are user-set minutes (0 = off/never). offTimeout is left as configured elsewhere.
 * @param settings the persisted DisplaySettings to apply
 */
void WidgetsManager::applyDisplaySettings(const DisplaySettings& settings)
{
    // Normal brightness: idx 0..9 -> (idx+1)*10 = 10..100 %.
    const uint8_t bIdx = settings.brightnessIdx > 9 ? 9 : settings.brightnessIdx;
    const uint8_t normalPct = static_cast<uint8_t>((bIdx + 1) * 10);
    _powerSaveConfig.normalBrightness = normalPct;

    // Dim-Level: idx 0 = "nie" (no dim), 1..9 -> 10..90 %. Effective dim is never brighter than
    // normal (min-rule). Dim is disabled if the level is "nie" OR the time is 0 (either off-switch).
    const uint8_t dIdx = settings.dimLevelIdx > 9 ? 9 : settings.dimLevelIdx;
    const bool dimOff = (dIdx == 0) || (settings.dimMin == 0);
    if (dIdx == 0)
    {
        _powerSaveConfig.dimBrightness = normalPct; // "nie" -> no visible dim
    }
    else
    {
        const uint8_t dimPct = static_cast<uint8_t>(dIdx * 10);
        _powerSaveConfig.dimBrightness = dimPct < normalPct ? dimPct : normalPct; // min(dim, normal)
    }

    // Dim / screensaver / sleep timeouts are DIRECT minutes; 0 = off/never (that stage is skipped).
    _powerSaveConfig.dimTimeout = dimOff ? 0u : static_cast<uint32_t>(settings.dimMin) * 60000u;
    _powerSaveConfig.screenSaverTimeout = static_cast<uint32_t>(settings.screenSaverMin) * 60000u;
    _powerSaveConfig.sleepTimeout = static_cast<uint32_t>(settings.sleepMin) * 60000u;

    logDebugP("applyDisplaySettings: normal=%d%%, dim=%d%%/%lu, screensaver=%lu, sleep=%lu",
              _powerSaveConfig.normalBrightness, _powerSaveConfig.dimBrightness,
              (unsigned long)_powerSaveConfig.dimTimeout,
              (unsigned long)_powerSaveConfig.screenSaverTimeout,
              (unsigned long)_powerSaveConfig.sleepTimeout);
}

/**********************************************************************
 ********************** UTILITY FUNCTIONS *****************************
 **********************************************************************/
/**
 * @brief Linear rotation progress of the current DefaultWidget in [0.0f, 1.0f]
 * @details Rises from 0.0f at the moment the widget was switched in to 1.0f when its
 *          display time elapses. Returns ROTATION_PROGRESS_NONE when there is no
 *          time-based rotation to show:
 *            - no current widget
 *            - rotation disabled / infinite display time (_currentTime == UINT32_MAX),
 *              which also covers Background, Priority and Screensaver widgets
 *            - the current widget is not a DefaultWidget (e.g. Menu, StatusWidget)
 *          Callers (e.g. getRotationStatus / Home overlay) treat a negative
 *          return as "no progress bar" and, when paused, render an empty bar instead.
 *          millis() overflow-safe: all arithmetic uses unsigned (uint32_t) differences,
 *          which wrap consistently across the ~49-day millis() rollover.
 * @return progress in [0.0f, 1.0f], or ROTATION_PROGRESS_NONE for non-rotation states
 */
float WidgetsManager::getRotationProgress() const
{
    // No widget, or an explicitly non-rotating (infinite) display time.
    if (!_currentWidget || _currentTime == UINT32_MAX)
    {
        return ROTATION_PROGRESS_NONE;
    }

    // Only rotating DefaultWidgets have a meaningful progress bar.
    if (!(_currentWidget->getAction() & DefaultWidget))
    {
        return ROTATION_PROGRESS_NONE;
    }

    // Total scheduled duration = end (_currentTime) - start (_currentWidgetStartTime).
    // Unsigned subtraction is correct even across a millis() wrap.
    const uint32_t total = _currentTime - _currentWidgetStartTime;
    if (total == 0)
    {
        // Zero display time: already at the switch boundary.
        return 1.0f;
    }

    // Elapsed since switch, overflow-safe.
    const uint32_t elapsed = millis() - _currentWidgetStartTime;
    if (elapsed >= total)
    {
        return 1.0f;
    }

    return static_cast<float>(elapsed) / static_cast<float>(total);
}

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
    if (_state == WidgetManagerState::Priority &&
        _currentWidget &&
        _currentWidget->wantsButtonInput())
    {
        return _currentWidget;
    }

    // Priority 2: BACKGROUND-Widgets (Menu aktiv mit DisplayEnabled)
    if (_state == WidgetManagerState::Background &&
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
