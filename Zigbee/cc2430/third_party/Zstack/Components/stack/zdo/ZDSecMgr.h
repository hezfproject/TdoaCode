/**************************************************************************************************
  Filename:       ZDSecMgr.h
  Revised:        $Date: 2009/02/07 01:08:32 $
  Revision:       $Revision: 1.1.1.1 $

  Description:    This file contains the interface to the ZigBee Device Security Manager.


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
  PROVIDED “AS IS” WITHOUT WARRANTY OF ANY KIND, EITHER EXPRESS OR IMPLIED, 
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

#ifndef ZDSECMGR_H
#define ZDSECMGR_H

#ifdef __cplusplus
extern "C"
{
#endif

/******************************************************************************
 * INCLUDES
 */
#include "ZComDef.h"
#include "nwk_globals.h"
#include "ZDApp.h"

/******************************************************************************
 * CONSTANTS
 */
// Security Configurations
#if ( SECURE != 0 ) && defined ( SECURITY_MODE )
  #define ZDSECMGR_SECURE
  #if ( SECURITY_MODE == SECURITY_COMMERCIAL )
    #define ZDSECMGR_COMMERCIAL
  #else
    #define ZDSECMGR_RESIDENTIAL
  #endif
#endif // ( SECURE != 0 ) && defined ( SECURITY_MODE )

/******************************************************************************
 * PUBLIC FUNCTIONS
 */
/******************************************************************************
 * @fn          ZDSecMgrInit
 *
 * @brief       Initialize ZigBee Device Security Manager.
 *
 * @param       none
 *
 * @return      none
 */
extern void ZDSecMgrInit( void );

/******************************************************************************
 * @fn          ZDSecMgrConfig
 *
 * @brief       Configure ZigBee Device Security Manager.
 *
 * @param       none
 *
 * @return      none
 */
extern void ZDSecMgrConfig( void );

/******************************************************************************
 * @fn          ZDSecMgrPermitJoining
 *
 * @brief       Process request to change joining permissions.
 *
 * @param       duration - [in] timed duration for join in seconds
 *                         - 0x00 not allowed
 *                         - 0xFF allowed without timeout
 *
 * @return      uint8 - success(TRUE:FALSE)
 */
extern uint8 ZDSecMgrPermitJoining( uint8 duration );

/******************************************************************************
 * @fn          ZDSecMgrPermitJoiningTimeout
 *
 * @brief       Process permit joining timeout
 *
 * @param       none
 *
 * @return      none
 */
extern void ZDSecMgrPermitJoiningTimeout( void );

/******************************************************************************
 * @fn          ZDSecMgrNewDeviceEvent
 *
 * @brief       Process a the new device event, if found reset new device 
 *              event/timer.
 *
 * @param       none
 *
 * @return      uint8 - found(TRUE:FALSE)
 */
extern uint8 ZDSecMgrNewDeviceEvent( void );

/******************************************************************************
 * @fn          ZDSecMgrEvent
 *
 * @brief       Handle ZDO Security Manager event/timer(ZDO_SECMGR_EVENT).
 *
 * @param       none
 *
 * @return      none
 */
extern void ZDSecMgrEvent( void );

/******************************************************************************
 * @fn          ZDSecMgrEstablishKeyCfm
 *
 * @brief       Process the ZDO_EstablishKeyCfm_t message.
 *
 * @param       cfm - [in] ZDO_EstablishKeyCfm_t confirmation
 *
 * @return      none
 */
extern void ZDSecMgrEstablishKeyCfm( ZDO_EstablishKeyCfm_t* cfm );

/******************************************************************************
 * @fn          ZDSecMgrEstablishKeyInd
 *
 * @brief       Process the ZDO_EstablishKeyInd_t message.
 *
 * @param       ind - [in] ZDO_EstablishKeyInd_t indication
 *
 * @return      none
 */
extern void ZDSecMgrEstablishKeyInd( ZDO_EstablishKeyInd_t* ind );

/******************************************************************************
 * @fn          ZDSecMgrTransportKeyInd
 *
 * @brief       Process the ZDO_TransportKeyInd_t message.
 *
 * @param       ind - [in] ZDO_TransportKeyInd_t indication
 *
 * @return      none
 */
extern void ZDSecMgrTransportKeyInd( ZDO_TransportKeyInd_t* ind );

/******************************************************************************
 * @fn          ZDSecMgrUpdateDeviceInd
 *
 * @brief       Process the ZDO_UpdateDeviceInd_t message.
 *
 * @param       ind - [in] ZDO_UpdateDeviceInd_t indication
 *
 * @return      none
 */
extern void ZDSecMgrUpdateDeviceInd( ZDO_UpdateDeviceInd_t* ind );

/******************************************************************************
 * @fn          ZDSecMgrRemoveDeviceInd
 *
 * @brief       Process the ZDO_RemoveDeviceInd_t message.
 *
 * @param       ind - [in] ZDO_RemoveDeviceInd_t indication
 *
 * @return      none
 */
extern void ZDSecMgrRemoveDeviceInd( ZDO_RemoveDeviceInd_t* ind );

/******************************************************************************
 * @fn          ZDSecMgrRequestKeyInd
 *
 * @brief       Process the ZDO_RequestKeyInd_t message.
 *
 * @param       ind - [in] ZDO_RequestKeyInd_t indication
 *
 * @return      none
 */
extern void ZDSecMgrRequestKeyInd( ZDO_RequestKeyInd_t* ind );

/******************************************************************************
 * @fn          ZDSecMgrSwitchKeyInd
 *
 * @brief       Process the ZDO_SwitchKeyInd_t message.
 *
 * @param       ind - [in] ZDO_SwitchKeyInd_t indication
 *
 * @return      none
 */
extern void ZDSecMgrSwitchKeyInd( ZDO_SwitchKeyInd_t* ind );

/******************************************************************************
 * @fn          ZDSecMgrUpdateNwkKey
 *
 * @brief       Load a new NWK key and trigger a network wide update.
 *
 * @param       key       - [in] new NWK key
 * @param       keySeqNum - [in] new NWK key sequence number
 *
 * @return      ZStatus_t
 */
extern ZStatus_t ZDSecMgrUpdateNwkKey( uint8* key, uint8 keySeqNum, uint16 dstAddr);

/******************************************************************************
 * @fn          ZDSecMgrSwitchNwkKey
 *
 * @brief       Causes the NWK key to switch via a network wide command.
 *
 * @param       keySeqNum - [in] new NWK key sequence number
 *
 * @return      ZStatus_t
 */
extern ZStatus_t ZDSecMgrSwitchNwkKey( uint8 keySeqNum, uint16 dstAddr );

/******************************************************************************
******************************************************************************/

#ifdef __cplusplus
}
#endif

#endif /* ZDSECMGR_H */