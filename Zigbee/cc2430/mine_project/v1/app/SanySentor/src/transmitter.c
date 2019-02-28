/**************************************************************************************************
Filename:       CR.c
Revised:        $Date: 2011/05/16 19:44:11 $
Revision:       $Revision: 1.7 $

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
#include "OSAL_Nv.h"
#include "OSAL_PwrMgr.h"


/* App Protocol*/
#include "App_cfg.h"

/* Application Includes */
#include "OnBoard.h"
#include "transmitter.h"


/* MAC Application Interface */
#include "mac_api.h"
#include "mac_main.h"
#include "mac_pib.h"
#include "ZComDef.h"
/* Application */
//#include "Station.h"
#include "AppProtocol.h"
#include "AppProtocolWrapper.h"
#include "delay.h"
#include "OSAL_Nv.h"
#include "ZComdef.h"


/* watchdog util */
#include "watchdogutil.h"
#include "MacUtil.h"
#include "crc.h"

/**************************************************************************************************
*                                           Macros
**************************************************************************************************/
#define SANY_PARAM_ADDR                     0xFEF0  //param space 1EF0-1EFF

#define SANY_RETREAT_NORMAL                 0
#define SANY_RETREAT_RETREAT                0x5A
#define SANY_RETREAT_CANCELRETREAT          0x68

#define SANY_LED_BLUE    HAL_LED_1
#define SANY_LED_RED      HAL_LED_2

#define HAL_LED_UART HAL_LED_2
#define HAL_LED_AIR HAL_LED_1
#define HAL_LED_RED HAL_LED_3


/*********************************************************************
* TYPEDEFS
*/

typedef enum
{
    E_SANY_INIT = 0,
    E_SANY_STARTED
} eSanyState;

typedef struct
{
    sAddrExt_t ExitAddr;
} extAddr_t;


typedef struct
{
    uint8 channel;
    uint16 shortAddr;
    extAddr_t extAddr;
} devInfo_t;

typedef struct
{
    /* device information */
    uint16        PanId;
    uint16        ShortAddr;
    uint8          Channel;
    sAddrExt_t extAddr;

    /* status */
    eSanyState State;

    /* retreat */
    uint8    retreat_flag;
    uint8    retreat_cnt;

} Sany_DevInfo_t;


typedef struct
{
    uint8  ResetFlag;
} storeParam_t;


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
static bool          Sany_MACTrue = TRUE;
static bool          Sany_MACFalse = FALSE;

/* Task ID */
uint8          Sany_TaskId;

/* Device Info from flash */
static Sany_DevInfo_t Sany_DevInfo;




/**************************************************************************************************
*                                     Local Function Prototypes
**************************************************************************************************/
/* Setup routines */
static void         Sany_Startup(void);

static void Sany_start_timerEx(uint8 taskID, uint16 event_id, uint16 timeout_value);

static void Sany_ParseDataCnf(const macMcpsDataCnf_t* pdatacnf);
static void Sany_ParseFrame(const macMcpsDataInd_t* pdataInd);

void Sany_ReadDevInfo(void)
{
    //extAddr_t *p = (extAddr_t *)(SANY_DEVINFO_ADDR);

    osal_nv_item_init(ZCD_NV_EXTADDR, Z_EXTADDR_LEN,NULL);
    osal_nv_read(ZCD_NV_EXTADDR, 0, Z_EXTADDR_LEN, Sany_DevInfo.extAddr);
    
    Sany_DevInfo.ShortAddr = 0x00;//Sany_devinfo.shortAddr;
    Sany_DevInfo.PanId = BUILD_UINT16(Sany_DevInfo.extAddr[EXT_MACADDR_DEVID_LBYTE],
                                          Sany_DevInfo.extAddr[EXT_MACADDR_DEVID_HBYTE]);
    Sany_DevInfo.Channel = Sany_DevInfo.extAddr[EXT_MACADDR_CHANNEL];

    /* if wrong address */
    if(Sany_DevInfo.Channel <11 || Sany_DevInfo.Channel > 26 ||  Sany_DevInfo.ShortAddr==0xFFFF)
    {
        while(1)
        {
	  HalLedSet(HAL_LED_ALL, HAL_LED_MODE_ON);
        }
    }
}

/**************************************************************************************************
*
* @fn          Sany_Init
*
* @brief       Initialize the application
*
* @param       taskId - taskId of the task after it was added in the OSAL task queue
*
* @return      none
*
**************************************************************************************************/
void Sany_Init(uint_8 taskId)
{
    /* Initialize the task id */
    Sany_TaskId = taskId;

    /* initialize MAC features */
    MAC_Init();
    MAC_InitCoord();

    /* Reset the MAC */
    MAC_MlmeResetReq(TRUE);

     /* LED*/
    HalLedSet(HAL_LED_UART, HAL_LED_MODE_OFF);
    HalLedSet(HAL_LED_AIR, HAL_LED_MODE_OFF);
    HalLedSet(HAL_LED_RED, HAL_LED_MODE_ON);

#if(defined WATCHDOG) && (WATCHDOG==TRUE)
    StartWatchDog(DOGTIMER_INTERVAL_1S);
    osal_set_event(Sany_TaskId, STATION_FEEDWATCHDOG_EVENT);
#endif

    halUARTCfg_t uartConfig;
    /* UART Configuration */
    uartConfig.configured           = TRUE;
    uartConfig.baudRate             = HAL_UART_BR_115200;
    uartConfig.flowControl          = false;
    uartConfig.flowControlThreshold = 48;
    uartConfig.rx.maxBufSize        = 0;
    uartConfig.tx.maxBufSize        = 128;
    uartConfig.idleTimeout          = 6;
    uartConfig.intEnable            = TRUE;
    uartConfig.callBackFunc         = NULL;
    
      /* Start UART */
    HalUARTOpen (HAL_UART_PORT_0, &uartConfig);


    Sany_ReadDevInfo();
    Sany_DevInfo.State = E_SANY_INIT;
    Sany_DevInfo.retreat_flag = SANY_RETREAT_NORMAL;
    Sany_DevInfo.retreat_cnt = 0;


    /* initial spi */
    /*halSPICfg_t spiConfig;
    spiConfig.bufSize= DMA_RT_MAX;
    spiConfig.taskID= Sany_TaskId;
    spiConfig.eventID = STATION_SPI_READ_EVENT;
    HalSpiStart(&spiConfig);*/

    /*Sany_SpiReadBuf = (uint8*) osal_mem_alloc(MINEAPP_MAX_DATA_LEN + 10);
    Sany_SpiBuf = (uint8*) osal_mem_alloc(MINEAPP_MAX_DATA_LEN + 10);
    pSpi = (SPIMSGPacket_t *) Sany_SpiBuf;
    pSpi->spihdr.hdr.event = SPI_RX_MSG;                                  // useless, just the same to v1.0
    pSpi->spihdr.srcAddr.addrMode = (AddrMode_t) Addr16Bit;
    pSpi->spihdr.srcAddr.endPoint = MINEAPP_ENDPOINT;
    pSpi->spihdr.dstAddr.addrMode = (AddrMode_t) Addr16Bit;
    pSpi->spihdr.dstAddr.endPoint = MINEAPP_ENDPOINT;
    pSpi->spihdr.transID = INIT_TRANID;
    pSpi->spihdr.options = INIT_OPN;
    pSpi->spihdr.radius = 1;*/

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
    Sany_start_timerEx(Sany_TaskId, STATION_RFMAC_EVENT, 1);
    Sany_start_timerEx(Sany_TaskId, STATION_LOC_BLAST_EVENT, 10);
}


/**************************************************************************************************
*
* @fn          SanyTr_ProcessEvent
*
* @brief       This routine handles events
*
* @param       taskId - ID of the application task when it registered with the OSAL
*              events - Events for this task
*
* @return      16bit - Unprocessed events
*
**************************************************************************************************/
uint16 SanyTr_ProcessEvent(uint_8 taskId, uint16 events)
{
    uint8* pMsg;
    macCbackEvent_t* pData;

#if(defined WATCHDOG) && (WATCHDOG==TRUE)
    if(events & STATION_FEEDWATCHDOG_EVENT)
    {
        Sany_start_timerEx(Sany_TaskId, STATION_FEEDWATCHDOG_EVENT, 300);
        FeedWatchDog();
        return events ^ STATION_FEEDWATCHDOG_EVENT;
    }
#endif

    if(events & SYS_EVENT_MSG)
    {
        while((pMsg = osal_msg_receive(Sany_TaskId)) != NULL)
        {
            pData = (macCbackEvent_t *) pMsg;
            switch(*pMsg)
            {          

            case MAC_MCPS_DATA_CNF:
            {
                Sany_ParseDataCnf(&pData->dataCnf);
                mac_msg_deallocate((uint8**)&pData->dataCnf.pDataReq);
                break;
            }

            case MAC_MCPS_DATA_IND:
            {
            		/* do not check the indication status*/
                //if(pData->dataInd.hdr.status == ZSUCCESS)
                {
                    Sany_ParseFrame(&pData->dataInd);
                }
                break;
            }
            case SPI_RX_MSG:
            {
                break;
            }
            default:
                break;
            }
            
            /* Deallocate */
            mac_msg_deallocate((uint8 **)&pMsg);
        }

        return events ^ SYS_EVENT_MSG;
    }
    
    if(events & STATION_RFMAC_EVENT)
    {


        //if(rfmac_setted)
        {
            Sany_Startup();
        }
        /*else
        {
            HalLedSet(HAL_LED_UART, HAL_LED_MODE_TOGGLE);
            Sany_SendDataToSpi(0, (uint8*)&rfmac_query, sizeof(rfmac_query));
            osal_start_timerEx(Sany_TaskId, STATION_RFMAC_EVENT, 1000);
        }*/
        return events ^ STATION_RFMAC_EVENT;
    }

    
    if (events & STATION_LOC_BLAST_EVENT)
    {
        Sany_start_timerEx ( Sany_TaskId, STATION_LOC_BLAST_EVENT, 2500);
        if(Sany_DevInfo.State == E_SANY_STARTED)
        {
            static uint8 locnode_seq;
            app_LocNodeCast_t app_LocNodeCast;
            app_LocNodeCast.msgtype = LOCNODE_CAST;
            app_LocNodeCast.vol = 0xFF;
            app_LocNodeCast.seq = locnode_seq++;
            app_LocNodeCast.DevID = Sany_DevInfo.PanId;
            MacParam_t param;
            param.panID = 0xFFFF;
            param.cluster_id = GASMONITOR_CLUSTERID;
            param.radius = 0x01;
            MAC_UTIL_BuildandSendDataPAN(&param, ( uint8 * ) &app_LocNodeCast,
                                         sizeof (app_LocNodeCast_t), MAC_UTIL_BROADCAST, MAC_UTIL_BROADCAST_SHORTADDR_DEVZCZR, 0);
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
        if(SUCCESS!=osal_msg_send(Sany_TaskId, (uint8 *) pMsg))
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
void Sany_Startup()
{
    macMlmeStartReq_t   startReq;

    //if(Sany_DevInfo.State == E_SANY_STARTED) return;

    /* Setup MAC_EXTENDED_ADDRESS */
    MAC_MlmeSetReq(MAC_EXTENDED_ADDRESS, &Sany_DevInfo.extAddr);

    /* Setup MAC_SHORT_ADDRESS */
    Sany_DevInfo.ShortAddr = 0;
    MAC_MlmeSetReq(MAC_SHORT_ADDRESS, &Sany_DevInfo.ShortAddr);

    /* Setup MAC_BEACON_PAYLOAD_LENGTH */
    uint_8 tmp8 = 0;
    MAC_MlmeSetReq(MAC_BEACON_PAYLOAD_LENGTH, &tmp8);

    /* Enable RX */
    MAC_MlmeSetReq(MAC_RX_ON_WHEN_IDLE, &Sany_MACTrue);

    /* Setup MAC_ASSOCIATION_PERMIT */
    MAC_MlmeSetReq(MAC_ASSOCIATION_PERMIT, &Sany_MACFalse);

    /* change CCA param */
    uint8 maxFrameRetries = 4;
    MAC_MlmeSetReq(MAC_MAX_FRAME_RETRIES, &maxFrameRetries);

    uint8 maxCsmaBackoff  = 5;
    MAC_MlmeSetReq(MAC_MAX_CSMA_BACKOFFS, &maxCsmaBackoff);

    uint8 minBe = 4;
    MAC_MlmeSetReq(MAC_MIN_BE, &minBe);

    uint8 maxBe = 6;
    MAC_MlmeSetReq(MAC_MAX_BE, &maxBe);

    /* Fill in the information for the start request structure */
    startReq.startTime = 0;
    startReq.panId = Sany_DevInfo.PanId;
    startReq.logicalChannel = Sany_DevInfo.Channel;
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

static void Sany_ParseDataCnf(const macMcpsDataCnf_t* pdatacnf)
{
    static uint8 fail_cnt = 0;
    if(pdatacnf->hdr.status == SUCCESS)
    {
        fail_cnt = 0;
    }
    else
    {
        /* if send data failed for 15 times, find reason and restart */
        if(fail_cnt++ > 30)
        {
            /* set restart reason and restart */
            storeParam_t param;

            uint8 *p1, *p2;
            p1 = osal_mem_alloc(8);    //small size memory
            p2 = osal_mem_alloc(64);    //big size memory
            if(p1 == NULL || p2 == NULL)
            {
                param.ResetFlag = ZC_REPORT_MEMORY_ERR_RESTART;
            }
            else
            {
                param.ResetFlag = ZC_REPORT_BLAST_ERR_RESTART;
            }
            * (storeParam_t*) SANY_PARAM_ADDR =  param;
            osal_mem_free(p1);
            osal_mem_free(p2);

            SystemReset();
        }
    }
}

static void Sany_ParseFrame(const macMcpsDataInd_t* pdataInd)
{

    if(pdataInd == NULL ||pdataInd->msdu.p==NULL  
		|| pdataInd->msdu.len <=  MAC_UTIL_ZIGBEEHDR_LEN // min length is zigbee header + MSGType
		|| pdataInd->msdu.len > MAC_MAX_FRAME_SIZE)  
    {
        return;
    }

    uint8  clusterid = MAC_UTIL_GetClusterID(pdataInd->msdu);
    sData_t appdata = MAC_UTIL_RemoveHeader(pdataInd->msdu);

    HalLedSet(SANY_LED_BLUE, HAL_LED_MODE_TOGGLE);

    switch(clusterid)
    {
        case CARD_CLUSTERID:
        {
            HalUARTWrite(HAL_UART_PORT_0, (uint8 *)appdata.p, appdata.len);
            //Sany_ParseCardFrame(&pdataInd->mac.srcAddr, pdataInd->mac.srcPanId, appdata, pdataInd->mac.rssi);
            break;
        }
        default:
        {
            break;
        }
    
    }
}

static void Sany_start_timerEx(uint8 taskID, uint16 event_id, uint16 timeout_value)
{
    if(ZSuccess != osal_start_timerEx(taskID, event_id, timeout_value))
    {
        SystemReset();
    }
}

/**************************************************************************************************
**************************************************************************************************/

