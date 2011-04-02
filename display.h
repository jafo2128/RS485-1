#ifndef DISP
#define DISP

#include <p18f25k22.h>

//Public function declaration

//Init
void display_init(void);

//Reset idle timer
void display_idleReset(void);

//Lock & Unlock display
void display_lock(void);
void display_unlock(void);

//Increment/decrement value
void display_inc(void);
void display_dec(void);

#endif
