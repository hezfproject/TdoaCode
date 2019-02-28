/**************************************************************************************************
Filename:       AmmeterApp_Collector.c
Revised:        $Date: 2010/06/30 02:51:41 $
Revision:       $Revision: 1.49 $

Description -   Ammeter Collector Application: collect ammeter data and transmit to data center
and also repeat the other ammeter data from Collector and transmit to data center.
**************************************************************************************************/
/*********************************************************************
* INCLUDES
*/

#include "ZComDef.h"
#include "AmmeterApp_Collector.h"
#include "AmmeterApp_cfg.h"
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

#if (defined DEBUG_REPORT_NW)  || (defined CFG_AMMETER_IEC)
#undef HAL_SPI
#undef DEBUG_ROUTE
#endif

#if !defined( AMMETER_APP_BAUD )
// CC2430 only allows 38.4k or 115.2k.
#define AMMETER_APP_BAUD  HAL_UART_BR_9600
//#define AMMETER_APP_BAUD  HAL_UART_BR_115200
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

#define AMMETER_APP_RSP_CNT  4

#if !defined(AMMETER_APP_PORT)
#define AMMETER_APP_PORT  0
#endif

#define AMMETER_APP_RTRY_TIMES  3

#if (defined CFG_AMMETER_IEC) || (defined DEBUG_REPORT_NW) 
#undef AMMETER_APP_RTRY_TIMES
#define AMMETER_APP_RTRY_TIMES 1
#endif

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
/*********************************************************************
* TYPEDEFS
*/
typedef struct
{
	uint16 len;
	uint16 seqnum;    // the seqnum of the whole packet
	uint8 packnum;    // How many blocks contained in a packet 
	uint8 packseq;    // one block's seqnum  in  the packet
} pkt_attr_t;

typedef struct
{
	uint16 candidateID;
	int8 	 rssi;
	uint8 nodeDepth;
	uint8 counter;
} Candidate_t;

typedef struct
{
	uint8* p;
	uint8 idx;
	uint8 sendIdx;
}buf_t;

/*********************************************************************
* GLOBAL VARIABLES
*/
uint8 AmmeterApp_TaskID;// Task ID for internal task/event processing.

/*********************************************************************
* EXTERNAL VARIABLES
*/

/*********************************************************************
* EXTERNAL FUNCTIONS
*/

/*********************************************************************
* LOCAL VARIABLES
*/
static devStates_t AmmeterApp_NwkState;

static uint8 rtryCnt = 0;

static NodeInfo MyInfo = {0, 0, 0, 0, INVALIDLINKQUAL, true, 0}; //Identify the node ID and node depth.

static buf_t dataBuf = {NULL,0,0};
#ifdef DEBUG_AMMETER
static uint8 DebugBuf[64];
static uint8 cnt;
static bool flag;
#endif
static uint8 txBuf[MAX_DATA_SIZE];
static uint8  txLen = 0;

static pkt_attr_t rxpkt = {0, 0, 0, 0};
static pkt_attr_t txpkt = {0, 0, 0, 0};

static uint8 uartdatalen = 0;
static bool IsPortBusy = false;

static RouteEntry_t* pREList = 0;
static int16 reListIndex = 0;
static int16 reValidChild = 0;

static bool  UartIsReading = false;
static uint8 UartReadingCnt = 0;
//static uint8 broadcastfreq = 0;

static bool IsParentAlive = false;
static uint8 toleranceCnt = 0;
static Candidate_t  candidateArray[CANDIDATE_MAX];

#if(defined CFG_AMMETER_IEC || defined CFG_AMMETER_TRANSPARENT)
/*reuse the buffer*/
static uint8 *AmmeterApp_UARTbuf; 
static uint8 AmmeterApp_UARTbufLen;
static uint8 UARTbufTailIndex = 0;
static uint8 UARTbufHeadIndex = 0;
static uint16 SendingInterval = 150;
static uint16 timeSinceLastSending = 0;
static halUARTIoctl_t UARTIOconfig;

#ifdef CFG_AMMETER_IEC
static tsIEC_info IEC_info;
#endif

static uint8 UARTreadInterval = 50;
#endif


/*********************************************************************
* LOCAL FUNCTIONS
*/
static void AmmeterApp_ProcessMSGCmd(afIncomingMSGPacket_t *pkt);
static void AmmeterApp_SendData(uint8 *buf, uint8 len);
static uint8 AmmeterApp_ParseData(uint8* buf, uint8 len);
static void AmmeterApp_BuildData( uint8 *buf, uint8 len);
#if (defined HAL_UART) && (HAL_UART == TRUE) 
static bool AmmeterApp_UartSearch(uint8 * pAmmeterID, uint8* pBaudRate);
#endif
//static uint16 AmmeterApp_GetTransBitsCnt(uint8* p, bool isRead);
//static void rxCB(uint8 port, uint8 event);
static uint16 GetCheckCode(const uint8 * pSendBuf, int8 nEnd);
static void InitMyInfo(void);
static uint8 RouteEvaluation(uint16 parentid, uint16 sinkID, uint8 nodeDepth, int8 rssi);
static int8   PollRoute(uint16 parentid, uint8 nodeDepth, int8 rssi);
static void AmmeterApp_ProcessDataLED(void );
static void AmmeterApp_StateReset(void);
static void AmmeterApp_StartReadAmmeter(void);
static void AmmeterApp_SendDataToCenter( uint8 *buf, uint8 len );
static void AmmeterApp_SendAmmeterAck( uint16 seqnum, uint8 packnum,uint8 packseq);

#ifdef DEBUG_REPORT_NW
static void ReportRoute(uint8 *data, uint8 len);

#ifdef CFG_AMMETER_CMDLINE
static void AmmeterApp_ProcessCmdData(uint8 * data);
void Ammeter_UartCBCmdLine (uint8 port, uint8 event);
void Ammeter_ParseUartCmd( char* cmd);
void AmmeterApp_SendDataCmdToCenter( uint8 *buf, uint8 len );
#endif

#endif //DEBUG_REPORT_NW

#if(defined CFG_AMMETER_IEC || defined CFG_AMMETER_TRANSPARENT)
static void AmmeterApp_ProcessUARTCB(uint8 *data, uint8 len);
static void AmmeterApp_ProcessTransparentData(uint8 * data, bool needACK);
void AmmeterApp_BuildDataLast( uint8 *buf, uint8 len);
void AmmeterApp_SendDataLastToCenter( uint8 *buf, uint8 len );

#endif

#ifdef CFG_AMMETER_MODBUS
static void AmmeterApp_ProcessModbusData(uint8 * data);
#endif

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
	halUARTCfg_t uartConfig;

	AmmeterApp_TaskID = task_id;
#if(defined WATCHDOG) && (WATCHDOG==TRUE)
	StartWatchDog ( DOGTIMER_INTERVAL_1S );
	osal_set_event ( AmmeterApp_TaskID, AMMETERAPP_FEEDWATCHDOG_EVENT);
#endif

	/* EP and  Profile Id are not used,Card will drop them directly */
	MacUtil_t MacUtil;
	MacUtil.panID = 0xFFFF;  /* Broadcast.*/
	MacUtil.dst_endpoint = AMMETERAPP_ENDPOINT;
	MacUtil.src_endpoint = 0x0F;//From sniffer.
	MacUtil.profile_id = 0x0500;//From sniffer.
	MacUtil.cluster_id = AMMETERAPP_CLUSTERID;
	MacUtil.NodeType = ZDO_Config_Node_Descriptor.LogicalType;
	MAC_UTIL_INIT ( &MacUtil );

	afRegister( (endPointDesc_t *)&AmmeterApp_epDesc );

	uartConfig.configured           = TRUE;              // 2430 don't care.
	uartConfig.baudRate             = AMMETER_APP_BAUD;
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

#if	(defined CFG_AMMETER_101) 
	halUARTIoctl_t UARTconfig;
	HalUARTIoctl(AMMETER_APP_PORT, HAL_UART_IOCTL_GET, &UARTconfig);
	UARTconfig.linkFormat=  HAL_UART_LINKFORMAT_2;
	HalUARTIoctl(AMMETER_APP_PORT, HAL_UART_IOCTL_SET, &UARTconfig);
#endif

	dataBuf.p = osal_mem_alloc(AMMETER_MAX_PKT_LEN);
	if (!dataBuf.p)
		SystemReset();

#if(defined CFG_AMMETER_IEC || defined CFG_AMMETER_TRANSPARENT)
	/*reuse the buffer*/
	AmmeterApp_UARTbuf = dataBuf.p; 
	AmmeterApp_UARTbufLen = AMMETER_MAX_PKT_LEN;
#endif

#if defined CFG_AMMETER_IEC
	HalUARTIoctl (AMMETER_APP_PORT, HAL_UART_IOCTL_GET, &UARTIOconfig);
	UARTIOconfig.linkFormat = HAL_UART_LINKFORMAT_1;
	HalUARTIoctl (AMMETER_APP_PORT, HAL_UART_IOCTL_SET, &UARTIOconfig);
#endif	

	pREList = osal_mem_alloc(MAXNEIGHBOR*sizeof(RouteEntry_t));
	if(!pREList)
		SystemReset();
	osal_memset((void *)pREList, 0, MAXNEIGHBOR*sizeof(RouteEntry_t));
	osal_memset((void *)candidateArray, 0, 5*sizeof(Candidate_t));

#ifdef CFG_AMMETER_IEC
	IEC_info.state = E_IEC_STATE_UART_IDLE;
	IEC_info.protocol = D_IEC_PROTOCOL_C;
	IEC_info.com_baud_rate = AMMETER_APP_BAUD;
	IEC_info.baud_switch = 0;
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
//uint8 rxbuf[16];
UINT16 AmmeterApp_ProcessEvent( uint8 task_id, UINT16 events )
{
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
		afIncomingMSGPacket_t *MSGpkt;
		while ( (MSGpkt = (afIncomingMSGPacket_t *)osal_msg_receive(
			AmmeterApp_TaskID)) )
		{
			switch ( MSGpkt->hdr.event )
			{
			case ZDO_STATE_CHANGE:
				{
					AmmeterApp_NwkState = (devStates_t)(MSGpkt->hdr.status);
					if (AmmeterApp_NwkState == DEV_ZB_COORD)
					{
#ifdef DEBUG_AMMETER
						if (ZSUCCESS != osal_start_timerEx( AmmeterApp_TaskID, AMMETERAPP_TX_RTRY_EVT, 2000))
							SystemReset();
#endif
						if (ZSUCCESS != osal_start_timerEx(AmmeterApp_TaskID, AMMETERAPP_ROUTE_EXPIRE_EVT, 10000/FREQUENCY_MULTIPLICATION))
							SystemReset();
						if (ZSUCCESS != osal_start_timerEx(AmmeterApp_TaskID, AMMETERAPP_ROUTE_ALIVE_EVT, 60000/FREQUENCY_MULTIPLICATION))
							SystemReset();
						if (ZSUCCESS != osal_start_timerEx(AmmeterApp_TaskID, AMMETERAPP_ROUTE_REPORT_EVT, 20000/FREQUENCY_MULTIPLICATION))
							SystemReset();
						if (ZSUCCESS != osal_start_timerEx(AmmeterApp_TaskID, AMMETERAPP_ROUTE_REPAIR_EVT, 65000/FREQUENCY_MULTIPLICATION))
							SystemReset();
#ifdef DEBUG_REPORT_NW
						if (0 && ZSUCCESS != osal_start_timerEx(AmmeterApp_TaskID, AMMETERAPP_REPORT_NW_EVT, 10000/FREQUENCY_MULTIPLICATION))
							SystemReset();						
#endif

#if (defined CFG_AMMETER_IEC || defined CFG_AMMETER_TRANSPARENT)
						if(ZSUCCESS !=osal_start_timerEx(AmmeterApp_TaskID, AMMETERAPP_UART_READ2_EVT, 20))
							SystemReset();		
#endif

					}
					break;
				}
			case AF_INCOMING_MSG_CMD:
				AmmeterApp_ProcessMSGCmd( MSGpkt );
				break;

			default:
				break;
			}

			osal_msg_deallocate( (uint8 *)MSGpkt );  // Release the memory.
		}

		//Return unprocessed events.
		return ( events ^ SYS_EVENT_MSG );
	}

	if ( events & AMMETERAPP_ACK_SEND_EVT )
	{
		MacParam_t param;
		param.panID = MyInfo.parentID;
		param.cluster_id = AMMETERAPP_CLUSTERID;
		param.radius = 0x01;
		MAC_UTIL_BuildandSendDataPAN(&param, txBuf,
			txLen, MAC_UTIL_UNICAST, 0, MAC_TXOPTION_ACK);
		return ( events ^ AMMETERAPP_ACK_SEND_EVT );
	}

	if (events & AMMETERAPP_MSG_SEND_EVT)
	{
		MacParam_t param;

		if(rtryCnt == AMMETER_APP_RTRY_TIMES)
		{
			AmmeterApp_StateReset();
			return ( events ^ AMMETERAPP_MSG_SEND_EVT);
		}
		
		param.panID = MyInfo.parentID;
		param.cluster_id = AMMETERAPP_CLUSTERID;
		param.radius = 0x01;
		MAC_UTIL_BuildandSendDataPAN(&param, txBuf,txLen, MAC_UTIL_UNICAST, 0, MAC_TXOPTION_ACK);
		AmmeterApp_ProcessDataLED();
		rtryCnt++;
		if (rtryCnt < AMMETER_APP_RTRY_TIMES)
		{
			osal_start_timerEx( AmmeterApp_TaskID, AMMETERAPP_MSG_SEND_EVT, 600);
		}
		
		return ( events ^ AMMETERAPP_MSG_SEND_EVT);
	}
	if (events & AMMETERAPP_ROUTE_REPORT_EVT)
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
		param.cluster_id = AMMETERAPP_NWK_CLUSTERID;
		param.radius = 0x01;
		if (MyInfo.nodeDepth)
			MAC_UTIL_BuildandSendDataPAN(&param, (uint8 *)&rtRsp,
			sizeof(Nwk_RouteRsp_t), MAC_UTIL_UNICAST, 0, 0);

		if (AMMETER_ROUTE_EXPIRETIME > 0)
		{
			uint16 RandDelay = MAC_RandomByte();             
			RandDelay <<= 2; 
			if (ZSUCCESS != osal_start_timerEx( AmmeterApp_TaskID, AMMETERAPP_ROUTE_REPORT_EVT, AMMETER_ROUTE_EXPIRETIME/FREQUENCY_MULTIPLICATION + RandDelay))
				SystemReset();
#ifdef DEBUG_REPORT_NW
			if(ZSuccess != osal_set_event(AmmeterApp_TaskID, AMMETERAPP_REPORT_NW_EVT))
			{
				SystemReset();
			}
#endif

		}

		return events ^ AMMETERAPP_ROUTE_REPORT_EVT;
	}
 	if (events & AMMETERAPP_UART_READ_EVT)
	{
		uint8 readLen = 0;
#if (defined HAL_UART) && (HAL_UART == TRUE)
		readLen = HalUARTRead(AMMETER_APP_PORT, dataBuf.p+dataBuf.idx, AMMETER_MAX_PKT_LEN-dataBuf.idx);	// read all uart data out
#endif
		dataBuf.idx += readLen;
		if(dataBuf.idx>= AMMETER_MAX_PKT_LEN) // read buffer full
		{
			AmmeterApp_SendDataToCenter(dataBuf.p, dataBuf.idx);
			dataBuf.sendIdx = txpkt.len;
		}
		else
		{	

			if(readLen > 0)   // read something out
			{
				UartIsReading= true;
				if (ZSUCCESS != osal_start_timerEx(AmmeterApp_TaskID, AMMETERAPP_UART_READ_EVT,AMMETER_UART_READINTERVAL))
					SystemReset();
			}
			else if(readLen==0 && UartIsReading == false) // not begin reading
			{	
				if (ZSUCCESS != osal_start_timerEx(AmmeterApp_TaskID, AMMETERAPP_UART_READ_EVT,AMMETER_UART_READINTERVAL))
					SystemReset();

				UartReadingCnt++;
				if(UartReadingCnt >= AMMETER_MAX_UART_READCNT)
				{
					AmmeterApp_StateReset();
				}
			}
			else if(readLen==0 &&  UartIsReading == true)  // reading end
			{
				if(dataBuf.idx > 0)
				{
					AmmeterApp_SendDataToCenter(dataBuf.p, dataBuf.idx);
					dataBuf.sendIdx = txpkt.len;
				}
				else
				{
					AmmeterApp_StateReset();
				}
			}
		}

		return ( events ^ AMMETERAPP_UART_READ_EVT);
	}
#ifdef DEBUG_AMMETER
	if (events & AMMETERAPP_TX_RTRY_EVT )
	{
		uint8 test[16];
		int8 i = 0;

		test[i++] = 0x11;
		test[i++] = 0x10;
		test[i++] = 0x03;
		test[i++] = 0xE9;    // 0x29;
		test[i++] = 0x00;
		test[i++] = 0x01;
		test[i++] = 0x02;
		test[i++] = 0x00;
		test[i++] = 0x00;
		/*
		test[i++] = 0x11;
		test[i++] = 0x03;
		test[i++] = 0x03;    // 0xa0;
		test[i++] = 0xE8;    // 0x29;
		test[i++] = 0x00;
		test[i++] = 0x02;
		*/

		uint16 CRC = GetCheckCode(test, i);
		test[i++] = LO_UINT16(CRC);
		test[i++] = HI_UINT16(CRC);

		uint16 ReadLen = 2*BUILD_UINT16(*((uint8*)test + AMMETER_REGCNT_IDX+1), *((uint8*)test + AMMETER_REGCNT_IDX));
		ReadLen += 3 + 2;

		//HalUARTFlushTxBuf(AMMETER_APP_PORT);
		//HalUARTFlushRxBuf(AMMETER_APP_PORT);
		HalUARTWrite(AMMETER_APP_PORT, test, i);
		uint16 ReadTimeout =100;    // read time is 1.5 byte/ms
		osal_start_timerEx( AmmeterApp_TaskID, AMMETERAPP_RX_RTRY_EVT, ReadTimeout);

		return ( events ^ AMMETERAPP_TX_RTRY_EVT );
	}
	if (events & AMMETERAPP_RX_RTRY_EVT )
	{
		uint8 ID,BaudRate;
		cnt = HalUARTRead(AMMETER_APP_PORT, DebugBuf, 255);
		//flag = AmmeterApp_UartSearch(&ID,&BaudRate);
		osal_start_timerEx( AmmeterApp_TaskID, AMMETERAPP_TX_RTRY_EVT, 500);
		return ( events ^ AMMETERAPP_RX_RTRY_EVT );
	}
#endif

	if (events & AMMETERAPP_ROUTE_EXPIRE_EVT)
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

		if (AMMETER_ROUTE_EXPIRETIME)
		{
			if (ZSUCCESS != osal_start_timerEx(AmmeterApp_TaskID, AMMETERAPP_ROUTE_EXPIRE_EVT, AMMETER_ROUTE_EXPIRETIME/FREQUENCY_MULTIPLICATION))
				SystemReset();
		}
		return events ^ AMMETERAPP_ROUTE_EXPIRE_EVT;
	}

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
		if (MyInfo.nodeDepth)
			MAC_UTIL_BuildandSendDataPAN(&param, (uint8 *)&rtAlive,
			sizeof(Nwk_RouteAlive_t), MAC_UTIL_BROADCAST, 0, 0);	
		if (AMMETER_ROUTE_ALIVETIME)
		{
			if (ZSUCCESS != osal_start_timerEx(AmmeterApp_TaskID, AMMETERAPP_ROUTE_ALIVE_EVT, AMMETER_ROUTE_ALIVETIME/FREQUENCY_MULTIPLICATION))
				SystemReset();
		}
		return events ^ AMMETERAPP_ROUTE_ALIVE_EVT;
	}

	if (events & AMMETERAPP_ROUTE_REPAIR_EVT)
	{
		if (IsParentAlive)
		{
			IsParentAlive = false;
			toleranceCnt = 0;
		}

		else if (!MyInfo.needRepair)
			toleranceCnt++;

		if (toleranceCnt >=AMMETER_ROUTE_TOLERANCECNT)
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
		if (AMMETER_ROUTE_ALIVETIME)
		{
			if (ZSUCCESS != osal_start_timerEx(AmmeterApp_TaskID, AMMETERAPP_ROUTE_REPAIR_EVT, AMMETER_ROUTE_ALIVETIME/FREQUENCY_MULTIPLICATION))
				SystemReset();
		}
		return events ^ AMMETERAPP_ROUTE_REPAIR_EVT;
	}

	if(events & AMMETERAPP_PORTBUSY_TIMEOUT_EVT)
	{	
		AmmeterApp_StateReset();
		return events ^ AMMETERAPP_PORTBUSY_TIMEOUT_EVT;
	}
#ifdef DEBUG_REPORT_NW
	if (events & AMMETERAPP_REPORT_NW_EVT)
	{
		if (reValidChild==0)
		{	
			ReportRoute(0 ,0);
		}	

		//if (ZSUCCESS != osal_start_timerEx(AmmeterApp_TaskID, AMMETERAPP_REPORT_NW_EVT, 10000))
		//	SystemReset();									

		return events ^ AMMETERAPP_REPORT_NW_EVT;
	}
#endif

#if (defined HAL_UART) && (HAL_UART == TRUE) && (defined CFG_AMMETER_IEC || defined CFG_AMMETER_TRANSPARENT)
	if (events & AMMETERAPP_UART_READ2_EVT)
	{
		uint16 len;

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
			AmmeterApp_SendDataToCenter(AmmeterApp_UARTbuf, UARTbufTailIndex);

			timeSinceLastSending = 0;
			UARTbufTailIndex = 0;
		}

/*
		switch (IEC_info.state)
		{
		case E_IEC_STATE_UART_IDLE:
			if( len != 0) IEC_info.state = E_IEC_STATE_UART_READING;
			break;
		case E_IEC_STATE_UART_READING:
			if( len != 0){
				timeSinceLastSending += UARTreadInterval;

				if(timeSinceLastSending >= SendingInterval && (UARTbufHeadIndex != UARTbufTailIndex))
				{
					AmmeterApp_SendDataToCenter(AmmeterApp_UARTbuf + UARTbufHeadIndex, UARTbufTailIndex - UARTbufHeadIndex);

					timeSinceLastSending = 0;
					UARTbufHeadIndex = UARTbufTailIndex;
				}
				 break;
			}
			else {
				 IEC_info.state = E_IEC_STATE_UART_FINISHED;
				 // no break for falling through			
			}
		case E_IEC_STATE_UART_FINISHED:
			{

				if (UARTbufHeadIndex != UARTbufTailIndex)
					AmmeterApp_SendDataToCenter(AmmeterApp_UARTbuf + UARTbufHeadIndex, UARTbufTailIndex - UARTbufHeadIndex);



				//IEC_ChangeBaudRate(AMMETER_APP_PORT, &IEC_info);
				
				IEC_info.state = E_IEC_STATE_UART_IDLE;
				UARTbufTailIndex = 0;				
				UARTbufHeadIndex = 0;
				timeSinceLastSending = 0;
	
				//return events ^ AMMETERAPP_UART_READ2_EVT;				
			}
			break;
		default:
			break;
		}
*/
		if(ZSuccess != osal_start_timerEx(AmmeterApp_TaskID, AMMETERAPP_UART_READ2_EVT, UARTreadInterval))
		{
			SystemReset();
		}
		return events ^ AMMETERAPP_UART_READ2_EVT;

	}
#endif

	return ( 0 );  // Discard unknown events.
}


/*********************************************************************
* @fn      AmmeterApp_ProcessMSGCmd
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
void AmmeterApp_ProcessMSGCmd( afIncomingMSGPacket_t *pkt )
{
	int8 rssi = INVALIDLINKQUAL;
	CONV_LQI_TO_RSSI(rssi, pkt->LinkQuality);
	switch ( pkt->clusterId )
	{
	case AMMETERAPP_CLUSTERID:
		{			
			//Put to dataBuf and prepare to write to uart.
			APPWrapper_t* AppPkt = ( APPWrapper_t* ) pkt->cmd.Data;
			if(AppPkt->app_flag == AMMETER_DATA)
			{
				AmmeterApp_ProcessDataLED();
			}
			AmmeterApp_ParseData(pkt->cmd.Data, pkt->cmd.DataLength);

			break;
		}
	case AMMETERAPP_NWK_CLUSTERID:
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
					param.cluster_id = AMMETERAPP_NWK_CLUSTERID;
					param.radius = 0x01;
					MAC_UTIL_BuildandSendDataPAN(&param, (uint8 *)&rtRsp,
						sizeof(Nwk_RouteRsp_t), MAC_UTIL_UNICAST, 0, 0);

					param.panID = 0xFFFF;
					param.cluster_id = AMMETERAPP_NWK_CLUSTERID;
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
							pREList[i].expiryTime = AMMETER_ROUTE_EXPIRECNT;
							IsRouteExist = true;
							break;
						}
					}
					if (!IsRouteExist)
					{
						pREList[reListIndex].dstAddress = pRRsp->srcAddress;
						pREList[reListIndex].nextHopAddress = pRRsp->nodeID;
						pREList[reListIndex++].expiryTime = AMMETER_ROUTE_EXPIRECNT;
						reValidChild++;
					}

					if (pRRsp->dstAddress != MyInfo.nodeID)
					{
						MacParam_t param;
						param.panID = MyInfo.parentID;
						param.cluster_id = AMMETERAPP_NWK_CLUSTERID;
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
					/*
					MyInfo.needRepair = false;
					MyInfo.parentID = pRAlive->nodeID;
					MyInfo.nodeID = zgConfigPANID;
					MyInfo.nodeDepth = pRAlive->nodeDepth + 1;
					MyInfo.sinkID = pRAlive->srcAddress; //Do we need fixed sink ID.
					*/

					Nwk_RouteRsp_t rtRsp;
					rtRsp.msgtype = ROUTERSP;

					//rtRsp.dstAddress = pRAlive->srcAddress;
					rtRsp.dstAddress = MyInfo.sinkID;
					rtRsp.seq = pRAlive->seq;
					rtRsp.srcAddress = MyInfo.nodeID;
					rtRsp.nodeID = MyInfo.nodeID;
					rtRsp.nodeDepth = MyInfo.nodeDepth;

					param.panID = MyInfo.parentID;
					param.cluster_id = AMMETERAPP_NWK_CLUSTERID;
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
					param.cluster_id = AMMETERAPP_NWK_CLUSTERID;
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

/*********************************************************************
* @fn      AmmeterApp_SendData
*
* @brief   Send data OTA.
*
* @param   none
*
* @return  none
*/
void AmmeterApp_SendDataToCenter( uint8 *buf, uint8 len )
{
	uint8 pktsize = MAX_DATA_SIZE - sizeof(app_amdata_t);
	txpkt.packnum = len%pktsize == 0 ? len/pktsize : (len/pktsize + 1);
	txpkt.packseq = 1;
	txpkt.len = len < pktsize ? len : pktsize;
	AmmeterApp_BuildData(buf, txpkt.len);
	if(ZSuccess != osal_set_event(AmmeterApp_TaskID, AMMETERAPP_MSG_SEND_EVT))
	{
		SystemReset();
	}
}

#ifdef CFG_AMMETER_CMDLINE
void AmmeterApp_SendDataCmdToCenter( uint8 *buf, uint8 len )
{
	uint8 pktsize = MAX_DATA_SIZE - sizeof(app_amdata_t);
	txpkt.packnum = len%pktsize == 0 ? len/pktsize : (len/pktsize + 1);
	txpkt.packseq = 1;
	txpkt.len = len < pktsize ? len : pktsize;
	AmmeterApp_BuildData(buf, txpkt.len);
	((app_amdata_t*)txBuf)->msgtype = AMMETER_DATA_CMDLINE;
	
	if(ZSuccess != osal_set_event(AmmeterApp_TaskID, AMMETERAPP_MSG_SEND_EVT))
	{
		SystemReset();
	}
}
#endif

#ifdef CFG_AMMETER_IEC

void AmmeterApp_SendDataLastToCenter( uint8 *buf, uint8 len )
{
	uint8 pktsize = MAX_DATA_SIZE - sizeof(app_amdata_t);
	txpkt.packnum = len%pktsize == 0 ? len/pktsize : (len/pktsize + 1);
	txpkt.packseq = 1;
	txpkt.len = len < pktsize ? len : pktsize;
	AmmeterApp_BuildDataLast(buf, txpkt.len);
	if(ZSuccess != osal_set_event(AmmeterApp_TaskID, AMMETERAPP_MSG_SEND_EVT))
	{
		SystemReset();
	}
}
#endif
/*********************************************************************
* @fn      AmmeterApp_BuildData
*
* @brief   Build data OTA.
*
* @param   none
*
* @return  none
*/
void AmmeterApp_BuildData( uint8 *buf, uint8 len)
{
	app_amdata_t app_amdata;
	app_amdata.msgtype = AMMETER_DATA;
	app_amdata.srcPan = MyInfo.nodeID;
	app_amdata.dstPan = MyInfo.sinkID;
	app_amdata.seqnum = txpkt.seqnum;
	app_amdata.packnum = txpkt.packnum;
	app_amdata.packseq = txpkt.packseq;
	app_amdata.len = len;
	osal_memcpy(txBuf, (void *)&app_amdata, sizeof(app_amdata_t));
	osal_memcpy(txBuf+sizeof(app_amdata_t), buf, len);
	txLen = sizeof(app_amdata_t) + len;
}

#ifdef CFG_AMMETER_IEC

void AmmeterApp_BuildDataLast( uint8 *buf, uint8 len)
{
	app_amdata_t app_amdata;
	app_amdata.msgtype = AMMETER_DATA_LAST;
	app_amdata.srcPan = MyInfo.nodeID;
	app_amdata.dstPan = MyInfo.sinkID;
	app_amdata.seqnum = txpkt.seqnum;
	app_amdata.packnum = txpkt.packnum;
	app_amdata.packseq = txpkt.packseq;
	app_amdata.len = len;
	osal_memcpy(txBuf, (void *)&app_amdata, sizeof(app_amdata_t));
	osal_memcpy(txBuf+sizeof(app_amdata_t), buf, len);
	txLen = sizeof(app_amdata_t) + len;
}
#endif

void AmmeterApp_SendAmmeterAck( uint16 seqnum, uint8 packnum,uint8 packseq)
{
	app_amack_t app_amack;
	app_amack.msgtype = AMMETER_DATA_ACK;
	app_amack.srcpan = MyInfo.nodeID;
	app_amack.dstpan = MyInfo.sinkID;
	app_amack.seqnum = seqnum;

	app_amack.packnum = packnum;
	app_amack.packseq  = packseq;
	osal_memcpy(txBuf, (uint8 *)&app_amack, sizeof(app_amack_t));
	txLen = sizeof(app_amack_t);
	osal_set_event(AmmeterApp_TaskID, AMMETERAPP_ACK_SEND_EVT);
}

uint8 AmmeterApp_ParseData(uint8* buf, uint8 len)
{
	APPWrapper_t* AppPkt = ( APPWrapper_t* ) buf;

	//Transmit to the other collector.
	if ((AppPkt->app_flag == AMMETER_DATA&&AppPkt->app_amdata.dstPan != MyInfo.nodeID)
		||(AppPkt->app_flag == AMMETER_DATA_ACK&&AppPkt->app_amack.dstpan != MyInfo.nodeID))
	{
		MacParam_t param;
		int16 i = 0;
		APPWrapper_t* appdata = (APPWrapper_t *) buf;
		param.cluster_id = AMMETERAPP_CLUSTERID;
		param.radius = 0x01;
		for(; i < reListIndex; ++i)
		{
			//handle broadcasting
			if (appdata->app_amdata.dstPan == 0xFFFF
				&& pREList[i].expiryTime)
			{
				param.panID = pREList[i].nextHopAddress;
				MAC_UTIL_BuildandSendDataPAN(&param, buf,
					len, MAC_UTIL_BROADCAST, 0, MAC_TXOPTION_ACK);
				continue;
			}
			if (appdata->app_amdata.dstPan == pREList[i].dstAddress
				&& pREList[i].expiryTime)
			{
				param.panID = pREList[i].nextHopAddress;
				MAC_UTIL_BuildandSendDataPAN(&param, buf,
					len, MAC_UTIL_UNICAST, 0, MAC_TXOPTION_ACK);	
				break;
			}
			else if (appdata->app_amdata.srcPan == pREList[i].dstAddress
				&& pREList[i].expiryTime)
			{
				param.panID = MyInfo.parentID;
				MAC_UTIL_BuildandSendDataPAN(&param, buf,
					len, MAC_UTIL_UNICAST, 0, MAC_TXOPTION_ACK);
				break;
			}
		}

#if (defined CFG_AMMETER_IEC || defined CFG_AMMETER_TRANSPARENT)
			if (appdata->app_amdata.dstPan == 0xFFFF)
				AmmeterApp_ProcessTransparentData(buf, false);
#endif
		
		return 0;
	}

	if (AppPkt->app_flag == AMMETER_DATA_ACK)
	{
		if (AppPkt->app_amack.seqnum == txpkt.seqnum) 
		{
			if (txpkt.packnum == 1) // single frame
			{
				AmmeterApp_StateReset();
			}
			else if (txpkt.packnum > 1)  // multi frame
			{
				if (txpkt.packnum == txpkt.packseq) // Last frame
				{
					AmmeterApp_StateReset();
				}
				else if(AppPkt->app_amack.packseq == txpkt.packseq)   
				{
					uint8* buf = dataBuf.p + dataBuf.sendIdx;
					rtryCnt = 0;
					uint8 pktsize = MAX_DATA_SIZE - sizeof(app_amdata_t);
					uint8 len;

					if(++txpkt.packseq == txpkt.packnum)  //last block
					{
						len = dataBuf.idx - (txpkt.packseq-1)*pktsize;
					}
					else
					{
						len = pktsize;
					}

					AmmeterApp_BuildData(buf, len);
					dataBuf.sendIdx +=  len;
					osal_set_event(AmmeterApp_TaskID, AMMETERAPP_MSG_SEND_EVT);
				}
			}
		}
	}
	else if (AppPkt->app_flag == AMMETER_DATA)  // if the data is for me
	{
#ifdef CFG_AMMETER_MODBUS  		
		AmmeterApp_ProcessModbusData(buf);
#endif

#if (defined CFG_AMMETER_IEC || defined CFG_AMMETER_TRANSPARENT)
		AmmeterApp_ProcessTransparentData(buf, true);
#endif
	}
	
#ifdef CFG_AMMETER_CMDLINE	
	else if (AppPkt->app_flag == AMMETER_DATA_CMDLINE)  // if the data is for me
	{
		AmmeterApp_ProcessCmdData(buf);
	}
#endif 
	
	return 0;
}

/*********************************************************************
* @fn      rxCB
*
* @brief   Process UART Rx event handling.
*
* @param   none
*
* @return  none
*/
/*
static void rxCB( uint8 port, uint8 event )
{
uint8 len;
if (IsPortBusy && (event & HAL_UART_RX_TIMEOUT)) //recv the whole data and prepare to tx by rf.
{
len = HalUARTRead(port, dataBuf.p, uartdatalen);
dataBuf.idx = 0;
uint8 pktsize = MAX_DATA_SIZE - sizeof(app_amdata_t);
txpkt.packnum = len%pktsize == 0 ? len/pktsize : (len/pktsize + 1);
txpkt.packseq = 1;
txLen = len < pktsize ? len : pktsize;
txpkt.len = uartdatalen;
AmmeterApp_BuildData(dataBuf.p, txLen);
dataBuf.idx += txLen;
osal_set_event(AmmeterApp_TaskID, AMMETERAPP_MSG_SEND_EVT);
}
}
*/

#ifdef CFG_AMMETER_CMDLINE

static void AmmeterApp_ProcessCmdData(uint8 * data)
{
	APPWrapper_t* AppPkt = ( APPWrapper_t* ) data;
	uint8* AmmeterData = (uint8 *)((uint8 *)&(AppPkt->app_amdata) + sizeof(app_amdata_t)); 

		
#if (defined HAL_UART) && (HAL_UART == TRUE)
	HalUARTWrite(AMMETER_APP_PORT, AmmeterData, AppPkt->app_amdata.len);
#endif

	//AmmeterApp_SendAmmeterAck(AppPkt->app_amdata.seqnum, AppPkt->app_amdata.packnum,AppPkt->app_amdata.packseq);	
}

#endif

#ifdef CFG_AMMETER_IEC

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

#if (defined CFG_AMMETER_IEC || defined CFG_AMMETER_TRANSPARENT) && (defined HAL_UART)
static void AmmeterApp_ProcessTransparentData(uint8 * data, bool needACK)
{
	APPWrapper_t* AppPkt = ( APPWrapper_t* ) data;
	uint8* AmmeterData = (uint8 *)((uint8 *)&(AppPkt->app_amdata) + sizeof(app_amdata_t)); 

#ifdef CFG_AMMETER_IEC
	/* handle baud switch after finish transmitting*/
	IEC_ParseFrame(AmmeterData, AppPkt->app_amdata.len, D_IEC_SERVER, &IEC_info);
#endif
		
#if (defined HAL_UART) && (HAL_UART == TRUE)
	//HalUARTFlushTxBuf(AMMETER_APP_PORT);
	//HalUARTFlushRxBuf(AMMETER_APP_PORT);
	HalUARTWrite(AMMETER_APP_PORT, AmmeterData, AppPkt->app_amdata.len);
#endif

	//uint32 interval = GetBitTransmitInterval(IEC_info.com_baud_rate);

	//osal_stop_timerEx(AmmeterApp_TaskID, AMMETERAPP_UART_READ2_EVT);
	if (needACK)
		AmmeterApp_SendAmmeterAck(AppPkt->app_amdata.seqnum, AppPkt->app_amdata.packnum,AppPkt->app_amdata.packseq);	

}
#endif

#ifdef CFG_AMMETER_MODBUS  		
static void AmmeterApp_ProcessModbusData(uint8 * data)
{	
	APPWrapper_t* AppPkt = ( APPWrapper_t* ) data;
	uint8* AmmeterData = (uint8 *)((uint8 *)&(AppPkt->app_amdata) + sizeof(app_amdata_t));

	
	/* when received a message, reset status after 10s  */
	if (ZSUCCESS != osal_start_timerEx( AmmeterApp_TaskID, AMMETERAPP_PORTBUSY_TIMEOUT_EVT, 10000))
		SystemReset();

	/* send ack first */	

	AmmeterApp_SendAmmeterAck(AppPkt->app_amdata.seqnum, AppPkt->app_amdata.packnum,AppPkt->app_amdata.packseq);

	if (!IsPortBusy)
	{
		if (AppPkt->app_amdata.packnum == 1)  //if  one frame case
		{	
#if (defined HAL_UART) && (HAL_UART == TRUE) 
			//HalUARTFlushTxBuf(AMMETER_APP_PORT);
			//HalUARTFlushRxBuf(AMMETER_APP_PORT);						
			HalUARTWrite(AMMETER_APP_PORT, AmmeterData, AppPkt->app_amdata.len);		
#endif
			AmmeterApp_StateReset();
			AmmeterApp_StartReadAmmeter();
			IsPortBusy = true;

		}
		else if(AppPkt->app_amdata.packnum > 1 && AppPkt->app_amdata.packseq == 1)   // multi frame case and received the first pkt 
		{
			rxpkt.packnum = AppPkt->app_amdata.packnum;
			rxpkt.packseq = AppPkt->app_amdata.packseq;
			rxpkt.seqnum = AppPkt->app_amdata.seqnum;

			osal_memcpy(dataBuf.p, AmmeterData, AppPkt->app_amdata.len);
			dataBuf.idx =  AppPkt->app_amdata.len;
			rxpkt.len =  AppPkt->app_amdata.len;
			IsPortBusy = true;
		}
		else // error pkt
		{
			//AmmeterApp_StateReset();
		}

	}
	else // Multi Frame, 2-max pkts
	{
		if (AppPkt->app_amdata.seqnum == rxpkt.seqnum
			&& rxpkt.packseq + 1 == AppPkt->app_amdata.packseq
			&& rxpkt.packnum==AppPkt->app_amdata.packnum &&  rxpkt.packnum> 1) //Assemble the blk to a pkt and send to ammeter.
		{
			rxpkt.packseq++;
			if(dataBuf.idx + AppPkt->app_amdata.len <= AMMETER_MAX_PKT_LEN)
			{
				osal_memcpy(dataBuf.p+dataBuf.idx, AmmeterData, AppPkt->app_amdata.len);
				dataBuf.idx += AppPkt->app_amdata.len;
				rxpkt.len += AppPkt->app_amdata.len;
			}
			else // data is too long
			{
				AmmeterApp_StateReset();
				return;
			}

			if (AppPkt->app_amdata.packnum == AppPkt->app_amdata.packseq) //last frame
			{
#if (defined HAL_UART) && (HAL_UART == TRUE)
				//HalUARTFlushTxBuf(AMMETER_APP_PORT);
				//HalUARTFlushRxBuf(AMMETER_APP_PORT);						
				HalUARTWrite(AMMETER_APP_PORT, dataBuf.p, rxpkt.len);
#endif
				AmmeterApp_StateReset();
				IsPortBusy = true;            // reset others, portbusy remain
				AmmeterApp_StartReadAmmeter();				
			}		
		}
		else // multi frame, received error pkt
		{
			//AmmeterApp_StateReset();
		}

	}
}
#endif


#ifdef CFG_AMMETERAPP_MODBUS 
bool AmmeterApp_UartSearch(uint8 * pAmmeterID,uint8* pBaudRate)
{
	halUARTCfg_t uartConfig;
	uartConfig.configured           = TRUE;              // 2430 don't care.
	uartConfig.flowControl          = FALSE;
	uartConfig.flowControlThreshold = AMMETER_APP_THRESH;
	uartConfig.rx.maxBufSize        = AMMETER_APP_RX_MAX;
	uartConfig.tx.maxBufSize        = AMMETER_APP_TX_MAX;
	uartConfig.idleTimeout          = AMMETER_APP_IDLE;   // 2430 don't care.
	uartConfig.intEnable            = TRUE;              // 2430 don't care.
	uartConfig.callBackFunc         = NULL;
	for(uint8 i=0;i<4;i++)
	{
		if(i==0)
		{
			uartConfig.baudRate = HAL_UART_BR_9600;
		}
		else if(i==1)
		{
			uartConfig.baudRate = HAL_UART_BR_4800;
		}
		else if(i==2)
		{
			uartConfig.baudRate = HAL_UART_BR_19200;
		}
		else if(i==3)
		{
			uartConfig.baudRate = HAL_UART_BR_38400;
		}
		HalUARTOpen (AMMETER_APP_PORT, &uartConfig);

		static uint8 WriteBuf[8];
		static uint8 ReadBuf[16];

		WriteBuf[1] = 0x03;
		WriteBuf[2] = 0x03;
		WriteBuf[3] = 0xEB;  //   41004->0x03EB  ID register
		WriteBuf[4] = 0x00;
		WriteBuf[5] = 0x01;
		for(uint8 j=1; j<=247;j++)
		{	
			WriteBuf[0] = j;
			if(j==0x11)
			{
				asm("nop");
			}
			uint16 CRC = GetCheckCode(WriteBuf, 6);
			WriteBuf[6] = LO_UINT16(CRC);
			WriteBuf[7] = HI_UINT16(CRC);
			uint16 ReadLen;
			ReadLen = 2*BUILD_UINT16(*((uint8*)WriteBuf + AMMETER_REGCNT_IDX+1), *((uint8*)WriteBuf + AMMETER_REGCNT_IDX));
			ReadLen += 3 + 2;
			HalUARTFlushTxBuf(AMMETER_APP_PORT);
			HalUARTFlushRxBuf(AMMETER_APP_PORT);
			HalUARTWrite(AMMETER_APP_PORT, WriteBuf, 8);
			uint16 ReadTimeout = ReadLen*3/2 + 30;    // read time is 1.5 byte/ms
			if(uartConfig.baudRate == HAL_UART_BR_4800)
			{
				ReadTimeout *=2;
			}
			DelayMs(ReadTimeout);

			uint8 cnt;

			osal_memset(ReadBuf, 0, 16);
			cnt = HalUARTRead(AMMETER_APP_PORT, ReadBuf, 16);
			if(cnt == ReadLen
				&& ReadBuf[1] == 0x03 && ReadBuf[2] == 0x02)    // length is correct,
			{
				*pBaudRate = uartConfig.baudRate;
				*pAmmeterID = BUILD_UINT16(ReadBuf[4], ReadBuf[3]);
				return true;
			}
		}
	}
	return false;
}
#endif


uint16 GetCheckCode(const uint8 * pSendBuf, int8 nEnd)//获得校验码 
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

#if 0
uint16 AmmeterApp_GetTransBitsCnt(uint8* p, bool isRead)
{
	uint16 len;
	uint8 cnt = BUILD_UINT16(*((uint8*)p + AMMETER_REGCNT_IDX+1), *((uint8*)p + AMMETER_REGCNT_IDX));
	if(isRead == TRUE)
	{
		/* write */
		len = 8;

		/* read */
		len += 3 + 2*cnt + 2;
	}
	else
	{
		/* write */
		len = 7 + 2*cnt + 2;

		/*read*/
		len += 8;
	}
	return len;
}
#endif
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

			if (candidateArray[i].counter >= 2*AMMETER_ROUTE_TOLERANCECNT)
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
			if(candidateArray[i].counter > 1.5*AMMETER_ROUTE_TOLERANCECNT)
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
			if(candidateArray[i].counter > 1.5*AMMETER_ROUTE_TOLERANCECNT 
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

static void AmmeterApp_ProcessDataLED(void )
{
	HalLedSet(HAL_LED_DATA,HAL_LED_MODE_OFF);
	HalLedBlink(HAL_LED_DATA, 1, 50, 300);
}

static void AmmeterApp_StateReset(void)
{
	rtryCnt = 0;
	IsPortBusy = false;
	osal_stop_timerEx(AmmeterApp_TaskID, AMMETERAPP_MSG_SEND_EVT);
	osal_unset_event(AmmeterApp_TaskID, AMMETERAPP_MSG_SEND_EVT);
	txpkt.seqnum++;
	txpkt.packseq = 0;
	txpkt.len = 0;
	txpkt.packnum = 0;
	dataBuf.idx = 0;
	dataBuf.sendIdx = 0;
}
static void AmmeterApp_StartReadAmmeter(void)
{
	dataBuf.idx = 0;
	UartIsReading = false;
	UartReadingCnt = 0;

	if(ZSuccess!=osal_start_timerEx(AmmeterApp_TaskID, AMMETERAPP_UART_READ_EVT, AMMETER_UART_READINTERVAL))
	{
		SystemReset();
	}
}


#ifdef DEBUG_REPORT_NW
static void ReportRoute(uint8 *data, uint8 len)
{
	MacParam_t param;

	if (MyInfo.sinkID == 0) return;

	param.panID = MyInfo.parentID;
	param.cluster_id = AMMETERAPP_NWK_CLUSTERID;
	param.radius = 0x01;

	txBuf[0] = ROUTEREPORT;
	txBuf[1] = (MyInfo.nodeID & 0xFF00) >> 8;
	txBuf[2] = MyInfo.nodeID & 0xFF;

	osal_memcpy(txBuf+3, data, len);
	txLen = 3+len;

	MAC_UTIL_BuildandSendDataPAN(&param, txBuf,
		txLen, MAC_UTIL_UNICAST, 0, MAC_TXOPTION_ACK);	

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
		int16 datalen = 0;
		
		p = strtok(NULL," "); 
		if(!p) return;

		datalen =  osal_strlen(p);
		
		uint8 pktsize = MAX_DATA_SIZE - sizeof(app_amdata_t);

		if (datalen > pktsize) datalen = pktsize;
		if (datalen > 0)
		{
			AmmeterApp_SendDataCmdToCenter((uint8*)p, datalen);
		}
	}
}

#endif

#endif //DEBUG_REPORT_NW 
/*********************************************************************
*********************************************************************/

