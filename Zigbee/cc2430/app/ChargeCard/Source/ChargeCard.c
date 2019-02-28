/**************************************************************************************************
Filename:       ChargeCard.c
Revised:        $Date: 2011/09/05 01:02:39 $
Revision:       $Revision: 1.3.2.2.8.1 $

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
//#include "hal_drivers.h"
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
#include "ChargeCard_cfg.h"
#include "ChargeCard.h"

/* watchdog util */
#include "watchdogutil.h"

#include "delay.h"

#include "drivers.h"

/**************************************************************************************************
*                                           Defines
**************************************************************************************************/
#define CHARGECARD_TAG_POLL			0
#define CHARGECARD_TAG_BLAST		1

#define CHARGECARD_FIXTAG_LEN 			2

#define CHARGECARD_BATTERY_STAT_NORMAL 	0
#define CHARGECARD_BATTERY_STAT_LOW	 	1
#define CHARGECARD_BATTERY_STAT_VERYLOW 	2
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
} time_fixtag_item_t;
typedef struct
{
    time_fixtag_item_t fixtag[CHARGECARD_FIXTAG_LEN];
} time_tag_t;

typedef struct
{
    //bool    isFirst;
    uint16 blast_period;
    uint16 poll_interval;
    uint16 poll_timeout;
    uint16 seqnum;
} blast_param_t;


/* a extra flag to indication the device is reseted by period reset func */
#define SETRAM_FLAG 0x1234

typedef struct
{
    /* key press */
    uint16   keypress_timeout;
    uint8   keypress_cnt;
    uint8   keyconfirm_cnt;

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

} system_param_t;

typedef struct
{
    uint16  setram_flag;
    blast_param_t blast_param;
    system_param_t system_param;
} Dev_Param_t;

static uint8 recv_retreat_cnt;
/**************************************************************************************************
*                                           Constant
**************************************************************************************************/
#define CHARGECARD_MAC_PAYLOAD_LEN			 127                  //mac payload length by datasheet.
#define CHARGECARD_SHORTADDR_PANCOORD		 0x0000             /* Short Address of PAN Coordinator, will always be 0x0000 */

#define CHARGECARD_KEY_OK    		 HAL_KEY_SW_7
#define CHARGECARD_KEY_HELP        	HAL_KEY_SW_6

/* Application Work State */
/* The priority is: Alert > Urgent > Normal */

#define CHARGECARD_STATE_NORMAL		0
#define CHARGECARD_STATE_URGENT		1
#define CHARGECARD_STATE_ALERT	    		2


#define CHARGECARD_PARAM_ADDR			0xFD00		//0xFD00-0xFD53 are used to store paramters
#define CHARGECARD_DEVINFO_ADDR		0x7FF8

#define CHARGECARD_MIN_POWER	25

#define CHARGECARD_LED_BLUE     HAL_LED_1
#define CHARGECARD_LED_RED       HAL_LED_2

#define CHARGECARD_BEEPFREQ_ALERT 	10
#define CHARGECARD_RETREATFREQ_ALERT 	20

/* Size table for MAC structures */
const CODE uint8 ChargeCard_cbackSizeTable [] =
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
static uint16        ChargeCard_PanId = CARD_NWK_ADDR;                      /* 0xFFF0 */
static uint16        ChargeCard_PANCoordShortAddr = 0x00; 			/* Always be 0x00 */
static uint16        ChargeCard_ShortAddr = 0x00;

/* TRUE and FALSE value */
static bool            ChargeCard_MACTrue = TRUE;
//static bool          ChargeCard_MACFalse = FALSE;

/* Beacon order */
static uint8 ChargeCard_SuperFrameOrder = CHARGECARD_MAC_SUPERFRAME_ORDER;
static uint8 ChargeCard_BeaconOrder = CHARGECARD_MAC_BEACON_ORDER;

/* Task ID */
uint8 ChargeCard_TaskId;

/* Device Info from flash */
static Dev_Info_t ChargeCard_DevInfo;

/*params */
static  time_tag_t  time_tag;
blast_param_t 	blast_param;
static system_param_t system_param;
static bool 		isFirstBlast;
/**************************************************************************************************
*                                     Local Function Prototypes
**************************************************************************************************/
/* Setup routines */
static void     ChargeCard_DeviceStartup(uint8 *pData);

/* Support */
static void     ChargeCard_blast(uint8 reqtype);
static void     ChargeCard_ReadDevInfo(void);
#ifdef DEBUG
static void     ChargeCard_SetDevInfo(uint8 type, void *pData);
#endif
static void		ChargeCard_PeriodReset(void);
static void     ChargeCard_Restart(void);
static void 	ChargeCard_SaveParam2RAM(void);
static void 	ChargeCard_ReadParmFromRAM(void);
static bool		ChargeCard_SetFixedTimetag(uint8 tag, uint16 time);
static void 	ChargeCard_ParseAppData(APPWrapper_t *p, int8 rssi);
static void 	ChargeCard_BatteryCheck(void );
static uint8    ChargeCard_GetBatteryValue(void);
static void 	ChargeCard_SetAlert(void);
static void 	ChargeCard_UnSetAlert(void);
static void 	ChargeCard_SetRetreat(void);
static void 	ChargeCard_UnSetRetreat(void);
static void 	ChargeCard_ContinueEvents(void);
/**************************************************************************************************
*
* @fn          ChargeCard_Init
*
* @brief       Initialize the application
*
* @param       taskId - taskId of the task after it was added in the OSAL task queue
*
* @return      none
*
**************************************************************************************************/
void ChargeCard_Init(uint8 taskId)
{
    /* Initialize the task id */
    ChargeCard_TaskId = taskId;

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
    blast_param.poll_interval = CHARGECARD_POLL_INTERVAL;
    blast_param.poll_timeout = CHARGECARD_POLL_TIMEOUT;
    blast_param.seqnum = 0;

    osal_memset(&system_param, 0, sizeof(system_param) );
    system_param.AlertSuccess = false;
    system_param.AlertCnt = 0;
    system_param.workStatus = CHARGECARD_STATE_NORMAL;
    system_param.workStatus_bk = CHARGECARD_STATE_NORMAL;

    system_param.BatteryStatus = CHARGECARD_BATTERY_STAT_NORMAL;
    system_param.isOKPressed = false;
    system_param.OKPressCnt	= 0;

    osal_memset(&time_tag, 0, sizeof(time_tag) );

    ChargeCard_ReadDevInfo();

    ChargeCard_DeviceStartup(NULL);

    /* turn off leds */
    HalLedSet(CHARGECARD_LED_BLUE, HAL_LED_MODE_OFF);
    HalLedSet(CHARGECARD_LED_RED, HAL_LED_MODE_OFF);

    /*Start Watch Dog*/
#ifdef WATCHDOG
    StartWatchDog(DOGTIMER_INTERVAL_1S);
#endif


    /* flash once to fix restart bugs!!! */
    HalLedBlink(CHARGECARD_LED_BLUE, 1, 50, 10);

    /* if reset from watchdog,continue events */
    if(GetResetFlag() == RESET_FLAG_WATCHDOG)
    {
        ChargeCard_ContinueEvents();
    }
#ifdef USE_STATE_UNINIT
    if(ZSuccess != osal_set_event(ChargeCard_TaskId,   CHARGECARD_BLAST_EVENT))
    {
        SystemReset();
    }
#else
    /* set first blast */
    isFirstBlast = true;
    if(ZSuccess != osal_set_event(ChargeCard_TaskId, CHARGECARD_BLAST_EVENT))
    {
        SystemReset();
    }
#endif

}

/**************************************************************************************************
*
* @fn          ChargeCard_ProcessEvent
*
* @brief       This routine handles events
*
* @param       taskId - ID of the application task when it registered with the OSAL
*              events - Events for this task
*
* @return      16bit - Unprocessed events
*
**************************************************************************************************/
uint16 ChargeCard_ProcessEvent(uint8 taskId, uint16 events)
{
    uint8 *pMsg;
    macCbackEvent_t *pData;

    /* process timetick every 1 ms*/
    if(events  & CHARGECARD_PROCTIMETICK_EVENT)
    {
        ChargeCard_ProcTimeTick();
        return events ^ CHARGECARD_PROCTIMETICK_EVENT;
    }

    /* system message */
    if (events & SYS_EVENT_MSG)
    {
        while ((pMsg = osal_msg_receive(ChargeCard_TaskId)) != NULL)
        {
            switch ( *pMsg )
            {

            case MAC_MCPS_DATA_CNF:
            {
                pData = (macCbackEvent_t *) pMsg;
                static uint8 fail_time;

                if(pData->dataCnf.hdr.status == MAC_SUCCESS)
                {
                    fail_time = 0;
                }
                else
                {
                    if(fail_time++ > CHARGECARD_MAX_BLAST_FAIL)
                    {
                        ChargeCard_SaveParam2RAM();
                        SystemReset();
                    }
                }
                osal_msg_deallocate((uint8 *) pData->dataCnf.pDataReq);
                break;
            }

            case MAC_MCPS_DATA_IND:
                /* Proess Command */
                pData = (macCbackEvent_t *) pMsg;
                uint8 cluster_id = MAC_UTIL_GetClusterID(pData->dataInd.msdu);
                if(cluster_id == CHARGEED_CLUSTERID || cluster_id == LOCNODE_CLUSTERID)
                {
                    sData_t AppData = MAC_UTIL_RemoveHeader(pData->dataInd.msdu);
                    int8 rssi = pData->dataInd.mac.rssi;
                    ChargeCard_ParseAppData((APPWrapper_t *) AppData.p, rssi);
                }
                break;
            }

            /* Deallocate */
            osal_msg_deallocate((uint8 *) pMsg);
        }

        return events ^ SYS_EVENT_MSG;

    }

    /* restart */
    if(events & CHARGECARD_RESET_EVENT)
    {
        ChargeCard_Restart();
        return events ^ CHARGECARD_RESET_EVENT;
    }

    /* alert */
    if(events & CHARGECARD_ALERT_EVENT)
    {
        const uint16 AlertInterval = 1000;  // 1s

        if(system_param.workStatus == CHARGECARD_STATE_ALERT)
        {
            if(system_param.AlertCnt-- != 0)
            {
                /* Send Alerts */
                app_Urgent_t Appdata;
                Appdata.msgtype = URGENT;
                Appdata.urgenttype = ALERT;
                Appdata.value = 0;
                ChargeCard_SetFixedTimetag(CHARGECARD_TAG_BLAST, CHARGECARD_POLL_TIMEOUT);
                MAC_UTIL_BuildandSendData((uint8 *)&Appdata, sizeof(app_Urgent_t),  MAC_UTIL_UNICAST, 0, NULL);

                /* blink LEDS */
                if(system_param.AlertCnt < 3 && system_param.AlertSuccess )   // if success, last 3 seconds flash blue
                {
                    HalLedBlink(CHARGECARD_LED_BLUE, 3, 50, 300);
                }
                else
                {
                    HalLedBlink(CHARGECARD_LED_RED, 3, 50, 300);
                }
                if(ZSuccess != osal_start_timerEx(ChargeCard_TaskId, CHARGECARD_ALERT_EVENT, AlertInterval))
                {
                    SystemReset();
                }
            }
            else // alert end
            {
                ChargeCard_UnSetAlert();
            }
        }
        return events ^ CHARGECARD_ALERT_EVENT;
    }

    if(events & CHARGECARD_KEYCONFIRM_EVENT)
    {
        if(system_param.keyconfirm_cnt > 0 && system_param.keyconfirm_cnt <= 10)
        {
            system_param.keyconfirm_cnt--;

            /* send out key confirmed  */
            app_Urgent_t app_Urgent;
            app_Urgent.msgtype = URGENT;
            app_Urgent.urgenttype = NOPWR;
            app_Urgent.value = URGENT_NOPWR_KEYCONFIRM;

            ChargeCard_SetFixedTimetag(CHARGECARD_TAG_BLAST, 20);
            MAC_UTIL_BuildandSendData((uint8 *)&app_Urgent, sizeof(app_Urgent), MAC_UTIL_UNICAST, 0, NULL);

            if(ZSuccess != osal_start_timerEx(ChargeCard_TaskId, CHARGECARD_KEYCONFIRM_EVENT, 1000))
            {
                SystemReset();
            }
        }
        else
        {
            system_param.keyconfirm_cnt = 0;
        }
        return events ^ CHARGECARD_KEYCONFIRM_EVENT;
    }
    /* retreat loop, never stop */
    if(events & CHARGECARD_RETREAT_EVENT)
    {
        if(system_param.workStatus == CHARGECARD_STATE_URGENT && !system_param.isOKPressed)
        {
            /* Blink LED */
            HalLedBlink(CHARGECARD_LED_RED, 10, 50, 480);

            /* sometimes BLUE LED is on in retreat, turn off it */
            HalLedSet(CHARGECARD_LED_BLUE, HAL_LED_MODE_OFF);

            if(!HalBeeperBusy())
            {
                HalStartBeeper(BEEP_TYPE_RETREAT, CHARGECARD_RETREATFREQ_ALERT);
            }

			  /* send out received urgent */
            if(recv_retreat_cnt < 5)
            {
		        recv_retreat_cnt++;
            
                app_Urgent_t app_Urgent;
                app_Urgent.msgtype = URGENT;
                app_Urgent.urgenttype = NOPWR;
                app_Urgent.value = URGENT_NOPWR_RECVRETREAT;

                ChargeCard_SetFixedTimetag(CHARGECARD_TAG_BLAST, 20);
                MAC_UTIL_BuildandSendData((uint8 *)&app_Urgent, sizeof(app_Urgent), MAC_UTIL_UNICAST, 0, NULL);
            }

        }

        if(ZSuccess != osal_start_timerEx(ChargeCard_TaskId, CHARGECARD_BLAST_EVENT, blast_param.blast_period))
        {
            SystemReset();
        }
        return events ^ CHARGECARD_RETREAT_EVENT;
    }

    /* blast loop, never stop */
    if(events & CHARGECARD_BLAST_EVENT)

    {
        /* process battery check */
        ChargeCard_BatteryCheck();

        /* normal or OK pressed */
        if(system_param.workStatus == CHARGECARD_STATE_NORMAL ||
                (system_param.workStatus == CHARGECARD_STATE_URGENT && system_param.isOKPressed))
        {
            /*if battery is  low, do not flash the blue LED, flash red LED*/
            /*if battery is  very low, do not flash any LED*/
            if(!isFirstBlast && system_param.BatteryStatus == CHARGECARD_BATTERY_STAT_NORMAL)
            {
                HalLedSet(CHARGECARD_LED_BLUE, HAL_LED_MODE_ON);
            }
            if(HalBeeperBusy())
            {
                HalStopBeeper(NULL, true);
            }

            /* if OK pressed, set URGET to NORMAL*/
            system_param.workStatus = CHARGECARD_STATE_NORMAL;
        }
        /* Send out  message */
        if(isFirstBlast == true)
        {
            isFirstBlast = false;
        }
        else
        {
            if(blast_param.seqnum % CHARGECARD_POLL_INTERVAL == 0)
            {
                ChargeCard_SetFixedTimetag(CHARGECARD_TAG_POLL, CHARGECARD_POLL_TIMEOUT);
                ChargeCard_blast(SSREQ_POLL);
            }
            else
            {
                ChargeCard_blast(SSREQ_OUT);
            }

        }

        /* start  process retreat */
        if(ZSuccess != osal_set_event(ChargeCard_TaskId, CHARGECARD_RETREAT_EVENT))
        {
            SystemReset();
        }
        return events ^ CHARGECARD_BLAST_EVENT;

    }

    /* test key long press */
    if(events & CHARGECARD_KEY_LONGPRESS_EVENT)
    {
        const uint16 testInterval = 300;   // test once each 300 ms
        uint16 testnum = system_param.keypress_timeout / testInterval;
        uint8 keys = HalKeyRead();

        if(system_param.keypress_cnt++ < testnum)
        {
            if(keys != 0 )
            {
                if(ZSuccess != osal_start_timerEx(ChargeCard_TaskId, CHARGECARD_KEY_LONGPRESS_EVENT, testInterval))
                {
                    SystemReset();
                }
            }
        }
        else // detected a long press
        {
            if(keys & CHARGECARD_KEY_HELP)  // start a alert
            {
                ChargeCard_SetAlert();
            }
        }
        return events ^ CHARGECARD_KEY_LONGPRESS_EVENT;
    }

    /* time counter of retreat suppress */
    if(events & CHARGECARD_OK_PRESS_EVENT)
    {
        if(system_param.OKPressCnt != 0 && --system_param.OKPressCnt != 0)
        {
            if(ZSuccess != osal_start_timerEx(ChargeCard_TaskId, CHARGECARD_OK_PRESS_EVENT, 60000))
            {
                SystemReset();
            }
        }
        else  // suppress end
        {
            system_param.isOKPressed = false;
            system_param.OKPressCnt = 0;
        }

        return events ^ CHARGECARD_OK_PRESS_EVENT;
    }

#ifdef USE_STATE_UNINIT
    if(events & CHARGECARD_UNINIT_EVENT)
    {

        return events ^ CHARGECARD_UNINIT_EVENT;
    }
#endif

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

    uint8 len = ChargeCard_cbackSizeTable[pData->hdr.event];

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
        osal_msg_send(ChargeCard_TaskId, (byte *) pMsg);
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
* @fn      ChargeCard_DeviceStartup(uint8* pData)
*
* @brief   Update the timer per tick
*
* @param   beaconEnable: TRUE/FALSE
*
* @return  None
*
**************************************************************************************************/
void ChargeCard_DeviceStartup(uint8 *pData)
{
    MAC_MlmeSetReq(MAC_EXTENDED_ADDRESS, &ChargeCard_DevInfo.ExitAddr);

    /* Setup MAC_BEACON_PAYLOAD_LENGTH */
    //MAC_MlmeSetReq(MAC_BEACON_PAYLOAD_LENGTH, &ChargeCard_BeaconPayloadLen);

    /* Setup MAC_BEACON_PAYLOAD */
    //MAC_MlmeSetReq(MAC_BEACON_PAYLOAD, &ChargeCard_BeaconPayload);

    /* Setup PAN ID */
    MAC_MlmeSetReq(MAC_PAN_ID, &ChargeCard_PanId);

    /* This device is setup for Direct Message */
    system_param.rx_on_idle = true;
    MAC_MlmeSetReq(MAC_RX_ON_WHEN_IDLE, &system_param.rx_on_idle);

    /* Setup Coordinator short address */
    MAC_MlmeSetReq(MAC_COORD_SHORT_ADDRESS, &ChargeCard_PANCoordShortAddr);

    /* Setup Beacon Order */
    MAC_MlmeSetReq(MAC_BEACON_ORDER, &ChargeCard_BeaconOrder);

    /* Setup Super Frame Order */
    MAC_MlmeSetReq(MAC_SUPERFRAME_ORDER, &ChargeCard_SuperFrameOrder);


    //uint8 tmp8 = ChargeCard_MAC_CHANNEL;
    uint8 channel = ChargeCard_DevInfo.ExitAddr[EXT_MACADDR_CHANNEL];
    MAC_MlmeSetReq(MAC_LOGICAL_CHANNEL, &channel);

    ChargeCard_ShortAddr = BUILD_UINT16(ChargeCard_DevInfo.ExitAddr[EXT_MACADDR_DEVID_LBYTE], ChargeCard_DevInfo.ExitAddr[EXT_MACADDR_DEVID_HBYTE]);
    MAC_MlmeSetReq(MAC_SHORT_ADDRESS, &ChargeCard_ShortAddr);

    MAC_MlmeSetReq(MAC_ASSOCIATED_PAN_COORD, &ChargeCard_MACTrue);

}
void ChargeCard_ParseAppData(APPWrapper_t *p, int8 rssi)
{
    switch(*((MSGType *)p))
    {
    case URGENT:
        switch(((app_Urgent_t *)p)->urgenttype)
        {
        case    RETREAT:
            ChargeCard_SetRetreat();
            break;
        case    CANCELRETREAT:
            ChargeCard_UnSetRetreat();
            break;
        case   ALERTACK:
            system_param.AlertSuccess = TRUE;
            break;
        }
        break;

    case CHARGEED_SSRSP:
    {
        app_chargeed_ssRsp_t *pssRsp = (app_chargeed_ssRsp_t *)p;
        if(pssRsp->urgent_type == RETREAT)
        {
            ChargeCard_SetRetreat();
        }
        else if(pssRsp->urgent_type == CANCELRETREAT)
        {
            ChargeCard_UnSetRetreat();
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
        HalLedBlink(ChargeCard_LED_RED, 3, 50, 300);
        */
        break;
    }
    case PWRCTL:
    {
        /*
        uint8 power = ((app_PWRCTL_t*)p)->workpower;
        if(power == 0)
        {
        MAC_MlmeSetReq(MAC_PHY_TRANSMIT_POWER, &power);
        }
        else if(power <= ChargeCard_MIN_POWER)
        {
        power+=5;      //see mac_radio_defs.c      line 56
        MAC_MlmeSetReq(MAC_PHY_TRANSMIT_POWER, &power);
        }
        */
        break;
    }
    default:
        break;

    }

}
/**************************************************************************************************
*
* @fn      ChargeCard_HandleKeys
*
* @brief   Callback service for keys
*
* @param   keys  - keys that were pressed
*          state - shifted
*
* @return  void
*
**************************************************************************************************/
void ChargeCard_HandleKeys(uint16 keys, uint8 shift)
{
    if(keys & CHARGECARD_KEY_OK)
    {
        if(system_param.workStatus == CHARGECARD_STATE_URGENT)
        {
            system_param.isOKPressed = TRUE;
            system_param.OKPressCnt = CHARGECARD_RETREATSUPPRESS_TIME;
            osal_set_event(ChargeCard_TaskId, CHARGECARD_OK_PRESS_EVENT);

            if(HalBeeperBusy())
            {
                HalStopBeeper(NULL, true);
            }

            /* if OK pressed, set URGET to NORMAL*/
            system_param.workStatus = CHARGECARD_STATE_NORMAL;
            HalLedSet(CHARGECARD_LED_RED, HAL_LED_MODE_OFF);

            /* send out key pressed */
            system_param.keyconfirm_cnt = 10;
            if(ZSuccess != osal_set_event(ChargeCard_TaskId, CHARGECARD_KEYCONFIRM_EVENT))
            {
                SystemReset();
            }
        }
    }

    /* process long press for all keys */
    system_param.keypress_timeout = CHARGECARD_KEY_LONGPRESS_TIME;
    system_param.keypress_cnt = 0;
    osal_set_event(ChargeCard_TaskId, CHARGECARD_KEY_LONGPRESS_EVENT);
}

/**************************************************************************************************
*
* @fn      ChargeCard_blast(void)
*
* @brief   Blast once to all Coords and routers without ACK or retrans.
*
* @param
*
* @return
*
**************************************************************************************************/

void ChargeCard_blast(uint8 reqtype)
{
    ChargeCard_PeriodReset();

    /* send out */
    if(system_param.BatteryStatus != CHARGECARD_BATTERY_STAT_VERYLOW)
    {

        app_chargeed_ssReq_t ssReq;
        ssReq.msgtype = CHARGEED_SSREQ;
        ssReq.reqtype = reqtype;
        ssReq.srcPan = CARD_NWK_ADDR;
        ssReq.seqnum = blast_param.seqnum++;
        ssReq.LocCnt = 0;
        ChargeCard_SetFixedTimetag(CHARGECARD_TAG_BLAST, 5);
        MAC_UTIL_BuildandSendData((uint8 *)&ssReq, sizeof(app_chargeed_ssReq_t), MAC_UTIL_UNICAST, 0, NULL);
    }

}


void ChargeCard_ReadDevInfo()
{
#if 0
    /* Make a fake DEV Info*/
    ChargeCard_DevInfo.ExitAddr[0] = 0x23;
    ChargeCard_DevInfo.ExitAddr[1] = 0x04;
    ChargeCard_DevInfo.ExitAddr[2] = 0xFF;
    ChargeCard_DevInfo.ExitAddr[3] = 0xFF;
    ChargeCard_DevInfo.ExitAddr[4] = 0x01;
    ChargeCard_DevInfo.ExitAddr[5] = 0x01;
    ChargeCard_DevInfo.ExitAddr[6] = 0x0E;
    ChargeCard_DevInfo.ExitAddr[7] = 0xFF;
#else
    Dev_Info_t *p = (Dev_Info_t *)(CHARGECARD_DEVINFO_ADDR);
    ChargeCard_DevInfo = *p;

    if(GetResetFlag() == RESET_FLAG_WATCHDOG)
    {
        ChargeCard_ReadParmFromRAM();
    }

    /*For Card, the lowest Byte of Exit Addr should be 0x01 */
    HAL_ASSERT(ChargeCard_DevInfo.ExitAddr[EXT_MACADDR_TYPE] == EXT_MACADDR_TYPE_CARD);
#endif
}

#if 0
void ChargeCard_IntervalCheck(void)
{
#define 	ChargeCard_MAX_INTERVAL	20000   // 20s

    static uint32 last_ticks = 0;

    /* read current ticks */
    uint32 ticks;
    ((uint8 *) &ticks)[0] = ST0;
    ((uint8 *) &ticks)[1] = ST1;
    ((uint8 *) &ticks)[2] = ST2;
    ((uint8 *) &ticks)[3] = 0;

    if(last_ticks != 0)
    {
        uint32 diff_ticks;
        if(ticks > last_ticks)
        {
            diff_ticks = ticks - last_ticks;
        }
        else
        {
            diff_ticks = 0x1000000 + ticks - last_ticks;
        }

        diff_ticks >>= 5; // convert  1/32k  to  ms,  diff_ticks = diff_ticks/32;

        if(diff_ticks > ChargeCard_MAX_INTERVAL)  //  if interval > 20s, reset
        {
            ChargeCard_Restart();
        }
    }
    last_ticks = ticks;
}
#endif
void ChargeCard_PeriodReset()
{
    //#define ChargeCard_PERIODRESET_NUM 	(10UL*24UL*3600UL/(ChargeCard_BLAST_PERIOD/1000UL))   // restart per 10 day

#define CHARGECARD_PERIODRESET_NUM 	(15UL*60UL*1000UL/CHARGEED_BLAST_PERIOD) 	//15min

    static uint32 ChargeCard_periodReset_cnt = 0;

    ChargeCard_SaveParam2RAM();
    if(ChargeCard_periodReset_cnt++ > CHARGECARD_PERIODRESET_NUM)
    {
        osal_start_timerEx(ChargeCard_TaskId, CHARGECARD_RESET_EVENT, 25);
    }

}
void ChargeCard_Restart()
{
    EA = 0;
    ChargeCard_SaveParam2RAM();
    STARTWATCHDOG(DOGTIMER_INTERVAL_2MS);  //set to 15ms to ensure the last frame is send out
    while(1);
}

void ChargeCard_SaveParam2RAM(void)
{

    Dev_Param_t param;
    param.setram_flag = SETRAM_FLAG;
    param.blast_param = blast_param;
    param.system_param = system_param;

    *((Dev_Param_t *)(CHARGECARD_PARAM_ADDR)) = param; // save parameters to idata ram

}
void ChargeCard_ReadParmFromRAM(void)
{
    Dev_Param_t param = *((Dev_Param_t *)(CHARGECARD_PARAM_ADDR));

    if(param.setram_flag == SETRAM_FLAG)
    {
        system_param = param.system_param;
        blast_param = param.blast_param;
    }
    osal_memset((void * )CHARGECARD_PARAM_ADDR, 0, sizeof(Dev_Param_t));
}

bool ChargeCard_SetFixedTimetag(uint8 tag, uint16 time)
{
    if(tag >= CHARGECARD_FIXTAG_LEN)
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
void ChargeCard_UnSetFixedTimetag(uint8 tag)
{
time_tag.fixtag[tag].isworking = false;
time_tag.fixtag[tag].winstart = 0;
time_tag.fixtag[tag].winend = 0;
return;
}*/

void ChargeCard_ProcTimeTick(void)
{

    bool canSleep = true;

    uint32 tick = osal_GetSystemClock();

#ifdef WATCHDOG
    FeedWatchDog();
#endif

    /* process fixed tags */
    for(uint8 i = 0; i < CHARGECARD_FIXTAG_LEN; i++)
    {
        if(time_tag.fixtag[i].isworking == true)
        {
            if(tick < time_tag.fixtag[i].winend) //busying
            {
                canSleep = false;
            }
            else //exactly at the end
            {
                time_tag.fixtag[i].isworking = false;

                /* process event at the end */
                if((i == CHARGECARD_TAG_BLAST || i == CHARGECARD_TAG_POLL)
                        && system_param.workStatus == CHARGECARD_STATE_NORMAL) // blast end
                {
                    /* in normal mode  turn off all */
                    HalLedSet(CHARGECARD_LED_BLUE, HAL_LED_MODE_OFF);
                    HalLedSet(CHARGECARD_LED_RED, HAL_LED_MODE_OFF);
                }
            }
        }
    }


    /*process sleep */
    //tick = osal_GetSystemClock();
    if(canSleep == true && !HalBeeperBusy())
    {
        osal_pwrmgr_task_state(ChargeCard_TaskId, PWRMGR_CONSERVE);

    }
    else
    {
        if(system_param.rx_on_idle  == false)
        {
            system_param.rx_on_idle  = true;
            MAC_MlmeSetReq(MAC_RX_ON_WHEN_IDLE, &system_param.rx_on_idle );
        }
        osal_pwrmgr_task_state(ChargeCard_TaskId, PWRMGR_HOLD);
    }

    return;
}
void ChargeCard_BatteryCheck(void )
{
    /*battery check */
    static uint8 vddlowcnt = 0;
    static uint8 vddnormolcnt = 0;
    uint8 vdd;
    // if(!(vddcheck_cnt++ & 0x1F))   // 32 blast time
    {
        vdd = ChargeCard_GetBatteryValue();

        if(vdd  < CHARGECARD_BATTERY_STAT_VERYLOW)
        {
            vddlowcnt = 0;
            vddnormolcnt = 0;
            system_param.BatteryStatus = CHARGECARD_BATTERY_STAT_VERYLOW;
        }
        else if(vdd  < CHARGECARD_VDD_LIMT_LOW)
        {
            vddlowcnt++;
            vddnormolcnt = 0;

            if(vddlowcnt % 5 == 0)
            {
                system_param.BatteryStatus = CHARGECARD_BATTERY_STAT_LOW;
                vddlowcnt = 0;
            }
        }
        else if(system_param.BatteryStatus !=  CHARGECARD_BATTERY_STAT_NORMAL && vdd >= CHARGECARD_VDD_LIMT_NORMAL)
        {
            vddnormolcnt++;
            vddlowcnt = 0;

            if(vddnormolcnt % 5 == 0)
            {
                system_param.BatteryStatus = CHARGECARD_BATTERY_STAT_NORMAL;
                vddnormolcnt = 0;
            }
        }
        else /* BATTERY_STAT_NORMAL */
        {
            vddlowcnt = 0;
            vddnormolcnt = 0;
        }
    }

    /* if low battery, report.  if very low, do nothing */
    if(system_param.workStatus == CHARGECARD_STATE_NORMAL && system_param.BatteryStatus == CHARGECARD_BATTERY_STAT_LOW)
    {
        HalLedSet(CHARGECARD_LED_BLUE, HAL_LED_MODE_OFF);
        HalLedBlink(CHARGECARD_LED_RED, 1, 50, 300);

        APPWrapper_t    Appdata;
        Appdata.app_Urgent.msgtype = URGENT;
        Appdata.app_Urgent.urgenttype = NOPWR;

        /* 0xFD and 0xFE for spectial protocol, */
        Appdata.app_Urgent.value = vdd<URGENT_NOPWR_RECVRETREAT ? vdd:0;

        /* 300 ms avoid sleep, flash RED LED*/
        ChargeCard_SetFixedTimetag(CHARGECARD_TAG_BLAST, 300);
        MAC_UTIL_BuildandSendData((uint8 *)&Appdata, sizeof(app_Urgent_t), MAC_UTIL_UNICAST, 0, NULL);

    }
    return;
}


uint8  ChargeCard_GetBatteryValue(void)
{
#define ChargeCard_VCC40V  (24496>>4)
#define ChargeCard_VCC39V  (23888>>4)
#define ChargeCard_VCC38V  (23328>>4)
#define ChargeCard_VCC37V  (22704>>4)
#define ChargeCard_VCC36V  (21984>>4)
#define ChargeCard_VCC35V  (21552>>4)
#define ChargeCard_VCC34V  (20868>>4)
#define ChargeCard_VCC33V  (20360>>4)


    uint16 BatteryList[] = {ChargeCard_VCC40V,
                            ChargeCard_VCC39V,
                            ChargeCard_VCC38V,
                            ChargeCard_VCC37V,
                            ChargeCard_VCC36V,
                            ChargeCard_VCC35V,
                            ChargeCard_VCC34V,
                            ChargeCard_VCC33V
                           };

    uint16 adcvalue  = HalAdcRead2(HAL_ADC_CHANNEL_7, HAL_ADC_RESOLUTION_12, HAL_ADC_REF_125V);

    for(uint8 i = 0; i < sizeof(BatteryList) / sizeof(BatteryList[0]); i++)
    {
        if(adcvalue > BatteryList[i])
        {
            return (40 - i);  //0->33 1->32 ... 3.3V, 3.2V
        }
    }
    return 32;            // default is 3.2V
}

void ChargeCard_SetAlert(void)
{
    if(system_param.workStatus != CHARGECARD_STATE_ALERT)
    {
        system_param.workStatus_bk = system_param.workStatus;
    }
    system_param.workStatus = CHARGECARD_STATE_ALERT;
    system_param.AlertSuccess = false;
    system_param.AlertCnt = CHARGECARD_ALERT_TIME;

    HalStartBeeper(BEEP_TYPE_ALERT, CHARGECARD_BEEPFREQ_ALERT);

    if(ZSuccess != osal_set_event(ChargeCard_TaskId, CHARGECARD_ALERT_EVENT))
    {
        SystemReset();
    }
}
void ChargeCard_UnSetAlert(void)
{
    system_param.workStatus = system_param.workStatus_bk;
    if(system_param.workStatus == CHARGECARD_STATE_NORMAL)
    {
        HalLedSet(CHARGECARD_LED_RED, HAL_LED_MODE_OFF);
        HalLedSet(CHARGECARD_LED_BLUE, HAL_LED_MODE_OFF);
    }
    HalStopBeeper(BEEP_TYPE_ALERT, TRUE);
}

void ChargeCard_SetRetreat(void)
{
    if(!system_param.isOKPressed)
    {
        if(system_param.workStatus == CHARGECARD_STATE_ALERT)
        {
            system_param.workStatus_bk = CHARGECARD_STATE_URGENT;
        }
        else
        {
            if(system_param.workStatus != CHARGECARD_STATE_URGENT)
            {
                system_param.workStatus = CHARGECARD_STATE_URGENT;
                recv_retreat_cnt = 0;
            }
        }
    }
}
void ChargeCard_UnSetRetreat(void)
{
    if(system_param.workStatus == CHARGECARD_STATE_ALERT)
    {
        system_param.workStatus_bk = CHARGECARD_STATE_NORMAL;
    }
    else if(system_param.workStatus == CHARGECARD_STATE_URGENT)
    {
        system_param.workStatus = CHARGECARD_STATE_NORMAL;
    }
}

void ChargeCard_ContinueEvents(void)
{
    if(system_param.AlertCnt > 0)
    {
        HalStartBeeper(BEEP_TYPE_ALERT, BEEP_TYPE_ALERT);
        osal_set_event(ChargeCard_TaskId, CHARGECARD_ALERT_EVENT);
    }
    if(system_param.OKPressCnt > 0 && system_param.OKPressCnt <= CHARGECARD_RETREATSUPPRESS_TIME)
    {
        system_param.isOKPressed = TRUE;
        osal_set_event(ChargeCard_TaskId, CHARGECARD_OK_PRESS_EVENT);
    }
    else
    {
        system_param.isOKPressed = FALSE;
        system_param.OKPressCnt  = 0;
    }
}
/**************************************************************************************************
**************************************************************************************************/

