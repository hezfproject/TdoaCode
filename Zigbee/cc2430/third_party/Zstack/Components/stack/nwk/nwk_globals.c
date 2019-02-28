/**************************************************************************************************
  Filename:       nwk_globals.c
  Revised:        $Date: 2010/04/29 20:19:58 $
  Revision:       $Revision: 1.7 $

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

/*********************************************************************
 * INCLUDES
 */
#include "ZComdef.h"
#include "OSAL.h"
#include "AddrMgr.h"
#include "AssocList.h"
#include "BindingTable.h"
#include "nwk_globals.h"
#include "ssp.h"
#include "rtg.h"
#include "ZDConfig.h"
#include "ZGlobals.h"

#if defined ( LCD_SUPPORTED )
  #include "OnBoard.h"
#endif

/* HAL */
#include "hal_lcd.h"

/*********************************************************************
 * MACROS
 */

/*********************************************************************
 * CONSTANTS
 */

// Maximums for the data buffer queue
#define NWK_MAX_DATABUFS_WAITING    8     // Waiting to be sent to MAC
#define NWK_MAX_DATABUFS_SCHEDULED  5     // Timed messages to be sent
#define NWK_MAX_DATABUFS_CONFIRMED  5     // Held after MAC confirms
#define NWK_MAX_DATABUFS_TOTAL      12    // Total number of buffers

// 1-255 (0 -> 256) X RTG_TIMER_INTERVAL
// A known shortcoming is that when a message is enqueued as "hold" for a
// sleeping device, the timer tick may have counted down to 1, so that msg
// will not be held as long as expected. If NWK_INDIRECT_MSG_TIMEOUT is set to 1
// the hold time will vary randomly from 0 - CNT_RTG_TIMER ticks.
// So the hold time will vary within this interval:
// { (NWK_INDIRECT_MSG_TIMEOUT-1)*CNT_RTG_TIMER,
//                                    NWK_INDIRECT_MSG_TIMEOUT*CNT_RTG_TIMER }
#define NWK_INDIRECT_CNT_RTG_TMR    1  //ggg - need hours or days?
// To hold msg for sleeping end devices for 30 secs:
// #define CNT_RTG_TIMER            1
// #define NWK_INDIRECT_MSG_TIMEOUT 30
// To hold msg for sleeping end devices for 30 mins:
// #define CNT_RTG_TIMER            60
// #define NWK_INDIRECT_MSG_TIMEOUT 30
// To hold msg for sleeping end devices for 30 days:
// #define CNT_RTG_TIMER            60
// #define NWK_INDIRECT_MSG_TIMEOUT (30 * 24 * 60)
// Maximum msgs to hold per associated device.
#define NWK_INDIRECT_MSG_MAX_PER    3
// Maximum total msgs to hold for all associated devices.
#define NWK_INDIRECT_MSG_MAX_ALL    \
                            (NWK_MAX_DATABUFS_TOTAL - NWK_INDIRECT_MSG_MAX_PER)


/*********************************************************************
 * TYPEDEFS
 */

/*********************************************************************
 * NWK GLOBAL VARIABLES
 */

// Variables for MAX list size
CONST uint16 gNWK_MAX_DEVICE_LIST = NWK_MAX_DEVICES;

// Variables for MAX data buffer levels
CONST byte gNWK_MAX_DATABUFS_WAITING = NWK_MAX_DATABUFS_WAITING;
CONST byte gNWK_MAX_DATABUFS_SCHEDULED = NWK_MAX_DATABUFS_SCHEDULED;
CONST byte gNWK_MAX_DATABUFS_CONFIRMED = NWK_MAX_DATABUFS_CONFIRMED;
CONST byte gNWK_MAX_DATABUFS_TOTAL = NWK_MAX_DATABUFS_TOTAL;

CONST byte gNWK_INDIRECT_CNT_RTG_TMR = NWK_INDIRECT_CNT_RTG_TMR;
CONST byte gNWK_INDIRECT_MSG_MAX_PER = NWK_INDIRECT_MSG_MAX_PER;
CONST byte gNWK_INDIRECT_MSG_MAX_ALL = NWK_INDIRECT_MSG_MAX_ALL;

#if defined ( RTR_NWK )
  // change this if using a different stack profile...
  // Cskip array
  uint16 *Cskip;

  #if ( STACK_PROFILE_ID == HOME_CONTROLS )
    byte CskipRtrs[MAX_NODE_DEPTH+1] = {MAX_RTRS_CNT,MAX_RTRS_CNT,
    				MAX_RTRS_CNT,MAX_RTRS_CNT,MAX_RTRS_CNT,0};
    byte CskipChldrn[MAX_NODE_DEPTH+1] = {MAX_CHLDRN_CNT,MAX_CHLDRN_CNT,
				MAX_CHLDRN_CNT,MAX_CHLDRN_CNT,MAX_CHLDRN_CNT,0};
  #elif ( STACK_PROFILE_ID == GENERIC_STAR )
    byte CskipRtrs[MAX_NODE_DEPTH+1] = {MAX_RTRS_CNT,MAX_RTRS_CNT,
    				MAX_RTRS_CNT,MAX_RTRS_CNT,MAX_RTRS_CNT,0};
    byte CskipChldrn[MAX_NODE_DEPTH+1] = {MAX_CHLDRN_CNT,MAX_CHLDRN_CNT,
				MAX_CHLDRN_CNT,MAX_CHLDRN_CNT,MAX_CHLDRN_CNT,0};
  #elif ( STACK_PROFILE_ID == NETWORK_SPECIFIC )
    byte CskipRtrs[MAX_NODE_DEPTH+1] = {MAX_RTRS_CNT,MAX_RTRS_CNT,
    				MAX_RTRS_CNT,MAX_RTRS_CNT,MAX_RTRS_CNT,0};
    byte CskipChldrn[MAX_NODE_DEPTH+1] = {MAX_CHLDRN_CNT,MAX_CHLDRN_CNT,
				MAX_CHLDRN_CNT,MAX_CHLDRN_CNT,MAX_CHLDRN_CNT,0};
#elif ( STACK_PROFILE_ID == GENERIC_MINE )
byte CskipRtrs[MAX_NODE_DEPTH+1] = {MAX_RTRS_CNT,MAX_RTRS_CNT,MAX_RTRS_CNT,
				MAX_RTRS_CNT,MAX_RTRS_CNT,MAX_RTRS_CNT,MAX_RTRS_CNT,
				MAX_RTRS_CNT,MAX_RTRS_CNT,0};
byte CskipChldrn[MAX_NODE_DEPTH+1] = {MAX_CHLDRN_CNT,MAX_CHLDRN_CNT,
				MAX_CHLDRN_CNT,MAX_CHLDRN_CNT,MAX_CHLDRN_CNT,
				MAX_CHLDRN_CNT,MAX_CHLDRN_CNT,MAX_CHLDRN_CNT,
				MAX_CHLDRN_CNT,0};
#elif ( STACK_PROFILE_ID == MINE_CONTROLS)
byte CskipRtrs[MAX_NODE_DEPTH+1] = {MAX_RTRS_CNT,0};
byte CskipChldrn[MAX_NODE_DEPTH+1] = {MAX_CHLDRN_CNT,0};
#elif (STACK_PROFILE_ID == MINE_RTR_TEST)
byte CskipRtrs[MAX_NODE_DEPTH+1] = {MAX_RTRS_CNT,MAX_RTRS_CNT,MAX_RTRS_CNT,0};
byte CskipChldrn[MAX_NODE_DEPTH+1] = {MAX_CHLDRN_CNT,MAX_CHLDRN_CNT,MAX_CHLDRN_CNT,0};
  #endif // STACK_PROFILE_ID
#endif  // RTR_NWK


// Minimum lqi value that is required for association
byte gMIN_TREE_LINK_COST = MIN_LQI_COST_3;

#if defined(RTR_NWK)
  // Statically defined Associated Device List
  associated_devices_t AssociatedDevList[NWK_MAX_DEVICES];
#endif

CONST byte gMAX_RTG_ENTRIES = MAX_RTG_ENTRIES;
CONST byte gMAX_UNRESERVED_RTG_ENTRIES = MAX_UNRESERVED_RTG_ENTRIES;
CONST byte gMAX_RREQ_ENTRIES = MAX_RREQ_ENTRIES;

CONST byte gMAX_NEIGHBOR_ENTRIES = MAX_NEIGHBOR_ENTRIES;

 // Table of neighboring nodes (not including child nodes)
neighborEntry_t neighborTable[MAX_NEIGHBOR_ENTRIES];

#if defined ( RTR_NWK )
  // Routing table
  rtgEntry_t rtgTable[MAX_RTG_ENTRIES];

  // Table of current RREQ packets in the network
  rtDiscEntry_t rtDiscTable[MAX_RREQ_ENTRIES];
  
  // Table of data broadcast packets currently in circulation.
  bcastEntry_t bcastTable[MAX_BCAST];
  
  // These 2 arrays are to be used as an array of struct { uint8, uint32 }.
  uint8 bcastHoldHandle[MAX_BCAST];
  uint32 bcastHoldAckMask[MAX_BCAST];
  
  CONST byte gMAX_BCAST = MAX_BCAST;
#endif

/*********************************************************************
 * APS GLOBAL VARIABLES
 */

//#if defined ( REFLECTOR )
  // The Maximum number of binding records
  // This number is defined in BindingTable.h - change it there.
  CONST uint16 gNWK_MAX_BINDING_ENTRIES = NWK_MAX_BINDING_ENTRIES;
#if defined ( REFLECTOR )
  // The Maximum number of cluster IDs in a binding record
  // This number is defined in BindingTable.h - change it there.
  CONST byte gMAX_BINDING_CLUSTER_IDS = MAX_BINDING_CLUSTER_IDS;

  CONST uint16 gBIND_REC_SIZE = sizeof( BindingEntry_t );

  // Binding Table
  BindingEntry_t BindingTable[NWK_MAX_BINDING_ENTRIES];
#endif

// Maximum number allowed in the groups table.
CONST uint8 gAPS_MAX_GROUPS = APS_MAX_GROUPS;

// The size of a tx window when using fragmentation
CONST uint8 apscMaxWindowSize = APS_DEFAULT_WINDOW_SIZE;

// The delay between tx packets when using fragmentaition
CONST uint16 gAPS_INTERFRAME_DELAY = APS_DEFAULT_INTERFRAME_DELAY;


/*********************************************************************
 * SECURITY GLOBAL VARIABLES
 */

// This is the default pre-configured key,
// change this to make a unique key
// SEC_KEY_LEN is defined in ssp.h.
CONST byte defaultKey[SEC_KEY_LEN] =
{
#if defined ( APP_TP ) || defined ( APP_TP2 )
  // Key for ZigBee Conformance Testing
  0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00,
  0x89, 0x67, 0x45, 0x23, 0x01, 0xEF, 0xCD, 0xAB
#else
  // Key for In-House Testing
  0x00, 0x01, 0x02, 0x03, 0x04, 0x05, 0x06, 0x07,
  0x08, 0x09, 0x0A, 0x0B, 0x0C, 0x0D, 0x0E, 0x0F
#endif
};


/*********************************************************************
 * STATUS STRINGS
 */
#if defined ( LCD_SUPPORTED )
  const char PingStr[]         = "Ping Rcvd from";
  const char AssocCnfStr[]     = "Assoc Cnf";
  const char SuccessStr[]      = "Success";
  const char EndDeviceStr[]    = "EndDevice:";
  const char ParentStr[]       = "Parent:";
  const char ZigbeeCoordStr[]  = "ZigBee Coord";
  const char NetworkIDStr[]    = "Network ID:";
  const char RouterStr[]       = "Router:";
  const char OrphanRspStr[]    = "Orphan Response";
  const char SentStr[]         = "Sent";
  const char FailedStr[]       = "Failed";
  const char AssocRspFailStr[] = "Assoc Rsp fail";
  const char AssocIndStr[]     = "Assoc Ind";
  const char AssocCnfFailStr[] = "Assoc Cnf fail";
  const char EnergyLevelStr[]  = "Energy Level";
  const char ScanFailedStr[]   = "Scan Failed";
#endif

/*********************************************************************
 * @fn       nwk_globals_init()
 *
 * @brief
 *
 *   Initialize nwk layer globals.  These are the system defaults and
 *   should be changed by the user here.  The default definitions are
 *   defined in nwk.h or NLMEDE.h.
 *
 * @param   none
 *
 * @return  none
 */
void nwk_globals_init( void )
{
  AddrMgrInit( NWK_MAX_ADDRESSES );

#if defined ( RTR_NWK )
  // Initialize the Cskip Table
  Cskip = osal_mem_alloc(sizeof(uint16) *(MAX_NODE_DEPTH+1));
  RTG_FillCSkipTable(CskipChldrn, CskipRtrs, MAX_NODE_DEPTH, Cskip);
#endif
}

/*********************************************************************
 * @fn       NIB_init()
 *
 * @brief
 *
 *   Initialize attribute values in NIB
 *
 * @param   none
 *
 * @return  none
 */
void NIB_init()
{
#if defined ( AUTO_SOFT_START )
  byte extAddr[Z_EXTADDR_LEN];
  ZMacGetReq( ZMacExtAddr, extAddr );
  _NIB.SequenceNum = extAddr[0];
#else
  _NIB.SequenceNum = 1;
#endif

  _NIB.nwkProtocolVersion = ZB_PROT_VERS;
  _NIB.MaxDepth = MAX_NODE_DEPTH;

#if ( NWK_MODE == NWK_MODE_MESH )
  _NIB.beaconOrder = BEACON_ORDER_NO_BEACONS;
  _NIB.superFrameOrder = BEACON_ORDER_NO_BEACONS;
#endif

   // BROADCAST SETTINGS:
   // *******************
   //   Broadcast Delivery Time
   //     - set to multiples of 100ms 
   //     - should be 500ms more than the retry time
   //       -  "retry time" = PassiveAckTimeout * (MaxBroadcastRetries + 1)
   //   Passive Ack Timeout
   //     - set to multiples of 100ms 
   _NIB.BroadcastDeliveryTime = zgBcastDeliveryTime;
   _NIB.PassiveAckTimeout     = zgPassiveAckTimeout;
   _NIB.MaxBroadcastRetries   = zgMaxBcastRetires;

   _NIB.ReportConstantCost = 0;
   _NIB.RouteDiscRetries = 0;
   _NIB.SecureAllFrames = USE_NWK_SECURITY;
   _NIB.SecurityLevel = zgSecurityLevel;
   _NIB.SymLink = 0;
   _NIB.CapabilityInfo = ZDO_Config_Node_Descriptor.CapabilityFlags;

   _NIB.TransactionPersistenceTime = zgIndirectMsgTimeout;

   _NIB.RouteDiscoveryTime = 5;
   _NIB.RouteExpiryTime = zgRouteExpiryTime;

   _NIB.nwkDevAddress = INVALID_NODE_ADDR;
   _NIB.nwkLogicalChannel = 0;
   _NIB.nwkCoordAddress = INVALID_NODE_ADDR;
   osal_memset( _NIB.nwkCoordExtAddress, 0, Z_EXTADDR_LEN );
   _NIB.nwkPanId = INVALID_NODE_ADDR;

   osal_cpyExtAddr( _NIB.extendedPANID, zgExtendedPANID );
   
   _NIB.nwkKeyLoaded = FALSE;
}

/*********************************************************************
 * @fn       nwk_Status()
 *
 * @brief
 *
 *   Status report.
 *
 * @param   statusCode
 * @param   statusValue
 *
 * @return  none
 */
void nwk_Status( uint16 statusCode, uint16 statusValue )
{
#if defined ( LCD_SUPPORTED )
  switch ( statusCode )
  {

#if defined( RTR_NWK )
    case NWK_STATUS_COORD_ADDR:
      HalLcdWriteString( (char*)ZigbeeCoordStr, HAL_LCD_LINE_1 );
      HalLcdWriteStringValue( (char*)NetworkIDStr, (uint32)statusValue, 16, HAL_LCD_LINE_2 );
      BuzzerControl( BUZZER_BLIP );
      break;

    case NWK_STATUS_ROUTER_ADDR:
      HalLcdWriteStringValue( (char*)RouterStr, (uint32)statusValue, 16, HAL_LCD_LINE_1 );
      break;

    case NWK_STATUS_ORPHAN_RSP:
      if ( statusValue == ZSuccess )
        HalLcdWriteScreen( (char*)OrphanRspStr, (char*)SentStr );
      else
        HalLcdWriteScreen( (char*)OrphanRspStr, (char*)FailedStr );
      break;

    case NWK_ERROR_ASSOC_RSP:
      HalLcdWriteString( (char*)AssocRspFailStr, HAL_LCD_LINE_1 );
      HalLcdWriteValue( (uint32)(statusValue), 16, HAL_LCD_LINE_2 );
      break;
#else
    case NWK_STATUS_ED_ADDR:
            HalLcdWriteStringValue( (char*)EndDeviceStr, (uint32)statusValue, 16, HAL_LCD_LINE_1 );
      break;
#endif

    case NWK_STATUS_PARENT_ADDR:
            HalLcdWriteStringValue( (char*)ParentStr, (uint32)statusValue, 16, HAL_LCD_LINE_2 );
      break;

    case NWK_STATUS_ASSOC_CNF:
      HalLcdWriteScreen( (char*)AssocCnfStr, (char*)SuccessStr );
      break;

    case NWK_ERROR_ASSOC_CNF_DENIED:
      HalLcdWriteString((char*)AssocCnfFailStr, HAL_LCD_LINE_1 );
      HalLcdWriteValue( (uint32)(statusValue), 16, HAL_LCD_LINE_2 );
      break;

    case NWK_ERROR_ENERGY_SCAN_FAILED:
      HalLcdWriteScreen( (char*)EnergyLevelStr, (char*)ScanFailedStr );
      break;
  }
#endif
}

/*********************************************************************
*********************************************************************/
