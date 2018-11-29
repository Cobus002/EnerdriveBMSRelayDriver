#include "EN_GPIO.h"

void initRelayOutputs(){
	//used to set up the relays as outputs 
	DDRC |= (1<<PC2)|(1<<PC0);
	DDRB |= (1<<PB7)|(1<<PB6);
	//TOR OUTPUTS
	DDRD |= (1<<PD3)|(1<<PD4);
}

void initLatchOutput(){
	DDRD|=(1<<6);
	DDRB|=(1<<2);
}

void writeBatteryLatch(ENOutput_t latch,  uint8_t state){
	if(latch == LATCH_ORANGE){
		state == 0? (PORTD &= ~(1<<6)):(PORTD|=(1<<6));
	}else if(latch == LATCH_BROWN){
		state == 0? (PORTB &= ~(1<<2)):(PORTB|=(1<<2));
	}
}

void initInputs(){
	
	//DDRD |= (1<<PD2)|(1<<PD1)|(1<<PD0); //optos 2-4
	//DDRC |= (1<<PC5); //opto 1
}

void initStatusLed(){
	DDRB |= (1<<5)|(1<<4)|(1<<3);
}

void initTempSensor(){
	//Make sure PC4 is input in free running mode with interrupt enable
	//implement ISR() in main to trigger on conversion complete 
	//DDRC &= ~(1<<4);
	ADMUX |= (1<<REFS0)|(1<<MUX2); //Select ADC4 as input
	
	ADCSRA |= (1<<ADEN)|(1<<ADATE)|(1<<ADPS2)|(1<<ADPS1)|(1<<ADPS0); 
	//Start the ADC
	ADCSRA |=(1<<ADSC);
	
	
}

uint8_t readInput(ENInput_t en_input){
	uint8_t temp = 0;
	switch (en_input)
	{
	case INPUT_1:
		temp = PINC & (1<<5);
		break;
	case INPUT_2:
		temp = PIND & (1<<0);
		break;
	case INPUT_3:
		temp = PIND & (1<<1);
		break;
	case INPUT_4:
		temp = PIND & (1<<2);
		break;
	case INPUT_MODE:
		temp = PINB & (1<<1);
		break;
	}
	//Set temp to 1 if greater than 0
	if (temp > 0){
		temp = 1;
	}
	
	return temp;
}
uint16_t readTempSens(){
	
	//Start the conversion
	ADCSRA |=(1<<ADSC);
	
	//Wait for conversion to finish
	while(!(ADCSRA & (1<<ADIF))){
		//do nothing
	}
	
	//Clear the flag
	ADCSRA |= (1<<ADIF);
	
	return(ADC);
}


void writeRelayOutput(uint8_t en_output, uint8_t val){
	switch (en_output){
		case EN_GPIO_OUTPUT_1:
		val == 0 ? (PORTB &= ~(1<<6)): (PORTB |= (1<<6));
			break;
		case EN_GPIO_OUTPUT_2:
		val == 0 ? (PORTB &= ~(1<<7)): (PORTB |= (1<<7));
			break;
		case EN_GPIO_OUTPUT_3:
		val == 0 ? (PORTC &= ~(1<<2)): (PORTC |= (1<<2));
			break;
		case EN_GPIO_OUTPUT_4:
		val == 0 ? (PORTC &= ~(1<<0)): (PORTC |= (1<<0));
			break;
		case EN_GPIO_TOR_1:
			val == 0 ? (PORTD &= ~(1<<3)): (PORTD |= (1<<3));
			break;
		case EN_GPIO_TOR_2:
			val == 0 ? (PORTD &= ~(1<<4)): (PORTD |= (1<<4));
			break;
		
	}
}

void writeLEDOutput(uint8_t red, uint8_t blue, uint8_t green){
	//DDRB |= (1<<5)|(1<<4)|(1<<3);
	red==0 ? (PORTB &= ~(1<<5)):(PORTB |= 1<<5);
	green==0 ? (PORTB &= ~(1<<4)):(PORTB |= 1<<4);
	blue==0 ? (PORTB &= ~(1<<3)):(PORTB |= 1<<3);
	
}

void toggleLEDOutput(uint8_t blue, uint8_t green, uint8_t red){
		red==1 ? (PORTB ^= (1<<5)):(PORTB &= ~(1<<5));
		green==1 ? (PORTB ^= (1<<4)):(PORTB &= ~(1<<4));
		blue==1 ? (PORTB ^= (1<<3)):(PORTB &= ~(1<<3));
}


//Simple bit of code to check the outputs
void checkRelayOutputs(){
	
	writeRelayOutput(EN_GPIO_OUTPUT_1, 1);
	_delay_ms(1000);
	writeRelayOutput(EN_GPIO_OUTPUT_2, 1);
	_delay_ms(1000);
	writeRelayOutput(EN_GPIO_OUTPUT_3, 1);
	_delay_ms(1000);
	writeRelayOutput(EN_GPIO_OUTPUT_4, 1);
	_delay_ms(1000);
	writeRelayOutput(EN_GPIO_OUTPUT_1, 0);
	_delay_ms(1000);
	writeRelayOutput(EN_GPIO_OUTPUT_2, 0);
	_delay_ms(1000);
	writeRelayOutput(EN_GPIO_OUTPUT_3, 0);
	_delay_ms(1000);
	writeRelayOutput(EN_GPIO_OUTPUT_4, 0);
	_delay_ms(1000);
}

void checkStatusLED(){
	writeLEDOutput(1,0,0);
	_delay_ms(1000);
	writeLEDOutput(0,1,0);
	_delay_ms(1000);
	writeLEDOutput(0,0,1);
	_delay_ms(1000);
	writeLEDOutput(0,0,0);
	_delay_ms(1000);
	
}