// ****************************************************************************
// Title        : Long Press Detection Example (Polling)
// File Name    : 'LongPressDetection.ino'
// Target MCU   : Espressif ESP32 (Doit DevKit Version 1)
// Description  : Demonstrates long-press detection using polling.
//                LED 1 lights while button is held past the threshold.
//                LED 2 flashes briefly when the long press is released.
//
// Revision History:
// When         Who         Description of change
// -----------  ----------- -----------------------
// 06-APR-2026  Brooks      Initial example
//
// ****************************************************************************

// Include Files
// ****************************************************************************
#include <Debounce16.h>

// Constants
// ****************************************************************************
const uint8_t  PIN_BUTTON        = 17;    // Button input pin
const uint8_t  PIN_LED_HELD      = 15;    // LED: ON while long press is active
const uint8_t  PIN_LED_RELEASED  = 16;    // LED: flashes once on long press release
const uint16_t INTERVAL_BLINK    = 1000;  // Heartbeat blink interval (ms)
const uint16_t INTERVAL_DEBOUNCE = 1;     // Debounce update interval (ms)
const uint16_t THRESHOLD_LONG    = 1000;  // Long-press threshold (ms)
const uint16_t DURATION_FLASH    = 200;   // Release LED flash duration (ms)

// Globals
// ****************************************************************************
uint8_t  stateHeartbeat  = LOW;           // Heartbeat LED state
uint32_t timeBlink       = 0;             // Last heartbeat timestamp (ms)
uint32_t timeDebounce    = 0;             // Last debounce update timestamp (ms)
uint32_t timeLedReleased = 0;             // Release flash start timestamp (ms)
bool     prevLongPress   = false;         // Previous isLongPressed() state for edge detect
bool     flagLedFlash    = false;         // Release LED flash active flag

Debounce button(PIN_BUTTON, HIGH);        // Active HIGH button; external pull-down required

// Setup Code
// ****************************************************************************
void setup()
{
    Serial.begin(115200);
    Serial.println();
    Serial.println("Debounce16 Library - Long Press Detection (Polling)");
    Serial.println("====================================================");
    Serial.println("- Hold button > 1000ms: LED 1 lights");
    Serial.println("- Release after long press: LED 1 off, LED 2 flashes");
    Serial.println("- Short press: no LEDs");
    Serial.println("- Built-in LED shows heartbeat");
    Serial.println("====================================================");
    Serial.println();

    pinMode(PIN_LED_HELD,     OUTPUT);
    pinMode(PIN_LED_RELEASED, OUTPUT);
    pinMode(LED_BUILTIN,      OUTPUT);

    digitalWrite(PIN_LED_HELD,     LOW);
    digitalWrite(PIN_LED_RELEASED, LOW);

    button.enableLongPressDetection();
    button.setLongPressThreshold(THRESHOLD_LONG);
}

// Main Program
// ****************************************************************************
void loop()
{
    uint32_t currentMillis = millis();

    // Update debounce state at regular intervals
    if (currentMillis - timeDebounce >= INTERVAL_DEBOUNCE)
    {
        timeDebounce = currentMillis;
        button.update();
    }

    bool curLongPress = button.isLongPressed();

    // LED 1: on while long press is active
    digitalWrite(PIN_LED_HELD, curLongPress ? HIGH : LOW);

    // Detect falling edge of long press (just released after threshold)
    if (prevLongPress && !curLongPress)
    {
        digitalWrite(PIN_LED_RELEASED, HIGH);   // Start release flash
        timeLedReleased = currentMillis;
        flagLedFlash    = true;
        Serial.println("Long press released");
    }

    if (!prevLongPress && curLongPress)
    {
        Serial.println("Long press active");
    }

    prevLongPress = curLongPress;

    // Auto-off: release flash LED
    if (flagLedFlash && (currentMillis - timeLedReleased >= DURATION_FLASH))
    {
        digitalWrite(PIN_LED_RELEASED, LOW);
        flagLedFlash = false;
    }

    // Heartbeat indicator
    if (currentMillis - timeBlink >= INTERVAL_BLINK)
    {
        timeBlink      = currentMillis;
        stateHeartbeat = !stateHeartbeat;
        digitalWrite(LED_BUILTIN, stateHeartbeat);
    }
}
