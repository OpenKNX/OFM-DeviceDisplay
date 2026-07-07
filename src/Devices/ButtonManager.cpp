#ifdef DEVICE_DISPLAY_MODULE
#include "ButtonManager.h"
#include "hardware.h"

//#define USE_GPIO_MODULE

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
#ifdef FRONT_CTRL_UP // front-panel buttons exist when the HardwareConfig defines them; driven via native openknx.gpio (PCA9557)
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

    //logDebugP("Button %d event %d", type, event->action);
    // Log button type and action as text for easier debugging
    const char* typeStr = nullptr;
    switch (type) {
      case ButtonType::UP: typeStr = "UP"; break;
      case ButtonType::DOWN: typeStr = "DOWN"; break;
      case ButtonType::SELECT: typeStr = "SELECT"; break;
      case ButtonType::LEFT: typeStr = "LEFT"; break;
      case ButtonType::RIGHT: typeStr = "RIGHT"; break;
      default: typeStr = "UNKNOWN"; break;
    }

    const char* actionStr = nullptr;
    switch (event->action) {
      case ButtonAction::PRESS: actionStr = "PRESS"; break;
      case ButtonAction::RELEASE: actionStr = "RELEASE"; break;
      case ButtonAction::LONG_PRESS: actionStr = "LONG_PRESS"; break;
      case ButtonAction::VERY_LONG_PRESS: actionStr = "VERY_LONG_PRESS"; break;
      default: actionStr = "UNKNOWN"; break;
    }
    logDebugP("Button: %s (%d) Action: %s (%d)", typeStr, static_cast<int>(type), actionStr, static_cast<int>(event->action));

    
    // Get active widget that wants button input
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
#ifdef FRONT_CTRL_UP // front-panel buttons exist when the HardwareConfig defines them; driven via native openknx.gpio (PCA9557)
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
#endif // DEVICE_DISPLAY_MODULE