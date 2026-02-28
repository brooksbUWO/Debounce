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
// 28-FEB-2026  davidc      Updates to work under ESP-IDF without Arduino
//
// ****************************************************************************

#ifndef DEBOUNCE16_H
#define DEBOUNCE16_H

// Include Files
// ****************************************************************************
#ifdef ARDUINO

#include <Arduino.h>

typedef uint8_t db_pin_t;

#define DB_HIGH HIGH
#define DB_LOW LOW

#elifdef IDF_VER

#include "driver/gpio.h"

typedef gpio_num_t db_pin_t;

#define DB_HIGH 1
#define DB_LOW 0

#else

#error Platform not supported

#endif

// Class Declaration
// ****************************************************************************
class Debounce16
{
public:
    // Constructors
    // ****************************************************************************
    Debounce16(db_pin_t pin, bool activeLevel = DB_HIGH);

    // Core Debouncing Methods (Always Available)
    // ****************************************************************************
    void update();                          // Update button state (call every 1ms)
    bool isPressed();                       // Press event detected this cycle
    bool isReleased();                      // Release event detected this cycle
    bool isDown();                          // Button currently held down
    bool isUp();                            // Button currently released

    // Advanced Feature Configuration
    // ****************************************************************************
    void enableDoublePressDetection(bool enable = true);
    void setDoublePressWindow(uint16_t windowMs);

    void enableLongPressDetection(bool enable = true);
    void setLongPressThreshold(uint16_t thresholdMs);

    // Advanced Feature Query Methods
    // ****************************************************************************
    bool isDoublePressed();                 // Double-press event detected
    bool isLongPressed();                   // Long-press event detected
    uint8_t getClickCount();                // Get current click count

    // Callback Registration (Optional)
    // ****************************************************************************
    void onPress(void (*callback)());
    void onRelease(void (*callback)());
    void onDoublePress(void (*callback)());
    void onLongPressStart(void (*callback)());
    void onLongPressEnd(void (*callback)());

private:
    // Core Debouncing Members
    // ****************************************************************************
    uint16_t historyButton;                 // 16-bit button state history
    db_pin_t pinButton;                  // GPIO pin number
    bool levelActive;                       // Active logic level (HIGH/LOW)

    // State Machine for Press Pattern Detection
    // ****************************************************************************
    enum class ButtonState : uint8_t
    {
        STATE_IDLE,                         // Waiting for input
        STATE_PRESS_FIRST,                  // First press detected
        STATE_RELEASE_FIRST,                // First release detected
        STATE_WAIT_DOUBLE_PRESS,            // Waiting for second press
        STATE_WAIT_LONG_PRESS,              // Checking for long press
        STATE_LONG_PRESS_ACTIVE             // Long press confirmed
    };
    ButtonState stateButton;                // Current state

    // Timing Management
    // ****************************************************************************
    unsigned long timeEvent;                // Timestamp of last event
    unsigned long timePress;                // Timestamp of press start

    // Feature Configuration
    // ****************************************************************************
    bool flagEnableDoublePress;             // Enable double-press detection
    bool flagEnableLongPress;               // Enable long-press detection
    uint16_t windowDoublePress;             // Double-press time window (ms)
    uint16_t thresholdLongPress;            // Long-press time threshold (ms)

    // Event Tracking
    // ****************************************************************************
    uint8_t countClick;                     // Click counter
    bool flagDoublePressed;                 // Double-press event flag
    bool flagLongPressed;                   // Long-press event flag
    bool flagPressProcessed;                // Prevent multiple press events
    bool flagReleaseProcessed;              // Prevent multiple release events

    // Callback Function Pointers (Optional)
    // ****************************************************************************
    void (*callbackPress)();                // Press callback
    void (*callbackRelease)();              // Release callback
    void (*callbackDoublePress)();          // Double-press callback
    void (*callbackLongPressStart)();       // Long-press start callback
    void (*callbackLongPressEnd)();         // Long-press end callback

    // Bit Pattern Constants (16-bit patterns)
    // ****************************************************************************
    static const uint16_t MASK_PRESS =      0b1111100000111111;
    static const uint16_t PATTERN_PRESS =   0b0000000000111111;
    static const uint16_t MASK_RELEASE =    0b1111100000111111;
    static const uint16_t PATTERN_RELEASE = 0b1111100000000000;

    static const uint16_t MASK_DOWN_UP =    0b0000000000111111;
    static const uint16_t PATTERN_DOWN =    0b0000000000111111;
    static const uint16_t PATTERN_UP =      0b0000000000000000;

    // Helper Methods
    // ****************************************************************************
    bool readButtonRaw();                   // Read raw button state
    void updateStateMachine();              // Process state transitions
    void resetState();                      // Reset to idle state
    void triggerCallback(void (*callback)()); // Execute callback if registered
};

#endif // DEBOUNCE16_H
