#include <hcs12.h>
#include <dbug12.h>

#include "lcd.h"
#include "keypad.h"
#include "util.h"

void main(void) {
	unsigned int edge1, period;
	TSCR1 = 0x90; 			// enable time counter, enable fast flag clear
	TIOS &= ~0x01; 			// enable input-capture 0
	TSCR2 = 0x06; 			// disable TCNT overflow interrupt, set prescaler to 64
	TCTL4 = 0x01; 			// capture the rising edge of the PT0 pin
	TFLG1 = C0F; 			// clear the C0F flag
	while (!(TFLG1 & C0F)); // wait for the arrival of the first rising edge
	edge1 = TC0; 			// save the first captured edge and clear C0F flag
	while (!(TFLG1 & C0F)); // wait for the arrival of the second rising edge
	period = TC0 - edge1; 	// calculate the period
	printf("%d\r\n", period);
	while (1);
}
