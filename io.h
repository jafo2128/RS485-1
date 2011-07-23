#ifndef IO
#define IO

#include <p18f45k22.h>

#include "display.h"

//Public function declaration

void io_init(void);
void io_loop(void);
void io_cnt_int(void);

unsigned char io_capt1(void); //Get the number of times the pulse has changed

void io_enableOutput(char output);
void io_disableOutput(char output);

unsigned char io_getInputs(void); //Get the current status of the inputs
#endif
