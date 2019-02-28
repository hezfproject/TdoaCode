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
#include <hal_button.h>
#include "app_flash.h"
#include "version.h"
#include "hal_adc.h"
#include "hal_spi.h"
#include "as3933.h"
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

//const LED_E CARD_LED_STATE[4] = {LED_RED, LED_RED, LED_ALL, LED_GREEN};

uint16 endDevID = 0xFFFF;   //卡自己的ID
uint16 midDevID = 0xFFFF;   //卡座ID

extern uint8 recframe[];//130
extern uint8 sendframe[];
extern uint8 sendframeLen;

uint32 lastTick;
uint32 curTick;

uint8 reportCnt = 11;

LPBSS_device_ID_e endDevType;  //卡自己是设备卡还是人员卡
RADIO_ASSET_CARD_LOC_T     s_stAppTxPkt;
RADIO_STAFF_CARD_LOC_T     s_stAlarmTxPkt;

extern LF_CARD_STATE_E cardState;
extern APP_WORK_STATE_T         s_stAppWork;

extern uint8 LF_modified;  //激励修改信息标志

uint8 U8iCount = 0 ;
//UINT8 U8ExciteCnt = 0;               //couter for excite
uint8 U8ExcitTure = true;
uint8 U8ExciteAddr =0;
uint16 U16ExciterID = 0;
uint16 U16PreExciterID = 0;
uint16 u16LastExciterID = 0;
uint16 crc;
uint16 crcCode;

uint8 u8PacketNum = 0;
/*******************************************************************************
* LOCAL FUNCTIONS DECLARATION
*/
static VOID app_Report(UINT16 dstaddr, UINT16 dstpanid);

static VOID app_TransmitInit(VOID);

static UINT16 app_BCDToDec(UINT8 u8Byte);

static UINT16 app_CheckIEEEInfo(UINT8 *pu8IEEEAddr);

static VOID ProcessExcite(VOID);
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
    ASSERT(pu8IEEEAddr[LPBSS_MAC_CARD_TYPE] == ASSET_CARD_DEVICE_ID );
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
    s_stAppTxPkt.u8MsgType  = ASSET_CARD_LOC;
    s_stAppTxPkt.u16Seqnum  = 0;
    s_stAppTxPkt.u8Battery  = BSP_ADC_GetVdd();
	s_stAppTxPkt.u16ExciterID = U16ExciterID;
    s_stAppTxPkt.u8Status= ASSET_STATUS_NOMAL;
    s_stAppTxPkt.u8Model    = au8IEEEAddr[LPBSS_MAC_MODEL];


//    app_InitInfo();
	//event_timer_del(EVENT_LOCATE_MSG);

}

/*
* 电量检测
*/
static void app_AdcCheck(void)
{
    static UINT32 u32AdcValue = 0;
    static UINT8  u8HungerCnt = 0;
    s_stAppTxPkt.u8Battery = BSP_ADC_GetVdd();

	if (s_stAppTxPkt.u8Battery < (VDD_LIMITE << 1))
    {
        s_stAppTxPkt.u8Status |= ASSET_STATUS_HUNGER;         //低电
    }
    else
    {
        s_stAppTxPkt.u8Status &= ~ASSET_STATUS_HUNGER;
    }
}


/*
* 定位信息帧上报
*/
static VOID app_msg_locate_proc(VOID)
{
    //定时上报，发一次
    UINT16 u16LittleEnd=0;
    s_stAppTxPkt.u16ExciterID = 0;
    u16LittleEnd = CRC16((uint8*)&s_stAppTxPkt, sizeof(RADIO_ASSET_CARD_LOC_T)-2,0xffff);
    s_stAppTxPkt.u16Crcsum = (u16LittleEnd>>8) | (u16LittleEnd<<8);
    //s_stAlarmTxPkt.u16Crcsum = u16LittleEnd;
    RF_SendPacket(BROADCAST_ADDR, 0xFFF8, (UINT8*)&(s_stAppTxPkt), sizeof(RADIO_ASSET_CARD_LOC_T));

	//RF_SendPacket(BROADCAST_ADDR, BROADCAST_PANID, (UINT8*)&(s_stAppTxPkt), PACKET_SIZE);
    s_stAppTxPkt.u16Seqnum++;

#ifdef OPEN_WTD
    BSP_WATCHDOG_Feed();
#endif
	app_SleepOff(EVENT_ON_SLEEP);
    event_timer_add(EVENT_ON_SLEEP, RADIO_WAIT_TIME);
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

    if ((READ_STATION_PANID == stPkt->u16SrcPanId )||(POS_STATION_PANID == stPkt->u16SrcPanId))
    {
        app_recv_card_proc(stPkt->u8Data);
    }

RET:
    RF_RevertPkt(stPkt);
}

static BOOL app_ReportVerInfo(VOID)
{
    //static UINT8 u8ReportCnt = 0;
    CARD_VERSION_INFO_T *pstCardVerInfo;
//    event_timer_add(EVENT_REPORT_MSG, 6000);
    //event_timer_add(EVENT_REPORT_MSG, 2000);
    if(++reportCnt > 10)
    {
        reportCnt = 0;
        pstCardVerInfo = rt_malloc(sizeof(VERSION) + sizeof(RELEASE)
            + sizeof(CARD_VERSION_INFO_T));

        if (!pstCardVerInfo)
        {
            return false;
        }

        pstCardVerInfo->u8MsgType = CARD_VERINFO;
        pstCardVerInfo->u8DevType = ASSET_CARD_DEVICE_ID ;
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
}


static VOID app_event_proc(VOID)
{

    UINT32 events = event_timer_take_all();

    if (events & EVENT_MAC_MSG)// 无线接收
    {
        app_msg_mac_proc();
    }

    if (events & EVENT_KEY_MSG)// 按键事件，状态查询和改变
    {
        ProcessExcite();
        lastTick = BSP_GetSysTick();
//        event_timer_del(EVENT_KEY_MSG);
    }

	if (events & EVENT_ADC_MSG)
	{
	    app_AdcCheck();
	}

	if(events & EVENT_REPORT_MSG)   //版本上报
	{
	    app_ReportVerInfo();
	}

	if (events & EVENT_LOCATE_MSG)// 定位上报
	{
        BSP_LED_RED_Flash();
        app_msg_locate_proc();
	   // close_all_led();
	}

	if (events & EVENT_ON_SLEEP)// 开启睡眠
    {
        app_SleepOn(EVENT_ON_SLEEP);
        RF_ReceiveOff();
    }

    if(events & EVENT_SEARCH_MSG)    //激励上报
    {
        UINT16 u16LittleEnd=0;
        u8PacketNum++;
        u16LittleEnd = CRC16((uint8*)&s_stAppTxPkt, sizeof(RADIO_ASSET_CARD_LOC_T)-2,0xffff);
        s_stAppTxPkt.u16Crcsum = (u16LittleEnd>>8) | (u16LittleEnd<<8);
        //s_stAlarmTxPkt.u16Crcsum = u16LittleEnd;
        RF_SendPacket(BROADCAST_ADDR, 0xFFF8, (UINT8*)&(s_stAppTxPkt), sizeof(RADIO_ASSET_CARD_LOC_T));
        if(u8PacketNum > 2)
        {
            //s_stAppTxPkt.u16Seqnum++;
            s_stAppTxPkt.u8Status &= (~ASSET_STATUS_IMPEL);
            s_stAppTxPkt.u16ExciterID = 0;
            u8PacketNum = 0;
            app_SleepOff(EVENT_ON_SLEEP);
            event_timer_add(EVENT_ON_SLEEP, RADIO_WAIT_TIME);
        }
        else
        {
            event_timer_add(EVENT_SEARCH_MSG,5);
        }

        P0_1=1;
        #ifdef OPEN_WTD
        BSP_WATCHDOG_Feed();
    #endif
    }

    if(events & EVENT_RESPONCELF_MSG)
    {
        P0IEN |= BV(4);
    }
}

/*
** 定功耗休眠
*/
VOID app_savepower(VOID)
{
    UINT32 next_tick;

    if (s_stAppWork.u32EventHold  || (BSP_GetSysTick() - lastTick) < 500)
        return;
    next_tick = event_timer_next_tick();

    if (next_tick > LOCATE_INTERVAL_TIME)
    {
        next_tick = LOCATE_INTERVAL_TIME;
    }

	BSP_SLEEP_Enter(next_tick);


}
/*
** AS3933 初始化
*/

void As3933_SpiInit(void)
{
	SPI_CC_Init();
//	IEN2 |= (0x0<<3);
}

UINT8 Calcrc_1byte(UINT8 abyte)
{
    UINT8 i,crc_1byte;
    crc_1byte=0;                //设定crc_1byte初值为0
    for(i = 0; i < 8; i++)
    {
       if(((crc_1byte^abyte)&0x01))
          {
            crc_1byte^=0x18;
            crc_1byte>>=1;
            crc_1byte|=0x80;
           }
        else
           crc_1byte>>=1;
        abyte>>=1;
     }
     return crc_1byte;
}


VOID ProcessExcite(VOID)
{
	static UINT16 U16HeadMs;
	static UINT16 U16TailMs;
	//static UINT16 U16ExciterID = 0;

	UINT16 CycleLenth;
	U16HeadMs = (UINT16)BSP_GetSysTick();
	if(U8iCount == 0)
	{
		U8iCount++;
		U16TailMs = U16HeadMs;
		U16ExciterID = 0;
		CycleLenth = 0;
	}
	else
	{
		CycleLenth = U16HeadMs - U16TailMs;
		U16TailMs = U16HeadMs;
	}

	if(CycleLenth > 1)
	{
		U8iCount = 1;
		U16TailMs = U16HeadMs;
		U16ExciterID = 0;
	}

	UINT8 U32SDAdio = P0_5;
	if(U32SDAdio & 0x1)
	{
		U16ExciterID |= 1<<(16-U8iCount);
	}
	U8iCount++;

	if(U8iCount == 17)
	{
		U8iCount = 0;
        UINT16 u16random;
        crc = ((U16ExciterID >> 8) & 0x00F0);

        U16ExciterID = U16ExciterID & 0x0FFF;

        crcCode = CRC16((uint8*)&U16ExciterID,2,0xffff);
        crcCode = crcCode & 0x00F0;

        if((crc == crcCode) && (U16ExciterID !=0) && (U16PreExciterID == U16ExciterID))
        {
            s_stAppTxPkt.u16ExciterID = U16ExciterID;
            if(u16LastExciterID == U16ExciterID)
            {
                u8PacketNum = 3;
            }
            else
            {
                u8PacketNum = 0;
            }
            u16LastExciterID = U16ExciterID;
            s_stAppTxPkt.u8Status |= ASSET_STATUS_IMPEL;
    		//U8ExciteCnt = 0;
    		U8ExcitTure = true;
            u16random = HAL_MCU_Random();
            u16random = u16random % 80;
            s_stAppTxPkt.u16Seqnum++;
    		event_timer_add(EVENT_SEARCH_MSG,u16random);
            event_timer_add(EVENT_RESPONCELF_MSG,2000);
            //BSP_LED_RED_Flash();
            //open_red_led();
    		P0_1=0;             //激励成功，亮红灯

            P0IEN &= (~BV(4));
        }

        U16PreExciterID = U16ExciterID;
	}
}
VOID GPIO_init(void)
{
	HAL_BUTTON_1_SEL &= ~(BV(4));    /* Set pin function to GPIO */
    HAL_BUTTON_1_DIR &= ~(BV(4));    /* Set pin direction to Input */
	HAL_BUTTON_1_SEL &= ~(BV(5));    /* Set pin function to GPIO */
    HAL_BUTTON_1_DIR &= ~(BV(5));    /* Set pin direction to Input */
	//HAL_BUTTON_1_INP2 &= ~(HAL_BUTTON_1_INP2BIT);
	HAL_BUTTON_1_INP0 |= (BV(4));     // p0_4为上拉输入
	HAL_BUTTON_1_INP0 |= (BV(5));    // p0_5为上拉输入

    P1SEL &= ~(BV(3));
    P1DIR &= ~(BV(3));
	P1INP |= BV(3);
}

VOID Close_SPI(void)
{
	P1SEL &= ~(BV(4));
	P1DIR |= (BV(4));
    P1_4 = 1;

	P1SEL &= ~(BV(5));
	P1DIR |= (BV(5));
    P1_5 = 0;

	P1SEL &= ~(BV(6));
	P1DIR &= ~(BV(6));
    P1INP |= BV(6);

	P1SEL &= ~(BV(7));
	P1DIR |= (BV(7));
    P1_7 = 0;

    //P1INP |= (BV(6)|BV(7));

    //P1INP |= (BV(4) | BV(5)|BV(6)|BV(7));

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
	GPIO_init();
#ifdef USE_HEAP
	rt_system_heap_init(CC2530_HEAP_BEGIN, CC2530_HEAP_END);
#endif

	event_timer_init();
	As3933_SpiInit();
	Config_As3933();
    BSP_LED_Init();
	BSP_KEY_Init();
	app_TransmitInit();
	app_AdcCheck();

#ifdef _DEBUG_TICK
	DEBUG_PIN_SEL &= ~0x01;
	DEBUG_PIN_DIR |=  0x01;

	DEBUG_PIN_H;
#endif
	P0_0=0x1;
	P0_1=0x1;
	event_timer_set(EVENT_ADC_MSG);
    event_timer_set(EVENT_REPORT_MSG);
//	event_timer_set(EVENT_LOCATE_MSG);
	//event_timer_add(EVENT_LOCATE_MSG, BOOT_DURATION_TIME);
	//P0_0=0x0;   //休眠结束，亮绿灯
	//HAL_SLEEP_Enter(20);
	//P0_0=0x1;   //休眠结束，亮绿灯
	//app_AdcCheck();
	//app_ReportVerInfo();
	//app_msg_locate_proc();
	event_timer_add_reload(EVENT_ADC_MSG, 700000);
    event_timer_add_reload(EVENT_LOCATE_MSG,10000);

    Close_SPI();
	while(1)
	{

		event_timer_update();

		app_event_proc();

#ifdef OPEN_WTD
        BSP_WATCHDOG_Feed();
#endif

#ifdef OPEN_SLEEP
        app_savepower();
#endif
	}
}

