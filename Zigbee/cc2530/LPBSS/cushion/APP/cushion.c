/*******************************************************************************
  Filename:     app_card.c

  Description:  Ӧ�ó������ļ�

*******************************************************************************/

/*******************************************************************************
* INCLUDES
*/
#include <rf.h>
#include <bsp.h>
#include <bsp_led.h>
#include <bsp_key.h>
#include <cushion.h>
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
#include <hal_button.h>
//#include "mac_api.h"


#include "..\..\..\..\..\common\crc.h"




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

//static bool OldStatus = true;
uint16 endDevID = 0xFFFF;   //���Լ���ID
uint16 midDevID = 0xFFFF;   //����ID
LPBSS_device_ID_e endDevType;  //���Լ����豸��������Ա��

extern uint8 recframe[];//128
extern uint8 sendframe[];
extern uint8 sendframeLen;
extern LF_CARD_STATE_E cardState;
extern APP_WORK_STATE_T         s_stAppWork;

uint8 key_status = 0;
int event_count=0;
int is_getstart =0;
uint8 report_num = 0;

/*******************************************************************************
* LOCAL FUNCTIONS DECLARATION
*/
static VOID app_Report(UINT16 dstaddr, UINT16 dstpanid);

static VOID app_TransmitInit(VOID);

static UINT16 app_BCDToDec(UINT8 u8Byte);

static UINT16 app_CheckIEEEInfo(UINT8 *pu8IEEEAddr);
static void app_LocateProc(void);


/*******************************************************************************
* LOCAL VARIABLES
*/
static RF_CFG_T                 s_stAppRfCfg;
static CUSHION_LOC_T   s_stAppTxPkt;
static RF_DATA_T *stPkt;
/*******************************************************************************
* LOCAL FUNCTIONS DEFINED
*/

/*******************************************************************************
* @fn          app_BCDToDec
*
* @brief       ��һ���ֽڵ�����ת���������������4λ�͵���λ��ɵ�
*              ʮ������
* @param    input-   u8Byte - ������ת������
*
*
* @return      ת�����
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

VOID app_StartGetBattery()
{
    P1_2 = 0x01;      //�򿪲�����ѹ
}

VOID app_EndGetBattery()
{
    P1_2 = 0x00;      //�򿪲�����ѹ
}

/*******************************************************************************
* @fn          app_CheckIEEEInfo
*
* @brief       �����������IEEE ��ַ�Ƿ���Ϲ涨
*
* @param    input-   pu8IEEEAddr - ָ�򴫹�����IEEE��ַ
*
*
* @return      ���ȫ�����ͨ����ô����һ���豸�ε�ַ
*/
static UINT16 app_CheckIEEEInfo(UINT8 *pu8IEEEAddr)
{
    UINT32 u32ShortAddr;

    ASSERT(pu8IEEEAddr);
    // ASSERT(EXT_LPBSS_MAC_TYPE_CARD == pu8IEEEAddr[EXT_LPBSS_MAC_TYPE]);
    ASSERT(!pu8IEEEAddr[LPBSS_MAC_CHA]
            || IEEE_MAC_CHA_MIN <= pu8IEEEAddr[LPBSS_MAC_CHA]);
    ASSERT(IEEE_MAC_CHA_MAX >= pu8IEEEAddr[LPBSS_MAC_CHA]);
 //   ASSERT(pu8IEEEAddr[LPBSS_MAC_CARD_TYPE] == SOS_DEVICE_ID);
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
* @brief       ������ѭ����ÿ���ϱ�һ��֡
*
* @param       u32Events - os ��������¼�
*
*
* @return      none
*/
static VOID app_Report(UINT16 dstaddr, UINT16 dstpanid)
{
    UINT16 u16LittleEnd=0;
    u16LittleEnd = CRC16((UINT8*)&(s_stAppTxPkt), PACKET_SIZE-2,0xffff);
//    s_stAppTxPkt.u16Crcsum = (u16LittleEnd>>8) | (u16LittleEnd<<8);
	s_stAppTxPkt.u16Crcsum = u16LittleEnd;

    RF_SendPacket(dstaddr, dstpanid, (UINT8*)&(s_stAppTxPkt), PACKET_SIZE);

    s_stAppTxPkt.u16Seqnum++;

#ifdef OPEN_WTD
    BSP_WATCHDOG_Feed();
#endif
}



static BOOL app_send_version(VOID)
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

	pstCardVerInfo = rt_malloc(sizeof(VERSION) + sizeof(RELEASE)
		+ sizeof(CARD_VERSION_INFO_T));

	if (!pstCardVerInfo)
	{
		return false;
	}

	pstCardVerInfo->u8MsgType = CARD_VERINFO;
	pstCardVerInfo->u8DevType = MATESS_ALERTOR_DEVICE_ID;
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
* @fn          app_TransmitInit
*
* @brief       ���������ʼ������
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
    s_stAppRfCfg.u16PanId = SOS_CARD_PANID;
    s_stAppRfCfg.u8Channel = !au8IEEEAddr[LPBSS_MAC_CHA] * LPBSS_MAC_CHA_DEFAULT
                            + au8IEEEAddr[LPBSS_MAC_CHA];
    s_stAppRfCfg.bAckReq = false;

    ASSERT(RF_Init(&s_stAppRfCfg, &stRfDevCfg) != FAILURE);

    // RF puts on receiver before transmission of packet, and turns off
    // after packet is sent
    RF_ReceiveOff();

//    app_StartGetBattery();

    // Initalise packet payload
    s_stAppTxPkt.u8MsgType  = MATTESS_ALERTOR_LOC;
    s_stAppTxPkt.u16Seqnum  = 0;
//    s_stAppTxPkt.u8Battery  = BSP_ADC_GetVdd();
    s_stAppTxPkt.u8SoftVer  = 0x0;     //0x12;
    s_stAppTxPkt.u8Status   = 128;
    key_status = 0;
    s_stAppTxPkt.u8Model    = au8IEEEAddr[LPBSS_MAC_MODEL];

//    app_EndGetBattery();
}

/*******************************************************************************
* @fn          app_Delay
*
* @brief       ���뼶��ʱ
*
* @param      input - timeout ��ʱ�ĳ���
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
* @brief       ���뼶��ʱ
*
* @param      input - timeout ����ȷ�ϵ�ʱ�䣬��λMS
*
* @return      ��������ﵽȷ��ʱ���򷵻�true,���򷵻�false
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

/*******************************************************************************
* @fn          app_HelpSurvey
*
* @brief       ������Ȱ����ĺ���
*
* @param      none
*
* @return      none
*/
static VOID app_HelpSurvey(VOID)
{
	UINT16 pre_type =0,cur_type=0;
	if(P1_3)
	{
		pre_type =1;
	}

	app_Delay(500);

	if(P1_3)
		cur_type =1;
	else
		cur_type =0;

	if(cur_type == pre_type)
	{
		if(!P1_3)                    //�½���,��������
		{
			s_stAppTxPkt.u8Status |= MATTESS_STATUS_SIT;
		}

		else                  //�����أ������봲
		{
			s_stAppTxPkt.u8Status &= ~(MATTESS_STATUS_SIT);
		}
	    app_LocateProc();
        report_num = 0;
        event_timer_add(EVENT_REPORT_LOC, 100);
	}
	return;
}

/*
* �������
*/
static void app_AdcCheck(void)
{
    static UINT32 u32AdcValue = 0;
    static UINT8  u8HungerCnt = 0;
    app_StartGetBattery();
    u32AdcValue = BSP_ADC_GetVdd();
	if (u32AdcValue < VDD_LIMITE)
    {
        s_stAppTxPkt.u8Status |= MATTESS_STATUS_HUNGER;         //�͵�
    }
    else
    {
        s_stAppTxPkt.u8Status &= ~MATTESS_STATUS_HUNGER;
    }
//    event_timer_add(EVENT_ADC_MSG, CHECK_ADC_DELAY_TICK);
	app_EndGetBattery();
}

/*
* ��λ��Ϣ֡�ϱ�
*/
static void app_LocateProc(void)
{

//    s_stAppTxPkt.u8Status = P1_3;

    app_Report(BROADCAST_ADDR, READ_STATION_PANID);
}

//����Ϣ���մ���
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

RET:
    RF_RevertPkt(stPkt);
}


/*
** ����������
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

    P1_7 = 0;
    P2_0 = 0;

    BSP_SLEEP_Enter(next_tick);
}

static void app_event_proc(void)
{
    uint32 events = event_timer_take_all();


    if (events & EVENT_MAC_MSG) //�����յ�����Ϣ
    {
        app_msg_recv_proc();
    }

    if (events & EVENT_KEY_MSG)
    {
        app_HelpSurvey();
    }
/*
    if (events & EVENT_ADC_MSG)
    {
        //if((cardState == CARD_STATE_FREE)||(cardState == CARD_STATE_WAIT))
        {
            app_AdcCheck();
        }
    }
*/
    if (events & EVENT_LOCATE_MSG)
    {
    	event_count++;
		if(is_getstart)      //just start power
		{
			app_AdcCheck();
			app_LocateProc();
	//		app_Delay(100);
	//		app_send_version();
			is_getstart =0;
		}
		if(event_count%15 ==0)
		{
			app_AdcCheck();
			app_LocateProc();
//			if(event_count %180)
//				app_send_version();
		}

    }

    if (events & EVENT_REPORT_LOC)
    {
        report_num++;
        app_LocateProc();
        if(report_num < 2)
            event_timer_add(EVENT_REPORT_LOC, 100); // BOOT_DURATION_TIME

    }
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

    BSP_LED_Init();
    BSP_KEY_Init();

    P1_7 = 0;
    P2_0 = 1;

    app_TransmitInit();

#ifdef _DEBUG_TICK
    DEBUG_PIN_SEL &= ~0x01;
    DEBUG_PIN_DIR |=  0x01;

    DEBUG_PIN_H;
#endif
   is_getstart =1;

   event_timer_add(EVENT_LOCATE_MSG, 1000); // BOOT_DURATION_TIME

	event_timer_add_reload(EVENT_LOCATE_MSG, 20*1000);

    while (1)
    {

        event_timer_update();
        app_event_proc();
        //cardTransInfoUpdata();

#ifdef OPEN_WTD
        BSP_WATCHDOG_Feed();
#endif

#ifdef OPEN_SLEEP
        app_savepower();
#endif
    }
}
