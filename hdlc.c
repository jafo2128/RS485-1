#include "hdlc.h"

typedef union {
	unsigned char byte;
	struct {
		unsigned SFD :1; //Start of frame delimiter
		unsigned button2 :1;
		unsigned bit2 :1;
		unsigned bit3 :1;
		unsigned bit4 :1;
		unsigned bit5 :1;
		unsigned bit6 :1;
		unsigned bit7 :1;
	};
} bits;		

bits control;
unsigned int current_size, received_size;
unsigned char address;
unsigned char receiving_data[7];
unsigned char received_data[7];
unsigned char send_data[6];

void hdlc_init() {
	control.byte= 0;
	current_size= 0;
}

void hdlc_checkData(char cnt) {
	
}	

void hdlc_read(unsigned char input) {
	//Drop data => to many packets!
	if (current_size > (unsigned)6) {
		current_size= 0;	
	}
	
	//Waiting on "Start of frame delimiter
	if (input == (unsigned)0x7E) {
		if (current_size >= (unsigned)3) { //Previous frame has valid size
			receiving_data[current_size]= 0x0; //null pointer for strcpy
			strcpy(received_data, receiving_data);
			received_size= current_size;
			
			if (crc16_calc(received_data, received_size) != 0) { //Calc CRC, if invalid, drop!
				
			}

		}	
		control.SFD= 1;
		current_size= 0;
	} else if (control.SFD == (unsigned)1 || current_size >= (unsigned)0) {
		receiving_data[current_size]= input;
		current_size++;
		//counter= 1 => Adress header
		if (current_size == (unsigned)1) {
			control.SFD= 0;
			if (input != address) { //Not for us, ...
				current_size= 0;
			}
		}				 
	}
	
}

char hdlc_write(void);
