#include "display.h"
#include <stdlib.h>

//Private variable declaration
char value, idle_timer, locked;

void display_init() {
	//Display2: RA0-4
	//Display1: RB2-5

	//Set port A/B as output (=0)
	TRISA&= 0b00001111;
	TRISB&= 0b11000011;

	//Read EEPROM
	value= 0;
	idle_timer= 0;
	locked= 0xff;
		
}

void display_show(char disp_value) {
	char buffer[3];
		
	//convert to ASCII and convert to binary
	itoa(disp_value,buffer);
	buffer[0]-= 0x30;
	buffer[1]-= 0x30; 
	
	//shift bits in to correct position
	PORTA= (buffer[0]&0x1)<<2 | (buffer[0]&0x2)<<5 | (buffer[0]&0x4)<<4 | (buffer[0]&0x8)<<3;
	PORTB= (buffer[0]&0x1) | (buffer[0]&0x2)<<3 | (buffer[0]&0x4)<<2 | (buffer[0]&0x8)<<1;
}

void display_lock() {
	locked= 0xff;	
}	
void display_unlock() {
	locked= 0x00;	
}

void display_idleReset() {
	//Everything above 9 = off.
	display_show(255);
}

void display_inc() {
	if (locked == 0x00) {
		value++;
		if (value > 32) {
			value= 0;
		}	
	}	
	display_show(value);
}

void display_dec() {
	if (locked == 0x00) {
		value--;
		if (value == 0) {
			value= 32;
		}
	}
	display_show(value);
}	