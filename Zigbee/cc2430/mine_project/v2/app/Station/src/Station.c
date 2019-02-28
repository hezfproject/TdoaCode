/**************************************************************************************************
Filename:       CR.c
Revised:        $Date: 2010/11/20 23:45:01 $
Revision:       $Revision: 1.5 $

Description:    This file contains the application that can be use to set a device as Location
node from MAC directly which will communicate with a FFD with full z-stack.

**************************************************************************************************/

/**************************************************************************************************
*                                           Includes
**************************************************************************************************/
#include <stdio.h>
#include <string.h>

/* Hal Driver includes */
#include "hal_types.h"
#include "hal_key.h"
#include "hal_timer.h"
#include "hal_drivers.h"
#include "hal_led.h"
#include "hal_assert.h"
#include "hal_uart.h"
#include "hal_spi.h"

/* OS includes */
#include "OSAL.h"
#include "OSAL_Tasks.h"
#include "OSAL_Nv.h"

/* App Protocol*/
#include "App_cfg.h"

/* Application Includes */
#include "OnBoard.h"

/* MAC Application Interface */
#include "mac_api.h"
#include "mac_main.h"
#include "mac_pib.h"
#include "ZComDef.h"
#include "bsmac_header.h"
#include "bsmac.h"

/* Application */
#include "Station.h"

/*My Sleep Util*/
#include "SleepUtil.h"

/* watchdog util */
#include "watchdogutil.h"


/**************************************************************************************************
*                                           Macros
**************************************************************************************************/
#define STATION_PARAM_ADDR           		  0xFEF0
#define STATION_SPIERR_TIMEOUT             12
#define STATION_URGENT_TIMEOUT                  10          /* time of continuous urgent signal emmission in minutes*/
#define STATION_SENDCMD_TIMEOUT             10           /* Mode Signal Keep for query in minutes*/ 

#define STATION_MAX_PHONE_NUMBER                           32

#define STATION_URGENT_NODATA                0
#define STATION_URGENT_RETREAT               1
#define STATION_URGENT_CANCELRETREAT    2

#define RSSI_MIN      (MAC_RADIO_RECEIVER_SENSITIVITY_DBM + MAC_SPEC_ED_MIN_DBM_ABOVE_RECEIVER_SENSITIVITY)
#define RSSI_MAX    MAC_RADIO_RECEIVER_SATURATION_DBM

#define CONV_LQI_TO_RSSI( rssi,lqi ) \
	st (   \
	rssi = lqi*(RSSI_MAX - RSSI_MIN)/MAC_SPEC_ED_MAX + RSSI_MIN; \
	)

#define STATION_LED_RED  	HAL_LED_2
#define STATION_LED_BLUE  	HAL_LED_1

#define STATION_SW_VERSION  	"1008RC3"
/*********************************************************************
* TYPEDEFS
*/

typedef enum
{
	E_STATION_INIT,
	E_STATION_STARTED
} eStationState;


typedef struct
{
	/* device information */
	uint16        PanId;
	uint16        ShortAddr;
	uint8          Channel; 
	sAddrExt_t extAddr;
	uint16        armid;

	/* status */
	eStationState State;

	/* counts */
	uint8 SPIErrCnt;
	uint8    urgent_timeout_cnt;

} Station_DevInfo_t;


typedef struct
{
	uint8  ResetFlag;
} storeParam_t;


typedef struct
{
	bool   inUse;
	uint8 extAddr[8];
}tsEndDeviceData;

#ifdef DEBUG
typedef struct 
{
	bool  InUse;
	uint8 status;
	uint16 cnt;
}Station_DataCnf_Info_t;

typedef struct
{
	uint16 srcnmbr;
	uint16 lastblk;
	uint16 recvcnt;
	uint16 errcnt;	
}Station_voice_blkInfo_t;
#endif

/* Size table for MAC structures */
const CODE uint_8 CR_cbackSizeTable [] =
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

/* TRUE and FALSE value */
static bool          Station_MACTrue = TRUE;
static bool          Station_MACFalse = FALSE;

/* Task ID */
uint8 		   Station_TaskId;

/* Device Info from flash */
static Station_DevInfo_t Station_DevInfo;

/* buf send on uart  */
static app_header_t* Station_UartBuf;

#ifdef DEBUG
static  Station_DataCnf_Info_t  Station_DataCnfReturns[8];
static Station_voice_blkInfo_t Station_voice_blkInfo[STATION_MAX_PHONE_NUMBER];
#endif
/**************************************************************************************************
*                                     Local Function Prototypes
**************************************************************************************************/
/* Setup routines */
static void         Station_Startup();

/* Support */
static void         Station_ReadDevInfo();

static uint8  Station_SendDataToAir(uint8 *p, uint8 len, uint16 shortAddr, uint8 msgtype, bool retrans);
static void Station_ParseUartRx(unsigned char *pbuf, unsigned char len);
static uint8 Station_ParseMobileFrame(macMcpsDataInd_t* pInd);
/**************************************************************************************************
*
* @fn          Station_Init
*
* @brief       Initialize the application
*
* @param       taskId - taskId of the task after it was added in the OSAL task queue
*
* @return      none
*
**************************************************************************************************/
void Station_Init(uint_8 taskId)
{       
	/* Initialize the task id */
	Station_TaskId = taskId;

	/* initialize MAC features */
	MAC_Init();
	MAC_InitCoord();

	/* Reset the MAC */
	MAC_MlmeResetReq(TRUE);

#if(defined WATCHDOG) && (WATCHDOG==TRUE)
	StartWatchDog ( DOGTIMER_INTERVAL_1S );
	osal_set_event ( Station_TaskId, STATION_FEEDWATCHDOG_EVENT );
#endif

	/*init params */
	Station_DevInfo.PanId = 0xFFFF;
	Station_DevInfo.ShortAddr = 0x00;
	Station_DevInfo.Channel = 0x0B; 
	Station_DevInfo.armid = 0xFFFF;
	Station_DevInfo.State = E_STATION_INIT;

	Station_UartBuf = (app_header_t*) osal_mem_alloc(BSMAC_MAX_TX_PAYLOAD_LEN+ 10); //guard 

	Station_ReadDevInfo();

	Station_Startup();

	/* initial bsmac */
	bsmac_init(Station_ParseUartRx, 0, 0, 1, Station_DevInfo.PanId);

#ifdef DEBUG
	osal_memset((void *)Station_voice_blkInfo, 0, sizeof(Station_voice_blkInfo_t)*STATION_MAX_PHONE_NUMBER);
#endif
}

/**************************************************************************************************
*
* @fn          Station_ProcessEvent
*
* @brief       This routine handles events
*
* @param       taskId - ID of the application task when it registered with the OSAL
*              events - Events for this task
*
* @return      16bit - Unprocessed events
*
**************************************************************************************************/
uint16 Station_ProcessEvent(uint_8 taskId, uint16 events)
{
	uint8* pMsg;
	macCbackEvent_t* pData;

#if(defined WATCHDOG) && (WATCHDOG==TRUE)
	if ( events & STATION_FEEDWATCHDOG_EVENT )
	{
		osal_start_timerEx ( Station_TaskId, STATION_FEEDWATCHDOG_EVENT, 300 );
		FEEDWATCHDOG();
		return events ^ STATION_FEEDWATCHDOG_EVENT;
	}
#endif

	if (events & SYS_EVENT_MSG)
	{
		while ((pMsg = osal_msg_receive(Station_TaskId)) != NULL)
		{
                       pData = (macCbackEvent_t *) pMsg;
			switch ( *pMsg )
			{			
			case MAC_MLME_START_CNF:
				{
					if(pData->startCnf.hdr.status != MAC_SUCCESS)
					{
						/* set restart reason and restart */
						storeParam_t param = * ( storeParam_t* ) STATION_PARAM_ADDR;
						param.ResetFlag = APP_MPRF_REPORT_STARTNWK_FAILED_RESTART;
						* ( storeParam_t* ) STATION_PARAM_ADDR =  param;

						HalLedBlink(STATION_LED_RED, 1, 50, 500);
						Station_DevInfo.State = E_STATION_INIT;
						if(ZSuccess!= osal_start_timerEx(Station_TaskId, STATION_WDG_RESTART_EVENT, 500))
						{
							SystemReset();
						}
					}

					/* power	on */
					HalLedSet(STATION_LED_RED,  HAL_LED_MODE_ON);

					if ( NO_TIMER_AVAIL == osal_start_timerEx ( Station_TaskId, STATION_STATUS_REPORT_EVENT, 10))
					{
						SystemReset();
					}
					break;
				}

			case MAC_MCPS_DATA_CNF:
				{
					mac_msg_deallocate((uint8**)&pData->dataCnf.pDataReq);
					break;
				}

			case MAC_MCPS_DATA_IND:
				{
					if(pData->dataInd.hdr.status == ZSUCCESS)
					{
						Station_ParseMobileFrame(&pData->dataInd);
					}
					break;
				}

			}

			/* Deallocate */
			mac_msg_deallocate((uint8 **)&pMsg);
		}

		return events ^ SYS_EVENT_MSG;
	} 

	if ( events & STATION_WDG_RESTART_EVENT )
	{
		SystemReset();
	}

	if ( events & STATION_STATUS_REPORT_EVENT )
	{

		if(Station_DevInfo.State == E_STATION_INIT)
		{
			/* get reset flag and send report */
			storeParam_t param = * ( storeParam_t* ) STATION_PARAM_ADDR;

			app_mprfReport_t * prfReport = (app_mprfReport_t *)(Station_UartBuf+1);

			prfReport->hdr.srcaddr = Station_DevInfo.PanId;
			prfReport->hdr.dstaddr = APP_ARMSHORTADDR;
			prfReport->reporttype = APP_MPRF_REPORT_TYPE_REQ;
			switch ( GetResetFlag() )
			{
			case RESET_FLAG_WATCHDOG:
				{
					prfReport->restartflag = param.ResetFlag;
					break;
				}
			case RESET_FLAG_EXTERNAL:
				{
					prfReport->restartflag   = APP_MPRF_REPORT_EXTERNAL_RESTART;
					break;
				}
			case RESET_FLAG_POWERON:
				{
					prfReport->restartflag   = APP_MPRF_REPORT_POWERON;
					break;
				}
			default:
				break;

			}
			prfReport->seqnum = 0;
			char* pData = (char*)( prfReport+1 );
			//const uint8 maxDataLen = BSMAC_MAX_TX_PAYLOAD_LEN- sizeof(app_header_t) - sizeof(app_mprfReport_t);
			sprintf(pData, "SoftVer:%s, BuildTime:%s,%s, Channel:%d",STATION_SW_VERSION,__DATE__,__TIME__,macPib.logicalChannel);
			prfReport->len = strlen(pData);

			Station_UartBuf->protocoltype = APP_PROTOCOL_TYPE_MOBILE;
			Station_UartBuf->msgtype = MPRF_REPORT;
			Station_UartBuf->len = sizeof(app_mprfReport_t) + prfReport->len;

			bsmac_write_data((uint8*)Station_UartBuf, sizeof(app_header_t)+Station_UartBuf->len);

			if(ZSuccess!= osal_start_timerEx(Station_TaskId, STATION_STATUS_REPORT_EVENT, 2000))
			{
				SystemReset();
			}

		}

		return events ^ STATION_STATUS_REPORT_EVENT;
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

	uint_8 len = CR_cbackSizeTable[pData->hdr.event];

	switch (pData->hdr.event)
	{
	case MAC_MLME_BEACON_NOTIFY_IND:

		len += sizeof(macPanDesc_t) + pData->beaconNotifyInd.sduLength +
			MAC_PEND_FIELDS_LEN(pData->beaconNotifyInd.pendAddrSpec);
		if ((pMsg = (macCbackEvent_t *) osal_msg_allocate(len)) != NULL)
		{
			/* Copy data over and pass them up */
			osal_memcpy(pMsg, pData, sizeof(macMlmeBeaconNotifyInd_t));
			pMsg->beaconNotifyInd.pPanDesc = (macPanDesc_t *) ((uint_8 *) pMsg + sizeof(macMlmeBeaconNotifyInd_t));
			osal_memcpy(pMsg->beaconNotifyInd.pPanDesc, pData->beaconNotifyInd.pPanDesc, sizeof(macPanDesc_t));
			pMsg->beaconNotifyInd.pSdu = (uint_8 *) (pMsg->beaconNotifyInd.pPanDesc + 1);
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
		if(pMsg->hdr.event == MAC_MCPS_DATA_CNF)
		{
			osal_msg_send(Hal_TaskID, (uint8 *) pMsg);
		}
		else
		{
			osal_msg_send(Station_TaskId, (uint8 *) pMsg);
		}
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
uint_8 MAC_CbackCheckPending(void)
{
	return (0);
}


/**************************************************************************************************
*
* @fn      Station_Startup()
*
* @brief   Update the timer per tick
*
* @param   beaconEnable: TRUE/FALSE
*
* @return  None
*
**************************************************************************************************/
void Station_Startup()
{
	macMlmeStartReq_t   startReq;

	if(Station_DevInfo.State == E_STATION_STARTED) return;

	/* Setup MAC_EXTENDED_ADDRESS */
	MAC_MlmeSetReq(MAC_EXTENDED_ADDRESS, &Station_DevInfo.extAddr);

	/* Setup MAC_SHORT_ADDRESS */
	MAC_MlmeSetReq(MAC_SHORT_ADDRESS, &Station_DevInfo.ShortAddr);

	/* Setup MAC_BEACON_PAYLOAD_LENGTH */
	uint_8 tmp8 = 0;    
	MAC_MlmeSetReq(MAC_BEACON_PAYLOAD_LENGTH, &tmp8);

	/* Enable RX */
	MAC_MlmeSetReq(MAC_RX_ON_WHEN_IDLE, &Station_MACTrue);

	/* Setup MAC_ASSOCIATION_PERMIT */
	MAC_MlmeSetReq(MAC_ASSOCIATION_PERMIT, &Station_MACFalse);

	/* change CCA param */
	uint8 maxFrameRetries = 4;
	MAC_MlmeSetReq ( MAC_MAX_FRAME_RETRIES, &maxFrameRetries );

	uint8 maxCsmaBackoff  = 5;
	MAC_MlmeSetReq ( MAC_MAX_CSMA_BACKOFFS, &maxCsmaBackoff );

	uint8 minBe = 4;
	MAC_MlmeSetReq ( MAC_MIN_BE, &minBe);

	uint8 maxBe = 6;
	MAC_MlmeSetReq ( MAC_MAX_BE, &maxBe );

	/* Fill in the information for the start request structure */
	startReq.startTime = 0;
	startReq.panId = Station_DevInfo.PanId;
	startReq.logicalChannel = Station_DevInfo.Channel;
	startReq.beaconOrder = 0xF;
	startReq.superframeOrder = 0xF;
	startReq.panCoordinator = TRUE;
	startReq.batteryLifeExt = FALSE;
	startReq.coordRealignment = FALSE;
	startReq.realignSec.securityLevel = FALSE;
	startReq.beaconSec.securityLevel = FALSE;

	/* Call start request to start the device as a coordinator */
	MAC_MlmeStartReq(&startReq);

}

void Station_ReadDevInfo()
{

	osal_nv_item_init( ZCD_NV_EXTADDR, Z_EXTADDR_LEN, NULL );
	osal_nv_read( ZCD_NV_EXTADDR, 0, Z_EXTADDR_LEN, &Station_DevInfo.extAddr);

	Station_DevInfo.PanId = BUILD_UINT16(Station_DevInfo.extAddr[EXT_MACADDR_DEVID_LBYTE],Station_DevInfo.extAddr[EXT_MACADDR_DEVID_HBYTE]);
	Station_DevInfo.Channel= Station_DevInfo.extAddr[EXT_MACADDR_CHANNEL];
	Station_DevInfo.ShortAddr = 0;

	HAL_ASSERT(Station_DevInfo.extAddr[EXT_MACADDR_TYPE] == EXT_MACADDR_TYPE_SUBSTATION);

}

uint8  Station_SendDataToAir(uint8 *p, uint8 len, uint16 shortAddr, uint8 msgtype, bool retrans)
{
	Hal_SendDataToAir(p, len, MOBILEPHONE_NWK_ADDR, shortAddr, msgtype,  retrans);

	return MAC_SUCCESS;
}

static uint8 Station_ParseMobileFrame(macMcpsDataInd_t* pInd)
{	
	app_header_t* pheader = (app_header_t*) pInd->msdu.p;

	if(pInd->msdu.len < sizeof(app_header_t) + pheader->len)
	{
		return FAILURE;
	}

	if(pheader->msgtype < MP_MP2ARM_CMDEND)
	{
		return bsmac_write_data(pInd->msdu.p, pInd->msdu.len);
	}
	else if(pheader->msgtype == MP_ARMID)
	{
		mp_Armid_t* pArmid = (mp_Armid_t*)(pheader+1) ;

		if(pArmid->armidtype == APP_ARMID_TYPE_REQ)
		{
			mp_Armid_t mp_Armid;
			mp_Armid.armidtype = APP_ARMID_TYPE_ACK;
			mp_Armid.seqnum = pArmid->seqnum;
			mp_Armid.armid = Station_DevInfo.armid;

			return Station_SendDataToAir((uint8 *)&mp_Armid, sizeof(mp_Armid), 
				pInd->mac.srcAddr.addr.shortAddr, MP_ARMID, true);
		}
	}
	return FAILURE;
}
static void Station_ParseUartRx(unsigned char *pbuf, unsigned char len)
{
	app_header_t *pheader =  (app_header_t *)pbuf;
	uint8* 		pPayload =	(uint8*)(pheader+1);

	if(pheader->protocoltype== APP_PROTOCOL_TYPE_MOBILE || len < sizeof(app_header_t)+pheader->len)
	{	
		return;
	}

	/* mp2arm,  send to  mobile directly, always have headr app_mpheader_t */
	if(pheader->msgtype < MP_MP2ARM_CMDEND)	
	{
		app_mpheader_t *pmpheader = (app_mpheader_t*)pPayload;

		bool retrans;
		if(pheader->msgtype == MP_VOICE)
		{
			retrans = false;
		}
		else
		{
			retrans = true;
		}
		Station_SendDataToAir(pPayload, pheader->len,pmpheader->dstaddr, pheader->msgtype, retrans);
	}

	/* mprf vs arm */
	if(pheader->msgtype == MPRF_REPORT)
	{
		app_mprfReport_t *prfReport = (app_mprfReport_t *)pPayload;
		if( prfReport->reporttype == APP_MPRF_REPORT_TYPE_ACK)
		{
			/* device start */
			Station_DevInfo.armid = prfReport->hdr.srcaddr;
			Station_DevInfo.State = E_STATION_STARTED;

			storeParam_t param;
			param.ResetFlag = APP_MPRF_REPORT_WATCHDOG_RESTART;  // clear restart  flag
			* ( storeParam_t* ) STATION_PARAM_ADDR = param;
		}
	}

}
/**************************************************************************************************
**************************************************************************************************/

