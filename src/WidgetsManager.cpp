#include "WidgetsManager.h"

void WidgetsManager::addWidget(Widget* widget)
{
    widget->setDisplayModule(_displayModule);
    logInfoP("Widget is added to the queue.");
    _widgetQueue.push(widget);
    // Setting up the widget
    widget->setup();
}

// To setup all widgets in the queue
void WidgetsManager::setup()
{
    if (!_widgetQueue.empty()) // Setup all widgets
    {
        std::queue<Widget*> tempQueue = _widgetQueue; // Make a copy of the widget queue and use it for setup
        while (!tempQueue.empty())
        {
            Widget* widget = tempQueue.front(); // Get the first widget
            widget->setup(); // Setup the widget
            tempQueue.pop(); // Remove the widget
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
                // logInfoP("Widget wird extern verwaltet.");
                logInfoP("Widget is externally managed.");
            }
            // ToDo: implement the rest...
            
            // Check if there are more widgets
            if( _currentWidget == nullptr && !_widgetQueue.empty()) // Check if there are more widgets
            {
                logInfoP("Switching to the next widget.");
                _currentWidget = _widgetQueue.front();
                if(_currentWidget != nullptr) {
                  _currentWidget->start();
                  _currentTime = millis() + _currentWidget->getDisplayTime();
                }
                
            }
            
            if(_currentWidget == nullptr )
            {
                logInfoP("No more widgets available.");
                //_currentWidget = nullptr; // No more widgets. Set the current widget to nullptr! ToDo: Clear the display!
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