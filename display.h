#ifndef DISP
#define DISP

//#include <p18f45k22.h>
#include "hdlc.h"
#include "io.h"

//Public function declaration

//Init
void display_init(void);

//Display current value
void display_on(void);

//Display a value
void display_show(unsigned char disp_value);

//Lock & Unlock change of value and display on/off
void display_lock(void);
void display_unlock(void);

//Increment/decrement value
void display_inc(void);
void display_dec(void);

#endif
