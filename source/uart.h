// uart.h
#include <avr/io.h>
#include <stdio.h>

#define sbi(var, mask)   ((var) |= (uint8_t)(1 << mask))
#define cbi(var, mask)   ((var) &= (uint8_t)~(1 << mask))

static int uart_putchar(char c, FILE *stream);
uint8_t uart_getchar(void);
void init_UART(int ubrr);
