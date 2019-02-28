/**************************************************************************************************
  Filename:       nwk_globals.h
  Revised:        $Date: 2011/08/11 01:57:22 $
  Revision:       $Revision: 1.8 $

  Description:    User definable Network Parameters.


  Copyright 2004-2007 Texas Instruments Incorporated. All rights reserved.

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

#ifndef NWK_GLOBALS_H
#define NWK_GLOBALS_H

#ifdef __cplusplus
extern "C" {
#endif

/*********************************************************************
 * INCLUDES
 */

#include "ZComDef.h"
#include "nwk_bufs.h"
#include "AssocList.h"
#include "BindingTable.h"

/*********************************************************************
 * MACROS
 */

/*********************************************************************
 * CONSTANTS
 */

// Controls the operational mode of network
#define NWK_MODE_STAR         0
#define NWK_MODE_TREE         1
#define NWK_MODE_MESH         2

// Controls the security mode of network
#define SECURITY_RESIDENTIAL  0
#define SECURITY_COMMERCIAL   1

// Controls various stack parameter settings
#define NETWORK_SPECIFIC      0
#define HOME_CONTROLS         1
#define ZIGBEEPRO_PROFILE     2
#define GENERIC_STAR          3
#define GENERIC_TREE          4
#define GENERIC_MINE          5
#define MINE_CONTROLS       6
#define MINE_RTR_TEST       7

#ifndef STACK_PROFILE_ID
//#define STACK_PROFILE_ID      HOME_CONTROLS
//#define STACK_PROFILE_ID      NETWORK_SPECIFIC
//#define STACK_PROFILE_ID      MINE_RTR_TEST
#define STACK_PROFILE_ID      MINE_CONTROLS
#endif

#if ( STACK_PROFILE_ID == HOME_CONTROLS )
    #define MAX_NODE_DEPTH      5
    #define NWK_MODE            NWK_MODE_MESH
    #define SECURITY_MODE       SECURITY_RESIDENTIAL
    #define MAX_CHLDRN_CNT  20
    #define MAX_RTRS_CNT  6
#if   ( SECURE != 0  )
    #define USE_NWK_SECURITY    1   // true or false
    #define SECURITY_LEVEL      5
#else
    #define USE_NWK_SECURITY    0   // true or false
    #define SECURITY_LEVEL      0
#endif

#elif ( STACK_PROFILE_ID == GENERIC_STAR )
    #define MAX_NODE_DEPTH      5
    #define NWK_MODE            NWK_MODE_STAR
    #define SECURITY_MODE       SECURITY_RESIDENTIAL
    #define MAX_CHLDRN_CNT  5
    #define MAX_RTRS_CNT  5
#if   ( SECURE != 0  )
    #define USE_NWK_SECURITY    1   // true or false
    #define SECURITY_LEVEL      5
#else
    #define USE_NWK_SECURITY    0   // true or false
    #define SECURITY_LEVEL      0
#endif

#elif ( STACK_PROFILE_ID == NETWORK_SPECIFIC )
// define your own stack profile settings
    #define MAX_NODE_DEPTH          5
    #define NWK_MODE            NWK_MODE_MESH
    #define SECURITY_MODE       SECURITY_RESIDENTIAL
    #define MAX_CHLDRN_CNT  5
    #define MAX_RTRS_CNT  5
#if   ( SECURE != 0  )
    #define USE_NWK_SECURITY    1   // true or false
    #define SECURITY_LEVEL      5
#else
    #define USE_NWK_SECURITY    0   // true or false
    #define SECURITY_LEVEL      0
#endif
#elif ( STACK_PROFILE_ID == GENERIC_MINE )
    #define MAX_NODE_DEPTH          9
    #define NWK_MODE            NWK_MODE_TREE
    #define SECURITY_MODE       SECURITY_RESIDENTIAL
    #define MAX_CHLDRN_CNT  1
    #define MAX_RTRS_CNT  1
#if   ( SECURE != 0  )
    #define USE_NWK_SECURITY    1   // true or false
    #define SECURITY_LEVEL      5
#else
    #define USE_NWK_SECURITY    0   // true or false
    #define SECURITY_LEVEL      0
#endif
#elif ( STACK_PROFILE_ID == MINE_CONTROLS)
    #define MAX_NODE_DEPTH          1
    #define NWK_MODE            NWK_MODE_STAR
    #define SECURITY_MODE       SECURITY_RESIDENTIAL
    #if(defined GASMONITOR_PROJECT || defined CARDSTATION_PROJECT)
      #define MAX_CHLDRN_CNT  0
    #else
      #define MAX_CHLDRN_CNT  30
    #endif
    #define MAX_RTRS_CNT  0
#if   ( SECURE != 0  )
    #define USE_NWK_SECURITY    1   // true or false
    #define SECURITY_LEVEL      5
#else
    #define USE_NWK_SECURITY    0   // true or false
    #define SECURITY_LEVEL      0
#endif
#elif (STACK_PROFILE_ID == MINE_RTR_TEST)
    #define MAX_NODE_DEPTH          3
    #define NWK_MODE            NWK_MODE_MESH
    #define SECURITY_MODE       SECURITY_RESIDENTIAL
    #define MAX_CHLDRN_CNT  2
    #define MAX_RTRS_CNT  1
#if   ( SECURE != 0  )
    #define USE_NWK_SECURITY    1   // true or false
    #define SECURITY_LEVEL      5
#else
    #define USE_NWK_SECURITY    0   // true or false
    #define SECURITY_LEVEL      0
#endif
#endif

// Zigbee protocol version
#define ZB_PROT_V1_0                 1
#define ZB_PROT_V1_1                 2

#define ZB_PROT_VERS      ZB_PROT_V1_1
#define ZIGBEE_PROT_ID            0x00

// no. of entries in the regular routing table plus
#define MAX_UNRESERVED_RTG_ENTRIES      (MAX_RTG_ENTRIES - 4)

// Status and error codes for extra information
#define NWK_STATUS_PING_RCVD            0x0001
#define NWK_STATUS_ASSOC_CNF            0x0002
#define NWK_STATUS_ED_ADDR              0x0003
#define NWK_STATUS_PARENT_ADDR          0x0004
#define NWK_STATUS_COORD_ADDR           0x0005
#define NWK_STATUS_ROUTER_ADDR          0x0006
#define NWK_STATUS_ORPHAN_RSP           0x0007

#define NWK_ERROR_ASSOC_RSP             0x1001
#define NWK_ERROR_ASSOC_RSP_MF          0x1002
#define NWK_ERROR_ASSOC_CNF_DENIED      0x1003
#define NWK_ERROR_ENERGY_SCAN_FAILED    0x1004

// Maximum number in tables
#if !defined( NWK_MAX_DEVICE_LIST )
  #if (defined MINE_STATION)
  #define NWK_MAX_DEVICE_LIST     30  // Maximum number of devices in the
                                    // Assoc/Device list.
  #else
  #define NWK_MAX_DEVICE_LIST     20  // Maximum number of devices in the
                                    // Assoc/Device list.  
  #endif
#endif

// Don't change this value to set the number of devices.  Change
//  NWK_MAX_DEVICE_LIST above
#define NWK_MAX_DEVICES      NWK_MAX_DEVICE_LIST + 1    // One extra space for parent

#if defined (RTR_NWK)
    #define MAX_NEIGHBOR_ENTRIES    8
#else
    #define MAX_NEIGHBOR_ENTRIES    4
#endif  // RTR

#if !defined ( APS_MAX_GROUPS )
  #define APS_MAX_GROUPS  10
#endif

#if !defined ( APSF_DEFAULT_WINDOW_SIZE )
  #define APS_DEFAULT_WINDOW_SIZE        5
#endif

#if !defined ( APSF_DEFAULT_INTERFRAME_DELAY )
  #define APS_DEFAULT_INTERFRAME_DELAY   50
#endif

// Maxiumum number of REFLECTOR address entries
#if defined ( REFLECTOR )
  #define NWK_MAX_REFLECTOR_ENTRIES ( NWK_MAX_BINDING_ENTRIES )
#else
  #define NWK_MAX_REFLECTOR_ENTRIES 0
#endif

#if !defined( MAX_BCAST )
  #define MAX_BCAST 4
#endif
  
// Maxiumum number of secure partners(Commercial mode only).
// Add 1 for the Trust Center(Coordinator) if it is not the parent.
#define NWK_MAX_SECURE_PARTNERS 1

// Maximum number of addresses managed by the Address Manager
#define NWK_MAX_ADDRESSES (uint16)                          \
                          ( ( NWK_MAX_DEVICES           ) +   \
                            ( NWK_MAX_REFLECTOR_ENTRIES ) +   \
                            ( NWK_MAX_SECURE_PARTNERS   )   )

// Network PAN Coordinator Address
#define NWK_PAN_COORD_ADDR 0x0000

/*********************************************************************
 * TYPEDEFS
 */

/*********************************************************************
 * NWK GLOBAL VARIABLES
 */

// Variables for MAX data buffer levels
extern CONST byte gNWK_MAX_DATABUFS_WAITING;
extern CONST byte gNWK_MAX_DATABUFS_SCHEDULED;
extern CONST byte gNWK_MAX_DATABUFS_CONFIRMED;
extern CONST byte gNWK_MAX_DATABUFS_TOTAL;

extern CONST byte gNWK_INDIRECT_CNT_RTG_TMR;
extern CONST byte gNWK_INDIRECT_MSG_MAX_PER;
extern CONST byte gNWK_INDIRECT_MSG_MAX_ALL;

extern CONST byte gMAX_NEIGHBOR_ENTRIES;

extern CONST byte gMAX_RTG_ENTRIES;
extern CONST byte gMAX_UNRESERVED_RTG_ENTRIES;
extern CONST byte gMAX_RREQ_ENTRIES;

// Variables for MAX list size
extern CONST uint16 gNWK_MAX_DEVICE_LIST;

extern uint16 *Cskip;
extern byte CskipRtrs[];
extern byte CskipChldrn[];

extern byte gMIN_TREE_LINK_COST;

extern CONST byte defaultKey[];

extern CONST byte gMAX_BCAST;


/*********************************************************************
 * APS GLOBAL VARIABLES
 */

// Variables for Binding Table
extern CONST uint16 gNWK_MAX_BINDING_ENTRIES;
extern CONST byte gMAX_BINDING_CLUSTER_IDS;
extern CONST uint16 gBIND_REC_SIZE;

extern CONST uint8 gAPS_MAX_GROUPS;

extern CONST uint8 apscMaxWindowSize;
extern CONST uint16 gAPS_INTERFRAME_DELAY;

/*********************************************************************
 * FUNCTIONS
 */

/*
 * Init Global Variables
 */
extern void nwk_globals_init( void );
extern void NIB_init( void );

extern void nwk_Status( uint16 statusCode, uint16 statusValue );

/*********************************************************************
*********************************************************************/
#ifdef __cplusplus
}
#endif

#endif /* NWK_GLOBALS_H */


