// ****************************************************************************
// Title		: Debounce
// File Name	: 'debounce.h'
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

#ifndef debounce_H
#define debounce_H
#pragma once

// Include Files
// ****************************************************************************
#include <Arduino.h>

// Class
// ****************************************************************************
class Debounce
{
public:
	// Constructors
	Debounce(uint8_t buttonPin); // Defaults to Active HIGH logic

	Debounce(uint8_t buttonPin, uint8_t activeLevel);

	void update(void);
	uint8_t isUp(void);
	uint8_t isDown(void);
	uint8_t isPressed(void);
	uint8_t isReleased(void);

protected:
private:
	uint8_t _histSize = 0;
	uint8_t _buttonHistory = 0;
	uint8_t _button = 0;
	uint8_t _logicLevel = 0;

	uint8_t readButton(void);
};

#endif // debounce_H