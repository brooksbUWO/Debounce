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
// 28-FEB-2026  davidc      Updates to work under ESP-IDF without Arduino
// 28-FEB-2026  davidc      Update masks to make press/release/up/down consistent
//
// ****************************************************************************

#ifndef ARDUINO
#ifdef IDF_VER
#include "esp_timer.h"

uint32_t millis()
{
  return esp_timer_get_time() / 1000;
}
#endif
#endif

// Include Files
// ****************************************************************************
#include "Debounce16.h"

// Constructor Implementation
// ****************************************************************************
Debounce16::Debounce16(db_pin_t pin, bool activeLevel)
{
    pinButton = pin;                        // Store GPIO pin number
    levelActive = activeLevel;              // Store active logic level
    historyButton = PATTERN_UP;             // Initialize history to UP state
    stateButton = ButtonState::STATE_IDLE;  // Start in IDLE state

    // Initialize timing variables
    timeEvent = 0;                          // Reset event timestamp
    timePress = 0;                          // Reset press timestamp

    // Set default timing parameters
    windowDoublePress = 300;                // 300ms double-press window
    thresholdLongPress = 1000;              // 1000ms long-press threshold

    // Disable advanced features by default
    flagEnableDoublePress = false;          // Double-press disabled
    flagEnableLongPress = false;            // Long-press disabled

    // Initialize event tracking flags
    countClick = 0;                         // Reset click counter
    flagDoublePressed = false;              // Clear double-press flag
    flagLongPressed = false;                // Clear long-press flag
    flagPressProcessed = false;             // Clear press processed flag
    flagReleaseProcessed = false;           // Clear release processed flag

    // Initialize all callback pointers to nullptr
    callbackPress = nullptr;                // No press callback
    callbackRelease = nullptr;              // No release callback
    callbackDoublePress = nullptr;          // No double-press callback
    callbackLongPressStart = nullptr;       // No long-press start callback
    callbackLongPressEnd = nullptr;         // No long-press end callback

    // Configure GPIO pin
#ifdef ARDUINO
    if (levelActive == HIGH)
    {
        pinMode(pin, INPUT);                // Active HIGH: use INPUT mode
    }
    else
    {
        pinMode(pin, INPUT_PULLUP);         // Active LOW: use INPUT_PULLUP
    }
#elifdef IDF_VER
    gpio_config_t io_conf = {
      .pin_bit_mask = (1ULL << pin),
      .mode = GPIO_MODE_INPUT,
      .pull_up_en = (levelActive == DB_HIGH ? GPIO_PULLUP_DISABLE : GPIO_PULLUP_ENABLE),
      .pull_down_en = GPIO_PULLDOWN_DISABLE,
      .intr_type = GPIO_INTR_DISABLE
    };
    gpio_config(&io_conf);
#endif
}

// Core Debouncing Methods Implementation
// ****************************************************************************

// ****************************************************************************
// Function: update
// Purpose:  Update button state (must be called every 1ms)
// Parameters: None
// Returns:  None
// ****************************************************************************
void Debounce16::update()
{
    bool currentState = readButtonRaw();    // Read current button state
    historyButton = (historyButton << 1) | currentState;  // Shift and add state

    // Update state machine if advanced features enabled
    if (flagEnableDoublePress || flagEnableLongPress)
    {
        updateStateMachine();               // Process state transitions
    }
}

// ****************************************************************************
// Function: isPressed
// Purpose:  Detect button press event (transition from UP to DOWN)
// Parameters: None
// Returns:  true if press event detected, false otherwise
// ****************************************************************************
bool Debounce16::isPressed()
{
    bool result = false;                    // Default to no press

    // Check for press pattern
    if ((historyButton & MASK_PRESS) == PATTERN_PRESS && !flagPressProcessed)
    {
        result = true;                      // Press detected
        flagPressProcessed = true;          // Mark as processed

        // Trigger callback if registered
        triggerCallback(callbackPress);
    }
    else if ((historyButton & MASK_PRESS) != PATTERN_PRESS)
    {
        flagPressProcessed = false;         // Reset flag when pattern changes
    }

    return result;                          // Return press detection result
}

// ****************************************************************************
// Function: isReleased
// Purpose:  Detect button release event (transition from DOWN to UP)
// Parameters: None
// Returns:  true if release event detected, false otherwise
// ****************************************************************************
bool Debounce16::isReleased()
{
    bool result = false;                    // Default to no release

    // Check for release pattern
    if ((historyButton & MASK_RELEASE) == PATTERN_RELEASE && !flagReleaseProcessed)
    {
        result = true;                      // Release detected
        flagReleaseProcessed = true;        // Mark as processed

        // Trigger callback if registered
        triggerCallback(callbackRelease);
    }
    else if ((historyButton & MASK_RELEASE) != PATTERN_RELEASE)
    {
        flagReleaseProcessed = false;       // Reset flag when pattern changes
    }

    return result;                          // Return release detection result
}

// ****************************************************************************
// Function: isDown
// Purpose:  Check if button is currently being held down
// Parameters: None
// Returns:  true if button is down, false otherwise
// ****************************************************************************
bool Debounce16::isDown()
{
    return (historyButton & MASK_DOWN_UP) == PATTERN_DOWN; // Recent bits are set = button down
}

// ****************************************************************************
// Function: isUp
// Purpose:  Check if button is currently released
// Parameters: None
// Returns:  true if button is up, false otherwise
// ****************************************************************************
bool Debounce16::isUp()
{
    return (historyButton & MASK_DOWN_UP) == PATTERN_UP;  // Recent bits are clear = button up
}

// Advanced Feature Configuration Implementation
// ****************************************************************************

// ****************************************************************************
// Function: enableDoublePressDetection
// Purpose:  Enable or disable double-press detection
// Parameters: enable - true to enable, false to disable
// Returns:  None
// ****************************************************************************
void Debounce16::enableDoublePressDetection(bool enable)
{
    flagEnableDoublePress = enable;         // Set enable flag

    if (!enable)
    {
        // Reset state if disabling
        countClick = 0;                     // Clear click counter
        flagDoublePressed = false;          // Clear double-press flag
    }
}

// ****************************************************************************
// Function: setDoublePressWindow
// Purpose:  Set time window for double-press detection
// Parameters: windowMs - time window in milliseconds
// Returns:  None
// ****************************************************************************
void Debounce16::setDoublePressWindow(uint16_t windowMs)
{
    windowDoublePress = windowMs;           // Store window value
}

// ****************************************************************************
// Function: enableLongPressDetection
// Purpose:  Enable or disable long-press detection
// Parameters: enable - true to enable, false to disable
// Returns:  None
// ****************************************************************************
void Debounce16::enableLongPressDetection(bool enable)
{
    flagEnableLongPress = enable;           // Set enable flag

    if (!enable)
    {
        flagLongPressed = false;            // Clear long-press flag
    }
}

// ****************************************************************************
// Function: setLongPressThreshold
// Purpose:  Set duration threshold for long-press detection
// Parameters: thresholdMs - threshold in milliseconds
// Returns:  None
// ****************************************************************************
void Debounce16::setLongPressThreshold(uint16_t thresholdMs)
{
    thresholdLongPress = thresholdMs;       // Store threshold value
}

// Advanced Feature Query Methods Implementation
// ****************************************************************************

// ****************************************************************************
// Function: isDoublePressed
// Purpose:  Check if double-press event has been detected
// Parameters: None
// Returns:  true if double-press detected, false otherwise
// ****************************************************************************
bool Debounce16::isDoublePressed()
{
    bool result = flagDoublePressed;        // Get current flag state

    if (result)
    {
        flagDoublePressed = false;          // Clear flag after reading
    }

    return result;                          // Return double-press status
}

// ****************************************************************************
// Function: isLongPressed
// Purpose:  Check if long-press is currently active
// Parameters: None
// Returns:  true if long-press active, false otherwise
// ****************************************************************************
bool Debounce16::isLongPressed()
{
    return flagLongPressed;                 // Return long-press flag state
}

// ****************************************************************************
// Function: getClickCount
// Purpose:  Get current click count
// Parameters: None
// Returns:  Number of clicks detected
// ****************************************************************************
uint8_t Debounce16::getClickCount()
{
    uint8_t result = countClick;            // Get current count

    if (countClick > 0 && stateButton == ButtonState::STATE_IDLE)
    {
        countClick = 0;                     // Reset count after reading in IDLE
    }

    return result;                          // Return click count
}

// Callback Registration Implementation
// ****************************************************************************

// ****************************************************************************
// Function: onPress
// Purpose:  Register callback for press event
// Parameters: callback - function pointer to callback
// Returns:  None
// ****************************************************************************
void Debounce16::onPress(void (*callback)())
{
    callbackPress = callback;               // Store callback pointer
}

// ****************************************************************************
// Function: onRelease
// Purpose:  Register callback for release event
// Parameters: callback - function pointer to callback
// Returns:  None
// ****************************************************************************
void Debounce16::onRelease(void (*callback)())
{
    callbackRelease = callback;             // Store callback pointer
}

// ****************************************************************************
// Function: onDoublePress
// Purpose:  Register callback for double-press event
// Parameters: callback - function pointer to callback
// Returns:  None
// ****************************************************************************
void Debounce16::onDoublePress(void (*callback)())
{
    callbackDoublePress = callback;         // Store callback pointer
}

// ****************************************************************************
// Function: onLongPressStart
// Purpose:  Register callback for long-press start event
// Parameters: callback - function pointer to callback
// Returns:  None
// ****************************************************************************
void Debounce16::onLongPressStart(void (*callback)())
{
    callbackLongPressStart = callback;      // Store callback pointer
}

// ****************************************************************************
// Function: onLongPressEnd
// Purpose:  Register callback for long-press end event
// Parameters: callback - function pointer to callback
// Returns:  None
// ****************************************************************************
void Debounce16::onLongPressEnd(void (*callback)())
{
    callbackLongPressEnd = callback;        // Store callback pointer
}

// Helper Methods Implementation
// ****************************************************************************

// ****************************************************************************
// Function: readButtonRaw
// Purpose:  Read current physical button state accounting for active level
// Parameters: None
// Returns:  true if button pressed, false if released
// ****************************************************************************
bool Debounce16::readButtonRaw()
{
#ifdef ARDUINO
    bool stateRaw = digitalRead(pinButton); // Read physical button state
#else
    bool stateRaw = gpio_get_level(pinButton);  // Read physical button state
#endif

    // Account for active logic level
    if (levelActive == DB_HIGH)
    {
        return stateRaw;                    // Active HIGH: return as-is
    }
    else
    {
        return !stateRaw;                   // Active LOW: invert
    }
}

// ****************************************************************************
// Function: updateStateMachine
// Purpose:  Process state transitions for advanced press pattern detection
// Parameters: None
// Returns:  None
// ****************************************************************************
void Debounce16::updateStateMachine()
{
    unsigned long currentTime = millis();   // Get current time
    unsigned long timeElapsed;              // Time elapsed variable

    // State machine logic
    switch (stateButton)
    {
        case ButtonState::STATE_IDLE:
            // Waiting for button press
            if (isPressed())
            {
                timePress = currentTime;    // Record press timestamp
                countClick = 1;             // First click
                stateButton = ButtonState::STATE_PRESS_FIRST;  // Move to press state
            }
            break;

        case ButtonState::STATE_PRESS_FIRST:
            // First press detected, checking for release or long press
            if (flagEnableLongPress)
            {
                timeElapsed = currentTime - timePress;  // Calculate press duration

                if (timeElapsed >= thresholdLongPress && !flagLongPressed)
                {
                    // Long press threshold reached
                    flagLongPressed = true;  // Set long-press flag
                    stateButton = ButtonState::STATE_LONG_PRESS_ACTIVE;  // Move to long-press state
                    triggerCallback(callbackLongPressStart);  // Trigger start callback
                }
            }

            if (isReleased())
            {
                timeEvent = currentTime;    // Record release timestamp

                if (flagLongPressed)
                {
                    // Was a long press, return to idle
                    resetState();           // Reset to idle state
                }
                else if (flagEnableDoublePress)
                {
                    // Check for double press
                    stateButton = ButtonState::STATE_RELEASE_FIRST;  // Move to release state
                }
                else
                {
                    // No double-press detection, return to idle
                    resetState();           // Reset to idle state
                }
            }
            break;

        case ButtonState::STATE_RELEASE_FIRST:
            // First release detected, waiting for second press
            timeElapsed = currentTime - timeEvent;  // Calculate time since release

            if (isPressed() && timeElapsed < windowDoublePress)
            {
                // Second press within window
                countClick = 2;             // Second click
                timePress = currentTime;    // Record second press time
                stateButton = ButtonState::STATE_PRESS_FIRST;  // Back to press state
            }
            else if (timeElapsed >= windowDoublePress)
            {
                // Window expired, single click confirmed
                if (countClick == 2)
                {
                    // Was a double press
                    flagDoublePressed = true;  // Set double-press flag
                    triggerCallback(callbackDoublePress);  // Trigger callback
                }
                resetState();               // Return to idle
            }
            break;

        case ButtonState::STATE_LONG_PRESS_ACTIVE:
            // Long press is active, waiting for release
            if (isReleased())
            {
                flagLongPressed = false;    // Clear long-press flag
                triggerCallback(callbackLongPressEnd);  // Trigger end callback
                resetState();               // Return to idle
            }
            break;

        case ButtonState::STATE_WAIT_DOUBLE_PRESS:
            // Reserved for future use
            break;

        case ButtonState::STATE_WAIT_LONG_PRESS:
            // Reserved for future use
            break;
    }
}

// ****************************************************************************
// Function: resetState
// Purpose:  Reset state machine to idle state
// Parameters: None
// Returns:  None
// ****************************************************************************
void Debounce16::resetState()
{
    stateButton = ButtonState::STATE_IDLE;  // Return to idle state
    countClick = 0;                         // Reset click counter
    flagDoublePressed = false;              // Clear double-press flag
    flagLongPressed = false;                // Clear long-press flag
}

// ****************************************************************************
// Function: triggerCallback
// Purpose:  Execute callback if registered
// Parameters: callback - function pointer to callback
// Returns:  None
// ****************************************************************************
void Debounce16::triggerCallback(void (*callback)())
{
    if (callback != nullptr)                // Check if callback registered
    {
        callback();                         // Execute callback function
    }
}
