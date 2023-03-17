#include <hcs12.h>
#include <dbug12.h>

#include "lcd.h"
#include "keypad.h"
#include "util.h"

#define Hicnt 900
#define Locnt 2100

char hiorlo;

void INTERRUPT oc5ISR(void);

void main(void) {
	UserTimerCh5 = (unsigned int) & UserTimerCh5;
	TSCR1 = 0x90; // enable TCNT and fast timer flag clear
	TSCR2 = 0x03; // disable TCNT interrupt, set prescaler to 8
	TIOS |= 0x20; // enable OC5 function
	TCTL1 = 0x0C; // set OC5 action to be pull high
	TFLG1 = 0xFF; // clear all CxF flags
	TC5 = TCNT + 10; // some delay value
	while (!(TFLG1 & C5F)); // wait until C5F is set
	TCTL1 = 0x04; // set OC5 pin action to toggle
	TC5 += Hicnt; //start a new OC5 operation
	hiorlo = 0; // add Locnt for the next OC5 operation
	TIE = 0x20; // enable OC5 inerrupt
	__asm("cli"); // enable OC5 interrupt
	while (1);
}

void INTERRUPT oc5ISR(void) {
	if (hiorlo) {
		TC5 += Hicnt;
		hiorlo = 0;
	} else {
		TC5 += Locnt;
		hiorlo = 1;
	}
}