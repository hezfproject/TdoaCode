/**************************************************************************************************
Filename:       CR.c
Revised:        $Date: 2011/06/14 01:47:48 $
Revision:       $Revision: 1.10 $

Description:    This file contains the application that can be use to set a device as Location
node from MAC directly which will communicate with a FFD with full z-stack.

**************************************************************************************************/

/**************************************************************************************************
*                                           Includes
**************************************************************************************************/
/* Hal Driver includes */
#include "hal_types.h"
#include "hal_timer.h"
#include "hal_drivers.h"
#include "hal_led.h"
#include "hal_assert.h"
#include "hal_uart.h"
#include "FlashUtil.h"
#include "MacUtil.h"
/* OS includes */
#include "OSAL.h"
#include "OSAL_Tasks.h"
#include "OSAL_Nv.h"

/* App Protocol*/
#include "AppProtocolWrapper.h"

/* Application Includes */
#include "OnBoard.h"

/* MAC Application Interface */
#include "mac_api.h"

/* Application */
#include "CardReader.h"
#include "MBusProto.h"
#include "CRC.h"

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

typedef enum
{
    CR_REPORT_DATA,
    CR_RESPOND,
    CR_EXCEPTION,
} eUartSendType;

/**************************************************************************************************
*                                           Constant
**************************************************************************************************/
#if (MBUS_BAUDRATE == 115200)
#define CR_APP_BAUD  HAL_UART_BR_115200
#endif

#if (MBUS_BAUDRATE == 9600)
#define CR_APP_BAUD  HAL_UART_BR_9600
#endif

// When the Rx buf space is less than this threshold, invoke the Rx callback.
#if !defined( CR_APP_THRESH )
#define CR_APP_THRESH  48
#endif

#if !defined( CR_APP_RX_MAX )
/* The generic safe Rx minimum is 48, but if you know your PC App will not
* continue to send more than a byte after receiving the ~CTS, lower max
* here and safe min in _hal_uart.c to just 8.
*/
#define CR_APP_RX_MAX  50
#endif

#if !defined( CR_APP_TX_MAX )
#define CR_APP_TX_MAX  250
#endif

#if !defined(CR_CARD_BUF_SIZE)
#define CR_CARD_BUF_SIZE 400
#endif

// Millisecs of idle time after a byte is received before invoking Rx callback.
#if !defined( CR_APP_IDLE )
#define CR_APP_IDLE  6
#endif

#define CR_MAX_CARD_IN_PACKET ((MBUS_FRAME_MAX_LEN-sizeof(mbus_hdr_slv_t) - sizeof(mbus_tlv_t) - 2)/sizeof(mbus_card_t))

#define CR_LED_UART0      HAL_LED_1
#define CR_LED_UART1      HAL_LED_3
#define CR_LED_AIR         HAL_LED_2

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
/* Coordinator and Device information */
static uint16        CR_PanId = 0xFFFF;
static uint16        CR_DevShortAddr   = 0;
static uint8         CR_DeviceID = MBUS_IDMAX;


/* TRUE and FALSE value */
static bool          CR_MACTrue = TRUE;
static bool          CR_MACFalse = FALSE;

//static bool     CR_IsBatteryLow   =  FALSE;


/* Task ID */
uint_8 CR_TaskId;

/* Device Info from flash */
static Dev_Info_t CR_DevInfo;

/* UART*/
static uint_8 uartPort = HAL_UART_PORT_0;
static uint_8 uartPortTemp = HAL_UART_PORT_0;

static uint_8 uartReadBufLen0 = 20;
static uint_8 uartReadBuf0[20];
static uint_8 uartReadLen0;

static uint_8 uartReadBufLen1 = 20;
static uint_8 uartReadBuf1[20];
static uint_8 uartReadLen1;

static eUartSendType MBUS_send_type;
static UrgentType chargeed_urgenttype = URGENT_NONE;
static uint_8 urgentCount = 0;

static uint_8 cardUrgenttype = SSREQ_NODATA;

static mbus_card_t cardMsgBuf[CR_CARD_BUF_SIZE];
static uint_16 cardBufHead;
static uint_16 cardBufTail;
static uint_16 cardBufSendIndex = 0;
static bool fSendIndexValid = FALSE;

static uint_8 sendSeqNum = 0;

static bool fSending = FALSE;

static uint_8 sendData[MBUS_FRAME_MAX_LEN];
static uint_16 recIndex;

static bool fClearAndReset = FALSE;

#ifdef DEBUG_CR

static uint_32 totalCards = 0;
static uint_32 sendCards = 0;

static uint_8 lastSend = 0;
static uint_8 lastTime = 0;

static uint_16 passedSeconds = 0;
static uint_16 sendTimes = 0;

static uint_16 readNum = 0;

static uint_16 sendIndex[100];
static uint_16 heads[100];
static uint_16 tails[100];
static bool valids[100];

#endif


/*
static uint_16 crct;
static uint_8 crcTest[] = {0x1, 0x2, 0x0, 0x0, 0x3f, 0x3, 0x2, 0x3c, 0x3, 0x2, 0x14, 0x0, 0x9c, 0x48, 0xca, 0x2, 0x12, 0x0, 0xc3, 0x48,
                                        0xb9, 0x2, 0xe2, 0x7, 0xf2, 0x15, 0xd5, 0x2, 0xe3, 0x6, 0x6e, 0xda, 0xd6, 0x2, 0x57, 0x6, 0xec, 0xd9, 0xc4, 0x2,
                                        0xca, 0x6, 0x1d, 0xde, 0xc5, 0x2, 0xba, 0x6, 0x23, 0x34, 0xd5, 0x2, 0xcf, 0x6, 0xd, 0x4e, 0xd2, 0x2, 0x97, 0x6,
                                        0x56, 0x31, 0xca, 0x2, 0xc5, 0x6, 0x8d, 0x30, 0xd2, 0x2, 0x61, 0x6, 0xea, 0xd9, 0xc8, 0x2, 0x88, 0x6, 0x8c, 0x30,
                                        0xd2, 0x2, 0xed, 0x7, 0x1, 0x16, 0xd5, 0x2, 0xea, 0x7, 0xcd, 0x14, 0xd6, 0x2, 0x25, 0x7, 0xb4, 0xda, 0xd9, 0x2,
                                        0xe7, 0x7, 0x9, 0x17, 0xc9, 0x2, 0xe5, 0x6, 0x6d, 0xdb, 0xbf, 0x2, 0x81, 0x6, 0x56, 0xda, 0xc9, 0x2, 0xfd, 0x6,
                                        0xeb, 0xdb, 0xd7, 0x2, 0x3e, 0x7, 0xff, 0x30, 0xd3, 0x2, 0x3d, 0x7, 0xd7, 0x32, 0xd3, 0x2, 0x42, 0x7, 0x69, 0xda,
                                        0xc7, 0x2, 0xff, 0x6, 0xec, 0xcf, 0xc4, 0x3, 0xff, 0x6, 0x0, 0xdb, 0xd5, 0x72, 0x0, 0xdc, 0x1, 0x1, 0x0, 0x1,
                                        0x8a, 0x0, 0xff, 0xff, 0x21, 0x20, 0x8, 0x2, 0x0, 0x1, 0x92, 0xf2, 0xc7, 0xc7, 0x0, 0x0, 0x31, 0x75, 0xff, 0xff,
                                        0x1, 0x6, 0xd, 0xff, 0x3a, 0x11, 0x2, 0x0, 0x66, 0x3, 0x8f, 0x1, 0x0, 0x0, 0xc, 0x2, 0x15, 0x0, 0xef, 0x47,
                                        0xdd, 0x3, 0x15, 0x0, 0x0, 0xdb, 0xc3, 0x2, 0x19, 0x7, 0xc1, 0xda, 0xc6, 0x2, 0x4e, 0x7, 0x0, 0xdb, 0xd8, 0x2,
                                        0xcc, 0x6, 0x89, 0xdb, 0xd0, 0x2, 0x69, 0x6, 0x61, 0xdb, 0xd1, 0x2, 0x30, 0x7, 0x13, 0xd0, 0xc0, 0x3, 0x30, 0x7,
                                        0x0, 0xd9, 0xd2, 0x2, 0xdd, 0x6, 0xf3, 0xda, 0xdd, 0x2, 0xc7, 0x6, 0xe9, 0x0, 0xd4, 0x2, 0xfe, 0x6, 0xcc, 0x90,
                                        0xd4, 0x2, 0x7d, 0x6, 0xeb, 0xcf, 0xbe, 0x3, 0x7d, 0x6, 0x0, 0xdb, 0xd7, 0x2, 0x10, 0x0, 0x8a, 0x48, 0xc7, 0x2,
                                        0xf8, 0x6, 0x4, 0x33, 0xd7, 0x2, 0xf0, 0x7, 0x90, 0x16, 0xde, 0x2, 0xd4, 0x7, 0x21, 0x1e, 0xe2, 0x2, 0x4e, 0x5,
                                        0x3e, 0x31, 0xdd, 0x2, 0x36, 0x7, 0x4f, 0xda, 0xcb, 0x2, 0x50, 0x7, 0x44, 0x1, 0xd0, 0x2, 0xd4, 0x6, 0x9b, 0x0,
                                        0xd0, 0x2, 0xe, 0x7, 0x71, 0xda, 0xca, 0x2, 0x31, 0x7, 0x3d, 0xdb, 0xdf, 0x2, 0x5b, 0x6, 0x1f, 0xda, 0xdf, 0x2,
                                        0x15, 0x7, 0x64, 0xda, 0xda, 0x2, 0xd0, 0x7, 0xa3, 0x16, 0xdf, 0x3, 0xd0, 0x7, 0x0, 0x7, 0xe6, 0x2, 0xf5, 0x7,
                                        0x8b, 0x16, 0xd8, 0x2, 0xec, 0x7, 0xf9, 0x16, 0xde, 0x2, 0xf3, 0x7, 0x47, 0x15, 0xd6, 0x3, 0x5, 0x0, 0x0, 0x31,
                                        0xca, 0x2, 0xe, 0x0, 0x8b, 0x48, 0xd3, 0x2, 0x22, 0x7, 0x42, 0xe6, 0xd8, 0x2, 0x8, 0x0, 0x5, 0x0, 0xc4, 0x2,
                                        0xe4, 0x6, 0x34, 0xda, 0xda, 0x2, 0x53, 0x6, 0xee, 0xda, 0xcd, 0x2, 0xda, 0x6, 0x95, 0xda, 0xd5, 0x2, 0xe6, 0x7,
                                        0xd4, 0x1d, 0xe1, 0x2, 0xb, 0x0, 0x5c, 0x48, 0xc5, 0x2, 0x46, 0x6, 0xb3, 0xcf, 0xbd, 0x3, 0x46, 0x6, 0x0, 0xdb,
                                        0xd2, 0x2, 0xf, 0x0, 0x7d, 0x48, 0xd2, 0x2, 0xf9, 0x6, 0xd3, 0x97, 0xc5, 0x2, 0xa2, 0x6, 0x4f, 0xda, 0xd8, 0x2,
                                        0xd3, 0x6, 0x1a, 0x33, 0xd8, 0x2, 0xc8, 0x6, 0xf8, 0x4e, 0xd8, 0x2, 0x51, 0x6, 0xaf, 0xda, 0xd8, 0x2, 0xed, 0x6,
                                        0xbb, 0x0, 0xd2, 0x2, 0x46, 0x7, 0x3e, 0x34, 0xd0, 0x2, 0xb0, 0x6, 0xdd, 0xd9, 0xd6, 0x3, 0xf9, 0x6, 0x0, 0xda,
                                        0xdd, 0x2, 0xe8, 0x7, 0x19, 0x16, 0xce, 0x2, 0xcd, 0x6, 0xd4, 0xdb, 0xc9, 0x2, 0x24, 0x7, 0x24, 0x1, 0xd1, 0x2,
                                        0x87, 0x6, 0x85, 0x30, 0xd3, 0x2, 0xeb, 0x6, 0xe4, 0xdb, 0xd6, 0x2, 0xe9, 0x7, 0x22, 0x7, 0xe4, 0x3, 0xe9, 0x7,
                                        0x0, 0x14, 0xd7, 0x2, 0x77, 0x6, 0xf7, 0xd9, 0xc4, 0x2, 0xbd, 0x6, 0xb7, 0x4d, 0xd5, 0x2, 0x38, 0x7, 0x8d, 0xdb,
                                        0xd2, 0x2, 0xf4, 0x7, 0x29, 0x15, 0xc2, 0x2, 0xd9, 0x7, 0x39, 0x1e, 0xd7, 0x2, 0xfb, 0x6, 0xd9, 0xd9, 0xce, 0x2,
                                        0xd6, 0x7, 0x27, 0x1e, 0xcb, 0x2, 0x6c, 0x6, 0x9e, 0xcf, 0xc2, 0x2, 0x45, 0x7, 0x6e, 0xda, 0xdd, 0x2, 0x50, 0x6,
                                        0x16, 0x26, 0xd5, 0x2, 0x7, 0x0, 0x4f, 0x0, 0xd6, 0x2, 0x10, 0x0, 0x63, 0x0, 0xd2, 0x3, 0x10, 0x0, 0x0, 0x48,
                                        0xc4, 0x2, 0x93, 0x6, 0xca, 0x6f, 0xc4, 0x2, 0xd8, 0x6, 0x28, 0x30, 0xd3, 0x2, 0x37, 0x7, 0xa1, 0xda, 0xd3, 0x2,
                                        0x3a, 0x8, 0x68, 0x20, 0xdf, 0x2, 0xd5, 0x6, 0xad, 0xda, 0xc6, 0x2, 0xca, 0x6, 0x1e, 0xde, 0xc4, 0x3, 0xca, 0x6,
                                        0x0, 0xdb, 0xd8, 0x2, 0xd0, 0x6, 0x6b, 0x39, 0xd2, 0x2, 0x9, 0x0, 0x85, 0x0, 0xd7, 0x2, 0xd8, 0x7, 0x57, 0x16,
                                        0xdb, 0x2, 0x2, 0x7, 0x21, 0xdb, 0xda, 0x2, 0xb3, 0x6, 0xf5, 0xd9, 0xda, 0x2, 0x47, 0x6, 0x49, 0x31, 0xd1, 0x2,
                                        0x84, 0x6, 0x19, 0xdb, 0xd4, 0x2, 0xc, 0x0, 0x55, 0x48, 0xd1, 0x2, 0xb9, 0xb, 0x7a, 0x20, 0xdf, 0x2, 0x7a, 0x6,
                                        0xce, 0x34, 0xd4, 0x2, 0x1, 0x0, 0x23, 0x0, 0xd0, 0x3, 0x1, 0x0, 0x0, 0x16, 0xe0, 0x2, 0xe7, 0x6, 0x73, 0x30,
                                        0xd9, 0x2, 0xff, 0x6, 0xed, 0xcf, 0xc3, 0x3, 0xff, 0x6, 0x0, 0x16, 0xd3, 0x2, 0xd, 0x0, 0x69, 0x48, 0xc5, 0x2,
                                        0x37, 0x6, 0xa2, 0x32, 0xde, 0x2, 0x8c, 0x6, 0xc2, 0xdb, 0xd3, 0x2, 0xd2, 0x6, 0xac, 0x0, 0xd8, 0x2, 0x4d, 0x6,
                                        0x21, 0xdc, 0xd8, 0x2, 0xf6, 0x7, 0x24, 0x17, 0xde, 0x2, 0x13, 0x0, 0xb0, 0x48, 0xc7, 0x2, 0xdf, 0x7, 0xd6, 0x14,
                                        0xd7, 0x2, 0x30, 0x7, 0x14, 0xd0, 0xc2, 0x2, 0x38, 0x6, 0x75, 0x31, 0xcd, 0x2, 0x11, 0x0, 0x33, 0x48, 0xd1, 0x2,
                                        0xee, 0x6, 0xa, 0x31, 0xc5, 0x2, 0x85, 0x6, 0x2, 0xae, 0xd0, 0x2, 0x7d, 0x6, 0xec, 0xcf, 0xbe};

*/

/**************************************************************************************************
*                                     Local Function Prototypes
**************************************************************************************************/
/* Setup routines */
static void         CR_DeviceStartup(uint_8* pData);
/* Support */
static void         CR_ReadDevInfo();
static void CR_ParseAirMessage(macCbackEvent_t *p);
static void CR_ParseUartCmd(uint_8* data, uint_16 len);


/**************************************************************************************************
*
* @fn          CR_Init
*
* @brief       Initialize the application
*
* @param       taskId - taskId of the task after it was added in the OSAL task queue
*
* @return      none
*
**************************************************************************************************/
void CR_Init(uint_8 taskId)
{
    halUARTCfg_t uartConfig;

    /* Initialize the task id */
    CR_TaskId = taskId;

    /* initialize MAC features */
    MAC_Init();
    MAC_InitCoord();

    /* Reset the MAC */
    MAC_MlmeResetReq(TRUE);

#if(defined WATCHDOG) && (WATCHDOG==TRUE)
    StartWatchDog(DOGTIMER_INTERVAL_1S);
    osal_set_event(CR_TaskId, CR_FEEDWATCHDOG_EVENT);
#endif


    /* initial MacUtil*/
    MacUtil_t Macutil;
    Macutil.panID = 0xFFFF;                          // Card broadcast to all PANs
    Macutil.dst_endpoint = APS_DST_ENDPOINT;
    Macutil.src_endpoint = APS_SRC_ENDPOINT;
    Macutil.cluster_id = APS_CLUSTER_ID;
    Macutil.profile_id = APS_PROFILE_ID;
    Macutil.NodeType =  NODETYPE_ROUTER;
    MAC_UTIL_INIT(&Macutil);

    /*init uart*/
    uartConfig.configured           = TRUE;              // 2430 don't care.
    uartConfig.baudRate             = CR_APP_BAUD;
    uartConfig.flowControl          = FALSE;
    uartConfig.flowControlThreshold = CR_APP_THRESH;
    uartConfig.rx.maxBufSize        = CR_APP_RX_MAX;
    uartConfig.tx.maxBufSize        = CR_APP_TX_MAX;
    uartConfig.idleTimeout          = CR_APP_IDLE;   // 2430 don't care.
    uartConfig.intEnable            = TRUE;              // 2430 don't care.
    uartConfig.callBackFunc         = NULL;

    /*two ports. one for backup*/
    HalUARTOpen(HAL_UART_PORT_0, &uartConfig);
    //HalUARTOpen(HAL_UART_PORT_1, &uartConfig);

    CR_ReadDevInfo();

    CR_DeviceStartup(NULL);


    /*Start Watch Dog*/
#ifdef WATCHDOG
    StartWatchDog(DOGTIMER_INTERVAL_1S);
#endif

    osal_start_timerEx(taskId, CR_UART_READ0_EVENT, 1000);
    //osal_start_timerEx(taskId, CR_UART_READ1_EVENT, 1010);
    osal_start_timerEx(taskId, CR_LED_CONTROL_EVENT, 2000);

    /*
        uint_16 len = sizeof(crcTest);
        uint_8 *p = crcTest;
        crct = CRC16(p, sizeof(mbus_header_t), 0xFFFF);
        len -= sizeof(mbus_header_t);
        p+= sizeof(mbus_header_t);

        crct = CRC16(p, sizeof(mbus_tlv_t), crct);
        p+= sizeof(mbus_tlv_t);
        len -= sizeof(mbus_tlv_t);

        crct = CRC16(p, len, crct);
    */

    //uint_8 Channel = CR_DevInfo.ExitAddr[EXT_MACADDR_CHANNEL]; // must after CR_ReadDevInfo()
    //MAC_UTIL_HalInit(Channel, 0);
}

/**************************************************************************************************
*
* @fn          CR_ProcessEvent
*
* @brief       This routine handles events
*
* @param       taskId - ID of the application task when it registered with the OSAL
*              events - Events for this task
*
* @return      16bit - Unprocessed events
*
**************************************************************************************************/
uint16 CR_ProcessEvent(uint_8 taskId, uint16 events)
{
    uint_8* pMsg;
    macCbackEvent_t* pData;

#if(defined WATCHDOG) && (WATCHDOG==TRUE)
    if(events & CR_FEEDWATCHDOG_EVENT)
    {
        osal_start_timerEx(CR_TaskId, CR_FEEDWATCHDOG_EVENT, 300);
        FEEDWATCHDOG();
        return events ^ CR_FEEDWATCHDOG_EVENT;
    }
#endif

    if(events & SYS_EVENT_MSG)
    {
        while((pMsg = osal_msg_receive(CR_TaskId)) != NULL)
        {
            switch(*pMsg)
            {
            case MAC_MLME_START_CNF:
            {
                /* if start fail, restart */
                pData = (macCbackEvent_t *) pMsg;
                if(pData->startCnf.hdr.status != ZSUCCESS)
                {
                    SystemReset();
                }
                break;
            }
            case MAC_MCPS_DATA_CNF:
            {
                pData = (macCbackEvent_t *) pMsg;
                osal_msg_deallocate((uint_8 *) pData->dataCnf.pDataReq);
                break;
            }

            case MAC_MCPS_DATA_IND:
            {
                pData = (macCbackEvent_t *) pMsg;

                CR_ParseAirMessage(pData);
                break;
            }

            }

            /* Deallocate */
            osal_msg_deallocate((uint_8 *) pMsg);
        }

        return events ^ SYS_EVENT_MSG;
    }

    if(events & CR_LED_CONTROL_EVENT)
    {
        HalLedSet(CR_LED_AIR, HAL_LED_MODE_OFF);
        HalLedSet(CR_LED_UART0, HAL_LED_MODE_OFF);
        HalLedSet(CR_LED_UART1, HAL_LED_MODE_OFF);

#ifdef DEBUG_CR
        if(passedSeconds) passedSeconds ++;
#endif

        osal_start_timerEx(taskId, CR_LED_CONTROL_EVENT, 1000);
        return events ^ CR_LED_CONTROL_EVENT;
    }

    if(events & CR_UART_READ0_EVENT)
    {
        uint_8 len;
        static bool fIgnore = FALSE;

        if(fSending)
        {
            osal_start_timerEx(taskId, CR_UART_READ0_EVENT, MBUS_FRAME_TIMEOUT);
            return events ^ CR_UART_READ0_EVENT;
        }

        len = HalUARTRead(HAL_UART_PORT_0, uartReadBuf0 + uartReadLen0, uartReadBufLen0 - uartReadLen0);

        uartReadLen0 += len;

        // too long to be a possible cmd from master
        if(uartReadLen0 > sizeof(mbus_hdr_mstr_t) + 2 && len > 0)
        {
            fIgnore = TRUE;
        }

        //continue ignoring till len == 0
        if(fIgnore)
        {
            uartReadLen0 = 0;
        }

        if(len == 0)
        {
            fIgnore = FALSE;
        }

#ifdef DEBUG_CR
        else
        {
            readNum++;
        }
#endif

        uint_8 sizet = sizeof(mbus_hdr_mstr_t) + 2;
        if(uartReadLen0 == sizet && len == 0)
        {
            uartPortTemp = HAL_UART_PORT_0;
            CR_ParseUartCmd(uartReadBuf0, uartReadLen0);
            uartReadLen0 = 0;
        }
        else if(uartReadLen0 > 0 && len == 0)
        {
            uartReadLen0 = 0;
        }

        osal_start_timerEx(taskId, CR_UART_READ0_EVENT, MBUS_FRAME_TIMEOUT);
        return events ^ CR_UART_READ0_EVENT;
    }

    if(events & CR_UART_READ1_EVENT)
    {
        uint_8 len;
        static bool fIgnore = FALSE;

        if(fSending)
        {
            osal_start_timerEx(taskId, CR_UART_READ0_EVENT, MBUS_FRAME_TIMEOUT);
            return events ^ CR_UART_READ0_EVENT;
        }

        len = HalUARTRead(HAL_UART_PORT_1, uartReadBuf1 + uartReadLen1, uartReadBufLen1 - uartReadLen1);

        uartReadLen1 += len;

        // too long to be a possible cmd from master
        if(uartReadLen1 > sizeof(mbus_hdr_mstr_t) + 2 && len > 0)
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

        if(uartReadLen1 == sizeof(mbus_hdr_mstr_t) + 2 && len == 0)
        {
            uartPortTemp = HAL_UART_PORT_1;
            CR_ParseUartCmd(uartReadBuf1, uartReadLen1);
            uartReadLen1 = 0;
        }
        else if(uartReadLen1 > 0 && len == 0)
        {
            uartReadLen1 = 0;
        }

        osal_start_timerEx(taskId, CR_UART_READ1_EVENT, MBUS_FRAME_TIMEOUT);
        return events ^ CR_UART_READ1_EVENT;
    }

    if(events & CR_UART_WRITE_EVENT)
    {
        if(MBUS_send_type == CR_REPORT_DATA)
        {
            uint_16 txAvail = Hal_UART_TxBufLen(uartPort);

            static bool fInit = FALSE;
            static uint_16 cardNum = 0;

            //first time
            if(!fInit)
            {

                fInit = TRUE;
                fSending = TRUE;
                //HAL_ASSERT(txAvail == CR_APP_TX_MAX-1 );

                if(txAvail != CR_APP_TX_MAX - 1)
                {
                    HalUARTFlushTxBuf(uartPort);
                }

                mbus_hdr_slv_t mbusHead;
                mbus_tlv_t mbusTlv;

                cardNum = (cardBufHead >= cardBufTail) ?
                          (cardBufHead - cardBufTail) :
                          (CR_CARD_BUF_SIZE + cardBufHead - cardBufTail);

                if(cardNum > CR_MAX_CARD_IN_PACKET) cardNum = CR_MAX_CARD_IN_PACKET;

#ifdef DEBUG_CR
                sendCards += cardNum;
                lastSend = cardNum;
                lastTime = 0;
                sendTimes++;
                if(passedSeconds == 0) passedSeconds = 1;
#endif
                osal_memcpy(mbusHead.sync, MBUS_SYNC, sizeof(mbusHead.sync));
                mbusHead.cmd = MBUS_CMD_RSP;
                MBUS_SET_SLAVE_VERSION(mbusHead.frame_control, MBUS_PROTO_VERSION);
                mbusHead.slv_id = CR_DeviceID;
                MBUS_SET_SLAVE_SEQ(mbusHead.frame_control, sendSeqNum);

                if(cardNum > 0)
                    mbusHead.data_len = sizeof(mbus_tlv_t) + cardNum * sizeof(mbus_card_t);
                else
                    mbusHead.data_len = sizeof(mbus_tlv_t);

                mbusTlv.len = cardNum * sizeof(mbus_card_t);
                mbusTlv.type = MBUS_TLV_CARD_READER;

                HalUARTWrite(uartPort, (uint_8*)(&mbusHead), sizeof(mbusHead));
                osal_memcpy(sendData + recIndex, (uint_8*)(&mbusHead), sizeof(mbusHead));
                recIndex += sizeof(mbusHead);

                txAvail -= sizeof(mbusHead);

                /* if no card , also send mbusTlv to ARM */
                // if(cardNum > 0)
                // {
                HalUARTWrite(uartPort, (uint_8*)(&mbusTlv), sizeof(mbusTlv));
                osal_memcpy(sendData + recIndex, (uint_8*)(&mbusTlv), sizeof(mbusTlv));
                recIndex += sizeof(mbusTlv);

                txAvail -= sizeof(mbusTlv);
                //   }

            }

#ifdef DEBUG_CR
            lastTime++;
#endif

            uint_8 count = txAvail / sizeof(mbus_card_t);
            uint_8 left = 0;

            /* in case the uart writing speed is slower than card coming speed*/
            if(!fSendIndexValid)
            {
                cardBufSendIndex = cardBufTail;
            }

#ifdef DEBUG_CR
            static uint_16 ii = 0;
            sendIndex[ii] = cardBufSendIndex;
            heads[ii] = cardBufHead;
            tails[ii] = cardBufTail;
            valids[ii] = fSendIndexValid;
            ii++;
            if(ii == 100) ii = 0;
#endif

            if(count > cardNum) count = cardNum;
            if(count > 0)
            {
                uint_8 tmpCount = count;
                if(tmpCount + cardBufSendIndex > CR_CARD_BUF_SIZE)
                {
                    left = tmpCount - (CR_CARD_BUF_SIZE - cardBufSendIndex);
                    tmpCount = (CR_CARD_BUF_SIZE - cardBufSendIndex);
                }
                HalUARTWrite(uartPort, (uint_8*)(&cardMsgBuf[cardBufSendIndex]), tmpCount * sizeof(mbus_card_t));
                osal_memcpy(sendData + recIndex, (uint_8*)(&cardMsgBuf[cardBufSendIndex]), tmpCount * sizeof(mbus_card_t));
                recIndex += tmpCount * sizeof(mbus_card_t);


                if(left)
                {
                    HalUARTWrite(uartPort, (uint_8*)(&cardMsgBuf[0]), left * sizeof(mbus_card_t));
                    osal_memcpy(sendData + recIndex, (uint_8*)(&cardMsgBuf[0]), left * sizeof(mbus_card_t));
                    recIndex += left * sizeof(mbus_card_t);

                }

                /*sendIndex moved, then send to true*/
                fSendIndexValid = TRUE;
            }
            /*something wrong with uartTX*/
            else if(cardNum != 0)
            {
                recIndex = 0;
                fInit = FALSE;
                fSending = FALSE;
                return events ^ CR_UART_WRITE_EVENT;
            }

            cardNum -= count;
            cardBufSendIndex += count;
            if(cardBufSendIndex >= CR_CARD_BUF_SIZE)
            {
                cardBufSendIndex -= CR_CARD_BUF_SIZE;
            }
            txAvail -= count * sizeof(mbus_card_t);

            //fill CRC
            if(cardNum == 0 && txAvail > 2)
            {
                uint_16 recCRC = CRC16(sendData, recIndex, 0xFFFF);
                HalUARTWrite(uartPort, (uint_8*)(&recCRC), sizeof(recCRC));

                recIndex = 0;
                fInit = FALSE;
                fSending = FALSE;
            }
            else
            {
                /*txbuf full, wait txbuf to be empty*/
                uint_32 bits = CR_APP_TX_MAX;
                bits *= 8 * 1000;
                osal_start_timerEx(taskId, CR_UART_WRITE_EVENT, bits / MBUS_BAUDRATE);
            }
        }

        if(MBUS_send_type == CR_RESPOND)
        {
            uint_16 txAvail = Hal_UART_TxBufLen(uartPort);

            uint_16 crc = 0xFFFF;

            if(txAvail != CR_APP_TX_MAX - 1)
            {
                HalUARTFlushTxBuf(uartPort);
            }

            mbus_hdr_slv_t mbusHead;

            osal_memcpy(mbusHead.sync, MBUS_SYNC, sizeof(mbusHead.sync));
            mbusHead.cmd = MBUS_CMD_RSP;
            MBUS_SET_SLAVE_VERSION(mbusHead.frame_control, MBUS_PROTO_VERSION);
            mbusHead.slv_id = CR_DeviceID;
            mbusHead.data_len = 0;

            /*seq is always 0 in this mode*/
            MBUS_SET_SLAVE_SEQ(mbusHead.frame_control, 0);

            crc = CRC16((uint_8*)(&mbusHead), sizeof(mbusHead), crc);
            HalUARTWrite(uartPort, (uint_8*)(&mbusHead), sizeof(mbusHead));
            HalUARTWrite(uartPort, (uint_8*)(&crc), sizeof(crc));

            if(fClearAndReset)
            {
                txAvail = Hal_UART_TxBufLen(uartPort);
                while(txAvail != CR_APP_TX_MAX - 1)
                {
                    txAvail = Hal_UART_TxBufLen(uartPort);
                }
                SystemReset();
            }


        }

        if(MBUS_send_type == CR_EXCEPTION)
        {
            uint_16 crc;
            uint_16 txAvail = Hal_UART_TxBufLen(uartPort);

            HAL_ASSERT(txAvail == CR_APP_TX_MAX - 1);

            mbus_hdr_slv_t mbusHead;
            mbusHead.cmd = MBUS_CMD_EXCEPCTION;
            mbusHead.data_len = 0;
            mbusHead.slv_id = CR_DeviceID;
            MBUS_SET_SLAVE_VERSION(mbusHead.frame_control, MBUS_PROTO_VERSION);
            /*seq is always 0 in this mode*/
            MBUS_SET_SLAVE_SEQ(mbusHead.frame_control, 0);

            HalUARTWrite(uartPort, (uint_8*)(&mbusHead), sizeof(mbusHead));
            txAvail -= sizeof(mbusHead);
            crc = CRC16((uint_8*)(&mbusHead), sizeof(mbusHead), 0xFFFF);

            HalUARTWrite(uartPort, (uint_8*)(&crc), sizeof(crc));

        }

        return events ^ CR_UART_WRITE_EVENT;
    }

    if(events & CR_BACKTO_NORMAL_EVENT)
    {
        if(urgentCount < 10)
        {
            urgentCount++;
            osal_start_timerEx(taskId, CR_BACKTO_NORMAL_EVENT, 60000);
        }
        else
        {
            chargeed_urgenttype = URGENT_NONE;
            cardUrgenttype = SSREQ_NODATA;
        }

        return events ^ CR_BACKTO_NORMAL_EVENT;
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

    switch(pData->hdr.event)
    {
    case MAC_MLME_BEACON_NOTIFY_IND:

        len += sizeof(macPanDesc_t) + pData->beaconNotifyInd.sduLength +
               MAC_PEND_FIELDS_LEN(pData->beaconNotifyInd.pendAddrSpec);
        if((pMsg = (macCbackEvent_t *) osal_msg_allocate(len)) != NULL)
        {
            /* Copy data over and pass them up */
            osal_memcpy(pMsg, pData, sizeof(macMlmeBeaconNotifyInd_t));
            pMsg->beaconNotifyInd.pPanDesc = (macPanDesc_t *)((uint_8 *) pMsg + sizeof(macMlmeBeaconNotifyInd_t));
            osal_memcpy(pMsg->beaconNotifyInd.pPanDesc, pData->beaconNotifyInd.pPanDesc, sizeof(macPanDesc_t));
            pMsg->beaconNotifyInd.pSdu = (uint_8 *)(pMsg->beaconNotifyInd.pPanDesc + 1);
            osal_memcpy(pMsg->beaconNotifyInd.pSdu, pData->beaconNotifyInd.pSdu, pData->beaconNotifyInd.sduLength);
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
        osal_msg_send(CR_TaskId, (byte *) pMsg);
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
* @fn      CR_DeviceStartup(uint_8* pData)
*
* @brief   Update the timer per tick
*
* @param   beaconEnable: TRUE/FALSE
*
* @return  None
*
**************************************************************************************************/
void CR_DeviceStartup(uint_8* pData)
{
    macMlmeStartReq_t   startReq;

    MAC_MlmeSetReq(MAC_EXTENDED_ADDRESS, &CR_DevInfo.ExitAddr);

    /* Setup PAN ID */
    MAC_MlmeSetReq(MAC_PAN_ID, &CR_PanId);

    /* This device is setup for Direct Message */
    MAC_MlmeSetReq(MAC_RX_ON_WHEN_IDLE, &CR_MACTrue);

    uint_8 tmp = 0;
    MAC_MlmeSetReq(MAC_MAX_FRAME_RETRIES, &tmp);

    uint_8 channel = CR_DevInfo.ExitAddr[EXT_MACADDR_CHANNEL];
    MAC_MlmeSetReq(MAC_LOGICAL_CHANNEL, &channel);

    CR_DevShortAddr = 0;
    MAC_MlmeSetReq(MAC_SHORT_ADDRESS, &CR_DevShortAddr);

    /* change CCA param */
    uint_8 min_be = 0;
    MAC_MlmeSetReq(MAC_MIN_BE, &min_be);

    uint_8 max_backoff = 5;
    MAC_MlmeSetReq(MAC_MAX_CSMA_BACKOFFS, &max_backoff);


    /* Fill in the information for the start request structure */
    startReq.startTime = 0;
    startReq.panId = CR_PanId;
    startReq.logicalChannel = channel;
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

void CR_ParseAirMessage(macCbackEvent_t *p)
{
    uint_8* data = p->dataInd.msdu.p;
    uint_8 len = p->dataInd.msdu.len;
    uint_16 srcAddr = p->dataInd.mac.srcAddr.addr.shortAddr;
    int_8 rssi = p->dataInd.mac.rssi;

    sData_t sData;
    sData.p = data;
    sData.len = len;

    uint_8 clusterID = MAC_UTIL_GetClusterID(sData);

    sData_t ApsData = MAC_UTIL_RemoveHeader(sData);

    bool flag = FALSE;

    HAL_TOGGLE_LED3();

    if(clusterID == CHARGEED_CLUSTERID)
    {
        APPWrapper_t* AppPkt = (APPWrapper_t*)(ApsData.p);
        uint_8 appflag = AppPkt->app_flag;
        switch(appflag)
        {
        case CHARGEED_SSREQ:
        {
            uint8 reqtype = AppPkt->app_chargeed_ssReq.reqtype;

            flag = TRUE;
            if(reqtype == SSREQ_POLL)
            {
                app_chargeed_ssRsp_t app_chargeed_ssRsp;
                app_chargeed_ssRsp.msgtype = CHARGEED_SSRSP;
                app_chargeed_ssRsp.srcPan = CR_PanId;
                app_chargeed_ssRsp.locnode_num = CR_DeviceID;
                app_chargeed_ssRsp.seqnum = AppPkt->app_chargeed_ssReq.seqnum;

                app_chargeed_ssRsp.urgent_type = chargeed_urgenttype;
                app_chargeed_ssRsp.urgent_value = 0;

                MacParam_t param;
                param.cluster_id = CHARGEED_CLUSTERID;
                param.panID = CARD_NWK_ADDR;
                param.radius = 1;

                if (URGENT_NONE != app_chargeed_ssRsp.urgent_type)
                {
                    MAC_UTIL_BuildandSendDataPAN(&param, (uint8 *) &app_chargeed_ssRsp,
                                                 sizeof(app_chargeed_ssRsp_t),
                                                 MAC_UTIL_UNICAST, srcAddr,
                                                 MAC_TXOPTION_NO_CNF);
                }
            }

            /* If received a SSREQ_OUT or SSREQ_POLL , Send SSIND to SPI*/
            if(reqtype == SSREQ_OUT || reqtype == SSREQ_POLL)
            {
                cardMsgBuf[cardBufHead].type = MBUS_TYPE_RSSI;

                cardMsgBuf[cardBufHead].data.cardrssi.card_num = srcAddr;
                cardMsgBuf[cardBufHead].data.cardrssi.seq = AppPkt->app_chargeed_ssReq.seqnum;
                cardMsgBuf[cardBufHead].data.cardrssi.rssi = rssi;

            }
            else
            {
                flag = FALSE;
            }
            break;
        }
        case URGENT:
        {
            uint8 chargeed_urgenttype = AppPkt->app_Urgent.urgenttype;

            flag = TRUE;
            if(chargeed_urgenttype == ALERT)
            {
                app_Urgent_t app_Urgent;
                app_Urgent.msgtype = URGENT;
                app_Urgent.urgenttype = ALERTACK;
                app_Urgent.value = 0;

                MacParam_t param;
                param.cluster_id = CHARGEED_CLUSTERID;
                param.panID = CARD_NWK_ADDR;
                param.radius = 1;
                MAC_UTIL_BuildandSendDataPAN(&param, (uint8 *) &app_Urgent, sizeof(app_Urgent_t),
                                             MAC_UTIL_UNICAST, srcAddr,
                                             MAC_TXOPTION_NO_CNF);

                cardMsgBuf[cardBufHead].data.alm.value = AppPkt->app_Urgent.value;
                cardMsgBuf[cardBufHead].data.alm.card_num = srcAddr;
                cardMsgBuf[cardBufHead].type = MBUS_TYPE_ALARM;
            }
            else if(chargeed_urgenttype == NOPWR)
            {
                cardMsgBuf[cardBufHead].data.nopower.value = AppPkt->app_Urgent.value;
                cardMsgBuf[cardBufHead].data.nopower.card_num = srcAddr;
                cardMsgBuf[cardBufHead].type = MBUS_TYPE_NOPOWER;
            }
            else
            {
                flag = FALSE;
            }

            break;
        }
        default:
            break;
        }

    }

    if(clusterID == CARD_CLUSTERID)
    {
        APPWrapper_t* AppPkt = (APPWrapper_t*)(ApsData.p);
        uint_8 appflag = AppPkt->app_flag;
        switch(appflag)
        {
        case SSREQ:
        {
            uint8 reqtype = AppPkt->app_ssReq.reqtype;

            flag = TRUE;
            if(reqtype == SSREQ_POLL)
            {
                if(cardUrgenttype == SSREQ_NODATA)
                {
                    app_ssReq_t app_ssReq;
                    app_ssReq.msgtype = SSREQ;
                    app_ssReq.reqtype = cardUrgenttype;
                    app_ssReq.NWK_ADDR = CARD_NWK_ADDR;
                    app_ssReq.seqnum = AppPkt->app_ssReq.seqnum;

                    MacParam_t param;
                    param.cluster_id = CARD_CLUSTERID;
                    param.panID = CARD_NWK_ADDR;
                    param.radius = 1;

                    MAC_UTIL_BuildandSendDataPAN(&param, (uint8 *) &app_ssReq, sizeof(app_ssReq_t),
                                                 MAC_UTIL_UNICAST, srcAddr,   MAC_TXOPTION_NO_CNF);
                }
                else
                {
                    app_Urgent_t app_Urgent;
                    app_Urgent.msgtype = URGENT;
                    app_Urgent.urgenttype = cardUrgenttype;
                    app_Urgent.value = 0;

                    MacParam_t param;
                    param.cluster_id = CARD_CLUSTERID;
                    param.panID = CARD_NWK_ADDR;
                    param.radius = 1;

                    //FIXME, why not retransmit when urgent???
                    MAC_UTIL_BuildandSendDataPAN(&param, (uint8 *) &app_Urgent, sizeof(app_Urgent),
                                                 MAC_UTIL_UNICAST, srcAddr,   MAC_TXOPTION_NO_CNF);
                }

            }

            /* If received a SSREQ_OUT or SSREQ_POLL , Send SSIND to SPI*/
            if(reqtype == SSREQ_OUT || reqtype == SSREQ_POLL)
            {
                cardMsgBuf[cardBufHead].type = MBUS_TYPE_RSSI;

                cardMsgBuf[cardBufHead].data.cardrssi.card_num = srcAddr;
                cardMsgBuf[cardBufHead].data.cardrssi.seq = AppPkt->app_chargeed_ssReq.seqnum;
                cardMsgBuf[cardBufHead].data.cardrssi.rssi = rssi;

            }
            break;
        }

        case URGENT:
        {
            uint8 chargeed_urgenttype = AppPkt->app_Urgent.urgenttype;

            flag = TRUE;
            if(chargeed_urgenttype == ALERT)
            {
                app_Urgent_t app_Urgent;
                app_Urgent.msgtype = URGENT;
                app_Urgent.urgenttype = ALERTACK;
                app_Urgent.value = 0;

                MacParam_t param;
                param.cluster_id = CARD_CLUSTERID;
                param.panID = CARD_NWK_ADDR;
                param.radius = 1;
                MAC_UTIL_BuildandSendDataPAN(&param, (uint8 *) &app_Urgent, sizeof(app_Urgent_t),
                                             MAC_UTIL_UNICAST, srcAddr,
                                             MAC_TXOPTION_NO_CNF);

                cardMsgBuf[cardBufHead].data.alm.value = AppPkt->app_Urgent.value;
                cardMsgBuf[cardBufHead].data.alm.card_num = srcAddr;
                cardMsgBuf[cardBufHead].type = MBUS_TYPE_ALARM;
            }
            else if(chargeed_urgenttype == NOPWR)
            {
                cardMsgBuf[cardBufHead].data.nopower.value = AppPkt->app_Urgent.value;
                cardMsgBuf[cardBufHead].data.nopower.card_num = srcAddr;
                cardMsgBuf[cardBufHead].type = MBUS_TYPE_NOPOWER;
            }
            break;
        }
        default:
            break;
        }

    }

    if(flag)
    {

#ifdef DEBUG_CR
        totalCards++;
#endif

        cardBufHead++;
        if(cardBufHead >= CR_CARD_BUF_SIZE)
        {
            cardBufHead = 0;
        }

        if(cardBufHead == cardBufTail)
        {
            cardBufTail++;
            if(cardBufTail == CR_CARD_BUF_SIZE)
                cardBufTail = 0;

            /*the tail catch up the sendIndex*/
            if(fSendIndexValid && cardBufTail == cardBufSendIndex)
                fSendIndexValid  = FALSE;
        }
    }

    //HAL_TOGGLE_LED3();

}


/**************************************************************************************************
*
* @fn      CR_ParseUartCmd
*
* @brief   parse the uart coming data
*
**************************************************************************************************/
static void CR_ParseUartCmd(uint_8* data, uint_16 len)
{
    /*CRC doesn't include the sync unit*/
    uint_16 crc =  CRC16(data, len - 2, 0xFFFF);
    bool f_CRC = TRUE;
    uint_8 cmdType;
    uint_8 querySeq;

    if(crc != *((uint_16*)(data + len - 2)))
        f_CRC = FALSE;

    if(!f_CRC)
    {
        //MBUS_send_type = CR_EXCEPTION;
        //osal_start_timerEx(CR_TaskId, CR_UART_WRITE_EVENT, MBUS_FRAME_INTERVAL);
        return;
    }

    if(uartPortTemp == HAL_UART_PORT_0)
        HAL_TOGGLE_LED1();
    else
        HAL_TOGGLE_LED2();

    if(((mbus_hdr_mstr_t*)data)->slv_id != CR_DeviceID) return;
    if(MBUS_GET_MASTER_VERSION(((mbus_hdr_mstr_t*)data)->frame_control) != MBUS_PROTO_VERSION) return;

    /*the port works well, then set uartPort*/
    uartPort = uartPortTemp;

    cmdType = ((mbus_hdr_mstr_t*)data)->cmd;
    querySeq = MBUS_GET_MASTER_SEQ(((mbus_hdr_mstr_t*)data)->frame_control);

    switch(cmdType)
    {
    case(MBUS_CMD_CLR):
    {
        fClearAndReset = TRUE;
#ifdef DEBUG_CR
        totalCards = 0;
        sendCards = 0;
#endif
        cardBufHead = 0;
        cardBufTail = 0;
        cardBufSendIndex = 0;
        sendSeqNum = 0;
        MBUS_send_type = CR_RESPOND;
        osal_start_timerEx(CR_TaskId, CR_UART_WRITE_EVENT, MBUS_FRAME_INTERVAL);
        break;
    }

    case(MBUS_CMD_QRY):
    {
        /*resend*/
        if(querySeq == sendSeqNum)
        {
            fSendIndexValid = FALSE;
        }
        /*request new data*/
        else
        {
            sendSeqNum = querySeq;
        }

        if(fSendIndexValid)
        {
            /*cardBufTail catches fSendIndexValid*/
            fSendIndexValid = FALSE;
            cardBufTail = cardBufSendIndex;
        }

        MBUS_send_type = CR_REPORT_DATA;
        osal_start_timerEx(CR_TaskId, CR_UART_WRITE_EVENT, MBUS_FRAME_INTERVAL);
        break;
    }

    case(MBUS_CMD_RETREAT):
    {
        urgentCount = 0;
        chargeed_urgenttype = RETREAT;
        cardUrgenttype = RETREAT;
        MBUS_send_type = CR_RESPOND;
        osal_start_timerEx(CR_TaskId, CR_UART_WRITE_EVENT, MBUS_FRAME_INTERVAL);
        osal_start_timerEx(CR_TaskId, CR_BACKTO_NORMAL_EVENT, 60000);
        break;
    }

    case(MBUS_CMD_CANCEL_RETREAT):
    {
        urgentCount = 0;
        chargeed_urgenttype = CANCELRETREAT;
        cardUrgenttype = CANCELRETREAT;
        MBUS_send_type = CR_RESPOND;
        osal_start_timerEx(CR_TaskId, CR_UART_WRITE_EVENT, MBUS_FRAME_INTERVAL);
        osal_start_timerEx(CR_TaskId, CR_BACKTO_NORMAL_EVENT, 60000);
        break;
    }

    default:
        break;
    }

}

/**************************************************************************************************
*
* @fn      CR_HandleKeys
*
* @brief   Callback service for keys
*
* @param   keys  - keys that were pressed
*          state - shifted
*
* @return  void
*
**************************************************************************************************/
void CR_HandleKeys(uint16 keys, uint_8 shift)
{
}


void CR_ReadDevInfo()
{
#if 0
    /* Make a fake DEV Info*/
    CR_DevInfo.ExitAddr[0] = 0x00;
    CR_DevInfo.ExitAddr[1] = 0xFF;
    CR_DevInfo.ExitAddr[2] = 0xFF;
    CR_DevInfo.ExitAddr[3] = 0xFF;
    CR_DevInfo.ExitAddr[4] = 0x00;
    CR_DevInfo.ExitAddr[5] = 0x00;
    CR_DevInfo.ExitAddr[6] = 0xFF;
    CR_DevInfo.ExitAddr[7] = 0x00;
#else

    uint8 aExtendedAddress[8];

    // Initialize NV System
    osal_nv_init(NULL);

    // Initialize extended address in NV
    osal_nv_item_init(ZCD_NV_EXTADDR, Z_EXTADDR_LEN, NULL);
    osal_nv_read(ZCD_NV_EXTADDR, 0, Z_EXTADDR_LEN, &aExtendedAddress);

    Dev_Info_t* p = (Dev_Info_t *)(&aExtendedAddress);
    CR_DevInfo = *p;

    CR_PanId = BUILD_UINT16(CR_DevInfo.ExitAddr[EXT_MACADDR_DEVID_LBYTE], CR_DevInfo.ExitAddr[EXT_MACADDR_DEVID_HBYTE]);
    HAL_ASSERT(CR_PanId  > 30000 && CR_PanId  < 30255);
    CR_DeviceID = CR_PanId - 30000;

    HAL_ASSERT(CR_DevInfo.ExitAddr[EXT_MACADDR_TYPE] == EXT_MACADDR_TYPE_CARDREADER);

#endif

}

/**************************************************************************************************
**************************************************************************************************/

