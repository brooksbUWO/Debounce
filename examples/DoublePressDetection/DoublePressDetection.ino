// ****************************************************************************
// Title        : Double Press Detection Example (Polling)
// File Name    : 'DoublePressDetection.ino'
// Target MCU   : Espressif ESP32 (Doit DevKit Version 1)
// Description  : Demonstrates double-press and single-press detection.
//                Single press lights LED 1. Double press lights LED 2.
//
// Revision History:
// When         Who         Description of change
// -----------  ----------- -----------------------
// 30-SEP-2025  Brooks      Initial example
// 06-APR-2026  Brooks      Replace getClickCount with isSinglePressed; style fixes
//
// ****************************************************************************

// Include Files
// ****************************************************************************
#include <Debounce16.h>

// Constants
// ****************************************************************************
const uint8_t  PIN_BUTTON          = 17;  // Button input pin
const uint8_t  PIN_LED_SINGLE      = 15;  // LED: single press indicator
const uint8_t  PIN_LED_DOUBLE      = 16;  // LED: double press indicator
const uint16_t INTERVAL_BLINK      = 1000; // Heartbeat interval (ms)
const uint16_t INTERVAL_DEBOUNCE   = 1;   // Debounce update interval (ms)
const uint16_t WINDOW_DOUBLE_PRESS = 300; // Double-press detection window (ms)
const uint16_t DURATION_FEEDBACK   = 500; // LED feedback duration (ms)

// Globals
// ****************************************************************************
uint8_t  stateHeartbeat    = LOW;         // Heartbeat LED state
uint32_t timeBlink         = 0;           // Last heartbeat timestamp (ms)
uint32_t timeDebounce      = 0;           // Last debounce update timestamp (ms)
uint32_t timeLedSingle     = 0;           // Single press LED timer start (ms)
uint32_t timeLedDouble     = 0;           // Double press LED timer start (ms)
bool     flagLedSingleActive = false;     // Single press LED currently on
bool     flagLedDoubleActive = false;     // Double press LED currently on

Debounce button(PIN_BUTTON, HIGH);        // Active HIGH button; external pull-down required

// Setup Code
// ****************************************************************************
void setup()
{
    Serial.begin(115200);
    Serial.println();
    Serial.println("Debounce16 Library - Double Press Detection");
    Serial.println("============================================");
    Serial.println("- Single press = LED 1 (green)");
    Serial.println("- Double press = LED 2 (red)");
    Serial.println("- Double-press window: 300ms");
    Serial.println("============================================");
    Serial.println();

    pinMode(PIN_LED_SINGLE, OUTPUT);
    pinMode(PIN_LED_DOUBLE, OUTPUT);
    pinMode(LED_BUILTIN,    OUTPUT);

    digitalWrite(PIN_LED_SINGLE, LOW);
    digitalWrite(PIN_LED_DOUBLE, LOW);

    button.enableDoublePressDetection(true);
    button.setDoublePressWindow(WINDOW_DOUBLE_PRESS);
}

// Main Program
// ****************************************************************************
void loop()
{
    uint32_t currentMillis = millis();

    // Update debounce state
    if (currentMillis - timeDebounce >= INTERVAL_DEBOUNCE)
    {
        timeDebounce = currentMillis;
        button.update();
    }

    // Double press: fires once when two taps confirmed within window
    if (button.isDoublePressed())
    {
        digitalWrite(PIN_LED_DOUBLE, HIGH);
        timeLedDouble       = currentMillis;
        flagLedDoubleActive = true;
        Serial.println(">>> DOUBLE PRESS <<<");
    }

    // Single press: fires once after window expires with only one tap
    if (button.isSinglePressed())
    {
        digitalWrite(PIN_LED_SINGLE, HIGH);
        timeLedSingle       = currentMillis;
        flagLedSingleActive = true;
        Serial.println("Single press");
    }

    // Auto-off: single press LED
    if (flagLedSingleActive && (currentMillis - timeLedSingle >= DURATION_FEEDBACK))
    {
        digitalWrite(PIN_LED_SINGLE, LOW);
        flagLedSingleActive = false;
    }

    // Auto-off: double press LED
    if (flagLedDoubleActive && (currentMillis - timeLedDouble >= DURATION_FEEDBACK))
    {
        digitalWrite(PIN_LED_DOUBLE, LOW);
        flagLedDoubleActive = false;
    }

    // Heartbeat indicator
    if (currentMillis - timeBlink >= INTERVAL_BLINK)
    {
        timeBlink      = currentMillis;
        stateHeartbeat = !stateHeartbeat;
        digitalWrite(LED_BUILTIN, stateHeartbeat);
    }
}
