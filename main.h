#ifndef _MAIN_H_
#define _MAIN_H_

//Comment out the line below to disable pulsing of brown relay in program 1
#define RE_ENABLE_LATCH_RELAY 1

//Prog modes
#define EN_MODE_A		1
#define EN_MODE_B		2

//Temperature values for the temp sensor
#define TEMP_3C		(uint8_t)109
#define TEMP_35C	(uint8_t)190
#define TEMP_45C	(uint8_t)209
#define TEMP_55C	(uint8_t)225
#define TEMP_65C	(uint8_t)234
#define TEMP_70C	(uint8_t)240

uint8_t OUT1_PIN = 3;
uint8_t OUT2_PIN = 4;

//typedef to store the timer stuff
typedef struct timer{
	uint16_t tickCount;
	uint8_t tickActive;
	uint8_t tickDone;
}Timer;

//Timers used in the programs
Timer timer1;	//Program 1 timer
Timer timer2;	//Program 2 timer
Timer timer3;	//Program 3 timer
Timer timer4;	//Not used
Timer timer5;	//Program 5 timer
Timer timer6;	//Program 6 timer
Timer timerLED;  //LED timer

//Enum to store the states
typedef enum {S0, S1, S2, S3, S4, S5, S6, S7, S9} state_t;

//Program state variables
state_t prog1State = S0;
state_t prog2State = S0;
state_t prog3State = S0;
state_t prog4State = S0;	//Not used
state_t prog5State = S0;
state_t prog6State = S0;

uint8_t tempCount = 0;
uint16_t tempVal = 0;

uint8_t ledState = 1;

//Initialize the hardware timer used to set the 1 sec global timer
void initTimer(void);

//Initialize the software timer
void initSoftTimer(Timer* softTimer);

//Start a soft timer referenced by pointer for number of seconds
void startSoftTimer(Timer* softTimer, uint16_t seconds);

//Update the software timers, called by the global sec timer interrupt
void updateSoftTimer(Timer* softTimer);

//Handle the programs
void handleProgram1(uint8_t in1PinState, uint8_t in2PinState, int* prog1Count);

void handleProgram2(uint8_t in3PinState, int* prog2Count);

//Program 3 handles temperature sensitive routines for temperatures greater than 35C
void handleProgram3(uint16_t temp);

//Program 4 handles temperature sensitive routines for temperatures greater than 45C
void handleProgram4(uint16_t temp);

//Program 5 handles temperature sensitive routines for temperatures greater than 55C
void handleProgram5(uint16_t temp);

//Program 6 handles temperature routines for temperatures less than 0C
void handleProgram6(uint16_t temp);

//Update the Status LED
void handleStatusLed(uint16_t temp);


#endif