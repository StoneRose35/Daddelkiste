/*
 * DaddelkistePowercontrol.c
 *
 * Created: 03.05.2021 18:04:11
 * Author : fuerh
 */ 

#include <avr/io.h>
#include <avr/interrupt.h>
#include <avr/sleep.h>

#define F_CPU 1000000L
#include <util/delay.h>

#define BAT_TH 644
#define I2C_ADDRESS 10

#define nop() asm volatile("nop")



// pinout
//rpi 3.3v sense: ADC0 (pin 23)
// battery voltage sense: ADC1 (pin24)
// switch: int0/pd2 (pin4)
// rpi global_en: PB0 (pin14)
// rpi shutdown: PB1 (pin15)
// audio amp switch: PD7 (pin 13) 

uint8_t last_cmd;
uint16_t bat_voltage;
uint8_t go_to_sleep;


void delayAndReturn()
{
		_delay_ms(200.0);
		_delay_ms(200.0);
		sei();
}

void readBatVoltage()
{
			// read battery voltage
			ADMUX |= (1 << MUX0);
			ADCSRA |= (1 << ADSC);
			while ((ADCSRA & (1 << ADIF)) == 0)
			{
			}
			bat_voltage = ADC;
}


int main(void)
{
    // setup adc
	ADCSRA = (1 << ADEN) | (1 << ADPS1) | (1 << ADPS0);
	ADMUX = (1 << REFS0);
	
	// setup external interrupt
	PORTD |= (1 << PORTD2);
	GICR |= (1 << INT0);
	
	// initialize i2c to listen to address 10
	TWAR &= (I2C_ADDRESS << 1);
	
	// audio amp switch as output
	DDRD |= (1 << DDD7);
	
	go_to_sleep = 1;

	
	
	uint8_t d_cnt = 0;
	while (d_cnt < 30)
	{
		_delay_ms(100);
		d_cnt++;
	}
	
	
	TWCR |= (1 << TWEA) | (1 << TWEN) | (1 << TWIE);
	sei();

    while (1) 
    {
		
			if (go_to_sleep==1)
			{
				set_sleep_mode(SLEEP_MODE_PWR_DOWN);
				sleep_mode();	
			}
    }
}


ISR ( INT0_vect )
{

	uint16_t rpi_sense;
	
	cli();
	
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
		TWCR &= ~((1 << TWEA) | (1 << TWEN) | (1 << TWIE));
		PORTD &= ~(1 << PORTD7); // audio amp off
		PORTB &= ~(1 << PORTB1);
		DDRB |= (1 << DDB1);
		_delay_ms(100.0);
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
			_delay_ms(100.0);
			DDRB &= ~(1 << DDB0);
			
			
				uint8_t d_cnt = 0;
				while (d_cnt < 30)
				{
					_delay_ms(100);
					d_cnt++;
				}
			// switch on twi communication
			TWCR |= (1 << TWEA) | (1 << TWEN) | (1 << TWIE);
		}
	}
	sei();
	//delayAndReturn();
}


ISR ( TWI_vect )
{
	cli();
	go_to_sleep = 1;
	if ((TWSR & 0xF8) == 0x60)
	{
		// got own address and request to write
		// wait for command
		TWCR |= (1 << TWEA) | (1 << TWINT)| (1 << TWEN);
		go_to_sleep = 0;
	}
	else if ((TWSR & 0xF8) == 0x80)
	{
		// command has been received
		// command are: 0: turn off audio amp
		//              1: turn on audio amp
		//              2: send battery voltage
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
			readBatVoltage();
		}
		TWCR |= (1 << TWEA) | (1 << TWINT)| (1 << TWEN);
	}
	else if ((TWSR & 0xF8) == 0xA8 && last_cmd == 2)
	{
		// read request has been received, last command is 2 --> send MSB of voltage reading
		TWDR = (bat_voltage >> 8) & 0xFF;
		TWCR |= (1 << TWEA) | (1 << TWINT) | (1 << TWEN);
		go_to_sleep = 0;
	}
	else if ((TWSR & 0xF8) == 0xB8 && last_cmd == 2)
	{
		// data has been sent, ACK has been received, last command is 2 --> send LSB of voltage reading
		TWDR = bat_voltage & 0xFF;
		TWCR &= ~(1 << TWEA); 
		TWCR |= (1 << TWINT) | (1 << TWEN);
	}
	else if ((TWSR & 0xF0) == 0xC0 && last_cmd == 2)
	{
		TWCR |= (1 << TWEA) | (1 << TWINT) | (1 << TWEN);
	}
	else
	{
		TWCR |= (1 << TWEA) | (1 << TWINT) | (1 << TWEN);
	}
	sei();
}


