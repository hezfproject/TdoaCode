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
#include "..\common\2g\app_protocol.h"
#include "config.h"

/* OS includes */
#include "OSAL.h"
#include "OSAL_Tasks.h"

#include "delay.h"

//#include "OSAL_Nv.h"

/* App Protocol*/
//#include "App_cfg.h"
//#include "..\..\..\..\mine_project\util\App_cfg.h"

/* Application Includes */
#include "OnBoard.h"

/* MAC Application Interface */
#include "mac_api.h"
#include "mac_main.h"
#include "mac_pib.h"
#include "ZComDef.h"
/* Application */
#include "ExciteReader.h"

/* watchdog util */
#include "watchdogutil.h"
#include "MacUtil.h"
#include "crc.h"

/* FLASH */
#include "Hal_flash.h"



#include "app_protocol.h"
#include "config.h"


/*********************************************************************
* TYPEDEFS
*/

typedef struct
{
    /* device information */
    uint16 PanId;
    uint16 ShortAddr;
    uint8 Channel;
    sAddrExt_t extAddr;

    /* retreat */
    uint8 retreat_flag;
    uint8 retreat_cnt;
} CR_DevInfo_t;

/* Size table for MAC structures */
const CODE uint8 CR_cbackSizeTable[] =
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
*                                           Macros
**************************************************************************************************/
// Definations of EXT MAC ADDRESS:

// BITS:      7            6              5         4                3                      2                     1                   0
//         undefined  channel      TYPE    Version     Customer(High)    Customer(Low)  DeviceID(High)  DeviceID(Low)

// Extend mac Address Definations, what per byte stand for
#define EXT_MACADDR_DEVID_LBYTE                      0
#define EXT_MACADDR_DEVID_HBYTE                      1
#define EXT_MACADDR_CUSTORMERID_LBYTE            2
#define EXT_MACADDR_CUSTORMERID_HBYTE          3
#define EXT_MACADDR_VERSION                              4
#define EXT_MACADDR_TYPE                                   5
    //#ifdef MENU_RF_DEBUG
#define EXT_MACADDR_CHANNEL                             6


//#if (MBUS_BAUDRATE == 115200)
#define CR_APP_BAUD  HAL_UART_BR_115200
//#endif

//#if (MBUS_BAUDRATE == 9600)
//#define CR_APP_BAUD  HAL_UART_BR_9600
//#endif

#if !defined( CR_APP_RX_MAX )
/* The generic safe Rx minimum is 48, but if you know your PC App will not
* continue to send more than a byte after receiving the ~CTS, lower max
* here and safe min in _hal_uart.c to just 8.
*/
#define CR_APP_RX_MAX  50
#endif

// When the Rx buf space is less than this threshold, invoke the Rx callback.
#if !defined( CR_APP_THRESH )
#define CR_APP_THRESH  48
#endif

#if !defined( CR_APP_TX_MAX )
#define CR_APP_TX_MAX  250
#endif

#if !defined(CR_CARD_BUF_SIZE)
#define CR_CARD_BUF_SIZE   400
#endif

// Millisecs of idle time after a byte is received before invoking Rx callback.
#if !defined( CR_APP_IDLE )
#define CR_APP_IDLE  8
#endif

#define CR_MAX_CARD_IN_PACKET ((MBUS_FRAME_MAX_LEN-sizeof(mbus_hdr_slv_t) - sizeof(mbus_tlv_t) - 2)/sizeof(mbus_card_t))

#define CR_LED_UART0      HAL_LED_1
#define CR_LED_UART1      HAL_LED_3
#define CR_LED_AIR         HAL_LED_2

#define SYNCSTATUS_COUNT    (15000)   //5*60*1000/20  5 min  for compile

#define CARD_NWK_ADDR                          0xFFF0
/**************************************************************************************************
*                                        Local Variables
**************************************************************************************************/

/* TRUE and FALSE value */
static bool CR_MACTrue = TRUE;
static bool CR_MACFalse = FALSE;

/* Task ID */
uint8 CR_TaskId;

/* Device Info from flash */
static CR_DevInfo_t CR_DevInfo;

static uint16 u16RssiNum = 0;
static app_tof_rssi_ts app_rssi_data;

static tof_station_status_te statusType = STATION_STATUS_NORMAL;

static uint16 syncstatus_count=0;
static uint8 uart_tx_buf[128];

typedef struct
{
    unsigned char preamble_H;
    unsigned char preamble_L;
    /* bit filed depends on endian
    unsigned char frame_type    :3;
    unsigned char ready              :1;
    unsigned char device_type   :2;
    unsigned char filler               :1; //filler
    unsigned char priority           :1;
    */
    unsigned char frame_control;
    unsigned char reserverd;
    unsigned char frame_count_H;
    unsigned char frame_count_L;
    unsigned char src_addr_H;
    unsigned char src_addr_L;
    unsigned char dst_addr_H;
    unsigned char dst_addr_L;
    unsigned char data_len_H;
    unsigned char data_len_L;
}bsmac_hdr_t;



typedef struct
{
	uint_8		type;
	uint_8		ttl;
	pan_addr_t	src;
	pan_addr_t 	dst;
	uint_16		len;
	/* The options start here */
}nwk_hdr_t;


static  bsmac_hdr_t *ph = (bsmac_hdr_t*)uart_tx_buf;
static  nwk_hdr_t *pnwk = ( nwk_hdr_t*)(((uint8*) uart_tx_buf) + sizeof(bsmac_hdr_t));


/**************************************************************************************************
*                                     Local Function Prototypes
**************************************************************************************************/
/* Setup routines */
static  void    init_uart(uint8  uartPort , uint8 baudRate);
static void     CR_Startup(void);
static void     CR_ReadDevInfo(void);
static void     CR_start_timerEx(uint8 taskID,
                                 uint16 event_id,
                                 uint16 timeout_value);
static void CR_ParseAirMessage(macCbackEvent_t *p);

static void vCheckCardStatus(uint16 u16ShortAddr, uint8 u8Status);
static void vReportCardRssi(void);
static void vWriteData2Stm(uint8* pbuf);
static void vReportCardExcite(uint16 u16ShortAddr, uint8 u8CardStatus,uint8 u8ExciteID);
static void vReportCardAlarm(uint16 u16ShortAddr, uint8 alarm_type);


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
void CR_Init(uint8 taskId)
{
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

    init_uart(HAL_UART_PORT_0,CR_APP_BAUD);


    /*init devinfo */
    CR_ReadDevInfo();
    app_rssi_data.app_tof_head.protocoltype = APP_PROTOCOL_TYPE_CARD;
    app_rssi_data.app_tof_head.msgtype = APP_TOF_MSG_RSSI;

    pnwk->type = NWK_DATA;
    pnwk->ttl = 1;
    pnwk->src = CR_DevInfo.PanId;

    ph->preamble_H = BSMAC_PREAMBLE_H;
    ph->preamble_L = BSMAC_PREAMBLE_L;
    BSMAC_SET_DEVICETYPE(ph->frame_control,BSMAC_DEVICE_TYPE_LOC);
    BSMAC_SET_RDY(ph->frame_control, 1);
    BSMAC_SET_FRAMETYPE(ph->frame_control, BSMAC_FRAME_TYPE_DATA);
    BSMAC_SET_PRIORITY(ph->frame_control, 1);

    ph->src_addr_H = 0xff;//(tBsMacHdl.mac_addr >> 8) & 0xff;
    ph->src_addr_L = 0xff;//(tBsMacHdl.mac_addr) & 0xff; // source mac address
    ph->dst_addr_H = 0;
    ph->dst_addr_L = 0;

    HalUARTInit();
    init_uart(HAL_UART_PORT_0,CR_APP_BAUD);
    //uint8 buf33[10] = {'0','1','2','3','4','5','6','7','8','9'};
    while(0)
    {
      //HalLedSet(HAL_LED_ALL, HAL_LED_MODE_ON);

      #if(defined WATCHDOG) && (WATCHDOG==TRUE)
      FeedWatchDog();
      #endif


          //HalUARTWrite(0,buf33,10);
      DelayMs(500);
      //HalLedSet(HAL_LED_ALL, HAL_LED_MODE_OFF);
      #if(defined WATCHDOG) && (WATCHDOG==TRUE)
      FeedWatchDog();
      #endif
      DelayMs(500);
    }


    //phdr->sync[0] = 'Y';
    //phdr->sync[1] = 'I';
    //phdr->sync[2] = 'R';
    //phdr->slv_id = CR_DevInfo.PanId;


    /* turn off leds */
    //HalLedSet(CR_LED_UART0, HAL_LED_MODE_OFF);
    //HalLedSet(CR_LED_UART1, HAL_LED_MODE_OFF);
    //HalLedSet(CR_LED_AIR, HAL_LED_MODE_OFF);



    /* start up*/
    CR_Startup();

    CR_start_timerEx(CR_TaskId, CR_CHECKV2_RETREAT_EVENT, 20);
    CR_start_timerEx(CR_TaskId, CR_SEND_RSSI_EVENT, 1000);
    CR_start_timerEx(CR_TaskId, CR_LED_CONTROL_EVENT, 2000);
}


static void vReportCardRssi(void)
{
    if(0 < u16RssiNum)
    {
        app_rssi_data.app_tof_head.len = u16RssiNum*sizeof(tof_rssi_ts);

        //uart_send
        //HalUARTWrite(HAL_UART_PORT_0, (uint8*)(&cardMsgBuf[cardBufSendIndex]), tmpCount * sizeof(mbus_card_t));
        vWriteData2Stm((uint8*)&app_rssi_data);
        u16RssiNum = 0;
    }
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
    uartConfig.flowControlThreshold = CR_APP_THRESH;
    uartConfig.rx.maxBufSize        = CR_APP_RX_MAX;
    uartConfig.tx.maxBufSize        = CR_APP_TX_MAX;
    uartConfig.idleTimeout          = CR_APP_IDLE;
    uartConfig.intEnable            = TRUE;
    uartConfig.callBackFunc         = NULL;

    /*two ports. one for backup*/
    HalUARTOpen(uartPort, &uartConfig);

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
uint16 CR_ProcessEvent(uint8 taskId, uint16 events)
{
    uint8* pMsg;
    macCbackEvent_t* pData;

#if(defined WATCHDOG) && (WATCHDOG==TRUE)
    if(events & CR_FEEDWATCHDOG_EVENT)
    {
        osal_start_timerEx(CR_TaskId, CR_FEEDWATCHDOG_EVENT, 300);
        FeedWatchDog();
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
                osal_msg_deallocate((uint8 *) pData->dataCnf.pDataReq);
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
            osal_msg_deallocate((uint8 *) pMsg);
        }

        return events ^ SYS_EVENT_MSG;
    }

    if(events & CR_LED_CONTROL_EVENT)
    {
        HalLedSet(CR_LED_AIR, HAL_LED_MODE_OFF);
        HalLedSet(CR_LED_UART0, HAL_LED_MODE_OFF);
        HalLedSet(CR_LED_UART1, HAL_LED_MODE_OFF);
        osal_start_timerEx(taskId, CR_LED_CONTROL_EVENT, 1000);
        return events ^ CR_LED_CONTROL_EVENT;
    }

    //for G2 card
    if(events & CR_CHECKV2_RETREAT_EVENT)
    {
        /* sent station status to card when retreat  or receve mismatching status
            1.alawys down sent when  retreat ,but sent two minute when receve mismatching status  */
        if(statusType == STATION_STATUS_RETREAT||syncstatus_count)
        {
            if(syncstatus_count)
            {
                syncstatus_count--;
            }

            RfTofWrapper_tu RfTofData;
            RfTofData.tof_head.protocoltype = APP_PROTOCOL_TYPE_CARD;
            RfTofData.tof_head.msgtype = TOF_STATION_RSSI;
            //5148 is little-endian
            RfTofData.tof_head.len = (((sizeof(rf_tof_station_signal_ts)-sizeof(app_header_t))&0xff)<<8)|(((sizeof(rf_tof_station_signal_ts)-sizeof(app_header_t))&0xff00)>>8);//0x0600;//len = 6

            RfTofData.rf_tof_station_signal.u8AvailableMs = 0;    // card will send request in random (0 ~ u8Val) ms
            RfTofData.rf_tof_station_signal.u8StationStatus = (statusType ==STATION_STATUS_RETREAT)? STATION_STATUS_RETREAT : STATION_STATUS_NORMAL;
            RfTofData.rf_tof_station_signal.u16CurSlot = 0;
            RfTofData.rf_tof_station_signal.u8RunMs = 0;
            RfTofData.rf_tof_station_signal.u8LocIdle = 0;

            MAC_UTIL_SendDataPAN(CARD_NWK_ADDR, (uint8 *) &RfTofData,
                                sizeof(rf_tof_station_signal_ts),
                                MAC_UTIL_UNICAST, 0xFFFF,
                                MAC_TXOPTION_NO_CNF);
        }
        osal_start_timerEx(taskId, CR_CHECKV2_RETREAT_EVENT, 20);
        return events ^ CR_CHECKV2_RETREAT_EVENT;
    }

    if(events & CR_SEND_RSSI_EVENT)
    {
        vReportCardRssi();
        //char buf[10]={'0','1','2','3','4','5','6','7','8','9'};

        //HalUARTWrite(HAL_UART_PORT_0,buf,10);

        osal_start_timerEx(taskId, CR_SEND_RSSI_EVENT, 100);
        return events ^ CR_SEND_RSSI_EVENT;
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

    uint8 len = CR_cbackSizeTable[pData->hdr.event];

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
                                             ((uint8 *) pMsg
                                              + sizeof(macMlmeBeaconNotifyInd_t));
            osal_memcpy(pMsg->beaconNotifyInd.pPanDesc,
                        pData->beaconNotifyInd.pPanDesc,
                        sizeof(macPanDesc_t));
            pMsg->beaconNotifyInd.pSdu = (uint8 *)
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
        if (SUCCESS != osal_msg_send(CR_TaskId, (uint8 *) pMsg))
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
uint8 MAC_CbackCheckPending(void)
{
    return (0);
}


/**************************************************************************************************
*
* @fn      CR_Startup()
*
* @brief   Update the timer per tick
*
* @param   beaconEnable: TRUE/FALSE
*
* @return  None
*
**************************************************************************************************/
void CR_Startup()
{
    macMlmeStartReq_t startReq;

    /* Setup MAC_EXTENDED_ADDRESS */
    MAC_MlmeSetReq(MAC_EXTENDED_ADDRESS, &CR_DevInfo.extAddr);

    /* Setup MAC_SHORT_ADDRESS */
    CR_DevInfo.ShortAddr = 0;
    MAC_MlmeSetReq(MAC_SHORT_ADDRESS, &CR_DevInfo.ShortAddr);

    /* Setup MAC_BEACON_PAYLOAD_LENGTH */
    uint8 tmp8 = 0;
    MAC_MlmeSetReq(MAC_BEACON_PAYLOAD_LENGTH, &tmp8);

    /* Enable RX */
    MAC_MlmeSetReq(MAC_RX_ON_WHEN_IDLE, &CR_MACTrue);

    /* Setup MAC_ASSOCIATION_PERMIT */
    MAC_MlmeSetReq(MAC_ASSOCIATION_PERMIT, &CR_MACFalse);

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
    startReq.panId = CR_DevInfo.PanId;
    startReq.logicalChannel = CR_DevInfo.Channel;
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

void CR_ReadDevInfo(void)
{

    HalFlashRead(HAL_FLASH_IEEE_PAGE, HAL_FLASH_IEEE_OSET,
                 (uint8 *)&CR_DevInfo.extAddr, HAL_FLASH_IEEE_SIZE);

    /* DeviceID */
    CR_DevInfo.PanId = BUILD_UINT16(CR_DevInfo.extAddr[EXT_MACADDR_DEVID_LBYTE], CR_DevInfo.extAddr[EXT_MACADDR_DEVID_HBYTE]);
    HAL_ASSERT( CR_DevInfo.PanId  >= 30000 &&  CR_DevInfo.PanId  < 40000);

    //HAL_ASSERT(CR_DevInfo.extAddr[EXT_MACADDR_TYPE] == EXT_MACADDR_TYPE_CARDREADER_V2);

    /* Channel */
    uint8 channel = CR_DevInfo.extAddr[EXT_MACADDR_CHANNEL];

    if(!(channel>=11 && channel <=26))
    {
        channel = 0x19;   //use default channel 25;
    }

    CR_DevInfo.Channel =  channel;

    CR_DevInfo.ShortAddr = 0x00;
}

static void CR_start_timerEx(uint8 taskID,
                             uint16 event_id,
                             uint16 timeout_value)
{
    if (ZSuccess != osal_start_timerEx(taskID, event_id, timeout_value))
    {
        SystemReset();
    }
}

static void vWriteData2Stm(uint8* pbuf)
{
    app_header_t* pAppHead = (app_header_t*) pbuf;
    //uint16 u16App_len = pAppHead->len;

    switch (pAppHead->msgtype)
    {
    case APP_TOF_MSG_RSSI:
    {
        app_tof_rssi_ts* pRssi_data = (app_tof_rssi_ts*)pbuf;
        //uint16 u16CardLen = u16App_len / sizeof(tof_rssi_ts);
        asm("nop");
        break;
    }

    case APP_TOF_MSG_ALARM:
    case APP_TOF_MSG_NEW_ALARM:
    {
        app_tof_alarm_ts* pAlarm_data = (app_tof_alarm_ts*) pbuf;
        break;
    }

    default:
        break;
    }
    pnwk->len = pAppHead->len + sizeof(app_header_t);
    pnwk->dst = 0xffff;

    static uint16 tx_frame_cnt;
    ph->frame_count_H = (tx_frame_cnt & 0xff00) >> 8; // framecnt_h
    ph->frame_count_L = tx_frame_cnt & 0xff; // framecnt_l
    tx_frame_cnt++;

    unsigned short tx_len = pnwk->len + sizeof(nwk_hdr_t) + BSMAC_FOOTER_LEN; // length = payload+footer
    ph->data_len_H = (tx_len >> 8) & 0xff; //
    ph->data_len_L = tx_len & 0xff; //

    osal_memcpy((void*) (uart_tx_buf + BSMAC_HEADER_LEN +sizeof(nwk_hdr_t)), pbuf, pnwk->len);

    unsigned short crc = CRC16((unsigned char *)(uart_tx_buf+2), pnwk->len + sizeof(nwk_hdr_t)+BSMAC_HEADER_LEN-2, 0xffff);
    uart_tx_buf[pnwk->len + sizeof(nwk_hdr_t)+BSMAC_HEADER_LEN] = (crc >> 8) & 0xff;
    uart_tx_buf[pnwk->len + sizeof(nwk_hdr_t)+BSMAC_HEADER_LEN+1] = crc & 0xff;

    HalUARTWrite(HAL_UART_PORT_0, (uint8*)uart_tx_buf,tx_len + BSMAC_HEADER_LEN);
}


void CR_ParseAirMessage(macCbackEvent_t *p)
{
    uint16 srcAddr = p->dataInd.mac.srcAddr.addr.shortAddr;

    //HAL_TOGGLE_LED3();

    RfTofWrapper_tu* pRfTofWrapperTu = (RfTofWrapper_tu*)(p->dataInd.msdu.p);
    uint16 v2Len = ((pRfTofWrapperTu->tof_head.len&0xff)<<8)|(pRfTofWrapperTu->tof_head.len>>8);

    if(pRfTofWrapperTu->tof_head.protocoltype == APP_PROTOCOL_TYPE_CARD)

    {
        if(v2Len != (sizeof(rf_tof_card_data_ts)- sizeof(app_header_t)))
            return;

        if(pRfTofWrapperTu->tof_head.msgtype == TOF_CARD_RSSI)
        {
            vCheckCardStatus(srcAddr, pRfTofWrapperTu->rf_tof_card_data.u8CardStatus & (~CARD_STATUS_EXCITER));
            uint16 seqnum = ((pRfTofWrapperTu->rf_tof_card_data.u16SeqNum&0x00ff)<<8)|(pRfTofWrapperTu->rf_tof_card_data.u16SeqNum>>8);
            app_rssi_data.tof_rssi[u16RssiNum].u16ShortAddr = srcAddr;
            app_rssi_data.tof_rssi[u16RssiNum].u16SeqNum = seqnum;
            app_rssi_data.tof_rssi[u16RssiNum].i8Rssi = p->dataInd.mac.rssi;
            app_rssi_data.tof_rssi[u16RssiNum].u8RssiType = APP_TOF_CARD_RSSI;
            app_rssi_data.tof_rssi[u16RssiNum].u8Reserved = 0;
            app_rssi_data.tof_rssi[u16RssiNum].u8Accel = pRfTofWrapperTu->rf_tof_card_data.u8ExciterIDorAccStatus;
            u16RssiNum++;
            if(APP_MAX_CARD_NUM == u16RssiNum)
            {
                vReportCardRssi();
                CR_start_timerEx(CR_TaskId, CR_SEND_RSSI_EVENT, 200);
                //TimerUtil_eSetTimer(RSSI_REPORT_DATA_EVENT, 500);
                //EventUtil_vUnsetEvent(RSSI_REPORT_DATA_EVENT);
            }
        }
        else if(pRfTofWrapperTu->tof_head.msgtype == TOF_CARD_ALARM)
        {
            vReportCardExcite(srcAddr,pRfTofWrapperTu->rf_tof_card_data.u8CardStatus,\
                               pRfTofWrapperTu->rf_tof_card_data.u8ExciterIDorAccStatus);
        }
    }
    //HAL_TOGGLE_LED3();

}


/****************************************************************************
 *
 * NAME: CR_CheckCardStatus
 *
 * DESCRIPTION:
 * Check the certain card's status: help, nopwd or normal
 *
 * PARAMETERS:
 *                u16ShortAddr - the checked card's addr
 *                 u8Status - the card's status bitmap
 *
 * RETURNS:
 *         void
 *
 ****************************************************************************/
void vCheckCardStatus(uint16 u16ShortAddr, uint8 u8Status)
{
    uint8 teStatus = APP_TOF_ALARM_NONE;

    //teStatus = u8Status;
    if((u8Status & CARD_STATUS_HELP) || (u8Status & CARD_STATUS_NOPWD)
        || (u8Status & CARD_STATUS_RETREAT_ACK) || (u8Status & CARD_STATUS_EXCITER))
    {
        //teStatus = APP_TOF_ALARM_CARD_HELP;
        teStatus = u8Status;
    }
    else
    {
        return;
    }
    //HalUARTWrite(HAL_UART_PORT_0, (uint8*)(&cardMsgBuf[cardBufSendIndex]), tmpCount * sizeof(mbus_card_t));
    vReportCardAlarm(u16ShortAddr, teStatus);
}

static void vReportCardExcite(uint16 u16ShortAddr, uint8 u8CardStatus,uint8 u8ExciteID)
{
    app_tof_alarm_ts app_tof_alarm_data;

    app_tof_alarm_data.app_tof_head.protocoltype = APP_PROTOCOL_TYPE_CARD;
    app_tof_alarm_data.app_tof_head.msgtype = APP_TOF_MSG_NEW_ALARM;
    app_tof_alarm_data.app_tof_head.len = 4;
    app_tof_alarm_data.u16ShortAddr = u16ShortAddr;
    app_tof_alarm_data.u8Status = u8CardStatus;
    app_tof_alarm_data.u8ExciterID = u8ExciteID;

    //uart_send
    vWriteData2Stm((uint8*)&app_tof_alarm_data);
    //HalUARTWrite(HAL_UART_PORT_0, (uint8*)(&cardMsgBuf[cardBufSendIndex]), tmpCount * sizeof(mbus_card_t));
}

static void vReportCardAlarm(uint16 u16ShortAddr, uint8 alarm_type)
{
    app_tof_alarm_ts app_tof_alarm_data;

    app_tof_alarm_data.app_tof_head.protocoltype = APP_PROTOCOL_TYPE_CARD;
    app_tof_alarm_data.app_tof_head.msgtype = APP_TOF_MSG_NEW_ALARM;
    app_tof_alarm_data.app_tof_head.len = 4;
    app_tof_alarm_data.u16ShortAddr = u16ShortAddr;
    app_tof_alarm_data.u8Status = alarm_type;

    vWriteData2Stm((uint8*)&app_tof_alarm_data);
}





/**************************************************************************************************
**************************************************************************************************/

