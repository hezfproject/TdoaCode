/**************************************************************************************************
  Filename:       nwk_globals.h
  Revised:        $Date: 2009/02/07 01:08:32 $
  Revision:       $Revision: 1.1.1.1 $

  Description:    User definable Z-Stack parameters.


  Copyright 2007 Texas Instruments Incorporated. All rights reserved.

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

#ifndef ZGLOBALS_H
#define ZGLOBALS_H

#ifdef __cplusplus
extern "C" {
#endif

/*********************************************************************
 * INCLUDES
 */

/*********************************************************************
 * MACROS
 */

/*********************************************************************
 * CONSTANTS
 */
  
// Values for ZCD_NV_LOGICAL_TYPE (zgDeviceLogicalType)
#define ZG_DEVICETYPE_COORDINATOR      0x00
#define ZG_DEVICETYPE_ROUTER           0x01
#define ZG_DEVICETYPE_ENDDEVICE        0x02
#define ZG_DEVICETYPE_SOFT             0x03

// Transmission retries numbers
#if !defined ( MAX_POLL_FAILURE_RETRIES )
  #define MAX_POLL_FAILURE_RETRIES 1
#endif
#if !defined ( MAX_DATA_RETRIES )
  #define MAX_DATA_RETRIES         2
#endif
  
// NIB parameters
#if !defined ( MAX_BCAST_RETRIES )
  #define MAX_BCAST_RETRIES        2
#endif
#if !defined ( PASSIVE_ACK_TIMEOUT )
  #define PASSIVE_ACK_TIMEOUT      5
#endif
#if !defined ( BCAST_DELIVERY_TIME )
  #define BCAST_DELIVERY_TIME      30
#endif
 
#if !defined ( APS_DEFAULT_MAXBINDING_TIME )
  #define APS_DEFAULT_MAXBINDING_TIME 16000
#endif
  
// Device Logical Type
#if defined ( SOFT_START )
  #define DEVICE_LOGICAL_TYPE ZG_DEVICETYPE_SOFT
#elif defined( ZDO_COORDINATOR )
  #define DEVICE_LOGICAL_TYPE ZG_DEVICETYPE_COORDINATOR
#elif defined (RTR_NWK)
  #define DEVICE_LOGICAL_TYPE ZG_DEVICETYPE_ROUTER
#else
  #define DEVICE_LOGICAL_TYPE ZG_DEVICETYPE_ENDDEVICE
#endif

// Concentrator values
#if !defined ( CONCENTRATOR_ENABLE )
  #define CONCENTRATOR_ENABLE          0
#endif

#if !defined ( CONCENTRATOR_DISCOVERY_TIME )
  #define CONCENTRATOR_DISCOVERY_TIME  0
#endif
  
#if !defined ( CONCENTRATOR_RADIUS )
  #define CONCENTRATOR_RADIUS          0x0a
#endif

#if !defined ( MAX_SOURCE_ROUTE )
  #define MAX_SOURCE_ROUTE             0x0c
#endif  

#if !defined ( START_DELAY )
  #define START_DELAY                  0x0a
#endif

#if !defined ( SAPI_ENDPOINT )
  #define SAPI_ENDPOINT                0xe0
#endif

#define ZG_STARTUP_CLEAR               0x00
#define ZG_STARTUP_SET                 0xFF
  
/*********************************************************************
 * TYPEDEFS
 */

/*********************************************************************
 * NWK GLOBAL VARIABLES
 */

extern uint16 zgPollRate;
extern uint16 zgQueuedPollRate;
extern uint16 zgResponsePollRate;
extern uint16 zgRejoinPollRate;

// Variables for number of transmission retries
extern uint8 zgMaxDataRetries;
extern uint8 zgMaxPollFailureRetries;

extern uint32 zgDefaultChannelList;
extern uint8  zgDefaultStartingScanDuration;

extern uint8 zgStackProfile;

extern uint8 zgIndirectMsgTimeout;
extern uint8 zgSecurityLevel;
extern uint8 zgRouteExpiryTime;

extern uint8 zgExtendedPANID[];

extern uint8 zgMaxBcastRetires;
extern uint8 zgPassiveAckTimeout;
extern uint8 zgBcastDeliveryTime;

extern uint8 zgNwkMode;

extern uint8 zgConcentratorEnable;
extern uint8 zgConcentratorDiscoveryTime;
extern uint8 zgConcentratorRadius;
extern uint8 zgMaxSourceRoute;

/*********************************************************************
 * APS GLOBAL VARIABLES
 */

extern uint8  zgApscMaxFrameRetries;
extern uint16 zgApscAckWaitDurationPolled;
extern uint8  zgApsAckWaitMultiplier;
extern uint16 zgApsDefaultMaxBindingTime;


/*********************************************************************
 * SECURITY GLOBAL VARIABLES
 */

extern uint8 zgPreConfigKey[];
extern uint8 zgPreConfigKeys;


/*********************************************************************
 * ZDO GLOBAL VARIABLES
 */

extern uint16 zgConfigPANID;
extern uint8  zgDeviceLogicalType;


/*********************************************************************
 * FUNCTIONS
 */

/*
 * Initialize the Z-Stack Globals.
 */
extern uint8 zgInit( void );


/*
 * Get Startup Options (ZCD_NV_STARTUP_OPTION NV Item)
 */
extern uint8 zgReadStartupOptions( void );

/*
 * Write Startup Options (ZCD_NV_STARTUP_OPTION NV Item)
 *
 *      action - ZG_STARTUP_SET set bit, ZG_STARTUP_CLEAR to clear bit.
 *               The set bit is an OR operation, and the clear bit is an
 *               AND ~(bitOptions) operation.
 *      bitOptions - which bits to perform action on:
 *                      ZCD_STARTOPT_DEFAULT_CONFIG_STATE
 *                      ZDC_STARTOPT_DEFAULT_NETWORK_STATE
 *
 * Returns - ZSUCCESS if successful   
 */
extern uint8 zgWriteStartupOptions( uint8 action, uint8 bitOptions );

/*********************************************************************
*********************************************************************/
#ifdef __cplusplus
}
#endif

#endif /* ZGLOBALS_H */
