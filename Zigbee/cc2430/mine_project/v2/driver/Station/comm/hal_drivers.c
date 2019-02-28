/**************************************************************************************************
  Filename:       hal_drivers.c
  Revised:        $Date: 2010/11/20 23:46:14 $
  Revision:       $Revision: 1.2 $

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
#if (defined WATCHDOG) &&(WATCHDOG==TRUE)
#include "WatchDogUtil.h"
#endif

#include "OnBoard.h"
#include "mac_api.h"
#include "app_protocol.h"
/**************************************************************************************************
*                                            MACROS
**************************************************************************************************/

/**************************************************************************************************
*                                          CONSTANTS
**************************************************************************************************/

#define HAL_AIR_RETRANS_TIME  3
#define HAL_AIR_HEAPLEN 5

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
}Hal_transInfo_t;


/**************************************************************************************************
*                                      GLOBAL VARIABLES
**************************************************************************************************/
uint8 Hal_TaskID;

extern void HalLedUpdate( void ); /* Notes: This for internal only so it shouldn't be in hal_led.h */
static Hal_transInfo_t  Hal_transInfo[HAL_AIR_HEAPLEN]; 

/**************************************************************************************************
*                                      FUNCTIONS - API
**************************************************************************************************/

static uint8 hal_ProcessDataCnf(macMcpsDataCnf_t *pCnf);
static int8 Hal_HeapFindFirstAvail(Hal_transInfo_t heap[]);
static uint8  HalAirPoll(void);
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

	osal_memset(Hal_transInfo, 0, sizeof(Hal_transInfo_t)*HAL_AIR_HEAPLEN);
	for(uint8 i=0;i< HAL_AIR_HEAPLEN; i++)
	{
		Hal_transInfo[i].pSend = MAC_McpsDataAlloc(MAC_MAX_FRAME_SIZE + 10, MAC_SEC_LEVEL_NONE, MAC_KEY_ID_MODE_NONE);
		if(Hal_transInfo[i].pSend == NULL)
		{
			SystemReset();
		}
	}
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
			switch ( *msgPtr )
			{
			case MAC_MCPS_DATA_CNF:
				{
					hal_ProcessDataCnf(&((macCbackEvent_t *) msgPtr)->dataCnf);
					break;
				}
			}
			/* De-allocate */
			osal_msg_deallocate( msgPtr );
			/* Next */
			msgPtr = osal_msg_receive( Hal_TaskID );
		}
		return events ^ SYS_EVENT_MSG;
	}

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

	HalAirPoll();
#if (defined WATCHDOG) &&(WATCHDOG==TRUE)
	FeedWatchDog();
#endif
}

uint8 Hal_SendDataToAir(const uint8 *p, uint8 len, uint16 dstPan, uint16 dstaddr, uint8 msgtype, bool retrans)
{
	if(p == NULL)
	{
		return MAC_INVALID_PARAMETER;
	}

	if(len > MAC_MAX_FRAME_SIZE - sizeof(app_header_t))
	{
		len = MAC_MAX_FRAME_SIZE - sizeof(app_header_t);
	}

	/* find a available sending buf */
	int8 heapIdx = 0;
	if((heapIdx = Hal_HeapFindFirstAvail(Hal_transInfo) ) < 0)
	{
		return MAC_DENIED;
	}

	macMcpsDataReq_t  *pData;	

	/* fill send infos */
	Hal_transInfo[heapIdx].IsNeedRetrans = retrans;
	Hal_transInfo[heapIdx].IsUsing= true;
	Hal_transInfo[heapIdx].IsSending = false;
	Hal_transInfo[heapIdx].retransCnt = 0;
	Hal_transInfo[heapIdx].SendTimeTick = osal_GetSystemClock(); // + (uint32)(time & 0x0F); //first send

	/* fill pData */
	pData = Hal_transInfo[heapIdx].pSend;
	pData->mac.srcAddrMode = SADDR_MODE_SHORT;
	pData->mac.dstAddr.addrMode = SADDR_MODE_SHORT;
	pData->mac.dstAddr.addr.shortAddr = dstaddr;
	pData->mac.dstPanId = dstPan;
	pData->mac.msduHandle = heapIdx;  //use heapIdx as machandle
	pData->mac.txOptions =  MAC_TXOPTION_ACK;  //always  need mac ack
	pData->sec.securityLevel = FALSE;

	app_header_t app_header;
	app_header.protocoltype = APP_PROTOCOL_TYPE_MOBILE;
	app_header.msgtype  = msgtype;
	app_header.len =  len;

	/* app_header */
	osal_memcpy (pData->msdu.p, (void *)&app_header, sizeof(app_header));
	osal_memcpy (pData->msdu.p +  sizeof(app_header), (void *)p, len);
	pData->msdu.len = sizeof(app_header) + len;

	return MAC_SUCCESS;
}
int8 Hal_HeapFindFirstAvail(Hal_transInfo_t heap[])
{
	for(int8 i=0;i<HAL_AIR_HEAPLEN;i++)
	{
		if(heap[i].IsUsing == false)
		{
			return i;
		}
	}
	return -1;
}

/* always safe to call this function in poll */
uint8  HalAirPoll(void)
{
	uint32 timeTick = osal_GetSystemClock();
	for(uint8 i=0; i< HAL_AIR_HEAPLEN; i++)
	{
		if(Hal_transInfo[i].IsUsing == true && Hal_transInfo[i].IsSending== false && timeTick > Hal_transInfo[i].SendTimeTick)
		{
			Hal_transInfo[i].IsSending = true;
			MAC_McpsDataReq(Hal_transInfo[i].pSend);
			break;  
		}
	}

	for(uint8 i=0; i< HAL_AIR_HEAPLEN; i++)  // send timeout 3s
	{
		if(Hal_transInfo[i].IsUsing == true && Hal_transInfo[i].IsSending== true
			&& timeTick>Hal_transInfo[i].SendTimeTick  && timeTick - Hal_transInfo[i].SendTimeTick > 3000)
		{
			Hal_transInfo[i].IsUsing= false;
			break;   
		}
	}
	return MAC_SUCCESS;
}

uint8  hal_ProcessDataCnf(macMcpsDataCnf_t *pCnf)
{	
	uint8 Idx = pCnf->msduHandle;
	if(Hal_transInfo[Idx].IsUsing == true && Hal_transInfo[Idx].IsSending== true)
	{
		Hal_transInfo[Idx].IsSending = false;

		/* send success or do not need retrans or retrans time overflow */
		if(pCnf->hdr.status == MAC_SUCCESS || Hal_transInfo[Idx].IsNeedRetrans == false || Hal_transInfo[Idx].retransCnt >= HAL_AIR_RETRANS_TIME)  
		{
			/* avail to send new one */	
			Hal_transInfo[Idx].IsUsing = false;
		}
		else // resend after a rand time  0-255 
		{
			{
				uint8 randtime = MAC_RandomByte();
				uint32 retransTime = osal_GetSystemClock() + (uint32)(randtime);

				Hal_transInfo[Idx].retransCnt++;
				Hal_transInfo[Idx].SendTimeTick = retransTime;
			}
		}
	}
	return MAC_SUCCESS;
}


/**************************************************************************************************
**************************************************************************************************/

