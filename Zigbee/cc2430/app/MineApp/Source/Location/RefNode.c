/**************************************************************************************************
  Filename:       RefNode.c
  Revised:        $Date: 2009/04/27 19:02:49 $
  Revision:       $Revision: 1.2 $

  Description:    Reference Node for the Z-Stack Location Profile.


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

/*********************************************************************
 * INCLUDES
 */

#include "OSAL.h"
#include "OSAL_Nv.h"
#include "MTEL.h"
#include "AF.h"
#include "ZDApp.h"

#include "OnBoard.h"
#include "hal_key.h"
#if defined ( LCD_SUPPORTED )
#include "hal_lcd.h"
#endif

#include "LocationProfile.h"
#include "RefNode.h"
#include "MineApp.h"
/*********************************************************************
 * CONSTANTS
 */

/*********************************************************************
 * TYPEDEFS
 */

typedef struct
{
  uint16 x;
  uint16 y;
} refNode_Config_t;

/* TBD - could put a timestamp in the blastAcc_t so timeout old blasts that
 *       were not followed up with an RSSI request. This happens when the
 *       unruly broadcast mechanism fails to head the request for 0 retries.
 *       After Blind Node has finished blasting and then collecting the RSSI's
 *       it is seen on the sniffer that some of the original blasts are getting
 *       re-transmitted.
 */
typedef struct _blastAcc_t
{
  struct _blastAcc_t *next;
  uint16 addr;
  uint16 acc;
  byte cnt;
} blastAcc_t;

/*********************************************************************
 * GLOBAL VARIABLES
 */

byte RefNode_TaskID;

/*********************************************************************
 * LOCAL VARIABLES
 */

static const cId_t RefNode_InputClusterList[] =
{
  LOCATION_XY_RSSI_REQUEST,
  LOCATION_RSSI_BLAST,
  //LOCATION_REFNODE_CONFIG,
  //LOCATION_REFNODE_REQUEST_CONFIG,
};

static const cId_t RefNode_OutputClusterList[] =
{
  LOCATION_XY_RSSI_RESPONSE,
  //LOCATION_REFNODE_CONFIG
};

static const SimpleDescriptionFormat_t RefNode_SimpleDesc =
{
  LOCATION_REFNODE_ENDPOINT,
  LOCATION_PROFID,

  LOCATION_REFNODE_DEVICEID,

  LOCATION_DEVICE_VERSION,
  LOCATION_FLAGS,

  sizeof(RefNode_InputClusterList),
  (cId_t*)RefNode_InputClusterList,

  sizeof(RefNode_OutputClusterList),
  (cId_t*)RefNode_OutputClusterList
};

static const endPointDesc_t RefNodeepDesc =
{
  LOCATION_REFNODE_ENDPOINT,
  &RefNode_TaskID,
  (SimpleDescriptionFormat_t *)&RefNode_SimpleDesc,
  noLatencyReqs
};

static afAddrType_t cfgAddr;
static afAddrType_t rspAddr;

static byte rspMsg[LOCATION_XY_RSSI_LEN];
static byte RefNodetransId;

static blastAcc_t *blastPtr;

/*********************************************************************
 * LOCAL FUNCTIONS
 */

static void RefNodeprocessMSGCmd( afIncomingMSGPacket_t *pkt );
#if 0
static void RefNodeparseConfig( byte *msg );
#endif

static void addBlast( uint16 addr, byte rssi );
static void rssiRsp( afIncomingMSGPacket_t *pkt );
static void RefNodeLocationHandleKeys( byte keys );

/*********************************************************************
 * @fn      RefNode_Init
 *
 * @brief   Initialization function for this OSAL task.
 *
 * @param   task_id - the ID assigned by OSAL.
 *
 * @return  none
 */
void RefNode_Init( byte task_id )
{
  refNode_Config_t config;

  RefNode_TaskID = task_id;

  config.x = LOC_DEFAULT_X_Y;
  config.y = LOC_DEFAULT_X_Y;

  if ( ZSUCCESS == osal_nv_item_init( LOC_NV_REFNODE_CONFIG, sizeof(refNode_Config_t), &config ) )
  {
    osal_nv_read( LOC_NV_REFNODE_CONFIG, 0, sizeof( refNode_Config_t ), &config );
  }
  else
  {
    osal_nv_write( LOC_NV_REFNODE_CONFIG, 0, sizeof( refNode_Config_t ), &config );
  }

  rspMsg[REFNODE_CONFIG_X_LO_IDX] = LO_UINT16( config.x );
  rspMsg[REFNODE_CONFIG_X_HI_IDX] = HI_UINT16( config.x );
  rspMsg[REFNODE_CONFIG_Y_LO_IDX] = LO_UINT16( config.y );
  rspMsg[REFNODE_CONFIG_Y_HI_IDX] = HI_UINT16( config.y );

  cfgAddr.addrMode = afAddrNotPresent;
  rspAddr.addrMode = afAddrNotPresent;

  // Register the endpoint/interface description with the AF.
  afRegister( (endPointDesc_t *)&RefNodeepDesc );

  // Register for all key events - This app will handle all key events.
  // RegisterForKeys( RefNode_TaskID );

#if defined ( LCD_SUPPORTED )
  HalLcdWriteString( "Location-RefNode", HAL_LCD_LINE_2 );
#endif
}

/*********************************************************************
 * @fn      RefNode_ProcessEvent
 *
 * @brief   Generic Application Task event processor.
 *
 * @param   task_id  - The OSAL assigned task ID.
 * @param   events - Bit map of events to process.
 *
 * @return  none
 */
uint16 RefNode_ProcessEvent( byte task_id, uint16 events )
{
  if ( events & SYS_EVENT_MSG )
  {
    afIncomingMSGPacket_t *MSGpkt;

    while ((MSGpkt = (afIncomingMSGPacket_t *)osal_msg_receive(RefNode_TaskID)))
    {
      switch ( MSGpkt->hdr.event )
      {
      case KEY_CHANGE:
        //LocationHandleKeys( ((keyChange_t *)MSGpkt)->state, ((keyChange_t *)MSGpkt)->keys );
        break;

      case AF_DATA_CONFIRM_CMD:
        break;

      case AF_INCOMING_MSG_CMD:
        RefNodeprocessMSGCmd( MSGpkt );
        break;

      case ZDO_STATE_CHANGE:
        osal_start_timerEx( RefNode_TaskID, ANNCE_EVT, ANNCE_DELAY );
        break;

      default:
        break;
      }

      osal_msg_deallocate( (uint8 *)MSGpkt );
    }

    // Return unprocessed events.
    return ( events ^ SYS_EVENT_MSG );
  }

#if 0
  if ( events & ANNCE_EVT )
  {
    /* Broadcast the X,Y location for any passive listeners in order to
     * register this node.
     */
    afAddrType_t dstAddr;

    dstAddr.addrMode = afAddrBroadcast;
    dstAddr.addr.shortAddr = NWK_BROADCAST_SHORTADDR_DEVALL;
    dstAddr.endPoint = MINEAPP_ENDPOINT;

    (void)AF_DataRequest( &dstAddr, (endPointDesc_t *)&RefNodeepDesc,
                           LOCATION_REFNODE_CONFIG, REFNODE_CONFIG_LEN, rspMsg,
                          &RefNodetransId, 0, AF_DEFAULT_RADIUS );

    return ( events ^ ANNCE_EVT );
  }
#endif

  return 0;  // Discard unknown events
}

/*********************************************************************
 * @fn      RefNodeLocationHandleKeys
 *
 * @brief   Handles key events for this device.
 *
 * @param   shift - true if in shift/alt.
 *
 * @return  none
 */
void RefNodeLocationHandleKeys( byte keys )
{
/*
  if ( keys & HAL_KEY_SW_1 )
  {
    rspMsg[REFNODE_CONFIG_X_LO_IDX] = 35;
    rspMsg[REFNODE_CONFIG_X_HI_IDX] = 0;
    rspMsg[REFNODE_CONFIG_Y_LO_IDX] = 35;
    rspMsg[REFNODE_CONFIG_Y_HI_IDX] = 0;
  }

  if ( keys & HAL_KEY_SW_2 )
  {
    rspMsg[REFNODE_CONFIG_X_LO_IDX] = 55;
    rspMsg[REFNODE_CONFIG_X_HI_IDX] = 0;
    rspMsg[REFNODE_CONFIG_Y_LO_IDX] = 55;
    rspMsg[REFNODE_CONFIG_Y_HI_IDX] = 0;
  }

  if ( keys & HAL_KEY_SW_3 )
  {
    rspMsg[REFNODE_CONFIG_X_LO_IDX] = 85;
    rspMsg[REFNODE_CONFIG_X_HI_IDX] = 0;
    rspMsg[REFNODE_CONFIG_Y_LO_IDX] = 85;
    rspMsg[REFNODE_CONFIG_Y_HI_IDX] = 0;
  }

  if ( keys & HAL_KEY_SW_4 )
  {
    osal_set_event( RefNode_TaskID, ANNCE_EVT );
  }
  */
}

/*********************************************************************
 * @fn      RefNodeprocessMSGCmd
 *
 * @brief   Data message processor callback.
 *
 * @param   none
 *
 * @return  none
 */
static void RefNodeprocessMSGCmd( afIncomingMSGPacket_t *pkt )
{
  switch ( pkt->clusterId )
  {
  case LOCATION_XY_RSSI_REQUEST:
    rssiRsp( pkt );
    break;
/*
  case LOCATION_REFNODE_CONFIG:
    RefNodeparseConfig( pkt->cmd.Data );
    break;

  case LOCATION_REFNODE_REQUEST_CONFIG:
    pkt->srcAddr.addrMode = afAddr16Bit;
    (void)AF_DataRequest( &pkt->srcAddr, (endPointDesc_t *)&RefNodeepDesc,
                           LOCATION_REFNODE_CONFIG, REFNODE_CONFIG_LEN, rspMsg,
                          &RefNodetransId, 0, AF_DEFAULT_RADIUS );
    break;
*/
  case LOCATION_RSSI_BLAST:
    addBlast( pkt->srcAddr.addr.shortAddr, pkt->LinkQuality );
    break;

  default:
    break;
  }
}

#if 0
/*********************************************************************
 * @fn      RefNodeparseConfig
 *
 * @brief   Parse message and populate the configuration struct
 *
 * @param   none
 *
 * @return  none
 */
static void RefNodeparseConfig( byte *msg )
{
  refNode_Config_t config;

  config.x = BUILD_UINT16( msg[REFNODE_CONFIG_X_LO_IDX],
                           msg[REFNODE_CONFIG_X_HI_IDX] );
  config.y = BUILD_UINT16( msg[REFNODE_CONFIG_Y_LO_IDX],
                           msg[REFNODE_CONFIG_Y_HI_IDX] );
  osal_nv_write( LOC_NV_REFNODE_CONFIG, 0, sizeof(refNode_Config_t), &config );

  rspMsg[REFNODE_CONFIG_X_LO_IDX] = msg[REFNODE_CONFIG_X_LO_IDX];
  rspMsg[REFNODE_CONFIG_X_HI_IDX] = msg[REFNODE_CONFIG_X_HI_IDX];
  rspMsg[REFNODE_CONFIG_Y_LO_IDX] = msg[REFNODE_CONFIG_Y_LO_IDX];
  rspMsg[REFNODE_CONFIG_Y_HI_IDX] = msg[REFNODE_CONFIG_Y_HI_IDX];
}
#endif

/*********************************************************************
 * @fn      addBlast
 *
 * @brief   Add or initialize an RSSI blast for the given Network Address.
 *
 * @param   uint16 - Network address of the Blind Node blasting.
 *          byte   - RSSI of the blast message received.
 *
 * @return  none
 */
static void addBlast( uint16 addr, byte rssi )
{
  blastAcc_t *ptr = blastPtr;

  while ( ptr )
  {
    if ( ptr->addr == addr )
    {
      break;
    }
    ptr = ptr->next;
  }

  if ( ptr )
  {
    ptr->acc += rssi;
    ptr->cnt++;
  }
  else
  {
    ptr = (blastAcc_t *)osal_mem_alloc( sizeof( blastAcc_t ) );

    if ( ptr )
    {
      ptr->next = blastPtr;
      blastPtr = ptr;

      ptr->addr = addr;
      ptr->acc = rssi;
      ptr->cnt = 1;
    }
  }
}

/*********************************************************************
 * @fn      rssiRsp
 *
 * @brief   Respond to requester with average of their RSSI blasts.
 *
 * @param   uint16 - Network address of the Blind Node requesting.
 *          byte   - Endpoint of the Blind Node requesting.
 *          byte   - RSSI of the blast message received.
 *
 * @return  none
 */
static void rssiRsp( afIncomingMSGPacket_t *pkt )
{
  blastAcc_t *ptr = blastPtr;
  blastAcc_t *prev = NULL;
  byte options, radius;

  while ( ptr )
  {
    if ( ptr->addr == pkt->srcAddr.addr.shortAddr )
    {
      break;
    }
    prev = ptr;
    ptr = ptr->next;
  }

  if ( ptr )
  {
    rspMsg[LOCATION_XY_RSSI_RSSI_IDX] =
      (ptr->acc + pkt->LinkQuality) / (ptr->cnt + 1);

    if ( prev )
    {
      prev->next = ptr->next;
    }
    else
    {
      blastPtr = ptr->next;
    }
    osal_mem_free( ptr );

    options = AF_SKIP_ROUTING;
    radius = 1;
  }
  else
  {
    rspMsg[LOCATION_XY_RSSI_RSSI_IDX] = pkt->LinkQuality;

    options = AF_TX_OPTIONS_NONE;
    radius = AF_DEFAULT_RADIUS;
  }

  pkt->srcAddr.addrMode = afAddr16Bit;
  (void)AF_DataRequest( &pkt->srcAddr, (endPointDesc_t *)&RefNodeepDesc,
                         LOCATION_XY_RSSI_RESPONSE, LOCATION_XY_RSSI_LEN,
                         rspMsg, &RefNodetransId, options, radius );
}

/*********************************************************************
*********************************************************************/
