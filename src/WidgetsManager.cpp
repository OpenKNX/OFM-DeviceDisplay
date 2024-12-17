#include "WidgetsManager.h"

void WidgetsManager::addWidget(Widget* widget)
{
    widget->setDisplayModule(_displayModule);
    _widgetQueue.push(widget);
}

void WidgetsManager::setup()
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
                // logInfoP("Widget wird automatisch entfernt.");
                openknx.logger.log("Widget wird automatisch entfernt.");

                _currentWidget->stop();
                _widgetQueue.pop(); // Remove the widget
                _currentWidget = nullptr;
            }

            if( _currentWidget == nullptr && !_widgetQueue.empty()) // Check if there are more widgets
            {
                openknx.logger.log("Wechsel zum nächsten Widget.");
                _currentWidget = _widgetQueue.front();
                if(_currentWidget != nullptr) {
                  _currentWidget->start();
                  _currentTime = millis() + _currentWidget->getDisplayTime();
                }
                
            }
            else // ToDo: implement the rest...
            {
                openknx.logger.log("Keine weiteren Widgets vorhanden.");
                _currentWidget = nullptr; // No more widgets. Set the current widget to nullptr! ToDo: Clear the display!
                return;
            }
        }
        _currentWidget->loop(); // Call the loop function of the current widget
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