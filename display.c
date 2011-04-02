#include "display.h"
#include <stdlib.h>

//Private variable declaration
char value, idle_timer, locked;

void display_show(char disp_value);

void display_init() {
	//Display2: RA0-4
	//Display1: RB2-5

	//Set port A/B as output (=0)
	TRISA&= 0b11110000;
	TRISB&= 0b11000011;

	//Read EEPROM
	value= 0;
	idle_timer= 0;
	locked= 0xff;
	
	display_show(value);
}

void display_show(char disp_value) {
	char buffer[4];	
	
	if (locked != 0xff) {
		//convert to ASCII and convert to binary
		btoa(disp_value,buffer);
		if (buffer[1] >= 0x30 && buffer[1]  <= 0x39) {//Valid ASCII charter: 0-9
			buffer[2]= buffer[0]; //Switching places for btoa
			buffer[0]= buffer[1]-0x30; //ASCII to binary
			buffer[1]= buffer[2]-0x30; 
		} else {
			buffer[1]= 0xff; //Hide leading zero
			if (buffer[0] >= 0x30 && buffer[0]  <= 0x39) { //Valid ASCII charter
				buffer[1]-= 0x30;
			} else {
				buffer[1]= 0;
			}
		}
	} else {
		buffer[0]= buffer[1]= 0xff;
	}	
	
	//shift bits in to correct position and send to BCD-to-7seg
	PORTA= (buffer[0]&0x1) | (buffer[0]&0x2)<<2 | (buffer[0]&0x4) | (buffer[0]&0x8)>>2;
	PORTB= (buffer[1]&0x1)<<2 | (buffer[1]&0x2)<<4 | (buffer[1]&0x4)<<2 | (buffer[1]&0x8);
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
	if (locked == 0x00 && value < 32) {
		value++;	
	}	
	display_show(value);
}

void display_dec() {
	if (locked == 0x00 && value > 0) {
		value--;
	}
	display_show(value);
}	
