/*
	Created on: July 22, 2010
	By: Jim Lindblom
*/

//======================//
//       Includes		//
//======================//
#include <avr/io.h>
#include <stdlib.h>
#include "spi.h"
#include "uart.h"
#include "bejeweled.h"

//======================//
//		  Macros		//
//======================//

#define sbi(var, mask)   ((var) |= (uint8_t)(1 << mask))
#define cbi(var, mask)   ((var) &= (uint8_t)~(1 << mask))

//======================//
// 	 Pin Definitions	//
//======================//
#define CS 2

//======================//
//  Golbal Definitions	//
//======================//
#define BOARD_SIZE 8

//======================//
// Function Definitions	//
//======================//
void ioinit(void);
void delay_ms(uint16_t x);
void delay_us(uint16_t x);

void generateSeed(void);
void initBoard(void);
void printBoard(void);

//======================//
//   Global Variables	//
//======================//
char currentBoard[BOARD_SIZE][BOARD_SIZE] = {
{0,0,0,0,0,0,0,0},
{0,0,0,0,0,0,0,0},
{0,0,0,0,0,0,0,0},
{0,0,0,0,0,0,0,0},
{0,0,0,0,0,0,0,0},
{0,0,0,0,0,0,0,0},
{0,0,0,0,0,0,0,0},
{0,0,0,0,0,0,0,0}};
char newBoard[BOARD_SIZE][BOARD_SIZE];

//======================//
//	       Main			//
//======================//
int main(void)
{
	uint8_t userInput[4];
	uint8_t row1, row2, column1, column2;
	uint8_t count = 0;
	char temp;
	
	ioinit();		// Initialize I/O
	initBoard();	// Create a random 8x8 board
	
	// Make new board equal to current board
	for (uint8_t i=0; i<BOARD_SIZE; i++)
	{
		for (uint8_t j=0; j<BOARD_SIZE; j++)
		{
			newBoard[i][j] = currentBoard[i][j];
		}
	}
	
	// loop 0 waiting for the game to start - does not start till correct move is made
	while(1)
	{
		printBoard();
		// wait for user input store as button 0
		// wait for user input store as button 1
		printf("Enter row column row column...\n");
		for (unsigned int i = 0; i<4; i++)
		{
			userInput[i] = uart_getchar() - 0x30;
			printf("%d", userInput[i]);
			// 0 - row 1
			// 1 - column 1
			// 2 - row 2
			// 3 - column 2
		}
		row1 = userInput[0];
		column1 = userInput[1];
		row2 = userInput[2];
		column2 = userInput[3];
		
		printf("\n\n");
		
		// if buttons legal (next to, or on top/bottom of eachother)
		// First check horizontal move
		if((row1 == row2)&&((column1==column2+1)||(column1==column2-1)))
		{
			newBoard[row1][column1] = currentBoard[row1][column2];
			newBoard[row1][column2] = currentBoard[row1][column1];
			
			printf("Horizontal move\n");
			printf("Swapping %d with %d\n", currentBoard[row1][column1], currentBoard[row2][column2]);
			// Check if gems match
			// Check to left
			count = 0;
			while((column1-count>=0)&&(currentBoard[row1][column2]==currentBoard[row1][column1-count-1]))
			{
				count++;
			}
			printf("left count==%d\n", count);
			
			// Delete matching gems
			// move the above gems down
			// generate new gems
			// increment score
			// exit loop 0
		}
		// Then check vertical move
		else if ((column1 == column2)&&((row1==row2+1)||(row1==row2-1)))
		{
			printf("Vertical move\n");
			printf("Swapping %d with %d\n", currentBoard[userInput[0]][userInput[1]], currentBoard[userInput[2]][userInput[3]]);
			uint8_t count = 0;
			while(((userInput[1]-count)>=0)||((currentBoard[userInput[0]][userInput[1]]==currentBoard[userInput[0]][userInput[1-count]])))
				count++;
			printf("count == %d\n", count);
		}
		// if move is incorrect
		else
		{
			// shortly blink incorrect gems
			// go to top of loop 0
			printf("Illegal move, please try again\n");
		}
				
	}
	// end loop 0
	
	// start timer constantly (interrupt) decrement timer, check for end
	
	// loop 1
		// wait for user input store as button 0
		// wait for user input store as button 1
		// if buttons legal (next to, or on top/bottom of eachother)
			// if move is correct
				// Delete matching gems
				// move the above gems down
				// generate new gems
				// increment score
				// exit loop 0
			// if move is incorrect
				// shortly blink incorrect gems
	// end loop 1
	
	while(1)
	{
	}
}

void ioinit (void)
{
	int myUBRR = 103;	// Results in 9600bps@8MHz or 19200bps@16MHz
	
    //1 = output, 0 = input
    DDRB = 0b11101101; //MISO input, PB1 input
    DDRC = 0xFF; // All outputs
    DDRD = 0b11111110; //PORTD (RX on PD0)
	
	init_UART(myUBRR);
//	init_SPI();
//	sbi(PORTB,CS);	// CS Idle High
}

void initBoard()
{
	generateSeed();
	
	// Fill board with random numbers between 1 and 7
	for (uint8_t i=0; i<BOARD_SIZE; i++)
	{
		for (uint8_t j=0; j<BOARD_SIZE; j++)
		{
			currentBoard[i][j] = rand()%7;
		}
	}
	
	// Make sure that there are no repetitions of 3
	for (uint8_t i=0; i<BOARD_SIZE; i++)
	{
		for (uint8_t j=0; j<BOARD_SIZE; j++)
		{
			while ((currentBoard[i][j]==currentBoard[i-1][j])&&(currentBoard[i][j]==currentBoard[i-2][j])&&(i>=2))
			{
				currentBoard[i][j] = rand()%7;
			}
			while ((currentBoard[i][j]==currentBoard[i][j-1])&&(currentBoard[i][j]==currentBoard[i][j-2])&&(j>=2))
			{
				currentBoard[i][j] = rand()%7;
			}
		}
	}
}

void generateSeed(void)
{
	unsigned long lotsOfADs = 0;
	
	ADMUX = 0x46;
	
	for (unsigned int i = 0; i<32; i++)
	{
		//ADCSRA = ADCSRA | (1<<ADSC);	// Start ADC conversion
		ADCSRA = (1 << ADEN)|(1 << ADSC)|(1<<ADPS2)|(1<<ADPS1);
		while(!(ADCSRA & (1<<ADIF)))	// Wait for AD interrupt flag
			;	
		lotsOfADs += (ADCL | ((ADCH)<<8));
	}
	
	srand(lotsOfADs);
}

void printBoard(void)
{
	// Print board out to terminal
	printf("\n");
	for (uint8_t i=0; i<BOARD_SIZE; i++)
	{
		for (uint8_t j=0; j<BOARD_SIZE; j++)
		{
			printf("%d", currentBoard[i][j]);
		}
		printf("\n");
	}
}

//General short delays
void delay_ms(uint16_t x)
{
    for (; x > 0 ; x--)
        delay_us(1000);
}

//General short delays
void delay_us(uint16_t x)
{
    while(x > 256)
    {
        TIFR2 = (1<<TOV2); //Clear any interrupt flags on Timer2
        TCNT2 = 0; //256 - 125 = 131 : Preload timer 2 for x clicks. Should be 1us per click
        while( (TIFR2 & (1<<TOV2)) == 0);

        x -= 256;
    }

    TIFR2 = (1<<TOV2); //Clear any interrupt flags on Timer2
    TCNT2= 256 - x; //256 - 125 = 131 : Preload timer 2 for x clicks. Should be 1us per click
    while( (TIFR2 & (1<<TOV2)) == 0);
}

