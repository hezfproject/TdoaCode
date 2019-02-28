/*******************************************************************************
  Filename:     app_card.c

  Description:  应用程序主文件

*******************************************************************************/

/*******************************************************************************
* INCLUDES
*/
#include <rf.h>
#include <bsp.h>
#include <bsp_led.h>
#include <bsp_key.h>
#include <bsp_beep.h>
#include <dev_card.h>
#include <RadioProto.h>
#include <lpbssnwk.h>
#include <string.h>
#include <mem.h>
#include <timer_event.h>
#include <track.h>
#include "app_card_wireless.h"
#include <hal_mcu.h>
#include "app_flash.h"
#include "version.h"
#include "hal_adc.h"
/*******************************************************************************
* CONSTANTS
*/
#ifndef DEV_CARD_PROJ
#error "not define DEV_CARD_PROJ"
#endif

#ifdef USE_HEAP
#define HEAP_SIZE       (4 * 1024)
UINT8 heap_pool[HEAP_SIZE];

#define CC2530_HEAP_BEGIN   (VOID *)(heap_pool)
#define CC2530_HEAP_END     (VOID *)(&heap_pool[HEAP_SIZE])
#endif

const LED_E CARD_LED_STATE[4] = {LED_RED, LED_RED, LED_ALL, LED_GREEN};

uint16 endDevID = 0xFFFF;   //卡自己的ID
uint16 midDevID = 0xFFFF;   //卡座ID
LPBSS_device_ID_e endDevType;  //卡自己是设备卡还是人员卡
RADIO_DEV_CARD_LOC_T     s_stAppTxPkt;

extern uint8 recframe[];//130
extern uint8 sendframe[];
extern uint8 sendframeLen;
extern LF_CARD_STATE_E cardState;
extern APP_WORK_STATE_T         s_stAppWork;
extern uint8 LF_modified;  //激励修改信息标志

/*******************************************************************************
* LOCAL FUNCTIONS DECLARATION
*/
static VOID app_Report(UINT16 dstaddr, UINT16 dstpanid);

static VOID app_TransmitInit(VOID);

static UINT16 app_BCDToDec(UINT8 u8Byte);

static UINT16 app_CheckIEEEInfo(UINT8 *pu8IEEEAddr);


/*******************************************************************************
* LOCAL VARIABLES
*/
static RF_CFG_T                 s_stAppRfCfg;
DEV_CARD_BASIC_INFO_T    s_stAppBasicInfo;
/*******************************************************************************
* LOCAL FUNCTIONS DEFINED
*/

/*******************************************************************************
* @fn          app_BCDToDec
*
* @brief       将一个字节的整数转换成由这个整数高4位和低四位组成的
*              十进制数
* @param    input-   u8Byte - 将进行转换的数
*
*
* @return      转换结果
*/
static UINT16 app_BCDToDec(UINT8 u8Byte)
{
    UINT16 u16ShortAddr = 0;

    ASSERT(HI_UINT8(u8Byte) < 0xA);
    ASSERT(LO_UINT8(u8Byte) < 0xA);
    u16ShortAddr += HI_UINT8(u8Byte);
    u16ShortAddr *= 10;
    u16ShortAddr += LO_UINT8(u8Byte);

    return u16ShortAddr;
}

/*******************************************************************************
* @fn          app_CheckIEEEInfo
*
* @brief       检验读出来的IEEE 地址是否符合规定
*
* @param    input-   pu8IEEEAddr - 指向传过来的IEEE地址
*
*
* @return      如果全部检测通过那么返回一个设备段地址
*/
static UINT16 app_CheckIEEEInfo(UINT8 *pu8IEEEAddr)
{
    UINT32 u32ShortAddr;

    ASSERT(pu8IEEEAddr);
    // ASSERT(EXT_LPBSS_MAC_TYPE_CARD == pu8IEEEAddr[EXT_LPBSS_MAC_TYPE]);
    ASSERT(!pu8IEEEAddr[LPBSS_MAC_CHA]
            || IEEE_MAC_CHA_MIN <= pu8IEEEAddr[LPBSS_MAC_CHA]);
    ASSERT(IEEE_MAC_CHA_MAX >= pu8IEEEAddr[LPBSS_MAC_CHA]);
    ASSERT(pu8IEEEAddr[LPBSS_MAC_CARD_TYPE] == DEVICE_CARD_DEVICE_ID);
    endDevType = (LPBSS_device_ID_e)pu8IEEEAddr[LPBSS_MAC_CARD_TYPE];

    // delete HIGH-4 bit of the third Byte
    u32ShortAddr = app_BCDToDec(0x0F & pu8IEEEAddr[LPBSS_MAC_DEVID_H4BIT]);
    u32ShortAddr *= 100;
    u32ShortAddr += app_BCDToDec(pu8IEEEAddr[LPBSS_MAC_DEVID_M8BIT]);
    u32ShortAddr *= 100;
    u32ShortAddr += app_BCDToDec(pu8IEEEAddr[LPBSS_MAC_DEVID_L8BIT]);
    ASSERT(u32ShortAddr >= LPBSS_DEVID_MIN);
    ASSERT(u32ShortAddr <= LPBSS_DEVID_MAX);

    return (UINT16)(u32ShortAddr & 0xFFFF);
}

/*******************************************************************************
* @fn          app_Report
*
* @brief       主任务循环，每次上报一个帧
*
* @param       u32Events - os 触发这个事件
*
*
* @return      none
*/
static VOID app_Report(UINT16 dstaddr, UINT16 dstpanid)
{
    RF_SendPacket(dstaddr, dstpanid, (UINT8*)&(s_stAppTxPkt), PACKET_SIZE);

    s_stAppTxPkt.u16Seqnum++;

#ifdef OPEN_WTD
    BSP_WATCHDOG_Feed();
#endif
}

static VOID inline app_ReadyRadioRecv(VOID)
{
    RF_ReceiveOn();
    app_SleepOff(EVENT_ON_SLEEP);
    event_timer_add(EVENT_ON_SLEEP, RADIO_WAIT_TIME);
}

static VOID app_InitInfo(VOID)
{
    UINT8 u8WorkState;
    UINT16 u16Len;

    app_get_workstate(&u8WorkState, sizeof(UINT8));
    if (u8WorkState < 4)
    {
        s_stAppTxPkt.unStatus.stStatus.u8WorkState = u8WorkState;
    }
    else// 出错后初始化
    {
        u8WorkState = 0;
        s_stAppTxPkt.unStatus.stStatus.u8WorkState = u8WorkState;
        app_set_workstate(&u8WorkState, sizeof(UINT8));
    }

    app_get_worktype((UINT8*)&s_stAppBasicInfo, sizeof(DEV_CARD_BASIC_INFO_T));

    s_stAppTxPkt.u8WorkType = s_stAppBasicInfo.u8WorkType;

    {
        UINT8 u8Buff[2];
        app_get_baseinfo(u8Buff, sizeof(UINT16));
        u16Len = *(UINT16*)(u8Buff);
        if (u16Len <= BASE_INFO_LEN)
        {
            s_stAppWork.u16DescLen = u16Len;
        }
    }
}


/*******************************************************************************
* @fn          app_TransmitInit
*
* @brief       发送任务初始化函数
*
* @param      none
*
* @return      none
*/
static VOID app_TransmitInit(VOID)
{
    //UINT16 u16ShortAddr = 0;
    UINT8 au8IEEEAddr[IEEE_ADDR_LEN];
    RF_DEV_T stRfDevCfg =
    {
        RF_GAIN_HIGH,
        RF_TXPOWER_4P5_DBM
    };

#ifdef OPEN_WTD
    BSP_WATCHDOG_Feed();
#endif

    // read MAC_IEEE address
    BSP_GetExIEEEInfo(au8IEEEAddr, IEEE_ADDR_LEN);
    endDevID = app_CheckIEEEInfo(au8IEEEAddr);

    // Config RF
    s_stAppRfCfg.u16MyAddr = endDevID;
    s_stAppRfCfg.u16PanId = DEVICE_CARD_PANID;
    s_stAppRfCfg.u8Channel = !au8IEEEAddr[LPBSS_MAC_CHA] * LPBSS_MAC_CHA_DEFAULT
                            + au8IEEEAddr[LPBSS_MAC_CHA];
    s_stAppRfCfg.bAckReq = false;

    ASSERT(RF_Init(&s_stAppRfCfg, &stRfDevCfg) != FAILURE);

    // RF puts on receiver before transmission of packet, and turns off
    // after packet is sent
    RF_ReceiveOff();

    // Initalise packet payload
    s_stAppTxPkt.u8MsgType  = DEV_CARD_LOC;
    s_stAppTxPkt.u16Seqnum  = 0;
    s_stAppTxPkt.u8Battery  = BSP_ADC_GetVdd();
    s_stAppTxPkt.u8SoftVer  = 0x12;
    s_stAppTxPkt.unStatus.u8Status = DEV_STATUS_NOMAL;
    s_stAppTxPkt.u8Model    = au8IEEEAddr[LPBSS_MAC_MODEL];
    s_stAppTxPkt.u8WorkType = 0xFF;

    //app_SleepOn();
    app_InitInfo();
}

static VOID app_display(VOID)
{
    UINT8 stage = s_stAppTxPkt.unStatus.stStatus.u8WorkState;

    if(stage == 0)
        stage = 4;

    BSP_BEEP_Off();
    BSP_LED_ALL_OFF();

    if (s_stAppWork.bIsSearch)// 处于找卡状态
    {
        s_stAppWork.bIsSearch = false;
        s_stAppTxPkt.unStatus.stStatus.u8IsSearch = 0;
        return;
    }
    s_stAppWork.u8DisplayCnt = stage * 2 - 1;
    s_stAppWork.u16DisInterval = DISPLAY_STAGE_TIME / s_stAppWork.u8DisplayCnt;
    event_timer_set(EVENT_DISPLAY_MSG);
}

static VOID inline app_StartSearchRespond(VOID)
{
    s_stAppWork.bIsSearch = true;
    s_stAppWork.u16SearchCnt = 60;
    s_stAppTxPkt.unStatus.stStatus.u8IsSearch = 1;
    event_timer_set(EVENT_SEARCH_MSG);
}

static VOID inline app_StopSearchRespond(VOID)
{
    s_stAppWork.bIsSearch = false;
    s_stAppWork.u16SearchCnt = 0;
    s_stAppTxPkt.unStatus.stStatus.u8IsSearch = 0;
    event_timer_set(EVENT_SEARCH_MSG);
}

static VOID app_ChangeState(VOID)
{
    UINT8 u8Buffer;

    s_stAppTxPkt.unStatus.stStatus.u8WorkState++;
    u8Buffer = s_stAppTxPkt.unStatus.stStatus.u8WorkState;
    BSP_FLASH_Erase(WORK_STATE_PG);
    BSP_FLASH_Write(WORK_STATE_PG, 0, &u8Buffer, sizeof(UINT8));
    app_display();
}

// 暂时不支持多个包
static VOID app_ConfigInfo(RF_DATA_T *stPkt)
{
    RADIO_DEV_INFO_T *pstDevInfo = (RADIO_DEV_INFO_T*)(stPkt->u8Data);
    UINT16 u16Len;
    bool rs;

    if (LF_modified == 0)   //激励修改一分钟后才能被无线修改
    {
        s_stAppWork.u8IsInfochange = 2;
        u16Len = pstDevInfo->stPayload.stDescInfo.u16Len;
        s_stAppWork.u16DescLen = u16Len;
        u16Len += sizeof(UINT16);
        s_stAppBasicInfo = pstDevInfo->stPayload.stBasicInfo;
        s_stAppTxPkt.u8WorkType= s_stAppBasicInfo.u8WorkType;

        if (s_stAppWork.u16RecvSeqNum != pstDevInfo->u16Seqnum)   //相同的seqNum，不写FLASH只回复。
        {
            rs = app_set_worktype((UINT8*)&s_stAppBasicInfo, sizeof(DEV_CARD_BASIC_INFO_T));
            if(!rs) return;
            rs = app_set_baseinfo((UINT8*)&pstDevInfo->stPayload.stDescInfo, u16Len);
            if(!rs) return;
            
            s_stAppWork.u16RecvSeqNum = pstDevInfo->u16Seqnum;
        }

        app_StartReport();
    }
}

static VOID app_ClearInfo(VOID)
{
    BSP_FLASH_Erase(WORK_STATE_PG);
    BSP_FLASH_Erase(WORK_TYPE_PG);
    BSP_FLASH_Erase(BASE_INFO_PG);
}

static BOOL app_ReportVerInfo(VOID)
{
    CARD_VERSION_INFO_T *pstCardVerInfo;

    pstCardVerInfo = rt_malloc(sizeof(VERSION) + sizeof(RELEASE)
        + sizeof(CARD_VERSION_INFO_T));

    if (!pstCardVerInfo)
    {
        return false;
    }

    pstCardVerInfo->u8MsgType = CARD_VERINFO;
    pstCardVerInfo->u8DevType = DEVICE_CARD_DEVICE_ID;
    pstCardVerInfo->u8VerInfoLen = sizeof(VERSION) + sizeof(RELEASE);
    pstCardVerInfo->u8VerOffset = 0;
    pstCardVerInfo->u8ReleaseOffset = sizeof(VERSION);
    memcpy(pstCardVerInfo->pu8VerInfo + pstCardVerInfo->u8VerOffset,
        VERSION, sizeof(VERSION));
    memcpy(pstCardVerInfo->pu8VerInfo + pstCardVerInfo->u8ReleaseOffset,
        RELEASE, sizeof(RELEASE));

    RF_SendPacket(BROADCAST_ADDR, BROADCAST_PANID, (UINT8*)pstCardVerInfo,
        pstCardVerInfo->u8VerInfoLen + sizeof(CARD_VERSION_INFO_T));

    rt_free(pstCardVerInfo);

    return true;
}

static VOID app_recv_station_proc(RF_DATA_T *stPkt)
{
    RADIO_CMD_T *pstMsgCmd;
    pstMsgCmd = (RADIO_CMD_T *)(stPkt->u8Data);

    switch(pstMsgCmd->u8MsgType)
    {
    case DEV_CARD_ACK:
        app_StopReport();
        break;

    case DEV_CARD_CLE:
        app_StopSearchRespond();
        break;

    case DEV_CARD_SCH:
        app_StartSearchRespond();
        break;

    case DEV_CARD_GET:
        app_StartReport();
        break;

    case DEV_CARD_SET:
        app_ConfigInfo(stPkt);
        break;

    default:
        break;
    }

}

/*
* 电量检测
*/
static VOID app_msg_adc_proc(VOID)
{
    static UINT32 u32AdcValue = 0;
    static UINT8  u8HungerCnt = 0;

    u32AdcValue += BSP_ADC_GetVdd();

    if (++u8HungerCnt == CHECK_ADC_CNT)
    {
        u8HungerCnt = 0;
        u32AdcValue /= CHECK_ADC_CNT;

        // 低电0.05个单位
        if (u32AdcValue < (VDD_LIMITE << 1))
        {
            s_stAppTxPkt.unStatus.stStatus.u8Lowpower = 1;
        }
        else
        {
            s_stAppTxPkt.unStatus.stStatus.u8Lowpower = 0;
        }

        s_stAppTxPkt.u8Battery = u32AdcValue;
        u32AdcValue = 0;
        event_timer_add(EVENT_ADC_MSG, CHECK_ADC_DELAY_TICK);
    }
    else
    {
        event_timer_add(EVENT_ADC_MSG, LOCATE_INTERVAL_TIME);
    }
}

/*
* 定位信息帧上报
*/
static VOID app_msg_locate_proc(VOID)
{
    static UINT32 u32PreFlash = 0;

    if (BSP_GetSysTick() - u32PreFlash >= 20000)
    {
        BSP_LED_GREEN_Flash();
        u32PreFlash = BSP_GetSysTick();
    }

    app_Report(BROADCAST_ADDR, READ_STATION_PANID);
    app_ReadyRadioRecv();
}

// 配置信息上报
static VOID app_msg_report_porc(VOID)
{
    UINT16 u16Remain = 0;
    RADIO_DEV_INFO_T *pstInfo = NULL;
    UINT32 u32NextTime = INFO_INTERVAL_TIME;

    if(s_stAppTxPkt.unStatus.stStatus.u8Lowpower == 1)  //2.3V低电
    {
        if (!HalAdcCheckVdd(VDD_MIN_NV))    //2.1V停止上报
        {
            goto RET;
        }
    }

    if (!s_stAppWork.u8ReportInfoCnt)
    {
        s_stAppWork.u8IsInfochange = 0;
        s_stAppWork.u8ReportInfoCnt = 5;
        goto RET;
    }
    else
    {
        s_stAppWork.bReportOn = true;
        s_stAppWork.u8ReportInfoCnt--;
        u32NextTime = INFO_REPORT_TIME;
    }

    app_ReportVerInfo();

    u16Remain = s_stAppWork.u16DescLen > 128 ? 128 : s_stAppWork.u16DescLen;
    pstInfo = rt_malloc(sizeof(RADIO_DEV_INFO_T) + u16Remain);
    if (pstInfo)
    {
        pstInfo->u16Seqnum = 0;
        pstInfo->u8MsgType = DEV_CARD_INFO;
        pstInfo->u8IsChange = s_stAppWork.u8IsInfochange;
        pstInfo->stPayload.stBasicInfo = s_stAppBasicInfo;  //实际是workType
        pstInfo->stPayload.stDescInfo.u16Len = u16Remain;   //实际是baseInfo的长度

        pstInfo->u8Len = (uint8)u16Remain+sizeof(DEV_CARD_BASIC_INFO_T)+sizeof(uint16);

        if(u16Remain > 0)
        {
            if(!app_get_baseinfo((UINT8*)&pstInfo->stPayload.stDescInfo,
                                  sizeof(UINT16) + u16Remain))
            {
                rt_free(pstInfo);
                goto RET;
            }
        }
        RF_SendPacket(BROADCAST_ADDR, BROADCAST_PANID,
            (UINT8*)pstInfo, sizeof(RADIO_DEV_INFO_T) + u16Remain);

        pstInfo->u16Seqnum++;
        rt_free(pstInfo);
        app_ReadyRadioRecv();
    }

RET:
    event_timer_add(EVENT_REPORT_MSG, u32NextTime);
}

static VOID app_msg_mac_proc(VOID)
{
    RF_DATA_T *stPkt;

    stPkt = RF_ReceivePkt();

    if (!stPkt)
        return;

    if (s_stAppRfCfg.u16MyAddr != stPkt->u16DstAddr
        || s_stAppRfCfg.u16PanId != stPkt->u16DstPanId)
    {
        goto RET;
    }

    if (READ_STATION_PANID == stPkt->u16SrcPanId)
    {
        app_recv_station_proc(stPkt);
    }
    else if (POS_STATION_PANID == stPkt->u16SrcPanId)
    {
        app_recv_card_proc(stPkt->u8Data);
    }

RET:
    RF_RevertPkt(stPkt);
}

VOID app_msg_key_proc(VOID)
{
    static UINT8 u8PressCnt = 0;

    if (BSP_KEY_IsDown())   // 按键按下
    {
        u8PressCnt++;
        event_timer_add(EVENT_KEY_MSG, _100ms);
        return;
    }

    if (u8PressCnt >= KEY_PRESS_LONGLONG)// 5s
    {
        app_ClearInfo();
        BSP_SYSTEM_REBOOT();
    }
    else if (u8PressCnt >= KEY_PRESS_LONG)// 1s
    {
        app_ChangeState();
    }
    else if (u8PressCnt >= KEY_PRESS_SHORT)
    {
        app_display();
    }

    u8PressCnt = 0;
}

static VOID app_msg_display_proc(VOID)
{
    if(s_stAppWork.u8DisplayCnt & 0x01)
    {
        BSP_BEEP_On();
        app_SleepOff(EVENT_DISPLAY_MSG);
        BSP_LED_Set(CARD_LED_STATE[s_stAppTxPkt.unStatus.stStatus.u8WorkState], LED_ON);
    }
    else
    {
        BSP_BEEP_Off();
        app_SleepOn(EVENT_DISPLAY_MSG);
        BSP_LED_Set(CARD_LED_STATE[s_stAppTxPkt.unStatus.stStatus.u8WorkState], LED_OFF);
    }

    if(s_stAppWork.u8DisplayCnt--)
    {
        event_timer_add(EVENT_DISPLAY_MSG, s_stAppWork.u16DisInterval);
    }
}

static VOID app_msg_search_proc(VOID)
{
    if (s_stAppWork.bIsSearch && s_stAppWork.u16SearchCnt)
    {
        if (s_stAppWork.u16SearchCnt & 1)
        {
            BSP_BEEP_Off();
            app_SleepOn(EVENT_SEARCH_MSG);
        }
        else
        {
            BSP_BEEP_On();
            app_SleepOff(EVENT_SEARCH_MSG);
        }
        BSP_LED_GREEN_Flash();
        s_stAppWork.u16SearchCnt--;
        event_timer_add(EVENT_SEARCH_MSG, 1000);
    }
}

static VOID app_event_proc(VOID)
{
    UINT32 events = event_timer_take_all();

    if (events & EVENT_MAC_MSG)// 无线接收
    {
        app_msg_mac_proc();
    }

    if (events & EVENT_LOCATE_MSG)// 定位上报
    {
        if((cardState == CARD_STATE_FREE)||(cardState == CARD_STATE_WAIT))
        {
            app_msg_locate_proc();
        }
    }

    if (events & EVENT_REPORT_MSG)// 上报信息
    {
        if((cardState == CARD_STATE_FREE)||(cardState == CARD_STATE_WAIT))
        {
            app_msg_report_porc();
        }
    }

    if (events & EVENT_ADC_MSG)// 电量检测，每隔10分钟连续检测5次
    {
        if((cardState == CARD_STATE_FREE)||(cardState == CARD_STATE_WAIT))
        {
            app_msg_adc_proc();
        }
    }

    if (events & EVENT_KEY_MSG)// 按键事件，状态查询和改变
    {
        if((cardState == CARD_STATE_FREE)||(cardState == CARD_STATE_WAIT))
        {
            app_msg_key_proc();
        }
    }

    if (events & EVENT_DISPLAY_MSG)// 状态显示
    {
        if((cardState == CARD_STATE_FREE)||(cardState == CARD_STATE_WAIT))
        {
            app_msg_display_proc();
        }
    }

    if (events & EVENT_SEARCH_MSG)// 被搜索
    {
        if((cardState == CARD_STATE_FREE)||(cardState == CARD_STATE_WAIT))
        {
            app_msg_search_proc();
        }
    }

    if (events & EVENT_ON_SLEEP)// 开启睡眠
    {
        app_SleepOn(EVENT_ON_SLEEP);
        RF_ReceiveOff();
    }

    /*************LF激励******************/
    if (events & EVENT_RESPONCELF_MSG)
    {
        app_ResponseLFProc();
    }

    if (events & EVENT_WAKE_TIMEOUT_MSG)
    {
        Reset_Wake_Status();
    }

    if (events & EVENT_WRITE_MSG)    //低频收到写信息指令
    {
        app_WriteProc(recframe);
    }

    if (events & EVENT_WRITE_ACK_RETRANS_MSG)    //重传写卡回复
    {
        app_WriteACKRetransProc();
    }

    if (events & EVENT_READ_MSG)    //低频收到读信息指令
    {
        app_ReadProc();
    }

    if (events & EVENT_READ_RETRANS_MSG)    //重传读卡
    {
        app_ReadRetransProc();
    }

    if (events & EVENT_READ_ACK_MSG)    //收到读信息指令
    {
        app_ReadACKProc(recframe);
    }

    if (events & EVENT_INFO_MASK_MSG)    //去除信息无线修改屏蔽
    {
        app_RemovInfoMask();
    }
}


/*
** 定功耗休眠
*/
VOID app_savepower(VOID)
{
    UINT32 next_tick;

    if (s_stAppWork.u32EventHold)
        return;

    next_tick = event_timer_next_tick();

    if (next_tick > LOCATE_INTERVAL_TIME)
    {
        next_tick = LOCATE_INTERVAL_TIME;
    }

    BSP_SLEEP_Enter(next_tick);
}



/*******************************************************************************
* @fn          main
*
* @brief       This is the main entry of the  application.
*
* @param       none
*
* @return      none
*/
VOID main (VOID)
{
    BSP_BoardInit();

#ifdef USE_HEAP
    rt_system_heap_init(CC2530_HEAP_BEGIN, CC2530_HEAP_END);
#endif
    event_timer_init();

    BSP_LED_Init();
    BSP_KEY_Init();
    BSP_BEEP_Init();
    BSP_Wakeup_Init();

    app_TransmitInit();

    app_CardStateInit();

#ifdef _DEBUG_TICK
    DEBUG_PIN_SEL &= ~0x01;
    DEBUG_PIN_DIR |=  0x01;

    DEBUG_PIN_H;
#endif

    event_timer_set(EVENT_ADC_MSG);
    //event_timer_set(EVENT_LOCATE_MSG);
    event_timer_add(EVENT_LOCATE_MSG, BOOT_DURATION_TIME);
    event_timer_add_reload(EVENT_LOCATE_MSG, LOCATE_INTERVAL_TIME);
    //app_StartReport();
    {
        s_stAppWork.bReportOn = true;
        s_stAppWork.u8ReportInfoCnt = 5;
        event_timer_add(EVENT_REPORT_MSG, BOOT_DURATION_TIME);
    }

    while (1)
    {
        event_timer_update();
        app_event_proc();
        cardTransInfoUpdata();

#ifdef OPEN_WTD
        BSP_WATCHDOG_Feed();
#endif

#ifdef OPEN_SLEEP
        app_savepower();
#endif
    }
}
