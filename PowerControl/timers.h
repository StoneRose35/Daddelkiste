/*
 * timers.h
 *
 *  Created on: 06.09.2021
 *      Author: philipp
 */


#ifndef TIMERS_H_
#define TIMERS_H_

#define NO_PRESC 1
#define PRESC_8 2
#define PRESC_64 3
#define PRESC_256 4
#define PRESC_1024 5

void startTimer0(uint8_t presc);

uint16_t getTimer0Value();

uint16_t getTimer0Prescaler();

void stopTimer0();

void startTimer1(uint8_t presc);

uint16_t getTimer1Value();

uint16_t getTimer1Prescaler();

void stopTimer1();

#endif /* TIMERS_H_ */
