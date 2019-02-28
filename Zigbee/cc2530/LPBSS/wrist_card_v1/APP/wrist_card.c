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
#include <hal_lcd.h>
#include <hal_adc.h>
#include <wrist_card.h>
#include <RadioProto.h>
#include <lpbssnwk.h>
#include <hal_button.h>
#include <hal_timer.h>
#include <mma8452q.h>
#include <string.h>
#include <mem.h>
#include <timer_event.h>
#include <track.h>
#include "app_card_wireless.h"
#include <hal_mcu.h>
#include "app_flash.h"
#include "version.h"

#include "..\..\..\..\..\common\crc.h"


#include "sms.h"
#include "bsp_beep.h"

/*******************************************************************************
 *			                                      MARCO
 */
#define FF_MT_SRC	   0x16
#define TRANSIENT_SRC  0x1E
#define PULCE_SRC      0x22

/*******************************************************************************
* CONSTANTS
*/
#ifdef USE_HEAP
#define HEAP_SIZE       (4 * 1024)
UINT8 heap_pool[HEAP_SIZE];

#define CC2530_HEAP_BEGIN   (void *)(heap_pool)
#define CC2530_HEAP_END     (void *)(&heap_pool[HEAP_SIZE])
#endif


uint16 endDevID = 0xFFFF;   //卡自己的ID
uint16 midDevID = 0xFFFF;   //卡座ID
LPBSS_device_ID_e endDevType;  //卡自己是设备卡还是人员卡

extern uint8 recframe[];//128
extern uint8 sendframe[];
extern uint8 sendframeLen;
extern LF_CARD_STATE_E cardState;
extern APP_WORK_STATE_T         s_stAppWork;

uint8 display_delay = 0;
uint8 display_motionCount = 0;

uint8 display_status = LCD_DISPLAY_OFF;
uint8 band_status = BAND_CONNECT;
uint8 band_last_status = BAND_CONNECT;
uint8 battery_level = 0;;
uint8 alarm_cnt = 0;
static uint8 smsSeq[SMS_NUMBER_MAX];
//static Bool isMoving = True;


/*******************************************************************************
* LOCAL FUNCTIONS DECLARATION
*/
static VOID app_Report(UINT16 dstaddr, UINT16 dstpanid);

static VOID app_TransmitInit(VOID);

static UINT16 app_BCDToDec(UINT8 u8Byte);

static UINT16 app_CheckIEEEInfo(UINT8 *pu8IEEEAddr);
static void app_get_battery_level(UINT32 u32AdcValue);

typedef struct
{
	uint8   u8FF_Mt_Src;//FF_MT_SRC
	uint8  	u8Transient_Src;
	uint8   u8Xbyte_h;
	uint8   u8Ybyte_h;
	uint8   u8Zbyte_h;
}MMA845x_DATA;

static uint8 Last_FF_MT_Status;



/*******************************************************************************
* LOCAL VARIABLES
*/
static RF_CFG_T                 s_stAppRfCfg;
static RADIO_STAFF_CARD_LOC_T   s_stAppTxPkt;
static RF_DATA_T *stPkt;
static MMA845x_DATA MMA845xData;
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

    ASSERT(HI_UINT8(u8Byte) < 0x0A);
    ASSERT(LO_UINT8(u8Byte) < 0x0A);
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
    ASSERT(pu8IEEEAddr[LPBSS_MAC_CARD_TYPE] == WRIST_CARD_DEVICE_ID);
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

static VOID inline app_ReadyRadioRecv(VOID)
{
    RF_ReceiveOn();
    app_SleepOff(EVENT_ON_SLEEP);
    event_timer_add(EVENT_ON_SLEEP, RADIO_WAIT_TIME);
}


static BOOL app_ReportVerInfo(VOID)
{
    static UINT8 u8ReportCnt = 0;
    CARD_VERSION_INFO_T *pstCardVerInfo;
    UINT32 u32NextTime;

    if (u8ReportCnt < REPORT_VERINFO_CNT)
    {
        u8ReportCnt++;
        u32NextTime = _1s;
    }
    else
    {
        u8ReportCnt = 0;
        u32NextTime = REPORT_VERINFO_TIME;
    }
    event_timer_add(EVENT_REPORT_MSG, u32NextTime);

    pstCardVerInfo = rt_malloc(sizeof(VERSION) + sizeof(RELEASE)
        + sizeof(CARD_VERSION_INFO_T));

    if (!pstCardVerInfo)
    {
        return false;
    }

    pstCardVerInfo->u8MsgType = CARD_VERINFO;
    pstCardVerInfo->u8DevType = WRIST_CARD_DEVICE_ID;
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
    UINT16 u16LittleEnd=0;
    u16LittleEnd = CRC16((UINT8*)&(s_stAppTxPkt), PACKET_SIZE-2,0xffff);
    s_stAppTxPkt.u16Crcsum = (u16LittleEnd>>8) | (u16LittleEnd<<8);
    RF_SendPacket(dstaddr, dstpanid, (UINT8*)&(s_stAppTxPkt), PACKET_SIZE);

    s_stAppTxPkt.u16Seqnum++;

#ifdef OPEN_WTD
    BSP_WATCHDOG_Feed();
#endif
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
    s_stAppRfCfg.u16PanId = STAFF_CARD_PANID;
    s_stAppRfCfg.u8Channel = !au8IEEEAddr[LPBSS_MAC_CHA] * LPBSS_MAC_CHA_DEFAULT
                            + au8IEEEAddr[LPBSS_MAC_CHA];
    s_stAppRfCfg.bAckReq = false;

    ASSERT(RF_Init(&s_stAppRfCfg, &stRfDevCfg) != FAILURE);

    // RF puts on receiver before transmission of packet, and turns off
    // after packet is sent
    RF_ReceiveOff();

    HalIsAdcStart(true);//打开采样电压

    // Initalise packet payload
    s_stAppTxPkt.u8MsgType  = WRIST_CARD_LOC;
    s_stAppTxPkt.u16Seqnum  = 0;
    s_stAppTxPkt.u8Battery  = BSP_ADC_GetVdd()<<1;
    s_stAppTxPkt.u8SoftVer  = 0x12;
    s_stAppTxPkt.u8Status   = WRIST_STATUS_NOMAL;
    s_stAppTxPkt.u8Model    = au8IEEEAddr[LPBSS_MAC_MODEL];

    HalIsAdcStart(false);

    app_get_battery_level(s_stAppTxPkt.u8Battery/2);
}

/*******************************************************************************
* @fn          app_Delay
*
* @brief       毫秒级延时
*
* @param      input - timeout 延时的长度
*
* @return      none
*/
/* timeout is  in ms */
static VOID app_Delay(UINT16 timeout)
{
    uint16 i, j, k;
    uint16 timeBig =  timeout >> 9;
    uint16 timeSmall = timeout - timeBig * 512;

    for(i = 0; i < timeBig; i++)
    {
#ifdef OPEN_WTD
        BSP_WATCHDOG_Feed();
#endif
        for(j = 0; j < 512; j++)
        {
            /* One Nop counts 12/32M, So 889  cyc is a ms*/
            k = 880;//k = 889;
            while(k--)
            {
                asm("NOP");
                asm("NOP");
                asm("NOP");
            }
        }
    }
#ifdef OPEN_WTD
    BSP_WATCHDOG_Feed();
#endif
    for(i = 0; i < timeSmall; i++)
    {
        k = 880;

        while(k--)
        {
            asm("NOP");
            asm("NOP");
            asm("NOP");
        }
    }
}

/***********************************************************************************
* @fn          app_TestKeyPress
*
* @brief       毫秒级延时
*
* @param      input - timeout 按键确认的时间，单位MS
*
* @return      如果按键达到确认时间则返回true,否则返回false
*/
/* Test If a key press is a long press */
BOOL app_TestKeyPress(UINT16 TimeOut)
{
    UINT16 testInterval = 300;   // test once each 300 ms
    UINT16 testnum = TimeOut / testInterval;

    for(uint16 i = 0; i < testnum; i++)
    {
        app_Delay(testInterval);

        if(!BSP_KEY_IsDown())   // low voltage when key press
        {
            return false;
        }
    }

    return true;
}

/***********************************************************************************
* @fn          app_SMSPrevious
*
* @brief       上一条短信
*
* @param     none
*
* @return      none
*/
void app_SMSPrevious(void)
{
	if(display_status == LCD_DISPLAY_SMS)
	{
		PreviousSMS();
		display_delay = 10;
	}
}

/***********************************************************************************
* @fn          app_SMSNext
*
* @brief       上一条短信
*
* @param     none
*
* @return      none
*/
void app_SMSNext(void)
{
	NextSMS();
	display_delay = 10;

}

/*******************************************************************************
* @fn          app_HelpSurvey
*
* @brief       处理求救按键的函数
*
* @param      none
*
* @return      none
*/
static VOID app_HelpSurvey(VOID)
{
    static UINT8 s_u8HelpCnt = 0;

    if (!(s_stAppTxPkt.u8Status & WRIST_STATUS_HELP))
    {
        if(BSP_KEY_IsDown())   // 按键按下
        {
			app_SMSPrevious();

        	if(app_TestKeyPress(KEY_HELP_TIME))
        	{
	            s_u8HelpCnt = HELP_REPORT_MAX;
                alarm_cnt = 0;
	            s_stAppTxPkt.u8Status |= WRIST_STATUS_HELP;
        	}
        }
    }


    #if 0
	if(BSP_KEY2_IsDown())  //加速度中断
	{
        MMA845x_ReadReg(TRANSIENT_SRC);

        //RF_SendPacket(BROADCAST_ADDR, BROADCAST_ADDR, (UINT8*)&(mma_regist), 3);

        if(((MMA845x_ReadReg(FF_MT_SRC)&0x80) || (Last_FF_MT_Status&0x80)) && (MMA845x_ReadReg(PULCE_SRC)&0x80))
        {
            s_stAppTxPkt.u8Status |= WRIST_STATUS_DIEDAO;
            display_status = LCD_DISPLAY_TUMBLE;
            display_delay = 5;
            HalLcdInit();
            event_timer_add(EVENT_UPDATE_MSG,1);
        }
        //s_stAppTxPkt.u8Status |= WRIST_STATUS_DIEDAO;
        //display_status = LCD_DISPLAY_TUMBLE;
        //display_delay = 5;
        //HalLcdInit();
        //event_timer_add(EVENT_UPDATE_MSG,1);
	}
    #endif

	//MMA845x_ReadReg(FF_MT_SRC);

    if(BSP_KEY1_IsDown())
    {
        if(display_status == LCD_DISPLAY_OFF
		 ||display_status == LCD_DISPLAY_CHARGE
		 ||display_status == LCD_DISPLAY_TUMBLE)
        {
            display_status = LCD_DISPLAY_TIME;
        }
        else if(display_status == LCD_DISPLAY_TIME)
        {
            display_status = LCD_DISPLAY_DATE;
        }
        else if(display_status == LCD_DISPLAY_DATE)
        {
            display_status = LCD_DISPLAY_ID;
        }
		else if(display_status == LCD_DISPLAY_ID)
		{
			display_status = LCD_DISPLAY_SMS;
            event_timer_add(EVENT_READ_TRANSIENT,1);
		}
        else if(display_status == LCD_DISPLAY_SMS)
        {
            display_status = LCD_DISPLAY_TIME;
        }
		if(display_status != LCD_DISPLAY_SMS)
		{
			ResetSMSIndex();
		}

        display_delay = 10;
        HalLcdInit();
        event_timer_add(EVENT_UPDATE_MSG,1);
        return;
    }

    if (s_u8HelpCnt > 0)
    {
        s_u8HelpCnt--;
        //BSP_LED_RED_Flash();
        if(s_u8HelpCnt > 38)
        {
            display_status = LCD_DISPLAY_SOS;
            display_delay = 20;
            HalLcdInit();
            event_timer_add(EVENT_UPDATE_MSG,1);
        }

        event_timer_add(EVENT_KEY_MSG, HELP_FLASH_TIME);
    }

    if (!s_u8HelpCnt)
    {
        s_stAppTxPkt.u8Status &= ~WRIST_STATUS_HELP;
    }
}

/*
* 电量检测
*/
static void app_AdcCheck(void)
{
    static UINT32 u32AdcValue = 0;
    static UINT8  u8HungerCnt = 0;

    //每2分钟测量5次，取平均值
	if(((s_stAppTxPkt.u16Seqnum % 600) == 0)|| (u8HungerCnt))
	{
		HalIsAdcStart(true);

	    u32AdcValue += BSP_ADC_GetVdd();

	    if (++u8HungerCnt >= CHECK_ADC_CNT)
	    {
	        u8HungerCnt = 0;
	        u32AdcValue /= CHECK_ADC_CNT;

	        if (u32AdcValue < VDD_LIMITE)
	        {
	            s_stAppTxPkt.u8Status |= WRIST_STATUS_HUNGER;
	        }
	        else
	        {
	            s_stAppTxPkt.u8Status &= ~WRIST_STATUS_HUNGER;
	        }
                if((!IsBatteryCharge() && s_stAppTxPkt.u8Battery >= u32AdcValue*2) ||(IsBatteryCharge() && s_stAppTxPkt.u8Battery <= u32AdcValue*2))
                {
                    s_stAppTxPkt.u8Battery = u32AdcValue*2;  //兼容以前人员
                    app_get_battery_level(u32AdcValue);
                }

	        u32AdcValue = 0;
	    }
		HalIsAdcStart(false);
	}
}

/*
* 定位信息帧上报
*/
static void app_LocateProc(void)
{
	static uint8 recCount = 0;

    GetTime();
    if(CheckBandDisconnect())
    {
        band_status = BAND_DISCONNECT;
        s_stAppTxPkt.u8Status |= WRIST_STATUS_BAND_DISCONNECT;
        if(band_last_status != band_status)
        {
            if(display_status == LCD_DISPLAY_OFF)
            {
                display_status = LCD_DISPLAY_TIME;
            }
			HalLcdInit();
            event_timer_add(EVENT_UPDATE_MSG,1);
        }
    }
    else
    {
        s_stAppTxPkt.u8Status &= ~WRIST_STATUS_BAND_DISCONNECT;

        band_status = BAND_CONNECT;
    }

	if(recCount++%5 == 0)
	{
		s_stAppTxPkt.u8Status |= WRIST_STATUS_POLL;
	}

    app_Report(BROADCAST_ADDR, READ_STATION_PANID);

	if(s_stAppTxPkt.u8Status&WRIST_STATUS_POLL)
	{
    	app_ReadyRadioRecv();
	}

    if(s_stAppTxPkt.u8Status & WRIST_STATUS_HELP)
    {
        if(alarm_cnt++ > 20)
        {
            s_stAppTxPkt.u8Status &= ~WRIST_STATUS_HELP;
            alarm_cnt = 0;
        }
    }


	s_stAppTxPkt.u8Status &= ~(WRIST_STATUS_DIEDAO|WRIST_STATUS_MOTION|WRIST_STATUS_POLL);
}

static VOID app_ConfigTime(RF_DATA_T *stPkt)
{
    RADIO_WRIST_TIME_T *pstWristTime = (RADIO_WRIST_TIME_T*)(stPkt->u8Data);
    Date_t date  = GetDate();
    Time_t time;
    time.hour = pstWristTime->u8Hour;
    time.min = pstWristTime->u8Minute;
    time.sec = pstWristTime->u8Second;
    SetTime(time);

    date.year = pstWristTime->u16Year;
    date.mon = pstWristTime->u8Month;
    date.day = pstWristTime->u8Day;
    SetDate(date);
}

static VOID app_ParseSms(RF_DATA_T *stPkt)
{
	static uint8 seqIndex = 0;
    Time_t time;
	Date_t date;
	uint8 bufflen = 0;
    RADIO_WRIST_SMS_T *pstWristSms = (RADIO_WRIST_SMS_T*)(stPkt->u8Data);
	uint8 smsBuff[SMS_MAX_LEN+5] = {0};

	if(pstWristSms->u8len > SMS_INFO_LEN_MAX||!pstWristSms->u8len)
	{
		return;
	}

	//; 检查是否是已存在的短信SEQ一致
	for(uint8 idx =0;idx < SMS_NUMBER_MAX; idx++)
	{
		if(smsSeq[idx]==pstWristSms->u16seqnum)
		{
			return;
		}
	}
	smsSeq[seqIndex%SMS_NUMBER_MAX] = pstWristSms->u16seqnum;
	seqIndex++;

#ifdef USE_BUZZER
	BSP_BEEP_On();
#endif

	date = GetDate();
	time = GetTime();
	memcpy(smsBuff+bufflen,(uint8 *)&time,sizeof(Time_t));
	bufflen += sizeof(Time_t);
	memcpy(smsBuff+bufflen,(uint8 *)&date,sizeof(Date_t));
	bufflen += sizeof(Date_t);

	memcpy(smsBuff+bufflen,(uint8 *)(pstWristSms) + sizeof(RADIO_WRIST_SMS_T)-sizeof(uint8)
			,sizeof(uint8) + pstWristSms->u8len);
	bufflen += sizeof(uint8) + pstWristSms->u8len;

	//;写进FLASH
	//SetSMS((uint8 *)(pstWristSms) + sizeof(RADIO_WRIST_SMS_T)-sizeof(uint8)
	//		,sizeof(uint8) + pstWristSms->u8len);
	SetSMS(smsBuff,bufflen);
	display_status = LCD_DISPLAY_SMS;
	display_delay = 10;
	ResetSMSIndex();
	HalLcdInit();
	event_timer_add(EVENT_CLOSE_BUZZER,1000);
	event_timer_add(EVENT_UPDATE_MSG,1);
}



static VOID app_recv_station_proc(RF_DATA_T *stPkt)
{
    RADIO_CMD_T *pstMsgCmd;
    pstMsgCmd = (RADIO_CMD_T *)(stPkt->u8Data);

    switch(pstMsgCmd->u8MsgType)
    {

    case WRIST_CARD_TIME:
        app_ConfigTime(stPkt);
        break;

	case WRIST_CARD_SMS:
		app_ParseSms(stPkt);
		break;

    default:
        break;
    }

}

void app_MotionDetect(void)
{
	static signed char smma[3];
	#define 	 D_VALUE	4
	#define    DEFF(x,y)	((x)>=(y)?((x)-(y)):((y)-(x)))

    uint8 u8mma[3];
    signed char mma[3];
    uint8 i;

    //RF_SendPacket(BROADCAST_ADDR, BROADCAST_ADDR, (UINT8*)&(mma_regist), 3);
    MMA845xData.u8FF_Mt_Src = MMA845x_ReadReg(FF_MT_SRC);//FF_MT_SRC

    if(((MMA845xData.u8FF_Mt_Src&0x80) || (Last_FF_MT_Status&0x80)) && (MMA845x_ReadReg(PULCE_SRC)&0x80))
    {
        s_stAppTxPkt.u8Status |= WRIST_STATUS_DIEDAO;
        display_status = LCD_DISPLAY_TUMBLE;
        display_delay = 5;
        HalLcdInit();
        event_timer_add(EVENT_UPDATE_MSG,1);
    }


    Last_FF_MT_Status = MMA845xData.u8FF_Mt_Src;
	MMA845xData.u8Transient_Src = MMA845x_ReadReg(TRANSIENT_SRC);
	u8mma[0] = MMA845x_ReadReg(0x01);
	u8mma[1] = MMA845x_ReadReg(0x03);
	u8mma[2] = MMA845x_ReadReg(0x05);

    for(i=0;i<3;i++)
    {
        if(u8mma[i] < 0x80)
        {
            mma[i] = u8mma[i];
        }
        else
        {
            mma[i] = (255 - u8mma[i] + 1)*(-1);
        }
    }

	if(((DEFF(mma[0],smma[0]) > D_VALUE)
		||(DEFF(mma[1],smma[1]) > D_VALUE)
		||(DEFF(mma[2],smma[2]) > D_VALUE)))
	{
		s_stAppTxPkt.u8Status |= WRIST_STATUS_MOTION;
	}

    for(i=0;i<3;i++)
    {
        smma[i] = mma[i];
    }

	if((MMA845xData.u8Transient_Src & 0x40)||(MMA845xData.u8FF_Mt_Src & 0x80))
	{
		if(MMA845xData.u8Transient_Src & 0x40)
		{
			s_stAppTxPkt.u8Status |= WRIST_STATUS_MOTION;
			if(display_status == LCD_DISPLAY_CHARGE)
			{
				display_status = LCD_DISPLAY_TIME;
				display_delay = 10;
			}
		}
	}

}

//卡信息接收处理
void app_msg_recv_proc(void)
{
    stPkt = RF_ReceivePkt();

    if (!stPkt)
        return;

    if (s_stAppRfCfg.u16MyAddr != stPkt->u16DstAddr
        || s_stAppRfCfg.u16PanId != stPkt->u16DstPanId)
    {
        goto RET;
    }

    if ((READ_STATION_PANID == stPkt->u16SrcPanId)
    ||(POS_STATION_PANID == stPkt->u16SrcPanId))
    {
        app_recv_card_proc(stPkt->u8Data);
    }
    if (READ_STATION_PANID == stPkt->u16SrcPanId)
    {
        app_recv_station_proc(stPkt);
    }

RET:
    RF_RevertPkt(stPkt);
}

/*
** 定功耗休眠
*/
void app_savepower(void)
{
    UINT32 next_tick;

    if (s_stAppWork.u32EventHold)
        return;

    next_tick = event_timer_next_tick();

    if (next_tick < SLEEP_TIME_MIN)
        return;

    if (next_tick > SLEEP_TIME_MAX)
    {
        next_tick = SLEEP_TIME_MAX;
    }

    P1_1 = 0;
    P1_4 = 0;

    BSP_SLEEP_Enter(next_tick);
}

static void app_event_proc(void)
{
    uint32 events = event_timer_take_all();

    if (events & EVENT_MAC_MSG) //处理收到的信息
    {
        app_msg_recv_proc();
    }

    if (events & EVENT_KEY_MSG)
    {
        app_HelpSurvey();
    }


    if (events & EVENT_CLOSE_BUZZER)
    {
        BSP_BEEP_Off();
    }

    if(events & EVENT_UPDATE_MSG)
    {
    	static uint8 sDisplayStatus = LCD_DISPLAY_OFF;
		static uint8 flash = 0;

		flash++;

		if(sDisplayStatus != display_status)
		{
			HalLcd_HW_Clear();
			sDisplayStatus = display_status;
		}

		if(LCD_DISPLAY_CHARGE == display_status &&IsBatteryCharge())
		{
			Menu_UpdadeBattery(battery_level + 5);
		}
		else
		{
			if((display_status != LCD_DISPLAY_CHARGE)
				&&(display_status != LCD_DISPLAY_SMS)
				&&(display_status != LCD_DISPLAY_TUMBLE))
			{
				Menu_UpdateWeek();
				Menu_UpdateBandState(flash,band_status);
				Menu_UpdadeBattery(battery_level);
				if(Is_SMS_Unread())
				{
					Menu_UpdateEnvelope(flash);
				}
			}

			if(display_status == LCD_DISPLAY_TIME)
	        {
	            Menu_UpdateTime(flash);
	        }
	        else if(display_status == LCD_DISPLAY_DATE)
	        {
	            Menu_UpdateDate();
	        }
	        else if(display_status == LCD_DISPLAY_WEATHER)
	        {
	            Menu_UpdateWeather();
	        }
	        else if(display_status == LCD_DISPLAY_SOS)
	        {
	            Menu_UpdateSOS();
	        }
                else if(display_status == LCD_DISPLAY_SMS)
                {
                    Menu_UpdateSMS();
                }
                else if(display_status == LCD_DISPLAY_ID)
                {
                    Menu_UpdateID(endDevID);
                }
                else if(display_status == LCD_DISPLAY_TUMBLE)
                {
                    Menu_UpdateTumble();
                }
		}


        if(display_delay>0)
        {
            event_timer_add(EVENT_UPDATE_MSG,1000);
            display_delay--;
        }
        else
        {
            display_status = LCD_DISPLAY_OFF;
            band_last_status = band_status;

	    ResetSMSIndex();

	    if(IsBatteryCharge() == false)
	    {
            	HalLcdTurnOff();
	    }
            else
            {
                display_status = LCD_DISPLAY_CHARGE;
                event_timer_add(EVENT_UPDATE_MSG,1000);
            }
        }
    }

    if (events & EVENT_LOCATE_MSG)
    {
        if(IsBatteryCharge() == true)
        {
            if(display_status == LCD_DISPLAY_OFF)
            {
                HalLcdInit();
                display_status = LCD_DISPLAY_CHARGE;
                event_timer_add(EVENT_UPDATE_MSG,1);
            }
            s_stAppTxPkt.u8Status |= WRIST_STATUS_CHARGING;
        }
        else
        {
            s_stAppTxPkt.u8Status &= ~WRIST_STATUS_CHARGING;
        }

        if((cardState == CARD_STATE_FREE)||(cardState == CARD_STATE_WAIT))
        {
            app_AdcCheck();
            app_MotionDetect();
            app_LocateProc();
        }
    }

    if (events & EVENT_REPORT_MSG)
    {
	if((cardState == CARD_STATE_FREE)||(cardState == CARD_STATE_WAIT))
        {
            app_ReportVerInfo();
        }
    }

    if (events & EVENT_ON_SLEEP)// 开启睡眠
    {
        app_SleepOn(EVENT_ON_SLEEP);
        RF_ReceiveOff();
    }

    if(events & EVENT_READ_TRANSIENT)
    {
        if(display_status == LCD_DISPLAY_SMS)
        {
            static Time_t stime;
            if(MMA845x_ReadReg(TRANSIENT_SRC) & 0x40)
            {
                Time_t time = GetTime();
                if((stime.hour != time.hour)
                    ||(stime.min!= time.min)
                    ||(stime.sec != time.sec))
                {
                    app_SMSNext();
                    stime = time;
                }
            }
            event_timer_add(EVENT_READ_TRANSIENT,100);
        }
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
}

static void app_get_battery_level(UINT32 u32AdcValue)
{
    if(u32AdcValue < BATTERYLE_LEVEL_1)
    {
        battery_level = 0;
    }
    else if(u32AdcValue < BATTERYLE_LEVEL_2)
    {
        battery_level = 1;
    }
    else if(u32AdcValue < BATTERYLE_LEVEL_3)
    {
        battery_level = 2;
    }
    else if(u32AdcValue <= BATTERYLE_LEVEL_4)
    {
        battery_level = 3;
    }
    else
        battery_level = 4;
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
void main (void)
{
    BSP_BoardInit();

#ifdef USE_HEAP
    rt_system_heap_init(CC2530_HEAP_BEGIN, CC2530_HEAP_END);
#endif

    event_timer_init();

    HalAdcInit();

    //BSP_LED_Init();
    HalLcdInit();

    BSP_KEY_Init();

    //BSP_BEEP_Init();

    app_TransmitInit();

    MMA845x_Init();

    app_CardStateInit();

    HalLcdTurnOff();

#ifdef _DEBUG_TICK
    DEBUG_PIN_SEL &= ~0x01;
    DEBUG_PIN_DIR |=  0x01;

    DEBUG_PIN_H;
#endif

    //event_timer_set(EVENT_ADC_MSG);
    event_timer_set(EVENT_REPORT_MSG);
    event_timer_add(EVENT_LOCATE_MSG, 10);
    //event_timer_add(EVENT_REPORT_MSG, BOOT_DURATION_TIME);
    //event_timer_add_reload(EVENT_UPDATE_MSG,1000);
    event_timer_add_reload(EVENT_LOCATE_MSG, LOCATE_TIME);

    while (1)
    {
        event_timer_update();
        app_event_proc();
        cardTransInfoUpdata();
#ifdef OPEN_WTD
        BSP_WATCHDOG_Feed();
#endif


#ifdef OPEN_SLEEP
        if(display_status == LCD_DISPLAY_OFF)
        {
            app_savepower();
        }



#endif
    }
}
