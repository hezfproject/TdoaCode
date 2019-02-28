/**************************************************************************************************
Filename:       med.c
Revised:        $Date: 2011/08/31 22:51:29 $
Revision:       $Revision: 1.45 $

Description:    This file contains the application that can be use to set a device as End
Device from MAC directly which will communicate with a FFD with full z-stack.

**************************************************************************************************/
/**************************************************************************************************

Work FLow:  poll---process polled data------blast---sleep
|                                               |
|       ---  ploll until no data ---     |
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
//#include "hal_led.h"
/* OS includes */
#include "OSAL.h"
#include "OSAL_Tasks.h"
//#include "OSAL_PwrMgr.h"

/* App Protocol*/
#include "AppProtocolWrapper.h"

/* Application Includes */
#include "OnBoard.h"

/* MAC Application Interface */
#include "mac_api.h"

/* Application */
#include "med.h"

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

typedef struct
{
    //   uint16        med_PanId,
    //   uint16        med_PANCoordShortAddr,
    //   uint16        med_DevShortAddr,

    uint16 med_sleeptime;
    uint16 med_poll_interval;
    uint16 med_poll_timeout;

    uint8    med_WorkState;
    uint8    med_WorkState_bk;
    bool     med_IsBatteryLow;

    //   uint8 med_SuperFrameOrder,
    //   uint8 med_BeaconOrder,


    uint8 med_blast_cnt ;
    uint16 med_seqnum;

    bool     med_AlertSuccess;
    uint8    med_AlertCnt;
    uint8    med_urgent_cnt;
    bool     med_Confirm;
    uint8    med_ConfirmCnt;
} Dev_Param_t;

/**************************************************************************************************
*                                           Constant
**************************************************************************************************/
#define MED_MAC_PAYLOAD_LEN          127                  //mac payload length by datasheet.
#define MED_SHORTADDR_PANCOORD       0x0000             /* Short Address of PAN Coordinator, will always be 0x0000 */
//#define MED_SHORTADDR_COORD           0x0000                   /* Short Address of Coordinator, Initial Value */
#define MED_BROADCAST_SHORTADDR_DEVZCZR 0xFFFC   /* Short Address of all coords and routers*/
#define MED_BROADCAST_SHORTADDR_DEVALL      0xFFFF   /* Short Address of all Devs*/

#define MED_KEY_SW_7_PXIFG    P2IFG
#define MED_KEY_SW_7_PORT     P2
#define MED_KEY_SW_7_BIT       0x0001

#define SLEEP_RST_POS       3
#define WDCTL_INT_POS       0
#define WDCTL_MODE_POS  2
#define WDCTL_EN_POS        3
#define WDCTL_CLR_POS       4

/* Application Work State */
/* The priority is: Alert > Urgent > Normal */

#define MED_STATE_NORMAL        0
#define MED_STATE_URGENT        1
#define MED_STATE_ALERT         2
#ifdef USE_STATE_UNINIT
#define MED_STATE_UNINIT        3
#endif

/* Every minute launch 12 frames, each frame five (5) seconds.
*  Every two OUT and a POLL constitute a cycle.
*/
#ifndef SAVEPOLL
    #define MED_NOSIGNAL            40
#elif SAVEPOLL > 0
    #define MED_NOSIGNAL            (4 * SAVEPOLL)
#endif
// wait 3cycle(9 frame) + 2OUT = 11 frames equals 55s
#define MED_SLEEP_POLL          (MED_NOSIGNAL + 11)
// 60 seconds when it launched a POOL
#define MED_GOTO_POLL           (MED_NOSIGNAL - 1)

#define MED_WORKMODE_NORMAL    0
#define MED_WORKMODE_EXAM      1

#define MED_UNICAST         0
#define MED_BROADCAST       1

//#define MED_VER           0
//#define MED_SHORTADDR     1
#define MED_EXITADDR        2

//#define MED_DEVINFO_ADDR      0xFFF0          //for test only

#define MED_PARAM_ADDR          0xFD00      //0xFD00-0xFD53 are used to store paramters
#define MED_DEVINFO_ADDR        0x7FF8

#define MED_MIN_POWER              25

#define MED_LED_BLUE    0x01
#define MED_LED_RED      0x02

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
/* Time out protect for each task, if current task can not be  finished in protect timeout, force device to initial*/
//#define MED_TIME_OUT_PROTECT()  st(osal_stop_timerEx(MED_TaskId, MED_TIMEOUT_EVENT);osal_start_timerEx(MED_TaskId, MED_TIMEOUT_EVENT,MED_PROTECT_TIMEOUT); )

/* Size table for MAC structures */
const CODE uint8 med_cbackSizeTable [] =
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
#ifdef DEBUG
static uint8        med_version;
#endif
/* Coordinator and Device information */
static uint16        med_PanId = CARD_NWK_ADDR;                      /* 0xFFF0 */
//static uint16        med_CoordShortAddr = MED_SHORTADDR_PANCOORD; /* Initial */
static uint16        med_PANCoordShortAddr = MED_SHORTADDR_PANCOORD; /* Always be 0x00 */
static uint16        med_DevShortAddr   = MED_BROADCAST_SHORTADDR_DEVALL; /* Initial */


/* TRUE and FALSE value */
static bool          med_MACTrue = TRUE;
static bool          med_MACFalse = FALSE;

/* Beacon payload, this is used to hold beacon information. */

//uint8         med_BeaconPayload[20];
//uint8         med_BeaconPayloadLen = 0;
//static uint8     med_macPayload[MED_MAC_PAYLOAD_LEN];


/* flags used in the application */
#ifdef RTR_NWK
static bool     med_IsCoordinator  = FALSE;   /* True if the device is started as a Pan Coordinate */
#endif

static uint16 med_sleeptime;
static uint16 med_poll_interval;
static uint16 med_poll_timeout;

static uint8    med_WorkState       =   MED_STATE_NORMAL;   /*Should be UNINIT, Normal   Urgent Alert */
static uint8    med_WorkState_bk  =   MED_STATE_NORMAL;
static bool     med_IsBatteryLow   =  FALSE;

/* Beacon order */
static uint8 med_SuperFrameOrder = MED_MAC_SUPERFRAME_ORDER;
static uint8 med_BeaconOrder = MED_MAC_BEACON_ORDER;

/* Task ID */
uint8 MED_TaskId;

/* counter */
static uint8 med_blast_cnt = 0;

/* Device Info from flash */
static Dev_Info_t med_DevInfo;

/* med_seqnum */
static uint16 med_seqnum = 0;

static bool     med_AlertSuccess = FALSE;
static uint8    med_AlertCnt = 0;
static uint8    med_urgent_cnt = 0;

/*med confirm */
static bool     med_Confirm = false;
static uint8    med_ConfirmCnt = 0;

#ifdef USE_STATE_UNINIT
static bool     med_isUninit = false;
#endif

static uint8   med_KeyConfirmRetransCnt = 0;

static uint8 s_u8SignNum;
/**************************************************************************************************
*                                     Local Function Prototypes
**************************************************************************************************/
/* Setup routines */
static void         MED_DeviceStartup(uint8 *pData);
//static void         MED_McpsDataReq(uint8* data, uint8 dataLength, sAddr_t dstAddr,uint8 txOption);
//static void         MED_McpsPollReq(void);

/* Support */
static void         MED_blast(uint8 reqtype);
static bool         MED_TestLongPress(uint16 TimeOut);
static void         MED_Delay(uint16 timeout);
static void         MED_LedBlink(uint8 leds, uint8 numBlinks, uint8 percent, uint16 period);
static void         MED_ReadDevInfo();
#ifdef DEBUG
static void         MED_SetDevInfo(uint8 type, void *pData);
#endif
/*
static void         MED_StartWatchDog(void);
static bool         MED_IsResetFromDog(void);
static void         MED_FeedWatchDog(void) ;
*/
static void        MED_IntervalCheck(void);
static void        MED_PeriodReset();
static void        MED_Restart();
static void     MED_SaveParam2RAM(void);
static void     MED_ReadParmFromRAM(void);
static void 	MED_BatteryCheck(void);
/**************************************************************************************************
*
* @fn          MED_Init
*
* @brief       Initialize the application
*
* @param       taskId - taskId of the task after it was added in the OSAL task queue
*
* @return      none
*
**************************************************************************************************/
void MED_Init(uint8 taskId)
{
    /* Initialize the task id */
    MED_TaskId = taskId;

    /* initialize MAC features */
    MAC_Init();
    MAC_InitDevice();

    /* Reset the MAC */
    MAC_MlmeResetReq(TRUE);

    /* Start Watch Dog */

    /* initial MacUtil*/
    MacUtil_t Macutil;
    Macutil.panID = 0xFFFF;                          // Card broadcast to all PANs
    Macutil.dst_endpoint = APS_DST_ENDPOINT;
    Macutil.src_endpoint = APS_SRC_ENDPOINT;
    Macutil.cluster_id = APS_CLUSTER_ID;
    Macutil.profile_id = APS_PROFILE_ID;
    Macutil.NodeType =  NODETYPE_DEVICE;
    MAC_UTIL_INIT(&Macutil);

    med_sleeptime = MED_SLEEP_PERIOD;
    med_poll_interval = MED_POLL_INTERVAL;
    med_poll_timeout = MED_POLL_TIMEOUT;
    med_blast_cnt = 0;

    MED_ReadDevInfo();

    MED_DeviceStartup(NULL);

    /* save param to Ram, if watchdog reset by accident,this value is default */
    MED_SaveParam2RAM();

    TURN_OFF_LED_RED();
    TURN_OFF_LED_BLUE();

#ifdef USE_STATE_UNINIT
    med_isUninit = false;
    if(GetResetFlag() == RESET_FLAG_POWERON)
    {

        /* Goto Uninit state*/
        med_isUninit = true;
        med_WorkState  = MED_STATE_UNINIT;//MED_STATE_UNINIT
        osal_set_event(MED_TaskId, MED_UNINIT_EVENT);   //MED_ALERT_EVENT);//MED_UNINIT_EVENT
    }
    else
#endif
    {
#ifdef WATCHDOG_TEST
        MED_LedBlink(MED_LED_RED, 10, 30, 100);
#endif

        /*Start Watch Dog*/
#ifdef WATCHDOG
        StartWatchDog(DOGTIMER_INTERVAL_1S);
#endif

        /* goto sleep state */
        med_WorkState  = MED_STATE_NORMAL;
        osal_set_event(MED_TaskId, MED_SLEEP_EVENT);   //MED_ALERT_EVENT);//MED_UNINIT_EVENT
    }
}

/**************************************************************************************************
*
* @fn          MED_ProcessEvent
*
* @brief       This routine handles events
*
* @param       taskId - ID of the application task when it registered with the OSAL
*              events - Events for this task
*
* @return      16bit - Unprocessed events
*
**************************************************************************************************/
uint16 MED_ProcessEvent(uint8 taskId, uint16 events)
{
    uint8 *pMsg;
    macCbackEvent_t *pData;

    if(events & SYS_EVENT_MSG)
    {
        while((pMsg = osal_msg_receive(MED_TaskId)) != NULL)
        {
            switch(*pMsg)
            {

            case MAC_MCPS_DATA_CNF:
            {
                pData = (macCbackEvent_t *) pMsg;
                osal_msg_deallocate((uint8 *) pData->dataCnf.pDataReq);
                break;
            }

            case MAC_MCPS_DATA_IND:
                /* Proess Command */
                pData = (macCbackEvent_t *) pMsg;

                if(MAC_UTIL_GetClusterID(pData->dataInd.msdu) == CARD_CLUSTERID)
                {
                    sData_t AppData = MAC_UTIL_RemoveHeader(pData->dataInd.msdu);

                    s_u8SignNum = 0;

                    switch(*((MSGType *)AppData.p))
                    {
                    case URGENT:

                        switch(((app_Urgent_t *)AppData.p)->urgenttype)
                        {
                        case    RETREAT:
                            if(!med_Confirm && med_WorkState == MED_STATE_NORMAL)
                            {
                                med_urgent_cnt = 0;
                                med_WorkState = MED_STATE_URGENT;
                            }

                            break;
                        case    CANCELRETREAT:
                            if(med_WorkState == MED_STATE_URGENT)
                            {
                                med_WorkState = MED_STATE_NORMAL;
                            }
                            break;
                        case   ALERTACK:
                            med_AlertSuccess = TRUE;
                            break;
                        }
                        if(med_WorkState != MED_STATE_URGENT)
                        {
                            osal_start_timerEx(MED_TaskId, MED_SLEEP_EVENT, 1);
                        }
                        break;

                    case SSREQ:
                        /* if received no data, goto sleep directly */
                        if(((app_ssReq_t *)AppData.p)->reqtype == SSREQ_NODATA)
                        {
                            if(med_WorkState != MED_STATE_URGENT)
                            {
                                osal_start_timerEx(MED_TaskId, MED_SLEEP_EVENT, 1);
                            }
                        }
                        break;

                    case  NODESLEEP:
                    {
                        SleepType  type = ((app_Sleep_t *)AppData.p)->sleeptype;
                        if(type == SLEEPTIME)
                            med_sleeptime = ((app_Sleep_t *)AppData.p)->value;
                        else if(type == POLL_INTERVAL)
                            med_poll_interval = ((app_Sleep_t *)AppData.p)->value;
                        else if(type == POLL_TIMEOUT)
                            med_poll_timeout = ((app_Sleep_t *)AppData.p)->value;
                        /*
                        med_sleepcfg.sleeptime= ((app_Sleep_t*)AppData.p)->sleeptime;
                        med_sleepcfg.poll_interval = ((app_Sleep_t*)AppData.p)->poll_interval;
                        med_poll_timeout= ((app_Sleep_t*)AppData.p)->poll_timeout;   */
                        MED_LedBlink(MED_LED_RED, 3, 50, 300);
                        break;
                    }
                    case PWRCTL:
                    {
                        uint8 power = ((app_PWRCTL_t *)AppData.p)->workpower;
                        if(power == 0)
                        {
                            MAC_MlmeSetReq(MAC_PHY_TRANSMIT_POWER, &power);
                        }
                        else if(power <= MED_MIN_POWER)
                        {
                            power += 5;    //see mac_radio_defs.c      line 56
                            MAC_MlmeSetReq(MAC_PHY_TRANSMIT_POWER, &power);
                        }

                        break;
                    }
                    default:
                        break;
                    }
                }
                break;
            }
            /* Deallocate */
            osal_msg_deallocate((uint8 *) pMsg);
        }
        return events ^ SYS_EVENT_MSG;
    }

    if(events & MED_SEND_KEYCONFIRM_EVT)
    {
        if(--med_KeyConfirmRetransCnt > 0)
        {
            app_Urgent_t app_Urgent;
            app_Urgent.msgtype = URGENT;
            app_Urgent.urgenttype = NOPWR;
            app_Urgent.value = URGENT_NOPWR_KEYCONFIRM;
            MAC_UTIL_BuildandSendData((uint8 *)&app_Urgent,
                                sizeof(app_Urgent), MED_UNICAST, 0, NULL);
            osal_start_timerEx(MED_TaskId, MED_SEND_KEYCONFIRM_EVT, 1000);
        }
        else  // send finished , go  to sleep
        {
            osal_start_timerEx(MED_TaskId, MED_SLEEP_EVENT, 1);
        }
        return events ^ MED_SEND_KEYCONFIRM_EVT;
    }

    if(events & MED_SEND_RECVRETREAT_EVT)
    {
        if(med_WorkState ==  MED_STATE_URGENT)
        {
            app_Urgent_t app_Urgent;
            app_Urgent.msgtype = URGENT;
            app_Urgent.urgenttype = NOPWR;
            app_Urgent.value = URGENT_NOPWR_RECVRETREAT;
            MAC_UTIL_BuildandSendData((uint8 *)&app_Urgent,
                            sizeof(app_Urgent), MED_UNICAST, 0, NULL);
        }
        return events ^ MED_SEND_RECVRETREAT_EVT;
    }

    if(events & MED_ALERT_EVENT)
    {
        med_AlertCnt++;
        if(med_AlertSuccess == false)
        {
            app_Urgent_t app_Urgent;
            app_Urgent.msgtype = URGENT;
            app_Urgent.urgenttype = ALERT;
            app_Urgent.value = 0;

            MAC_MlmeSetReq(MAC_RX_ON_WHEN_IDLE, &med_MACTrue);
            MAC_UTIL_BuildandSendData((uint8 *)&app_Urgent,
                            sizeof(app_Urgent_t), MED_UNICAST, 0, NULL);

            osal_start_timerEx(MED_TaskId, MED_SLEEP_EVENT, MED_ALERT_TIMEOUT);
        }
        else
        {
            osal_start_timerEx(MED_TaskId, MED_SLEEP_EVENT, 1);
        }
        return events ^ MED_ALERT_EVENT;
    }


    if(events & MED_BLAST_EVENT)
    {
        /* set blast LED */
        if(!med_IsBatteryLow && med_WorkState != MED_STATE_URGENT)
        {
            TURN_ON_LED_BLUE();
        }

        /* Send out  message */
        med_blast_cnt++;
        if(s_u8SignNum < MED_NOSIGNAL && !(med_blast_cnt % med_poll_interval))
        {
            s_u8SignNum++;
            MAC_MlmeSetReq(MAC_RX_ON_WHEN_IDLE, &med_MACTrue);
            MED_blast(SSREQ_POLL);
            if(med_WorkState == MED_STATE_URGENT)
            {
                osal_start_timerEx(MED_TaskId, MED_SLEEP_EVENT, MED_URGENT_POLL_TIMEOUT);
            }
            else
            {
                osal_start_timerEx(MED_TaskId, MED_SLEEP_EVENT, med_poll_timeout);
            }
        }
        else
        {   // If not received signal then into a minute a POLL
            if (s_u8SignNum >= MED_NOSIGNAL && ++s_u8SignNum >= MED_SLEEP_POLL)
            {
                s_u8SignNum = MED_GOTO_POLL;
            }

            MED_blast(SSREQ_OUT);
            osal_start_timerEx(MED_TaskId, MED_SLEEP_EVENT, 1);
        }

        /* in urgent state, send out recvretreat */
        if(med_WorkState ==  MED_STATE_URGENT && med_urgent_cnt < 6)
        {
            osal_start_timerEx(MED_TaskId, MED_SEND_RECVRETREAT_EVT, 20);
            osal_start_timerEx(MED_TaskId, MED_SLEEP_EVENT, 100);
        }

        return events ^ MED_BLAST_EVENT;
    }

    if(events & MED_SLEEP_EVENT)   /* Do battery_check, and sleep*/
    {
#ifdef WATCHDOG
        FeedWatchDog();
#endif

        /* turn off blast LED  */
        TURN_OFF_LED_BLUE();

        /* if the key confirm retrans is not completed, send it first */
        if(med_KeyConfirmRetransCnt > 0)
        {
            // osal_start_timerEx(MED_TaskId, MED_SEND_KEYCONFIRM_EVT, 1000);
            return events ^ MED_SLEEP_EVENT;
        }

        MAC_MlmeSetReq(MAC_RX_ON_WHEN_IDLE, &med_MACFalse);
        while(MAC_PwrOffReq(MAC_PWR_SLEEP_DEEP) != MAC_SUCCESS);

        MED_PeriodReset();

        uint8 RandDelay = MAC_RandomByte();
        RandDelay >>= 2;                                                  // random delay 0-64 ms

        if(med_ConfirmCnt > 0 && --med_ConfirmCnt == 0)
        {
            med_Confirm = false;
        }

        /* Normal: Begin to sleep        Urgent: blink*/
        if(med_WorkState == MED_STATE_NORMAL)
        {
            /*Turn Off radio first */
            if(med_sleeptime != 0)
            {
                UtilSleep(CC2430_PM2, med_sleeptime + RandDelay);
            }
            osal_start_timerEx(MED_TaskId, MED_BLAST_EVENT, 1);
        }
        else if(med_WorkState == MED_STATE_URGENT)
        {
            if (med_urgent_cnt < 6)
            {
                med_urgent_cnt++;
            }

            if(med_Confirm)
            {
                med_WorkState = MED_STATE_NORMAL;
            }

            /* Blink LED */
            MED_LedBlink(MED_LED_RED, 4, 30, 150);        // blink, unblock
            UtilSleep(CC2430_PM2, MED_URGENT_SLEEP_PERIOD + RandDelay);
            osal_start_timerEx(MED_TaskId, MED_BLAST_EVENT, 1);
        }
        else if(med_WorkState == MED_STATE_ALERT)
        {
            if(med_AlertCnt > MED_ALERT_TIME) /* alert end */
            {
                if(med_WorkState_bk == MED_STATE_NORMAL
                    || med_WorkState_bk == MED_STATE_URGENT)
                {
                    med_WorkState = med_WorkState_bk;
                }
                else
                {
                    med_WorkState = MED_STATE_NORMAL;
                }
                osal_start_timerEx(MED_TaskId, MED_BLAST_EVENT, 1);
            }
            else                            /* alerting */
            {
                if(med_AlertCnt + 3 > MED_ALERT_TIME && med_AlertSuccess)   // if sucess, last 3 seconds flash blue
                {
                    MED_LedBlink(MED_LED_BLUE, 2, 30, 500);
                }
                else
                {
                    MED_LedBlink(MED_LED_RED, 2, 30, 500);
                }
                osal_start_timerEx(MED_TaskId, MED_ALERT_EVENT, 1);
            }

        }

        MAC_PwrOnReq(); // turn on mac
        MED_BatteryCheck();

        return events ^ MED_SLEEP_EVENT;
    }
#ifdef USE_STATE_UNINIT
    if(events & MED_UNINIT_EVENT)
    {
        if(med_isUninit == true)
        {
            /* sleep until iniitial */
            bool longpress;
            longpress = FALSE;

            /* turn off radio first */
            MAC_MlmeSetReq(MAC_RX_ON_WHEN_IDLE, &med_MACFalse);
            while(MAC_PwrOffReq(MAC_PWR_SLEEP_DEEP) != MAC_SUCCESS);
            while(!longpress)
            {
                UtilSleep(CC2430_PM3, 0);
                longpress = MED_TestLongPress(MED_KEY_PRESS_TIME_INIT);
            }

            HAL_DISABLE_INTERRUPTS();

            MAC_PwrOnReq();
            med_WorkState = MED_STATE_NORMAL;

            /*Blink LED*/
            TURN_ON_LED_BLUE();
            MED_Delay(MED_LED_FLASH_TIME_INIT);
            TURN_OFF_LED_BLUE();

            //MAC_MlmeSetReq(MAC_RX_ON_WHEN_IDLE, &med_MACTrue);
            /*Start Watch Dog*/
            MED_SaveParam2RAM();
#ifdef WATCHDOG
            StartWatchDog(DOGTIMER_INTERVAL_1S);
#endif

            HAL_ENABLE_INTERRUPTS();
            med_blast_cnt = 0;
            osal_start_timerEx(MED_TaskId, MED_BLAST_EVENT, 1);

            med_isUninit = false;
        }
        return events ^ MED_UNINIT_EVENT;
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

    uint8 len = med_cbackSizeTable[pData->hdr.event];

    switch(pData->hdr.event)
    {
    case MAC_MLME_BEACON_NOTIFY_IND:

        len += sizeof(macPanDesc_t) + pData->beaconNotifyInd.sduLength +
               MAC_PEND_FIELDS_LEN(pData->beaconNotifyInd.pendAddrSpec);
        if((pMsg = (macCbackEvent_t *) osal_msg_allocate(len)) != NULL)
        {
            /* Copy data over and pass them up */
            osal_memcpy(pMsg, pData, sizeof(macMlmeBeaconNotifyInd_t));
            pMsg->beaconNotifyInd.pPanDesc
                = (macPanDesc_t *)((uint8 *) pMsg
                + sizeof(macMlmeBeaconNotifyInd_t));

            osal_memcpy(pMsg->beaconNotifyInd.pPanDesc,
                pData->beaconNotifyInd.pPanDesc, sizeof(macPanDesc_t));

            pMsg->beaconNotifyInd.pSdu
                = (uint8 *)(pMsg->beaconNotifyInd.pPanDesc + 1);

            osal_memcpy(pMsg->beaconNotifyInd.pSdu,
                pData->beaconNotifyInd.pSdu, pData->beaconNotifyInd.sduLength);
        }
        break;

    case MAC_MCPS_DATA_IND:
        pMsg = pData;
        break;
    default:
        if((pMsg = (macCbackEvent_t *) osal_msg_allocate(len)) != NULL)
        {
            osal_memcpy(pMsg, pData, len);
        }
        break;
    }

    if(pMsg != NULL)
    {
        osal_msg_send(MED_TaskId, (byte *) pMsg);
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
* @fn      MED_DeviceStartup(uint8* pData)
*
* @brief   Update the timer per tick
*
* @param   beaconEnable: TRUE/FALSE
*
* @return  None
*
**************************************************************************************************/
void MED_DeviceStartup(uint8 *pData)
{
    MAC_MlmeSetReq(MAC_EXTENDED_ADDRESS, &med_DevInfo.ExitAddr);

    /* Setup MAC_BEACON_PAYLOAD_LENGTH */
    //MAC_MlmeSetReq(MAC_BEACON_PAYLOAD_LENGTH, &med_BeaconPayloadLen);

    /* Setup MAC_BEACON_PAYLOAD */
    //MAC_MlmeSetReq(MAC_BEACON_PAYLOAD, &med_BeaconPayload);

    /* Setup PAN ID */
    MAC_MlmeSetReq(MAC_PAN_ID, &med_PanId);

    /* This device is setup for Direct Message */
    MAC_MlmeSetReq(MAC_RX_ON_WHEN_IDLE, &med_MACFalse);

    /* Setup Coordinator short address */
    MAC_MlmeSetReq(MAC_COORD_SHORT_ADDRESS, &med_PANCoordShortAddr);

    /* Setup Beacon Order */
    MAC_MlmeSetReq(MAC_BEACON_ORDER, &med_BeaconOrder);

    /* Setup Super Frame Order */
    MAC_MlmeSetReq(MAC_SUPERFRAME_ORDER, &med_SuperFrameOrder);


    //uint8 tmp8 = MED_MAC_CHANNEL;
    uint8 tmp8 = med_DevInfo.ExitAddr[EXT_MACADDR_CHANNEL];
    MAC_MlmeSetReq(MAC_LOGICAL_CHANNEL, &tmp8);

    MAC_MlmeSetReq(MAC_SHORT_ADDRESS, &med_DevShortAddr);

    MAC_MlmeSetReq(MAC_ASSOCIATED_PAN_COORD, &med_MACTrue);

}

#if 0
/**************************************************************************************************
*
* @fn      MED_McpsDataReq()
*
* @brief   This routine calls the Data Request
*
* @param   data       - contains the data that would be sent
*          dataLength - length of the data that will be sent
*
* @return  None
*
**************************************************************************************************/
void MED_McpsDataReq(uint8 *data, uint8 dataLength, sAddr_t dstAddr, uint8 txOption)
{

    macMcpsDataReq_t  *pData;
    static uint8      handle = 0;

    if((pData = MAC_McpsDataAlloc(dataLength, MAC_SEC_LEVEL_NONE, MAC_KEY_ID_MODE_NONE)) != NULL)
    {
        pData->mac.srcAddrMode = SADDR_MODE_SHORT;
        pData->mac.dstAddr = dstAddr;
        pData->mac.dstPanId = med_PanId;
        pData->mac.msduHandle = handle++;
        pData->mac.txOptions = txOption;
        pData->sec.securityLevel = false;

        // If it's the coordinator and the device is in-direct message
        //if (med_IsCoordinator)
        // {
        //  if (!directMsg)
        //  {
        //    pData->mac.txOptions |= MAC_TXOPTION_INDIRECT;
        //  }
        // }

        osal_memcpy(pData->msdu.p, data, dataLength);
        pData->msdu.len = dataLength;

        MAC_McpsDataReq(pData);
    }

}

/**************************************************************************************************
*
* @fn      MED_McpsPollReq()
*
* @brief   Performs a poll request on the coordinator
*
* @param   None
*
* @return  None
*
**************************************************************************************************/
void MED_McpsPollReq(void)
{
    macMlmePollReq_t  pollReq;

    /* Fill in information for poll request */
    pollReq.coordAddress.addrMode = SADDR_MODE_SHORT;
    pollReq.coordAddress.addr.shortAddr = med_PANCoordShortAddr;
    pollReq.coordPanId = med_PanId;
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
void MED_ScanReq(uint8 scanType, uint8 scanDuration)
{
    macMlmeScanReq_t scanReq;
#ifdef DEBUG
    HalLedSet(HAL_LED_1, HAL_LED_MODE_ON);
    MED_Delay(500);
    HalLedSet(HAL_LED_1, HAL_LED_MODE_OFF);
#endif
    /* Fill in information for scan request structure */
    scanReq.scanChannels = (uint32) 1 << MED_MAC_CHANNEL;
    scanReq.scanType = scanType;
    scanReq.scanDuration = scanDuration;
    scanReq.maxResults = MED_MAC_MAX_RESULTS;
    scanReq.result.pPanDescriptor = med_PanDesc;

    /* Call scan request */
    MAC_MlmeScanReq(&scanReq);
}

/**************************************************************************************************
*
* @fn      MED_SyncReq()
*
* @brief   Sync Request
*
* @param   None
*
* @return  None
*
**************************************************************************************************/
void MED_SyncReq(void)
{
    macMlmeSyncReq_t syncReq;

    /* Fill in information for sync request structure */
    syncReq.logicalChannel = MED_MAC_CHANNEL;
    syncReq.channelPage    = MAC_CHANNEL_PAGE_0;
    syncReq.trackBeacon    = FALSE;

    /* Call sync request */
    MAC_MlmeSyncReq(&syncReq);
}
#endif
/**************************************************************************************************
*
* @fn      MED_HandleKeys
*
* @brief   Callback service for keys
*
* @param   keys  - keys that were pressed
*          state - shifted
*
* @return  void
*
**************************************************************************************************/
void MED_HandleKeys(uint16 keys, uint8 shift)
{
#ifdef USE_STATE_UNINIT
    if(med_WorkState != MED_STATE_UNINIT)
#endif
    {
#ifdef DEBUG
        TURN_ON_LED_RED();
        MED_Delay(10);
        TURN_OFF_LED_RED();
#endif
        uint8  red_state, blue_state;
        BACK_AND_OFF_LED(blue_state, red_state);
        uint16 timeout;
        /* If The ISR fun is run from sleep, the clock is 16MHz,else the clock is 32MHz*/
        if(CLKCON & 0x01 == 0)
        {
            timeout = MED_KEY_PRESS_TIME_ALERT;
        }
        else
        {
            timeout = MED_KEY_PRESS_TIME_ALERT / 2;
        }


        if(MED_TestLongPress(timeout))  /* Alert */
        {

            if(med_WorkState != MED_STATE_ALERT)
            {
                med_WorkState_bk = med_WorkState;
            }
            med_WorkState = MED_STATE_ALERT;
            med_AlertSuccess = FALSE;
            med_AlertCnt = 0;
            osal_start_timerEx(MED_TaskId, MED_ALERT_EVENT, 1);
        }
        else                            /* Confirm */
        {
            // if state is urgent, send keyconfirm
            if(med_WorkState ==  MED_STATE_URGENT)
            {
                med_KeyConfirmRetransCnt = 20;
                osal_start_timerEx(MED_TaskId, MED_SEND_KEYCONFIRM_EVT, 1);
            }
            med_Confirm = true;
            med_ConfirmCnt = MED_MAX_CONFIRM_CNT;
            med_WorkState = MED_STATE_NORMAL;
        }
        SET_LED_BLUE(blue_state);
        SET_LED_RED(red_state);
    }

}

#if 0
/**************************************************************************************************
*
* @fn      MED_selectBestScanResult(macMlmeScanCnf_t *pScanCnf)
*
* @brief   Select the best scan result form scan results
*
* @param   pScanCnf
*           pdata-point to scanconf
*
*
* @return  pointer to scan results with best LQI
*
**************************************************************************************************/

macPanDesc_t *MED_selectBestScanResult(macMlmeScanCnf_t *pScanCnf)
{
    uint8 i;
    uint8 bestLQI = 0;
    macPanDesc_t *pBestScan =  pScanCnf->result.pPanDescriptor;

    for(i = 0; i < pScanCnf->resultListSize; i++)
    {
        if(pScanCnf->result.pPanDescriptor[i].linkQuality > bestLQI)
        {
            bestLQI = pScanCnf->result.pPanDescriptor[i].linkQuality;
            pBestScan = pScanCnf->result.pPanDescriptor + i;
        }
    }
    return pBestScan;
}
#endif
/**************************************************************************************************
*
* @fn      MED_blast(void)
*
* @brief   Blast once to all Coords and routers without ACK or retrans.
*
* @param
*
* @return
*
**************************************************************************************************/

void MED_blast(uint8 reqtype)
{
    MED_IntervalCheck();

    app_ssReq_t app_ssReq;
    app_ssReq.msgtype = SSREQ;
    app_ssReq.reqtype = reqtype;
    app_ssReq.NWK_ADDR = CARD_NWK_ADDR;
    app_ssReq.seqnum = med_seqnum++;
    MAC_UTIL_BuildandSendData((uint8 *)&app_ssReq,
        sizeof(app_ssReq_t), MED_UNICAST, 0 , NULL);
    //osal_mem_free(Appdata.p);
}


/* Test If a key press is a long press */
bool MED_TestLongPress(uint16 TimeOut)
{
    uint16 testInterval = 300;   // test once each 300 ms

    uint16 testnum = TimeOut / testInterval;

    for(uint16 i = 0; i < testnum; i++)
    {
        MED_Delay(testInterval);
        //halSleep(testInterval);
        if(MED_KEY_SW_7_PORT & MED_KEY_SW_7_BIT)   // low voltage when key press
        {
            return false;
        }
    }
    return true;
}

/* timeout is  in ms */
void MED_Delay(uint16 timeout)
{
    uint16 i, j, k;
    uint16 timeBig =  timeout >> 9;
    uint16 timeSmall = timeout - timeBig * 512;
    for(i = 0; i < timeBig; i++)
    {
#ifdef WATCHDOG
        FeedWatchDog();
#endif
        //MED_FeedWatchDog(); // feed dog every 512 ms
        for(j = 0; j < 512; j++)
        {
            /* One Nop counts 12/32M, So 889  cyc is a ms*/
            k = 880;//k = 889;
            while(k--)
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
    //MED_FeedWatchDog();
    for(i = 0; i < timeSmall; i++)
    {
        k = 880;//k = 889;
        while(k--)
        {
            asm("NOP");
            asm("NOP");
            asm("NOP");
        }
    }
}
/* Set Led Blink, program will blocked here*/
void    MED_LedBlink(uint8 leds, uint8 numBlinks, uint8 percent, uint16 period)
{
    uint8 i;
    uint16 ontime, offtime;
    if(percent >= 100)
    {
        ontime = period;
        offtime = 0;
    }
    else if(percent <= 0)
    {
        ontime = 0;
        offtime = period;
    }
    else
    {
        ontime = period * percent / 100;
        offtime = period - ontime;
    }
    if(leds == MED_LED_BLUE)
    {
        for(i = 0; i < numBlinks; i++)
        {
            //HalLedSet(leds, HAL_LED_MODE_ON);
            TURN_ON_LED_BLUE();
            MED_Delay(ontime);
            // HalLedSet(leds, HAL_LED_MODE_OFF);
            TURN_OFF_LED_BLUE();
            MED_Delay(offtime);
        }
    }
    else if(leds == MED_LED_RED)
    {
        for(i = 0; i < numBlinks; i++)
        {
            //HalLedSet(leds, HAL_LED_MODE_ON);
            TURN_ON_LED_RED();
            MED_Delay(ontime);
            // HalLedSet(leds, HAL_LED_MODE_OFF);
            TURN_OFF_LED_RED();
            MED_Delay(offtime);
        }
    }
}

void MED_ReadDevInfo()
{
#if 0
    /* Make a fake DEV Info*/
    med_DevInfo.ExitAddr[0] = 0x00;
    med_DevInfo.ExitAddr[1] = 0xFF;
    med_DevInfo.ExitAddr[2] = 0xFF;
    med_DevInfo.ExitAddr[3] = 0xFF;
    med_DevInfo.ExitAddr[4] = 0x00;
    med_DevInfo.ExitAddr[5] = 0x00;
    med_DevInfo.ExitAddr[6] = 0xFF;
    med_DevInfo.ExitAddr[7] = 0x00;
#else


    Dev_Info_t *p = (Dev_Info_t *)(MED_DEVINFO_ADDR);
    med_DevInfo = *p;

    if(GetResetFlag() == RESET_FLAG_WATCHDOG)
    {
        MED_ReadParmFromRAM();
    }

    /*For Card, the lowest Byte of Exit Addr should be 0x01 */
    HAL_ASSERT(med_DevInfo.ExitAddr[EXT_MACADDR_TYPE] == EXT_MACADDR_TYPE_CARD);
    //med_version = med_DevInfo.ExitAddr[EXT_MACADDR_VERSION];
    med_DevShortAddr = BUILD_UINT16(med_DevInfo.ExitAddr[EXT_MACADDR_DEVID_LBYTE],
                                    med_DevInfo.ExitAddr[EXT_MACADDR_DEVID_HBYTE]);

#endif

}
#ifdef DEBUG
void MED_SetDevInfo(uint8 type , void *pData)
{
    MED_ReadDevInfo();
    switch(type)
    {
    case   MED_EXITADDR:
    {
        uint8 *p = (uint8 *)pData;
        for(uint8 i = 0; i < 8; i++)
        {
            med_DevInfo.ExitAddr[i] = *p++;
        }
        break;
    }
    }
    FlashWrite(MED_DEVINFO_ADDR, (uint8 *)&med_DevInfo, sizeof(Dev_Info_t));

    return;
}
#endif

/*
bool MED_IsResetFromDog(void)
{
if((SLEEP & (0x03<<SLEEP_RST_POS))>>SLEEP_RST_POS == 0x02)
{
return TRUE;
}
else
{
return FALSE;
}
}
void MED_StartWatchDog(void)
{
WDCTL &= ~(0x01<<WDCTL_MODE_POS);     // set mode to watchdog mode
WDCTL &= ~(0x03<<WDCTL_INT_POS);        //set interval to 1s

WDCTL |=   (0x01<<WDCTL_EN_POS);          //Enable
}

void MED_FeedWatchDog(void) // write 0xA followed by 0x5 within o.5 watchdog clock
{
uint8 RS0,RS1;
RS0 = RS1 = WDCTL;

RS0 &= ~(0x0F<<WDCTL_CLR_POS);
RS0 |=   (0x0A<<WDCTL_CLR_POS);
RS1 &= ~(0x0F<<WDCTL_CLR_POS);
RS1 |=   (0x05<<WDCTL_CLR_POS);

WDCTL = RS0;
WDCTL = RS1;
return;
}
*/
void MED_IntervalCheck(void)
{
#define     MED_MAX_INTERVAL    20000   // 20s

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

        if(diff_ticks > MED_MAX_INTERVAL)  //  if interval > 20s, reset
        {
            MED_Restart();
        }
    }
    last_ticks = ticks;
}
void MED_PeriodReset()
{
#define MED_PERIODRESET_NUM     (1*60*60/5)   // restart per hour

    static uint16 MED_periodReset_cnt = 0;

    /* only restart in normal */
    if(med_WorkState == MED_STATE_NORMAL
        && MED_periodReset_cnt++ > MED_PERIODRESET_NUM)
    {
        MED_Restart();
    }
}

void MED_Restart()
{
    EA = 0;
    MED_SaveParam2RAM();
    STARTWATCHDOG(DOGTIMER_INTERVAL_2MS);
    while(1);
}

void MED_SaveParam2RAM(void)
{
    Dev_Param_t param;
    param.med_sleeptime = med_sleeptime;
    param.med_poll_interval = med_poll_interval;
    param.med_poll_timeout = med_poll_timeout;
    param.med_WorkState =  med_WorkState;
    param.med_WorkState_bk = med_WorkState_bk;
    param.med_IsBatteryLow = med_IsBatteryLow;
    param.med_blast_cnt = med_blast_cnt;
    param.med_seqnum = med_seqnum;
    param.med_AlertSuccess = med_AlertSuccess;
    param.med_AlertCnt = med_AlertCnt;
    param.med_urgent_cnt = med_urgent_cnt;
    param.med_seqnum = med_seqnum;
    param.med_Confirm = med_Confirm;
    param.med_ConfirmCnt = med_ConfirmCnt;

    *((Dev_Param_t *)(MED_PARAM_ADDR)) = param; // save parameters to idata ram
}
void MED_ReadParmFromRAM(void)
{
    Dev_Param_t DevParam = *((Dev_Param_t *)(MED_PARAM_ADDR));
    med_sleeptime = DevParam.med_sleeptime;
    med_poll_interval = DevParam.med_poll_interval;
    med_poll_timeout = DevParam.med_poll_timeout;
    med_WorkState = DevParam.med_WorkState;
    med_WorkState_bk = DevParam.med_WorkState_bk;
    med_IsBatteryLow = DevParam.med_IsBatteryLow;
    med_blast_cnt  = DevParam.med_blast_cnt;
    med_AlertSuccess = DevParam.med_AlertSuccess;
    med_AlertCnt = DevParam.med_AlertCnt;
    med_urgent_cnt = DevParam.med_urgent_cnt;
    med_seqnum  = DevParam.med_seqnum;
    med_Confirm = DevParam.med_Confirm;
    med_ConfirmCnt = DevParam.med_ConfirmCnt;

    /* incorrect value */
    if(med_ConfirmCnt > MED_MAX_CONFIRM_CNT)
    {
        med_Confirm = false;
        med_ConfirmCnt = 0;
    }
}

static void MED_BatteryCheck(void)
{
#define LOWBATTERY_CHECK_CNT 3

    static uint8 med_lowbatt_cnt = 0;

    if(med_WorkState == MED_STATE_NORMAL)
    {
        if( (med_blast_cnt & 0x03)==0 || med_lowbatt_cnt>0 )		//check every 20 second
        {
            if(FALSE == HalAdcCheckVdd(MED_VDD_LIMT))
            {
                /* only send when polling, there are more waiting time */
                if(med_lowbatt_cnt >= LOWBATTERY_CHECK_CNT
                    && (med_blast_cnt + 1) % med_poll_interval == 0)
                {
                    /* set status bit */
                    med_IsBatteryLow = TRUE;

                    /* send NOPWR */
                    app_Urgent_t    app_Urgent;
                    app_Urgent.msgtype = URGENT;
                    app_Urgent.urgenttype = NOPWR;
                    app_Urgent.value = URGENT_NOPWR_REALNOPWR;                //  Should be the low battery value, to be added
                    MAC_UTIL_BuildandSendData((uint8 *)&app_Urgent,
                        sizeof(app_Urgent_t), MED_UNICAST, 0, NULL);
                }
                else
                {
                    med_lowbatt_cnt++;
                }
            }
            else
            {
                med_lowbatt_cnt = 0;
                med_IsBatteryLow = FALSE;
            }
        }
        if(med_IsBatteryLow)
        {
#ifdef WATCHDOG
            FeedWatchDog();
#endif
            TURN_ON_LED_RED();
            MED_Delay(MED_LED_FLASH_TIME_LOWBATTERY);
            TURN_OFF_LED_RED();
        }
    }

}
/**************************************************************************************************
**************************************************************************************************/

