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
#define NOTHING 0

// button press length states

// pinout
//rpi 3.3v sense: ADC0 (pin 23)
// battery voltage sense: ADC1 (pin24)
// switch: int0/pd2 (pin4)
// rpi global_en: PB0 (pin14)
// rpi shutdown: PB1 (pin15)
// audio amp switch: PD7 (pin 13) 

uint8_t last_cmd;
uint16_t bat_voltage;
uint8_t task;
uint8_t buttonPushLength=0; // 1: short, 2: long



/*
void delayAndReturn()
{
		_delay_ms(200.0);
		_delay_ms(200.0);
		sei();
}
*/

void readBatVoltage()
{
			// read battery voltage
			ADMUX |= (1 << MUX0);
			ADCSRA |= (1 << ADSC);
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
}

void sendButtonPushLength()
{
	TWDR = buttonPushLength;
	TWCR |= (1 << TWEA) | (1 << TWINT) | (1 << TWEN);
	task &= ~(1 << SEND_BUTTON_PUSH_LENGTH);
}

void handleButtonPush()
{
	uint16_t rpi_sense;
	// read rpi 3.3v sense
	ADMUX &= ~(1 << MUX0);
	ADCSRA |= (1 << ADSC);
	while ((ADCSRA & (1 << ADIF)) == 0)
	{
	}
	rpi_sense = ADC;

	if (rpi_sense > 100)
	// raspberry pi  is on --> switch off
	{
		// switch off twi communication
		//TWCR &= ~((1 << TWEA) | (1 << TWEN) | (1 << TWIE));
		PORTD &= ~(1 << PORTD7); // audio amp off
		PORTB &= ~(1 << PORTB1);
		DDRB |= (1 << DDB1);
		_delay_ms(10.0);
		DDRB &= ~(1 << DDB1);


	}
	else
	{
		readBatVoltage();
		if (bat_voltage > BAT_TH)
		{
			PORTD &= ~(1 << PORTD7); // audio amp off
			PORTB &= ~(1 << PORTB0);
			DDRB |= (1 << DDB0);
			_delay_ms(10);
			DDRB &= ~(1 << DDB0);


				//uint8_t d_cnt = 0;
				//while (d_cnt < 30)
				//{
				//	_delay_ms(100);
				//	d_cnt++;
				//}
			// switch on twi communication
			//TWCR |= (1 << TWEA) | (1 << TWEN) | (1 << TWIE);
		}
	}
	task &= ~(1 << HANDLE_BUTTON);
}

int main(void)
{
    // setup adc
	ADCSRA = (1 << ADEN) | (1 << ADPS1) | (1 << ADPS0);
	ADMUX = (1 << REFS0);
	
	// setup external interrupt
	PORTD |= (1 << PORTD2);
	GICR |= (1 << INT0);
	MCUCR |= 0; // active on low level

	// initialize i2c to listen to address 10
	TWAR &= (I2C_ADDRESS << 1);
	
	// audio amp switch as output
	DDRD |= (1 << DDD7);
	
	// init Watchdog
	enableWdt();

	task |= (1 << GO_TO_SLEEP);

	
	TWCR |= (1 << TWEA) | (1 << TWEN) | (1 << TWIE);
	sei();

    while (1) 
    {

    	if ((task & (1 << HANDLE_BUTTON)) == (1 << HANDLE_BUTTON))
    	{
    		handleButtonPush();
    		//task |= (1 << GO_TO_SLEEP);
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
    	if (task == (1 << GO_TO_SLEEP))
    	{
			set_sleep_mode(SLEEP_MODE_PWR_DOWN);
			//task &= ~(1 << GO_TO_SLEEP);
			disableWdt();
			sleep_mode();

    	}
    	wdt_reset();

    }
}


ISR ( INT0_vect )
{
	cli();

	if ((MCUCR & 0x3) == 0x0) // low level (result of pushing down initially)
	{
		uint16_t t_val;
		t_val = getTimer1Value();
		if ((TCCR1B & 0x7) == 0x0 || t_val > 50)
		{
			startTimer1(PRESC_1024);
			MCUCR |= 0x3; // set trigger to rising edge
			task &= ~(1 << GO_TO_SLEEP);
			task |= (1 << HANDLE_BUTTON);
		}

	}
	else // rising edge
	{
		uint16_t t_val;
		t_val = getTimer1Value();
		if (t_val > 1000)
		{
			// handle long press
			buttonPushLength = 2;
			// switch trigger edge and restart timer
			startTimer1(PRESC_1024);
			MCUCR &= ~0x3; // set trigger back to low
			task |= (1 << GO_TO_SLEEP);
		}
		else if (t_val > 50)
		{
			// handle short press
			buttonPushLength = 1;
			// switch trigger edge and restart timer
			startTimer1(PRESC_1024);
			MCUCR &=  ~0x3; // set trigger back to low
			task |= (1 << GO_TO_SLEEP);
		}
	}

	sei();
}


ISR ( TWI_vect )
{
	cli();
	if ((TWSR & 0xF8) == 0x60)
	{
		// got own address and request to write
		// wait for command
		TWCR |= (1 << TWEA) | (1 << TWINT)| (1 << TWEN);
		task &= ~(1 << GO_TO_SLEEP);
	}
	else if ((TWSR & 0xF8) == 0x80)
	{
		// command has been received
		// command are: 0: turn off audio amp
		//              1: turn on audio amp
		//              2: send battery voltage
		//              3: return button push duration
		last_cmd = TWDR;
		if (TWDR == 0)
		{
			PORTD &= ~(1 << PORTD7);
		}
		else if (TWDR == 1)
		{
			PORTD |= (1 << PORTD7);
		}
		else if (TWDR == 2)
		{
			task |= (1 << READ_BATTERY_VOLTAGE);
		}
		TWCR |= (1 << TWEA) | (1 << TWINT)| (1 << TWEN);
	}
	else if ((TWSR & 0xF8) == 0xA8 && last_cmd == 2)
	{
		// read request has been received, last command is 2 --> send MSB of voltage reading
		task |= (1 << SEND_BATTERY_VOLTAGE_MSB);
		task &= ~(1 << GO_TO_SLEEP);
	}
	else if ((TWSR & 0xF8) == 0xA8 && last_cmd == 3)
	{
		// read request has been received, last command is 3 --> send button Push length
		task |= (1 << SEND_BUTTON_PUSH_LENGTH);
		task |= (1 << GO_TO_SLEEP);
	}
	else if ((TWSR & 0xF8) == 0xB8 && last_cmd == 2)
	{
		// data has been sent, ACK has been received, last command is 2 --> send LSB of voltage reading
		TWDR = bat_voltage & 0xFF;
		TWCR &= ~(1 << TWEA); 
		TWCR |= (1 << TWINT) | (1 << TWEN);
		task |= (1 << GO_TO_SLEEP);
	}
	else if ((TWSR & 0xF8) == 0xC8 && last_cmd == 2)
	{
		// last data byte has been transmitted successfully
		TWCR |= (1 << TWEA) | (1 << TWINT) | (1 << TWEN);
		task |= (1 << GO_TO_SLEEP);
	}
	else
	{
		TWCR |= (1 << TWEA) | (1 << TWINT) | (1 << TWEN);
		task |= (1 << GO_TO_SLEEP);
	}
	sei();
}


