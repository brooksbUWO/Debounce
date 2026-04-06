// ****************************************************************************
// Title        : Simple Debounce Example (Interrupt)
// File Name    : 'DebounceSimpleInterrupt.ino'
// Target MCU   : Espressif ESP32 (Doit DevKit Version 1)
// Description  : Demonstrates button debouncing using a hardware timer interrupt
//                for precise 1ms update timing
//
// Requires: ESP32 Arduino core >= 3.0.0
//
// Revision History:
// When         Who         Description of change
// -----------  ----------- -----------------------
// 30-SEP-2025  Brooks      Initial example
// 06-APR-2026  Brooks      Update timer API to ESP32 core v3.x; style fixes
//
// ****************************************************************************

// Include Files
// ****************************************************************************
#include <Debounce16.h>

// Constants
// ****************************************************************************
const uint8_t  PIN_BUTTON     = 17;       // Button input pin
const uint8_t  PIN_LED        = 15;       // LED output pin
const uint16_t INTERVAL_BLINK = 1000;     // Heartbeat blink interval (ms)
const uint32_t FREQ_TIMER_HZ  = 1000000;  // Timer frequency: 1 MHz (1 us resolution)
const uint32_t PERIOD_1MS_US  = 1000;     // Timer alarm period: 1000 us = 1 ms

// Globals
// ****************************************************************************
bool     stateLed       = false;          // LED toggle state
uint8_t  stateHeartbeat = LOW;            // Heartbeat LED state
uint32_t timeBlink      = 0;              // Last heartbeat timestamp (ms)

hw_timer_t *timer = nullptr;              // Hardware timer handle

Debounce button(PIN_BUTTON, HIGH);        // Active HIGH button; external pull-down required

// Timer Interrupt Service Routine
// ****************************************************************************
void IRAM_ATTR onTimer()
{
    button.update();                      // Precise 1ms update; button is ISR-safe
}

// Setup Code
// ****************************************************************************
void setup()
{
    Serial.begin(115200);
    Serial.println();
    Serial.println("Debounce16 Library - Simple Example (Interrupt)");
    Serial.println("================================================");
    Serial.println("- Press button to toggle LED");
    Serial.println("- Hardware timer provides precise 1ms updates");
    Serial.println("- Requires ESP32 Arduino core >= 3.0.0");
    Serial.println("================================================");
    Serial.println();

    pinMode(PIN_LED, OUTPUT);
    digitalWrite(PIN_LED, LOW);

    pinMode(LED_BUILTIN, OUTPUT);

    // Configure hardware timer for 1ms interrupts (ESP32 core v3.x API)
    timer = timerBegin(FREQ_TIMER_HZ);                   // 1 MHz clock
    timerAttachInterrupt(timer, &onTimer);                // Attach ISR
    timerAlarm(timer, PERIOD_1MS_US, true, 0);           // 1ms, auto-reload, unlimited

    Serial.println("Timer interrupt configured for 1ms updates");
}

// Main Program
// ****************************************************************************
void loop()
{
    uint32_t currentMillis = millis();

    // No debounce update needed here -- handled by ISR

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
        timeBlink      = currentMillis;
        stateHeartbeat = !stateHeartbeat;
        digitalWrite(LED_BUILTIN, stateHeartbeat);
    }
}
