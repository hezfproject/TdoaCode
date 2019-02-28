/**************************************************************************************************
Filename:       CardTransferApp_Collector.c
Revised:        $Date: 2010/08/06 18:42:10 $
Revision:       $Revision: 1.2 $

Description -   Ammeter Collector Application: collect ammeter data and transmit to data center
and also repeat the other ammeter data from Collector and transmit to data center.
**************************************************************************************************/
/*********************************************************************
* INCLUDES
*/

#include "ZComDef.h"
#include "CardTransferApp_Collector.h"
#include "CardTransferApp_cfg.h"
#include "AF.h"
#include "OSAL_Tasks.h"
#include "ZDApp.h"
#include "ZDObject.h"
#include "ZDProfile.h"
#include "ZGlobals.h"

#include "hal_drivers.h"
#include "hal_key.h"
#if defined ( LCD_SUPPORTED )
#include "hal_lcd.h"
#endif
#include "hal_led.h"
#include "hal_uart.h"
#include "OnBoard.h"

#include "WatchdogUtil.h"
#include "MacUtil.h"

#include "AppProtocolWrapper.h"
#include "delay.h"

#include "Mac_radio_defs.h"

#ifdef DEBUG_REPORT_NW
#include "StringUtil.h"
#endif

#ifdef CFG_AMMETER_IEC
#include "Ammeter_IEC.h"
#endif

/*********************************************************************
* MACROS
*/

/*********************************************************************
* CONSTANTS
*/

#if (defined DEBUG_REPORT_NW)
//#undef HAL_SPI
#undef DEBUG_ROUTE
#endif

#define CARDTRANSFER_APP_RTRY_TIMES  3

#define RSSI_MIN      (MAC_RADIO_RECEIVER_SENSITIVITY_DBM + MAC_SPEC_ED_MIN_DBM_ABOVE_RECEIVER_SENSITIVITY)
#define RSSI_MAX    MAC_RADIO_RECEIVER_SATURATION_DBM   

#define CONV_LQI_TO_RSSI( rssi,lqi ) \
	st (   \
	rssi = lqi*(RSSI_MAX - RSSI_MIN)/MAC_SPEC_ED_MAX + RSSI_MIN; \
	)

#define CANDIDATE_MAX			5

#define ROUTE_AVAILABLE	0
#define ROUTE_EXPIRY		1
#define ROUTE_INVALID		2

// This list should be filled with Application specific Cluster IDs.
const cId_t CardTransferApp_ClusterList[] =
{
	CARDTRANSFERAPP_CLUSTERID,
	CARDTRANSFERAPP_NWK_CLUSTERID,
};

const SimpleDescriptionFormat_t CardTransferApp_SimpleDesc =
{
	CARDTRANSFERAPP_ENDPOINT,              //  int   Endpoint;
	CARDTRANSFERAPP_PROFID,                //  uint16 AppProfId[2];
	CARDTRANSFERAPP_DEVICEID,              //  uint16 AppDeviceId[2];
	CARDTRANSFERAPP_DEVICE_VERSION,        //  int   AppDevVer:4;
	CARDTRANSFERAPP_FLAGS,                 //  int   AppFlags:4;
	sizeof(CardTransferApp_ClusterList),          //  byte  AppNumInClusters;
	(cId_t *)CardTransferApp_ClusterList,  //  byte *pAppInClusterList;
	sizeof(CardTransferApp_ClusterList),          //  byte  AppNumOutClusters;
	(cId_t *)CardTransferApp_ClusterList   //  byte *pAppOutClusterList;
};

const endPointDesc_t CardTransferApp_epDesc =
{
	CARDTRANSFERAPP_ENDPOINT,
	&CardTransferApp_TaskID,
	(SimpleDescriptionFormat_t *)&CardTransferApp_SimpleDesc,
	noLatencyReqs
};

/*********************************************************************
* TYPEDEFS
*/
typedef struct
{
	uint16 candidateID;
	int8 	 rssi;
	uint8 nodeDepth;
	uint8 counter;
} Candidate_t;

/*********************************************************************
* GLOBAL VARIABLES
*/
uint8 CardTransferApp_TaskID;// Task ID for internal task/event processing.

/*********************************************************************
* EXTERNAL VARIABLES
*/

/*********************************************************************
* EXTERNAL FUNCTIONS
*/

/*********************************************************************
* LOCAL VARIABLES
*/
static devStates_t CardTransferApp_NwkState;

//static uint8 rtryCnt = 0;

static NodeInfo MyInfo = {0, 0, 0, 0, INVALIDLINKQUAL, true, 0}; //Identify the node ID and node depth.

#ifdef DEBUG_CARDTRANSFER
static uint8 DebugBuf[64];
static uint8 cnt;
static bool flag;
#endif

static uint8 txBuf[MAX_DATA_SIZE];
static uint8 txLen = 0;

static RouteEntry_t* pREList = 0;
static int16 reListIndex = 0;
static int16 reValidChild = 0;

static bool IsParentAlive = false;
static uint8 toleranceCnt = 0;
static Candidate_t  candidateArray[CANDIDATE_MAX];

__idata static  int8  CardTransferApp_RSSI;
static uint16 seq;
/*********************************************************************
* LOCAL FUNCTIONS
*/
static void CardTransferApp_ProcessMSGCmd(afIncomingMSGPacket_t *pkt);
static uint8 CardTransferApp_ParseData(afIncomingMSGPacket_t* pkt);

static void InitMyInfo(void);
static uint8 RouteEvaluation(uint16 parentid, uint16 sinkID, uint8 nodeDepth, int8 rssi);
static int8   PollRoute(uint16 parentid, uint8 nodeDepth, int8 rssi);
static void CardTransferApp_ProcessDataLED(void );
#ifdef DEBUG_REPORT_NW
static void ReportRoute(uint8 *data, uint8 len);
#endif //DEBUG_REPORT_NW

/*********************************************************************
* @fn      CardTransferApp_Init
*
* @brief   This is called during OSAL tasks' initialization.
*
* @param   task_id - the Task ID assigned by OSAL.
*
* @return  none
*/
void CardTransferApp_Init( uint8 task_id )
{
	CardTransferApp_TaskID = task_id;
#if(defined WATCHDOG) && (WATCHDOG==TRUE)
	StartWatchDog ( DOGTIMER_INTERVAL_1S );
	osal_set_event ( CardTransferApp_TaskID, CARDTRANSFERAPP_FEEDWATCHDOG_EVENT);
#endif

	/* EP and  Profile Id are not used,Card will drop them directly */
	MacUtil_t MacUtil;
	MacUtil.panID = 0xFFFF;  /* Broadcast.*/
	MacUtil.dst_endpoint = CARDTRANSFERAPP_ENDPOINT;
	MacUtil.src_endpoint = 0x20;//From sniffer.
	MacUtil.profile_id = 0x0100;//From sniffer.
	MacUtil.cluster_id = CARDTRANSFERAPP_CLUSTERID;
	MacUtil.NodeType = ZDO_Config_Node_Descriptor.LogicalType;
	MAC_UTIL_INIT ( &MacUtil );

	afRegister( (endPointDesc_t *)&CardTransferApp_epDesc );

	pREList = osal_mem_alloc(MAXNEIGHBOR*sizeof(RouteEntry_t));
	if(!pREList)
		SystemReset();
	osal_memset((void *)pREList, 0, MAXNEIGHBOR*sizeof(RouteEntry_t));
	osal_memset((void *)candidateArray, 0, 5*sizeof(Candidate_t));
}

/*********************************************************************
* @fn      CardTransferApp_ProcessEvent
*
* @brief   Generic Application Task event processor.
*
* @param   task_id  - The OSAL assigned task ID.
* @param   events   - Bit map of events to process.
*
* @return  none
*/
//uint8 rxbuf[16];
UINT16 CardTransferApp_ProcessEvent( uint8 task_id, UINT16 events )
{
#if(defined WATCHDOG) && (WATCHDOG==TRUE)
	if ( events & CARDTRANSFERAPP_FEEDWATCHDOG_EVENT )
	{
		osal_start_timerEx ( CardTransferApp_TaskID, CARDTRANSFERAPP_FEEDWATCHDOG_EVENT, 300 );
		FEEDWATCHDOG();
		return events ^ CARDTRANSFERAPP_FEEDWATCHDOG_EVENT;
	}
#endif
	if ( events & SYS_EVENT_MSG )
	{
		afIncomingMSGPacket_t *MSGpkt;
		while ( (MSGpkt = (afIncomingMSGPacket_t *)osal_msg_receive(
			CardTransferApp_TaskID)) )
		{
			switch ( MSGpkt->hdr.event )
			{
			case ZDO_STATE_CHANGE:
				{
					CardTransferApp_NwkState = (devStates_t)(MSGpkt->hdr.status);
					if (CardTransferApp_NwkState == DEV_ZB_COORD)
					{
						if (ZSUCCESS != osal_start_timerEx(CardTransferApp_TaskID, CARDTRANSFERAPP_ROUTE_EXPIRE_EVT, 10000/FREQUENCY_MULTIPLICATION))
							SystemReset();
						if (ZSUCCESS != osal_start_timerEx(CardTransferApp_TaskID, CARDTRANSFERAPP_ROUTE_ALIVE_EVT, 60000/FREQUENCY_MULTIPLICATION))
							SystemReset();
						if (ZSUCCESS != osal_start_timerEx(CardTransferApp_TaskID, CARDTRANSFERAPP_ROUTE_REPORT_EVT, 20000/FREQUENCY_MULTIPLICATION))
							SystemReset();
						if (ZSUCCESS != osal_start_timerEx(CardTransferApp_TaskID, CARDTRANSFERAPP_ROUTE_REPAIR_EVT, 65000/FREQUENCY_MULTIPLICATION))
							SystemReset();
					#ifdef DEBUG_REPORT_NW
						if (ZSUCCESS != osal_start_timerEx(CardTransferApp_TaskID, CARDTRANSFERAPP_REPORT_NW_EVT, 10000/FREQUENCY_MULTIPLICATION))
							SystemReset();						
					#endif
					}
					break;
				}
			case AF_INCOMING_MSG_CMD:
				CardTransferApp_ProcessMSGCmd( MSGpkt );
				break;

			default:
				break;
			}

			osal_msg_deallocate( (uint8 *)MSGpkt );  // Release the memory.
		}

		//Return unprocessed events.
		return ( events ^ SYS_EVENT_MSG );
	}
	
	if (events & CARDTRANSFERAPP_ROUTE_REPORT_EVT)
	{
		Nwk_RouteRsp_t rtRsp;
		rtRsp.msgtype = ROUTERSP;
		rtRsp.dstAddress = MyInfo.sinkID; //report to sink node.
		rtRsp.srcAddress = MyInfo.nodeID;
		rtRsp.seq = 0;
		rtRsp.nodeID = MyInfo.nodeID;
		rtRsp.nodeDepth = MyInfo.nodeDepth;

		MacParam_t param;
		param.panID = MyInfo.parentID;
		param.cluster_id = CARDTRANSFERAPP_NWK_CLUSTERID;
		param.radius = 0x01;
		if (MyInfo.nodeDepth)
			MAC_UTIL_BuildandSendDataPAN(&param, (uint8 *)&rtRsp,
			sizeof(Nwk_RouteRsp_t), MAC_UTIL_UNICAST, 0, 0);

		if (CARDTRANSFER_ROUTE_EXPIRETIME > 0)
		{
			uint16 RandDelay = MAC_RandomByte();             
			RandDelay <<= 2; 
			if (ZSUCCESS != osal_start_timerEx( CardTransferApp_TaskID, CARDTRANSFERAPP_ROUTE_REPORT_EVT, CARDTRANSFER_ROUTE_EXPIRETIME/FREQUENCY_MULTIPLICATION + RandDelay))
				SystemReset();
#ifdef DEBUG_REPORT_NW
			if(ZSuccess != osal_set_event(CardTransferApp_TaskID, CARDTRANSFERAPP_REPORT_NW_EVT))
			{
				SystemReset();
			}
#endif

		}

		return events ^ CARDTRANSFERAPP_ROUTE_REPORT_EVT;
	}
	
	if (events & CARDTRANSFERAPP_ROUTE_EXPIRE_EVT)
	{
		int16 i;
		for (i = 0; i < reListIndex; ++i)
		{
			if (pREList[i].expiryTime)
			{
				pREList[i].expiryTime--;
				if (pREList[i].expiryTime == 0) reValidChild --;
			}
		}

		if (CARDTRANSFER_ROUTE_EXPIRETIME)
		{
			if (ZSUCCESS != osal_start_timerEx(CardTransferApp_TaskID, CARDTRANSFERAPP_ROUTE_EXPIRE_EVT, CARDTRANSFER_ROUTE_EXPIRETIME/FREQUENCY_MULTIPLICATION))
				SystemReset();
		}
		return events ^ CARDTRANSFERAPP_ROUTE_EXPIRE_EVT;
	}

	if (events & CARDTRANSFERAPP_ROUTE_ALIVE_EVT)
	{
		MacParam_t param;
		param.panID = 0xFFFF;
		param.cluster_id = CARDTRANSFERAPP_NWK_CLUSTERID;
		param.radius = 0x01;
		Nwk_RouteAlive_t rtAlive;
		rtAlive.msgtype = ROUTEALIVE;
		rtAlive.nodeID = MyInfo.nodeID;
		rtAlive.nodeDepth = MyInfo.nodeDepth;
		rtAlive.srcAddress = MyInfo.sinkID;
		rtAlive.dstAddress = 0xFFFF;
		rtAlive.seq = 0;
		if (MyInfo.nodeDepth)
			MAC_UTIL_BuildandSendDataPAN(&param, (uint8 *)&rtAlive,
			sizeof(Nwk_RouteAlive_t), MAC_UTIL_BROADCAST, 0, 0);	
		if (CARDTRANSFER_ROUTE_ALIVETIME)
		{
			if (ZSUCCESS != osal_start_timerEx(CardTransferApp_TaskID, CARDTRANSFERAPP_ROUTE_ALIVE_EVT, CARDTRANSFER_ROUTE_ALIVETIME/FREQUENCY_MULTIPLICATION))
				SystemReset();
		}
		return events ^ CARDTRANSFERAPP_ROUTE_ALIVE_EVT;
	}

	if (events & CARDTRANSFERAPP_ROUTE_REPAIR_EVT)
	{
		if (IsParentAlive)
		{
			IsParentAlive = false;
			toleranceCnt = 0;
		}

		else if (!MyInfo.needRepair)
			toleranceCnt++;

		if (toleranceCnt >=CARDTRANSFER_ROUTE_TOLERANCECNT)
		{
			toleranceCnt = 0;
			//Clear my route info.
			InitMyInfo();
			//HalLedSet( HAL_LED_ROUTE, HAL_LED_MODE_OFF );
			//MyInfo.parentQuality = INVALIDLINKQUAL;
			//MyInfo.nodeID = 0;
			//MyInfo.nodeDepth = 0;
			//MyInfo.needRepair = true;
		}
		if (CARDTRANSFER_ROUTE_ALIVETIME)
		{
			if (ZSUCCESS != osal_start_timerEx(CardTransferApp_TaskID, CARDTRANSFERAPP_ROUTE_REPAIR_EVT, CARDTRANSFER_ROUTE_ALIVETIME/FREQUENCY_MULTIPLICATION))
				SystemReset();
		}
		return events ^ CARDTRANSFERAPP_ROUTE_REPAIR_EVT;
	}
	
#ifdef DEBUG_REPORT_NW
	if (events & CARDTRANSFERAPP_REPORT_NW_EVT)
	{
		if (reValidChild==0)
		{	
			ReportRoute(0 ,0);
		}
		return events ^ CARDTRANSFERAPP_REPORT_NW_EVT;
	}
#endif

	return ( 0 );  // Discard unknown events.
}


/*********************************************************************
* @fn      CardTransferApp_ProcessMSGCmd
*
* @brief   Data message processor callback. This function processes
*          any incoming data - probably from other devices. Based
*          on the cluster ID, perform the intended action.
*
* @param   pkt - pointer to the incoming message packet
*
* @return  TRUE if the 'pkt' parameter is being used and will be freed later,
*          FALSE otherwise.
*/
void CardTransferApp_ProcessMSGCmd( afIncomingMSGPacket_t *pkt )
{
	int8 rssi = INVALIDLINKQUAL;
	CONV_LQI_TO_RSSI(rssi, pkt->LinkQuality);
	switch ( pkt->clusterId )
	{
	case CARDTRANSFERAPP_CLUSTERID:
		{	
			if (MyInfo.parentID)
			{
				CardTransferApp_ProcessDataLED();
				CardTransferApp_ParseData(pkt);
			}
			break;
		}
	case CARDTRANSFERAPP_NWK_CLUSTERID:
		{
			if (pkt->cmd.Data[0] == ROUTEREQ)
			{
				MacParam_t param;
				Nwk_RouteReq_t *pRReq = (Nwk_RouteReq_t *)pkt->cmd.Data;

				if(rssi > TOLERANCERSSI && MyInfo.parentID == pRReq->nodeID)
				{
					IsParentAlive = true;
				}
				
				if ((((MyInfo.needRepair && MyInfo.nodeDepth == 0)
					||(MyInfo.nodeDepth != 0 && pRReq->nodeDepth + 1 < MyInfo.nodeDepth))
					&& rssi > TOLERANCERSSI)
					|| (pRReq->nodeDepth + 1 == MyInfo.nodeDepth && rssi > MyInfo.parentQuality))
				{
					//MyInfo.alivecnt = 2*AMMETER_ROUTE_TOLERANCECNT + 1;
					MyInfo.needRepair = false;
					MyInfo.parentID = pRReq->nodeID;
					MyInfo.nodeID = zgConfigPANID;
					MyInfo.nodeDepth = pRReq->nodeDepth + 1;
					MyInfo.sinkID = pRReq->srcAddress; //Do we need fixed sink ID.
					CONV_LQI_TO_RSSI(MyInfo.parentQuality, pkt->LinkQuality);
					IsParentAlive = true;
					MyInfo.cleanDepth = 0;
					HalLedSet( HAL_LED_ROUTE, HAL_LED_MODE_ON);

					Nwk_RouteRsp_t rtRsp;
					rtRsp.msgtype = ROUTERSP;
					rtRsp.dstAddress = pRReq->srcAddress;
					rtRsp.seq = pRReq->seq;
					rtRsp.srcAddress = MyInfo.nodeID;
					rtRsp.nodeID = MyInfo.nodeID;
					rtRsp.nodeDepth = MyInfo.nodeDepth;

					param.panID = MyInfo.parentID;
					param.cluster_id = CARDTRANSFERAPP_NWK_CLUSTERID;
					param.radius = 0x01;
					MAC_UTIL_BuildandSendDataPAN(&param, (uint8 *)&rtRsp,
						sizeof(Nwk_RouteRsp_t), MAC_UTIL_UNICAST, 0, 0);

					param.panID = 0xFFFF;
					param.cluster_id = CARDTRANSFERAPP_NWK_CLUSTERID;
					param.radius = 0x01;
					Nwk_RouteReq_t rtReq;
					rtReq.msgtype = ROUTEREQ;
					rtReq.nodeID = MyInfo.nodeID;
					rtReq.nodeDepth = MyInfo.nodeDepth;
					rtReq.srcAddress = pRReq->srcAddress;
					rtReq.dstAddress = 0xFFFF;
					rtReq.seq = pRReq->seq;
					MAC_UTIL_BuildandSendDataPAN(&param, (uint8 *)&rtReq,
						sizeof(Nwk_RouteReq_t), MAC_UTIL_BROADCAST, 0, 0);
				}
				/*
				else if (pRReq->dstAddress != 0xFFFF) //For route discovery.
				{
				int16 i = 0;
				bool IsRouteExist = false;
				for (; i < reListIndex; ++i)
				{
				if (pREList[i].dstAddress == pRReq->dstAddress)
				{
				IsRouteExist = true;
				break;
				}
				}
				if (IsRouteExist)
				{
				Nwk_RouteRsp_t rtRsp;
				rtRsp.msgtype = ROUTERSP;
				rtRsp.dstAddress = pRReq->srcAddress;
				rtRsp.seq = pRReq->seq;
				rtRsp.srcAddress = MyInfo.nodeID;
				rtRsp.nodeID = MyInfo.nodeID;
				rtRsp.nodeDepth = MyInfo.nodeDepth;

				param.panID = MyInfo.parentID;
				param.cluster_id = AMMETERAPP_NWK_CLUSTERID;
				param.radius = 0x01;
				MAC_UTIL_BuildandSendDataPAN(&param, (uint8 *)&rtRsp,
				sizeof(Nwk_RouteRsp_t), MAC_UTIL_UNICAST, 0, 0);
				}
				else
				{	
				Nwk_RouteReq_t rtReq;
				rtReq.msgtype = ROUTEREQ;
				rtReq.nodeID = MyInfo.nodeID;
				rtReq.nodeDepth = MyInfo.nodeDepth;
				rtReq.srcAddress = pRReq->srcAddress;
				rtReq.dstAddress = 0xFFFF;
				rtReq.seq = pRReq->seq;

				param.panID = 0xFFFF;
				param.cluster_id = AMMETERAPP_NWK_CLUSTERID;
				param.radius = 0x01;
				MAC_UTIL_BuildandSendDataPAN(&param, (uint8 *)&rtReq,
				sizeof(Nwk_RouteReq_t), MAC_UTIL_BROADCAST, 0, 0);		
				}
				}
				*/
			}
			else if (pkt->cmd.Data[0] == ROUTERSP && MyInfo.nodeID != 0)
			{
				Nwk_RouteRsp_t *pRRsp = (Nwk_RouteRsp_t *)pkt->cmd.Data;
				Nwk_RouteRsp_t rtRsp;
				int16 i = 0;
				bool IsRouteExist = false;
				rtRsp.msgtype = ROUTERSP;
				rtRsp.dstAddress = pRRsp->dstAddress;
				rtRsp.seq = pRRsp->seq;
				rtRsp.srcAddress = pRRsp->srcAddress;
				rtRsp.nodeID = MyInfo.nodeID;
				rtRsp.nodeDepth = MyInfo.nodeDepth;

				if (reListIndex < MAXNEIGHBOR)
				{
					for (; i < reListIndex; ++i)
					{
						if (pREList[i].dstAddress == pRRsp->srcAddress)
						{
							pREList[i].nextHopAddress = pRRsp->nodeID;
							if (pREList[i].expiryTime == 0) reValidChild++;
							pREList[i].expiryTime = CARDTRANSFER_ROUTE_EXPIRECNT;
							IsRouteExist = true;
							break;
						}
					}
					if (!IsRouteExist)
					{
						pREList[reListIndex].dstAddress = pRRsp->srcAddress;
						pREList[reListIndex].nextHopAddress = pRRsp->nodeID;
						pREList[reListIndex++].expiryTime = CARDTRANSFER_ROUTE_EXPIRECNT;
						reValidChild++;
					}

					if (pRRsp->dstAddress != MyInfo.nodeID)
					{
						MacParam_t param;
						param.panID = MyInfo.parentID;
						param.cluster_id = CARDTRANSFERAPP_NWK_CLUSTERID;
						param.radius = 0x01;
						MAC_UTIL_BuildandSendDataPAN(&param, (uint8 *)&rtRsp,
							sizeof(Nwk_RouteRsp_t), MAC_UTIL_UNICAST, 0, 0);	
					}
				}
				else
				{	
					//Compact pREList.
					uint8* pREtmp = osal_mem_alloc(MAXNEIGHBOR*sizeof(RouteEntry_t));
					uint8* ptmp = pREtmp;
					int16 newCnt = 0;
					if (pREtmp)
					{
						osal_memset(pREtmp, 0, MAXNEIGHBOR*sizeof(RouteEntry_t));
						for (i = 0; i < reListIndex; ++i)
						{
							if (pREList[i].expiryTime)
							{
								osal_memcpy(ptmp, (uint8 *)&pREList[i], sizeof(RouteEntry_t));
								ptmp += sizeof(RouteEntry_t);
								newCnt++;
							}
						}
						osal_memset((uint8 *)pREList, 0, MAXNEIGHBOR*sizeof(RouteEntry_t));
						osal_memcpy((uint8 *)pREList, pREtmp, newCnt*sizeof(RouteEntry_t));
						reListIndex = newCnt;
						osal_mem_free(pREtmp);
					}			
				}

			}
			else if (pkt->cmd.Data[0] == ROUTEALIVE)
			{
				MacParam_t param;
				Nwk_RouteAlive_t* pRAlive = (Nwk_RouteAlive_t *)pkt->cmd.Data;
				uint8 rtnStatus = RouteEvaluation(pRAlive->nodeID, pRAlive->srcAddress, pRAlive->nodeDepth, rssi);
				if (ROUTE_EXPIRY == rtnStatus)
				{
					HalLedSet( HAL_LED_ROUTE, HAL_LED_MODE_ON);

					Nwk_RouteRsp_t rtRsp;
					rtRsp.msgtype = ROUTERSP;

					//rtRsp.dstAddress = pRAlive->srcAddress;
					rtRsp.dstAddress = MyInfo.sinkID;
					rtRsp.seq = pRAlive->seq;
					rtRsp.srcAddress = MyInfo.nodeID;
					rtRsp.nodeID = MyInfo.nodeID;
					rtRsp.nodeDepth = MyInfo.nodeDepth;

					param.panID = MyInfo.parentID;
					param.cluster_id = CARDTRANSFERAPP_NWK_CLUSTERID;
					param.radius = 0x01;
					MAC_UTIL_BuildandSendDataPAN(&param, (uint8 *)&rtRsp,
						sizeof(Nwk_RouteRsp_t), MAC_UTIL_UNICAST, 0, 0);			
				}
				else if (ROUTE_AVAILABLE == rtnStatus)
				{
					IsParentAlive = true;
				}
			}
			else if (pkt->cmd.Data[0] == ROUTECLEAN)
			{
				InitMyInfo();
				MacParam_t param;
				Nwk_RouteClean_t *pRClean = (Nwk_RouteClean_t *)pkt->cmd.Data;
				if (pRClean->nodeDepth + 1 < MyInfo.cleanDepth
					|| MyInfo.cleanDepth == 0)
				{
					MyInfo.cleanDepth = pRClean->nodeDepth + 1;				
					param.panID = 0xFFFF;
					param.cluster_id = CARDTRANSFERAPP_NWK_CLUSTERID;
					param.radius = 0x01;
					Nwk_RouteClean_t rtClean;
					rtClean.msgtype = ROUTECLEAN;
					rtClean.nodeID = MyInfo.nodeID;
					rtClean.nodeDepth = MyInfo.cleanDepth;
					rtClean.srcAddress = pRClean->srcAddress;
					rtClean.dstAddress = 0xFFFF;
					rtClean.seq = pRClean->seq;
					osal_memset((void *)pREList, 0, MAXNEIGHBOR*sizeof(RouteEntry_t));
					reListIndex = 0;
					reValidChild = 0;

					MAC_UTIL_BuildandSendDataPAN(&param, (uint8 *)&rtClean,
						sizeof(Nwk_RouteClean_t), MAC_UTIL_BROADCAST, 0, 0);
				}
			}

		#ifdef DEBUG_REPORT_NW
			else if (pkt->cmd.Data[0] == ROUTEREPORT)
			{
				ReportRoute(&(pkt->cmd.Data[1]), pkt->cmd.DataLength-1);
			}
		#endif 
			break;
		}
	}
}

uint8 CardTransferApp_ParseData(afIncomingMSGPacket_t* pkt)
{
	APPWrapper_t* appdata = (APPWrapper_t *) (pkt->cmd.Data);
	MacParam_t param;
	param.cluster_id = CARDTRANSFERAPP_CLUSTERID;
	param.radius = 0x01;
	
	//Transmit to the other collector.
	if (appdata->app_flag == DATA_CROSS_PAN && appdata->app_dataCrossPan.dstPan != MyInfo.nodeID)
	{
		int16 i = 0;
		for(; i < reListIndex; ++i)
		{
			//handle broadcasting
			if (appdata->app_dataCrossPan.dstPan == 0xFFFF
				&& pREList[i].expiryTime)
			{
				param.panID = pREList[i].nextHopAddress;
				MAC_UTIL_BuildandSendDataPAN(&param, pkt->cmd.Data,
					pkt->cmd.DataLength, MAC_UTIL_BROADCAST, 0, MAC_TXOPTION_ACK);
				continue;
			}
			if (appdata->app_dataCrossPan.dstPan == pREList[i].dstAddress
				&& pREList[i].expiryTime)
			{
				param.panID = pREList[i].nextHopAddress;
				MAC_UTIL_BuildandSendDataPAN(&param, pkt->cmd.Data,
					pkt->cmd.DataLength, MAC_UTIL_UNICAST, 0, MAC_TXOPTION_ACK);	
				break;
			}
			else if (appdata->app_dataCrossPan.srcPan == pREList[i].dstAddress
				&& pREList[i].expiryTime)
			{
				param.panID = MyInfo.parentID;
				MAC_UTIL_BuildandSendDataPAN(&param, pkt->cmd.Data,
					pkt->cmd.DataLength, MAC_UTIL_UNICAST, 0, MAC_TXOPTION_ACK);
				break;
			}
		}	
		return 0;
	}
	
	if (appdata->app_flag == SSREQ)
	{
		app_ssInd_t ssInd;
		ssInd.msgtype  = SSIND;
		CONV_LQI_TO_RSSI( CardTransferApp_RSSI, pkt->LinkQuality);
		ssInd.rssipkt.RSSI = CardTransferApp_RSSI;
		ssInd.rssipkt.NWK_ADDR = zgConfigPANID;//appdata->app_ssReq.NWK_ADDR;
		ssInd.rssipkt.NODE_ADDR = pkt->srcAddr.addr.shortAddr;
		ssInd.rssipkt.seqnum = appdata->app_ssReq.seqnum;

		app_dataCrossPan_t dataCrossPan;
		dataCrossPan.msgtype = DATA_CROSS_PAN;
		dataCrossPan.srcPan = MyInfo.nodeID;
		dataCrossPan.dstPan = MyInfo.sinkID;
		dataCrossPan.seqnum = seq++;
		dataCrossPan.packnum = 1;
		dataCrossPan.packseq = 1;
		dataCrossPan.len = sizeof(app_ssInd_t);

		osal_memcpy(txBuf, &dataCrossPan, sizeof(app_dataCrossPan_t));
		osal_memcpy(txBuf+sizeof(app_dataCrossPan_t),
			&ssInd, sizeof(app_ssInd_t));
		
		param.panID = MyInfo.parentID;
		MAC_UTIL_BuildandSendDataPAN(&param, txBuf,
			sizeof(app_dataCrossPan_t)+sizeof(app_ssInd_t), MAC_UTIL_UNICAST, 0, MAC_TXOPTION_ACK);	
	}
	
	return 0;
}

void InitMyInfo(void)
{
	MyInfo.nodeID = 0;
	MyInfo.nodeDepth = 0;
	MyInfo.sinkID = 0;
	MyInfo.parentID = 0;
	MyInfo.parentQuality = INVALIDLINKQUAL;
	MyInfo.needRepair = true;
	HalLedSet( HAL_LED_ROUTE, HAL_LED_MODE_OFF);
}

/*********************************************************************
* @fn      RouteEvaluation
*
* @brief   Evaluate route quality.
*
* @param 	Depth - depth of the request node.
*			rssi - the rssi of the request pkt.
*
* @return  ROUTE_EXPIRY - need to update the route.
*   		   ROUTE_INVALID - route don't need to be updated.
*		   ROUTE_AVAILABLE - unavailable route.	
*/
uint8 RouteEvaluation(uint16 parentid, uint16 sinkID, uint8 nodeDepth, int8 rssi)
{
	if (rssi > TOLERANCERSSI)
	{
		//new comer or bad route, evaluate and search a healthy one.
		if (MyInfo.needRepair || MyInfo.nodeDepth == 0)
		{
			int8 idx;
			if ((idx = PollRoute(parentid, nodeDepth, rssi)) >= 0)
			{
				MyInfo.needRepair = false;
				MyInfo.parentID = candidateArray[idx].candidateID; 
				MyInfo.nodeID = zgConfigPANID;
				MyInfo.nodeDepth = candidateArray[idx].nodeDepth + 1;
				MyInfo.sinkID = sinkID;
				MyInfo.parentQuality = rssi;
				MyInfo.cleanDepth = 0;

				//clear the cache buffer
				osal_memset(candidateArray, 0, CANDIDATE_MAX*sizeof(Candidate_t));  

				return ROUTE_EXPIRY;
			}
			else
			{
				return ROUTE_INVALID;
			}
		}
		else if (MyInfo.parentID == parentid && MyInfo.nodeDepth != 0)
		{
			return ROUTE_AVAILABLE;
		}
		else
		{
			return ROUTE_INVALID;
		}

		/*
		if (MyInfo.parentID != parentid 
		&& MyInfo.nodeDepth != 0 
		&& Depth + 1 < MyInfo.nodeDepth)
		{
		return ROUTE_EXPIRY;
		}
		*/
	}

	return ROUTE_INVALID;
}

// return the route idx in candidate Array, 
// if no need to update route, return -1

int8   PollRoute(uint16 parentid, uint8 nodeDepth, int8 rssi)
{
	uint8 i;
	bool Routeflag = false;

	//if the id is in my children list,  give up rebuild route 
	for (i = 0; i < reListIndex; ++i)
	{
		if (pREList[i].expiryTime >0 && pREList[i].dstAddress == parentid)
		{
			return -1;
		}
	}

	for (i = 0; i < CANDIDATE_MAX; ++i)
	{
		if (parentid == candidateArray[i].candidateID)
		{
			candidateArray[i].nodeDepth= nodeDepth;
			candidateArray[i].rssi = rssi;
			candidateArray[i].counter++;

			if (candidateArray[i].counter >= 2*CARDTRANSFER_ROUTE_TOLERANCECNT)
			{
				//osal_memset(candidateArray, 0, CANDIDATE_MAX*sizeof(Candidate_t));
				Routeflag =  true;
			}
			else
				break;
		}
		else if (candidateArray[i].candidateID == 0)
		{
			candidateArray[i].candidateID = parentid;
			candidateArray[i].nodeDepth= nodeDepth;
			candidateArray[i].rssi = rssi;
			candidateArray[i].counter++;
			break;
		}
	}


	if(Routeflag)  // there is someone counter >= 10
	{

		// find the mindepth from where (conter > 7)
		uint8 minDepth = 255;

		for (i = 0; i < CANDIDATE_MAX; ++i)
		{
			if(candidateArray[i].counter > 1.5*CARDTRANSFER_ROUTE_TOLERANCECNT)
			{	
				if(candidateArray[i].nodeDepth < minDepth)
				{
					minDepth = candidateArray[i].nodeDepth;
				}
			}
		}

		// find the index with max rssi where (counter>7 and depth==mindepth)
		uint8 idx = 0;
		int8 maxrssi = -127;

		for (i = 0; i < CANDIDATE_MAX; ++i)
		{
			if(candidateArray[i].counter > 1.5*CARDTRANSFER_ROUTE_TOLERANCECNT 
				&& candidateArray[i].nodeDepth ==  minDepth)
			{	
				if(candidateArray[i].rssi > maxrssi)
				{
					maxrssi = candidateArray[i].rssi;
					idx = i;
				}
			}
		}

		if(idx < CANDIDATE_MAX)
			return idx;
	}

	return -1;
}

static void CardTransferApp_ProcessDataLED(void )
{
	HalLedSet(HAL_LED_DATA,HAL_LED_MODE_OFF);
	HalLedBlink(HAL_LED_DATA, 1, 50, 300);
}

#ifdef DEBUG_REPORT_NW
static void ReportRoute(uint8 *data, uint8 len)
{
	MacParam_t param;

	if (MyInfo.sinkID == 0) return;

	param.panID = MyInfo.parentID;
	param.cluster_id = CARDTRANSFERAPP_NWK_CLUSTERID;
	param.radius = 0x01;

	txBuf[0] = ROUTEREPORT;
	txBuf[1] = (MyInfo.nodeID & 0xFF00) >> 8;
	txBuf[2] = MyInfo.nodeID & 0xFF;

	osal_memcpy(txBuf+3, data, len);
	txLen = 3 + len;
	if (txLen <= MAX_DATA_SIZE)
		MAC_UTIL_BuildandSendDataPAN(&param, txBuf,
			txLen, MAC_UTIL_UNICAST, 0, MAC_TXOPTION_ACK);	

}
#endif //DEBUG_REPORT_NW 
/*********************************************************************
*********************************************************************/

