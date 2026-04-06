// ****************************************************************************
// Title        : Long Press Detection Example (Interrupt)
// File Name    : 'LongPressDetectionInterrupt.ino'
// Target MCU   : Espressif ESP32 (Doit DevKit Version 1)
// Description  : Demonstrates long-press detection using a hardware timer ISR.
//                onLongPressStart/End callbacks fire automatically from update().
//                No polling of isLongPressed() is needed for callback-based usage.
//
// Requires: ESP32 Arduino core >= 3.0.0
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
const uint8_t  PIN_BUTTON     = 17;       // Button input pin
const uint8_t  PIN_LED_HELD   = 15;       // LED: ON while long press active
const uint8_t  PIN_LED_END    = 16;       // LED: flashes on long press release
const uint16_t INTERVAL_BLINK = 1000;     // Heartbeat blink interval (ms)
const uint16_t THRESHOLD_LONG = 1000;     // Long-press threshold (ms)
const uint16_t DURATION_FLASH = 200;      // Release LED flash duration (ms)
const uint32_t FREQ_TIMER_HZ  = 1000000;  // Timer frequency: 1 MHz
const uint32_t PERIOD_1MS_US  = 1000;     // Timer alarm period: 1 ms

// Globals
// ****************************************************************************
uint8_t  stateHeartbeat  = LOW;           // Heartbeat LED state
uint32_t timeBlink       = 0;             // Last heartbeat timestamp (ms)
uint32_t timeLedEnd      = 0;             // Release LED flash start timestamp (ms)
bool     flagLedFlash    = false;         // Release LED flash active flag

hw_timer_t *timer = nullptr;              // Hardware timer handle

Debounce button(PIN_BUTTON, HIGH);        // Active HIGH button; external pull-down required

// Callbacks (fire automatically from update() inside the ISR)
// ****************************************************************************
void IRAM_ATTR onLongStart()
{
    digitalWrite(PIN_LED_HELD, HIGH);     // Light held LED immediately at threshold
}

void IRAM_ATTR onLongEnd()
{
    digitalWrite(PIN_LED_HELD, LOW);      // Turn off held LED
    flagLedFlash = true;                  // Schedule release flash (handled in loop)
    timeLedEnd   = millis();
}

// Timer Interrupt Service Routine
// ****************************************************************************
void IRAM_ATTR onTimer()
{
    button.update();                      // Callbacks fire from here if threshold reached
}

// Setup Code
// ****************************************************************************
void setup()
{
    Serial.begin(115200);
    Serial.println();
    Serial.println("Debounce16 Library - Long Press Detection (Interrupt)");
    Serial.println("=======================================================");
    Serial.println("- Hold button > 1000ms: LED 1 lights at threshold");
    Serial.println("- Release: LED 1 off, LED 2 flashes");
    Serial.println("- Requires ESP32 Arduino core >= 3.0.0");
    Serial.println("=======================================================");
    Serial.println();

    pinMode(PIN_LED_HELD, OUTPUT);
    pinMode(PIN_LED_END,  OUTPUT);
    pinMode(LED_BUILTIN,  OUTPUT);

    digitalWrite(PIN_LED_HELD, LOW);
    digitalWrite(PIN_LED_END,  LOW);

    button.enableLongPressDetection();
    button.setLongPressThreshold(THRESHOLD_LONG);
    button.onLongPressStart(onLongStart);
    button.onLongPressEnd(onLongEnd);

    timer = timerBegin(FREQ_TIMER_HZ);
    timerAttachInterrupt(timer, &onTimer);
    timerAlarm(timer, PERIOD_1MS_US, true, 0);
}

// Main Program
// ****************************************************************************
void loop()
{
    uint32_t currentMillis = millis();

    // Release LED flash (set by onLongEnd callback; managed here to avoid
    // calling delay() or digitalWrite() inside the ISR context)
    if (flagLedFlash)
    {
        digitalWrite(PIN_LED_END, HIGH);

        if (currentMillis - timeLedEnd >= DURATION_FLASH)
        {
            digitalWrite(PIN_LED_END, LOW);
            flagLedFlash = false;
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