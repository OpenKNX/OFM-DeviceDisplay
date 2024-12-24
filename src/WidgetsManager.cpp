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
    uint32_t currentTime = millis();

    // Falls ein Status-Widget läuft, prüfe seine Bedingungen
    if (_currentWidget && (_currentWidget->getAction() & StatusFlag))
    {
        if (!(_currentWidget->getAction() & InternalEnabled))
        {
            // Deaktiviertes Status-Widget stoppen
            logInfoP("Stopping disabled status widget: %s", _currentWidget->getName().c_str());
            _currentWidget->stop();
            // delete _currentWidget;
            _currentWidget = nullptr;
            return;
        }
        else
        {
            _currentWidget->loop();
            return; // Ein Status-Widget wird weiterhin bevorzugt behandelt
        }
    }

    // Suche nach einem priorisierten Status-Widget in der Warteschlange
    Widget *priorityWidget = nullptr;

    size_t queueSize = _widgetQueue.size();
    for (size_t i = 0; i < queueSize; ++i)
    {
        Widget *widget = _widgetQueue.front();   // Vorne aus der Warteschlange holen
        _widgetQueue.push(_widgetQueue.front()); // Wieder hinten anhängen
        _widgetQueue.pop();                      // Entfernen jetzt vorne

        WidgetsAction action = widget->getAction();

        // Prüfe, ob es ein aktives Status-Widget ist
        if ((action & StatusFlag) && (action & InternalEnabled))
        {
            priorityWidget = widget; // Priorisiere dieses Widget
        }
    }

    // Falls ein priorisiertes Status-Widget gefunden wurde, aktiviere es
    if (priorityWidget)
    {
        if (_currentWidget && _currentWidget->getState() == WidgetState::RUNNING)
        {
            logInfoP("Pausing current widget: %s", _currentWidget->getName().c_str());
            _currentWidget->pause(); // Pausiere das aktuelle Widget
        }
        _currentWidget = priorityWidget;
        logInfoP("Starting priority status widget: %s", _currentWidget->getName().c_str());
        _currentWidget->start();
        _currentWidget->loop();
        return;
    }

    // Wenn kein priorisiertes Status-Widget gefunden wurde, verarbeite normale Widgets
    if (!_currentWidget && !_widgetQueue.empty())
    {
        _currentWidget = _widgetQueue.front();
        _widgetQueue.push(_widgetQueue.front());
        _widgetQueue.pop();

        if (_currentWidget)
        {
            logInfoP("Starting normal widget: %s", _currentWidget->getName().c_str());
            _currentWidget->start();
            _currentTime = currentTime + _currentWidget->getDisplayTime();
        }
    }

    // Aktualisiere das aktuelle Widget
    if (_currentWidget)
    {
        WidgetsAction action = _currentWidget->getAction();
        _currentWidget->loop();

        // Überprüfe Ablaufzeit oder Flags
        if (currentTime >= _currentTime)
        {
            if (action & AutoRemoveFlag)
            {
                logInfoP("AutoRemoveFlag is set. Stopping and removing widget: %s", _currentWidget->getName().c_str());
                _currentWidget->stop();
                removeWidgetFromQueue(_currentWidget);
                _currentWidget = nullptr;
            }
            else if (action & ExternalManaged)
            {
                if (!(action & InternalEnabled) && _currentWidget->getState() == WidgetState::RUNNING)
                {
                    logInfoP("ExternalManaged widget is no longer enabled. Stopping: %s", _currentWidget->getName().c_str());
                    _currentWidget->stop();
                    _currentWidget = nullptr;
                }
            }
            else if (action & MarkedForRemove)
            {
                logInfoP("MarkedForRemove is set. Removing widget: %s", _currentWidget->getName().c_str());
                _currentWidget->stop();
                removeWidgetFromQueue(_currentWidget);
                _currentWidget = nullptr;
            }
        }
    }

    // Display-Modul aktualisieren
    if (_displayModule)
    {
        _displayModule->loop();
    }
}

Widget *WidgetsManager::getWidgetFromQueue(const char *widgetName)
{
    if ( widgetName[0] == '\0' || _widgetQueue.empty() ) return nullptr;
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
    if ( widgetName[0] == '\0' || _widgetQueue.empty() ) return;
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
