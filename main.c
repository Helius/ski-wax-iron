#include <avr/io.h>
//#include <avr/wdt.h>
#include <avr/interrupt.h>
#include <util/delay.h>
//#include <avr/eeprom.h>
//#include <avr/sleep.h>
#include <avr/pgmspace.h>
//#include <string.h>
#include "uart.h"
#include "DS18S20Library/ds18S20.h"

#define FALSE         0
#define TRUE          1

#define TGLBIT(REG, BIT)   (REG ^= (1 << BIT))
#define CLRBIT(REG, BIT)   (REG &= ~(1 << BIT))
#define SETBIT(REG, BIT)   (REG |= (1 << BIT))
#define TSTBIT(REG, BIT)   (REG & (1 << BIT))

#define LED           PB3
#define OUT           PB1
#define DS_LINE       PB2
#define ADJ_CH        0
#define TR_CH         1

int trm[17][2] ={
	{70,  7900},
	{75,  6850},
	{80,  5900},
	{85,  5130},
	{90,  4460},
	{95,  3850},
	{100, 3350},
	{105, 2930},
	{110, 2590},
	{115, 2232},
	{120, 1850},
	{125, 1620},
	{130, 1430},
	{135, 1250},
	{140, 1100},
	{145, 950},
	{150, 850}
};

uint16_t tr_convert_tempr(uint16_t val)
{
	if ((val < trm[0][1]) && (val > trm[16][1])) {
		for (int i = 0; i < 16; ++i) {
			if (val >= trm[i+1][1]) {
				return trm[i][0]*16 + (5*16 * (trm[i][1] - val))/(trm[i][1] - trm[i+1][1]);
			}
		}
	}
	return 0; // invalid value
}

TSDS18x20 DS18x20;
TSDS18x20 *pDS18x20 = &DS18x20;
uint16_t ds_tempr = 0;// 0 - invalide value
uint16_t tr_tempr = 0;// 0 - invalide value
uint16_t tpm_tr_tempr = 0;// 0 - invalide value
int out = 0;          // output [0..50] cycles 
int adj_val = 0;      // adjuster resistor
uint16_t ustavka = 0; // wanted temperature value
int diff = 0;         // ustavka - real
int pr = 0;           // dT/dt
int32_t intg = 0;

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
	switch(ADMUX & 0x0F) {
		case 0: // 0 channel is adjuster resistor
			adj_val = ADCH;
			adc_start(1,1);
			break;
		case 1: // 1 channel is termoresistor
			add_termo_value(ADCH);
			adc_start(0,1);
			break;
	}
}


// 50 Hz
ISR(TIMER1_COMPA_vect)
{
	// ds stuff
	static int ds_cnt = 0;

	// start conversion each 800ms
	if (++ds_cnt > 40) {
		ds_cnt = 0;
		ds_tempr = ds_get_result();
		ds_start_conversion();
		ustavka = (adj_val * 140)/180;
		do_pid();
	}

	// output regulator
	static int duty_cnt = 0;
	if (++duty_cnt > 50) {
		duty_cnt = 0;
	}

	if (duty_cnt < out) {
		SETBIT(PORTB,LED);
		CLRBIT(PORTB,OUT);
	} else {
		CLRBIT(PORTB,LED);
		SETBIT(PORTB,OUT);
	}

	// PID stuff will be here
	//out = adj_val/2;
}

// out [0..50]
int Kprop = 4;
int Kdiff = 2;
int Kintg = 2000000;
void do_pid()
{
	static int pr_cnt = 0;
	static int prev_diff = 0;
	static int first_start = 1;
#define INT_LEN 500
	static int integral[INT_LEN] = {0};
	static int integral_cnt = 0;

	diff = ustavka*16 - tr_tempr;

	++pr_cnt;
	if (pr_cnt == 4) {
		pr = diff - prev_diff;
		prev_diff = diff;
		pr_cnt = 0;
		first_start++;
	}

	integral[integral_cnt] = diff;
	integral_cnt++;
	if (integral_cnt > INT_LEN)
		integral_cnt = 0;
	for (int i = 0; i < INT_LEN; ++i) {
		intg += integral[i];
	}
	
	if ((first_start < 2) && (tr_tempr > 140*16) && (ds_tempr > 140*16)) {
		out = 0;
	} else {
		out = diff/Kprop + pr/Kdiff;// + intg/Kintg;
	}




}

void add_termo_value(int value)
{
	static uint16_t summ = 0;
	static int cnt = 0;
	summ += value;
	++cnt;
	if (cnt > 30){
		cnt = 0;
		tpm_tr_tempr = summ;
		tr_tempr = tr_convert_tempr(summ);
		summ = 0;
	}
}

int main(void) 
{
	pDS18x20->SensorModel = DS18B20Sensor;

	timer1_init();
	
	uart_init();
	adc_init();
	adc_start(0,1);
	printf("Start now!\n\r");
	
	// led
	SETBIT(DDRB, LED);
	SETBIT(PORTB,LED);
	// output
	SETBIT(DDRB, OUT);
	SETBIT(PORTB,OUT);
	
	printf("Searching for ds18b20! ");
	while(DS18x20_Init(pDS18x20, &PORTB, DS_LINE)) {
		_delay_ms(1000);
		printf(".");
	} 
	ds_start_conversion();
	printf("\n\rds18b20 init ok.\n\r");
	
	sei();

	while (1) {
		_delay_ms(800);		
		//TGLBIT(PORTB,LED);
		printf("adj:%d T16C:%d T2:%d diff:%d out:%d pr:%d u:%d i:%d\n\r", adj_val, ds_tempr, tr_tempr, diff, out, pr, ustavka, intg/Kintg);
	}
	return 0;
}
