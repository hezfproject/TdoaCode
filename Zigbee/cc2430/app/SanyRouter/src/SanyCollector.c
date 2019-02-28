/**************************************************************************************************
  Filename:       msa.c
  Revised:        $Date: 2011/08/10 23:30:03 $
  Revision:       $Revision: 1.1.2.11 $

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

#define SANY_APP_BAUD           HAL_UART_BR_115200
#define SANY_PACK_LEN           69
#define SANY_PACK_MAXCNT        4
#define SANY_BUFFER_LEN         (SANY_PACK_LEN*SANY_PACK_MAXCNT)
#define SANY_RETRY_CNT          3
#define SANY_RAW_DATA_LEN       (SANY_BUFFER_LEN-sizeof(app_SanyCRC_t))



/**************************************************************************************************
 *                                        Local Variables
 **************************************************************************************************/
uint16 Sany_ShortAddr;
uint8 Sany_TimeCnt;

uint8 Sany_UartBuf[SANY_BUFFER_LEN + 10];
uint16 Sany_BufLen;
uint8 Sany_AirBuf[sizeof(app_SanyData_t)+SANY_PACK_LEN+10];

uint16 air_framecnt;
uint8 air_totalpackcnt;
uint8 air_packcnt;
uint8 air_retrycnt;
/**************************************************************************************************
 *                                     Local Function Prototypes
 **************************************************************************************************/
/* Setup routines */


void Sany_Init(uint16 shortAddr)
{
    Sany_ShortAddr = shortAddr;

    /* LED*/
    /*HalLedSet(HAL_LED_UART, HAL_LED_MODE_OFF);
    HalLedSet(HAL_LED_AIR, HAL_LED_MODE_OFF);
    HalLedSet(HAL_LED_RED, HAL_LED_MODE_ON);*/



#ifdef LL_NODEROLE_REPORTNODE
    /* initial MacUtil*/
    MacUtil_t Macutil;
    Macutil.panID = 0xFFFF;
    Macutil.dst_endpoint = APS_DST_ENDPOINT;
    Macutil.src_endpoint = APS_SRC_ENDPOINT;
    Macutil.cluster_id = APS_CLUSTER_ID;
    Macutil.profile_id = APS_PROFILE_ID;
    Macutil.NodeType =  NODETYPE_DEVICE;
    MAC_UTIL_INIT(&Macutil);
#endif

#ifdef LL_NODEROLE_COLLECTNODE

    HalLedSet(HAL_LED_3, HAL_LED_MODE_ON);
    /*init uart*/
    halUARTCfg_t uartConfig;
    uartConfig.configured           = TRUE;              // 2430 don't care.
    uartConfig.baudRate             = HAL_UART_BR_115200;
    uartConfig.flowControl          = FALSE;
    uartConfig.flowControlThreshold = 48;
    uartConfig.rx.maxBufSize        = SANY_RAW_DATA_LEN;
    uartConfig.tx.maxBufSize        = 0;
    uartConfig.idleTimeout          = 8;             // 2430 don't care.
    uartConfig.intEnable            = TRUE;              // 2430 don't care.
    uartConfig.callBackFunc         = NULL;

    HalUARTOpen(HAL_UART_PORT_0, &uartConfig);
#endif

    Sany_TimeCnt = 15;
}

void Sany_SendVersionEvent()
{
  if(Sany_TimeCnt++ >= 15)
  {
    Sany_TimeCnt = 0;
    Sany_SendVersion(Sany_UartBuf);
  }
}


uint8 Sany_SendDataEvent()
{
  if(air_packcnt < air_totalpackcnt){
    uint16 len = (air_packcnt == air_totalpackcnt-1) ?
                 (Sany_BufLen - (air_totalpackcnt-1)*SANY_PACK_LEN)  :  SANY_PACK_LEN;

    if(air_retrycnt < 3){
      Sany_SendData(Sany_UartBuf + air_packcnt*SANY_PACK_LEN, len, air_framecnt, air_totalpackcnt, air_packcnt);
      air_retrycnt++;
    }else{
      air_packcnt++;
      air_retrycnt = 0;
    }
  }

  return (air_packcnt < air_totalpackcnt) ? 1 : 0;
}


#ifdef SIMULATE_COLLECT
static uint8 realdata[] = { \
  0x1B, 0x57, 0x04, 0x4C, 0x00, 0x28, 0x00, 0x00, 0xA0, 0x00, \
  0xA0, 0x01, 0xA0, 0x01, 0x00, 0x00, 0x00, 0x00, 0xB6, 0x1C, \
  0x10, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, \
  0x00, 0x00, 0x00, 0x00, 0x80, 0x7B, 0x80, 0x81, 0x80, 0x39, \
  0x80, 0x7D, 0x80, 0x7F, 0x80, 0x39, 0x80, 0x48, 0x00, 0x00, \
  0x00, 0x00, 0x00, 0x00, 0x00, 0x0C, 0x00, 0x00, 0x00, 0x00, \
  0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x90, 0x90, \
  0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x14, 0x1B, \
  0x52, 0x04, 0x7E, 0x00, 0x03, 0xF2 \
};
#endif
uint8 Sany_ReadData()
{
#ifndef SIMULATE_COLLECT
  Sany_BufLen = HalUARTRead(HAL_UART_PORT_0, Sany_UartBuf, SANY_RAW_DATA_LEN);
#else
  Sany_BufLen = SANY_RAW_DATA_LEN;
  uint16 idx = osal_rand() % 86;
  for(uint16 i = 0; i < Sany_BufLen; ++i, ++idx){
    if(idx >= 86) idx = 0;
    Sany_UartBuf[i] = realdata[idx];
  }
  DelayMs(20);
#endif

  if(Sany_BufLen > SANY_RAW_DATA_LEN)
  {
    Sany_BufLen = SANY_RAW_DATA_LEN;
  }
  app_SanyCRC_t *pCrcData = (app_SanyCRC_t*)(Sany_UartBuf+Sany_BufLen);
  pCrcData->data_len = Sany_BufLen;

  pCrcData->crc_code = CRC16(Sany_UartBuf, Sany_BufLen+2, 0xFFFF);
  Sany_BufLen += sizeof(app_SanyCRC_t);

  //IEN0 &=~(0x01<<2);
  //IEN2 |= 0x01;
  //MAC_InitDevice();
  //MAC_InitCoord();
  //MAC_MlmeResetReq(TRUE);
  //LL_SetupDevInfo();
  //LL_DeviceStartup();

  return (Sany_BufLen > sizeof(app_SanyCRC_t)) ? 1 : 0;
}

void Sany_PrepSend()
{
  air_framecnt++;
  air_retrycnt = 0;
  air_totalpackcnt = Sany_BufLen/SANY_PACK_LEN;
  if(Sany_BufLen > air_totalpackcnt*SANY_PACK_LEN)
  {
    air_totalpackcnt++;
  }
  air_packcnt = 0;
}

uint8 Sany_SendData(uint8* p, uint16 len, uint16 framecnt,uint8 total_packcnt, uint8 packcnt)
{
    if(p==NULL || len > SANY_PACK_LEN)
    {
        return 0;
    }

    //HalLedSet(HAL_LED_AIR, HAL_LED_MODE_TOGGLE);

    app_SanyData_t *pHeader = (app_SanyData_t *)Sany_AirBuf;

    pHeader->msgtype = SANY_DATA;
    pHeader->srcAddr = Sany_ShortAddr;
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

    //HalLedSet(HAL_LED_AIR, HAL_LED_MODE_TOGGLE);

    app_SanyStatus_t *pHeader = (app_SanyStatus_t *)Sany_AirBuf;

    pHeader->msgtype = SANY_STATUS;
    pHeader->statustype = APP_SANY_STATUS_VERSION;
    pHeader->srcAddr = Sany_ShortAddr;
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
