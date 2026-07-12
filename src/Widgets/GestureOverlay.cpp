#ifdef DEVICE_DISPLAY_MODULE
    /**
     * @file        GestureOverlay.cpp
     * @brief       Hold-/Auto-confirm countdown overlay widget
     * @details     Renders the gesture confirm overlay analogous to WidgetProgMode: partial
     *              _drawStep drawing, per-action title/label, a horizontal countdown bar
     *              (fraction * width) and a subtext with the remaining seconds. Phase, bar
     *              fraction, seconds and action are read from the bound GestureEngine.
     *              Confirmed/cancelled callbacks fire exactly once per gesture.
     * @version     0.0.1
     * @date        2026-07-09
     * @copyright   Copyright (c) 2026, Erkan Çolak
     *              Licensed under GNU GPL v3.0
     **/
    #include "GestureOverlay.h"
    #include "OpenKNX.h"

WidgetGestureOverlay::WidgetGestureOverlay(uint32_t displayTime, WidgetFlags action)
    : _display(nullptr), _action(action), _displayTime(displayTime), _state(WidgetState::STOPPED)
{
    // Overlay must win over everything on screen while a gesture is running.
    _priority = WidgetPriority::WIDGET_PRIO_CRITICAL;
}

WidgetGestureOverlay::~WidgetGestureOverlay()
{
}

void WidgetGestureOverlay::setup()
{
    if (!_display)
    {
        logErrorP("Display module is NULL.");
        return;
    }
}

void WidgetGestureOverlay::start()
{
    logDebugP("start...");
    _state = WidgetState::RUNNING;

    // Reset the change-tracking snapshot + one-shot callback latches so a fresh overlay
    // paints on the first loop() and re-arms the confirm/cancel callbacks.
    _lastPhase = GesturePhase::Idle;
    _lastBarCols = 0xFF;
    _lastSeconds = 0xFF;
    _lastAction = GestureAction::None;
    _confirmedFired = false;
    _cancelledFired = false;
    _drawStep = 1;
}

void WidgetGestureOverlay::stop()
{
    logDebugP("stop...");
    _state = WidgetState::STOPPED;
    if (_display)
    {
        _display->display->clearDisplay();
        _display->displayBuff();
    }
}

void WidgetGestureOverlay::pause()
{
    logDebugP("paused...");
    _state = WidgetState::PAUSED;
}

void WidgetGestureOverlay::resume()
{
    logDebugP("resume...");
    _state = WidgetState::RUNNING;
    _drawStep = 1;
}

void WidgetGestureOverlay::loop()
{
    if (!_display || _state != WidgetState::RUNNING) return;
    if (!_engine) return; // nothing to show without a bound engine

    const GesturePhase phase = _engine->getPhase();
    const GestureAction action = _engine->getCurrentAction();
    const float frac = _engine->getBarFraction();
    const uint8_t seconds = _engine->getRemainingSeconds();

    // Fire the one-shot outcome callbacks. Firing/Done -> confirmed, Aborted -> cancelled.
    if ((phase == GesturePhase::Firing || phase == GesturePhase::Done) && !_confirmedFired)
    {
        _confirmedFired = true;
        if (_onConfirmed) _onConfirmed();
    }
    else if (phase == GesturePhase::Aborted && !_cancelledFired)
    {
        _cancelledFired = true;
        if (_onCancelled) _onCancelled();
    }

    // Bar fill in pixels, so we can detect visible changes and only repaint on change.
    const uint8_t width = _display->GetDisplayWidth();
    uint8_t barCols = static_cast<uint8_t>(frac * static_cast<float>(width) + 0.5f);
    if (barCols > width) barCols = width;

    const bool changed = (phase != _lastPhase) || (action != _lastAction) ||
                         (barCols != _lastBarCols) || (seconds != _lastSeconds);

    if (changed)
    {
        _lastPhase = phase;
        _lastAction = action;
        _lastBarCols = barCols;
        _lastSeconds = seconds;
        _drawStep = 1;
    }
    else if (_drawStep == 0)
    {
        // nothing changed and no partial draw in progress
        return;
    }

    draw();
}

const char *WidgetGestureOverlay::titleForAction(GestureAction action) const
{
    // Mirrors the mockup gTitle(): pause -> "Auto-Wechsel", reboot -> "System",
    // prog -> "Prog-Mode".
    switch (action)
    {
        case GestureAction::Pause:
            return "Auto-Wechsel";
        case GestureAction::Reboot:
            return "System";
        case GestureAction::ProgMode:
            return "Prog-Mode";
        case GestureAction::DisplayOff:
            return "Display";
        default:
            return "Aktion";
    }
}

const char *WidgetGestureOverlay::labelForAction(GestureAction action) const
{
    // Mirrors the mockup gLabel(false) (the running-countdown label). The "done"/checkmark
    // variants depend on runtime toggle state (progOn/homePaused) which the engine does not
    // expose here, so the overlay shows the imperative call-to-action label.
    switch (action)
    {
        case GestureAction::Pause:
            // Reflect the current pause state — holding while paused RESUMES the rotation.
            return _pauseActive ? "FORTSETZEN" : "PAUSIEREN";
        case GestureAction::Reboot:
            return "NEUSTART";
        case GestureAction::ProgMode:
            // Holding while active ENDS prog mode.
            // ("DEAKTIVIEREN" is too wide for the big font on 128px, so "BEENDEN".)
            return _progActive ? "BEENDEN" : "AKTIVIEREN";
        case GestureAction::DisplayOff:
            return "AUS";
        default:
            return "";
    }
}

void WidgetGestureOverlay::draw()
{
    if (!_display) return;

    const uint8_t width = _display->GetDisplayWidth();

    // Geometry for the horizontal countdown bar (matches the mockup .pbar block).
    const int16_t barX = 0;
    const int16_t barY = 40;
    const int16_t barH = 10;
    const int16_t barW = static_cast<int16_t>(width);
    const int16_t fillW = static_cast<int16_t>(_lastBarCols);

    // Partial drawing, step by step in loop()-calls.
    switch (_drawStep++)
    {
        case 1:
            _display->display->clearDisplay();
            _display->display->setTextColor(SSD1306_WHITE);
            break;
        case 2:
            // Title row (per action): gTitle()
            _display->display->setTextSize(1);
            _display->display->setCursor(0, 0);
            _display->display->print(titleForAction(_lastAction));
            break;
        case 3:
            // Big action label (call-to-action): gLabel()
            _display->display->setTextColor(SSD1306_WHITE);
            _display->display->setTextSize(2);
            _display->display->setCursor(0, 16);
            _display->display->print(labelForAction(_lastAction));
            break;
        case 4:
            // Countdown bar: outline + fill (fraction * width)
            _display->display->drawRect(barX, barY, barW, barH, SSD1306_WHITE);
            if (fillW > 0)
            {
                _display->display->fillRect(barX, barY, fillW, barH, SSD1306_WHITE);
            }
            break;
        case 5:
        {
            // Subtext with remaining seconds. Hold -> "weiter halten · loslassen = Abbruch",
            // Auto -> "◀ Abbrechen" (the arrow is drawn as "<" for the SSD1306 font).
            _display->display->setTextColor(SSD1306_WHITE);
            _display->display->setTextWrap(false);
            _display->display->setTextSize(1);
            _display->display->setCursor(0, 54);
            if (_mode == GestureOverlayMode::Hold)
            {
                // Mock renderProg(hold): "weiter halten . loslassen = Abbruch . <N> s"
                // (middot rendered as ASCII "-" separator; SSD1306 font has no middot).
                _display->display->print("weiter halten - loslassen = Abbruch - ");
            }
            else
            {
                // Auto branch: "< Abbrechen - <N> s" (leading "<" stands in for the mock arrow).
                _display->display->print("< Abbrechen - ");
            }
            _display->display->print(static_cast<int>(_lastSeconds));
            _display->display->print(" s");
            break;
        }
        case 6:
            _display->displayBuff();
            // fall through
        default:
            _drawStep = 0; // completed
            break;
    }
}
#endif // DEVICE_DISPLAY_MODULE
