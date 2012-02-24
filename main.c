//Configurations
//#pragma config FOSC= INTIO67, PLLCFG= OFF, PRICLKEN= ON, FCMEN= OFF, IESO= OFF, PWRTEN= OFF
//#pragma config WDTEN= OFF, BOREN = OFF
//#pragma config LVP= OFF, MCLRE= EXTMCLR, DEBUG = ON

//#include <p18f45k22.h>
#include <htc.h>


//Default C library's
//#include <stdio.h>

//Include project library's
#include "io.h"
#include "display.h"
#include "hdlc.h"
#include "CRC16.h"

char CRC[8];

void main (void)
{
	//Init internal oscillator
	OSCCON= 0b11110111;
	OSCCON2= 0b00000011;
	
	//Init library's
	io_init();
	hdlc_init();
	display_init(); //After hdlc_init, otherwise wrong address. Maybe working with poiters??
	
	//Enable global interrupts: High priority: 0008h, Low priority: 0018h
	RCONbits.IPEN= 0;
	INTCONbits.PEIE= 1;
	INTCONbits.GIE= 1;
	
	//Enable watchdog
	#pragma config WDTEN = 2
	#pragma config WDTPS= 0x02
	//CONFIG2H|= 0x03; //Enabele WDT
	//CONFIG2H|= 0x08; //Set postscaler to 1:4
	
	//Main loop	
	while (1) {
		CLRWDT();
		io_loop();
		while (PIR1bits.RC1IF) {
			hdlc_receive(RCREG1);
		}
		if (TXSTA1bits.TRMT == 1u) { //Disable transmitter after last byte.
			PORTC&= 0xCF; //Disable transmit, enable receive
		}
	}	
}


void interrupt InterruptHandlerHigh () {
	if (PIR1bits.TX1IF) { //check for UART1 transmit interrupt
		hdlc_transmit();
	}
	
	if (PIR1bits.TMR2IF) { //check for TMR2 match                                   
			PIR1bits.TMR2IF= 0;            //clear interrupt flag
			io_cnt_int();
	}
	
	if (INTCONbits.TMR0IF) {
		INTCONbits.TMR0IF= 0;
		io_control_rs485_int();	
	}	
}

void interrupt low_priority InterruptHandlerLow() {

}
