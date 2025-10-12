#include "WidgetsManager.h"
#include "OpenKNX.h"

void WidgetsManager::addWidget(Widget* widget)
{
    if (widget == nullptr || _displayModule == nullptr) return;

    if (!_widgetQueue.empty() && getWidgetFromQueue(widget) != nullptr)
    {
        const std::string widgetName = widget->getName() + "_" + std::to_string(random(0, 9)) + (char)random(65, 90);
        widget->setName(widgetName);
        logDebugP("Widget name already in use. Added suffix to name: %s", widgetName.c_str());
    }

    widget->setDisplayModule(_displayModule);
    logDebugP("Widget added to queue: %s", widget->getName().c_str());
    widget->setup();
    _widgetQueue.push_back(widget); // Keine Kopie, nur Zeiger wird in die Queue gelegt
}

void WidgetsManager::setup()
{
    for (auto& widget : _widgetQueue)
    {
        if (widget != nullptr)
        {
            widget->setup();
        }
    }
}

void WidgetsManager::start()
{
    for (auto& widget : _widgetQueue)
    {
        if (!widget) continue;

        // Only start background widgets initially in background mode!!
        if (widget->getAction() & WidgetFlags::Background)
        {
            logDebugP("Initial starting background widget: %s", widget->getName().c_str());
            widget->background();
        }
    }
}

void WidgetsManager::loop()
{
    uint32_t currentTime = millis();

    handleCurrentWidget(currentTime); // 1. We handle the current widget first

    if (activatePriorityWidget(currentTime)) // 2. Then we check for priority widgets
    {
        _currentWidget->loop();
        return;
    }

    activateNormalWidget(currentTime); // 3. Then we check for normal widgets

    handleBackgroundAndDefaultWidgets(currentTime); // 4. Finally, handle background and default widgets

    if (_currentWidget && _currentWidget->getState() == WidgetState::RUNNING)
    { // 5. Loop the current widget if exists and is running
        _currentWidget->loop();
    }
    if (_displayModule) _displayModule->loop(); // 6. Always loop the display module if exists
}

Widget* WidgetsManager::getNextPriorityWidget()
{
    for (auto& widget : _widgetQueue)
    {
        WidgetFlags flags = widget->getAction();
        // Check if the widget is a status widget with `InternalEnabled`
        if ((flags & StatusWidget) && (flags & DisplayEnabled))
        {
            return widget; // StatusWidget has highest priority
        }
    }
    return nullptr;
}

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

Widget* WidgetsManager::getWidgetFromQueue(Widget* widget)
{
    if (widget != nullptr) return getWidgetFromQueue(widget->getName());
    return nullptr;
}

void WidgetsManager::removeWidgetFromQueue(const char* widgetName)
{
    if (widgetName[0] == '\0' || _widgetQueue.empty()) return;
    for (auto it = _widgetQueue.begin(); it != _widgetQueue.end(); ++it)
    {
        if ((*it)->getName().compare(widgetName) == 0)
        {
            // delete *it;
            _widgetQueue.erase(it);
            return;
        }
    }
}

void WidgetsManager::removeWidgetFromQueue(Widget* widget)
{
    if (widget != nullptr) removeWidgetFromQueue(widget->getName().c_str());
}

// Loop - functionality support
void WidgetsManager::handleCurrentWidget(uint32_t currentTime)
{
    if (!_currentWidget) return;

    const WidgetFlags flags = _currentWidget->getAction();
    const WidgetState state = _currentWidget->getState();

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

    if ((flags & StatusWidget) && state == WidgetState::RUNNING && !(flags & DisplayEnabled))
    {
        logDebugP("StatusWidget no longer DisplayEnabled: %s", _currentWidget->getName().c_str());
        _currentWidget->stop();
        _currentWidget = nullptr;
        return;
    }

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
    }

    if ((flags & ManagedExternally) && !(flags & DisplayEnabled) && !(flags & Background))
    {
        logDebugP("Widget no longer DisplayEnabled: %s", _currentWidget->getName().c_str());
        _currentWidget = nullptr;
    }

    if ((flags & AutoRemove) && currentTime >= _currentTime)
    {
        logDebugP("AutoRemove widget expired: %s", _currentWidget->getName().c_str());
        removeWidgetFromQueue(_currentWidget);
        _currentWidget = nullptr;
    }
}

bool WidgetsManager::activatePriorityWidget(uint32_t currentTime)
{
    Widget* priorityWidget = getNextPriorityWidget();
    if (!priorityWidget) return false;

    if (priorityWidget == _currentWidget) return true; // Already active
    if (_currentWidget)
    {
        if (_currentWidget->getAction() & DefaultWidget)
        {
            logDebugP("Stopping active DefaultWidget: %s", _currentWidget->getName().c_str());
            _currentWidget->stop();
        }
        else
        {
            logDebugP("Pausing current widget: %s", _currentWidget->getName().c_str());
            if (_currentWidget->getState() == WidgetState::RUNNING)
            {
                _currentWidget->pause();
            }
        }
    }

    logDebugP("Starting priority status widget: %s", priorityWidget->getName().c_str());
    _currentWidget = priorityWidget;
    _currentWidget->start();
    _currentTime = currentTime + _currentWidget->getDisplayTime();
    _lastInteractionTime = currentTime;
    return true;
}

void WidgetsManager::activateNormalWidget(uint32_t currentTime)
{
    if (_currentWidget || _widgetQueue.empty() || currentTime < _currentTime) return;

    _currentWidget = _widgetQueue.front();
    _widgetQueue.push_back(_currentWidget);
    _widgetQueue.pop_front();

    const WidgetFlags currentWidgetFlags = _currentWidget->getAction();
    if (_currentWidget &&
        !(currentWidgetFlags & DefaultWidget) &&
        !(currentWidgetFlags & Background) &&
        !(currentWidgetFlags & ManagedExternally))
    {
        logDebugP("Starting normal widget: %s", _currentWidget->getName().c_str());
        _currentWidget->start();
        _currentTime = currentTime + _currentWidget->getDisplayTime();
        _lastInteractionTime = currentTime;
    }
    else
    {
        _currentWidget = nullptr;
    }
}

void WidgetsManager::handleBackgroundAndDefaultWidgets(uint32_t currentTime)
{
    for (auto& widget : _widgetQueue)
    {
        if (widget && (widget->getAction() & Background))
        {
            widget->loop();
            if (widget->getAction() & DisplayEnabled)
            {
                if (_currentWidget && _currentWidget != widget)
                {
                    logDebugP("Stopping current widget: %s", _currentWidget->getName().c_str());
                    _currentWidget->stop();
                }
                if (_currentWidget != widget)
                {
                    logDebugP("Activating background widget: %s", widget->getName().c_str());
                    _currentWidget = widget;
                    _currentWidget->start();
                    _currentTime = millis() + _currentWidget->getDisplayTime();
                    _lastInteractionTime = millis();
                }
            }
        }
        else if ((currentTime - _lastInteractionTime >= _idleTimeout) &&
                 widget && (widget->getAction() & DefaultWidget))
        {
            WidgetState state = widget->getState();
            if (_currentTime < currentTime)
            {
                if (_currentWidget != widget)
                {
                    if (_currentWidget && _currentWidget->getAction() & DefaultWidget)
                    {
                        logDebugP("Stopping current: DefaultWidget: %s", _currentWidget->getName().c_str());
                        _currentWidget->stop();
                    }
                    _currentWidget = widget;
                    if (state != WidgetState::RUNNING)
                    {
                        logDebugP("Starting DefaultWidget: %s", _currentWidget->getName().c_str());
                        _currentWidget->start();
                        _currentTime = currentTime + _currentWidget->getDisplayTime();
                    }
                }
                if (state == WidgetState::RUNNING)
                {
                    widget->loop();
                }
            }

            if (currentTime - _lastInteractionTime >= _idleTimeout)
            {
                if (state == WidgetState::RUNNING)
                {
                    logDebugP("Stopping DefaultWidget due to timeout: %s", widget->getName().c_str());
                    widget->stop();
                }
            }
        }
    }
}

void WidgetsManager::logWidgetQueue()
{
    logDebugP("=== Widget Queue Dump ===");
    if (_widgetQueue.empty())
    {
        logDebugP("Queue is empty.");
        return;
    }

    for (size_t i = 0; i < _widgetQueue.size(); ++i)
    {
        Widget* widget = _widgetQueue[i];
        if (widget)
        {
            logDebugP("Index: %d | Name: %s | Flags: %d | State: %d",
                      i,
                      widget->getName().c_str(),
                      widget->getAction(),
                      widget->getState());
        }
        else
        {
            logDebugP("Index: %d | nullptr", i);
        }
    }
    logDebugP("=========================");
}