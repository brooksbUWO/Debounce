// ****************************************************************************
// Title        : Simple Debounce Example (Polling)
// File Name    : 'DebounceSimple.ino'
// Target MCU   : Espressif ESP32 (Doit DevKit Version 1)
// Description  : Demonstrates basic button debouncing with toggle functionality
//                using the polling-based update method
//
// Revision History:
// When         Who         Description of change
// -----------  ----------- -----------------------
// 30-SEP-2025  Brooks      Initial example
// 06-APR-2026  Brooks      Style fixes: uint32_t, uint8_t heartbeat, class Debounce
//
// ****************************************************************************

// Include Files
// ****************************************************************************
#include <Debounce16.h>

// Constants
// ****************************************************************************
const uint8_t  PIN_BUTTON        = 17;    // Button input pin
const uint8_t  PIN_LED           = 15;    // LED output pin
const uint16_t INTERVAL_BLINK    = 1000;  // Heartbeat blink interval (ms)
const uint16_t INTERVAL_DEBOUNCE = 1;     // Debounce update interval (ms)

// Globals
// ****************************************************************************
bool     stateLed       = false;          // LED toggle state
uint8_t  stateHeartbeat = LOW;            // Heartbeat LED state (LOW/HIGH are uint8_t macros)
uint32_t timeBlink      = 0;              // Last heartbeat timestamp (ms)
uint32_t timeDebounce   = 0;              // Last debounce update timestamp (ms)

Debounce button(PIN_BUTTON, HIGH);        // Active HIGH button; external pull-down required

// Setup Code
// ****************************************************************************
void setup()
{
    Serial.begin(115200);
    Serial.println();
    Serial.println("Debounce16 Library - Simple Example (Polling)");
    Serial.println("==============================================");
    Serial.println("- Press button to toggle LED");
    Serial.println("- Built-in LED shows heartbeat");
    Serial.println("==============================================");
    Serial.println();

    pinMode(PIN_LED, OUTPUT);
    digitalWrite(PIN_LED, LOW);

    pinMode(LED_BUILTIN, OUTPUT);
}

// Main Program
// ****************************************************************************
void loop()
{
    uint32_t currentMillis = millis();

    // Update debounce state at regular intervals (polling method)
    if (currentMillis - timeDebounce >= INTERVAL_DEBOUNCE)
    {
        timeDebounce = currentMillis;
        button.update();
    }

    // Check for button press event
    if (button.isPressed())
    {
        stateLed = !stateLed;
        digitalWrite(PIN_LED, stateLed);

        Serial.print("Button pressed - LED is now: ");
        Serial.println(stateLed ? "ON" : "OFF");
    }

    // Heartbeat indicator
    if (currentMillis - timeBlink >= INTERVAL_BLINK)
    {
        timeBlink       = currentMillis;
        stateHeartbeat  = !stateHeartbeat;
        digitalWrite(LED_BUILTIN, stateHeartbeat);
    }
}
