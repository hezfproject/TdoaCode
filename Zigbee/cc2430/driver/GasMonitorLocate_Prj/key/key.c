#include <iocc2430.h>
#include "key.h"

/***************************
// Local macro define
****************************/
// Key-chip interface define

/*******************************************************************************
// Initial Keyboard
*******************************************************************************/
/****************************************************************
*** Initial key-scan chip and key interrupt
*****************************************************************/
void InitialKey(void)
{
      PICTL |= 0x02;	       // falling  edge trigger for P1
      IRCON2 &= ~0x08;		// clear ifg of P1  
	IEN2 |= 0x10;   		// enable P1 interrupt

	P1SEL &= ~(0x1<<KEY_UP_BIT);			//  Select GPIO
	P1DIR &= ~(0x1<<KEY_UP_BIT);			//  Set to input 
	P1INP |= (0x1<<KEY_UP_BIT);                 // set to tristate
	P1IFG &= ~(0x1<<KEY_UP_BIT);			 // clear ifg
	P1IEN |=(0x1<<KEY_UP_BIT);                  // Enable interupt

	P1SEL &= ~(0x1<<KEY_DOWN_BIT);			//  Select GPIO
	P1DIR &= ~(0x1<<KEY_DOWN_BIT);			//  Set to input 
	P1INP |= (0x1<<KEY_DOWN_BIT);                 // set to tristate
	P1IFG &= ~(0x1<<KEY_DOWN_BIT);			 // clear ifg
	P1IEN |=(0x1<<KEY_DOWN_BIT);                  // Enable interupt
	
	P1SEL &= ~(0x1<<KEY_POWER_BIT);			//  Select GPIO
	P1DIR &= ~(0x1<<KEY_POWER_BIT);			//  Set to input 
	P1INP |= (0x1<<KEY_POWER_BIT);                 // set to tristate
	P1IFG &= ~(0x1<<KEY_POWER_BIT);			 // clear ifg
	P1IEN |=(0x1<<KEY_POWER_BIT);                  // Enable interupt

       P1SEL &= ~(0x1<<KEY_OK_BIT);			//  Select GPIO	
	P1DIR &= ~(0x1<<KEY_OK_BIT);			//  Set to input 
	P1INP |= (0x1<<KEY_OK_BIT);                 // set to tristate
	P1IFG &= ~(0x1<<KEY_OK_BIT);			 // clear ifg
	P1IEN |=(0x1<<KEY_OK_BIT);                  // Enable interupt

       P1SEL &= ~(0x1<<KEY_HELP_BIT);			//  Select GPIO	
	P1DIR &= ~(0x1<<KEY_HELP_BIT);			//  Set to input 
	P1INP |= (0x1<<KEY_HELP_BIT);                 // set to tristate
	P1IFG &= ~(0x1<<KEY_HELP_BIT);			 // clear ifg
	P1IEN |=(0x1<<KEY_HELP_BIT);                  // Enable interupt

       P1SEL &= ~(0x1<<KEY_MODE_BIT);			//  Select GPIO	
	P1DIR &= ~(0x1<<KEY_MODE_BIT);			//  Set to input 
	P1INP |= (0x1<<KEY_MODE_BIT);                 // set to tristate
	P1IFG &= ~(0x1<<KEY_MODE_BIT);			 // clear ifg
	P1IEN |=(0x1<<KEY_MODE_BIT);                  // Enable interupt

}
