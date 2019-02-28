/**************************************************************************************************
Filename:       ChargeED.c
Revised:        $Date: 2010/12/08 22:58:17 $
Revision:       $Revision: 1.37 $

Description:    This file contains the application that can be use to set a device as End
Device from MAC directly which will communicate with a FFD with full z-stack.

**************************************************************************************************/
/**************************************************************************************************

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
#include "hal_led.h"
//#include "FlashUtil.h"
#include "MacUtil.h"

/* OS includes */
#include "OSAL.h"
#include "OSAL_Tasks.h"
#include "OSAL_Timers.h"
#include "OSAL_PwrMgr.h"

/* App Protocol*/
#include "AppProtocolWrapper.h"

/* Application Includes */
#include "OnBoard.h"

/* MAC Application Interface */
#include "mac_api.h"

/* Application */
#include "ChargeED_cfg.h"
#include "ChargeED.h"

/* watchdog util */
#include "watchdogutil.h"

#include "delay.h"

#include "drivers.h"

#ifdef DPRINT
#include "DPrint.h"
#include "stdio.h"
#endif
/**************************************************************************************************
*                                           Defines
**************************************************************************************************/
#define CHARGEED_TAG_LOCSYNC 	0
#define CHARGEED_TAG_POLL		1
#define CHARGEED_TAG_BLAST		2

#define CHARGEED_FIXTAG_LEN 			3	
#define CHARGEED_LOCNODE_TAG_LEN 	CHARGEED_MAX_SYNCNUM   // max sync LocNodes
#define CHARGEED_LOCTICK_LEN			2

#define CHARGEED_BATTERY_STAT_NORMAL 	0
#define CHARGEED_BATTERY_STAT_LOW	 	1
#define CHARGEED_BATTERY_STAT_VERYLOW 	2
/**************************************************************************************************
*                                           Typedefs
**************************************************************************************************/

typedef struct
{
	sAddrExt_t ExitAddr;
} Dev_Info_t;

typedef struct
{
	bool isworking;
	uint32 winstart;
	uint32 winend;
}time_fixtag_item_t;

typedef struct
{
	uint16 DevID;
	uint_8 vol; 
	uint_8 seq;
	int8	rssi;
	int8  health;
	bool hasBlasted;
	uint32 winstart;
	uint32 Locticks[CHARGEED_LOCTICK_LEN];
}time_loctag_item_t;


typedef struct
{
	time_fixtag_item_t fixtag[CHARGEED_FIXTAG_LEN];
	time_loctag_item_t loctag[CHARGEED_LOCNODE_TAG_LEN];
}time_tag_t;

typedef struct
{
	//bool    isFirst;
	uint16 blast_period;
	uint16 poll_interval;
	uint16 poll_timeout;
	uint16 seqnum;
}blast_param_t;

typedef struct
{
	uint8 desired_locnum;
	uint8 synced_locnum;
}sync_param_t;

typedef struct
{
	app_chargeed_ssReq_t ssReq;
	LocPos_t p[CHARGEED_LOCNODE_TAG_LEN];
}blast_buf_t;

/* a extra flag to indication the device is reseted by period reset func */
#define SETRAM_FLAG 0x1234

typedef struct
{
	/* key press */
	uint16   keypress_timeout;
	uint8   keypress_cnt;

	/* alert */
	bool     AlertSuccess;
	uint8    AlertCnt;

	/* status */
	uint8    workStatus;
	uint8    workStatus_bk;

	/* battery */
	uint8     BatteryStatus;

	/* mac */
	bool	    rx_on_idle;

	/* pressed OK */
	bool     isOKPressed;
	uint8    OKPressCnt;

}system_param_t;

typedef struct
{
	uint16  setram_flag;	
	blast_param_t blast_param;
	system_param_t system_param;
}Dev_Param_t;


/**************************************************************************************************
*                                           Constant
**************************************************************************************************/
#define CHARGEED_MAC_PAYLOAD_LEN			 127                  //mac payload length by datasheet.
#define CHARGEED_SHORTADDR_PANCOORD		 0x0000             /* Short Address of PAN Coordinator, will always be 0x0000 */

#define CHARGEED_KEY_OK    		 HAL_KEY_SW_7
#define CHARGEED_KEY_HELP        HAL_KEY_SW_6

/* Application Work State */
/* The priority is: Alert > Urgent > Normal */

#define CHARGEED_STATE_NORMAL		0
#define CHARGEED_STATE_URGENT		1
#define CHARGEED_STATE_ALERT	    		2


#define CHARGEED_PARAM_ADDR		0xFD00		//0xFD00-0xFD53 are used to store paramters
#define CHARGEED_DEVINFO_ADDR		0x7FF8

#define CHARGEED_MIN_POWER	25

#define CHARGEED_LED_BLUE     HAL_LED_1
#define CHARGEED_LED_RED       HAL_LED_2

#define CHARGEED_BEEPFREQ_ALERT 	10
#define CHARGEED_RETREATFREQ_ALERT 	20

/* Size table for MAC structures */
const CODE uint8 ChargeED_cbackSizeTable [] =
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
static uint16        ChargeED_PanId = CARD_NWK_ADDR;                      /* 0xFFF0 */
static uint16        ChargeED_PANCoordShortAddr = CHARGEED_SHORTADDR_PANCOORD; /* Always be 0x00 */
static uint16        ChargeED_ShortAddr = 0x00;             

/* TRUE and FALSE value */
static bool          ChargeED_MACTrue = TRUE;
//static bool          ChargeED_MACFalse = FALSE;

/* Beacon order */
static uint8 ChargeED_SuperFrameOrder = CHARGEED_MAC_SUPERFRAME_ORDER;
static uint8 ChargeED_BeaconOrder = CHARGEED_MAC_BEACON_ORDER;

/* Task ID */
uint8 ChargeED_TaskId;

/* Device Info from flash */
static Dev_Info_t ChargeED_DevInfo;

/*params */
static  time_tag_t  time_tag;
static blast_buf_t blast_buf;
static blast_param_t blast_param;
static sync_param_t sync_param;
static system_param_t system_param;
static bool 		isFirstBlast;
/**************************************************************************************************
*                                     Local Function Prototypes
**************************************************************************************************/
/* Setup routines */
static void     ChargeED_DeviceStartup(uint8* pData);

/* Support */
static void     ChargeED_blast(uint8 reqtype);
static void     ChargeED_ReadDevInfo(void);
#ifdef DEBUG
static void     ChargeED_SetDevInfo(uint8 type, void * pData);
#endif
static void	ChargeED_PeriodReset(void);
static void     ChargeED_Restart(void);
static void 	ChargeED_SaveParam2RAM(void);
static void 	ChargeED_ReadParmFromRAM(void);
static void 	ChargeED_StartSync(void);
static uint8  	ChargeED_ProcSync(app_LocNodeCast_t *p, int8 rssi);
static bool	ChargeED_SetFixedTimetag(uint8 tag,uint16 time);
//static void 	ChargeED_UnSetFixedTimetag(uint8 tag);
//static void 	ChargeED_UnSetLocTimetag(uint16 DevID);
static void 	ChargeED_ParseAppData(APPWrapper_t *p,int8 rssi);
static void	ChargeED_ProcWindowCast(app_LocNodeCast_t *p, int8 rssi);
//static uint8  ChargeED_UpdateSyncedNum(void);
static void 	ChargeED_BatteryCheck(void );
static uint8    ChargeED_GetBatteryValue(void);
//static void	ChargeED_LocSortByRSSI(LocPos_t* LocNode, int8 LastCnt);
static uint32 	ChargeED_GetWinStart(uint32 Locticks[]);
static void 	ChargeED_SetAlert(void);
static void 	ChargeED_UnSetAlert(void);
static void 	ChargeED_SetRetreat(void);
static void 	ChargeED_UnSetRetreat(void);
static void 	ChargeED_ContinueEvents(void);
/**************************************************************************************************
*
* @fn          ChargeED_Init
*
* @brief       Initialize the application
*
* @param       taskId - taskId of the task after it was added in the OSAL task queue
*
* @return      none
*
**************************************************************************************************/
void ChargeED_Init(uint8 taskId)
{       
	/* Initialize the task id */
	ChargeED_TaskId = taskId;

	/* initialize MAC features */
	MAC_Init();
	MAC_InitDevice();

	/* Reset the MAC */
	MAC_MlmeResetReq(TRUE);
	MAC_PwrOnReq();

	/* initial MacUtil*/
	MacUtil_t Macutil;
	Macutil.panID = 0xFFFF;                          // Card broadcast to all PANs
	Macutil.dst_endpoint = APS_DST_ENDPOINT;
	Macutil.src_endpoint = APS_SRC_ENDPOINT;
	Macutil.cluster_id = APS_CLUSTER_ID;
	Macutil.profile_id = APS_PROFILE_ID;
	Macutil.NodeType =  NODETYPE_DEVICE;
	MAC_UTIL_INIT(&Macutil);

	/* power saving */
	osal_pwrmgr_init();
	osal_pwrmgr_device(PWRMGR_BATTERY);

	/* fill params */
	blast_param.blast_period = CHARGEED_BLAST_PERIOD;
	blast_param.poll_interval = CHARGEED_POLL_INTERVAL;
	blast_param.poll_timeout = CHARGEED_POLL_TIMEOUT;
	blast_param.seqnum = 0;

	osal_memset(&system_param,0,sizeof(system_param) );
	system_param.AlertSuccess = false;
	system_param.AlertCnt = 0;
	system_param.workStatus = CHARGEED_STATE_NORMAL;
	system_param.workStatus_bk = CHARGEED_STATE_NORMAL;

	system_param.BatteryStatus = CHARGEED_BATTERY_STAT_NORMAL;
	system_param.isOKPressed = false;
	system_param.OKPressCnt	= 0;

	osal_memset(&time_tag,0,sizeof(time_tag) );

	ChargeED_ReadDevInfo();

	ChargeED_DeviceStartup(NULL);

	/* turn off leds */
	HalLedSet(CHARGEED_LED_BLUE, HAL_LED_MODE_OFF);
	HalLedSet(CHARGEED_LED_RED, HAL_LED_MODE_OFF);

#ifdef DPRINT
	DPrint_init();
#endif

	/* start first sync */
	ChargeED_StartSync();

	/*Start Watch Dog*/
#ifdef WATCHDOG
	StartWatchDog(DOGTIMER_INTERVAL_1S);
#endif


	/* flash once to fix restart bugs!!! */
	HalLedBlink(CHARGEED_LED_BLUE, 1, 50, 10);

	/* if reset from watchdog,continue events */
	if(GetResetFlag() == RESET_FLAG_WATCHDOG)
	{
		ChargeED_ContinueEvents();
	}
#ifdef USE_STATE_UNINIT
	if(ZSuccess!=osal_set_event(ChargeED_TaskId, CHARGEED_BLAST_EVENT))
	{
		SystemReset();
	}
#else 
	/* set first blast */
	isFirstBlast = true;
	if(ZSuccess!=osal_set_event(ChargeED_TaskId, CHARGEED_BLAST_EVENT))
	{
		SystemReset();
	}
#endif 

}

/**************************************************************************************************
*
* @fn          ChargeED_ProcessEvent
*
* @brief       This routine handles events
*
* @param       taskId - ID of the application task when it registered with the OSAL
*              events - Events for this task
*
* @return      16bit - Unprocessed events
*
**************************************************************************************************/
uint16 ChargeED_ProcessEvent(uint8 taskId, uint16 events)
{              
	uint8* pMsg;
	macCbackEvent_t* pData;

	/* process timetick every 1 ms*/
	if(events  & CHARGEED_PROCTIMETICK_EVENT)
	{
		ChargeED_ProcTimeTick();
		return events ^ CHARGEED_PROCTIMETICK_EVENT;
	}

	/* system message */
	if (events & SYS_EVENT_MSG)
	{
		while ((pMsg = osal_msg_receive(ChargeED_TaskId)) != NULL)
		{
			switch ( *pMsg )
			{

			case MAC_MCPS_DATA_CNF:
				{
					pData = (macCbackEvent_t *) pMsg;
					/*
					static uint8 fail_time;
					uint8 status;                
					status = pData->dataCnf.hdr.status;
					if(status == MAC_SUCCESS)
					{
					fail_time = 0;
					}
					else
					{
					if(fail_time++ > CHARGEED_MAX_BLAST_FAIL)
					{
					ChargeED_SaveParam2RAM();
					SystemReset();
					}
					}*/
					osal_msg_deallocate((uint8 *) pData->dataCnf.pDataReq);
					break;
				}

			case MAC_MCPS_DATA_IND:
				/* Proess Command */
				//DPrint_PrintTimeInterVal();
				pData = (macCbackEvent_t *) pMsg;
				uint8 cluster_id = MAC_UTIL_GetClusterID(pData->dataInd.msdu);
				if(cluster_id== CHARGEED_CLUSTERID ||cluster_id== LOCNODE_CLUSTERID)
				{
					sData_t AppData = MAC_UTIL_RemoveHeader(pData->dataInd.msdu);
					int8 rssi = pData->dataInd.mac.rssi;

					if(NUMBER_IS_LOCNODE(pData->dataInd.mac.srcAddr.addr.shortAddr))
					{
						rssi += A_SUBSTATION-A_LOCNODE; //normalization to substation value
					}
					ChargeED_ParseAppData((APPWrapper_t*) AppData.p, rssi);
				}
				break;
			}

			/* Deallocate */
			osal_msg_deallocate((uint8 *) pMsg);
		}

		return events ^ SYS_EVENT_MSG;

	}

	/* restart */
	if(events & CHARGEED_RESET_EVENT)
	{
		ChargeED_Restart();
		return events ^ CHARGEED_RESET_EVENT;
	}

	/* alert */
	if(events & CHARGEED_ALERT_EVENT)
	{ 
		const uint16 AlertInterval = 1000;  // 1s

		if(system_param.workStatus == CHARGEED_STATE_ALERT)
		{
			if(system_param.AlertCnt--!= 0)
			{
				/* Send Alerts */
				app_Urgent_t Appdata;
				Appdata.msgtype = URGENT;
				Appdata.urgenttype = ALERT;
				Appdata.value = 0;
				ChargeED_SetFixedTimetag(CHARGEED_TAG_BLAST, CHARGEED_POLL_TIMEOUT);
				MAC_UTIL_BuildandSendData((uint8 *)&Appdata,sizeof(app_Urgent_t),  MAC_UTIL_UNICAST, 0, NULL);

				/* blink LEDS */
				if(system_param.AlertCnt < 3 && system_param.AlertSuccess )   // if success, last 3 seconds flash blue
				{
					HalLedBlink(CHARGEED_LED_BLUE, 3, 50, 300);
				}
				else
				{
					HalLedBlink(CHARGEED_LED_RED, 3, 50, 300);
				}
				if(ZSuccess!= osal_start_timerEx(ChargeED_TaskId, CHARGEED_ALERT_EVENT, AlertInterval))
				{
					SystemReset();
				}
			}
			else // alert end
			{
				ChargeED_UnSetAlert();
			}
		}
		return events ^ CHARGEED_ALERT_EVENT;
	}

	/* retreat loop, never stop */
	if(events & CHARGEED_RETREAT_EVENT)
	{
		if(system_param.workStatus == CHARGEED_STATE_URGENT && !system_param.isOKPressed)
		{
			/* Blink LED */
			HalLedBlink(CHARGEED_LED_RED, 10, 50, 480);
			if(!HalBeeperBusy())
			{
				HalStartBeeper(BEEP_TYPE_RETREAT,CHARGEED_RETREATFREQ_ALERT);
			}
		}

		if(ZSuccess!=osal_start_timerEx(ChargeED_TaskId, CHARGEED_BLAST_EVENT, blast_param.blast_period))
		{
			SystemReset();
		}
		return events ^ CHARGEED_RETREAT_EVENT;
	}

	/* blast loop, never stop */
	if(events & CHARGEED_BLAST_EVENT)

	{	
	       /* process battery check */
		ChargeED_BatteryCheck();
		   
		/* normal or OK pressed */
		if(system_param.workStatus == CHARGEED_STATE_NORMAL || 
			(system_param.workStatus == CHARGEED_STATE_URGENT && system_param.isOKPressed))
		{
			/*if battery is  low, do not flash the blue LED, flash red LED*/
			/*if battery is  very low, do not flash any LED*/
			if(!isFirstBlast && system_param.BatteryStatus ==CHARGEED_BATTERY_STAT_NORMAL)
			{
				HalLedSet(CHARGEED_LED_BLUE, HAL_LED_MODE_ON);
			}
			if(HalBeeperBusy())
			{
				HalStopBeeper(NULL, true);
			}

			/* if OK pressed, set URGET to NORMAL*/
			system_param.workStatus = CHARGEED_STATE_NORMAL;
		}
		/* Send out  message */
		if(isFirstBlast == true)
		{
			isFirstBlast = false;
		}
		else
		{
			if(blast_param.seqnum % CHARGEED_POLL_INTERVAL == 0)
			{
				ChargeED_SetFixedTimetag(CHARGEED_TAG_POLL, CHARGEED_POLL_TIMEOUT);
				ChargeED_blast(SSREQ_POLL);

				/* start count the sesired_locnum*/
				sync_param.desired_locnum = 0;
			}
			else
			{
				ChargeED_blast(SSREQ_OUT);
			}     

		}

		/* start  process retreat */
		if(ZSuccess !=osal_set_event(ChargeED_TaskId, CHARGEED_RETREAT_EVENT))
		{
			SystemReset();
		}
		return events ^ CHARGEED_BLAST_EVENT;

	}

	/* test key long press */
	if(events & CHARGEED_KEY_LONGPRESS_EVENT)
	{
		const uint16 testInterval = 300;   // test once each 300 ms
		uint16 testnum = system_param.keypress_timeout / testInterval;
		uint8 keys = HalKeyRead();

		if(system_param.keypress_cnt++ < testnum)
		{
			if(keys !=0 )  
			{
				if(ZSuccess!=osal_start_timerEx(ChargeED_TaskId, CHARGEED_KEY_LONGPRESS_EVENT, testInterval))
				{
					SystemReset();
				}
			}
		}
		else // detected a long press
		{
			if(keys & CHARGEED_KEY_HELP)  // start a alert
			{
				ChargeED_SetAlert();
			}
		}
		return events ^ CHARGEED_KEY_LONGPRESS_EVENT;
	}

	/* time counter of retreat suppress */
	if(events & CHARGEED_OK_PRESS_EVENT)
	{
		if(system_param.OKPressCnt-- != 0)
		{
			if(ZSuccess!= osal_start_timerEx(ChargeED_TaskId, CHARGEED_OK_PRESS_EVENT, 60000))
			{
				SystemReset();
			}
		}
		else  // suppress end
		{
			system_param.isOKPressed = false;
			system_param.OKPressCnt = 0;
		}

		return events ^ CHARGEED_OK_PRESS_EVENT;
	}

#ifdef USE_STATE_UNINIT
	if(events & CHARGEED_UNINIT_EVENT)
	{

		return events ^ CHARGEED_UNINIT_EVENT;
	}
#endif 

	/* do nothing, only make osal to wake, must set to the last one !!!*/
	if(events & CHARGEED_NEXTTICK_EVENT)
	{
		return events ^ CHARGEED_NEXTTICK_EVENT;
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

	uint8 len = ChargeED_cbackSizeTable[pData->hdr.event];

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
		//DPrint_PrintTimeInterVal();
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
		osal_msg_send(ChargeED_TaskId, (byte *) pMsg);
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
* @fn      ChargeED_DeviceStartup(uint8* pData)
*
* @brief   Update the timer per tick
*
* @param   beaconEnable: TRUE/FALSE
*
* @return  None
*
**************************************************************************************************/
void ChargeED_DeviceStartup(uint8* pData)
{
	MAC_MlmeSetReq(MAC_EXTENDED_ADDRESS, &ChargeED_DevInfo.ExitAddr);

	/* Setup MAC_BEACON_PAYLOAD_LENGTH */
	//MAC_MlmeSetReq(MAC_BEACON_PAYLOAD_LENGTH, &ChargeED_BeaconPayloadLen);

	/* Setup MAC_BEACON_PAYLOAD */
	//MAC_MlmeSetReq(MAC_BEACON_PAYLOAD, &ChargeED_BeaconPayload);

	/* Setup PAN ID */
	MAC_MlmeSetReq(MAC_PAN_ID,&ChargeED_PanId);

	/* This device is setup for Direct Message */
	system_param.rx_on_idle = true;
	MAC_MlmeSetReq(MAC_RX_ON_WHEN_IDLE, &system_param.rx_on_idle);

	/* Setup Coordinator short address */
	MAC_MlmeSetReq(MAC_COORD_SHORT_ADDRESS, &ChargeED_PANCoordShortAddr);

	/* Setup Beacon Order */
	MAC_MlmeSetReq(MAC_BEACON_ORDER, &ChargeED_BeaconOrder);

	/* Setup Super Frame Order */
	MAC_MlmeSetReq(MAC_SUPERFRAME_ORDER, &ChargeED_SuperFrameOrder);


	//uint8 tmp8 = ChargeED_MAC_CHANNEL;
	uint8 channel = ChargeED_DevInfo.ExitAddr[EXT_MACADDR_CHANNEL];
	MAC_MlmeSetReq(MAC_LOGICAL_CHANNEL,&channel);

	ChargeED_ShortAddr = BUILD_UINT16(ChargeED_DevInfo.ExitAddr[EXT_MACADDR_DEVID_LBYTE],ChargeED_DevInfo.ExitAddr[EXT_MACADDR_DEVID_HBYTE]);
	MAC_MlmeSetReq(MAC_SHORT_ADDRESS,&ChargeED_ShortAddr);

	MAC_MlmeSetReq(MAC_ASSOCIATED_PAN_COORD,&ChargeED_MACTrue);

}
void ChargeED_ParseAppData(APPWrapper_t *p, int8 rssi)
{
	switch(*((MSGType *)p))
	{
	case URGENT:
		switch(((app_Urgent_t*)p)->urgenttype)
		{
		case    RETREAT:
			ChargeED_SetRetreat();
			break;
		case    CANCELRETREAT:
			ChargeED_UnSetRetreat();
			break;
		case   ALERTACK:
			system_param.AlertSuccess = TRUE;
			break;
		}
		break;

	case CHARGEED_SSRSP:
		{
			app_chargeed_ssRsp_t *pssRsp = (app_chargeed_ssRsp_t*)p;
			if(time_tag.fixtag[CHARGEED_TAG_POLL].isworking == true)
				//if(pssRsp->seqnum == blast_param.seqnum)
			{
				sync_param.desired_locnum += pssRsp->locnode_num;
			}
			if(pssRsp->urgent_type == RETREAT)
			{
				ChargeED_SetRetreat();
			}
			else if(pssRsp->urgent_type == CANCELRETREAT)
			{
				ChargeED_UnSetRetreat();
			}
			break;
		}
	case  NODESLEEP:
		{
			/*
			SleepType  type = ((app_Sleep_t*)p)->sleeptype;
			if(type == SLEEPTIME)
			blast_param.blast_period = ((app_Sleep_t*)p)->value;
			else if(type == POLL_INTERVAL)
			blast_param.poll_interval = ((app_Sleep_t*)p)->value;
			else if(type == POLL_TIMEOUT)
			blast_param.poll_timeout= ((app_Sleep_t*)p)->value;
			HalLedBlink(CHARGEED_LED_RED, 3, 50, 300);

			*/
			break;
		}
	case PWRCTL:
		{
			uint8 power = ((app_PWRCTL_t*)p)->workpower;
			if(power == 0)
			{
				MAC_MlmeSetReq(MAC_PHY_TRANSMIT_POWER, &power);
			}
			else if(power <= CHARGEED_MIN_POWER)
			{
				power+=5;      //see mac_radio_defs.c      line 56
				MAC_MlmeSetReq(MAC_PHY_TRANSMIT_POWER, &power);
			}

			break;
		}
	case LOCNODE_CAST:
		{
			app_LocNodeCast_t * pLocNodeCast = (app_LocNodeCast_t *)p;


			if(time_tag.fixtag[CHARGEED_TAG_LOCSYNC].isworking)   //is syncing
			{
				ChargeED_ProcSync(pLocNodeCast, rssi);
			}
			else   // not syncing
			{
				ChargeED_ProcWindowCast(pLocNodeCast, rssi);
			}
			if(pLocNodeCast->vol < MLN_VDD_LIMT && system_param.BatteryStatus!= CHARGEED_BATTERY_STAT_VERYLOW)
			{
				app_LocNodeAlarm_t app_LocNodeAlarm;
				app_LocNodeAlarm.msgtype = LOCNODE_ALARM;
				app_LocNodeAlarm.DevID = ChargeED_ShortAddr;
				app_LocNodeAlarm.DstPan = 0xFFFF;
				app_LocNodeAlarm.seq = pLocNodeCast->seq;
				app_LocNodeAlarm.AlarmType = LOCNODE_LOW_BATTERY;
				app_LocNodeAlarm.LocNodeID = pLocNodeCast->DevID;
				app_LocNodeAlarm.value = pLocNodeCast->vol;

				ChargeED_SetFixedTimetag(CHARGEED_TAG_BLAST, 5);
				MAC_UTIL_BuildandSendData((uint8 *)&app_LocNodeAlarm,sizeof(app_LocNodeAlarm_t),  MAC_UTIL_UNICAST, 0, NULL);
			}
			break;
		}
	default:
		break;

	}

}

uint8  ChargeED_ProcSync(app_LocNodeCast_t *p, int8 rssi)
{
	if(p->DevID == 0)
	{	
		return ZFailure;
	}

	/* find indexs */
	int8 replace_idx = -1; 
	int8 insert_idx = -1; 
	int8 minrssi_idx = 0;
	for(uint8 i=0; i<CHARGEED_LOCNODE_TAG_LEN; i++)
	{
		if(time_tag.loctag[i].DevID>0 && p->DevID ==  time_tag.loctag[i].DevID)   
		{
			replace_idx = i;
		}
		if(time_tag.loctag[i].DevID == 0 && insert_idx <0)
		{
			insert_idx = i;
		}
		if(time_tag.loctag[i].rssi < time_tag.loctag[minrssi_idx].rssi)
		{
			minrssi_idx = i;
		}
	}


	int8  idx= -1;
	if(replace_idx>=0) /* if the id is already exist, replace it */
	{
		idx = replace_idx;
	}
	else if(insert_idx>=0) /* if the id is not exist, insert it */
	{
		idx =  insert_idx;
	}
	else /* if all is full, replace it with a min rssi */
	{
		if(rssi > time_tag.loctag[minrssi_idx].rssi)
		{
			idx = minrssi_idx;
		}
	}

	if(idx >= 0)
	{
		time_tag.loctag[idx].DevID= p->DevID;			
		time_tag.loctag[idx].vol = p->vol;
		time_tag.loctag[idx].seq = p->seq;
		time_tag.loctag[idx].rssi = rssi;
		time_tag.loctag[idx].hasBlasted = false;
		time_tag.loctag[idx].health = CHARGEED_HEALTH_MAX;
		time_tag.loctag[idx].winstart = ChargeED_GetWinStart(time_tag.loctag[idx].Locticks);
	}

	return ZSuccess;
}

void ChargeED_ProcWindowCast(app_LocNodeCast_t *p, int8 rssi)
{
	ChargeED_ProcSync(p, rssi);
}
/**************************************************************************************************
*
* @fn      ChargeED_HandleKeys
*
* @brief   Callback service for keys
*
* @param   keys  - keys that were pressed
*          state - shifted
*
* @return  void
*
**************************************************************************************************/
void ChargeED_HandleKeys(uint16 keys, uint8 shift)
{
	if(keys & CHARGEED_KEY_OK)
	{
		system_param.isOKPressed = TRUE;
		system_param.OKPressCnt = CHARGEED_RETREATSUPPRESS_TIME;
		osal_set_event(ChargeED_TaskId, CHARGEED_OK_PRESS_EVENT);
		if(system_param.workStatus == CHARGEED_STATE_URGENT)
		{
			if(HalBeeperBusy())
			{
				HalStopBeeper(NULL, true);
			}

			/* if OK pressed, set URGET to NORMAL*/
			system_param.workStatus = CHARGEED_STATE_NORMAL;
			HalLedSet(CHARGEED_LED_RED, HAL_LED_MODE_OFF);
		}
	}

	/* process long press for all keys */
	system_param.keypress_timeout = CHARGEED_KEY_LONGPRESS_TIME;
	system_param.keypress_cnt = 0;
	osal_set_event(ChargeED_TaskId,CHARGEED_KEY_LONGPRESS_EVENT);
}

/**************************************************************************************************
*
* @fn      ChargeED_blast(void)
*
* @brief   Blast once to all Coords and routers without ACK or retrans.
*
* @param   
*              
* @return  
*
**************************************************************************************************/

void ChargeED_blast(uint8 reqtype)
{	
	ChargeED_PeriodReset();

	//MAC_PwrOnReq(); 
	//system_param.rx_on_idle = true;
	//MAC_MlmeSetReq(MAC_RX_ON_WHEN_IDLE, &system_param.rx_on_idle );

	/*the data is in time_tag.loctag, copy to blast_buffer*/
	uint8 idx = 0;
	for(uint8 i=0; i<CHARGEED_LOCNODE_TAG_LEN; i++)
	{
		if(time_tag.loctag[i].DevID >0) 
		{
			if(time_tag.loctag[i].hasBlasted== false)   
			{
				blast_buf.p[idx].LocNode_ID = time_tag.loctag[i].DevID;
				blast_buf.p[idx].RSSI = time_tag.loctag[i].rssi;
				time_tag.loctag[i].hasBlasted = true;
				idx++;
			}

			/* if the health is poor, unset this window */
			if(--time_tag.loctag[i].health <= 0)
			{
				time_tag.loctag[i].DevID = 0;
				for(uint8 i=0;i<CHARGEED_LOCTICK_LEN;i++)
				{
					time_tag.loctag[i].Locticks[i] = 0;
				}
			}
		}
	}
	/* sort the rssi */
	/* sort on the substation Board  */
	// ChargeED_LocSortByRSSI(blast_buf.p, idx);

	/* fill the head */
	blast_buf.ssReq.msgtype = CHARGEED_SSREQ;
	blast_buf.ssReq.reqtype = reqtype;
	blast_buf.ssReq.srcPan= CARD_NWK_ADDR;
	blast_buf.ssReq.seqnum = blast_param.seqnum++;
	blast_buf.ssReq.LocCnt= idx;

	/* update synced_num before blast*/
	sync_param.synced_locnum = idx;

#ifdef DPRINT
	char p[8];
	sprintf(p, "B%d:",idx);
	DPrint(p);
#endif

	/* send out */
	if(system_param.BatteryStatus != CHARGEED_BATTERY_STAT_VERYLOW)
	{

		uint8 len = sizeof(blast_buf.ssReq) + sizeof(LocPos_t)*idx;
		ChargeED_SetFixedTimetag(CHARGEED_TAG_BLAST,5);
		MAC_UTIL_BuildandSendData((uint8 *)&blast_buf,len, MAC_UTIL_UNICAST, 0, NULL);
	}

}


#if 0
/* timeout is  in ms */
void ChargeED_Delay( uint16 timeout )
{
	uint16 i,j,k;
	uint16 timeBig =  timeout >> 9;
	uint16 timeSmall = timeout - timeBig*512;
	for(i=0;i<timeBig;i++)
	{
#ifdef WATCHDOG
		FeedWatchDog();
#endif
		//ChargeED_FeedWatchDog(); // feed dog every 512 ms
		for(j = 0;j< 512;j++)
		{
			/* One Nop counts 12/32M, So 889  cyc is a ms*/
			k = 880;//k = 889;
			while (k--)
			{
				asm("NOP");
				asm("NOP");
				asm("NOP");
			}
		}
	}
#ifdef WATCHDOG
	FeedWatchDog();
#endif
	//ChargeED_FeedWatchDog();
	for (i=0;i<timeSmall;i++)
	{
		k = 880;//k = 889;
		while (k--)
		{
			asm("NOP");
			asm("NOP");
			asm("NOP");
		}
	}
}
#endif

void ChargeED_ReadDevInfo()
{
#if 0
	/* Make a fake DEV Info*/
	ChargeED_DevInfo.ExitAddr[0] = 0x23;
	ChargeED_DevInfo.ExitAddr[1] = 0x04;
	ChargeED_DevInfo.ExitAddr[2] = 0xFF;
	ChargeED_DevInfo.ExitAddr[3] = 0xFF;
	ChargeED_DevInfo.ExitAddr[4] = 0x01;
	ChargeED_DevInfo.ExitAddr[5] = 0x01;
	ChargeED_DevInfo.ExitAddr[6] = 0x0E;
	ChargeED_DevInfo.ExitAddr[7] = 0xFF;
#else
	Dev_Info_t* p = (Dev_Info_t *)(CHARGEED_DEVINFO_ADDR);
	ChargeED_DevInfo = *p;

	if(GetResetFlag() == RESET_FLAG_WATCHDOG)
	{
		ChargeED_ReadParmFromRAM();
	}
	/*For Card, the lowest Byte of Exit Addr should be 0x01 */
	HAL_ASSERT(ChargeED_DevInfo.ExitAddr[EXT_MACADDR_TYPE] == EXT_MACADDR_TYPE_CARD);
#endif
}

#if 0
void ChargeED_IntervalCheck(void)
{
#define 	ChargeED_MAX_INTERVAL	20000   // 20s

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

		if(diff_ticks > ChargeED_MAX_INTERVAL)  //  if interval > 20s, reset
		{
			ChargeED_Restart();
		}
	}
	last_ticks = ticks;
}
#endif 
void ChargeED_PeriodReset()
{
	//#define CHARGEED_PERIODRESET_NUM 	(10UL*24UL*3600UL/(CHARGEED_BLAST_PERIOD/1000UL))   // restart per 10 day

#define CHARGEED_PERIODRESET_NUM 	(15UL*60UL*1000UL/CHARGEED_BLAST_PERIOD) 	// 15min

	static uint32 ChargeED_periodReset_cnt = 0;

	ChargeED_SaveParam2RAM();
	if(ChargeED_periodReset_cnt++ > CHARGEED_PERIODRESET_NUM)
	{
		osal_start_timerEx(ChargeED_TaskId,CHARGEED_RESET_EVENT, 25);
	}
}
void ChargeED_Restart()
{	
	EA = 0;
	ChargeED_SaveParam2RAM();
	STARTWATCHDOG(DOGTIMER_INTERVAL_2MS);  //set to 15ms to ensure the last frame is send out
	while(1);
}

void ChargeED_SaveParam2RAM(void)
{

	Dev_Param_t param;
	param.setram_flag = SETRAM_FLAG;
	param.blast_param = blast_param;
	param.system_param = system_param;

	*((Dev_Param_t*)(CHARGEED_PARAM_ADDR)) = param; // save parameters to idata ram

}
void ChargeED_ReadParmFromRAM(void)
{
	Dev_Param_t param = *((Dev_Param_t*)(CHARGEED_PARAM_ADDR));

	if(param.setram_flag == SETRAM_FLAG)
	{
		system_param = param.system_param;
		blast_param = param.blast_param;
	}
	osal_memset((void* )CHARGEED_PARAM_ADDR,0,sizeof(Dev_Param_t));
}

bool ChargeED_SetFixedTimetag(uint8 tag,uint16 time)
{
	if(tag >= CHARGEED_FIXTAG_LEN)
	{
		return false;
	}

	uint32 tick = osal_GetSystemClock();

	time_tag.fixtag[tag].isworking = true;
	time_tag.fixtag[tag].winstart = tick;
	time_tag.fixtag[tag].winend = tick + time;
	return true;
}
/*
void ChargeED_UnSetFixedTimetag(uint8 tag)
{
time_tag.fixtag[tag].isworking = false;
time_tag.fixtag[tag].winstart = 0;
time_tag.fixtag[tag].winend = 0;
return;
}*/

/*
void ChargeED_UnSetLocTimetag(uint16 DevID)
{
for(uint8 i=0;i<CHARGEED_LOCNODE_TAG_LEN;i++)
{
if(time_tag.loctag[i].DevID == DevID)
{
time_tag.loctag[i].DevID= 0;
time_tag.loctag[i].health= 0;
for(uint8 i=0;i<CHARGEED_LOCTICK_LEN;i++)
{
time_tag.loctag[i].Locticks[i] = 0;
}
}
}
return;
}
*/
void ChargeED_ProcTimeTick(void)
{
	//static bool isMacOn = true;

	uint32 minnexttick = 0xFFFFFFFF;
	bool canSleep = true;

	uint32 tick = osal_GetSystemClock();
#ifdef WATCHDOG
	static uint8 watchdog_tick;
	if(watchdog_tick++ == 0)
	{
		FeedWatchDog();
	}
#endif

	/* process fixed tags */
	for(uint8 i=0;i<CHARGEED_FIXTAG_LEN;i++)
	{
		if(time_tag.fixtag[i].isworking == true) 
		{					 
			if(tick<time_tag.fixtag[i].winend)  //busying
			{
				canSleep = false;
			}
			else //exactly at the end
			{	
				time_tag.fixtag[i].isworking = false;

				/* process event at the end */
				if(i== CHARGEED_TAG_POLL &&  blast_buf.ssReq.reqtype == SSREQ_POLL)   // at the poll end
				{
					static uint8 sync_seqnum;
					if(sync_param.desired_locnum > 0    // there is some loc node
						//&& sync_param.synced_locnum < CHARGEED_MIN_SYNCNUM  // synced num is too small
						//&& sync_param.desired_locnum > sync_param.synced_locnum // and not all synced
						)
					{
						if(sync_seqnum++ % CHARGEED_SYNC_INTERVAL == 0) 
						{
							ChargeED_StartSync();
							canSleep = false;
						}
					}
					else  // do not need sync 
					{

						sync_seqnum = 0;
					}
				}
				else if(i== CHARGEED_TAG_LOCSYNC) // sync end
				{
					asm("nop");
#ifdef DPRINT
					DPrint("D");
#endif

				}
				else if(i== CHARGEED_TAG_BLAST && system_param.workStatus == CHARGEED_STATE_NORMAL) // blast end
				{
					/* in normal mode  turn off all */
					HalLedSet(CHARGEED_LED_BLUE, HAL_LED_MODE_OFF);
					HalLedSet(CHARGEED_LED_RED, HAL_LED_MODE_OFF);
				}
			}
		}
		else  //if not woring
		{
		}
	}

	/* process loc tags */
	for(uint8 i=0; i<CHARGEED_LOCNODE_TAG_LEN; i++)
	{		
		uint32 endtick;
		if(time_tag.loctag[i].DevID > 0)
		{
			endtick  = time_tag.loctag[i].winstart + CHARGEED_SYNC_WINLEN;

			if(tick < time_tag.loctag[i].winstart)  //before window
			{
				minnexttick = MIN(minnexttick, time_tag.loctag[i].winstart);
			}
			else if(tick ==  time_tag.loctag[i].winstart) // window start
			{
				canSleep = false;
#ifdef DPRINT
				static uint32 winstart_time;
				winstart_time = osal_GetSystemClock();
				DPrint("S");
#endif
			}
			else if(tick < endtick) // in window
			{
				canSleep = false;
#ifdef DPRINT
				DPrint("I");
#endif
			}
			else
			{
#ifdef DPRINT
				static uint32 winend_time;
				winend_time = osal_GetSystemClock();
				DPrint("E");
#endif
				time_tag.loctag[i].winstart += CHARGEED_RECV_PERIOD;
				minnexttick = MIN(minnexttick, time_tag.loctag[i].winstart );
			}
		}
	}

	/*process sleep */
	//tick = osal_GetSystemClock();
	if(canSleep == true && !HalBeeperBusy())
	{	
		if(minnexttick>tick  && minnexttick < 0xFFFFFFFF)
		{
			osal_start_timerEx(ChargeED_TaskId, CHARGEED_NEXTTICK_EVENT, minnexttick-tick);
		}
		if(system_param.rx_on_idle  == true)
		{
			system_param.rx_on_idle  = false;
			MAC_MlmeSetReq(MAC_RX_ON_WHEN_IDLE, &system_param.rx_on_idle );
		}
		osal_pwrmgr_task_state(ChargeED_TaskId, PWRMGR_CONSERVE);

	}
	else
	{
		if(system_param.rx_on_idle  == false)
		{
			system_param.rx_on_idle  = true;
			MAC_MlmeSetReq(MAC_RX_ON_WHEN_IDLE, &system_param.rx_on_idle );
		}
		osal_pwrmgr_task_state(ChargeED_TaskId, PWRMGR_HOLD);
	}

	return;
}

void ChargeED_StartSync(void)
{
	osal_memset(&time_tag.loctag, 0, sizeof(time_tag.loctag));

	sync_param.synced_locnum = 0;

	ChargeED_SetFixedTimetag(CHARGEED_TAG_LOCSYNC,  MLN_LOCCAST_FREQ); //sync for 300+50ms

#ifdef DPRINT
	DPrint("C");
#endif
	return;
}
/*
uint8 ChargeED_UpdateSyncedNum(void)
{
uint8 num = 0;
for(uint8 i=0;i<CHARGEED_LOCNODE_TAG_LEN;i++)
{
if(time_tag.loctag[i].DevID>0 && time_tag.loctag[i].hasBlasted == false)
{	
num++;
}
}
return num;
}
*/
void ChargeED_BatteryCheck(void )
{
	/*battery check */
	static uint8 vddlowcnt =0;
	static uint8 vddnormolcnt=0;
	uint8 vdd;
	// if(!(vddcheck_cnt++ & 0x1F))   // 32 blast time
	{
		vdd = ChargeED_GetBatteryValue();

		if(vdd  < CHARGEED_VDD_LIMT_VERYLOW)
		{
		        vddlowcnt =0;
	             vddnormolcnt=0;
			system_param.BatteryStatus = CHARGEED_BATTERY_STAT_VERYLOW;
		}
		else if(vdd  < CHARGEED_VDD_LIMT_LOW)
		{
		         vddlowcnt++;
		      vddnormolcnt=0;			
		 				
			 if(vddlowcnt%5==0)
			 	{
			          system_param.BatteryStatus = CHARGEED_BATTERY_STAT_LOW;
				   vddlowcnt=0;
			     	}
		}
		else if(system_param.BatteryStatus !=  CHARGEED_BATTERY_STAT_NORMAL && vdd >= CHARGEED_VDD_LIMT_NORMAL)
		{
			vddnormolcnt++;
		      vddlowcnt=0;	
			  
			  if(vddnormolcnt%5==0)
			     	{
			          system_param.BatteryStatus = CHARGEED_BATTERY_STAT_NORMAL; 
				   vddnormolcnt=0;
			     	}
		}
		else /* BATTERY_STAT_NORMAL */
		{	
		     vddlowcnt =0;
	             vddnormolcnt=0;
		}
	}

	/* if low battery, report.  if very low, do nothing */
	if(system_param.workStatus == CHARGEED_STATE_NORMAL && system_param.BatteryStatus == CHARGEED_BATTERY_STAT_LOW)
	{
		HalLedSet(CHARGEED_LED_BLUE, HAL_LED_MODE_OFF);
		HalLedBlink(CHARGEED_LED_RED, 1, 50, 300);

		APPWrapper_t    Appdata;
		Appdata.app_Urgent.msgtype = URGENT;
		Appdata.app_Urgent.urgenttype = NOPWR;
		Appdata.app_Urgent.value = vdd;                

		/* 300 ms avoid sleep */
		ChargeED_SetFixedTimetag(CHARGEED_TAG_BLAST,300);
		MAC_UTIL_BuildandSendData((uint8 *)&Appdata,sizeof(app_Urgent_t),MAC_UTIL_UNICAST, 0, NULL);

	}
	return;
}


uint8  ChargeED_GetBatteryValue(void)
{
#define CHARGEED_VCC40V  (24496>>4)
#define CHARGEED_VCC39V  (23888>>4)
#define CHARGEED_VCC38V  (23328>>4)
#define CHARGEED_VCC37V  (22704>>4)
#define CHARGEED_VCC36V  (21984>>4)
#define CHARGEED_VCC35V  (21552>>4)
#define CHARGEED_VCC34V  (20868>>4)
#define CHARGEED_VCC33V  (20360>>4)


	uint16 BatteryList[] = {CHARGEED_VCC40V,
		CHARGEED_VCC39V,
		CHARGEED_VCC38V,
		CHARGEED_VCC37V,
		CHARGEED_VCC36V,
		CHARGEED_VCC35V,
		CHARGEED_VCC34V,
		CHARGEED_VCC33V};

	uint16 adcvalue  = HalAdcRead2(HAL_ADC_CHANNEL_7, HAL_ADC_RESOLUTION_12,HAL_ADC_REF_125V);

	for(uint8 i=0;i<sizeof(BatteryList)/sizeof(BatteryList[0]);i++)
	{
		if(adcvalue > BatteryList[i])
		{
			return (40-i);    //0->33 1->32 ... 3.3V, 3.2V
		}
	}
	return 32;            // default is 3.2V
}

/*
void ChargeED_LocSortByRSSI(LocPos_t* LocNode, int8 LastCnt)
{
if(LastCnt < 2)
{
return;
}
for(int8 i = 0; i<LastCnt;i++)
{
for(int8 j=LastCnt-1;j>i;j--)
{
if(LocNode[j-1].RSSI< LocNode[j].RSSI)
{
LocPos_t loctmp;
loctmp = LocNode[j-1] ;
LocNode[j-1] = LocNode[j];
LocNode[j]   = loctmp;
}
}
}
}
*/
uint32 ChargeED_GetWinStart(uint32 Locticks[])
{
	uint32 tick = osal_GetSystemClock();

	/* get valid_ticks */
	uint32 valid_tick[CHARGEED_LOCTICK_LEN];
	uint8   valid_num  = 0;
	for(uint8 i=0;i<CHARGEED_LOCTICK_LEN;i++)
	{
		if(Locticks[i] >0 && tick>Locticks[i] && (tick-Locticks[i]) < 4*CHARGEED_RECV_PERIOD )
		{
			uint32 err = (tick-Locticks[i])%CHARGEED_RECV_PERIOD;

			if(err < 20)
			{
				valid_tick[valid_num++] = tick - err;
			}
			else if(err>CHARGEED_RECV_PERIOD-20)
			{
				valid_tick[valid_num++] = tick + CHARGEED_RECV_PERIOD-err;				

			}
			else
			{
				Locticks[i] = 0;
			}
		}
		else
		{
			Locticks[i] = 0;
		}
	}

	/* get min tick and winstart */

	uint32 mean_tick = tick/(valid_num+1);
	for(uint8 i=0;i<valid_num;i++)
	{
		mean_tick += valid_tick[i]/(valid_num+1);
	}

	/* set the winstart point */
	uint32 winstart = mean_tick + CHARGEED_RECV_PERIOD - CHARGEED_SYNC_WINLEN/2;

	/* move tick to tick buffer */
	for(uint8 i=0;i<CHARGEED_LOCTICK_LEN-1;i++)
	{
		Locticks[i] = Locticks[i+1];
	}
	Locticks[CHARGEED_LOCTICK_LEN-1] = tick;

	return winstart;

}
void ChargeED_SetAlert(void)
{
	if(system_param.workStatus!= CHARGEED_STATE_ALERT)
	{
		system_param.workStatus_bk = system_param.workStatus;
	}
	system_param.workStatus = CHARGEED_STATE_ALERT;
	system_param.AlertSuccess = false;
	system_param.AlertCnt = CHARGEED_ALERT_TIME;

	HalStartBeeper(BEEP_TYPE_ALERT,CHARGEED_BEEPFREQ_ALERT);

	if(ZSuccess!=osal_set_event(ChargeED_TaskId, CHARGEED_ALERT_EVENT))
	{
		SystemReset();
	}		
}
void ChargeED_UnSetAlert(void)
{
	system_param.workStatus = system_param.workStatus_bk;
	if(system_param.workStatus == CHARGEED_STATE_NORMAL)
	{
		HalLedSet(CHARGEED_LED_RED, HAL_LED_MODE_OFF);
		HalLedSet(CHARGEED_LED_BLUE, HAL_LED_MODE_OFF);
	}
	HalStopBeeper(BEEP_TYPE_ALERT, TRUE);
}

void ChargeED_SetRetreat(void)
{
	if(!system_param.isOKPressed)
	{
		if(system_param.workStatus == CHARGEED_STATE_ALERT)
		{
			system_param.workStatus_bk = CHARGEED_STATE_URGENT;
		}
		else
		{
			system_param.workStatus = CHARGEED_STATE_URGENT;
		}
	}
}
void ChargeED_UnSetRetreat(void)
{
	if(system_param.workStatus == CHARGEED_STATE_ALERT)
	{
		system_param.workStatus_bk = CHARGEED_STATE_NORMAL;
	}
	else if(system_param.workStatus == CHARGEED_STATE_URGENT)
	{
		system_param.workStatus = CHARGEED_STATE_NORMAL;			
	}
}

void ChargeED_ContinueEvents(void)
{
	if(system_param.AlertCnt > 0)
	{
		HalStartBeeper(BEEP_TYPE_ALERT,BEEP_TYPE_ALERT);	
		osal_set_event(ChargeED_TaskId, CHARGEED_ALERT_EVENT);
	}
	if(system_param.OKPressCnt >0)
	{
		osal_set_event(ChargeED_TaskId, CHARGEED_OK_PRESS_EVENT);
	}
}
/**************************************************************************************************
**************************************************************************************************/

