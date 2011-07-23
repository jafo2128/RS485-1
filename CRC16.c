//http://www.dattalo.com/technical/software/pic/crc_1021.asm

// Digital Nemesis Pty Ltd
// www.digitalnemesis.com
// ash@digitalnemesis.com

// Original Code: Ashley Roll
// Optimisations: Scott Dattalo

#include "CRC16.h"

int x;
int calc_crc;

int crc_1021(int old_crc, char data) {
	
  x = ((old_crc>>8) ^ data) & 0xff;
  x ^= x>>4;

  calc_crc = (old_crc << 8) ^ (x << 12) ^ (x <<5) ^ x;

  calc_crc &= 0xffff;

  return calc_crc;
}
