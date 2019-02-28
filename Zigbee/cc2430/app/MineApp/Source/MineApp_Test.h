/**************************************************************************************************
  Filename:       MineApp_test.h
  Revised:        $Date: 2009/05/26 01:27:14 $
  Revision:       $Revision: 1.1 $

  Description:    Mine Application test interface.
  **************************************************************************************************/
#ifndef MINEAPP_TEST_H
#define MINEAPP_TEST_H

/*************************************************************************************************
  *INCLUDES
  */
#include "ZComDef.h"
#include "AF.h"
#include "AppProtocolWrapper.h"

#if !defined( NONWK )
  #include "APS.h"
  #include "nwk.h"
  #include "ZDApp.h"
#endif
/*********************************************************************
 * TYPEDEFS
 */

/*************************************************************************************************
  *CONSTANTS
  */
/*************************************************************************************************
  *MACROS
  */
#define MINEAPP_FAKE_MSG_EVT    0x4000
#define MINEAPP_SEND_MSG_EVT   0x2000
/**************************************************************************************************
  *FUNCTIONS
  */
void MineAppTest_Init(SPIMSGPacket_t* pSpi, 
						devStates_t NwkState, 
						afAddrType_t* dstAddr,
						uint8 MaxDataLength,
						byte* Msg, byte TaskID,
						uint32 num,
						endPointDesc_t* epDesc);
uint16 MineAppTest_HandleMSG(uint16 events);
void MineAppTest_HandleKey( uint16 keys, byte shifts);
void MineAppTest_MessageMSGCB(const afIncomingMSGPacket_t *MSGpkt);
void MineAppTest_MessageSPICB(const afIncomingMSGPacket_t *MSGpkt);

#endif

