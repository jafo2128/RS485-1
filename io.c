#include "io.h"

//Defines
#define BTN_DEC PORTCbits.RC0
#define BTN_INC PORTAbits.RA6
#define CAPT1	PORTCbits.RC1
#define INPUT1	(PORTA&0b00111111)
#define INPUT2	(PORTE&0b00000111)

//Counter for delay's
unsigned char cnt_dec, cnt_inc, cnt_capt, cnt_input;
unsigned int delay_off, delay_unlock;

//Counter for input capture
unsigned int puls_counter;

//The buttons who have been pressed
unsigned char inputs;

//Keeps the current status of the RS485 LED
unsigned char rs485;

bit old_btn_inc= 0;
bit old_btn_dec= 0;
bit capt1= 0;
bit btn_ignore=0 ;
bit buttons_input= 0;

void io_init() {
	
	//RC0, RA6 as input for buttons; RC1 as input for pulse
	TRISC|= 0b00000011;
	ANSELC&= 0b11111100;
	TRISA|= 0b01000000;
	ANSELA&=0b10111111;
	
	//Set I1-9: RA0, RA1, RA2, RA3, RA4, RA5, RE0, RE1, RE2.
	TRISA|= 0b00111111;
	ANSELA&= 0b11000000;
	TRISE|= 0b00000111;
	ANSELE&= 0b11111000;

	
	//OUTPUT=> O1: RB4, O2: RB5
	TRISB&=0b11001111;
	LATB&=0b11001111;	 //Disable output
	
	//Set default values
	buttons_input= old_btn_dec= old_btn_inc= btn_ignore= 0;
	cnt_dec= cnt_inc= cnt_capt= cnt_input= delay_off= delay_unlock= 0;
	capt1= 0;
	
	//Init timer 2: 16MHz/4, prescaler 1:16. Used for debouncing
	T2CON=  0b00000110;
	PR2= 250; //interrupt 1 ms
	PIR1bits.TMR2IF= 0;
	PIE1bits.TMR2IE= 1; //TMR2 to PR2 Match Interrupt Enable bit: 1= enable
	IPR1bits.TMR2IP= 1; //TMR2 to PR2 Match Interrupt Priority bit: 0= Low priority
	
	//Init timer 0: 16MHz/4, prescaler 16. Used for RS2485 LED
	T0CON= 0b10000011; //interrupt ~0,262s
	INTCONbits.TMR0IE= 1;
	INTCON2bits.TMR0IP= 1;
}

void io_loop() {
	
	//Button decrement
	if ( BTN_DEC == 1u && old_btn_dec == 0u && cnt_dec == 0u ) {
		cnt_dec= 100;
		old_btn_dec= 1u;
		display_on(); //Show current address
	} else 	if ( BTN_DEC == 0u && old_btn_dec == 1u && cnt_dec == 0u ) {
		cnt_dec= 100;
		old_btn_dec= 0u;
		delay_unlock= 0u; //Reset unlocking time
		
		if (old_btn_inc == 0u) {
			if (!btn_ignore) {
				display_dec();
			}
			btn_ignore= 0u;
		} else {
			btn_ignore= 1u;
		}
	}
	//Button increment
	if ( BTN_INC == 1u && old_btn_inc == 0u && cnt_inc == 0u ) {
		cnt_inc= 100;
		old_btn_inc= 1u;
		display_on(); //Show current address
	} else if ( BTN_INC == 0u && old_btn_inc == 1u && cnt_inc == 0u ) {
		cnt_inc= 100;
		old_btn_inc= 0u;
		delay_unlock= 0u; //Reset unlocking time
		
		if (old_btn_dec == 0u) {
			if (!btn_ignore) {
				display_inc();
			}
			btn_ignore= 0u;
		} else {
			btn_ignore= 1u;
		}
	}
	
	//Input buttons
	if ( buttons_input == 0u && (INPUT1 == 0u && INPUT2 == 0u) && cnt_input == 0u ) {
		cnt_input= 100;
		buttons_input= 1u;
	} else if (buttons_input == 1u && (INPUT1 != 0u || INPUT2 != 0u) && cnt_input == 0u ) {
		cnt_input= 100;
		buttons_input= 0u;
		if (INPUT1&0x01) {inputs=9;}
		if (INPUT1&0x02) {inputs=8;}
		if (INPUT1&0x04) {inputs=7;}
		if (INPUT1&0x08) {inputs=6;}
		if (INPUT1&0x10) {inputs=5;}
		if (INPUT1&0x20) {inputs=4;}
		if (INPUT2&0x01) {inputs=3;}
		if (INPUT2&0x02) {inputs=2;}
		if (INPUT2&0x04) {inputs=1;}
	}
	//Input capture
	if ( CAPT1 == 1u && capt1 == 0u && cnt_capt == 0u ) {
		cnt_capt= 100;
		capt1= 1;
	} else if ( CAPT1 == 0u && capt1 == 1u && cnt_capt == 0u ) {
		cnt_capt= 100;
		capt1= 0;
		puls_counter++;
	}
	//both switches pressed
	if ( BTN_INC != 0u && BTN_DEC != 0u ) {
		if (cnt_dec == 0u && cnt_inc == 0u && delay_unlock == 0u) {
			delay_unlock= 2000;
		}
		
		//Unlock up/down counting.
		if (cnt_dec == 0u && cnt_inc == 0u && delay_unlock <= 200u) {
			display_unlock();
		}
	}
	
	//Display on
	if ( BTN_INC != 0u || BTN_DEC != 0u) {
		delay_off= 3000;
	}
	
	//Display off
	if ( delay_off == 0u ) {
		display_lock();
	}
}

unsigned int io_getCapt1() {
	unsigned int tmp= puls_counter;
	puls_counter=0;
	return tmp;
		
}

unsigned char io_getInputs() {
	unsigned char tmp= inputs;
	inputs= 0;
	return tmp;	
}

void io_enableOutput(char output) {
	if (output == 1) {
		LATB|=0x20;
	} else if (output == 2) {
		LATB|=0x10;
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
	
	if (cnt_input != 0u) {
		cnt_input--;	
	}	
	
	if (delay_off != 0u) {
		delay_off--;
	}
	
	if (delay_unlock != 0u) {
		delay_unlock--;
	}
}	

void io_control_rs485(char forme) {
	if (forme != 0) {
		rs485= 0xff;	
	} else if (rs485 == 0u) {
		rs485= 0x5f;
	} 
}

void io_control_rs485_reset() {
	rs485= 0x0;	
}	
	
void io_control_rs485_int() {
	if (rs485 != 0u) {
		if (rs485 >= 0xf0) { //Valid packets received for this ID => blink 
			if ( (LATD&0x08) == 0u) {
				LATD|=0x08; //Enable LED	
			} else {
				LATD&=0xF7; //Disable LED
			}
		} else  { //Only valid packets received, but not for me
			LATD|=0x08; //Enable LED
		}
		rs485--;
		if (rs485 == 0xf0u || rs485 == 0x50u) {
			rs485= 0;
		}
	} else { //Nothing received => disable
		LATD&=0xF7; //Disable LED
	}	
}
