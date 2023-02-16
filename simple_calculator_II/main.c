#include <hcs12.h>
#include <dbug12.h>

#include "lcd.h"
#include "keypad.h"
#include "util.h"

/**
 * 7 segments LED decoder
 * 0,1,2,3,4,5,6,7,8,9,A,B,C,D,E,F,G,H
 *
 * Example: if you want to show "1" on LED segments
 * you should do the following:
 * DDRB = 0xff; //set all pin on port b to output
 * PORTB = segment_decoder[1]; //which means one decodes to 0x06:
 * G F E D C B A
 * 0 0 0 0 1 1 0
 *
 *		 A
 * 		----
 * 	   |	| B
 * 	 F |  	|
 * 		--G-        ===> if B and C segments are one we get the shape of 1 (number one)
 * 	   |	| C
 * 	 E |	|
 * 		----
 *       D
 */
void mSDelay(unsigned int);

unsigned int segment_decoder[]={
                                 0x3f,0x06,0x5b,0x4f,0x66,
                                 0x6d,0x7d,0x07,0x7f,0x6f,
                                 0x77,0x7c,0x39,0x5e,0x79,
                                 0x71,0x3d,0x76
                               };
const unsigned char keypad[4][4] = {
									'1','2','3','A',
									'4','5','6','B',
									'7','8','9','C',
									'*','0','#','D'
									};
unsigned int digit_decoder[] = {0x07, 0x0B, 0x0D, 0x0E};

unsigned char column,row;
int num1 = 0, num2 = 0, opCode = 0, res = 3;

/****************MAIN*****************/
void main(void){                          //OPEN MAIN
   DDRB = 0xFF;                           //MAKE PORTB OUTPUT
   DDRJ |=0x02;
   PTJ &=~0x02;                            //ACTIVATE LED ARRAY ON PORT B
   DDRP |=0x0F;                           //
   DDRA = 0x0F;                           //MAKE ROWS INPUT AND COLUMNS OUTPUT


   while(1){                              //OPEN WHILE(1)
      do{                                 //OPEN do1
         PORTA = PORTA | 0x0F;            //COLUMNS SET HIGH
         row = PORTA & 0xF0;              //READ ROWS
      }while(row == 0x00);                //WAIT UNTIL KEY PRESSED //CLOSE do1



      do{                                 //OPEN do2
         do{                              //OPEN do3
            mSDelay(1);                   //WAIT
            row = PORTA & 0xF0;           //READ ROWS
         }while(row == 0x00);             //CHECK FOR KEY PRESS //CLOSE do3

         mSDelay(15);                     //WAIT FOR DEBOUNCE
         row = PORTA & 0xF0;
      }while(row == 0x00);                //FALSE KEY PRESS //CLOSE do2

      while(1){                           //OPEN while(1)
         PORTA &= 0xF0;                   //CLEAR COLUMN
         PORTA |= 0x01;                   //COLUMN 0 SET HIGH
         row = PORTA & 0xF0;              //READ ROWS
         if(row != 0x00){                 //KEY IS IN COLUMN 0
            column = 0;
            break;                        //BREAK OUT OF while(1)
         }
         PORTA &= 0xF0;                   //CLEAR COLUMN
         PORTA |= 0x02;                   //COLUMN 1 SET HIGH
         row = PORTA & 0xF0;              //READ ROWS
         if(row != 0x00){                 //KEY IS IN COLUMN 1
            column = 1;
            break;                        //BREAK OUT OF while(1)
         }

         PORTA &= 0xF0;                   //CLEAR COLUMN
         PORTA |= 0x04;                   //COLUMN 2 SET HIGH
         row = PORTA & 0xF0;              //READ ROWS
         if(row != 0x00){                 //KEY IS IN COLUMN 2
            column = 2;
            break;                        //BREAK OUT OF while(1)
         }
         PORTA &= 0xF0;                   //CLEAR COLUMN
         PORTA |= 0x08;                   //COLUMN 3 SET HIGH
         row = PORTA & 0xF0;              //READ ROWS
         if(row != 0x00){                 //KEY IS IN COLUMN 3
            column = 3;
            break;                        //BREAK OUT OF while(1)
         }
         row = 0;                         //KEY NOT FOUND
      break;                              //step out of while(1) loop to not get stuck
      }                                   //end while(1)

      unsigned char ipt = 0;
      if(row == 0x10){
         ipt = keypad[0][column];         //OUTPUT TO PORTB LED
      } else if(row == 0x20) {
    	 ipt = keypad[1][column];
      } else if(row == 0x40) {
    	 ipt = keypad[2][column];
      } else if(row == 0x80) {
    	 ipt = keypad[3][column];
      }

      if (num1 != -1 && num2 != -1) {
    	  if (ipt >= '0' && ipt <= '9') {
    		  int dig = ipt - '0';
    		  PORTB = segment_decoder[dig]; // input indicator
    		  num1 = num1 * 10 + dig;
    	  } else {
    		  if (ipt != '#') {
    			  if (ipt == 'A') {
    				  num2 = num1; num1 = 0; // don't understand why it has to be here
    				  PORTB = segment_decoder[10]; // '+' operator indicator
    				  opCode = 1;
    			  } else if (ipt == 'B') {
    				  num2 = num1; num1 = 0;
    				  PORTB = segment_decoder[11]; // '-' operator indicator
    				  opCode = 2;
    			  } else if (ipt == 'C') {
    				  num2 = num1; num1 = 0;
    				  PORTB = segment_decoder[12]; // '*' operator indicator
    				  opCode = 3;
    			  } else if (ipt == 'D') {
    				  num2 = num1; num1 = 0;
    				  PORTB = segment_decoder[13]; // '/' operator indicator
    				  opCode = 4;
    			  }

    		  } else {
    			  PORTB = 0xFF; // '=' operator indicator

    			  if (opCode == 1) { // addition
    				  res = num1 + num2;
    			  } else if (opCode == 2) { // multiplication
    				  res = num1 * num2;
    			  } else if (opCode == 3) { // subtraction
    				  res = num2 - num1;
    			  } else if (opCode == 4) { // division
    				  res = num2 / num1;
    			  }

    			  num1 = -1; num2 = -1;
    		  }
    	  }
    	  PTP = digit_decoder[0]; // output at right most
      } else {
    	  if (res == 0) {
    		  PORTB = segment_decoder[res];
    		  PTP = digit_decoder[1];
    	  } else {
		  break;
    		  
    	  }
      }

      do {
         mSDelay(15);
         PORTA = PORTA | 0x0F;            //COLUMNS SET HIGH
         row = PORTA & 0xF0;              //READ ROWS
      } while(row != 0x00);                //MAKE SURE BUTTON IS NOT STILL HELD
   }                                      //CLOSE WHILE(1)
	
	while (1) {
		  int temp = res, i = 0;

    		  while (i != 4 && temp != 0) {
    			  int dig = temp % 10;
    			  PORTB = segment_decoder[dig];
    			  PTP = digit_decoder[i++];
    			  mSDelay(175);
    			  temp /= 10;
    		  }
	}
}                                         //CLOSE MAIN

/**********************SUBROUTINES***********/
void mSDelay(unsigned int itime){
	unsigned int i;
	unsigned int j;

	for(i=0;i<itime;i++)
		for(j=0;j<4000;j++);
}
