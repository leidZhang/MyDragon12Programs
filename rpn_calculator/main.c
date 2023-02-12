#include <hcs12.h>
#include <dbug12.h>

#include "lcd.h"
#include "keypad.h"
#include "util.h"

volatile unsigned int counter_for_real_time_interrupt;
volatile unsigned int display_counter = 0;
volatile unsigned int counter_for_real_time_interrupt_limit;
volatile unsigned int keypad_debounce_timer = 0;

void rti_init(unsigned char, unsigned int);
void execute_the_jobs(void);
void INTERRUPT rti_isr(void);

int main(void) {
	set_clock_24mhz(); //usually done by D-Bug12 anyway
	rti_init(0x11, 10);
	__asm("cli"); //enable interrupts (maskable and I bit in CCR)

	DDRH = 0x00; //for push buttons
	KeypadInitPort();

	DispInit (2, 16);
	DispClrScr();
	DispStr(1, 1, "LCD calculator");
	// Receive input from keypad
	int num = 0, i = 0;
	int rpn[100];

	while(1) {
		char x = ~PTH;
		if (x) printf("DPKey: %x\r\n", x);//print on serial console

		unsigned char ipt = KeypadReadPort();
		if(ipt != KEYPAD_KEY_NONE) printf("Keypad: %c\r\n", ipt); //print on serial console

		if (ipt == '#') {
			rpn[i++] = -100; // stop signal
			break;
		}

		if (ipt >= '0' && ipt <= '0') {
			num = num * 10 + (ipt - '0');
		}

		if (ipt == 'A') {
			rpn[i++] = -1;
		} else if (ipt == 'B') {
			rpn[i++] = -2;
		} else if (ipt == 'C') {
			rpn[i++] = -3;
		} else if (ipt == 'D') {
			rpn[i++] = -4;
		} else if (ipt == '*') {
			rpn[i++] = num; num = 0; // put into rpn
		}

	}
	// printf("num1 = %d, num2 = %d, opCode = %d\r\n", num1, num2, opCode);
	int st[100]; // mock stack
	int btn = 0, top = 0, j = 0;

	for (i=0; i<sizeof(rpn); i++) {
		if (rpn[i] >= 0) {
			st[j++] = rpn[i];
			continue;
		}

		int num1 = st[--j];
		int num2 = st[--j];
		if (rpn[i] == -1) { // addition
			st[j] = num1 + num2;
		} else if (rpn[i] == -2) { // multiplication
			st[j] = num1 * num2;
		} else if (rpn[i] == -3) { // subtraction
			st[j] = num1 - num2;
		} else if (rpn[i] == -4) { // division
			st[j] = num1 / num2;
		} else { // find stop signal
			break;
		}
	}

	int res = st[j];
	// my itoa()
	int cnt = (res > 0) ? 0 : 1;
	int temp = (res > 0) ? res : -res;

	while (temp != 0) { // get num of digits
		cnt++;
		temp /= 10;
	}

	char opt[cnt+1]; // create output string
	opt[0] = (res < 0) ? '-' : '0'; // edge cases
	opt[cnt--] = '\0'; // '\0' at the tail

	j = cnt;
	res = (res >= 0) ? res : -res;
	while (res != 0) {
		opt[j--] = res % 10 + '0';
		res /= 10;
	}
	// print result
	DispInit (2, 16);
	DispClrScr();
	DispStr(1, 1, opt);
}

void execute_the_jobs() {
  //put the jobs you want to be done here ...
  keypad_debounce_timer ++;
  if (keypad_debounce_timer > 400){
	  keypad_debounce_timer = 0;
	  EnableKeyboardAgain();
  }
}

void INTERRUPT rti_isr(void) {
  //clear the RTI - don't block the other interrupts
  CRGFLG = 0x80;

  //for instance if limit is "10", every 10 interrupts do something ...
  if (counter_for_real_time_interrupt == counter_for_real_time_interrupt_limit) {
      //reset the counter
      counter_for_real_time_interrupt = 0;

      //do some work
      execute_the_jobs();
  } else {
	  counter_for_real_time_interrupt ++;
  }
}

/**
 * initialize the rti: rti_ctl_value will set the pre-scaler ...
 */
void rti_init(unsigned char rti_ctl_value, unsigned int counter_limit) {
  UserRTI = (unsigned int) & rti_isr; //register the ISR unit

  /**
   * set the maximum limit for the counter:
   * if max set to be 10, every 10 interrupts some work will be done
   */
  counter_for_real_time_interrupt_limit = counter_limit;


  /**
   * RTICTL can be calculated like:
   * i.e: RTICTL == 0x63 == set rate to 16.384 ms:
   * The clock divider is set in register RTICTL and is: (N+1)*2^(M+9),
   * where N is the bit field RTR3 through RTR0  (N is lower bits)
   * 	and M is the bit field RTR6 through RTR4. (M is higher bits)
   * 0110 0011 = 0x63 ==> 1 / (8MHz / 4*2^15)
   * 	which means RTI will happen every 16.384 ms
   * Another example:
   * 0111 1111 = 0x7F ==> 1 / (8MHz / 16*2^16)
   * 	which means RTI will happen every 131.072 ms
   * Another example:
   * 0001 0001 = 0x11 ==> 1 / (8MHz / 2*2^10)   = 256us
   */
  RTICTL = rti_ctl_value;

  // How many times we had RTI interrupts
  counter_for_real_time_interrupt = 0;

  // Enable RTI interrupts
  CRGINT |= 0x80;
  // Clear RTI Flag
  CRGFLG = 0x80;
}

