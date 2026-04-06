// ****************************************************************************
// Title        : Multiple Buttons Example (Interrupt)
// File Name    : 'MultipleButtonsInterrupt.ino'
// Target MCU   : Espressif ESP32 (Doit DevKit Version 1)
// Description  : Demonstrates two independent Debounce instances sharing one
//                hardware timer ISR for precise 1ms updates. Button A toggles
//                LED A. Button B cycles through three LED modes on LED B.
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
const uint8_t  PIN_BUTTON_A       = 17;   // Button A input pin
const uint8_t  PIN_BUTTON_B       = 18;   // Button B input pin
const uint8_t  PIN_LED_A          = 15;   // LED A output pin
const uint8_t  PIN_LED_B          = 16;   // LED B output pin
const uint16_t INTERVAL_BLINK     = 1000; // Heartbeat blink interval (ms)
const uint16_t INTERVAL_LED_BLINK = 250;  // LED B blink interval in blink mode (ms)
const uint32_t FREQ_TIMER_HZ      = 1000000; // Timer frequency: 1 MHz
const uint32_t PERIOD_1MS_US      = 1000; // Timer alarm period: 1 ms

// Globals
// ****************************************************************************
uint8_t  stateHeartbeat = LOW;            // Heartbeat LED state
uint32_t timeBlink      = 0;              // Last heartbeat timestamp (ms)
uint32_t timeLedBlink   = 0;              // LED B blink timer (ms)
bool     stateLedA      = false;          // LED A toggle state
uint8_t  modeLedB       = 0;             // LED B mode: 0=off, 1=on, 2=blink
uint8_t  stateLedBlink  = LOW;           // LED B blink output state

hw_timer_t *timer = nullptr;              // Hardware timer handle

Debounce buttonA(PIN_BUTTON_A, HIGH);     // Button A: active HIGH, external pull-down
Debounce buttonB(PIN_BUTTON_B, HIGH);     // Button B: active HIGH, external pull-down

// Timer Interrupt Service Routine
// ****************************************************************************
void IRAM_ATTR onTimer()
{
    buttonA.update();                     // Both buttons updated every 1ms by the ISR
    buttonB.update();
}

// Setup Code
// ****************************************************************************
void setup()
{
    Serial.begin(115200);
    Serial.println();
    Serial.println("Debounce16 Library - Multiple Buttons (Interrupt)");
    Serial.println("==================================================");
    Serial.println("- Button A: toggle LED A");
    Serial.println("- Button B: cycle LED B (off -> on -> blink -> off)");
    Serial.println("- Shared timer ISR updates both buttons at 1ms");
    Serial.println("- Requires ESP32 Arduino core >= 3.0.0");
    Serial.println("==================================================");
    Serial.println();

    pinMode(PIN_LED_A,   OUTPUT);
    pinMode(PIN_LED_B,   OUTPUT);
    pinMode(LED_BUILTIN, OUTPUT);

    digitalWrite(PIN_LED_A,   LOW);
    digitalWrite(PIN_LED_B,   LOW);

    timer = timerBegin(FREQ_TIMER_HZ);
    timerAttachInterrupt(timer, &onTimer);
    timerAlarm(timer, PERIOD_1MS_US, true, 0);
}

// Main Program
// ****************************************************************************
void loop()
{
    uint32_t currentMillis = millis();

    // Button A: toggle LED A
    if (buttonA.isPressed())
    {
        stateLedA = !stateLedA;
        digitalWrite(PIN_LED_A, stateLedA);
        Serial.print("Button A -- LED A: ");
        Serial.println(stateLedA ? "ON" : "OFF");
    }

    // Button B: advance LED B mode
    if (buttonB.isPressed())
    {
        modeLedB = (modeLedB + 1) % 3;

        const char* modeNames[] = {"OFF", "ON", "BLINK"};
        Serial.print("Button B -- LED B mode: ");
        Serial.println(modeNames[modeLedB]);

        if (modeLedB == 0) { digitalWrite(PIN_LED_B, LOW);  }
        if (modeLedB == 1) { digitalWrite(PIN_LED_B, HIGH); }
    }

    // LED B blink mode
    if (modeLedB == 2 && (currentMillis - timeLedBlink >= INTERVAL_LED_BLINK))
    {
        timeLedBlink   = currentMillis;
        stateLedBlink  = !stateLedBlink;
        digitalWrite(PIN_LED_B, stateLedBlink);
    }

    // Heartbeat indicator
    if (currentMillis - timeBlink >= INTERVAL_BLINK)
    {
        timeBlink      = currentMillis;
        stateHeartbeat = !stateHeartbeat;
        digitalWrite(LED_BUILTIN, stateHeartbeat);
    }
}