#ifndef HDLC
#define HDLC

#include <string.h>
#include "CRC16.h"

//Public function declaration

//Init
void hdlc_init(void);

//Read/Write data from UART
void hdlc_read(unsigned char input);
char hdlc_write(void);

#endif
