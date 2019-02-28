/****************************************************************************/
/***        Include files                                                 ***/
/****************************************************************************/
#include <jendefs.h>
#include <AppHardwareApi.h>
#include <AppQueueApi.h>
#include <mac_sap.h>
#include <mac_pib.h>
#include <Utilities.h>
#include <OAD.h>

#include "CRC.h"
#include "string.h"
#include "ground_crsys.h"

#define CARDREADER_DEVID_MIN                 1000
#define CARDREADER_DEVID_MAX                 65000
#define CARDREADER_IS_DEVID(x)  \
    ((x)>=CARDREADER_DEVID_MIN && (x)<=CARDREADER_DEVID_MAX)

/****************************************************************************/
/***        Type Definitions                                              ***/
/****************************************************************************/
typedef union
{
    RADIO_CMD_T msg_head;
    RADIO_DEV_INFO_T rf_dev_data;
    RADIO_WRIST_TIME_T rf_wrist_time;
}RfWrapper_tu;

/****************************************************************************/
/***        Local Function Prototypes                                     ***/
/****************************************************************************/
PRIVATE void _MCPS_data_dcfm(MAC_McpsDcfmInd_s const * const psMcpsInd);

PRIVATE void _MCPS_data_ind(MAC_McpsDcfmInd_s const * const psMcpsInd);

PRIVATE bool_t _radio_filter(uint8 msgtype, uint16 seqnum, uint16 srcaddr, uint16 srcpan);

PRIVATE void _reply_search_cmd(uint16 dstAddr, uint8 type);
PRIVATE void _devece_card_response(LPBSS_card_data_t const* const pstCard,
                                    RADIO_DEV_CARD_LOC_T *pstLoc);
PRIVATE void _card_loc_packed(LPBSS_card_data_t* pstLocdata);

PRIVATE void _devece_card_loc_parser(MAC_RxFrameData_s const * const psFrame);
PRIVATE void _staff_card_loc_parser(MAC_RxFrameData_s const * const psFrame);
PRIVATE void _wrist_card_loc_parser(MAC_RxFrameData_s const * const psFrame,uint8 type);
PRIVATE void _mattess_alertor_parser(MAC_RxFrameData_s const * const psFrame);
PRIVATE void _sos_card_parser(MAC_RxFrameData_s const * const psFrame);
PRIVATE void _asset_card_parser(MAC_RxFrameData_s const * const psFrame);


PRIVATE void _devece_card_info_parser(MAC_RxFrameData_s const * const psFrame);
PRIVATE void _card_version_parser(MAC_RxFrameData_s const * const psFrame);

/****************************************************************************/
/***        Local Variables                                               ***/
/****************************************************************************/
/* Handles from the MAC */
PRIVATE void* s_pvMac;
PRIVATE MAC_Pib_s* s_psMacPib;
PRIVATE MAC_ExtAddr_s psMacAddr;

/****************************************************************************/
/***        Exported Variables                                            ***/
/****************************************************************************/
static uint8 u8BusCardLocBuf[CARD_LOC_BUFF_LEN];
card_loc_t g_st485LocBuf =
{
    .u16LimitLen = CARD_LOC_BUFF_LEN,
    .u8CardLocBuf = u8BusCardLocBuf
};

static uint8 u8BusCardInfoBuf[CARD_INFO_BUFF_LEN];
card_info_t g_st485InfoBuf =
{
    .u16LimitLen = CARD_INFO_BUFF_LEN,
    .u8CardInfoBuf = u8BusCardInfoBuf
};

static uint8 u8BusCardVerBuf[CARD_VER_BUFF_LEN];
card_ver_t g_st485VerBuf =
{
    .u16LimitLen = CARD_VER_BUFF_LEN,
    .u8CardVerBuf = u8BusCardVerBuf
};

#ifdef USE_JMP_NET
static uint8 u8JmpCardInfoBuf[JMP_PAYLOAD_LEN];
static uint8 u8JmpCardVerBuf[JMP_PAYLOAD_LEN];
#endif

uint16 wrist_backoff_addr = NWK_BROADCAST_ADDR;

/****************************************************************************/
/***        Exported Functions                                            ***/
/****************************************************************************/

/****************************************************************************
 *
 * NAME: radio_check_cfg
 * Check channel and address setting
 *
 ****************************************************************************/
PRIVATE inline void radio_check_cfg(void)
{
    //channels check
    if (g_stSysMgr.u8SysChannel < 11 || g_stSysMgr.u8SysChannel > 25
        || !CARDREADER_IS_DEVID(g_stSysMgr.u16SysAddr))
    {
        while (1)
        {
            LedUtil_vOn(LED_RUN);
            LedUtil_vOn(LED_RF);
            LedUtil_vOn(LED_LINK);
        }
    }
}

PUBLIC void radio_cast(uint8 u8CmdType, uint16 u16DstAddr, uint16 u16DstPanId,
                        uint8 u8TxOptions, uint8* buf)
{
    MacUtil_SendParams_s sParams;
    RfWrapper_tu RfHeadType;

    g_stSysMgr.bMsgSending = TRUE;

    sParams.u8Radius = 1;
    sParams.u16DstAddr = u16DstAddr;
    sParams.u16DstPanId = u16DstPanId;
    sParams.u16ClusterId = 0;
    sParams.u8DeliverMode = MAC_UTIL_UNICAST;

    switch (u8CmdType)
    {
        case DEV_CARD_ACK:
        case DEV_CARD_SCH:
        case DEV_CARD_CLE:
        {
            RfHeadType.msg_head.u8MsgType = u8CmdType;
            g_stSysMgr.u8LastHandle = MacUtil_vSendData(&sParams,
                                        (uint8 *)&RfHeadType,
                                        sizeof(RfHeadType.msg_head.u8MsgType),
                                        u8TxOptions);
            break;
        }

        case WRIST_CARD_SMS:
        {
            RADIO_WRIST_SMS_T *pstSms;
            uint16 u16len;

            pstSms = (RADIO_WRIST_SMS_T*)(buf);
            u16len = sizeof(RADIO_WRIST_SMS_T) + pstSms->u8len;
            g_stSysMgr.u8LastHandle = MacUtil_vSendData(&sParams,
                                                        (uint8 *)buf,
                                                        u16len,
                                                        u8TxOptions);
        }
            break;
        case DEV_CARD_SET:
        {

            RADIO_DEV_INFO_T *p = (RADIO_DEV_INFO_T*) buf;
            g_stSysMgr.u8LastHandle = MacUtil_vSendData(&sParams,
                                                        (uint8 *)buf,
                                                        (p->u8Len + 5),
                                                        u8TxOptions);
            break;
        }

        case WRIST_CARD_TIME:
        {
            DBG_LOG("wrist set time, %d-%d-%d %d:%d:%d\n",
                g_stStationTime.u16Year,
                g_stStationTime.u8Month,
                g_stStationTime.u8Day,
                g_stStationTime.u8Hour,
                g_stStationTime.u8Minute,
                g_stStationTime.u8Second);
            RfHeadType.rf_wrist_time.u8MsgType = u8CmdType;
            RfHeadType.rf_wrist_time.u16Year = g_stStationTime.u16Year;
            RfHeadType.rf_wrist_time.u8Month = g_stStationTime.u8Month;
            RfHeadType.rf_wrist_time.u8Day = g_stStationTime.u8Day;
            RfHeadType.rf_wrist_time.u8Hour = g_stStationTime.u8Hour;
            RfHeadType.rf_wrist_time.u8Minute = g_stStationTime.u8Minute;
            RfHeadType.rf_wrist_time.u8Second = g_stStationTime.u8Second;


            g_stSysMgr.u8LastHandle = MacUtil_vSendData(&sParams,
                                                        (uint8 *)(&RfHeadType),
                                                        sizeof(RADIO_WRIST_TIME_T),
                                                        u8TxOptions);
            break;
        }
        default:
            break;
    }
}

PUBLIC void radio_search_timeout(uint32 u32CurMs)
{
    uint8 i;

    if (g_stSearchCardBuf.u8SearchCnt == 0)
        return;

    if (g_stSearchCardBuf.bRecordActive[CARD_CANCEL_IDX])
    {
        if (u32CurMs - g_stSearchCardBuf.u32RecordTime[CARD_CANCEL_IDX]
            >= SEARCH_TIMEOUT)
        {
            g_stSearchCardBuf.u8SearchCnt = 0;
            g_stSearchCardBuf.bRecordActive[CARD_CANCEL_IDX] = FALSE;
        }

        return;
    }

    for (i=0; i<CARD_SEARCH_MAX; i++)
    {
        if (i != CARD_CANCEL_IDX && g_stSearchCardBuf.bRecordActive[i]
            && u32CurMs - g_stSearchCardBuf.u32RecordTime[i] >= SEARCH_TIMEOUT)
        {
            g_stSearchCardBuf.u8SearchCnt--;
            g_stSearchCardBuf.bRecordActive[i] = FALSE;
        }
    }
}

PUBLIC void radio_cardinfo_timeout(uint32 u32CurMs)
{
    uint8 i;

    g_stSysMgr.u32LastScanCardInfo = u32CurMs;

    if (g_stSetCardBuf.u8CardCnt == 0)
        return;

    for (i=0; i<SET_CARD_INFO_NUM; i++)
    {
        if (g_stSetCardBuf.bRecordActive[i]
            && u32CurMs - g_stSetCardBuf.u32RecordTime[i] >= CARDINFO_TIMEOUT)
        {
            g_stSetCardBuf.bRecordActive[i] = FALSE;
            g_stSetCardBuf.u8CardCnt--;
        }
    }
}

PUBLIC void radio_extaddr_init(void)
{
    uint32 tmp32;
    uint8 channel;

    /*init address*/
    MacUtil_vReadExtAddress(&psMacAddr);

    // the first byte is 0xFF that  get error shortaddr
    tmp32 = StringUtil_BCD2uint32(psMacAddr.u32H & 0xfffff);

    g_stSysMgr.u16SysAddr = tmp32 % 100000;   //psMacAddr.u32L & 0xFFFF;
    g_stSysMgr.u16SysPanid = READ_STATION_PANID;

    g_stSysMgr.u8SysModel = ((psMacAddr.u32L) >> 24) & 0xFF;

    channel = ((psMacAddr.u32L) >> 16) & 0xFF;

    if(channel >= 11 && channel <= 26)
    {
        g_stSysMgr.u8SysChannel = channel;
    }
    else
    {
        g_stSysMgr.u8SysChannel = LPBSS_MAC_CHA_DEFAULT;
    }

    DBG_LOG("System Start! Addr: %d   Channel: %d \n",
            g_stSysMgr.u16SysAddr, g_stSysMgr.u8SysChannel);

    radio_check_cfg();
}

PUBLIC void radio_mac_init(void)
{
    /* Set up the MAC handles. Must be called AFTER u32AppQApiInit() */
    s_pvMac = pvAppApiGetMacHandle();
    s_psMacPib = MAC_psPibGetHandle(s_pvMac);

    /* Set Pan ID and short address in PIB (also sets match registers in hardware) */
    MAC_vPibSetPanId(s_pvMac, g_stSysMgr.u16SysPanid);
    MAC_vPibSetShortAddr(s_pvMac, g_stSysMgr.u16SysAddr);

    /* Enable receiver to be on when idle */
    MAC_vPibSetRxOnWhenIdle(s_pvMac, TRUE, FALSE);

    MAC_vPibSetRxOnWhenIdle(s_pvMac, TRUE, FALSE);
    MAC_vPibSetMaxCsmaBackoffs(s_pvMac, 2);
    MAC_vPibSetMinBe(s_pvMac, 1);

    s_psMacPib->bAssociationPermit = 0;
    s_psMacPib->bAutoRequest = 0;
    s_psMacPib->bGtsPermit = FALSE;
    //FIXME
    s_psMacPib->u8MaxFrameRetries = 1;

    MacUtil_Setting_s sMacUtil;
    sMacUtil.u16SrcPanId         = g_stSysMgr.u16SysPanid;
    sMacUtil.u16SrcShortAddr     = g_stSysMgr.u16SysAddr;

    // the following init are fake, because station is 802.15.4
    sMacUtil.u16Profile_id      = 0x2001; //for backward compatable
    sMacUtil.u8Dst_endpoint     = 0x21;
    sMacUtil.u8NodeType         = 0;
    sMacUtil.u8Src_endpoint     = 0;

    MacUtil_vInit(&sMacUtil);
}

/****************************************************************************
 *
 * NAME: radio_incoming_MLME
 *
 ****************************************************************************/
PUBLIC void radio_incoming_MLME(MAC_MlmeDcfmInd_s const* const psMlmeInd)
{
}

/****************************************************************************
 *
 * NAME: vProcessIncomingData
 *
 ****************************************************************************/
PUBLIC void radio_incoming_MCPS(MAC_McpsDcfmInd_s const * const psMcpsInd)
{
#ifndef USE_JMP_NET
    if (g_stSysMgr.u8StationState == E_STATE_STARTED)
#endif
    {
        switch(psMcpsInd->u8Type)
        {
        case MAC_MCPS_IND_DATA:
          /* Incoming data frame */
          _MCPS_data_ind(psMcpsInd);
          break;
        case MAC_MCPS_DCFM_DATA:
          /* Incoming acknowledgement or ack timeout */
          _MCPS_data_dcfm(psMcpsInd);
          break;
        default:
          break;
        }
    }
}

/****************************************************************************
 *
 * NAME: radio_start_coordinator
 *
 ****************************************************************************/
PUBLIC void radio_start_coordinator(void)
{
    /* Structures used to hold data for MLME request and response */
    MAC_MlmeReqRsp_s sMlmeReqRsp;
    MAC_MlmeSyncCfm_s sMlmeSyncCfm;

    sMlmeReqRsp.u8Type = MAC_MLME_REQ_START;
    sMlmeReqRsp.u8ParamLength = sizeof(MAC_MlmeReqStart_s);
    sMlmeReqRsp.uParam.sReqStart.u16PanId = g_stSysMgr.u16SysPanid;
    sMlmeReqRsp.uParam.sReqStart.u8Channel = g_stSysMgr.u8SysChannel;
    sMlmeReqRsp.uParam.sReqStart.u8BeaconOrder = 0x0F;
    sMlmeReqRsp.uParam.sReqStart.u8SuperframeOrder = 0x0F;
    sMlmeReqRsp.uParam.sReqStart.u8PanCoordinator = TRUE;
    sMlmeReqRsp.uParam.sReqStart.u8BatteryLifeExt = FALSE;
    sMlmeReqRsp.uParam.sReqStart.u8Realignment = FALSE;
    sMlmeReqRsp.uParam.sReqStart.u8SecurityEnable = FALSE;

    vAppApiMlmeRequest(&sMlmeReqRsp, &sMlmeSyncCfm);
}

PUBLIC void radio_stack_check(void)
{
    uint32 u32CurrentTimeMs = get_system_ms();

    // if over 1 min did not received any signal  from air, then reset,
    if(u32CurrentTimeMs -  g_stSysMgr.u32LastAirRxTimeMs > 300 * 1000)
    {
        DBG_ERR("No Air! %d %d\n ", g_stSysMgr.u32LastAirRxTimeMs,
                u32CurrentTimeMs);
        vAHI_SwReset();
    }
}

/****************************************************************************/
/***        Local Functions                                               ***/
/****************************************************************************/

/****************************************************************************
 *
 * NAME: _MCPS_data_dcfm
 *
 ****************************************************************************/
PRIVATE void _MCPS_data_dcfm(MAC_McpsDcfmInd_s const * const psMcpsInd)
{
    if (psMcpsInd->uParam.sDcfmData.u8Handle == g_stSysMgr.u8LastHandle)
    {
        g_stSysMgr.bMsgSending = FALSE;
    }
}

/****************************************************************************
 *
 * NAME: _MCPS_data_ind
 *
 ****************************************************************************/
PRIVATE void _MCPS_data_ind(MAC_McpsDcfmInd_s const * const psMcpsInd)
{
    MAC_RxFrameData_s const * psFrame = &(psMcpsInd->uParam.sIndData.sFrame);
    uint8 MsgType;
    uint16 seqnum;
    uint16 srcaddr = psFrame->sSrcAddr.uAddr.u16Short;
    uint16 srcpan = psFrame->sSrcAddr.u16PanId;

    /* msg from a card and other card reader */
    if (srcpan != DEVICE_CARD_PANID && srcpan != STAFF_CARD_PANID
        && srcpan != READ_STATION_PANID && srcpan != SOS_CARD_PANID)
    {
        return;
    }

    g_stSysMgr.u32LastAirRxTimeMs = get_system_ms();

    app_led_ontimer(LED_RF, 20);

    MsgType = *(uint8*)(psFrame->au8Sdu);
    seqnum = *(uint16*)(psFrame->au8Sdu + 2);
    CONVERT_ENDIAN(seqnum);

    switch (MsgType)
    {
    case STAFF_CARD_LOC:
        if (!_radio_filter(MsgType, seqnum, srcaddr, srcpan))
            _staff_card_loc_parser(psFrame);
        break;

#ifdef USE_JMP_NET
    case RADIO_JMP_PT:
        if (!_radio_filter(MsgType, seqnum, srcaddr, srcpan))
            jmp_rx_parser(psFrame);
        break;
#endif

    case WRIST_CARD_LOC:
        if (!_radio_filter(MsgType, seqnum, srcaddr, srcpan))
            _wrist_card_loc_parser(psFrame,WRIST_CARD_DEVICE_ID);
        break;
    case BADGE_CARD_LOC:
        if (!_radio_filter(MsgType, seqnum, srcaddr, srcpan))
            _wrist_card_loc_parser(psFrame,BADGE_DEVICE_ID);
        break;

    case MATTESS_ALERTOR_LOC:
        _mattess_alertor_parser(psFrame);
        break;

    case SOS_CARD_LOC:
        _sos_card_parser(psFrame);

        break;

    case ASSET_CARD_LOC:
        _asset_card_parser(psFrame);
        break;

    case DEV_CARD_LOC:
        if (!_radio_filter(MsgType, seqnum, srcaddr, srcpan))
            _devece_card_loc_parser(psFrame);
        break;

    case DEV_CARD_INFO:
        if (!_radio_filter(MsgType, seqnum, srcaddr, srcpan))
            _devece_card_info_parser(psFrame);
        break;

    case CARD_VERINFO:
        _card_version_parser(psFrame);
        break;

    case DEV_CARD_ACK:
        break;

    default:
        break;
    }
}

PUBLIC void radio_card_sms_pool(void)
{
    uint8 currentidx;
    uint32 cur_time;
    uint32 next_time;

REDO:
    if (!g_stSendSmsBuf.u8SmsCnt)
        return;

    cur_time = get_system_ms();
    next_time = TIMER_TICK_MAX;

    currentidx = g_stSendSmsBuf.u8PoolIdx;

    while (SMS_OVER == g_stSendSmsBuf.u8SmsState[currentidx])
    {
        currentidx++;

        if (CARD_SMS_MAX == currentidx)
            currentidx = 0;

        if (g_stSendSmsBuf.u8PoolIdx == currentidx)
            return;
    }

    g_stSendSmsBuf.u8PoolIdx = currentidx;

    if (SMS_READY == g_stSendSmsBuf.u8SmsState[currentidx])
    {
        g_stSendSmsBuf.u8SmsState[currentidx] = SMS_WAIT;
        g_stSendSmsBuf.u32RecordTime[currentidx] = cur_time + CARD_SMS_TIMEOUT;
    }
    if (cur_time - g_stSendSmsBuf.u32RecordTime[currentidx] < TIMER_TICK_MAX)
    {
        g_stSendSmsBuf.u8SmsState[currentidx] = SMS_OVER;
        g_stSendSmsBuf.u8SmsCnt--;
        goto REDO;
    }

    if (next_time > g_stSendSmsBuf.u32RecordTime[currentidx] - cur_time)
        next_time = g_stSendSmsBuf.u32RecordTime[currentidx] - cur_time;

    TimerUtil_eSetTimer(CARDREADER_SEND_SMS_EVENT, next_time);
}

PRIVATE bool _wrist_card_sms_find(uint16 src_addr)
{
    uint8 i;
    RADIO_WRIST_SMS_T *pstSms;
    uint16 dstaddr;

    if (!g_stSendSmsBuf.u8SmsCnt)
        return FALSE;

    for(i=0;i<CARD_SMS_MAX;i++)
    {
        if(g_stSendSmsBuf.u8SmsState[i] != SMS_OVER)
        {
            if((g_stSendSmsBuf.u16RecordAddress[i] == src_addr) || (g_stSendSmsBuf.u16RecordAddress[i] == NWK_BROADCAST_ADDR))
            {
                pstSms = (RADIO_WRIST_SMS_T*)(g_stSendSmsBuf.astCardSms[i]);
                radio_cast(WRIST_CARD_SMS, src_addr, STAFF_CARD_PANID, MAC_TX_OPTION_ACK, (void*)pstSms);
                //PrintfUtil_vPrintf("cast sms:%d\n",src_addr);
                return TRUE;
            }
        }
    }

    return FALSE;
}

PRIVATE bool _wrist_card_set_time(uint16 dstAddr)
{
    if(g_stStationTime.bRecordActive)
    {
        DBG_LOG("set time\n");
        radio_cast(WRIST_CARD_TIME, dstAddr, STAFF_CARD_PANID, 1, NULL);
    }

    return g_stStationTime.bRecordActive;
}

PUBLIC void radio_wrist_card_rsp(void)
{
    bool ret;

    ret = _wrist_card_sms_find(wrist_backoff_addr);

    if (!ret)
    {
        _wrist_card_set_time(wrist_backoff_addr);
    }

    wrist_backoff_addr = NWK_BROADCAST_ADDR;
}

PRIVATE bool_t _radio_filter(uint8 msgtype, uint16 seqnum, uint16 srcaddr, uint16 srcpan)
{
#if 0
    static uint8 PrevMsgType;
    static uint16 PrevSeqNum;
    static uint16 PrevSrcAddr;
    static uint16 PrevSrcPanId;

    if (PrevSrcAddr == srcaddr && seqnum < PrevMsgType + 1)
        return TRUE;

    if (PrevMsgType != msgtype || PrevSeqNum != seqnum || PrevSrcAddr != srcaddr
        || PrevSrcPanId != srcpan)
    {
        PrevMsgType = msgtype;
        PrevSeqNum = seqnum;
        PrevSrcAddr = srcaddr;
        PrevSrcPanId = srcpan;
        return FALSE;
    }

    return TRUE;
#else
    return FALSE;
#endif
}

PRIVATE void _reply_search_cmd(uint16 dstAddr, uint8 type)
{
    uint8 i;

    for(i=0; i<CARD_SEARCH_MAX; ++i)
    {
        switch (g_stSearchCardBuf.astCardSet[i].u8SearchType)
        {
        case LPBSS_SEARCH_CARD_BY_TYPE:

            if (type != g_stSearchCardBuf.astCardSet[i].CardSearch.CardSearchType.u8WorkType)
                continue;
            break;

        case LPBSS_SEARCH_CARD_BY_ADDR:
            if (dstAddr !=  g_stSearchCardBuf.astCardSet[i].CardSearch.u16DstAddr || (!g_stSearchCardBuf.bRecordActive[i]))
                continue;
            break;
        }

        DBG_LOG("search\n");
        CONVERT_ENDIAN(dstAddr);
        radio_cast(DEV_CARD_SCH, dstAddr, DEVICE_CARD_PANID, 1, NULL);
        return;
    }
}

PRIVATE void _devece_card_response(LPBSS_card_data_t const* const pstCard,
                                RADIO_DEV_CARD_LOC_T *pstLoc)
{
    uint8 i;
    uint16 u16DstAddr;

    DBG_JUDGE_RET(NULL == pstCard || NULL == pstLoc, "pointer is null\n");

     // set info to card
    for (i=0; i<SET_CARD_INFO_NUM; i++)
    {
        if (g_stSetCardBuf.bRecordActive[i]
            && g_stSetCardBuf.u16CardAddrList[i] == pstCard->u16ShortAddr)
        {
            DBG_LOG("send info\n");

            u16DstAddr = pstCard->u16ShortAddr;
            CONVERT_ENDIAN(u16DstAddr);

            radio_cast(DEV_CARD_SET, u16DstAddr,
                        DEVICE_CARD_PANID, 1,
                        (uint8*)g_stSetCardBuf.aau8CardSet[i]);
            break;
        }
    }

    // send search/cancel card cmd
    if (pstCard->u8IsCardAlarm
        && g_stSearchCardBuf.bRecordActive[CARD_CANCEL_IDX])
    {
        u16DstAddr = pstCard->u16ShortAddr;

        CONVERT_ENDIAN(u16DstAddr);

        DBG_LOG("shutup card %d\n", u16DstAddr);

        radio_cast(DEV_CARD_CLE, u16DstAddr, DEVICE_CARD_PANID, 1, NULL);
    }
    else if (g_stSearchCardBuf.u8SearchCnt > 0
            && !g_stSearchCardBuf.bRecordActive[CARD_CANCEL_IDX])
    {
        _reply_search_cmd(pstCard->u16ShortAddr, pstLoc->u8WorkType);
    }
}

PRIVATE void _card_loc_packed(LPBSS_card_data_t* pstLocdata)
{
    card_loc_t *pstDstBuf = &g_st485LocBuf;
    uint16 addr, seq;

    ASSERT_RET(pstLocdata != NULL);
    addr = pstLocdata->u16ShortAddr;
    seq = pstLocdata->u16Seqnum;
    CONVERT_ENDIAN(addr);
    CONVERT_ENDIAN(seq);
    DBG_LOG("src=%d, seq=%d\n", addr, seq);

#ifdef USE_JMP_NET
    if (jmp_type_get() != NET_TYPE_485)
    {
        jmp_data_packed(LPBSS_CARD_LOC_DATA, (uint8*)pstLocdata, sizeof(LPBSS_card_data_t));
        return;
    }
#endif
    if (pstDstBuf->u16CardLocDataLen + sizeof(LPBSS_card_data_t) > pstDstBuf->u16LimitLen)
    {
        pstDstBuf->u16CardLocDataLen = 0;
    }

    pstDstBuf->u16CardLocDataLen +=
        packed_card_loc(pstDstBuf->u8CardLocBuf + pstDstBuf->u16CardLocDataLen,
        (uint8*)pstLocdata, sizeof(LPBSS_card_data_t));
}

PRIVATE void _devece_card_loc_parser(MAC_RxFrameData_s const * const psFrame)
{
    LPBSS_card_data_t dev_card_data;
    RADIO_DEV_CARD_LOC_T* pDevRadio = (RADIO_DEV_CARD_LOC_T*)psFrame->au8Sdu;

    dev_card_data.u16ShortAddr = psFrame->sSrcAddr.uAddr.u16Short;
    dev_card_data.u16Seqnum = pDevRadio->u16Seqnum;
    dev_card_data.u8Device = DEVICE_CARD_DEVICE_ID;
    dev_card_data.Rssi = SysUtil_vConvertLQI2Dbm(psFrame->u8LinkQuality);
    dev_card_data.u8Battery = pDevRadio->u8Battery;
    dev_card_data.u8SoftVer = pDevRadio->u8SoftVer;
    dev_card_data.u8Status = pDevRadio->unStatus.u8Status;

    dev_card_data.u8IsCardAlarm = pDevRadio->unStatus.stStatus.u8IsSearch;

    dev_card_data.u8IsCardAlarm = pDevRadio->unStatus.u8Status & DEV_STATUS_SEARCH;
    dev_card_data.u8Model = pDevRadio->u8Model;
    dev_card_data.u8Padding = 0;

    CONVERT_ENDIAN(dev_card_data.u16ShortAddr);

    _devece_card_response(&dev_card_data, pDevRadio);

    _card_loc_packed(&dev_card_data);
}

PRIVATE void _staff_card_loc_parser(MAC_RxFrameData_s const * const psFrame)
{
    LPBSS_card_data_t staff_card_data;
    RADIO_STAFF_CARD_LOC_T* pStaffRadio = (RADIO_STAFF_CARD_LOC_T*)psFrame->au8Sdu;

		switch(psFrame->u8SduLength)
	{
		case 9:
		if(pStaffRadio->u16Crcsum != CRC16((uint8*)pStaffRadio, sizeof(RADIO_STAFF_CARD_LOC_T)-2,0xffff))
		return;
		break;
		case 7:
		break;
		default:
		return;
	}

    staff_card_data.u16ShortAddr = psFrame->sSrcAddr.uAddr.u16Short;
    staff_card_data.u16Seqnum = pStaffRadio->u16Seqnum;
    staff_card_data.u8Device = STAFF_CARD_DEVICE_ID;
    staff_card_data.Rssi = SysUtil_vConvertLQI2Dbm(psFrame->u8LinkQuality);
    staff_card_data.u8Battery = pStaffRadio->u8Battery;
    staff_card_data.u8SoftVer = pStaffRadio->u8SoftVer;
    staff_card_data.u8Status = pStaffRadio->u8Status;
    staff_card_data.u8IsCardAlarm = 0;
    staff_card_data.u8Model = pStaffRadio->u8Model;
    staff_card_data.u8Padding = 0;
    CONVERT_ENDIAN(staff_card_data.u16ShortAddr);

    _card_loc_packed(&staff_card_data);
}

PRIVATE void _wrist_card_loc_parser(MAC_RxFrameData_s const * const psFrame,uint8 type)
{
    LPBSS_card_data_t wrist_card_data;
    RADIO_STAFF_CARD_LOC_T* pStaffRadio = (RADIO_STAFF_CARD_LOC_T*)psFrame->au8Sdu;
	switch(psFrame->u8SduLength)
	{
		case 9:             //增加crc检验
		if(pStaffRadio->u16Crcsum != CRC16((uint8*)pStaffRadio, sizeof(RADIO_STAFF_CARD_LOC_T)-2,0xffff))
		return;
		break;
		case 7:            //兼容不带crc校验的腕带
		break;
		default:
		return;
	}

    wrist_card_data.u16ShortAddr = psFrame->sSrcAddr.uAddr.u16Short;
    wrist_card_data.u16Seqnum = pStaffRadio->u16Seqnum;
    wrist_card_data.u8Device = type;
    wrist_card_data.Rssi = SysUtil_vConvertLQI2Dbm(psFrame->u8LinkQuality);
    wrist_card_data.u8Battery = pStaffRadio->u8Battery;
    wrist_card_data.u8SoftVer = pStaffRadio->u8SoftVer;
    wrist_card_data.u8Status = pStaffRadio->u8Status;
    wrist_card_data.u8Model = pStaffRadio->u8Model;
    CONVERT_ENDIAN(wrist_card_data.u16ShortAddr);

    if ((WRIST_STATUS_POLL & pStaffRadio->u8Status)
        && (NWK_BROADCAST_ADDR == wrist_backoff_addr))
    {
        uint32 backoff;

        vAHI_StartRandomNumberGenerator(E_AHI_RND_SINGLE_SHOT, E_AHI_INTS_DISABLED);
        backoff = (u16AHI_ReadRandomNumber() & _5ms);
        wrist_backoff_addr = psFrame->sSrcAddr.uAddr.u16Short;
        //DBG_LOG("wrist backof %d\n", psFrame->sSrcAddr.uAddr.u16Short);
        TimerUtil_eSetTimer(CARDREADER_SEND_WRIST_BACKOF, backoff);
        //_wrist_card_sms_find(wrist_backoff_addr);
    }

    _card_loc_packed(&wrist_card_data);
}


PRIVATE void _mattess_alertor_parser(MAC_RxFrameData_s const * const psFrame)
{
    LPBSS_card_data_t mattess_alertor_data;
    MATTESS_ALERTOR_LOC_T* pStaffRadio = (MATTESS_ALERTOR_LOC_T*)psFrame->au8Sdu;

	switch(psFrame->u8SduLength)
	{
		case 8:             //crc检验
		CONVERT_ENDIAN(pStaffRadio->u16Crcsum);
		if(pStaffRadio->u16Crcsum != CRC16((uint8*)pStaffRadio, sizeof(MATTESS_ALERTOR_LOC_T)-2,0xffff))
		    return;
		break;
		default:
		return;
	}

    mattess_alertor_data.u16ShortAddr = psFrame->sSrcAddr.uAddr.u16Short;
    mattess_alertor_data.u16Seqnum = pStaffRadio->u16Seqnum;
    mattess_alertor_data.u8Device = MATESS_ALERTOR_DEVICE_ID;
    mattess_alertor_data.Rssi = SysUtil_vConvertLQI2Dbm(psFrame->u8LinkQuality);
    mattess_alertor_data.u8Battery = 0;
    mattess_alertor_data.u8SoftVer = pStaffRadio->u8SoftVer;
    mattess_alertor_data.u8Status = pStaffRadio->u8Status;
    mattess_alertor_data.u8Model = pStaffRadio->u8Model;
    CONVERT_ENDIAN(mattess_alertor_data.u16ShortAddr);

    _card_loc_packed(&mattess_alertor_data);
}

PRIVATE void _sos_card_parser(MAC_RxFrameData_s const * const psFrame)
{
    LPBSS_card_data_t sos_card_data;
    RADIO_STAFF_CARD_LOC_T* pStaffRadio = (RADIO_STAFF_CARD_LOC_T*)psFrame->au8Sdu;

    switch(psFrame->u8SduLength)
    {
        case 9:
        if(pStaffRadio->u16Crcsum != CRC16((uint8*)pStaffRadio, sizeof(RADIO_STAFF_CARD_LOC_T)-2,0xffff))
        {
            DBG_ERR("CRC err:%d,%d\n ", pStaffRadio->u16Crcsum,CRC16((uint8*)pStaffRadio, sizeof(RADIO_STAFF_CARD_LOC_T)-2,0xffff));
            return;
        }
        break;
        case 7:
        break;
        default:
        return;
    }

    sos_card_data.u16ShortAddr = psFrame->sSrcAddr.uAddr.u16Short;
    sos_card_data.u16Seqnum = pStaffRadio->u16Seqnum;
    sos_card_data.u8Device = SOS_DEVICE_ID;
    sos_card_data.Rssi = SysUtil_vConvertLQI2Dbm(psFrame->u8LinkQuality);
    sos_card_data.u8Battery = pStaffRadio->u8Battery;
    sos_card_data.u8SoftVer = pStaffRadio->u8SoftVer;
    sos_card_data.u8Status = pStaffRadio->u8Status;
    sos_card_data.u8IsCardAlarm = 0;
    sos_card_data.u8Model = pStaffRadio->u8Model;
    sos_card_data.u8Padding = 0;
    CONVERT_ENDIAN(sos_card_data.u16ShortAddr);

    _card_loc_packed(&sos_card_data);
}

PRIVATE void _asset_card_parser(MAC_RxFrameData_s const * const psFrame)
{
    ASSET_card_data_t asset_card_data;
    RADIO_ASSET_CARD_LOC_T* pAssetRadio = (RADIO_STAFF_CARD_LOC_T*)psFrame->au8Sdu;

    if(pAssetRadio->u16Crcsum !=  CRC16((uint8*)pAssetRadio, sizeof(RADIO_ASSET_CARD_LOC_T)-2,0xffff))
    {
        DBG_ERR("CRC err:%d,%d\n ", pAssetRadio->u16Crcsum,CRC16((uint8*)pAssetRadio, sizeof(RADIO_ASSET_CARD_LOC_T)-2,0xffff));
        return;
    }

    asset_card_data.u16ShortAddr = psFrame->sSrcAddr.uAddr.u16Short;
    asset_card_data.u16Seqnum = pAssetRadio->u16Seqnum;
    asset_card_data.u8Device = ASSET_CARD_DEVICE_ID;
    asset_card_data.Rssi = SysUtil_vConvertLQI2Dbm(psFrame->u8LinkQuality);
    asset_card_data.u8Battery = pAssetRadio->u8Battery;
    //sos_card_data.u8SoftVer = pStaffRadio->u8SoftVer;
    asset_card_data.u8Status = pAssetRadio->u8Status;
    asset_card_data.u8IsCardAlarm = 0;
    asset_card_data.u8Model = pAssetRadio->u8Model;
    asset_card_data.u16ExciteID = pAssetRadio->u16ExciterID;

    //DBG_ERR("CARD:%d,%d\n ", asset_card_data.u16ShortAddr,pAssetRadio->u16ExciterID);
    CONVERT_ENDIAN(asset_card_data.u16ShortAddr);
    _card_loc_packed(&asset_card_data);


    //DBG_ERR("CRC err:%d,%d\n ", pAssetRadio->u16Crcsum,CRC16((uint8*)pAssetRadio, sizeof(RADIO_ASSET_CARD_LOC_T)-2,0xffff));
}


PRIVATE void _devece_card_info_parser(MAC_RxFrameData_s const * const psFrame)
{
    card_info_t *pstDstBuf = &g_st485InfoBuf;
    uint16 u16Size;

    radio_cast(DEV_CARD_ACK, psFrame->sSrcAddr.uAddr.u16Short,
        DEVICE_CARD_PANID, 1, NULL);

#ifdef USE_JMP_NET
    if (jmp_type_get() != NET_TYPE_485)
    {
        u16Size = packed_card_info(psFrame, u8JmpCardInfoBuf, JMP_PAYLOAD_LEN);
        jmp_data_packed(LPBSS_DEV_INFO, u8JmpCardInfoBuf, u16Size);
        return;
    }
#endif

    pstDstBuf->u16CardInfoLen += packed_card_info(psFrame,
        pstDstBuf->u8CardInfoBuf + pstDstBuf->u16CardInfoLen,
        pstDstBuf->u16LimitLen - pstDstBuf->u16CardInfoLen);
}

PRIVATE void _card_version_parser(MAC_RxFrameData_s const * const psFrame)
{
    CARD_VERSION_INFO_T* pstCardVer = (CARD_VERSION_INFO_T*)(psFrame->au8Sdu);
    card_ver_t *pstDstBuf = &g_st485VerBuf;
    uint16 u16Size;

    u16Size = sizeof(LPBSS_card_ver_t) + pstCardVer->u8VerInfoLen;

#ifdef USE_JMP_NET
    if (jmp_type_get() != NET_TYPE_485)
    {
        u16Size = packed_card_ver(psFrame, u8JmpCardVerBuf, JMP_PAYLOAD_LEN);
        jmp_data_packed(LPBSS_CARD_VERSION, u8JmpCardVerBuf, u16Size);
        return;
    }
#endif

    if (pstDstBuf->u16LimitLen < (pstDstBuf->u16CardVerLen + u16Size))
    {
        if (u16Size > pstDstBuf->u16LimitLen)
        {
            DBG_WARN("version length=%d error, lose version\n", u16Size);
            return;
        }

        pstDstBuf->u16CardVerLen = 0;
    }

    pstDstBuf->u16CardVerLen += packed_card_ver(psFrame,
        pstDstBuf->u8CardVerBuf + pstDstBuf->u16CardVerLen,
        pstDstBuf->u16LimitLen - pstDstBuf->u16CardVerLen);
}

