/**************************************************************************************************
Filename:       med.c
Revised:        $Date: 2012/02/09 20:52:56 $
Revision:       $Revision: 1.1.4.6 $

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
#include "hal_led.h"
#include "hal_assert.h"
//#include "FlashUtil.h"
#include "ZComDef.h"

#include "MacUtil.h"
#include "crc.h"
/* OS includes */
#include "OSAL.h"
#include "OSAL_NV.h"
#include "OSAL_Tasks.h"
#include "OSAL_PwrMgr.h"

/* App Protocol*/
#include "AppProtocolWrapper.h"

/* Application Includes */
#include "OnBoard.h"

/* MAC Application Interface */
#include "mac_api.h"

/* Application */
#include "med.h"

/*2530 Sleep*/
#include "Hal_sleep.h"

/* watchdog util */
#include "watchdogutil.h"
/* FLASH */
#include "Hal_flash.h"

/*******************************************************************************
*                                           Constant
*******************************************************************************/
/* Setting beacon order to 15 will disable the beacon */
#define MED_MAC_BEACON_ORDER 15
/* Setting superframe order to 15 will disable the superframe */
#define MED_MAC_SUPERFRAME_ORDER 15

#define MED_MAC_PAYLOAD_LEN          127                  //mac payload length by datasheet.
#define MED_SHORTADDR_PANCOORD       0x0000             /* Short Address of PAN Coordinator, will always be 0x0000 */

#define MED_BROADCAST_SHORTADDR_DEVZCZR 0xFFFC   /* Short Address of all coords and routers*/
#define MED_BROADCAST_SHORTADDR_DEVALL      0xFFFF   /* Short Address of all Devs*/

#define SLEEP_RST_POS       3
#define WDCTL_INT_POS       0
#define WDCTL_MODE_POS      2
#define WDCTL_EN_POS        3
#define WDCTL_CLR_POS       4

#define MED_UNICAST             0
#define MED_BROADCAST           1

//#define MED_VER               0
//#define MED_SHORTADDR         1
#define MED_EXITADDR            2

//0xFD00-0xFD53 are used to store paramters
#define MED_PARAM_ADDR          0xFD00
#define MED_DEVINFO_ADDR        0x7FF8

#define MED_LED_BLUE            0x01
#define MED_LED_RED             0x02

// reboot NV flag
#define MED_NV_REBOOT_ID        0x401
#define MED_NV_REBOOT_LEN       0x04
#define MED_REBOOT_MAGIC1       0x72
#define MED_REBOOT_MAGIC2       0x62
#define MED_REBOOT_MAGIC3       0x74
/*********************************************************************
* TYPEDEFS
*/

typedef struct
{
    uint8 u8RebootCnt;
    uint8 u8MagicNumOne;    //r
    uint8 u8MagicNumTwo;    //b
    uint8 u8MagicNumThree;  //t
} Dev_RebootInfo_st;

typedef struct
{
    sAddrExt_t ExitAddr;
} Dev_Info_t;

typedef struct
{
    uint8 u8ReqCnt;
    bool bNoRecv;
} Dev_Param_t;

/*******************************************************************************
*                                Local Variables
*******************************************************************************/

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

/* Coordinator and Device information */
static uint16        med_PanId = BLAST_NWK_ADDR;
static uint16        med_PANCoordShortAddr = MED_SHORTADDR_PANCOORD;
static uint16        med_DevShortAddr   = MED_BROADCAST_SHORTADDR_DEVALL;

/* TRUE and FALSE value */
static bool          med_MACTrue = TRUE;
static bool          med_MACFalse = FALSE;

/* Beacon order */
static uint8 med_SuperFrameOrder = MED_MAC_SUPERFRAME_ORDER;
static uint8 med_BeaconOrder = MED_MAC_BEACON_ORDER;

/* Task ID */
uint8 MED_TaskId;

/* Device Info from flash */
static Dev_Info_t med_DevInfo;
static Dev_RebootInfo_st s_stDev_RebootInfo;

static uint8 s_u8ReqCnt = 0;
static bool s_bNoRecv = TRUE;

/**************************************************************************************************
*                                     Local Function Prototypes
**************************************************************************************************/
/* Setup routines */
static void         MED_DeviceStartup(uint8 *pData);
/* Support */
static void         MED_blast(const MSGType);
static void         MED_ReadDevInfo(void);
static void         MED_Restart(void);
static void         MED_SaveParam2RAM(void);
static void         MED_ReadParmFromRAM(void);
static void         Med_IndMsgParse(sData_t *);
static void         MsgSysEventProc(void);

/*******************************************************************************
*                                   Macro function
*******************************************************************************/
#define TURN_ON_LED_BLUE()          HAL_TURN_ON_LED1()
#define TURN_OFF_LED_BLUE()         HAL_TURN_OFF_LED1()
#define TURN_ON_LED_RED()           HAL_TURN_ON_LED2()
#define TURN_OFF_LED_RED()          HAL_TURN_OFF_LED2()

#define STATE_LED_BLUE()            HAL_STATE_LED1()
#define STATE_LED_RED()             HAL_STATE_LED2()

#define BACK_AND_OFF_LED(blue,red)  \
    st(                             \
        blue = STATE_LED_BLUE();    \
        red  =  STATE_LED_RED();    \
        TURN_OFF_LED_BLUE();        \
        TURN_OFF_LED_RED();         \
    )

#define SET_LED_BLUE(blue)          \
    st(                             \
        if (blue) {                 \
            TURN_ON_LED_BLUE();     \
        }                           \
        else {                      \
            TURN_OFF_LED_BLUE();    \
        }                           \
    )

#define SET_LED_RED(red)            \
    st(                             \
        if (red) {                  \
            TURN_ON_LED_RED();      \
        }                           \
        else {                      \
            TURN_OFF_LED_RED();     \
        }                           \
    )

#define BLAST_CLOSE()                       \
do{                                         \
    BLAST_IO_CLOSE();                       \
}while(0)

#define BLAST_OPEN()                        \
do{                                         \
    BLAST_IO_OPEN();                        \
}while(0)

#define MACINITUTIL()               \
do {                                \
    MacUtil_t Macutil;              \
    Macutil.panID = 0xFFFF;         \
    Macutil.dst_endpoint = APS_DST_ENDPOINT;\
    Macutil.src_endpoint = APS_SRC_ENDPOINT;\
    Macutil.cluster_id = APS_CLUSTER_ID;\
    Macutil.profile_id = APS_PROFILE_ID;\
    Macutil.NodeType =  NODETYPE_DEVICE;\
    MAC_UTIL_INIT(&Macutil);             \
}while(0)

/*******************************************************************************
*
* @fn          MED_Init
*
* @brief       Initialize the application
*
* @param       taskId - taskId of the task after it was added in the OSAL task queue
*
* @return      none
*
********************************************************************************/
void MED_Init(uint8 taskId)
{
    /* Initialize the task id */
    MED_TaskId = taskId;

    /* initialize MAC features */
    MAC_Init();
    MAC_InitDevice();

    /* Reset the MAC */
    MAC_MlmeResetReq(TRUE);

    /* initial MacUtil*/
    MACINITUTIL();

    MED_ReadDevInfo();
    MED_DeviceStartup(NULL);
    /* save param to Ram, if watchdog reset by accident,this value is default */
    MED_SaveParam2RAM();
    TURN_OFF_LED_RED();
    TURN_OFF_LED_BLUE();

    /*Start Watch Dog*/
#ifdef WATCHDOG
    StartWatchDog(DOGTIMER_INTERVAL_1S);
    osal_set_event(MED_TaskId, MED_FEEDDOG_EVENT);
#endif
    /* goto sleep state */
    osal_set_event(MED_TaskId, MED_REQUEST_EVENT);
    MAC_MlmeSetReq(MAC_RX_ON_WHEN_IDLE, &med_MACTrue);
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
    if(events & SYS_EVENT_MSG)
    {
        MsgSysEventProc();
        return events ^ SYS_EVENT_MSG;
    }
#ifdef WATCHDOG
    if(events & MED_FEEDDOG_EVENT)
    {
        FeedWatchDog();

        static uint16 s_u16AutoRebootCnt = 0;

        s_u16AutoRebootCnt++;
        // 0.5s * 1200 = 600s = 10Min
        if (AUTOREBOOT == s_u16AutoRebootCnt)
        {
            MED_Restart();
        }

        osal_start_timerEx(MED_TaskId, MED_FEEDDOG_EVENT, FEEDDOG_SPACE);
        return events ^ MED_FEEDDOG_EVENT;
    }
#endif
    if(events & MED_REQUEST_EVENT)
    {
        if (s_u8ReqCnt < LIMITCNT)
        {
            ++s_u8ReqCnt;
            MED_blast(BLAST_REQUEST);

            if (s_bNoRecv)
                HalLedBlink(HAL_LED_2, 1, 50, 1000);
            osal_start_timerEx(MED_TaskId, MED_REQUEST_EVENT, REQ_SPACE);
        }
        else
        {
            SET_LED_RED(TRUE);
            SET_LED_BLUE(FALSE);
            BLAST_CLOSE();
        }

        return events ^ MED_REQUEST_EVENT;
    }

    return 0;
}

/*******************************************************************************
* SYS_EVENT_MSG Event processing function
*******************************************************************************/
static void MsgSysEventProc(void)
{
    uint8 *pMsg;
    macCbackEvent_t *pData;

    while((pMsg = osal_msg_receive(MED_TaskId)) != NULL)
    {
        pData = (macCbackEvent_t *) pMsg;

        switch(*pMsg)
        {
        case MAC_MCPS_DATA_CNF:
            osal_msg_deallocate((uint8 *)(pData->dataCnf.pDataReq));
            break;

        case MAC_MCPS_DATA_IND:
            /* Proess Command */
            if(MAC_UTIL_GetClusterID(pData->dataInd.msdu) == BLAST_CLUSTERID)
            {
                sData_t stAppData = MAC_UTIL_RemoveHeader(pData->dataInd.msdu);
                Med_IndMsgParse(&stAppData);
            }
            break;
        }
        /* Deallocate */
        osal_msg_deallocate((uint8 *)pData);
    }
}

/*******************************************************************************
* MAC_MCPS_DATA_IND Message processing function
*******************************************************************************/
static void Med_IndMsgParse(sData_t *pstAppData)
{
    app_BlastRequest_t *pstBlastReq = (app_BlastRequest_t *) (pstAppData->p);

    if (CRC16(((uint8 *)pstBlastReq), sizeof(app_BlastRequest_t) - 3, 0xFFFF)
            == pstBlastReq->crc && med_DevShortAddr == pstBlastReq->dstAddr)
    {
        switch (pstBlastReq->msgtype)
        {
        case BLAST_REQUEST:
            s_bNoRecv = FALSE;

            if (pstBlastReq->blastEnable)
            {
                s_u8ReqCnt = 0;
                SET_LED_RED(FALSE);
                SET_LED_BLUE(TRUE);
            }
            else
            {
                s_u8ReqCnt = LIMITCNT;
                BLAST_CLOSE();
                SET_LED_RED(TRUE);
                SET_LED_BLUE(FALSE);
            }
            break;
        }
    }
}

/*******************************************************************************
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
*******************************************************************************/
void MAC_CbackEvent(macCbackEvent_t *pData)
{
    macCbackEvent_t *pMsg = NULL;
    uint8 len = med_cbackSizeTable[pData->hdr.event];

    switch(pData->hdr.event)
    {
    case MAC_MLME_BEACON_NOTIFY_IND:

        len += sizeof(macPanDesc_t) + pData->beaconNotifyInd.sduLength
               + MAC_PEND_FIELDS_LEN(pData->beaconNotifyInd.pendAddrSpec);
        if((pMsg = (macCbackEvent_t *) osal_msg_allocate(len)) != 0)
        {
            /* Copy data over and pass them up */
            osal_memcpy(pMsg, pData, sizeof(macMlmeBeaconNotifyInd_t));
            pMsg->beaconNotifyInd.pPanDesc
                = (macPanDesc_t *)((uint8 *) pMsg
                                   + sizeof(macMlmeBeaconNotifyInd_t));
            osal_memcpy(pMsg->beaconNotifyInd.pPanDesc,
                        pData->beaconNotifyInd.pPanDesc,
                        sizeof(macPanDesc_t));
            pMsg->beaconNotifyInd.pSdu
                = (uint8 *)(pMsg->beaconNotifyInd.pPanDesc + 1);
            osal_memcpy(pMsg->beaconNotifyInd.pSdu,
                        pData->beaconNotifyInd.pSdu,
                        pData->beaconNotifyInd.sduLength);
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
        osal_msg_send(MED_TaskId, (uint8 *) pMsg);
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
void MED_blast(const MSGType reqtype)
{
    app_BlastRequest_t stAppdata;
    uint8 *pu8BlastReq = (uint8 *)(&stAppdata);
    uint16 u16DataLen = sizeof(app_BlastRequest_t);

    stAppdata.msgtype = reqtype;
    stAppdata.blastCnt = s_stDev_RebootInfo.u8RebootCnt;
    stAppdata.srcAddr = med_DevShortAddr;
    stAppdata.crc = CRC16(pu8BlastReq, u16DataLen - 3, 0xFFFF);

    MAC_MlmeSetReq(MAC_RX_ON_WHEN_IDLE, &med_MACTrue);
    MAC_UTIL_BuildandSendData(pu8BlastReq, u16DataLen, MED_UNICAST, 0, 0);
}

void MED_ReadDevInfo()
{
    //Dev_Info_t *p = (Dev_Info_t *)(MED_DEVINFO_ADDR);
    //med_DevInfo = *p;

    if(GetResetFlag() == RESET_FLAG_WATCHDOG)
    {
        MED_ReadParmFromRAM();
    }

    if(ZSuccess != osal_nv_read(ZCD_NV_EXTADDR, 0, Z_EXTADDR_LEN, &med_DevInfo))
    {
        SystemReset();
    }

    /*For Card, the lowest Byte of Exit Addr should be 0x01 */
    HAL_ASSERT(med_DevInfo.ExitAddr[EXT_MACADDR_TYPE] == EXT_MACADDR_TYPE_BLAST);
    //med_version = med_DevInfo.ExitAddr[EXT_MACADDR_VERSION];
    med_DevShortAddr = BUILD_UINT16(med_DevInfo.ExitAddr[EXT_MACADDR_DEVID_LBYTE],
                                    med_DevInfo.ExitAddr[EXT_MACADDR_DEVID_HBYTE]);

    while (ZSuccess != osal_nv_item_init(MED_NV_REBOOT_ID, MED_NV_REBOOT_LEN, NULL));

    if (ZSuccess != osal_nv_read(MED_NV_REBOOT_ID,
                                 0,
                                 MED_NV_REBOOT_LEN,
                                 &s_stDev_RebootInfo))
    {
        SystemReset();
    }

    if (MED_REBOOT_MAGIC1 == s_stDev_RebootInfo.u8MagicNumOne
        && MED_REBOOT_MAGIC2 == s_stDev_RebootInfo.u8MagicNumTwo
        && MED_REBOOT_MAGIC3 == s_stDev_RebootInfo.u8MagicNumThree)
    {
        s_stDev_RebootInfo.u8RebootCnt++;
    }
    else
    {
        s_stDev_RebootInfo.u8RebootCnt = 0;
        s_stDev_RebootInfo.u8MagicNumOne = MED_REBOOT_MAGIC1;
        s_stDev_RebootInfo.u8MagicNumTwo = MED_REBOOT_MAGIC2;
        s_stDev_RebootInfo.u8MagicNumThree = MED_REBOOT_MAGIC3;
    }

    if (ZSuccess != osal_nv_write(MED_NV_REBOOT_ID,
                                  0,
                                  MED_NV_REBOOT_LEN,
                                  &s_stDev_RebootInfo))
    {
        SystemReset();
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
    Dev_Param_t stParam;

    stParam.u8ReqCnt = s_u8ReqCnt;
    stParam.bNoRecv = s_bNoRecv;
    *((Dev_Param_t *)(MED_PARAM_ADDR)) = stParam; // save parameters to idata ram
}

void MED_ReadParmFromRAM(void)
{
    Dev_Param_t stParam = *((Dev_Param_t *)(MED_PARAM_ADDR));
    s_u8ReqCnt = stParam.u8ReqCnt;
    s_bNoRecv = stParam.bNoRecv;
}

/**************************************************************************************************
**************************************************************************************************/

