/**************************************************************************************************
Filename:       CardTransferApp_Datacenter.c
Revised:        $Date: 2010/08/13 23:23:41 $
Revision:       $Revision: 1.3 $

Description -  Cadr Datacenter Application:Ammeter data center.
**************************************************************************************************/
/*********************************************************************
* INCLUDES
*/
#include "ZComDef.h"
#include "CardTransferApp_DataCenter.h"
#include "CardTransferApp_cfg.h"
#include "AF.h"
#include "OSAL_Tasks.h"

#if !defined( NONWK )
#include "ZDApp.h"
#include "ZDObject.h"
#include "ZDProfile.h"
#include "ZGlobals.h"
#endif

#include "hal_drivers.h"
#include "hal_key.h"
#if defined ( LCD_SUPPORTED )
#include "hal_lcd.h"
#endif
#include "hal_led.h"
#include "hal_spi.h"
#include "OnBoard.h"

#include "WatchdogUtil.h"
#include "MacUtil.h"
#include "AppProtocolWrapper.h"

#include "Rtg.h"
#include "AssocList.h"
#include "AddrMgr.h"
#include "delay.h"

#ifdef CFG_AMMETER_CMDLINE
#include "String.h"
#endif
#ifdef DEBUG_REPORT_NW
#include "StringUtil.h"
#endif

/*********************************************************************
* MACROS
*/
#define CARDTRANSFERAPP_PARAM_ADDR 			0xFEF0

#ifndef CARDTRANSFERAPP_SPIERR_TIMEOUT
#define CARDTRANSFERAPP_SPIERR_TIMEOUT  3
#endif

/*********************************************************************
* CONSTANTS
*/

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

typedef struct
{
	uint8  ResetFlag;
} storeParam_t;

typedef struct
{
	uint8* databuf;
	uint8 datasize;
	int16 reIndex;
} AmmeterDataTrans_t;

/*********************************************************************
* TYPEDEFS
*/

/*********************************************************************
* GLOBAL VARIABLES
*/
uint8 CardTransferApp_TaskID;    // Task ID for internal task/event processing.

/*********************************************************************
* LOCAL VARIABLES
*/
static devStates_t CardTransferApp_NwkState;
//static uint8 CardTransferApp_TransID;
static byte CardTransferApp_Msg[CARDTRANSFERAPP_MAX_DATA_LEN];
static SPIMSGPacket_t* pSpi = ( SPIMSGPacket_t* ) CardTransferApp_Msg;
//static AmmeterDataTrans_t CardTransferApp_DataTrans = {0, 0, 0};
static NodeInfo MyInfo = {0, 0, 0, 0, INVALIDLINKQUAL, true, 0}; //Identify the node ID and node depth.
static RouteEntry_t* pREList = 0;
static int16 reListIndex = 0;
static uint8 rtBuildCnt = 0;
static uint8 rtCleanCnt = 0;
static uint8 pktSeqnum = 0;

#ifdef DEBUG_ROUTE
static RouteEntry_t* pREListCopy;
static int16 printIdx = -1;
static uint8 aliveRoute = 0;
#endif
static uint8 SPIErrCnt = 0;

/*********************************************************************
* LOCAL FUNCTIONS
*/
static void CardTransferApp_ProcessSPICB (const SPIMSGPacket_t *MSGpkt);
static void CardTransferApp_ProcessMSGCB (const afIncomingMSGPacket_t *MSGpkt);
static void CardTransferApp_ProcessDataLED(void);
static void CardTransferApp_StartProbeSpiEvt ( void );
static void CardTransferApp_StopProbeSpiEvt ( void );
#if 0
static void  CardTransferApp_SendData(app_amdata_t *pHead,  uint8*pData, uint16 len);
#endif

#ifdef DEBUG_REPORT_NW
static void PrintRoute(uint8* data, uint8 len);
#endif

/*********************************************************************
* FUNCTIONS
*********************************************************************/
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

#if (defined HAL_SPI) && (HAL_SPI==TRUE)
	halSPICfg_t spiConfig;
	spiConfig.maxBufSize = DMA_MAX;
	spiConfig.intEnable = false;
	HalSpiStart ( &spiConfig );
	RegisterForSpi ( CardTransferApp_TaskID );
#endif

	afRegister((endPointDesc_t *)&CardTransferApp_epDesc);

	pSpi= ( SPIMSGPacket_t * ) CardTransferApp_Msg;
	pSpi->spihdr.hdr.event = SPI_RX;
	pSpi->spihdr.srcAddr.addrMode = ( AddrMode_t ) Addr16Bit;
	pSpi->spihdr.srcAddr.endPoint = CARDTRANSFERAPP_ENDPOINT;
	pSpi->spihdr.dstAddr.addrMode = ( AddrMode_t ) Addr16Bit;
	pSpi->spihdr.dstAddr.endPoint = CARDTRANSFERAPP_ENDPOINT;
	pSpi->spihdr.transID = INIT_TRANID;
	pSpi->spihdr.options = INIT_OPN;
	pSpi->spihdr.radius = MAX_DEPTH;

	/* EP and  Profile Id are not used,Card will drop them directly */
	MacUtil_t MacUtil;
	MacUtil.panID = 0xFFFF;  /* Communication with Card */
	MacUtil.dst_endpoint = CARDTRANSFERAPP_ENDPOINT;
	MacUtil.src_endpoint = 0x20;//From sniffer.
	MacUtil.profile_id = 0x0100; //From sniffer.
	MacUtil.cluster_id = CARDTRANSFERAPP_CLUSTERID;
	MacUtil.NodeType = ZDO_Config_Node_Descriptor.LogicalType;
	MAC_UTIL_INIT (&MacUtil);

#ifdef ZC_REPORT_TO_ARM
	osal_start_timerEx ( CardTransferApp_TaskID, CARDTRANSFERAPP_STATUS_REPORT_EVENT, 10000 ); //delay for a minute
#endif

	pREList = osal_mem_alloc(MAXNEIGHBOR*sizeof(RouteEntry_t));
	if(!pREList)
		SystemReset();
	osal_memset((void *)pREList, 0, MAXNEIGHBOR*sizeof(RouteEntry_t));

#ifdef DEBUG_ROUTE
	pREListCopy = osal_mem_alloc(MAXNEIGHBOR*sizeof(RouteEntry_t));
	if(!pREListCopy)
		SystemReset();
	osal_memset((void *)pREListCopy, 0, MAXNEIGHBOR*sizeof(RouteEntry_t));
#endif

	//CardTransferApp_DataTrans.databuf = osal_mem_alloc(CARDTRANSFERAPP_MAX_DATA_LEN);
	//if (!CardTransferApp_DataTrans.databuf)
	//	SystemReset();

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
UINT16 CardTransferApp_ProcessEvent( uint8 task_id, UINT16 events )
{
	afIncomingMSGPacket_t* MSGpkt;
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
		MSGpkt = ( afIncomingMSGPacket_t * ) osal_msg_receive ( CardTransferApp_TaskID );
		while ( MSGpkt )
		{
			switch ( MSGpkt->hdr.event )
			{
			case ZDO_STATE_CHANGE:
				{
					CardTransferApp_NwkState = (devStates_t)(MSGpkt->hdr.status);
					if (CardTransferApp_NwkState == DEV_ZB_COORD)
					{
						osal_start_timerEx (CardTransferApp_TaskID, CARDTRANSFERAPP_ROUTE_CLEAN_EVT, 10000/FREQUENCY_MULTIPLICATION);
					#ifdef DEBUG_ROUTE
						osal_start_timerEx(CardTransferApp_TaskID, CARDTRANSFERAPP_REPORT_ROUTE_EVENT, 5000);
					#endif
						osal_start_timerEx(CardTransferApp_TaskID, CARDTRANSFERAPP_ROUTE_EXPIRE_EVT, 10000/FREQUENCY_MULTIPLICATION);
						MyInfo.nodeID = zgConfigPANID;
						MyInfo.parentID = zgConfigPANID;
						MyInfo.nodeDepth = 0;
						MyInfo.sinkID = zgConfigPANID;						
					}
					else
					{
						storeParam_t param = *( storeParam_t* ) CARDTRANSFERAPP_PARAM_ADDR;
						param.ResetFlag = ZC_REPORT_STARTNWK_FAILED_RESTART;
						* ( storeParam_t* ) CARDTRANSFERAPP_PARAM_ADDR =  param;
						if (NO_TIMER_AVAIL == osal_start_timerEx ( CardTransferApp_TaskID, CARDTRANSFERAPP_WDG_RESTART_EVENT, 1000))
						{
							SystemReset();
						}
					}
					if ( NO_TIMER_AVAIL == osal_start_timerEx ( CardTransferApp_TaskID, CARDTRANSFERAPP_DETECT_SPI_EVENT, 65000 ) )
					{
						SystemReset();
					}
					break;
				}
			case AF_INCOMING_MSG_CMD:
				{
					CardTransferApp_ProcessMSGCB ( MSGpkt );
					break;
				}
#if (defined HAL_SPI) && (HAL_SPI == TRUE)
			case SPI_RX: //I'm coordinator, just dispatch the packet.
				{
					CardTransferApp_ProcessSPICB ( ( SPIMSGPacket_t * ) MSGpkt );
					break;
				}
#endif
			default:
				break;
			}
			osal_msg_deallocate ( ( uint8 * ) MSGpkt );
			MSGpkt = ( afIncomingMSGPacket_t * ) osal_msg_receive ( CardTransferApp_TaskID );
		}
		return ( events ^ SYS_EVENT_MSG );
	}

	if ( events & CARDTRANSFERAPP_DETECT_SPI_EVENT )
	{
		if ( NO_TIMER_AVAIL == osal_start_timerEx ( CardTransferApp_TaskID, CARDTRANSFERAPP_DETECT_SPI_EVENT, 40000 ) )
		{
			SystemReset();
		}
		app_SPIDetect_t spi_report;
		spi_report.msgtype = SPI_DETECT;

		for ( uint8 i =0; i< SPIDETECT_LEN; i++ )
		{
			spi_report.detectdata[i] = i;
		}
		pSpi->DataLength = sizeof ( app_SPIDetect_t );
		osal_memcpy ( ( void * ) ( pSpi+1 ), ( void * ) &spi_report, pSpi->DataLength );
		HalSPIWrite ( ( uint8 * ) pSpi, pSpi ->DataLength + SPIPKTHDR_LEN );
		CardTransferApp_StartProbeSpiEvt();
		return events ^ CARDTRANSFERAPP_DETECT_SPI_EVENT;
	}

	if ( events & CARDTRANSFERAPP_PROBE_SPI_EVENT )
	{
		SPIErrCnt++;
		if ( SPIErrCnt >= CARDTRANSFERAPP_SPIERR_TIMEOUT )
		{
			uint8 *p1, *p2;
			storeParam_t param = * ( storeParam_t* ) CARDTRANSFERAPP_PARAM_ADDR;
			p1 = osal_mem_alloc ( 8 ); //small size memory
			p2 = osal_mem_alloc ( 64 ); //big size memory
			if ( p1==NULL || p2==NULL )
			{
				param.ResetFlag = ZC_REPORT_MEMORY_ERR_RESTART;
			}
			else
			{
				param.ResetFlag = ZC_REPORT_SPI_RESTART;
			}
			* ( storeParam_t* ) CARDTRANSFERAPP_PARAM_ADDR =  param;
			osal_mem_free ( p1 );
			osal_mem_free ( p2 );

			if ( NO_TIMER_AVAIL == osal_start_timerEx ( CardTransferApp_TaskID, CARDTRANSFERAPP_WDG_RESTART_EVENT, 1000 ) )
			{
				SystemReset	();
			}
			SPIErrCnt = 0;
		}
		return ( events ^ CARDTRANSFERAPP_PROBE_SPI_EVENT );
	}

#ifdef ZC_REPORT_TO_ARM
	if ( events & CARDTRANSFERAPP_STATUS_REPORT_EVENT )
	{
#if (defined HAL_SPI) && (HAL_SPI == TRUE)
		app_ZC_Report_t zc_report;

		zc_report.msgtype = ZC_REPORT;
		zc_report.PanId = zgConfigPANID;

		/* get reset flag and send report */
		storeParam_t param = * ( storeParam_t* ) CARDTRANSFERAPP_PARAM_ADDR;
		switch ( GetResetFlag() )
		{
		case RESET_FLAG_WATCHDOG:
			{
				if ( param.ResetFlag == ZC_REPORT_WATCHDOG_RESTART
					||param.ResetFlag == ZC_REPORT_SPI_RESTART
					||param.ResetFlag == ZC_REPORT_STARTNWK_FAILED_RESTART
					||param.ResetFlag == ZC_REPORT_BLAST_ERR_RESTART
					||param.ResetFlag == ZC_REPORT_MEMORY_ERR_RESTART
					||param.ResetFlag == ZC_REPORT_PARTBLAST_ERR_RESTART)
				{
					zc_report.flag  = param.ResetFlag;
				}
				break;
			}
		case RESET_FLAG_EXTERNAL:
			{
				zc_report.flag  = ZC_REPORT_EXTERNAL_RESTART;
				break;
			}
		case RESET_FLAG_POWERON:
			{
				zc_report.flag  = ZC_REPORT_POWERON;
				break;
			}
		default:
			break;

		}
		{ // send out report
			pSpi->DataLength = sizeof ( app_ZC_Report_t );
			osal_memcpy ( ( void * ) ( pSpi+1 ), ( void * ) &zc_report, pSpi->DataLength );
			HalSPIWrite ( ( uint8 * ) pSpi, pSpi ->DataLength + SPIPKTHDR_LEN );
			osal_start_timerEx ( CardTransferApp_TaskID, CARDTRANSFERAPP_WDG_RESTART_EVENT, 20000 );
		}
		param.ResetFlag = ZC_REPORT_WATCHDOG_RESTART;  // clear restart  flag
		* ( storeParam_t* ) CARDTRANSFERAPP_PARAM_ADDR = param;
#endif
		return events ^ CARDTRANSFERAPP_STATUS_REPORT_EVENT;
	}
#endif

	if (events & CARDTRANSFERAPP_WDG_RESTART_EVENT)
	{
		SystemReset();
	}

	if (events & CARDTRANSFERAPP_ROUTE_REQ_EVENT)
	{
		Nwk_RouteReq_t rtReq;
		rtReq.msgtype = ROUTEREQ;
		rtReq.nodeID = MyInfo.nodeID;
		rtReq.srcAddress = MyInfo.nodeID;
		rtReq.dstAddress = 0xFFFF;
		rtReq.nodeDepth = 0;

		MacParam_t param;
		param.panID = 0xFFFF;
		param.cluster_id = CARDTRANSFERAPP_NWK_CLUSTERID;
		param.radius = 0x01;
		MAC_UTIL_BuildandSendDataPAN(&param, (uint8 *)&rtReq,
			sizeof(Nwk_RouteReq_t), MAC_UTIL_BROADCAST, 0, 0);

		rtBuildCnt++;
		//afAddrType_t dstAddr;
		//uint8 CardTransferApp_TransID = 0;
		//dstAddr.addrMode = afAddrBroadcast;
		//dstAddr.addr.shortAddr = NWK_BROADCAST_SHORTADDR_DEVALL;
		//dstAddr.endPoint = CARDTRANSFERAPP_ENDPOINT;
		//AF_DataRequest(&dstAddr, ( endPointDesc_t * ) &CardTransferApp_epDesc,
		//	            CARDTRANSFERAPP_NWK_CLUSTERID, sizeof(Nwk_RouteReq_t), (uint8 *)&rtReq,
		//	             &CardTransferApp_TransID, INIT_OPN, 1);
		if (rtBuildCnt <= CARDTRANSFER_ROUTE_BUILDCNT)
			osal_start_timerEx(CardTransferApp_TaskID, CARDTRANSFERAPP_ROUTE_REQ_EVENT, CARDTRANSFER_ROUTE_REBUILDINTERVAL/FREQUENCY_MULTIPLICATION);
		else
		{
			if (CARDTRANSFER_ROUTE_ALIVETIME)
				osal_start_timerEx(CardTransferApp_TaskID, CARDTRANSFERAPP_ROUTE_ALIVE_EVT, CARDTRANSFER_ROUTE_ALIVETIME/FREQUENCY_MULTIPLICATION);
		}
		return events ^ CARDTRANSFERAPP_ROUTE_REQ_EVENT;
	}
	
	if (events & CARDTRANSFERAPP_ROUTE_EXPIRE_EVT)
	{
		int16 i;
		for (i = 0; i < reListIndex; ++i)
		{
			if (pREList[i].expiryTime)
				pREList[i].expiryTime--;
		}
		if (CARDTRANSFER_ROUTE_EXPIRETIME)
			osal_start_timerEx(CardTransferApp_TaskID, CARDTRANSFERAPP_ROUTE_EXPIRE_EVT, CARDTRANSFER_ROUTE_EXPIRETIME/FREQUENCY_MULTIPLICATION);
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
		MAC_UTIL_BuildandSendDataPAN(&param, (uint8 *)&rtAlive,
			sizeof(Nwk_RouteAlive_t), MAC_UTIL_BROADCAST, 0, 0);	
		if (CARDTRANSFER_ROUTE_ALIVETIME)
			osal_start_timerEx(CardTransferApp_TaskID, CARDTRANSFERAPP_ROUTE_ALIVE_EVT, CARDTRANSFER_ROUTE_ALIVETIME/FREQUENCY_MULTIPLICATION);
		return events ^ CARDTRANSFERAPP_ROUTE_ALIVE_EVT;
	}

	if (events & CARDTRANSFERAPP_ROUTE_CLEAN_EVT)
	{
		Nwk_RouteClean_t rtClean;
		rtClean.msgtype = ROUTECLEAN;
		rtClean.nodeID = MyInfo.nodeID;
		rtClean.srcAddress = MyInfo.nodeID;
		rtClean.dstAddress = 0xFFFF;
		rtClean.nodeDepth = 0;

		MacParam_t param;
		param.panID = 0xFFFF;
		param.cluster_id = CARDTRANSFERAPP_NWK_CLUSTERID;
		param.radius = 0x01;
		MAC_UTIL_BuildandSendDataPAN(&param, (uint8 *)&rtClean,
			sizeof(Nwk_RouteClean_t), MAC_UTIL_BROADCAST, 0, 0);

		rtCleanCnt++;
		if (rtCleanCnt <= CARDTRANSFER_ROUTE_BUILDCNT)
			osal_start_timerEx(CardTransferApp_TaskID, CARDTRANSFERAPP_ROUTE_CLEAN_EVT, CARDTRANSFER_ROUTE_REBUILDINTERVAL/3/FREQUENCY_MULTIPLICATION);
		else
		{
			if (CARDTRANSFER_ROUTE_REBUILDINTERVAL)
				osal_start_timerEx(CardTransferApp_TaskID, CARDTRANSFERAPP_ROUTE_REQ_EVENT, CARDTRANSFER_ROUTE_REBUILDINTERVAL/FREQUENCY_MULTIPLICATION);
		}		
		return events ^ CARDTRANSFERAPP_ROUTE_CLEAN_EVT;
	}

#ifdef DEBUG_ROUTE
	if(events & CARDTRANSFERAPP_REPORT_ROUTE_EVENT)
	{	
		if(printIdx == MAXNEIGHBOR)
		{
			app_am_routerpt_t routerpt;
			routerpt.msgtype = AMMETER_ROUTE_REPORT;
			routerpt.reporttype = AMMETER_ROUTERPT_TYPE_BEGIN;
			
			pSpi->DataLength = sizeof(app_am_routerpt_t);
			osal_memcpy((uint8 *)(pSpi + 1), &routerpt, pSpi->DataLength);
			HalSPIWrite(( uint8 *) pSpi, pSpi->DataLength + SPIPKTHDR_LEN);

			osal_memcpy(pREListCopy, pREList, MAXNEIGHBOR*sizeof(RouteEntry_t));
			printIdx = reListIndex-1;
			aliveRoute = 0;

			if(ZSuccess != osal_start_timerEx(CardTransferApp_TaskID, CARDTRANSFERAPP_REPORT_ROUTE_EVENT, 300))
			{
				SystemReset();
			}
		}
		else if(printIdx < 0)
		{
			app_am_routerpt_t routerpt;
			routerpt.msgtype = AMMETER_ROUTE_REPORT;
			routerpt.reporttype = AMMETER_ROUTERPT_TYPE_END;
			
			pSpi->DataLength = sizeof(app_am_routerpt_t);
			osal_memcpy((uint8 *)(pSpi + 1), &routerpt, pSpi->DataLength);
			HalSPIWrite(( uint8 *) pSpi, pSpi->DataLength + SPIPKTHDR_LEN);

			printIdx = MAXNEIGHBOR;
			if(ZSuccess != osal_start_timerEx(CardTransferApp_TaskID, CARDTRANSFERAPP_REPORT_ROUTE_EVENT, 5000))
			{
				SystemReset();
			}
		}
		else
		{
			if ( pREListCopy[printIdx].expiryTime)
			{
				aliveRoute++;

				app_am_routerpt_t routerpt;
				routerpt.msgtype = AMMETER_ROUTE_REPORT;
				routerpt.reporttype = AMMETER_ROUTERPT_TYPE_MSG;
				routerpt.nextHopAddress = pREListCopy[printIdx].nextHopAddress;
				routerpt.dstAddress = pREListCopy[printIdx].dstAddress;
				routerpt.expiryTime = pREListCopy[printIdx].expiryTime;
				
				pSpi->DataLength = sizeof(app_am_routerpt_t);
				osal_memcpy((uint8 *)(pSpi + 1), &routerpt, pSpi->DataLength);
				HalSPIWrite(( uint8 *) pSpi, pSpi->DataLength + SPIPKTHDR_LEN);
			}
			printIdx--;
			if(ZSuccess != osal_start_timerEx(CardTransferApp_TaskID, CARDTRANSFERAPP_REPORT_ROUTE_EVENT, 300))
			{
				SystemReset();
			}
		}

		return events ^ CARDTRANSFERAPP_REPORT_ROUTE_EVENT;
	}
#endif

	return 0;
}

#if (defined HAL_SPI) && (HAL_SPI == TRUE)
/*********************************************************************
* @fn      CardTransferApp_MessageSPICB
*
* @brief   SPI message processor callback.  This function processes
*          any incoming data from ARM. Actually, It's based on Zigbee
*	      Coordinator.
*
* @param   MSGpkt - The incoming msg.
*
* @return  none
*/
void CardTransferApp_ProcessSPICB ( const SPIMSGPacket_t *MSGpkt )
{
	APPWrapper_t* appwrapper = ( APPWrapper_t* ) ( MSGpkt+1 );
	if ( appwrapper->app_flag == SPI_DETECT )
	{
		bool checkflag;
		checkflag = true;

		for ( uint8 i=0; i<SPIDETECT_LEN; i++ )
		{
			if ( appwrapper->app_SPIDetect.detectdata[i] != i )
			{
				checkflag = false;
			}
		}
		if ( checkflag )
		{
			CardTransferApp_StopProbeSpiEvt();
		}
	}
	else if ( appwrapper->app_flag == TRY_CONNECTION )
	{
		//Notify ARM the PANID as its id.
		if ( CardTransferApp_NwkState == DEV_ZB_COORD )
		{
			pSpi->spihdr.srcAddr.addr.shortAddr = 0;
			pSpi->DataLength = sizeof ( app_startNwk_t );
			APPWrapper_t* pTryconRSP = ( APPWrapper_t* ) ( pSpi + 1 );
			pTryconRSP->app_StartNwk.msgtype = ZB_START_NETWORK;
			pTryconRSP->app_StartNwk.PANID = zgConfigPANID;
			osal_cpyExtAddr ( pTryconRSP->app_StartNwk.macAddr, ( void * ) aExtendedAddress );
			HalSPIWrite ( ( uint8* ) pSpi, pSpi->DataLength + SPIPKTHDR_LEN );
		}
		else
			ZDApp_StartUpFromApp ( APP_STARTUP_COORD );
	}
}
#endif

void CardTransferApp_ProcessMSGCB ( const afIncomingMSGPacket_t *pkt )
{
	switch ( pkt->clusterId )
	{
	case CARDTRANSFERAPP_CLUSTERID:
		{
			APPWrapper_t* appwrapper = ( APPWrapper_t* ) (pkt->cmd.Data);
			app_ssInd_t* pssInd = (app_ssInd_t *)(pkt->cmd.Data+sizeof(app_dataCrossPan_t));
			if (appwrapper->app_flag == DATA_CROSS_PAN)
			{
				CardTransferApp_ProcessDataLED();
				pSpi->spihdr.srcAddr.addr.shortAddr = pssInd->rssipkt.NODE_ADDR;
				pSpi->DataLength = appwrapper->app_dataCrossPan.len;
				osal_memcpy((uint8 *)(pSpi + 1), pkt->cmd.Data+sizeof(app_dataCrossPan_t), pSpi->DataLength);
				HalSPIWrite(( uint8 *) pSpi, pSpi->DataLength + SPIPKTHDR_LEN);			
			}
			else if (appwrapper->app_flag == SSREQ)
			{
				app_ssInd_t ssInd;
				ssInd.msgtype  = SSIND;
				CONV_LQI_TO_RSSI( CardTransferApp_RSSI, pkt->LinkQuality);
				ssInd.rssipkt.RSSI = CardTransferApp_RSSI;
				ssInd.rssipkt.NWK_ADDR = zgConfigPANID;//appdata->app_ssReq.NWK_ADDR;
				ssInd.rssipkt.NODE_ADDR = pkt->srcAddr.addr.shortAddr;
				ssInd.rssipkt.seqnum = appwrapper->app_ssReq.seqnum;
				
				pSpi->spihdr.srcAddr.addr.shortAddr = pkt->srcAddr.addr.shortAddr;
				pSpi->DataLength = sizeof ( app_ssInd_t );
				osal_memcpy ( ( void * ) ( pSpi+1 ), ( void * ) &ssInd, pSpi->DataLength);
				HalSPIWrite ( ( uint8 * ) pSpi, pSpi ->DataLength + SPIPKTHDR_LEN);		
			}
			break;
		}
	case CARDTRANSFERAPP_NWK_CLUSTERID:
		{
			if (pkt->cmd.Data[0] == ROUTERSP)
			{
				Nwk_RouteRsp_t* pRRsp = (Nwk_RouteRsp_t *)pkt->cmd.Data;
				int16 i;
				bool IsRouteExist = false;
				if (pRRsp->dstAddress != MyInfo.nodeID)
					return;
				if (reListIndex < MAXNEIGHBOR)
				{
					for (i = 0; i < reListIndex; ++i)
					{
						if (pREList[i].dstAddress == pRRsp->srcAddress)
						{
							pREList[i].nextHopAddress = pRRsp->nodeID;
							pREList[i].expiryTime = CARDTRANSFER_ROUTE_EXPIRECNT;
							IsRouteExist = true;
							break;
						}
					}
					if (!IsRouteExist)
					{
						pREList[reListIndex].dstAddress = pRRsp->srcAddress;
						pREList[reListIndex].nextHopAddress = pRRsp->nodeID;
						pREList[reListIndex].expiryTime = CARDTRANSFER_ROUTE_EXPIRECNT;
						reListIndex++;
					}
				}
				else
				{
					//Compact route list.
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
		#ifdef DEBUG_REPORT_NW
			else if(pkt->cmd.Data[0] == ROUTEREPORT)
			{
				PrintRoute(&(pkt->cmd.Data[1]), pkt->cmd.DataLength-1);
			}	
		#endif
			break;
		}
	}
}

void CardTransferApp_ProcessDataLED(void )
{
	HalLedSet(HAL_LED_DATA,HAL_LED_MODE_OFF);
	HalLedBlink(HAL_LED_DATA, 1, 50, 300);
}

#ifdef DEBUG_REPORT_NW
static void PrintRoute(uint8* data, uint8 len)
{
#if 1
	app_RouteInfo_t RouteInfo;
	RouteInfo.msgtype = ROUTE_INFO;
	RouteInfo.len = len;
	pSpi->DataLength = sizeof(app_RouteInfo_t)+len;
	osal_memcpy((uint8 *)(pSpi + 1), &RouteInfo, sizeof(app_RouteInfo_t));
	osal_memcpy((uint8 *)(pSpi + 1)+sizeof(app_RouteInfo_t), data, len);
	HalSPIWrite(( uint8 *) pSpi, pSpi->DataLength + SPIPKTHDR_LEN);
#else
	uint8 count = 0;
	char AscString[255];
	char* prefix = "Center -> ";
	uint8 plen = osal_strlen(prefix);

	osal_memcpy(AscString, prefix, plen);

	/*short address is 16 bits, so elen is 2*/
	count = u16DataToHexStr(data, len, 2, AscString+plen, 255-plen, " -> ", 4);
	count += plen;

	AscString[count++] = '\n';
	AscString[count++] = '\r';	

	HalUARTWrite(AMMETER_APP_PORT, (uint8*)AscString, count);
#endif
}
#endif

void CardTransferApp_StartProbeSpiEvt ( void )
{
	osal_start_timerEx ( CardTransferApp_TaskID, CARDTRANSFERAPP_PROBE_SPI_EVENT, 20000 );
}

void CardTransferApp_StopProbeSpiEvt ( void )
{
	SPIErrCnt = 0;
	osal_stop_timerEx ( CardTransferApp_TaskID, CARDTRANSFERAPP_PROBE_SPI_EVENT );
}

#if 0
void CardTransferApp_SendData(app_amdata_t *pHead,  uint8* pData, uint16 len)
{
	uint8 sendbuf[MAX_DATA_SIZE];
	uint8 sendlen;
	
	CardTransferApp_ProcessDataLED();		
	pHead->srcPan = zgConfigPANID;
	if (pHead->dstPan == 0xFFFF)
	{
		osal_memcpy(CardTransferApp_DataTrans.databuf, (uint8 *)pHead, sizeof(app_amdata_t));
		osal_memcpy(CardTransferApp_DataTrans.databuf + sizeof(app_amdata_t), pData,len);			
		CardTransferApp_DataTrans.datasize = sizeof(app_amdata_t) + len;
		CardTransferApp_DataTrans.reIndex = reListIndex;
		osal_start_timerEx(CardTransferApp_TaskID, CARDTRANSFERAPP_MSG_SEND_EVT, 20);
	}
	else
	{
		MacParam_t param;
		param.cluster_id = CARDTRANSFERAPP_CLUSTERID;
		param.radius = 0x01;
		int16 i = 0;
		for (; i < reListIndex; ++i)
		{
			if (pREList[i].dstAddress == pHead->dstPan
				/*&& pREList[i].expiryTime*/)
			{
				osal_memcpy(sendbuf, (uint8 *)pHead, sizeof(app_amdata_t));
				osal_memcpy(sendbuf + sizeof(app_amdata_t), pData,len);	
				sendlen = sizeof(app_amdata_t) + len;
				param.panID = pREList[i].nextHopAddress;
				MAC_UTIL_BuildandSendDataPAN(&param, (uint8 *)sendbuf,
					sendlen, MAC_UTIL_UNICAST, 0, MAC_TXOPTION_ACK);	
				break;
			}
		}		
	}

}
#endif
