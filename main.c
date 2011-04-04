#include <p18f25k22.h>

//Configurations
//Oscillator
#pragma config FOSC= INTIO67, PLLCFG= OFF, PRICLKEN= ON, FCMEN= OFF, IESO= OFF, PWRTEN= OFF
#pragma config WDTEN= OFF, BOREN = OFF
#pragma config LVP= OFF, MCLRE= EXTMCLR, DEBUG = ON


//Default C library's
//#include <stdio.h>

//Include project library's
#include "input.h"
#include "display.h"
#include "hdlc.h"


void InterruptHandlerHigh (void);
void InterruptHandlerLow (void);

void main (void)
{
	//Init internal oscillator
	OSCCON= 0b11110111;
	OSCCON2= 0b00000011;
	
	//Init timer 2: 16MHz/4, postscaler 1:16
	T2CON=  0b00000110;
	PR2= 250; //interrupt 1 ms
	PIR1bits.TMR2IF= 0;
	PIE1bits.TMR2IE= 1; //TMR2 to PR2 Match Interrupt Enable bit: 1= enable
	IPR1bits.TMR2IP= 1; //TMR2 to PR2 Match Interrupt Priority bit: 0= Low priority
	
	
	//Init library's
	input_init();
	hdlc_init();
	display_init(); //After hdlc_init, otherwise wrong address. Maybe working with poiters??
	
	//Enable global interrupts: High priority: 0008h, Low priority: 0018h
	RCONbits.IPEN= 0;
	INTCONbits.PEIE= 1;
	INTCONbits.GIE= 1;
	
	//Main loop
	while (1) {
		input_loop();
	}	
}


//----------------------------------------------------------------------------
// High priority interrupt vector

#pragma code InterruptVectorHigh = 0x08
void InterruptVectorHigh (void) {
  _asm
    goto InterruptHandlerHigh //jump to interrupt routine
  _endasm
}

//----------------------------------------------------------------------------
// High priority interrupt routine

#pragma code
#pragma interrupt InterruptHandlerHigh

void InterruptHandlerHigh () {
	if (PIR1bits.TMR2IF) { //check for TMR2 match                                   
			PIR1bits.TMR2IF= 0;            //clear interrupt flag
			input_cnt_int();
	}
}

//----------------------------------------------------------------------------


//----------------------------------------------------------------------------
// Low priority interrupt vector

#pragma code InterruptVectorLow = 0x018
void InterruptVectorLow (void) {
  _asm
    goto InterruptHandlerLow //jump to interrupt routine
  _endasm
}

//----------------------------------------------------------------------------
// Low priority interrupt routine

#pragma code
#pragma interrupt InterruptHandlerHigh

void InterruptHandlerLow () {
	if (PIR1bits.TMR2IF) { //check for TMR2 match                                   
			PIR1bits.TMR2IF= 0;            //clear interrupt flag
			input_cnt_int();
	}
}

//----------------------------------------------------------------------------
