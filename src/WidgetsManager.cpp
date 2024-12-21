#include "WidgetsManager.h"

void WidgetsManager::addWidget(Widget *widget)
{
    logInfoP("Widget added to queue.");
    if (!_widgetQueue.empty() && getWidgetFromQueue(widget->getName().c_str()) != nullptr)
    {
        const std::string widgetName = widget->getName() + "_" + std::to_string(random(0, 9)) + (char)random(65, 90);
        //widget->setName(widgetName);
        logDebugP("Widget name already in use. Added suffix to name: %s", widgetName.c_str());
    }
    widget->setDisplayModule(_displayModule); // Set the display module
    _widgetQueue.push(widget);                // Add the widget to the queue
    logInfoP("Widget added to queue: %s", widget->getName().c_str());
    widget->setup(); // Setup the widget (ToDo: Check if this is necessary here)
}

// To setup all widgets in the queue
void WidgetsManager::setup()
{
    if (!_widgetQueue.empty()) // Setup all widgets
    {
        std::queue<Widget *> tempQueue = _widgetQueue; // Make a copy of the widget queue and use it for setup
        while (!tempQueue.empty())
        {
            Widget *widget = tempQueue.front(); // Get the first widget
            widget->setup();                    // Setup the widget
            tempQueue.pop();                    // Remove the widget
        }
    }
}

void WidgetsManager::start()
{
    if (!_widgetQueue.empty())
    {
        // Start the first widget
        _currentWidget = _widgetQueue.front();                      // Get the first widget
        _currentWidget->start();                                    // Start the widget
        _currentTime = millis() + _currentWidget->getDisplayTime(); // Set the time when the widget should be removed
    }
}

void WidgetsManager::loop()
{
    if (_currentWidget != nullptr)
    {
        uint32_t currentTime = millis();

        if (currentTime >= _currentTime) // Check if the widget should be removed
        {
            WidgetsAction action = _currentWidget->getAction();
            if (action & AutoRemoveFlag)
            {
                logInfoP("AutoRemoveFlag is set. Stopping and removing the widget.");

                _currentWidget->stop();
                _widgetQueue.pop(); // Remove the widget
                _currentWidget = nullptr;
            }
            else if (action & ExternalManaged)
            {
                //logInfoP("Widget is externally managed.");
                _currentWidget->stop(); // FOr now, just stop the widget. Will be managed later externally.
            }
            // ToDo: implement the rest...

            // Check if there are more widgets
            if (_currentWidget == nullptr && !_widgetQueue.empty()) // Check if there are more widgets
            {
                logInfoP("Switching to the next widget.");
                _currentWidget = _widgetQueue.front();
                if (_currentWidget != nullptr)
                {
                    _currentWidget->start();
                    _currentTime = millis() + _currentWidget->getDisplayTime();
                }
            }

            if (_currentWidget == nullptr)
            {
                logInfoP("No more widgets available.");
                //_currentWidget = nullptr; // No more widgets. Set the current widget to nullptr! ToDo: Clear the display!
                return;
            }
        }
        _currentWidget->loop(); // Call the loop function of the current widget
    }
    if (_displayModule != nullptr)
    {
        _displayModule->loop();
    }
}

void WidgetsManager::stopCurrent()
{
    if (_currentWidget != nullptr)
    {
        _currentWidget->stop();
        _widgetQueue.pop(); // Remove the widget
    }
}

Widget *WidgetsManager::getWidgetFromQueue(const char *widgetName)
{
    if (!_widgetQueue.empty())
    {
        std::queue<Widget *> tempQueue = _widgetQueue; // Make a copy of the widget queue and use it for setup
        while (!tempQueue.empty())
        {
            Widget *widget = tempQueue.front(); // Get the first widget
            if (widget->getName().compare(widgetName) == 0)
            {
                return widget;
            }
            tempQueue.pop(); // Remove the widget
        }
    }
    return nullptr;
}
