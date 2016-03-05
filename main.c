#include <avr/io.h>
//#include <avr/wdt.h>
#include <avr/interrupt.h>
#include <util/delay.h>
//#include <avr/eeprom.h>
//#include <avr/sleep.h>
//#include <avr/pgmspace.h>
//#include <string.h>
#include "uart.h"
#include "DS18S20Library/ds18S20.h"

#define FALSE         0
#define TRUE          1

#define TGLBIT(REG, BIT)   (REG ^= (1 << BIT))
#define CLRBIT(REG, BIT)   (REG &= ~(1 << BIT))
#define SETBIT(REG, BIT)   (REG |= (1 << BIT))
#define TSTBIT(REG, BIT)   (REG & (1 << BIT))

#define LED           5
#define OUT           PD2
#define ADJ_CH        0
#define TR_CH         1

TSDS18x20 DS18x20;
TSDS18x20 *pDS18x20 = &DS18x20;
int16_t ds_tempr = 0; // 0 - invalide value
int tr_tempr = 0; // 0 - invalide value
int out = 0;      // output [0..50] cycles 
int adj_val = 0;  // adjuster resistor

void ds_start_conversion()
{
	OWReset(pDS18x20);
	OWWriteByte(pDS18x20,SKIP_ROM);
	OWWriteByte(pDS18x20,CONVERT_T);
}

int ds_get_result()
{
	if(DS18x20_ReadScratchPad(pDS18x20)) {
		return DS18x20_TemperatureValue(pDS18x20);
	}
	return 0;
}

void adc_init()
{
	ADMUX = (1<<REFS1) | (1<<REFS0) | 1 << ADLAR | 0; // 0 ch
	ADCSRA = (1<<ADEN) | (1<<ADIE) | (1<<ADPS2) | (1<<ADPS1) | (1<<ADPS0);
}

void adc_start(int ch, int adlar)
{
	ADMUX &= 0xF0;
	ADMUX |= ch & 0x0F;

	if (adlar)
		SETBIT(ADMUX, ADLAR);
	else
		CLRBIT(ADMUX, ADLAR);

	ADCSRA |= (1<<ADSC);
}

void timer1_init()
{
	TCCR1A = 1 << WGM11 | 1 << WGM10;
	TCCR1B = 1 << WGM13 | 1 << WGM12 | 1 << CS12;
	OCR1A  = 1250; // 50 Hz
	TIMSK1 = 1 << OCIE1A;
}

ISR(ADC_vect)
{
	switch(ADMUX&0x0F) {
		case 0: // 0 channel is adjuster resistor
			adj_val = ADCH;
			adc_start(1,0);
			break;
		case 1: // 1 channel is termoresistor
			add_termo_value(ADC);
			adc_start(0,1);
			break;
	}
}


// 50 Hz
ISR(TIMER1_COMPA_vect)
{
	//TGLBIT(PORTB,LED);

	// ds stuff
	static int ds_cnt = 0;
	if (++ds_cnt > 40) {
		ds_cnt = 0;
		ds_tempr = ds_get_result();
		ds_start_conversion();
	}

	// output regulator
	static int duty_cnt = 0;
	if (++duty_cnt > 50) {
		duty_cnt = 0;
		//CLRBIT(PORTD,OUT);
		//CLRBIT(PORTB,PB3);
		//CLRBIT(PORTB,PB4);
	}

	if (duty_cnt < out) {
		//SETBIT(PORTD,OUT);
		SETBIT(PORTB,PB3);
		SETBIT(PORTB,PB4);
	} else {
		//CLRBIT(PORTD,OUT);
		CLRBIT(PORTB,PB3);
		CLRBIT(PORTB,PB4);
	}

	// PID stuff will be here
	out = adj_val/2;
}


void add_termo_value(int value)
{

}

int main(void) 
{
	pDS18x20->SensorModel = DS18B20Sensor;

	timer1_init();
	
	uart_init();
	adc_init();
	adc_start(0,1);
	printf("Start now!\n\r");
	
	printf("Searching for ds18b20! ");
	while(DS18x20_Init(pDS18x20, &PORTC, PC2)) {
		_delay_ms(1000);
		printf(".");
	} 
	ds_start_conversion();
	printf("\n\rds18b20 init ok.\n\r");
	
	SETBIT(DDRB, LED);
	SETBIT(PORTB,LED);
	SETBIT(DDRB, PB3);
	SETBIT(PORTB,PB3);
	SETBIT(DDRB, PB4);
	SETBIT(PORTB,PB4);
	
	sei();

	while (1) {
		_delay_ms(250);		
		//TGLBIT(PORTB,LED);
		printf("adj:%d T1:%d.%d T2:%d\n\r", adj_val, ds_tempr/16, ds_tempr%16, tr_tempr);
	}
	return 0;
}
