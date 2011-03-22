#include <p18f26k22.h>


//Default C library's
#include <stdio.h>
#include "buttons.h"

//Project library's


void main (void)
{
	//Init
	buttons_init();
	
	//Main loop
	while (1) {
		buttons_loop();
	}	
}
