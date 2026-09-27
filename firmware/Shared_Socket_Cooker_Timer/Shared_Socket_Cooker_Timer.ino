// SPDX-FileCopyrightText: Johannes Stockhammer
// SPDX-License-Identifier: Apache-2.0
/*
 * Shared_Socket_Cooker_Timer.ino
 *
 *  Author: Johannes Stockhammer
 *
 * Version:      2.0
 * Hardware:     ATmega32U4 (Arduino Leonardo/Micro) or ATmega2560 (Arduino Mega 2560), 16 MHz,
 *               changeover relay (SPDT) with driver, 2 status LEDs, optional restart jumper
 * Software:     Arduino IDE, own main()
 * Description:  Only one 230 V socket (~2.5 kW) is available, but two meals have to be cooked.
 *               The changeover relay switches the socket to pot A and pot B alternately,
 *               every 5 min. LED A / LED B show which pot is powered.
 *               HEATUP_ENABLED 1: the first two phases heat each pot for 15 min.
 *               Phase and remaining time are saved in the EEPROM about once a minute;
 *               after a power loss the timer carries on there. Pin 7 to GND at power-up
 *               = start fresh at pot A.
 */

//Includes
#include <util/atomic.h>
#include <util/delay.h>
#include "State_Storage.h"

//Defines
	//Options
#define HEATUP_ENABLED 0							//1 = first heat up each pot for 15 min
#define RESUME_ENABLED 1							//1 = carry on after power loss, 0 = always start at pot A
	//Times (Timer3 overflow: 65536 / (16MHz / 256) = 1,048576s)
#define TIME_1min 57								//(60s / 1,048576s) = 57
#define TIME_5min 286								//(300s / 1,048576s) = 286
#define TIME_15min 858								//(900s / 1,048576s) = 858
#define TIME_SAVE TIME_1min							//EEPROM save interval
	//States
#define STARTUP 0									//Load time of the next phase
#define WAIT 1										//Wait until the phase is over
	//Pins
#define PIN_POT 4									//Changeover relay pot A / pot B
#define PIN_LED_A 6									//Status LED pot A
#define PIN_LED_B 5									//Status LED pot B
#define PIN_RESTART 7								//Pull-up, to GND at power-up = restart at pot A
	//Relay
#define POT_A 1										//Pin 4 HIGH = pot A
#define POT_B 0										//Pin 4 LOW = pot B

//Variables
	//Timers (counted down in the ISR)
volatile uint16_t timer_sec=0;						//16 bit -> access only in ATOMIC_BLOCK
volatile uint8_t timer_save=0;						//8 bit -> access is atomic
	//Sequence
uint8_t state=STARTUP;
uint8_t pot=POT_A;									//Pot powered at the moment
uint16_t cycle=0;									//Running phase (1 = first phase, odd = pot A)
uint16_t timer_sec_copy=0;							//Copy of timer_sec for the main loop
	//Saved state
uint16_t cycle_saved=0, time_left_saved=0;
bool state_found=0, restart=0;

//Prototypes
uint16_t Phase_Time(uint16_t cycle);
void Set_Outputs(uint8_t pot);

int main()
{
	//Timer
		//16-bit Timer 3
	TCCR3A=0b00000000;								//Normal mode
	TCCR3B=0b00000100;								//Prescaler CLK/256 -> overflow every 1,048576s
	TCCR3C=0b00000000;
	TIMSK3=0b00000001;								//Timer 3 overflow interrupt on

	//Inputs/Outputs
		//Outputs
	pinMode(PIN_POT,OUTPUT);						//Pin 4 as output for changeover relay
	pinMode(PIN_LED_A,OUTPUT);						//Pin 6 as output for LED pot A
	pinMode(PIN_LED_B,OUTPUT);						//Pin 5 as output for LED pot B
		//Inputs
	pinMode(PIN_RESTART,INPUT_PULLUP);				//Pin 7 as input with pull-up
	_delay_us(100);									//Let the pull-up settle

	//Load saved state
	state_found=State_Load(cycle_saved,time_left_saved);	//Always call: also finds the write position
	restart=!digitalRead(PIN_RESTART);				//LOW = jumper to GND -> start fresh
	if(RESUME_ENABLED&&state_found&&!restart)
	{
		cycle=cycle_saved;							//Carry on in the saved phase
		if(cycle&1) pot=POT_A;						//Odd phase = pot A
		else pot=POT_B;
		if(time_left_saved>Phase_Time(cycle)) time_left_saved=Phase_Time(cycle);	//Plausibility check
		timer_sec=time_left_saved;					//Interrupts still off -> no ATOMIC_BLOCK needed
		timer_save=TIME_SAVE;
		state=WAIT;									//Wait out the rest of the phase
	}
	Set_Outputs(pot);								//Relay + LEDs

	//Interrupt
	sei();											//Switch on interrupts globally

	while(1)		//Main loop
	{
		ATOMIC_BLOCK(ATOMIC_RESTORESTATE)			//Read 16 bit atomically
		{
			timer_sec_copy=timer_sec;
		}

		switch(state)
		{
			case STARTUP:	cycle++;								//Next phase
							ATOMIC_BLOCK(ATOMIC_RESTORESTATE)		//Write 16 bit atomically
							{
								timer_sec=Phase_Time(cycle);		//Load phase time
							}
							State_Save(cycle,Phase_Time(cycle));	//Save new phase immediately
							timer_save=TIME_SAVE;					//Next save in ~1 min
							state=WAIT;								//Jump to the next case
							break;

			case WAIT:		if(!timer_sec_copy)						//Phase time over
							{
								pot=!pot;							//Relay to the other pot
								Set_Outputs(pot);
								state=STARTUP;						//Prepare the next phase
							}
							else
							{
								if(!timer_save)						//~1 min over
								{
									State_Save(cycle,timer_sec_copy);	//Save phase + remaining time
									timer_save=TIME_SAVE;
								}
							}
							break;
		}//end switch state
	}//end while
}//end main

//----------------------------------------------------------//

uint16_t Phase_Time(uint16_t cycle)					//Length of a phase in ticks
{
	if((HEATUP_ENABLED)&&(cycle<=2)) return TIME_15min;	//First two phases: heat up each pot
	return TIME_5min;								//5 min alternation
}//end Phase_Time

void Set_Outputs(uint8_t pot)						//Relay + matching LED
{
	digitalWrite(PIN_POT,pot);						//Changeover relay
	digitalWrite(PIN_LED_A,pot==POT_A);				//LED A on at pot A
	digitalWrite(PIN_LED_B,pot==POT_B);				//LED B on at pot B
}//end Set_Outputs

ISR(TIMER3_OVF_vect)		//every 1,048576s
{
	if(timer_sec) timer_sec--;
	if(timer_save) timer_save--;
}
