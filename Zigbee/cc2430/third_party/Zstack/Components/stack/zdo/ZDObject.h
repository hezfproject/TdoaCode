/**************************************************************************************************
  Filename:       ZDObject.h
  Revised:        $Date: 2009/06/22 20:09:41 $
  Revision:       $Revision: 1.2 $

  Description:    This file contains the interface to the Zigbee Device Object.


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
  PROVIDED �AS IS� WITHOUT WARRANTY OF ANY KIND, EITHER EXPRESS OR IMPLIED, 
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

#ifndef ZDOBJECT_H
#define ZDOBJECT_H

#ifdef __cplusplus
extern "C"
{
#endif

/*********************************************************************
 * INCLUDES
 */
#include "NLMEDE.h"
#include "ZDApp.h"

/*********************************************************************
 * CONSTANTS
 */
#define ZDO_MAX_LQI_ITEMS       3
#define ZDO_MAX_NWKDISC_ITEMS   5
#define ZDO_MAX_RTG_ITEMS       10
#define ZDO_MAX_BIND_ITEMS      3

/*********************************************************************
 * TYPEDEFS
 */
typedef enum
{
  MODE_JOIN,
  MODE_RESUME,
//MODE_SOFT,      // Not supported yet
  MODE_HARD,
  MODE_REJOIN
} devStartModes_t;

typedef struct
{
  uint8  status;
  uint16 nwkAddr;
  uint8  extAddr[Z_EXTADDR_LEN];
  uint8  numAssocDevs;
  uint8  startIndex;
  uint16 devList[];
} ZDO_NwkIEEEAddrResp_t;

typedef struct
{
  uint8 status;
  uint16 nwkAddr;   // Network address of interest
  NodeDescriptorFormat_t nodeDesc;
} ZDO_NodeDescRsp_t;

typedef struct
{
  uint8 status;
  uint16 nwkAddr;   // Network address of interest
  NodePowerDescriptorFormat_t pwrDesc;
} ZDO_PowerRsp_t;

typedef struct
{
  uint8  status;
  uint16 nwkAddr;   // Network address of interest
  SimpleDescriptionFormat_t simpleDesc;
} ZDO_SimpleDescRsp_t;

typedef struct
{
  uint8  status;
  uint16 nwkAddr;   // Network address of interest
  uint8  cnt;
  uint8  epList[];
} ZDO_ActiveEndpointRsp_t;

typedef ZDO_ActiveEndpointRsp_t ZDO_MatchDescRsp_t;

typedef struct
{
  uint8  status;
  uint8  networkCount;
  uint8  startIndex;
  uint8  networkListCount;
  mgmtNwkDiscItem_t list[];
} ZDO_MgmNwkDiscRsp_t;

typedef struct
{
  uint8  status;
  uint8  neighborLqiEntries;
  uint8  startIndex;
  uint8  neighborLqiCount;
  neighborLqiItem_t list[];
} ZDO_MgmtLqiRsp_t;

typedef struct
{
  uint8  status;
  uint8  rtgCount;
  uint8  startIndex;
  uint8  rtgListCount;
  rtgItem_t list[];
} ZDO_MgmtRtgRsp_t;

typedef struct
{
  uint8  status;
  uint8  bindingCount;
  uint8  startIndex;
  uint8  bindingListCount;
  apsBindingItem_t list[];
} ZDO_MgmtBindRsp_t;

typedef struct
{
  uint8  status;
  uint16 nwkAddr;   // Address of interest
  uint8  length;
  uint8  desc[];
} ZDO_UserDescRsp_t;

typedef struct
{
  uint8  status;
  uint16 serverMask;
} ZDO_ServerDiscRsp_t;

typedef struct
{
  uint8       srcAddress[Z_EXTADDR_LEN];
  uint8       srcEndpoint;
  uint16      clusterID;
  zAddrType_t dstAddress;
  uint8       dstEndpoint;
} ZDO_BindUnbindReq_t;

typedef struct
{
  uint16      nwkAddr;
  uint8       extAddr[Z_EXTADDR_LEN];
  uint8       capabilities;
} ZDO_DeviceAnnce_t;

/*********************************************************************
 * Internal ZDO types
 */

enum
{
  ZDMATCH_REASON_START,
  ZDMATCH_REASON_TIMEOUT,
  ZDMATCH_REASON_UNBIND_RSP,
  ZDMATCH_REASON_BIND_RSP
};

typedef struct
{
  ZDEndDeviceBind_t ed1;
  ZDEndDeviceBind_t ed2;
  uint8  state;            // One of the above states
  uint8  sending;         // 0 - not sent, 1 - unbind, 2 bind - expecting response
  uint8  transSeq;
  uint8  ed1numMatched;
  uint16 *ed1Matched;
  uint8  ed2numMatched;
  uint16 *ed2Matched;
} ZDMatchEndDeviceBind_t;

/*********************************************************************
 * MACROS
 */

/*********************************************************************
 * GLOBAL VARIABLES
 */
#if defined ( ZDO_COORDINATOR )
  extern ZDMatchEndDeviceBind_t *matchED;
#endif


/*********************************************************************
 * FUNCTIONS - API
 */

/*
 * ZDO_Init - ZDObject and ZDApp Initialization.
 */
extern void ZDO_Init( void );

/*
 * ZDO_StartDevice - Start the device in a network
 */
extern void ZDO_StartDevice( byte logicalType, devStartModes_t startMode,
                             byte beaconOrder, byte superframeOrder );

/*
 * ZDO_UpdateNwkStatus - Update nwk state in the apps
 */
extern void ZDO_UpdateNwkStatus( devStates_t state );

/*
 * ZDO_MatchEndDeviceBind - Match End Device Bind Requests
 */
extern void ZDO_MatchEndDeviceBind( ZDEndDeviceBind_t *bindReq );

/*********************************************************************
 * Call Back Functions from ZDProfile  - API
 */

extern byte ZDO_AnyClusterMatches(
                              byte ACnt, uint16 *AList, byte BCnt, uint16 *BList );

/*
 * ZDO_ProcessNodeDescReq - Process the Node_Desc_req message.
 */
extern void ZDO_ProcessNodeDescReq( zdoIncomingMsg_t *inMsg );

/*
 * ZDO_ProcessPowerDescReq - Process the Power_Desc_req message.
 */
extern void ZDO_ProcessPowerDescReq( zdoIncomingMsg_t *inMsg );

/*
 * ZDO_ProcessSimpleDescReq - Process the Simple_Desc_req message
 */
extern void ZDO_ProcessSimpleDescReq( zdoIncomingMsg_t *inMsg );

/*
 * ZDO_ProcessActiveEPReq - Process the Active_EP_req message
 */
extern void ZDO_ProcessActiveEPReq( zdoIncomingMsg_t *inMsg );

/*
 * ZDO_ProcessMatchDescReq - Process the Match_Desc_req message
 */
extern void ZDO_ProcessMatchDescReq( zdoIncomingMsg_t *inMsg );

/*
 * ZDO_ProcessServerDiscRsp - Process the Server_Discovery_rsp message.
 */
#if defined ( ZDO_SERVERDISC_REQUEST )
void ZDO_ProcessServerDiscRsp( zdoIncomingMsg_t *inMsg );
#endif

/*
 * ZDO_ProcessServerDiscReq - Process the Server_Discovery_req message.
 */
#if defined ( ZDO_SERVERDISC_RESPONSE )
void ZDO_ProcessServerDiscReq( zdoIncomingMsg_t *inMsg );
#endif

/*********************************************************************
 * Internal ZDO interfaces
 */

extern uint8 ZDMatchSendState( uint8 reason, uint8 status, uint8 TransSeq );

/*********************************************************************
 * Call Back Functions from APS  - API
 */

/*
 * ZDO_EndDeviceTimeoutCB - Called when the binding timer expires
 */
extern void ZDO_EndDeviceTimeoutCB( void );

/*********************************************************************
 * Optional Management Messages
 */

#if defined ( ZDO_MGMT_NWKDISC_REQUEST )
  /*
   * ZDO_ProcessMgmtNwkDiscReq - Called to parse the incoming
   * Management Network Discover Response
   */
  extern void ZDO_ProcessMgmNwkDiscRsp( zdoIncomingMsg_t *inMsg );
#endif

#if defined ( ZDO_MGMT_NWKDISC_RESPONSE )
  /*
   * ZDO_ProcessMgmtNwkDiscReq - Called to parse the incoming
   * Management LQI Request
   */
  extern void ZDO_ProcessMgmtNwkDiscReq( zdoIncomingMsg_t *inMsg );

  /*
   * ZDO_FinishProcessingMgmtNwkDiscReq - Called to parse the incoming
   * Management LQI Request
   */
  extern void ZDO_FinishProcessingMgmtNwkDiscReq(byte ResultCount,
                                             networkDesc_t *NetworkList );
#endif

#if defined( ZDO_MGMT_LQI_RESPONSE )
  /*
   * ZDO_ProcessMgmtLqiReq - Called to parse the incoming
   * Management LQI Request
   */
  extern void ZDO_ProcessMgmtLqiReq( zdoIncomingMsg_t *inMsg );
#endif

#if defined( ZDO_MGMT_RTG_RESPONSE )
  /*
   * ZDO_ProcessMgmtRtgReq - Called to parse the incoming
   * Management Routing Request
   */
  extern void ZDO_ProcessMgmtRtgReq( zdoIncomingMsg_t *inMsg );
#endif

#if defined ( ZDO_MGMT_BIND_RESPONSE )
  extern void ZDO_ProcessMgmtBindReq( zdoIncomingMsg_t *inMsg );
#endif

#if defined ( ZDO_MGMT_BIND_REQUEST )
  extern void ZDO_ProcessMgmtBindRsp( zdoIncomingMsg_t *inMsg );
#endif

#if defined ( ZDO_MGMT_JOINDIRECT_RESPONSE ) && defined ( RTR_NWK )
  extern void ZDO_ProcessMgmtDirectJoinReq( zdoIncomingMsg_t *inMsg );
#endif

#if defined ( ZDO_MGMT_LEAVE_RESPONSE )
  extern void ZDO_ProcessMgmtLeaveReq( zdoIncomingMsg_t *inMsg );
#endif

#if defined ( ZDO_MGMT_PERMIT_JOIN_RESPONSE )
  extern void ZDO_ProcessMgmtPermitJoinReq( zdoIncomingMsg_t *inMsg );
#endif

#if defined ( ZDO_USERDESC_RESPONSE )
  extern void ZDO_ProcessUserDescReq( zdoIncomingMsg_t *inMsg );
#endif

#if defined ( ZDO_USERDESCSET_REQUEST )
  extern void ZDO_ProcessUserDescConf( zAddrType_t *SrcAddr, byte *msg,
                                       byte SecurityUse );
#endif

#if defined ( ZDO_USERDESCSET_RESPONSE )
  extern void ZDO_ProcessUserDescSet( zdoIncomingMsg_t *inMsg );
#endif

#if defined ( ZDO_ENDDEVICE_ANNCE )
void ZDO_ProcessEndDeviceAnnce( zdoIncomingMsg_t *inMsg );
#endif

void ZDO_BuildSimpleDescBuf( byte *buf, SimpleDescriptionFormat_t *desc );

uint8 ZDO_ParseSimpleDescBuf( byte *buf, SimpleDescriptionFormat_t *desc );


extern void ZDO_UpdateAddrManager( uint16 nwkAddr, uint8 *extAddr );

/*
 * ZDO_ParseAddrRsp - Parse the NWK_addr_rsp and IEEE_addr_rsp messages
 *
 * returns a pointer to parsed structures.  This structure was 
 *          allocated using osal_mem_alloc, so it must be freed
 *          by the calling function [osal_mem_free()].
 */
#if defined ( ZDO_NWKADDR_REQUEST ) || defined ( ZDO_IEEEADDR_REQUEST ) || defined ( REFLECTOR )
extern ZDO_NwkIEEEAddrResp_t *ZDO_ParseAddrRsp( zdoIncomingMsg_t *inMsg );
#endif
/*
 * ZDO_ParseNodeDescRsp - Parse the Node_Desc_rsp message
 */
extern void ZDO_ParseNodeDescRsp( zdoIncomingMsg_t *inMsg, ZDO_NodeDescRsp_t *pNDRsp );

/*
 * ZDO_ParsePowerDescRsp - Parse the Power_Desc_rsp message
 */
extern void ZDO_ParsePowerDescRsp( zdoIncomingMsg_t *inMsg, ZDO_PowerRsp_t *pNPRsp );

/*
 * ZDO_ParseSimpleDescRsp - Parse the Simple_Desc_rsp message
 *   NOTE: The pAppInClusterList and pAppOutClusterList fields
 *         in the SimpleDescriptionFormat_t structure are allocated
 *         and the calling function needs to free [osal_msg_free()]
 *         these buffers.
 */
extern void ZDO_ParseSimpleDescRsp( zdoIncomingMsg_t *inMsg, ZDO_SimpleDescRsp_t *pSimpleDescRsp );

/*
 * ZDO_ParseEPListRsp - Parse the Active_EP_rsp or Match_Desc_rsp message
 *
 * returns a pointer to parsed structures.  This structure was 
 *          allocated using osal_mem_alloc, so it must be freed
 *          by the calling function [osal_mem_free()].
 */
extern ZDO_ActiveEndpointRsp_t *ZDO_ParseEPListRsp( zdoIncomingMsg_t *inMsg );

/*
 * ZDO_ParseBindRsp - Parse the Bind_rsp, Unbind_rsp or End_Device_Bind_rsp message.
 *     Parameter: a - pointer to the message to parse [zdoIncomingMsg_t *]
 *
 *  returns the status field of the message.
 */
#define ZDO_ParseBindRsp(a) ((uint8)(*(a->asdu)))

/*
 * ZDO_ParseMgmNwkDiscRsp - Parse the Mgmt_NWK_Disc_rsp message
 *
 * returns a pointer to parsed response structure (NULL if not allocated).  
 *          This structure was allocated using osal_mem_alloc, so it must be freed
 *          by the calling function [osal_mem_free()].
 */
extern ZDO_MgmNwkDiscRsp_t *ZDO_ParseMgmNwkDiscRsp( zdoIncomingMsg_t *inMsg );

/*
 * ZDO_ParseMgmtLqiRsp - Parse the Mgmt_Lqi_rsp message
 *
 * returns a pointer to parsed response structure (NULL if not allocated).  
 *          This structure was allocated using osal_mem_alloc, so it must be freed
 *          by the calling function [osal_mem_free()].
 */
extern ZDO_MgmtLqiRsp_t *ZDO_ParseMgmtLqiRsp( zdoIncomingMsg_t *inMsg );

/*
 * ZDO_ParseMgmtRtgRsp - Parse the Mgmt_Rtg_rsp message
 *
 * returns a pointer to parsed response structure (NULL if not allocated).  
 *          This structure was allocated using osal_mem_alloc, so it must be freed
 *          by the calling function [osal_mem_free()].
 */
extern ZDO_MgmtRtgRsp_t *ZDO_ParseMgmtRtgRsp( zdoIncomingMsg_t *inMsg );

/*
 * ZDO_ParseMgmtBindRsp - Parse the Mgmt_Bind_rsp message
 *
 * returns a pointer to parsed response structure (NULL if not allocated).  
 *          This structure was allocated using osal_mem_alloc, so it must be freed
 *          by the calling function [osal_mem_free()].
 */
extern ZDO_MgmtBindRsp_t *ZDO_ParseMgmtBindRsp( zdoIncomingMsg_t *inMsg );

/*
 * ZDO_ParseMgmtDirectJoinRsp - Parse the Mgmt_Direct_Join_rsp message.
 *     Parameter: a - pointer to the message to parse [zdoIncomingMsg_t *]
 *
 *  returns the status field of the message.
 */
#define ZDO_ParseMgmtDirectJoinRsp(a) ((uint8)(*(a->asdu)))

/*
 * ZDO_ParseMgmtLeaveRsp - Parse the Mgmt_Leave_rsp message.
 *     Parameter: a - pointer to the message to parse [zdoIncomingMsg_t *]
 *
 *  returns the status field of the message.
 */
#define ZDO_ParseMgmtLeaveRsp(a) ((uint8)(*(a->asdu)))

/*
 * ZDO_ParseMgmtPermitJoinRsp - Parse the Mgmt_Permit_Join_rsp message.
 *     Parameter: a - pointer to the message to parse [zdoIncomingMsg_t *]
 *
 *  returns the status field of the message.
 */
#define ZDO_ParseMgmtPermitJoinRsp(a) ((uint8)(*(a->asdu)))

/*
 * ZDO_ParseUserDescRsp - Parse the User_Desc_rsp message
 *
 * returns a pointer to parsed response structure (NULL if not allocated).  
 *          This structure was allocated using osal_mem_alloc, so it must be freed
 *          by the calling function [osal_mem_free()].
 */
extern ZDO_UserDescRsp_t *ZDO_ParseUserDescRsp( zdoIncomingMsg_t *inMsg );

/*
 * ZDO_ParseServerDiscRsp - Parse the Server_Discovery_rsp message
 *
 */
extern void ZDO_ParseServerDiscRsp( zdoIncomingMsg_t *inMsg, ZDO_ServerDiscRsp_t *pRsp );

/*
 * ZDO_ParseEndDeviceBindReq - Parse the User_Desc_rsp message
 *
 *   NOTE:  The clusters lists in bindReq are allocated in this
 *          function and must be freed by that calling function.
 */
extern void ZDO_ParseEndDeviceBindReq( zdoIncomingMsg_t *inMsg, ZDEndDeviceBind_t *bindReq );

/*
 * ZDO_ParseBindUnbindReq - Parses the Bind_req or Unbind_req messages
 */
extern void ZDO_ParseBindUnbindReq( zdoIncomingMsg_t *inMsg, ZDO_BindUnbindReq_t *pReq );

/*
 * ZDApp_ProcessBindUnbindReq - Called to process a Bind_req or Unbind_req message
 */
extern void ZDO_ProcessBindUnbindReq( zdoIncomingMsg_t *inMsg, ZDO_BindUnbindReq_t *pReq );

/*
 * ZDO_ParseUserDescConf - Parse the User_Desc_conf message.
 *     Parameter: a - pointer to the message to parse [zdoIncomingMsg_t *]
 *
 *  returns the status field of the message.
 */
#define ZDO_ParseUserDescConf(a) ((uint8)(*(a->asdu)))

/*
 * ZDO_ParseDeviceAnnce - Called to parse an End_Device_annce message
 */
extern void ZDO_ParseDeviceAnnce( zdoIncomingMsg_t *inMsg, ZDO_DeviceAnnce_t *pAnnce );


/*********************************************************************
*********************************************************************/

#ifdef __cplusplus
}
#endif

#endif /* ZDOBJECT_H */
