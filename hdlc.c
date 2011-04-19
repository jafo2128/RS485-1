#include "hdlc.h"

const unsigned char DLE= 0x10;
const unsigned char STX= 0x02;
const unsigned char ETX= 0x03;

typedef union {
	unsigned char byte;
	struct {
		unsigned SYNC :1; // Found DLE/STX from start of fram
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
unsigned char receiving_data[16];
unsigned char received_data[16];

//HDLC transmit/receive data
unsigned char send_sequence_number, receive_sequence_number; //3bit used!! Count to 7, 8 => 0!

//HDLC transmit list
unsigned char transmit_buffer[0x20u];
unsigned char head, tail;

unsigned char tmp;

//function prototype's
void hdlc_checkData(void);
void hdlc_removeDLEs(void);

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
	
	//Set up Tx & Rx pins of UART
	TRISC|= 0xC0;
	ANSELC&= 0x3F;
	
	//Set up Receive-/Transmit enable pins, as output
	TRISC&= 0b11001111;
	PORTC&= 0xCF; //Transmit enable, receive disable
	
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
	RCSTA1bits.CREN= 1; //Continuous receive enable bit
	RCSTA1bits.SPEN= 1; //Enable UART
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

void hdlc_transmit() {
	if ( (TXSTA1bits.TRMT == 1u) && (head != tail) ) { //If transmit buffer empty and something to send
		PORTC|= 0x30; //Transmit enable, receive disable
		TXREG1= transmit_buffer[tail]; //Send tail
		tail++;
	} else if (TXSTA1bits.TRMT == 1u) { //Disable transmitter after last byte.
		PORTC&= 0xCF; //Disable transmit, enable receive
	}
}

void hdlc_sendbuffer(unsigned char data, char headers) {
	//Set data in transmission buffer
	transmit_buffer[head]= data;
	head++;
	
	if ( (data == DLE) && (headers == 0)) {  //Do not escape DLE when it is in the header!
		//Set DLE in transmission buffer
		transmit_buffer[head]= DLE;
		head++;
	}
}

void hdlc_checkData() {  //What do I have to do?
	if ( (received_data[1]&0xC0) == 0xC0u) { //Unnumbered Frame
		if ( (received_data[1]&0x37) == 0x01u) { //U-F, set normal response mode
			receive_sequence_number= send_sequence_number= 0; //init sequence numbers to zero
			control.NRM= 1u; //Now we are in Normal Response Mode
			if ( (received_data[1]&0x08) == 0x08u) { //Check poll flag
				// Send Unnumbered acknowledge
				head= tail= 0; //reset buffer
				hdlc_sendbuffer(DLE, 1);
				hdlc_sendbuffer(STX, 1);
				hdlc_sendbuffer(address, 0);
				hdlc_sendbuffer(0xCE, 0); //UA
				hdlc_sendbuffer(0x00, 0); //FCS!!
				hdlc_sendbuffer(0x00, 0);
				hdlc_sendbuffer(DLE, 1);
				hdlc_sendbuffer(ETX, 1);
			}
		} else if ( (received_data[1]&0x37) == 0x02u) { //U-F, disconnect
			control.NRM= 0u;
			if ( (received_data[1]&0x08) == 0x08u) { //Check poll flag
				//Send Unnumbered acknowledge
				head= tail= 0; //reset buffer
				hdlc_sendbuffer(DLE, 1);
				hdlc_sendbuffer(STX, 1);
				hdlc_sendbuffer(address, 0);
				hdlc_sendbuffer(0xCE, 0); //UA
				hdlc_sendbuffer(0, 0); //FCS!!
				hdlc_sendbuffer(0, 0);
				hdlc_sendbuffer(DLE, 1);
				hdlc_sendbuffer(ETX, 1);
			}
		} else if ((received_data[1]&0x37) == 0x06u) { //U-F, ACK
			//Do nothing
		} else if ( (received_data[1]&0x08) == 0x08u) { //Unknown, poll-flag, ask to connect
			//Send FRMR
			head= tail= 0;
			hdlc_sendbuffer(DLE, 1);
			hdlc_sendbuffer(STX, 1);
			hdlc_sendbuffer(address, 0);
			hdlc_sendbuffer(0xE9, 0); //FRMR
			hdlc_sendbuffer(0, 0); //FCS!!
			hdlc_sendbuffer(0, 0);
			hdlc_sendbuffer(DLE, 1);
			hdlc_sendbuffer(ETX, 1);
		}
	} else if (control.NRM == 1u) { //A connection is alive
		//Check frame type's
		if ( (received_data[1]&0x80) == 0x00u) { //Information Frame
			if (receive_sequence_number == (received_data[1]&0x07) ) { //Testing sequence numbers from master.
				receive_sequence_number++; //increment sequence number.
				receive_sequence_number%= 0x07;
			}
			//Normaly not used here, so send ack
			if ( (received_data[1]&0x08) == 0x08u) {
				// Send ack
			}
		} else if ( (received_data[1]&0xC0) == 0x80u) { //Supervisory Frame
			if (send_sequence_number == (received_data[1]&0x07) ) { //Testing sequence numbers from master.
				head= tail= 0; //reset buffer
				if ( (received_data[1]&0x60) == 0x00u) { //Receive Ready, used to poll for data
					//if (input_capt1() == 0u) { //No data => RR
					if ( 1 == 0 ) { //Testing I-Frame
						hdlc_sendbuffer(DLE, 1);
						hdlc_sendbuffer(STX, 1);
						hdlc_sendbuffer(address, 0);
						tmp= receive_sequence_number;
						hdlc_sendbuffer((0x88|tmp), 0); //RR: no data to transmit
						hdlc_sendbuffer(0, 0); //FCS!!
						hdlc_sendbuffer(0, 0);
						hdlc_sendbuffer(DLE, 1);
						hdlc_sendbuffer(ETX, 1);
					} else { //Send Data => I-Frame
						hdlc_sendbuffer(DLE, 1);
						hdlc_sendbuffer(STX, 1);
						hdlc_sendbuffer(address, 0);
						tmp= send_sequence_number<<4;
						send_sequence_number++;
						send_sequence_number%= 0x07;
						tmp|= receive_sequence_number;
						tmp|= 0x08; //Poll-flag
						hdlc_sendbuffer((0x7F&tmp), 0); //I-Frame
						hdlc_sendbuffer(0, 0); //FCS!!
						hdlc_sendbuffer(0, 0);
						hdlc_sendbuffer(DLE, 1);
						hdlc_sendbuffer(ETX, 1);
					}
				}
			} else  {
				//Resend buffer!!
				if ( (received_data[1]&0x08) == 0x08u) {
					tail= 0;
				}
			}
		}
	} else { // Not Unnumbered Frame, not in Normal Response Mode
		if ((received_data[1]&0x08) == 0x08u) { //Check poll flag, if true send data
			//Send disconnect mode
			head= tail= 0;
			hdlc_sendbuffer(DLE, 1);
			hdlc_sendbuffer(STX, 1);
			hdlc_sendbuffer(address, 0);
			hdlc_sendbuffer(0xF8, 0); //DM
			hdlc_sendbuffer(0, 0); //FCS!!
			hdlc_sendbuffer(0, 0);
			hdlc_sendbuffer(DLE, 1);
			hdlc_sendbuffer(ETX, 1);
		}	
	}
}

void hdlc_receive(unsigned char received_byte) {
	//Drop data => to many packets!
	if (current_size >= 0x10u) { //Max size = 16 + 2. (2 from DLE & STX)
		control.SYNC= 0u;
		control.DLE= 0u;
	}
	
	//Waiting on Start of frame: DLE & STX
	if (control.SYNC == 0u) {
		if (received_byte == DLE) {
			control.DLE= 1u; //DLE found => next STX!
		} else if (control.DLE == 1u && received_byte == STX) { //Start of frame found!
			control.SYNC= 1u;
			control.DLE= 0u;
			current_size= 0u;
		} else {control.DLE= 0u;} //Not valid charter
	} else {
		//Save all charters for CRC and handeling of commands
		receiving_data[current_size]= received_byte;
		current_size++;
		
		//Testing for invalid start of frame
		if (received_byte == DLE) {control.DLE= !control.DLE;}
		else if (control.DLE == 1u && received_byte == STX) { //Reset was invalid start of frame!
			current_size= 0u;
			control.DLE= 0u;
		} else if (control.DLE == 1u && received_byte == ETX) { //End of frame found :):)
			control.SYNC= 0u; //We have to resync
			received_size= current_size - 2 ; //Remove DLE & ETX, unneeded!
			if (receiving_data[0] == address) {
				hdlc_removeDLEs();
				/*if (*(&received_data[received_size-1]) == crc16_calc(received_data, received_size)) {
					hdlc_checkData;
				} else {
					//Send S-Frame if address is correct, wrong CRC, and P bit=1 => REJ
				}*/
				hdlc_checkData();
			}
		} else {
			control.DLE= 0u;
		}
	}
}

void hdlc_removeDLEs() {
	unsigned int i, offset;
	offset= 0;
	for (i=0; i < received_size; i++) {
		if (receiving_data[i] == DLE && receiving_data[i+1] == DLE) { // If DLE, skip one place
			i++; offset++;
		}
		received_data[i-offset]= receiving_data[i];
	}
	received_size-= offset; //Set receiving_size to correct site.
}
