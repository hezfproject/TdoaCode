/***********************************************
KeyAudioISR.c
Revised:        $Date: 2011/04/01 22:13:57 $
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
#include "MobilePhone_global.h"
#include "MobilePhone.h"
#include "MobilePhone_Function.h"
//#include "Lcd_aging_check.h"

/*******************************************************************************
// Global macro define
*******************************************************************************/

HAL_ISR_FUNCTION(halKeyAudioIsr, P1INT_VECTOR)
{
    //only serve when IEN is set
    if((P1IFG & 0x80) && (P1IEN & 0x80))   //key  p1.7
    {
        if(MP_IsPowerOn())
        {
            uint8 x;
            HAL_ENTER_CRITICAL_SECTION(x);
            HalKeyPoll();
            HAL_EXIT_CRITICAL_SECTION(x);
#if ((defined HAL_AUDIO) && (HAL_AUDIO==TRUE))

            extern bool     keyactive;
            extern bool     lastkeyactive;

            //lastkeyactive = keyactive;

            Hal_SetKeyActive(TRUE);
            extern uint8 MP_Function_TaskID;
            MP_start_timerEx(MP_Function_TaskID, MP_FUNC_KEY_STATE_EVENT, 45000);

            //MP_start_timerEx(MP_TaskId, MP_KEYBOARD_STATE_EVENT, 1000);
#endif
            //Check_LCD_aging_stop_and_reset();
        }

        P1IFG &= ~0x80;
    }

    if((P1IFG & 0x40) && (P1IEN & 0x40))   //key  p1.6
    {
        //if()
        extern uint8 MP_TaskId;
        MP_start_timerEx(MP_TaskId, MP_UART_READ_EVENT, 30);

        P1IFG &= ~0x40;
    }

    //only serve when IEN is set
    if((P1IFG & 0x01) && (P1IEN & 0x01))  // EPR=p1.0
    {
        P1IFG &= ~0x01;
        ReadAudioData();
    }

    //only serve when IEN is set
    if((P1IFG & 0x04) && (P1IEN & 0x04))  // DPE=p1.2
    {
        P1IFG &= ~0x04;
        WriteAudioData();
    }
    // clear interrupt flag on P1
    P1IF = 0;
}
