#include "input.h"

//Private variable declaration

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
	
	//RC2, RC3 as input.
	TRISC|= 0b000001100;
	ANSELC&= 0b11110011;
	
	//RB0 as input
	TRISB|= 0b00000001;
	ANSELB&=0b11111110;
	
	
	//Set default values
	input.btn_dec= input.btn_inc= input.btn_ignore= 0;
	cnt1= cnt2= delay_off= 0;
	capt1= 0;
	
}

void input_loop() {
	
	//Button decrement
	if ( input.btn_dec == (unsigned)0 && PORTCbits.RC2 == (unsigned)1 && cnt1 == (unsigned)0) {
		cnt1= 100;
		input.btn_dec= (unsigned)1;
		display_on();
	} else 	if (input.btn_dec == (unsigned)1 && PORTCbits.RC2 == (unsigned)0 && cnt1 == (unsigned)0) {
		cnt1= 100;
		input.btn_dec= (unsigned)0;
		
		if (input.btn_inc == (unsigned)0) {
			if (!input.btn_ignore) {
				display_dec();
			}
			input.btn_ignore= 0;
		} else {;
			input.btn_ignore= 1;
		}
	}
	//Button increment
	if ( input.btn_inc == (unsigned)0 && PORTCbits.RC3 == (unsigned)1 && cnt2 == (unsigned)0) {
		cnt2= 100;
		input.btn_inc= (unsigned)1;
		display_on();
	} else if (input.btn_inc == (unsigned)1 && PORTCbits.RC3 == (unsigned)0 && cnt2 == (unsigned)0) {
		cnt2= 100;
		input.btn_inc= (unsigned)0;
		
		if (input.btn_dec == (unsigned)0) {
			if (!input.btn_ignore) {
				display_inc();
			}
			input.btn_ignore= 0;
		} else {
			input.btn_ignore= 1;		
		}
	} 
	//Input capture
	if ( input.capt1 == (unsigned)0 && PORTBbits.RB0 == (unsigned)1 && cnt3 == (unsigned)0) {
		cnt3= 100;
		input.capt1= (unsigned)1;
	} else if (input.capt1 == (unsigned)1 && PORTBbits.RB0 == (unsigned)0 && cnt3 == (unsigned)0) {
		cnt3= 100;
		input.capt1= (unsigned)0;
		capt1++;					
	}
	//both switches pressed
	if (PORTCbits.RC2 == (unsigned)1 && PORTCbits.RC3 == (unsigned)1 && cnt1 == (unsigned)0 && cnt2 == (unsigned)0) {
		if (cnt1 != (unsigned)0 || cnt2 != (unsigned)0 ) {
			delay_unlock= 2000;
		}
		delay_unlock--;
		if (delay_unlock == (unsigned)0) {
			display_unlock();
		}
	}
	
	//Display on
	if ( PORTCbits.RC2 == (unsigned)1 || PORTCbits.RC3 == (unsigned)1) {
		delay_off= 3000;	
	}
	
	//Display off
	if ( delay_off == 0 ) {
		display_lock();
	}	
}

void input_cnt_int() {
	if (cnt1 != (unsigned)0) {
		cnt1--;
	}
	
	if (cnt2 != (unsigned)0) {
		cnt2--;	
	}
	
	if (delay_off != (unsigned)0) {
		delay_off--;
	}
}
		
