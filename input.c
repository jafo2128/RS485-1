#include "input.h"

//Defines
#define BTN_INC PORTCbits.RC0
#define BTN_DEC PORTAbits.RA6
#define CAPT1	PORTCbits.RC1

//Counter for delay's
unsigned char cnt1, cnt2, cnt3;
unsigned int delay_off, delay_unlock;

//Counter for input capture
unsigned char capt1;

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

void input_init() {
	
	//RC0, RA6 as input for buttons; RC1 as input for pulse
	TRISC|= 0b00000011;
	ANSELC&= 0b11111100;
	TRISA|= 0b01000000;
	ANSELA&=0b10111111;
	
	//Set default values
	input.btn_dec= input.btn_inc= input.btn_ignore= 0;
	cnt1= cnt2= delay_off= delay_unlock= 0;
	capt1= 0;
}

void input_loop() {
	
	//Button decrement
	if ( input.btn_dec == 0u && BTN_DEC == 1u && cnt1 == 0u) {
		cnt1= 100;
		input.btn_dec= 1u;
		display_on(); //Show current address
	} else 	if (input.btn_dec == 1u && BTN_DEC == 0u && cnt1 == 0u) {
		cnt1= 100;
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
	if ( input.btn_inc == 0u && BTN_INC == 1u && cnt2 == 0u) {
		cnt2= 100;
		input.btn_inc= 1u;
		display_on(); //Show current address
	} else if (input.btn_inc == 1u && BTN_INC == 0u && cnt2 == 0u) {
		cnt2= 100;
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
	if ( input.capt1 == 0u && CAPT1 == 1u && cnt3 == 0u) {
		cnt3= 100;
		input.capt1= 1u;
	} else if (input.capt1 == 1u && CAPT1 == 0u && cnt3 == 0u) {
		cnt3= 100;
		input.capt1= 0u;
		capt1++;
		display_on(); //Testing Input capture
		display_show(capt1);
	}
	//both switches pressed
	if (BTN_INC == 1u && BTN_DEC == 1u) {
		if (cnt1 == 0u && cnt2 == 0u && delay_unlock == 0u) {
			delay_unlock= 2000;
		}
		
		//Unlock up/down counting.
		if (cnt1 == 0u && cnt2 == 0u && delay_unlock <= 200u) {
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

unsigned char input_capt1() {
	unsigned char tmp= capt1;
	capt1=0;
	return tmp;
		
}	

void input_cnt_int() {
	if (cnt1 != 0u) {
		cnt1--;
	}
	
	if (cnt2 != 0u) {
		cnt2--;	
	}

	if (cnt3 != 0u) {
		cnt3--;	
	}
	if (delay_off != 0u) {
		delay_off--;
	}
	
	if (delay_unlock != 0u) {
		delay_unlock--;
	}
}
