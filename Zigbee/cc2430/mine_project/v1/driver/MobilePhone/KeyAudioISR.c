/***********************************************
KeyAudioISR.c 
Revised:        $Date: 2010/12/11 22:57:33 $
Revision:       $Revision: 1.2 $

Description:    This file define the interrupt routine on P0 port for yiri mobile phone.
we multiplexing use the port for LCD/audio, so need to handle the interrupt specially.

Update:
For new audio, use serial-port and no multiplex use for LCD/audio. and interrupt 
move to P1 now.
************************************************/
/*********************************************************************
*********************************************************************/
#include "KeyAudioISR.h"
#include "hal_mcu.h"
#include "lcd_serial.h"
#include "hal_audio.h"
#include "hal_key.h" 
#include "hal_drivers.h"
#include "MobilePhone_Dep.h"
/*******************************************************************************
// Global macro define
*******************************************************************************/

HAL_ISR_FUNCTION( halKeyAudioIsr, P1INT_VECTOR )
{
	//only serve when IEN is set
	if((P1IFG & 0x10) && (P1IEN & 0x10))
	{
		P1IFG &= ~0x10;
		if(MP_IsPowerOn())
		{
			HalKeyPoll();
#if ((defined HAL_AUDIO) && (HAL_AUDIO==TRUE)) 
			Hal_SetKeyActive(TRUE);
#endif
		}
	}

	//only serve when IEN is set
	if((P1IFG & 0x01) && (P1IEN & 0x01))
	{
		P1IFG &= ~0x01;
		ReadAudioData();
	}

	//only serve when IEN is set
	if((P1IFG & 0x02) && (P1IEN & 0x02))
	{
		P1IFG &= ~0x02;
		WriteAudioData();
	}
	// clear interrupt flag on P1
	P1IF = 0; 
}
