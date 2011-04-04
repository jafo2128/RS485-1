#include "hdlc.h"

const unsigned char DLE= 0x10;
const unsigned char STX= 0x02;
const unsigned char ETX= 0x03;

typedef union {
	unsigned char byte;
	struct {
		unsigned SFD :1; //Start of frame delimiter
		unsigned DLE :1;
		unsigned bit2 :1;
		unsigned bit3 :1;
		unsigned bit4 :1;
		unsigned bit5 :1;
		unsigned bit6 :1;
		unsigned NRM :1; //True if a connection setup has happened (U-frame,SNRM)
	};
} bits;		


//Receiving charter
bits control;
unsigned int current_size, received_size, cnt;
unsigned char address;
unsigned char receiving_data[7];
unsigned char received_data[7];
unsigned char send_data[6];

//HDLC
char counter_slave, counter_master;

void hdlc_init() {
	//Read address in EEPROM.
	EEADR= 0x0; //EEPROM address to read
	EECON1bits.EEPGD= 0; //Select data EEPROM
	EECON1bits.CFGS= 0;  //Acces EEPROM memory
	EECON1bits.RD= 1; //Read byte, auto cleared!
	address= EEDATA;  //Save EEPROM in address
	if (address > 32) {address= 0;}
	
	control.byte= 0;
	current_size= 0;
}

char hdlc_getAddress() {
	return address;
}
	
void hdlc_setAddress(char new_address) {
	address= new_address;
	
	EEADR= 0x0; //EEPROM address to write
	EEDATA= new_address; //Data to write
	EECON1bits.EEPGD= 0; //Select data EEPROM
	EECON1bits.CFGS= 0;  //Acces EEPROM memory
	EECON1bits.WREN= 1; //Enable write to EEPROM
	INTCONbits.GIE= 0; //Disable global interrupts
	EECON2= 0x55; //Required => see datacheet
	EECON2= 0x0AA; //same as above
	EECON1bits.WR= 1; //Write data
	INTCONbits.GIE= 1; //Re-enable global interrupts
	EECON1bits.WREN= 0; //disable write to EEPROM
}	


void hdlc_removeDLEs() {
	
}	

void hdlc_checkData() {
	if (received_data[0] == address) { //For this slave?
		
		if (control.NRM == (unsigned)0) { //Waiting on connection setup
			if (received_data[1]&0xC0 == 0xC0) { //Unnumbered Frame
				if (received_data[1]&0x37 == 0x01) { //Set normal response mode
					counter_slave= counter_master= 0;
					control.NRM= (unsigned)1;
					if (received_data[1]&0x08 == 0x08) { //Check poll flag
						// Send Unnumbered acknowledge
					}		
				}
			}
		} else {
		
			//Check type frame	
			if (received_data[1]&0x80 == 0x00) { //Information Frame
				
			} else if (received_data[1]&0xC0 == 0x80) { //Supervisory Frame
				//Supervisory Code
				if (received_data[1]&0x60 == 0x00) { //Receive Ready
					
				} else if (received_data[1]&0x60 == 0x10) { //Reject
				
				} else if (received_data[1]&0x60 == 0x20) { //Receive Not Ready
				
				} else if (received_data[1]&0x60 == 0x30) { //Selective reject
				
				}	
			
			} else if (received_data[1]&0xC0 == 0xC0) { //Unnumbered Frame
				if (received_data[1]&0x37 == 0x01) { //Set normal response mode
					
				} else if (received_data[1]&0x37 == 0x02) { //Disconnect
					if (received_data[1]&0x08 == 0x08) { //Check poll flag
						control.NRM= (unsigned)0;
						// Send Unnumbered acknowledge
					}
				} else if (received_data[1]&0x37 == 0x0C) { //Unnumbered acknowledge
				
				}
			}
		}
	}		
	
}	

void hdlc_read(unsigned char input) {
	//Drop data => to many packets!
	if (current_size >= (unsigned)6) {
		control.SFD= (unsigned)0;
		control.DLE= (unsigned)0;
	}
	
	//Waiting on Start of frame: DLE & STX
	if (control.SFD == (unsigned)0) {
		if (input == DLE) {control.DLE == (unsigned)1;} //DLE found => next STX!
		else if (control.DLE && input == STX) { //Start of frame found!
			control.SFD= (unsigned)1;
			control.DLE= (unsigned)0;
			current_size= (unsigned)0;
		} else {control.DLE= (unsigned)0;} //Not valid charter
	} else {
		//Save al charters for CRC and handeling of commands
		receiving_data[current_size++]= input;
		
		//Testing for invalid start of frame
		if (input == DLE) {control.DLE!= control.DLE;}
		else if (control.DLE && input == STX) { //Reset was invalid start of frame!
			current_size= (unsigned)0;
			control.DLE= (unsigned)0;
		} else if (control.DLE && input == ETX) { //End of frame found :):)
			control.SFD= (unsigned)0; //We have to resync
			received_size= current_size;
			received_data[0]= receiving_data[0];
			received_data[1]= receiving_data[1];
			received_data[2]= receiving_data[2];
			received_data[3]= receiving_data[3];
			received_data[4]= receiving_data[4];
			received_data[5]= receiving_data[5];
			received_data[6]= receiving_data[6];
			if (crc16_calc(received_data, received_size)) {
				hdlc_removeDLEs;
				hdlc_checkData;
			} else {
				//Send S-Frame, REJ
			}
		} else {
			control.DLE= (unsigned)0;
		}
	}
}

char hdlc_write(void) {
	
}	
