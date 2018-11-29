#ifndef _EN_GPIO_
#define _EN_GPIO_
#include <avr/io.h>
#include <util/delay.h>


//Available Inputs
#define EN_GPIO_INPUT_1		(uint8_t)0
#define EN_GPIO_INPUT_2		(uint8_t)1
#define EN_GPIO_INPUT_3		(uint8_t)2
#define EN_GPIO_INPUT_4		(uint8_t)3
//Declare for later implementation
typedef enum{INPUT_1, INPUT_2, INPUT_3, INPUT_4, INPUT_MODE} ENInput_t;

//Declared for later implementation
typedef enum {OUTPUT_1, OUTPUT_2, OUTPUT_3, OUTPUT_4, TOR_1, TOR_2, LATCH_ORANGE, LATCH_BROWN} ENOutput_t;
//Available Outputs
#define EN_GPIO_OUTPUT_1	(uint8_t)4
#define EN_GPIO_OUTPUT_2	(uint8_t)5
#define EN_GPIO_OUTPUT_3	(uint8_t)6
#define EN_GPIO_OUTPUT_4	(uint8_t)7
#define EN_GPIO_TOR_1		(uint8_t)8
#define EN_GPIO_TOR_2		(uint8_t)9
#define EN_GPIO_LATCH_0		(uint8_t)10
#define EN_GPIO_LATCH_1		(uint8_t)11

//Prog modes
#define EN_PROGRAM_A		1
#define EN_PROGRAM_B		0

//Available output states
#define EN_OUTPUT_HIGH		(uint8_t)1
#define EN_OUTPUT_LOW		(uint8_t)0

//Available input states swapped for optoisolator inversion
#define EN_INPUT_HIGH		(uint8_t)0
#define EN_INPUT_LOW		(uint8_t)1

//Temperature Constants
#define EN_TEMP_HIGH		(uint16_t)195  //40C
#define EN_TEMP_MED			(uint16_t)184  //36C 
#define EN_TEMP_LOW			(uint16_t)180  //0C

//Initialization functions
void initRelayOutputs();
void initStatusLed();
void initInputs();
void initTempSensor();
void initLatchOutput();

void checkRelayOutputs();
void checkStatusLED();

uint8_t readInput(ENInput_t en_input);

uint16_t readTempSens();

void writeRelayOutput(uint8_t en_output, uint8_t val);

void writeLEDOutput(uint8_t blue, uint8_t green, uint8_t red);

void toggleLEDOutput(uint8_t blue, uint8_t green, uint8_t red);

void writeBatteryLatch(ENOutput_t latch, uint8_t state);



#endif