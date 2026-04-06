// ****************************************************************************
// Title        : Double Press Detection Example (Interrupt)
// File Name    : 'DoublePressDetectionInterrupt.ino'
// Target MCU   : Espressif ESP32 (Doit DevKit Version 1)
// Description  : Demonstrates double-press and single-press detection using
//                a hardware timer ISR for precise 1ms updates.
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
const uint8_t  PIN_BUTTON          = 17;  // Button input pin
const uint8_t  PIN_LED_SINGLE      = 15;  // LED: single press indicator
const uint8_t  PIN_LED_DOUBLE      = 16;  // LED: double press indicator
const uint16_t INTERVAL_BLINK      = 1000; // Heartbeat interval (ms)
const uint16_t WINDOW_DOUBLE_PRESS = 300; // Double-press detection window (ms)
const uint16_t DURATION_FEEDBACK   = 500; // LED feedback duration (ms)
const uint32_t FREQ_TIMER_HZ       = 1000000; // Timer frequency: 1 MHz
const uint32_t PERIOD_1MS_US       = 1000; // Timer alarm period: 1 ms

// Globals
// ****************************************************************************
uint8_t  stateHeartbeat    = LOW;         // Heartbeat LED state
uint32_t timeBlink         = 0;           // Last heartbeat timestamp (ms)
uint32_t timeLedSingle     = 0;           // Single press LED timer start (ms)
uint32_t timeLedDouble     = 0;           // Double press LED timer start (ms)
bool     flagLedSingleActive = false;     // Single press LED currently on
bool     flagLedDoubleActive = false;     // Double press LED currently on

hw_timer_t *timer = nullptr;              // Hardware timer handle

Debounce button(PIN_BUTTON, HIGH);        // Active HIGH button; external pull-down required

// Timer Interrupt Service Routine
// ****************************************************************************
void IRAM_ATTR onTimer()
{
    button.update();                      // Precise 1ms update
}

// Setup Code
// ****************************************************************************
void setup()
{
    Serial.begin(115200);
    Serial.println();
    Serial.println("Debounce16 Library - Double Press Detection (Interrupt)");
    Serial.println("=========================================================");
    Serial.println("- Single press = LED 1 (green)");
    Serial.println("- Double press = LED 2 (red)");
    Serial.println("- Double-press window: 300ms");
    Serial.println("- Requires ESP32 Arduino core >= 3.0.0");
    Serial.println("=========================================================");
    Serial.println();

    pinMode(PIN_LED_SINGLE, OUTPUT);
    pinMode(PIN_LED_DOUBLE, OUTPUT);
    pinMode(LED_BUILTIN,    OUTPUT);

    digitalWrite(PIN_LED_SINGLE, LOW);
    digitalWrite(PIN_LED_DOUBLE, LOW);

    button.enableDoublePressDetection(true);
    button.setDoublePressWindow(WINDOW_DOUBLE_PRESS);

    timer = timerBegin(FREQ_TIMER_HZ);
    timerAttachInterrupt(timer, &onTimer);
    timerAlarm(timer, PERIOD_1MS_US, true, 0);
}

// Main Program
// ****************************************************************************
void loop()
{
    uint32_t currentMillis = millis();

    // Double press: confirmed within window
    if (button.isDoublePressed())
    {
        digitalWrite(PIN_LED_DOUBLE, HIGH);
        timeLedDouble       = currentMillis;
        flagLedDoubleActive = true;
        Serial.println(">>> DOUBLE PRESS <<<");
    }

    // Single press: window expired with one tap
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
