#include <p18f25k22.h>

// Configurations
//Oscillator
#pragma config FOSC= INTIO67, PLLCFG= OFF, PRICLKEN= ON, FCMEN= OFF, IESO= OFF, PWRTEN= OFF
#pragma config WDTEN= OFF, BOREN = OFF
#pragma config LVP= OFF, MCLRE= EXTMCLR, DEBUG = ON


//Default C library's
//#include <stdio.h>
#include "buttons.h"


//Project library's


void main (void)
{
	//Init
	buttons_init();
	
	//Main loop
	while (1) {
		buttons_loop();
		buttons_cnt_int();
		Nop();
	}	
}
