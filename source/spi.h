// SPI.h
#include <avr/io.h>

#define sbi(var, mask)   ((var) |= (uint8_t)(1 << mask))
#define cbi(var, mask)   ((var) &= (uint8_t)~(1 << mask))

char rxdata(void);
void txdata(char data);
void init_SPI(void);