#include "buttons.h"

//Private variable declaration

char cnt1, cnt2;

typedef union {
	unsigned char byte;
	struct {
		unsigned button1 :1;
		unsigned button2 :1;
		unsigned bit2 :1;
		unsigned bit3 :1;
		unsigned bit4 :1;
		unsigned bit5 :1;
		unsigned bit6 :1;
		unsigned bit7 :1;
	};
} bits;		

bits btn;

void buttons_init() {
	
	display_init();
	
	//RC2, RC3 as input.
	TRISC|= 0b00110000;
	
	cnt1= cnt2 = 0;
	
}

void buttons_loop() {
	
	//Input changed?
	if ( btn.button1 != PORTCbits.RC2 && cnt1 == 0) {
		cnt1=20;
		
	}	
	if ( btn.button2 != PORTCbits.RC3 && cnt2 == 0) {
		cnt2= 20;
	}
	
	//Still changed?
	if (cnt1 == 1) {
		cnt1= 0;
		if (btn.button1 != PORTCbits.RC2) {
			btn.button1^= 1;
			//falling edge and only 1 button
			if (btn.button1 == (unsigned)0 && btn.button2 == (unsigned)0 && cnt2 == 0) {
				display_inc();
			} else if (btn.button1 == (unsigned)1 && btn.button2 == (unsigned)1) {
				display_unlock();
			}
		}						
	}	
	if (cnt2 == 1) {
		cnt2= 0;
		if (btn.button2 != PORTCbits.RC3) {
			btn.button2^= 1;
			//falling edge and only 1 button
			if (btn.button2 == (unsigned)0 && btn.button1 == (unsigned)0 && cnt1 == 0) {
				display_dec();
			} else if (btn.button1 == (unsigned)1 && btn.button2 == (unsigned)1) {
				display_unlock();
			}
		}						
	}		
}

void buttons_cnt_int() {
	if (cnt1 != 0) {
		cnt1--;
	}
	
	if (cnt2 != 0) {
		cnt2--;	
	}	 
}
		
