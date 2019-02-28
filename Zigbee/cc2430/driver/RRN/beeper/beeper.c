#include <iocc2430.h>
#include "hal_types.h"
#include "ZComDef.h"
#include "delay.h"
#include "beeper.h"

#define BEEPER       P2_0
#define BEEPER_BIT 0

#define MOTOR 	     P1_2
#define MOTOR_BIT  2

void beep_init(void)
{
	P2DIR |= (0x01<<BEEPER_BIT);  //set to output
	PERCFG |= (0x01<<4); // set timer4 location
	BEEPER = 0;
}

void beep_begin(void)
{
	P2SEL |= (0x01<<BEEPER_BIT);  // set to peripheral	

	T4CCTL0 = (0x00<<6) | (0x02<<3) |(0x01<<2) ;
	T4CC0 = 0x7D;
	T4CTL = (0x06<<5) | (0x00<<3) | (0x02<<0);

	T4CTL &= ~(0x01<<2); //clear
	T4CTL |= (0x01<<4);  //start
}
void beep_stop(void)
{
	P2SEL &= ~(0x01<<BEEPER_BIT);
	BEEPER = 0;
}



