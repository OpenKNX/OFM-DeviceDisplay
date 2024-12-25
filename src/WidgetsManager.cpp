#include "WidgetsManager.h"
#include "OpenKNX.h"

void WidgetsManager::addWidget(Widget *widget)
{
    if (widget == nullptr || _displayModule == nullptr) return;

    if (!_widgetQueue.empty() && getWidgetFromQueue(widget) != nullptr)
    {
        const std::string widgetName = widget->getName() + "_" + std::to_string(random(0, 9)) + (char)random(65, 90);
        widget->setName(widgetName);
        logDebugP("Widget name already in use. Added suffix to name: %s", widgetName.c_str());
    }

    widget->setDisplayModule(_displayModule);
    logInfoP("Widget added to queue: %s", widget->getName().c_str());
    widget->setup();
    _widgetQueue.push(widget); // Keine Kopie, nur Zeiger wird in die Queue gelegt
}

void WidgetsManager::setup()
{
    size_t queueSize = _widgetQueue.size();
    for (size_t i = 0; i < queueSize; ++i)
    {
        Widget *widget = _widgetQueue.front();
        widget->setup();
        _widgetQueue.push(_widgetQueue.front());
        _widgetQueue.pop();
    }
}

void WidgetsManager::start()
{
    if (!_widgetQueue.empty())
    {
        _currentWidget = _widgetQueue.front();
        _widgetQueue.push(_currentWidget);
        _widgetQueue.pop();
        if (_currentWidget && !(_currentWidget->getAction() & Background))
        {
            _currentWidget->start();
            _currentTime = millis() + _currentWidget->getDisplayTime();
        }
    }
}

void WidgetsManager::loop()
{
    uint32_t currentTime = millis();
    // 1. If a current widget exists, check its flags and state.
    if (_currentWidget)
    {
        const WidgetFlags flags = _currentWidget->getAction(); // Get the action flags
        const WidgetState state = _currentWidget->getState();  // Get the state of the widget

        // a. If it is a `StatusWidget` and `DisplayEnabled`, keep it active and continue running.
        if ((flags & StatusWidget) && (flags & DisplayEnabled))
        {
            if (state == WidgetState::PAUSED) _currentWidget->resume(); // Resume Widget
            if (state == WidgetState::STOPPED) _currentWidget->start(); // Start Widget
            _currentWidget->loop();
            return; // StatusWidget has highest priority, so skip the rest of the code.
        }
        // b. If it is a `ManagedExternally` and `DisplayEnabled`, keep it active and continue running.
        if ((flags & ManagedExternally) && (flags & DisplayEnabled))
        {
            if (state == WidgetState::PAUSED) _currentWidget->resume(); // Resume Widget
            if (state == WidgetState::STOPPED) _currentWidget->start(); // Start Widget
            _currentWidget->loop();
            // External managed widgets are always active. No exit, since we need to check for StatusWidgets
        }
        // c. If it is a `ManagedExternally` and not `DisplayEnabled`, deactivate the widget.
        if ((flags & ManagedExternally) && !(flags & DisplayEnabled) && !(flags & Background))
        {
            logInfoP("Widget no longer DisplayEnabled: %s", _currentWidget->getName().c_str());
            _currentWidget->stop();
            _currentWidget = nullptr; // Widget deaktivieren
        }
        // d. If `AutoRemove` and the time has expired, remove the widget.
        if (flags & AutoRemove && currentTime >= _currentTime)
        {
            logInfoP("AutoRemove widget expired: %s", _currentWidget->getName().c_str());
            removeWidgetFromQueue(_currentWidget); // Widget entfernen
            _currentWidget = nullptr;
        }
        // e. If it is a `Background` widget, do not display it.
        if (flags & Background) // Background widgets are not displayed
        {
            //_currentWidget = nullptr;
        }
    }
    // 2. Search for a prioritized status widget in the queue.
    Widget *priorityWidget = getNextPriorityWidget(); // Returns the first prioritized status widget in the queue.
    // a. If a prioritized status widget exists:
    if (priorityWidget)
    {
        // i. Pause the current widget, if available.
        if (_currentWidget)
        {
            logInfoP("Pausing current widget: %s", _currentWidget->getName().c_str());
            _currentWidget->pause();
        }
        // ii. Activate the prioritized status widget.
        logInfoP("Starting priority status widget: %s", priorityWidget->getName().c_str());
        _currentWidget = priorityWidget;
        _currentWidget->start();
        _currentTime = currentTime + _currentWidget->getDisplayTime(); // Set the display time
        _currentWidget->loop();                                        // Call the loop() directly
        return;                                                        // Exit the loop, since the status widget has the highest priority.
    }
    // 3. If no status widget was prioritized, activate a normal widget.
    if (!_currentWidget && !_widgetQueue.empty())
    {
        _currentWidget = _widgetQueue.front();
        _widgetQueue.push(_currentWidget);
        _widgetQueue.pop();

        if (_currentWidget)
        {
            logInfoP("Starting normal widget: %s", _currentWidget->getName().c_str());
            _currentWidget->start();
            _currentTime = currentTime + _currentWidget->getDisplayTime();
        }
    }
    // 4. Run the `loop()` of the current widget if it is active.
    if (_currentWidget)
    {
        _currentWidget->loop();
    }
    else
    {
        // logInfoP("No active widget found."); ToDo! We need a default widget!
    }
    // 5. Check for Background widgets and run their loop() method
    for (size_t i = 0; i < _widgetQueue.size(); ++i)
    {
        Widget *widget = _widgetQueue.front();
        if (widget && widget->getAction() & Background)
        {
            widget->loop(); // Run the loop() method of the background widget
        }
        _widgetQueue.push(widget);
        _widgetQueue.pop();
    }
    // 6. If a display module is available, update it.
    if (_displayModule)
    {
        _displayModule->loop();
    }
}

Widget *WidgetsManager::getNextPriorityWidget()
{
    Widget *priorityWidget = nullptr;
    size_t queueSize = _widgetQueue.size();

    for (size_t i = 0; i < queueSize; ++i)
    {
        Widget *widget = _widgetQueue.front();
        _widgetQueue.push(widget);
        _widgetQueue.pop();

        WidgetFlags flags = widget->getAction();

        // Check if the widget is a status widget with `InternalEnabled`
        if ((flags & StatusWidget) && (flags & DisplayEnabled))
        {
            priorityWidget = widget; // StatusWidget has highest priority
        }
    }
    return priorityWidget;
}

Widget *WidgetsManager::getWidgetFromQueue(const char *widgetName)
{
    if (widgetName[0] == '\0' || _widgetQueue.empty()) return nullptr;
    size_t queueSize = _widgetQueue.size();
    for (size_t i = 0; i < queueSize; ++i)
    {
        Widget *widget = _widgetQueue.front();
        if (widget->getName().compare(widgetName) == 0)
        {
            return widget;
        }
        _widgetQueue.push(_widgetQueue.front());
        _widgetQueue.pop();
    }
    return nullptr;
}

Widget *WidgetsManager::getWidgetFromQueue(Widget *widget)
{
    if (widget != nullptr) return getWidgetFromQueue(widget->getName().c_str());
    return nullptr;
}

void WidgetsManager::removeWidgetFromQueue(const char *widgetName)
{
    if (widgetName[0] == '\0' || _widgetQueue.empty()) return;
    size_t queueSize = _widgetQueue.size();
    for (size_t i = 0; i < queueSize; ++i)
    {
        Widget *widget = _widgetQueue.front();
        if (widget->getName().compare(widgetName) == 0)
        {
            _widgetQueue.pop();
            delete widget;
            return;
        }
        _widgetQueue.push(_widgetQueue.front());
        _widgetQueue.pop();
    }
}

void WidgetsManager::removeWidgetFromQueue(Widget *widget)
{
    if (widget != nullptr) return removeWidgetFromQueue(widget->getName().c_str());
}
