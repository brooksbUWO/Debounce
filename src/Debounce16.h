// ****************************************************************************
// Title        : Debounce16 Library
// File Name    : 'Debounce16.h'
// Target MCU   : Espressif ESP32 (Doit DevKit Version 1)
// Description  : 16-bit pattern-based button debouncing with advanced press
//                pattern detection capabilities
//
// Based on:
// - Jack Ganssle's "A Guide to Debouncing"
//   https://www.ganssle.com/debouncing.pdf
//   https://www.ganssle.com/debouncing-pt2.htm
// - Elliot Williams' "Ultimate Debouncer" (Hackaday)
//   https://hackaday.com/2015/12/09/embed-with-elliot-debounce-your-noisy-buttons-part-i/
//   https://hackaday.com/2015/12/10/embed-with-elliot-debounce-your-noisy-buttons-part-ii/
//
// Revision History:
// When         Who         Description of change
// -----------  ----------- -----------------------
// 30-SEP-2025  Brooks      Initial implementation
// 06-APR-2026  Brooks      Fix volatile/ISR safety, private edge helpers,
//                          enum cleanup, Doxygen API documentation
//
// ****************************************************************************

#pragma once

// Include Files
// ****************************************************************************
#include <Arduino.h>

// Class Declaration
// ****************************************************************************
class Debounce
{
public:
    // Constructors
    // ****************************************************************************

    /**
     * @brief Construct a Debounce object.
     * @param pin GPIO pin connected to the button.
     * @param activeLevel Logic level when button is pressed: HIGH or LOW.
     *        Active LOW automatically configures the pin with INPUT_PULLUP.
     */
    Debounce(uint8_t pin, bool activeLevel = HIGH);

    // Core Debouncing Methods (Always Available)
    // ****************************************************************************

    /**
     * @brief Update the 16-bit debounce shift register.
     *        Must be called once per millisecond. Safe to call from a timer ISR.
     */
    void update();

    /**
     * @brief Detect a press event (LOW-to-HIGH transition after debounce).
     * @return true once per press; false until the next press event.
     *         Consume-once: resets automatically when the press pattern changes.
     * @note Fires the onPress() callback when this method returns true.
     * @note User code and the internal state machine use separate mechanisms.
     *       Calling this method will NOT interfere with the state machine.
     */
    bool isPressed();

    /**
     * @brief Detect a release event (HIGH-to-LOW transition after debounce).
     * @return true once per release; false until the next release event.
     *         Consume-once: resets automatically when the release pattern changes.
     * @note Fires the onRelease() callback when this method returns true.
     * @note User code and the internal state machine use separate mechanisms.
     *       Calling this method will NOT interfere with the state machine.
     */
    bool isReleased();

    /**
     * @brief Check if the button is currently held down.
     * @return true only when all 16 history bits are HIGH (stable 16ms hold).
     */
    bool isDown();

    /**
     * @brief Check if the button is currently released.
     * @return true only when all 16 history bits are LOW (stable 16ms release).
     */
    bool isUp();

    // Advanced Feature Configuration
    // ****************************************************************************
    void enableDoublePressDetection(bool enable = true);
    void setDoublePressWindow(uint16_t windowMs);
    void enableLongPressDetection(bool enable = true);
    void setLongPressThreshold(uint16_t thresholdMs);

    // Advanced Feature Query Methods
    // ****************************************************************************

    /**
     * @brief Check if a single-press event has been confirmed.
     * @return true once after the double-press window expires with exactly one click.
     *         Consume-once: clears to false after the first read.
     * @note Requires enableDoublePressDetection() to be called first.
     *       Returns false if double-press detection is not enabled.
     */
    bool isSinglePressed();

    /**
     * @brief Check if a double-press event has been confirmed.
     * @return true once after two presses are detected within the window.
     *         Consume-once: clears to false after the first read.
     * @note Requires enableDoublePressDetection() to be called first.
     */
    bool isDoublePressed();

    /**
     * @brief Check if a long press is currently active.
     * @return true while the button is held past the long-press threshold.
     *         NOT consume-once: returns true on every read until the button releases.
     * @note Requires enableLongPressDetection() to be called first.
     *       Returns false if long-press detection is not enabled.
     */
    bool isLongPressed();

    /**
     * @brief Get the current click count within the double-press detection window.
     * @return Number of clicks counted; resets to 0 when state returns to IDLE.
     * @deprecated Prefer isSinglePressed() and isDoublePressed() which provide
     *             correct consume-once semantics and avoid race conditions.
     */
    uint8_t getClickCount();

    // Callback Registration (Optional)
    // ****************************************************************************

    /**
     * @brief Register a callback for press events.
     * @param callback Function pointer called when isPressed() returns true.
     * @warning The callback fires only when user code calls isPressed() explicitly.
     *          It does NOT fire automatically from update(). If you register
     *          onPress() but never call isPressed() in loop(), the callback
     *          will never execute.
     */
    void onPress(void (*callback)());

    /**
     * @brief Register a callback for release events.
     * @param callback Function pointer called when isReleased() returns true.
     * @warning Same polling requirement as onPress(). Must call isReleased()
     *          from loop() for the callback to fire.
     */
    void onRelease(void (*callback)());

    /**
     * @brief Register a callback for double-press events.
     * @param callback Function pointer called when a double press is confirmed.
     *        Fires automatically from update(). No polling required.
     * @note Requires enableDoublePressDetection() to be called first.
     */
    void onDoublePress(void (*callback)());

    /**
     * @brief Register a callback for long-press start events.
     * @param callback Function pointer called when the long-press threshold is reached.
     *        Fires automatically from update(). No polling required.
     * @note Requires enableLongPressDetection() to be called first.
     */
    void onLongPressStart(void (*callback)());

    /**
     * @brief Register a callback for long-press end events.
     * @param callback Function pointer called when a long press is released.
     *        Fires automatically from update(). No polling required.
     * @note Requires enableLongPressDetection() to be called first.
     */
    void onLongPressEnd(void (*callback)());

private:
    // Core Debouncing Members
    // ****************************************************************************
    uint16_t historyButton;                 // 16-bit button state history shift register
    uint8_t  pinButton;                     // GPIO pin number
    bool     levelActive;                   // Active logic level (HIGH or LOW)

    // State Machine for Press Pattern Detection
    // ****************************************************************************
    enum class ButtonState : uint8_t
    {
        STATE_IDLE,                         // Waiting for input
        STATE_PRESS_FIRST,                  // First press detected
        STATE_RELEASE_FIRST,                // First release detected, awaiting second
        STATE_LONG_PRESS_ACTIVE             // Long press threshold crossed
    };
    volatile ButtonState stateButton;       // Current state machine state (ISR-shared)

    // Timing Management
    // ****************************************************************************
    volatile uint32_t timeEvent;            // Timestamp of last state event (ms, ISR-shared)
    volatile uint32_t timePress;            // Timestamp of press start (ms, ISR-shared)

    /**
     * @anchor ADR002
     * @brief ADR 002: Advanced Features Disabled by Default
     * @details
     *     Status: ACCEPTED
     *
     *     Context: Double-press and long-press detection require a state machine
     *     that runs inside update(). When both features are disabled, update()
     *     only maintains the shift register -- no branching, no timing, minimal
     *     overhead. Enabling both features when only basic debounce is needed
     *     wastes cycles and adds latency.
     *
     *     Decision: flagEnableDoublePress and flagEnableLongPress both default to
     *     false. The state machine is only entered when at least one feature is
     *     enabled. Users opt in by calling enableDoublePressDetection() or
     *     enableLongPressDetection() during setup().
     *
     *     Consequences:
     *     - Positive: Zero state machine overhead for basic debounce use cases.
     *     - Positive: isSinglePressed() and isLongPressed() always return false
     *       without explicit opt-in, preventing accidental false positives.
     *     - Negative: Users who expect advanced features to work without
     *       configuration will see no events until they call the enable methods.
     */
    // Feature Configuration (written only from setup(); not ISR-shared)
    // ****************************************************************************
    bool     flagEnableDoublePress;         // Double-press detection enabled
    bool     flagEnableLongPress;           // Long-press detection enabled
    uint16_t windowDoublePress;             // Double-press time window (ms, default 300)
    uint16_t thresholdLongPress;            // Long-press time threshold (ms, default 1000)

    /**
     * @anchor ADR003
     * @brief ADR 003: Consume-Once Semantics for One-Shot Events
     * @details
     *     Status: ACCEPTED
     *
     *     Context: isSinglePressed() and isDoublePressed() represent discrete
     *     events (a confirmed tap or double-tap). If the flag is not cleared on
     *     read, calling the method multiple times in a single loop() iteration
     *     returns true on every call, causing the event to be handled more than
     *     once (e.g., toggling an LED twice for one tap).
     *
     *     Decision: isSinglePressed() and isDoublePressed() clear their respective
     *     flags (flagSinglePressed, flagDoublePressed) after returning true.
     *     Subsequent calls within the same loop() iteration return false.
     *     isLongPressed() is intentionally NOT consume-once because long press is
     *     a continuous state, not a one-shot event: it is expected to return true
     *     on every call while the button is held.
     *
     *     Consequences:
     *     - Positive: Each discrete press event is delivered exactly once per
     *       physical action, matching typical UI interaction expectations.
     *     - Negative: Calling isSinglePressed() twice in the same loop() iteration
     *       (e.g., in two different conditional branches) will miss the event in
     *       the second branch. Read the result into a local variable if it must
     *       be tested more than once.
     */
    /**
     * @anchor ADR005
     * @brief ADR 005: ISR-Shared Variables Declared volatile
     * @details
     *     Status: ACCEPTED
     *
     *     Context: update() is designed to be called from a hardware timer ISR
     *     (see DebounceSimpleInterrupt.ino). Any member written by update() and
     *     read by loop() is shared between two execution contexts: the ISR and the
     *     main thread. Without volatile, the compiler may cache shared variables
     *     in CPU registers and the main thread will read stale values.
     *
     *     Decision: All members written inside update() or updateStateMachine() are
     *     declared volatile: stateButton, timeEvent, timePress, countClick,
     *     flagSinglePressed, flagDoublePressed, flagLongPressed, flagPressProcessed,
     *     flagReleaseProcessed. Per the EGRT 390 style guide, the flag prefix
     *     convention (flagXxx) is RESERVED for ISR-shared event markers and always
     *     implies volatile.
     *
     *     Consequences:
     *     - Positive: The compiler cannot cache ISR-written values in registers;
     *       loop() always reads the latest value from memory.
     *     - Negative: volatile prevents certain compiler optimizations. For
     *       critical-section operations (e.g., reading a multi-byte value atomically)
     *       interrupts must be disabled manually; volatile alone is not sufficient.
     *     - Neutral: flagEnableDoublePress, flagEnableLongPress, windowDoublePress,
     *       and thresholdLongPress are NOT volatile because they are written only
     *       during setup() before the timer ISR is enabled.
     */
    // Event Tracking (written in ISR context via update(); must be volatile)
    // ****************************************************************************
    volatile uint8_t countClick;            // Click counter for current press sequence
    volatile bool flagSinglePressed;        // Single-press confirmed flag (consume-once)
    volatile bool flagDoublePressed;        // Double-press confirmed flag (consume-once)
    volatile bool flagLongPressed;          // Long press active flag (continuous)
    volatile bool flagPressProcessed;       // Prevents duplicate press events per cycle
    volatile bool flagReleaseProcessed;     // Prevents duplicate release events per cycle

    // Callback Function Pointers (Optional)
    // ****************************************************************************
    void (*callbackPress)();                // Press callback (fires from isPressed())
    void (*callbackRelease)();              // Release callback (fires from isReleased())
    void (*callbackDoublePress)();          // Double-press callback (fires from update())
    void (*callbackLongPressStart)();       // Long-press start callback (fires from update())
    void (*callbackLongPressEnd)();         // Long-press end callback (fires from update())

    /**
     * @anchor ADR001
     * @brief ADR 001: 16-Bit Shift Register Debounce Algorithm
     * @details
     *     Status: ACCEPTED
     *
     *     Context: Button contacts bounce for 1-10 ms after press or release,
     *     producing multiple transitions. A naive level check fires multiple
     *     events per physical press. Jack Ganssle and Elliot Williams independently
     *     describe a shift-register pattern-matching approach that accumulates N
     *     consecutive samples before declaring a stable state.
     *
     *     Decision: historyButton is a uint16_t shift register. update() shifts
     *     left by one and ORs in the current reading on each 1 ms call. Press is
     *     detected when the 6 LSBs are all HIGH (PATTERN_PRESS = 0x003F), meaning
     *     6 consecutive pressed readings. Release is detected by
     *     (historyButton & MASK_RELEASE) == PATTERN_RELEASE: bits 15-10 are HIGH
     *     (confirming a prior press) and bits 5-0 are LOW (6 consecutive releases),
     *     with bits 9-6 masked as don't-care to handle held-then-released sequences.
     *
     *     Consequences:
     *     - Positive: 6 ms debounce window eliminates typical mechanical bounce.
     *     - Positive: No dynamic memory allocation; the shift register is a fixed
     *       uint16_t member.
     *     - Negative: update() must be called at exactly 1 ms intervals; timing
     *       jitter degrades debounce accuracy proportionally.
     *     - Neutral: The 16-bit width limits history to 16 ms; patterns requiring
     *       longer histories require a redesigned data structure.
     *
     * @see J. Ganssle, "A Guide to Debouncing," 2004.
     *      https://www.ganssle.com/debouncing.pdf
     * @see E. Williams, "Embed with Elliot: Debounce Your Noisy Buttons (Part II),"
     *      Hackaday, Dec. 2015.
     *      https://hackaday.com/2015/12/10/embed-with-elliot-debounce-your-noisy-buttons-part-ii/
     */
    // Bit Pattern Constants (16-bit patterns for historyButton matching)
    // ****************************************************************************
    static const uint16_t MASK_PRESS      = 0x003F; // Bits 5-0: press detection mask
    static const uint16_t PATTERN_PRESS   = 0x003F; // 0b0000000000111111: 6 cons. HIGHs
    static const uint16_t MASK_RELEASE    = 0xFC3F; // Bits 15-10 + 5-0; bits 9-6 don't-care
    static const uint16_t PATTERN_RELEASE = 0xFC00; // 0b1111110000000000: 6 prior HIGHs, 6 LOWs
    static const uint16_t PATTERN_DOWN    = 0xFFFF; // 0b1111111111111111: stable held
    static const uint16_t PATTERN_UP      = 0x0000; // 0b0000000000000000: stable released

    /**
     * @anchor ADR006
     * @brief ADR 006: State Machine Uses Private Edge Helpers, Not Public API
     * @details
     *     Status: ACCEPTED
     *
     *     Context: The original updateStateMachine() called the public isPressed()
     *     and isReleased() methods internally to detect button edges. Those methods
     *     have side effects: they set flagPressProcessed or flagReleaseProcessed to
     *     true and fire the onPress/onRelease callbacks. Because the state machine
     *     runs inside update() (before user code runs in loop()), the side effects
     *     occurred before the user had a chance to call isPressed()/isReleased().
     *     The user's subsequent call found the processed flag already set and
     *     received false -- the event was silently consumed.
     *
     *     Decision: Two private methods are introduced:
     *     - detectPressEdge(): returns (historyButton & MASK_PRESS) == PATTERN_PRESS
     *       with no side effects on any flags.
     *     - detectReleaseEdge(): returns (historyButton & MASK_RELEASE) == PATTERN_RELEASE
     *       with no side effects on any flags.
     *     updateStateMachine() uses these private helpers exclusively. The public
     *     isPressed() and isReleased() methods retain full side effects and are
     *     reserved for user code.
     *
     *     Consequences:
     *     - Positive: isPressed() and isReleased() remain available to user code
     *       even when advanced features (and therefore the state machine) are active.
     *     - Positive: The onPress/onRelease callbacks fire only when user code calls
     *       isPressed()/isReleased(), not from inside update(). This is consistent
     *       with the documented behavior.
     *     - Negative: Two additional private methods must be maintained. Their
     *       return expressions must match the corresponding patterns in
     *       isPressed()/isReleased() exactly.
     */
    // Private Helper Methods
    // ****************************************************************************

    bool readButtonRaw();                   // Read raw pin state, accounting for active level
    bool detectPressEdge();                 // Check press pattern; no side effects on flags
    bool detectReleaseEdge();               // Check release pattern; no side effects on flags
    void updateStateMachine();              // Run state machine logic
    void resetState();                      // Return state machine to STATE_IDLE
    void triggerCallback(void (*callback)()); // Call function pointer if not nullptr
};
