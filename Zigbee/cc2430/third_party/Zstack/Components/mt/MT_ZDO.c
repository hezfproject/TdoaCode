/**************************************************************************************************
  Filename:       MT_ZDO.c
  Revised:        $Date: 2009/02/07 01:08:32 $
  Revision:       $Revision: 1.1.1.1 $

  Description:    MonitorTest functions for the ZDO layer.


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

#ifdef MT_ZDO_FUNC

/*********************************************************************
 * INCLUDES
 */
#include "ZComDef.h"
#include "OSAL.h"
#include "MTEL.h"
#include "MT_ZDO.h"
#include "APSMEDE.h"
#include "ZDConfig.h"
#include "ZDProfile.h"
#include "ZDObject.h"
#include "ZDApp.h"

#if !defined( WIN32 )
  #include "OnBoard.h"
#endif

#include "nwk_util.h"

/*********************************************************************
 * MACROS
 */

/*********************************************************************
 * CONSTANTS
 */

/*********************************************************************
 * TYPEDEFS
 */

/*********************************************************************
 * GLOBAL VARIABLES
 */
uint32 _zdoCallbackSub;

/*********************************************************************
 * EXTERNAL VARIABLES
 */

/*********************************************************************
 * EXTERNAL FUNCTIONS
 */

/*********************************************************************
 * LOCAL VARIABLES
 */
uint8 mtzdoResponseBuffer[100];

/*********************************************************************
 * LOCAL FUNCTIONS
 */
byte *zdo_MT_MakeExtAddr( zAddrType_t *devAddr, byte *pData );
byte *zdo_MT_CopyRevExtAddr( byte *dstMsg, byte *addr );

uint8 zdo_MTCB_NwkIEEEAddrRspCB( zdoIncomingMsg_t *inMsg, uint8 *msg );
uint8 zdo_MTCB_NodeDescRspCB( zdoIncomingMsg_t *inMsg, uint8 *msg );
uint8 zdo_MTCB_PowerDescRspCB( zdoIncomingMsg_t *inMsg, uint8 *msg );
uint8 zdo_MTCB_SimpleDescRspCB( zdoIncomingMsg_t *inMsg, uint8 *msg );
uint8 zdo_MTCB_MatchActiveEPRspCB( zdoIncomingMsg_t *inMsg, uint8 *msg );
uint8 zdo_MTCB_BindRspCB( zdoIncomingMsg_t *inMsg, uint8 *msg );
uint8 zdo_MTCB_MgmtNwkDiscRspCB( zdoIncomingMsg_t *inMsg, uint8 *msg );
uint8 zdo_MTCB_MgmtLqiRspCB( zdoIncomingMsg_t *inMsg, uint8 *msg );
uint8 zdo_MTCB_MgmtRtgRspCB( zdoIncomingMsg_t *inMsg, uint8 *msg );
uint8 zdo_MTCB_MgmtBindRspCB( zdoIncomingMsg_t *inMsg, uint8 *msg );
uint8 zdo_MTCB_MgmtDirectJoinRspCB( zdoIncomingMsg_t *inMsg, uint8 *msg );
uint8 zdo_MTCB_MgmtLeaveRspCB( zdoIncomingMsg_t *inMsg, uint8 *msg );
uint8 zdo_MTCB_MgmtPermitJoinRspCB( zdoIncomingMsg_t *inMsg, uint8 *msg );
uint8 zdo_MTCB_UserDescRspCB( zdoIncomingMsg_t *inMsg, uint8 *msg );
uint8 zdo_MTCB_UserDescConfCB( zdoIncomingMsg_t *inMsg, uint8 *msg );
uint8 zdo_MTCB_ServerDiscRspCB( zdoIncomingMsg_t *inMsg, uint8 *msg );

/*********************************************************************
 * Callback Table
 */
typedef uint8 (*pfnMtZdoRspProc)( zdoIncomingMsg_t *inMsg, uint8 *buf );

typedef struct
{
  uint16            clusterID;
  uint32            subCBID;    // Subscription bit
  uint16            mtID;       // SPI message ID
  pfnMtZdoRspProc   pFn;
} MTZDO_ConversionItem_t;

CONST MTZDO_ConversionItem_t mtzdoConvTable[] = 
{
#if defined ( ZDO_NWKADDR_REQUEST )
  {NWK_addr_rsp, CB_ID_ZDO_NWK_ADDR_RSP, SPI_CB_ZDO_NWK_ADDR_RSP, zdo_MTCB_NwkIEEEAddrRspCB},
#endif  
#if defined ( ZDO_IEEEADDR_REQUEST )
  {IEEE_addr_rsp, CB_ID_ZDO_IEEE_ADDR_RSP, SPI_CB_ZDO_IEEE_ADDR_RSP, zdo_MTCB_NwkIEEEAddrRspCB},
#endif  
#if defined ( ZDO_NODEDESC_REQUEST )
  {Node_Desc_rsp, CB_ID_ZDO_NODE_DESC_RSP, SPI_CB_ZDO_NODE_DESC_RSP, zdo_MTCB_NodeDescRspCB},
#endif
#if defined ( ZDO_POWERDESC_REQUEST )
  {Power_Desc_rsp, CB_ID_ZDO_POWER_DESC_RSP, SPI_CB_ZDO_POWER_DESC_RSP, zdo_MTCB_PowerDescRspCB},
#endif
#if defined ( ZDO_SIMPLEDESC_REQUEST )
  {Simple_Desc_rsp, CB_ID_ZDO_SIMPLE_DESC_RSP, SPI_CB_ZDO_SIMPLE_DESC_RSP, zdo_MTCB_SimpleDescRspCB},
#endif
#if defined ( ZDO_ACTIVEEP_REQUEST )
  {Active_EP_rsp, CB_ID_ZDO_ACTIVE_EPINT_RSP, SPI_CB_ZDO_ACTIVE_EPINT_RSP, zdo_MTCB_MatchActiveEPRspCB},
#endif
#if defined ( ZDO_MATCH_REQUEST )
  {Match_Desc_rsp, CB_ID_ZDO_MATCH_DESC_RSP, SPI_CB_ZDO_MATCH_DESC_RSP, zdo_MTCB_MatchActiveEPRspCB},
#endif
#if defined ( ZDO_BIND_UNBIND_REQUEST )
  {Bind_rsp, CB_ID_ZDO_BIND_RSP, SPI_CB_ZDO_BIND_RSP, zdo_MTCB_BindRspCB},
  {Unbind_rsp, CB_ID_ZDO_UNBIND_RSP, SPI_CB_ZDO_UNBIND_RSP, zdo_MTCB_BindRspCB},
#endif
#if defined ( ZDO_ENDDEVICEBIND_REQUEST )
  {End_Device_Bind_rsp, CB_ID_ZDO_END_DEVICE_BIND_RSP, SPI_CB_ZDO_END_DEVICE_BIND_RSP, zdo_MTCB_BindRspCB},
#endif  
#if defined ( ZDO_USERDESC_REQUEST )
  {User_Desc_rsp, CB_ID_ZDO_USER_DESC_RSP, SPI_CB_ZDO_USER_DESC_RSP, zdo_MTCB_UserDescRspCB},
#endif
#if defined ( ZDO_USERDESCSET_REQUEST )
  {User_Desc_conf, CB_ID_ZDO_USER_DESC_CONF, SPI_CB_ZDO_USER_DESC_CNF, zdo_MTCB_UserDescConfCB},
#endif
#if defined ( ZDO_SERVERDISC_REQUEST )
  {Server_Discovery_rsp, CB_ID_ZDO_SERVERDISC_RSP, SPI_CB_ZDO_SERVERDISC_RSP, zdo_MTCB_ServerDiscRspCB},
#endif
#if defined ( ZDO_MGMT_NWKDISC_REQUEST )
  {Mgmt_NWK_Disc_rsp, CB_ID_ZDO_MGMT_NWKDISC_RSP, SPI_CB_ZDO_MGMT_NWKDISC_RSP, zdo_MTCB_MgmtNwkDiscRspCB},
#endif
#if defined ( ZDO_MGMT_LQI_REQUEST )
  {Mgmt_Lqi_rsp, CB_ID_ZDO_MGMT_LQI_RSP, SPI_CB_ZDO_MGMT_LQI_RSP, zdo_MTCB_MgmtLqiRspCB},
#endif
#if defined ( ZDO_MGMT_RTG_REQUEST )
  {Mgmt_Rtg_rsp, CB_ID_ZDO_MGMT_RTG_RSP, SPI_CB_ZDO_MGMT_RTG_RSP, zdo_MTCB_MgmtRtgRspCB},
#endif
#if defined ( ZDO_MGMT_BIND_REQUEST )
  {Mgmt_Bind_rsp, CB_ID_ZDO_MGMT_BIND_RSP, SPI_CB_ZDO_MGMT_BIND_RSP, zdo_MTCB_MgmtBindRspCB},
#endif
#if defined ( ZDO_MGMT_LEAVE_REQUEST )
  {Mgmt_Leave_rsp, CB_ID_ZDO_MGMT_LEAVE_RSP, SPI_CB_ZDO_MGMT_LEAVE_RSP, zdo_MTCB_MgmtLeaveRspCB},
#endif
#if defined ( ZDO_MGMT_JOINDIRECT_REQUEST )
  {Mgmt_Direct_Join_rsp, CB_ID_ZDO_MGMT_DIRECT_JOIN_RSP, SPI_CB_ZDO_MGMT_DIRECT_JOIN_RSP, zdo_MTCB_MgmtDirectJoinRspCB},
#endif
#if defined ( ZDO_MGMT_PERMIT_JOIN_REQUEST )
  {Mgmt_Permit_Join_rsp, CB_ID_ZDO_MGMT_PERMIT_JOIN_RSP, SPI_CB_ZDO_MGMT_PERMIT_JOIN_RSP, zdo_MTCB_MgmtPermitJoinRspCB},
#endif
  {0, 0, 0, NULL}
};

/*********************************************************************
 * @fn      MT_ZdoCommandProcessing
 *
 * @brief
 *
 *   Process all the ZDO commands that are issued by test tool
 *
 * @param   cmd_id - Command ID
 * @param   len    - Length of received SPI data message
 * @param   pData  - pointer to received SPI data message
 *
 * @return  void
 */
void MT_ZdoCommandProcessing( uint16 cmd_id , byte len , byte *pData )
{
  byte i;
  byte x;
  byte ret;
  byte attr;
  byte attr1;
  uint16 cID;
  uint16 shortAddr;
  uint16 uAttr;
  byte *ptr;
  byte *ptr1;
  zAddrType_t devAddr;
  zAddrType_t dstAddr;
  byte respLen;
#if defined ( ZDO_MGMT_NWKDISC_REQUEST )
  uint32 scanChans;
#endif
#if defined ( ZDO_USERDESCSET_REQUEST )
  UserDescriptorFormat_t userDesc;
#endif

  ret = UNSUPPORTED_COMMAND;
  len = SPI_0DATA_MSG_LEN + SPI_RESP_LEN_ZDO_DEFAULT;
  respLen = SPI_RESP_LEN_ZDO_DEFAULT;

  switch (cmd_id)
  {
    case SPI_CMD_ZDO_AUTO_FIND_DESTINATION_REQ:
    case SPI_CMD_ZDO_AUTO_ENDDEVICEBIND_REQ:
      //Not supported anymore
      ret = ZFailure;
      break;

#if defined ( ZDO_NWKADDR_REQUEST )
    case SPI_CMD_ZDO_NWK_ADDR_REQ:
      // Copy and flip incoming 64-bit address
      pData = zdo_MT_MakeExtAddr( &devAddr, pData );

      ptr = (byte*)&devAddr.addr.extAddr;

      attr = *pData++;   // RequestType
      attr1 = *pData++;  // StartIndex
      x = *pData;
      ret = (byte)ZDP_NwkAddrReq( ptr, attr, attr1, x );
      break;
#endif

#if defined ( ZDO_IEEEADDR_REQUEST )
    case SPI_CMD_ZDO_IEEE_ADDR_REQ:
      shortAddr = BUILD_UINT16( pData[1], pData[0] );
      pData += sizeof( shortAddr );
      attr = *pData++;   // RequestType
      attr1 = *pData++;  // StartIndex
      x = *pData;        // SecuritySuite
      ret = (byte)ZDP_IEEEAddrReq( shortAddr, attr, attr1, x );
      break;
#endif

#if defined ( ZDO_NODEDESC_REQUEST )
    case SPI_CMD_ZDO_NODE_DESC_REQ:
      // destination address
      devAddr.addrMode = Addr16Bit;
      devAddr.addr.shortAddr = BUILD_UINT16( pData[1], pData[0] );
      pData += 2;

      // Network address of interest
      shortAddr = BUILD_UINT16( pData[1], pData[0] );
      pData += 2;

      attr = *pData;
      ret = (byte)ZDP_NodeDescReq( &devAddr, shortAddr, attr );
      break;
#endif

#if defined ( ZDO_POWERDESC_REQUEST )
    case SPI_CMD_ZDO_POWER_DESC_REQ:
      // destination address
      devAddr.addrMode = Addr16Bit;
      devAddr.addr.shortAddr = BUILD_UINT16( pData[1], pData[0] );
      pData += 2;

      // Network address of interest
      shortAddr = BUILD_UINT16( pData[1], pData[0] );
      pData += 2;

      attr = *pData;
      ret = (byte)ZDP_PowerDescReq( &devAddr, shortAddr, attr );
      break;
#endif

#if defined ( ZDO_SIMPLEDESC_REQUEST )
    case SPI_CMD_ZDO_SIMPLE_DESC_REQ:
      // destination address
      devAddr.addrMode = Addr16Bit;
      devAddr.addr.shortAddr = BUILD_UINT16( pData[1], pData[0] );
      pData += 2;

      // Network address of interest
      shortAddr = BUILD_UINT16( pData[1], pData[0] );
      pData += 2;

      attr = *pData++;  // endpoint/interface
      attr1 = *pData;   // SecuritySuite
      ret = (byte)ZDP_SimpleDescReq( &devAddr, shortAddr, attr, attr1 );
      break;
#endif

#if defined ( ZDO_ACTIVEEP_REQUEST )
    case SPI_CMD_ZDO_ACTIVE_EPINT_REQ:
      // destination address
      devAddr.addrMode = Addr16Bit;
      devAddr.addr.shortAddr = BUILD_UINT16( pData[1], pData[0] );
      pData += 2;

      // Network address of interest
      shortAddr = BUILD_UINT16( pData[1], pData[0] );
      pData += 2;

      attr = *pData;  // SecuritySuite
      ret = (byte)ZDP_ActiveEPReq( &devAddr, shortAddr, attr );
      break;
#endif

#if defined ( ZDO_MATCH_REQUEST )
    case SPI_CMD_ZDO_MATCH_DESC_REQ:
      {
        uint16 inC[16], outC[16];

        // destination address
        devAddr.addrMode = Addr16Bit;
        devAddr.addr.shortAddr = BUILD_UINT16( pData[1], pData[0] );
        pData += 2;

        // Network address of interest
        shortAddr = BUILD_UINT16( pData[1], pData[0] );
        pData += 2;

        uAttr = BUILD_UINT16( pData[1], pData[0] );   // Profile ID
        pData += 2;

        attr = *pData++;   // NumInClusters
        for (i=0; i<16; ++i)  {
          inC[i] = BUILD_UINT16(pData[1], pData[0]);
          pData += 2;
        }

        attr1 = *pData++;  // NumOutClusters
        for (i=0; i<16; ++i)  {
          outC[i] = BUILD_UINT16(pData[1], pData[0]);
          pData += 2;
        }

        i = *pData;        // SecuritySuite

        ret = (byte)ZDP_MatchDescReq( &devAddr, shortAddr, uAttr,
                                  attr, inC, attr1, outC, i );
      }
      break;
#endif

#if defined ( ZDO_COMPLEXDESC_REQUEST )
    case SPI_CMD_ZDO_COMPLEX_DESC_REQ:
      // destination address
      devAddr.addrMode = Addr16Bit;
      devAddr.addr.shortAddr = BUILD_UINT16( pData[1], pData[0] );
      pData += 2;

      // Network address of interest
      shortAddr = BUILD_UINT16( pData[1], pData[0] );
      pData += 2;

      attr = *pData;  // SecuritySuite
      ret = (byte)ZDP_ComplexDescReq( &devAddr, shortAddr, attr );
      break;
#endif

#if defined ( ZDO_USERDESC_REQUEST )
    case SPI_CMD_ZDO_USER_DESC_REQ:
      // destination address
      devAddr.addrMode = Addr16Bit;
      devAddr.addr.shortAddr = BUILD_UINT16( pData[1], pData[0] );
      pData += 2;

      // Network address of interest
      shortAddr = BUILD_UINT16( pData[1], pData[0] );
      pData += 2;

      attr = *pData;  // SecuritySuite
      ret = (byte)ZDP_UserDescReq( &devAddr, shortAddr, attr );
      break;
#endif

#if defined ( ZDO_ENDDEVICEBIND_REQUEST )
    case SPI_CMD_ZDO_END_DEV_BIND_REQ:
      //TODO: When ZTool supports 16 bits the code below will need to take it into account
      {
        uint16 inC[15], outC[15];

        // destination address
        devAddr.addrMode = Addr16Bit;
        devAddr.addr.shortAddr = BUILD_UINT16( pData[1], pData[0] );
        pData += 2;

        // Local coordinator of the binding 
        shortAddr = BUILD_UINT16( pData[1], pData[0] );
        pData += 2;

        x = *pData++;      // EPInt

        uAttr = BUILD_UINT16( pData[1], pData[0] );   // Profile ID
        pData += 2;

        attr = *pData++;   // NumInClusters
        for (i=0; i<15; ++i)  {
          inC[i] = BUILD_UINT16(pData[1], pData[0]);
          pData += 2;
        }

        attr1 = *pData++;  // NumOutClusters
        for (i=0; i<15; ++i)  {
          outC[i] = BUILD_UINT16(pData[1], pData[0]);
          pData += 2;
        }

        i = *pData;        // SecuritySuite

        ret = (byte)ZDP_EndDeviceBindReq( &devAddr, shortAddr, x, uAttr,
                                attr, inC, attr1, outC, i );
      }
      break;
#endif

#if defined ( ZDO_BIND_UNBIND_REQUEST )
    case SPI_CMD_ZDO_BIND_REQ:
      // destination address
      devAddr.addrMode = Addr16Bit;
      devAddr.addr.shortAddr = BUILD_UINT16( pData[1], pData[0] );
      pData += 2;

      MT_ReverseBytes( pData, Z_EXTADDR_LEN );
      ptr = pData;       // SrcAddress
      pData += Z_EXTADDR_LEN;

      attr = *pData++;   // SrcEPInt

      cID = BUILD_UINT16( pData[1], pData[0]);      // ClusterID
      pData += 2;
      
      dstAddr.addrMode = *pData++;
      
      MT_ReverseBytes( pData, Z_EXTADDR_LEN );
      if ( dstAddr.addrMode == Addr64Bit )
      {
        ptr1 = pData;      // DstAddress
        osal_cpyExtAddr( dstAddr.addr.extAddr, ptr1 );
      }
      else
      {
        dstAddr.addr.shortAddr = BUILD_UINT16( pData[0], pData[1] ); 
      }
      
      // The short address occupies lsb two bytes
      pData += Z_EXTADDR_LEN;

      
      attr1 = *pData++;  // DstEPInt

      x = *pData;        // SecuritySuite
     
      ret = (byte)ZDP_BindReq( &devAddr, ptr, attr, cID, &dstAddr, attr1, x );
      break;
#endif

#if defined ( ZDO_BIND_UNBIND_REQUEST )
    case SPI_CMD_ZDO_UNBIND_REQ:
      // destination address
      devAddr.addrMode = Addr16Bit;
      devAddr.addr.shortAddr = BUILD_UINT16( pData[1], pData[0] );
      pData += 2;

      MT_ReverseBytes( pData, Z_EXTADDR_LEN );
      ptr = pData;       // SrcAddress
      pData += Z_EXTADDR_LEN;

      attr = *pData++;   // SrcEPInt

      cID = BUILD_UINT16( pData[1], pData[0]);      // ClusterID
      pData += 2;

      dstAddr.addrMode = *pData++;
      
      MT_ReverseBytes( pData, Z_EXTADDR_LEN );
      if ( dstAddr.addrMode == Addr64Bit )
      {
        ptr1 = pData;      // DstAddress
        osal_cpyExtAddr( dstAddr.addr.extAddr, ptr1 );
      }
      else
      {
        dstAddr.addr.shortAddr = BUILD_UINT16( pData[0], pData[1] ); 
      }      
      pData += Z_EXTADDR_LEN;

      attr1 = *pData++;  // DstEPInt

      x = *pData;        // SecuritySuite

      ret = (byte)ZDP_UnbindReq( &devAddr, ptr, attr, cID, &dstAddr, attr1, x );
      break;
#endif

#if defined ( ZDO_MGMT_NWKDISC_REQUEST )
    case SPI_CMD_ZDO_MGMT_NWKDISC_REQ:
      devAddr.addrMode = Addr16Bit;
      devAddr.addr.shortAddr = BUILD_UINT16( pData[1], pData[0] );
      pData += 2;
      scanChans = BUILD_UINT32( pData[3], pData[2], pData[1], pData[0] );
      ret = (byte)ZDP_MgmtNwkDiscReq( &devAddr, scanChans, pData[4], pData[5], false );
      break;
#endif

#if defined ( ZDO_MGMT_LQI_REQUEST )
    case SPI_CMD_ZDO_MGMT_LQI_REQ:
      devAddr.addrMode = Addr16Bit;
      devAddr.addr.shortAddr = BUILD_UINT16( pData[1], pData[0] );
      ret = (byte)ZDP_MgmtLqiReq( &devAddr, pData[2], false );
      break;
#endif

#if defined ( ZDO_MGMT_RTG_REQUEST )
    case SPI_CMD_ZDO_MGMT_RTG_REQ:
      devAddr.addrMode = Addr16Bit;
      devAddr.addr.shortAddr = BUILD_UINT16( pData[1], pData[0] );
      ret = (byte)ZDP_MgmtRtgReq( &devAddr, pData[2], false );
      break;
#endif

#if defined ( ZDO_MGMT_BIND_REQUEST )
    case SPI_CMD_ZDO_MGMT_BIND_REQ:
      devAddr.addrMode = Addr16Bit;
      devAddr.addr.shortAddr = BUILD_UINT16( pData[1], pData[0] );
      ret = (byte)ZDP_MgmtBindReq( &devAddr, pData[2], false );
      break;
#endif

#if defined ( ZDO_MGMT_JOINDIRECT_REQUEST )
    case SPI_CMD_ZDO_MGMT_DIRECT_JOIN_REQ:
      devAddr.addrMode = Addr16Bit;
      devAddr.addr.shortAddr = BUILD_UINT16( pData[1], pData[0] );
      MT_ReverseBytes( &pData[2], Z_EXTADDR_LEN );
      ret = (byte)ZDP_MgmtDirectJoinReq( &devAddr,
                               &pData[2],
                               pData[2 + Z_EXTADDR_LEN],
                               false );
      break;
#endif

#if defined ( ZDO_MGMT_LEAVE_REQUEST )
    case SPI_CMD_ZDO_MGMT_LEAVE_REQ:
      devAddr.addrMode = Addr16Bit;
      devAddr.addr.shortAddr = BUILD_UINT16( pData[1], pData[0] );
      MT_ReverseBytes( &pData[2], Z_EXTADDR_LEN );
      ret = (byte)ZDP_MgmtLeaveReq( &devAddr, &pData[2], false );
      break;
#endif

#if defined ( ZDO_MGMT_PERMIT_JOIN_REQUEST )
    case SPI_CMD_ZDO_MGMT_PERMIT_JOIN_REQ:
      devAddr.addrMode = Addr16Bit;
      devAddr.addr.shortAddr = BUILD_UINT16( pData[1], pData[0] );
      ret = (byte)ZDP_MgmtPermitJoinReq( &devAddr, pData[2], pData[3], false );
      break;
#endif


#if defined ( ZDO_USERDESCSET_REQUEST )
    case SPI_CMD_ZDO_USER_DESC_SET:
      // destination address
      devAddr.addrMode = Addr16Bit;
      devAddr.addr.shortAddr = BUILD_UINT16( pData[1], pData[0] );
      pData += 2;

      // Network address of interest
      shortAddr = BUILD_UINT16( pData[1], pData[0] );
      pData += 2;

      // User descriptor
      userDesc.len = *pData++;
      osal_memcpy( userDesc.desc, pData, userDesc.len );
      pData += 16;  // len of user desc

      ret =(byte)ZDP_UserDescSet( &devAddr, shortAddr, &userDesc, pData[0] );
      break;
#endif

#if defined ( ZDO_ENDDEVICE_ANNCE_REQUEST )
    case SPI_CMD_ZDO_END_DEV_ANNCE:
      // network address
      shortAddr = BUILD_UINT16( pData[1], pData[0] );
      pData += 2;

      // extended address
      ptr = pData;
      MT_ReverseBytes( ptr, Z_EXTADDR_LEN );
      pData += Z_EXTADDR_LEN;

      // security
      attr = *pData++;

      ret = (byte)ZDP_EndDeviceAnnce( shortAddr, ptr, *pData, attr );
      break;
#endif

#if defined (ZDO_SERVERDISC_REQUEST )
    case SPI_CMD_ZDO_SERVERDISC_REQ:
      
      // Service Mask
      uAttr = BUILD_UINT16( pData[1], pData[0] );
      pData += 2;
      attr = *pData++; // Security suite
      
      ret = (byte) ZDP_ServerDiscReq( uAttr, attr );
      break;
#endif
      
#if defined (ZDO_NETWORKSTART_REQUEST )
    case SPI_CMD_ZDO_NETWORK_START_REQ:
//      NOT SUPPORTED ANYMORE
//      ret = ZDApp_StartUpFromApp( ZDAPP_STARTUP_AUTO );
      // Replaced with 
      ret = ZDOInitDevice( 100 );
      break;
    
#endif
    
    default:
      break;
  }

  MT_SendSPIRespMsg( ret, cmd_id, len, respLen );
}

/*********************************************************************
 * Utility FUNCTIONS
 */

/*********************************************************************
 * @fn      zdo_MT_CopyRevExtAddr
 *
 */
byte *zdo_MT_CopyRevExtAddr( byte *dstMsg, byte *addr )
{
  // Copy the 64-bit address
  osal_cpyExtAddr( dstMsg, addr );
  // Reverse byte order
  MT_ReverseBytes( dstMsg, Z_EXTADDR_LEN );
  // Return ptr to next destination location
  return ( dstMsg + Z_EXTADDR_LEN );
}

/*********************************************************************
 * @fn      zdo_MT_MakeExtAddr
 *
 */
byte *zdo_MT_MakeExtAddr( zAddrType_t *devAddr, byte *pData )
{
  // Define a 64-bit address
  devAddr->addrMode = Addr64Bit;
  // Copy and reverse the 64-bit address
  zdo_MT_CopyRevExtAddr( devAddr->addr.extAddr, pData );
  // Return ptr to next destination location
  return ( pData + Z_EXTADDR_LEN );
}

/*********************************************************************
 * CALLBACK FUNCTIONS
 */

/*********************************************************************
 * @fn      zdo_MTCB_NwkIEEEAddrRspCB
 *
 * @brief
 *
 *   Called by ZDO when a NWK_addr_rsp message is received.
 *
 * @param  SrcAddr  - Source address
 * @param  Status - response status
 * @param  IEEEAddr - 64 bit IEEE address of device
 * @param  nwkAddr - 16 bit network address of device
 * @param  NumAssocDev - number of associated devices to reporting device
 * @param  AssocDevList - array short addresses of associated devices
 *
 * @return  none
 */
uint8 zdo_MTCB_NwkIEEEAddrRspCB( zdoIncomingMsg_t *inMsg, uint8 *msg )
{
  uint8 len = 0;
  uint8 x;
  ZDO_NwkIEEEAddrResp_t * pRsp;
  
  pRsp = ZDO_ParseAddrRsp( inMsg );
  if ( pRsp )
  {
    //First fill in details
    if ( inMsg->srcAddr.addrMode == Addr16Bit )
    {
      *msg++ = Addr16Bit;
      for ( x = 0; x < (Z_EXTADDR_LEN - 2); x++ )
        *msg++ = 0;
      *msg++ = HI_UINT16( inMsg->srcAddr.addr.shortAddr );
      *msg++ = LO_UINT16( inMsg->srcAddr.addr.shortAddr );
    }
    else
    {
      *msg++ = Addr64Bit;
      msg = zdo_MT_CopyRevExtAddr( msg, inMsg->srcAddr.addr.extAddr );
    }
    len = Z_EXTADDR_LEN + 1;
  
    *msg++ = pRsp->status;
    msg = zdo_MT_CopyRevExtAddr( msg, pRsp->extAddr );
    len += Z_EXTADDR_LEN + 1;
  
    if ( inMsg->clusterID == NWK_addr_rsp )
    {
      *msg++ = HI_UINT16( pRsp->nwkAddr );
      *msg++ = LO_UINT16( pRsp->nwkAddr );
      len += 2;
    }
  
    *msg++ = pRsp->startIndex;
    *msg++ = pRsp->numAssocDevs;
    len += 2;

    uint8 cnt = pRsp->numAssocDevs - pRsp->startIndex;
  
    for ( x = 0; x < cnt; x++ )
    {
      *msg++ = HI_UINT16( pRsp->devList[x] );
      *msg++ = LO_UINT16( pRsp->devList[x] );
      len += 2;
    }
    osal_mem_free ( pRsp );
  }
  return ( len );
}

/*********************************************************************
 * @fn      zdo_MTCB_NodeDescRspCB()
 *
 * @brief
 *
 *   Called by ZDO when a Node_Desc_rsp message is received.
 *
 * @param  SrcAddr  - Source address
 * @param  Status - response status
 * @param  nwkAddr - 16 bit network address of device
 * @param  pNodeDesc - pointer to the devices Node Descriptor
 *                     NULL if Status != ZDP_SUCCESS
 *
 * @return  none
 */
uint8 zdo_MTCB_NodeDescRspCB( zdoIncomingMsg_t *inMsg, uint8 *msg )
{
  ZDO_NodeDescRsp_t NDRsp;
  
  ZDO_ParseNodeDescRsp( inMsg, &NDRsp );
  
  //Fill up the data bytes
  *msg++ = NDRsp.status;
  *msg++ = HI_UINT16( inMsg->srcAddr.addr.shortAddr );
  *msg++ = LO_UINT16( inMsg->srcAddr.addr.shortAddr );

  *msg++ = HI_UINT16( NDRsp.nwkAddr );
  *msg++ = LO_UINT16( NDRsp.nwkAddr );

  *msg++ = (byte)(NDRsp.nodeDesc.LogicalType);

  // Since Z-Tool can't treat V1.0 and V1.1 differently,
  // we just output these two byte in both cases, although
  // in V1.0, they are always zeros.
  *msg++ = (byte) NDRsp.nodeDesc.ComplexDescAvail;
  *msg++ = (byte) NDRsp.nodeDesc.UserDescAvail;

  *msg++ = NDRsp.nodeDesc.APSFlags;
  *msg++ = NDRsp.nodeDesc.FrequencyBand;
  *msg++ = NDRsp.nodeDesc.CapabilityFlags;
  *msg++ = NDRsp.nodeDesc.ManufacturerCode[1];
  *msg++ = NDRsp.nodeDesc.ManufacturerCode[0];
  *msg++ = NDRsp.nodeDesc.MaxBufferSize;
  *msg++ = NDRsp.nodeDesc.MaxTransferSize[1];
  *msg++ = NDRsp.nodeDesc.MaxTransferSize[0];
  *msg++ = HI_UINT16( NDRsp.nodeDesc.ServerMask);
  *msg++ = LO_UINT16(NDRsp.nodeDesc.ServerMask);
  
  return ( 18 );
}

/*********************************************************************
 * @fn      zdo_MTCB_PowerDescRspCB()
 *
 * @brief
 *
 *   Called by ZDO when a Power_Desc_rsp message is received.
 *
 * @param  SrcAddr  - Source address
 * @param  Status - response status
 * @param  nwkAddr - 16 bit network address of device
 * @param  pPwrDesc - pointer to the devices Power Descriptor
 *                     NULL if Status != ZDP_SUCCESS
 *
 * @return  none
 */
uint8 zdo_MTCB_PowerDescRspCB( zdoIncomingMsg_t *inMsg, uint8 *msg )
{
  ZDO_PowerRsp_t NPRsp;
  
  ZDO_ParsePowerDescRsp( inMsg, &NPRsp );
  
  //Fill up the data bytes
  *msg++ = NPRsp.status;
  *msg++ = HI_UINT16( inMsg->srcAddr.addr.shortAddr );
  *msg++ = LO_UINT16( inMsg->srcAddr.addr.shortAddr );
  *msg++ = HI_UINT16( NPRsp.nwkAddr );
  *msg++ = LO_UINT16( NPRsp.nwkAddr );

  *msg++ = NPRsp.pwrDesc.PowerMode;
  *msg++ = NPRsp.pwrDesc.AvailablePowerSources;
  *msg++ = NPRsp.pwrDesc.CurrentPowerSource;
  *msg   = NPRsp.pwrDesc.CurrentPowerSourceLevel;
  
  return ( 9 );
}

#define ZDO_SIMPLE_DESC_CB_LEN  78
/*********************************************************************
 * @fn      zdo_MTCB_SimpleDescRspCB()
 *
 * @brief
 *
 *   Called by ZDO when a Simple_Desc_rsp message is received.
 *
 * @param  SrcAddr  - Source address
 * @param  Status - response status
 * @param  nwkAddr - 16 bit network address of device
 * @param  EPIntf - Endpoint/Interface for description
 * @param  pSimpleDesc - pointer to the devices Simple Descriptor
 *                     NULL if Status != ZDP_SUCCESS
 *
 * @return  none
 */
uint8 zdo_MTCB_SimpleDescRspCB( zdoIncomingMsg_t *inMsg, uint8 *msg  )
{
  byte len = 0;
  byte x;
  ZDO_SimpleDescRsp_t SimpleDescRsp;

  ZDO_ParseSimpleDescRsp( inMsg, &SimpleDescRsp );
  
  //Fill up the data bytes
  *msg++ = SimpleDescRsp.status;
  *msg++ = HI_UINT16( inMsg->srcAddr.addr.shortAddr );
  *msg++ = LO_UINT16( inMsg->srcAddr.addr.shortAddr );
  *msg++ = HI_UINT16( SimpleDescRsp.nwkAddr );
  *msg++ = LO_UINT16( SimpleDescRsp.nwkAddr );
  *msg++ = SimpleDescRsp.simpleDesc.EndPoint;
  *msg++ = HI_UINT16( SimpleDescRsp.simpleDesc.AppProfId );
  *msg++ = LO_UINT16( SimpleDescRsp.simpleDesc.AppProfId );
  *msg++ = HI_UINT16( SimpleDescRsp.simpleDesc.AppDeviceId );
  *msg++ = LO_UINT16( SimpleDescRsp.simpleDesc.AppDeviceId );
  *msg++ = SimpleDescRsp.simpleDesc.AppDevVer;
  *msg++ = SimpleDescRsp.simpleDesc.Reserved;
  len = 12;
  
  *msg++ = SimpleDescRsp.simpleDesc.AppNumInClusters;
  len++;
  
  // ZTool supports 16 bits the code has taken it into account      
  for ( x = 0; x < SimpleDescRsp.simpleDesc.AppNumInClusters; x++ )
  {
    *msg++ = HI_UINT16( SimpleDescRsp.simpleDesc.pAppInClusterList[x]);
    *msg++ = LO_UINT16( SimpleDescRsp.simpleDesc.pAppInClusterList[x]);
    len += 2;
  }
  
  *msg++ = SimpleDescRsp.simpleDesc.AppNumOutClusters;
  len++;

  for ( x = 0; x < SimpleDescRsp.simpleDesc.AppNumOutClusters; x++ )
  {
    *msg++ = HI_UINT16( SimpleDescRsp.simpleDesc.pAppOutClusterList[x]);
    *msg++ = LO_UINT16( SimpleDescRsp.simpleDesc.pAppOutClusterList[x]);
    len += 2;
  }
  
  if ( SimpleDescRsp.simpleDesc.pAppInClusterList )
    osal_mem_free( SimpleDescRsp.simpleDesc.pAppInClusterList );
  if ( SimpleDescRsp.simpleDesc.pAppOutClusterList )
    osal_mem_free( SimpleDescRsp.simpleDesc.pAppOutClusterList );
  
  return ( len );
}

/*********************************************************************
 * @fn      zdo_MTCB_ActiveEPRspCB()
 *
 * @brief
 *
 *   Called by ZDO when a Active_EP_rsp or Match_Desc_rsp message is received.
 *
 * @param  SrcAddr  - Source address
 * @param  Status - response status
 * @param  nwkAddr - Device's short address that this response describes
 * @param  epIntfCnt - number of epIntfList items
 * @param  epIntfList - array of active endpoint/interfaces.
 *
 * @return  none
 */
uint8 zdo_MTCB_MatchActiveEPRspCB( zdoIncomingMsg_t *inMsg, uint8 *msg )
{
  uint8 len = 0;
  ZDO_ActiveEndpointRsp_t *pRsp = ZDO_ParseEPListRsp( inMsg );

  if ( pRsp )
  {
    //Fill up the data bytes
    *msg++ = pRsp->status;
    if ( pRsp->status != ZDP_SUCCESS )
      pRsp->cnt = 0;
    
    *msg++ = HI_UINT16( inMsg->srcAddr.addr.shortAddr );
    *msg++ = LO_UINT16( inMsg->srcAddr.addr.shortAddr );
    *msg++ = HI_UINT16( pRsp->nwkAddr );
    *msg++ = LO_UINT16( pRsp->nwkAddr );
    *msg++ = pRsp->cnt;
  
    if ( pRsp->cnt )
    {
      osal_memcpy( msg, pRsp->epList, pRsp->cnt );
    }
    
    len = 6 + pRsp->cnt;
    
    osal_mem_free( pRsp );
  }
  return ( len );
}

/*********************************************************************
 * @fn      zdo_MTCB_BindRspCB()
 *
 * @brief
 *
 *   Called to send MT callback response for binding responses
 *
 * @param  type - binding type (end device, bind, unbind)
 * @param  SrcAddr  - Source address
 * @param  Status - response status
 *
 * @return  none
 */
uint8 zdo_MTCB_BindRspCB( zdoIncomingMsg_t *inMsg, uint8 *msg )
{
  msg[0] = ZDO_ParseBindRsp( inMsg );
  msg[1] = HI_UINT16( inMsg->srcAddr.addr.shortAddr );
  msg[2] = LO_UINT16( inMsg->srcAddr.addr.shortAddr );
  
  return ( 3 );
}

/*********************************************************************
 * @fn      zdo_MTCB_MgmtLqiRspCB()
 *
 * @brief
 *
 *   Called to send MT callback response for Management LQI response
 *
 * @param  type - binding type (end device, bind, unbind)
 * @param  SrcAddr  - Source address
 * @param  Status - response status
 *
 * @return  none
 */
uint8 zdo_MTCB_MgmtLqiRspCB( zdoIncomingMsg_t *inMsg, uint8 *msg )
{
  byte len = 0;
  byte x;
  ZDO_MgmtLqiRsp_t *pRsp = ZDO_ParseMgmtLqiRsp( inMsg );

  if ( pRsp )
  {
    if ( pRsp->neighborLqiCount > ZDO_MAX_LQI_ITEMS )
      pRsp->neighborLqiCount = ZDO_MAX_LQI_ITEMS;
    
    *msg++ = HI_UINT16( inMsg->srcAddr.addr.shortAddr );
    *msg++ = LO_UINT16( inMsg->srcAddr.addr.shortAddr );    
    *msg++ = pRsp->status;
    *msg++ = pRsp->neighborLqiEntries;
    *msg++ = pRsp->startIndex;
    *msg++ = pRsp->neighborLqiCount;
    len = 2 + 1 + 1 + 1 + 1;

    for ( x = 0; x < pRsp->neighborLqiCount; x++ )
    {
      osal_cpyExtAddr(msg, pRsp->list[x].extPANId);
      msg += Z_EXTADDR_LEN;
      *msg++ = HI_UINT16( pRsp->list[x].nwkAddr );
      *msg++ = LO_UINT16( pRsp->list[x].nwkAddr );
      *msg++ = pRsp->list[x].rxLqi;
      *msg++ = pRsp->list[x].txQuality;
      len += Z_EXTADDR_LEN + 4;
    }

    osal_mem_free( pRsp );
  }
  
  return ( len );
}

/*********************************************************************
 * @fn      zdo_MTCB_MgmtNwkDiscRspCB()
 *
 * @brief
 *
 *   Called to send MT callback response for Management Network
 *   Discover response
 *
 * @param  SrcAddr  - Source address
 * @param  Status - response status
 *
 * @return  none
 */
uint8 zdo_MTCB_MgmtNwkDiscRspCB( zdoIncomingMsg_t *inMsg, uint8 *msg )
{
  uint8 x;
  uint8 len = 0;
  ZDO_MgmNwkDiscRsp_t *pRsp = ZDO_ParseMgmNwkDiscRsp( inMsg );

  if ( pRsp )
  {
    if ( pRsp->networkListCount > ZDO_MAX_NWKDISC_ITEMS )
      pRsp->networkListCount = ZDO_MAX_NWKDISC_ITEMS;
    
    //Fill up the data bytes
    *msg++ = HI_UINT16( inMsg->srcAddr.addr.shortAddr );
    *msg++ = LO_UINT16( inMsg->srcAddr.addr.shortAddr );
    *msg++ = pRsp->status;
    *msg++ = pRsp->networkCount;
    *msg++ = pRsp->startIndex;
    *msg++ = pRsp->networkListCount;
    len = 2 + 1 + 1 + 1 + 1; 
  
    for ( x = 0; x < pRsp->networkListCount; x++ )
    {
      osal_cpyExtAddr( msg, pRsp->list[x].extendedPANID );
      msg += Z_EXTADDR_LEN;
      *msg++ = pRsp->list[x].logicalChannel;
      *msg++ = pRsp->list[x].stackProfile;
      *msg++ = pRsp->list[x].version;
      *msg++ = pRsp->list[x].beaconOrder;
      *msg++ = pRsp->list[x].superFrameOrder;
      *msg++ = pRsp->list[x].permitJoining;
      len += Z_EXTADDR_LEN + 6;
    }
    
    osal_mem_free( pRsp );
  }

  return ( len );
}

/*********************************************************************
 * @fn      zdo_MTCB_MgmtRtgRspCB()
 *
 * @brief
 *
 *   Called to send MT callback response for Management Network
 *   Discover response
 *
 * @param  SrcAddr  - Source address
 * @param  Status - response status
 *
 * @return  none
 */
uint8 zdo_MTCB_MgmtRtgRspCB( zdoIncomingMsg_t *inMsg, uint8 *msg  )
{
  uint8 len = 0;
  uint8 x;
  ZDO_MgmtRtgRsp_t *pRsp = ZDO_ParseMgmtRtgRsp( inMsg );
  
  if ( pRsp )
  {
    if ( pRsp->rtgListCount > ZDO_MAX_RTG_ITEMS )
      pRsp->rtgListCount = ZDO_MAX_RTG_ITEMS;
    
    //Fill up the data bytes
    *msg++ = HI_UINT16( inMsg->srcAddr.addr.shortAddr );
    *msg++ = LO_UINT16( inMsg->srcAddr.addr.shortAddr );
    *msg++ = pRsp->status;
    *msg++ = pRsp->rtgCount;
    *msg++ = pRsp->startIndex;
    *msg++ = pRsp->rtgListCount;
    len = 2 + 1 + 1 + 1 + 1; //  SrcAddr + Status + RtgCount + StartIndex + RtgListCount

    for ( x = 0; x < pRsp->rtgListCount; x++ )
    {
      *msg++ = HI_UINT16( pRsp->list[x].dstAddress );
      *msg++ = LO_UINT16( pRsp->list[x].dstAddress );
      *msg++ = HI_UINT16( pRsp->list[x].nextHopAddress );
      *msg++ = LO_UINT16( pRsp->list[x].nextHopAddress );
      *msg++ = pRsp->list[x].status;
      len += 5;
    }

    osal_mem_free( pRsp );
  }
  return ( len );
}

/*********************************************************************
 * @fn      zdo_MTCB_MgmtBindRspCB()
 *
 * @brief
 *
 *   Called to send MT callback response for Management Network
 *   Discover response
 *
 * @param  SrcAddr  - Source address
 * @param  Status - response status
 *
 * @return  none
 */
uint8 zdo_MTCB_MgmtBindRspCB( zdoIncomingMsg_t *inMsg, uint8 *msg )
{
  uint8 len = 0;       
  uint8 x, y;
  ZDO_MgmtBindRsp_t *pRsp = ZDO_ParseMgmtBindRsp( inMsg );
  
  if ( pRsp )
  {
    if ( pRsp->bindingListCount > ZDO_MAX_BIND_ITEMS )
      pRsp->bindingListCount = ZDO_MAX_BIND_ITEMS;
    
    //Fill up the data bytes
    *msg++ = HI_UINT16( inMsg->srcAddr.addr.shortAddr );
    *msg++ = LO_UINT16( inMsg->srcAddr.addr.shortAddr );
    *msg++ = pRsp->status;
    *msg++ = pRsp->bindingCount;
    *msg++ = pRsp->startIndex;
    *msg++ = pRsp->bindingListCount;
    len = (2 + 1 + 1 + 1 + 1);    //  SrcAddr + Status + BindCount + StartIndex + BindListCount
      
    for ( x = 0; x < pRsp->bindingListCount; x++ )
    {
      msg = zdo_MT_CopyRevExtAddr( msg, pRsp->list[x].srcAddr );
      *msg++ = pRsp->list[x].srcEP;
      *msg++ = HI_UINT16( pRsp->list[x].clusterID);
      *msg++ = LO_UINT16( pRsp->list[x].clusterID);
      *msg++ = pRsp->list[x].dstAddr.addrMode;
    
      if ( pRsp->list[x].dstAddr.addrMode == Addr64Bit )
      {         
        msg = zdo_MT_CopyRevExtAddr( msg, pRsp->list[x].dstAddr.addr.extAddr );
      }
      else
      {
        for ( y = 0; y < (Z_EXTADDR_LEN - 2); y++ )
          *msg++ = 0;
        *msg++ = HI_UINT16( pRsp->list[x].dstAddr.addr.shortAddr );
        *msg++ = LO_UINT16( pRsp->list[x].dstAddr.addr.shortAddr );
      }
      *msg++ = pRsp->list[x].dstEP;           
      len += (Z_EXTADDR_LEN + 1 + 2 + 1) + (Z_EXTADDR_LEN + 1);
    }
    osal_mem_free( pRsp );
  }
  return ( len );
}

/*********************************************************************
 * @fn      zdo_MTCB_MgmtDirectJoinRspCB()
 *
 * @brief
 *
 *   Called to send MT callback response for Management Direct Join
 *   responses
 *
 * @param  SrcAddr  - Source address
 * @param  Status - response status
 *
 * @return  none
 */
uint8 zdo_MTCB_MgmtDirectJoinRspCB( zdoIncomingMsg_t *inMsg, uint8 *msg )
{
  msg[0] = HI_UINT16( inMsg->srcAddr.addr.shortAddr );
  msg[1] = LO_UINT16( inMsg->srcAddr.addr.shortAddr );
  msg[2] = ZDO_ParseMgmtDirectJoinRsp( inMsg );

  return ( 3 );
}

/*********************************************************************
 * @fn      zdo_MTCB_MgmtLeaveRspCB()
 *
 * @brief
 *
 *   Called to send MT callback response for Management Leave
 *   responses
 *
 * @param  SrcAddr  - Source address
 * @param  Status - response status
 *
 * @return  none
 */
uint8 zdo_MTCB_MgmtLeaveRspCB( zdoIncomingMsg_t *inMsg, uint8 *msg )
{
  msg[0] = ZDO_ParseMgmtLeaveRsp( inMsg );
  msg[1] = HI_UINT16( inMsg->srcAddr.addr.shortAddr );
  msg[2] = LO_UINT16( inMsg->srcAddr.addr.shortAddr );

  return ( 3 );
}

/*********************************************************************
 * @fn      zdo_MTCB_MgmtPermitJoinRspCB()
 *
 * @brief
 *
 *   Called to send MT callback response for Management Permit Join
 *   responses
 *
 * @param  SrcAddr  - Source address
 * @param  Status - response status
 *
 * @return  none
 */
uint8 zdo_MTCB_MgmtPermitJoinRspCB( zdoIncomingMsg_t *inMsg, uint8 *msg )
{

  msg[0] = ZDO_ParseMgmtPermitJoinRsp( inMsg );
  msg[1] = HI_UINT16( inMsg->srcAddr.addr.shortAddr );
  msg[2] = LO_UINT16( inMsg->srcAddr.addr.shortAddr );

  return ( 3 );
}

#define USER_DESC_CB_LEN  22
/*********************************************************************
 * @fn      zdo_MTCB_UserDescRspCB()
 *
 * @brief
 *
 *   Called to send MT callback response for User Descriptor
 *   responses
 *
 * @param  SrcAddr  - Source address
 * @param  Status - response status
 * @param  nwkAddrOfInterest -
 * @param  userDescLen -
 * @param  userDesc -
 * @param  SecurityUse -
 *
 * @return  none
 */
uint8 zdo_MTCB_UserDescRspCB( zdoIncomingMsg_t *inMsg, uint8 *msg )
{
  uint8 len = 0;
  ZDO_UserDescRsp_t *pRsp = ZDO_ParseUserDescRsp( inMsg );
  
  if ( pRsp )
  {
    len = USER_DESC_CB_LEN;
    osal_memset( msg, 0, USER_DESC_CB_LEN );
  
    *msg++ = pRsp->status;
    *msg++ = HI_UINT16( inMsg->srcAddr.addr.shortAddr );
    *msg++ = LO_UINT16( inMsg->srcAddr.addr.shortAddr );
    *msg++ = HI_UINT16( pRsp->nwkAddr );
    *msg++ = LO_UINT16( pRsp->nwkAddr );
    *msg++ = pRsp->length;
    osal_memcpy( msg, pRsp->desc, pRsp->length ); 
  
    osal_mem_free( pRsp );
  }
  return ( len );
}

/*********************************************************************
 * @fn      zdo_MTCB_UserDescConfCB()
 *
 * @brief
 *
 *   Called to send MT callback response for User Descriptor
 *   confirm
 *
 * @param  SrcAddr  - Source address
 * @param  Status - response status
 * @param  SecurityUse -
 *
 * @return  none
 */
uint8 zdo_MTCB_UserDescConfCB( zdoIncomingMsg_t *inMsg, uint8 *msg )
{
  msg[0] = ZDO_ParseUserDescConf( inMsg );
  msg[1] = HI_UINT16( inMsg->srcAddr.addr.shortAddr );
  msg[2] = LO_UINT16( inMsg->srcAddr.addr.shortAddr );

  return ( 3 );
}

/*********************************************************************
 * @fn     zdo_MTCB_ServerDiscRspCB()
 *
 * @brief  Called to send MT callback response for Server_Discovery_rsp responses.
 *
 * @param  srcAddr - Source address.
 * @param  status - Response status.
 * @param  aoi - Network Address of Interest.
 * @param  serverMask - Bit mask of services that match request.
 * @param  SecurityUse -
 *
 * @return  none
 */
uint8 zdo_MTCB_ServerDiscRspCB( zdoIncomingMsg_t *inMsg, uint8 *msg )
{
  ZDO_ServerDiscRsp_t Rsp;
  ZDO_ParseServerDiscRsp( inMsg, &Rsp );

  *msg++ = Rsp.status;
  *msg++ = HI_UINT16( inMsg->srcAddr.addr.shortAddr );
  *msg++ = LO_UINT16( inMsg->srcAddr.addr.shortAddr );
  *msg++ = HI_UINT16( Rsp.serverMask );
  *msg++ = LO_UINT16( Rsp.serverMask );

  return ( 5 );
}

/*********************************************************************
 * @fn     MT_ZdoRsp()
 *
 * @brief  Called to process incoming response messages.
 *
 * @param  inMsg - incoming message
 *
 * @return  none
 */
void MT_ZdoRsp( zdoIncomingMsg_t *inMsg )
{
  uint8 x = 0;
  
  // Look through the table
  while ( mtzdoConvTable[x].clusterID )
  {
    // Cluster ID Match?
    if ( mtzdoConvTable[x].clusterID == inMsg->clusterID )
    {
      // Registered for this callback?
      if ( _zdoCallbackSub & mtzdoConvTable[x].subCBID )
      {
        uint8 len = mtzdoConvTable[x].pFn( inMsg, mtzdoResponseBuffer );
        if ( len )
        {
          // Send the response message
          MT_BuildAndSendZToolCB( mtzdoConvTable[x].mtID, len, mtzdoResponseBuffer );
        }
      }
      break;    // get out of the loop
    }
    x++;
  }
}

/*********************************************************************
*********************************************************************/

#endif   /*ZDO Command Processing in MT*/
