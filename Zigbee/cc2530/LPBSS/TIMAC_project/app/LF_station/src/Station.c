/**************************************************************************************************
Filename:       CR.c
Revised:        $Date: 2011/08/01 20:41:20 $
Revision:       $Revision: 1.17 $

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

/* OS includes */
#include "OSAL.h"
#include "OSAL_Tasks.h"
//#include "OSAL_Nv.h"

/* App Protocol*/
#include "App_cfg.h"

/* Application Includes */
#include "OnBoard.h"

/* MAC Application Interface */
#include "mac_api.h"
#include "mac_main.h"
#include "mac_pib.h"
#include "ZComDef.h"
/* Application */
#include "Station.h"

/* watchdog util */
#include "watchdogutil.h"
#include "MacUtil.h"
#include "crc.h"

/* FLASH */
#include "Hal_flash.h"

#include "Lpbssnwk.h"
#include "Hal_sleep.h"
#include "Version.h"
/**************************************************************************************************
*                                           Constant
**************************************************************************************************/
//UART
#define UART_BAUDRATE 115200 //bit per second
#define UART_FRAME_TIMEOUT 15 // millisecond


#if (UART_BAUDRATE == 115200)
#define STATION_APP_BAUD  HAL_UART_BR_115200
#endif

#if (UART_BAUDRATE == 9600)
#define STATION_APP_BAUD  HAL_UART_BR_9600
#endif

#if !defined( STATION_APP_RX_MAX )
#define STATION_APP_RX_MAX  150
#endif

// When the Rx buf space is less than this threshold, invoke the Rx callback.
#if !defined( STATION_APP_THRESH )
#define STATION_APP_THRESH  130
#endif



#if !defined( STATION_APP_TX_MAX )
#define STATION_APP_TX_MAX  250
#endif

// Millisecs of idle time after a byte is received before invoking Rx callback.
#if !defined( CR_APP_IDLE )
#define CR_APP_IDLE  8
#endif


/**************************************************************************************************
*                                           Macros
**************************************************************************************************/
#define STATION_RETREAT_NORMAL                 0

#define STATION_LED_BLUE     HAL_LED_1

#define LF_SEND_CONSEL      P0SEL
#define LF_SEND_CONDIR      P0DIR
#define LF_SEND_CONPIN      P0_2
#define LF_SEND_CONBIT      BV(2)

#define  LF_DATA_MAX_LEN    (LF_DATA_LEN*5)
#define  WIRE_DATA_MAX_LEN   (sizeof(app_mDev_hdr_t)+sizeof(app_mDev_fdr_t)+ MAC_MAX_FRAME_SIZE)
#define  DATA_START_BYTE_NUM    1

#define  LINKED         BV(0)  // L:unlink  H:linked
#define  LF_0R_1W       BV(1)  // l:read    H:write


/*********************************************************************
* TYPEDEFS
*/

typedef enum
{
    E_STATION_INIT          = 0,
    E_STATION_STARTED
} eStationState;

typedef enum
{
    CR_REPORT_DATA,
    CR_RESPOND,
    CR_EXCEPTION,
} eUartSendType;

typedef enum
{
    CRC_ERROR = 0,
    LEN_SHORT,
    NON_MINE,
    GOOD_WORK,
} eUartRecReturn;


typedef struct
{
    /* device information */
    uint16 PanId;
    uint16 ShortAddr;
    uint8 Channel;
    sAddrExt_t extAddr;

    /* status */
    eStationState State;

    /* retreat */
    uint8 retreat_flag;
    uint8 retreat_cnt;
} Station_DevInfo_t;


typedef struct
{
    uint8 ResetFlag;
} storeParam_t;

typedef struct
{
    bool    sendingFlag;
    bool    needSentFlag;
    uint16  len;
    uint8   LfData[LF_DATA_MAX_LEN+1];
} LFDataBuff_t;


/* Size table for MAC structures */
const CODE uint_8 station_cbackSizeTable[] =
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

/***********************************************************
 *                  Global Variables
 */
bool sentFlag = false;

/***********************************************************
 *                  Local Variables
*/
/* TRUE and FALSE value */
static bool Station_MACTrue = TRUE;
static bool Station_MACFalse = FALSE;

/* Task ID */
uint8 Station_TaskId;

/* Device Info from flash */
static Station_DevInfo_t Station_DevInfo;

static uint16 dstID = 0xffff;
static uint16 saveDevID;
static uint8  mDevStatus = 0;
static uint8  eDevType = 0;
static uint8  linkCount = 0;
static uint8  WireSendBuff[WIRE_DATA_MAX_LEN+10];
static uint8  sLFBuf[LF_DATA_MAX_LEN+DATA_START_BYTE_NUM+1];
/* buf send on LF  */
static LFDataBuff_t  LFDataBuff[2];

/* UART*/
static uint8 uartReadBufLen1 = WIRE_DATA_MAX_LEN;
static uint8 uartReadBuf1[WIRE_DATA_MAX_LEN+10];
static uint8 uartReadLen1;

/**************************************************************************************************
*                                     Local Function Prototypes
**************************************************************************************************/

static void  init_uart(uint8  uartPort , uint8 baudRate);
static void  Station_Startup(void);
static void  Station_ReadDevInfo(void);
static eUartRecReturn  Station_ParseUartCmd(uint_8* data, uint_16 len);
static void  Station_start_timerEx(uint8 taskID,
                                         uint16 event_id,
                                         uint16 timeout_value);
static void  Station_ParseDataCnf(const macMcpsDataCnf_t* pdatacnf);
static void  Station_ParseFrame(const macMcpsDataInd_t* pdataInd);
static uint8 Station_ParseCardFrame(const sAddr_t* sAddr,
                                             uint16 srcPanId,
                                             const sData_t msdu,
                                             int8 rssi);
static void  Station_send_by_LF(uint8 *pdata,uint16 datalen);
static void  Data_to_1010000_code(uint8 *p01011Code,
                                          uint8 *pdata,uint16 datalen);
static void  Station_Send_Data_by_Wire(uint8* pData, uint16 dataLen);
static void  Start_Timer4(void);
static void  Init_LF_Exciter(void);
static void  Send_Data_by_Wireless(uint8* pdata ,
                                            uint8 devType,
                                            uint16 len,
                                            uint8 DeliverMode,
                                            uint16 uniDstAddr,
                                            uint8 txOption);
static void  Start_buzzer(void);
static void  Stop_buzzer(void);

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
    StartWatchDog(DOGTIMER_INTERVAL_1S);
    osal_set_event(Station_TaskId, STATION_FEEDWATCHDOG_EVENT);
#endif

    /*init devinfo */
    Station_ReadDevInfo();

    /* turn off leds */
    //HalLedSet(STATION_LED_BLUE, HAL_LED_MODE_OFF);
    init_uart(HAL_UART_PORT_1,STATION_APP_BAUD);

    /* initial macutil */
    MacUtil_t MacUtil;
    MacUtil.panID = 0xfff7;  /* default is card nwk id */
    MacUtil.dst_endpoint = MINEAPP_ENDPOINT;
    MacUtil.src_endpoint = 0x20;//MINEAPP_ENDPOINT;
    MacUtil.profile_id = 0x0100;//MINEAPP_PROFID;
    MacUtil.cluster_id = CARD_CLUSTERID;
    MacUtil.NodeType = NODETYPE_COORDINATOR;

    MAC_UTIL_INIT(&MacUtil);

    LF_SEND_CONSEL &=~ LF_SEND_CONBIT;          //  P0.2 to general io
    LF_SEND_CONDIR |= LF_SEND_CONBIT;           //  P0.2 to output
    LF_SEND_CONPIN = 0;

    /* start up*/
    Station_start_timerEx(Station_TaskId, STATION_UART_LINK_EVENT, 10);
    Station_start_timerEx(Station_TaskId, STATION_UART_READ1_EVENT,20);
}

/**************************************************************************************************
*
* @fn          init_uart
*
* @brief       init uart
*
* @param       UART_PORT - number of  uart port
*              baudRate -
*
* @return      void
*
**************************************************************************************************/
 void init_uart(uint8  uartPort , uint8 baudRate)
 {
            halUARTCfg_t uartConfig;

            uartConfig.configured           = TRUE;
            uartConfig.baudRate             = baudRate;
            uartConfig.flowControl          = FALSE;
            uartConfig.flowControlThreshold = STATION_APP_THRESH;
            uartConfig.rx.maxBufSize        = STATION_APP_RX_MAX;
            uartConfig.tx.maxBufSize        = STATION_APP_TX_MAX;
            uartConfig.idleTimeout          = CR_APP_IDLE;
            uartConfig.intEnable            = TRUE;
            uartConfig.callBackFunc         = NULL;

            /*two ports. one for backup*/
            HalUARTOpen(uartPort, &uartConfig);

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
    if (events & STATION_FEEDWATCHDOG_EVENT)
    {
        Station_start_timerEx(Station_TaskId, STATION_FEEDWATCHDOG_EVENT, 300);
        FeedWatchDog();
        return events ^ STATION_FEEDWATCHDOG_EVENT;
    }
#endif

    if (events & SYS_EVENT_MSG)
    {
        while ((pMsg = osal_msg_receive(Station_TaskId)) != NULL)
        {
            pData = (macCbackEvent_t *) pMsg;

            switch (*pMsg)
            {
                case MAC_MLME_START_CNF:
                {
                    if (pData->startCnf.hdr.status == MAC_SUCCESS)
                    {
                        /* power on */
                        Station_DevInfo.State = E_STATION_STARTED;
                        //HalLedSet(STATION_LED_BLUE, HAL_LED_MODE_ON);
                    }
                    break;
                }

                case MAC_MCPS_DATA_CNF:
                {
                    /* sometimes received  a "fake" MAC_MCPS_DATA_CNF with status of MAC_INVALID_PARAMETER */
                    /* I do not know where it comes from, but it will cause systemreset when call mac_msg_deallocate() */
                    if (pData->dataCnf.hdr.status != MAC_INVALID_PARAMETER)
                    {
                        Station_ParseDataCnf(&pData->dataCnf);
                        mac_msg_deallocate((uint8 * *) &pData->dataCnf.pDataReq);
                    }
                    break;
                }

                case MAC_MCPS_DATA_IND:
                {
                    /* do not check the indication status */
                    //if(pData->dataInd.hdr.status == ZSUCCESS)
                    {
                        Station_ParseFrame(&pData->dataInd);
                    }
                    break;
                }

           }

            /* Deallocate */
            mac_msg_deallocate((uint8 * *) &pMsg);
        }

        return events ^ SYS_EVENT_MSG;
    }

    if(events & STATION_STOP_BUZZER_EVENT)
    {
        Stop_buzzer();
        return events ^ STATION_STOP_BUZZER_EVENT;
    }

    if(events & STATION_WRITE_TIMEOUT_EVENT)
    {
        if(mDevStatus&LF_0R_1W)
        {
            mDevStatus &=~ LF_0R_1W;
        }
        return events ^ STATION_WRITE_TIMEOUT_EVENT;
    }

    if(events & STATION_RESET_SAVEDEVID_EVENT)
    {
         saveDevID = 0;
         return events ^ STATION_RESET_SAVEDEVID_EVENT;
    }

    if (events & STATION_REPORT_VERSION_EVENT)
    {
        if(mDevStatus&LINKED)
        {
            uint16 len = 0;
            app_mDev_hdr_t mDevHdr;
            app_eDev_hdr_t eDevHdr;
            //version+' '+release
            app_mDev_fdr_t mDevFdr;

            osal_memset(WireSendBuff, 0,WIRE_DATA_MAX_LEN+10);

            osal_memcpy(WireSendBuff, WIRE_SYNC,sizeof(mDevHdr.sync));
            len += sizeof(mDevHdr.sync);

            eDevHdr.MSGType = REPORT_VERSION;
            eDevHdr.eDevID = 0;
            eDevHdr.eDevType = 0;
            eDevHdr.mDevID = Station_DevInfo.ShortAddr;
            eDevHdr.dataLen = sizeof(VERSION)-1+1+sizeof(RELEASE)-1;
            osal_memcpy(WireSendBuff+len, &eDevHdr,sizeof(app_eDev_hdr_t));
            len += sizeof(app_eDev_hdr_t);


            osal_memcpy(WireSendBuff+len, VERSION,sizeof(VERSION)-1);
            len += sizeof(VERSION)-1;

            osal_memcpy(WireSendBuff+len, " ",sizeof(" ")-1);
            len += sizeof(" ")-1;

            osal_memcpy(WireSendBuff+len, RELEASE,sizeof(RELEASE)-1);
            len += sizeof(RELEASE)-1;

            mDevFdr.padding = 0xffff;
            mDevFdr.crc = CRC16(WireSendBuff,len,0xFFFF);
            osal_memcpy(WireSendBuff+len, (uint8 *)&mDevFdr, sizeof(app_mDev_fdr_t));
            len += sizeof(app_mDev_fdr_t);
            Station_Send_Data_by_Wire(WireSendBuff,len);
        }
        Station_start_timerEx(Station_TaskId, STATION_REPORT_VERSION_EVENT, 60000);
        return events ^ STATION_REPORT_VERSION_EVENT;
    }

    if (events & STATION_START_EVENT)
    {
        Station_Startup();
        Station_start_timerEx(Station_TaskId, STATION_LF_INSPIRE_EVENT, 1);
        return events ^ STATION_START_EVENT;
    }

    if(events & STATION_LF_INSPIRE_EVENT)
    {
        uint8 LFCmd = 0x00;

        if(mDevStatus&LINKED)
        {
            if(mDevStatus&LF_0R_1W)
            {
                LFCmd |= LF_CMD_WRITE;
            }
            else
            {
                LFCmd |= LF_CMD_READ;
            }

            LFCmd |= (0xf0&(eDevType<<4));

            uint16 sShortAddr = Station_DevInfo.ShortAddr;
            uint8 LFBuf[LF_DATA_LEN]={LFCmd,(uint8)((sShortAddr&0xff00)>>8),(uint8)(sShortAddr&0xff),(uint8)((dstID&0xff00)>>8),(uint8)(dstID&0xff)};
            osal_memset(&sLFBuf,0,sizeof(sLFBuf));
            Data_to_1010000_code(sLFBuf,LFBuf,sizeof(LFBuf));
            Station_send_by_LF(sLFBuf+1,sLFBuf[0]);
            sentFlag = TRUE;
        }
        osal_start_timerEx(taskId, STATION_LF_INSPIRE_EVENT, 300);
        return events ^ STATION_LF_INSPIRE_EVENT;
    }

    if(events & STATION_UART_LINK_EVENT)
    {

        uint16 len = 0;
        app_mDev_hdr_t mDevHdr;
        app_eDev_hdr_t eDevHdr;
        app_mDev_fdr_t mDevFdr;

        linkCount++;
        if(linkCount > 35)
        {
            SystemReset();
        }
        else if(linkCount > 3)
        {
            LF_SEND_CONPIN = 0;
            mDevStatus &=~ LINKED;
            HalLedSet(STATION_LED_BLUE, HAL_LED_MODE_TOGGLE);
        }

        osal_memcpy(WireSendBuff, WIRE_SYNC,sizeof(mDevHdr.sync));
        len += sizeof(mDevHdr.sync);

        eDevHdr.MSGType = DETECT;
        eDevHdr.eDevID = 0;
        eDevHdr.eDevType = 0;
        eDevHdr.mDevID = Station_DevInfo.ShortAddr;
        eDevHdr.dataLen = 0;
        osal_memcpy(WireSendBuff+len, &eDevHdr,sizeof(app_eDev_hdr_t));
        len += sizeof(app_eDev_hdr_t);
        mDevFdr.padding = 0xffff;
        mDevFdr.crc = CRC16(WireSendBuff,len,0xFFFF);
        osal_memcpy(WireSendBuff+len, (uint8 *)&mDevFdr, sizeof(app_mDev_fdr_t));
        len += sizeof(app_mDev_fdr_t);
        Station_Send_Data_by_Wire(WireSendBuff,len);

        if(mDevStatus&LINKED)
        {
            osal_start_timerEx(taskId, STATION_UART_LINK_EVENT, 45000);
            if(!Station_DevInfo.State)
            {
                Init_LF_Exciter();
                Station_start_timerEx(Station_TaskId, STATION_START_EVENT, 10);
                Station_start_timerEx(Station_TaskId, STATION_REPORT_VERSION_EVENT, 1000);
            }
        }
        else
        {
            osal_start_timerEx(taskId, STATION_UART_LINK_EVENT, 1000);
        }

        return events ^ STATION_UART_LINK_EVENT;
    }

    if(events & STATION_UART_READ1_EVENT)
    {
        uint_8 len;
        static bool fIgnore = FALSE;
        eUartRecReturn ret = GOOD_WORK;

        len = HalUARTRead(HAL_UART_PORT_1, uartReadBuf1 + uartReadLen1, uartReadBufLen1 - uartReadLen1);

        uartReadLen1 += len;


        if(uartReadBuf1[0] != 'Y' )
        {
            fIgnore = TRUE;
        }

        //continue ignoring till len == 0
        if(fIgnore)
        {
            uartReadLen1 = 0;
        }

        if(len == 0)
        {
            fIgnore = FALSE;
        }

        if(uartReadLen1 <= WIRE_DATA_MAX_LEN
            &&uartReadLen1 >= (sizeof(app_mDev_fdr_t)+sizeof(app_mDev_hdr_t)+sizeof(app_eDev_hdr_t))
            &&len == 0)
        {
            ret = Station_ParseUartCmd(uartReadBuf1, uartReadLen1);
            if(ret != LEN_SHORT)
            {
                uartReadLen1 = 0;
            }
        }
        else if(uartReadLen1 > 0 && len == 0)
        {
            uartReadLen1 = 0;
        }

        osal_start_timerEx(taskId, STATION_UART_READ1_EVENT, UART_FRAME_TIMEOUT);
        return events ^ STATION_UART_READ1_EVENT;
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
void MAC_CbackEvent(macCbackEvent_t* pData)
{
    macCbackEvent_t* pMsg = NULL;

    uint_8 len = station_cbackSizeTable[pData->hdr.event];

    switch (pData->hdr.event)
    {
    case MAC_MLME_BEACON_NOTIFY_IND:
        len += sizeof(macPanDesc_t)
               + pData->beaconNotifyInd.sduLength
               + MAC_PEND_FIELDS_LEN(pData->beaconNotifyInd.pendAddrSpec);
        if ((pMsg = (macCbackEvent_t *) osal_msg_allocate(len)) != NULL)
        {
            /* Copy data over and pass them up */
            osal_memcpy(pMsg, pData, sizeof(macMlmeBeaconNotifyInd_t));
            pMsg->beaconNotifyInd.pPanDesc = (macPanDesc_t *)
                                             ((uint_8 *) pMsg
                                              + sizeof(macMlmeBeaconNotifyInd_t));
            osal_memcpy(pMsg->beaconNotifyInd.pPanDesc,
                        pData->beaconNotifyInd.pPanDesc,
                        sizeof(macPanDesc_t));
            pMsg->beaconNotifyInd.pSdu = (uint_8 *)
                                         (pMsg->beaconNotifyInd.pPanDesc + 1);
            osal_memcpy(pMsg->beaconNotifyInd.pSdu,
                        pData->beaconNotifyInd.pSdu,
                        pData->beaconNotifyInd.sduLength);
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
        if (SUCCESS != osal_msg_send(Station_TaskId, (uint8 *) pMsg))
        {
            SystemReset();
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
    macMlmeStartReq_t startReq;

    if (Station_DevInfo.State == E_STATION_STARTED)
        return;

    /* Setup MAC_EXTENDED_ADDRESS */
    MAC_MlmeSetReq(MAC_EXTENDED_ADDRESS, &Station_DevInfo.extAddr);

    /* Setup MAC_SHORT_ADDRESS */
    //Station_DevInfo.ShortAddr = 0;
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
    MAC_MlmeSetReq(MAC_MAX_FRAME_RETRIES, &maxFrameRetries);

    uint8 maxCsmaBackoff = 5;
    MAC_MlmeSetReq(MAC_MAX_CSMA_BACKOFFS, &maxCsmaBackoff);

    uint8 minBe = 4;
    MAC_MlmeSetReq(MAC_MIN_BE, &minBe);

    uint8 maxBe = 6;
    MAC_MlmeSetReq(MAC_MAX_BE, &maxBe);

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

void Station_ReadDevInfo(void)
{

    uint16 number, number0, number1, number2, number3, number4 ;

    HalFlashRead(HAL_FLASH_IEEE_PAGE, HAL_FLASH_IEEE_OSET,
                 (uint8 *)&Station_DevInfo.extAddr, HAL_FLASH_IEEE_SIZE);

    /* DeviceID */
    number0=Station_DevInfo.extAddr[LPBSS_MAC_DEVID_L8BIT]&0x0F;
    number1=(Station_DevInfo.extAddr[LPBSS_MAC_DEVID_L8BIT]>>4)&0x0F;
    number2=Station_DevInfo.extAddr[LPBSS_MAC_DEVID_M8BIT]&0x0F;
    number3=(Station_DevInfo.extAddr[LPBSS_MAC_DEVID_M8BIT]>>4)&0x0F;
    number4=Station_DevInfo.extAddr[LPBSS_MAC_DEVID_H4BIT]&0x0F;

    number = number0 + 10*number1 + 100*number2 + 1000*number3 + 10000*number4 ;

    if((number0>9)||(number1>9)||(number2>9)||(number3>9)||(number4>6)
            ||!LPBSS_IS_DEVID(number))
    {
        HAL_ASSERT(false);
    }

    Station_DevInfo.PanId = POS_STATION_PANID;;


    /* Channel */
    uint8 channel = Station_DevInfo.extAddr[LPBSS_MAC_CHA];

    if(!(channel>=11 && channel <=26))
    {
        channel = 0x0B;   //use default channel 11;
    }

    Station_DevInfo.Channel =  channel;

    /*For Card, the EXT_LPBSS_MACADDR_TYPE Byte of Exit Addr should be 0x11 */
    //HAL_ASSERT(Station_DevInfo.extAddr[LPBSS_MAC_MODEL] == EXT_LPBSS_MAC_TYPE_CARDREADER);

    Station_DevInfo.ShortAddr = number;
    Station_DevInfo.State = E_STATION_INIT;
    Station_DevInfo.retreat_flag = STATION_RETREAT_NORMAL;
    Station_DevInfo.retreat_cnt = 0;
}

static void Station_ParseDataCnf(const macMcpsDataCnf_t* pdatacnf)
{
    static uint8 fail_cnt = 0;

    if (MAC_SUCCESS == pdatacnf->hdr.status
        || MAC_NO_ACK == pdatacnf->hdr.status)
    {
        fail_cnt = 0;
    }
    else
    {
        /* if send data failed for 100 times, find reason and restart */
        if (fail_cnt++ > 100)
        {
            /* set restart reason and restart */
            SystemReset();
        }
    }
}

static void Station_ParseFrame(const macMcpsDataInd_t* pdataInd)
{
    if ((pdataInd == NULL)
            || (pdataInd->msdu.p == NULL)
            || (pdataInd->msdu.len > MAC_MAX_FRAME_SIZE))
    {
        return;
    }

    app_eDev_hdr_t* eDevWls = (app_eDev_hdr_t *)pdataInd->msdu.p;

    if(( eDevWls->mDevID == Station_DevInfo.ShortAddr)
        &&(eDevWls->dataLen <= MAC_MAX_FRAME_SIZE)
        &&(pdataInd->msdu.p != NULL)
        &&(pdataInd->msdu.len <= MAC_MAX_FRAME_SIZE)
            )
    {
            Station_ParseCardFrame(&pdataInd->mac.srcAddr,
                   pdataInd->mac.srcPanId,
                   pdataInd->msdu,
                   pdataInd->mac.rssi);
    }

}

static uint8 Station_ParseCardFrame(const sAddr_t* sAddr,
                                    uint16 srcPanId,
                                    const sData_t msdu,
                                    int8 rssi)
{
    uint8 ret;

    if((sAddr->addr.shortAddr == 0)//||(rssi <-50)
        )
    {
            return 0;
    }

    app_eDev_hdr_t eDevHdr;
    app_mDev_fdr_t mDevFdr;
    uint16 curDevID = 0;
    eDevHdr = *((app_eDev_hdr_t*)msdu.p);
    curDevID = eDevHdr.eDevID;

    if(eDevHdr.MSGType < READ_DATA)
    {
        return 0;
    }

    if(((curDevID != saveDevID)&&(eDevHdr.MSGType == READ_DATA))
    //||(eDevHdr.MSGType == READY)
    )
    {
        Start_buzzer();
        saveDevID = curDevID;
        Station_start_timerEx(Station_TaskId, STATION_STOP_BUZZER_EVENT, 150);
        Station_start_timerEx(Station_TaskId, STATION_RESET_SAVEDEVID_EVENT, 2800);
    }

    if(eDevHdr.MSGType == READY)
    {
        mDevStatus &=~ LF_0R_1W;
    }
    osal_memcpy(WireSendBuff,WIRE_SYNC,sizeof(app_mDev_hdr_t));
    osal_memcpy(WireSendBuff+sizeof(app_mDev_hdr_t),(uint8 *)msdu.p,msdu.len);

    mDevFdr.padding = 0xffff;
    mDevFdr.crc = CRC16(WireSendBuff,(sizeof(app_mDev_hdr_t)+msdu.len),0xFFFF);
    osal_memcpy(WireSendBuff+sizeof(app_mDev_hdr_t)+msdu.len,(uint8 *)&mDevFdr,sizeof(app_mDev_fdr_t));
    ret= HalUARTWrite(HAL_UART_PORT_1,(uint_8*)(&WireSendBuff),sizeof(app_mDev_hdr_t)+msdu.len+sizeof(app_mDev_fdr_t));
    //Stop_buzzer();

    return ret;
}

static void Station_start_timerEx(uint8 taskID,
                                  uint16 event_id,
                                  uint16 timeout_value)
{
    if (ZSuccess != osal_start_timerEx(taskID, event_id, timeout_value))
    {
        SystemReset();
    }
}

/**************************************************************************************************
*
* @fn      Station_ParseUartCmd
*
* @brief   parse the uart coming data
*
**************************************************************************************************/
static eUartRecReturn Station_ParseUartCmd(uint_8* pdata, uint_16 len)
{
    /*CRC doesn't include the sync unit*/
    uint_16 crc =  CRC16(pdata, len - sizeof(app_mDev_fdr_t), 0xFFFF);
    uint8 cmdType;
    uint_16 dataLen =0;

    app_eDev_hdr_t* pDevHdr = (app_eDev_hdr_t*)(pdata+sizeof(app_mDev_hdr_t));

    if(pDevHdr->mDevID != Station_DevInfo.ShortAddr) return NON_MINE;

    cmdType  = pDevHdr->MSGType;
    eDevType = pDevHdr->eDevType;
    dstID    = pDevHdr->eDevID;
    dataLen  = pDevHdr->dataLen;

    if((dataLen + sizeof(app_mDev_hdr_t)+ sizeof(app_eDev_hdr_t)+ sizeof(app_mDev_fdr_t)) > len
         &&dataLen < MAC_MAX_FRAME_SIZE -sizeof(app_eDev_hdr_t))
    {
        return LEN_SHORT;
    }

    if(crc != *((uint_16*)(pdata + len - 2)))
    {
        return CRC_ERROR;
    }

    switch(cmdType)
    {
        case (DETECT_ACK):
        {
            HalLedSet(STATION_LED_BLUE, HAL_LED_MODE_ON);
            LF_SEND_CONPIN = 1;
            linkCount = 0;
            if(!(mDevStatus&LINKED))
            {
                mDevStatus |= LINKED;
            }
            break;
        }
        case(WRITE_CARD):
        {
            if(!(mDevStatus&LF_0R_1W))
            {
                mDevStatus |= LF_0R_1W;
                Station_start_timerEx(Station_TaskId, STATION_WRITE_TIMEOUT_EVENT, 5000);
            }
            break;
        }
        case(WRITE_DATA):
        {
            //don't need break;
        }
        case(READ_DATA_ACK):
        {
            uint16 eDevID = ((app_eDev_hdr_t*)(pdata+sizeof(app_mDev_hdr_t)))->eDevID;
            uint8  eDevType = ((app_eDev_hdr_t*)(pdata+sizeof(app_mDev_hdr_t)))->eDevType;
            Send_Data_by_Wireless((uint8 *)(pdata+sizeof(app_mDev_hdr_t)),
                                   eDevType,
                                  (len - sizeof(app_mDev_hdr_t)- sizeof(app_mDev_fdr_t)),
                                   MAC_UTIL_UNICAST,
                                   eDevID,
                                   0);
            break;
        }

        default:
            break;
    }
    return GOOD_WORK;
}

/**************************************************************************************************
*
* @fn      void     Station_send_mbusHdr();
*
* @brief   sent   mbusHdr
*
* @return  crc
**************************************************************************************************/

void Station_Send_Data_by_Wire(uint8*  pData, uint16 dataLen)
{
    HalUARTWrite(HAL_UART_PORT_1, pData, dataLen);
}


/**************************************************************************************************
*
* @fn      void     Station_send_by_LF();
*
* @brief
*
* @return  void
**************************************************************************************************/

static void   Station_send_by_LF(uint8 *pdata,uint16 datalen)
{
    halIntState_t his;
    HAL_ENTER_CRITICAL_SECTION(his);


    if((pdata == NULL)||(datalen == 0)||(datalen > LF_DATA_MAX_LEN))
    {
        return;
    }
    for(uint8 i=0; i<2; i++)
    {
        if(LFDataBuff[i].sendingFlag)
        {
            continue;
        }

        LFDataBuff[i].len = datalen;
        osal_memcpy(&LFDataBuff[i].LfData,pdata,datalen);
        LFDataBuff[i].needSentFlag = true;
        break;
    }
    HAL_EXIT_CRITICAL_SECTION(his);

}

/**************************************************************************************************
*
* @fn   bool Get_Bool_for_LF(void)
*
* @brief
*
* @return  bool
**************************************************************************************************/

uint8 Get_8bits_for_LF(void)
{
    static uint8 saveLen = 0;
    static uint8 buffIdx =0;
    static uint8 idx =2;
    uint8 data =0 ;
    if(saveLen == 0)
    {
        for(uint8 i=0; i<2; i++)
        {
            if(LFDataBuff[i].needSentFlag && LFDataBuff[i].len)
            {
                LFDataBuff[i].sendingFlag = true;
                idx = i;
                saveLen = LFDataBuff[i].len;
                buffIdx = 0;
                break;
            }
        }
    }

    if(idx <2)
    {
         if(saveLen)saveLen -= 1;
         data = LFDataBuff[idx].LfData[buffIdx++];
         if(saveLen ==0)
         {
             LFDataBuff[idx].needSentFlag = false;
             LFDataBuff[idx].sendingFlag = false;
             idx =2;
         }

    }
    return data;
}

/*
 *  data--> 1010000   1==>10000   0==>10
 */
void Data_to_1010000_code(uint8 *p1010000Code,uint8 *pdata,uint16 datalen)
{
    uint8 sData[LF_DATA_LEN+1];
    uint16 bitCount = 0;

    if((pdata == NULL)||(p1010000Code == NULL)||(datalen == 0)||(datalen > LF_DATA_MAX_LEN))
    {
        return;
    }
    osal_memcpy(&sData,pdata,datalen);

    for(uint8 idx =0; idx < datalen; idx++)
    {
        uint8 i=0;
        for(i=0; i<8; i++)
        {
            p1010000Code[(bitCount/8)+DATA_START_BYTE_NUM]|=BV(7-(bitCount%8));
            bitCount++;
            p1010000Code[(bitCount/8)+DATA_START_BYTE_NUM] &=~ BV(7-(bitCount%8));
            bitCount++;
            if(sData[idx]&(0x80>>i))
            {
                p1010000Code[(bitCount/8)+DATA_START_BYTE_NUM]&=~ BV(7-(bitCount%8));
                bitCount++;
                p1010000Code[(bitCount/8)+DATA_START_BYTE_NUM]&=~ BV(7-(bitCount%8));
                bitCount++;
                p1010000Code[(bitCount/8)+DATA_START_BYTE_NUM]&=~ BV(7-(bitCount%8));
                bitCount++;
            }
        }
    }
    p1010000Code[(bitCount/8)+DATA_START_BYTE_NUM]|=BV(7-(bitCount%8));
    p1010000Code[0]= (bitCount+1)/8+(((bitCount+1)%8)?1:0);

}

/*
 *  Get Len of LF data
 */
uint16 Get_LFdata_Len(void)
{
    return sLFBuf[0];
}

/*
 *  Start  Timer4
 */
void Start_Timer4(void)
{
    // TIMER4 channel 1
    P2DIR |= (0x01 << 0);  //set to output
    PERCFG |= (0x01 << 4); // set timer4 location
    P2_0 = 0;

    P2SEL |= (0x01 << 0);  // set to peripheral

    T4CCTL0 = (0x00 << 6) | (0x02 << 3) | (0x01 << 2) ;
    T4CC0 = 0x7F;
    T4CTL = (0x00 << 5) | (0x00 << 3) | (0x02 << 0);
    T4CTL &= ~(0x01 << 2); //clear
    T4CTL |= (0x01 << 4);  //start
}



/*
 *  Ready for Send  of  LF Data      timer4 use to make 125k
 */
void Init_LF_Exciter(void)
{
    //timer4
    Start_Timer4();

    // P0_3
    P0DIR |= (0x01 << 3);
    P0SEL &= ~(0x01 << 3);

    //HalTimerStart(HAL_TIMER_0,0x1f4);
    HalTimerStart(HAL_TIMER_0,0x218);

}

void Send_Data_by_Wireless(uint8* pdata ,uint8 devType,uint16 len,uint8 DeliverMode, uint16 uniDstAddr, uint8 txOption)
{
    MacParam_t param;
    param.cluster_id = CARD_CLUSTERID;
    param.radius = 0x01;
    if(devType == STAFF_CARD_DEVICE_ID)
    {
        param.panID = STAFF_CARD_PANID;
    }
    else
    {
        param.panID = DEVICE_CARD_PANID;
    }
    if(DeliverMode == MAC_UTIL_UNICAST)
    {
        sAddr_t DstAddr;
        DstAddr.addrMode             = SADDR_MODE_SHORT;

        DstAddr.addr.shortAddr = uniDstAddr; //if an enddevice, need to set mac dstaddr to coordinator's addr.
        MAC_UTIL_McpsDataReq(pdata, len, param.panID, DstAddr, txOption);//MAC_TXOPTION_ACK);
    }
    else
    {
        sAddr_t DstAddr;
        DstAddr.addrMode             = SADDR_MODE_SHORT;  /* MAC addMode shoud be 0X02 even broadcast */
        DstAddr.addr.shortAddr      = uniDstAddr; /* Address should be 0xFF in mac header and 0xFc or 0xFF in NWK header */
        MAC_UTIL_McpsDataReq(pdata, len,param.panID, DstAddr, txOption);//MAC_TXOPTION_NO_RETRANS);
    }

}
static void Start_buzzer(void)
{
    //halTimerSetCount (0x00, 0x105);
    // P0_4
    P0SEL |= (0x01 << 4);// set to peripheral

    T1CCTL2 = (0x00 << 6) | (0x02 << 3) | (0x01 << 2) ;
    T1CC0H = 0x29;
    T1CC0L = 0x28;
    T1CTL = 0;

    T1CTL &= ~0x03; //clear
    T1CTL |= 0x02;  //start
}

static void Stop_buzzer(void)
{
    //HalTimerStop(HAL_TIMER_3);
    T1CTL &= ~(0x03); //clear
    P0_4 = 0;
}
/**************************************************************************************************
**************************************************************************************************/

