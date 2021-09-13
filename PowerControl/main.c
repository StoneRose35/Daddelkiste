/*
 * DaddelkistePowercontrol.c
 *
 * Created: 03.05.2021 18:04:11
 * Author : fuerh
 */ 


#include "system.h"
#include <avr/io.h>
#include <avr/interrupt.h>
#include <avr/sleep.h>
#include <avr/wdt.h>
#include <util/delay.h>
#include "timers.h"
#include "wdtConfig.h"

#define nop() asm volatile("nop")

// task id's for the main, denote individual bits
#define HANDLE_BUTTON 1
#define HANDLE_BUTTON_LONG 2
#define READ_BATTERY_VOLTAGE 3
#define SEND_BATTERY_VOLTAGE_MSB 4
#define GO_TO_SLEEP 5
#define SEND_BUTTON_PUSH_LENGTH 6
#define SEND_BATTERY_VOLTAGE_LSB 7
#define NOTHING 0

// button press length states

// pinout
//rpi 3.3v sense: ADC0 (pin 23)
// battery voltage sense: ADC1 (pin24)
// switch: int0/pd2 (pin4)
// rpi global_en: PB0 (pin14)
// rpi shutdown: PB1 (pin15)
// audio amp switch: PD7 (pin 13) 

volatile uint8_t last_cmd;
volatile uint16_t bat_voltage;
volatile uint8_t task;
volatile uint8_t buttonPushLength=0; // 1: short, 2: long
volatile uint8_t timerOverflown = 0;
volatile uint8_t transmissionOngoing = 0;


void readBatVoltage()
{
			// read battery voltage
			ADMUX &= ~0xF;
			ADMUX |= (1 << MUX0);
			ADCSRA |= (1 << ADSC) | (1 << ADIF);
			while ((ADCSRA & (1 << ADIF)) == 0)
			{
			}
			bat_voltage = ADC;
			task &= ~(1 << READ_BATTERY_VOLTAGE);
}

void sendBatteryMsb()
{
	TWDR = (bat_voltage >> 8) & 0xFF;
	TWCR |= (1 << TWEA) | (1 << TWINT) | (1 << TWEN);
	task &= ~(1 << SEND_BATTERY_VOLTAGE_MSB);
	transmissionOngoing = 1;
}

void sendBatteryLsb()
{
		TWDR = bat_voltage & 0xFF;
		TWCR &= ~(1 >> TWEA);
		TWCR |= (1 << TWINT) | (1 << TWEN);
		task &= ~(1 << SEND_BATTERY_VOLTAGE_LSB);
		transmissionOngoing = 0;
}

void sendButtonPushLength()
{
	TWDR = buttonPushLength;
	TWCR &= ~(1 >> TWEA);
	TWCR |= (1 << TWINT) | (1 << TWEN);
	task &= ~(1 << SEND_BUTTON_PUSH_LENGTH);
	transmissionOngoing = 0;
}

void handleButtonPush()
{
	uint16_t rpi_sense;
	
	// read rpi 3.3v sense
	ADMUX &= ~0xF;
	ADCSRA |= (1 << ADSC)| (1 << ADIF);
	while ((ADCSRA & (1 << ADIF)) == 0)
	{
	}
	rpi_sense = ADC;

	if (rpi_sense > 100)
	// raspberry pi  is on --> switch off
	{
		// switch off TWI/i2c
		TWCR &= ~((1 << TWEN) | (1 << TWEA));
		PORTD &= ~0x1;
		PORTD &= ~(1 << PD7); // audio amp off
		PORTB &= ~(1 << PB1);
		DDRB |= (1 << DDB1);
		_delay_ms(10.0);
		DDRB &= ~(1 << DDB1);
		
	}
	else
	{
		readBatVoltage();
		if (bat_voltage > BAT_TH)
		{
			PORTD &= ~0x2;
			PORTD &= ~(1 << PD7); // audio amp off
			PORTB &= ~(1 << PB0);
			DDRB |= (1 << DDB0);
			_delay_ms(10);
			DDRB &= ~(1 << DDB0);
			
			// read the SCL as an analog input and wait until it remains high for at two consecutive measurements 40 ms  apart
			// then switch the twi back on
			uint8_t n_highs = 0;
					
			while (n_highs < 2)
			{
				wdt_reset();
				TCNT2 = 0;
				TCCR2 |= 0x7;
				while (TCNT2 < 39)
				{
				}
				ADMUX &= ~0xF;
				ADMUX |= (1 << MUX2) | (1 << MUX0);
				ADCSRA |= (1 << ADSC) | (1 << ADIF);
				while ((ADCSRA & (1 << ADIF)) == 0)
				{
				}
				rpi_sense = ADC;
				if (rpi_sense > 800)
				{
					n_highs++;
				}
				else
				{
					n_highs = 0;
				}
			}
			PORTD |= 1;
			ADMUX &= ~0xF;
			TWCR |= (1 << TWEN) | (1 << TWEA) | (1 << TWINT);
		}
	}
	task &= ~(1 << HANDLE_BUTTON);
	//PORTD |= 0x2;
}

int main(void)
{
	uint16_t scl_level;
	  	wdt_reset();
    // setup adc
	ADCSRA = (1 << ADEN) | (1 << ADPS1) | (1 << ADPS0);
	ADMUX = (1 << REFS0);
	
	// configure debug leds on pd0 and pd1
	DDRD |= 0x3;
	PORTD &= ~0x3;
	
	// setup external interrupt
	PORTD |= (1 << PD2);
	GICR |= (1 << INT0);
	MCUCR |= 0; // active on low level

	// initialize i2c to listen to address 10
	TWAR = (I2C_ADDRESS << 1);
	
	// audio amp switch as output
	DDRD |= (1 << DDD7);
	
	TCNT1 = 0;
	
	TWCR |= (1 << TWIE);	
	// check initially if arduino is ready, if so: enable TWI/i2c
	ADMUX &= ~0xF;
	ADMUX |= (1 << MUX2) | (1 << MUX0);
	ADCSRA |= (1 << ADSC) | (1 << ADIF);
	while ((ADCSRA & (1 << ADIF)) == 0)
	{
	}
	scl_level = ADC;
	if (scl_level > 800)
	{
		PORTD |= 1;
		ADMUX &= ~0xF;
		TWCR |= (1 << TWEN) | (1 << TWEA) | (1 << TWINT);
	}
	
	// init Watchdog
	enableWdt();

	
	sei();

    while (1) 
    {
		PORTD |=2;
    	if ((task & (1 << HANDLE_BUTTON)) == (1 << HANDLE_BUTTON))
    	{
    		handleButtonPush();
    	}
    	if ((task & (1 << READ_BATTERY_VOLTAGE)) == (1 << READ_BATTERY_VOLTAGE))
    	{
    		readBatVoltage();
    	}
    	if ((task & (1 << SEND_BATTERY_VOLTAGE_MSB)) == (1 << SEND_BATTERY_VOLTAGE_MSB))
    	{
    		sendBatteryMsb();
    	}
    	if ((task & (1 << SEND_BUTTON_PUSH_LENGTH)) == (1 << SEND_BUTTON_PUSH_LENGTH))
    	{
    		sendButtonPushLength();
    	}
		/*
    	if ((TCCR1B & 0x7) == 0 && transmissionOngoing == 0 && task == 0)
    	{
			
			set_sleep_mode(SLEEP_MODE_PWR_DOWN);
			disableWdt();
			sei();
			sleep_mode();
    	}
		*/
    	wdt_reset();

    }
}

ISR ( TIMER1_COMPA_vect )
{
	uint8_t buttonState;
	//PORTD &= ~0x2;
	buttonState = PIND & (1 << PD2);
	if (buttonState == 0)
	{
		 buttonPushLength=2;
		 TCNT1=0;
		 timerOverflown = 1;
	}
	else
	{
		timerOverflown = 0;
		TCCR1B &= ~0x7;
	}
	//PORTD |= 0x2;
}


ISR(INT0_vect)
{
	//PORTD &= ~0x2;
	if ((MCUCR & 0x3) == 0x0) // low level (result of pushing down initially)
	{
		uint16_t t_val;
		t_val = getTimer1Value();
		if (((TCCR1B & 0x7) == 0x0 || t_val > 50) && timerOverflown == 0)
		{
			startTimer1(PRESC_1024);
			MCUCR |= 0x3; // set trigger to rising edge
			handleButtonPush();
		}
		
	}
	else // rising edge
	{
		uint16_t t_val;
		t_val = getTimer1Value();

		if (t_val > 50 && timerOverflown == 0)
		{
			// handle short press
			buttonPushLength = 1;
		}
		else if (timerOverflown == 1)
		{
			buttonPushLength = 2;
		}
		timerOverflown = 0;
		TCNT1 = 0;
		MCUCR &=  ~0x3; // set trigger back to low
	}
	//PORTD |= 0x2;
}


ISR ( TWI_vect )
{
	//PORTD &= ~0x2;
	if ((TWSR & 0xF8) == 0x60)
	{
		// got own address and request to write
		// wait for command
		TWCR |= (1 << TWEA) | (1 << TWINT)| (1 << TWEN);
		transmissionOngoing = 1;
	}
	else if ((TWSR & 0xF8) == 0x80)
	{
		// command has been received
		// command are: 0: turn off audio amp
		//              1: turn on audio amp
		//              2: send battery voltage
		//              3: return button push duration
		last_cmd = TWDR;
		if (last_cmd == 0)
		{
			PORTD &= ~(1 << PD7);
			transmissionOngoing = 0;
		}
		else if (last_cmd == 1)
		{
			PORTD |= (1 << PD7);
			transmissionOngoing = 0;
		}
		else if (last_cmd == 2)
		{
			task |= (1 << READ_BATTERY_VOLTAGE);
			transmissionOngoing = 1;
		}
		TWCR |= (1 << TWINT) | (1 << TWEN) | (1 << TWEA);
	}
	else if ((TWSR & 0xF8) == 0xA8 && last_cmd == 2)
	{
		// read request has been received, last command is 2 --> send MSB of voltage reading
		sendBatteryMsb();
		transmissionOngoing = 1;
	}
	else if ((TWSR & 0xF8) == 0xA8 && last_cmd == 3)
	{
		// read request has been received, last command is 3 --> send button Push length
		sendButtonPushLength();
	}
	else if ((TWSR & 0xF8) == 0xB8 && last_cmd == 2)
	{
		// data has been sent, ACK has been received, last command is 2 --> send LSB of voltage reading
		sendBatteryLsb();
	}
	else if ((TWSR & 0xF0) == 0xC0)
	{
		// last data byte has been transmitted successfully, not ack or ack (both c0 or c8 match) have been received
		TWCR |= (1 << TWINT) | (1 << TWEN) | (1 << TWEA);
		transmissionOngoing = 0;
	}
	else
	{
		TWCR |= (1 << TWINT) | (1 << TWEN) | (1 << TWEA);
		transmissionOngoing = 0;
	}
	//PORTD |= 0x2;
}


