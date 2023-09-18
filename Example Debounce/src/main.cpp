// ****************************************************************************
// Title		: Example Debounce
// File Name	: 'main.cpp'
// Target MCU	: Espressif ESP32 (Doit DevKit Version 1)
//
// Revision History:
// When			Who			Description of change
// -----------	-----------	-----------------------
// 18-APR-2022	brooks		program start
// ****************************************************************************

// Include Files
// ****************************************************************************
#include <Arduino.h>
#include <debounce.h> // Debouncing library

// Globals
// ****************************************************************************
const int LED = 15;		// Pin number external LED
int ledState = LOW;		// State of external LED
int ledBlueState = LOW; // State of BUILTIN_LED
const int BUTTON = 17;	// Pin number for external BUTTON

hw_timer_t *timer0 = NULL; // Create timer
portMUX_TYPE mux = portMUX_INITIALIZER_UNLOCKED;
volatile bool flagTimer0 = false; // Flag if Interrupt triggered

unsigned long prevTime = millis(); // Timing LED_BUILTIN heartbeat

Debounce myButton(BUTTON, 1); // Instantiate debouncing object

// Interrupt Service Routine (ISR)
// ****************************************************************************
void IRAM_ATTR timerISR0()
{
	portENTER_CRITICAL_ISR(&mux);
	flagTimer0 = true; // Set flag variable
	portEXIT_CRITICAL_ISR(&mux);
}

// Begin Code
// ****************************************************************************
void setup()
{
	Serial.begin(115200);		  // Starts serial monitor
	pinMode(LED, OUTPUT);		  // Declare pin as digital output
	digitalWrite(LED, LOW);		  // Start with LED off
	pinMode(BUTTON, INPUT);		  // Declare pin as digital input
	pinMode(LED_BUILTIN, OUTPUT); // LED digital pin as an output

	timer0 = timerBegin(0, 80, true); // 80MHz/80 prescaler=1 MHz
	timerAttachInterrupt(timer0, &timerISR0, true);
	timerAlarmWrite(timer0, 10000, true); // ISR 1ms
	timerAlarmEnable(timer0);			  // Enable ISR
}

// Main program
// ****************************************************************************
void loop()
{
	if (flagTimer0) // Process timer interrupt
	{
		flagTimer0 = false; // Reset flag
		myButton.update();	// Check button every 1ms
	}

	if (millis() - prevTime >= 1000) // Repeats every 1sec
	{
		prevTime = millis();		  // Reset time
		ledBlueState = !ledBlueState; // Toggle LED on/off
		digitalWrite(LED_BUILTIN, ledBlueState);
	}

	if (myButton.isPressed()) // Check IF button was pushed
	{
		ledState = !ledState; // Toggle LED on/off
		digitalWrite(LED, ledState);
		if (ledState)
			Serial.printf("External LED is ON\r\n");
		else
			Serial.printf("External LED is OFF\r\n");
	}
}