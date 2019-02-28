/***********************************************
KeyAudioISR.c 
    Revised:        $Date: 2009/10/16 23:45:03 $
    Revision:       $Revision: 1.12 $

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
#include "lcd_interface.h"
#include "hal_audio.h"
#include "hal_key.h" 
#include "MPDep.h"
#include "hal_drivers.h"
/*******************************************************************************
// Global macro define
*******************************************************************************/
#ifdef AUDIO_SERIAL
//uint8 nWorkStatus = WORK_STATUS_IDLE;
#else
uint8 nWorkStatus = WORK_STATUS_LCDKEY;  //= WORK_STATUS_IDLE;
uint8 IsNumberOn = FALSE; //If no number key is pressed, and just pressed yes. no answer.
#endif

#if (defined AUDIO_SERIAL) || (defined MINE_CONTROLLER)
HAL_ISR_FUNCTION( halKeyAudioIsr, P1INT_VECTOR )
{
	if (P1IFG & 0x10)
	{
		P1IFG &= ~0x10;
	#ifdef SINGLE_AUDIO_TEST
		// Get key
		uint8 rs = 0;
		rs = GetKey();
		InitialLcd();
		LcdClearDisplay();

		if (rs == KEY_NAME_SELECT)
		{
			StartAudio();
			//LcdWriteReg(0x03,0x52);   
			//LcdSetCurXY(0, 0);
			//LcdWriteMem('O');
			//LcdWriteMem('N');
		}
	#else
             if(MP_IsPowerOn())
                {
		        HalKeyPoll();
                     #if ((defined HAL_AUDIO) && (HAL_AUDIO==TRUE)) || (defined MINE_CONTROLLER)
                     Hal_SetKeyActive(TRUE);
                    #endif
                }
	#endif
	}
	
	if(P1IFG & 0x01)
	{
		P1IFG &= ~0x01;
		ReadAudioData();
	}

	if(P1IFG & 0x02)
	{
		P1IFG &= ~0x02;
		WriteAudioData();
	}
	// clear interrupt flag on P1
	P1IF = 0; 
       }
#else
/**************************************************************************************************
 * @fn      halKeyAudioIsr
 *
 * @brief   Port0 ISR. For mobile phone
 *
 * @param
 *
 * @return
 **************************************************************************************************/
HAL_ISR_FUNCTION( halKeyAudioIsr, P0INT_VECTOR )
{
	if (P0IFG & 0x40) 
	{ 
	#ifndef SINGLE_AUDIO_TEST
		// interrupt on P0_6, used by key
		HalKeyPoll();
	#else //for one node test only.
		uint16 iKey;
		// clear intterup flag on P0_6
		//P0IFG &= ~0x40;
		if (nWorkStatus == WORK_STATUS_LCDKEY) 
		{
			// turn on backlight
			BACKLIGHT_ON;                        
			// Get Key by ADC sampling
			iKey = GetKey();        
			// Display it on LCD
			           
			LCDDisplay(iKey);
			// clear and start timer1 for backlight counting
			//TIMER1_RESET;           // reset timer
			//TIMER1_START;           // start timer 
			            
			if (iKey == KEY_YES) 
			{                  //Start Audio once press "yes"
				nWorkStatus = WORK_STATUS_AUDIO;
				AUDIO_ON;
				StartAudio();
				P0IFG = 0;
				P0IF = 0;
			}                 
		} 
		else if (nWorkStatus == WORK_STATUS_AUDIO) 
		{

			iKey = GetKey();                        
			if (iKey == KEY_NO) //to see if "No" is pressed
			{     //if "No" pressed, stop Audio
				nWorkStatus = WORK_STATUS_LCDKEY;
				AUDIO_OFF;
				StartLCD();
			}  
		} 
		else
		{
			// pseudo interrupt, do nothing
		}
	#endif

		// clear intterupt flag on P0_6. we move the clear clause to the end
		//of the part. For we click the key may produce two interrupts.
		P0IFG &= ~0x40;
	}
	if ((P0IFG & 0x02) && ON_AUDIO()) 
	{// interrupt on P0_1, used by aduio decoder
		P0IFG &= ~0x02;
		WriteAudioData();
	}
	if ((P0IFG & 0x01) && ON_AUDIO()) 
	{// interrupt on P0_0, used by aduio encoder
		// clear intterup flag on P0_0
		P0IFG &= ~0x01;
		ReadAudioData();
		//HalAudioPoll();
	}
	// clear interrupt flag on P0    
	P0IF = 0;
}
#endif
