#ifdef DEVICE_DISPLAY_MODULE
/**
 * @file        GestureEngine.cpp
 * @brief       Hold-to-confirm gesture state machine + Home key map (GEST-02/03/04)
 * @details     Non-blocking implementation. tick(now) drives the PreRoll -> Counting ->
 *              Firing progression; endHold() before Firing yields Aborted (no action).
 *              Only a single gesture is ever active. resolveAction() maps a physical
 *              button to a GestureAction using the Home-screen key map (OK is fixed to
 *              ProgMode). On the Counting -> Firing transition fireAction() invokes exactly
 *              the callback matching the action once (GEST-04); Prog/Pause then dwell in
 *              Done for GESTURE_DONE_MS before returning to Idle (Reboot restarts instead).
 * @version     0.0.3
 * @date        2026-07-09
 * @copyright   Copyright (c) 2026, Erkan Çolak
 *              Licensed under GNU GPL v3.0
 **/

    #include "GestureEngine.h"
    #include <cmath> // ceilf

// --- Hold-to-confirm control (GEST-02) ------------------------------------------------

void GestureEngine::startHold(GestureAction action)
{
    // Only ONE active gesture: ignore re-entrant starts while a gesture is live.
    if (_holding || _phase != GesturePhase::Idle) return;
    if (action == GestureAction::None) return;

    _action = action;
    _phase = GesturePhase::PreRoll;
    _holding = true;
    _startPending = true; // seed _holdStart on the next tick() so timing uses one time base
    _holdStart = 0;
    _elapsed = 0;
    _fired = false; // fresh gesture: the action has not fired yet (GEST-04)
    _firedAt = 0;
}

void GestureEngine::endHold()
{
    if (!_holding) return;
    _holding = false;

    // Released before the action fired -> abort, no action is triggered.
    if (_phase == GesturePhase::PreRoll || _phase == GesturePhase::Counting)
    {
        _phase = GesturePhase::Aborted;
        _action = GestureAction::None;
    }
    // If already Firing/Done, the release does not undo the (single) fired action.
}

void GestureEngine::tick(uint32_t now)
{
    // Seed the hold start on the first tick after startHold() so PreRoll/Counting share
    // the exact same time base as the caller's loop.
    if (_startPending)
    {
        _holdStart = now;
        _startPending = false;
    }

    switch (_phase)
    {
        case GesturePhase::PreRoll:
        {
            _elapsed = now - _holdStart; // millis() wraps cleanly for unsigned subtraction
            if (_elapsed >= HOLD_PHASE1_MS)
            {
                _phase = GesturePhase::Counting;
            }
            break;
        }
        case GesturePhase::Counting:
        {
            _elapsed = now - _holdStart;
            // Bar empty -> fire exactly once. Firing is latched for the caller to observe.
            if (_elapsed >= HOLD_PHASE1_MS + GESTURE_BAR_MS)
            {
                _phase = GesturePhase::Firing;
                _firedAt = now;
                fireAction(); // GEST-04: dispatch the matching callback exactly once
            }
            break;
        }
        case GesturePhase::Firing:
        {
            // Prog/Pause dwell in the Done acknowledgement phase for GESTURE_DONE_MS, then
            // return to Idle. Reboot fires and stays in Firing: the callback restarts the
            // device, so there is nothing left to advance to (no Done for Reboot).
            if (_action != GestureAction::Reboot &&
                (now - _firedAt) >= GESTURE_DONE_MS)
            {
                _phase = GesturePhase::Done;
            }
            break;
        }
        case GesturePhase::Done:
        {
            // Acknowledgement shown; fold back to Idle so a new gesture can arm.
            _phase = GesturePhase::Idle;
            _action = GestureAction::None;
            break;
        }
        case GesturePhase::Aborted:
            // #15 fix: fold the aborted gesture back to Idle on the next tick so a new hold can
            // arm. startHold() only re-arms from Idle; without this the engine stays stuck in
            // Aborted after ANY released-early gesture and no further gesture ever works.
            _phase = GesturePhase::Idle;
            _action = GestureAction::None;
            break;
        case GesturePhase::Idle:
        default:
            // No time-based transitions from the resting state.
            break;
    }
}

void GestureEngine::reset()
{
    _phase = GesturePhase::Idle;
    _action = GestureAction::None;
    _holding = false;
    _startPending = false;
    _holdStart = 0;
    _elapsed = 0;
    _fired = false;
    _firedAt = 0;
    // Note: the registered callbacks are intentionally preserved across reset() so the
    // engine stays wired after clearing a completed/aborted gesture.
}

// --- Getters (GEST-02) ----------------------------------------------------------------

GesturePhase GestureEngine::getPhase() const
{
    return _phase;
}

GestureAction GestureEngine::getCurrentAction() const
{
    return _action;
}

float GestureEngine::barFractionFor(uint32_t elapsed)
{
    // Before the confirm bar starts the fraction is full (1.0).
    if (elapsed <= HOLD_PHASE1_MS) return 1.0f;

    const uint32_t barElapsed = elapsed - HOLD_PHASE1_MS;
    if (barElapsed >= GESTURE_BAR_MS) return 0.0f;

    // Linear 1..0 ramp over GESTURE_BAR_MS.
    return 1.0f - static_cast<float>(barElapsed) / static_cast<float>(GESTURE_BAR_MS);
}

float GestureEngine::getBarFraction() const
{
    switch (_phase)
    {
        case GesturePhase::PreRoll:
            return 1.0f; // bar not started yet
        case GesturePhase::Counting:
            return barFractionFor(_elapsed);
        case GesturePhase::Firing:
        case GesturePhase::Done:
            return 0.0f; // bar fully consumed
        case GesturePhase::Aborted:
        case GesturePhase::Idle:
        default:
            return 0.0f;
    }
}

uint8_t GestureEngine::getRemainingSeconds() const
{
    const float frac = getBarFraction();
    if (frac <= 0.0f) return 0; // empty bar: no countdown left

    // ceil(frac * 3); a still-running bar never shows 0 (mockup: Math.max(1, ...)).
    uint8_t sec = static_cast<uint8_t>(ceilf(frac * 3.0f));
    if (sec < 1) sec = 1;
    if (sec > 3) sec = 3;
    return sec;
}

// --- Key map (GEST-03) ----------------------------------------------------------------

void GestureEngine::setKeyMap(const GestureKeyMap &keyMap)
{
    _keyMap = keyMap;
}

const GestureKeyMap &GestureEngine::getKeyMap() const
{
    return _keyMap;
}

GestureAction GestureEngine::resolveAction(ButtonType button, bool isHomeScreen) const
{
    // OK/SELECT always toggles ProgMode, on any screen.
    if (button == ButtonType::SELECT) return GestureAction::ProgMode;

    // Directional buttons only carry a gesture while the Home screen is shown.
    if (!isHomeScreen) return GestureAction::None;

    switch (button)
    {
        case ButtonType::UP:
            return _keyMap.up;
        case ButtonType::DOWN:
            return _keyMap.down;
        case ButtonType::LEFT:
            return _keyMap.left;
        case ButtonType::RIGHT:
            return _keyMap.right;
        default:
            return GestureAction::None;
    }
}

// --- Action-execution callbacks (GEST-04) ---------------------------------------------

void GestureEngine::setOnProgToggle(ActionCallback cb)
{
    _onProgToggle = cb;
}

void GestureEngine::setOnReboot(ActionCallback cb)
{
    _onReboot = cb;
}

void GestureEngine::setOnPauseToggle(ActionCallback cb)
{
    _onPauseToggle = cb;
}

void GestureEngine::setOnDisplayOff(ActionCallback cb)
{
    _onDisplayOff = cb;
}

void GestureEngine::fireAction()
{
    // Fire exactly once per gesture. Guarding here (rather than only at the call site) makes
    // the invariant local and robust against future call sites.
    if (_fired) return;
    _fired = true;

    // Dispatch strictly the callback matching the current action. An unset callback is a
    // no-op (the FSM still advances through Firing/Done). The callbacks must be non-blocking;
    // this method does not block or wait for them.
    switch (_action)
    {
        case GestureAction::ProgMode:
            if (_onProgToggle) _onProgToggle();
            break;
        case GestureAction::Reboot:
            if (_onReboot) _onReboot();
            break;
        case GestureAction::Pause:
            if (_onPauseToggle) _onPauseToggle();
            break;
        case GestureAction::DisplayOff:
            if (_onDisplayOff) _onDisplayOff();
            break;
        case GestureAction::None:
        default:
            break; // nothing to fire
    }
}

#endif // DEVICE_DISPLAY_MODULE
