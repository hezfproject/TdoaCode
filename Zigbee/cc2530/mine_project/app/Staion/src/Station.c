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
/* Application */
#include "Station.h"
#include "AppProtocol.h"
#include "AppProtocolWrapper.h"

/* watchdog util */
#include "watchdogutil.h"
#include "MacUtil.h"
#include "crc.h"

/**************************************************************************************************
*                                           Macros
**************************************************************************************************/
#define STATION_PARAM_ADDR                     0x1EF0  //param space 1EF0-1EFF

#define STATION_RETREAT_NORMAL                 0
#define STATION_RETREAT_RETREAT                0x5A
#define STATION_RETREAT_CANCELRETREAT    0x68

#define STATION_LED_BLUE    HAL_LED_1
#define STATION_LED_RED      HAL_LED_2

/*********************************************************************
* TYPEDEFS
*/

typedef enum
{
    E_STATION_INIT          = 0,
    E_STATION_STARTED
} eStationState;


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


/* Size table for MAC structures */
const CODE uint_8 CR_cbackSizeTable[] =
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
static bool Station_MACTrue = TRUE;
static bool Station_MACFalse = FALSE;

/* Task ID */
uint8 Station_TaskId;

/* Device Info from flash */
static Station_DevInfo_t Station_DevInfo;

/* buf send on uart  */
static uint8* Station_SpiBuf;
static SPIMSGPacket_t* pSpi;    // point to spi buffer
static uint8* Station_SpiReadBuf;

/* rfmac status */
static bool rfmac_setted = false;

/*spi */
static uint8 spi_errcnt;



/**************************************************************************************************
*                                     Local Function Prototypes
**************************************************************************************************/
/* Setup routines */
static void     Station_Startup(void);

static void     Station_start_timerEx(uint8 taskID,
                                      uint16 event_id,
                                      uint16 timeout_value);
static uint8    Station_SendDataToSpi(uint16 shortAddr,
                                      const uint8* p,
                                      uint16 len);
static uint8    Station_SendDataToSpiEx(uint16 shortAddr, uint16 len);

static void     Station_ParseDataCnf(const macMcpsDataCnf_t* pdatacnf);
static void     Station_ParseFrame(const macMcpsDataInd_t* pdataInd);
static uint8    Station_ParseCardFrame(const sAddr_t* sAddr,
                                       uint16 srcPanId,
                                       const sData_t msdu,
                                       int8 rssi);
static uint8    Station_ParseChargeCardFrame(const sAddr_t* sAddr,
        uint16 srcPanId,
        const sData_t msdu,
        int8 rssi);
static uint8    Station_ParseGasMonitorFrame(const sAddr_t* sAddr,
        uint16 srcPanId,
        const sData_t msdu,
        int8 rssi);
static uint8    Station_ParseMobileFrame(const sAddr_t* sAddr,
        uint16 srcPanId,
        const sData_t msdu,
        int8 rssi);

static void     Station_ProcessSPICB(const  SPIMSGPacket_t* rxmsg);
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
    Station_DevInfo.PanId = 0xFFFF;
    Station_DevInfo.ShortAddr = 0x00;
    Station_DevInfo.Channel = 0x0B;
    Station_DevInfo.State = E_STATION_INIT;
    Station_DevInfo.retreat_flag = STATION_RETREAT_NORMAL;
    Station_DevInfo.retreat_cnt = 0;

    /* turn off leds */
    HalLedSet(STATION_LED_RED, HAL_LED_MODE_OFF);
    HalLedSet(STATION_LED_BLUE, HAL_LED_MODE_OFF);

    /* initial spi */
    halSPICfg_t spiConfig;
    spiConfig.bufSize = DMA_RT_MAX;
    spiConfig.taskID = Station_TaskId;
    spiConfig.eventID = STATION_SPI_READ_EVENT;
    HalSpiStart(&spiConfig);

    Station_SpiReadBuf = (uint8 *) osal_mem_alloc(MINEAPP_MAX_DATA_LEN + 10);
    Station_SpiBuf = (uint8 *) osal_mem_alloc(MINEAPP_MAX_DATA_LEN + 10);
    pSpi = (SPIMSGPacket_t *) Station_SpiBuf;
    pSpi->spihdr.hdr.event = SPI_RX_MSG;                                  // useless, just the same to v1.0
    pSpi->spihdr.srcAddr.addrMode = (AddrMode_t) Addr16Bit;
    pSpi->spihdr.srcAddr.endPoint = MINEAPP_ENDPOINT;
    pSpi->spihdr.dstAddr.addrMode = (AddrMode_t) Addr16Bit;
    pSpi->spihdr.dstAddr.endPoint = MINEAPP_ENDPOINT;
    pSpi->spihdr.transID = INIT_TRANID;
    pSpi->spihdr.options = INIT_OPN;
    pSpi->spihdr.radius = 1;

    /* initial macutil */
    MacUtil_t MacUtil;
    MacUtil.panID = CARD_NWK_ADDR;  /* default is card nwk id */
    MacUtil.dst_endpoint = MINEAPP_ENDPOINT;
    MacUtil.src_endpoint = 0x20;//MINEAPP_ENDPOINT;
    MacUtil.profile_id = 0x0100;//MINEAPP_PROFID;
    MacUtil.cluster_id = CARD_CLUSTERID;
    MacUtil.NodeType = NODETYPE_COORDINATOR;
    MAC_UTIL_INIT(&MacUtil);

    /* start up*/
    Station_start_timerEx(Station_TaskId, STATION_RFMAC_EVENT, 1);
    Station_start_timerEx(Station_TaskId, STATION_LOC_BLAST_EVENT, 10);
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
                    HalLedSet(STATION_LED_RED, HAL_LED_MODE_ON);

                    /* send power on */
                    app_startNwk_t app_startNwk;
                    app_startNwk.msgtype = ZB_START_NETWORK;
                    app_startNwk.PANID = Station_DevInfo.PanId;
                    osal_cpyExtAddr((uint8 *) app_startNwk.macAddr,
                                    (uint8 *) Station_DevInfo.extAddr);
                    Station_SendDataToSpi(0,
                                          (uint8 *) &app_startNwk,
                                          sizeof(app_startNwk));

                    /* detect spi */
                    Station_start_timerEx(Station_TaskId,
                                          STATION_DETECT_BLAST_EVENT,
                                          5);

                    /* report restart status */
                    Station_start_timerEx(Station_TaskId,
                                          STATION_STATUS_REPORT_EVENT,
                                          10);
                }
                else
                {
                    Station_DevInfo.State = E_STATION_INIT;
                    HalLedSet(STATION_LED_RED, HAL_LED_MODE_OFF);

                    /* set restart reason and restart */
                    storeParam_t param;
                    param.ResetFlag = ZC_REPORT_STARTNWK_FAILED_RESTART;
                    *(storeParam_t *) STATION_PARAM_ADDR = param;
                    SystemReset();
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

#if (defined HAL_SPI) && (HAL_SPI == TRUE)
            case SPI_RX_MSG:
            {
                Station_ProcessSPICB(( SPIMSGPacket_t * ) pMsg );
                break;
            }
#endif
            }

            /* Deallocate */
            mac_msg_deallocate((uint8 * *) &pMsg);
        }

        return events ^ SYS_EVENT_MSG;
    }
#if 0
    if (events & STATION_SPI_READ_EVENT)
    {
        uint16 len;
        if (HalSPIRead(Station_SpiReadBuf, &len) == SUCCESS)
        {
            Station_ProcessSPICB((SPIMSGPacket_t *) Station_SpiReadBuf);
        }
        return events ^ STATION_SPI_READ_EVENT;
    }
#endif
    if (events & STATION_RFMAC_EVENT)
    {
        static uint8 rfmac_seqnum;
        app_rfmac_query_t rfmac_query;
        rfmac_query.msgtype = RFMAC_QUERY;
        rfmac_query.seq = rfmac_seqnum++;

        if (rfmac_setted)
        {
            Station_Startup();
        }
        else
        {
            HalLedSet(STATION_LED_RED, HAL_LED_MODE_TOGGLE);
            Station_SendDataToSpi(0, (uint8 *) &rfmac_query, sizeof(rfmac_query));
            osal_start_timerEx(Station_TaskId, STATION_RFMAC_EVENT, 1000);
        }
        return events ^ STATION_RFMAC_EVENT;
    }

    if (events & STATION_STATUS_REPORT_EVENT)
    {
        app_ZC_Report_t zc_report;
        zc_report.msgtype = ZC_REPORT;
        zc_report.PanId = Station_DevInfo.PanId;

        /* get reset flag and send report */
        storeParam_t param = *(storeParam_t*) STATION_PARAM_ADDR;
        switch (ResetReason())
        {
        case RESET_FLAG_WATCHDOG:
        {
            zc_report.flag = param.ResetFlag;

            break;
        }
        case RESET_FLAG_EXTERNAL:
        {
            zc_report.flag = ZC_REPORT_EXTERNAL_RESTART;
            break;
        }
        case RESET_FLAG_POWERON:
        {
            zc_report.flag = ZC_REPORT_POWERON;
            break;
        }
        default:
            break;
        }

        Station_SendDataToSpi(0, (uint8 *) &zc_report, sizeof(zc_report));

        /* set  reset param to watchdog */
        param.ResetFlag = ZC_REPORT_WATCHDOG_RESTART;
        *(storeParam_t *) STATION_PARAM_ADDR = param;

        return events ^ STATION_STATUS_REPORT_EVENT;
    }

    if (events & STATION_DETECT_BLAST_EVENT)
    {
        Station_start_timerEx(Station_TaskId, STATION_DETECT_BLAST_EVENT, 6000);

        app_SPIDetect_t spi_report;
        spi_report.msgtype = SPI_DETECT;
        for (uint8 i = 0; i < SPIDETECT_LEN; i++)
        {
            spi_report.detectdata[i] = i;
        }
        Station_SendDataToSpi(0, (uint8 *) &spi_report, sizeof(spi_report));

        /* time out: 6s*5=30s */
        if (spi_errcnt++ > 5)
        {
            storeParam_t param;
            param.ResetFlag = ZC_REPORT_SPI_RESTART;
            *(storeParam_t *) STATION_PARAM_ADDR = param;
            SystemReset();
        }
        return events ^ STATION_DETECT_BLAST_EVENT;
    }
    if (events & STATION_URGENT_TIME_EVENT)
    {
        if (Station_DevInfo.retreat_cnt < 15)
        {
            osal_start_timerEx(Station_TaskId, STATION_URGENT_TIME_EVENT, 60000);    //delay for a minute
            Station_DevInfo.retreat_cnt++;
        }
        else
        {
            Station_DevInfo.retreat_flag = STATION_RETREAT_NORMAL;
            Station_DevInfo.retreat_cnt = 0;
        }
        return (events ^ STATION_URGENT_TIME_EVENT);
    }

    if (events & STATION_LOC_BLAST_EVENT)
    {
        Station_start_timerEx(Station_TaskId, STATION_LOC_BLAST_EVENT, 2500);

        if (Station_DevInfo.State == E_STATION_STARTED)
        {
            static uint8 locnode_seq;
            app_LocNodeCast_t app_LocNodeCast;
            app_LocNodeCast.msgtype = LOCNODE_CAST;
            app_LocNodeCast.vol = 0xFF;
            app_LocNodeCast.seq = locnode_seq++;
            app_LocNodeCast.DevID = Station_DevInfo.PanId;
            MacParam_t param;
            param.panID = 0xFFFF;
            param.cluster_id = GASMONITOR_CLUSTERID;
            param.radius = 0x01;
            MAC_UTIL_BuildandSendDataPAN(&param,
                                         (uint8 *) &app_LocNodeCast,
                                         sizeof(app_LocNodeCast_t),
                                         MAC_UTIL_BROADCAST,
                                         MAC_UTIL_BROADCAST_SHORTADDR_DEVZCZR,
                                         0);
        }
        return events ^ STATION_LOC_BLAST_EVENT;
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

    uint_8 len = CR_cbackSizeTable[pData->hdr.event];

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
    Station_DevInfo.ShortAddr = 0;
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

/* copy data and send to SPI */
uint8 Station_SendDataToSpi(uint16 shortAddr, const uint8* p, uint16 len)
{
    const uint8 maxDataLen = MINEAPP_MAX_DATA_LEN - sizeof(SPIMSGPacket_t);

    if (len > maxDataLen)
    {
        return FAILURE;
    }
    pSpi->spihdr.srcAddr.addr.shortAddr = shortAddr;
    pSpi->spihdr.dstAddr.addr.shortAddr = 0;
    pSpi->DataLength = len;
    osal_memcpy((void *) (pSpi + 1), (void *) p, pSpi->DataLength);
    return HalSPIWrite((uint8 *) pSpi, pSpi ->DataLength + SPIPKTHDR_LEN);
}

/* data is prepared, just send to SPI */
uint8 Station_SendDataToSpiEx(uint16 shortAddr, uint16 len)
{
    const uint8 maxDataLen = MINEAPP_MAX_DATA_LEN - sizeof(SPIMSGPacket_t);
    if (len > maxDataLen)
    {
        return FAILURE;
    }
    pSpi->spihdr.srcAddr.addr.shortAddr = shortAddr;
    pSpi->DataLength = len;
    return HalSPIWrite((uint8 *) pSpi, pSpi ->DataLength + SPIPKTHDR_LEN);
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
            storeParam_t param;

            uint8* p1, * p2;
            p1 = osal_mem_alloc(8);    //small size memory
            p2 = osal_mem_alloc(64);    //big size memory
            if (p1 == NULL || p2 == NULL)
            {
                param.ResetFlag = ZC_REPORT_MEMORY_ERR_RESTART;
            }
            else
            {
                param.ResetFlag = ZC_REPORT_BLAST_ERR_RESTART;
            }
            *(storeParam_t *) STATION_PARAM_ADDR = param;
            osal_mem_free(p1);
            osal_mem_free(p2);

            SystemReset();
        }
    }
}

static void Station_ParseFrame(const macMcpsDataInd_t* pdataInd)
{
    if ((pdataInd == NULL)
            || (pdataInd->msdu.p == NULL)
            || (pdataInd->msdu.len <= MAC_UTIL_ZIGBEEHDR_LEN) // min length is zigbee header + MSGType
            || (pdataInd->msdu.len > MAC_MAX_FRAME_SIZE))
    {
        return;
    }

    uint8 clusterid = MAC_UTIL_GetClusterID(pdataInd->msdu);
    sData_t appdata = MAC_UTIL_RemoveHeader(pdataInd->msdu);
    int8 rssi = pdataInd->mac.rssi;

    HalLedSet(STATION_LED_BLUE, HAL_LED_MODE_TOGGLE);

    switch (clusterid)
    {
    case BLAST_CLUSTERID:
    {
        app_BlastRequest_t *pstBlast = (app_BlastRequest_t*)(appdata.p);
        pstBlast->rssi = rssi;
        Station_SendDataToSpi(pdataInd->mac.srcAddr.addr.shortAddr,
                              appdata.p,
                              appdata.len);
        break;
    }
    case CARD_CLUSTERID:
    {
        Station_ParseCardFrame(&pdataInd->mac.srcAddr,
                               pdataInd->mac.srcPanId,
                               appdata,
                               pdataInd->mac.rssi);
        break;
    }
    case CHARGEED_CLUSTERID:
    {
        Station_ParseChargeCardFrame(&pdataInd->mac.srcAddr,
                                     pdataInd->mac.srcPanId,
                                     appdata,
                                     pdataInd->mac.rssi);
        break;
    }
    case MINEAPP_CLUSTERID:
    {
        Station_ParseMobileFrame(&pdataInd->mac.srcAddr,
                                 pdataInd->mac.srcPanId,
                                 appdata,
                                 pdataInd->mac.rssi);
        break;
    }
    case GASMONITOR_CLUSTERID:
    {
        Station_ParseGasMonitorFrame(&pdataInd->mac.srcAddr,
                                     pdataInd->mac.srcPanId,
                                     appdata,
                                     pdataInd->mac.rssi);
        break;
    }
    }
}
static uint8 Station_ParseCardFrame(const sAddr_t* sAddr,
                                    uint16 srcPanId,
                                    const sData_t msdu,
                                    int8 rssi)
{
    uint8 ret;
    APPWrapper_t* AppPkt = (APPWrapper_t*) (msdu.p);

    MacParam_t param;
    param.panID = srcPanId;
    param.cluster_id = CARD_CLUSTERID;
    param.radius = 0x01;

    if (msdu.p == NULL || msdu.len == 0)
    {
        return FAILURE;
    }
    switch (AppPkt->app_flag)
    {
    case SSREQ:
    {
        if (msdu.len != sizeof(app_ssReq_t))
            return FAILURE;

        uint8 reqtype = AppPkt->app_ssReq.reqtype;

        /* If poll, Send data to card */
        if (reqtype == SSREQ_POLL)
        {
            if (Station_DevInfo.retreat_flag == STATION_RETREAT_NORMAL)
            {
                app_ssReq_t app_ssReq;
                app_ssReq.msgtype = SSREQ;
                app_ssReq.reqtype = SSREQ_NODATA;
                app_ssReq.NWK_ADDR = CARD_NWK_ADDR;
                MAC_UTIL_BuildandSendDataPAN(&param,
                                             (uint8 *) &app_ssReq,
                                             sizeof(app_ssReq_t),
                                             MAC_UTIL_BROADCAST,
                                             MAC_UTIL_BROADCAST_SHORTADDR_DEVALL,
                                             0);
            }
            else if (Station_DevInfo.retreat_flag
                     == STATION_RETREAT_RETREAT)
            {
                app_Urgent_t app_Urgent;
                app_Urgent.msgtype = URGENT;
                app_Urgent.urgenttype = RETREAT;
                app_Urgent.value = 0;
                MAC_UTIL_BuildandSendDataPAN(&param,
                                             (uint8 *) &app_Urgent,
                                             sizeof(app_Urgent),
                                             MAC_UTIL_BROADCAST,
                                             MAC_UTIL_BROADCAST_SHORTADDR_DEVALL,
                                             0);
            }
            else if ((Station_DevInfo.retreat_flag
                      == STATION_RETREAT_CANCELRETREAT))
            {
                app_Urgent_t app_Urgent;
                app_Urgent.msgtype = URGENT;
                app_Urgent.urgenttype = CANCELRETREAT;
                app_Urgent.value = 0;
                MAC_UTIL_BuildandSendDataPAN(&param,
                                             (uint8 *) &app_Urgent,
                                             sizeof(app_Urgent),
                                             MAC_UTIL_BROADCAST,
                                             MAC_UTIL_BROADCAST_SHORTADDR_DEVALL,
                                             0);
            }
        }

        /* If received a SSREQ_OUT or SSREQ_POLL , Send SSIND to SPI*/
        app_ssInd_t ssInd;
        ssInd.rssipkt.rssi = rssi;
        ssInd.rssipkt.NWK_ADDR = AppPkt->app_ssReq.NWK_ADDR;
        ssInd.rssipkt.NODE_ADDR = sAddr->addr.shortAddr;
        ssInd.rssipkt.seqnum = AppPkt->app_ssReq.seqnum;
        ssInd.msgtype = SSIND;

        ret = Station_SendDataToSpi(sAddr->addr.shortAddr,
                                    (uint8 *) &ssInd,
                                    sizeof(ssInd));

        break;
    }
    case URGENT:
    {
        if (msdu.len != sizeof(app_Urgent_t))
            return FAILURE;

        uint8 urgenttype = AppPkt->app_Urgent.urgenttype;
        if (urgenttype == ALERT)
        {
            app_Urgent_t app_Urgent;
            app_Urgent.msgtype = URGENT;
            app_Urgent.urgenttype = ALERTACK;
            app_Urgent.value = 0;
            MAC_UTIL_BuildandSendDataPAN(&param,
                                         (uint8 *) &app_Urgent,
                                         sizeof(app_Urgent_t),
                                         MAC_UTIL_UNICAST,
                                         sAddr->addr.shortAddr,
                                         0);
        }

        ret = Station_SendDataToSpi(sAddr->addr.shortAddr, msdu.p, msdu.len);
        break;
    }
    default:
    {
        ret = Station_SendDataToSpi(sAddr->addr.shortAddr, msdu.p, msdu.len);
        break;
    }
    }
    return ret;
}
static uint8 Station_ParseChargeCardFrame(const sAddr_t* sAddr,
        uint16 srcPanId,
        const sData_t msdu,
        int8 rssi)
{
    uint8 ret;
    APPWrapper_t* AppPkt = (APPWrapper_t*) (msdu.p);

    MacParam_t param;
    param.cluster_id = CHARGEED_CLUSTERID;
    param.panID = srcPanId;
    param.radius = 0x01;
    if (msdu.p == NULL || msdu.len == 0)
    {
        return FAILURE;
    }

    switch (AppPkt->app_flag)
    {
    case CHARGEED_SSREQ:
    {
        if (msdu.len != sizeof(app_chargeed_ssReq_t))
            return FAILURE;  // do not use locnode, so size is sizeof(app_chargeed_ssReq_t)

        uint8 reqtype = AppPkt->app_chargeed_ssReq.reqtype;
        if (reqtype == SSREQ_POLL)
        {
            app_chargeed_ssRsp_t app_chargeed_ssRsp;
            app_chargeed_ssRsp.msgtype = CHARGEED_SSRSP;
            app_chargeed_ssRsp.srcPan = Station_DevInfo.PanId;
            app_chargeed_ssRsp.locnode_num = 0;
            app_chargeed_ssRsp.seqnum = AppPkt->app_chargeed_ssReq.seqnum;

            if (Station_DevInfo.retreat_flag == STATION_RETREAT_RETREAT)
            {
                app_chargeed_ssRsp.urgent_type = RETREAT;
                app_chargeed_ssRsp.urgent_value = 0;
            }
            else if (Station_DevInfo.retreat_flag
                     == STATION_RETREAT_CANCELRETREAT)
            {
                app_chargeed_ssRsp.urgent_type = CANCELRETREAT;
                app_chargeed_ssRsp.urgent_value = 0;
            }
            else
            {
                app_chargeed_ssRsp.urgent_type = URGENT_NONE;
                app_chargeed_ssRsp.urgent_value = 0;
            }

            if (URGENT_NONE != app_chargeed_ssRsp.urgent_type)
            {
                MAC_UTIL_BuildandSendDataPAN(&param,
                                             (uint8 *) &app_chargeed_ssRsp,
                                             sizeof(app_chargeed_ssRsp_t),
                                             MAC_UTIL_BROADCAST,
                                             MAC_UTIL_BROADCAST_SHORTADDR_DEVALL,
                                             0);
            }
        }

        /* If received a SSREQ_OUT or SSREQ_POLL , Send SSIND to SPI*/
        app_chargeed_ssInd_t chargeed_ssInd;
        chargeed_ssInd.rssipkt.rssi = rssi;
        chargeed_ssInd.rssipkt.NWK_ADDR = AppPkt->app_chargeed_ssReq.srcPan;
        chargeed_ssInd.rssipkt.NODE_ADDR = sAddr->addr.shortAddr;
        chargeed_ssInd.rssipkt.seqnum = AppPkt->app_chargeed_ssReq.seqnum;
        chargeed_ssInd.msgtype = CHARGEED_SSIND;
        chargeed_ssInd.LocCnt = 0;

        ret = Station_SendDataToSpi(sAddr->addr.shortAddr,
                                    (uint8 *) &chargeed_ssInd,
                                    sizeof(chargeed_ssInd));
        break;
    }
    case URGENT:
    {
        if (msdu.len != sizeof(app_Urgent_t))
            return FAILURE;

        uint8 urgenttype = AppPkt->app_Urgent.urgenttype;
        if (urgenttype == ALERT)
        {
            app_Urgent_t app_Urgent;
            app_Urgent.msgtype = URGENT;
            app_Urgent.urgenttype = ALERTACK;
            app_Urgent.value = 0;

            MAC_UTIL_BuildandSendDataPAN(&param,
                                         (uint8 *) &app_Urgent,
                                         sizeof(app_Urgent),
                                         MAC_UTIL_UNICAST,
                                         sAddr->addr.shortAddr,
                                         0);
        }

        ret = Station_SendDataToSpi(sAddr->addr.shortAddr, msdu.p, msdu.len);
        break;
    }
    default:
    {
        ret = Station_SendDataToSpi(sAddr->addr.shortAddr, msdu.p, msdu.len);
        break;
    }
    }
    return ret;
}
static uint8 Station_ParseGasMonitorFrame(const sAddr_t* sAddr,
        uint16 srcPanId,
        const sData_t msdu,
        int8 rssi)
{
    uint8 ret;
    APPWrapper_t* AppPkt = (APPWrapper_t*) (msdu.p);

    MacParam_t param;
    param.panID = srcPanId;
    param.cluster_id = GASMONITOR_CLUSTERID;
    param.radius = 0x01;
    if (msdu.p == NULL || msdu.len == 0)
    {
        return FAILURE;
    }

    if (AppPkt->app_flag == GASALARM)
    {
        if (msdu.len != sizeof(app_GasAlarm_t))
            return FAILURE;

        app_GasAlarm_ack_t app_GasAlarm_ack;
        app_GasAlarm_ack.msgtype = GASALARM_ACK;
        app_GasAlarm_ack.DevID = AppPkt->app_GasAlarm.DevID;
        app_GasAlarm_ack.SrcPan = Station_DevInfo.PanId;
        app_GasAlarm_ack.seq = AppPkt->app_GasAlarm.seq;
        app_GasAlarm_ack.AlarmType = AppPkt->app_GasAlarm.AlarmType;
        MAC_UTIL_BuildandSendDataPAN(&param,
                                     (uint8 *) &app_GasAlarm_ack,
                                     sizeof(app_GasAlarm_ack),
                                     MAC_UTIL_UNICAST,
                                     sAddr->addr.shortAddr,
                                     MAC_TXOPTION_ACK);
    }

    /* do not send locnode_cast to arm */
    if (AppPkt->app_flag != LOCNODE_CAST)
    {
        ret = Station_SendDataToSpi(sAddr->addr.shortAddr, msdu.p, msdu.len);
    }

    return ret;
}
static uint8 Station_ParseMobileFrame(const sAddr_t* sAddr,
                                      uint16 srcPanId,
                                      const sData_t msdu,
                                      int8 rssi)
{
    uint8 ret;
    APPWrapper_t* AppPkt = (APPWrapper_t*) (msdu.p);

    MacParam_t param;
    param.panID = srcPanId;
    param.cluster_id = MINEAPP_CLUSTERID;
    param.radius = 0x01;
    if (msdu.p == NULL || msdu.len == 0)
    {
        return FAILURE;
    }

    if (AppPkt->app_flag == TIMAC_MP_SCAN)
    {
        if (msdu.len != sizeof(app_Timac_Mp_Scan_t))
            return FAILURE;

        app_Timac_Mp_Scan_t* pscanreq = (app_Timac_Mp_Scan_t*) (msdu.p);
        app_Timac_Mp_Scan_t mp_scan;
        mp_scan.msgtype = TIMAC_MP_SCAN;
        mp_scan.scantype = APP_SCAN_TYPE_ACK;
        mp_scan.seqnum = pscanreq->seqnum;

        ret = MAC_UTIL_BuildandSendDataPAN(&param,
                                           (uint8 *) &mp_scan,
                                           sizeof(mp_scan),
                                           MAC_UTIL_UNICAST,
                                           sAddr->addr.shortAddr,
                                           MAC_TXOPTION_ACK);
    }
    else
    {
        ret = Station_SendDataToSpi(sAddr->addr.shortAddr, msdu.p, msdu.len);
    }

    return ret;
}

static void Station_ProcessSPICB(const  SPIMSGPacket_t* rxmsg)
{
    if (rxmsg == NULL
            || (rxmsg->DataLength + sizeof(SPIMSGPacket_t)) > MINEAPP_MAX_DATA_LEN
            || rxmsg->spihdr.hdr.event != SPI_RX_MSG)
    {
        return;
    }

    APPWrapper_t* appwrapper = (APPWrapper_t*) (rxmsg + 1);

    HalLedSet(STATION_LED_BLUE, HAL_LED_MODE_TOGGLE);

    if(BLAST_REQUEST == appwrapper->app_flag)
    {
        MacParam_t param;
        app_BlastRequest_t *pstBlast = (app_BlastRequest_t*) appwrapper;
        param.cluster_id = BLAST_CLUSTERID;
        param.panID = BLAST_NWK_ADDR;
        param.radius = 0x01;
        MAC_UTIL_BuildandSendDataPAN (&param, (uint8 *)pstBlast,
                                      sizeof(app_BlastRequest_t),
                                      MAC_UTIL_UNICAST,
                                      pstBlast->dstAddr,
                                      MAC_TXOPTION_NO_CNF);
    }
    /* if dst address is not 0, send to mobile */
    else if (rxmsg->spihdr.dstAddr.addr.shortAddr != 0)
    {
        MacParam_t macparam;
        macparam.radius = 0x01;
        macparam.cluster_id = MINEAPP_CLUSTERID;
        macparam.panID = MOBILEPHONE_NWK_ADDR;
        MAC_UTIL_BuildandSendDataPAN(&macparam,
                                     (uint8 *) (rxmsg + 1),
                                     rxmsg->DataLength,
                                     MAC_UTIL_UNICAST,
                                     rxmsg->spihdr.dstAddr.addr.shortAddr,
                                     MAC_TXOPTION_ACK);
    }
    else
    {
        switch (appwrapper->app_flag)
        {
        case RFMAC_SET:
        {
            if (rxmsg->DataLength != sizeof(app_rfmac_set_t))
            {
                return;
            }
            if ((rfmac_setted == false)
                    && (CRC16(appwrapper->app_rfmac_set.macAddr, 8, 0xFFFF)
                        == appwrapper->app_rfmac_set.crc)
                    && (appwrapper->app_rfmac_set.macAddr[EXT_MACADDR_TYPE]
                        == EXT_MACADDR_TYPE_SUBSTATION))
            {
                memcpy(Station_DevInfo.extAddr,
                       appwrapper->app_rfmac_set.macAddr,
                       SADDR_EXT_LEN);

                Station_DevInfo.PanId = BUILD_UINT16(Station_DevInfo.extAddr[EXT_MACADDR_DEVID_LBYTE],
                                                     Station_DevInfo.extAddr[EXT_MACADDR_DEVID_HBYTE]);
                Station_DevInfo.Channel = Station_DevInfo.extAddr[EXT_MACADDR_CHANNEL];

                /* start device directly */
                rfmac_setted = true;
                osal_stop_timerEx(Station_TaskId, STATION_RFMAC_EVENT);
                Station_Startup();
            }
            break;
        }
        case CARD_SPI_CMD:
        {
            if (rxmsg->DataLength != sizeof(app_spicmd_t))
            {
                return;
            }
            app_spicmd_t* app_spicmd = (app_spicmd_t*) appwrapper;

            if ((app_spicmd->cmdtype == SPI_RETREAT
                    || app_spicmd->cmdtype == SPI_CANCEL_RETREAT)
                    && (CRC16((uint8 *) app_spicmd,
                              sizeof(app_spicmd_t) - 2,
                              0xFFFF) == app_spicmd->crc))
            {
                /* retreat state for 15 min */
                Station_DevInfo.retreat_cnt = 0;
                osal_set_event(Station_TaskId, STATION_URGENT_TIME_EVENT);

                app_spicmd_t spicmd;
                spicmd.msgtype = CARD_SPI_CMD;

                if (app_spicmd->cmdtype == SPI_RETREAT)
                {
                    Station_DevInfo.retreat_flag = STATION_RETREAT_RETREAT;
                    spicmd.cmdtype = SPI_RETREAT_ACK;
                }
                else if (app_spicmd->cmdtype == SPI_CANCEL_RETREAT)
                {
                    Station_DevInfo.retreat_flag = STATION_RETREAT_CANCELRETREAT;
                    spicmd.cmdtype = SPI_CANCEL_RETREAT_ACK;
                }

                spicmd.crc = CRC16((uint8 *) &spicmd,
                                   sizeof(spicmd) - 2,
                                   0xFFFF);
                Station_SendDataToSpi(0, (uint8 *) &spicmd, sizeof(spicmd));
            }
            break;
        }
        case CROSSPAN:
        {
            if (rxmsg->DataLength
                    != (sizeof(app_CrossPan_t) + appwrapper->app_CrossPan.len))
            {
                return;
            }

            MacParam_t macparam;
            macparam.radius = 0x01;
            macparam.cluster_id = MINEAPP_CLUSTERID;
            macparam.panID = appwrapper->app_CrossPan.dstPan;
            MAC_UTIL_BuildandSendDataPAN(&macparam,
                                         (uint8 *) (rxmsg + 1),
                                         rxmsg->DataLength,
                                         MAC_UTIL_UNICAST,
                                         0,
                                         MAC_TXOPTION_ACK);
            break;
        }
        case SPI_DETECT:
        {
            if (rxmsg->DataLength != sizeof(app_SPIDetect_t))
            {
                return;
            }

            bool checkflag;
            checkflag = true;

            for (uint8 i = 0; i < SPIDETECT_LEN; i++)
            {
                if (appwrapper->app_SPIDetect.detectdata[i] != i)
                {
                    checkflag = false;
                }
            }
            if (checkflag)
            {
                spi_errcnt = 0;
            }
            break;
        }
        case SPI_DEBUG:
        {
            if (rxmsg->DataLength
                    != (sizeof(app_SPIDebug_t)
                        + appwrapper->app_SPIDebug.debugstrlen))
            {
                return;
            }

            Station_SendDataToSpi(0,
                                  (void *) appwrapper,
                                  sizeof(app_SPIDebug_t)
                                  + appwrapper->app_SPIDebug.debugstrlen);
            break;
        }
        case RF_VERSION_REQ:
        {
            if (rxmsg->DataLength
                    != (sizeof(app_rfversion_t) + appwrapper->app_rfversion.size))
            {
                return;
            }

            uint8* pData = (uint8*) (pSpi + 1) + sizeof(app_rfversion_t);

            const uint8 maxDataLen = MINEAPP_MAX_DATA_LEN
                                     - sizeof(SPIMSGPacket_t) - sizeof(app_rfversion_t);
            sprintf((char *) pData,
                    "SoftVer:%s, BuildTm:%s,%s, Ch:%d",
                    STATION_SW_VERSION,
                    __DATE__,
                    __TIME__,
                    macPib.logicalChannel);

            app_rfversion_t* pHead = (app_rfversion_t*) (pSpi + 1);
            pHead->msgtype = RF_VERSION_RSP;
            pHead->seq = appwrapper->app_rfversion.seq;
            pHead->size = strlen((char *) pData);
            Station_SendDataToSpiEx(0,
                                    sizeof(app_rfversion_t) + pHead->size);
            break;
        }

        case GASDEV_SMS:
        {
            if (rxmsg->DataLength
                    != (sizeof(app_gassms_t) - 1 + appwrapper->app_gassms.len))
            {
                return;
            }

            MacParam_t macparam;
            macparam.radius = 0x01;
            macparam.cluster_id = MINEAPP_CLUSTERID;
            macparam.panID = appwrapper->app_gassms.dstpan;
            MAC_UTIL_BuildandSendDataPAN(&macparam,
                                         (uint8 *) (rxmsg + 1),
                                         rxmsg->DataLength,
                                         MAC_UTIL_UNICAST,
                                         0,
                                         MAC_TXOPTION_ACK);
            break;
        }
        default:
        {
            break;
        }
        }
    }
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
**************************************************************************************************/

