#include "WidgetsManager.h"
#include "OpenKNX.h"

void WidgetsManager::addWidget(Widget *widget)
{
    if (widget == nullptr || _displayModule == nullptr) return;

    if (!_widgetQueue.empty() && getWidgetFromQueue(widget->getName().c_str()) != nullptr)
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
    // Initialisiert alle Widgets in der Warteschlange
    std::queue<Widget *> tempQueue = _widgetQueue;
    while (!tempQueue.empty())
    {
        Widget *widget = tempQueue.front();
        widget->setup();
        tempQueue.pop();
    }
}

void WidgetsManager::start()
{
    if (!_widgetQueue.empty())
    {
        _currentWidget = _widgetQueue.front();
        _widgetQueue.pop();
        if (_currentWidget)
        {
            _currentWidget->start();
            _currentTime = millis() + _currentWidget->getDisplayTime();
        }
    }
}

void WidgetsManager::loop()
{
    if (_currentWidget)
    {
        uint32_t currentTime = millis();

        if (currentTime >= _currentTime)
        {
            WidgetsAction action = _currentWidget->getAction();
            if (action & AutoRemoveFlag)
            {
                logInfoP("AutoRemoveFlag is set. Stopping and removing the widget.");
                _currentWidget->stop();
                if (_currentWidget->getState() == WidgetState::STOPPED)
                {
                    logDebugP("Widget %s is stopped and removed.", _currentWidget->getName().c_str());
                    delete _currentWidget;
                    _currentWidget = nullptr;
                }
            }
            else if (action & ExternalManaged)
            {
                if (_currentWidget->getState() == WidgetState::RUNNING)
                {
                    logInfoP("ExternalManaged is set. Stopping the widget. FOR TESTING!");
                    _currentWidget->stop();
                }
            }

            if (!_currentWidget && !_widgetQueue.empty())
            {
                logInfoP("Switching to the next widget.");
                _currentWidget = _widgetQueue.front();
                _widgetQueue.pop();
                if (_currentWidget)
                {
                    _currentWidget->start();
                    _currentTime = millis() + _currentWidget->getDisplayTime();
                }
            }

            if (!_currentWidget)
            {
                logInfoP("No more widgets available.");
                return;
            }
        }
        _currentWidget->loop();
    }
    if (_displayModule != nullptr)
    {
        _displayModule->loop();
    }
}

void WidgetsManager::stopCurrent()
{
    if (_currentWidget)
    {
        _currentWidget->stop();
        delete _currentWidget; // Speicher freigeben
        _currentWidget = nullptr;
    }
}

Widget *WidgetsManager::getWidgetFromQueue(const char *widgetName)
{
    std::queue<Widget *> tempQueue = _widgetQueue; // Temporäre Kopie der Queue
    while (!tempQueue.empty())
    {
        Widget *widget = tempQueue.front();
        if (widget->getName().compare(widgetName) == 0)
        {
            return widget;
        }
        tempQueue.pop();
    }
    return nullptr;
}