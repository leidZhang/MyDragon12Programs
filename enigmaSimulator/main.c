#include <hcs12.h>
#include <dbug12.h>

#include "lcd.h"
#include "keypad.h"
#include "util.h"

volatile unsigned int keypad_debounce_timer = 0;

unsigned int keyboard[10][6] = {{'0',' ','@'},{'#','*','1','@'},{'A','B','C','2','@'},{'D','E','F','3','@'},
					            {'G','H','I','4','@'},{'J','K','L','5','@'},{'M','N','O','6','@'},
					            {'P','Q','R','S','7','@'},{'T','U','V','8','@'},{'W','X','Y','Z','9','@'}
					           };
unsigned int segment_decoder[]={
                                 0x77,0x7c,0x39,0x5e,0x79,
                                 0x71,0x3f,0x06,0x5b,0x4f,
								 0x66,0x6d,0x7d,0x07,0x7f,
								 0x6f, 0xf7,0xfc,0xb9,0xde,
								 0xf9,0xf1,0xbf,0x86,0xbb,
								 0xcf
                               };
unsigned int digit_decoder[] = {0x07, 0x0B, 0x0D, 0x0E};

char rotor_wiring[4][26] = {
    "EKMFLGDQVZNTOWYHXUSPAIBRCJ",
    "AJDKSIRUXBLHWTMCQGZNPYFVOE",
    "BDFHJLCPRTXVZNYEIWGAKMUSQO",
	"ZNPYFVOWTMDKSIRUXCQGEAJBLH"
};
char reflector_wiring[26] = {
    "YRUHQSLDPXNGOKMIEBFZCWVJAT"
};
char plugboard_wiring[26] = {
    "ABCDEFGHIJKLMNOPQRSTUVWXYZ"
};

int pos = 0, pos2 = 0;
int keypos = 0, rest = 0;
int ix = 0, count = 400;
char key;
char entered[17];
char outputted[16];
char plugBoard[16];
int shifts[4] = {0,0,0,0};
unsigned char ipt;
unsigned char opCode = '*';

void INTERRUPT rti_isr(void);
void initRTI(void);
void reset(void);
void service(void);
void setPlugBoard(void);
void setRotor(void);
void initEnigma(void);
void step(void);
char forward(int, char);
char backward(int, char);
char reflect(char[], char);
char process(char);

void main(void) {
	set_clock_24mhz(); 											// usually done by D-Bug12 anyway
	initRTI();
	__asm("cli");

	KeypadInitPort();
	DispInit (2, 16);
	DDRB = 0xff;
	DDRP = 0xff;

	while (1) {
		ipt = KeypadReadPort();
		if (ipt != KEYPAD_KEY_NONE) printf("Keypad: %c\r\n", ipt); //print on serial console

		if (ipt == 'A') {
			reset();
			// private key check
			for (int i=0; i<26; i++) {
				printf("%c", plugboard_wiring[i]);
			}
			printf("\r\n");
			for (int i=0; i<4; i++) {
				printf("%d,", shifts[i]);
			}
			printf("\r\n");

			opCode = 'A';
			DispClrScr();
			service();
		} else if (ipt == 'B') {
			reset();
			opCode = 'B';
			DispClrScr();
			setPlugBoard();
		} else if (ipt == 'C') {
			reset();
			opCode = 'C';
			DispClrScr();
			setRotor();
		} else if (ipt == 'D') {
			initEnigma();
		} else {
			opCode = '*';
			DispStr(0, 0, "A.start B.set pb");
			DispStr(1, 0, "C.set rtr D.init");
		}
	}
}

void initEnigma(void) {
	for (int i=0; i<26; i++) {
		plugboard_wiring[i] = (char)(i + 'A');
	}
	for (int i=0; i<4; i++) {
		shifts[i] = 0;
	}
};

void step() {
    shifts[0]++;
    if (shifts[0] == 26) {
        shifts[0] = 0;
        shifts[1]++;
    }
    if (shifts[1] == 26) {
        shifts[1] = 0;
        shifts[2]++;
    }
    if (shifts[2] == 26) {
        shifts[2] = 0;
        shifts[3]++;
    }
    if (shifts[3] == 26)
    	shifts[3] = 0;
}

char reflect(char arr[], char ch) {
    int idx = ch - 'A';
    return arr[idx];
}

char forward(int i, char ch) {
    int idx = ch - 'A';
    idx = (idx + shifts[i] + 26) % 26;
    return rotor_wiring[i][idx];
}

char backward(int i, char ch) {
    int idx = 0;

    for (int j=0; j<26; j++) {
        if (rotor_wiring[i][j] == ch) {
            idx = j - shifts[i] + 26;
            idx = (idx < 0) ? (-idx % 26) : idx % 26;
            break;
        }
    }

    return idx + 'A';
}

char process(char ch) {
    if (ch < 'A' || ch > 'Z') return ch;

    ch = reflect(plugboard_wiring, ch); // pass plug board
    for (int j=0; j<4; j++) {
        ch = forward(j, ch);            // rotors in
    }
    ch = reflect(reflector_wiring, ch); // pass refector
    for (int j=3; j>=0; j--) {
        ch = backward(j, ch);           // rotors out
    }
    ch = reflect(plugboard_wiring, ch); // pass plug board
    step();

    return ch;
}

void setRotor(void) {
	int i = 0;
	int shift = 0;
	DispStr(0, 0, "Set Rotor: ");

	while (i < 4) {
		ipt = KeypadReadPort();

		if (ipt != KEYPAD_KEY_NONE) {
			printf("Keypad: %c\r\n", ipt); //print on serial console
			if (ipt == 'D') {
				break;
			} else if (ipt >= '0' && ipt <= '9') {
				shift = shift * 10 + (ipt - '0');
			} else if (ipt == '#') {
				shifts[i++] = shift % 26; // make sure shifts[i] is < 26
				shift = 0;
			} else {
				continue;
			}
			// print entered
			entered[pos] = ipt;
			entered[pos+1] = '\0';
			DispStr(1, 0, entered);
			pos = pos + 1;
		}
	}
}

void setPlugBoard(void) {
	service();

	for (int i=0; i<strlen(plugBoard); i++) {
		if (plugBoard[i] >= 'A' && plugBoard[i] <= 'Z') {
			if (i+1 < strlen(plugBoard) && plugBoard[i+1] >= 'A' && plugBoard[i+1] <= 'Z') {
				// get two characters
				int idx1 = plugBoard[i] - 'A';
				int idx2 = plugBoard[++i] - 'A';
				// exchange order
				char temp = plugboard_wiring[idx1];
				plugboard_wiring[idx1] = plugboard_wiring[idx2];
				plugboard_wiring[idx2] = temp;
			}
		}
	}
}

void service(void) {
	printf("%c\r\n", opCode);

	while (1) {
		ipt = KeypadReadPort();
		if (ipt != KEYPAD_KEY_NONE) printf("Keypad: %c\r\n", ipt);  //print on serial console
		if (ipt == 'D') break;										// back to menu
		if (ipt == '*') {											// reset input before selecting
			reset();
			continue;
		}

		if(ipt >= '0' && ipt <= '9') {
			DispStr(1, 0, entered);
			key = ipt;
			entered[pos] = keyboard[key - '0'][keypos++];
			entered[pos+1] = '\0';
			DispStr(1, 0, entered);


			while (1) {
				ipt = KeypadReadPort();

				if (ipt != KEYPAD_KEY_NONE) printf("Keypad: %c\r\n", ipt); //print on serial console
				if (ipt == '#') break;								// have selected the character
				if (ipt == '*') {
					rest = 1;										// reset while selecting
					break;
				}
				if (ipt == KEYPAD_KEY_NONE || ipt != key) continue; // make sure we push the same key to select
				if (keyboard[key-'0'][keypos] == '@') keypos = 0;	// get to stop sign, go to keyboard[0]

				entered[pos] = keyboard[key - '0'][keypos++];       // push the character into entered
				entered[pos+1] = '\0';								// push '\0'
				DispStr(1, 0, entered);								// display entered
			}

			if (rest) {
				reset();
				continue;
			}

			if (opCode == 'A') {
				outputted[pos2] = process(entered[pos]);
				outputted[pos2+1] = '\0';
				DispStr(0, 0, outputted);							// display encrypted/decrypted text
			} else if (opCode == 'B') {
				plugBoard[pos2] = entered[pos];						// push to plugboard
				plugBoard[pos2+1] = '\0';
			}

			keypos = 0;												// reset keyboard pointer
			pos = pos + 1;											// pos++;
			pos2 = pos2 + 1;										// pos2++
		} else {
			entered[pos] = '_';										// position indicator
			entered[pos+1] = '\0';
			DispStr(1, 0, entered);

			if (opCode == 'B') {
				DispStr(0, 0, "Set plug board");					// print title
			}
		}
	}
}

void reset(void) {
	rest = 0;								// have rest
	DispClrScr();							// clear screen
	pos = 0;								// reset pointer1
	pos2 = 0;								// reset pointer2
	keypos = 0;								// reset key pointer
	entered[pos+1] = '\0';					// stop entered
	outputted[pos2] = '\0';					// stop outputted
}

void initRTI(void) {
	RTICTL = 0x11;							// RTI interval set to 256ms
	CRGINT |= 0x80;							// enable RTI (RTIE == 0x80?)
	UserRTI = (unsigned int) & rti_isr; 	// register the ISR unit
}

void INTERRUPT rti_isr(void) {
	CRGFLG = 0x80; 							// clear the RTI - don't block the other interrupts
	// rotor simulation
	PORTB = segment_decoder[shifts[ix]];    // output rotor indicator on LED
	PTP = digit_decoder[ix++];				// rotor position indicator
	if (ix == 4) ix = 0;
	count--;
	if (count == 0) count = 400;
	// debounce
	keypad_debounce_timer++;
	if (keypad_debounce_timer > 4000){ 		// original timer, no modification
		keypad_debounce_timer = 0;
		EnableKeyboardAgain();
  }
}
