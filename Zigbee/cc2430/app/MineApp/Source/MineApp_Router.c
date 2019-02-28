/**************************************************************************************************
Filename:       MineApp_Router.c
Revised:        $Date: 2009/09/29 23:14:08 $
Revision:       $Revision: 1.2 $

Description:    Mine Application of router.
**************************************************************************************************/
/*********************************************************************
* INCLUDES
*/
#include "MineApp_Router.h"
#include "AppProtocolWrapper.h"
#include "MacUtil.h"
#include "App_cfg.h"
#include "Mac_radio_defs.h"
#include "Mac_spec.h"

#if !defined( NONWK )
#include "ZDApp.h"
#include "ZDObject.h"
#include "ZDProfile.h"
#endif

#include "Rtg.h"
#include "AssocList.h"
#include "AddrMgr.h"

#include "OSAL.h"
#include "OSAL_Nv.h"

#include "OnBoard.h"

#if (defined HAL_SPI) && (HAL_SPI == TRUE)
#include "hal_spi.h"
#endif
#include "hal_drivers.h"
#include "hal_led.h"
#include "hal_dma.h"
#include "hal_key.h"

//MUST be times of BLAST_SIGNALSTRENGTH_PERIOD
#ifndef TOLERANCE_TIME
#define TOLERANCE_TIME (BLAST_SIGNALSTRENGTH_PERIOD*8)
#endif

#ifndef MINEAPP_REJOINTIMES
#define MINEAPP_REJOINTIMES 20
#endif

#define MINEAPP_URGENT_NODATA                0
#define MINEAPP_URGENT_RETREAT               1
#define MINEAPP_URGENT_CANCELRETREAT    2

#define INVALID_ADDR INVALID_NODE_ADDR // for pre- allocate nwkaddr, get from function debug.
#define RSSI_MIN      (MAC_RADIO_RECEIVER_SENSITIVITY_DBM + MAC_SPEC_ED_MIN_DBM_ABOVE_RECEIVER_SENSITIVITY)
#define RSSI_MAX    MAC_RADIO_RECEIVER_SATURATION_DBM   

#define CONV_LQI_TO_RSSI( rssi,lqi ) \
	st (   \
	rssi = lqi*(RSSI_MAX - RSSI_MIN)/MAC_SPEC_ED_MAX + RSSI_MIN; \
	)
/*********************************************************************
* TYPEDEFS
*/

/*********************************************************************
* GLOBAL VARIABLES
*/
uint8 MineApp_TaskID;

/*********************************************************************
* LOCAL VARIABLES
*/
static const cId_t MineApp_InputClusterList[] = 
{
	MINEAPP_CLUSTERID,
	CARD_CLUSTERID,
};
static const cId_t MineApp_OutputClusterList[] = 
{
	MINEAPP_CLUSTERID,
	CARD_CLUSTERID,
};

static const SimpleDescriptionFormat_t MineApp_SimpleDesc =
{
	MINEAPP_ENDPOINT,
	MINEAPP_PROFID,

	MINEAPP_DEVICEID,

	MINEAPP_VERSION,
	MINEAPP_FLAGS,

	sizeof(MineApp_InputClusterList),
	(cId_t*)MineApp_InputClusterList,

	sizeof(MineApp_OutputClusterList),
	(cId_t*)MineApp_OutputClusterList
};

static const endPointDesc_t MineApp_epDesc =
{
	MINEAPP_ENDPOINT,
	&MineApp_TaskID,
	(SimpleDescriptionFormat_t *)&MineApp_SimpleDesc,
	noLatencyReqs
};

static uint16 MineApp_RejoinTimes = 0;

static devStates_t MineApp_NwkState;
#ifdef MINE_TEST
static uint8 MineApp_MaxDataLength;
#endif
static uint8 MineApp_TransID;
#if (defined HAL_SPI) && (HAL_SPI == TRUE)
static byte MineApp_Msg[MINEAPP_MAX_DATA_LEN];
static SPIMSGPacket_t* pSpi = (SPIMSGPacket_t*)MineApp_Msg;
#endif
static uint8  MineApp_retreat = MINEAPP_URGENT_NODATA;
static uint16  MineApp_SleepPeriod;
static bool   MineApp_SettingSleepPeriod = false;
__idata static  int8  MineApp_RSSI;
static NodeAddr deviceAddr;
static uint8    MineApp_urgent_timeout_cnt = 0;
static uint8    MineApp_SleepPeriod_timeout_cnt = 0;

#ifdef MED_TEST
#define         MAXCardNum  128
static uint8*  pSSReq_cnt;
static uint8    RecCardNum;
static uint16   TotalCount;
#endif
/*********************************************************************
* LOCAL FUNCTIONS
*/
static void MineApp_ProcessMSGCB(const afIncomingMSGPacket_t *MSGpkt);
#if (defined HAL_SPI) && (HAL_SPI == TRUE)
static void MineApp_ProcessSPICB(const SPIMSGPacket_t *MSGpkt);
#endif
static byte MineApp_ParseAppFrame(const afIncomingMSGPacket_t* MSGpkt);
static void MineApp_HandleKeys( uint16 keys, byte shifts);
/*********************************************************************
* FUNCTIONS
*********************************************************************/

/*********************************************************************
* @fn      MineApp_Init
*
* @brief   Initialization function for the MineApp OSAL task.
*
* @param   task_id - the ID assigned by OSAL.
*
* @return  none
*/
void MineApp_Init( uint8 task_id )
{
	MineApp_TaskID = task_id;
	MineApp_NwkState = DEV_INIT;

#if (defined HAL_SPI) && (HAL_SPI==TRUE)
	halSPICfg_t spiConfig;
	spiConfig.maxBufSize = DMA_RT_MAX;
	spiConfig.intEnable = false;
	HalSpiStart(&spiConfig);
	RegisterForSpi(MineApp_TaskID);
#endif

	afRegister( (endPointDesc_t *)&MineApp_epDesc);
	RegisterForKeys(MineApp_TaskID);

#ifdef TIME_TEST
	INIT_HWTIMER1();
#endif
#ifdef MINE_TEST
	afDataReqMTU_t mtu;
	mtu.kvp = FALSE;
	mtu.aps.secure = FALSE;
	MineApp_MaxDataLength = afDataReqMTU( &mtu);
#endif

#ifdef MED_TEST
	pSSReq_cnt = (uint8 *)osal_mem_alloc(MAXCardNum);

	for(uint8 i=0;i<MAXCardNum;i++)
	{
		pSSReq_cnt[i] = 0;
	}
	RecCardNum = 0;
	TotalCount = 0;
#endif

#if (defined HAL_SPI) && (HAL_SPI == TRUE)
	//construct SPIMSGPacket_t hdr.
	pSpi= (SPIMSGPacket_t *)MineApp_Msg;
	pSpi->spihdr.hdr.event = SPI_RX;
	pSpi->spihdr.srcAddr.addrMode = (AddrMode_t)Addr16Bit;
	pSpi->spihdr.srcAddr.endPoint = MINEAPP_ENDPOINT;
	pSpi->spihdr.dstAddr.addrMode = (AddrMode_t)Addr16Bit;
	pSpi->spihdr.dstAddr.endPoint = MINEAPP_ENDPOINT;
	pSpi->spihdr.transID = INIT_TRANID;
	pSpi->spihdr.options = INIT_OPN;
	pSpi->spihdr.radius = MAX_DEPTH;
#endif

	/* EP and  Profile Id are not used,Card will drop them directly */
	MacUtil_t MacUtil;
	MacUtil.panID = CARD_NWK_ADDR;  /* Communication with Card */
	MacUtil.dst_endpoint = MINEAPP_ENDPOINT;
	MacUtil.src_endpoint = 0x20;//MINEAPP_ENDPOINT;
	MacUtil.profile_id = 0x0100;//MINEAPP_PROFID;
	MacUtil.cluster_id = CARD_CLUSTERID;
	MacUtil.NodeType = ZDO_Config_Node_Descriptor.LogicalType;
	MAC_UTIL_INIT(&MacUtil);

}

/*********************************************************************
* @fn      MineApp_ProcessEvent
*
* @brief   Mine Application Task event processor.
*
* @param   task_id  - The OSAL assigned task ID.
* @param   events - Bit map of events to process.
*
* @return  none
*/

#ifdef TIME_TEST
uint32 times = 0;
#endif
//uint16 addr;
//associated_devices_t* assocaddr;
//uint8 macAddr[8];
//afAddrType_t dstAddr;
uint16 MineApp_ProcessEvent( uint8 task_id, uint16 events )
{
	afIncomingMSGPacket_t* MSGpkt;

	if (events & SYS_EVENT_MSG)
	{
		MSGpkt = (afIncomingMSGPacket_t *)osal_msg_receive(MineApp_TaskID);
		while ( MSGpkt )
		{
			switch ( MSGpkt->hdr.event )
			{
			case ZDO_STATE_CHANGE:
				{
					MineApp_NwkState = (devStates_t)(MSGpkt->hdr.status);

#ifdef MINE_TEST
					MineAppTest_Init(pSpi, MineApp_NwkState, &dstAddr
						MineApp_MaxDataLength, MineApp_Msg,
						MineApp_TaskID, MineApp_Num,
						(endPointDesc_t *)&MineApp_epDesc);
#endif
					if(MineApp_NwkState == DEV_ROUTER)
					{
						uint8 maxFrameRetries = 1;
						MAC_MlmeSetReq(MAC_MAX_FRAME_RETRIES, &maxFrameRetries);
						osal_set_event(MineApp_TaskID, MINEAPP_SIGNALSTRENGTH_EVENT);
					}
					break;
				}
			case KEY_CHANGE:
				{
					if (MineApp_NwkState != DEV_INIT)
					{
						MineApp_HandleKeys(((keyChange_t *)MSGpkt)->keys, ((keyChange_t *)MSGpkt)->state);
					}
					break;
				}
			case AF_INCOMING_MSG_CMD:
				{
#ifdef TIME_TEST
					times = 0;
					RESET_T1CNT();
#endif
					MineApp_ProcessMSGCB(MSGpkt);
					break;
				}

#if (defined HAL_SPI) && (HAL_SPI == TRUE)
			case SPI_RX: //I'm coordinator, just dispatch the packet.
				{
#ifdef TIME_TEST
					times = CAL_HWTIMER1_TIME();
#endif
					MineApp_ProcessSPICB((SPIMSGPacket_t *)MSGpkt);
					break;
				}
#endif
			default:
				break;
			}
			osal_msg_deallocate( (uint8 *)MSGpkt );
			MSGpkt = (afIncomingMSGPacket_t *)osal_msg_receive( MineApp_TaskID );
		}
		return (events ^ SYS_EVENT_MSG);
	}

	if(events & MINEAPP_URGENT_TIME_EVENT)
	{ 
		if(MineApp_urgent_timeout_cnt < MINEAPP_URGENT_TIMEOUT)
		{
			osal_start_timerEx(MineApp_TaskID, MINEAPP_URGENT_TIME_EVENT, 60000); //delay for a minute
			MineApp_urgent_timeout_cnt++;
		}
		else
		{
			MineApp_retreat = MINEAPP_URGENT_NODATA;
			MineApp_urgent_timeout_cnt = 0;
		}
		return (events ^ MINEAPP_URGENT_TIME_EVENT);
	}

	if(events & MINEAPP_SLEEPPERIOD_TIME_EVENT)
	{ 
		if(MineApp_SleepPeriod_timeout_cnt < MINEAPP_SLEEPPERIOD_TIMEOUT)
		{
			osal_start_timerEx(MineApp_TaskID, MINEAPP_SLEEPPERIOD_TIME_EVENT, 60000); //delay for a minute
			MineApp_SleepPeriod_timeout_cnt++;
		}
		else
		{
			MineApp_SettingSleepPeriod =  false;
			MineApp_SleepPeriod_timeout_cnt = 0;
		}
		return (events ^ MINEAPP_SLEEPPERIOD_TIME_EVENT);
	}
	if(events & MINEAPP_SIGNALSTRENGTH_EVENT)
	{
		if(MineApp_NwkState == DEV_ROUTER)
		{
			afAddrType_t dstAddr;
			dstAddr.addrMode = afAddrBroadcast;
			dstAddr.addr.shortAddr = NWK_BROADCAST_SHORTADDR_DEVALL;
			dstAddr.endPoint = MINEAPP_ENDPOINT;

			app_SignalStrength_t SignalStrength;
			SignalStrength.msgtype = SIGNAL_STRENGTH;

			AF_DataRequest(&dstAddr, (endPointDesc_t *)&MineApp_epDesc,
				MINEAPP_CLUSTERID, sizeof(SignalStrength), (uint8 *)&SignalStrength,
				&MineApp_TransID, INIT_OPN,1);
		}
		osal_start_timerEx(MineApp_TaskID,MINEAPP_SIGNALSTRENGTH_EVENT, BLAST_SIGNALSTRENGTH_PERIOD);

		return (events ^ MINEAPP_SIGNALSTRENGTH_EVENT);
	}

	if (events & MINEAPP_ROUTER_PROBENWK_EVENT)
	{
		if ( ++MineApp_RejoinTimes > MINEAPP_REJOINTIMES)
		{
			MineApp_RejoinTimes = 0;
			ZDApp_StartUpFromApp(0);
		}
		else
		{
			osal_start_timerEx(MineApp_TaskID, MINEAPP_ROUTER_PROBENWK_EVENT, TOLERANCE_TIME);
		}
		
		return events ^ MINEAPP_ROUTER_PROBENWK_EVENT;
	}
	return 0;	
}

/*********************************************************************
* @fn      MineApp_ProcessMSGCB
*
* @brief   This function processes OTA incoming message.
*
* @param   MSGpkt - The incoming msg.
*
* @return  none
*/
void MineApp_ProcessMSGCB(const afIncomingMSGPacket_t *MSGpkt)
{
	switch ( MSGpkt->clusterId )
	{
	case MINEAPP_CLUSTERID:
		{
#if (defined HAL_SPI) && (HAL_SPI == TRUE)
			APPWrapper_t* AppPkt = (APPWrapper_t* )(MSGpkt->cmd.Data);
			if ( (AppPkt->app_flag == MP_SSREQ) && (AppPkt->app_MPssReq.reqtype&SSREQ_OUT))
			{
				//MineApp_RSSI = RSSIL;
				CONV_LQI_TO_RSSI(MineApp_RSSI, MSGpkt->LinkQuality);
				app_MPssInd_t ssInd;
				ssInd.rssipkt.RSSI = MineApp_RSSI;
				ssInd.rssipkt.NWK_ADDR = AppPkt->app_MPssReq.NWK_ADDR;
				ssInd.rssipkt.NODE_ADDR = MSGpkt->srcAddr.addr.shortAddr;
				ssInd.rssipkt.seqnum = AppPkt->app_MPssReq.seqnum;
				ssInd.msgtype  = MP_SSIND;
				osal_memcpy(ssInd.nmbr.Nmbr, AppPkt->app_MPssReq.nmbr.Nmbr, sizeof(termNbr_t));

				pSpi->spihdr.srcAddr.addr.shortAddr = MSGpkt->srcAddr.addr.shortAddr;
				pSpi->DataLength = sizeof(app_MPssInd_t);
				osal_memcpy((void *)(pSpi+1), (void *)&ssInd, pSpi->DataLength);
				HalSPIWrite((uint8 *)pSpi, pSpi ->DataLength + SPIPKTHDR_LEN);
			}
			else if (AppPkt->app_flag == ZB_LEAVE_NOTIFY)
			{
				uint8* extAddr = osal_mem_alloc(8);
				if (extAddr)
				{
					RTG_DeAllocAddress(MSGpkt->srcAddr.addr.shortAddr);
					AddrMgrExtAddrLookup(MSGpkt->srcAddr.addr.shortAddr, extAddr);
					AssocRemove(extAddr);
					osal_mem_free(extAddr);
				}

				if (ZSuccess == OSmsg2SPIpkt(MSGpkt, pSpi))
				{
					HalSPIWrite((uint8 *)pSpi, pSpi ->DataLength + SPIPKTHDR_LEN);
				}
			}
			else if (ZSuccess == OSmsg2SPIpkt(MSGpkt, pSpi))
			{
				HalSPIWrite((uint8 *)pSpi, pSpi ->DataLength + SPIPKTHDR_LEN);
			}
#endif
			APPWrapper_t* AppPkt = (APPWrapper_t* )(MSGpkt->cmd.Data);
			if (AppPkt->app_flag == SIGNAL_STRENGTH)
			{
				MineApp_RejoinTimes = 0;
				osal_start_timerEx(MineApp_TaskID, MINEAPP_ROUTER_PROBENWK_EVENT, TOLERANCE_TIME);
			}
			else if (AppPkt->app_flag == ZB_LEAVE_NOTIFY)
			{
				uint8* extAddr = osal_mem_alloc(8);
				if (extAddr)
				{
					RTG_DeAllocAddress(MSGpkt->srcAddr.addr.shortAddr);
					if (AddrMgrExtAddrLookup(MSGpkt->srcAddr.addr.shortAddr, extAddr))
						AssocRemove(extAddr);
					osal_mem_free(extAddr);
				}

				afAddrType_t dstaddr;
				dstaddr.endPoint = MINEAPP_ENDPOINT;
				dstaddr.addrMode = afAddr16Bit;
				dstaddr.addr.shortAddr = 0;
				AF_DataRequest(&dstaddr, (endPointDesc_t *)&MineApp_epDesc,
				MINEAPP_CLUSTERID, sizeof(app_LeaveNwk_t), (byte *)&AppPkt,
				&MineApp_TransID, INIT_OPN|AF_ACK_REQUEST, MAX_DEPTH);
			}
			else if (AppPkt->app_flag == CLEARNWK)
			{
				uint16 nodeshortAddr = 0;
				AddrMgrNwkAddrLookup(AppPkt->app_ClearNWK.macAddr, &nodeshortAddr);
				if (nodeshortAddr)
				{
					RTG_DeAllocAddress(nodeshortAddr);
					AssocRemove(AppPkt->app_ClearNWK.macAddr);	
				}
			}
			break;
		}
	case CARD_CLUSTERID:
		HalLedSet(HAL_LED_1, HAL_LED_MODE_TOGGLE);
		MineApp_ParseAppFrame(MSGpkt);
		break;
	default:
		break;
	}
}

#if (defined HAL_SPI) && (HAL_SPI == TRUE)
/*********************************************************************
* @fn      MineApp_MessageSPICB
*
* @brief   SPI message processor callback.  This function processes
*          any incoming data from ARM. Actually, It's based on Zigbee
*	      Coordinator.
*
* @param   MSGpkt - The incoming msg.
*
* @return  none
*/
void MineApp_ProcessSPICB(const SPIMSGPacket_t *MSGpkt)
{
	SPIMSGPacket_t * rxmsg = (SPIMSGPacket_t *)MSGpkt;
	APPWrapper_t* appwrapper = (APPWrapper_t*)(rxmsg+1);
#if defined MINE_TEST
	MineAppTest_MessageSPICB(MSGpkt);
#else

	//handle differernt SPI msg.
	HalLedSet(HAL_LED_1, HAL_LED_MODE_TOGGLE);

	/* Message to Phone*/
	if (rxmsg->spihdr.dstAddr.addr.shortAddr != 0)
	{
		AF_DataRequest((afAddrType_t *)&(rxmsg->spihdr.dstAddr), (endPointDesc_t *)&MineApp_epDesc,
			MINEAPP_CLUSTERID, rxmsg->DataLength, (byte *)(rxmsg+1),
			&rxmsg->spihdr.transID, rxmsg->spihdr.options, rxmsg->spihdr.radius);
	}
	/*Message to Phone | Card | Coord*/
	else
	{
		if(appwrapper->app_flag == URGENT)
		{
			afAddrType_t dstAddr;
			dstAddr.addrMode = afAddrBroadcast;
			dstAddr.addr.shortAddr = NWK_BROADCAST_SHORTADDR_DEVALL;
			dstAddr.endPoint = MINEAPP_ENDPOINT;
			if(appwrapper->app_Urgent.urgenttype == RETREAT) 
			{
				AF_DataRequest(&dstAddr, (endPointDesc_t *)&MineApp_epDesc,
					MINEAPP_CLUSTERID, rxmsg->DataLength, (byte *)(rxmsg+1),
					&(rxmsg->spihdr.transID), rxmsg->spihdr.options, rxmsg->spihdr.radius);
				MineApp_retreat = MINEAPP_URGENT_RETREAT;
				MineApp_urgent_timeout_cnt = 0;
				osal_set_event(MineApp_TaskID, MINEAPP_URGENT_TIME_EVENT);
			}
			else if(appwrapper->app_Urgent.urgenttype == CANCELRETREAT) 
			{
				AF_DataRequest(&dstAddr, (endPointDesc_t *)&MineApp_epDesc,
					MINEAPP_CLUSTERID, rxmsg->DataLength, (byte *)(rxmsg+1),
					&(rxmsg->spihdr.transID), rxmsg->spihdr.options, rxmsg->spihdr.radius);
				MineApp_retreat = MINEAPP_URGENT_CANCELRETREAT;
				MineApp_urgent_timeout_cnt = 0;
				osal_set_event(MineApp_TaskID, MINEAPP_URGENT_TIME_EVENT);
			}
		}
		else if(appwrapper->app_flag == NODESLEEP)
		{
			MineApp_SleepPeriod = appwrapper->app_Sleep.sleeptime;
			MineApp_SettingSleepPeriod = true;
			MineApp_SleepPeriod_timeout_cnt = 0;
			osal_set_event(MineApp_TaskID, MINEAPP_SLEEPPERIOD_TIME_EVENT);
		}
		else if (appwrapper->app_flag == TRY_CONNECTION)
		{
			//Notify ARM the PANID as its id.
			//if (MineApp_NwkState == DEV_ZB_COORD)
			//{
				pSpi->spihdr.srcAddr.addr.shortAddr = 0;
				pSpi->DataLength = sizeof(app_startNwk_t);
				APPWrapper_t* pTryconRSP = (APPWrapper_t*)(pSpi + 1);
				pTryconRSP->app_StartNwk.msgtype = ZB_START_NETWORK;
				pTryconRSP->app_StartNwk.PANID = zgConfigPANID;
				osal_cpyExtAddr(pTryconRSP->app_StartNwk.macAddr, (void *)aExtendedAddress);
				HalSPIWrite((uint8* )pSpi, pSpi->DataLength + SPIPKTHDR_LEN);
				uint8 maxFrameRetries = 1;
				MAC_MlmeSetReq(MAC_MAX_FRAME_RETRIES, &maxFrameRetries);
			//}
		}
		else if (appwrapper->app_flag == DEPUTY_JOIN)
		{
			byte status = MineApp_BuildAssocAddr(&(appwrapper->app_DeputyJoin));
			APPWrapper_t* pDeputyRSP = (APPWrapper_t*)(pSpi + 1);
			//build deputyjoin_RSP pkt for the neighbor station.
			//pSpi->spihdr.srcAddr.addr.shortAddr = 0;

			//To use the pre-join child address as src address.
			pSpi->spihdr.srcAddr.addr.shortAddr = deviceAddr.shortAddr;
			pSpi->DataLength = sizeof(app_DeputyJoinRsp_t);
			pDeputyRSP->app_DeputyJoinrsp.deputyNmbr = appwrapper->app_DeputyJoin.deputyNmbr;
			if (status == ZSUCCESS)
			{
				pDeputyRSP->app_DeputyJoinrsp.msgtype = DEPUTYJOIN_RSP;
				pDeputyRSP->app_DeputyJoinrsp.rsptype = PERMIT_JOIN;
				pDeputyRSP->app_DeputyJoinrsp.nodeAddr = deviceAddr;
			}
			else
			{
				pDeputyRSP->app_DeputyJoinrsp.msgtype = DEPUTYJOIN_RSP;
				pDeputyRSP->app_DeputyJoinrsp.rsptype = NWK_FULL;
			}
			HalSPIWrite((uint8* )pSpi, pSpi->DataLength + SPIPKTHDR_LEN);
		}
		else if(appwrapper->app_flag == TIME_SSIND)
		{
			afAddrType_t dstAddr;
			dstAddr.addrMode = afAddrBroadcast;
			dstAddr.addr.shortAddr = NWK_BROADCAST_SHORTADDR_DEVALL;
			dstAddr.endPoint = MINEAPP_ENDPOINT;

			app_TimessInd_t TimessInd;
			TimessInd = appwrapper->app_TimessInd;
			AF_DataRequest(&dstAddr, (endPointDesc_t *)&MineApp_epDesc,
				MINEAPP_CLUSTERID, sizeof(app_TimessInd_t), (uint8 *)&TimessInd,
				&MineApp_TransID, INIT_OPN,1);
		}
		else if (appwrapper->app_flag == CLEARNWK)
		{
			uint16 nodeshortAddr = 0;
			AddrMgrNwkAddrLookup(appwrapper->app_ClearNWK.macAddr, &nodeshortAddr);
			if (nodeshortAddr)
			{
				RTG_DeAllocAddress(nodeshortAddr);
				AssocRemove(appwrapper->app_ClearNWK.macAddr);	
			}
		}
	}

#endif	
}
#endif

/*********************************************************************
* @fn      MineApp_ParseAppFrame
*
* @brief   This function processes OTA message to app data if needed.
*
* @param   MSGpkt - The incoming msg.
*
* @return  none
*/
byte MineApp_ParseAppFrame(const afIncomingMSGPacket_t* MSGpkt)
{
	APPWrapper_t* AppPkt = (APPWrapper_t* )(MSGpkt->cmd.Data);
	byte appflag = AppPkt->app_flag;
	switch (appflag)
	{
	case SSREQ:
		{
			uint8 reqtype = AppPkt->app_ssReq.reqtype;
			/* If received a SSREQ_OUT or SSREQ_POLL , Send SSIND to ZC*/
			if (reqtype & SSREQ_OUT || reqtype & SSREQ_POLL)
			{
				//construct SPIMSGPacket_t and Data.
				CONV_LQI_TO_RSSI(MineApp_RSSI, MSGpkt->LinkQuality);
				app_ssInd_t ssInd;
				ssInd.rssipkt.RSSI = MineApp_RSSI;
				ssInd.rssipkt.NWK_ADDR = BUILD_UINT16(aExtendedAddress[EXT_MACADDR_DEVID_LBYTE],
									aExtendedAddress[EXT_MACADDR_DEVID_HBYTE]);
				ssInd.rssipkt.NODE_ADDR = MSGpkt->srcAddr.addr.shortAddr;
				ssInd.rssipkt.seqnum = AppPkt->app_ssReq.seqnum;
				ssInd.msgtype  = SSIND;
				MacParam_t macparam;
				macparam.cluster_id = CARD_CLUSTERID;
				macparam.panID = _NIB.nwkPanId;
				MAC_UTIL_BuildandSendDataPAN(&macparam, (uint8 *)&ssInd, sizeof(app_ssInd_t), 
												MAC_UTIL_UNICAST, 0, MAC_TXOPTION_NO_RETRANS);
			}
			/* If poll, Send data to card */
			if (reqtype & SSREQ_POLL)
			{
#ifdef MED_TEST
				HalLedBlink(HAL_LED_2,3, 30, 100);
#endif
				APPWrapper_t Appdata;
				if(MineApp_retreat == MINEAPP_URGENT_RETREAT)
				{
					Appdata.app_Urgent.msgtype = URGENT;
					Appdata.app_Urgent.urgenttype = RETREAT;
					Appdata.app_Urgent.value = 0;
					MAC_UTIL_BuildandSendData((uint8 *)&Appdata,sizeof(app_Urgent_t), MAC_UTIL_UNICAST, MSGpkt->srcAddr.addr.shortAddr,MAC_TXOPTION_NO_RETRANS);
				}
				else if((MineApp_retreat == MINEAPP_URGENT_CANCELRETREAT))
				{
					Appdata.app_Urgent.msgtype = URGENT;
					Appdata.app_Urgent.urgenttype = CANCELRETREAT;
					Appdata.app_Urgent.value = 0;
					MAC_UTIL_BuildandSendData((uint8 *)&Appdata,sizeof(app_Urgent_t), MAC_UTIL_UNICAST, MSGpkt->srcAddr.addr.shortAddr,MAC_TXOPTION_NO_RETRANS);
				}
				else
				{
					if(MineApp_SettingSleepPeriod)
					{
						Appdata.app_Sleep.msgtype = NODESLEEP;
						Appdata.app_Sleep.sleeptime = MineApp_SleepPeriod;
						MAC_UTIL_BuildandSendData((uint8 *)&Appdata,sizeof(app_Sleep_t), MAC_UTIL_UNICAST, MSGpkt->srcAddr.addr.shortAddr,MAC_TXOPTION_NO_RETRANS);
					}
					else
					{
						Appdata.app_ssReq.msgtype = SSREQ;
						Appdata.app_ssReq.reqtype = SSREQ_NODATA;
						Appdata.app_ssReq.NWK_ADDR = CARD_NWK_ADDR;
						MAC_UTIL_BuildandSendData((uint8 *)&Appdata,sizeof(app_ssReq_t), MAC_UTIL_UNICAST, MSGpkt->srcAddr.addr.shortAddr,MAC_TXOPTION_NO_RETRANS);
					}
				}
			}
			break;
		}
	case URGENT:
		{
#ifdef MED_TEST
			HalLedBlink(HAL_LED_2,5, 30, 100);
#endif
			uint8 urgenttype = AppPkt->app_Urgent.urgenttype;
			if (urgenttype == ALERT)
			{
				APPWrapper_t Appdata;
				Appdata.app_Urgent.msgtype = URGENT;
				Appdata.app_Urgent.urgenttype = ALERTACK;
				Appdata.app_Urgent.value = 0;
				MAC_UTIL_BuildandSendData((uint8 *)&Appdata,sizeof(app_Urgent_t), MAC_UTIL_UNICAST, MSGpkt->srcAddr.addr.shortAddr,MAC_TXOPTION_NO_RETRANS);

				//send data to ZC.
				MacParam_t macparam;
				macparam.cluster_id = CARD_CLUSTERID;
				macparam.panID = _NIB.nwkPanId;
				MAC_UTIL_BuildandSendDataPAN(&macparam, (uint8 *)&AppPkt, sizeof(app_Urgent_t), 
												MAC_UTIL_UNICAST, 0, 0);
			}
			else if(urgenttype == NOPWR)
			{
				//send data to ZC.
				MacParam_t macparam;
				macparam.cluster_id = CARD_CLUSTERID;
				macparam.panID = _NIB.nwkPanId;
				MAC_UTIL_BuildandSendDataPAN(&macparam, (uint8 *)&AppPkt, sizeof(app_Urgent_t), 
												MAC_UTIL_UNICAST, 0, 0);
			}
			else if (urgenttype == RETREAT)
			{
				MineApp_retreat = MINEAPP_URGENT_RETREAT;
				MineApp_urgent_timeout_cnt = 0;
				osal_set_event(MineApp_TaskID, MINEAPP_URGENT_TIME_EVENT);			
			}
			else if (urgenttype == CANCELRETREAT)
			{
				MineApp_retreat = MINEAPP_URGENT_CANCELRETREAT;
				MineApp_urgent_timeout_cnt = 0;
				osal_set_event(MineApp_TaskID, MINEAPP_URGENT_TIME_EVENT);			
			}
			break;
		}
	default:
		break;
	}
	return ZSUCCESS;
}

/*********************************************************************
* @fn      MineApp_HandleKeys
*
* @brief   This function is used to handle key event.
*
* @param   keys - key.
*               shifts - 
*
* @return  none
*/
void MineApp_HandleKeys( uint16 keys, byte shifts)
{
	switch (keys)
	{
	case HAL_KEY_SW_7:
#ifdef MINE_TEST
		MineAppTest_HandleKey(keys, shifts);
#endif
#ifdef MED_TEST
		{   
			/*
			if(MineApp_retreat== MINEAPP_URGENT_RETREAT)
			{
			MineApp_retreat = MINEAPP_URGENT_CANCELRETREAT;
			}
			else
			{
			MineApp_retreat = MINEAPP_URGENT_RETREAT;
			}
			MineApp_urgent_timeout_cnt = 0;
			osal_set_event(MineApp_TaskID, MINEAPP_URGENT_TIME_EVENT);
			*/

			MineApp_SleepPeriod = 5000;
			MineApp_SleepPeriod_timeout_cnt = 0;
			MineApp_SettingSleepPeriod = true;
			osal_set_event(MineApp_TaskID, MINEAPP_SLEEPPERIOD_TIME_EVENT);
		}
#endif
		break;
	default:
		break;
	}
}

