#ifndef HDLC
#define HDLC

#include "CRC16.h"
#include "input.h"
#include <p18f45k22.h>


//Public function declaration

//Init
void hdlc_init(void);

//Read, and set the address of this device + write to EEPROM
char hdlc_getAddress(void);
void hdlc_setAddress(char new_address);

//Read and Write data from UART
void hdlc_receive(unsigned char received_byte); //Read data from UART
void hdlc_transmit(void); //Write data to UART

#endif
