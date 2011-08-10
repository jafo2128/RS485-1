#include "display.h"
#include <stdlib.h>

//Private variable declaration
unsigned char value, idle_timer, locked, status;
char port;

//For display
char buffer[4];

void display_init() {
	//Set port B/D as output (=0)
	TRISB&= 0b11110000;
	TRISD&= 0b00001111;

	//Read EEPROM
	value= hdlc_getAddress();
	idle_timer= 0;
	locked= 0xff;
	status= 0x00;
	
	//Reset variables
	buffer[0]= buffer[1]= buffer[2]= buffer[3]= 0;
	
	display_show(0xff);
}	

void display_show(unsigned char disp_value) {	
	
	if (disp_value != 0xffu) {
		//convert to ASCII and convert to binary
		itoa(buffer, disp_value, 10);
		if (buffer[1] >= 0x30 && buffer[1]  <= 0x39) {//Valid ASCII charter: 0-9
			buffer[2]= buffer[0]; //Switching places for btoa
			buffer[0]= buffer[1]-0x30; //ASCII to binary
			buffer[1]= buffer[2]-0x30;
		} else {
			buffer[1]= 0xff; //Hide leading zero
			if (buffer[0] >= 0x30 && buffer[0]  <= 0x39) { //Valid ASCII charter
				buffer[0]-= 0x30;
			} else {
				buffer[0]= 0;
			}
		}
	} else {
		buffer[0]= buffer[1]= 0xff;
	}
	
	//shift bits in to correct position and send to BCD-to-7seg
	port= (buffer[1]&0x1) | (buffer[1]&0x2)<<2 | (buffer[1]&0x4) | (buffer[1]&0x8)>>2;
	LATB= (LATB&0xF0)|port;
	port= (buffer[0]&0x1)<<4 | (buffer[0]&0x2)<<6 | (buffer[0]&0x4)<<4 | (buffer[0]&0x8)<<2;
	LATD= (LATD&0x0F)|port;
}

void display_lock() {
	if (status == 0xffu)	{
		locked= 0xff;
		status= 0x00;
		
		//Everything above 9 = off.
		display_show(255);
		
		//Write new address to EEPROM, only if changed
		if (value != hdlc_getAddress()) {
			io_control_rs485_reset();
			hdlc_setAddress(value);
		}
	}	
}
	
void display_unlock() {
	locked= 0x00;
}

void display_on() {
	display_show(value);
	status= 0xff;
}	

void display_inc() {
	if (locked == 0x00u && value++ >= 64u) {
		value= 1;
	}		
	display_show(value);
}

void display_dec() {
	if (locked == 0x00u && value > 0u) {
		value--;
	}
	display_show(value);
}	
