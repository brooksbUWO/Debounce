// ****************************************************************************
// Title        : Debounce16 Library
// File Name    : 'Debounce16.cpp'
// Target MCU   : Espressif ESP32 (Doit DevKit Version 1)
// Description  : Implementation of 16-bit pattern-based button debouncing
//
// Revision History:
// When         Who         Description of change
// -----------  ----------- -----------------------
// 30-SEP-2025  Brooks      Initial implementation
// 06-APR-2026  Brooks      Fix C2 (private edge helpers), C4 (missing break),
//                          uint32_t timing, remove dead states, // --- comments
//
// ****************************************************************************

// Include Files
// ****************************************************************************
#include "Debounce16.h"

// ---
// Debounce -- constructor. Initializes all members; configures GPIO pin.
// Params: pin       -- GPIO pin number
//         activeLevel -- HIGH (active HIGH, INPUT) or LOW (active LOW, INPUT_PULLUP)
// ---
Debounce::Debounce(uint8_t pin, bool activeLevel)
{
    pinButton    = pin;                         // Store GPIO pin number
    levelActive  = activeLevel;                 // Store active logic level
    historyButton = PATTERN_UP;                 // Initialize history to UP state
    stateButton  = ButtonState::STATE_IDLE;     // Start in IDLE state

    timeEvent = 0;                              // Reset event timestamp
    timePress = 0;                              // Reset press timestamp

    windowDoublePress  = 300;                   // Default: 300ms double-press window
    thresholdLongPress = 1000;                  // Default: 1000ms long-press threshold

    flagEnableDoublePress = false;              // Double-press disabled by default
    flagEnableLongPress   = false;              // Long-press disabled by default

    countClick          = 0;                    // Reset click counter
    flagSinglePressed   = false;                // Clear single-press flag
    flagDoublePressed   = false;                // Clear double-press flag
    flagLongPressed     = false;                // Clear long-press flag
    flagPressProcessed  = false;                // Clear press processed flag
    flagReleaseProcessed = false;               // Clear release processed flag

    callbackPress         = nullptr;            // No press callback
    callbackRelease       = nullptr;            // No release callback
    callbackDoublePress   = nullptr;            // No double-press callback
    callbackLongPressStart = nullptr;           // No long-press start callback
    callbackLongPressEnd  = nullptr;            // No long-press end callback

    if (levelActive == HIGH)
    {
        pinMode(pin, INPUT);                    // Active HIGH: external pull-down
    }
    else
    {
        pinMode(pin, INPUT_PULLUP);             // Active LOW: internal pull-up
    }
}

// ---
// update -- shift new reading into historyButton; run state machine if enabled.
//           Call once per millisecond. Safe to call from a hardware timer ISR.
// ---
void Debounce::update()
{
    bool currentState = readButtonRaw();        // Read current button state
    historyButton = (historyButton << 1) | currentState;  // Shift in new reading

    if (flagEnableDoublePress || flagEnableLongPress)
    {
        updateStateMachine();                   // Run state machine when features active
    }
}

// ---
// isPressed -- detect one press event per physical press.
//              Fires callbackPress if registered.
//              NOTE: uses flagPressProcessed to provide consume-once semantics.
//              The internal state machine uses detectPressEdge() and does NOT
//              interfere with flagPressProcessed.
// Returns: true once when press pattern first matches; false until next press.
// ---
bool Debounce::isPressed()
{
    bool result = false;

    if ((historyButton & MASK_PRESS) == PATTERN_PRESS && !flagPressProcessed)
    {
        result = true;
        flagPressProcessed = true;
        triggerCallback(callbackPress);
    }
    else if ((historyButton & MASK_PRESS) != PATTERN_PRESS)
    {
        flagPressProcessed = false;             // Reset when pattern no longer matches
    }

    return result;
}

// ---
// isReleased -- detect one release event per physical release.
//               Fires callbackRelease if registered.
//               NOTE: uses flagReleaseProcessed for consume-once semantics.
//               The internal state machine uses detectReleaseEdge() and does NOT
//               interfere with flagReleaseProcessed.
// Returns: true once when release pattern first matches; false until next release.
// ---
bool Debounce::isReleased()
{
    bool result = false;

    if ((historyButton & MASK_RELEASE) == PATTERN_RELEASE && !flagReleaseProcessed)
    {
        result = true;
        flagReleaseProcessed = true;
        triggerCallback(callbackRelease);
    }
    else if ((historyButton & MASK_RELEASE) != PATTERN_RELEASE)
    {
        flagReleaseProcessed = false;           // Reset when pattern no longer matches
    }

    return result;
}

// ---
// isDown -- check if button is continuously held.
// Returns: true only when all 16 history bits are HIGH.
// ---
bool Debounce::isDown()
{
    return (historyButton == PATTERN_DOWN);
}

// ---
// isUp -- check if button is continuously released.
// Returns: true only when all 16 history bits are LOW.
// ---
bool Debounce::isUp()
{
    return (historyButton == PATTERN_UP);
}

// ---
// enableDoublePressDetection -- enable or disable double-press state machine.
// Params: enable -- true to enable, false to disable
// ---
void Debounce::enableDoublePressDetection(bool enable)
{
    flagEnableDoublePress = enable;

    if (!enable)
    {
        countClick        = 0;                  // Clear click counter on disable
        flagDoublePressed = false;              // Clear double-press flag
        flagSinglePressed = false;              // Clear single-press flag
    }
}

// ---
// setDoublePressWindow -- set the time window for double-press detection.
// Params: windowMs -- window duration in milliseconds
// ---
void Debounce::setDoublePressWindow(uint16_t windowMs)
{
    windowDoublePress = windowMs;
}

// ---
// enableLongPressDetection -- enable or disable long-press state machine.
// Params: enable -- true to enable, false to disable
// ---
void Debounce::enableLongPressDetection(bool enable)
{
    flagEnableLongPress = enable;

    if (!enable)
    {
        flagLongPressed = false;                // Clear long-press flag on disable
    }
}

// ---
// setLongPressThreshold -- set the hold duration required to confirm a long press.
// Params: thresholdMs -- threshold duration in milliseconds
// ---
void Debounce::setLongPressThreshold(uint16_t thresholdMs)
{
    thresholdLongPress = thresholdMs;
}

// ---
// isSinglePressed -- check if a single-press event has been confirmed.
//                    Consume-once: clears the flag on read.
// Returns: true once after the double-press window expires with one click.
// ---
bool Debounce::isSinglePressed()
{
    bool result = flagSinglePressed;

    if (result)
    {
        flagSinglePressed = false;              // Clear flag after reading
    }

    return result;
}

// ---
// isDoublePressed -- check if a double-press event has been confirmed.
//                    Consume-once: clears the flag on read.
// Returns: true once after two presses within the detection window.
// ---
bool Debounce::isDoublePressed()
{
    bool result = flagDoublePressed;

    if (result)
    {
        flagDoublePressed = false;              // Clear flag after reading
    }

    return result;
}

// ---
// isLongPressed -- check if a long press is currently active.
//                  NOT consume-once: returns true on every read while held.
// Returns: true while button is held past the long-press threshold.
// ---
bool Debounce::isLongPressed()
{
    return flagLongPressed;
}

// ---
// getClickCount -- return the current click count within the detection window.
//                  Resets to 0 when state returns to IDLE.
// Returns: number of clicks counted; 0 after the window expires.
// ---
uint8_t Debounce::getClickCount()
{
    uint8_t result = countClick;

    if (countClick > 0 && stateButton == ButtonState::STATE_IDLE)
    {
        countClick = 0;                         // Reset count after reading in IDLE
    }

    return result;
}

// ---
// onPress -- register callback for press events.
// Params: callback -- function pointer; nullptr removes the callback
// ---
void Debounce::onPress(void (*callback)())
{
    callbackPress = callback;
}

// ---
// onRelease -- register callback for release events.
// Params: callback -- function pointer; nullptr removes the callback
// ---
void Debounce::onRelease(void (*callback)())
{
    callbackRelease = callback;
}

// ---
// onDoublePress -- register callback for double-press events.
//                  Fires automatically from update(); no polling required.
// Params: callback -- function pointer; nullptr removes the callback
// ---
void Debounce::onDoublePress(void (*callback)())
{
    callbackDoublePress = callback;
}

// ---
// onLongPressStart -- register callback for long-press start events.
//                     Fires automatically from update(); no polling required.
// Params: callback -- function pointer; nullptr removes the callback
// ---
void Debounce::onLongPressStart(void (*callback)())
{
    callbackLongPressStart = callback;
}

// ---
// onLongPressEnd -- register callback for long-press end events.
//                   Fires automatically from update(); no polling required.
// Params: callback -- function pointer; nullptr removes the callback
// ---
void Debounce::onLongPressEnd(void (*callback)())
{
    callbackLongPressEnd = callback;
}

// ---
// readButtonRaw -- read physical pin state, accounting for active logic level.
// Returns: true if button is pressed; false if released.
// ---
bool Debounce::readButtonRaw()
{
    bool stateRaw = digitalRead(pinButton);

    if (levelActive == HIGH)
    {
        return stateRaw;                        // Active HIGH: return as-is
    }
    else
    {
        return !stateRaw;                       // Active LOW: invert
    }
}

// ---
// detectPressEdge -- check if the press pattern is present in historyButton.
//                    No side effects: does not touch flagPressProcessed and does
//                    not fire any callbacks. Used internally by the state machine
//                    so that isPressed() remains available to user code.
// Returns: true when (historyButton & MASK_PRESS) == PATTERN_PRESS.
// ---
bool Debounce::detectPressEdge()
{
    return (historyButton & MASK_PRESS) == PATTERN_PRESS;
}

// ---
// detectReleaseEdge -- check if the release pattern is present in historyButton.
//                      No side effects: does not touch flagReleaseProcessed and
//                      does not fire any callbacks. Used internally by the state
//                      machine so that isReleased() remains available to user code.
// Returns: true when (historyButton & MASK_RELEASE) == PATTERN_RELEASE.
// ---
bool Debounce::detectReleaseEdge()
{
    return (historyButton & MASK_RELEASE) == PATTERN_RELEASE;
}

// ---
// updateStateMachine -- process state transitions for press pattern detection.
//                       Called from update() when advanced features are enabled.
//                       Uses detectPressEdge() and detectReleaseEdge() instead of
//                       isPressed()/isReleased() to avoid consuming user-visible events.
// ---
void Debounce::updateStateMachine()
{
    uint32_t currentTime = millis();
    uint32_t timeElapsed;

    switch (stateButton)
    {
        case ButtonState::STATE_IDLE:
            if (detectPressEdge())              // No side effects on flagPressProcessed
            {
                timePress   = currentTime;      // Record press start timestamp
                countClick  = 1;                // First click
                stateButton = ButtonState::STATE_PRESS_FIRST;
            }
            break;

        case ButtonState::STATE_PRESS_FIRST:
            if (flagEnableLongPress)
            {
                timeElapsed = currentTime - timePress;

                if (timeElapsed >= thresholdLongPress && !flagLongPressed)
                {
                    flagLongPressed = true;
                    stateButton     = ButtonState::STATE_LONG_PRESS_ACTIVE;
                    triggerCallback(callbackLongPressStart);
                    break;                      // Do not evaluate release in same tick (C4 fix)
                }
            }

            if (detectReleaseEdge())            // No side effects on flagReleaseProcessed
            {
                timeEvent = currentTime;        // Record release timestamp

                if (flagLongPressed)
                {
                    resetState();               // Long press already handled; return to idle
                }
                else if (flagEnableDoublePress)
                {
                    stateButton = ButtonState::STATE_RELEASE_FIRST;
                }
                else
                {
                    resetState();
                }
            }
            break;

        case ButtonState::STATE_RELEASE_FIRST:
            timeElapsed = currentTime - timeEvent;

            if (detectPressEdge() && timeElapsed < windowDoublePress)
            {
                // Second press within window: double press confirmed
                resetState();                   // resetState clears flagDoublePressed
                flagDoublePressed = true;       // Set flag AFTER reset
                triggerCallback(callbackDoublePress);
            }
            else if (timeElapsed >= windowDoublePress)
            {
                // Window expired with one press: single press confirmed
                flagSinglePressed = true;
                resetState();
            }
            break;

        case ButtonState::STATE_LONG_PRESS_ACTIVE:
            if (detectReleaseEdge())            // No side effects on flagReleaseProcessed
            {
                flagLongPressed = false;
                triggerCallback(callbackLongPressEnd);
                resetState();
            }
            break;
    }
}

/**
 * @anchor ADR004
 * @brief ADR 004: flagSinglePressed Excluded from resetState()
 * @details
 *     Status: ACCEPTED
 *
 *     Context: resetState() is called at the end of every successful state
 *     transition: after a double press is confirmed, after a long press is
 *     released, and (in the single-press path) after the double-press window
 *     expires. If resetState() cleared flagSinglePressed, the following sequence
 *     would silently drop the event:
 *       1. Window expires -> flagSinglePressed = true, then resetState() called.
 *       2. resetState() clears flagSinglePressed.
 *       3. User calls isSinglePressed() in loop() -> returns false.
 *     The single-press confirmation and the state reset happen within the same
 *     call to update(). The flag must survive the reset.
 *
 *     Decision: resetState() clears stateButton, countClick, flagDoublePressed,
 *     and flagLongPressed. It does NOT clear flagSinglePressed.
 *     flagSinglePressed is cleared only by isSinglePressed() after the user reads
 *     it (consume-once semantics, see @ref ADR003).
 *
 *     Consequences:
 *     - Positive: isSinglePressed() reliably returns true in the loop() iteration
 *       immediately after the double-press window expires, regardless of when
 *       in the update() call the reset occurs.
 *     - Neutral: flagDoublePressed is not subject to the same issue because it is
 *       set AFTER resetState() is called in the double-press confirmation path.
 */

// ---
// resetState -- return state machine to STATE_IDLE and clear transient flags.
//               NOTE: flagSinglePressed is intentionally NOT cleared here.
//               It must survive the reset so user code can read it in loop().
// ---
void Debounce::resetState()
{
    stateButton       = ButtonState::STATE_IDLE;
    countClick        = 0;
    flagDoublePressed = false;
    flagLongPressed   = false;
}

// ---
// triggerCallback -- execute a callback function pointer if it is not nullptr.
// Params: callback -- function pointer to call
// ---
void Debounce::triggerCallback(void (*callback)())
{
    if (callback != nullptr)
    {
        callback();
    }
}
