/***********************************************
KeyAudioISR.c 
    Revised:        $Date: 2011/06/01 22:55:27 $
    Revision:       $Revision: 1.1 $

    Description:    This file define the interrupt routine on P1 port and P0 port for Gas monitor

************************************************/
/*********************************************************************
*********************************************************************/
#include "KeyAudioISR.h"
#include "hal_mcu.h"
#include "hal_key.h"
#include "Key.h"

static uint16 keys;

/*******************************************************************************
// Global macro define
*******************************************************************************/

HAL_ISR_FUNCTION( halKeyAudioIsr, P1INT_VECTOR )
{
      if(P1IFG & KEY_MASK)
      {
            keys = (~P1) & KEY_MASK;
            
            P1IFG &= ~KEY_MASK;
            HalKeyPoll();
      }
      
	// clear interrupt flag on P1
	P1IF = 0; 
}

/**************************************************************************************************
 * @fn      halKeyAudioIsr
 *
 * @brief   Port0 ISR. 
 *
 * @param
 *
 * @return
 **************************************************************************************************/
/*
HAL_ISR_FUNCTION( halKeyAudioIsr, P0INT_VECTOR )
{
}
*/

uint16 GetKey(void)
{
    return keys;
}

