/**************************************************************************************************
  Filename:       GroundCardReadr.c
  Revised:        $Date: 2010/05/27 22:07:57 $
  Revision:       $Revision: 1.1 $
****************************************************************************/

/****************************************************************************/
/***        Include files                                                 ***/
/****************************************************************************/
#include <string.h>
#include <jendefs.h>
#include <AppHardwareApi.h>
#include <AppQueueApi.h>
#include <mac_sap.h>
#include <mac_pib.h>
#include <Utilities.h>
#include <OAD.h>

#include "JN5148_util.h"
#include "system_util.h"
#include "ground_crsys.h"

/****************************************************************************/
/***        Macro Definitions                                             ***/
/****************************************************************************/

/****************************************************************************/
/***        Type Definitions                                              ***/
/****************************************************************************/

typedef void (*RADIO_MSG_PARSER)(MAC_RxFrameData_s* psFrame);

/****************************************************************************/
/***        Local Function Prototypes                                     ***/
/****************************************************************************/
PRIVATE inline void _led_init(void);
PRIVATE inline void _sys_init(void);
PRIVATE inline void _debug_init(void);

PRIVATE inline void _application_init(void);
PRIVATE void _sys_led_toggle(void);

PRIVATE void _sync_netlink(uint32);
PRIVATE void _system_feed_watchdog(uint32);
PRIVATE void _event_systick_proc(void);

PRIVATE void _app_appsys_poll(void);
PRIVATE void _app_init_system(void);
PRIVATE void _app_system_events(void);
PRIVATE void _app_application_events(void);
PRIVATE void _app_incoming_hw_events(AppQApiHwInd_s*);

/****************************************************************************/
/***        Exported Variables                                            ***/
/****************************************************************************/
PUBLIC card_search_t g_stSearchCardBuf;
PUBLIC card_set_t g_stSetCardBuf;

PUBLIC SYS_MGR_T g_stSysMgr;
PUBLIC card_time_t g_stStationTime;
PUBLIC card_sms_t g_stSendSmsBuf;
PRIVATE uint16 u16LastMsgSeq = 0;

/****************************************************************************/
/***        Local Variables                                               ***/
/****************************************************************************/
const uint8 baudrate_list[] =
{
    UART_BAUDRATE_460800,
    UART_BAUDRATE_115200,
    UART_BAUDRATE_38400,
    UART_BAUDRATE_9600
};

/****************************************************************************/
/***        Local Functions                                               ***/
/****************************************************************************/
PRIVATE inline void _sys_init(void)
{
    (void) u32AppQApiInit(NULL, NULL, NULL); //vHwDeviceIntCallback);
    (void) u32AHI_Init();

    // vOADInitClient(TRUE);

    // Enable high power modules, tof function, timerUtil
    vAHI_HighPowerModuleEnable(TRUE, TRUE);

    vAHI_WakeTimerEnable(E_AHI_WAKE_TIMER_0, TRUE);

    /* initial Flash */
    bAHI_FlashInit(E_FL_CHIP_ST_M25P40_A, NULL);
}

PRIVATE inline void _led_init(void)
{
    LedUtil_bRegister(LED_RF);
    LedUtil_bRegister(LED_LINK);
    LedUtil_bRegister(LED_RUN);
    LedUtil_bRegister(HARDWARE_DOG);

    vAHI_DioSetDirection(0, HARDWARE_DOG);
    //Defaule off
    vAHI_DioSetOutput(HARDWARE_DOG, 0);

    ErrorUtil_vRegisterLed0(LED_RF);
    ErrorUtil_vRegisterLed1(LED_LINK);
    ErrorUtil_vRegisterLed2(LED_RUN);
}

PRIVATE inline void _debug_init(void)
{
#if (defined _GROUND_DEBUG_ || defined DEBUG_ERROR)
    PrintfUtil_u8Init(E_AHI_UART_0, PRINTF_BAUD_RATE_1M);
#endif
}

PRIVATE inline void _application_init(void)
{
    g_stSysMgr.u8StationState = E_STATE_IDLE;
    g_stSysMgr.u32LastAirRxTimeMs = get_system_ms();

    EventUtil_vSetEvent(SYSTEM_TICK_EVENT);
    TimerUtil_eSetCircleTimer(SYSTEM_TICK_EVENT, _10ms);
    EventUtil_vSetEvent(CARDREADER_LED_TOGGLE_EVENT);
}

PRIVATE void _sys_led_toggle(void)
{
    if (g_stSysMgr.u8StationState == E_STATE_IDLE
#ifdef USE_JMP_NET
        && jmp_state_get() != NET_STATE_CN
#endif
        )
    {
        LedUtil_vToggle(LED_LINK);
        LedUtil_vToggle(LED_RUN);
        LedUtil_vToggle(LED_RF);
        TimerUtil_eSetTimer(CARDREADER_LED_TOGGLE_EVENT, 500);
    }
    else
    {
        LedUtil_vOn(LED_RUN);
        LedUtil_vOff(LED_RF);
        LedUtil_vOff(LED_LINK);
    }

    DBG_LOG("toggle led, type = %d, state=%d\n", jmp_type_get(), jmp_state_get());
}

PRIVATE void _sync_netlink(uint32 u32CurMs)
{
    static uint8 baudrate_idx = 1;

    g_stSysMgr.u32LastSyncBus485 = u32CurMs;

    if (g_stSysMgr.u8StationState == E_STATE_IDLE)
    {
        DBG_LOG("baudrate_idx:%d\n", baudrate_idx);
        bus485_setbuaudrate(baudrate_list[baudrate_idx++]);

        if(baudrate_idx >= sizeof(baudrate_list) / sizeof(baudrate_list[0]))
            baudrate_idx = 0;
    }
    else if(g_stSysMgr.u8StationState == E_STATE_STARTED)
    {
        if(g_stSysMgr.u8LoseLinkCnt++ >= 5)
        {
            g_stSysMgr.u8LoseLinkCnt = 0;
            g_stSysMgr.u8StationState = E_STATE_IDLE;

            // 需要尽快同步，所以调整更新时间
            g_stSysMgr.u32LastSyncBus485 -= BUS485_LINK_SYNC_CYCLE;

            EventUtil_vSetEvent(CARDREADER_LED_TOGGLE_EVENT);
        #ifdef USE_JMP_NET
            jmp_type_change(NET_TYPE_JMP);
        #endif
        }
    }
}

PRIVATE void _system_feed_watchdog(uint32 u32CurMs)
{
    static uint8 errorCount = 0;

    g_stSysMgr.u32LastFeedDog = u32CurMs;

    vAHI_WatchdogRestart();

    if (bus485_is_tx())
    {
        if (g_stSysMgr.u8StationState == E_STATE_IDLE)
        {
            DBG_ERR("485 control error1!\n");
            vAHI_SwReset();
        }
        else if (g_stSysMgr.u8StationState == E_STATE_STARTED)
        {
            errorCount = 0;
        }
    }
    else
    {
        if (E_AHI_DIO11_INT & u32AHI_DioReadInput())
        {
            DBG_ERR("485 control error3!\n");

            if (errorCount ++ > 5)
            {
                errorCount = 0;
                vAHI_SwReset();
            }
        }
    }

    //just  use the API ,not LED
    LedUtil_vToggle(HARDWARE_DOG);
}

PRIVATE void _event_systick_proc(void)
{
    uint32 u32CurMs = get_system_ms();

    if (u32CurMs - g_stSysMgr.u32LastFeedDog >= _500ms)
    {
        _system_feed_watchdog(u32CurMs);
    }

    if (u32CurMs - g_stSysMgr.u32LastScanCardInfo >= _1S)
    {
        radio_search_timeout(u32CurMs);
        radio_cardinfo_timeout(u32CurMs);
    }

    if (u32CurMs - g_stSysMgr.u32LastSyncBus485 >= BUS485_LINK_SYNC_CYCLE)
    {
        _sync_netlink(u32CurMs);
    }
}

PRIVATE void _app_appsys_poll(void)
{
    TimerUtil_vUpdate();
    radio_stack_check();
}

/****************************************************************************
 *
 * NAME: _app_init_system
 *
 ****************************************************************************/
PRIVATE void _app_init_system(void)
{
    _sys_init();

    _led_init();

    _debug_init();

    TimerUtil_vInit();
    init_system_ms();

    radio_extaddr_init();

    radio_mac_init();

    bus485_init();
#ifdef USE_JMP_NET
    jmp_Init();
#endif
    _application_init();

    radio_start_coordinator();
}

/****************************************************************************
 *
 * NAME: _app_system_events
 *
 ****************************************************************************/
PRIVATE void _app_system_events(void)
{
    MAC_MlmeDcfmInd_s* psMlmeInd;
    MAC_McpsDcfmInd_s* psMcpsInd;
    AppQApiHwInd_s* psAHI_Ind;

    do
    {
        psMcpsInd = psAppQApiReadMcpsInd();
        if (psMcpsInd != NULL)
        {
            radio_incoming_MCPS(psMcpsInd);
            vAppQApiReturnMcpsIndBuffer(psMcpsInd);
        }
    }
    while (psMcpsInd != NULL);

    do
    {
        psMlmeInd = psAppQApiReadMlmeInd();
        if(psMlmeInd != NULL)
        {
            radio_incoming_MLME(psMlmeInd);
            vAppQApiReturnMlmeIndBuffer(psMlmeInd);
        }
    }
    while (psMlmeInd != NULL);

    do
    {
        psAHI_Ind = psAppQApiReadHwInd();
        if(psAHI_Ind != NULL)
        {
            _app_incoming_hw_events(psAHI_Ind);
            vAppQApiReturnHwIndBuffer(psAHI_Ind);
        }
    }
    while (psAHI_Ind != NULL);
}

/****************************************************************************
 *
 * NAME: _app_application_events
 *
 ****************************************************************************/
PRIVATE void _app_application_events(void)
{
    uint32 event = EventUtil_u32ReadEvents();
    uint8 count = 0;

    while (event && count++ < 32)
    {
    #ifdef USE_JMP_NET
        event = jmp_event_process(event);
    #endif
        event = bus485_event_process(event);

        switch (event)
        {
        case SYSTEM_TICK_EVENT:
            _event_systick_proc();
            EventUtil_vUnsetEvent(event);
            break;

        case CARDREADER_OFF_LED_LINK_EVENT:
            LedUtil_vOff(LED_LINK);
            EventUtil_vUnsetEvent(event);
            break;

        case CARDREADER_OFF_LED_RF_EVENT:
            LedUtil_vOff(LED_RF);
            EventUtil_vUnsetEvent(event);
            break;

        case CARDREADER_LED_TOGGLE_EVENT:
            _sys_led_toggle();
            EventUtil_vUnsetEvent(event);
            break;

        case CARDREADER_STETIME_TIMEOUT:
            g_stStationTime.bRecordActive = FALSE;
            EventUtil_vUnsetEvent(event);
            break;

        case CARDREADER_SEND_SMS_EVENT:
            radio_card_sms_pool();
            EventUtil_vUnsetEvent(event);
            break;

        case CARDREADER_SEND_WRIST_BACKOF:
            radio_wrist_card_rsp();
            EventUtil_vUnsetEvent(event);
            break;

        default:
            break;
        }

        event = EventUtil_u32ReadEvents();
    }
}

/****************************************************************************
 *
 * NAME: _app_incoming_hw_events
 *
 ****************************************************************************/
PRIVATE void _app_incoming_hw_events(AppQApiHwInd_s* psAHI_Ind)
{
}

/****************************************************************************/
/***        Exported Functions                                            ***/
/****************************************************************************/
PUBLIC uint16 app_get_sysver(uint8 * const pu8Buf, const uint16 size)
{
#define CHAN ", Ch   "
    LPBSS_Msg_Header_t* pstHdr;
    uint16 dataLen = strlen(VERSION) + strlen(CHAN) + 1;
    uint8 num10 = ' ', num01 = ' ';

    num10 = g_stSysMgr.u8SysChannel / 10 + '0';
    num01 = g_stSysMgr.u8SysChannel % 10 + '0';

    ASSERT_RETV(size != 0, 0);

    if (size < sizeof(LPBSS_Msg_Header_t))
        return 0;

    if ((get_system_ms() - g_stSysMgr.u32LastSyncVer >= TIMER_TICK_MAX))
        return 0;

    if (size - sizeof(LPBSS_Msg_Header_t) < dataLen)
    {
        dataLen = size - sizeof(LPBSS_Msg_Header_t);
    }

    // note:version
    g_stSysMgr.u32LastSyncVer = get_system_ms() + SYS_VER_REPORT_CYCLE;

    pstHdr = (LPBSS_Msg_Header_t*)pu8Buf;
    pstHdr->u8Msg = LPBSS_VERSION;
    pstHdr->u16len = dataLen;

    // strcpy((uint8*)(pver + 1), VERSION);
    memcpy((uint8*)(pstHdr + 1), VERSION, strlen(VERSION));
    memcpy((uint8*)(pstHdr + 1) + strlen(VERSION), CHAN, strlen(CHAN)+1);
    pu8Buf[sizeof(LPBSS_Msg_Header_t) + pstHdr->u16len - 3] = num10;
    pu8Buf[sizeof(LPBSS_Msg_Header_t) + pstHdr->u16len - 2] = num01;

    CONVERT_ENDIAN(pstHdr->u16len);

    return dataLen + sizeof(LPBSS_Msg_Header_t);
}

PUBLIC void app_cmd_search(LPBSS_Msg_Header_t *pstMsg, uint16 size)
{
    LPBSS_card_search_record_t* psearch;
    uint8 cnt, i;
    uint32 u32CurTime;

    ASSERT_RET(pstMsg != NULL);

    ASSERT_RET(size >= sizeof(LPBSS_Msg_Header_t) + sizeof(LPBSS_card_search_record_t));

    psearch = (LPBSS_card_search_record_t*)(pstMsg + 1);

    if (psearch->u8Cmdtype == LPBSS_SEARCH_CARD_CANCEL_ALERT)
    {
        DBG_LOG("ready shutup\n");
        g_stSearchCardBuf.u8SearchCnt = 1;
        g_stSearchCardBuf.bRecordActive[CARD_CANCEL_IDX] = TRUE;
        g_stSearchCardBuf.u32RecordTime[CARD_CANCEL_IDX] = get_system_ms();
        g_stSearchCardBuf.astCardSet[CARD_CANCEL_IDX] = *psearch;

        return;
    }

    if (g_stSearchCardBuf.bRecordActive[CARD_CANCEL_IDX])
    {
        return;
    }

    //DBG_ERR("ready search\n");

    CONVERT_ENDIAN(pstMsg->u16len);
    cnt = pstMsg->u16len / sizeof(LPBSS_card_search_record_t);

    if (cnt + g_stSearchCardBuf.u8SearchCnt > CARD_SEARCH_MAX)
        cnt = CARD_SEARCH_MAX - g_stSearchCardBuf.u8SearchCnt;

    u32CurTime = get_system_ms();

    for (i=0; i<CARD_SEARCH_MAX && cnt>0; i++)
    {
        if (i != CARD_CANCEL_IDX && !g_stSearchCardBuf.bRecordActive[i])
        {
            g_stSearchCardBuf.astCardSet[i] = *psearch++;
            g_stSearchCardBuf.u32RecordTime[i] = u32CurTime;
            g_stSearchCardBuf.bRecordActive[i] = TRUE;
            g_stSearchCardBuf.u8SearchCnt++;
            cnt--;
        }
    }
}

PUBLIC void app_cmd_setinfo(LPBSS_Msg_Header_t *pstMsg, uint16 size)
{
    uint8 i;
    uint8 pool;
    RADIO_DEV_INFO_T* p;
    LPBSS_devicecard_info_t *pstInfo = (LPBSS_devicecard_info_t*)(pstMsg + 1);

    ASSERT_RET(pstMsg != NULL);
    ASSERT_RET(size > sizeof(LPBSS_Msg_Header_t) + sizeof(LPBSS_devicecard_info_t));

    DBG_LOG("ready setinfo\n");

    if (pstInfo->DevCardInfo.u8Len + sizeof(LPBSS_devicecard_info_t)
        > BUS485_MAX_RX_LEN)
    {
        DBG_WARN("CardInfo's length too long\n");
        return;
    }

    if (g_stSetCardBuf.u8CardCnt == SET_CARD_INFO_NUM)
    {
        DBG_WARN("set info buffer full, lose a packet\n");
        return;
    }

    pool = SET_CARD_INFO_NUM;

    for (i=0; i<SET_CARD_INFO_NUM; i++)
    {
        if (!g_stSetCardBuf.bRecordActive[i])
        {
            pool = i;
        }
        else if (g_stSetCardBuf.u16CardAddrList[i] == pstInfo->u16ShortAddr)
        {
            pool = i;
            break;
        }
    }

    if (pool == SET_CARD_INFO_NUM)
    {
        DBG_WARN("set info buffer full, lose a packet\n");
        return;
    }

    p = (RADIO_DEV_INFO_T*)g_stSetCardBuf.aau8CardSet[pool];
    g_stSetCardBuf.u16CardAddrList[pool] = pstInfo->u16ShortAddr;
    g_stSetCardBuf.bRecordActive[pool] = TRUE;
    g_stSetCardBuf.u32RecordTime[pool] = get_system_ms();

    p->u8MsgType = DEV_CARD_SET;
    p->u8IsChange = TRUE;
    p->u16Seqnum = pstInfo->u16Seqnum;
    p->u8Len     = pstInfo->DevCardInfo.u8Len + 3;//worktype + sizeof(lenth)
    p->stPayload.stBasicInfo.u8WorkType = pstInfo->DevCardInfo.u8WorkType;
    p->stPayload.stDescInfo.u16Len = pstInfo->DevCardInfo.u8Len;

    CONVERT_ENDIAN(p->stPayload.stDescInfo.u16Len);

    memcpy(((uint8*)g_stSetCardBuf.aau8CardSet[pool]) + 8,
            pstInfo + 1,
            pstInfo->DevCardInfo.u8Len);

    g_stSetCardBuf.u8CardCnt++;
}

PUBLIC void app_cmd_setime(LPBSS_Msg_Header_t *pstMsg, uint16 size)
{
    LPBSS_card_time_t *pstTime = (LPBSS_card_time_t*)(pstMsg + 1);

    ASSERT_RET(size >= sizeof(LPBSS_Msg_Header_t) + sizeof(LPBSS_card_time_t));

    g_stStationTime.bRecordActive = TRUE;
    g_stStationTime.u16Year  = pstTime->year;
    g_stStationTime.u8Month = pstTime->month;
    g_stStationTime.u8Day   = pstTime->day;
    g_stStationTime.u8Hour  = pstTime->hour;
    g_stStationTime.u8Minute = pstTime->minute;
    g_stStationTime.u8Second = pstTime->second;
    EventUtil_vUnsetEvent(CARDREADER_STETIME_TIMEOUT);
    TimerUtil_eSetTimer(CARDREADER_STETIME_TIMEOUT, TIME_SERVICE_TIMEOUT);

    DBG_LOG("485 recv set time\n");
}

PUBLIC void app_cmd_send_sms(LPBSS_Msg_Header_t *pstMsg, uint16 size)
{
    LPBSS_SMS_Header_t *pstSms = (LPBSS_SMS_Header_t*)(pstMsg + 1);
    RADIO_WRIST_SMS_T wrist_sms;
    uint8 *p;
    uint8 idx;
    uint16 len = 0;

#undef MSG_HDR_SIZE
#define MSG_HDR_SIZE (sizeof(LPBSS_Msg_Header_t) + sizeof(LPBSS_SMS_Header_t))

    ASSERT_RET(pstMsg != NULL && size > MSG_HDR_SIZE);
    ASSERT_RET(size - MSG_HDR_SIZE <= CARD_SMS_SIZE);

    idx = g_stSendSmsBuf.u8FreeIdx;

    if(u16LastMsgSeq == pstSms->u16seqnum)
    {
        return;
    }
    else
    {
        g_stSendSmsBuf.u8SmsCnt++;
        u16LastMsgSeq = pstSms->u16seqnum;
    }

    if (0 == pstSms->u8dstType)
    {
        g_stSendSmsBuf.u16RecordAddress[idx] = NWK_BROADCAST_ADDR;
    }
    else
    {
        CONVERT_ENDIAN(pstSms->u16dstID);
        g_stSendSmsBuf.u16RecordAddress[idx] = pstSms->u16dstID;
    }

    /* start time on quere head */
    // g_stSendSmsBuf.u32RecordTime[idx] = get_system_ms() + CARD_SMS_TIMEOUT;
    g_stSendSmsBuf.u8SmsState[idx] = SMS_READY;
    wrist_sms.u8MsgType = WRIST_CARD_SMS;
    wrist_sms.u16seqnum = pstSms->u16seqnum;

    len = pstSms->u16len;
    CONVERT_ENDIAN(len);
    wrist_sms.u8len = (0xFF & len);

    p = &g_stSendSmsBuf.astCardSms[idx][0];
    memcpy(p, &wrist_sms, sizeof(wrist_sms));

    p += sizeof(wrist_sms);
    memcpy(p, pstSms+1, wrist_sms.u8len);

    g_stSendSmsBuf.u8FreeIdx++;

    if (CARD_SMS_MAX == g_stSendSmsBuf.u8FreeIdx)
        g_stSendSmsBuf.u8FreeIdx = 0;

    EventUtil_vSetEvent(CARDREADER_SEND_SMS_EVENT);

    //DBG_ERR("send sms\n");
}


PUBLIC void app_packet_parser(LPBSS_Msg_Header_t *pstMsg, uint16 size)
{
    DBG_JUDGE_RET(!pstMsg || size < sizeof(LPBSS_Msg_Header_t),
        "packet size error:%d\n", size);

    switch (pstMsg->u8Msg)
    {
    case LPBSS_DEV_SET:
        app_cmd_setinfo(pstMsg, size);
        break;

    case LPBSS_CARD_TIME:
        app_cmd_setime(pstMsg, size);
        break;

    case LPBSS_DEV_CARD_SCH:
        app_cmd_search(pstMsg, size);
        break;

    case LPBSS_CARD_SMS:
        app_cmd_send_sms(pstMsg, size);
        break;

#ifdef USE_JMP_NET
    case LPBSS_JMP_TOPO:
        jmp_start_topo_proc();
        break;
#endif

    default:
        return;
    }
}

PUBLIC void app_led_ontimer(uint32 u32LedDIO, uint32 u32Ms)
{
    LedUtil_vOn(u32LedDIO);

    if(u32LedDIO == LED_LINK)
    {
        TimerUtil_eSetTimer(CARDREADER_OFF_LED_LINK_EVENT, u32Ms);
    }
    else
    {
        TimerUtil_eSetTimer(CARDREADER_OFF_LED_RF_EVENT, u32Ms);
    }
}

/****************************************************************************
 *
 * NAME: AppColdStart
 *
 ****************************************************************************/
PUBLIC void AppColdStart(void)
{
    _app_init_system();

    while (1)
    {
        _app_appsys_poll();
        _app_system_events();
        _app_application_events();
    }
}

/****************************************************************************
 *
 * NAME: AppWarmStart
 *
 ****************************************************************************/
PUBLIC void AppWarmStart(void)
{
    AppColdStart();
}

/****************************************************************************/
/***        END OF FILE                                                   ***/
/****************************************************************************/

