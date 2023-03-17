#include <hcs12.h>
#include <dbug12.h>

#include "lcd.h"
#include "keypad.h"
#include "util.h"

volatile unsigned int keypad_debounce_timer = 0;

void INTERRUPT rti_isr(void);
void initPWM(void);
void msDelay(int);

int main(void) {
	set_clock_24mhz(); 						// usually done by D-Bug12 anyway
	UserRTI = (unsigned int) & rti_isr; 	// register the ISR unit
	RTICTL = 0x11;							// RTI interval set to 256ms
	CRGINT |= 0x80;							// enable RTI (RTIE == 0x80?)
	__asm("cli"); 							// enable interrupts (maskable and I bit in CCR)
	initPWM();

	DDRH = 0x00; //for push buttons
	KeypadInitPort();
	DDRB = 0xff;
	PORTB = 0x7f;

	int a = 0;
	while(1) {
		char x = ~PTH;
		if (x) printf("DPKey: %x\r\n", x);//print on serial console

		unsigned char ipt = KeypadReadPort();
		if(ipt != KEYPAD_KEY_NONE) printf("Keypad: %c\r\n", ipt); //print on serial console
		if (ipt == '2') a = ipt - '0';
		if (ipt == '8') a = ipt - '0';

		if (a == 2) {
			PWMDTY0 = 20;		// set duty cycle to 20%
		} else if (a == 8) {
			PWMDTY0 = 80;		// set duty cycle to 80%
		}

		msDelay(100);
	}
}

void msDelay(int delay) {
	for (int i=0; i<delay; i++) {
		for (int j=0; j<4000; j++);
	}
}

void initPWM(void) {
	PWMCLK = 0;			// select clock A as the clock source
	PWMPOL = 1;			// make waveform to start with high level
	PWMCTL = 0x0C;		// select 8-bit mode
	PWMPRCLK = 2;		// set clock prescaler to 4
	PWMCAE = 0;			// select left aligned mode
	PWMPER0 = 100;		// set period of PWM0 to 0.1ms
	PWMDTY0 = 80;		// set duty cycle to 100%
	PWME |= 0xff;		// enable PWM0 channel
}

void INTERRUPT rti_isr(void) {
  CRGFLG = 0x80; 					 //clear the RTI - don't block the other interrupts

  keypad_debounce_timer++;
  if (keypad_debounce_timer > 4000){ // original timer, no modification
	  keypad_debounce_timer = 0;
	  EnableKeyboardAgain();
  }
}
