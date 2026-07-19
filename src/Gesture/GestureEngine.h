#ifdef DEVICE_DISPLAY_MODULE
    #pragma once
/**
 * @file        GestureEngine.h
 * @brief       Central data model and hold-to-confirm state machine for gesture handling
 * @details     Phase-1 (GEST-02/GEST-03): defines the GestureAction/GesturePhase enums,
 *              the central timing constants and the GestureEngine hold-to-confirm state
 *              machine (startHold/endHold/tick) plus the Home-screen key map that resolves
 *              a physical button to a GestureAction. The engine is non-blocking and only
 *              ever tracks a SINGLE active gesture at a time.
 *              Phase-2 (GEST-04): action execution is decoupled via std::function callbacks
 *              (setOnProgToggle/setOnReboot/setOnPauseToggle). When the FSM reaches Firing
 *              for an action, exactly the matching callback is invoked once (non-blocking);
 *              Prog/Pause then dwell in Done for ~DONE_MS before returning to Idle.
 *              Navigation coupling (GEST-05) and the auto gesture (GEST-09) are added in
 *              later phases; this file stays additive.
 * @version     0.0.3
 * @date        2026-07-09
 * @copyright   Copyright (c) 2026, Erkan Çolak
 *              Licensed under GNU GPL v3.0
 **/

    #include "../Devices/ButtonEvent.h" // ButtonType (UP/DOWN/SELECT/LEFT/RIGHT)
    #include <cstdint>
    #include <functional> // std::function (GEST-04 execution callbacks)

/**
 * @brief Action that a completed gesture triggers.
 * @note  Kept central here so that later phases (state machine, key map, overlay widget)
 *        share a single definition.
 */
enum class GestureAction : uint8_t
{
    None = 0,   // No action (e.g. released too early, or key not mapped)
    Pause,      // Pause the widget rotation
    Reboot,     // Reboot the device
    ProgMode,   // Toggle KNX programming mode
    DisplayOff, // Turn the display off immediately (#17); any button wakes it
    Screenshot  // Capture the framebuffer to SD (#value MUST match HomeKeyAction::Screenshot = 5)
};

/**
 * @brief Current phase of the hold-to-confirm gesture life cycle.
 * @note  Idle          - no gesture active
 *        PreRoll       - button held, but the confirm bar (HOLD_PHASE1_MS) has not started yet
 *        Counting      - confirm bar running down (GESTURE_BAR_MS window)
 *        Firing        - bar reached the end, action is being triggered
 *        Done          - action fired, short acknowledgement phase
 *        Aborted       - button released before Firing -> no action
 */
enum class GesturePhase : uint8_t
{
    Idle = 0,
    PreRoll,
    Counting,
    Firing,
    Done,
    Aborted
};

/**
 * @brief Central gesture timing constants.
 * @note  HOLD_PHASE1_MS - initial hold time before the confirm bar appears/starts.
 *        GESTURE_BAR_MS  - duration of the confirm bar (concept: 3 s).
 *                          The mockup value of 2400 ms (PROG_MS) is deliberately rejected
 *                          in favour of the concept's 3 s (see 40-umsetzung-todo.md, GEST-01).
 *        GESTURE_DONE_MS - acknowledgement dwell after firing Prog/Pause before returning
 *                          to Idle (mockup: progDoneStart + 1300 ms, see reg2-menu-mockup.html).
 *                          Reboot does not use this dwell (the device restarts).
 */
static constexpr uint32_t HOLD_PHASE1_MS = 1000;  // ms until the confirm bar starts
static constexpr uint32_t GESTURE_BAR_MS = 3000;  // ms confirm-bar duration (concept 3 s)
static constexpr uint32_t GESTURE_DONE_MS = 1300; // ms Done dwell after Prog/Pause (mockup 1.3 s)

/**
 * @brief Home-screen button-to-action mapping (GEST-03).
 * @details Maps each of the four directional buttons to a GestureAction. The mapping is
 *          only consulted while the Home screen is shown; OK/SELECT is hard-wired to
 *          ProgMode and NOT part of the key map. Defaults follow the mockup:
 *          Up = Pause, Down = Reboot, Left = None, Right = None.
 * @note    Persistence (GEST-08) is handled later by the DeviceDisplay flash blob.
 */
struct GestureKeyMap
{
    GestureAction up = GestureAction::Pause;          // Home: button Up
    GestureAction down = GestureAction::Reboot;       // Home: button Down
    GestureAction left = GestureAction::DisplayOff;   // Home: button Left-hold -> display off (#17)
    GestureAction right = GestureAction::Screenshot;  // Home: button Right-hold -> screenshot to SD
};

/**
 * @brief Hold-to-confirm gesture engine (GEST-02/GEST-03).
 * @details Non-blocking state machine driven by tick(now):
 *          - startHold(action) arms a gesture (only ONE at a time; re-entrant calls ignored).
 *          - tick(now) advances PreRoll -> Counting -> Firing based on the elapsed hold time.
 *          - endHold() releases the button: before Firing -> Aborted (no action).
 *          The confirm bar fraction is a linear 1..0 ramp over GESTURE_BAR_MS, starting
 *          after HOLD_PHASE1_MS. resolveAction() maps a physical button to a GestureAction.
 *          On entering Firing the engine invokes exactly the callback matching the action
 *          once (GEST-04); Prog/Pause then dwell in Done for GESTURE_DONE_MS before Idle.
 */
class GestureEngine
{
  public:
    GestureEngine() = default;

    // --- Hold-to-confirm control (GEST-02) --------------------------------------------

    /**
     * @brief Arm a hold-to-confirm gesture for the given action.
     * @param action The action the gesture would trigger once the bar completes.
     * @note  No-op when a gesture is already active (only ONE active gesture) or when
     *        action == None. Sets phase to PreRoll and records the hold start time on the
     *        next tick(). Non-blocking.
     */
    void startHold(GestureAction action);

    /**
     * @brief Release the currently held button.
     * @note  If the gesture has not reached Firing yet, it transitions to Aborted and no
     *        action is triggered. If it already fired, endHold() is a no-op with respect
     *        to the action. Non-blocking.
     */
    void endHold();

    /**
     * @brief Advance the gesture state machine.
     * @param now Current time base in milliseconds (e.g. millis()).
     * @details PreRoll  : until HOLD_PHASE1_MS elapsed, then -> Counting.
     *          Counting : while the confirm bar runs; at bar end -> Firing.
     *          Firing   : latched for one tick, action is considered triggered.
     *          Done/Aborted/Idle: no time-based transitions here.
     */
    void tick(uint32_t now);

    /**
     * @brief Reset the engine back to its idle state.
     */
    void reset();

    // --- Getters (GEST-02) ------------------------------------------------------------

    /** @brief Current gesture phase. */
    GesturePhase getPhase() const;

    /** @brief Action the current gesture would trigger. */
    GestureAction getCurrentAction() const;

    /**
     * @brief Linear confirm-bar fraction, 1.0 (full) .. 0.0 (empty).
     * @return 1 - (elapsed - HOLD_PHASE1_MS) / GESTURE_BAR_MS, clamped to [0,1].
     *         Returns 1.0 during PreRoll/before the bar starts and 0.0 from Firing onwards.
     */
    float getBarFraction() const;

    /**
     * @brief Remaining whole seconds shown next to the bar.
     * @return ceil(barFraction * 3), clamped so a running bar never shows 0 (min 1).
     *         Returns 0 only once the bar is fully empty (Firing/Done/Aborted/Idle).
     */
    uint8_t getRemainingSeconds() const;

    // --- Key map (GEST-03) ------------------------------------------------------------

    /** @brief Replace the Home-screen key map. */
    void setKeyMap(const GestureKeyMap &keyMap);

    /** @brief Read the current Home-screen key map. */
    const GestureKeyMap &getKeyMap() const;

    /**
     * @brief Resolve a physical button to the GestureAction it should arm.
     * @param button      The pressed button (ButtonType).
     * @param isHomeScreen True when the Home screen is currently shown.
     * @return OK/SELECT -> always ProgMode. UP/DOWN/LEFT/RIGHT -> the mapped action,
     *         but only while isHomeScreen; otherwise None.
     */
    GestureAction resolveAction(ButtonType button, bool isHomeScreen) const;

    // --- Action-execution callbacks (GEST-04) -----------------------------------------

    /** @brief Callback type for a decoupled gesture action (nullary, non-blocking). */
    using ActionCallback = std::function<void()>;

    /**
     * @brief Set the callback fired when a ProgMode gesture reaches Firing.
     * @note  Intended to wrap knx.toggleProgMode(); the engine does NOT activate the
     *        ProgMode widget itself (handleProgMode() mirrors knx.progMode()). The callback
     *        is invoked exactly once per gesture and must not block.
     */
    void setOnProgToggle(ActionCallback cb);

    /**
     * @brief Set the callback fired when a Reboot gesture reaches Firing.
     * @note  Intended to wrap openknx.common.restart(). Invoked exactly once; may not return
     *        (the device restarts). No Done dwell is scheduled for Reboot.
     */
    void setOnReboot(ActionCallback cb);

    /**
     * @brief Set the callback fired when a Pause gesture reaches Firing.
     * @note  Intended to toggle the widget-rotation pause. Invoked exactly once and must not
     *        block; the engine dwells in Done for GESTURE_DONE_MS afterwards.
     */
    void setOnPauseToggle(ActionCallback cb);

    /**
     * @brief Set the callback fired when a DisplayOff gesture reaches Firing (#17).
     * @note  Intended to turn the display off immediately (any button then wakes it). Invoked
     *        exactly once and must not block; the engine dwells in Done for GESTURE_DONE_MS.
     */
    void setOnDisplayOff(ActionCallback cb);

    /**
     * @brief Set the callback fired when a Screenshot gesture reaches Firing.
     * @note  Must only REQUEST a screenshot (latch a flag); the actual SD write runs from loop().
     *        A blocking file write in this callback would reboot the RP2040. Invoked once, non-blocking.
     */
    void setOnScreenshot(ActionCallback cb);

  private:
    GesturePhase _phase = GesturePhase::Idle;    // Current gesture phase
    GestureAction _action = GestureAction::None; // Action of the current gesture

    bool _holding = false;      // True while the button is physically held
    bool _startPending = false; // startHold() latched a fresh gesture, seed time on next tick
    uint32_t _holdStart = 0;    // millis() when the hold began (seeded on first tick)
    uint32_t _elapsed = 0;      // ms elapsed since _holdStart (updated in tick)

    GestureKeyMap _keyMap; // Home-screen button-to-action mapping (GEST-03)

    // Action-execution callbacks (GEST-04). Any of these may be empty (then Firing simply
    // advances without a side effect). They are invoked at most once per gesture.
    ActionCallback _onProgToggle;  // fired for GestureAction::ProgMode
    ActionCallback _onReboot;      // fired for GestureAction::Reboot
    ActionCallback _onPauseToggle; // fired for GestureAction::Pause
    ActionCallback _onDisplayOff;  // fired for GestureAction::DisplayOff (#17)
    ActionCallback _onScreenshot;  // fired for GestureAction::Screenshot

    bool _fired = false;   // Firing callback already dispatched for this gesture
    uint32_t _firedAt = 0; // millis() when the action fired (for the Done dwell)

    /** @brief Compute the linear bar fraction for a given elapsed time (1..0, clamped). */
    static float barFractionFor(uint32_t elapsed);

    /**
     * @brief Dispatch exactly the callback matching the current action (once).
     * @details Called on the Counting -> Firing transition. Guarded by _fired so the action
     *          can never fire twice. Non-blocking: the callback itself must not block.
     */
    void fireAction();
};

#endif // DEVICE_DISPLAY_MODULE
