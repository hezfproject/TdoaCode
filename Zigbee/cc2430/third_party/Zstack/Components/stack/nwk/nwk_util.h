/**************************************************************************************************
  Filename:       nwk_util.h
  Revised:        $Date: 2009/02/07 01:08:32 $
  Revision:       $Revision: 1.1.1.1 $

  Description:    Network layer utility functions.


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

#ifndef NWK_UTIL_H
#define NWK_UTIL_H

#ifdef __cplusplus
extern "C" {
#endif

/*********************************************************************
 * INCLUDES
 */
#include "NLMEDE.h"

/*********************************************************************
 * MACROS
 */

/*********************************************************************
 * CONSTANTS
 */

/*********************************************************************
 * FRAME FORMAT CONSTANTS
 */

// Command identifiers
#define CMD_ID_RREQ                  1
#define CMD_ID_RREP                  2
#define CMD_ID_RERR                  3
#define CMD_ID_LEAVE                 4
#define CMD_ID_REJOIN_REQ            6
#define CMD_ID_REJOIN_RSP            7

#define CMD_ID_PING                  7
#define CMD_ID_PING_RSP              8
#define CMD_ID_TREE_REQ              9
#define CMD_ID_TREE_RSP             10
#define CMD_ID_PARENT_REQ           11
#define CMD_ID_PARENT_RSP           12

// header fields
#define NWK_HDR_FRAME_CTRL_LSB 0
#define NWK_HDR_FRAME_CTRL_MSB 1
#define NWK_HDR_DST_ADDR_LSB   2
#define NWK_HDR_DST_ADDR_MSB   3
#define NWK_HDR_SRC_ADDR_LSB   4
#define NWK_HDR_SRC_ADDR_MSB   5
#define NWK_HDR_RADIUS         6
#define NWK_HDR_SEQ_NUM        7
#define NWK_HDR_LEN            8

// optional header fields
#define NWK_HDR_DST_EXTADDR_LEN        Z_EXTADDR_LEN
#define NWK_HDR_SRC_EXTADDR_LEN        Z_EXTADDR_LEN
#define NWK_HDR_MULTICAST_CTRL_LEN     1
#define NWK_HDR_SRC_ROUTE_SUBFRAME_LEN 0

// frame control fields
#define NWK_FC_FRAME_TYPE   0
#define NWK_FC_PROT_VERSION 2
#define NWK_FC_DISC_ROUTE   6
#define NWK_FC_MULTICAST    8
#define NWK_FC_SECURE       9
#define NWK_FC_SRC_ROUTE    10
#define NWK_FC_DST_EXTADDR  11
#define NWK_FC_SRC_EXTADDR  12

// frame control field masks
#define NWK_FC_FRAME_TYPE_MASK   0x03
#define NWK_FC_PROT_VERSION_MASK 0x0F
#define NWK_FC_DISC_ROUTE_MASK   0x03
#define NWK_FC_MULTICAST_MASK    0x01
#define NWK_FC_SECURE_MASK       0x01
#define NWK_FC_SRC_ROUTE_MASK    0x01
#define NWK_FC_DST_EXTADDR_MASK  0x01
#define NWK_FC_SRC_EXTADDR_MASK  0x01

// Frame Type sub-field
#define DATA_FRAME_TYPE           0x00
#define CMD_FRAME_TYPE            0x01

// nNetwork command fields
#define NWK_CMD_LEAVE_OPTIONS 1
#define NWK_CMD_LEAVE_SIZE    2
#define NWK_CMD_LEAVE_RJ      0x20 // rejoin
#define NWK_CMD_LEAVE_REQ     0x40 // request(1)/indication(0)
#define NWK_CMD_LEAVE_RC      0x80 // remove children

#define NWK_CMD_REJOIN_REQ_SIZE 2
#define NWK_CMD_REJOIN_RSP_SIZE 4

// Command lengths
#define NSDU_SIZE_RREQ  6
#define NSDU_SIZE_RREP  8
#define NSDU_SIZE_RERR  4

#define NWK_AUX_HDR_LEN  14

/*********************************************************************
 * TYPEDEFS
 */
typedef struct
{
  uint16 dstAddr;
  uint8* extAddr;
  uint8  rejoin;
  uint8  request;
  uint8  removeChildren;
  uint8  cnf;
} NLME_LeaveCmd_t;

/*********************************************************************
 * GLOBAL VARIABLES
 */

/*********************************************************************
 * FUNCTION PROTOTYPES
 */

/*
 * Build a NWK data indication and send to the next higher layer.
 */
extern ZStatus_t NLDE_DataIndSend( NLDE_DataReq_t* req );

/*
 * Build a NWK data service frame and send to the MAC.
 */
extern ZStatus_t NLDE_DataReqSend( NLDE_DataReq_t* req );

/*
 * Send an msdu
 */
extern ZStatus_t NLDE_SendMsg( uint8* msdu, uint16 nextHopAddr,
                               uint8 msduLength, uint8 nsduHandle,
                               uint16 nsduHandleOptions,
                               nwkDB_UserData_t* ud );

/*
 * Call this function to parse an incoming message.
 */
extern void NLDE_ParseMsg( byte *buf, byte bufLength, NLDE_FrameFormat_t *ff );

/*
 * Updates entry in the neighbor table
 */
extern void RTG_UpdateNeighborEntry( uint16 nodeAddress, uint16 panId,
                                     byte linkQuality, byte sent);

extern void NLME_SetAssocFlags( void );

/*
 * Send a simulated MAC->NWK Data Confirm message
 */
extern ZStatus_t nwkSimulateDataCnf( byte handle, byte status );

/*
 * Send the NWK LEAVE cmd
 */
extern ZStatus_t NLME_LeaveCmdSend( NLME_LeaveCmd_t* cmd );

/*
 * Process the NWK LEAVE cmd
 */
extern void NLME_LeaveCmdProcess( NLDE_FrameFormat_t *ff );

/*
 * Handle NWK commands during MACCB_DATA_CNF_CMD processing
 */
extern void NLME_CmdCnf( NLDE_DataCnf_t* cnf );

/*
 * Check for NWK commands that can legally be broadcast
 */
extern uint8 NLME_CmdBcast( uint8 cmdID );

/*
 * Allocate a NLDE_DatatReq_t buffer for NWK commands
 */
extern NLDE_DataReq_t* NLME_CmdDataReqAlloc
                         ( NLDE_DataReqAlloc_t* dra );

/*
 * Stub to load the next higher layer frame data
 */
extern void* NLDE_FrameDataLoad( NLDE_DataReq_t* req );

/*
 * Process Rejoin Rsp command pkt
 */
extern void NLME_RejoinRspCmdProcess( NLDE_FrameFormat_t* ff );

/*
 * Send Rejoin Rsp command pkt
 */
extern ZStatus_t NLME_RejoinRspSend( ZMacAssociateRsp_t *AssocRsp, byte relation  );

/*
 * Process Rejoin Request command pkt
 */
extern void NLME_RejoinReqCmdProcess( NLDE_FrameFormat_t* ff );

/*********************************************************************
 * HELPERS FUNCTION PROTOTYPES
 */

/*
 * Returns a unique handle to identify a packet transaction
 */
extern byte NLDE_GetHandle( void );

extern linkInfo_t *NLME_GetNeighborLinkInfo( uint16 NeighborAddress,
                                             uint16 panId );

extern byte NLME_GetNeighborIndex( uint16 NeighborAddress, uint16 PanId );

extern void NLME_RemoveFromNeighborTable( uint16 NeighborAddress,
                                          uint16 PanId );

extern byte NLME_GetProtocolVersion( void );

extern uint8 NLME_GetEnergyThreshold( void );

extern void NLME_SetEnergyThreshold( uint8 value );

extern void NLME_SetBroadcastFilter(byte capabilities);

extern addr_filter_t NLME_IsAddressBroadcast(uint16 shortAddress);

extern void NLME_RemoveChild( uint8* extAddr, uint8 dealloc );

extern uint8 nwk_multicast( NLDE_FrameFormat_t* ff );

extern void nwkNeighborInitTable( void );


/****************************************************************************
****************************************************************************/
#ifdef __cplusplus
}
#endif

#endif /* NWK_UTIL_H */


