#ifndef HDLC
#define HDLC

#include <string.h>
#include "CRC16.h"
#include <p18f25k22.h>

//Public function declaration

//Init
void hdlc_init(void);

//Set and read the address of this device
char hdlc_getAddress();
void hdlc_setAddress(char new_address);

//Read and Write data from UART
void hdlc_read(unsigned char input);

#endif
