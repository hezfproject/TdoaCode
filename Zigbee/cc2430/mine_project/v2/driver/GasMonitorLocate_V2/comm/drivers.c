/**************************************************************************************************
Filename:       drivers.c
Revised:        $Date: 2011/06/19 02:14:50 $
Revision:       $Revision: 1.3 $

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
#include "drivers.h"
#include "delay.h"
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

#if (defined HAL_UART) && (HAL_UART == TRUE)
#include "Locate_uart.h"
#endif

#if (defined GASMONITOR_PROJECT)
#include "lcd_interface.h"
#include "OnBoard.h"
#include "ch4.h"
#include "beeper.h"
#endif

/**************************************************************************************************
*                                            MACROS
**************************************************************************************************/

/**************************************************************************************************
*                                          CONSTANTS
**************************************************************************************************/
static uint8 Hal_BeeperCnt;
static uint8 Hal_BeeperFreq;
static uint8 Hal_BeeperType;
static bool  Hal_IsBeeping = false;
static bool  Hal_IsBeeping_bak;
/**************************************************************************************************
*                                          TYPEDEFS
**************************************************************************************************/


/**************************************************************************************************
*                                      GLOBAL VARIABLES
**************************************************************************************************/
uint8 Hal_TaskID;

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
#if (defined AUDIO_SERIAL) || (defined GASMONITOR_PROJECT)
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

	/*LDO beeper and motor */
#if(defined GASMONITOR_PROJECT)
	ch4_init();
	beeper_init();
	motor_init();
#endif

	/*Delay to fix phone bug*/
	DelayMs(100);
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
#ifdef GASMONITOR_PROJECT
	if (events & GASMONITOR_LCD_PERSIST_EVT)
	{
		/* backlight close */
		BACKLIGHT_CLOSE();
		return events ^ GASMONITOR_LCD_PERSIST_EVT;
	}
	if (events & GASMONITOR_BEEPER_EVT)
	{
		if(Hal_BeeperCnt & 0x01 == 1)
		{
			beep_begin();
		}
		else
		{
			beep_stop();
		}
		Hal_BeeperCnt++;

		if(ZSuccess != osal_start_timerEx(Hal_TaskID, GASMONITOR_BEEPER_EVT, Hal_BeeperFreq*30))
		{
			SystemReset();
		}

		return events ^ GASMONITOR_BEEPER_EVT;
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
}

#if(defined GASMONITOR_PROJECT)
void HalResetBackLightEvent(void)
{
	uint8 backlight_ctl = LCDGetBackLightCtl();
	uint16 backlight_time;
	if(backlight_ctl == BACKLIGHT_CTL_OFF)
	{
		BACKLIGHT_CLOSE();
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
		BACKLIGHT_OPEN();
		osal_start_timerEx(Hal_TaskID, GASMONITOR_LCD_PERSIST_EVT, backlight_time);	
	}
}

// start a beeper, identify a type and frequency
// freq=0 stands for long beeping

void HalStartBeeper(uint8 type, uint8 freq)
{	
	if(freq ==0)
	{
		HalStopBeeper(NULL, true);
		Hal_IsBeeping = true;	
		beep_begin();
		MOTOR_OPEN();
		return;
	}

	if(!Hal_IsBeeping || Hal_BeeperType!=type || Hal_BeeperFreq != freq)
	{
		Hal_BeeperType = type;
		Hal_IsBeeping = true;		
		Hal_BeeperFreq = freq;
		if(ZSuccess!= osal_start_timerEx(Hal_TaskID, GASMONITOR_BEEPER_EVT, 10))
		{
			SystemReset();
		}
		MOTOR_OPEN();
	}
}

void HalStopBeeper(uint8 type, bool force)
{

	if((Hal_IsBeeping && Hal_BeeperType==type) || force)
	{
		osal_stop_timerEx(Hal_TaskID, GASMONITOR_BEEPER_EVT);
		osal_unset_event(Hal_TaskID, GASMONITOR_BEEPER_EVT);
		MOTOR_CLOSE();
		beep_stop();
		Hal_IsBeeping = false;
	}
}
void HalHaltBeeper(void)
{
	Hal_IsBeeping_bak = Hal_IsBeeping;
	HalStopBeeper(NULL, true);
	return;
}

void HalResumeBeeper(void)
{
	if(Hal_IsBeeping_bak)
	{
		HalStartBeeper(Hal_BeeperType, Hal_BeeperFreq);
	}
}

#endif


/**************************************************************************************************
**************************************************************************************************/

