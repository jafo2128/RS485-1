#ifndef IO
#define IO

//#include <p18f45k22.h>
#include <htc.h>

#include "display.h"

//Public function declaration

void io_init(void);
void io_loop(void);
void io_cnt_int(void);

unsigned char io_capt1(void); //Get the number of times the pulse has changed

void io_enableOutput(char output);
void io_disableOutput(char output);

unsigned int io_getInputs(void); //Get the current status of the inputs

void io_control_rs485(char forme); //Control the status of the rs485 LED
void io_control_rs485_reset(void); //Reset after address change
void io_control_rs485_int(void); //Controls the RS485 LED via interrupts

#endif
