/**************************************************************************************************
Filename:       MLN.c
Revised:        $Date: 2010/07/22 05:12:00 $
Revision:       $Revision: 1.11 $

Description:    This file contains the application that can be use to set a device as Location
node from MAC directly which will communicate with a FFD with full z-stack.

**************************************************************************************************/
/**************************************************************************************************

Work FLow:  blast---sleep
|		|
---------
**************************************************************************************************/

/**************************************************************************************************
*                                           Includes
**************************************************************************************************/
/* Hal Driver includes */
#include "hal_types.h"
#include "hal_key.h"
#include "hal_timer.h"
#include "hal_drivers.h"
#include "hal_adc.h"
#include "hal_assert.h"
#include "FlashUtil.h"
#include "MacUtil.h"
/* OS includes */
#include "OSAL.h"
#include "OSAL_Tasks.h"

/* App Protocol*/
#include "AppProtocolWrapper.h"

/* Application Includes */
#include "OnBoard.h"

/* MAC Application Interface */
#include "mac_api.h"

/* Application */
#include "MLN.h"

/*My Sleep Util*/
#include "SleepUtil.h"

/* watchdog util */
#include "watchdogutil.h"

/*********************************************************************
* TYPEDEFS
*/

typedef struct
{
	sAddrExt_t ExitAddr;
} Dev_Info_t;

/**************************************************************************************************
*                                           Constant
**************************************************************************************************/
#define MLN_MAC_PAYLOAD_LEN			 127                  //mac payload length by datasheet.
#define MLN_SHORTADDR_PANCOORD		 0x0000             /* Short Address of PAN Coordinator, will always be 0x0000 */
//#define MLN_SHORTADDR_COORD           0x0000                   /* Short Address of Coordinator, Initial Value */
#define MLN_BROADCAST_SHORTADDR_DEVZCZR	0xFFFC   /* Short Address of all coords and routers*/
#define MLN_BROADCAST_SHORTADDR_DEVALL		0xFFFF   /* Short Address of all Devs*/

#define SLEEP_RST_POS		3
#define WDCTL_INT_POS		0
#define WDCTL_MODE_POS	2
#define WDCTL_EN_POS		3
#define WDCTL_CLR_POS		4


#define MLN_UNICAST			0
#define MLN_BROADCAST		1

#define MLN_EXITADDR		2


#define MLN_PARAM_ADDR		0xFD00		//0xFD00-0xFD53 are used to store paramters
#define MLN_DEVINFO_ADDR		0x7FF8
#define MLN_MIN_POWER              25

#define MLN_LED_BLUE    0x01
#define MLN_LED_RED      0x02

#define MLN_LOCCASTADJUST  0   //Adjust time  every time cast, measured by oscilloscope

#define TURN_ON_LED_BLUE()          HAL_TURN_ON_LED1()
#define TURN_OFF_LED_BLUE()        HAL_TURN_OFF_LED1()
#define TURN_ON_LED_RED()           HAL_TURN_ON_LED2()
#define TURN_OFF_LED_RED()          HAL_TURN_OFF_LED2()

#define STATE_LED_BLUE()              HAL_STATE_LED1()
#define STATE_LED_RED()                HAL_STATE_LED2()

#define BACK_AND_OFF_LED(blue,red)  st( blue = STATE_LED_BLUE(); \
	red  =  STATE_LED_RED(); \
	TURN_OFF_LED_BLUE(); \
	TURN_OFF_LED_RED(); ) 

#define SET_LED_BLUE(blue) st( if(blue){TURN_ON_LED_BLUE();}else{TURN_OFF_LED_BLUE();} )
#define SET_LED_RED(red)   st(if(red)  {TURN_ON_LED_RED();} else {TURN_OFF_LED_RED();} ) 

/* Size table for MAC structures */
const CODE uint8 MLN_cbackSizeTable [] =
{
	0,                                               /* unused */
	sizeof(macMlmeAssociateInd_t),       /* MAC_MLME_ASSOCIATE_IND */
	sizeof(macMlmeAssociateCnf_t),       /* MAC_MLME_ASSOCIATE_CNF */
	sizeof(macMlmeDisassociateInd_t),    /* MAC_MLME_DISASSOCIATE_IND */
	sizeof(macMlmeDisassociateCnf_t),    /* MAC_MLME_DISASSOCIATE_CNF */
	sizeof(macMlmeBeaconNotifyInd_t),    /* MAC_MLME_BEACON_NOTIFY_IND */
	sizeof(macMlmeOrphanInd_t),           /* MAC_MLME_ORPHAN_IND */
	sizeof(macMlmeScanCnf_t),              /* MAC_MLME_SCAN_CNF */
	sizeof(macMlmeStartCnf_t),             /* MAC_MLME_START_CNF */
	sizeof(macMlmeSyncLossInd_t),        /* MAC_MLME_SYNC_LOSS_IND */
	sizeof(macMlmePollCnf_t),                /* MAC_MLME_POLL_CNF */
	sizeof(macMlmeCommStatusInd_t),      /* MAC_MLME_COMM_STATUS_IND */
	sizeof(macMcpsDataCnf_t),            /* MAC_MCPS_DATA_CNF */
	sizeof(macMcpsDataInd_t),            /* MAC_MCPS_DATA_IND */
	sizeof(macMcpsPurgeCnf_t),           /* MAC_MCPS_PURGE_CNF */
	sizeof(macEventHdr_t)                  /* MAC_PWR_ON_CNF */
};

/**************************************************************************************************
*                                        Local Variables
**************************************************************************************************/

/*
Dev number and Extended address of the device.
*/
/* Coordinator and Device information */
static uint16        MLN_PanId = LOCNODE_NWK_ADDR;
//static uint16        MLN_CoordShortAddr = MLN_SHORTADDR_PANCOORD; /* Initial */
static uint16        MLN_PANCoordShortAddr = MLN_SHORTADDR_PANCOORD; /* Always be 0x00 */
static uint16        MLN_DevShortAddr   = MLN_BROADCAST_SHORTADDR_DEVALL; /* Initial */


/* TRUE and FALSE value */
static bool          MLN_MACTrue = TRUE;
static bool          MLN_MACFalse = FALSE;

static uint32 MLN_sleeptime;
//static bool     MLN_IsBatteryLow   =  FALSE;


/* Task ID */
uint8 MLN_TaskId;

/* counter */
static uint8 MLN_blast_cnt = 0;
//static uint8 MLN_vddcheck_cnt=0;

/* Device Info from flash */
static Dev_Info_t MLN_DevInfo;

/* MLN_seqnum */
static uint16 MLN_seqnum = 0;

/* Timer ticks */
static uint32 MLN_sleeptick = 0;
static uint32 MLN_waketick = 0;

/**************************************************************************************************
*                                     Local Function Prototypes
**************************************************************************************************/
/* Setup routines */
static void         MLN_DeviceStartup(uint8* pData);
/* Support */
static void         MLN_blast(uint8 reqtype);
static void         MLN_ReadDevInfo();
static void 	   MLN_IntervalCheck(void);
static void	  MLN_PeriodReset();
static void        MLN_Restart();
//static void 	MLN_SaveParam2RAM(void);
//static void 	MLN_ReadParmFromRAM(void);
static uint8  	MLN_GetBatteryValue(void);
static uint32  	MLN_GetTimeTick(void);
static inline uint32 MLN_ConvTick2Us(uint32 tick);

/**************************************************************************************************
*
* @fn          MLN_Init
*
* @brief       Initialize the application
*
* @param       taskId - taskId of the task after it was added in the OSAL task queue
*
* @return      none
*
**************************************************************************************************/
void MLN_Init(uint8 taskId)
{       
	/* Initialize the task id */
	MLN_TaskId = taskId;

	/* initialize MAC features */
	MAC_Init();
	MAC_InitDevice();

	/* Reset the MAC */
	MAC_MlmeResetReq(TRUE);

	/* initial MacUtil*/
	MacUtil_t Macutil;
	Macutil.panID = 0xFFFF;                          // Card broadcast to all PANs
	Macutil.dst_endpoint = APS_DST_ENDPOINT;
	Macutil.src_endpoint = APS_SRC_ENDPOINT;
	Macutil.cluster_id = APS_CLUSTER_ID;
	Macutil.profile_id = APS_PROFILE_ID;
	Macutil.NodeType =  NODETYPE_DEVICE;
	MAC_UTIL_INIT(&Macutil);

	MLN_sleeptime = MLN_LOCCAST_FREQ*1000UL;//*1000UL - MLN_LOCCASTADJUST;
	MLN_blast_cnt = 0;
	//MLN_vddcheck_cnt = 0;

	MLN_ReadDevInfo();

	MLN_DeviceStartup(NULL);


	/*Start Watch Dog*/
#ifdef WATCHDOG
	StartWatchDog(DOGTIMER_INTERVAL_1S);
#endif

	//uint8 Channel = MLN_DevInfo.ExitAddr[EXT_MACADDR_CHANNEL]; // must after MLN_ReadDevInfo()
	//MAC_UTIL_HalInit(Channel, 0);
	osal_set_event(MLN_TaskId, MLN_SLEEP_EVENT);
}

/**************************************************************************************************
*
* @fn          MLN_ProcessEvent
*
* @brief       This routine handles events
*
* @param       taskId - ID of the application task when it registered with the OSAL
*              events - Events for this task
*
* @return      16bit - Unprocessed events
*
**************************************************************************************************/
uint16 MLN_ProcessEvent(uint8 taskId, uint16 events)
{
	uint8* pMsg;
	macCbackEvent_t* pData;
	if (events & SYS_EVENT_MSG)
	{
		while ((pMsg = osal_msg_receive(MLN_TaskId)) != NULL)
		{
			switch ( *pMsg )
			{
			case MAC_MCPS_DATA_CNF:
				{
					pData = (macCbackEvent_t *) pMsg;
					osal_msg_deallocate((uint8 *) pData->dataCnf.pDataReq);
					break;
				}
			}

			/* Deallocate */
			osal_msg_deallocate((uint8 *) pMsg);
		}

		return events ^ SYS_EVENT_MSG;
	} 

	if(events & MLN_BLAST_EVENT)
	{
		/* Send out  message */
		MLN_blast_cnt++;
		MLN_blast(SSREQ_OUT);
		osal_set_event(MLN_TaskId, MLN_SLEEP_EVENT);

		return events ^ MLN_BLAST_EVENT;

	}
	if (events & MLN_SLEEP_EVENT)  /* Do battery_check, and sleep*/
	{
#ifdef WATCHDOG
		FeedWatchDog();
#endif
		/*Turn Off radio first */
		MAC_MlmeSetReq(MAC_RX_ON_WHEN_IDLE, &MLN_MACFalse);
		while(MAC_PwrOffReq(MAC_PWR_SLEEP_DEEP)!=MAC_SUCCESS);

		uint32 time_elipse = 0;

		MLN_sleeptick = MLN_GetTimeTick();

		if(MLN_waketick > 0)
		{
			if(MLN_sleeptick >= MLN_waketick)
			{
				time_elipse = MLN_ConvTick2Us(MLN_sleeptick - MLN_waketick);
			}
			else
			{
				time_elipse = MLN_ConvTick2Us(MLN_sleeptick + 0x01000000 - MLN_waketick);
			}
		}

		if(time_elipse < 20*1000)
		{
			MLN_sleeptime = MLN_LOCCAST_FREQ*1000UL - time_elipse + MLN_LOCCASTADJUST;
		}
		else
		{
			MLN_sleeptime = MLN_LOCCAST_FREQ*1000UL - 2*1000UL + MLN_LOCCASTADJUST;
		}

		UtilSleepUs(CC2430_PM2, MLN_sleeptime);
		MLN_PeriodReset();
		MLN_waketick = MLN_GetTimeTick();

		osal_set_event(MLN_TaskId, MLN_BLAST_EVENT);


		MAC_PwrOnReq(); // turn on mac 

		return events ^ MLN_SLEEP_EVENT;
	}
	return 0;
}

/**************************************************************************************************
*
* @fn          MAC_CbackEvent
*
* @brief       This callback function sends MAC events to the application.
*              The application must implement this function.  A typical
*              implementation of this function would allocate an OSAL message,
*              copy the event parameters to the message, and send the message
*              to the application's OSAL event handler.  This function may be
*              executed from task or interrupt context and therefore must
*              be reentrant.
*
* @param       pData - Pointer to parameters structure.
*
* @return      None.
*
**************************************************************************************************/
void MAC_CbackEvent(macCbackEvent_t *pData)
{

	macCbackEvent_t *pMsg = NULL;

	uint8 len = MLN_cbackSizeTable[pData->hdr.event];

	switch (pData->hdr.event)
	{
	case MAC_MLME_BEACON_NOTIFY_IND:

		len += sizeof(macPanDesc_t) + pData->beaconNotifyInd.sduLength +
			MAC_PEND_FIELDS_LEN(pData->beaconNotifyInd.pendAddrSpec);
		if ((pMsg = (macCbackEvent_t *) osal_msg_allocate(len)) != NULL)
		{
			/* Copy data over and pass them up */
			osal_memcpy(pMsg, pData, sizeof(macMlmeBeaconNotifyInd_t));
			pMsg->beaconNotifyInd.pPanDesc = (macPanDesc_t *) ((uint8 *) pMsg + sizeof(macMlmeBeaconNotifyInd_t));
			osal_memcpy(pMsg->beaconNotifyInd.pPanDesc, pData->beaconNotifyInd.pPanDesc, sizeof(macPanDesc_t));
			pMsg->beaconNotifyInd.pSdu = (uint8 *) (pMsg->beaconNotifyInd.pPanDesc + 1);
			osal_memcpy(pMsg->beaconNotifyInd.pSdu, pData->beaconNotifyInd.pSdu, pData->beaconNotifyInd.sduLength);
		}
		break;

	case MAC_MCPS_DATA_IND:
		pMsg = pData;
		break;
	default:
		if ((pMsg = (macCbackEvent_t *) osal_msg_allocate(len)) != NULL)
		{
			osal_memcpy(pMsg, pData, len);
		}
		break;
	}

	if (pMsg != NULL)
	{
		osal_msg_send(MLN_TaskId, (byte *) pMsg);
	}
}

/**************************************************************************************************
*
* @fn      MAC_CbackCheckPending
*
* @brief   Returns the number of indirect messages pending in the application
*
* @param   None
*
* @return  Number of indirect messages in the application
*
**************************************************************************************************/
uint8 MAC_CbackCheckPending(void)
{
	return (0);
}


/**************************************************************************************************
*
* @fn      MLN_DeviceStartup(uint8* pData)
*
* @brief   Update the timer per tick
*
* @param   beaconEnable: TRUE/FALSE
*
* @return  None
*
**************************************************************************************************/
void MLN_DeviceStartup(uint8* pData)
{
	MAC_MlmeSetReq(MAC_EXTENDED_ADDRESS, &MLN_DevInfo.ExitAddr);

	/* Setup PAN ID */
	MAC_MlmeSetReq(MAC_PAN_ID,&MLN_PanId);

	/* This device is setup for Direct Message */
	MAC_MlmeSetReq(MAC_RX_ON_WHEN_IDLE, &MLN_MACFalse);

	uint8 tmp = 0;
	MAC_MlmeSetReq(MAC_MAX_FRAME_RETRIES, &tmp);

	/* Setup Coordinator short address */
	MAC_MlmeSetReq(MAC_COORD_SHORT_ADDRESS, &MLN_PANCoordShortAddr);

	//uint8 tmp8 = MLN_MAC_CHANNEL;
	uint8 tmp8 = MLN_DevInfo.ExitAddr[EXT_MACADDR_CHANNEL];
	MAC_MlmeSetReq(MAC_LOGICAL_CHANNEL,&tmp8);

	MAC_MlmeSetReq(MAC_SHORT_ADDRESS,&MLN_DevShortAddr);

	MAC_MlmeSetReq(MAC_ASSOCIATED_PAN_COORD,&MLN_MACTrue);

	/* change CCA param */
	uint8 min_be = 0;
	MAC_MlmeSetReq(MAC_MIN_BE,&min_be);

	uint8 max_backoff = 5;
	MAC_MlmeSetReq(MAC_MAX_CSMA_BACKOFFS,&max_backoff);
}

#if 0
/**************************************************************************************************
*
* @fn      MLN_McpsDataReq()
*
* @brief   This routine calls the Data Request
*
* @param   data       - contains the data that would be sent
*          dataLength - length of the data that will be sent
*
* @return  None
*
**************************************************************************************************/
void MLN_McpsDataReq(uint8* data, uint8 dataLength, sAddr_t dstAddr,uint8 txOption)
{

	macMcpsDataReq_t  *pData;
	static uint8      handle = 0;

	if ((pData = MAC_McpsDataAlloc(dataLength, MAC_SEC_LEVEL_NONE, MAC_KEY_ID_MODE_NONE)) != NULL)
	{
		pData->mac.srcAddrMode = SADDR_MODE_SHORT;
		pData->mac.dstAddr = dstAddr;
		pData->mac.dstPanId = MLN_PanId;
		pData->mac.msduHandle = handle++;
		pData->mac.txOptions = txOption;
		pData->sec.securityLevel = false;

		// If it's the coordinator and the device is in-direct message 
		//if (MLN_IsCoordinator)
		// {
		//  if (!directMsg)
		//  {
		//    pData->mac.txOptions |= MAC_TXOPTION_INDIRECT;
		//  }
		// }

		osal_memcpy (pData->msdu.p, data, dataLength);
		pData->msdu.len = dataLength;

		MAC_McpsDataReq(pData);
	}

}

/**************************************************************************************************
*
* @fn      MLN_McpsPollReq()
*
* @brief   Performs a poll request on the coordinator
*
* @param   None
*
* @return  None
*
**************************************************************************************************/
void MLN_McpsPollReq(void)
{
	macMlmePollReq_t  pollReq;

	/* Fill in information for poll request */
	pollReq.coordAddress.addrMode = SADDR_MODE_SHORT;
	pollReq.coordAddress.addr.shortAddr = MLN_PANCoordShortAddr;
	pollReq.coordPanId = MLN_PanId;
	pollReq.sec.securityLevel = MAC_SEC_LEVEL_NONE;

	/* Call poll reuqest */
	MAC_MlmePollReq(&pollReq);
}

/**************************************************************************************************
*
* @fn      MacSampelApp_ScanReq()
*
* @brief   Performs active scan on specified channel
*
* @param   None
*
* @return  None
*
**************************************************************************************************/
void MLN_ScanReq(uint8 scanType, uint8 scanDuration)
{
	macMlmeScanReq_t scanReq;
#ifdef DEBUG
	HalLedSet(HAL_LED_1, HAL_LED_MODE_ON);
	MLN_Delay(500);
	HalLedSet(HAL_LED_1, HAL_LED_MODE_OFF);
#endif
	/* Fill in information for scan request structure */
	scanReq.scanChannels = (uint32) 1 << MLN_MAC_CHANNEL;
	scanReq.scanType = scanType;
	scanReq.scanDuration = scanDuration;
	scanReq.maxResults = MLN_MAC_MAX_RESULTS;
	scanReq.result.pPanDescriptor = MLN_PanDesc;

	/* Call scan request */
	MAC_MlmeScanReq(&scanReq);
}

/**************************************************************************************************
*
* @fn      MLN_SyncReq()
*
* @brief   Sync Request
*
* @param   None
*
* @return  None
*
**************************************************************************************************/
void MLN_SyncReq(void)
{
	macMlmeSyncReq_t syncReq;

	/* Fill in information for sync request structure */
	syncReq.logicalChannel = MLN_MAC_CHANNEL;
	syncReq.channelPage    = MAC_CHANNEL_PAGE_0;
	syncReq.trackBeacon    = FALSE;

	/* Call sync request */
	MAC_MlmeSyncReq(&syncReq);
}
#endif

/**************************************************************************************************
*
* @fn      MLN_HandleKeys
*
* @brief   Callback service for keys
*
* @param   keys  - keys that were pressed
*          state - shifted
*
* @return  void
*
**************************************************************************************************/
void MLN_HandleKeys(uint16 keys, uint8 shift)
{
}


/**************************************************************************************************
*
* @fn      MLN_blast(void)
*
* @brief   Blast once to all Coords and routers without ACK or retrans.
*
* @param   
*              
* @return  
*
**************************************************************************************************/
void MLN_blast(uint8 reqtype)
{
	TURN_ON_LED_BLUE();
	//MLN_IntervalCheck();
	uint8 battery = MLN_GetBatteryValue();

	app_LocNodeCast_t Appdata;
	Appdata.msgtype = LOCNODE_CAST;
	Appdata.vol = battery;
	Appdata.seq = MLN_seqnum++;
	Appdata.DevID = MLN_DevShortAddr;
#if 1
	MAC_UTIL_BuildandSendData((uint8 *)&Appdata,sizeof(app_LocNodeCast_t), MLN_BROADCAST, MLN_BROADCAST_SHORTADDR_DEVZCZR,MAC_TXOPTION_NO_RETRANS);
#else
	MacParam_t param;
	param.panID = 0xFFFF;
	param.cluster_id = APS_CLUSTER_ID;
	param.radius = 0x01;
	MAC_UTIL_HalBuildandSendDataPAN(&param, (uint8 *)&Appdata, sizeof(app_LocNodeCast_t), MLN_BROADCAST, MLN_BROADCAST_SHORTADDR_DEVZCZR, MAC_TXOPTION_NO_RETRANS);
#endif
	TURN_OFF_LED_BLUE();
}


void MLN_ReadDevInfo()
{
#if 0
	/* Make a fake DEV Info*/
	MLN_DevInfo.ExitAddr[0] = 0x00;
	MLN_DevInfo.ExitAddr[1] = 0xFF;
	MLN_DevInfo.ExitAddr[2] = 0xFF;
	MLN_DevInfo.ExitAddr[3] = 0xFF;
	MLN_DevInfo.ExitAddr[4] = 0x00;
	MLN_DevInfo.ExitAddr[5] = 0x00;
	MLN_DevInfo.ExitAddr[6] = 0xFF;
	MLN_DevInfo.ExitAddr[7] = 0x00;
#else


	Dev_Info_t* p = (Dev_Info_t *)(MLN_DEVINFO_ADDR);
	MLN_DevInfo = *p;

	/*For Card, the lowest Byte of Exit Addr should be 0x01 */
	HAL_ASSERT(MLN_DevInfo.ExitAddr[EXT_MACADDR_TYPE] == EXT_MACADDR_TYPE_LOCNODE);
	//MLN_version = MLN_DevInfo.ExitAddr[EXT_MACADDR_VERSION];
	MLN_DevShortAddr = BUILD_UINT16(MLN_DevInfo.ExitAddr[EXT_MACADDR_DEVID_LBYTE],MLN_DevInfo.ExitAddr[EXT_MACADDR_DEVID_HBYTE]);

#endif

}


void MLN_IntervalCheck(void)
{
#define 	MLN_MAX_INTERVAL	20000   // 20s

	static uint32 last_ticks = 0;

	/* read current ticks */
	uint32 ticks;
	((uint8 *) &ticks)[0] = ST0;
	((uint8 *) &ticks)[1] = ST1;
	((uint8 *) &ticks)[2] = ST2;
	((uint8 *) &ticks)[3] = 0;

	if(last_ticks !=0)
	{
		uint32 diff_ticks;
		if(ticks > last_ticks)
		{
			diff_ticks = ticks - last_ticks;
		}
		else
		{
			diff_ticks = 0x1000000 + ticks -last_ticks;
		}

		diff_ticks >>= 5; // convert  1/32k  to  ms,  diff_ticks = diff_ticks/32;

		if(diff_ticks > MLN_MAX_INTERVAL)  //  if interval > 20s, reset
		{
			MLN_Restart();
		}
	}
	last_ticks = ticks;
}

void MLN_PeriodReset()
{
#define MLN_PERIODRESET_NUM 	(10UL*24UL*60UL*60UL/MLN_LOCCAST_FREQ*1000UL)   // restart per 10 day

	static uint32 MLN_periodReset_cnt = 0;

	if(MLN_periodReset_cnt ++ > MLN_PERIODRESET_NUM)
	{
		MLN_Restart();
	}
}
void MLN_Restart()
{	
	EA = 0;
	//MLN_SaveParam2RAM();
	STARTWATCHDOG(DOGTIMER_INTERVAL_2MS);
	while(1);
}

#if 0
void MLN_SaveParam2RAM(void)
{
	Dev_Param_t param;
	param.MLN_sleeptime = MLN_sleeptime;
	//param.MLN_poll_interval = MLN_poll_interval;
	//param.MLN_poll_timeout = MLN_poll_timeout;
	param.MLN_WorkState =  MLN_WorkState;
	param.MLN_WorkState_bk = MLN_WorkState_bk;
	param.MLN_IsBatteryLow = MLN_IsBatteryLow;
	param.MLN_blast_cnt = MLN_blast_cnt;
	param.MLN_vddcheck_cnt = MLN_vddcheck_cnt;
	param.MLN_seqnum = MLN_seqnum;
	//param.MLN_AlertSuccess = MLN_AlertSuccess;
	//param.MLN_AlertCnt = MLN_AlertCnt;
	//param.MLN_urgent_cnt = MLN_urgent_cnt;
	param.MLN_seqnum = MLN_seqnum;

	*((Dev_Param_t*)(MLN_PARAM_ADDR)) = param; // save parameters to idata ram
}
void MLN_ReadParmFromRAM(void)
{
	Dev_Param_t DevParam = *((Dev_Param_t*)(MLN_PARAM_ADDR));
	MLN_sleeptime = DevParam.MLN_sleeptime;
	//MLN_poll_interval = DevParam.MLN_poll_interval;
	//MLN_poll_timeout = DevParam.MLN_poll_timeout;
	MLN_WorkState = DevParam.MLN_WorkState;
	MLN_WorkState_bk = DevParam.MLN_WorkState_bk;
	MLN_IsBatteryLow = DevParam.MLN_IsBatteryLow;
	MLN_blast_cnt  = DevParam.MLN_blast_cnt;
	MLN_vddcheck_cnt = DevParam.MLN_vddcheck_cnt;
	//MLN_AlertSuccess = DevParam.MLN_AlertSuccess;
	//MLN_AlertCnt = DevParam.MLN_AlertCnt;
	//MLN_urgent_cnt = DevParam.MLN_urgent_cnt;
	MLN_seqnum  = DevParam.MLN_seqnum;
}
#endif
uint32  MLN_GetTimeTick(void)
{
	uint32 ticks = 0;

	/* read the sleep timer; ST0 must be read first */
	((uint8 *) &ticks)[0] = ST0;
	((uint8 *) &ticks)[1] = ST1;
	((uint8 *) &ticks)[2] = ST2;
	((uint8 *) &ticks)[3] = 0;

	return ticks;
}

inline uint32 MLN_ConvTick2Us(uint32 ticks)
{
	uint32 time=0;
	time = ticks*125UL*125UL/512UL;  //unit us;
	return time;
}
uint8  MLN_GetBatteryValue(void)
{
#define MLN_VDD33V 0x0731
#define MLN_VDD32V 0x06FB
#define MLN_VDD31V 0x06C2
#define MLN_VDD30V 0x068C
#define MLN_VDD29V 0x0656
#define MLN_VDD28V 0x061D
#define MLN_VDD27V 0x05E6
#define MLN_VDD26V 0x05B1
#define MLN_VDD25V 0x0578
#define MLN_VDD24V 0x0544
#define MLN_VDD23V 0x050B
#define MLN_VDD22V 0x04DB
#define MLN_VDD21V 0x04A0
#define MLN_VDD20V 0x0468
#define MLN_VDD19V 0x0430
#define MLN_VDD18V 0x03FB

	uint16 BatteryList[] = {MLN_VDD33V,
		MLN_VDD32V,
		MLN_VDD31V,
		MLN_VDD30V,
		MLN_VDD29V,
		MLN_VDD28V,
		MLN_VDD27V,
		MLN_VDD26V,
		MLN_VDD25V,
		MLN_VDD24V,
		MLN_VDD23V,
		MLN_VDD22V,
		MLN_VDD21V,
		MLN_VDD20V,
		MLN_VDD19V,
		MLN_VDD18V};

	uint16 adcvalue  = HalAdcRead(HAL_ADC_CHANNEL_VDD3, HAL_ADC_RESOLUTION_12);

	for(uint8 i=0;i<sizeof(BatteryList)/sizeof(BatteryList[0]);i++)
	{
		if(adcvalue > BatteryList[i])
		{
			return (33-i);    //0->33 1->32 ... 3.3V, 3.2V
		}
	}
	return 18;            // default is 1.8V
}
/**************************************************************************************************
**************************************************************************************************/

