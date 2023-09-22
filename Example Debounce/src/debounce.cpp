// ****************************************************************************
// Title		: Debounce
// File Name	: 'debounce.cpp'
// Target MCU	: Espressif ESP32 (Doit DevKit Version 1)
//
// Code based on
// https://hackaday.com/2015/12/10/embed-with-elliot-debounce-your-noisy-buttons-part-ii/
//
// Revision History:
// When			Who			Description of change
// -----------	-----------	-----------------------
// 14-APR-2022	brooks		program start
// ****************************************************************************

// Include Files
// ****************************************************************************
#include <arduino.h>
#include <debounce.h>

#define MASK 0b11000111	   // Don't care bits = 0
#define RELEASE 0b11000000 // Button released
#define DOWN 0b11111111	   // Button down mask
#define UP 0b00000000	   // Button up
#define PRESS 0b00000111   // Button pressed
#define CLEAR 0b00000000   // Clear history
#define SET 0b11111111	   // Set history

// Constructors
// ****************************************************************************
Debounce::Debounce(uint8_t buttonPin) // Pin to debounce
{
	_button = buttonPin;
	_logicLevel = HIGH;
}

Debounce::Debounce(uint8_t buttonPin, uint8_t logicLevel)
{
	_button = buttonPin;
	_logicLevel = logicLevel; // LOW or HIGH
}

// Functions
// ****************************************************************************
uint8_t Debounce::readButton(void)
{
	uint8_t result = false;
	if (_logicLevel == LOW && digitalRead(_button) == LOW)
	{
		result = true;
	}
	else if (_logicLevel == HIGH && digitalRead(_button) == HIGH)
	{
		result = true;
	}
	return result;
}

void Debounce::update(void)
{
	_buttonHistory = (_buttonHistory << 1);
	_buttonHistory |= readButton();
}

uint8_t Debounce::isPressed(void)
{
	uint8_t _pressed = 0;

	if ((_buttonHistory & MASK) == PRESS)
	{
		_pressed = 1;
		_buttonHistory = SET;
	}
	return _pressed;
}

uint8_t Debounce::isReleased(void)
{
	uint8_t _released = 0;

	if ((MASK & _buttonHistory) == RELEASE)
	{
		_released = 1;
		_buttonHistory = CLEAR;
	}
	return _released;
}

uint8_t Debounce::isDown(void)
{
	return (_buttonHistory == DOWN);
}

uint8_t Debounce::isUp(void)
{
	return (_buttonHistory == UP);
}
