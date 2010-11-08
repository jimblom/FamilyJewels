// uart.c
#include "uart.h"

void init_UART(int ubrr)
{
	static FILE mystdout = FDEV_SETUP_STREAM(uart_putchar, NULL, _FDEV_SETUP_WRITE);
	
	stdout = &mystdout; //Required for printf init

	UBRR0H = (ubrr) >> 8;
	UBRR0L = ubrr;
	UCSR0B = (1<<RXEN0)|(1<<TXEN0);
	UCSR0C = (3<<UCSZ00);
	UCSR0A = (1<<U2X0);

	TCCR2B = (1<<CS21);
}

static int uart_putchar(char c, FILE *stream)
{
    if (c == '\n') uart_putchar('\r', stream);

    loop_until_bit_is_set(UCSR0A, UDRE0);
    UDR0 = c;

    return 0;
}

uint8_t uart_getchar(void)
{
    while( !(UCSR0A & (1<<RXC0)) );
    return(UDR0);
}
