// ****************************************************************************
// Title        : Multiple Buttons Example (Polling)
// File Name    : 'MultipleButtons.ino'
// Target MCU   : Espressif ESP32 (Doit DevKit Version 1)
// Description  : Demonstrates two independent Debounce instances. Button A
//                toggles LED A. Button B cycles through three LED patterns on
//                LED B (off, on, blink). Polling-based update method.
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
const uint8_t  PIN_BUTTON_A      = 17;    // Button A input pin
const uint8_t  PIN_BUTTON_B      = 18;    // Button B input pin
const uint8_t  PIN_LED_A         = 15;    // LED A output pin
const uint8_t  PIN_LED_B         = 16;    // LED B output pin
const uint16_t INTERVAL_BLINK    = 1000;  // Heartbeat blink interval (ms)
const uint16_t INTERVAL_DEBOUNCE = 1;     // Debounce update interval (ms)
const uint16_t INTERVAL_LED_BLINK = 250;  // LED B blink interval in blink mode (ms)

// Globals
// ****************************************************************************
uint8_t  stateHeartbeat = LOW;            // Heartbeat LED state
uint32_t timeBlink      = 0;              // Last heartbeat timestamp (ms)
uint32_t timeDebounce   = 0;              // Last debounce update timestamp (ms)
uint32_t timeLedBlink   = 0;              // LED B blink timer (ms)
bool     stateLedA      = false;          // LED A toggle state
uint8_t  modeLedB       = 0;             // LED B mode: 0=off, 1=on, 2=blink
uint8_t  stateLedBlink  = LOW;           // LED B blink output state

Debounce buttonA(PIN_BUTTON_A, HIGH);     // Button A: active HIGH, external pull-down
Debounce buttonB(PIN_BUTTON_B, HIGH);     // Button B: active HIGH, external pull-down

// Setup Code
// ****************************************************************************
void setup()
{
    Serial.begin(115200);
    Serial.println();
    Serial.println("Debounce16 Library - Multiple Buttons (Polling)");
    Serial.println("================================================");
    Serial.println("- Button A: toggle LED A");
    Serial.println("- Button B: cycle LED B (off -> on -> blink -> off)");
    Serial.println("- Built-in LED shows heartbeat");
    Serial.println("================================================");
    Serial.println();

    pinMode(PIN_LED_A,   OUTPUT);
    pinMode(PIN_LED_B,   OUTPUT);
    pinMode(LED_BUILTIN, OUTPUT);

    digitalWrite(PIN_LED_A,   LOW);
    digitalWrite(PIN_LED_B,   LOW);
}

// Main Program
// ****************************************************************************
void loop()
{
    uint32_t currentMillis = millis();

    // Update both buttons at regular intervals
    if (currentMillis - timeDebounce >= INTERVAL_DEBOUNCE)
    {
        timeDebounce = currentMillis;
        buttonA.update();
        buttonB.update();
    }

    // Button A: toggle LED A
    if (buttonA.isPressed())
    {
        stateLedA = !stateLedA;
        digitalWrite(PIN_LED_A, stateLedA);
        Serial.print("Button A -- LED A: ");
        Serial.println(stateLedA ? "ON" : "OFF");
    }

    // Button B: advance LED B mode (off -> on -> blink -> off -> ...)
    if (buttonB.isPressed())
    {
        modeLedB = (modeLedB + 1) % 3;     // Cycle through 0, 1, 2

        const char* modeNames[] = {"OFF", "ON", "BLINK"};
        Serial.print("Button B -- LED B mode: ");
        Serial.println(modeNames[modeLedB]);

        if (modeLedB == 0)
        {
            digitalWrite(PIN_LED_B, LOW);   // Turn off immediately when mode set to OFF
        }
        if (modeLedB == 1)
        {
            digitalWrite(PIN_LED_B, HIGH);  // Turn on immediately when mode set to ON
        }
    }

    // LED B output based on current mode
    if (modeLedB == 2)                      // Blink mode: toggle at INTERVAL_LED_BLINK
    {
        if (currentMillis - timeLedBlink >= INTERVAL_LED_BLINK)
        {
            timeLedBlink   = currentMillis;
            stateLedBlink  = !stateLedBlink;
            digitalWrite(PIN_LED_B, stateLedBlink);
        }
    }

    // Heartbeat indicator
    if (currentMillis - timeBlink >= INTERVAL_BLINK)
    {
        timeBlink      = currentMillis;
        stateHeartbeat = !stateHeartbeat;
        digitalWrite(LED_BUILTIN, stateHeartbeat);
    }
}
