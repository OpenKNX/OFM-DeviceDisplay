#ifdef DEVICE_DISPLAY_MODULE
    #include "ProgMode.h"
    #include "OpenKNX.h"

WidgetProgMode::WidgetProgMode(uint32_t displayTime, WidgetFlags action)
    : _display(nullptr), _action(action), _displayTime(displayTime), _state(WidgetState::STOPPED),
      _lastBlinkTime(0), _showProgMode(true)
{
}

WidgetProgMode::~WidgetProgMode()
{
}

void WidgetProgMode::setup()
{
    if (!_display)
    {
        logErrorP("Display module is NULL.");
        return;
    }
}

void WidgetProgMode::start()
{
    logDebugP("start...");
    _state = WidgetState::RUNNING;
    _lastBlinkTime = millis();
    _drawStep = 1; // Start initial draw in next loop
}

void WidgetProgMode::stop()
{
    logDebugP("stop...");
    _state = WidgetState::STOPPED;
    if (_display)
    {
        _display->display->clearDisplay();
        _display->displayBuff();
    }
}

void WidgetProgMode::pause()
{
    logDebugP("paused...");
    _state = WidgetState::PAUSED;
}

void WidgetProgMode::resume()
{
    logDebugP("resume...");
    _state = WidgetState::RUNNING;
    _lastBlinkTime = millis();
    _drawStep = 1; // Start redraw in next loop
}

void WidgetProgMode::loop()
{
    if (!_display || _state != WidgetState::RUNNING) return;

    unsigned long currentTime = millis();

    // TODO cleanup split implementation...

    // Check if it's time to toggle the blink state
    if (currentTime - _lastBlinkTime >= PROG_MODE_BLINK_DELAY)
    {
        _lastBlinkTime = currentTime;
        _showProgMode = !_showProgMode; // Toggle the blink state
        _drawStep = 1;
    }
    else if (_drawStep == 0)
    {
        // nothing to draw
        return;
    }

    draw(); // Update the display
}

void WidgetProgMode::draw()
{
    if (!_display) return;

    // Partial drawing, step by step in loop()-calls
    switch (_drawStep++)
    {
        case 1:
            _display->display->clearDisplay(); // Clear the display
            _display->display->setTextColor(SSD1306_WHITE);
            break;
        case 2:
            // Header: "OpenKNX" (always visible)
            _display->display->setTextSize(1);
            _display->display->setCursor(0, 0);
            _display->display->print("   www.OpenKNX.de   ");
            break;
        case 3:
            // "ProgMode!" message (blinking) - big, per mock (renderProgActive: "ProgMode aktiv")
            if (_showProgMode)
            {
                _display->display->setTextColor(SSD1306_BLACK, SSD1306_WHITE);
            }
            else
            {
                _display->display->setTextColor(SSD1306_WHITE);
            }

            _display->display->setTextSize(2);
            _display->display->setCursor(0, 16);
            _display->display->print(" ProgMode!");
            break;
        case 4:
            // PA row (mock: "PA  15.15.0"). Individual address from openknx.info.
            _display->display->setTextColor(SSD1306_WHITE);
            _display->display->setTextWrap(false);
            _display->display->setTextSize(1);
            _display->display->setCursor(0, 36);
            _display->display->print(" PA ");
            _display->display->print(openknx.info.humanIndividualAddress().c_str());
            break;
        case 5:
            // ETS hint (mock renderProgActive: ".pasub" line "ETS: Geraet programmieren").
            _display->display->setTextColor(SSD1306_WHITE);
            _display->display->setTextWrap(false);
            _display->display->setTextSize(1);
            _display->display->setCursor(0, 46);
            _display->display->print(" ETS: Geraet programmieren");
            break;
        case 6:
            // Footer hint. "gedrueckthalten" (ASCII; the SSD1306 CP437 font has no umlaut).
            // TODO(polish): marquee-scroll this line — it is wider than 128 px at size 1.
            _display->display->setTextColor(SSD1306_WHITE);
            _display->display->setTextWrap(false);
            _display->display->setTextSize(1);
            _display->display->setCursor(0, 52);
            _display->display->print(" OK gedrueckthalten = beenden");
            break;
        case 7:
            // Update the display
            _display->displayBuff();
            // fall trough
        default:
            _drawStep = 0; // completed
            break;
    }
}
#endif // DEVICE_DISPLAY_MODULE