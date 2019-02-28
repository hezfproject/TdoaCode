/**************************************************************************************************
  Filename:       msa.c
  Revised:        $Date: 2011/09/30 02:43:32 $
  Revision:       $Revision: 1.1.2.14 $

  Description:    This file contains the sample application that can be use to test
                  the functionality of the MAC, HAL and low level.


  Copyright 2006-2009 Texas Instruments Incorporated. All rights reserved.

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
  PROVIDED 揂S IS?WITHOUT WARRANTY OF ANY KIND, EITHER EXPRESS OR IMPLIED,
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

    Description:

                                KEY UP (or S1)
                              - Non Beacon
                              - First board in the network will be setup as the coordinator
                              - Any board after the first one will be setup as the device
                                 |
          KEY LEFT               |      KEY RIGHT(or S2)
                             ----+----- - Start transmitting
                                 |
                                 |
                                KEY DOWN



**************************************************************************************************/


/**************************************************************************************************
 *                                           Includes
 **************************************************************************************************/

/* Hal Driver includes */
#include "hal_types.h"
#include "hal_key.h"
#include "hal_timer.h"
#include "hal_drivers.h"
#include "hal_led.h"
#include "hal_adc.h"
#include "hal_lcd.h"
#include "hal_uart.h"

/* OS includes */
#include "OSAL.h"
#include "OSAL_Tasks.h"
#include "OSAL_PwrMgr.h"

/* Application Includes */
#include "OnBoard.h"
#include "crc.h"

/* MAC Application Interface */
#include "mac_api.h"
#include "mac_main.h"

/* Application */
#include "SanyCollector.h"

/* utils */
#include "macutil.h"
#include "App_cfg.h"
#include "AppProtocol.h"
#include "delay.h"
#include "OSAL_Nv.h"
#include "ZComdef.h"
#ifdef WATCHDOG
#include "WatchdogUtil.h"
#endif
/**************************************************************************************************
 *                                           Constant
 **************************************************************************************************/
typedef struct
{
    sAddrExt_t ExitAddr;
} extAddr_t;

typedef struct
{
    uint8 channel;
    uint16 shortAddr;
    extAddr_t extAddr;
} devInfo_t;


/**************************************************************************************************
 *                                           Constant
 **************************************************************************************************/

#define SANY_APP_BAUD  HAL_UART_BR_115200
#define SANY_PACK_LEN   		70
#define SANY_PACK_MAXCNT   	4
#define SANY_BUFFER_LEN   	(SANY_PACK_LEN*SANY_PACK_MAXCNT)
#define SANY_RETRY_CNT   	3
#define SANY_RAW_DATA_LEN  (SANY_BUFFER_LEN-sizeof(app_SanyCRC_t))

#define SANY_15MINUTE    180

#define HAL_LED_UART HAL_LED_2
#define HAL_LED_AIR HAL_LED_1
#define HAL_LED_RED HAL_LED_3

/* Size table for MAC structures */
const CODE uint8 sany_cbackSizeTable [] =
{
    0,                                   /* unused */
    sizeof(macMlmeAssociateInd_t),       /* MAC_MLME_ASSOCIATE_IND */
    sizeof(macMlmeAssociateCnf_t),       /* MAC_MLME_ASSOCIATE_CNF */
    sizeof(macMlmeDisassociateInd_t),    /* MAC_MLME_DISASSOCIATE_IND */
    sizeof(macMlmeDisassociateCnf_t),    /* MAC_MLME_DISASSOCIATE_CNF */
    sizeof(macMlmeBeaconNotifyInd_t),    /* MAC_MLME_BEACON_NOTIFY_IND */
    sizeof(macMlmeOrphanInd_t),          /* MAC_MLME_ORPHAN_IND */
    sizeof(macMlmeScanCnf_t),            /* MAC_MLME_SCAN_CNF */
    sizeof(macMlmeStartCnf_t),           /* MAC_MLME_START_CNF */
    sizeof(macMlmeSyncLossInd_t),        /* MAC_MLME_SYNC_LOSS_IND */
    sizeof(macMlmePollCnf_t),            /* MAC_MLME_POLL_CNF */
    sizeof(macMlmeCommStatusInd_t),      /* MAC_MLME_COMM_STATUS_IND */
    sizeof(macMcpsDataCnf_t),            /* MAC_MCPS_DATA_CNF */
    sizeof(macMcpsDataInd_t),            /* MAC_MCPS_DATA_IND */
    sizeof(macMcpsPurgeCnf_t),           /* MAC_MCPS_PURGE_CNF */
    sizeof(macEventHdr_t)                /* MAC_PWR_ON_CNF */
};


/**************************************************************************************************
 *                                        Local Variables
 **************************************************************************************************/

/* Task ID */
uint8 Sany_TaskId;


uint8 Sany_TimeCnt;

devInfo_t Sany_devinfo;

bool	Sany_ture = TRUE;
bool Sany_false = FALSE;

uint8 Sany_UartBuf[SANY_BUFFER_LEN + 10];
uint16 Sany_BufLen;
uint8 Sany_AirBuf[sizeof(app_SanyData_t)+SANY_PACK_LEN+10];

uint16 air_framecnt;
uint16 air_totalpackcnt;
uint16 air_packcnt;
uint16 air_retrycnt;
/**************************************************************************************************
 *                                     Local Function Prototypes
 **************************************************************************************************/
/* Setup routines */

void Sany_ReadDevInfo(void);
void Sany_DeviceStartup(void);
uint8 Sany_SendData(uint8* p, uint16 len, uint16 framecnt,uint8 total_packcnt, uint8 packcnt);
uint8 Sany_SendVersion(uint8* p);
/**************************************************************************************************
 *
 * @fn          MSA_Init
 *
 * @brief       Initialize the application
 *
 * @param       taskId - taskId of the task after it was added in the OSAL task queue
 *
 * @return      none
 *
 **************************************************************************************************/
void Sany_Init(uint8 taskId)
{

    /* Initialize the task id */    
    Sany_TaskId = taskId;

    /* initialize MAC features */
    MAC_InitDevice();
    MAC_InitCoord();

    /* Reset the MAC */
    MAC_MlmeResetReq(TRUE);

    /* LED*/
    HalLedSet(HAL_LED_UART, HAL_LED_MODE_OFF);
    HalLedSet(HAL_LED_AIR, HAL_LED_MODE_OFF);
    HalLedSet(HAL_LED_RED, HAL_LED_MODE_ON);


    /* initial MacUtil*/
    MacUtil_t Macutil;
    Macutil.panID = 0xFFFF;
    Macutil.dst_endpoint = APS_DST_ENDPOINT;
    Macutil.src_endpoint = APS_SRC_ENDPOINT;
    Macutil.cluster_id = APS_CLUSTER_ID;
    Macutil.profile_id = APS_PROFILE_ID;
    Macutil.NodeType =  NODETYPE_DEVICE;
    MAC_UTIL_INIT(&Macutil);

    Sany_ReadDevInfo();

    Sany_DeviceStartup();

    /*init uart*/
    halUARTCfg_t uartConfig;
    uartConfig.configured           = TRUE;              // 2430 don't care.
    uartConfig.baudRate             = HAL_UART_BR_115200;
    uartConfig.flowControl          = FALSE;
    uartConfig.flowControlThreshold = 48;
    uartConfig.rx.maxBufSize        = SANY_RAW_DATA_LEN;
    uartConfig.tx.maxBufSize        = 0;
    uartConfig.idleTimeout          = 8;  			 // 2430 don't care.
    uartConfig.intEnable            = TRUE;              // 2430 don't care.
    uartConfig.callBackFunc         = NULL;

    HalUARTOpen(HAL_UART_PORT_0, &uartConfig);

    /*Start Watch Dog*/
#if(defined WATCHDOG) && (WATCHDOG==TRUE)
    StartWatchDog(DOGTIMER_INTERVAL_1S);
    osal_start_timerEx(Sany_TaskId, SANY_UART_READ_EVENT, 10);
#endif

    //MAC_MlmeSetReq(MAC_RX_ON_WHEN_IDLE, &Sany_false);
    //while(MAC_PwrOffReq(MAC_PWR_SLEEP_DEEP) != MAC_SUCCESS);

	Sany_TimeCnt = SANY_15MINUTE;          //init Sany_TimeCnt,一上电先发送版本信息
	osal_start_timerEx(Sany_TaskId,SANY_SEND_VER_EVENT,10);
	osal_start_timerEx(Sany_TaskId, SANY_UART_READ_EVENT, 30);
}

/**************************************************************************************************
 *
 * @fn          MSA_ProcessEvent
 *
 * @brief       This routine handles events
 *
 * @param       taskId - ID of the application task when it registered with the OSAL
 *              events - Events for this task
 *
 * @return      16bit - Unprocessed events
 *
 **************************************************************************************************/
uint16 Sany_ProcessEvent(uint8 taskId, uint16 events)
{
    uint8* pMsg;
    macCbackEvent_t* pData;
    app_SanyCRC_t* pCrcData;
    

#if(defined WATCHDOG) && (WATCHDOG==TRUE)
    if (events & SANY_FEED_WATCHDOG_EVENT)
    {
    	FEEDWATCHDOG();
		if(SUCCESS!=osal_start_timerEx(Sany_TaskId, SANY_FEED_WATCHDOG_EVENT, 300))
		{
			SystemReset();
		}
    	return events ^  SANY_FEED_WATCHDOG_EVENT;
    }
#endif

    if (events & SYS_EVENT_MSG)
    {
        while ((pMsg = osal_msg_receive(Sany_TaskId)) != NULL)
        {
            switch ( *pMsg )
            {
            case MAC_MCPS_DATA_CNF:
                mac_msg_deallocate((uint8**)&pData->dataCnf.pDataReq);
                break;

            case MAC_MCPS_DATA_IND:
                break;
            }

            /* Deallocate */
            mac_msg_deallocate((uint8 **)&pMsg);
        }

        return events ^ SYS_EVENT_MSG;
    }
	if(events & SANY_SEND_VER_EVENT)
	{
		Sany_SendVersion(Sany_UartBuf);
		return events ^ SANY_SEND_VER_EVENT;
	}

    if(events & SANY_UART_READ_EVENT)
    {
        if(SUCCESS != osal_start_timerEx(Sany_TaskId, SANY_UART_READ_EVENT, 5000))
        {
            SystemReset();
        }

        Sany_BufLen = HalUARTRead(HAL_UART_PORT_0, Sany_UartBuf, SANY_RAW_DATA_LEN);
        if(Sany_BufLen > SANY_RAW_DATA_LEN)
        {
            Sany_BufLen = SANY_RAW_DATA_LEN;
        }
		pCrcData = (app_SanyCRC_t*)(Sany_UartBuf+Sany_BufLen);		
        pCrcData->data_len = Sany_BufLen;
        
        pCrcData->crc_code = CRC16(Sany_UartBuf, Sany_BufLen+2, 0xFFFF);
        Sany_BufLen += sizeof(app_SanyCRC_t);

        //if(Sany_BufLen >0)
        {
            air_framecnt++;
            air_retrycnt = 0;
            air_totalpackcnt = Sany_BufLen/SANY_PACK_LEN;
            if(Sany_BufLen > air_totalpackcnt*SANY_PACK_LEN)
            {
                air_totalpackcnt++;
            }
            air_packcnt = 0;

            //MAC_PwrOnReq(); // turn on mac
            //MAC_MlmeSetReq(MAC_RX_ON_WHEN_IDLE, &Sany_ture);
            IEN0 &=~(0x01<<2);         //UART0 RX Interrupt Disable
            IEN2 |= 0x01;              //RF Interrupt Disable
            MAC_InitDevice();
            MAC_InitCoord();
            MAC_MlmeResetReq(TRUE);
            Sany_DeviceStartup();
            
            if(SUCCESS != osal_start_timerEx(Sany_TaskId, SANY_AIR_SEND_EVENT, 10))
            {
                SystemReset();
            }
            if(Sany_TimeCnt++ >= SANY_15MINUTE)
            {
                if(SUCCESS != osal_start_timerEx(Sany_TaskId, SANY_SEND_VER_EVENT, 10))
                {
                    SystemReset();
                }
                Sany_TimeCnt = 0;
            }
            
			if(Sany_BufLen >sizeof(app_SanyCRC_t))
	        	HalLedBlink(HAL_LED_UART, 5, 50, 1000);
        }
        return events ^ SANY_UART_READ_EVENT;
    }
    if(events & SANY_AIR_SEND_EVENT)
    {
        uint16 len;
	    if(air_packcnt < air_totalpackcnt)
	    {
	        len = (air_packcnt == air_totalpackcnt-1) ?
	              (Sany_BufLen - (air_totalpackcnt-1)*SANY_PACK_LEN)  :  SANY_PACK_LEN;

	        if(air_retrycnt < 3)
	        {
	            Sany_SendData(Sany_UartBuf + air_packcnt*SANY_PACK_LEN, len, air_framecnt, air_totalpackcnt, air_packcnt);
	            air_retrycnt++;
	        }
	        else
	        {
	            air_packcnt++;
	            air_retrycnt = 0;
	        }

	        if(SUCCESS != osal_start_timerEx(Sany_TaskId, SANY_AIR_SEND_EVENT, 30))
	        {
	            SystemReset();
	        }
	    }
		else
		{
			HalLedSet(HAL_LED_AIR, HAL_LED_MODE_OFF);
            osal_start_timerEx(Sany_TaskId,SANY_SEND_END_EVENT,30);
		}
        return events ^ SANY_AIR_SEND_EVENT;
    }

    if(events & SANY_SEND_END_EVENT)
    {
        //MAC_MlmeSetReq(MAC_RX_ON_WHEN_IDLE, &Sany_false);
        //MAC_PwrOffReq(MAC_PWR_SLEEP_LITE);
        IEN2 &=~0x01;              //RF Interrupt Disable
        IEN0 |= (0x01<<2);         //UART0 RX Interrupt Enable
        return events ^ SANY_SEND_END_EVENT;
    }
    return 0;
}

/**************************************************************************************************
 *
 * @fn          MAC_CbackEvent
 *
 * @brief       This callback function sends MAC events to the application.
 *              The application must implement this function.  A typical
 *              implementation of this function would allocate an OSAL message,
 *              copy the event parameters to the message, and send the message
 *              to the application's OSAL event handler.  This function may be
 *              executed from task or interrupt context and therefore must
 *              be reentrant.
 *
 * @param       pData - Pointer to parameters structure.
 *
 * @return      None.
 *
 **************************************************************************************************/
void MAC_CbackEvent(macCbackEvent_t *pData)
{

    macCbackEvent_t *pMsg = NULL;

    uint8 len = sany_cbackSizeTable[pData->hdr.event];

    switch (pData->hdr.event)
    {
    case MAC_MLME_BEACON_NOTIFY_IND:

        len += sizeof(macPanDesc_t) + pData->beaconNotifyInd.sduLength +
               MAC_PEND_FIELDS_LEN(pData->beaconNotifyInd.pendAddrSpec);
        if ((pMsg = (macCbackEvent_t *) osal_msg_allocate(len)) != NULL)
        {
            /* Copy data over and pass them up */
            osal_memcpy(pMsg, pData, sizeof(macMlmeBeaconNotifyInd_t));
            pMsg->beaconNotifyInd.pPanDesc = (macPanDesc_t *) ((uint8 *) pMsg + sizeof(macMlmeBeaconNotifyInd_t));
            osal_memcpy(pMsg->beaconNotifyInd.pPanDesc, pData->beaconNotifyInd.pPanDesc, sizeof(macPanDesc_t));
            pMsg->beaconNotifyInd.pSdu = (uint8 *) (pMsg->beaconNotifyInd.pPanDesc + 1);
            osal_memcpy(pMsg->beaconNotifyInd.pSdu, pData->beaconNotifyInd.pSdu, pData->beaconNotifyInd.sduLength);
        }
        break;

    case MAC_MCPS_DATA_IND:
        pMsg = pData;
        break;

    default:
        if ((pMsg = (macCbackEvent_t *) osal_msg_allocate(len)) != NULL)
        {
            osal_memcpy(pMsg, pData, len);
        }
        break;
    }

    if (pMsg != NULL)
    {
        osal_msg_send(Sany_TaskId, (uint8 *) pMsg);
    }
}

/**************************************************************************************************
 *
 * @fn      MAC_CbackCheckPending
 *
 * @brief   Returns the number of indirect messages pending in the application
 *
 * @param   None
 *
 * @return  Number of indirect messages in the application
 *
 **************************************************************************************************/
uint8 MAC_CbackCheckPending(void)
{
    return (0);
}

/**************************************************************************************************
 *
 * @fn      MSA_DeviceStartup()
 *
 * @brief   Update the timer per tick
 *
 * @param   beaconEnable: TRUE/FALSE
 *
 * @return  None
 *
 **************************************************************************************************/
void Sany_DeviceStartup()
{
    MAC_MlmeSetReq(MAC_EXTENDED_ADDRESS, &Sany_devinfo.extAddr.ExitAddr);

    /* Setup PAN ID */
    uint16 panid = SANY_NWK_ADDR;
    MAC_MlmeSetReq(MAC_PAN_ID, &panid);

    /* This device is setup for Direct Message */
    bool rx_on_idle = true;
    MAC_MlmeSetReq(MAC_RX_ON_WHEN_IDLE, &rx_on_idle);

    /* Setup Coordinator short address */
    uint16 coorshortAddr = 0;
    MAC_MlmeSetReq(MAC_COORD_SHORT_ADDRESS, &coorshortAddr);

    /* Setup Beacon Order */
    uint8 BeaconOrder  = 15;
    MAC_MlmeSetReq(MAC_BEACON_ORDER, &BeaconOrder);

    /* Setup Super Frame Order */
    uint8 SuperFrameOrder  = 15;
    MAC_MlmeSetReq(MAC_SUPERFRAME_ORDER, &SuperFrameOrder);

    MAC_MlmeSetReq(MAC_LOGICAL_CHANNEL, &Sany_devinfo.channel);

    MAC_MlmeSetReq(MAC_SHORT_ADDRESS, &Sany_devinfo.shortAddr);

    MAC_MlmeSetReq(MAC_ASSOCIATED_PAN_COORD, &Sany_ture);

}

void Sany_ReadDevInfo(void)
{
    //extAddr_t *p = (extAddr_t *)(SANY_DEVINFO_ADDR);

    osal_nv_item_init(ZCD_NV_EXTADDR, Z_EXTADDR_LEN,NULL);
    osal_nv_read(ZCD_NV_EXTADDR, 0, Z_EXTADDR_LEN, Sany_devinfo.extAddr.ExitAddr);

    Sany_devinfo.shortAddr = BUILD_UINT16(Sany_devinfo.extAddr.ExitAddr[EXT_MACADDR_DEVID_LBYTE],
                                          Sany_devinfo.extAddr.ExitAddr[EXT_MACADDR_DEVID_HBYTE]);
    Sany_devinfo.channel = Sany_devinfo.extAddr.ExitAddr[EXT_MACADDR_CHANNEL];


    /* if wrong address */
    if(Sany_devinfo.channel <11 || Sany_devinfo.channel > 26 || Sany_devinfo.shortAddr==0 || Sany_devinfo.shortAddr==0xFFFF)
    {
        while(1)
        {
			HalLedSet(HAL_LED_ALL, HAL_LED_MODE_ON);
            
#if(defined WATCHDOG) && (WATCHDOG==TRUE)
			FEEDWATCHDOG();
#endif
			DelayMs(500);
			HalLedSet(HAL_LED_ALL, HAL_LED_MODE_OFF);
#if(defined WATCHDOG) && (WATCHDOG==TRUE)
			FEEDWATCHDOG();
#endif
			DelayMs(500);
        }
    }
}

uint8 Sany_SendData(uint8* p, uint16 len, uint16 framecnt,uint8 total_packcnt, uint8 packcnt)
{
    if(p==NULL || len > SANY_PACK_LEN)
    {
        return 0;
    }

    HalLedSet(HAL_LED_AIR, HAL_LED_MODE_TOGGLE);

    app_SanyData_t *pHeader = (app_SanyData_t *)Sany_AirBuf;

    pHeader->msgtype = SANY_DATA;
    pHeader->srcAddr = Sany_devinfo.shortAddr;
    pHeader->dstAddr = 0xFFFF;
    pHeader->seqnum = framecnt;
    pHeader->packnum = total_packcnt;
    pHeader->packseq = packcnt;
    pHeader->len = len;

    osal_memcpy((uint8*)(pHeader+1), p, len);

    MacParam_t param;
    param.panID = 0xFFFF;
    param.cluster_id = APS_CLUSTER_ID;
    param.radius = 0x01;

    return  MAC_UTIL_BuildandSendDataPAN(&param, Sany_AirBuf,  sizeof(app_SanyData_t)+pHeader->len,  MAC_UTIL_UNICAST,  0,  MAC_TXOPTION_NO_RETRANS);
}

uint8 Sany_SendVersion(uint8* p)
{
    if(p==NULL)
    {
        return 0;
    }

    HalLedSet(HAL_LED_AIR, HAL_LED_MODE_TOGGLE);

    app_SanyStatus_t *pHeader = (app_SanyStatus_t *)Sany_AirBuf;

    pHeader->msgtype = SANY_STATUS;
	pHeader->statustype = APP_SANY_STATUS_VERSION;
    pHeader->srcAddr = Sany_devinfo.shortAddr;
    pHeader->dstAddr = 0xFFFF;
    pHeader->len = sizeof(SANY_VERSION);
    if(pHeader->len > SANY_PACK_LEN)
    {
        return 0;
    }

    osal_memcpy((uint8*)(pHeader+1), SANY_VERSION, pHeader->len);

    MacParam_t param;
    param.panID = 0xFFFF;
    param.cluster_id = APS_CLUSTER_ID;
    param.radius = 0x01;

    return  MAC_UTIL_BuildandSendDataPAN(&param, Sany_AirBuf,  sizeof(app_SanyStatus_t)+pHeader->len,  MAC_UTIL_UNICAST,  0,  MAC_TXOPTION_NO_RETRANS);
}


/**************************************************************************************************
 **************************************************************************************************/
