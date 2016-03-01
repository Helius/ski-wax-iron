#include <avr/io.h>
//#include <avr/wdt.h>
//#include <avr/interrupt.h>
#include <util/delay.h>
//#include <avr/eeprom.h>
//#include <avr/sleep.h>
//#include <avr/pgmspace.h>
//#include <string.h>
#include "uart.h"
#include "DS18S20Library/ds18S20.h"

#define FALSE         0
#define TRUE          1

#define LED           5

#define TGLBIT(REG, BIT)   (REG ^= (1 << BIT))
#define CLRBIT(REG, BIT)   (REG &= ~(1 << BIT))
#define SETBIT(REG, BIT)   (REG |= (1 << BIT))
#define TSTBIT(REG, BIT)   (REG & (1 << BIT))

int main(void) 
{
	TSDS18x20 DS18x20;
	TSDS18x20 *pDS18x20 = &DS18x20;
	pDS18x20->SensorModel = DS18B20Sensor;
	
	uart_init();
	printf("Hellow!\n\r");
	
	while(DS18x20_Init(pDS18x20, &PORTB, PB3)) {
		printf("Error!!! Can not find ds18b20!\n\r");
		_delay_ms(1000);
	} 
	printf("ds18b20 init ok.\n\r");
	

	SETBIT(DDRB, LED);
	SETBIT(PORTB,LED);


	while (1) {
		_delay_ms(500);		
		TGLBIT(PORTB,LED);
		// Initiate a temperature conversion and get the temperature reading
		if (DS18x20_MeasureTemperature(pDS18x20))
		{
			// Send the temperature over serial port
			int tempr = DS18x20_TemperatureValue(pDS18x20);
			printf("Current Temperature is: %d.%d\n\r", tempr/16,tempr%16);
		}
	}
	return 0;
}
