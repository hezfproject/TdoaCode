/**************************************************************************************************
  Filename:       rtg.h
  Revised:        $Date: 2009/02/07 01:08:32 $
  Revision:       $Revision: 1.1.1.1 $

  Description:    Interface to mesh routing functions


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

#ifndef RTG_H
#define RTG_H

#ifdef __cplusplus
extern "C"
{
#endif

#if defined (RTR_NWK)

/*********************************************************************
 * INCLUDES
 */

#include "ZComDef.h"
#include "nwk_util.h"
#include "nwk_bufs.h"

/*********************************************************************
 * MACROS
 */

/*********************************************************************
 * CONSTANTS
 */

#define RTG_TIMER_INTERVAL            1000

/*********************************************************************
 * TYPEDEFS
 */

typedef enum
{
  RTG_SUCCESS,
  RTG_FAIL,
  RTG_TBL_FULL,
  RTG_HIGHER_COST,
  RTG_NO_ENTRY,
  RTG_INVALID_PATH,
  RTG_INVALID_PARAM
} RTG_Status_t;

// status values for routing entries
#define RT_INIT       0
#define RT_ACTIVE     1
#define RT_DISC       2
#define RT_LINK_FAIL  3
#define RT_REPAIR     4

// Routing table entry
//   Notice, if you change this structure, you must also change
//   rtgItem_t in ZDProfile.h
typedef struct
{
  uint16  dstAddress;
  uint16  nextHopAddress;
  byte    expiryTime;
  byte    status;
} rtgEntry_t;

// Route discovery table entry
typedef struct
{
  byte    rreqId;
  uint16  srcAddress;
  uint16  previousNode;
  byte    forwardCost;
  byte    residualCost;
  byte    expiryTime;
} rtDiscEntry_t;

// Broadcast table entry.
typedef struct
{
  uint16 srcAddr;
  uint8  bdt; // broadcast delivery time
  uint8  pat; // passive ack timeout
  uint8  mbr; // max broadcast retries
  uint8  handle;
  // Count of non-sleeping neighbors and router children.
  uint8  ackCnt;
  uint8  id;
} bcastEntry_t;

/*********************************************************************
 * GLOBAL VARIABLES
 */

extern rtgEntry_t rtgTable[];
extern rtDiscEntry_t rtDiscTable[];

/*********************************************************************
 * FUNCTIONS
 */

extern void RTG_Init( void );

extern byte RTG_GetRtgEntry( uint16 DstAddress );

extern RTG_Status_t RTG_RemoveRtgEntry( uint16 DstAddress );

extern uint16 RTG_GetNextHop( uint16 DstAddress );

extern byte RTG_ProcessRreq(
           NLDE_FrameFormat_t *ff, uint16 macSrcAddress, uint16 *nextHopAddr );

extern void RTG_ProcessRrep( NLDE_FrameFormat_t *ff );

extern void RTG_ProcessRErr( NLDE_FrameFormat_t *ff );

extern void RTG_TimerEvent( void );

extern uint16 RTG_AllocNewAddress( byte deviceType );

extern void RTG_DeAllocAddress( uint16 shortAddr );

extern void RTG_BcastTimerHandler( void );

extern byte RTG_BcastChk( NLDE_FrameFormat_t *ff, uint16 macSrcAddr );

extern byte RTG_BcastAdd(NLDE_FrameFormat_t*ff, uint16 macSrcAddr, byte handle);

extern void RTG_BcastDel( byte handle );

extern void RTG_DataReq( ZMacPollInd_t *param );

extern byte RTG_PoolAdd( NLDE_FrameFormat_t *ff );

extern uint16 RTG_GetTreeRoute( uint16 dstAddress );

extern RTG_Status_t RTG_CheckRtStatus( uint16 DstAddress, byte RtStatus );

extern uint8 RTG_ProcessRtDiscBits( uint8 rtDiscFlag, uint16 dstAddress );

extern uint8 RTG_RouteMaintanence( uint16 DstAddress, uint16 SrcAddress );

extern void RTG_FillCSkipTable( byte *children, byte *routers,
                                byte depth, uint16 *pTbl );


extern uint8 RTG_IsAncestor( uint16 deviceAddress  );

#endif// RTR_NWK

/*********************************************************************
*********************************************************************/
#ifdef __cplusplus
}
#endif

#endif /* RTG_H */
