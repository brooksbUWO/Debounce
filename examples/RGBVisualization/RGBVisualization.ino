// ****************************************************************************
// Title        : RGB Visualization Example for Debounce Library
// Filename     : 'RGBVisualization.ino'
// Target MCU   : Espressif ESP32 (Doit DevKit Version 1)
//
// Revision History:
// When         Who         Description of change
// -----------  ----------- -----------------------
// 28-MAR-2025  brooks      program start
// ****************************************************************************

// Include Files
// ****************************************************************************
#include <Arduino.h>
#include <Debounce.h>                      // Debouncing library
#include <FastLED.h>                       // FastLED library

// Globals
// ****************************************************************************
const uint8_t PIN_LED = 15;                // Pin number connected to LED
const uint8_t PIN_DATA = 16;               // Pin number for RGB LED
const uint8_t NUM_LEDS = 1;                // Number of RGB LEDs
const uint8_t PIN_BUTTON = 17;             // Pin number for BUTTON
const uint16_t INTERVAL_HEARTBEAT = 1000;  // Blink on/off time in milliseconds
const uint8_t INTERVAL_DEBOUNCE = 1;       // Update button state every 1ms

bool stateLed = false;                     // State false=LOW, true=HIGH
uint8_t stateLedBlue = LOW;                // State of BUILTIN_LED
uint32_t timeLedBlink = 0;                 // Time of last LED heartbeat blink
uint32_t timeDebounce = 0;                 // Time of last debounce update

CRGB leds[NUM_LEDS];                       // Define the array of leds

// Define colors for different button states
const uint8_t BRIGHTNESS = 100;            // LED brightness level
const CRGB COLOR_OFF = CRGB::Black;        // LED off
const CRGB COLOR_PRESSED = CRGB::Red;      // Color when button is pressed
const CRGB COLOR_RELEASED = CRGB::Blue;    // Color when button is released
const CRGB COLOR_DOWN = CRGB::Green;       // Color when button is held down
const CRGB COLOR_UP = CRGB::Purple;        // Color when button is up

// Setup button debouncing with active HIGH logic
const uint8_t logicLevel = HIGH;           // Use HIGH for active HIGH buttons
Debounce myButton(PIN_BUTTON, logicLevel); // Instantiate debouncing object

// Setup Code
// ****************************************************************************
void setup()
{
    Serial.begin(115200);                  // Starts serial monitor
    Serial.println("Debounce Library Example with RGB LED");
    Serial.println("====================================");
    Serial.println("- RED: Button pressed event");
    Serial.println("- BLUE: Button released event");
    Serial.println("- GREEN: Button held down state");
    Serial.println("- PURPLE: Button up state");
    Serial.println("- BLUE LED: Heartbeat (blinks every second)");
    Serial.println("====================================");
    
    pinMode(PIN_LED, OUTPUT);              // Declare pin as digital output
    digitalWrite(PIN_LED, LOW);            // Start with LED off
    
    // Configure button pin based on logic level
    if (logicLevel == LOW) 
    {
        pinMode(PIN_BUTTON, INPUT_PULLUP); // Use internal pullup for active LOW button
    } 
    else 
    {
        pinMode(PIN_BUTTON, INPUT);        // Regular input for active HIGH button
    }
    
    pinMode(LED_BUILTIN, OUTPUT);          // LED digital pin as an output

    // Initialize FastLED library
    FastLED.addLeds<WS2812, PIN_DATA, GRB>(leds, NUM_LEDS);
    FastLED.setBrightness(BRIGHTNESS);
    fill_solid(leds, NUM_LEDS, COLOR_UP);  // Initial state - up
    FastLED.show();
}

// Main program
// ****************************************************************************
void loop()
{
    // Current time for non-blocking operations
    uint32_t currentMillis = millis();
    
    // Heartbeat LED blinking
    if (currentMillis - timeLedBlink > INTERVAL_HEARTBEAT)
    {
        timeLedBlink = currentMillis;           // Update last LED blink time
        stateLed = !stateLed;                   // Toggle LED state
        digitalWrite(PIN_LED, stateLed);        // Set the LED state
        stateLedBlue = !stateLedBlue;           // Toggle LED on/off
        digitalWrite(LED_BUILTIN, stateLedBlue);
    }
    
    // Non-interrupt approach: update button state at regular intervals
    // This replaces the timer interrupt approach in the other examples
    if (currentMillis - timeDebounce >= INTERVAL_DEBOUNCE)
    {
        timeDebounce = currentMillis;
        myButton.update();                      // Update button state every 1ms
    }

    // Check button states and update RGB LED
    if (myButton.isPressed())
    {
        fill_solid(leds, NUM_LEDS, COLOR_PRESSED);
        FastLED.show();
        Serial.println("Button PRESSED - LED Red");
    }
    else if (myButton.isReleased())
    {
        fill_solid(leds, NUM_LEDS, COLOR_RELEASED);
        FastLED.show();
        Serial.println("Button RELEASED - LED Blue");
    }
    else if (myButton.stateChanged())
    {
        if (myButton.isDown())
        {
            fill_solid(leds, NUM_LEDS, COLOR_DOWN);
            FastLED.show();
            Serial.println("Button DOWN - LED Green");
        }
        else if (myButton.isUp())
        {
            fill_solid(leds, NUM_LEDS, COLOR_UP);
            FastLED.show();
            Serial.println("Button UP - LED Purple");
        }
    }
}