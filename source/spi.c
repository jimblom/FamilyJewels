// spi.c
#include "spi.h"

char rxdata(void)
{
	//while((SPSR&0x80) == 0x80);
	SPDR = 0x55;
	while((SPSR&0x80) == 0x00);

	return SPDR;
}

void txdata(char data)
{
	//while((SPSR&0x80) == 0x80);
	SPDR = data;
	while((SPSR&0x80) == 0x00);
}

void init_SPI(void)
{
	sbi(SPCR,MSTR); //make SPI master
//	cbi(SPCR,CPOL); sbi(SPCR,CPHA); //SCL idle low, sample data on rising edge
	cbi(SPCR,CPOL); cbi(SPCR,CPHA); //SCL idle low, sample data on rising edge
	cbi(SPCR,SPR1);cbi(SPCR,SPR0);cbi(SPSR,SPI2X); //Fosc/4 is SPI frequency
	sbi(SPCR,SPE); //enable SPI
}