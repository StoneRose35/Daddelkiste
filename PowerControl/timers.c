/*
 * timers.c
 *
 *  Created on: 06.09.2021
 *      Author: philipp
 */

#include "system.h"
#include <avr/io.h>
#include <avr/interrupt.h>
#include "timers.h"

void startTimer0(uint8_t presc)
{
	TCNT0 = 0;
	TIFR |= 1 << TOV0;
	TCCR0 |= presc & 0x7;
}

void stopTimer0()
{
	TCCR0 = 0;
}

uint16_t getTimer0Value()
{
	if ((TIFR & 0x1) == 0x1)
	{
		// timer overflown
		return 1000*(255/F_CPU)*getTimer0Prescaler();
	}
	else
	{
		return 1000*(TCNT0/F_CPU)*getTimer0Prescaler();
	}
}

uint16_t getTimer0Prescaler()
{
	uint8_t psCode = TCCR0 & 0x7;
	switch (psCode)
	{
	case 1:
		return 1;
	case 2:
		return 8;
	case 3:
		return 64;
	case 4:
		return 256;
	case 5:
		return 1024;
	default:
		return 0;

	}
}

void startTimer1(uint8_t presc)
{
	//cli();
	TCNT1 = 0x0;
	OCR1A = 1000;
	//sei();
	TIFR |= (1 << TOV1);
	TIMSK |= (1 << OCIE1A);
	//TCCR1B |= (1 << WGM12);

	TCCR1B |= presc & 0x7;
}

void stopTimer1()
{
	TCCR1B = 0;
}

// return the direct timer value, which is in unit of 1.024ms
uint16_t getTimer1Value()
{
	if ((TIFR & (1 << TOV1)) == (1 << TOV1))
	{
		// timer overflown
		return 0xFFFF;
	}
	else
	{
		uint16_t tval;
		//cli();
		tval = TCNT1;
		//sei();
		return tval;
	}
}

uint16_t getTimer1Prescaler()
{
	uint8_t psCode = TCCR1B & 0x7;
	switch (psCode)
	{
	case 1:
		return 1;
	case 2:
		return 8;
	case 3:
		return 64;
	case 4:
		return 256;
	case 5:
		return 1024;
	default:
		return 0;

	}
}

