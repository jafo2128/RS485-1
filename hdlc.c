#include "hdlc.h"

//Define constants for  receiving & transmission
const unsigned char DLE= 0x10;
const unsigned char STX= 0x02;
const unsigned char ETX= 0x03;

//used as boolean
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
bits control;

//For int to char
typedef union combo {
	unsigned int Int;		
	unsigned char Char[2];		
} Tcombo;

//Receiving charters
unsigned char current_size, received_size, cnt; //Counters
unsigned char address;	//Address of slave
unsigned char receiving_data[0x10]; //Buffer for receiving data
unsigned char received_data[0x10]; //Same as receiving buffer, maybe remove??

//HDLC transmit/receive counter
unsigned char send_sequence_number, receive_sequence_number; //3bit used!! Count to 7, 8 => 0!

//Transmit buffer, and counters for position in buffer
unsigned char transmit_buffer[0x20u];
unsigned char head, tail;

//Temp variable
unsigned char tmp;
//Temp variable for capt1
Tcombo tmp_capt1;
//Temp variable for input
unsigned char tmp_input;

Tcombo crc;

//function prototype's
void hdlc_checkFrame(void);
void hdlc_parseFrame(void);
void hdlc_sendData(void);

void hdlc_init() {
	//Read address in EEPROM.
	EEADR= 0x0; //EEPROM address to read
	EECON1bits.EEPGD= 0; //Select data EEPROM
	EECON1bits.CFGS= 0;  //Acces EEPROM memory
	EECON1bits.RD= 1; //Read byte, auto cleared!
	address= EEDATA;  //Save EEPROM in address
	if (address > (unsigned)32) {address= 0;} //Valid address?
	
	
	//Init variables
	control.byte= 0u;
	current_size= received_size= 0;
	head= tail= 0;
	
	//Set up Tx & Rx pins of UART
	TRISC|= 0xC0;
	ANSELC&= 0x3F;
	//Set up Receive-/Transmit enable pins, as output
	TRISC&= 0b11001111;
	LATC&= 0xCF; //Transmit enable, receive disable
	
	//Set up RS485 output led
	TRISD&= 0b11110111;
	
	LATD&=0xF7; //Disable led
	
	//Enable UART interrupts
	//PIE1bits.RC1IE= 1;
	
	//Set up UART
	BAUDCON1bits.BRG16= 0; //Timer in 8 bit mode
	//SPBRG1= 0x19;// =25 => 9600 baud.
	SPBRG1= 0x17; // =23 => 10417 baud
	TXSTA1bits.TX9= 0; //8bit transmission
	TXSTA1bits.SYNC= 0; //Asynchronous mode
	TXSTA1bits.BRGH= 0; //Timer in 8 bit mode
	TXSTA1bits.TXEN= 1; //Transmit enable
	RCSTA1bits.RX9= 0; //8bit receiving
	RCSTA1bits.CREN= 1; //Continuous receive enable bit
	RCSTA1bits.SPEN= 1; //Enable UART
}

//Get current address of slave
unsigned char hdlc_getAddress() {
	return address;
}

//Change address of slave, and write to EEPROM
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


//If transmission buffer is not full and we have something to transmit, transmit it.
void hdlc_transmit() {
	//For polling use: (TXSTA1bits.TRMT == 1u)
	if ( (head != tail) ) { //If transmit buffer empty and something to send
		PORTC|= 0x30; //Transmit enable, receive disable
		TXREG1= transmit_buffer[tail]; //Send tail
		tail++;
	} else {
		PIE1bits.TX1IE= 0; //Disable transmit interrupts!
	}
}

//Add data to transmission buffer, and if data equals DLE when it isn't a header, escape it.
void hdlc_sendbuffer(unsigned char data, char header) {
	//Set data in transmission buffer
	transmit_buffer[head]= data;
	head++;
	
	if ( (data == DLE) && (header == 0)) {  //Do not escape DLE when it is in the header!
		//Set DLE in transmission buffer
		transmit_buffer[head]= DLE;
		head++;
	}
}

//Parse incomming data and act accordingly. ( HDLC )
void hdlc_parseFrame() {  //What do I have to do?
	if ((received_data[1]&0xC0) == 0xC0u) { //Unnumbered Frame
		if ( (received_data[1]&0x37) == 0x01u) { //U-F: SNRM => Set Normal Response Mode
			receive_sequence_number= send_sequence_number= 0; //init sequence numbers to zero
			control.NRM= 1u; //Now we are in Normal Response Mode
			if ( (received_data[1]&0x08) == 0x08u) { //Check poll flag
				// Send Unnumbered acknowledge
				//Calculate CRC
				crc.Int= 0xffff;
				crc.Int= crc_1021(crc.Int, address);
				crc.Int= crc_1021(crc.Int, 0xCE);
				
				head= tail= 0; //Reset send buffer
				hdlc_sendbuffer(DLE, 1);
				hdlc_sendbuffer(STX, 1);
				hdlc_sendbuffer(address, 0);
				PIE1bits.TX1IE= 1; //Enable interrupts!
				hdlc_sendbuffer(0xCE, 0); //UA
				hdlc_sendbuffer(crc.Char[0], 0); //FCS!!
				hdlc_sendbuffer(crc.Char[1], 0);
				hdlc_sendbuffer(DLE, 1);
				hdlc_sendbuffer(ETX, 1);
			}
		} else if ( (received_data[1]&0x37) == 0x02u) { //U-F: DISC => Disconnect
			control.NRM= 0u;
			if ( (received_data[1]&0x08) == 0x08u) { //Check poll flag
				//Send Unnumbered acknowledge
				//Calculate CRC
				crc.Int= 0xffff;
				crc.Int= crc_1021(crc.Int, address);
				crc.Int= crc_1021(crc.Int, 0xCE);
				
				head= tail= 0; //reset send buffer
				hdlc_sendbuffer(DLE, 1);
				hdlc_sendbuffer(STX, 1);
				hdlc_sendbuffer(address, 0);
				PIE1bits.TX1IE= 1; //Enable interrupts!
				hdlc_sendbuffer(0xCE, 0); //UA
				hdlc_sendbuffer(crc.Char[0], 0); //FCS!!
				hdlc_sendbuffer(crc.Char[1], 0);
				hdlc_sendbuffer(DLE, 1);
				hdlc_sendbuffer(ETX, 1);
			}
		} else if ((received_data[1]&0x37) == 0x06u) { //U-F, ACK => Unnumbered Acknowledge
			//Do nothing
		} else if (control.NRM == 1u &&(received_data[1]&0x37) == 0x31u) {//U-F, RSET => Reset 
			head= tail= 0; //Reset buffer
		  	receive_sequence_number= send_sequence_number= 0;
			if ( (received_data[1]&0x08) == 0x08u) { //Check poll flag
				//Send Unnumbered acknowledge
				//Calculate CRC
				crc.Int= 0xffff;
  				crc.Int= crc_1021(crc.Int, address);
  				crc.Int= crc_1021(crc.Int, 0xCE);
  			
				hdlc_sendbuffer(DLE, 1);
				hdlc_sendbuffer(STX, 1);
				hdlc_sendbuffer(address, 0);
				PIE1bits.TX1IE= 1; //Enable interrupts!
				hdlc_sendbuffer(0xCE, 0); //UA
				hdlc_sendbuffer(crc.Char[0], 0); //FCS!!
			  	hdlc_sendbuffer(crc.Char[1], 0);
		 		hdlc_sendbuffer(DLE, 1);
				hdlc_sendbuffer(ETX, 1);
			}	
		} else if (control.NRM == 1u && (received_data[1]&0x02) == 0x01u) { //U-F, DISC => Disconnect
			control.NRM = 0u;
 			head= tail= 0; //Reset buffer
			receive_sequence_number= send_sequence_number= 0;
			if ( (received_data[1]&0x08) == 0x08u) { //Check poll flag
				//Send Unnumbered acknowledge
			  	//Calculate CRC
			  	crc.Int= 0xffff;
  				crc.Int= crc_1021(crc.Int, address);
  				crc.Int= crc_1021(crc.Int, 0xCE);
  				
				hdlc_sendbuffer(DLE, 1);
				hdlc_sendbuffer(STX, 1);
				hdlc_sendbuffer(address, 0);
				PIE1bits.TX1IE= 1; //Enable interrupts!
				hdlc_sendbuffer(0xCE, 0); //UA
				hdlc_sendbuffer(crc.Char[0], 0); //FCS!!
			  	hdlc_sendbuffer(crc.Char[1], 0);
		 		hdlc_sendbuffer(DLE, 1);
				hdlc_sendbuffer(ETX, 1);
  			}		  
		} else if ( control.NRM == 1u && (received_data[1]&0x08) == 0x08u) { //Unknown data AND poll-flag. Send RIM
			//Send RIM: Request initialisation mode
			//Calculate CRC
			crc.Int= 0xffff;
			crc.Int= crc_1021(crc.Int, address);
			crc.Int= crc_1021(crc.Int, 0xE8);
				
			head= tail= 0; //Reset send buffer
			hdlc_sendbuffer(DLE, 1);
			hdlc_sendbuffer(STX, 1);
			hdlc_sendbuffer(address, 0);
			PIE1bits.TX1IE= 1; //Enable interrupts!
			hdlc_sendbuffer(0xE8, 0); //RIM
			hdlc_sendbuffer(crc.Char[0], 0); //FCS!!
			hdlc_sendbuffer(crc.Char[1], 0);
			hdlc_sendbuffer(DLE, 1);
			hdlc_sendbuffer(ETX, 1);
		}
	} else if (control.NRM == 1u) { //A connection is alive
		//Check frame type's
		if ( (received_data[1]&0x80) == 0x00u) { //Information Frame, can have data!
			if (receive_sequence_number == ((received_data[1] & 0x70)>>4) ) {//Correct send number received?
				//We received everything from master, so tell him that. (Master: drop send buffer)
				receive_sequence_number++;
				receive_sequence_number&= 0x07;
				
				//Do we have data in the frame?
				if (received_size > 1u) {
					if ( (received_data[2] & 0x02) > 0) { //Update output 2
						io_enableOutput(2);
					} else {
						io_disableOutput(2);
					}
					
					if ( (received_data[2] & 0x01) > 0u) { //Update output 1
						io_enableOutput(1);
					} else {
						io_disableOutput(1);
					}	
				}	
			}
			
			if (send_sequence_number == (received_data[1]&0x07) ) { //Correct receive number?
				//Master received previous frame correct => drop send buffer
				//Send our data if we have anything.
				head= tail= 0u; //Reset send buffer
				if ( (received_data[1]&0x08) == 0x08u) { //I-F, RR => Receive Ready, used to poll for data
					hdlc_sendData(); //Transmit pulse count or buttons pressed otherwise send RR
				}
			} else { //Resend buffer
				if ( (received_data[1]&0x08) == 0x08u) {
					tail= 0;
				}
			}
		} else if ( (received_data[1]&0xC0) == 0x80u) { //Supervisory Frame
			if (send_sequence_number == (received_data[1]&0x07) ) { //Testing sequence numbers from master.
				head= tail= 0; //Reset send buffer
				if ( (received_data[1]&0x08) == 0x08u) { //Receive Ready, used to poll for data
					hdlc_sendData(); //Transmit pulse count or buttons pressed otherwise send RR
				}
			} else  { //Resend buffer!!
				if ( (received_data[1]&0x08) == 0x08u) {
					tail= 0;
				}
			}
		}
	} else { // Not Unnumbered Frame, not in Normal Response Mode
		if ((received_data[1]&0x08) == 0x08u) { //Check poll flag, if true send data
			//Request initialisation mode
			//Calculate CRC
			crc.Int= 0xffff;
			crc.Int= crc_1021(crc.Int, address);
			crc.Int= crc_1021(crc.Int, 0xE8);
			
			head= tail= 0; //Reset buffer
			hdlc_sendbuffer(DLE, 1);
			hdlc_sendbuffer(STX, 1);
			hdlc_sendbuffer(address, 0);
			PIE1bits.TX1IE= 1; //Enable interrupts!
			hdlc_sendbuffer(0xE8, 0); //RIM
			hdlc_sendbuffer(crc.Char[0], 0); //FCS!!
			hdlc_sendbuffer(crc.Char[1], 0);
			hdlc_sendbuffer(DLE, 1);
			hdlc_sendbuffer(ETX, 1);
		}
	}
}

void hdlc_sendData() {
  if ( (tmp_capt1.Int= io_getCapt1()) > 0u) { //Send puls data => I-Frame
		//Control field
		tmp= send_sequence_number<<4;
		send_sequence_number++;
	  	send_sequence_number&= 0x07;
		tmp|= receive_sequence_number;
		tmp|= 0x08; //Poll-flag
	  	tmp= 0x7F&tmp;
		//Calculate CRC
		crc.Int= 0xffff;
		crc.Int= crc_1021(crc.Int, address);
		crc.Int= crc_1021(crc.Int, tmp);
		crc.Int= crc_1021(crc.Int, tmp_capt1.Char[0]);
		crc.Int= crc_1021(crc.Int, tmp_capt1.Char[1]);		
		
		hdlc_sendbuffer(DLE, 1);
		hdlc_sendbuffer(STX, 1);
		hdlc_sendbuffer(address, 0);
		PIE1bits.TX1IE= 1; //Enable interrupts!
		hdlc_sendbuffer(tmp, 0); //I-Frame
		hdlc_sendbuffer(tmp_capt1.Char[0], 0); //Data to transmit
		hdlc_sendbuffer(tmp_capt1.Char[1], 0);		
		hdlc_sendbuffer(crc.Char[0], 0); //FCS!!
		hdlc_sendbuffer(crc.Char[1], 0);
		hdlc_sendbuffer(DLE, 1);
		hdlc_sendbuffer(ETX, 1);
	} else if ((tmp_input= io_getInputs()) > 0u) { //Send input data => I-frame
	  	//Control field
	  	tmp= send_sequence_number<<4;
		send_sequence_number++;
		send_sequence_number&= 0x07;
		tmp|= receive_sequence_number;
		tmp|= 0x08; //Poll-flag
		tmp= 0x7F&tmp;
	  	//Calculate CRC
	  	crc.Int= 0xffff;
		crc.Int= crc_1021(crc.Int, address);
		crc.Int= crc_1021(crc.Int, tmp);
		crc.Int= crc_1021(crc.Int, tmp_input);
				
		hdlc_sendbuffer(DLE, 1);
		hdlc_sendbuffer(STX, 1);
		hdlc_sendbuffer(address, 0);
		PIE1bits.TX1IE= 1; //Enable interrupts!
		hdlc_sendbuffer(tmp, 0); //I-Frame
		hdlc_sendbuffer(tmp_input, 0); //Data to transmit
		hdlc_sendbuffer(crc.Char[0], 0); //FCS!!
		hdlc_sendbuffer(crc.Char[1], 0);
		hdlc_sendbuffer(DLE, 1);
		hdlc_sendbuffer(ETX, 1);
	} else { //No data => RR
	  	//Control field
		tmp= 0x88|receive_sequence_number;
		//Calculate CRC
		crc.Int= 0xffff;
		crc.Int= crc_1021(crc.Int, address);
		crc.Int= crc_1021(crc.Int, tmp);
		
		hdlc_sendbuffer(DLE, 1);
		hdlc_sendbuffer(STX, 1);
		hdlc_sendbuffer(address, 0);
		PIE1bits.TX1IE= 1; //Enable interrupts!
		hdlc_sendbuffer(tmp, 0); //RR: no data to transmit
		hdlc_sendbuffer(crc.Char[0], 0);
		hdlc_sendbuffer(crc.Char[1], 0);
		hdlc_sendbuffer(DLE, 1);
		hdlc_sendbuffer(ETX, 1);
	}
}

//Parse incomming byte, search for start of frame and end of frame.
//If found, parse frame
void hdlc_receive(unsigned char received_byte) {
	//Drop data => to many packets! (Buffer overflow!)
	if (current_size >= 0x10u) { //Max size = 16 + 2. (2 from DLE & STX)
		control.SYNC= 0u;
		control.DLE= 0u;
		current_size= 0u;
	}
	
	//Waiting on Start of frame: DLE & STX
	if (control.SYNC == 0u) {
		if (received_byte == DLE) {
			control.DLE= 1u; //DLE found => next STX!
		} else if (control.DLE == 1u && received_byte == STX) { //Start of frame found!
			control.SYNC= 1u;
			control.DLE= 0u;
			current_size= 0u;
		} else {control.DLE= 0u;} //Not a valid charter
	} else {
		//Save all charters for CRC and handeling of commands
		receiving_data[current_size]= received_byte;
		current_size++;
		
		//End of frame found :):)
		if (control.DLE == 1u && received_byte == ETX) {
			received_size= current_size - 3; //Remove DLE & ETX, unneeded!
 			if (receiving_data[0] == address) { //For this slave?
				hdlc_checkFrame(); //Remove dupplicate DLE's and calculate CRC
				if ( (received_data[received_size-1] == crc.Char[0]) && (received_data[received_size] == crc.Char[1]) ) {
						hdlc_parseFrame(); //What do we have to do?
					io_control_rs485(1); //Let the RS485-LED blink to show we are receiving data for this slave.
				}
			} else {
 				io_control_rs485(0); //Enable the RS485-LED to show we receive some data, but not for this slave.
			}
			control.SYNC= 0u; //We have to resync
		} else if (control.DLE == 1u && received_byte == STX) { //Reset was invalid start of frame!
			current_size= 0u;
			control.DLE= 0u;
		} if (received_byte == DLE) { //Toggle DLE status
			control.DLE= !control.DLE;
		} else {  //Incoming data != DLE => reset DLE status
			control.DLE= 0u;
		}
	}
}


//Remove dupplicate DLE's from frame, and calculate CRC.
void hdlc_checkFrame() {
	unsigned int i, offset;
	offset= 0;
	crc.Int= 0xffff;
		
	//Remove double DLE's
	for (i=0; i <= received_size; i++) {
		if (receiving_data[i] == DLE && receiving_data[i+1] == DLE) { // If DLE, skip one place
			i++; offset++;
		}
		received_data[i-offset]= receiving_data[i];
	}
	
	//Set receiving_size to correct size.
	received_size-= offset;
	
	//Calculate CRC
	for (i=0; i <= received_size-2; i++) {
		crc.Int= crc_1021(crc.Int, received_data[i]);
	}	
}
