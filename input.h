#ifndef INPUT
#define INPUT

#include <p18f45k22.h>

#include "display.h"

//Public function declaration

void input_init(void);
void input_loop(void);
void input_cnt_int(void);
unsigned char input_capt1(void);

#endif
