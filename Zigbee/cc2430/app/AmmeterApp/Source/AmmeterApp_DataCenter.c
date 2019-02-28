/**************************************************************************************************
Filename:       AmmeterApp_Datacenter.c
Revised:        $Date: 2010/08/11 02:02:04 $
Revision:       $Revision: 1.27 $

Description -  Ammeter Datacenter Application:Ammeter data center.
**************************************************************************************************/
/*********************************************************************
* INCLUDES
*/
#include "ZComDef.h"
#include "AmmeterApp_DataCenter.h"
#include "AmmeterApp_cfg.h"
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

#ifdef CFG_AMMETER_IEC
#include "Ammeter_IEC.h"
#endif

/*********************************************************************
* MACROS
*/
#define AMMETERAPP_PARAM_ADDR 			0xFEF0

/*********************************************************************
* CONSTANTS
*/

// This list should be filled with Application specific Cluster IDs.
const cId_t AmmeterApp_ClusterList[] =
{
	AMMETERAPP_CLUSTERID,
	AMMETERAPP_NWK_CLUSTERID,
};

const SimpleDescriptionFormat_t AmmeterApp_SimpleDesc =
{
	AMMETERAPP_ENDPOINT,              //  int   Endpoint;
	AMMETERAPP_PROFID,                //  uint16 AppProfId[2];
	AMMETERAPP_DEVICEID,              //  uint16 AppDeviceId[2];
	AMMETERAPP_DEVICE_VERSION,        //  int   AppDevVer:4;
	AMMETERAPP_FLAGS,                 //  int   AppFlags:4;
	sizeof(AmmeterApp_ClusterList),          //  byte  AppNumInClusters;
	(cId_t *)AmmeterApp_ClusterList,  //  byte *pAppInClusterList;
	sizeof(AmmeterApp_ClusterList),          //  byte  AppNumOutClusters;
	(cId_t *)AmmeterApp_ClusterList   //  byte *pAppOutClusterList;
};

const endPointDesc_t AmmeterApp_epDesc =
{
	AMMETERAPP_ENDPOINT,
	&AmmeterApp_TaskID,
	(SimpleDescriptionFormat_t *)&AmmeterApp_SimpleDesc,
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

typedef struct
{
	uint8* p;
	uint8 idx;
}buf_t;
/*********************************************************************
* TYPEDEFS
*/

/*********************************************************************
* GLOBAL VARIABLES
*/
uint8 AmmeterApp_TaskID;    // Task ID for internal task/event processing.

/*********************************************************************
* LOCAL VARIABLES
*/
static devStates_t AmmeterApp_NwkState;
static uint8 AmmeterApp_TransID;
static byte AmmeterApp_Msg[AMMETERAPP_MAX_DATA_LEN];
static SPIMSGPacket_t* pSpi = ( SPIMSGPacket_t* ) AmmeterApp_Msg;
static AmmeterDataTrans_t AmmeterApp_DataTrans = {0, 0, 0};
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

#if (defined CFG_AMMETER_IEC) || (defined CFG_AMMETER_TRANSPARENT)
/*reuse the buffer*/
static uint8 *AmmeterApp_UARTbuf = AmmeterApp_Msg + sizeof (app_amdata_t); 
static uint8 AmmeterApp_UARTbufLen = AMMETERAPP_MAX_DATA_LEN - sizeof (app_amdata_t);
static uint8 UARTbufTailIndex = 0;
static uint8 UARTbufReadIndex = 0;
static uint16 timeSinceLastSending = 0;
static uint8 UARTreadInterval = 50;
static uint16 SendingInterval = 150;
static halUARTIoctl_t UARTconfig;

static uint8 *txBuf;
static uint16 txLen;
static uint16 ackAddr;

#if (defined CFG_AMMETER_IEC)
static tsIEC_info IEC_info;
#endif
static uint16 GetBitTransmitInterval(uint8 bautRate);

#endif

#ifdef CFG_AMMETER_CMDLINE
static buf_t dataBuf = {NULL,0};
#endif
/*********************************************************************
* LOCAL FUNCTIONS
*/
static void AmmeterApp_ProcessSPICB ( const SPIMSGPacket_t *MSGpkt );
static void AmmeterApp_ProcessMSGCB ( const afIncomingMSGPacket_t *MSGpkt );

#if (defined CFG_AMMETER_IEC) || (defined CFG_AMMETER_TRANSPARENT)
static void AmmeterApp_BuildData(uint8 *buf, uint8 len);
static void AmmeterApp_ProcessUARTCB(uint8 *data, uint8 len);
static void AmmeterApp_SendAmmeterAck( uint16 seqnum, uint8 packnum,uint8 packseq, uint16 dstAddr);
#endif

#ifdef DEBUG_REPORT_NW
static void PrintRoute(uint8* data, uint8 len);
#ifdef CFG_AMMETER_CMDLINE
static void Ammeter_UartCBCmdLine (uint8 port, uint8 event);
static void Ammeter_ParseUartCmd( char* cmd);
#endif
#endif


static  void AmmeterApp_ProcessDataLED(void );
static void  AmmeterApp_SendData(app_amdata_t *pHead,  uint8*pData, uint16 len);
#ifdef DEBUG_AMMETER
static uint16 GetCheckCode(const uint8 * pSendBuf, int8 nEnd);
#endif


#if (defined DEBUG_REPORT_NW)  || (defined CFG_AMMETER_IEC)
	#undef HAL_SPI
	#undef DEBUG_ROUTE
#endif




#if !defined(AMMETER_APP_PORT)
#define AMMETER_APP_PORT  0
#endif

#if !defined( AMMETER_APP_BAUD )
// CC2430 only allows 38.4k or 115.2k.
#define AMMETER_APP_BAUD  HAL_UART_BR_115200
#endif

// When the Rx buf space is less than this threshold, invoke the Rx callback.
#if !defined( AMMETER_APP_THRESH )
#define AMMETER_APP_THRESH  48
#endif

#if !defined( AMMETER_APP_RX_MAX )
/* The generic safe Rx minimum is 48, but if you know your PC App will not
* continue to send more than a byte after receiving the ~CTS, lower max
* here and safe min in _hal_uart.c to just 8.
*/
#define AMMETER_APP_RX_MAX  249
#endif

#if !defined( AMMETER_APP_TX_MAX )
#define AMMETER_APP_TX_MAX  AMMETER_MAX_PKT_LEN
#endif

// Millisecs of idle time after a byte is received before invoking Rx callback.
#if !defined( AMMETER_APP_IDLE )
#define AMMETER_APP_IDLE  6
#endif


/*********************************************************************
* FUNCTIONS
*********************************************************************/
/*********************************************************************
* @fn      AmmeterApp_Init
*
* @brief   This is called during OSAL tasks' initialization.
*
* @param   task_id - the Task ID assigned by OSAL.
*
* @return  none
*/
void AmmeterApp_Init( uint8 task_id )
{
	AmmeterApp_TaskID = task_id;
#if(defined WATCHDOG) && (WATCHDOG==TRUE)
	StartWatchDog ( DOGTIMER_INTERVAL_1S );
	osal_set_event ( AmmeterApp_TaskID, AMMETERAPP_FEEDWATCHDOG_EVENT);
#endif

#if (defined HAL_SPI) && (HAL_SPI==TRUE)
	halSPICfg_t spiConfig;
	spiConfig.maxBufSize = DMA_MAX;
	spiConfig.intEnable = false;
	HalSpiStart ( &spiConfig );
	RegisterForSpi ( AmmeterApp_TaskID );
#endif

#ifdef DEBUG_REPORT_NW
	halUARTCfg_t uartConfig;

	uartConfig.configured           = TRUE;              // 2430 don't care.
	uartConfig.baudRate             = HAL_UART_BR_115200;
	uartConfig.flowControl          = FALSE;
	uartConfig.flowControlThreshold = AMMETER_APP_THRESH;
	uartConfig.rx.maxBufSize        = AMMETER_APP_RX_MAX;
	uartConfig.tx.maxBufSize        = AMMETER_APP_TX_MAX;
	uartConfig.idleTimeout          = AMMETER_APP_IDLE;   // 2430 don't care.
	uartConfig.intEnable            = TRUE;              // 2430 don't care.
	uartConfig.callBackFunc         = NULL;
#ifdef CFG_AMMETER_CMDLINE
	uartConfig.workMode = HAL_UART_WORKMODE_REPORT;
	uartConfig.CallBackFuncCmdLine =  Ammeter_UartCBCmdLine;
#endif

	HalUARTOpen (AMMETER_APP_PORT, &uartConfig);
#endif // DEBUG_REPORT_NW

#if (defined CFG_AMMETER_IEC)
	halUARTCfg_t uartConfig;

	uartConfig.configured           = TRUE;              // 2430 don't care.
	uartConfig.baudRate             = AMMETER_APP_BAUD;
	uartConfig.flowControl          = FALSE;
	uartConfig.flowControlThreshold = AMMETER_APP_THRESH;
	uartConfig.rx.maxBufSize        = AMMETER_APP_RX_MAX;
	uartConfig.tx.maxBufSize        = AMMETER_APP_TX_MAX;
	uartConfig.idleTimeout          = AMMETER_APP_IDLE;   // 2430 don't care.
	uartConfig.intEnable            = TRUE;              // 2430 don't care.
	uartConfig.callBackFunc         = NULL;

	HalUARTOpen (AMMETER_APP_PORT, &uartConfig);
	
	HalUARTIoctl (AMMETER_APP_PORT, HAL_UART_IOCTL_GET, &UARTconfig);
	UARTconfig.linkFormat = HAL_UART_LINKFORMAT_1;
	HalUARTIoctl (AMMETER_APP_PORT, HAL_UART_IOCTL_SET, &UARTconfig);
#endif

	afRegister((endPointDesc_t *)&AmmeterApp_epDesc);

	pSpi= ( SPIMSGPacket_t * ) AmmeterApp_Msg;
	pSpi->spihdr.hdr.event = SPI_RX;
	pSpi->spihdr.srcAddr.addrMode = ( AddrMode_t ) Addr16Bit;
	pSpi->spihdr.srcAddr.endPoint = AMMETERAPP_ENDPOINT;
	pSpi->spihdr.dstAddr.addrMode = ( AddrMode_t ) Addr16Bit;
	pSpi->spihdr.dstAddr.endPoint = AMMETERAPP_ENDPOINT;
	pSpi->spihdr.transID = INIT_TRANID;
	pSpi->spihdr.options = INIT_OPN;
	pSpi->spihdr.radius = MAX_DEPTH;

	/* EP and  Profile Id are not used,Card will drop them directly */
	MacUtil_t MacUtil;
	MacUtil.panID = 0xFFFF;  /* Communication with Card */
	MacUtil.dst_endpoint = AMMETERAPP_ENDPOINT;
	MacUtil.src_endpoint = 0x0F;//From sniffer.
	MacUtil.profile_id = 0x0500;//From sniffer.
	MacUtil.cluster_id = AMMETERAPP_CLUSTERID;
	MacUtil.NodeType = ZDO_Config_Node_Descriptor.LogicalType;
	MAC_UTIL_INIT (&MacUtil);

#ifdef ZC_REPORT_TO_ARM
	osal_start_timerEx ( AmmeterApp_TaskID, AMMETERAPP_STATUS_REPORT_EVENT, 10000 ); //delay for a minute
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

	AmmeterApp_DataTrans.databuf = osal_mem_alloc(AMMETERAPP_MAX_DATA_LEN);
	if (!AmmeterApp_DataTrans.databuf)
		SystemReset();

#if (defined CFG_AMMETER_IEC) || (defined CFG_AMMETER_TRANSPARENT)
	txBuf = AmmeterApp_DataTrans.databuf;
	txLen = 0;
#endif

#ifdef CFG_AMMETER_IEC
	IEC_info.state = E_IEC_STATE_UART_IDLE;
	IEC_info.protocol = D_IEC_PROTOCOL_C;
	IEC_info.com_baud_rate = AMMETER_APP_BAUD;
#endif

#ifdef CFG_AMMETER_CMDLINE
	dataBuf.p = osal_mem_alloc(AMMETERAPP_MAX_DATA_LEN);
	if (!dataBuf.p)
		SystemReset();
#endif
}

/*********************************************************************
* @fn      AmmeterApp_ProcessEvent
*
* @brief   Generic Application Task event processor.
*
* @param   task_id  - The OSAL assigned task ID.
* @param   events   - Bit map of events to process.
*
* @return  none
*/
UINT16 AmmeterApp_ProcessEvent( uint8 task_id, UINT16 events )
{
	afIncomingMSGPacket_t* MSGpkt;
#if(defined WATCHDOG) && (WATCHDOG==TRUE)
	if ( events & AMMETERAPP_FEEDWATCHDOG_EVENT )
	{
		osal_start_timerEx ( AmmeterApp_TaskID, AMMETERAPP_FEEDWATCHDOG_EVENT, 300 );
		FEEDWATCHDOG();
		return events ^ AMMETERAPP_FEEDWATCHDOG_EVENT;
	}
#endif
	if ( events & SYS_EVENT_MSG )
	{
		MSGpkt = ( afIncomingMSGPacket_t * ) osal_msg_receive ( AmmeterApp_TaskID );
		while ( MSGpkt )
		{
			switch ( MSGpkt->hdr.event )
			{
			case ZDO_STATE_CHANGE:
				{
					AmmeterApp_NwkState = (devStates_t)(MSGpkt->hdr.status);
					if (AmmeterApp_NwkState == DEV_ZB_COORD)
					{
						osal_start_timerEx (AmmeterApp_TaskID, AMMETERAPP_ROUTE_CLEAN_EVT, 10000/FREQUENCY_MULTIPLICATION);
#ifdef DEBUG_AMMETER
						osal_start_timerEx(AmmeterApp_TaskID, AMMETERAPP_TX_TEST_EVT, 200);
#endif
#ifdef DEBUG_ROUTE
						osal_start_timerEx(AmmeterApp_TaskID, AMMETERAPP_REPORT_ROUTE_EVENT, 5000);
#endif
#if (defined CFG_AMMETER_IEC || defined CFG_AMMETER_TRANSPARENT)
						osal_start_timerEx(AmmeterApp_TaskID, AMMETERAPP_UART_READ_EVT, 15000/FREQUENCY_MULTIPLICATION);
#endif

						osal_start_timerEx(AmmeterApp_TaskID, AMMETERAPP_ROUTE_EXPIRE_EVT, 10000/FREQUENCY_MULTIPLICATION);
						MyInfo.nodeID = zgConfigPANID;
						MyInfo.parentID = zgConfigPANID;
						MyInfo.nodeDepth = 0;
						MyInfo.sinkID = zgConfigPANID;
						/*
						dstAddr.endPoint = AMMETERAPP_ENDPOINT;
						dstAddr.addrMode = Addr16Bit;
						dstAddr.addr.shortAddr = RTG_AllocNewAddress ( 0 );
						uint8 macAddr[8];
						macAddr[0] = 0xB2;
						macAddr[1] = 0x01;
						macAddr[2] = 0x22;
						macAddr[3] = 0x23;
						macAddr[4] = 0x24;
						macAddr[5] = 0x25;
						macAddr[6] = 0x26;
						macAddr[7] = 0x27;
						associated_devices_t* assocaddr = AssocAddNew ( dstAddr.addr.shortAddr, macAddr, CHILD_RFD_RX_IDLE);
						*/						
					}
					else
					{
						storeParam_t param = *( storeParam_t* ) AMMETERAPP_PARAM_ADDR;
						param.ResetFlag = ZC_REPORT_STARTNWK_FAILED_RESTART;
						* ( storeParam_t* ) AMMETERAPP_PARAM_ADDR =  param;
						if (NO_TIMER_AVAIL == osal_start_timerEx ( AmmeterApp_TaskID, AMMETERAPP_WDG_RESTART_EVENT, 1000))
						{
							SystemReset();
						}
					}
					break;
				}
			case AF_INCOMING_MSG_CMD:
				{
					AmmeterApp_ProcessMSGCB ( MSGpkt );
					break;
				}
#if (defined HAL_SPI) && (HAL_SPI == TRUE)
			case SPI_RX: //I'm coordinator, just dispatch the packet.
				{
					AmmeterApp_ProcessSPICB ( ( SPIMSGPacket_t * ) MSGpkt );
					break;
				}
#endif
			default:
				break;
			}
			osal_msg_deallocate ( ( uint8 * ) MSGpkt );
			MSGpkt = ( afIncomingMSGPacket_t * ) osal_msg_receive ( AmmeterApp_TaskID );
		}
		return ( events ^ SYS_EVENT_MSG );
	}

#ifdef ZC_REPORT_TO_ARM
	if ( events & AMMETERAPP_STATUS_REPORT_EVENT )
	{
#if (defined HAL_SPI) && (HAL_SPI == TRUE)
		app_ZC_Report_t zc_report;

		zc_report.msgtype = ZC_REPORT;
		zc_report.PanId = zgConfigPANID;

		/* get reset flag and send report */
		storeParam_t param = * ( storeParam_t* ) AMMETERAPP_PARAM_ADDR;
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
			osal_start_timerEx ( AmmeterApp_TaskID, AMMETERAPP_WDG_RESTART_EVENT, 20000 );
		}
		param.ResetFlag = ZC_REPORT_WATCHDOG_RESTART;  // clear restart  flag
		* ( storeParam_t* ) AMMETERAPP_PARAM_ADDR = param;
#endif
		return events ^ AMMETERAPP_STATUS_REPORT_EVENT;
	}
#endif

	if (events & AMMETERAPP_WDG_RESTART_EVENT)
	{
		SystemReset();
	}

	if (events & AMMETERAPP_MSG_SEND_EVT)
	{
		if (AmmeterApp_DataTrans.reIndex > 0)
		{
			MacParam_t param;
			param.cluster_id = AMMETERAPP_CLUSTERID;
			param.radius = 0x01;
			AmmeterApp_DataTrans.reIndex--;
			if (*AmmeterApp_DataTrans.databuf == AMMETER_DATA)
			{
				APPWrapper_t* appwrapper = (APPWrapper_t* ) (AmmeterApp_DataTrans.databuf);
				if (pREList[AmmeterApp_DataTrans.reIndex].dstAddress != 0xffff
					&& pREList[AmmeterApp_DataTrans.reIndex].expiryTime)
				{
					param.panID = pREList[AmmeterApp_DataTrans.reIndex].nextHopAddress;


#if (defined CFG_AMMETER_TRANSPARENT)
					appwrapper->app_amdata.dstPan= 0xFFFF;
					MAC_UTIL_BuildandSendDataPAN(&param, AmmeterApp_DataTrans.databuf,
						AmmeterApp_DataTrans.datasize, MAC_UTIL_BROADCAST, 0, MAC_TXOPTION_ACK); // no ACK needed

#else					
					appwrapper->app_amdata.dstPan = pREList[AmmeterApp_DataTrans.reIndex].dstAddress;
					MAC_UTIL_BuildandSendDataPAN(&param, AmmeterApp_DataTrans.databuf,
						AmmeterApp_DataTrans.datasize, MAC_UTIL_BROADCAST, 0, MAC_TXOPTION_ACK);
#endif					
				}
			}

			osal_start_timerEx(AmmeterApp_TaskID, AMMETERAPP_MSG_SEND_EVT, 50);
		}
		else
		{
			osal_memset(AmmeterApp_DataTrans.databuf, 0, AmmeterApp_DataTrans.datasize);
			AmmeterApp_DataTrans.datasize = 0;
			AmmeterApp_DataTrans.reIndex = 0;			
		}
		return events ^ AMMETERAPP_MSG_SEND_EVT;
	}

	if (events & AMMETERAPP_ROUTE_REQ_EVENT)
	{
		Nwk_RouteReq_t rtReq;
		rtReq.msgtype = ROUTEREQ;
		rtReq.nodeID = MyInfo.nodeID;
		rtReq.srcAddress = MyInfo.nodeID;
		rtReq.dstAddress = 0xFFFF;
		rtReq.nodeDepth = 0;

		MacParam_t param;
		param.panID = 0xFFFF;
		param.cluster_id = AMMETERAPP_NWK_CLUSTERID;
		param.radius = 0x01;
		MAC_UTIL_BuildandSendDataPAN(&param, (uint8 *)&rtReq,
			sizeof(Nwk_RouteReq_t), MAC_UTIL_BROADCAST, 0, 0);

		rtBuildCnt++;
		//AF_DataRequest(&dstAddr, ( endPointDesc_t * ) &AmmeterApp_epDesc,
		//	            AMMETERAPP_NWK_CLUSTERID, sizeof(Nwk_RouteReq_t), (uint8 *)&rtReq,
		//	             &MineApp_TransID, 0, 1);
		if (rtBuildCnt <= AMMETER_ROUTE_BUILDCNT)
			osal_start_timerEx(AmmeterApp_TaskID, AMMETERAPP_ROUTE_REQ_EVENT, AMMETER_ROUTE_REBUILDINTERVAL/FREQUENCY_MULTIPLICATION);
		else
		{
			if (AMMETER_ROUTE_ALIVETIME)
				osal_start_timerEx(AmmeterApp_TaskID, AMMETERAPP_ROUTE_ALIVE_EVT, AMMETER_ROUTE_ALIVETIME/FREQUENCY_MULTIPLICATION);
		}
		return events ^ AMMETERAPP_ROUTE_REQ_EVENT;
	}
	if (events & AMMETERAPP_ROUTE_EXPIRE_EVT)
	{
		int16 i;
		for (i = 0; i < reListIndex; ++i)
		{
			if (pREList[i].expiryTime)
				pREList[i].expiryTime--;
		}
		if (AMMETER_ROUTE_EXPIRETIME)
			osal_start_timerEx(AmmeterApp_TaskID, AMMETERAPP_ROUTE_EXPIRE_EVT, AMMETER_ROUTE_EXPIRETIME/FREQUENCY_MULTIPLICATION);
		return events ^ AMMETERAPP_ROUTE_EXPIRE_EVT;
	}

#ifdef DEBUG_AMMETER
	if (events & AMMETERAPP_TX_TEST_EVT)
	{
		uint8 WriteBuf[64];
		osal_memset(WriteBuf, 0, 64);
		app_amdata_t* p = (app_amdata_t*)WriteBuf;
		p->msgtype = AMMETER_DATA;
		p->srcPan = 0x271B;
		p->dstPan = 0x271C;
		p->seqnum = 1;
		p->packnum = 1;
		p->packseq = 0;
		p->len = 8;

		int8 i = 0;
		i += sizeof(app_amdata_t);
		WriteBuf[i++] = 0x11;
		WriteBuf[i++] = 0x03;
		WriteBuf[i++] = 0x03;
		WriteBuf[i++] = 0xEB;
		WriteBuf[i++] = 0x00;
		WriteBuf[i++] = 0x01;
		uint16 CRC = GetCheckCode(WriteBuf+sizeof(app_amdata_t), 6);
		WriteBuf[i++] = LO_UINT16(CRC);
		WriteBuf[i] = HI_UINT16(CRC);

		osal_memcpy(AmmeterApp_DataTrans.databuf, WriteBuf, sizeof(app_amdata_t) + 8);
		AmmeterApp_DataTrans.datasize = sizeof(app_amdata_t) + 8;
		AmmeterApp_DataTrans.reIndex = 3;
		//osal_start_timerEx(AmmeterApp_TaskID, AMMETERAPP_MSG_SEND_EVT, 20);

		MacParam_t param;
		param.cluster_id = AMMETERAPP_CLUSTERID;
		param.radius = 0x01;
		param.panID = 0xFFFF;
		MAC_UTIL_BuildandSendDataPAN(&param, AmmeterApp_DataTrans.databuf,
			AmmeterApp_DataTrans.datasize, MAC_UTIL_BROADCAST, 0, MAC_TXOPTION_ACK);

		osal_start_timerEx(AmmeterApp_TaskID, AMMETERAPP_TX_TEST_EVT, 30000/FREQUENCY_MULTIPLICATION);

		return events ^ AMMETERAPP_TX_TEST_EVT;
	}
#endif

	if (events & AMMETERAPP_ROUTE_ALIVE_EVT)
	{
		MacParam_t param;
		param.panID = 0xFFFF;
		param.cluster_id = AMMETERAPP_NWK_CLUSTERID;
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
		if (AMMETER_ROUTE_ALIVETIME)
			osal_start_timerEx(AmmeterApp_TaskID, AMMETERAPP_ROUTE_ALIVE_EVT, AMMETER_ROUTE_ALIVETIME/FREQUENCY_MULTIPLICATION);
		return events ^ AMMETERAPP_ROUTE_ALIVE_EVT;
	}

	if (events & AMMETERAPP_ROUTE_CLEAN_EVT)
	{
		Nwk_RouteClean_t rtClean;
		rtClean.msgtype = ROUTECLEAN;
		rtClean.nodeID = MyInfo.nodeID;
		rtClean.srcAddress = MyInfo.nodeID;
		rtClean.dstAddress = 0xFFFF;
		rtClean.nodeDepth = 0;

		MacParam_t param;
		param.panID = 0xFFFF;
		param.cluster_id = AMMETERAPP_NWK_CLUSTERID;
		param.radius = 0x01;
		MAC_UTIL_BuildandSendDataPAN(&param, (uint8 *)&rtClean,
			sizeof(Nwk_RouteClean_t), MAC_UTIL_BROADCAST, 0, 0);

		rtCleanCnt++;
		if (rtCleanCnt <= AMMETER_ROUTE_BUILDCNT)
			osal_start_timerEx(AmmeterApp_TaskID, AMMETERAPP_ROUTE_CLEAN_EVT, AMMETER_ROUTE_REBUILDINTERVAL/3/FREQUENCY_MULTIPLICATION);
		else
		{
			if (AMMETER_ROUTE_REBUILDINTERVAL)
				osal_start_timerEx(AmmeterApp_TaskID, AMMETERAPP_ROUTE_REQ_EVENT, AMMETER_ROUTE_REBUILDINTERVAL/FREQUENCY_MULTIPLICATION);
		}		
		return events ^ AMMETERAPP_ROUTE_CLEAN_EVT;
	}

#ifdef DEBUG_ROUTE
	if(events & AMMETERAPP_REPORT_ROUTE_EVENT)
	{	
		if(printIdx == MAXNEIGHBOR)
		{
			/*
			uint8 startStr[5];
			uint8 i = 0;
			startStr[i++] = 's';
			startStr[i++] = 't';
			startStr[i++] = 'a';
			startStr[i++] = 'r';
			startStr[i] = 't';
			pSpi->DataLength = 5;
			osal_memcpy((uint8 *)(pSpi + 1), (uint8 *)startStr, pSpi->DataLength);
			HalSPIWrite(( uint8 *) pSpi, pSpi->DataLength + SPIPKTHDR_LEN);
			*/
			
			app_am_routerpt_t routerpt;
			routerpt.msgtype = AMMETER_ROUTE_REPORT;
			routerpt.reporttype = AMMETER_ROUTERPT_TYPE_BEGIN;
			
			pSpi->DataLength = sizeof(app_am_routerpt_t);
			osal_memcpy((uint8 *)(pSpi + 1), &routerpt, pSpi->DataLength);
			HalSPIWrite(( uint8 *) pSpi, pSpi->DataLength + SPIPKTHDR_LEN);

			osal_memcpy(pREListCopy, pREList, MAXNEIGHBOR*sizeof(RouteEntry_t));
			printIdx = reListIndex-1;
			aliveRoute = 0;

	
			
			if(ZSuccess != osal_start_timerEx(AmmeterApp_TaskID, AMMETERAPP_REPORT_ROUTE_EVENT, 300))
			{
				SystemReset();
			}
		}
		else if(printIdx < 0)
		{
			/*
			uint8 endStr[5];
			uint8 i = 0;
			endStr[i++] = 'e';
			endStr[i++] = 'n';
			endStr[i++] = 'd';
			endStr[i++] = aliveRoute;
			endStr[i] = '\0';
			pSpi->DataLength = 5;
			osal_memcpy((uint8 *)(pSpi + 1), (uint8 *)&endStr, pSpi->DataLength);
			HalSPIWrite(( uint8 *) pSpi, pSpi->DataLength + SPIPKTHDR_LEN);
			*/
			app_am_routerpt_t routerpt;
			routerpt.msgtype = AMMETER_ROUTE_REPORT;
			routerpt.reporttype = AMMETER_ROUTERPT_TYPE_END;
			
			pSpi->DataLength = sizeof(app_am_routerpt_t);
			osal_memcpy((uint8 *)(pSpi + 1), &routerpt, pSpi->DataLength);
			HalSPIWrite(( uint8 *) pSpi, pSpi->DataLength + SPIPKTHDR_LEN);

			printIdx = MAXNEIGHBOR;
			if(ZSuccess != osal_start_timerEx(AmmeterApp_TaskID, AMMETERAPP_REPORT_ROUTE_EVENT, 5000))
			{
				SystemReset();
			}
		}
		else
		{
			if ( pREListCopy[printIdx].expiryTime)
			{
				aliveRoute++;

				/*
				pSpi->DataLength = sizeof(RouteEntry_t);
				osal_memcpy((uint8 *)(pSpi + 1), (uint8 *)&pREListCopy[printIdx], sizeof(RouteEntry_t));
				HalSPIWrite(( uint8 *) pSpi, pSpi->DataLength + SPIPKTHDR_LEN);
				*/
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
			if(ZSuccess != osal_start_timerEx(AmmeterApp_TaskID, AMMETERAPP_REPORT_ROUTE_EVENT, 300))
			{
				SystemReset();
			}
		}

		return events ^ AMMETERAPP_REPORT_ROUTE_EVENT;
	}
#endif

#if (defined CFG_AMMETER_IEC || defined CFG_AMMETER_TRANSPARENT) && (defined HAL_UART)

	if ( events & AMMETERAPP_ACK_SEND_EVT )
	{
		MacParam_t param;
		param.panID = ackAddr;
		param.cluster_id = AMMETERAPP_CLUSTERID;
		param.radius = 0x01;
		MAC_UTIL_BuildandSendDataPAN(&param, txBuf,
			txLen, MAC_UTIL_UNICAST, 0, MAC_TXOPTION_ACK);
		return ( events ^ AMMETERAPP_ACK_SEND_EVT );
	}

	if(events & AMMETERAPP_UART_READ_EVT)
	{
		uint16 len=0;

#ifdef CFG_AMMETER_CMDLINE		
		halUARTIoctl_t  ioctl;
		
		HalUARTIoctl(AMMETER_APP_PORT, HAL_UART_IOCTL_GET,  &ioctl);
		if(ioctl.workMode == HAL_UART_WORKMODE_REPORT)
		{
			len = HalUARTRead(AMMETER_APP_PORT, AmmeterApp_UARTbuf + UARTbufTailIndex, AmmeterApp_UARTbufLen - UARTbufTailIndex);
		}
		else
		{
			len = 0;
		}
#else 
			len = HalUARTRead(AMMETER_APP_PORT, AmmeterApp_UARTbuf + UARTbufTailIndex, AmmeterApp_UARTbufLen - UARTbufTailIndex);
#endif 
		UARTbufTailIndex += len;
		timeSinceLastSending += UARTreadInterval;

		if((timeSinceLastSending >= SendingInterval || len ==0) && UARTbufTailIndex > 0)
		{
			//Fixme, tricky code, build the head only here
			AmmeterApp_BuildData(AmmeterApp_Msg, UARTbufTailIndex);
			AmmeterApp_ProcessUARTCB(AmmeterApp_Msg, UARTbufTailIndex+sizeof(app_amdata_t));

			timeSinceLastSending = 0;
			UARTbufTailIndex = 0;
		}

/*  
// keep this logic for frame data parsing

		switch (IEC_info.state)
		{
			case E_IEC_STATE_UART_IDLE:
				if( len != 0) IEC_info.state = E_IEC_STATE_UART_READING;
				break;
			case E_IEC_STATE_UART_READING:
				if( len == 0) IEC_info.state = E_IEC_STATE_UART_FINISHED;
				break;
			case E_IEC_STATE_UART_FINISHED:
			{
				//IEC_ParseFrame(AmmeterApp_UARTbuf, UARTbufTailIndex, D_IEC_SERVER, &IEC_info);

				//if(ZSuccess != osal_start_timerEx(AmmeterApp_TaskID, AMMETERAPP_IEC_TEST_EVT, 250))
				//{
				//	SystemReset();
				//}	
		
				

				
				AmmeterApp_BuildData(AmmeterApp_Msg, UARTbufTailIndex);
				AmmeterApp_ProcessUARTCB(AmmeterApp_Msg, UARTbufTailIndex+sizeof(app_amdata_t));		

				//server decide the baud_rate and send to client in this frame

				IEC_info.state = E_IEC_STATE_UART_IDLE;
				UARTbufTailIndex = 0;
			}
				break;
			default:
				break;
		}
*/		
		if(ZSuccess != osal_start_timerEx(AmmeterApp_TaskID, AMMETERAPP_UART_READ_EVT, 50))
		{
			SystemReset();
		}	
		return events ^ AMMETERAPP_UART_READ_EVT; 
	}

#endif 


	return 0;
}

#if (defined HAL_SPI) && (HAL_SPI == TRUE)
/*********************************************************************
* @fn      AmmeterApp_MessageSPICB
*
* @brief   SPI message processor callback.  This function processes
*          any incoming data from ARM. Actually, It's based on Zigbee
*	      Coordinator.
*
* @param   MSGpkt - The incoming msg.
*
* @return  none
*/
void AmmeterApp_ProcessSPICB ( const SPIMSGPacket_t *MSGpkt )
{
	APPWrapper_t* appwrapper = ( APPWrapper_t* ) ( MSGpkt+1 );

	if (appwrapper->app_flag == AMMETER_DATA
		|| appwrapper->app_flag == AMMETER_DATA_ACK)
	{
		appwrapper->app_amdata.srcPan = zgConfigPANID;//set the srcpan for the pkt.
		if ((appwrapper->app_flag == AMMETER_DATA && appwrapper->app_amdata.dstPan == 0xFFFF)
			||( appwrapper->app_flag == AMMETER_DATA_ACK &&appwrapper->app_amack.dstpan == 0xFFFF))
		{
			if(appwrapper->app_flag ==AMMETER_DATA)
			{
		 		AmmeterApp_ProcessDataLED();
			}
			osal_memcpy(AmmeterApp_DataTrans.databuf, (uint8 *)appwrapper, MSGpkt->DataLength);
			AmmeterApp_DataTrans.datasize = MSGpkt->DataLength;
			AmmeterApp_DataTrans.reIndex = reListIndex;
			osal_start_timerEx(AmmeterApp_TaskID, AMMETERAPP_MSG_SEND_EVT, 20);
		}
		else
		{
			MacParam_t param;
			param.cluster_id = AMMETERAPP_CLUSTERID;
			param.radius = 0x01;

			int16 i = 0;
			for (; i < reListIndex; ++i)
			{
				if (pREList[i].dstAddress == appwrapper->app_amdata.dstPan
					/*&& pREList[i].expiryTime*/)
				{
					param.panID = pREList[i].nextHopAddress;
					MAC_UTIL_BuildandSendDataPAN(&param, (uint8 *)appwrapper,
						MSGpkt->DataLength, MAC_UTIL_UNICAST, 0, MAC_TXOPTION_ACK);	
					break;
				}
			}		
		}
	}
}
#endif

#if (defined CFG_AMMETER_IEC)  || (defined CFG_AMMETER_TRANSPARENT)
static void AmmeterApp_BuildData(uint8 *buf, uint8 len)
{
	app_amdata_t app_amdata;
	app_amdata.msgtype = AMMETER_DATA;
	app_amdata.srcPan = MyInfo.nodeID;
	app_amdata.dstPan = 0xFFFF;
	app_amdata.seqnum = 1;
	app_amdata.packnum = 1;
	app_amdata.packseq = 1;
	app_amdata.len = len;
	osal_memcpy(buf, (void *)&app_amdata, sizeof(app_amdata_t));	
}


static void AmmeterApp_ProcessUARTCB(uint8 *data, uint8 len)
{
	APPWrapper_t* appwrapper = ( APPWrapper_t* ) (data);

	appwrapper->app_amdata.srcPan = zgConfigPANID;//set the srcpan for the pkt.
	if ((appwrapper->app_flag == AMMETER_DATA && appwrapper->app_amdata.dstPan == 0xFFFF))
	{
	 	AmmeterApp_ProcessDataLED();
		osal_memcpy(AmmeterApp_DataTrans.databuf, (uint8 *)appwrapper, len);
		AmmeterApp_DataTrans.datasize = len;
		AmmeterApp_DataTrans.reIndex = reListIndex;
		osal_start_timerEx(AmmeterApp_TaskID, AMMETERAPP_MSG_SEND_EVT, 20);
	}
}

static uint16 GetBitTransmitInterval(uint8 bautRate)
{
	switch (bautRate)
	{
		case HAL_UART_BR_300:
			return 34000;
		case HAL_UART_BR_600:
			return 17000;
		case HAL_UART_BR_1200:
			return 8400;
		case HAL_UART_BR_2400:
			return 4200;
		case HAL_UART_BR_4800:
			return 2100;
		case HAL_UART_BR_9600:
			return 1050;
		case HAL_UART_BR_19200:
			return 530;
		case HAL_UART_BR_31250:
			return 320;
		case HAL_UART_BR_38400:
			return 270;
		case HAL_UART_BR_57600:
			return 175;
		case HAL_UART_BR_115200:
			return 90;
		default:
			;
}

	return 40000;
}

#endif 
void AmmeterApp_ProcessMSGCB ( const afIncomingMSGPacket_t *pkt )
{
	switch ( pkt->clusterId )
	{
	case AMMETERAPP_CLUSTERID:
		{
			APPWrapper_t* appwrapper = ( APPWrapper_t* ) (pkt->cmd.Data);
			if (appwrapper->app_flag == AMMETER_DATA || appwrapper->app_flag == AMMETER_DATA_LAST || appwrapper->app_flag == AMMETER_DATA_CMDLINE)
			{
				AmmeterApp_ProcessDataLED();
			}
#if ((!defined CFG_AMMETER_IEC) && (!defined DEBUG_REPORT_NW))
			pSpi->DataLength = pkt->cmd.DataLength;
			osal_memcpy((uint8 *)(pSpi + 1), pkt->cmd.Data, pSpi->DataLength);
			HalSPIWrite(( uint8 *) pSpi, pSpi->DataLength + SPIPKTHDR_LEN);		
#else 
			// APPWrapper_t* appwrapper = ( APPWrapper_t* ) (pkt->cmd.Data);
			//osal_stop_timerEx(AmmeterApp_TaskID, AMMETERAPP_UART_READ_EVT);

			if (appwrapper->app_flag == AMMETER_DATA || appwrapper->app_flag == AMMETER_DATA_LAST || appwrapper->app_flag == AMMETER_DATA_CMDLINE)
			{
				HalUARTWrite(AMMETER_APP_PORT, pkt->cmd.Data + sizeof(app_amdata_t), pkt->cmd.DataLength - sizeof(app_amdata_t));
				AmmeterApp_SendAmmeterAck(appwrapper->app_amdata.seqnum, appwrapper->app_amdata.packnum,appwrapper->app_amdata.packseq, appwrapper->app_amdata.srcPan);					
			}
			
#endif
			break;
		}
	case AMMETERAPP_NWK_CLUSTERID:
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
							pREList[i].expiryTime = AMMETER_ROUTE_EXPIRECNT;
							IsRouteExist = true;
							break;
						}
					}
					if (!IsRouteExist)
					{
						pREList[reListIndex].dstAddress = pRRsp->srcAddress;
						pREList[reListIndex].nextHopAddress = pRRsp->nodeID;
						pREList[reListIndex].expiryTime = AMMETER_ROUTE_EXPIRECNT;
						reListIndex++;
					}
				}
				else
				{
					/*
					for (i = 0; i < reListIndex; ++i)
					{
					if (pREList[i].dstAddress == pRRsp->dstAddress)
					{
					pREList[i].nextHopAddress = pRRsp->nodeID;
					pREList[i].expiryTime = AMMETER_ROUTE_EXPIRECNT;
					}
					else
					{
					if (pREList[i].expiryTime)
					pREList[i].expiryTime--;
					}
					}
					*/
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
#ifdef CFG_AMMETER_CMDLINE
				halUARTIoctl_t  ioctl;
				HalUARTIoctl(AMMETER_APP_PORT, HAL_UART_IOCTL_GET,  &ioctl);
				if(ioctl.workMode == HAL_UART_WORKMODE_REPORT)
				{
					PrintRoute(&(pkt->cmd.Data[1]), pkt->cmd.DataLength-1);
				}
#else
				PrintRoute(&(pkt->cmd.Data[1]), pkt->cmd.DataLength-1);
#endif
			}	

#endif
			break;
		}
	}
}
void AmmeterApp_ProcessDataLED(void )
{
	HalLedSet(HAL_LED_DATA,HAL_LED_MODE_OFF);
	HalLedBlink(HAL_LED_DATA, 1, 50, 300);
}

void AmmeterApp_SendData(app_amdata_t *pHead,  uint8* pData, uint16 len)
{
	uint8 sendbuf[MAX_DATA_SIZE];
	uint8 sendlen;
	
	AmmeterApp_ProcessDataLED();		
	pHead->srcPan = zgConfigPANID;
	if (pHead->dstPan == 0xFFFF)
	{
		osal_memcpy(AmmeterApp_DataTrans.databuf, (uint8 *)pHead, sizeof(app_amdata_t));
		osal_memcpy(AmmeterApp_DataTrans.databuf + sizeof(app_amdata_t), pData,len);			
		AmmeterApp_DataTrans.datasize = sizeof(app_amdata_t) + len;
		AmmeterApp_DataTrans.reIndex = reListIndex;
		osal_start_timerEx(AmmeterApp_TaskID, AMMETERAPP_MSG_SEND_EVT, 20);
	}
	else
	{
		MacParam_t param;
		param.cluster_id = AMMETERAPP_CLUSTERID;
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
#ifdef DEBUG_AMMETER
uint16 GetCheckCode(const uint8 * pSendBuf, int8 nEnd)
{ 
	int8 i, j;
	uint16 wCrc = 0xFFFF;
	for(i=0; i<nEnd; i++)  
	{   
		wCrc ^= pSendBuf[i];    
		for(j=0; j<8; j++)   
		{   
			if(wCrc & 1)   
			{ 
				wCrc >>= 1;   
				wCrc ^= 0xA001;    
			}   
			else   
			{   
				wCrc >>= 1;     
			}   
		}   
	}    
	return wCrc;
}
#endif

#ifdef DEBUG_REPORT_NW

static void PrintRoute(uint8* data, uint8 len)
{
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
}
#ifdef CFG_AMMETER_CMDLINE
static void Ammeter_UartCBCmdLine (uint8 port, uint8 event)
{
	uint8 readlen;
	readlen = HalUARTRead(AMMETER_APP_PORT, dataBuf.p, AMMETERAPP_MAX_DATA_LEN);
	dataBuf.idx = readlen;
	dataBuf.p[readlen] = '\0';

 	if(readlen > 0)
	{
		Ammeter_ParseUartCmd((char *)dataBuf.p);
	}
}
static void Ammeter_ParseUartCmd( char* cmd)
{
	char *s = StrUtil_trim(cmd);
	char* p = strtok(s," ");
	if(osal_strcmp((void*)p, (void *)"run") == 0)
	{
		halUARTIoctl_t  ioctl;

		HalUARTIoctl(AMMETER_APP_PORT, HAL_UART_IOCTL_GET,  &ioctl);
		ioctl.workMode = HAL_UART_WORKMODE_REPORT;
		HalUARTIoctl(AMMETER_APP_PORT, HAL_UART_IOCTL_SET,  &ioctl);

		char *p="\r\n";
		HalUARTWrite(AMMETER_APP_PORT, (uint8*)p, osal_strlen(p));
	}
	else if(osal_strcmp((void*)p, (void *)"send") == 0) // send addr ddata
	{
		uint16 addr;
		char   buf[128];
		int16 datalen = 0;
		int16 addrlen;
		
		p = strtok(NULL," "); 
		if(!p || osal_strlen(p) != 4) return;
		

		addrlen =  HexStrToU8Data((uint8*)buf, p);

		if(addrlen == 2)
		{
			addr = BUILD_UINT16(buf[1], buf[0]);
		}
		else
		{
			return;
		}

		p = strtok(NULL," "); 
		if(!p) return;

		datalen =  osal_strlen(p);
		
		uint8 pktsize = MAX_DATA_SIZE - sizeof(app_amdata_t);

		if (datalen > pktsize) datalen = pktsize;
		if (datalen > 0)
		{
			app_amdata_t head;
			head.msgtype = AMMETER_DATA_CMDLINE;
			head.srcPan =  zgConfigPANID;
			head.dstPan = addr;
			head.seqnum = pktSeqnum++;
			head.packnum = 1;
			head.packseq  = 1;
			head.len = datalen;
			AmmeterApp_SendData(&head,  (uint8*)p, datalen);
		}
	}
}
#endif

#endif //DEBUG_REPORT_NW

#if (defined CFG_AMMETER_IEC || defined CFG_AMMETER_TRANSPARENT)

static void AmmeterApp_SendAmmeterAck( uint16 seqnum, uint8 packnum,uint8 packseq, uint16 dstAddr)
{
	app_amack_t app_amack;
	app_amack.msgtype = AMMETER_DATA_ACK;
	app_amack.srcpan = MyInfo.nodeID;
	app_amack.dstpan = dstAddr;
	app_amack.seqnum = seqnum;

	app_amack.packnum = packnum;
	app_amack.packseq  = packseq;
	osal_memcpy(txBuf, (uint8 *)&app_amack, sizeof(app_amack_t));
	txLen = sizeof(app_amack_t);

	ackAddr = dstAddr;
	osal_set_event(AmmeterApp_TaskID, AMMETERAPP_ACK_SEND_EVT);
}

#endif

