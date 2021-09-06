/*
 * wdt.c
 *
 *  Created on: 06.09.2021
 *      Author: philipp
 */

#include "system.h"
#include <avr/io.h>

#include "wdtConfig.h"

void enableWdt()
{
	WDTCR = (1 << WDCE) | (1 << WDE);
	WDTCR = (1 << WDE) | (1 << WDP2) | (1 << WDP0);
}

void disableWdt()
{
	WDTCR = (1 << WDCE) | (1 << WDE);
	WDTCR &= ~(1 << WDE);
}
