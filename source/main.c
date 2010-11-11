/*
	Created on: July 22, 2010
	By: Jim Lindblom
*/

//======================//
//       Includes		//
//======================//
#include <avr/io.h>
#include <stdlib.h>
#include <avr/interrupt.h>
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
#define HS_ADDRESS_L 1
#define HS_ADDRESS_H 2

//======================//
// Function Definitions	//
//======================//
void ioinit(void);
void delay_ms(uint16_t x);
void delay_us(uint16_t x);
void writeEEPROM(char toWrite, uint8_t addr);
char readEEPROM(char addr);

void generateSeed(void);
void initBoard(void);
void printBoard(void);
int validBoard(void);
void removeGems(void);
void addGems(void);
int noMovesLeft(void);
uint16_t getHighScore(void);

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
int score = 0;
int timeLimit = 60;
int gameTimer = 0;
int highScore;

ISR(TIMER1_COMPA_vect)
{
	gameTimer++;
}

//======================//
//	       Main			//
//======================//
int main(void)
{
	uint8_t userInput[4];
	uint8_t row1, row2, column1, column2;
	
	ioinit();
	
	/* Reset high scores
	writeEEPROM(0, HS_ADDRESS_L);
	writeEEPROM(0, HS_ADDRESS_H);
	*/
	highScore = getHighScore();
	printf("Can you beat %d?\n", highScore);
	
	initBoard();
	printBoard();
	while(1)
	{
		//--- get user input---
		printf("Enter column row column row...\n");
		for (unsigned int i = 0; i<4; i++)
		{
			userInput[i] = uart_getchar() - 0x30;
			printf("%d ", userInput[i]);
			// 0 - row 1
			// 1 - column 1
			// 2 - row 2
			// 3 - column 2
		}
		row1 = userInput[0];
		column1 = userInput[1];
		row2 = userInput[2];
		column2 = userInput[3];	
		printf("\n");
		
		// --- check valid move ---
		if (((column1 == column2 && (row1 == row2 + 1)) ||
			(column1 == column2 && (row1 == row2 - 1)) ||
			(row1 == row2 && (column1 == column2 + 1)) ||
			(row1 == row2 && (column1 == column2 - 1))))
		{
			// --- swap inputs ---
			currentBoard[column1][row1] = currentBoard[column1][row1] ^ currentBoard[column2][row2];
			currentBoard[column2][row2] = currentBoard[column2][row2] ^ currentBoard[column1][row1];
			currentBoard[column1][row1] = currentBoard[column1][row1] ^ currentBoard[column2][row2];
			
			if (!validBoard())
			{	// If there's no valid moves revert to old board
				currentBoard[column1][row1] = currentBoard[column1][row1] ^ currentBoard[column2][row2];
				currentBoard[column2][row2] = currentBoard[column2][row2] ^ currentBoard[column1][row1];
				currentBoard[column1][row1] = currentBoard[column1][row1] ^ currentBoard[column2][row2];
				printf("Nothing to eliminate\n");
			}
			else
			{
				while (validBoard())
				{
					removeGems();
					addGems();
					printBoard();
					printf("Score: %d\n", score);
					printf("Timer: %d/%d\n", gameTimer, timeLimit);
				}/*
				if(validBoard())
				{
					while(validBoard())
					{
						removeGems();
						addGems();
						printBoard();
					}
				}*/
				if(noMovesLeft())
					break;
			}
		}
		else
		{
			printf("Invalid Move\n");
		}	
		
		if (gameTimer >= timeLimit)
		{
			printf("Out of time!!!\n");
			break;
		}
		else if (noMovesLeft())
		{
			printf("No moves remain\n");
			break;
		}
		// !!!To DO!!!
		// else increase speed of timer go back to top of while
	}
	printf("Game Over!\n");
	printf("Final Score: %d\n", score);
	if (score > highScore)
	{
		printf("That's a new high score!\n");
		writeEEPROM((score & 0xFF00)>>8, HS_ADDRESS_H);
		writeEEPROM((score & 0xFF), HS_ADDRESS_L);
	}
}

void ioinit (void)
{
	int myUBRR = 103;	// Results in 9600bps@8MHz or 19200bps@16MHz
		
	// Set 16-bit Timer 1 to 1Hz
	TCCR1A = 0x00;	// no output, CTC operation
	TCCR1B = (_BV(WGM12) | _BV(CS12));	// CTC operation, 256 prescaler
	OCR1A = F_CPU/256;
	TIMSK1 = _BV(OCIE1A);	// Overflow interrupt enable
	sei();
	
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
			
			while (currentBoard[i][j] == currentBoard[i-1][j] && currentBoard[i][j] == currentBoard[i-2][j] && i >= 2)
			{
				currentBoard[i][j] = rand()%7;
			}
			while (currentBoard[i][j] == currentBoard[i][j-1] && currentBoard[i][j] == currentBoard[i][j-2] && j >= 2)
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
	printf("   0 1 2 3 4 5 6 7\n");
	printf("------------------\n");
	for (uint8_t i=0; i<BOARD_SIZE; i++)
	{
		printf("%d: ", i);
		for (uint8_t j=0; j<BOARD_SIZE; j++)
		{
			printf("%d ", currentBoard[i][j]);
		}
		printf("\n");
	}
}

int validBoard(void)
{
	int i, j, valid = 0;

	for (j=0; j<BOARD_SIZE; j++)
	{
		for (i=0; i<BOARD_SIZE; i++)
		{
			if(currentBoard[i][j]==currentBoard[i+1][j] && 
				currentBoard[i][j]==currentBoard[i+2][j] && 
				i < BOARD_SIZE-2 && 
				currentBoard[i][j] != 7)
			{
				valid = 1;
			}
			if(currentBoard[i][j]==currentBoard[i][j+1] && 
				currentBoard[i][j]==currentBoard[i][j+2] && 
				j < BOARD_SIZE-2 && 
				currentBoard[i][j] != 7)
			{
				valid = 1;
			}
		}
	}
	return valid;
}

void removeGems(void)
{
	int i, j;
	char copyBoard[BOARD_SIZE][BOARD_SIZE];
	
	for (j=0; j<BOARD_SIZE; j++)
	{
		for (i=0; i<BOARD_SIZE; i++)
		{
			copyBoard[i][j] = 0;
		}
	}
	
	for(j=0; j<BOARD_SIZE; j++)
	{
		for(i=0; i<BOARD_SIZE; i++)
		{
			if(currentBoard[i][j] == currentBoard[i+1][j] && currentBoard[i][j] == currentBoard[i+2][j] && i < BOARD_SIZE-2)
			// Test for 3+ in vertical line
			{
				while (currentBoard[i][j] == currentBoard[i+1][j] && currentBoard[i][j] != 7)
				// Place 1's in spots where there are matches
				{
					copyBoard[i][j] = 1;
					i++;
				}
				copyBoard[i][j] = 1;
			}
		}
	}
	for(j=0; j<BOARD_SIZE; j++)
	{
		for(i=0; i<BOARD_SIZE; i++)
		{
			if(currentBoard[i][j] == currentBoard[i][j+1] && currentBoard[i][j] == currentBoard[i][j+2] && j < BOARD_SIZE-2)
			// New check for 3+ in a horizontal line
			{
				while (currentBoard[i][j] == currentBoard[i][j+1] && currentBoard[i][j] != 7)
				// Place 1's in spots where there are matches
				{
					copyBoard[i][j] = 1;
					j++;
				}
				copyBoard[i][j] = 1;
			}
		}
	}

	for (j=0; j<BOARD_SIZE; j++)
	{
		for (i=0; i<BOARD_SIZE; i++)
		{
			if (copyBoard[i][j] == 1)
			{
				currentBoard[i][j] = 7; // Every correct gem temporarily receives a 7
				score++;
				timeLimit+=2;
			}

		}
	}
}

void addGems(void)
{
	int i, j, l;

	for(i=0; i<BOARD_SIZE; i++)
	{
		for(j=0; j<BOARD_SIZE; j++)
		{
			if(currentBoard[i][j] == 7 && j == 0)
			{
				currentBoard[i][j] = rand()%7;
				continue;
			}
			if(currentBoard[i][j] == 7 && j > 0)
			{
				for(l=j; l>0; l--)
				{
					currentBoard[i][l] = currentBoard[i][l-1];
				}

				currentBoard[i][0] = rand()%7;

				continue;
			}
		}
	}

}

int noMovesLeft(void)
{
	int moves, i, j;

	for(i=0; i<BOARD_SIZE; i++)
	{
		for(j=0; j<BOARD_SIZE; j++)
		{
			if(currentBoard[i][j] == currentBoard[i][j+1] && (currentBoard[i][j] == currentBoard[i][j+3]
			|| currentBoard[i][j] == currentBoard[i-1][j+2] || currentBoard[i][j] == currentBoard[i+1][j+2]
			|| currentBoard[i][j] == currentBoard[i][j-2] || currentBoard[i][j] == currentBoard[i-1][j-1]
			|| currentBoard[i][j] == currentBoard[i+1][j-1]))
			// Checking horizontal
			{
				moves = 1;
			}
			else
			{
				if(currentBoard[i][j] == currentBoard[i+1][j] && (currentBoard[i][j] == currentBoard[i+3][j]
				|| currentBoard[i][j] == currentBoard[i+2][j-1] || currentBoard[i][j] == currentBoard[i+2][j+1]
				|| currentBoard[i][j] == currentBoard[i-2][j] || currentBoard[i][j] == currentBoard[i-1][j-1]
				|| currentBoard[i][j] == currentBoard[i-1][j+1]))
				// Checking vertical
				{
					moves = 1;
				}
				else
				{
					moves = 0;
				}
			}
		}
	}
	return moves;
}

uint16_t getHighScore(void)
{
	uint8_t hsLow, hsHigh;
	
	hsLow = readEEPROM(HS_ADDRESS_L);
	hsHigh = readEEPROM(HS_ADDRESS_H);
	
	return (hsHigh << 8) | hsLow;
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

// Write toWrite to addr in EEPROM
void writeEEPROM(char toWrite, uint8_t addr)
{
	// Write toWrite value to EEPROM address addr
	/* Wait for completion of previous write */
	while(EECR & (1<<EEPE))
		;
	/* Set up address and Data Registers */
	EEAR = addr;
	EEDR = toWrite;	// Write data1 into EEPROM
	/* Write logical one to EEMPE */
	EECR |= (1<<EEMPE);
	/* Start eeprom write by setting EEPE */
	EECR |= (1<<EEPE);
}

// Read EEPROM address addr and return value in EEDR
char readEEPROM(char addr)
{
	/* Wait for completion of previous write */
	while(EECR & (1<<EEPE))
		;
	/* Set up address register */
	EEAR = addr;
	/* Start eeprom read by writing EERE */
	EECR |= (1<<EERE);

	return EEDR;
}