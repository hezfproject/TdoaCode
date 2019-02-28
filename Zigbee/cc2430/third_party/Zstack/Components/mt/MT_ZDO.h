/**************************************************************************************************
  Filename:       MT_ZDO.h
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

/*********************************************************************
 * INCLUDES
 */
#include "ZComDef.h"
#include "MTEL.h"
#include "APSMEDE.h"
#include "AF.h"
#include "ZDProfile.h"
#include "ZDObject.h"
#include "ZDApp.h"

#if !defined( WIN32 )
  #include "OnBoard.h"
#endif


/*********************************************************************
 * MACROS
 */

/*********************************************************************
 * CONSTANTS
 */
//ZDO commands
#define SPI_CMD_ZDO_AUTO_ENDDEVICEBIND_REQ    0x0A00
#define SPI_CMD_ZDO_AUTO_FIND_DESTINATION_REQ 0x0A01
#define SPI_CMD_ZDO_NWK_ADDR_REQ              0x0A02
#define SPI_CMD_ZDO_IEEE_ADDR_REQ             0x0A03
#define SPI_CMD_ZDO_NODE_DESC_REQ             0x0A04
#define SPI_CMD_ZDO_POWER_DESC_REQ            0x0A05
#define SPI_CMD_ZDO_SIMPLE_DESC_REQ           0x0A06
#define SPI_CMD_ZDO_ACTIVE_EPINT_REQ          0x0A07
#define SPI_CMD_ZDO_MATCH_DESC_REQ            0x0A08
#define SPI_CMD_ZDO_COMPLEX_DESC_REQ          0x0A09
#define SPI_CMD_ZDO_USER_DESC_REQ             0x0A0A
#define SPI_CMD_ZDO_END_DEV_BIND_REQ          0x0A0B
#define SPI_CMD_ZDO_BIND_REQ                  0x0A0C
#define SPI_CMD_ZDO_UNBIND_REQ                0x0A0D
#define SPI_CMD_ZDO_MGMT_NWKDISC_REQ          0x0A0E
#define SPI_CMD_ZDO_MGMT_LQI_REQ              0x0A0F
#define SPI_CMD_ZDO_MGMT_RTG_REQ              0x0A10
#define SPI_CMD_ZDO_MGMT_BIND_REQ             0x0A11
#define SPI_CMD_ZDO_MGMT_DIRECT_JOIN_REQ      0x0A12
#define SPI_CMD_ZDO_USER_DESC_SET             0x0A13
#define SPI_CMD_ZDO_END_DEV_ANNCE             0x0A14
#define SPI_CMD_ZDO_MGMT_LEAVE_REQ            0x0A15
#define SPI_CMD_ZDO_MGMT_PERMIT_JOIN_REQ      0x0A16
#define SPI_CMD_ZDO_SERVERDISC_REQ            0X0A17
#define SPI_CMD_ZDO_NETWORK_START_REQ         0X0A18

#define SPI_ZDO_CB_TYPE                       0x0A80

#define SPI_CB_ZDO_NWK_ADDR_RSP               0x0A80
#define SPI_CB_ZDO_IEEE_ADDR_RSP              0x0A81
#define SPI_CB_ZDO_NODE_DESC_RSP              0x0A82
#define SPI_CB_ZDO_POWER_DESC_RSP             0x0A83
#define SPI_CB_ZDO_SIMPLE_DESC_RSP            0x0A84
#define SPI_CB_ZDO_ACTIVE_EPINT_RSP           0x0A85
#define SPI_CB_ZDO_MATCH_DESC_RSP             0x0A86
#define SPI_CB_ZDO_END_DEVICE_BIND_RSP        0x0A87
#define SPI_CB_ZDO_BIND_RSP                   0x0A88
#define SPI_CB_ZDO_UNBIND_RSP                 0x0A89
#define SPI_CB_ZDO_MGMT_NWKDISC_RSP           0x0A8A
#define SPI_CB_ZDO_MGMT_LQI_RSP               0x0A8B
#define SPI_CB_ZDO_MGMT_RTG_RSP               0x0A8C
#define SPI_CB_ZDO_MGMT_BIND_RSP              0x0A8D
#define SPI_CB_ZDO_MGMT_DIRECT_JOIN_RSP       0x0A8E
#define SPI_CB_ZDO_USER_DESC_RSP              0x0A8F

#define SPI_ZDO_CB2_TYPE                      0x0A90

#define SPI_CB_ZDO_USER_DESC_CNF              0x0A90
#define SPI_CB_ZDO_MGMT_LEAVE_RSP             0x0A91
#define SPI_CB_ZDO_MGMT_PERMIT_JOIN_RSP       0x0A92
#define SPI_CB_ZDO_SERVERDISC_RSP             0x0A93

#define SPI_RESP_LEN_ZDO_DEFAULT              0x01

#define CB_ID_ZDO_NWK_ADDR_RSP               0x00000001
#define CB_ID_ZDO_IEEE_ADDR_RSP              0x00000002
#define CB_ID_ZDO_NODE_DESC_RSP              0x00000004
#define CB_ID_ZDO_POWER_DESC_RSP             0x00000008
#define CB_ID_ZDO_SIMPLE_DESC_RSP            0x00000010
#define CB_ID_ZDO_ACTIVE_EPINT_RSP           0x00000020
#define CB_ID_ZDO_MATCH_DESC_RSP             0x00000040
#define CB_ID_ZDO_END_DEVICE_BIND_RSP        0x00000080
#define CB_ID_ZDO_BIND_RSP                   0x00000100
#define CB_ID_ZDO_UNBIND_RSP                 0x00000200
#define CB_ID_ZDO_MGMT_NWKDISC_RSP           0x00000400
#define CB_ID_ZDO_MGMT_LQI_RSP               0x00000800
#define CB_ID_ZDO_MGMT_RTG_RSP               0x00001000
#define CB_ID_ZDO_MGMT_BIND_RSP              0x00002000
#define CB_ID_ZDO_MGMT_DIRECT_JOIN_RSP       0x00004000
#define CB_ID_ZDO_USER_DESC_RSP              0x00008000
#define CB_ID_ZDO_USER_DESC_CONF             0x00010000
#define CB_ID_ZDO_MGMT_LEAVE_RSP             0x00020000
#define CB_ID_ZDO_MGMT_PERMIT_JOIN_RSP       0x00040000
#define CB_ID_ZDO_SERVERDISC_RSP             0x00080000

/*********************************************************************
 * TYPEDEFS
 */

/*********************************************************************
 * GLOBAL VARIABLES
 */
extern uint32 _zdoCallbackSub;

/*********************************************************************
 * EXTERNAL VARIABLES
 */

/*********************************************************************
 * EXTERNAL FUNCTIONS
 */

/*********************************************************************
 * LOCAL VARIABLES
 */

/*********************************************************************
 * LOCAL FUNCTIONS
 */

/*
 *   Process all the NWK commands that are issued by test tool
 */
extern void MT_ZdoCommandProcessing( uint16 cmd_id , byte len , byte *pData );
extern void MT_ZdoRsp( zdoIncomingMsg_t *inMsg );

/*********************************************************************
*********************************************************************/
