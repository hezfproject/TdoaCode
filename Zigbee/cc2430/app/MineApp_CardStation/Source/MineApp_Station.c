/**************************************************************************************************
Filename:       MineApp_Station.c
Revised:        $Date: 2011/07/26 23:45:33 $
Revision:       $Revision: 1.1 $

Description:    Mine Application of station.
**************************************************************************************************/
/*********************************************************************
* INCLUDES
*/
#include "stdio.h"
#include "string.h"
#include "MineApp_Station.h"
#include "AppProtocolWrapper.h"
#include "MacUtil.h"
#include "App_cfg.h"
#include "Mac_radio_defs.h"
#include "Mac_spec.h"

#include "Rtg.h"
#include "AssocList.h"
#include "AddrMgr.h"
#include "CRC.h"

#if !defined( NONWK )
#include "ZDApp.h"
#include "ZDObject.h"
#include "ZDProfile.h"
#include "ZGlobals.h"
#endif

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

//#if(defined WATCHDOG) && (WATCHDOG==TRUE)
#include "WatchdogUtil.h"
//#endif
#if(defined USE_RF_DEBUG)
#include "mac_pib.h"
#include "mac_low_level.h"
#endif

/*********************************************************************
* GLOBAL VARIABLES
*/

#define MINEAPP_URGENT_NODATA                0
#define MINEAPP_URGENT_RETREAT               0x5A
#define MINEAPP_URGENT_CANCELRETREAT   		 0x68

#define INVALID_ADDR INVALID_NODE_ADDR // for pre- allocate nwkaddr, get from function debug.
#define RSSI_MIN      (MAC_RADIO_RECEIVER_SENSITIVITY_DBM + MAC_SPEC_ED_MIN_DBM_ABOVE_RECEIVER_SENSITIVITY)
#define RSSI_MAX    MAC_RADIO_RECEIVER_SATURATION_DBM

#define CONV_LQI_TO_RSSI( rssi,lqi ) \
	st (   \
	rssi = lqi*(RSSI_MAX - RSSI_MIN)/MAC_SPEC_ED_MAX + RSSI_MIN; \
	)

#define MINEAPP_PARAM_ADDR 			0xFEF0

#define MINEAPP_RFREPORT_LEN  (MINEAPP_MAX_DATA_LEN - sizeof(SPIMSGPacket_t))
#define MINEAPP_ADDRLIST_MAXHEALTH    4     //  tolerance  time is MAXHEALTH * 6s = 24s
/*********************************************************************
* TYPEDEFS
*/
typedef struct
{
	uint8  ResetFlag;
} storeParam_t;

#ifdef USE_SPI_DEBUG
#define SPI_DEBUG_BUFLEN 32
typedef struct
{	
	uint16* txBuf;
	uint16* rxBuf;
	uint8 txIdx;
	uint8 rxIdx;
	uint32 txNum;
	uint32 rxNum;
}spi_debug_t;
#endif

typedef struct
{
	uint8   rfmacseq;
	bool    rfmacSetFinish;
	bool    IsrfmacReceving;
}spi_rfmac_t;

typedef struct
{
	uint8 cnt;
	bool flag;
} NodeAdjustInfo_t;

typedef struct
{
	uint16 Locnum;
	bool flag;
} LocNodeStatistic_t;

typedef struct
{
	app_rfreport_t* pBuf;
	uint8 addrListIdx;
}RFReport_t;

typedef struct 
{
	uint8* pHealth;	
}AssociatedDevList_Info_t;

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
	GASMONITOR_CLUSTERID,
	CHARGEED_CLUSTERID,
};
static const cId_t MineApp_OutputClusterList[] =
{
	MINEAPP_CLUSTERID,
	CARD_CLUSTERID,
	GASMONITOR_CLUSTERID,
	CHARGEED_CLUSTERID,
};

static const SimpleDescriptionFormat_t MineApp_SimpleDesc =
{
	MINEAPP_ENDPOINT,
	MINEAPP_PROFID,

	MINEAPP_DEVICEID,

	MINEAPP_VERSION,
	MINEAPP_FLAGS,

	sizeof ( MineApp_InputClusterList ),
	( cId_t* ) MineApp_InputClusterList,

	sizeof ( MineApp_OutputClusterList ),
	( cId_t* ) MineApp_OutputClusterList
};

static const endPointDesc_t MineApp_epDesc =
{
	MINEAPP_ENDPOINT,
	&MineApp_TaskID,
	( SimpleDescriptionFormat_t * ) &MineApp_SimpleDesc,
	noLatencyReqs
};

static devStates_t MineApp_NwkState;
static uint8 MineApp_TransID;
static uint8 MineApp_BlastTransID;
#ifdef USE_ADVBLAST_CHECK
static uint8 MineApp_AdvBlastTransID;
#endif

static byte MineApp_Msg[MINEAPP_MAX_DATA_LEN];
static SPIMSGPacket_t* pSpi = ( SPIMSGPacket_t* ) MineApp_Msg;
static uint8  MineApp_retreat = MINEAPP_URGENT_NODATA;

static uint8 MineApp_RfBuf[MAX_DATA_SIZE];

static uint16  MineApp_SleepPeriod;
static uint8    MineApp_poll_interval;
static NodeAdjustInfo_t MineApp_Sleep_info = {0,FALSE};


__idata static  int8  MineApp_RSSI;
static NodeAddr deviceAddr;
static uint8    MineApp_urgent_timeout_cnt = 0;
static uint8 SPIErrCnt = 0;
static bool IsBlastNormal = false;
static uint8 BlastCnt = 0;

#ifdef USE_ADVBLAST_CHECK
static bool AdvIsBlastNormal = false;
static uint8 AdvBlastCnt = 0;
#endif

static  spi_rfmac_t spi_rfmac={0, false, false};

static uint8    MineApp_seq = 0;
static LocNodeStatistic_t MineApp_LocNodeStatistic = {0, false};
#ifdef USE_SPI_DEBUG
static spi_debug_t spi_debug;
#endif

static RFReport_t MineApp_RFReport;
static AssociatedDevList_Info_t AssociatedDevList_Info;
/*********************************************************************
* LOCAL FUNCTIONS
*/
static byte OSmsg2SPIpkt ( const afIncomingMSGPacket_t* MSGpkt, SPIMSGPacket_t * msg );
static void MineApp_ProcessMSGCB ( const afIncomingMSGPacket_t *MSGpkt );
static void MineApp_ProcessSPICB ( const SPIMSGPacket_t *MSGpkt );
static byte MineApp_ParseCardFrame ( const afIncomingMSGPacket_t* MSGpkt );
static void MineApp_HandleKeys ( uint16 keys, byte shifts );
static byte MineApp_BuildAssocAddr ( app_DeputyJoin_t* appData );
static void MineApp_StartProbeSpiEvt ( void );
static void MineApp_StopProbeSpiEvt ( void );
static uint8  MineApp_SendDataToSpi(uint16 shortAddr,const uint8 *p, uint16 len);
static uint8 MineApp_SendDataToSpiEx(uint16 shortAddr,uint16 len);
static void MineApp_LocSortByRSSI(LocPos_t* LocNode, int8 LastCnt);
#ifdef USE_RF_DEBUG
static void MineApp_RFDebug_SendFrame (app_RFDebug_t *pHeader, uint8 *p, uint16 len, uint8 FrameLen);
static  void MineApp_RFDebug_SendMultiFrame (app_RFDebug_t *pHeader, uint8 *p, uint16 len, uint8 FrameLen);
#endif
static byte MineApp_ParseChargedCardFrame (const afIncomingMSGPacket_t* MSGpkt);
static byte MineApp_ParseMobileFrame(const afIncomingMSGPacket_t* MSGpkt);
static byte MineApp_ParseGasFrame(const afIncomingMSGPacket_t* MSGpkt);

static void MineApp_UpdateAddrListHeadth(uint16 shortAddr);
static void MineApp_CheckAddrListHealth(void );
static void MineApp_AddrListReport(void);
extern __near_func void flashErasePage(uint8, uint8 __xdata *);
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
void MineApp_Init ( uint8 task_id )
{
	MineApp_TaskID = task_id;
	MineApp_NwkState = DEV_INIT;

#if(defined WATCHDOG) && (WATCHDOG==TRUE)
	StartWatchDog ( DOGTIMER_INTERVAL_1S );
	osal_set_event ( MineApp_TaskID, MINEAPP_FEEDWATCHDOG_EVENT );
#endif

#if (defined HAL_SPI) && (HAL_SPI==TRUE)
	halSPICfg_t spiConfig;
	spiConfig.maxBufSize = DMA_RT_MAX;
	spiConfig.intEnable = false;
	HalSpiStart ( &spiConfig );
	RegisterForSpi ( MineApp_TaskID );
#endif

	afRegister ( ( endPointDesc_t * ) &MineApp_epDesc );
	RegisterForKeys ( MineApp_TaskID );

	//construct SPIMSGPacket_t hdr.
	pSpi= ( SPIMSGPacket_t * ) MineApp_Msg;
	pSpi->spihdr.hdr.event = SPI_RX;
	pSpi->spihdr.srcAddr.addrMode = ( AddrMode_t ) Addr16Bit;
	pSpi->spihdr.srcAddr.endPoint = MINEAPP_ENDPOINT;
	pSpi->spihdr.dstAddr.addrMode = ( AddrMode_t ) Addr16Bit;
	pSpi->spihdr.dstAddr.endPoint = MINEAPP_ENDPOINT;
	pSpi->spihdr.transID = INIT_TRANID;
	pSpi->spihdr.options = INIT_OPN;
	pSpi->spihdr.radius = MAX_DEPTH;

	/*alloc rf buffer */

	/* EP and  Profile Id are not used,Card will drop them directly */
	MacUtil_t MacUtil;
	MacUtil.panID = CARD_NWK_ADDR;  /* Communication with Card */
	MacUtil.dst_endpoint = MINEAPP_ENDPOINT;
	MacUtil.src_endpoint = 0x20;//MINEAPP_ENDPOINT;
	MacUtil.profile_id = 0x0100;//MINEAPP_PROFID;
	MacUtil.cluster_id = CARD_CLUSTERID;
	MacUtil.NodeType = ZDO_Config_Node_Descriptor.LogicalType;
	MAC_UTIL_INIT ( &MacUtil );
#ifdef USE_SPI_DEBUG
	spi_debug.txBuf = osal_mem_alloc(sizeof(uint16)*SPI_DEBUG_BUFLEN);
	spi_debug.rxBuf = osal_mem_alloc(sizeof(uint16)*SPI_DEBUG_BUFLEN);
	spi_debug.txIdx = 0; 
	spi_debug.rxIdx = 0; 
	spi_debug.txNum = 0; 
	spi_debug.rxNum = 0; 
#endif

	MineApp_RFReport.pBuf = (app_rfreport_t *) osal_mem_alloc(MINEAPP_RFREPORT_LEN);
	MineApp_RFReport.addrListIdx = 0;
	AssociatedDevList_Info.pHealth = (uint8*)osal_mem_alloc(NWK_MAX_DEVICES + 5);  // add some guard 

#ifdef ZC_REPORT_TO_ARM
	osal_start_timerEx ( MineApp_TaskID, MINEAPP_START_NWK_EVENT, 28000 ); //delay for a minute
#endif
	if ( NO_TIMER_AVAIL == osal_start_timerEx ( MineApp_TaskID, MINEAPP_LOC_BLAST_EVENT, 5000))
	{
		SystemReset();
	}

	if(ZSuccess!= osal_start_timerEx(MineApp_TaskID, MINEAPP_ADDRLIST_HEALTH_EVENT, 6000))
	{
		SystemReset();
	}
	if ( NO_TIMER_AVAIL == osal_start_timerEx ( MineApp_TaskID, MINEAPP_ADDRLIST_REPORT_EVENT, 10000))
	{
		SystemReset();
	}
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

uint16 MineApp_ProcessEvent ( uint8 task_id, uint16 events )
{
	afIncomingMSGPacket_t* MSGpkt;
#if(defined WATCHDOG) && (WATCHDOG==TRUE)
	if ( events & MINEAPP_FEEDWATCHDOG_EVENT )
	{
		osal_start_timerEx ( MineApp_TaskID, MINEAPP_FEEDWATCHDOG_EVENT, 300 );
		FEEDWATCHDOG();
		return events ^ MINEAPP_FEEDWATCHDOG_EVENT;
	}
#endif
	if ( events & SYS_EVENT_MSG )
	{
		MSGpkt = ( afIncomingMSGPacket_t * ) osal_msg_receive ( MineApp_TaskID );
		while ( MSGpkt )
		{
			switch ( MSGpkt->hdr.event )
			{
			case ZDO_STATE_CHANGE:
				{
					MineApp_NwkState = ( devStates_t ) ( MSGpkt->hdr.status );

					if ( MineApp_NwkState == DEV_ZB_COORD )
					{
						app_startNwk_t TryconRSP;
						TryconRSP.msgtype = ZB_START_NETWORK;
						TryconRSP.PANID = zgConfigPANID;
						osal_cpyExtAddr ((void*)TryconRSP.macAddr, ( void * ) aExtendedAddress );
						MineApp_SendDataToSpi(0, (uint8*)&TryconRSP, sizeof(TryconRSP));

						uint8 maxFrameRetries = 3;
						MAC_MlmeSetReq ( MAC_MAX_FRAME_RETRIES, &maxFrameRetries );

						if ( NO_TIMER_AVAIL == osal_start_timerEx ( MineApp_TaskID, MINEAPP_STATUS_REPORT_EVENT, 2000 ) )
						{
							SystemReset();
						}
						osal_set_event ( MineApp_TaskID, MINEAPP_SIGNALSTRENGTH_EVENT );
						if ( NO_TIMER_AVAIL == osal_start_timerEx ( MineApp_TaskID, MINEAPP_DETECT_BLAST_EVENT, 65000 ) )
						{
							SystemReset();
						}
					}
					else
					{
						storeParam_t param = * ( storeParam_t* ) MINEAPP_PARAM_ADDR;
						param.ResetFlag = ZC_REPORT_STARTNWK_FAILED_RESTART;
						* ( storeParam_t* ) MINEAPP_PARAM_ADDR =  param;
						if ( NO_TIMER_AVAIL == osal_start_timerEx ( MineApp_TaskID, MINEAPP_WDG_RESTART_EVENT, 1000 ) )
						{
							SystemReset();
						}
					}

					if ( NO_TIMER_AVAIL == osal_start_timerEx ( MineApp_TaskID, MINEAPP_DETECT_SPI_EVENT, 65000 ) )
					{
						SystemReset();
					}
					if ( NO_TIMER_AVAIL == osal_start_timerEx ( MineApp_TaskID, MINEAPP_LOC_STATISTIC_EVENT, 5000))
					{
						SystemReset();
					}
					break;

				}
			case KEY_CHANGE:
				{
					if ( MineApp_NwkState != DEV_INIT )
					{
						MineApp_HandleKeys ( ( ( keyChange_t * ) MSGpkt )->keys, ( ( keyChange_t * ) MSGpkt )->state );
					}
					break;
				}
			case AF_INCOMING_MSG_CMD:
				{
					MineApp_ProcessMSGCB ( MSGpkt );
					break;
				}
			case AF_DATA_CONFIRM_CMD:
				{
					if ( ( ( afDataConfirm_t * ) MSGpkt )->transID == MineApp_BlastTransID)
					{
						if ( ( ( afDataConfirm_t * ) MSGpkt )->hdr.status == ZSUCCESS
							&& IsBlastNormal == false )
							IsBlastNormal = true;
					}
#ifdef USE_ADVBLAST_CHECK
					else if ( ( ( afDataConfirm_t * ) MSGpkt )->transID == MineApp_AdvBlastTransID )
					{
						if ( ( ( afDataConfirm_t * ) MSGpkt )->hdr.status == ZSUCCESS
							&& AdvIsBlastNormal == false )
							AdvIsBlastNormal = true;
					}
#endif
					break;
				}

#if (defined HAL_SPI) && (HAL_SPI == TRUE)
			case SPI_RX: //I'm coordinator, just dispatch the packet.
				{
					MineApp_ProcessSPICB ( ( SPIMSGPacket_t * ) MSGpkt );
					break;
				}
#endif
			default:
				break;
			}
			osal_msg_deallocate ( ( uint8 * ) MSGpkt );
			MSGpkt = ( afIncomingMSGPacket_t * ) osal_msg_receive ( MineApp_TaskID );
		}
		return ( events ^ SYS_EVENT_MSG );
	}
	if ( events & MINEAPP_URGENT_TIME_EVENT )
	{
		if ( MineApp_urgent_timeout_cnt < MINEAPP_URGENT_TIMEOUT )
		{
			osal_start_timerEx ( MineApp_TaskID, MINEAPP_URGENT_TIME_EVENT, 60000 ); //delay for a minute
			MineApp_urgent_timeout_cnt++;
		}
		else
		{
			MineApp_retreat = MINEAPP_URGENT_NODATA;
			MineApp_urgent_timeout_cnt = 0;
		}
		return ( events ^ MINEAPP_URGENT_TIME_EVENT );
	}

	if ( events & MINEAPP_SENDCMD_EVENT )
	{
		if ( MineApp_Sleep_info.cnt < MINEAPP_SENDCMD_TIMEOUT )
		{
			osal_start_timerEx ( MineApp_TaskID, MINEAPP_SENDCMD_EVENT, 60000 ); //delay for a minute
			MineApp_Sleep_info.cnt++;
		}
		else
		{
			MineApp_Sleep_info.flag=  false;
			//MineApp_Sleep_info.cnt = 0;   // do not clear here
		}

		return ( events ^ MINEAPP_SENDCMD_EVENT );
	}
	if ( events & MINEAPP_SIGNALSTRENGTH_EVENT )
	{
		if ( NO_TIMER_AVAIL == osal_start_timerEx ( MineApp_TaskID,MINEAPP_SIGNALSTRENGTH_EVENT, BLAST_SIGNALSTRENGTH_PERIOD ) )
		{
			SystemReset();
		}

		afAddrType_t dstAddr;
		dstAddr.addrMode = afAddrBroadcast;
		dstAddr.addr.shortAddr = NWK_BROADCAST_SHORTADDR_DEVALL;
		dstAddr.endPoint = MINEAPP_ENDPOINT;

		app_SignalStrength_t SignalStrength;
		SignalStrength.msgtype = SIGNAL_STRENGTH;
		SignalStrength.len = ( MAX_DATA_SIZE - sizeof ( app_SignalStrength_t ) );
		osal_memcpy ( MineApp_RfBuf, &SignalStrength, sizeof ( SignalStrength ) );

		MineApp_BlastTransID = MineApp_TransID;
		AF_DataRequest ( &dstAddr, ( endPointDesc_t * ) &MineApp_epDesc,
			MINEAPP_CLUSTERID, MAX_DATA_SIZE, ( uint8 * ) MineApp_RfBuf, &MineApp_TransID, INIT_OPN,1 );

#ifdef 	USE_ADVBLAST_CHECK
		MineApp_AdvBlastTransID = MineApp_TransID;
		AF_DataRequest ( &dstAddr, ( endPointDesc_t * ) &MineApp_epDesc,
			MINEAPP_CLUSTERID, 1, ( uint8 * ) MineApp_RfBuf, &MineApp_TransID, INIT_OPN,1 );
#endif

		return ( events ^ MINEAPP_SIGNALSTRENGTH_EVENT );
	}

	if (events & MINEAPP_LOC_BLAST_EVENT)
	{
		if ( NO_TIMER_AVAIL == osal_start_timerEx ( MineApp_TaskID, MINEAPP_LOC_BLAST_EVENT, 2000))
		{
			SystemReset();
		}
		if(MineApp_NwkState == DEV_ZB_COORD )
		{
			app_LocNodeCast_t app_LocNodeCast;
			app_LocNodeCast.msgtype = LOCNODE_CAST;
			app_LocNodeCast.vol = 0xFF;
			app_LocNodeCast.seq = MineApp_seq++;
			app_LocNodeCast.DevID = zgConfigPANID;
			MacParam_t param;
			param.panID = 0xFFFF;
			param.cluster_id = GASMONITOR_CLUSTERID;
			param.radius = 0x01;
			MAC_UTIL_BuildandSendDataPAN(&param, ( uint8 * ) &app_LocNodeCast,
				sizeof (app_LocNodeCast_t), MAC_UTIL_BROADCAST, MAC_UTIL_BROADCAST_SHORTADDR_DEVZCZR, 0);	
		}
		return events ^ MINEAPP_LOC_BLAST_EVENT;
	}


#ifdef ZC_REPORT_TO_ARM
	if ( events & MINEAPP_STATUS_REPORT_EVENT )
	{
#if (defined HAL_SPI) && (HAL_SPI == TRUE)
		app_ZC_Report_t zc_report;
		zc_report.msgtype = ZC_REPORT;
		zc_report.PanId = zgConfigPANID;

		/* get reset flag and send report */
		storeParam_t param = * ( storeParam_t* ) MINEAPP_PARAM_ADDR;
		switch ( GetResetFlag() )
		{
		case RESET_FLAG_WATCHDOG:
			{
				zc_report.flag  = param.ResetFlag;
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
			MineApp_SendDataToSpi(0, (uint8*)&zc_report, sizeof(zc_report));			
		}
		param.ResetFlag = ZC_REPORT_WATCHDOG_RESTART;  // clear restart  flag
		* ( storeParam_t* ) MINEAPP_PARAM_ADDR = param;
#endif
		return events ^ MINEAPP_STATUS_REPORT_EVENT;
	}
#endif

	if ( events & MINEAPP_START_NWK_EVENT )
	{
		app_rfmac_query_t   rfmac_query;
		rfmac_query.msgtype=RFMAC_QUERY;
		rfmac_query.seq=spi_rfmac.rfmacseq++;

		if(spi_rfmac.rfmacSetFinish)
		{
			ZDApp_StartUpFromApp ( APP_STARTUP_COORD );
		}
		else
		{
			//send req in order to get aExtendedAddress
			MineApp_SendDataToSpi(0, (uint8*)&rfmac_query, sizeof(rfmac_query));
			spi_rfmac.IsrfmacReceving= true;
			osal_start_timerEx(MineApp_TaskID, MINEAPP_START_NWK_EVENT, 3000);
		}
		return events ^ MINEAPP_START_NWK_EVENT;
	}

	if ( events & MINEAPP_WDG_RESTART_EVENT )
	{
		SystemReset();
	}

	if ( events & MINEAPP_DETECT_SPI_EVENT )
	{
		if ( NO_TIMER_AVAIL == osal_start_timerEx ( MineApp_TaskID, MINEAPP_DETECT_SPI_EVENT, 10000 ) )
		{
			SystemReset();
		}
		app_SPIDetect_t spi_report;
		spi_report.msgtype = SPI_DETECT;

		for ( uint8 i =0; i< SPIDETECT_LEN; i++ )
		{
			spi_report.detectdata[i] = i;
		}
		MineApp_SendDataToSpi(0, (uint8*)&spi_report, sizeof(spi_report));
		MineApp_StartProbeSpiEvt();

#ifdef USE_SPI_DEBUG
		spi_debug.txNum++;
		spi_debug.txBuf[spi_debug.txIdx] = spi_seqnum;

		if(++spi_debug.txIdx >= SPI_DEBUG_BUFLEN)
		{
			spi_debug.txIdx = 0;
		}
#endif
		return events ^ MINEAPP_DETECT_SPI_EVENT;
	}

	if ( events & MINEAPP_PROBE_SPI_EVENT )
	{
		SPIErrCnt++;
		if ( SPIErrCnt >= MINEAPP_SPIERR_TIMEOUT )
		{
			uint8 *p1, *p2;
			storeParam_t param = * ( storeParam_t* ) MINEAPP_PARAM_ADDR;
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
			* ( storeParam_t* ) MINEAPP_PARAM_ADDR =  param;
			osal_mem_free ( p1 );
			osal_mem_free ( p2 );

			if ( NO_TIMER_AVAIL == osal_start_timerEx ( MineApp_TaskID, MINEAPP_WDG_RESTART_EVENT, 1000 ) )
			{
				SystemReset	();
			}
			SPIErrCnt = 0;
		}
		return ( events ^ MINEAPP_PROBE_SPI_EVENT );
	}

	if ( events & MINEAPP_DETECT_BLAST_EVENT )
	{
		if ( true == IsBlastNormal )
		{
			BlastCnt = 0;
			IsBlastNormal = false;
		}
		else
		{
			BlastCnt++;
		}

#ifdef USE_ADVBLAST_CHECK
		if ( true == AdvIsBlastNormal )
		{
			AdvBlastCnt = 0;
			AdvIsBlastNormal = false;
		}
		else
		{
			AdvBlastCnt++;
		}
#endif
		if ( BlastCnt >= MINEAPP_BLAST_TIMEOUT )
		{
			uint8 *p1, *p2;
			storeParam_t param = * ( storeParam_t* ) MINEAPP_PARAM_ADDR;
			p1 = osal_mem_alloc ( 8 ); //small size memory
			p2 = osal_mem_alloc ( 64 ); //big size memory
			if ( p1==NULL || p2==NULL )
			{
				param.ResetFlag = ZC_REPORT_MEMORY_ERR_RESTART;
			}
			else
			{
#ifdef USE_ADVBLAST_CHECK
				if ( AdvBlastCnt>=MINEAPP_BLAST_TIMEOUT/2 )
				{
					param.ResetFlag = ZC_REPORT_BLAST_ERR_RESTART;
				}
				else
				{
					param.ResetFlag = ZC_REPORT_PARTBLAST_ERR_RESTART;
				}
#else
				param.ResetFlag = ZC_REPORT_BLAST_ERR_RESTART;
#endif
			}
			* ( storeParam_t* ) MINEAPP_PARAM_ADDR =  param;
			osal_mem_free ( p1 );
			osal_mem_free ( p2 );

			if ( NO_TIMER_AVAIL == osal_start_timerEx ( MineApp_TaskID, MINEAPP_WDG_RESTART_EVENT, 1000 ) )
			{
				SystemReset();
			}
			BlastCnt = 0;
			//IsBlastNormal = true;
		}

		if ( NO_TIMER_AVAIL == osal_start_timerEx ( MineApp_TaskID, MINEAPP_DETECT_BLAST_EVENT, 30000 ) )
		{
			SystemReset	();
		}
		return ( events ^ MINEAPP_DETECT_BLAST_EVENT );
	}

	if (events & MINEAPP_LOC_STATISTIC_EVENT)
	{	
		if (!MineApp_LocNodeStatistic.flag)
		{
			static uint16 seqnum;
			app_locnode_info_req_t app_locnode_info_req;
			app_locnode_info_req.msgtype = LOCNODE_INFO_REQ;
			app_locnode_info_req.DevID = _NIB.nwkPanId;
			app_locnode_info_req.seqnum = seqnum++;

			MineApp_SendDataToSpi(0, (uint8 *)&app_locnode_info_req, sizeof(app_locnode_info_req));

			if ( NO_TIMER_AVAIL == osal_start_timerEx(MineApp_TaskID, MINEAPP_LOC_STATISTIC_EVENT, 65000))
			{
				SystemReset();
			}	
		}
		return events ^ MINEAPP_LOC_STATISTIC_EVENT;
	}
	if (events & MINEAPP_ADDRLIST_HEALTH_EVENT)
	{
		/* theck health every 6 seconds */
		MineApp_CheckAddrListHealth();
		if(ZSuccess!= osal_start_timerEx(MineApp_TaskID, MINEAPP_ADDRLIST_HEALTH_EVENT, 6000))
		{
			SystemReset();
		}
		return (events ^ MINEAPP_ADDRLIST_HEALTH_EVENT);		
	}
	if (events & MINEAPP_ADDRLIST_REPORT_EVENT)
	{
		MineApp_AddrListReport();
		return events ^ MINEAPP_ADDRLIST_REPORT_EVENT;
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
void MineApp_ProcessMSGCB ( const afIncomingMSGPacket_t *MSGpkt )
{
	switch ( MSGpkt->clusterId )
	{
	case MINEAPP_CLUSTERID:
		{
			MineApp_ParseMobileFrame(MSGpkt);
			break;
		}
	case CARD_CLUSTERID:
		{
			HalLedSet(HAL_LED_1, HAL_LED_MODE_TOGGLE);
			MineApp_ParseCardFrame ( MSGpkt );
			break;
		}

	case CHARGEED_CLUSTERID:
		{
			HalLedSet (HAL_LED_1, HAL_LED_MODE_TOGGLE);
			MineApp_ParseChargedCardFrame(MSGpkt);
			break;
		}
	case GASMONITOR_CLUSTERID:
		{
			MineApp_ParseGasFrame(MSGpkt);
			break;
		}
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
void MineApp_ProcessSPICB ( const SPIMSGPacket_t *MSGpkt )
{
	SPIMSGPacket_t * rxmsg = ( SPIMSGPacket_t * ) MSGpkt;
	APPWrapper_t* appwrapper = ( APPWrapper_t* ) ( rxmsg+1 );

	//handle differernt SPI msg.
	HalLedSet ( HAL_LED_1, HAL_LED_MODE_TOGGLE );

	/* Message to Phone*/
	if ( rxmsg->spihdr.dstAddr.addr.shortAddr != 0 )
	{
		if ( appwrapper->app_flag == VOICE)
		{
			/*
			MAC_UTIL_BuildandSendDataPAN ( &macparam, ( uint8 * ) ( rxmsg+1 ), rxmsg->DataLength,
			MAC_UTIL_UNICAST, rxmsg->spihdr.dstAddr.addr.shortAddr, NULL );
			*/
			AF_DataRequest (( afAddrType_t * ) & ( rxmsg->spihdr.dstAddr ), ( endPointDesc_t * ) &MineApp_epDesc,
				MINEAPP_CLUSTERID, rxmsg->DataLength, ( byte * ) ( rxmsg+1 ),
				&MineApp_TransID, INIT_OPN, 0x01 );
		}
		else
		{
			AF_DataRequest ( ( afAddrType_t * ) & ( rxmsg->spihdr.dstAddr ), ( endPointDesc_t * ) &MineApp_epDesc,
				MINEAPP_CLUSTERID, rxmsg->DataLength, ( byte * ) ( rxmsg+1 ),
				&MineApp_TransID, rxmsg->spihdr.options, 0x01 );
		}
	}
	/*Message to Phone | Card | Coord*/
	else
	{      
		if(appwrapper->app_flag==RFMAC_SET && spi_rfmac.IsrfmacReceving==true)
		{
			if(CRC16(appwrapper->app_rfmac_set.macAddr, 8, 0xFFFF)==appwrapper->app_rfmac_set.crc
				&& appwrapper->app_rfmac_set.macAddr[EXT_MACADDR_TYPE] == EXT_MACADDR_TYPE_SUBSTATION)
			{
				osal_memcpy(aExtendedAddress, appwrapper->app_rfmac_set.macAddr, 8);
                uint8 oldExtendedAddress[Z_EXTADDR_LEN];
                if(ZSuccess != osal_nv_read( ZCD_NV_EXTADDR, 0, Z_EXTADDR_LEN, oldExtendedAddress))
                {
                    SystemReset();
                }

                /* if getted address is not equal to my address, save it to NV and reset */
                /* will erase the last 2k page, be sure the code will not use it !!! */
                if(memcmp(oldExtendedAddress,aExtendedAddress,Z_EXTADDR_LEN)!=0)
                {
                    uint8 buffer[16];
                    flashErasePage(63, buffer);
                    osal_nv_write( ZCD_NV_EXTADDR, 0, Z_EXTADDR_LEN, aExtendedAddress);
                    SystemReset();
                }
				zgConfigPANID = BUILD_UINT16(aExtendedAddress[EXT_MACADDR_DEVID_LBYTE],
					aExtendedAddress[EXT_MACADDR_DEVID_HBYTE]);
				zgDefaultChannelList = (uint32)((uint32)1 << aExtendedAddress[EXT_MACADDR_CHANNEL]);

				spi_rfmac.rfmacSetFinish=true;
				spi_rfmac.IsrfmacReceving=false;	
			}
		}
        else if ( appwrapper->app_flag == CARD_SPI_CMD )
		{
            app_spicmd_t*  app_spicmd = (app_spicmd_t*)appwrapper;
            if( (app_spicmd->cmdtype == SPI_RETREAT ||app_spicmd->cmdtype == SPI_CANCEL_RETREAT)
                    && CRC16((uint8*)app_spicmd, sizeof(app_spicmd_t)-2, 0xFFFF) == app_spicmd->crc)
            {
			MineApp_urgent_timeout_cnt = 0;
			osal_set_event ( MineApp_TaskID, MINEAPP_URGENT_TIME_EVENT );

                app_spicmd_t spicmd;
                spicmd.msgtype = CARD_SPI_CMD;
                if ( app_spicmd->cmdtype == SPI_RETREAT )
			{
				MineApp_retreat = MINEAPP_URGENT_RETREAT;
                    spicmd.cmdtype =SPI_RETREAT_ACK;
			}
                else if ( app_spicmd->cmdtype == SPI_CANCEL_RETREAT )
			{
				MineApp_retreat = MINEAPP_URGENT_CANCELRETREAT;
                    spicmd.cmdtype = SPI_CANCEL_RETREAT_ACK;
			}
                spicmd.crc = CRC16((uint8*)&spicmd, sizeof(spicmd)-2, 0xFFFF);
                MineApp_SendDataToSpi(0,(uint8*)&spicmd, sizeof(spicmd) );
            }

		}
		else if ( appwrapper->app_flag == NODESLEEP )
		{
#if 0
			MineApp_SleepPeriod = appwrapper->app_Sleep.sleeptime;
			MineApp_poll_interval= appwrapper->app_Sleep.poll_interval;
			MineApp_Sleep_info.flag= true;
			MineApp_Sleep_info.cnt = 0;
			osal_set_event ( MineApp_TaskID, MINEAPP_SENDCMD_EVENT );
#endif
		}
		else if ( appwrapper->app_flag == TRY_CONNECTION )
		{
			//Notify ARM the PANID as its id.
			if ( MineApp_NwkState == DEV_ZB_COORD )
			{
				app_startNwk_t TryconRSP;
				TryconRSP.msgtype = ZB_START_NETWORK;
				TryconRSP.PANID = zgConfigPANID;
				osal_cpyExtAddr ( TryconRSP.macAddr, ( void * ) aExtendedAddress );

				MineApp_SendDataToSpi(0,(uint8*)&TryconRSP, sizeof(TryconRSP));

				uint8 maxFrameRetries = 3;
				MAC_MlmeSetReq ( MAC_MAX_FRAME_RETRIES, &maxFrameRetries );
			}
			else
			{
				osal_set_event(MineApp_TaskID, MINEAPP_START_NWK_EVENT);
			}
		}
		else if ( appwrapper->app_flag == DEPUTY_JOIN )
		{
			byte status = MineApp_BuildAssocAddr ( & ( appwrapper->app_DeputyJoin ) );
			APPWrapper_t* pDeputyRSP = ( APPWrapper_t* ) ( pSpi + 1 );

			if ( status == ZSUCCESS )
			{
				pSpi->spihdr.srcAddr.addr.shortAddr = 0;
				pSpi->DataLength = sizeof ( app_DeputyJoinRsp_t );
				pDeputyRSP->app_DeputyJoinrsp.deputyNmbr = appwrapper->app_DeputyJoin.deputyNmbr;
				pDeputyRSP->app_DeputyJoinrsp.msgtype = DEPUTYJOIN_RSP;
				pDeputyRSP->app_DeputyJoinrsp.rsptype = PERMIT_JOIN;
				pDeputyRSP->app_DeputyJoinrsp.nodeAddr = deviceAddr;
				if (HAL_SPI_SUCCESS != HalSPIWrite ( ( uint8* ) pSpi, pSpi->DataLength + SPIPKTHDR_LEN ))
					NLME_RemoveChild(appwrapper->app_DeputyJoin.macAddr, true);
			}
		}
		else if ( appwrapper->app_flag == TIME_SSIND )
		{
			afAddrType_t dstAddr;
			dstAddr.addrMode = afAddrBroadcast;
			dstAddr.addr.shortAddr = NWK_BROADCAST_SHORTADDR_DEVALL;
			dstAddr.endPoint = MINEAPP_ENDPOINT;

			app_TimessInd_t TimessInd;
			TimessInd = appwrapper->app_TimessInd;
			AF_DataRequest ( &dstAddr, ( endPointDesc_t * ) &MineApp_epDesc,
				MINEAPP_CLUSTERID, sizeof ( app_TimessInd_t ), ( uint8 * ) &TimessInd,
				&MineApp_TransID, INIT_OPN,1 );
		}
		else if ( appwrapper->app_flag == CLEARNWK )
		{
			NLME_RemoveChild ( appwrapper->app_ClearNWK.macAddr, true );

			app_LeaveNwk_t app_LeaveNwk;
			app_LeaveNwk.msgtype = ZB_LEAVE_NOTIFY;
			osal_cpyExtAddr(app_LeaveNwk.macAddr, appwrapper->app_ClearNWK.macAddr);
			app_LeaveNwk.srcnbr = appwrapper->app_ClearNWK.nmbr;
			MineApp_SendDataToSpi(0, (uint8*)&app_LeaveNwk, sizeof(app_LeaveNwk));
		}
		else if ( appwrapper->app_flag == CROSSPAN )
		{
			MacParam_t macparam;
			macparam.radius = 0x01;
			macparam.cluster_id = MINEAPP_CLUSTERID;
			macparam.panID = appwrapper->app_CrossPan.dstPan;
			MAC_UTIL_BuildandSendDataPAN ( &macparam, ( uint8 * ) ( rxmsg+1 ), rxmsg->DataLength,
				MAC_UTIL_UNICAST, 0, 0 );
		}
		else if ( appwrapper->app_flag == SPI_DETECT )
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
				MineApp_StopProbeSpiEvt();
			}
#ifdef USE_SPI_DEBUG
			spi_debug.rxNum++;
			spi_debug.rxBuf[spi_debug.rxIdx] = appwrapper->app_SPIDetect.seqnum;

			if(++spi_debug.rxIdx >= SPI_DEBUG_BUFLEN)
			{
				spi_debug.rxIdx = 0;
			}
#endif
		}
		else if ( appwrapper->app_flag == SPI_DEBUG )
		{
			MineApp_SendDataToSpi(0, ( void * ) appwrapper, sizeof ( app_SPIDebug_t ) + appwrapper->app_SPIDebug.debugstrlen);
		}
		else if (appwrapper->app_flag == LOCNODE_INFO_SET)
		{
			MineApp_LocNodeStatistic.flag = true;
			MineApp_LocNodeStatistic.Locnum = appwrapper->app_locnode_info_set.LocCnt;
		}
		else if(appwrapper->app_flag == RF_VERSION_REQ)
		{

			uint8 * pData =  (uint8* ) ( pSpi+1 ) + sizeof(app_rfversion_t);

			const uint8 maxDataLen = MINEAPP_MAX_DATA_LEN - sizeof(SPIMSGPacket_t) - sizeof(app_rfversion_t);
			sprintf((char*)pData,"SoftVer:%s, BuildTime:%s,%s, Channel:%d",MINEAPP_SW_VERSION,__DATE__,__TIME__,macPib.logicalChannel);

			app_rfversion_t* pHead =  (app_rfversion_t* ) ( pSpi+1 );
			pHead->msgtype = RF_VERSION_RSP;
			pHead->seq = appwrapper->app_rfversion.seq;
			pHead->size = strlen((char *)pData);			
			MineApp_SendDataToSpiEx(0, sizeof(app_rfversion_t) + pHead->size);

		}
		else if( appwrapper->app_flag == GASDEV_SMS)
		{
			MacParam_t macparam;
			macparam.radius = 0x01;
			macparam.cluster_id = MINEAPP_CLUSTERID;
			macparam.panID = appwrapper->app_gassms.dstpan;
			MAC_UTIL_BuildandSendDataPAN ( &macparam, ( uint8 * ) ( rxmsg+1 ), rxmsg->DataLength,
				MAC_UTIL_UNICAST, 0 , 0 );
		}
#ifdef USE_RF_DEBUG
		else if ( appwrapper->app_flag == RF_DEBUG )
		{
			const uint8 FramePayloadLen = MINEAPP_MAX_DATA_LEN - sizeof ( SPIMSGPacket_t ) - sizeof ( app_RFDebug_t );
			app_RFDebug_t *pRFDebugRcv = ( app_RFDebug_t* ) appwrapper;
			app_RFDebug_t *pRFDebugSend = ( app_RFDebug_t * ) ( pSpi+1 );

			pSpi->spihdr.srcAddr.addr.shortAddr = 0;
			switch ( pRFDebugRcv->type )
			{
			case RFDEBUG_TYPE_NIB:
				{
					pRFDebugSend->msgtype  = RF_DEBUG;
					pRFDebugSend->type = pRFDebugRcv->type;
					pRFDebugSend->cmd = RFDEBUG_CMD_ACK;
					pRFDebugSend->startAddr = 0;
					pRFDebugSend->Addrlen = 0;
					pRFDebugSend->seq = 0;
					MineApp_RFDebug_SendFrame ( pRFDebugSend, ( uint8 * ) &_NIB, sizeof ( _NIB ), FramePayloadLen );

					break;
				}
			case RFDEBUG_TYPE_MACPIB:
				{
					pRFDebugSend->msgtype  = RF_DEBUG;
					pRFDebugSend->type = pRFDebugRcv->type;
					pRFDebugSend->cmd = RFDEBUG_CMD_ACK;
					pRFDebugSend->startAddr = 0;
					pRFDebugSend->Addrlen = 0;

					MineApp_RFDebug_SendFrame ( pRFDebugSend, ( uint8 * ) &macPib, sizeof ( macPib ), FramePayloadLen );

					break;
				}
			case RFDEBUG_TYPE_ASSOCDEVS:
				{
					pRFDebugSend->msgtype  = RF_DEBUG;
					pRFDebugSend->type = pRFDebugRcv->type;
					pRFDebugSend->cmd = RFDEBUG_CMD_ACK;
					pRFDebugSend->startAddr = 0;
					pRFDebugSend->Addrlen = 0;
					for ( uint8 i=0; i<3; i++ )   // 21 elemenet, send for 3 times;
					{
						pRFDebugSend->seq = i;
						MineApp_RFDebug_SendFrame ( pRFDebugSend, ( uint8* ) &AssociatedDevList[i*7], ( uint16 ) sizeof ( associated_devices_t ) *7,FramePayloadLen );
					}
					break;
				}
			case RFDEBUG_TYPE_READMEM:
				{
					pRFDebugSend->msgtype  = RF_DEBUG;
					pRFDebugSend->type = pRFDebugRcv->type;
					pRFDebugSend->cmd = RFDEBUG_CMD_ACK;
					MineApp_RFDebug_SendMultiFrame ( pRFDebugSend,  ( uint8 * ) pRFDebugRcv->startAddr, pRFDebugRcv->Addrlen, FramePayloadLen );
					break;
				}
			case RFDEBUG_TYPE_WTIREMEM:
				{
					uint8 *p = ( uint8 * ) ( pRFDebugRcv->startAddr );
					*p = * ( uint8 * ) ( pRFDebugRcv+1 );

					pRFDebugSend->msgtype  = RF_DEBUG;
					pRFDebugSend->type = pRFDebugRcv->type;
					pRFDebugSend->cmd = RFDEBUG_CMD_ACK;
					pRFDebugSend->startAddr = pRFDebugRcv->startAddr;
					pRFDebugSend->Addrlen = pRFDebugRcv->Addrlen;
					pRFDebugSend->seq= 0;
					pRFDebugSend->len= 1;

					uint8 data = * ( uint8 * ) ( pRFDebugSend->startAddr );  // read the data setted out again
					MineApp_RFDebug_SendFrame ( pRFDebugSend, &data, 1,  FramePayloadLen );
					break;
				}
			case RFDEBUG_TYPE_RESETNLME:
				{
					ZStatus_t status;
					status = NLME_ResetRequest();

					pRFDebugSend->msgtype  = RF_DEBUG;
					pRFDebugSend->type = pRFDebugRcv->type;
					pRFDebugSend->cmd = RFDEBUG_CMD_ACK;
					pRFDebugSend->startAddr = pRFDebugSend->startAddr;
					pRFDebugSend->Addrlen = pRFDebugSend->Addrlen;
					pRFDebugSend->seq= 0;
					pRFDebugSend->len= sizeof ( status );
					MineApp_RFDebug_SendFrame ( pRFDebugSend, &status, sizeof ( status ),  FramePayloadLen );
					break;
				}
			case RFDEBUG_TYPE_RESETMLME:
				{
					uint8 status;
					status = MAC_MlmeResetReq ( FALSE );
					pRFDebugSend->msgtype  = RF_DEBUG;
					pRFDebugSend->type = pRFDebugRcv->type;
					pRFDebugSend->cmd = RFDEBUG_CMD_ACK;
					pRFDebugSend->startAddr = pRFDebugSend->startAddr;
					pRFDebugSend->Addrlen = pRFDebugSend->Addrlen;
					pRFDebugSend->seq= 0;
					pRFDebugSend->len= sizeof ( status );
					MineApp_RFDebug_SendFrame ( pRFDebugSend, &status, sizeof ( status ),  FramePayloadLen );

					break;
				}
			case RFDEBUG_TYPE_RESETMACLOW:
				{
					halIntState_t intState;
					HAL_ENTER_CRITICAL_SECTION(intState);
					macLowLevelReset();
					HAL_EXIT_CRITICAL_SECTION(intState);

					pRFDebugSend->msgtype  = RF_DEBUG;
					pRFDebugSend->type = pRFDebugRcv->type;
					pRFDebugSend->cmd = RFDEBUG_CMD_ACK;
					pRFDebugSend->startAddr = pRFDebugSend->startAddr;
					pRFDebugSend->Addrlen = pRFDebugSend->Addrlen;
					pRFDebugSend->seq= 0;
					pRFDebugSend->len= 0;
					MineApp_RFDebug_SendFrame ( pRFDebugSend, NULL, 0,  FramePayloadLen );

					break;
				}
			}
		}
	}

#endif
}
#endif

/******************************************************************************
* @fn      OSmsg2SPIpkt
*
* @brief   Construct SPIMSGPacket_t.
*
* @param
*                 MSGpkt - The incoming msg need to be processed.
*                 msg - the returned buffer to hold the converted msg.
*
* @return  Status of the function call
*****************************************************************************/
byte OSmsg2SPIpkt ( const afIncomingMSGPacket_t* MSGpkt, SPIMSGPacket_t * msg )
{
	const uint8 maxDataLen = MINEAPP_MAX_DATA_LEN - sizeof(SPIMSGPacket_t);

	switch ( MSGpkt->hdr.event )
	{
	case AF_INCOMING_MSG_CMD:
		msg->spihdr.hdr.event = SPI_RX;
		msg->spihdr.srcAddr.addr.shortAddr = MSGpkt->srcAddr.addr.shortAddr;
		msg->spihdr.transID = MSGpkt->cmd.TransSeqNumber;
		msg->spihdr.options = INIT_OPN;
		msg->spihdr.radius = 0x01;
		msg->DataLength = MSGpkt->cmd.DataLength < maxDataLen ? MSGpkt->cmd.DataLength : maxDataLen;
		osal_memcpy((void *)(msg+1), MSGpkt->cmd.Data, msg->DataLength);
		break;
	default:
		break;

	}
	return ZSuccess;
}

byte MineApp_ParseChargedCardFrame ( const afIncomingMSGPacket_t* MSGpkt )
{
	APPWrapper_t* AppPkt = ( APPWrapper_t* ) ( MSGpkt->cmd.Data );
	byte appflag = AppPkt->app_flag;
	switch ( appflag )
	{
	case CHARGEED_SSREQ:
		{
			uint8 reqtype = AppPkt->app_chargeed_ssReq.reqtype;
			if ( reqtype == SSREQ_POLL )
			{
				app_chargeed_ssRsp_t app_chargeed_ssRsp;
				app_chargeed_ssRsp.msgtype = CHARGEED_SSRSP;
				app_chargeed_ssRsp.srcPan = _NIB.nwkPanId;
				app_chargeed_ssRsp.locnode_num = MineApp_LocNodeStatistic.Locnum;
				app_chargeed_ssRsp.seqnum = AppPkt->app_chargeed_ssReq.seqnum;
				if ( MineApp_retreat == MINEAPP_URGENT_RETREAT )
				{
					app_chargeed_ssRsp.urgent_type = RETREAT;
					app_chargeed_ssRsp.urgent_value = 0;
				}
				else if ( MineApp_retreat == MINEAPP_URGENT_CANCELRETREAT)
				{
					app_chargeed_ssRsp.urgent_type = CANCELRETREAT;
					app_chargeed_ssRsp.urgent_value = 0;
				}
				else
				{
					app_chargeed_ssRsp.urgent_type = URGENT_NONE;
					app_chargeed_ssRsp.urgent_value = 0;					
				}

				MacParam_t param;
				param.cluster_id = CHARGEED_CLUSTERID;
				param.panID = CARD_NWK_ADDR;
				param.radius = 0x01;

				MAC_UTIL_BuildandSendDataPAN ( &param, ( uint8 * ) &app_chargeed_ssRsp, sizeof(app_chargeed_ssRsp_t),
					MAC_UTIL_UNICAST, MSGpkt->srcAddr.addr.shortAddr , MAC_TXOPTION_NO_CNF );

			}

			/* If received a SSREQ_OUT or SSREQ_POLL , Send SSIND to SPI*/
			if ( reqtype == SSREQ_OUT || reqtype == SSREQ_POLL )
			{
#if (defined HAL_SPI) && (HAL_SPI == TRUE)

				CONV_LQI_TO_RSSI(MineApp_RSSI, MSGpkt->LinkQuality);
				app_chargeed_ssInd_t chargeed_ssInd;
				chargeed_ssInd.rssipkt.rssi = MineApp_RSSI;
				chargeed_ssInd.rssipkt.NWK_ADDR = AppPkt->app_chargeed_ssReq.srcPan;
				chargeed_ssInd.rssipkt.NODE_ADDR = MSGpkt->srcAddr.addr.shortAddr;
				chargeed_ssInd.rssipkt.seqnum = AppPkt->app_chargeed_ssReq.seqnum;
				chargeed_ssInd.msgtype  = CHARGEED_SSIND;

				uint8 locCnt = AppPkt->app_chargeed_ssReq.LocCnt + 1;  // count passed by chargeED + myself
				chargeed_ssInd.LocCnt = locCnt;

				if(sizeof(SPIMSGPacket_t) + sizeof(app_chargeed_ssInd_t) + locCnt* sizeof(LocPos_t) < MINEAPP_MAX_DATA_LEN)
				{
					/* protocol header */
					uint8  *p = (uint8 * ) ( pSpi+1 );
					osal_memcpy ( (void *)p, ( void * ) &chargeed_ssInd, sizeof(chargeed_ssInd) );

					/* LocCnt-1 LocPos */
					p += sizeof(chargeed_ssInd);
					osal_memcpy( (void *)p,(void *)(&(AppPkt->app_chargeed_ssReq) + 1), 
						(locCnt -1) * sizeof(LocPos_t));

					/* Add my LocPos*/
					LocPos_t LocPos;
					LocPos.LocNode_ID = _NIB.nwkPanId;
					LocPos.rssi  = MineApp_RSSI;
					osal_memcpy( p + (locCnt -1) * sizeof(LocPos_t),(void*)&LocPos, sizeof(LocPos_t));

					/* Sort */
					MineApp_LocSortByRSSI((LocPos_t *)p,locCnt);

					MineApp_SendDataToSpiEx(MSGpkt->srcAddr.addr.shortAddr, sizeof( app_chargeed_ssInd_t) + locCnt * sizeof(LocPos_t));
				}
#endif
			}
			break;
		}
	case URGENT:
		{
			uint8 urgenttype = AppPkt->app_Urgent.urgenttype;
			if ( urgenttype == ALERT )
			{
				app_Urgent_t app_Urgent;
				app_Urgent.msgtype = URGENT;
				app_Urgent.urgenttype = ALERTACK;
				app_Urgent.value = 0;

				MacParam_t param;
				param.cluster_id = CHARGEED_CLUSTERID;
				param.panID = CARD_NWK_ADDR;
				param.radius = 1;
				MAC_UTIL_BuildandSendDataPAN(&param, ( uint8 * ) &app_Urgent, sizeof ( app_Urgent ), 
					MAC_UTIL_UNICAST, MSGpkt->srcAddr.addr.shortAddr,
					MAC_TXOPTION_NO_CNF);

				if ( ZSuccess == OSmsg2SPIpkt ( MSGpkt, pSpi ) )
				{
#if (defined HAL_SPI) && (HAL_SPI == TRUE)
					HalSPIWrite ( ( uint8 * ) pSpi, pSpi ->DataLength + SPIPKTHDR_LEN );
#endif
				}
			}
			else if ( urgenttype == NOPWR )
			{
				if ( ZSuccess == OSmsg2SPIpkt ( MSGpkt, pSpi ) )
				{
#if (defined HAL_SPI) && (HAL_SPI == TRUE)
					HalSPIWrite ( ( uint8 * ) pSpi, pSpi ->DataLength + SPIPKTHDR_LEN );
#endif				
				}
			}
			break;
		}	
		/* write to SPI directly */
	case LOCNODE_ALARM:
		{
			if(ZSuccess == OSmsg2SPIpkt ( MSGpkt, pSpi ))
			{
#if (defined HAL_SPI) && (HAL_SPI == TRUE)
				HalSPIWrite(( uint8 * ) pSpi, pSpi ->DataLength + SPIPKTHDR_LEN);
#endif
			}

			break;
		}
	default:
		break;
	}
	return 0;
}

/*********************************************************************
* @fn      MineApp_ParseCardFrame
*
* @brief   This function processes OTA message to app data if needed.
*
* @param   MSGpkt - The incoming msg.
*
* @return  none
*/
byte MineApp_ParseCardFrame ( const afIncomingMSGPacket_t* MSGpkt )
{
	APPWrapper_t* AppPkt = ( APPWrapper_t* ) ( MSGpkt->cmd.Data );
	byte appflag = AppPkt->app_flag;
	switch ( appflag )
	{
	case SSREQ:
		{
			uint8 reqtype = AppPkt->app_ssReq.reqtype;

			/* If poll, Send data to card */
			if ( reqtype & SSREQ_POLL )
			{
#ifdef MED_TEST
				HalLedBlink ( HAL_LED_2,3, 30, 100 );
#endif
				if ( MineApp_retreat == MINEAPP_URGENT_RETREAT )
				{
					app_Urgent_t app_Urgent;
					app_Urgent.msgtype = URGENT;
					app_Urgent.urgenttype = RETREAT;
					app_Urgent.value = 0;
					MAC_UTIL_BuildandSendData ( ( uint8 * ) &app_Urgent,sizeof ( app_Urgent ), MAC_UTIL_UNICAST, 
						MSGpkt->srcAddr.addr.shortAddr, NULL );
				}
				else if ( ( MineApp_retreat == MINEAPP_URGENT_CANCELRETREAT ) )
				{
					app_Urgent_t app_Urgent;
					app_Urgent.msgtype = URGENT;
					app_Urgent.urgenttype = CANCELRETREAT;
					app_Urgent.value = 0;
					MAC_UTIL_BuildandSendData ( ( uint8 * ) &app_Urgent,sizeof ( app_Urgent ), MAC_UTIL_UNICAST, 
						MSGpkt->srcAddr.addr.shortAddr, NULL );
				}
				else
				{
					if ( MineApp_Sleep_info.flag )
					{
#if 0
						Appdata.app_Sleep.msgtype = NODESLEEP;
						Appdata.app_Sleep.sleeptime = MineApp_SleepPeriod;
						Appdata.app_Sleep.poll_interval = MineApp_poll_interval;
						MAC_UTIL_BuildandSendData ( ( uint8 * ) &Appdata,sizeof ( app_Sleep_t ), MAC_UTIL_UNICAST, MSGpkt->srcAddr.addr.shortAddr,NULL );
#endif
					}
					else
					{
						app_ssReq_t app_ssReq;
						app_ssReq.msgtype = SSREQ;
						app_ssReq.reqtype = SSREQ_NODATA;
						app_ssReq.NWK_ADDR = CARD_NWK_ADDR;
						MAC_UTIL_BuildandSendData ( ( uint8 * ) &app_ssReq,sizeof ( app_ssReq_t ), MAC_UTIL_UNICAST, 
							MSGpkt->srcAddr.addr.shortAddr, NULL);
					}
				}
			}

			/* If received a SSREQ_OUT or SSREQ_POLL , Send SSIND to SPI*/
			if ( reqtype & SSREQ_OUT || reqtype & SSREQ_POLL )
			{
#if (defined HAL_SPI) && (HAL_SPI == TRUE)
				CONV_LQI_TO_RSSI ( MineApp_RSSI, MSGpkt->LinkQuality );
				app_ssInd_t ssInd;
				ssInd.rssipkt.rssi = MineApp_RSSI;
				ssInd.rssipkt.NWK_ADDR = AppPkt->app_ssReq.NWK_ADDR;
				ssInd.rssipkt.NODE_ADDR = MSGpkt->srcAddr.addr.shortAddr;
				ssInd.rssipkt.seqnum = AppPkt->app_ssReq.seqnum;
				ssInd.msgtype  = SSIND;
				MineApp_SendDataToSpi(MSGpkt->srcAddr.addr.shortAddr, (uint8*)&ssInd, sizeof(ssInd));
#endif
			}
			break;
		}
	case URGENT:
		{
#ifdef MED_TEST
			HalLedBlink ( HAL_LED_2,5, 30, 100 );
#endif
			uint8 urgenttype = AppPkt->app_Urgent.urgenttype;
			if ( urgenttype == ALERT )
			{
				app_Urgent_t app_Urgent;
				app_Urgent.msgtype = URGENT;
				app_Urgent.urgenttype = ALERTACK;
				app_Urgent.value = 0;
				MAC_UTIL_BuildandSendData ( ( uint8 * ) &app_Urgent,sizeof ( app_Urgent_t ), MAC_UTIL_UNICAST, MSGpkt->srcAddr.addr.shortAddr,NULL );

				if ( ZSuccess == OSmsg2SPIpkt ( MSGpkt, pSpi ) )
				{
#if (defined HAL_SPI) && (HAL_SPI == TRUE)
					HalSPIWrite ( ( uint8 * ) pSpi, pSpi ->DataLength + SPIPKTHDR_LEN );
#endif
				}
			}
			else if ( urgenttype == NOPWR )
			{
				if ( ZSuccess == OSmsg2SPIpkt ( MSGpkt, pSpi ) )
				{
#if (defined HAL_SPI) && (HAL_SPI == TRUE)
					HalSPIWrite ( ( uint8 * ) pSpi, pSpi ->DataLength + SPIPKTHDR_LEN );
#endif
				}
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
void MineApp_HandleKeys ( uint16 keys, byte shifts )
{
	switch ( keys )
	{
	case HAL_KEY_SW_7:

#ifdef CHARGEED_TEST
		{
			if(MineApp_retreat== MINEAPP_URGENT_RETREAT)
			{
				MineApp_retreat = MINEAPP_URGENT_CANCELRETREAT;
			}
			else
			{
				MineApp_retreat = MINEAPP_URGENT_RETREAT;
			}
		}
#endif
		break;
	default:
		break;
	}
}

/*********************************************************************
* @fn      MineApp_BuildAssocAddr
*
* @brief   This function is used to build an assocAddr directly insead of start a nwk-join cycle.
*
* @param   appData - app_DeputyJoin_t data to hold the needed info.
*
* @return  status.
*/
byte MineApp_BuildAssocAddr ( app_DeputyJoin_t* appData )
{
	associated_devices_t* assocaddr = NULL;
	uint16 prenodeaddr = INVALID_ADDR;

	halIntState_t   intState;
	HAL_ENTER_CRITICAL_SECTION ( intState );
	//check if a dead link existed, if so ,clear it and allocate a new one..
	NLME_RemoveChild ( appData->macAddr, true );

	prenodeaddr = RTG_AllocNewAddress ( 0 );
	if ( prenodeaddr != INVALID_ADDR )
	{
		if ( AssocGetWithShort ( prenodeaddr ) ) //No idea why it uses a prevois shortAddr, and filter it here.
		{
			HAL_EXIT_CRITICAL_SECTION ( intState );
			return NOT_READY;
		}

		if ( appData->capabilityInfo & CAPINFO_RCVR_ON_IDLE )
			assocaddr = AssocAddNew ( prenodeaddr, appData->macAddr, CHILD_RFD_RX_IDLE );
		else
			assocaddr = AssocAddNew ( prenodeaddr, appData->macAddr, CHILD_RFD );
		osal_cpyExtAddr ( deviceAddr.PANInfo.extendedPANID, _NIB.extendedPANID );
		osal_cpyExtAddr ( deviceAddr.PANInfo.extendedCoordAddr, _NIB.nwkCoordExtAddress );
		deviceAddr.PANInfo.PanId = _NIB.nwkPanId;
		deviceAddr.PANInfo.Channel = _NIB.nwkLogicalChannel;
		deviceAddr.PANInfo.CoordAddr = _NIB.nwkDevAddress;
		deviceAddr.shortAddr = assocaddr->shortAddr;
		HAL_EXIT_CRITICAL_SECTION ( intState );
		return ZSUCCESS;
	}
	HAL_EXIT_CRITICAL_SECTION ( intState );
	return MSG_BUFFER_NOT_AVAIL;
}

void MineApp_StartProbeSpiEvt ( void )
{
	osal_start_timerEx ( MineApp_TaskID, MINEAPP_PROBE_SPI_EVENT, 8000 );
}

void MineApp_StopProbeSpiEvt ( void )
{
	//IsSPIProbeOpen = false;
	SPIErrCnt = 0;
	osal_stop_timerEx ( MineApp_TaskID, MINEAPP_PROBE_SPI_EVENT );
}
/* copy data and send to SPI */
uint8  MineApp_SendDataToSpi(uint16 shortAddr,const uint8 *p, uint16 len)
{
	const uint8 maxDataLen = MINEAPP_MAX_DATA_LEN - sizeof(SPIMSGPacket_t);
	len = len<maxDataLen ? len : maxDataLen;
	pSpi->spihdr.srcAddr.addr.shortAddr = shortAddr;
	pSpi->DataLength = len;
	osal_memcpy ( ( void * ) ( pSpi+1 ), ( void * ) p, pSpi->DataLength );
	return HalSPIWrite ( ( uint8 * ) pSpi, pSpi ->DataLength + SPIPKTHDR_LEN );
}

/* data is prepared, just send to SPI */
uint8 MineApp_SendDataToSpiEx(uint16 shortAddr, uint16 len)
{
	const uint8 maxDataLen = MINEAPP_MAX_DATA_LEN - sizeof(SPIMSGPacket_t);
	len = len<maxDataLen ? len : maxDataLen;
	pSpi->spihdr.srcAddr.addr.shortAddr = shortAddr;
	pSpi->DataLength = len;
	return HalSPIWrite ( ( uint8 * ) pSpi, pSpi ->DataLength + SPIPKTHDR_LEN );
}

void MineApp_LocSortByRSSI(LocPos_t* LocNode, int8 LastCnt)
{
	if(LastCnt < 2)
	{
		return;
	}
	for(int8 i = 0; i<LastCnt;i++)
	{
		for(int8 j=LastCnt-1;j>i;j--)
		{
			if(LocNode[j-1].rssi< LocNode[j].rssi)
			{
				LocPos_t loctmp;
				loctmp = LocNode[j-1] ;
				LocNode[j-1] = LocNode[j];
				LocNode[j]   = loctmp;
			}
		}
	}
}

#ifdef USE_RF_DEBUG
void MineApp_RFDebug_SendFrame ( app_RFDebug_t *pHeader, uint8 *p, uint16 len, uint8 FrameLen )
{
	if ( len > FrameLen )
	{
		len = FrameLen;
	}

	pSpi->DataLength = sizeof ( app_RFDebug_t ) + len;
	pHeader->len = len;
	osal_memcpy ( ( void * ) ( pHeader+1 ), p, len );
	HalSPIWrite ( ( uint8* ) pSpi, pSpi->DataLength + SPIPKTHDR_LEN );
}

void MineApp_RFDebug_SendMultiFrame ( app_RFDebug_t *pHeader, uint8 *p, uint16 len, uint8 FrameLen )
{
	uint8 SendCnt = len/FrameLen;
	//	app_RFDebug_t *pRFDebugSend = pRFDebugSend;
	uint8 seq = 0;

	for ( uint8 i=0; i< SendCnt; i++ )
	{
		pSpi->DataLength = sizeof ( app_RFDebug_t ) + FrameLen;

		pHeader->startAddr = ( uint16 ) ( p+i*FrameLen );
		pHeader->Addrlen = FrameLen;
		pHeader->seq = seq++;
		pHeader->len = FrameLen;
		osal_memcpy ( ( void * ) ( pHeader+1 ), ( uint8* ) pHeader->startAddr, pHeader->len);
		HalSPIWrite ( ( uint8* ) pSpi, pSpi->DataLength + SPIPKTHDR_LEN );
	}

	uint8 RestLen = len- SendCnt*FrameLen;
	pSpi->DataLength = sizeof ( app_RFDebug_t ) + RestLen;
	pHeader->startAddr = ( uint16 ) ( p+SendCnt*FrameLen );
	pHeader->Addrlen = FrameLen;
	pHeader->seq = seq++;
	pHeader->len = RestLen;
	osal_memcpy ( ( void * ) ( pHeader+1 ), ( uint8 * ) pHeader->startAddr , pHeader->len );
	HalSPIWrite ( ( uint8* ) pSpi, pSpi->DataLength + SPIPKTHDR_LEN );
}

#endif

static byte MineApp_ParseMobileFrame(const afIncomingMSGPacket_t* MSGpkt)
{
#if (defined HAL_SPI) && (HAL_SPI == TRUE)
	APPWrapper_t* AppPkt = ( APPWrapper_t* ) ( MSGpkt->cmd.Data );

	/* the phone is health when received any packet of it */
	MineApp_UpdateAddrListHeadth(MSGpkt->srcAddr.addr.shortAddr);

	if ( ( AppPkt->app_flag == MP_SSREQ ) && ( AppPkt->app_MPssReq.reqtype == SSREQ_OUT ) )
	{
		CONV_LQI_TO_RSSI ( MineApp_RSSI, MSGpkt->LinkQuality );
		app_MPssInd_t ssInd;
		ssInd.rssipkt.rssi = MineApp_RSSI;
		ssInd.rssipkt.NWK_ADDR = AppPkt->app_MPssReq.NWK_ADDR;
		ssInd.rssipkt.NODE_ADDR = MSGpkt->srcAddr.addr.shortAddr;
		ssInd.rssipkt.seqnum = AppPkt->app_MPssReq.seqnum;
		ssInd.msgtype  = MP_SSIND;
		osal_memcpy ( ssInd.nmbr.Nmbr, AppPkt->app_MPssReq.nmbr.Nmbr, sizeof ( termNbr_t ) );

		MineApp_SendDataToSpi(MSGpkt->srcAddr.addr.shortAddr, (uint8*) &ssInd, sizeof(ssInd));
	}
	else if ( AppPkt->app_flag == ZB_LEAVE_NOTIFY )
	{
		NLME_RemoveChild ( AppPkt->app_LeaveNwk.macAddr, true );
		if ( ZSuccess == OSmsg2SPIpkt ( MSGpkt, pSpi ) )
			HalSPIWrite ( ( uint8 * ) pSpi, pSpi ->DataLength + SPIPKTHDR_LEN );
	}
	else if ( ZSuccess == OSmsg2SPIpkt ( MSGpkt, pSpi ) )
	{
		HalSPIWrite ( ( uint8 * ) pSpi, pSpi ->DataLength + SPIPKTHDR_LEN );
	}
#endif
	return ZSuccess;
}
static byte MineApp_ParseGasFrame(const afIncomingMSGPacket_t* MSGpkt)
{
	APPWrapper_t* AppPkt = (APPWrapper_t*)(MSGpkt->cmd.Data);
	if (AppPkt->app_flag == GASALARM)// && AppPkt->app_GasAlarm.AlarmType == GASNODE_URGENT)
	{
		app_GasAlarm_ack_t app_GasAlarm_ack;
		app_GasAlarm_ack.msgtype = GASALARM_ACK;
		app_GasAlarm_ack.DevID = AppPkt->app_GasAlarm.DevID;
		app_GasAlarm_ack.SrcPan = zgConfigPANID;
		app_GasAlarm_ack.seq = AppPkt->app_GasAlarm.seq;
		app_GasAlarm_ack.AlarmType = AppPkt->app_GasAlarm.AlarmType;
		MacParam_t param;
		param.panID = AppPkt->app_GasAlarm.DevID;
		param.cluster_id = GASMONITOR_CLUSTERID;
		param.radius = 0x01;
		MAC_UTIL_BuildandSendDataPAN(&param, (uint8 *)&app_GasAlarm_ack,
			sizeof(app_GasAlarm_ack), MAC_UTIL_UNICAST, 0, 0);

		if (ZSuccess == OSmsg2SPIpkt(MSGpkt, pSpi))
		{
			HalSPIWrite(( uint8 *)pSpi, pSpi ->DataLength + SPIPKTHDR_LEN);
		}	
	}
	/* else write to spi directly */
	else if ( AppPkt->app_flag != LOCNODE_CAST && ZSuccess == OSmsg2SPIpkt ( MSGpkt, pSpi ))
	{
		HalSPIWrite(( uint8 * ) pSpi, pSpi ->DataLength + SPIPKTHDR_LEN);
	}
	return ZSuccess;
}

void MineApp_UpdateAddrListHeadth(uint16 shortAddr)
{
	for(uint8 i=0; i< NWK_MAX_DEVICES;i++)
	{
		if(AssociatedDevList[i].shortAddr!=0xFFFF &&  AssociatedDevList[i].shortAddr == shortAddr)
		{
			AssociatedDevList_Info.pHealth[i] = 0;
			break;
		}
	}
}
void MineApp_CheckAddrListHealth(void )
{	
	for(uint8 i=0; i< NWK_MAX_DEVICES;i++)
	{
		if(AssociatedDevList[i].shortAddr!=0xFFFF)
		{
			AssociatedDevList_Info.pHealth[i]++;
			if(AssociatedDevList_Info.pHealth[i] >= MINEAPP_ADDRLIST_MAXHEALTH)
			{
				uint8 extAddr[Z_EXTADDR_LEN];
				uint16 shortAddr = AssociatedDevList[i].shortAddr;
				if(APSME_LookupExtAddr(shortAddr, extAddr) == true)
				{
					NLME_RemoveChild(extAddr, true);
				}
				else
				{	
					//RTG_DeAllocAddress(shortAddr);
				}
			}
		}
		else
		{
			AssociatedDevList_Info.pHealth[i] = 0;
		}
	}
}
void MineApp_AddrListReport(void)
{
		uint8 idx = MineApp_RFReport.addrListIdx;

		if(idx >= NWK_MAX_DEVICES-1)  	//start another print after 10s
		{
			if(ZSuccess!= osal_start_timerEx(MineApp_TaskID, MINEAPP_ADDRLIST_REPORT_EVENT, 10000))
			{
				SystemReset();
			}
		}
		else 	//after 100ms
		{
			if(ZSuccess!= osal_start_timerEx(MineApp_TaskID, MINEAPP_ADDRLIST_REPORT_EVENT, 100))
			{
				SystemReset();
			}
		}

		if(idx == 0 || AssociatedDevList[idx].shortAddr!=0xFFFF)
		{
			MineApp_RFReport.pBuf->msgtype = RF_REPORT;
			MineApp_RFReport.pBuf->type = APP_RFREPORT_TYPE_ADDRLIST;
			if(idx ==0 )
			{
				MineApp_RFReport.pBuf->tag = APP_RFREPORT_TAG_BEGIN;
			}
			else
			{
				MineApp_RFReport.pBuf->tag = APP_RFREPORT_TAG_MSG;
			}

			MineApp_RFReport.pBuf->seqnum++;
			
			if(AssociatedDevList[idx].shortAddr == 0xFFFF)
			{
				MineApp_RFReport.pBuf->len = 0;
			}
			else
			{
				uint8* pContant = (uint8*)(MineApp_RFReport.pBuf+1);
				uint16 shortAddr = AssociatedDevList[idx].shortAddr;
				uint8 extAddr[Z_EXTADDR_LEN];
				if(APSME_LookupExtAddr(shortAddr, extAddr) == true)
				{
					sprintf((char*)pContant,"sAddr:%x eAddr:%x %x %x %x %x %x %x %x H:%d\n"
					,shortAddr,extAddr[0],extAddr[1],extAddr[2],extAddr[3],extAddr[4],extAddr[5],extAddr[6],extAddr[7],AssociatedDevList_Info.pHealth[idx]);
				}
				else
				{
					sprintf((char*)pContant,"sAddr:%x eAddr:Invalid H:%d\n"
					,shortAddr,AssociatedDevList_Info.pHealth[idx]);
				}

				MineApp_RFReport.pBuf->len = strlen((char*)pContant);
			}
			MineApp_SendDataToSpi(0, (uint8* )MineApp_RFReport.pBuf, sizeof(app_rfreport_t) + MineApp_RFReport.pBuf->len);
		}

		// increase idx
		if(++MineApp_RFReport.addrListIdx >=  NWK_MAX_DEVICES)
		{
			MineApp_RFReport.addrListIdx = 0;
		}
}
