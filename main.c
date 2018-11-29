/*
 * TimerTest.c
 *
 * Created: 9/24/2018 2:00:03 PM
 * Author : Cobus Hoffmann
 */ 

#include <avr/io.h>
#include <avr/interrupt.h>
#include <util/delay.h>
#include "main.h"
#include "EN_GPIO.h"


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
	uint8_t inModePinState = EN_MODE_A;
	uint8_t in0PinState = EN_INPUT_HIGH;
	uint8_t in1PinState = EN_INPUT_HIGH;
	uint8_t in2PinState = EN_INPUT_HIGH;
	uint8_t combined0And1PinState = EN_INPUT_HIGH;
	
	uint8_t currentMode = EN_MODE_A;
	
	//Make sure the timers are in the inactive state
	initSoftTimer(&timer1);
	initSoftTimer(&timer2);
	initSoftTimer(&timer3);
	initSoftTimer(&timer4);
	initSoftTimer(&timer5);
	initSoftTimer(&timer6);
	initSoftTimer(&timerLED);
	
	//Program 1 and program 2 counters
	int prog1Count = 0;
	int prog2Count = 0;
	
	
	
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
		inModePinState = readInput(INPUT_MODE);
		in0PinState = readInput(INPUT_1);
		in1PinState = readInput(INPUT_2);
		in2PinState = readInput(INPUT_3);
		tempVal = readTempSens();
		_delay_ms(1);
		
		if(timer1.tickDone){
			handleProgram1(!in0PinState, !in1PinState, &prog1Count); //Flip sign since 0 is On and 1 is off
		}
		
		if(timer2.tickDone){
			handleProgram2(!in2PinState, &prog2Count);
		}
		
		
		if(timer3.tickDone){
			handleProgram3(tempVal);
		}
		
		//Check the mode switch and if in prog B then disable temperature sensor routines
		if(inModePinState == EN_MODE_A){
			if(currentMode != EN_MODE_A){
				//Set the current mode to A
				currentMode = EN_MODE_A;
			}
			//In program A so execute temperature programs 4 and 5
			
			if(timer4.tickDone){
				handleProgram4(tempVal);
			}
			
			if(timer5.tickDone){
				handleProgram5(tempVal);
			}
			
			if(timer6.tickDone){
				handleProgram6(tempVal);
			}
			
		}else{
			//Switch is in MODE B position
			if(currentMode != EN_MODE_B){
				//Only want to set this once
				writeLEDOutput(0,0,0);
				//TURN OFF TOR 1&2 and OUTPUTS 1,2,3 and 4
				writeRelayOutput(EN_GPIO_TOR_1, 0);
				writeRelayOutput(EN_GPIO_TOR_2, 0);
				writeRelayOutput(EN_GPIO_OUTPUT_1, 0);
				writeRelayOutput(EN_GPIO_OUTPUT_2, 0);
				writeRelayOutput(EN_GPIO_OUTPUT_3, 0);
				writeRelayOutput(EN_GPIO_OUTPUT_4, 0);
				//Set the states to S0
				prog4State = S0;
				prog5State = S0;
				prog6State = S0;
				//Update mode
				currentMode = EN_MODE_B;
			}

		}
		
		//Update the status led
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
	OCR1A = 1000;
	
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

//Check inputs 1 and 2 then set TOR and OUTPUTS accordingly
void handleProgram1(uint8_t in1PinState, uint8_t in2PinState, int* prog1Count){
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
			if( !(in1PinState && in2PinState)){
				//One or both of the inputs are low
				startSoftTimer(&timer1, 1);	//Start software timer for 1sec
				prog1State = S2;
			}else{
				//The inputs are high again, start timer and go back to S0
				startSoftTimer(&timer1, 1);
				prog1State = S0;
			}
			break;
		case S2:
			//Write TOR1 & 2 and  Outputs 1,2 & 3 high
			writeRelayOutput(EN_GPIO_TOR_1, 1);
			writeRelayOutput(EN_GPIO_TOR_2, 1);
			writeRelayOutput(EN_GPIO_OUTPUT_1, 1);
			writeRelayOutput(EN_GPIO_OUTPUT_2, 1);
			writeRelayOutput(EN_GPIO_OUTPUT_3, 1);
			//Delay 15 sec, then go to state 2
			startSoftTimer(&timer1, 15);
			prog1State = S3;
			break;
			
		case S3:
			//Pulse orange relay 
			writeBatteryLatch(LATCH_ORANGE, 1);
			_delay_ms(1000);
			writeBatteryLatch(LATCH_ORANGE, 0);
			//Delay 15 sec, then go to state 3
			startSoftTimer(&timer1, 15);
			prog1State = S4;
			break;
		
		case S4:
			//Write TOR1 & 2 and  Outputs 1,2 & 3 high
			writeRelayOutput(EN_GPIO_TOR_1, 0);
			writeRelayOutput(EN_GPIO_TOR_2, 0);
			writeRelayOutput(EN_GPIO_OUTPUT_1, 0);
			writeRelayOutput(EN_GPIO_OUTPUT_2, 0);
			//Delay 6min, then go to state 4
			startSoftTimer(&timer1, 60*6);
			prog1State = S5;
			break;
		
		case S5:
			if(!(in1PinState && in2PinState)){
				//Input still low, go back to state 2
				startSoftTimer(&timer1, 2);
				prog1State = S2;
			}else{
				//Set prog1Count = 0
				(*prog1Count) = 0;
				//Delay 1 sec and go to state 6
				startSoftTimer(&timer1, 1);
				prog1State = S6;
			}
			break;
		
		case S6:
			if ((*prog1Count)<15){
				//Check the inputs
				if(!(in1PinState && in2PinState)){
					//The inputs have gone low again go back to state 2
					startSoftTimer(&timer1, 15);
					prog1State = S2;
				}else{
					//Still High so increment prog1Count, delay and stay in state
					(*prog1Count)++;
					startSoftTimer(&timer1, 60*1);
					prog1State = S6;
				}
			}else{
#ifdef RE_ENABLE_LATCH_RELAY
				//pulse latch relay brown
				writeBatteryLatch(LATCH_BROWN, 1);
				_delay_ms(1000);
				writeBatteryLatch(LATCH_BROWN, 0);
#endif
				//Turn off OUTPUT 3
				writeRelayOutput(EN_GPIO_OUTPUT_3, 0);
				//Go back to state 0
				startSoftTimer(&timer1, 2);
				prog1State = S0;
				
			}
			
			break;
	}
}

//Handle Input 3 routine
void handleProgram2(uint8_t in3PinState, int* prog2Count){
	switch(prog2State){
		case S0:
			if(!(in3PinState)){
				//Input 3 is low, start timer and go to state 1
				startSoftTimer(&timer2, 5);
				prog2State = S1;
			}
			break;
		
		case S1:
			if(!(in3PinState)){
				//Set the prog2 count to 0
				(*prog2Count) = 0;
				//Start soft timer for 1 sec
				startSoftTimer(&timer2, 1);
				//Go to next state
				prog2State = S2;
			}else{
				//Input 3 is High again, start timer and go back to state 0
				startSoftTimer(&timer2, 1);
				prog2State = S0;
			}
			break;
		
		case S2:
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
				prog2State = S2;
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
				//Go back to state 0
				prog2State = S0; 
			}
			break;
	}
}

//Checks if temperature is greater than 35C then performs appropriate actions
void handleProgram3(uint16_t temp){
	switch (prog3State){
		case S0:
			if(temp > TEMP_35C){
				//Turn on Output4
				writeRelayOutput(EN_GPIO_OUTPUT_4, 1);
				//Start 2 sec timer and go to state S1
				startSoftTimer(&timer3, 2);
				prog3State = S1;
			}
			break;
		case S1:
			if (temp < TEMP_35C){
				//Turn off OUTPUT4
				writeRelayOutput(EN_GPIO_OUTPUT_4, 0);
				//Start 2 sec timer and go to state 0
				startSoftTimer(&timer3, 2);
				prog3State = S0;
			}else{
				//Ensure OUTPUT4 is on
				writeRelayOutput(EN_GPIO_OUTPUT_4, 1);
				//start 2 sec timer and stay in current state
				startSoftTimer(&timer3, 2);
				prog3State = S1;
			}
			break;
	}
}

//Checks if the temperature is greater than 45C then performs appropriate actions
void handleProgram4(uint16_t temp){
	switch(prog4State){
		case S0:
			if(temp > TEMP_45C){
				//Turn on TOR 1&2 and OUTPUT 1&2 
				writeRelayOutput(EN_GPIO_TOR_1, 1);
				writeRelayOutput(EN_GPIO_TOR_2, 1);
				writeRelayOutput(EN_GPIO_OUTPUT_1, 1);
				writeRelayOutput(EN_GPIO_OUTPUT_2, 1);
				//Start 2 sec soft timer 
				startSoftTimer(&timer4, 2);
				prog4State = S1;
			}
			break;
		case S1:
			if(temp < TEMP_45C){
				//Turn off TOR 1&2 and OUTPUT 1&2
				writeRelayOutput(EN_GPIO_TOR_1, 0);
				writeRelayOutput(EN_GPIO_TOR_2, 0);
				writeRelayOutput(EN_GPIO_OUTPUT_1, 0);
				writeRelayOutput(EN_GPIO_OUTPUT_2, 0);
				//Start 2 sec timer and go back to state S0
				startSoftTimer(&timer4, 2);
				prog4State = S0;
			}else{
				//Temp is still high, ensure TOR 1&2 is on and OUTPUT 1&2
				writeRelayOutput(EN_GPIO_TOR_1, 1);
				writeRelayOutput(EN_GPIO_TOR_2, 1);
				writeRelayOutput(EN_GPIO_OUTPUT_1, 1);
				writeRelayOutput(EN_GPIO_OUTPUT_2, 1);
				//Start 2 sec soft timer and stay in current state
				startSoftTimer(&timer4, 2);
				prog4State = S1;
			}
			break;
		
	}
}

//Checks if temperature is higher than 55C then performs appropriate actions
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

//Checks if the temp is too Low (<3C) then performs actions accordingly
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
	startSoftTimer(&timerLED, 0);
	
}

