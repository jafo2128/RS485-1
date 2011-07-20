#include "display.h"
#include <stdlib.h>

//Private variable declaration
char value, idle_timer, locked;
char port;

void display_init() {
	//Set port B/D as output (=0)
	TRISB&= 0b11110000;
	ANSELB&=0b00001111;
	TRISD&= 0b00001111;
	ANSELD&=0b11110000;

	//Read EEPROM
	value= hdlc_getAddress();
	idle_timer= 0;
	locked= 0xff;
	
	display_show(0xff);
}	

void display_show(char disp_value) {
	char buffer[4];	
	
	if (disp_value != 0xff) {
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
	port= (buffer[1]&0x1) | (buffer[1]&0x2)<<2 | (buffer[1]&0x4) | (buffer[1]&0x8)>>2;
	LATB= (PORTB&0xF0)|port;
	port= (buffer[0]&0x1)<<4 | (buffer[0]&0x2)<<6 | (buffer[0]&0x4)<<4 | (buffer[0]&0x8)<<2;
	LATD= (PORTD&0xF0)|port;
}

void display_lock() {
	locked= 0xff;
	
	//Everything above 9 = off.
	display_show(255);
	
	//Write new address to EEPROM, only if changed
	if (value != hdlc_getAddress()) {
		hdlc_setAddress(value);
	}
}
	
void display_unlock() {
	locked= 0x00;
}

void display_on() {
	display_show(value);
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
