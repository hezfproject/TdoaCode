
/****************************************************************************/
/***        Include files                                                 ***/
/****************************************************************************/
#include <jendefs.h>
#include <AppHardwareApi.h>
#include <AppQueueApi.h>
#include <mac_sap.h>
#include <mac_pib.h>
#include <Utilities.h>
#include <stdbool.h>

#include "config.h"
#include "app_protocol.h"
#include "numtrans.h"
//#include "bsmac.h"
#include "JN5148_util.h"

/****************************************************************************/
/***        Macro Definitions                                             ***/
/****************************************************************************/

//#define DEBUG
//#define DEBUG_MP_STATION

#if (defined DEBUG_COM_APP)
#define DBG(x) {x}
#else
#define DBG(x)
#endif

/* app envents */
#define STATION_PORT_QUERY_EVENT       BIT(1)
#define   STATION_AIRSENT_EVENT                 BIT(2)

#define MOBILEPHONE_NWK_ADDR            0xFFF4
#define AIR_BUF_SIZE                    16
#define HI_UINT16(a) (((a) >> 8) & 0xFF)
#define LO_UINT16(a) ((a) & 0xFF)
#define CONVERT_ENDIAN(x)   SysUtil_vConvertEndian(&(x), sizeof(x))
#define MEMCOPY(x, y)   memcpy(&(x), &(y), sizeof(x))

/****************************************************************************/
/***        Type Definitions                                              ***/
/****************************************************************************/
typedef enum
{
    E_STATE_IDLE,
    E_STATE_COORDINATOR_STARTED,
    E_STATE_AVAILABLE,
    E_STATE_RSSI,
    E_STATE_ACCEPT,
    E_STATE_TOF,
    E_STATE_LOCATOR,
    E_STATE_BUSY,
} teState;

typedef struct
{
    uint16 destAddr;
    uint16 msduLen;
    uint8 msdu[MAC_MAX_DATA_PAYLOAD_LEN];
} tsAirData_t;

typedef struct
{
    tsAirData_t   tsAirData[AIR_BUF_SIZE];
    uint8   sendIdx;
    uint8   fillIdx;
    bool_t  isSending;
} tsAirDataInfo_t;

/****************************************************************************/
/***        Local Function Prototypes                                     ***/
/****************************************************************************/
PRIVATE void vInitSystem(void);
PRIVATE void vStartCoordinator(void);
PRIVATE void vProcessSysEventQueues(void);
PRIVATE void vProcessIncomingMlme(MAC_MlmeDcfmInd_s *psMlmeInd);
PRIVATE void vProcessIncomingMcps(MAC_McpsDcfmInd_s *psMcpsInd);
PRIVATE void vProcessIncomingHwEvent(AppQApiHwInd_s *psAHI_Ind);
PRIVATE void vHandleMcpsDataInd(MAC_McpsDcfmInd_s *psMcpsInd);
PRIVATE void vHandleMcpsDataDcfm(MAC_McpsDcfmInd_s *psMcpsInd);
PRIVATE void vProcessAppEventQueues(void);
PRIVATE void vProcessPhoneData(MAC_McpsIndData_s *psMcpsInd);
//PRIVATE bool_t bArmReady();
PRIVATE void vAirFillSendBuf(uint16 dstAddr, const uint8 *msdu, uint16 msduLen);
//PRIVATE void vBsMac_rx_callback(unsigned char *pbuf, unsigned char len);

/****************************************************************************/
/***        Exported Variables                                            ***/
/****************************************************************************/

/****************************************************************************/
/***        Local Variables                                               ***/
/****************************************************************************/
/* Handles from the MAC */
PRIVATE void *s_pvMac;
PRIVATE MAC_Pib_s *s_psMacPib;
PRIVATE MAC_ExtAddr_s psMacAddr;
PRIVATE uint16 u16StationShortAddr;
PRIVATE uint16 u16StationPanId;
PRIVATE uint8 u8Channel;
PRIVATE uint8 u8ExtAddr[8];

PRIVATE uint16 u16ArmId = 0x1234;
PRIVATE bool_t bGetArmId = TRUE;
PRIVATE teState teStaionState = E_STATE_IDLE;
PRIVATE bool_t bIsStationUp = TRUE;

//PRIVATE uint8 u8LiveSeqNum = 0;
//PRIVATE uint8 u8LastLiveAckSeq = 0;

/*use u32 for alignment*/
PRIVATE uint32 u32CommonBuf[40];

PRIVATE tsAirDataInfo_t  tsAirDataInfo;

/* print debug logs */
#ifdef DEBUG
PRIVATE uint32 airRecvVoiceCnt = 0;
PRIVATE uint32 uartRecvVoiceCnt = 0;

PRIVATE uint16 u16MPnum = 0;
PRIVATE uint32 u32MPRecvCnt = 0;
PRIVATE uint32 u32MPErrCnt = 0;
PRIVATE uint8   u8MPSeqnum = 0;
PRIVATE bool    bMPSynced  = false;
#endif
/****************************************************************************/
/***        Exported Functions                                            ***/
/****************************************************************************/
/****************************************************************************
 *
 * NAME: AppColdStart
 *
 ****************************************************************************/
PUBLIC void AppColdStart(void)
{
#ifdef WATCHDOG_ENABLED
    vAHI_WatchdogStop();
#endif

    vInitSystem();

    vStartCoordinator();
    DBG(PrintfUtil_vPrintf("System started. \n\r");)


    while(1)
    {
	    TimerUtil_vUpdate();
        vProcessSysEventQueues();
        vProcessAppEventQueues();
    }
}

/****************************************************************************
 *
 * NAME: AppWarmStart
 *
 ****************************************************************************/
PUBLIC void AppWarmStart(void)
{
    //PrintfUtil_u8Init(0, 2);
    // DBG(PrintfUtil_vPrintf("warm start. \n\r");)
    AppColdStart();
}

/****************************************************************************/
/***        Local Functions                                               ***/
/****************************************************************************/

/****************************************************************************
 *
 * NAME: vInitSystem
 *
 ****************************************************************************/
PRIVATE void vInitSystem(void)
{
    (void)u32AHI_Init();
    (void)u32AppQApiInit(NULL, NULL, NULL); //vHwDeviceIntCallback);
    PrintfUtil_u8Init(0, 2);

    //vAHI_ProtocolPower(TRUE);
    //vAHI_BbcSetHigherDataRate(E_AHI_BBC_CTRL_DATA_RATE_500_KBPS);
    //vAHI_BbcSetInterFrameGap(50);


    // Enable high power modules, tof function, timerUtil
    vAHI_HighPowerModuleEnable(TRUE, TRUE);
    //TimerUtil_vEnableTimer();
    TimerUtil_vInit();

    /*init address*/
    MacUtil_vReadExtAddress(&psMacAddr);
    
    uint32 tmp32;
    tmp32 = StringUtil_BCD2uint32(psMacAddr.u32H);
    
    u16StationShortAddr = 0x0000;   //psMacAddr.u32L & 0xFFFF;
   
    u16StationPanId = tmp32%100000;
    u8Channel = ((psMacAddr.u32L) >> 16) & 0xFF;
    
    //check setting
    if(u8Channel < 11 || u8Channel > 25
        || u16StationPanId < 20000 || u16StationPanId > 29999 
        )
    {
        while(1)
        {
            LedUtil_vFlashAll(300, 10000);
        }
    }
    
    //Jbsmac_u8Init(vBsMac_rx_callback, E_AHI_UART_1, BSMAC_UART_BAUD_DIVISOR_500k, u16StationPanId);

    memcpy(u8ExtAddr, (void *)&psMacAddr.u32L, 4);
    memcpy(u8ExtAddr + 4, (void *)&psMacAddr.u32H, 4);
    SysUtil_vConvertEndian(u8ExtAddr, 4);
    SysUtil_vConvertEndian(u8ExtAddr + 4, 4);

    /* Set up the MAC handles. Must be called AFTER u32AppQApiInit() */
    s_pvMac = pvAppApiGetMacHandle();
    s_psMacPib = MAC_psPibGetHandle(s_pvMac);

    /* Set Pan ID and short address in PIB (also sets match registers in hardware) */
    MAC_vPibSetPanId(s_pvMac, u16StationPanId);
    MAC_vPibSetShortAddr(s_pvMac, u16StationShortAddr);

    /* Enable receiver to be on when idle */
    MAC_vPibSetRxOnWhenIdle(s_pvMac, TRUE, FALSE);


    /* Allow nodes to associate */
    s_psMacPib->bAssociationPermit = 0;
//    s_psMacPib->bAutoRequest = 0;
//    s_psMacPib->bGtsPermit = FALSE;
//    s_psMacPib->u16CoordShortAddr = 0x00;

    /* change csma sets*/
    // s_psMacPib->u8MaxFrameRetries = 4;
    // MAC_vPibSetMaxCsmaBackoffs(s_pvMac, 5);
    // MAC_vPibSetMinBe(s_pvMac, 4);

    MacUtil_Setting_s sMacUtil;
    sMacUtil.u16SrcPanId        = u16StationPanId;
    sMacUtil.u16SrcShortAddr    = u16StationShortAddr;
    sMacUtil.u16Profile_id      = 0x2001; //for backward compatable
    sMacUtil.u8Dst_endpoint     = 0x21;
    sMacUtil.u8NodeType         = 0;
    sMacUtil.u8Src_endpoint     = 0;
    MacUtil_vInit(&sMacUtil);

    //TimerUtil_eSetTimer(STATION_PORT_QUERY_EVENT, 1000);

    memset(&tsAirDataInfo, 0, sizeof(tsAirDataInfo));

}

/****************************************************************************
 *
 * NAME: vProcessSysEventQueues
 *
 ****************************************************************************/
PRIVATE void vProcessSysEventQueues(void)
{
    MAC_MlmeDcfmInd_s *psMlmeInd;
    MAC_McpsDcfmInd_s *psMcpsInd;
    AppQApiHwInd_s    *psAHI_Ind;

    do
    {
        psMcpsInd = psAppQApiReadMcpsInd();
        if(psMcpsInd != NULL)
        {
            vProcessIncomingMcps(psMcpsInd);
            vAppQApiReturnMcpsIndBuffer(psMcpsInd);
        }
    }
    while(psMcpsInd != NULL);

    do
    {
        psMlmeInd = psAppQApiReadMlmeInd();
        if(psMlmeInd != NULL)
        {
            vProcessIncomingMlme(psMlmeInd);
            vAppQApiReturnMlmeIndBuffer(psMlmeInd);
        }
    }
    while(psMlmeInd != NULL);

    do
    {
        psAHI_Ind = psAppQApiReadHwInd();
        if(psAHI_Ind != NULL)
        {
            vProcessIncomingHwEvent(psAHI_Ind);
            vAppQApiReturnHwIndBuffer(psAHI_Ind);
        }
    }
    while(psAHI_Ind != NULL);
}


/****************************************************************************
 *
 * NAME: vProcessAppEventQueues
 *
 ****************************************************************************/
PRIVATE void vProcessAppEventQueues(void)
{
    uint32 event = EventUtil_u32ReadEvents();
    switch(event)
    {
    case STATION_PORT_QUERY_EVENT:
    {
        EventUtil_vUnsetEvent(STATION_PORT_QUERY_EVENT);
        break;
    }
    case STATION_AIRSENT_EVENT:
    {
        /* send air data */
        if(tsAirDataInfo.sendIdx != tsAirDataInfo.fillIdx)
        {
            uint8 idx = tsAirDataInfo.sendIdx;

            MacUtil_SendParams_s sParams;
            sParams.u8Radius        = 1;
            sParams.u16DstAddr  = tsAirDataInfo.tsAirData[idx].destAddr;
            sParams.u16DstPanId     = MOBILEPHONE_NWK_ADDR;
            sParams.u16ClusterId    = 0;
            sParams.u8DeliverMode   = MAC_UTIL_UNICAST;

            tsAirDataInfo.isSending = true;
            MacUtil_vSendData(&sParams, (uint8 *)tsAirDataInfo.tsAirData[idx].msdu, tsAirDataInfo.tsAirData[idx].msduLen, MAC_TX_OPTION_ACK);

            if(tsAirDataInfo.sendIdx < AIR_BUF_SIZE - 1)
            {
                tsAirDataInfo.sendIdx++;
            }
            else
            {
                tsAirDataInfo.sendIdx = 0;
            }
        }

        EventUtil_vUnsetEvent(STATION_AIRSENT_EVENT);
        break;
    }
    default:
        break;
    }
}

/****************************************************************************
 *
 * NAME: vProcessIncomingMlme
 *
 ****************************************************************************/
PRIVATE void vProcessIncomingMlme(MAC_MlmeDcfmInd_s *psMlmeInd)
{
}

/****************************************************************************
 *
 * NAME: vProcessIncomingData
 *
 ****************************************************************************/
PRIVATE void vProcessIncomingMcps(MAC_McpsDcfmInd_s *psMcpsInd)
{
    if(teStaionState >= E_STATE_COORDINATOR_STARTED)
    {
        switch(psMcpsInd->u8Type)
        {
        case MAC_MCPS_IND_DATA:  /* Incoming data frame */
            vHandleMcpsDataInd(psMcpsInd);
            break;
        case MAC_MCPS_DCFM_DATA: /* Incoming acknowledgement or ack timeout */
            vHandleMcpsDataDcfm(psMcpsInd);
            break;
        default:
            break;
        }
    }
}

/****************************************************************************
 *
 * NAME: vHandleMcpsDataDcfm
 *
 ****************************************************************************/
PRIVATE void vHandleMcpsDataDcfm(MAC_McpsDcfmInd_s *psMcpsInd)
{
    tsAirDataInfo.isSending = false;

    if(tsAirDataInfo.sendIdx != tsAirDataInfo.fillIdx)
    {
        EventUtil_vSetEvent(STATION_AIRSENT_EVENT);
    }
}

/****************************************************************************
 *
 * NAME: vHandleMcpsDataInd
 *
 ****************************************************************************/
PRIVATE void vHandleMcpsDataInd(MAC_McpsDcfmInd_s *psMcpsInd)
{
    MAC_RxFrameData_s *psFrame;
    psFrame = &psMcpsInd->uParam.sIndData.sFrame;

    app_header_t *psAppPkt = (app_header_t *)(psFrame->au8Sdu);

    // if(!bIsStationUp) return;

    switch(psAppPkt->protocoltype)
    {
    case APP_PROTOCOL_TYPE_MOBILE:
    {

        vProcessPhoneData(&psMcpsInd->uParam.sIndData);
        break;
    }

    default:
        break;
    }
}


/****************************************************************************
 *
 * NAME: vProcessIncomingHwEvent
 *
 ****************************************************************************/
PRIVATE void vProcessIncomingHwEvent(AppQApiHwInd_s *psAHI_Ind)
{
}


/****************************************************************************
 *
 * NAME: vStartCoordinator
 *
 ****************************************************************************/
PRIVATE void vStartCoordinator(void)
{
    /* Structures used to hold data for MLME request and response */
    MAC_MlmeReqRsp_s   sMlmeReqRsp;
    MAC_MlmeSyncCfm_s  sMlmeSyncCfm;

    teStaionState = E_STATE_COORDINATOR_STARTED;

    sMlmeReqRsp.u8Type = MAC_MLME_REQ_START;
    sMlmeReqRsp.u8ParamLength = sizeof(MAC_MlmeReqStart_s);
    sMlmeReqRsp.uParam.sReqStart.u16PanId = u16StationPanId;
    sMlmeReqRsp.uParam.sReqStart.u8Channel = u8Channel;
    sMlmeReqRsp.uParam.sReqStart.u8BeaconOrder = 0x0F;
    sMlmeReqRsp.uParam.sReqStart.u8SuperframeOrder = 0x0F;
    sMlmeReqRsp.uParam.sReqStart.u8PanCoordinator = TRUE;
    sMlmeReqRsp.uParam.sReqStart.u8BatteryLifeExt = FALSE;
    sMlmeReqRsp.uParam.sReqStart.u8Realignment = FALSE;
    sMlmeReqRsp.uParam.sReqStart.u8SecurityEnable = FALSE;

    vAppApiMlmeRequest(&sMlmeReqRsp, &sMlmeSyncCfm);
}

PRIVATE void vProcessPhoneData(MAC_McpsIndData_s *psMcpsIndData)
{
    MAC_RxFrameData_s *psFrame;
    psFrame = &psMcpsIndData->sFrame;
	
    if(psFrame->u8SduLength < sizeof(app_header_t) || psFrame->u8SduLength > MAC_MAX_DATA_PAYLOAD_LEN)
    {
        DBG(PrintfUtil_vPrintf("air msg  len ERR! len:%d\n", psFrame->u8SduLength););
        return;
    }

    app_header_t *pheader = (app_header_t *)(psFrame->au8Sdu);

    DBG(
        if(pheader->msgtype == MP_VOICE)
        {
            static uint32 cnt;
            if(cnt++ % 20 == 0)
            {
                PrintfUtil_vPrintf("AR voice\n");
            }
        }
        else
        {
            PrintfUtil_vPrintf("AR\n");
            PrintfUtil_vPrintMem(psFrame->au8Sdu, psFrame->u8SduLength);
        }
    );

    uint16 payloadlen = pheader->len;
    CONVERT_ENDIAN(payloadlen);

    if(payloadlen + sizeof(app_header_t) != psFrame->u8SduLength)
    {
        DBG(PrintfUtil_vPrintf("app_header len ERR!\n"););
        return;
    }
	
#ifdef DEBUG
    static  uint32 voicecnt = 0;
    if(pheader->msgtype == MP_VOICE)
    {
        airRecvVoiceCnt++;
        app_mpVoice_t *pVoice = (app_mpVoice_t *)(pheader + 1);

        /* count the spectial mp number */
        if(psFrame->sSrcAddr.uAddr.u16Short == u16MPnum)
        {
            //PrintfUtil_vPrintf("voice, src:%x, seq:%d\n", pVoice->hdr.srcaddr, pVoice->seqnum);
            u32MPRecvCnt++;
            if(bMPSynced)
            {
                if(pVoice->seqnum - u8MPSeqnum > 1 && pVoice->seqnum - u8MPSeqnum < 10)
                {
                    u32MPErrCnt += pVoice->seqnum - u8MPSeqnum - 1;
                }
                u8MPSeqnum = pVoice->seqnum;
            }
            else
            {
                bMPSynced = true;
                u8MPSeqnum = pVoice->seqnum;
            }
        }
    }
    else if(pheader->msgtype == MP_CMD_UP)
    {
        app_mpCmd_t *pCmd = (app_mpCmd_t *)(pheader + 1);
        if(pCmd->cmdtype == MP_UP_DIALUP)
        {
            char str[20];
            uint16 srcAddr;
            num_term2str(str, &pCmd->srcnbr);
            srcAddr = SysUtil_u16atou(str);

            u16MPnum = srcAddr;
            u32MPRecvCnt = 0;
            u32MPErrCnt = 0;
            u8MPSeqnum = 0;
            bMPSynced = false;
        }
    }

    if(++voicecnt % 100 == 0)
    {
        /* print air received voice and uart received voice */
        PrintfUtil_vPrintf("VoiceRecvCnt air:%d uart:%d\n", airRecvVoiceCnt, uartRecvVoiceCnt);

        /* print one mobilephone aire upload err rate */
        PrintfUtil_vPrintf("MPnum:%d, MPRecv:%d, MPErr:%d\n", u16MPnum, u32MPRecvCnt, u32MPErrCnt);
    }
#endif

    /* check if the src addr is equal as the wireless srcaddr */    
    DBG
    (
        if(pheader->msgtype < MP_MP2ARM_CMDEND)
        {
            app_mpheader_t *pmpheader = (app_mpheader_t *)(pheader + 1);
            uint16 srcaddr = pmpheader->srcaddr;
            CONVERT_ENDIAN(srcaddr);
            if(srcaddr != psFrame->sSrcAddr.uAddr.u16Short)
            {
                PrintfUtil_vPrintf("shortaddr ERR! stack addr: %d, protocol addr: %d\n", psFrame->sSrcAddr.uAddr.u16Short, srcaddr);
            }
        }
    )

    /* reuse the upload  buffer, and memcpy in vAirFillSendBuf */
    if(pheader->msgtype == MP_JOIN_NOTIFY)
    {
        app_mpJoinNwk_t *pJoinNwk = (app_mpJoinNwk_t *)(pheader + 1);

        pJoinNwk->hdr.dstaddr = pJoinNwk->hdr.srcaddr;
        pJoinNwk->hdr.srcaddr = 0x0;
        pJoinNwk->joinnwktype = APP_MP_JOINNWK_SUCCESS;
        pJoinNwk->armid = u16ArmId;
        CONVERT_ENDIAN(pJoinNwk->armid);

        uint16 addr = pJoinNwk->hdr.dstaddr;
        CONVERT_ENDIAN(addr);
        vAirFillSendBuf(addr, (uint8 *)pheader, sizeof(app_header_t) + payloadlen);
    }
    else if(pheader->msgtype == MP_POLL)
    {
        app_mpPoll_t *pPoll = (app_mpPoll_t *)(pheader + 1);
        pPoll->hdr.dstaddr = pPoll->hdr.srcaddr;
        pPoll->hdr.srcaddr = 0x0;
        pPoll->polltype = APP_MPPOLL_TYPE_ACK;
        pPoll->flag = APP_MPPOLL_FLAG_START;
        //pPoll->seqnum = pPoll->seqnum;

        uint16 addr = pPoll->hdr.dstaddr;
        CONVERT_ENDIAN(addr);
        vAirFillSendBuf(addr, (uint8 *)pheader, sizeof(app_header_t) + payloadlen);
    }
    else if(pheader->msgtype == MP_CMD_UP)
    {
        app_mpCmd_t *pmpCmd = (app_mpCmd_t *)(pheader + 1);

        char str[20];
        uint16 dstAddr;
        num_term2str(str, &pmpCmd->dstnbr);
        dstAddr = SysUtil_u16atou(str);

        pheader->msgtype = MP_CMD_DOWN;
        switch(pmpCmd->cmdtype)
        {
        case MP_UP_DIALUP:
        {
            pmpCmd->cmdtype = MP_DOWN_CALL;
            break;
        }
        case MP_UP_FOUND:
        {
            pmpCmd->cmdtype = MP_DOWN_FOUND;
            break;
        }
        case MP_UP_BUSY:
        {
            pmpCmd->cmdtype = MP_DOWN_BUSY;
            break;
        }
        case MP_UP_ACCEPT:
        {
            pmpCmd->cmdtype = MP_DOWN_ACCEPT;
            break;
        }
        case MP_UP_CLOSE:
        {
            pmpCmd->cmdtype = MP_DOWN_CLOSE;
            break;
        }
        }

        vAirFillSendBuf(dstAddr, (uint8 *)pheader, sizeof(app_header_t) + payloadlen);
    }
    else if(pheader->msgtype == MP_VOICE)
    {
        app_mpVoice_t *pVoice = (app_mpVoice_t *)(pheader + 1);

        uint16 addr = pVoice->hdr.dstaddr;
        CONVERT_ENDIAN(addr);
        vAirFillSendBuf(addr, (uint8 *)pheader, sizeof(app_header_t) + payloadlen);
    }

    else if(pheader->msgtype == MP_SCAN)
    {

        mp_Scan_t *pArmid = (mp_Scan_t *)(pheader + 1) ;

        if(pArmid->scantype == APP_SCAN_TYPE_REQ)
        {
            app_header_t *app_hdr = (app_header_t *)u32CommonBuf;
            mp_Scan_t *mp_Armid = (mp_Scan_t *)(app_hdr + 1);

            app_hdr->protocoltype = APP_PROTOCOL_TYPE_MOBILE;
            app_hdr->msgtype = MP_SCAN;
            app_hdr->len = sizeof(mp_Scan_t);
            SysUtil_vConvertEndian(&app_hdr->len, sizeof(app_hdr->len));

            mp_Armid->scantype = APP_SCAN_TYPE_ACK;
            mp_Armid->seqnum = pArmid->seqnum;
            mp_Armid->armid = u16ArmId; //??????????????
            SysUtil_vConvertEndian(&mp_Armid->armid, sizeof(mp_Armid->armid));

            vAirFillSendBuf(psFrame->sSrcAddr.uAddr.u16Short, (uint8 *)app_hdr, sizeof(app_hdr) + sizeof(mp_Scan_t));

            //MacUtil_vSendData(&sParams, (uint8 *)&u32CommonBuf, (sizeof(app_header_t) + sizeof(mp_Scan_t)), MAC_TX_OPTION_ACK);
        }
    }
}

PRIVATE void vAirFillSendBuf(uint16 dstAddr, const uint8 *msdu, uint16 msduLen)
{
    /* fill the send buffer */
    uint8 idx = tsAirDataInfo.fillIdx;
    tsAirDataInfo.tsAirData[idx].destAddr = dstAddr;
    tsAirDataInfo.tsAirData[idx].msduLen = msduLen  > MAC_MAX_DATA_PAYLOAD_LEN ? MAC_MAX_DATA_PAYLOAD_LEN : msduLen;
    memcpy((uint8 *)tsAirDataInfo.tsAirData[idx].msdu, msdu, tsAirDataInfo.tsAirData[idx].msduLen);

    /*update fill idx*/
    if(tsAirDataInfo.fillIdx < AIR_BUF_SIZE - 1)
    {
        tsAirDataInfo.fillIdx++;
    }
    else
    {
        tsAirDataInfo.fillIdx = 0;
    }

    if(!tsAirDataInfo.isSending)
    {
        EventUtil_vSetEvent(STATION_AIRSENT_EVENT);
    }
}
/****************************************************************************/
/***        END OF FILE                                                   ***/
/****************************************************************************/




