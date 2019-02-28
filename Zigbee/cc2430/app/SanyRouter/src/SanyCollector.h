/**************************************************************************************************
  Filename:       msa.h
  Revised:        $Date: 2011/07/18 22:03:49 $
  Revision:       $Revision: 1.1.2.5 $

  Description:    This file contains the the Mac Sample Application protypes and definitions


  Copyright 2006-2007 Texas Instruments Incorporated. All rights reserved.

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

#ifndef SANYCOLLECTOR_H
#define SANYCOLLECTOR_H

#ifdef __cplusplus
extern "C"
{
#endif

/**************************************************************************************************
 * INCLUDES
 **************************************************************************************************/
#include "hal_types.h"

/**************************************************************************************************
 *                                        User's  Defines
 **************************************************************************************************/


/**************************************************************************************************
 * CONSTANTS
 **************************************************************************************************/
#define APS_DST_ENDPOINT        MINEAPP_ENDPOINT 
#define APS_SRC_ENDPOINT        0x20  
#define APS_CLUSTER_ID          GASMONITOR_CLUSTERID
#define APS_PROFILE_ID          MINEAPP_APS_PROFID 


#define SANY_SEND_VERSION_EVENT_PERIOD      60000
#define SANY_SEND_DATA_EVENT_INTERVAL       30


/**************************************************************************************************
 * GLOBALS
 **************************************************************************************************/
extern uint8 Sany_UartBuf[];
extern uint16 Sany_BufLen;

/*********************************************************************
 * FUNCTIONS
 */

/*
 * Task Initialization for the Mac Sample Application
 */
extern void Sany_Init(uint16 shortAddr);
extern uint8 Sany_SendData(uint8* p, uint16 len, uint16 framecnt,uint8 total_packcnt, uint8 packcnt);
extern uint8 Sany_SendVersion(uint8* p);
extern void Sany_SendVersionEvent();
extern uint8 Sany_SendDataEvent();
extern void Sany_PrepSend();
extern uint8 Sany_ReadData();


/*********************************************************************
*********************************************************************/

#ifdef __cplusplus
}
#endif

#endif /* MACSAMPLEAPP_H */
