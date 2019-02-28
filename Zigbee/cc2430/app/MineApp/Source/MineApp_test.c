/**************************************************************************************************
  Filename:       MineApp_test.c
  Revised:        $Date: 2009/09/10 17:35:09 $
  Revision:       $Revision: 1.3 $

  Description:    Mine Application test interface.
  **************************************************************************************************/
#include "MineApp_Test.h"

#include "App_cfg.h"

#include "OSAL.h"
#include "OSAL_Custom.h"
#include "OSAL_Tasks.h"

#include "OnBoard.h"
#include "hal_led.h"
#include "hal_spi.h"
#include "NLMEDE.h"

/*********************************************************************
 * LOCAL VARIABLES
 */
static devStates_t MineAppTest_NwkState;
static uint8 MineAppTest_MaxDataLength;
static byte MineAppTest_TaskID;
static SPIMSGPacket_t* MineAppTest_pSpi;
static byte* MineAppTest_Msg;
static uint32 MineAppTest_Num;
static afAddrType_t MineAppTest_dstAddr;
static endPointDesc_t MineAppTest_epDesc;

/*********************************************************************
 * LOCAL FUNCTIONS
 */
static byte MineAppTest_OSmsg2SPIpkt(const afIncomingMSGPacket_t* MSGpkt, SPIMSGPacket_t * msg);



void MineAppTest_Init(SPIMSGPacket_t* pSpi, 
						devStates_t NwkState, 
						afAddrType_t* dstAddr,
						uint8 MaxDataLength,
						byte* Msg, byte TaskID,
						uint32 num,
						endPointDesc_t* epDesc)
{
	MineAppTest_pSpi = pSpi;
	MineAppTest_NwkState = NwkState;
	MineAppTest_dstAddr.addrMode = dstAddr->addrMode;
	MineAppTest_dstAddr.addr.shortAddr = dstAddr->addr.shortAddr;
	MineAppTest_MaxDataLength = MaxDataLength;
	MineAppTest_Msg = Msg;
	MineAppTest_TaskID = TaskID;
	MineAppTest_Num = num;
	
	osal_memcpy(&MineAppTest_epDesc, epDesc, sizeof(endPointDesc_t));

	//fake a location msg.
	uint8 i = SPIPKTHDR_LEN;
	MineAppTest_Msg[i++] = LOCATION;
	MineAppTest_Msg[i++] = 11;
	MineAppTest_Msg[i++] = 12;
	MineAppTest_Msg[i++] = 13;
	MineAppTest_Msg[i++] = 14;
	MineAppTest_Msg[i++] = 15;
	MineAppTest_Msg[i++] = 16;
}

uint16 MineAppTest_HandleMSG(uint16 events)
{
  	if (events & MINEAPP_FAKE_MSG_EVT)
	{
	#if (defined HAL_SPI) && (HAL_SPI == TRUE)
		MineAppTest_pSpi->spihdr.dstAddr.addr.shortAddr = NWK_BROADCAST_SHORTADDR_DEVALL;
		MineAppTest_pSpi->spihdr.dstAddr.addrMode= (AddrMode_t)AddrBroadcast;
		MineAppTest_pSpi->DataLength = sizeof(app_Loc_t);
		uint8 i = SPIPKTHDR_LEN;
		MineAppTest_Msg[i++] = LOCATION;
		MineAppTest_Msg[i++] = 11;
		MineAppTest_Msg[i++] = 12;
		MineAppTest_Msg[i++] = 13;
		MineAppTest_Msg[i++] = 14;
		MineAppTest_Msg[i++] = 15;
		MineAppTest_Msg[i++] = 16;
		HalLedSet(HAL_LED_1, HAL_LED_MODE_TOGGLE);
		HalSPIWrite((uint8 *)MineAppTest_pSpi, MineAppTest_MaxDataLength + SPIPKTHDR_LEN);
	#endif
		return events ^ MINEAPP_FAKE_MSG_EVT;
	}
	
	if (events & MINEAPP_SEND_MSG_EVT)
	{
		//int8 pwr = 1;
		//MAC_MlmeSetReq(MAC_PHY_TRANSMIT_POWER, &pwr);
		//now fake a location msg.
		MineAppTest_pSpi->DataLength = sizeof(app_Loc_t);
		uint8 i = SPIPKTHDR_LEN;
		MineAppTest_Msg[i++] = LOCATION;
		MineAppTest_Msg[i++] = 11;
		MineAppTest_Msg[i++] = 12;
		MineAppTest_Msg[i++] = 13;
		MineAppTest_Msg[i++] = 14;
		MineAppTest_Msg[i++] = 15;
		MineAppTest_Msg[i++] = 16;
		HalLedSet(HAL_LED_1, HAL_LED_MODE_TOGGLE);
	#ifdef TIME_TEST
		RESET_T1CNT();
	#endif
		MineAppTest_pSpi->spihdr.dstAddr.addr.shortAddr = 0;
		AF_DataRequest((afAddrType_t *)&(MineAppTest_pSpi->spihdr.dstAddr), 
						(endPointDesc_t *)&MineAppTest_epDesc,
              				MINEAPP_CLUSTERID, MineAppTest_pSpi->DataLength,
              				(byte *)(MineAppTest_pSpi+1), &(MineAppTest_pSpi->spihdr.transID),
              				MineAppTest_pSpi->spihdr.options, MineAppTest_pSpi->spihdr.radius);
		return (events ^ MINEAPP_SEND_MSG_EVT);
	}
        return events;
}

void MineAppTest_HandleKey( uint16 keys, byte shifts)
{
#ifdef MINE_TEST
	if (MineAppTest_NwkState == DEV_ZB_COORD)
	{
		osal_set_event(MineAppTest_TaskID, MINEAPP_FAKE_MSG_EVT);
	}
	else
	{
		osal_set_event(MineAppTest_TaskID, MINEAPP_SEND_MSG_EVT);
	}
#else
#if defined ( LOCATION_BLINDNODE ) && defined (LOC_TEST)
	BlindNode_FindRequest();
#else
	osal_set_event(MineAppTest_TaskID, MINEAPP_SEND_MSG_EVT);
#endif
#endif
}

void MineAppTest_MessageMSGCB(const afIncomingMSGPacket_t *MSGpkt)
{
	if (ZSuccess == MineAppTest_OSmsg2SPIpkt(MSGpkt, MineAppTest_pSpi)
		&& LOCATION == *(byte *)(MineAppTest_pSpi+1))
	{
		HalLedSet(HAL_LED_1, HAL_LED_MODE_TOGGLE);
		AF_DataRequest((afAddrType_t *)&(MineAppTest_pSpi->spihdr.dstAddr), 
						(endPointDesc_t *)&MineAppTest_epDesc,
  						MINEAPP_CLUSTERID, MineAppTest_pSpi->DataLength, 
  						(byte *)(MineAppTest_pSpi+1), &(MineAppTest_pSpi->spihdr.transID), 
  						MineAppTest_pSpi->spihdr.options, MineAppTest_pSpi->spihdr.radius);
	}	
}

void MineAppTest_MessageSPICB(const afIncomingMSGPacket_t *MSGpkt)
{
	SPIMSGPacket_t * rxmsg = (SPIMSGPacket_t *)MSGpkt;
	if (*(byte *)(rxmsg +1) == LOCATION)
	{
		HalLedSet(HAL_LED_1, HAL_LED_MODE_TOGGLE);
		AF_DataRequest((afAddrType_t *)&(rxmsg->spihdr.dstAddr), (endPointDesc_t *)&MineAppTest_epDesc,
		MINEAPP_CLUSTERID, rxmsg->DataLength, (byte *)(rxmsg+1),
		&(rxmsg->spihdr.transID), rxmsg->spihdr.options, rxmsg->spihdr.radius);
	}	
}

byte MineAppTest_OSmsg2SPIpkt(const afIncomingMSGPacket_t* MSGpkt, SPIMSGPacket_t * msg)
{
  	uint8 dataLen;  
	uint8 *tempPtr;
	tempPtr = (uint8 *)msg;
	switch(MSGpkt->hdr.event)
	{
		case ZDO_STATE_CHANGE:
			*tempPtr++ = SPI_RX;++tempPtr;
			//construct srcAddr.
			*tempPtr++ = LO_UINT16(_NIB.nwkDevAddress);
			*tempPtr++ = HI_UINT16(_NIB.nwkDevAddress);
			*tempPtr++ = (AddrMode_t)afAddr16Bit;
			*tempPtr++ = MINEAPP_ENDPOINT;

			//construct dstAddr, transID, options, radius.
			*tempPtr++ = LO_UINT16(_NIB.nwkCoordAddress);
  			*tempPtr++ = HI_UINT16(_NIB.nwkCoordAddress);
			*tempPtr++ = (AddrMode_t)afAddr16Bit;
			*tempPtr++ = MINEAPP_ENDPOINT;
				
			*tempPtr++ = INIT_TRANID;
			*tempPtr++ = INIT_OPN;
			*tempPtr++ = MAX_DEPTH;
			*tempPtr++ = BREAK_UINT32(MineAppTest_Num, 0);
			*tempPtr++ = BREAK_UINT32(MineAppTest_Num, 1);
			*tempPtr++ = BREAK_UINT32(MineAppTest_Num, 2);
			*tempPtr++ = BREAK_UINT32(MineAppTest_Num, 3);
			if (MineAppTest_NwkState == DEV_ZB_COORD)
			{		
				dataLen = sizeof(app_startNwk_t);
				*tempPtr++ = LO_UINT16(dataLen);
				*tempPtr++ = HI_UINT16(dataLen);
				
				*tempPtr++ = ZB_START_NETWORK;
				*tempPtr++ = LO_UINT16(_NIB.nwkPanId);
				*tempPtr++ = HI_UINT16(_NIB.nwkPanId);
				osal_cpyExtAddr(tempPtr, NLME_GetExtAddr());
			}
			//else if (MineApp_NwkState == DEV_INIT)
			//{	
			//	GET_PKT_SIZE(dataLen, ZB_LEAVE_NOTIFY);
			//	*tempPtr++ = LO_UINT16(dataLen);
			//	*tempPtr++ = HI_UINT16(dataLen);
			//	*tempPtr = ZB_LEAVE_NOTIFY;
			//}
			else if (MineAppTest_NwkState != DEV_INIT)
			{
				dataLen = sizeof(app_JoinNwk_t);
				*tempPtr++ = LO_UINT16(dataLen);
				*tempPtr++ = HI_UINT16(dataLen);
				
				*tempPtr++ = ZB_JOIN_NOTIFY;
				osal_cpyExtAddr(tempPtr, NLME_GetExtAddr());
			}
			break;
		case AF_INCOMING_MSG_CMD:
			*tempPtr++ = SPI_RX;
			++tempPtr;
			//construct srcAddr.
		 	*tempPtr++ = LO_UINT16(MSGpkt->srcAddr.addr.shortAddr);
  			*tempPtr++ = HI_UINT16(MSGpkt->srcAddr.addr.shortAddr);
			*tempPtr++ = (AddrMode_t)(MSGpkt->srcAddr.addrMode);
			*tempPtr++ = MSGpkt->srcAddr.endPoint;
                        
			//construct dstAddr, transID, options, radius.
		#ifdef MINE_TEST
			//retranmit to src node.
			*tempPtr++ = LO_UINT16(MSGpkt->srcAddr.addr.shortAddr);;
  			*tempPtr++ = HI_UINT16(MSGpkt->srcAddr.addr.shortAddr);;
			*tempPtr++ = (AddrMode_t)afAddr16Bit;
			*tempPtr++ = MINEAPP_ENDPOINT;
		#else
			tempPtr += 4;
		#endif
			
			*tempPtr++ = MSGpkt->cmd.TransSeqNumber;
			*tempPtr++ = INIT_OPN;
			*tempPtr++ = MAX_DEPTH;
			*tempPtr++ = BREAK_UINT32(MineAppTest_Num, 0);
			*tempPtr++ = BREAK_UINT32(MineAppTest_Num, 1);
			*tempPtr++ = BREAK_UINT32(MineAppTest_Num, 2);
			*tempPtr++ = BREAK_UINT32(MineAppTest_Num, 3);
			
			*tempPtr++ = LO_UINT16(MSGpkt->cmd.DataLength);
  			*tempPtr++ = HI_UINT16(MSGpkt->cmd.DataLength);
			osal_memcpy((void *)tempPtr, MSGpkt->cmd.Data, MSGpkt->cmd.DataLength);
			break;
		
	}
	return ZSuccess;
}
