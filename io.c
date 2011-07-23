#include "io.h"

//Defines
#define BTN_INC PORTCbits.RC0
#define BTN_DEC PORTAbits.RA6
#define CAPT1	PORTCbits.RC1

//Counter for delay's
unsigned char cnt_dec, cnt_inc, cnt_capt;
unsigned int delay_off, delay_unlock;

//Counter for input capture
unsigned char capt1;

//The buttons who have been pressed
unsigned char inputs;

typedef union {
	unsigned char byte;
	struct {
		unsigned btn_inc :1;
		unsigned btn_dec :1;
		unsigned capt1 :1;
		unsigned btn_ignore :1;
		unsigned bit4 :1;
		unsigned bit5 :1;
		unsigned bit6 :1;
		unsigned bit7 :1;
	};
} bits;

bits input;

void io_init() {
	
	//RC0, RA6 as input for buttons; RC1 as input for pulse
	TRISC|= 0b00000011;
	ANSELC&= 0b11111100;
	TRISA|= 0b01000000;
	ANSELA&=0b10111111;
	
	//OUTPUT=> O1: RB4, O2: RB5
	TRISB&=0b11001111;
	LATB&=0b11001111;	 //Disable output
	
	//Set default values
	input.btn_dec= input.btn_inc= input.btn_ignore= 0;
	cnt_dec= cnt_inc= cnt_capt= delay_off= delay_unlock= 0;
	capt1= 0;
	inputs= 0;
}

void io_loop() {
	
	//Button decrement
	if ( input.btn_dec == 0u && BTN_DEC == 1u && cnt_dec == 0u) {
		cnt_dec= 100;
		input.btn_dec= 1u;
		display_on(); //Show current address
	} else 	if (input.btn_dec == 1u && BTN_DEC == 0u && cnt_dec == 0u) {
		cnt_dec= 100;
		input.btn_dec= 0u;
		delay_unlock= 0u; //Reset unlocking time
		
		if (input.btn_inc == 0u) {
			if (!input.btn_ignore) {
				display_dec();
			}
			input.btn_ignore= 0u;
		} else {
			input.btn_ignore= 1u;
		}
	}
	//Button increment
	if ( input.btn_inc == 0u && BTN_INC == 1u && cnt_inc == 0u) {
		cnt_inc= 100;
		input.btn_inc= 1u;
		display_on(); //Show current address
	} else if (input.btn_inc == 1u && BTN_INC == 0u && cnt_inc == 0u) {
		cnt_inc= 100;
		input.btn_inc= 0u;
		delay_unlock= 0u; //Reset unlocking time
		
		if (input.btn_dec == 0u) {
			if (!input.btn_ignore) {
				display_inc();
			}
			input.btn_ignore= 0u;
		} else {
			input.btn_ignore= 1u;
		}
	}
	//Input capture
	if ( input.capt1 == 0u && CAPT1 == 1u && cnt_capt == 0u) {
		cnt_capt= 100;
		input.capt1= 1u;
	} else if (input.capt1 == 1u && CAPT1 == 0u && cnt_capt == 0u) {
		cnt_capt= 100;
		input.capt1= 0u;
		capt1++;
	}
	//both switches pressed
	if (BTN_INC == 1u && BTN_DEC == 1u) {
		if (cnt_dec == 0u && cnt_inc == 0u && delay_unlock == 0u) {
			delay_unlock= 2000;
		}
		
		//Unlock up/down counting.
		if (cnt_dec == 0u && cnt_inc == 0u && delay_unlock <= 200u) {
			display_unlock();
		}
	}
	
	//Display on
	if ( BTN_INC == 1u || BTN_DEC == 1u) {
		delay_off= 3000;
	}
	
	//Display off
	if ( delay_off == 0u ) {
		display_lock();
	}
}

unsigned char io_capt1() {
	unsigned char tmp= capt1;
	capt1=0;
	return tmp;
		
}

unsigned char io_getInputs() {
	return inputs;	
}

void io_enableOutput(char output) {
	if (output == 1) {
		LATB|=0x10;
	} else if (output == 2) {
		LATB|=0x20;
	}	
}

void io_disableOutput(char output) {
	if (output == 1) {
		LATB&=0xEF;
	} else if (output == 2) {
		LATB&=0xDF;
	}
}	

void io_cnt_int() {
	if (cnt_dec != 0u) {
		cnt_dec--;
	}
	
	if (cnt_inc != 0u) {
		cnt_inc--;	
	}

	if (cnt_capt != 0u) {
		cnt_capt--;	
	}
	if (delay_off != 0u) {
		delay_off--;
	}
	
	if (delay_unlock != 0u) {
		delay_unlock--;
	}
}
