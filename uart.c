#include <stdio.h>
#include <avr/io.h>

#include "uart.h"

#define BAUD_PRESCALLER (((F_CPU / (BAUDRATE * 16UL))) - 1)

int uart_putchar(char c, FILE *stream) {
	while(!(UCSR0A & (1<<UDRE0)));
	UDR0 = c;
	return 0;
}

int uart_getchar(FILE *sttream) {
	while(!(UCSR0A & (1<<RXC0)));
	return UDR0;
}

FILE uart_output = FDEV_SETUP_STREAM(uart_putchar, NULL, _FDEV_SETUP_WRITE);
FILE uart_input = FDEV_SETUP_STREAM(NULL, uart_getchar, _FDEV_SETUP_READ);
FILE uart_io = FDEV_SETUP_STREAM(uart_putchar, uart_getchar, _FDEV_SETUP_RW);

void uart_init(void) {

	UBRR0H = (uint8_t)(BAUD_PRESCALLER>>8);
	UBRR0L = (uint8_t)(BAUD_PRESCALLER);
	UCSR0B = (1<<RXEN0)|(1<<TXEN0);
	UCSR0C = ((1<<UCSZ00)|(1<<UCSZ01));

	stdout = &uart_output;
	stdin  = &uart_input;
}

