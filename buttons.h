#ifndef BUTTONS
#define BUTTONS

#include <p18f26k22.h>
#include "display.h"

//Public function declaration

void buttons_init(void);
void buttons_loop(void);
void buttons_cnt_int(void);

#endif