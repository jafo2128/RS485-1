#include "hdlc.h"

#define  BUFFER_SIZE     8

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


//Receiving charters
bits control;
unsigned int current_size, received_size, cnt;
unsigned char address;
unsigned char receiving_data[BUFFER_SIZE];
unsigned char received_data[BUFFER_SIZE];

//HDLC transmit/receive data
unsigned char send_sequence_number, receive_sequence_number; //3bit used!! Count to 7, 8 => 0!

//HDLC transmit list
unsigned char transmit_data[BUFFER_SIZE];
char head, tail;

void hdlc_init() {
	//Read address in EEPROM.
	EEADR= 0x0; //EEPROM address to read
	EECON1bits.EEPGD= 0; //Select data EEPROM
	EECON1bits.CFGS= 0;  //Acces EEPROM memory
	EECON1bits.RD= 1; //Read byte, auto cleared!
	address= EEDATA;  //Save EEPROM in address
	if (address > (unsigned)32) {address= 0;} //Valid address?
	
	control.byte= 0u;
	current_size= received_size= 0;
	head= tail= 0;
	
	//Set up Tx & Rx pins
	TRISC|= 0xC0;
	
	//Set up Receive/Transmit select pins, as output
	TRISC&= 0b11001111;
	
	//Enable UART interrupts
	//PIE1bits.RC1IE= 1;
	
	//Set up UART
	BAUDCON1bits.BRG16= 0; //Timer in 8 bit mode
	SPBRG1= 0x19;// =25 => 9600 boud.
	TXSTA1bits.TX9= 0; //8bit transmission
	TXSTA1bits.SYNC= 0; //Asynchronous mode
	TXSTA1bits.BRGH= 0; //Timer in 8 bit mode
	TXSTA1bits.TXEN= 1; //Transmit enable
	RCSTA1bits.RX9= 0; //8bit receiving
	RCSTA1bits.CREN= 0; //Continuous receive enable bit
	RCSTA1bits.SPEN= 1; //Enable UART
	
	
	/*//Test transmission
	PORTC|= 0x30; //Transmit enable, receive disable
	while (1 == 1) {
		while (TXSTA1bits.TRMT == 0u);
			TXREG1= 'T';
		while (TXSTA1bits.TRMT == 0u);
			TXREG1= 'x';	
		while (TXSTA1bits.TRMT == 0u);
			TXREG1= '\r';
		while (TXSTA1bits.TRMT == 0u);
			TXREG1= '\n';
	}	
	PORTC&= 0xCF; //Disable receive/transmit
	*/
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

void hdlc_send() {
	if (TXSTA1bits.TRMT == 1u) { //If transmit buffer empty
		TXREG1= transmit_data[head];
		head++;
		if (head >= BUFFER_SIZE-1) {
			head= 0;	
		}
	}
}	

void hdlc_send(unsigned char data) {

	if (tail >= BUFFER_SIZE-1) { //Set data in transmission buffer
		transmit_data[0]= data;	
		tail= 0;
	} else {
		transmit_data[tail+1]= data;
		tail++;
	}
		
	if (data == DLE) {
		if (tail >= BUFFER_SIZE-1) { //Set DLE in transmission buffer
			transmit_data[0]= data;	
			tail= 0;
		} else {
			transmit_data[tail+1]= data;
			tail++;
		}
	}	
}	

void hdlc_removeDLEs() {
	
}	

void hdlc_checkData() {
	if (received_data[0] == address) { //For this slave? Otherwise do nothing
		if (control.NRM == 0u) { //Waiting on connection setup
			if (received_data[1]&0xC0 == 0xC0 && received_data[1]&0x37 == 0x01) { //Unnumbered Frame, set normal response mode
				receive_sequence_number= send_sequence_number= 0; //init sequence numbers to zero
				control.NRM= 1u; //Now we are in Normal Response Mode
				if (received_data[1]&0x08 == 0x08) { //Check poll flag
					// Send Unnumbered acknowledge
				}
			} else if (received_data[1]&0xC0 == 0xC0 && received_data[1]&0x37 == 0x02) { //Unnumbered Frame, disconnect
				control.NRM= 0u;
				if (received_data[1]&0x08 == 0x08) { //Check poll flag
					//Send Unnumbered acknowledge
					hdlc_send(address);
					hdlc_send(0b11001110);
					hdlc_send(0); //FCS!!
					hdlc_send(0);
				}
			} else if (received_data[1]&0x08 == 0x08) {
				//Send DM: disconnect mode
			}
		} else { //A connection is alive
			//Check frame type's
			if (received_data[1]&0x80 == 0x00u) { //Information Frame
				if (receive_sequence_number == received_data[1]&0x03 ) { //Testing sequence numbers from master.
					receive_sequence_number++; //increment sequence number.
					receive_sequence_number%= 3;
				}
				

				//Normaly not used here, so send ack
				if (received_data[1]&0x08 == 0x08) {
					// Send ack
				}	
			} else if (received_data[1]&0xC0 == 0x80) { //Supervisory Frame
				if (receive_sequence_number == received_data[1]&0x03 ) { //Testing sequence numbers from master.
					receive_sequence_number++; //increment sequence number.
					receive_sequence_number%= 3;
				}
			
				//Supervisory Code
				if (received_data[1]&0x60 == 0x00) { //Receive Ready, used to poll for data
					//Send data or send RR when no data
				} else if (received_data[1]&0x60 == 0x10) { //Reject
					//Data not correct received by master, resend! 
				} else if (received_data[1]&0x60 == 0x20) { //Receive Not Ready
				
				} else if (received_data[1]&0x60 == 0x30) { //Selective reject
				
				}	
			
			} else if (received_data[1]&0xC0 == 0xC0) { //Unnumbered Frame
				if (received_data[1]&0x37 == 0x01) { //Set normal response mode
					receive_sequence_number= send_sequence_number= 0;
					control.NRM= 1u;
					if (received_data[1]&0x08 == 0x08) { //Check poll flag
						// Send Unnumbered acknowledge
					}
				} else if (received_data[1]&0x37 == 0x02) { //Disconnect
					control.NRM= 0u;
					if (received_data[1]&0x08 == 0x08) { //Check poll flag
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
	if (current_size >= (unsigned)(BUFFER_SIZE-1)) {
		control.SFD= 0u;
		control.DLE= 0u;
	}
	
	//Waiting on Start of frame: DLE & STX
	if (control.SFD == 0u) {
		if (input == DLE) {control.DLE == 1u;} //DLE found => next STX!
		else if (control.DLE && input == STX) { //Start of frame found!
			control.SFD= 1u;
			control.DLE= 0u;
			current_size= 0u;
		} else {control.DLE= 0u;} //Not valid charter
	} else {
		//Save all charters for CRC and handeling of commands
		receiving_data[current_size++]= input;
		
		//Testing for invalid start of frame
		if (input == DLE) {control.DLE= !control.DLE;}
		else if (control.DLE && input == STX) { //Reset was invalid start of frame!
			current_size= 0u;
			control.DLE= 0u;
		} else if (control.DLE && input == ETX) { //End of frame found :):)
			control.SFD= 0u; //We have to resync
			received_size= current_size-2;
			received_data[0]= receiving_data[0];
			received_data[1]= receiving_data[1];
			received_data[2]= receiving_data[2];
			received_data[3]= receiving_data[3];
			received_data[4]= receiving_data[4];
			received_data[5]= receiving_data[5];
			received_data[6]= receiving_data[6];
			received_data[7]= receiving_data[7];
			if (*(&received_data[received_size-1]) == crc16_calc(received_data, received_size)) {
				hdlc_removeDLEs;
				hdlc_checkData;
			} else {
				//Send S-Frame if address is correct, wrong CRC, and P bit=1 => REJ
			}
		} else {
			control.DLE= 0u;
		}
	}
}

char hdlc_write(void) {
	
}
