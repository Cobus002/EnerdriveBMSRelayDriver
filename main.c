/*
 * TimerTest.c
 *
 * Created: 9/24/2018 2:00:03 PM
 * Author : Cobus Hoffmann
 */ 

#include <avr/io.h>
#include <avr/interrupt.h>
#include <util/delay.h>
#include "EN_GPIO.h"

//Temperature values for the temp sensor
#define	TEMP_3C		(uint8_t)109
#define TEMP_35C	(uint8_t)190
#define TEMP_45C	(uint8_t)210
#define TEMP_55C	(uint8_t)230
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
void handleProgram1(uint8_t in1PinState, uint8_t in2PinState);

void handleProgram2(uint8_t in3PinState, int* prog2Count);

//Program 3 handles temperature sensitive routines for temperatures greater than 35C and 45C
void handleProgram3(uint16_t temp);

//Program 5 handles temperature routines for temperatures greater than 55C
void handleProgram5(uint16_t temp);

//Program 6 handles temperature routines for temperatures less than 0C
void handleProgram6(uint16_t temp);

//Update the Status LED
void handleStatusLed(uint16_t temp);


int main(void)
{
	//Initialize the hardware timer
	initTimer();
	//Initialize the relay outputs
	initRelayOutputs();
	//Initialize the latch relays
	initLatchOutput();
	
	//checkRelayOutputs();
	initStatusLed();
	//start the temperature reading
	initTempSensor();
	
	//checkStatusLED();
	writeLEDOutput(0,0,0);
	sei();
	
	//Variables to store the input results
	uint8_t in0PinState = EN_INPUT_HIGH;
	uint8_t in1PinState = EN_INPUT_HIGH;
	uint8_t in2PinState = EN_INPUT_HIGH;
	uint8_t combined0And1PinState = EN_INPUT_HIGH;
	
	//Make sure the timers are in the inactive state
	initSoftTimer(&timer1);
	initSoftTimer(&timer2);
	initSoftTimer(&timer3);
	initSoftTimer(&timer5);
	initSoftTimer(&timer6);
	initSoftTimer(&timerLED);
	
	int* prog2Count;
	(*prog2Count) = 0;
	
	
	//Test the LEDs on startup
	writeLEDOutput(1,0,0); //Blue
	_delay_ms(1000);
	writeLEDOutput(0,1,0); //Green
	_delay_ms(1000);
	writeLEDOutput(0,0,1); //Red
	_delay_ms(1000);
	writeLEDOutput(0,0,0); //Off
	
	while(1)
	{
		//BEGIN WHILE LOOP
		//Read the inputs
		in0PinState = readInput(EN_GPIO_INPUT_1);
		in1PinState = readInput(EN_GPIO_INPUT_2);
		in2PinState = readInput(EN_GPIO_INPUT_3);
		tempVal = readTempSens();
		_delay_ms(1);
		
		if(timer1.tickDone){
			handleProgram1(!in0PinState, !in1PinState); //Flip sign since 0 is On and 1 is off
		}
		
		if(timer2.tickDone){
			handleProgram2(!in2PinState, prog2Count);
		}
		
		handleProgram3(tempVal);
		
		if(timer5.tickDone){
			handleProgram5(tempVal);
		}
		
		if(timer6.tickDone){
			handleProgram6(tempVal);
		}
		
		if(timerLED.tickDone){
			handleStatusLed(tempVal);
			
		}
		
		//END WHILE LOOP
	}
}

/*
 * initialize the 1 sec timer used to control all the events
 */
void initTimer(void){
	// set up timer with prescaler = 64 and CTC mode
	TCCR1B |= (1 << WGM12)|(1 << CS12)|(1 << CS10);
	
	// initialize counter
	TCNT1 = 0;
	
	// initialize compare value for 1 sec timer 500
	OCR1A = 500;
	
	//Timer 2
	
	// enable compare interrupt
	TIMSK1 |= (1 << OCIE1A);
	
	// enable global interrupts
	sei();
}
//Used to update the sofTimers 
void updateSoftTimer(Timer* tickTimer){
	if(tickTimer->tickActive){
		if(tickTimer->tickCount > 0){
			tickTimer->tickCount--;
		}else{
			tickTimer->tickDone = 1;
			tickTimer->tickActive = 0;
		}
	}
}
/*
 * Start the timer for timeSeconds seconds
 */
void startSoftTimer(Timer* tickTimer, uint16_t timeSeconds){
	tickTimer->tickDone=0;
	tickTimer->tickCount = timeSeconds;
	tickTimer->tickActive = 1;
}

//Initialize the software timer
void initSoftTimer(Timer* softTimer){
	softTimer->tickDone = 1;
	softTimer->tickCount = 0;
	softTimer->tickActive = 0;
}

//Timer ISR handler used for the second timer
ISR(TIMER1_COMPA_vect){
	//Clear the global interrupt so we don't get interrupted
	cli();
	//Update all the software timers
	updateSoftTimer(&timer1);
	updateSoftTimer(&timer2);
	updateSoftTimer(&timer3);
	updateSoftTimer(&timer4);
	updateSoftTimer(&timer5);
	updateSoftTimer(&timer6);
	updateSoftTimer(&timerLED);
	//Re-enable the global interrupts
	sei();
	
}


void handleProgram1(uint8_t in1PinState, uint8_t in2PinState){
	//Check the state of program 1
	switch(prog1State){
		case S0:
			if( !(in1PinState && in2PinState)){
				//One or both of the inputs are low
				startSoftTimer(&timer1, 15);	//Start software timer for 15sec
				prog1State = S1;
			}
			break;
		case S1:
			//Write TOR1 & 2 and  Outputs 1,2 & 3 high
			writeRelayOutput(EN_GPIO_TOR_1, 1);
			writeRelayOutput(EN_GPIO_TOR_2, 1);
			writeRelayOutput(EN_GPIO_OUTPUT_1, 1);
			writeRelayOutput(EN_GPIO_OUTPUT_2, 1);
			writeRelayOutput(EN_GPIO_OUTPUT_3, 1);
			//Delay 15 sec, then go to state 2
			startSoftTimer(&timer1, 15);
			prog1State = S2;
			break;
			
		case S2:
			//Pulse orange relay 
			writeBatteryLatch(LATCH_ORANGE, 1);
			_delay_ms(1000);
			writeBatteryLatch(LATCH_ORANGE, 0);
			//Delay 15 sec, then go to state 3
			startSoftTimer(&timer1, 15);
			prog1State = S3;
			break;
		
		case S3:
			//Write TOR1 & 2 and  Outputs 1,2 & 3 high
			writeRelayOutput(EN_GPIO_TOR_1, 0);
			writeRelayOutput(EN_GPIO_TOR_2, 0);
			writeRelayOutput(EN_GPIO_OUTPUT_1, 0);
			writeRelayOutput(EN_GPIO_OUTPUT_2, 0);
			//Delay 6min, then go to state 4
			startSoftTimer(&timer1, 60*6);
			prog1State = S4;
			break;
		
		case S4:
			if(!(in1PinState && in2PinState)){
				//Input still low, go back to state 1
				startSoftTimer(&timer1, 2);
				prog1State = S1;
			}else{
				//Delay 15min and then go to state 5
				startSoftTimer(&timer1, 60*15);
				prog1State = S5;
			}
			break;
		
		case S5:
			//pulse latch relay brown
			writeBatteryLatch(LATCH_BROWN, 1);
			_delay_ms(1000);
			writeBatteryLatch(LATCH_BROWN, 0);
			//Turn off OUTPUT 3
			writeRelayOutput(EN_GPIO_OUTPUT_3, 0);
			//Go back to state 0
			startSoftTimer(&timer1, 2);
			prog1State = S0;
			break;
	}
}

//Handle Input 3 routine
void handleProgram2(uint8_t in3PinState, int* prog2Count){
	switch(prog2State){
		case S0:
			if(!(in3PinState)){
				//Set the prog2 count to 0
				(*prog2Count) = 0;
				//Start soft timer for 5 sec
				startSoftTimer(&timer2, 5);
				//Go to next state
				prog2State = S1;
			}
			break;
		
		case S1:
			if((*prog2Count)<10){
				//Turn on TOR 1&2, and OUTPUT 1&2
				writeRelayOutput(EN_GPIO_TOR_1, 1);
				writeRelayOutput(EN_GPIO_TOR_2, 1);
				writeRelayOutput(EN_GPIO_OUTPUT_1, 1);
				writeRelayOutput(EN_GPIO_OUTPUT_2, 1);
				//Increment prog2Count
				(*prog2Count)++;
				//Start soft timer for 1 min 
				startSoftTimer(&timer2, 60*1);
				//Stay in current state
				prog2State = S1;
			}else if(!(in3PinState)){
				//Still low
				//Reset prog counter and stay in current state
				(*prog2Count) = 0;
			}else{
				//prog2Count >=10 and in3PinState is HIGH
				//Turn off TOR 1&2, and OUTPUT 1&2
				writeRelayOutput(EN_GPIO_TOR_1, 0);
				writeRelayOutput(EN_GPIO_TOR_2, 0);
				writeRelayOutput(EN_GPIO_OUTPUT_1, 0);
				writeRelayOutput(EN_GPIO_OUTPUT_2, 0);
				//Start soft timer for 2 sec
				startSoftTimer(&timer2, 2);
				prog2State = S0; 
			}
			break;
	}
}

//Handle the temp values up to 45C
void handleProgram3(uint16_t temp){
	switch (prog3State){
		case S0:
			if(temp > TEMP_35C){
				//Turn on Output4
				writeRelayOutput(EN_GPIO_OUTPUT_4, 1);
			}else{
				//Turn off output4
				writeRelayOutput(EN_GPIO_OUTPUT_4, 0);
			}
			
			if((temp > TEMP_35C)&&(temp < TEMP_45C)){
				//Turn off TOR 1&2 and OUTPUT 1&2
				writeRelayOutput(EN_GPIO_TOR_1, 0);
				writeRelayOutput(EN_GPIO_TOR_2, 0);
				writeRelayOutput(EN_GPIO_OUTPUT_1, 0);
				writeRelayOutput(EN_GPIO_OUTPUT_2, 0);
			}
			if(temp > TEMP_45C){
				//Turn on TOR 1&2 and OUTPUT 1&2
				writeRelayOutput(EN_GPIO_TOR_1, 1);
				writeRelayOutput(EN_GPIO_TOR_2, 1);
				writeRelayOutput(EN_GPIO_OUTPUT_1, 1);
				writeRelayOutput(EN_GPIO_OUTPUT_2, 1);
			}
			break;
	}
}

void handleProgram5(uint16_t temp){
	switch (prog5State){
		case S0:
			if(temp > TEMP_55C){
				//Turn on Output 3 and pulse Orange Relay
				writeRelayOutput(EN_GPIO_OUTPUT_3, 1);
				//Pulse orange latch relay
				writeBatteryLatch(LATCH_ORANGE, 1);
				_delay_ms(1000);
				writeBatteryLatch(LATCH_ORANGE, 0);
				//Start soft timer 1 min
				startSoftTimer(&timer5, 60*1);
				prog5State = S1;
			}
			break;
		case S1:
			if(temp > TEMP_55C){
				//Turn on OUTPUT 3 and pulse Orange Relay
				writeRelayOutput(EN_GPIO_OUTPUT_3, 1);
				//Pulse Orange latch relay
				writeBatteryLatch(LATCH_ORANGE, 1);
				_delay_ms(1000);
				writeBatteryLatch(LATCH_ORANGE, 0);
				startSoftTimer(&timer5, 60*1);
				//Stay in same state
				prog5State = S1;
				
			}else{
				//Turn off OUTPUT3 and pulse Brown Relay
				writeRelayOutput(EN_GPIO_OUTPUT_3, 0);
				//Pulse Brown latch relay
				writeBatteryLatch(LATCH_BROWN, 1);
				_delay_ms(1000);
				writeBatteryLatch(LATCH_BROWN, 0);
				startSoftTimer(&timer5, 2);
				prog5State = S0;
			}
			break;	
	}
}

void handleProgram6(uint16_t temp){
	switch (prog6State){
		case S0:
			if(temp < TEMP_3C){
				//Turn on TOR 1&2, and OUTPUT 1&2
				writeRelayOutput(EN_GPIO_TOR_1, 1);
				writeRelayOutput(EN_GPIO_TOR_2, 1);
				writeRelayOutput(EN_GPIO_OUTPUT_1, 1);
				writeRelayOutput(EN_GPIO_OUTPUT_2, 1);
				//Start soft timer 1 min
				startSoftTimer(&timer6, 60*1);
				//Go to next state
				prog6State = S1;
			}
			break;
		case S1:
			if(temp < TEMP_3C){
				//Turn on TOR 1&2, and OUTPUT 1&2
				writeRelayOutput(EN_GPIO_TOR_1, 1);
				writeRelayOutput(EN_GPIO_TOR_2, 1);
				writeRelayOutput(EN_GPIO_OUTPUT_1, 1);
				writeRelayOutput(EN_GPIO_OUTPUT_2, 1);
				//Start soft timer 1 min
				startSoftTimer(&timer6, 60*1);
				//Stay in current state
				prog6State = S1; 
			}else{
				//Turn Off TOR 1&2, and OUTPUT 1&2
				writeRelayOutput(EN_GPIO_TOR_1, 0);
				writeRelayOutput(EN_GPIO_TOR_2, 0);
				writeRelayOutput(EN_GPIO_OUTPUT_1, 0);
				writeRelayOutput(EN_GPIO_OUTPUT_2, 0);
				//Start soft timer 2 sec
				startSoftTimer(&timer6, 2);
				//Go to state S0
				prog6State = S0;
			}
			break;
		
	}
}

/*
 * handleStatusLed() function takes the values of input1, input2 and temperature 
 * and updates the status LED accordingly.
 */
 
void handleStatusLed(uint16_t temp){
	if(prog5State!=S0){
		//Greater than 55C flash BLUE <-> RED
		ledState ? writeLEDOutput(1, 0, 0): writeLEDOutput(0, 0, 1);
	}else if(prog6State!=S0){
		ledState ? writeLEDOutput(1, 0, 0): writeLEDOutput(0, 0, 0);
	}else if(temp>TEMP_45C){
		//Solid red
		writeLEDOutput(0, 0, 1);
	}else if(prog2State!=S0){
		//flash red
		ledState ? writeLEDOutput(0, 0, 0) : writeLEDOutput(0, 0, 1);
	}else if(prog1State != S0){
		//flash green
		ledState ? writeLEDOutput(0, 1, 0): writeLEDOutput(0, 0, 0);
	}else{
		writeLEDOutput(0, 0, 0);
	}
	ledState=!ledState;
	//Start 1 sec timer
	startSoftTimer(&timerLED, 1);
	
}

