#ifdef DEVICE_DISPLAY_MODULE
    #include "ButtonManager.h"
    #include "../DeviceDisplay.h" // route events through the gesture engine before the widget
    #include "hardware.h"

// #define USE_GPIO_MODULE

ButtonManager::ButtonManager(WidgetsManager* widgetManager)
    : _widgetManager(widgetManager)
{
}
ButtonManager::~ButtonManager()
{
}

/**
 * @brief Initialize button hardware (GPIO pins)
 * @return true if initialization successful, false otherwise
 */
bool ButtonManager::setup()
{
    #ifdef FRONT_CTRL_UP // front-panel buttons; driven via openknx.gpio (PCA9557)
    if (!openknx.gpio.isInitialized(1))
    {
        logErrorP("GPIO not initialized");
        return false;
    }

    // Assign hardware pins
    _buttonUp = FRONT_CTRL_UP;
    _buttonDown = FRONT_CTRL_DOWN;
    _buttonSelect = FRONT_CTRL_OK;
    _buttonLeft = FRONT_CTRL_LEFT;
    _buttonRight = FRONT_CTRL_RIGHT;

    // Configure all pins as INPUT with pull-up
    const uint16_t pins[] = {_buttonUp, _buttonDown, _buttonSelect, _buttonLeft, _buttonRight};
    for (auto pin : pins)
    {
        openknx.gpio.pinMode(pin, INPUT, true, 0);
    }

    _enabled = true;
    logInfoP("Initialized (5-way navigation)");
    return true;
    #else
    logWarningP("GPIO module not available");
    return false;
    #endif
}

/**
 * @brief Process button state changes (call in loop())
 * Should be called regularly from main loop
 */
void ButtonManager::loop()
{
    if (!_enabled) return;

    uint32_t currentTime = millis();
    if (currentTime - _lastCheck < _checkInterval) return;
    _lastCheck = currentTime;

    // Check all buttons
    const struct
    {
        uint16_t pin;
        ButtonType type;
        size_t index;
    } buttons[] = {
        {_buttonUp, ButtonType::UP, 0},
        {_buttonDown, ButtonType::DOWN, 1},
        {_buttonSelect, ButtonType::SELECT, 2},
        {_buttonLeft, ButtonType::LEFT, 3},
        {_buttonRight, ButtonType::RIGHT, 4}};

    for (const auto& btn : buttons)
    {
        processButton(btn.pin, btn.type, btn.index);
    }
}

/**
 * @brief Process button and forward events to active widget
 */
void ButtonManager::processButton(uint16_t pin, ButtonType type, size_t index)
{
    ButtonEvent* event = checkButton(pin, type, index);
    if (!event) return;

    const char* typeStr = nullptr;
    switch (type)
    {
        case ButtonType::UP: typeStr = "UP"; break;
        case ButtonType::DOWN: typeStr = "DOWN"; break;
        case ButtonType::SELECT: typeStr = "SELECT"; break;
        case ButtonType::LEFT: typeStr = "LEFT"; break;
        case ButtonType::RIGHT: typeStr = "RIGHT"; break;
        default: typeStr = "UNKNOWN"; break;
    }

    const char* actionStr = nullptr;
    switch (event->action)
    {
        case ButtonAction::PRESS: actionStr = "PRESS"; break;
        case ButtonAction::RELEASE: actionStr = "RELEASE"; break;
        case ButtonAction::LONG_PRESS: actionStr = "LONG_PRESS"; break;
        case ButtonAction::VERY_LONG_PRESS: actionStr = "VERY_LONG_PRESS"; break;
        default: actionStr = "UNKNOWN"; break;
    }
    logDebugP("Button: %s (%d) Action: %s (%d)", typeStr, static_cast<int>(type), actionStr, static_cast<int>(event->action));

    // When a gesture router is wired it forwards to the active widget itself; do not also forward here.
    if (_gestureRouter)
    {
        _gestureRouter->handleButtonEvent(*event);
        delete event;
        return;
    }

    // Legacy path (no router wired): forward straight to the active widget.
    Widget* activeWidget = _widgetManager->getActiveButtonWidget();

    if (!activeWidget)
    {
        // No active widget → wake up display
        _widgetManager->wakeUpDisplay();
        logDebugP("Waking display (no active widget)");
    }
    else if (activeWidget->handleButtonEvent(*event))
    {
        // Widget handled event → reset user interaction timer
        _widgetManager->userInteraction();
    }

    delete event;
}

/**
 * @brief Check button state and generate events; returns ButtonEvent* if event occurred, nullptr otherwise
 * @return ButtonEvent* if event occurred, nullptr otherwise
 */
ButtonEvent* ButtonManager::checkButton(uint16_t pin, ButtonType type, size_t index)
{
    bool isPressed = readButton(pin);

    // LEFT button is active-LOW (inverted logic)
    if (type == ButtonType::LEFT)
        isPressed = !isPressed;

    uint32_t currentTime = millis();
    ButtonEvent* event = nullptr;

    if (isPressed && !_buttonPressed[index])
    {
        // Button pressed
        _buttonPressed[index] = true;
        _buttonPressTime[index] = currentTime;
        event = new ButtonEvent(type, ButtonAction::PRESS);
    }
    else if (!isPressed && _buttonPressed[index])
    {
        // Button released
        _buttonPressed[index] = false;
        uint32_t duration = currentTime - _buttonPressTime[index];

        if (duration > 5000)
            event = new ButtonEvent(type, ButtonAction::VERY_LONG_PRESS);
        else if (duration > 500)
            event = new ButtonEvent(type, ButtonAction::LONG_PRESS);
        else
            event = new ButtonEvent(type, ButtonAction::RELEASE);
    }

    return event;
}

/**
 * @brief Read button state from hardware GPIO pin number
 * @return true if button is pressed, false otherwise
 */
bool ButtonManager::readButton(uint16_t pin)
{
    #ifdef FRONT_CTRL_UP // front-panel buttons; driven via openknx.gpio (PCA9557)
    return openknx.gpio.digitalRead(pin);
    #else
    return false;
    #endif
}

/**
 * @brief Enable/disable button input
 * @param enabled true to enable buttons, false to disable
 */
void ButtonManager::setEnabled(bool enabled)
{
    _enabled = enabled;
    logInfoP("%s", enabled ? "Enabled" : "Disabled");
}

// --- Continuous hold-duration query API ---------------------------------------------
// Getters only read the debounced state maintained by checkButton(); no hardware read, alloc or events.

/**
 * @brief Index (0..4) of the currently held button, lowest index wins; -1 if none held.
 */
int ButtonManager::getHeldButtonIndex() const
{
    for (size_t i = 0; i < ButtonCount; ++i)
    {
        if (_buttonPressed[i]) return static_cast<int>(i);
    }
    return -1;
}

/**
 * @brief ButtonType of the currently held button (lowest index wins).
 * @param outType receives the held button's type; untouched when nothing is held
 * @return true if a button is currently held, false otherwise
 */
bool ButtonManager::getHeldButton(ButtonType& outType) const
{
    const int idx = getHeldButtonIndex();
    if (idx < 0) return false;

    // Index order matches loop(): UP=0, DOWN=1, SELECT=2, LEFT=3, RIGHT=4 (== ButtonType values).
    outType = static_cast<ButtonType>(idx);
    return true;
}

/**
 * @brief True while any navigation button is currently held down.
 */
bool ButtonManager::isAnyButtonDown() const
{
    return getHeldButtonIndex() >= 0;
}
#endif // DEVICE_DISPLAY_MODULE