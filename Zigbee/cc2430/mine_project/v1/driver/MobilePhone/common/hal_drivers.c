/**************************************************************************************************
Filename:       hal_drivers.c
Revised:        $Date: 2011/01/08 00:09:17 $
Revision:       $Revision: 1.10 $

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
#include "OnBoard.h"
#include "hal_drivers.h"
#include "hal_adc.h"
#if (defined HAL_DMA) && (HAL_DMA == TRUE)
#include "hal_dma.h"
#endif
#include "hal_key.h"
#include "key.h"
#include "hal_lcd.h"
#include "hal_led.h"
#include "hal_timer.h"
#include "hal_uart.h"
#include "hal_sleep.h"
#include "hal_alarm.h"
#if (defined HAL_AES) && (HAL_AES == TRUE)
#include "hal_aes.h"
#endif
#if (defined HAL_SPI) && (HAL_SPI == TRUE)
#include "hal_spi.h"
#endif
#if (defined HAL_AUDIO) && (HAL_AUDIO == TRUE)
#include "hal_audio.h"
#include "lcd_serial.h"
#include "delay.h"
#include "mac_api.h"
#include "MobilePhone_global.h"
#include "MobilePhone_MenuLib.h"
#include "MenuAdjustUtil.h"
#include "MobilePhone_Function.h"
#include "MobilePhone_cfg.h"
#include "MobilePhone_Dep.h"
#endif
#if (defined WATCHDOG) &&(WATCHDOG==TRUE)
#include "WatchDogUtil.h"
#endif
#include "mac_api.h"
#include "mac_main.h"
#include "MacUtil.h"
#include "App_cfg.h"
#include "Osal_Nv.h"
#include "MenuLib_Nv.h"
/**************************************************************************************************
*                                            MACROS
**************************************************************************************************/

/**************************************************************************************************
*                                          CONSTANTS
**************************************************************************************************/

#define HAL_AIR_RETRANS_TIME  3
#define HAL_AIR_HEAPLEN 1

/**************************************************************************************************
*                                          TYPEDEFS
**************************************************************************************************/
typedef struct
{
	macMcpsDataReq_t* pSend;
	bool IsNeedRetrans;
	bool IsUsing;
	bool IsSending;
	uint8 retransCnt;
	uint8 handle;
	uint32 SendTimeTick;
} Hal_transInfo_t;

/**************************************************************************************************
*                                      GLOBAL VARIABLES
**************************************************************************************************/
uint8 Hal_TaskID;

#if ((defined HAL_AUDIO) && (HAL_AUDIO == TRUE))
static bool     keyactive = TRUE;
#endif

extern void HalLedUpdate ( void ); /* Notes: This for internal only so it shouldn't be in hal_led.h */
/**************************************************************************************************
*                                      static  VARIABLES
**************************************************************************************************/
//static Hal_transInfo_t  Hal_transInfo[HAL_AIR_HEAPLEN];
static bool   Hal_voicebell_playing;
static VoiceBellName_t Hal_voicebell_ringname;
/**************************************************************************************************
*                                      FUNCTIONS - API
**************************************************************************************************/

//static int8 Hal_HeapFindFirstAvail ( Hal_transInfo_t heap[] );
static void HalProcessRingEnd ( void );
static void Hal_ProcessVoiceBellEnd(void );
/**************************************************************************************************
* @fn      Hal_Init
*
* @brief   Hal Initialization function.
*
* @param   task_id - Hal TaskId
*
* @return  None
**************************************************************************************************/
void Hal_Init ( uint8 task_id )
{
	/* Register task ID */
	Hal_TaskID = task_id;

    MacUtil_t MacUtil;
    MacUtil.panID = MOBILEPHONE_NWK_ADDR;  /* default is card nwk id */
    MacUtil.dst_endpoint = MINEAPP_ENDPOINT;
    MacUtil.src_endpoint = 0x20;//MINEAPP_ENDPOINT;
    MacUtil.profile_id = 0x0100;//MINEAPP_PROFID;
    MacUtil.cluster_id = CARD_CLUSTERID;
    MacUtil.NodeType = NODETYPE_DEVICE;
    MAC_UTIL_INIT(&MacUtil);

    /*
    osal_memset(Hal_transInfo, 0, sizeof(Hal_transInfo_t) *HAL_AIR_HEAPLEN);

    for(uint8 i = 0; i < HAL_AIR_HEAPLEN; i++)
    {
        Hal_transInfo[i].pSend = MAC_McpsDataAlloc(MAC_MAX_FRAME_SIZE + 10, MAC_SEC_LEVEL_NONE, MAC_KEY_ID_MODE_NONE);
        if(Hal_transInfo[i].pSend == NULL)
        {
            SystemReset();
        }
    }
    */
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
void HalDriverInit ( void )
{
    /* TIMER */
#if (defined HAL_TIMER) && (HAL_TIMER == TRUE)
#error "The hal timer driver module is removed."
#endif

    /* ADC */
#if (defined HAL_ADC) && (HAL_ADC == TRUE)
    HalAdcInit();
#endif

    /* DMA */
#if (defined HAL_DMA) && (HAL_DMA == TRUE)
    // Must be called before the init call to any module that uses DMA.
    HalDmaInit();
#endif

    /* Flash */
#if (defined HAL_FLASH) && (HAL_FLASH == TRUE)
    // Must be called before the init call to any module that uses Flash access or NV.
    HalFlashInit();
#endif

	/*SPI*/
#if (defined HAL_SPI) && (HAL_SPI == TRUE)
	HalSpiInit();
#endif

	/*AUDIO*/
//#if (defined HAL_AUDIO) && (HAL_AUDIO == TRUE)
//	HalAudioInit();
//#endif

	//InitialLcd();

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

	HALSHAKEINITIAL ( false );
	BACKLIGHT_INIT;
	BACKLIGHT_OFF;

	/*Delay to fix phone bug*/
#if (defined HAL_AUDIO) && (HAL_AUDIO == TRUE)
	DelayMs ( 100 );
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
uint16 Hal_ProcessEvent ( uint8 task_id, uint16 events )
{
	uint8 *msgPtr;

	if ( events & SYS_EVENT_MSG )
	{
		msgPtr = osal_msg_receive ( Hal_TaskID );

		while ( msgPtr )
		{
			/* Do something here - for now, just deallocate the msg and move on */
			switch ( *msgPtr )
			{
			case MAC_MCPS_DATA_CNF:
			{
				macCbackEvent_t * pData = (macCbackEvent_t *) msgPtr;
				osal_msg_deallocate((uint8 *) pData->dataCnf.pDataReq);
				//hal_ProcessDataCnf ( & ( ( macCbackEvent_t * ) msgPtr )->dataCnf );				
				break;
			}
			}
			/* De-allocate */
			osal_msg_deallocate ( msgPtr );
			/* Next */
			msgPtr = osal_msg_receive ( Hal_TaskID );
		}
		return events ^ SYS_EVENT_MSG;
	}

#if ((defined HAL_AUDIO) && (HAL_AUDIO == TRUE))
	if ( events & HAL_LCD_PERSIST_EVT )
	{
		/* backlight close */
		BACKLIGHT_CLOSE();
        uint8 abnormalRst_backLightOn;
        osal_nv_read(MP_STOREPARAM_ITEM, 0, sizeof(uint8), &abnormalRst_backLightOn);
        abnormalRst_backLightOn &=0xF0;
        osal_nv_write(MP_STOREPARAM_ITEM, 0, sizeof(uint8), &abnormalRst_backLightOn);
       // StoreParam_t param = * (StoreParam_t *) MP_STOREPARAM_ADDR;
       //param.backLightOn = false;
       //* (StoreParam_t *) MP_STOREPARAM_ADDR = param;

		/* pad lock*/
#if ((defined HAL_AUDIO) && (HAL_AUDIO == TRUE))
		Menu_handle_msg ( MSG_PAD_LOCK, NULL, 0 );

		/* set key active flag */
		Hal_SetKeyActive ( FALSE );
#endif
		return events ^ HAL_LCD_PERSIST_EVT;
	}
#endif

#if (defined HAL_AUDIO) && (HAL_AUDIO == TRUE)
	if ( events & HAL_VOICEBELL_EVENT )
	{
		uint8 state=HalFillVoiceBellbuf();
		if ( state==VOICEBELL_SEND_COMPLETE )
		{
		       Hal_ProcessVoiceBellEnd();
			Hal_EndVoiceBell();
		}
		else
		{
			osal_start_timerEx ( Hal_TaskID, HAL_VOICEBELL_EVENT, 120);
		}
		return events ^ HAL_VOICEBELL_EVENT;
	}
#endif


	if ( events & HAL_RING_EVENT )
	{
		uint8 play = HalRingPlay();
        if(HalRingGetOpenFlag() == OPENFLAG_ASBELL || HalRingGetOpenFlag() == OPENFLAG_ASSMS_POW)
		{
			HalShakePlay();
		}
		if ( play!=RING_ENDED )
		{
			osal_start_timerEx ( Hal_TaskID, HAL_RING_EVENT, 15 );
		}
		else
		{
			HalProcessRingEnd();
		}
		return events ^ HAL_RING_EVENT;
	}

	if ( events & HAL_LED_BLINK_EVENT )
	{
#if (defined (BLINK_LEDS)) && (HAL_LED == TRUE)
		HalLedUpdate();
#endif /* BLINK_LEDS && HAL_LED */
		return events ^ HAL_LED_BLINK_EVENT;
	}

	if ( events & HAL_KEY_EVENT )
	{

#if (defined HAL_KEY) && (HAL_KEY == TRUE)
		/* Check for keys */
		HalKeyPoll();

		/* if interrupt disabled, do next polling */
		if ( !Hal_KeyIntEnable )
		{
			osal_start_timerEx ( Hal_TaskID, HAL_KEY_EVENT, 100 );
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

	//HalAirPoll();

	HAL_AlarmPoll();
#if defined( POWER_SAVING )
    ALLOW_SLEEP_MODE();
#endif

#if (defined WATCHDOG) &&(WATCHDOG==TRUE)
	FeedWatchDog();
#endif
}

#if ((defined HAL_AUDIO) && (HAL_AUDIO == TRUE))
void HalResetBackLightEvent ( void )
{
    //StoreParam_t param = * (StoreParam_t *) MP_STOREPARAM_ADDR;
      uint8 abnormalRst_backLightOn;
      osal_nv_read(MP_STOREPARAM_ITEM, 0, sizeof(uint8), &abnormalRst_backLightOn);

	uint8 backlight_ctl = LCDGetBackLightCtl();
	uint16 backlight_time;
	if ( backlight_ctl == BACKLIGHT_CTL_OFF )
	{
        BACKLIGHT_CLOSE();
        //param.backLightOn = false;
        abnormalRst_backLightOn&=0xF0;
	}
	else
	{
		switch ( backlight_ctl )
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

	if ( backlight_ctl == BACKLIGHT_CTL_10S ||backlight_ctl == BACKLIGHT_CTL_20S || backlight_ctl == BACKLIGHT_CTL_30S )
	{
        BACKLIGHT_OPEN();
        //param.backLightOn = true;
        abnormalRst_backLightOn |=0x0F;
		osal_start_timerEx ( Hal_TaskID, HAL_LCD_PERSIST_EVT, backlight_time );
	}
	else
	{
		backlight_time = 10000;
		osal_start_timerEx ( Hal_TaskID, HAL_LCD_PERSIST_EVT, backlight_time );
	}
   // * (StoreParam_t *) MP_STOREPARAM_ADDR = param;            
        osal_nv_write(MP_STOREPARAM_ITEM, 0, sizeof(uint8), &abnormalRst_backLightOn);
}
void Hal_SetKeyActive ( bool val )
{
	keyactive = val;
}

bool Hal_AllowSleep ( void )
{
	return ! ( keyactive || HalRingIsPlaying() || Hal_voicebell_playing);
}

#endif


uint8 Hal_SendDataToAir(uint8 *p, uint8 len, uint16 dstPan, uint16 dstaddr, bool ack, bool retrans)
{
	if ( p == NULL )
	{
		return MAC_INVALID_PARAMETER;
	}

    MacParam_t macparam;
    macparam.radius = 0x01;
    macparam.cluster_id = MINEAPP_CLUSTERID;
    macparam.panID = dstPan;
    uint8 ret;
    if(ack)
	{
        ret =  MAC_UTIL_BuildandSendDataPAN(&macparam, p, len, MAC_UTIL_UNICAST, dstaddr, MAC_TXOPTION_ACK);
	}
    else    
    {
        ret =  MAC_UTIL_BuildandSendDataPAN(&macparam, p, len, MAC_UTIL_UNICAST, dstaddr, 0);
    }
    return ret;
}


void Hal_EndVoiceBell ( void )
{
        Hal_voicebell_playing = false;
        Hal_voicebell_ringname = VOICEBELL_NULL;
        osal_unset_event(Hal_TaskID, HAL_VOICEBELL_EVENT);
	osal_stop_timerEx ( Hal_TaskID, HAL_VOICEBELL_EVENT );
	HalAudioClose();
}


void Hal_StartVoiceBell ( uint8 bell_name )
{
        Hal_voicebell_playing = true;
        Hal_voicebell_ringname = ( VoiceBellName_t ) bell_name;
	HalVoiceBellOpen();
	HalSetVoiceBellBuf ( ( VoiceBellName_t ) bell_name );

	osal_start_timerEx ( Hal_TaskID, HAL_VOICEBELL_EVENT, 120 );
	HalFillVoiceBellbuf();
}
bool Hal_IsVoiceBellPlaying(void)
{
    return Hal_voicebell_playing;
}
void Hal_RingStart ( uint8 ringname, uint8 openflag )
{
	HalRingOpen ( ( RingName ) ringname,openflag );
	osal_start_timerEx ( Hal_TaskID, HAL_RING_EVENT, 5 );

	//HAL_AlarmSet ( MP_ALARM_RING, 65000 );
}
void Hal_RingStop ( void )
{
	osal_stop_timerEx ( Hal_TaskID, HAL_RING_EVENT );
        osal_unset_event(Hal_TaskID, HAL_RING_EVENT);
}
void HalProcessRingEnd ( void )
{
	HALRING_SHAKEOFF();
	HalRingClose();

	if ( HalRingGetPlayingName() == RING_POWERON )  //  poweron music
	{
		if (MP_IsNwkOn())            
		{
		        /* already have nwk */
			Menu_handle_msg ( MSG_INIT_MAIN, NULL, 0 );
		}
              else
		{
			/* show searching nwk on menu */
			Menu_handle_msg ( MSG_INIT_NWK, NULL, 0 );
			HAL_AlarmSet ( MP_ALARM_INITNWK, MP_INIT_NWK_TIMEOUT );
		}
	}
	else if ( HalRingGetPlayingName() == RING_POWEROFF )
	{
		MP_PowerOFF();
	}else if (ON_CALLED() && HalRingGetOpenFlag() == OPENFLAG_ASBELL) //called side
	{
		Menu_handle_msg(MSG_MISSED_CALL, NULL, 0);
	}

}
void Hal_ProcessVoiceBellEnd()
{
    if(Hal_voicebell_ringname == VOICEBELL_OUTOFREACH
   || Hal_voicebell_ringname == VOICEBELL_BUSY
   || Hal_voicebell_ringname == VOICEBELL_NOBODY)
    {
        Menu_handle_msg ( MSG_INIT_MAIN, NULL, 0 );
    }
}

/**************************************************************************************************
**************************************************************************************************/

