/**************************************************************************************************
  Filename:       hal_drivers.c
  Revised:        $Date: 2010/09/07 22:17:49 $
  Revision:       $Revision: 1.29 $

  Description:    This file contains the interface to the Drivers Service.


  Copyright 2005-2007 Texas Instruments Incorporated. All rights reserved.

  IMPORTANT: Your use of this Software is limited to those specific rights
  granted under the terms of a software license agreement between the user
  who downloaded the software, his/her employer (which must be your employer)
  and Texas Instruments Incorporated (the "License").  You may not use this
  Software unless you agree to abide by the terms of the License. The License
  limits your use, and you acknowledge, that the Software may not be modified,
  copied or distributed unless embedded on a Texas Instruments microcontroller
  or used solely and exclusively in conjunction with a Texas Instruments radio
  frequency transceiver, which is integrated into your product.  Other than for
  the foregoing purpose, you may not use, reproduce, copy, prepare derivative
  works of, modify, distribute, perform, display or sell this Software and/or
  its documentation for any purpose.

  YOU FURTHER ACKNOWLEDGE AND AGREE THAT THE SOFTWARE AND DOCUMENTATION ARE
  PROVIDED “AS IS?WITHOUT WARRANTY OF ANY KIND, EITHER EXPRESS OR IMPLIED, 
  INCLUDING WITHOUT LIMITATION, ANY WARRANTY OF MERCHANTABILITY, TITLE, 
  NON-INFRINGEMENT AND FITNESS FOR A PARTICULAR PURPOSE. IN NO EVENT SHALL
  TEXAS INSTRUMENTS OR ITS LICENSORS BE LIABLE OR OBLIGATED UNDER CONTRACT,
  NEGLIGENCE, STRICT LIABILITY, CONTRIBUTION, BREACH OF WARRANTY, OR OTHER
  LEGAL EQUITABLE THEORY ANY DIRECT OR INDIRECT DAMAGES OR EXPENSES
  INCLUDING BUT NOT LIMITED TO ANY INCIDENTAL, SPECIAL, INDIRECT, PUNITIVE
  OR CONSEQUENTIAL DAMAGES, LOST PROFITS OR LOST DATA, COST OF PROCUREMENT
  OF SUBSTITUTE GOODS, TECHNOLOGY, SERVICES, OR ANY CLAIMS BY THIRD PARTIES
  (INCLUDING BUT NOT LIMITED TO ANY DEFENSE THEREOF), OR OTHER SIMILAR COSTS.


Should you have any questions regarding your right to use this Software,
contact Texas Instruments Incorporated at www.TI.com. 
**************************************************************************************************/


/**************************************************************************************************
*                                            INCLUDES
**************************************************************************************************/
#include "hal_types.h"
#include "OSAL.h"
#include "hal_drivers.h"
#include "hal_adc.h"
#if (defined HAL_DMA) && (HAL_DMA == TRUE)
#include "hal_dma.h"
#endif
#include "hal_key.h"
#include "hal_lcd.h"
#include "hal_led.h"
#include "hal_timer.h"
#include "hal_uart.h"
#include "hal_sleep.h"
#if (defined HAL_AES) && (HAL_AES == TRUE)
#include "hal_aes.h"
#endif
#if (defined HAL_SPI) && (HAL_SPI == TRUE)
#include "hal_spi.h"
#endif
#if (defined HAL_AUDIO) && (HAL_AUDIO == TRUE)
#include "hal_audio.h"
#include "lcd_interface.h"
#include "delay.h"
#include "mac_api.h"
#include "MineApp_global.h"
#ifndef NEW_MENU_LIB
#include "MineApp_MP_Menu.h"
#else
#include "MineApp_MenuLib.h"
#include "MenuAdjustUtil.h"
#include "MineApp_MP_Function.h"
#endif

#elif (defined MINE_CONTROLLER)
#include "lcd_interface.h"
#include "delay.h"
#include "mac_api.h"
#endif
#if (defined WATCHDOG) &&(WATCHDOG==TRUE)
#include "WatchDogUtil.h"
#endif
/**************************************************************************************************
*                                            MACROS
**************************************************************************************************/
/*
#if (defined HAL_AUDIO) && (HAL_AUDIO == TRUE)
#ifndef BACKLIGHT_PERSIST_TIME
#define BACKLIGHT_PERSIST_TIME 3000 
#endif
#endif
*/

/**************************************************************************************************
*                                          CONSTANTS
**************************************************************************************************/


/**************************************************************************************************
*                                          TYPEDEFS
**************************************************************************************************/


/**************************************************************************************************
*                                      GLOBAL VARIABLES
**************************************************************************************************/
uint8 Hal_TaskID;

#if ((defined HAL_AUDIO) && (HAL_AUDIO == TRUE)) || (defined MINE_CONTROLLER)
static bool     keyactive = TRUE;
static bool     HalDoingCellSwitch = FALSE;
#endif

extern void HalLedUpdate( void ); /* Notes: This for internal only so it shouldn't be in hal_led.h */

/**************************************************************************************************
*                                      FUNCTIONS - API
**************************************************************************************************/

/**************************************************************************************************
* @fn      Hal_Init
*
* @brief   Hal Initialization function.
*
* @param   task_id - Hal TaskId
*
* @return  None
**************************************************************************************************/
void Hal_Init( uint8 task_id )
{
	/* Register task ID */
	Hal_TaskID = task_id;

}

/**************************************************************************************************
* @fn      Hal_DriverInit
*
* @brief   Initialize HW - These need to be initialized before anyone.
*
* @param   task_id - Hal TaskId
*
* @return  None
**************************************************************************************************/
void HalDriverInit (void)
{
	/* TIMER */
	HalTimerInit();
	/* ADC */
#if (defined HAL_ADC) && (HAL_ADC == TRUE)
	HalAdcInit();
#endif

	/* DMA */
#if (defined HAL_DMA) && (HAL_DMA == TRUE)
	// Must be called before the init call to any module that uses DMA.
	HalDmaInit();
#endif

	/*SPI*/
#if (defined HAL_SPI) && (HAL_SPI == TRUE)
	HalSpiInit();
#if (defined HAL_UART) //if SPI enable, disable UART.
#undef  HAL_UART
#endif
#endif

	/*AUDIO*/
#if (defined HAL_AUDIO) && (HAL_AUDIO == TRUE)
	HalAudioInit();
#endif
#ifdef AUDIO_SERIAL
	//BACKLIGHT_OPEN();
	InitialLcd();
#endif

	/* AES */
#if (defined HAL_AES) && (HAL_AES == TRUE)
	HalAesInit();
#endif

	/* LCD */
#if (defined HAL_LCD) && (HAL_LCD == TRUE)
	HalLcdInit();
#endif

	/* LED */
#if (defined HAL_LED) && (HAL_LED == TRUE)
	HalLedInit();
#endif

	/* UART */
#if (defined HAL_UART) && (HAL_UART == TRUE)
	HalUARTInit();
#endif

	/* KEY */
#if (defined HAL_KEY) && (HAL_KEY == TRUE)
	HalKeyInit();
#endif

	/*Delay to fix phone bug*/
#if (defined HAL_AUDIO) && (HAL_AUDIO == TRUE)
	DelayMs(100);
#endif
}


/**************************************************************************************************
* @fn      Hal_ProcessEvent
*
* @brief   Hal Process Event
*
* @param   task_id - Hal TaskId
*          events - events
*
* @return  None
**************************************************************************************************/
uint16 Hal_ProcessEvent( uint8 task_id, uint16 events )
{
	uint8 *msgPtr;

	if ( events & SYS_EVENT_MSG )
	{
		msgPtr = osal_msg_receive(Hal_TaskID);

		while (msgPtr)
		{
			/* Do something here - for now, just deallocate the msg and move on */

			/* De-allocate */
			osal_msg_deallocate( msgPtr );
			/* Next */
			msgPtr = osal_msg_receive( Hal_TaskID );
		}
		return events ^ SYS_EVENT_MSG;
	}

#if ((defined HAL_AUDIO) && (HAL_AUDIO == TRUE)) || (defined MINE_CONTROLLER)
	if (events & MINEAPP_LCD_PERSIST_EVT)
	{
			/* backlight close */
#ifndef MP_VERSION_1_1
			BACKLIGHT_CLOSE();
			StoreParam_t param = *(StoreParam_t*)MINEAPP_PARAM_ADDR;
			param.backLightOn = false;
			*(StoreParam_t*)MINEAPP_PARAM_ADDR = param;
#endif

			/* pad lock*/
#if ((defined HAL_AUDIO) && (HAL_AUDIO == TRUE) && (defined NEW_MENU_LIB))
			Menu_handle_msg(MSG_PAD_LOCK, NULL, 0);

			/* set key active flag */
			Hal_SetKeyActive(FALSE);
#endif       
		return events ^ MINEAPP_LCD_PERSIST_EVT;
	}
#endif

#if (defined HAL_AUDIO) && (HAL_AUDIO == TRUE)
	#if 0
	if (events & MINEAPP_AUDIO_START_EVT)
	{
		uint8 maxFrameRetries = 0;
		MAC_MlmeSetReq(MAC_MAX_FRAME_RETRIES, &maxFrameRetries);
		HalResetBackLightEvent();
		//HalRingClose();
		osal_stop_timerEx(Hal_TaskID, MINEAPP_RING_EVENT);
	#ifndef NEW_MENU_LIB	
		DelayMs(65);
	#endif
		//if (ON_CALLEDWAIT())
		//MineApp_OpenCall();
	#ifdef NEW_MENU_LIB
		SET_ON_AUDIO();
	#endif
		HalAudioOpen();
		return events ^ MINEAPP_AUDIO_START_EVT;
	}

	if (events & MINEAPP_AUDIO_STOP_EVT)
	{
		HalRingClose();
		HalAudioClose();
		uint8 maxFrameRetries = 3;
		MAC_MlmeSetReq(MAC_MAX_FRAME_RETRIES, &maxFrameRetries);
		HalResetBackLightEvent();
		return events ^ MINEAPP_AUDIO_STOP_EVT;
	}
	#endif
	
	if (events & MINEAPP_RING_EVENT)
	{
		uint8 play = HalRingPlay();
		if(HalRingGetOpenFlag() == OPENFLAG_ASBELL ||HalRingGetOpenFlag() == OPENFLAG_ASSMS)
		{
			HalShakePlay();
		}
		if(play!=RING_ENDED) 
		{
			osal_start_timerEx(Hal_TaskID, MINEAPP_RING_EVENT, 15);
		}
		else
		{   
				HALRING_SHAKEOFF();
				if (ON_CALLED() && HalRingGetOpenFlag() == OPENFLAG_ASBELL) //called side
				{
#ifndef NEW_MENU_LIB
					Display_Msg_Menu(MISSED_CALL_MSG, NULL);
#else
					Menu_handle_msg(MSG_MISSED_CALL, NULL, 0);
#endif

				}
				else if (ON_CALLING() || ON_CALLINGWAIT()) //calling side
				{
#ifndef NEW_MENU_LIB
					Display_Msg_Menu(INIT_MAIN_MSG, NULL);
#else
					Menu_handle_msg(MSG_INIT_MAIN, NULL, 0);
#endif

				}
			HalRingClose();
		}
		return events ^ MINEAPP_RING_EVENT;
	}
#endif

	if ( events & HAL_LED_BLINK_EVENT )
	{
#if (defined (BLINK_LEDS)) && (HAL_LED == TRUE)
		HalLedUpdate();
#endif /* BLINK_LEDS && HAL_LED */
		return events ^ HAL_LED_BLINK_EVENT;
	}

	if (events & HAL_KEY_EVENT)
	{

#if (defined HAL_KEY) && (HAL_KEY == TRUE)
		/* Check for keys */
		HalKeyPoll();

		/* if interrupt disabled, do next polling */
		if (!Hal_KeyIntEnable)
		{
			osal_start_timerEx( Hal_TaskID, HAL_KEY_EVENT, 100);
		}
#endif // HAL_KEY

		return events ^ HAL_KEY_EVENT;
	}

#ifdef POWER_SAVING
	if ( events & HAL_SLEEP_TIMER_EVENT )
	{
		halRestoreSleepLevel();
		return events ^ HAL_SLEEP_TIMER_EVENT;
	}
#endif

	/* Nothing interested, discard the message */
	return 0;

}

/**************************************************************************************************
* @fn      Hal_ProcessPoll
*
* @brief   This routine will be called by OSAL to poll UART, TIMER...
*
* @param   task_id - Hal TaskId
*
* @return  None
**************************************************************************************************/
__near_func void Hal_ProcessPoll ()
{

	/* Timer Poll */
	HalTimerTick();

	/* UART Poll */
#if (defined HAL_UART) && (HAL_UART == TRUE)
	HalUARTPoll();
#endif

	/* SPI Poll */
#if (defined HAL_SPI) && (HAL_SPI == TRUE)
	HalSPIPoll();
#endif

	/* Audio Poll */
#if (defined HAL_AUDIO) && (HAL_AUDIO == TRUE)
	HalAudioPoll();
#endif
#if (defined WATCHDOG) &&(WATCHDOG==TRUE)
	FeedWatchDog();
#endif
}

#if ((defined HAL_AUDIO) && (HAL_AUDIO == TRUE)) || (defined MINE_CONTROLLER)
void HalResetBackLightEvent(void)
{
	StoreParam_t param = *(StoreParam_t*)MINEAPP_PARAM_ADDR;
	
	uint8 backlight_ctl = LCDGetBackLightCtl();
      uint16 backlight_time;
      if(backlight_ctl == BACKLIGHT_CTL_OFF)
      {
      		BACKLIGHT_CLOSE();
		param.backLightOn = false;
      }
      else
      {
            switch(backlight_ctl)
            {
                case    BACKLIGHT_CTL_10S:
                    backlight_time = 10000;
                    break;
                case    BACKLIGHT_CTL_20S:
                    backlight_time = 20000;
                    break;
                case    BACKLIGHT_CTL_30S:
                    backlight_time = 30000;
                    break;
            }
      }
      
	if(backlight_ctl == BACKLIGHT_CTL_10S ||backlight_ctl == BACKLIGHT_CTL_20S || backlight_ctl == BACKLIGHT_CTL_30S)
	{
	       BACKLIGHT_OPEN();
		param.backLightOn = true;
		osal_start_timerEx(Hal_TaskID, MINEAPP_LCD_PERSIST_EVT, backlight_time);	
	}
#ifdef NEW_MENU_LIB
	else //if(CurrentNodeID==MENU_ID_MAIN && HalGetPadLockEnable())
	{
        	backlight_time = 10000;
		osal_start_timerEx(Hal_TaskID, MINEAPP_LCD_PERSIST_EVT, backlight_time);	
	}
#endif
	*(StoreParam_t*)MINEAPP_PARAM_ADDR = param;
}
void Hal_SetKeyActive(bool val)
{
    keyactive = val;
}

void Hal_SetCellSwitchStat(bool val)
{
	HalDoingCellSwitch = val;
}
bool Hal_AllowSleep(void)
{
#if (defined MINE_CONTROLLER)
	return !(keyactive);
#else
    return !(keyactive || HalRingIsPlaying() || HalDoingCellSwitch);
#endif
}
/*
bool Hal_IsKeyActive()
{
    return keyactive;
}
*/
#endif


/**************************************************************************************************
**************************************************************************************************/

