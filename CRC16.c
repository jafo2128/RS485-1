#include "crc16.h"

#define polynoom 0x1021

unsigned int crc, i, j, tmp1, tmp2, bits;
char valid;

char crc16_calc (unsigned char data[], unsigned int size) {

	//save current CRC
	//crc= (((int)data[1])<<8) | (int)data[4];
	crc= *(&data[1]);

	//data only
	size-= 2;
	//number of bits
	bits= size*8-16;
	
	
	tmp1= tmp2= 0;
	for ( i=0; i<size; i++) {
		//Exor data
		if ((data[0]&0x80) != (unsigned)0) {
			tmp1= *(&data[0]);
			tmp1^=polynoom;
		}
		
		//shift data
		for (j=size-1; j >= (unsigned)0; j--) {
			
			tmp2 = tmp1;
			
			//Test for cary
			if (data[0]&0x80 != (unsigned)0) {
				tmp1=1;
			} else {
				tmp1=0;
			}		
			
			data[j] == data[j]<<1;
			
			//Insert previous cary
			if (tmp2 != (unsigned)0) {
				data[j]|= 0x01;	
			}	
		}	
		
	}
	
	
	//Test if valid CRC
	if (crc == *(&data[0])) {
		valid= 0xff;
	} else {
		valid= 0x00;
	}
	
	return valid;
}	
