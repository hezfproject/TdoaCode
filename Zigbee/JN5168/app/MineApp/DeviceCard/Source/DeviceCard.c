
/****************************************************************************/
/***        Include files                                                 ***/
/****************************************************************************/
#include <jendefs.h>
#include <AppHardwareApi.h>
#include <AppQueueApi.h>
#include <mac_sap.h>
#include <mac_pib.h>
#include <AppApiTof.h>
#include <OAD.h>
#include <stdlib.h>

#include "config.h"
#include "app_protocol.h"
#include "JN5148_util.h"
#include "printf_util.h"
#include "mma8452q.h"
#include "string.h"
#include "spi.h"
#include "as3933.h"
#include "crc.h"

/****************************************************************************/
/***        Macro Definitions                                             ***/
/****************************************************************************/

#if (defined DEBUG_CARD)
#define DBG(x) do{x} while(0);
#else
#define DBG(x)
#endif

#define TOF_VDD_3000		300	// 3.0v
#define TOF_VDD_3800		380	// 3.8v
#define TOF_VDD_3900		390	// 3.9v

#define TOF_OK_PIN			E_AHI_DIO15_INT
#define TOF_HELP_PIN		E_AHI_DIO14_INT
#define TOF_CHECKIN_PIN	E_AHI_DIO10_INT
#define TOF_BUZZER_PIN		E_AHI_DIO13_INT
#define TOF_EXCITER_SAD_PIN     E_AHI_DIO17_INT
#define TOF_EXCITER_CLK_PIN     E_AHI_DIO16_INT
#define TOF_MOTION_IT_PIN       E_AHI_DIO0_INT


#define TOF_BUZZER_TIMER	E_AHI_TIMER_3	// for buzzer's input
#define TOF_SHAKER_TIMER	E_AHI_TIMER_1	// for buzzer's input

#define TOF_OAD_EVENT					BIT(3)	// the card's OAD event

#define TOF_LED_RED_OFF_EVENT			BIT(5)	// the card's red LED off
#define TOF_LED_GREEN_OFF_EVENT			BIT(6)	// the card's green LED off
#define TOF_ACK_EVENT					BIT(7)	// sleep

#define TOF_READCARD_EVENT				BIT(8)	// sleep

#define TOF_SLEEP_EVENT					BIT(9)	// sleep
#define TOF_CHECK_HELP_EVENT			BIT(10)	// check if the help button is press or not
#define TOF_HELP_EVENT					BIT(11)	// the card is helping

#define TOF_LISTEN_EVENT				BIT(12)	// card listen: to search station
#define TOF_LISTEN_TIMEOUT_EVENT		BIT(13)	// no station broadcast
#define TOF_REQUEST_EVENT				BIT(15)	// send join request msg to station
#define TOF_RSSI_EVENT					BIT(16)	// send RSSI request msg
#define TOF_STATION_EVENT				BIT(17)	// card wait station to do tof
#define TOF_STATION_DONE_EVENT		BIT(18)	// the station finish tof
#define TOF_LOCATOR_EVENT				BIT(19)	// the card need locator to do TOF
#define TOF_LOCATOR_DONE_EVENT		BIT(20)	// the locator finish tof
#define TOF_ALARM_EVENT				BIT(21)	// the card alarm
#define TOF_ALARM_TIMEOUT_EVENT		BIT(22)	// the card alarm timeout within 1s

#define TOF_EXCITE_EVENT            BIT(23)
#define TOF_EXCITE_TIMEOUT_EVENT    BIT(24)
#define TOF_SYNC_EVENT					BIT(25)	// card change to common channel to sync with station
#define TOF_CHECKIN_EVENT				BIT(26)	// card checkin
#define TOF_BUZZER_EVENT				BIT(27)	// card buzzer
#define TOF_STOP_BUZZER_EVENT			BIT(28)	// stop buzzer
#define TOF_RETREAT_EVENT				BIT(29)	// station retreat
#define TOF_WATCHDOG_EVENT			BIT(30)	// feed watch dog event
#define TOF_RSSI_SEND_TIMEOUT_EVENT			BIT(31)


#define TOF_HELP_BUTTON_MS			2000	// when press help button for 1s, will trick the help event
#define TOF_CARD_HELP_TIMES			20		// help 10 times (1 second per time)
#define TOF_CARD_CHECKIN_TIMES		3		// card re-checkin 4 times
#define TOF_CARD_RETREAT_ACK_TIMES  5      // retreat ack 5 times (1 second per time)

#define TOF_CARD_SYNC_DELTA_MAX		30		// card wakeup at most TOF_CARD_SYNC_DELTA_MAX ms before station do TOF
#define TOF_CARD_SYNC_DELTA_MIN		10		// card wakeup at least TOF_CARD_SYNC_DELTA_MIN ms before station do TOF

#define TOF_RETREAT_PERIOD_MS			500 		// card flash and buzzer every 500ms
#define TOF_CARD_HELP_PERIOD_MS		500 		// LED flash every 500ms
#define TOF_LED_RED_MS					10		// red led on TOF_LED_RED_MS ms
#define TOF_LED_GREEN_MS				10		// green led on TOF_LED_RED_MS ms

#define TOF_CHECKIN_TIMEOUT			(TOF_SLOT_MS*2+3)	// checkin timeout
#define TOF_LISTEN_TIMEOUT				(TOF_SLOT_MS*2+3)	// listen timeout
#define TOF_ALARM_TIMEOUT				(TOF_SLOT_MS*6+3)	// alarm timeout
#define TOF_SYNC_TIMEOUT				(TOF_SLOT_MS*6+3)	// when lost station, or wakeup from button, need sync with station
#define TOF_RSSI_TIMEOUT				3					// need 3ms to send rssi
#define TOF_PENDING_TIMEOUT			(60)		// timeout: waiting for station to do tof
#define TOF_REQUEST_TIMEOUT			20					// send request to join's timeout
#define TOF_BUZZER_TIMEOUT			200					// buzzer timeout
#define CARD_5S_HELP_STATUS_TIMEOUT    180                 // 5S card:15 minutes
#define CARD_1S_HELP_STATUS_TIMEOUT    (CARD_5S_HELP_STATUS_TIMEOUT*5)    // 1S card:15 minutes
#define CARD_15S_MOTION_CNT           8        //2minutes
#define CARD_5S_MOTION_CNT            24       //2minutes
#define CARD_1S_MOTION_CNT            120      //2minutes

#define MAX_STATION_STATUS_NORMAL   1
#define PARAM_STORE_ADDR 	0x38000
#define INFO_STORE_ADDR 	0x30000
#define PERIOD_STORE_ADDR 	0x28000

#define D_VALUE  4
#define DEFF(x,y)   ((x)>=(y)?((x)-(y)):((y)-(x)))

#define OPTION_CARDNUM     "cardnum="
#define OPTION_CHANNEL     "channel="
#define OPTION_CARDTYPE    "cardtype="
#define OPTION_RSSICH      "rssich="
#define OPTION_REBOOT   "reboot"
#define OPTION_ERASE    "erase"
#define OPTION_SHOW     "show"
#define OPTION_HELP     "help"
#define OPTION_SAVEENV  "saveenv"




#define DATA_FILE_MAX_SIZE  (1024 * 4*4)
#define BLOCK_CNT   4
#define BLOCK_SIZE   DATA_FILE_MAX_SIZE/BLOCK_CNT



typedef struct
{
	unsigned int   r_pos;
  unsigned int   w_pos;
  unsigned int   state;
}data_block_state;

/*typedef struct
{
	unsigned int	current_write_block;
	unsigned int    u8writetotalcnt;
	data_block_state block_state[BLOCK_CNT];
}data_rw_state;


//static  data_rw_state ldata_state;



/***        Type Definitions                                              ***/
/****************************************************************************/
typedef enum
{
    E_TOF_STATE_IDLE,					// nothing to do
    E_TOF_STATE_LISTEN,				// card listen
    E_TOF_STATE_SYNC,					// card sync
    E_TOF_STATE_RSSI,					// card send rssi
    E_TOF_STATE_REQUEST_STATION,	// card send join request
    E_TOF_STATE_STATION_ACCEPT,		// station accept card's join request
    E_TOF_STATE_WAIT_STATION_TOF,	// wait station to do tof
    E_TOF_STATE_STATION_FINISH,		// station finish tof with card
    E_TOF_STATE_STATION_WAIT,		// station finish tof with card
    E_TOF_STATE_WAIT_LOCATOR,		// wait locator to do tof
    E_TOF_STATE_LOCATOR_FINISH,		// locator finish tof with card
    E_TOF_STATE_ALARM,				// card alarm
    E_TOF_STATE_WAIT_ALARM_ACK,		// card wait alarm's ack
    //E_TOF_STATE_WAIT_TO_SLEEP,
} teCardState;

typedef enum
{
    CARD_TOF_5S = 0,
    CARD_TOF_1S = 1,
    CARD_TOF_15S = 2,
} teCardTofPeriod;

typedef struct
{
    uint16 StationPanid;
    uint16 StationShortAddr;
    uint8      u8LinkQuality;    /**< Link quality of received frame */
    uint8      u8ListenTimeOutCount;
    uint8      u8ListenCount;
} RxFrameData_List;


typedef struct
{
	uint8 u8Sysmod;
	uint8 u8IntSrc;
	uint8 u8FFMtSrc;
	uint8 u8TransientSrc;
	uint8 u8XbyteH;
	uint8 u8YbyteH;
	uint8 u8ZbyteH;
}teMMA845x_Data;

typedef struct
{
    uint8 u8Status;
    uint8 u8CardType;
    uint16 u16ShortAddr;
    uint8 u8Channel;
    uint8 u8RssiCh;
}CFG_OPTION_T;
typedef struct
{
  app_header_t tof_head;
  uint8 ackmsg;
  uint8 sequnum;
}ACK_PACKET_T;

//PRIVATE teMMA845x_Data mma845xData;

PRIVATE CFG_OPTION_T cfg_option;

// 信号侦听阶段存储的最强候选基站号
PRIVATE RxFrameData_List LQ_StationPanid;
PRIVATE RxFrameData_List LQ_StationPanidNext;

// 当前信号侦听的状态 0: normal , 1 hungry
// hungry 状态下，收到任意一个基站的信号立即加入。
// 当某个周期收不到任何基站信号时，转入hungry状态
#define   LQ_STATION_SEARCH_STATUS_NORMAL  0
#define   LQ_STATION_SEARCH_STATUS_HUNGRY  1

PRIVATE uint8	 LQ_StationSearchStatus;
PRIVATE RfTofWrapper_tu* psAppPkt_Temp = NULL;
/****************************************************************************/
/***        Local Function Prototypes                                     ***/
/****************************************************************************/
PRIVATE void vInitCardSystem(void);
PRIVATE void vInitParam();
PRIVATE void vProcessIncomingMlme(MAC_MlmeDcfmInd_s *psMlmeInd);
PRIVATE void vProcessIncomingMcps(MAC_McpsDcfmInd_s *psMcpsInd);
PRIVATE void vProcessIncomingHwEvent(AppQApiHwInd_s *psAHI_Ind);
PRIVATE void vHandleMcpsDataInd(MAC_McpsDcfmInd_s *psMcpsInd);
PRIVATE void vHandleMcpsDataDcfm(MAC_McpsDcfmInd_s *psMcpsInd);
PRIVATE void vCardDeviceStartup();
//PRIVATE void vCardMotionDetect();
PRIVATE void vProcessSysEvent();
PRIVATE void vProcessAppEvent();

PRIVATE void vCardCast(uint8 u8CmdType, uint16 u16DstAddr, uint16 u16DstPanId, uint8 u8TxOptions, uint16 u16value);
PRIVATE void vSendRssi();
PRIVATE void vSendAlarm(uint16 u16ShortAddr, uint16 u16PanId);
PRIVATE void vOnlySendRssi();

PRIVATE void vCardSleep(uint32 u32SleepMs);
PRIVATE void vSetLed(uint32 u32Pin, bool_t bOnOff, uint16 u16OnMs);
PRIVATE void vSetCardChannel(uint8 channel);
PRIVATE void vSyncStation(RfTofWrapper_tu* psAppPkt);
PRIVATE void vBuzzer(uint32 u32Ms);
PRIVATE void vStopBuzzer();
PRIVATE void vCheckStationStatus(uint8 u8StationStatus);
PRIVATE void vDioCallBack(uint32 u32Device, uint32 u32ItemBitmap);
PRIVATE bool  vSaveCardStatus(uint8 CardStatus);
PRIVATE void vGetLocatorMs(void);
PRIVATE void vCardTypeCheck(uint8 PeriodType);
PRIVATE void vFetchCardVersion(uint8* p,uint8 u8LenVer);
PRIVATE void vCardShortAddrCheck(uint16 u16Addr,uint32 u32BCD);
PRIVATE void vCheckVddValue(void);

PRIVATE bool vGet_bootparam(void);
PRIVATE void vBootsh_show_menu(void);
PRIVATE void vBootsh_run_line(char* cmd, int cmdlen);
PRIVATE bool vGet_flashparam(void);
PRIVATE void vGet_MACAddress(void);

//PRIVATE void vDeviceCardCast( uint16 u16DstAddr, uint16 u16DstPanId, char * sendBuffer,uint32 len);
//PRIVATE int load_location_data(unsigned char* p_buf, int buf_len);
//PRIVATE int dump_location_data( unsigned char* p_data, int data_len);
//PRIVATE void ldata_state_init(void);
PRIVATE uint8 vParse_card_info(uint8* pdata,uint8 len);
//PRIVATE void receiveAck(uint8 sequnum,uint8 ackmsg);
uint8 calcrc_1byte(uint8 abyte);
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

// card's short addr / pan ID, and station's short addr / pan ID
PRIVATE uint16 u16CardShortAddr;
PRIVATE uint16 u16CardPanId;
PRIVATE uint16 u16StationShortAddr;
PRIVATE uint16 u16StationPanId;
//PRIVATE uint16 u16HandStationShortAddr;
//PRIVATE uint16 u16HandStationPanId;

PRIVATE uint8 u8StationChannel;		// channel for station do tof with card
PRIVATE uint8 u8LocatorChannel;		// channel for locator do tof with card
PRIVATE uint8 u8BroadcastChannel;	// channel for station to send signal msg
PRIVATE uint8 u8CurChannel;			// card's current channel
PRIVATE uint8 u8RssiChannel;

PRIVATE uint32 u32WakeupEvent = TOF_LISTEN_EVENT;	// card wakeup event
PRIVATE uint8 u8NeedLocator = 0;		// if need locator
PRIVATE uint8 u8LocToCardIndex = 0xFF;		// seqnum of TOF with card

PRIVATE uint16 u16CardSlot;			// the station slot index for me, use this to quickly sync with station if need
PRIVATE uint16 u16SeqNum = 0;		// card's sequence number
PRIVATE uint8 u8LostTimes = TOF_STATION_CARD_MAX_FAILED;	// to record the times that card lost station
PRIVATE uint8 u8ListenIdleTime = 0;                         // to record the times that card listen none station signal
PRIVATE bool_t bHelpSync = FALSE;			// is the sync is from card help event
PRIVATE bool_t bTofConnected = FALSE;		// connect with station or not
PRIVATE bool_t bRSSIed = FALSE;			// weather card send rssi msg during the sleep period
PRIVATE uint16 u16Battery = 0;				// battery power, unit is  0.1v
PRIVATE bool_t bReportNoPwd = FALSE;		// report no power success or not
PRIVATE bool_t bNoPwd =  FALSE;			// no power or not
PRIVATE bool_t bRetreatAck = FALSE;		// if station retreat, use need to press OK button to stop buzzer
PRIVATE uint8 u8RetreatAckCnt = TOF_CARD_RETREAT_ACK_TIMES;  //count for retreat ack times
PRIVATE bool_t bHelped = FALSE;			// station receive card's help or not
PRIVATE uint8 u8HelpCnt = TOF_CARD_HELP_TIMES;	// couter for help times
PRIVATE uint8 u8CheckinCnt = 0; 					// couter for checkin
PRIVATE uint8 u8ExciteCnt = 0; 					// couter for excite
PRIVATE bool_t u8ExciteTure = FALSE; 					// couter for excite
PRIVATE uint8 u8LocCnt = 0;
PRIVATE bool_t u8ReportDistance = FALSE;
//PRIVATE bool_t u8WriteData = FALSE; 					// couter for excite

PRIVATE bool_t bMsgSending = FALSE;    // if the msg is still sending or not
PRIVATE uint8 bProtectCnt = 0;

PRIVATE uint8 u8ExciteAddr = 0;

PRIVATE uint8 u8iCount = 0;
PRIVATE uint8 u8LowPowerConf = 0;    //low power confirm
PRIVATE uint8 u8HighPowerConf = 0;    //high power confirm
PRIVATE uint8 u8SendCnt = 0;
//PRIVATE uint8 u8LastSeqnum;
PRIVATE uint32 u32Timeout = 0;
PRIVATE teCardState eCardTofState;					// card's state
PRIVATE teCardState eCardRssiState = FALSE;					// card's state

PRIVATE uint8 u8CardStatus = CARD_STATUS_NORMAL;	// card's status, should be normal, help, nopwd, retreat
PRIVATE uint8 u8CardMotion = 0;
PRIVATE uint8 u8LastHandle;				// to record the last handle of the sending msg

PRIVATE uint32 u32WaitLocatorTimeout;	// timeout for wait locator to do TOF
PRIVATE uint32 u32LocatorRecTick = 0;		// to record the beginning tick of wait locator
PRIVATE uint32 u32StationRecTick = 0;		// to record the tick that received station's finish msg
PRIVATE uint16 u16StationConnectPrt = 0;
PRIVATE uint16 u16StationMs = TOF_STATION_PERIOD_MS;		// the coming time of my next slot
PRIVATE uint16 u16LocatorMs;				// the coming time of the locator
PRIVATE uint16 u16NextLocatorMs;            // the next coming time of the locator
PRIVATE uint8	u8StationRunMs;				// the station has ran ms in current slot
PRIVATE uint8   u8CardRunMs;
PRIVATE uint8	u16SleepDelta;
PRIVATE uint8	u16WakeDelta = TOF_CARD_SYNC_DELTA_MAX;
PRIVATE uint8 u8ProtectCounter = 0;
PRIVATE uint16 u16CardHelpStatusCnt = 0;
//PRIVATE uint8 u8BuzzerProtect = 0;
PRIVATE uint8 tofPeriodType = CARD_TOF_1S;
PRIVATE uint16 tofSlotNum = TOF_SLOT_NUM;
PRIVATE uint16 tofStationPeriodMs = TOF_STATION_PERIOD_MS;
PRIVATE uint16 u16CardHelpTimeout = CARD_5S_HELP_STATUS_TIMEOUT;
PRIVATE uint16 u16CardMotionTimeout = CARD_5S_MOTION_CNT;

PRIVATE uint16 u16CardMotionCnt = 0;

PRIVATE uint8 u8CardStatusNormal = 0;

PRIVATE uint8 u8HelpCntError = 0;
PRIVATE uint32 u16SendVersionCnt = 0;

PRIVATE uint8 au8UartRxBuffer[100];

PRIVATE uint16 u16CardVersion = 0xFFFF;
PRIVATE uint32 u32BootDelayCnt = 0;
//PRIVATE uint8 u8readsequnum = 0;
//PRIVATE uint8 receivedAck=TRUE;
//PRIVATE uint8 checkin_ack=FALSE;

PRIVATE uint8 u8MaxCnt = 5;
PRIVATE uint8 u8LedFlag = 1;

PRIVATE uint8 u8AckCnt;


typedef struct
{
	uint8 u8ErpIdLen;
    uint8 u8ErpBuf[20];
}rt_erpid_info_ts;

rt_erpid_info_ts rt_erpid_info;


#define CMD_LINE_MAX 256

typedef struct bootsh_t
{
    char line[CMD_LINE_MAX];
    int line_position;
}BOOTSH_T;

typedef struct bootsh_cmd_t
{
    char *cmd;
    void(*pfn)(char *op, const char* pos, uint32* pVal);
    uint32 *pVal;
}BOOTSH_CMD_T;



PRIVATE BOOTSH_T _bootsh;
PRIVATE BOOTSH_T *bootsh = &_bootsh;

//uint8 flash_read_buffer[128];
//uint8 flash_write_buffer[4096];
//uint8 flash_check_buffer[4096];
PRIVATE uint8 flash_info[0x2000];     //8k
PRIVATE uint8 flash_info_temp[0x2000];     //8k


PRIVATE uint8 read_buf[128];
PRIVATE uint8 write_buf1[128];
//PRIVATE uint8 write_buf2[128];

#define CARD_INFO_SIZE   64


typedef struct card_info_t
{
    uint8 u8Frame_sum;
    uint8 u8Frame_cur;
    uint8 u8BaseInfoCnt;
}CARD_INFO_T;

CARD_INFO_T card_info_cfg;
PRIVATE uint8 u8FrameCnt = 0;

PRIVATE uint8 u8ReportIdx = 0;


typedef enum
{
    REPORT_IDLE = 0,
    //REPORT_ALARM = 1,
    READ_INFO = 1,
    WRITE_INFO = 2,
} teReportCardState;



PRIVATE uint8 u8CardPeriod = 254;     //一天的卡
PRIVATE uint8 u8MaxTofCnt = 1;
PRIVATE uint16 u16MaxWarmStartCnt = 20;   //24h*60min*60S / 15S


PRIVATE uint16 u16SleepCnt;
PRIVATE uint8 u8SleepMode = 0;    //1:长时间休眠;0:正常休眠,2:激励

PRIVATE uint32 u32state = REPORT_IDLE;


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
    // watch dog is start by default (16392ms)
    vInitCardSystem();
    vInitParam();
    vFetchCardVersion(VERSION,strlen(VERSION));
    vCardDeviceStartup();

    uint8 CardProtect;
    if(!bAHI_FullFlashRead(PARAM_STORE_ADDR, sizeof(CardProtect),(uint8*)&CardProtect))
    {
        DBG(PrintfUtil_vPrintf("Flash Read Fail\n"););
    }
    else if((CardProtect & CARD_STATUS_RETREAT) && (CardProtect != 0xFF))
    {
        EventUtil_vSetEvent(TOF_RETREAT_EVENT);
        u8CardStatus |= CARD_STATUS_RETREAT;
    }
    EventUtil_vSetEvent(TOF_LISTEN_EVENT);
    TimerUtil_eSetCircleTimer(TOF_WATCHDOG_EVENT, 1000);	// feed watch dog every 1s

    bAHI_FlashInit(E_FL_CHIP_INTERNAL, NULL);
    //ldata_state_init();
    vOADInitClient(TRUE);
    while (1)
    {
        TimerUtil_vUpdate();
        vProcessSysEvent();
        vProcessAppEvent();
    }
}

/****************************************************************************
 *
 * NAME: vInitCardSystem
 *
 * DESCRIPTION:
 * Init system when cold start.
 *
 * PARAMETERS:      None
 *
 * RETURNS:
 * void
 *
 ****************************************************************************/
PRIVATE void vInitCardSystem(void)
{
    (void)u32AppQApiInit(NULL, NULL, vDioCallBack);
    (void)u32AHI_Init();

    vAHI_UartSetRTSCTS(E_AHI_UART_0,FALSE);
    PrintfUtil_u8Init(E_AHI_UART_0, PRINTF_BAUD_RATE_1M,au8UartRxBuffer,sizeof(au8UartRxBuffer));
    PrintfUtil_vPrintf("cold start\n");

    bAHI_FlashInit(E_FL_CHIP_INTERNAL, NULL);

	//MMA845x_Init();

    //As3933_SpiInit();
    //config_As3933();

    bAHI_FullFlashRead(INFO_STORE_ADDR,sizeof(rt_erpid_info),&rt_erpid_info);

    if(rt_erpid_info.u8ErpIdLen >= 20)
        rt_erpid_info.u8ErpIdLen = 0;

    bAHI_FullFlashRead(PERIOD_STORE_ADDR,1,flash_info_temp);

    if(flash_info_temp[0] !=0xff)
    {
        u8CardPeriod = flash_info_temp[0];
        if((u8CardPeriod>4) && (u8CardPeriod<253))
            u8MaxTofCnt = 4;
        else
            u8MaxTofCnt = 1;

        if(u8CardPeriod == 253)    //1分钟
        {
            u16MaxWarmStartCnt = 4;
        //u16SleepCnt = 65000;
        }
        else if(u8CardPeriod == 254)   //5分钟
        {
            u16MaxWarmStartCnt = 20;
        }
        else if(u8CardPeriod == 168)   //仓库模式
            u16MaxWarmStartCnt = u8CardPeriod*3;
        else
            u16MaxWarmStartCnt = u8CardPeriod*60*4;
       
    }

    vAHI_DioSetDirection(0, JN_RED | JN_GREEN);
    vSetLed(JN_GREEN, FALSE, 0); //TRUE, TOF_LED_GREEN_MS);
    vSetLed(JN_RED, FALSE, 0);


    vAHI_DioSetDirection(TOF_CHECKIN_PIN, 0);
    vAHI_DioWakeEnable(TOF_CHECKIN_PIN, 0);
    vAHI_DioWakeEdge(0,TOF_CHECKIN_PIN);
    vAHI_DioSetPullup(TOF_CHECKIN_PIN, TOF_BUZZER_PIN);

	//vAHI_DioSetDirection(TOF_EXCITER_SAD_PIN,0);
    //vAHI_DioSetPullup(0,TOF_EXCITER_SAD_PIN);
	//vAHI_DioInterruptEnable(0,TOF_EXCITER_SAD_PIN);
	//vAHI_DioSetPullup(TOF_EXCITER_SAD_PIN, 0);

	//vAHI_DioSetDirection(TOF_EXCITER_CLK_PIN,0);
	//vAHI_DioWakeEnable(TOF_EXCITER_CLK_PIN,0);
	//vAHI_DioWakeEdge(TOF_EXCITER_CLK_PIN,0);
	//vAHI_DioSetPullup(TOF_EXCITER_CLK_PIN);

	//vAHI_DioSetPullup(0,TOF_EXCITER_CLK_PIN);

    vAHI_DioSetDirection(E_AHI_DIO0_INT,0);
    vAHI_DioSetPullup(0,E_AHI_DIO0_INT);


    vAHI_DioSetDirection(0,E_AHI_DIO1_INT);
    vAHI_DioSetOutput(0,E_AHI_DIO1_INT);

    vAHI_WakeTimerEnable(E_AHI_WAKE_TIMER_0, TRUE);


    if(!vGet_bootparam())
    {
        vBootsh_show_menu();

        cfg_option.u16ShortAddr = u16CardShortAddr;
        cfg_option.u8CardType = tofPeriodType;
        cfg_option.u8Channel = u8BroadcastChannel;
        cfg_option.u8RssiCh = u8RssiChannel;

        if(!vGet_flashparam())
        {
            cfg_option.u16ShortAddr = u16CardShortAddr;
            cfg_option.u8CardType = tofPeriodType;
            cfg_option.u8Channel = u8BroadcastChannel;
            cfg_option.u8RssiCh = u8RssiChannel;
        }

        PrintfUtil_vPrintf("bootsh>>");
        memset(bootsh, 0, sizeof(BOOTSH_T));
        char ch;
        while (1)
    	{
	    	if(u32BootDelayCnt<0x6c0000)
	    	{
	            vAHI_WatchdogRestart();
				u32BootDelayCnt++;
	    	}
            /* read one character from uart */
    		while ((ch = u8UartRead(E_AHI_UART_0)) != 0)
    		{
    			u32BootDelayCnt=0;
    			/* handle CR key */
    			if (ch == '\r')
    			{
    				if ((ch = u8UartRead(E_AHI_UART_0)) == 0)
                        ch = '\n';
    			}
    			/* handle backspace key */
    			else if (ch == 0x7f || ch == 0x08)
    			{
    				if (bootsh->line_position != 0)
    				{
    					PrintfUtil_vPrintf("%c %c", ch, ch);
    				}
    				if (bootsh->line_position <= 0)
                        bootsh->line_position = 0;
    				else
                        bootsh->line_position--;
    				bootsh->line[bootsh->line_position] = 0;
    				continue;
    			}

    			/* handle end of line, break */
    			if (ch == '\n' || ch == '\r')
    			{
    				/* change to ';' and break */
    				bootsh->line[bootsh->line_position] = ';';
                    PrintfUtil_vPrintf("\n");

    				if (bootsh->line_position != 0)
                        vBootsh_run_line(bootsh->line, bootsh->line_position);

    				PrintfUtil_vPrintf("bootsh>>");
    				memset(bootsh->line, 0, sizeof(bootsh->line));
    				bootsh->line_position = 0;

    				break;
    			}

    			/* it's a large line, discard it */
    			if (bootsh->line_position >= CMD_LINE_MAX)
                    bootsh->line_position = 0;

    			/* normal character */
    			bootsh->line[bootsh->line_position] = ch;
                PrintfUtil_vPrintf("%c", ch);
    			bootsh->line_position++;
    		} /* end of device read */
    	}

    }

	vGet_flashparam();
    DBG(PrintfUtil_vPrintf("macAddress cfg\n");)
    vGet_MACAddress();


    u16CardPanId = TOF_CARD_NWK_ADDR;
    vCardTypeCheck(tofPeriodType);

    vCheckVddValue();

    //关掉SPI，激励芯片配置已经完成，为了省电，关掉spi
    vAHI_SpiDisable();

    u8CurChannel = u8BroadcastChannel;
//	u8LocatorChannel = ((psMacAddr.u32L) >> 16) & 0xFF;

    // ***************************************************/
    MacUtil_Setting_s sMacUtil;
    sMacUtil.u16SrcPanId		= TOF_CARD_NWK_ADDR;
    sMacUtil.u8Dst_endpoint 	= 0x21;
    sMacUtil.u8Src_endpoint 	= 0x20;   // ??
    sMacUtil.u16Profile_id 		= 0x2001; //0x2001; //for backward compatable
    sMacUtil.u8NodeType 		= 0x02;
    sMacUtil.u16SrcShortAddr 	= u16CardShortAddr;

    MacUtil_vInit(&sMacUtil);
}

/****************************************************************************
 *
 * NAME: AppWarmStart
 *
 * DESCRIPTION:
 * When card warm up, need determine its wakeup type: normal, checkin or help
 *
 * PARAMETERS:      None
 *
 * RETURNS:
 * void
 *
 ****************************************************************************/
PUBLIC void AppWarmStart(void)
{
    uint32 u32WakeStatus = u32AHI_DioWakeStatus();

	/* Disable JTAG */
	vAHI_SetJTAGdebugger(FALSE, FALSE);
	/* Wait for 32Khz clock to run */
	while((bAHI_Clock32MHzStable() == FALSE) &&  ++u32Timeout < 10000);

    (void)u32AppQApiInit(NULL, NULL, vDioCallBack);
    (void)u32AHI_Init();
	if(!bAHI_FlashInit(E_FL_CHIP_AUTO, NULL))
	{
		PrintfUtil_vPrintf("FlashInit fail\r\n");
	}

    // need enable card's OK button when wakeup (disable when sleep)
    vAHI_DioWakeEnable(TOF_OK_PIN, 0);
    vAHI_DioWakeEdge(0, TOF_OK_PIN);
    DBG(
		vAHI_UartSetRTSCTS(E_AHI_UART_0,FALSE);
        PrintfUtil_u8Init(E_AHI_UART_0, PRINTF_BAUD_RATE_1M,au8UartRxBuffer,sizeof(au8UartRxBuffer));
        PrintfUtil_vPrintf("V: %d, Seq: %d\n", OAD_CARD_VERSION, u16SeqNum);
    )

    u8ProtectCounter = 0;
    u16SeqNum++;
    bHelped = FALSE;
    bHelpSync = FALSE;
    vInitParam();

    vCardDeviceStartup();

    if(u16SendVersionCnt>0)
    {
        u16SendVersionCnt--;
    }

    if(u8CardStatus&CARD_STATUS_HELP)
    {
        if(u16CardHelpStatusCnt++ > u16CardHelpTimeout)
        {
            u8CardStatus &= (~CARD_STATUS_HELP);
        }
    }
    else
    {
        u16CardHelpStatusCnt = 0;
    }

	vCheckVddValue();

    //////////////////////////////////////////////////
    vAHI_DioSetPullup(0, TOF_BUZZER_PIN);	// need pulldown this pin to save battery pwd
    //vAHI_DioSetPullup(0, TOF_BUZZER_PIN);
    //vAHI_TimerDisable(TOF_BUZZER_TIMER);
    /*if(u32WakeStatus & TOF_HELP_PIN)
    {
        // just to wait station coming, not need to consider locator
        //TimerUtil_eSetTimer(TOF_CHECK_HELP_EVENT, TOF_HELP_BUTTON_MS);	// from now on, if 1s later the button is still press, HELP!
        eCardTofState = E_TOF_STATE_WAIT_STATION_TOF;
        vSetCardChannel(u8StationChannel);
        TimerUtil_eSetTimer(TOF_LISTEN_EVENT, tofStationPeriodMs);	// if in this period, there's not station coming, it must be lost, need to listen
    }*/

    if(u32WakeStatus & TOF_CHECKIN_PIN)
    {
        u8CheckinCnt = 0;
        //EventUtil_vSetEvent(TOF_CHECKIN_EVENT);
        TimerUtil_eSetTimer(TOF_CHECKIN_EVENT,20);
        TimerUtil_eSetTimer(TOF_LISTEN_EVENT, TOF_CHECKIN_TIMEOUT+TOF_BUZZER_TIMEOUT);
    }
    else if(TOF_STATION_EVENT == u32WakeupEvent)
    {
        //FIXME, for beta only:share E_TOF_STATE_WAIT_STATION_TOF state
        eCardTofState = E_TOF_STATE_WAIT_STATION_TOF;
        eCardRssiState = TRUE;

        //DBG(PrintfUtil_vPrintf("AAA\n");)
        vSendRssi();
        u16SleepCnt = 0;

        //timeout
        TimerUtil_eSetTimer(u32WakeupEvent,15);
    }

    //else if(u8SleepMode == 1 && u16SleepCnt < (u8CardPeriod*3))
    else if(u8SleepMode == 1 && u16SleepCnt < u16MaxWarmStartCnt)
    {
        //DBG(PrintfUtil_vPrintf("!!!%d,%d,%d\n",u8SleepMode,u16SleepCnt,u8CardPeriod);)
        u16SeqNum--;
        u8LedFlag = 0;
        if(u8CardPeriod == 168)   //仓库模式
            vCardSleep(1200000);
        else
        {
            if (bNoPwd)
                vSetLed(JN_RED, TRUE, 3);
            else
                vSetLed(JN_GREEN, TRUE, 3);
            vOnlySendRssi();
        }
        //vCardSleep(1500);

        //eCardTofState = E_TOF_STATE_WAIT_TO_SLEEP;
        //EventUtil_vSetEvent(TOF_SLEEP_EVENT);
    }
    else
    {
        eCardRssiState = TRUE;
        vSendRssi();
        u16SleepCnt = 0;

        //timeout
        TimerUtil_eSetTimer(u32WakeupEvent,15);
    }
    //////////////////////////////////////////////////
    // LED flash when wakeup
    // if card is wakeup for locator, not need to flash
    if(u32WakeupEvent != TOF_LOCATOR_EVENT && u8LedFlag)
    {
        if (bNoPwd)
            vSetLed(JN_RED, TRUE, TOF_LED_RED_MS);
        else
            vSetLed(JN_GREEN, TRUE, TOF_LED_GREEN_MS);
    }
    u8LedFlag = 1;

    PrintfUtil_vPrintf("rssi channel:%d,boardcastchannel:%d\n",u8RssiChannel,u8BroadcastChannel);

    DBG(PrintfUtil_vPrintf("warm:%d,%d,%d,%d,%d\n",u8SleepMode,u16SleepCnt,u8CardPeriod,u8MaxTofCnt,u8LocCnt);)
    TimerUtil_eSetCircleTimer(TOF_WATCHDOG_EVENT, 1000);	// feed watch dog every 1s
    while(1)
    {
        TimerUtil_vUpdate();
        vProcessSysEvent();
        vProcessAppEvent();
    }
}


/****************************************************************************
 *
 * NAME: vCardDeviceStartup
 *
 * DESCRIPTION:
 * Init mac settings, reset tof function, enable timer, set high power mode
 *
 * PARAMETERS:      None
 *
 * RETURNS:
 * void
 *
 ****************************************************************************/
PRIVATE void vCardDeviceStartup()
{
    vAppApiTofInit(FALSE);
    vAppApiTofInit(TRUE);

    //vAHI_ProtocolPower(TRUE);
    //vAHI_BbcSetHigherDataRate(E_AHI_BBC_CTRL_DATA_RATE_500_KBPS);
    //vAHI_BbcSetInterFrameGap(50);

    vAHI_HighPowerModuleEnable(TRUE, TRUE);
    TimerUtil_vInit();

    s_pvMac = pvAppApiGetMacHandle();
    s_psMacPib = MAC_psPibGetHandle(s_pvMac);

    MAC_vPibSetRxOnWhenIdle(s_pvMac, TRUE, FALSE);
    MAC_vPibSetMaxCsmaBackoffs(s_pvMac, 2);
    MAC_vPibSetMinBe(s_pvMac, 1);


    MAC_vPibSetPanId(s_pvMac, u16CardPanId);
    MAC_vPibSetShortAddr(s_pvMac, u16CardShortAddr);

    s_psMacPib->u8MaxFrameRetries 	= 1;
    s_psMacPib->sCoordExtAddr.u32H	= 0xAAAA;	// fake
    s_psMacPib->sCoordExtAddr.u32L 	= 0xBBBB;	// fake
    s_psMacPib->u16CoordShortAddr 	= 0x0000;	// fake
    s_psMacPib->u8SecurityMode 		= 0;

    eAppApiPlmeSet(PHY_PIB_ATTR_CURRENT_CHANNEL, u8CurChannel);
}

/****************************************************************************
 *
 * NAME: vProcessSysEvent
 *
 * DESCRIPTION:
 * Proccess the system's event queues.
 *
 * PARAMETERS:      None
 *
 * RETURNS:
 * void
 *
 ****************************************************************************/
PRIVATE void vProcessSysEvent()
{
    MAC_MlmeDcfmInd_s *psMlmeInd;
    MAC_McpsDcfmInd_s *psMcpsInd;
    AppQApiHwInd_s    *psAHI_Ind;

    do
    {
        psMcpsInd = psAppQApiReadMcpsInd();
        if (psMcpsInd != NULL)
        {
            vProcessIncomingMcps(psMcpsInd);
            vAppQApiReturnMcpsIndBuffer(psMcpsInd);
        }
    }
    while (psMcpsInd != NULL);

    do
    {
        psMlmeInd = psAppQApiReadMlmeInd();
        if (psMlmeInd != NULL)
        {
            vProcessIncomingMlme(psMlmeInd);
            vAppQApiReturnMlmeIndBuffer(psMlmeInd);
        }
    }
    while (psMlmeInd != NULL);

    do
    {
        psAHI_Ind = psAppQApiReadHwInd();
        if (psAHI_Ind != NULL)
        {
            vProcessIncomingHwEvent(psAHI_Ind);
            vAppQApiReturnHwIndBuffer(psAHI_Ind);
        }
    }
    while (psAHI_Ind != NULL);
}

/****************************************************************************
 *
 * NAME: vProcessIncomingMcps
 *
 * DESCRIPTION:
 * Proccess the incoming mcps
 *
 * PARAMETERS:
 *				MAC_McpsDcfmInd_s*
 *
 * RETURNS:
 * void
 *
 ****************************************************************************/
PRIVATE void vProcessIncomingMcps(MAC_McpsDcfmInd_s *psMcpsInd)
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


/****************************************************************************
 *
 * NAME: vHandleMcpsDataInd
 *
 * DESCRIPTION:
 * 	Proccess the incoming data, including TOF protocol msg
 *
 * NOTE:
 *		Use station's RSSI or AVAILABLE event to alarm
 *		Use station's RSSI, AVAILABLE, BUSY, IDLE to sync slot
 *
 * PARAMETERS:
 *				MAC_McpsDcfmInd_s*
 *
 * RETURNS:
 * void
 *
 ****************************************************************************/
PRIVATE void vHandleMcpsDataInd(MAC_McpsDcfmInd_s *psMcpsInd)
{
    MAC_RxFrameData_s *psFrame;
    psFrame = &psMcpsInd->uParam.sIndData.sFrame;
    RfTofWrapper_tu* psAppPkt = (RfTofWrapper_tu*)(psFrame->au8Sdu);
    psAppPkt_Temp = psAppPkt;

    // not for me or len is error
    if((APP_PROTOCOL_TYPE_CARD != psAppPkt->tof_head.protocoltype))
            //|| (psFrame->u8SduLength != sizeof(app_header_t) + psAppPkt->tof_head.len))
        return;

	//PrintfUtil_vPrintf("u8ExciteCnt=%d\r\n",u8ExciteCnt);
	//if(!u8ExciteCnt)
	{
    switch (psAppPkt->tof_head.msgtype)
    {

    case TOF_STATION_ILDE:
    {
        if (eCardTofState == E_TOF_STATE_LISTEN)
        {
            TimerUtil_eStopTimer(TOF_LISTEN_TIMEOUT_EVENT);
            EventUtil_vUnsetEvent(TOF_LISTEN_TIMEOUT_EVENT);
            EventUtil_vSetEvent(TOF_LISTEN_EVENT);
        }
        else if ((eCardTofState == E_TOF_STATE_SYNC) && (psFrame->sSrcAddr.u16PanId == u16StationPanId) && (psFrame->sSrcAddr.uAddr.u16Short == u16StationShortAddr))
        {
            vSyncStation(psAppPkt);
        }

        u8ListenIdleTime = 0;
        break;
    }

    case TOF_STATION_BUSY:
    {
        if ((eCardTofState == E_TOF_STATE_SYNC) && (psFrame->sSrcAddr.u16PanId == u16StationPanId) && (psFrame->sSrcAddr.uAddr.u16Short == u16StationShortAddr))
        {
            vSyncStation(psAppPkt);
        }
        break;
    }

    case TOF_STATION_RSSI:
    {
        if((eCardTofState == E_TOF_STATE_ALARM) &&
                ((CARD_STATUS_HELP & u8CardStatus) ||
                (CARD_STATUS_NOPWD & u8CardStatus) ||
                //(CARD_STATUS_EXCITER & u8CardStatus) ||
                (CARD_STATUS_RETREAT_ACK & u8CardStatus)))
        {
            vSendAlarm(psFrame->sSrcAddr.uAddr.u16Short, psFrame->sSrcAddr.u16PanId);
        }
        //vCheckStationStatus(psAppPkt->rf_tof_station_signal.u8StationStatus);

        break;
    }

    case TOF_STATION_AVAILABLE:
    {
        uint32 u32MyEvent = 0;
        u8ListenIdleTime = 0;

		if((eCardTofState == E_TOF_STATE_ALARM) &&
                ((CARD_STATUS_HELP & u8CardStatus) ||
                (CARD_STATUS_NOPWD & u8CardStatus) ||
                (CARD_STATUS_NOPWD & u8CardStatus))) //||
                //(CARD_STATUS_EXCITER & u8CardStatus)))
		{
            vSendAlarm(psFrame->sSrcAddr.uAddr.u16Short, psFrame->sSrcAddr.u16PanId);
		}

        if(((!(psAppPkt->rf_tof_station_signal.u8LocIdle&HAVE_IDLE_ONES_SLOT))&&(tofPeriodType == CARD_TOF_1S))
                ||((!(psAppPkt->rf_tof_station_signal.u8LocIdle&HAVE_IDLE_FIVES_SLOT))&&(tofPeriodType == CARD_TOF_5S))
          )
        {
            break;
        }

        if((eCardTofState == E_TOF_STATE_REQUEST_STATION) // this show that the station accept another card and send available msg again
                && (psFrame->sSrcAddr.uAddr.u16Short == u16StationShortAddr)
                && (psFrame->sSrcAddr.u16PanId == u16StationPanId))
        {
            TimerUtil_eStopTimer(TOF_LISTEN_EVENT);	// if request timeout, listen again, so need to stop it
            EventUtil_vUnsetEvent(TOF_LISTEN_EVENT);
            u32MyEvent = TOF_REQUEST_EVENT;
        }

        else if ((eCardTofState == E_TOF_STATE_LISTEN) || (eCardTofState == E_TOF_STATE_RSSI))
        {
            // 1. 信号强度 > -65dB 直接加入
            // 2. 当前已经进入 hungry 状态，直接加入
            // 3. 从第二周期开始，当前强度大于上周期最强（如果上周期没收到，就为零），直接加入
            // 4. 从第二周期开始，收到上周期最强信号，直接加入。
            if(psFrame->u8LinkQuality > 3*(95-55)    //LQI=3*(95 + rssi)
                    || LQ_StationSearchStatus == LQ_STATION_SEARCH_STATUS_HUNGRY
                    || (psFrame->u8LinkQuality > LQ_StationPanid.u8LinkQuality && LQ_StationPanid.u8ListenTimeOutCount > 0)
                    ||((psFrame->sSrcAddr.u16PanId == LQ_StationPanid.StationPanid &&
                        psFrame->sSrcAddr.uAddr.u16Short == LQ_StationPanid.StationShortAddr)&& (LQ_StationPanid.u8ListenTimeOutCount > 0)))
            {
                TimerUtil_eStopTimer(TOF_LISTEN_TIMEOUT_EVENT);
                EventUtil_vUnsetEvent(TOF_LISTEN_TIMEOUT_EVENT);
                TimerUtil_eStopTimer(TOF_RSSI_EVENT);
                EventUtil_vUnsetEvent(TOF_RSSI_EVENT);

                u16StationShortAddr = psFrame->sSrcAddr.uAddr.u16Short;
                u16StationPanId = psFrame->sSrcAddr.u16PanId;
                u32MyEvent = TOF_REQUEST_EVENT;

                // 开始入网，清除入网暂存数据
                LQ_StationPanid.StationPanid = 0;
                LQ_StationPanid.StationShortAddr = 0;
                LQ_StationPanid.u8LinkQuality = 0;
                LQ_StationPanid.u8ListenTimeOutCount = 0;
                LQ_StationPanidNext.u8LinkQuality= 0;
                LQ_StationPanidNext.StationPanid = 0;
                LQ_StationPanidNext.StationShortAddr = 0;
                LQ_StationSearchStatus = LQ_STATION_SEARCH_STATUS_NORMAL;
            }
            // 挑选最强基站
            else if(LQ_StationPanidNext.u8LinkQuality < psFrame->u8LinkQuality)
            {
                LQ_StationPanidNext.StationPanid = psFrame->sSrcAddr.u16PanId;
                LQ_StationPanidNext.StationShortAddr = psFrame->sSrcAddr.uAddr.u16Short;
                LQ_StationPanidNext.u8LinkQuality = psFrame->u8LinkQuality;
            }
        }

        else if ((eCardTofState == E_TOF_STATE_SYNC) && (psFrame->sSrcAddr.u16PanId == u16StationPanId) && (psFrame->sSrcAddr.uAddr.u16Short == u16StationShortAddr))
        {
            vSyncStation(psAppPkt_Temp);
        }

        if(u32MyEvent > 0)
        {
            uint16 u16Tmp = SysUtil_u16GenRndNum() % ((uint16)(psAppPkt->rf_tof_station_signal.u8AvailableMs));
            if(u16Tmp == 0)
                EventUtil_vSetEvent(u32MyEvent);
            else
                TimerUtil_eSetTimer(u32MyEvent, u16Tmp);
        }

        //vCheckStationStatus(psAppPkt->rf_tof_station_signal.u8StationStatus);

        break;
    }

    case TOF_STATION_FINISH:
    {
        DBG(
            PrintfUtil_vPrintf("station_fin: %d,%d\n", eCardTofState,u16CardSlot);
        )
        if((eCardTofState == E_TOF_STATE_WAIT_STATION_TOF) || (eCardTofState == E_TOF_STATE_ALARM))
        {
            if(u8ReportDistance || u8LocCnt == 255)
            {
                vSetCardChannel(u8RssiChannel);
                u8LocCnt++;
                vCardCast(CARD_LOC_INFO,0x0000,0xFFFF,0,psAppPkt->rf_tof_station_finish.u16Dist2Station);

                u8ReportDistance = FALSE;
            }
            TimerUtil_eStopTimer(TOF_SYNC_EVENT);
            EventUtil_vUnsetEvent(TOF_SYNC_EVENT);

            // for wakeup from help DIO
            TimerUtil_eStopTimer(TOF_LISTEN_EVENT);
            EventUtil_vUnsetEvent(TOF_LISTEN_EVENT);

            eCardTofState = E_TOF_STATE_STATION_FINISH;

            u8StationRunMs 	= psAppPkt->rf_tof_station_finish.u8RunMs;
            u8NeedLocator	= psAppPkt->rf_tof_station_finish.u8LocN;
            u8LocToCardIndex  = psAppPkt->rf_tof_station_finish.u8Reserved;
            bTofConnected 	= TRUE;
            u32StationRecTick = u32AHI_TickTimerRead();

            // if card receive station_finish, need to report alarm status,
            // else, card must send out rssi msg, which include alarm status already
            if((u8CardStatus & CARD_STATUS_HELP) ||
               ((u8CardStatus & CARD_STATUS_RETREAT) &&(u8CardStatus & CARD_STATUS_RETREAT_ACK)
                &&(bRetreatAck == TRUE) && (u8RetreatAckCnt < TOF_CARD_RETREAT_ACK_TIMES)))
            {
                EventUtil_vSetEvent(TOF_ALARM_EVENT);
                TimerUtil_eSetTimer(TOF_STATION_DONE_EVENT, TOF_ALARM_TIMEOUT+10);
            }

            else
            {
                //u8CardStatus &= (~CARD_STATUS_RETREAT_ACK);
                EventUtil_vSetEvent(TOF_STATION_DONE_EVENT);
            }

            //vCardCast(CARD_LOC_INFO,0x0000,0xFFFF,0,psAppPkt->rf_tof_station_finish.u16Dist2Station);

            //vCheckStationStatus(psAppPkt->rf_tof_station_finish.u8StationStatus);
        }

        break;
    }

    case TOF_STATION_ACCEPT:	// The station accept some card's join request
    {
        if(eCardTofState == E_TOF_STATE_REQUEST_STATION)
        {
            uint8 u8AcceptNum = psAppPkt->rf_tof_station_accept.u8AcceptNum;
            uint8 u8Tmp;
            for(u8Tmp = 0; u8Tmp < u8AcceptNum; u8Tmp++)
            {
                if(psAppPkt->rf_tof_station_accept.tsAccptData[u8Tmp].u16ShortAddr == u16CardShortAddr) // if the accept list include myself
                {
                    TimerUtil_eStopTimer(TOF_LISTEN_EVENT);	// if request timeout, listen again
                    EventUtil_vUnsetEvent(TOF_LISTEN_EVENT);

                    eCardTofState 	= E_TOF_STATE_STATION_ACCEPT;
                    bTofConnected 	= TRUE;
                    u8LostTimes		= 0;
                    u16LocatorMs 	= 0;
                    u8NeedLocator	= 0;
                    u16SleepDelta 	= TOF_CARD_SYNC_DELTA_MAX;

                    u8StationChannel = psAppPkt->rf_tof_station_accept.u8StationChannel;
                    u8LocatorChannel = psAppPkt->rf_tof_station_accept.u8LocatorCardChannel;
                    u16CardSlot = psAppPkt->rf_tof_station_accept.tsAccptData[u8Tmp].u16SlotIndex;

                    u16StationMs = ((u16CardSlot + TOF_SLOT_NUM - psAppPkt->rf_tof_station_accept.u16CurSlot)*TOF_SLOT_MS - (uint16)(psAppPkt->rf_tof_station_accept.u8RunMs)) % tofStationPeriodMs;
                    DBG(PrintfUtil_vPrintf("station%d\n",u16StationPanId);)

                    EventUtil_vSetEvent(TOF_SLEEP_EVENT);

                    DBG(
                        PrintfUtil_vPrintf("accept,%d- slot: %d, run: %d, sleep: %d\n", sizeof(psAppPkt->rf_tof_station_accept.tsAccptData[u8Tmp].u16SlotIndex),u16CardSlot, psAppPkt->rf_tof_station_accept.u8RunMs, u16StationMs);
                    )
                    break;
                }
            }
        }

        break;
    }

    ///////////////////////////////////////////////////////////
    // locator msg
    case TOF_LOCATOR_FINISH:	// The locator finish TOF
    {
        if(eCardTofState == E_TOF_STATE_WAIT_LOCATOR)
        {
            DBG(PrintfUtil_vPrintf("locator finish:%d,%d\n",psFrame->sSrcAddr.uAddr.u16Short,psAppPkt->tof_head.len);)
            eCardTofState = E_TOF_STATE_LOCATOR_FINISH;
            TimerUtil_eStopTimer(TOF_LOCATOR_DONE_EVENT);
            EventUtil_vSetEvent(TOF_LOCATOR_DONE_EVENT);
        }
        break;
    }

    /*case CARD_BASE_INFO:
    {
        if(psAppPkt->tof_head.len && (u32state == WRITE_INFO))
		{
            //app_eDev_Data_t *p_data = (app_eDev_Data_t *)(psAppPkt->tof_head +1);
            uint8 ret;
            DBG(PrintfUtil_vPrintf("recv info\n");)
            ret = vParse_card_info((unsigned char*)(psFrame->au8Sdu),psAppPkt->tof_head.len+4);
            TimerUtil_eStopTimer(TOF_EXCITE_TIMEOUT_EVENT);
            EventUtil_vUnsetEvent(TOF_EXCITE_TIMEOUT_EVENT);
            if(ret)
            {
                vSetCardChannel(20);
                vCardCast(HAND_STATION_WRITE_ACK,0x0000,0xFFFF,0,u16SeqNum);
		TimerUtil_eSetTimer(TOF_CHECK_HELP_EVENT,5);
		u8AckCnt = 0;

            }
            u8ExciteCnt = 0;
            //u8ExciteTure = FALSE;

            u8ExciteTure = FALSE;
            //u8WriteData = FALSE;
            u8SendCnt = 0;
            //checkin_ack = FALSE;
            card_info_cfg.u8Frame_cur = 0;
            vAHI_WatchdogStart(12);
            TimerUtil_eSetTimer(TOF_LISTEN_EVENT,20);
            TimerUtil_eSetCircleTimer(TOF_WATCHDOG_EVENT, 1000);    // feed watch dog every 1s
            vAHI_DioWakeEnable(TOF_EXCITER_CLK_PIN,0);
		}
        break;
    }*/

    case CHANGE_PERIOD:
    {
        DBG(PrintfUtil_vPrintf("per1\n");)
        uint8 temp = u8CardPeriod;
        u8CardPeriod = psAppPkt->rf_tof_locator_card_info.u8CardNum;
        if(u32state == WRITE_INFO)
        {
            //DBG(PrintfUtil_vPrintf("per1:%d\n",u8CardPeriod);)

            if((u8CardPeriod>3) && (u8CardPeriod<253))
            {
                u8MaxTofCnt = 4;
            }
            else
            {
                u8MaxTofCnt = 1;
            }

            if(u8CardPeriod == 253)    //1分钟
            {
                u16MaxWarmStartCnt = 4;
            //u16SleepCnt = 65000;
            }
            else if(u8CardPeriod == 254)   //5分钟
            {
                u16MaxWarmStartCnt = 20;
            }
            else if(u8CardPeriod == 168)   //仓库模式
                u16MaxWarmStartCnt = u8CardPeriod*3;
            else
                u16MaxWarmStartCnt = u8CardPeriod*60*4;


            vSetCardChannel(u8RssiChannel);
            
            vCardCast(TOF_CARD_ALARM,0x0000,0xFFFF,0,u16SeqNum);
            TimerUtil_vDelay(10, E_TIMER_UNIT_MILLISECOND);
            if(temp != psAppPkt->rf_tof_locator_card_info.u8CardNum)
            {
                u8CardPeriod = psAppPkt->rf_tof_locator_card_info.u8CardNum;
                bAHI_FlashInit(E_FL_CHIP_INTERNAL, NULL);

                if(!bAHI_FlashEraseSector(5))
                {
                    PrintfUtil_vPrintf("Flash Erase Fail\n");
                    return;
                }

                if(bAHI_FullFlashProgram(PERIOD_STORE_ADDR, sizeof(u8CardPeriod),(uint8*)&u8CardPeriod))
                {
                    PrintfUtil_vPrintf("saving period!\n");
                }
            }

            u8ExciteTure = FALSE;
            //u8WriteData = FALSE;
            //checkin_ack = FALSE;
            card_info_cfg.u8Frame_cur = 0;
            vAHI_WatchdogStart(12);
            TimerUtil_eSetTimer(TOF_SLEEP_EVENT,5);
            TimerUtil_eSetCircleTimer(TOF_WATCHDOG_EVENT, 1000);    // feed watch dog every 1s
            //vAHI_DioWakeEnable(TOF_EXCITER_CLK_PIN,0);

        }
        break;
    }

    case HAND_STATION_RSSI_CHANNEL:
    {
        if(u32state == WRITE_INFO && psAppPkt->rf_card_type.tof_head.len == 1)
        {
            PrintfUtil_vPrintf("RSSIC :%d,%d\n",psFrame->sSrcAddr.u16PanId,psFrame->sSrcAddr.uAddr.u16Short);
            vSetCardChannel(u8RssiChannel);
            vCardCast(HAND_STATION_RSSI_CHANNEL_ACK,psFrame->sSrcAddr.uAddr.u16Short,psFrame->sSrcAddr.u16PanId,0,0);
            TimerUtil_vDelay(10, E_TIMER_UNIT_MILLISECOND);
            if(psAppPkt->rf_card_type.cardtype != cfg_option.u8RssiCh)
            {
                cfg_option.u8RssiCh = psAppPkt->rf_card_type.cardtype;
                cfg_option.u8Status = u8CardStatus;
                bAHI_FlashInit(E_FL_CHIP_INTERNAL, NULL);

                if(!bAHI_FlashEraseSector(7))
                {
                    PrintfUtil_vPrintf("Flash Erase Fail\n");
                    return;
                }

                if(bAHI_FullFlashProgram(PARAM_STORE_ADDR, sizeof(CFG_OPTION_T),(uint8*)&cfg_option))
                {
                    PrintfUtil_vPrintf("finish saving  !\n");

                    u16CardShortAddr = cfg_option.u16ShortAddr;
                    tofPeriodType = cfg_option.u8CardType;
                    u8BroadcastChannel = cfg_option.u8Channel;
                    u8RssiChannel = cfg_option.u8RssiCh;
                }
                vAHI_SwReset();

            }
        }
        break;
    }

    case HANDLE_STATION_ACK:
    {
        //if((u8LastSeqnum != ) || ())
        card_info_cfg.u8Frame_cur++;
        u8FrameCnt++;
        u8SendCnt = 0;

        u8ReportIdx = psAppPkt->rf_tof_locator_card_info.u8CardNum;

        //PrintfUtil_vPrintf("IDX:%d\n",u8ReportIdx);

        if(u8ReportIdx == 0)
        {
            TimerUtil_eStopTimer(TOF_EXCITE_EVENT);
            EventUtil_vUnsetEvent(TOF_EXCITE_EVENT);
            u8ExciteTure = FALSE;
            //u8WriteData = FALSE;
            u8SendCnt = 0;
            //checkin_ack = FALSE;
            card_info_cfg.u8Frame_cur = 0;
            u8FrameCnt = 0;
            vAHI_WatchdogStart(12);
            EventUtil_vSetEvent(TOF_LISTEN_EVENT);
            TimerUtil_eSetCircleTimer(TOF_WATCHDOG_EVENT, 1000);    // feed watch dog every 1s
            //vAHI_DioWakeEnable(TOF_EXCITER_CLK_PIN,0);
        }
        else if(card_info_cfg.u8Frame_cur == 1)
        {
            u8MaxCnt = 5;
            if(card_info_cfg.u8Frame_sum > 5)
            {
                if(card_info_cfg.u8Frame_sum > u8ReportIdx*5)
                    card_info_cfg.u8Frame_cur = card_info_cfg.u8Frame_sum - u8ReportIdx*5 +1;
                else
                {
                    card_info_cfg.u8Frame_cur = 1;
                    if(card_info_cfg.u8Frame_sum - ((u8ReportIdx-1)*5)> 0)
                        u8MaxCnt = card_info_cfg.u8Frame_sum - ((u8ReportIdx-1)*5);
                }
            }
            else
            {
                card_info_cfg.u8Frame_cur = 1;
            }

            //card_info_cfg.u8Frame_cur = (u8ReportIdx-1)*5 + 1;
        }
        PrintfUtil_vPrintf("IDX:%d,%d,%d\n",u8ReportIdx,card_info_cfg.u8Frame_cur,u8MaxCnt);
        //u8LastSeqnum =
        break;
    }

    case TOF_STATION_CHECKIN_ACK:	// station received card's checkin, that is, checkin successed.
    {
        DBG(PrintfUtil_vPrintf("checkin success\n");)
        TimerUtil_eStopTimer(TOF_CHECKIN_EVENT);
        EventUtil_vUnsetEvent(TOF_CHECKIN_EVENT);
        break;
    }

    case DEVICE_CARD_PERIOD_CHANNEL:
    {
        PrintfUtil_vPrintf("Setting period:%d\n",psAppPkt->tof_head.len);
        TimerUtil_eStopTimer(TOF_CHECKIN_EVENT);
        EventUtil_vUnsetEvent(TOF_CHECKIN_EVENT);
        vCardCast(DEVICE_CARD_PERIOD_CHANNEL_ACK,0x0000,0xFFFF,0,u16SeqNum);
        if(psAppPkt->tof_head.len == 4)
        {
            PrintfUtil_vPrintf("AAAAA\n");
            if(psAppPkt->rf_card_info2.info_data.frameSeq != cfg_option.u8RssiCh)
            {
                cfg_option.u8RssiCh = psAppPkt->rf_card_info2.info_data.frameSeq;
                cfg_option.u8Status = u8CardStatus;
                bAHI_FlashInit(E_FL_CHIP_INTERNAL, NULL);
                PrintfUtil_vPrintf("BBBB:%d\n",cfg_option.u8RssiCh);
                if(!bAHI_FlashEraseSector(7))
                {
                    PrintfUtil_vPrintf("Flash Erase Fail\n");
                    return;
                }

                if(bAHI_FullFlashProgram(PARAM_STORE_ADDR, sizeof(CFG_OPTION_T),(uint8*)&cfg_option))
                {
                    PrintfUtil_vPrintf("finish saving  !\n");

                    u16CardShortAddr = cfg_option.u16ShortAddr;
                    tofPeriodType = cfg_option.u8CardType;
                    u8BroadcastChannel = cfg_option.u8Channel;
                    u8RssiChannel = cfg_option.u8RssiCh;
                }
            }

            if(u8CardPeriod != psAppPkt->rf_card_info2.info_data.len)
            {
                PrintfUtil_vPrintf("CCCC\n");
                u8CardPeriod = psAppPkt->rf_card_info2.info_data.len;

                if((u8CardPeriod>3) && (u8CardPeriod<253))
                {
                    u8MaxTofCnt = 4;
                }
                else
                {
                    u8MaxTofCnt = 1;
                }

                if(u8CardPeriod == 253)    //1分钟
                {
                    u16MaxWarmStartCnt = 4;
                //u16SleepCnt = 65000;
                }
                else if(u8CardPeriod == 254)   //5分钟
                {
                    u16MaxWarmStartCnt = 20;
                }
                else if(u8CardPeriod == 168)   //仓库模式
                    u16MaxWarmStartCnt = u8CardPeriod*3;
                else
                    u16MaxWarmStartCnt = u8CardPeriod*60*4;
                bAHI_FlashInit(E_FL_CHIP_INTERNAL, NULL);

                if(!bAHI_FlashEraseSector(5))
                {
                    PrintfUtil_vPrintf("Flash Erase Fail\n");
                    return;
                }

                if(bAHI_FullFlashProgram(PERIOD_STORE_ADDR, sizeof(u8CardPeriod),(uint8*)&u8CardPeriod))
                {
                    PrintfUtil_vPrintf("saving period!\n");
                }
            }
        }
        else if(psAppPkt->tof_head.len == 2)
        {
            PrintfUtil_vPrintf("DDDDD :%d\n",psAppPkt->rf_card_info2.info_data.frameSum);
            if(psAppPkt->rf_card_info2.info_data.frameSum == 0)  //修改rssi频道
            {
                PrintfUtil_vPrintf("EEEEE:%d\n",psAppPkt->rf_card_info2.info_data.frameSeq);
                if(psAppPkt->rf_card_info2.info_data.frameSeq != cfg_option.u8RssiCh)
                {
                    cfg_option.u8RssiCh = psAppPkt->rf_card_info2.info_data.frameSeq;
                    cfg_option.u8Status = u8CardStatus;
                    bAHI_FlashInit(E_FL_CHIP_INTERNAL, NULL);

                    if(!bAHI_FlashEraseSector(7))
                    {
                        PrintfUtil_vPrintf("Flash Erase Fail\n");
                        return;
                    }

                    if(bAHI_FullFlashProgram(PARAM_STORE_ADDR, sizeof(CFG_OPTION_T),(uint8*)&cfg_option))
                    {
                        PrintfUtil_vPrintf("finish saving  !\n");

                        u16CardShortAddr = cfg_option.u16ShortAddr;
                        tofPeriodType = cfg_option.u8CardType;
                        u8BroadcastChannel = cfg_option.u8Channel;
                        u8RssiChannel = cfg_option.u8RssiCh;
                    }
                }
            }
            else if(psAppPkt->rf_card_info2.info_data.frameSum == 1)   //修改周期
            {
                PrintfUtil_vPrintf("FFFFFF:%d\n",psAppPkt->rf_card_info2.info_data.frameSeq);
                if(u8CardPeriod != psAppPkt->rf_card_info2.info_data.frameSeq)
                {
                    u8CardPeriod = psAppPkt->rf_card_info2.info_data.frameSeq;

                    if((u8CardPeriod>3) && (u8CardPeriod<253))
                    {
                        u8MaxTofCnt = 4;
                    }
                    else
                    {
                        u8MaxTofCnt = 1;
                    }

                    if(u8CardPeriod == 253)    //1分钟
                    {
                        u16MaxWarmStartCnt = 4;
                    //u16SleepCnt = 65000;
                    }
                    else if(u8CardPeriod == 254)   //5分钟
                    {
                        u16MaxWarmStartCnt = 20;
                    }
                    else if(u8CardPeriod == 168)   //仓库模式
                        u16MaxWarmStartCnt = u8CardPeriod*3;
                    else
                        u16MaxWarmStartCnt = u8CardPeriod*60*4;
                    bAHI_FlashInit(E_FL_CHIP_INTERNAL, NULL);

                    if(!bAHI_FlashEraseSector(5))
                    {
                        PrintfUtil_vPrintf("Flash Erase Fail\n");
                        return;
                    }

                    if(bAHI_FullFlashProgram(PERIOD_STORE_ADDR, sizeof(u8CardPeriod),(uint8*)&u8CardPeriod))
                    {
                        PrintfUtil_vPrintf("saving period!\n");
                    }
                }
            }
        }


        vAHI_SwReset();

        
        break;
    }

    case DEVICE_CARD_ERPID:
    {
        if(psAppPkt->tof_head.len > 0 && psAppPkt->tof_head.len < 20)
        {
            vCardCast(DEVICE_CARD_PERIOD_CHANNEL_ACK,0x0000,0xFFFF,0,u16SeqNum);
            rt_erpid_info.u8ErpIdLen = psAppPkt->tof_head.len;
            memcpy(rt_erpid_info.u8ErpBuf,psAppPkt->rf_tof_erp_id_data.u8Erpid,rt_erpid_info.u8ErpIdLen);
            bAHI_FlashEraseSector(6);
            bAHI_FullFlashProgram(INFO_STORE_ADDR,rt_erpid_info.u8ErpIdLen+1,&rt_erpid_info);
        }
        break;
    }
    default:
        break;
    }
	}
    #if 0
	else
	{
	switch (psAppPkt->tof_head.msgtype)
	{
	case 0xE0://write
    {
        u16HandStationShortAddr = psFrame->sSrcAddr.uAddr.u16Short;
        u16HandStationPanId = psFrame->sSrcAddr.u16PanId;

    	PrintfUtil_vPrintf("write\r\n");
		PrintfUtil_vPrintf("psAppPkt->tof_head.len=%d\r\n",(uint16)psAppPkt->tof_head.len);
    	if(psAppPkt->tof_head.len)
		{
    		++ldata_state.u8writetotalcnt;
    		dump_location_data( (unsigned char*)(&(psAppPkt->tof_head)),psAppPkt->tof_head.len+4);
		}
		receiveAck(0,0xe0);
		u8ExciteTure = TRUE;
		break;
    }
    case 0xE1:
    {
		#if 0
        u16HandStationShortAddr = psFrame->sSrcAddr.uAddr.u16Short;
        u16HandStationPanId = psFrame->sSrcAddr.u16PanId;
    	PrintfUtil_vPrintf("read\r\n");
		int read_length = 1;
		while(read_length != 0)
		{
			read_length = load_location_data(flash_read_buffer, 128);
			PrintfUtil_vPrintf("read_length=%d\r\n",read_length);
			while(bMsgSending);
			vDeviceCardCast(u16HandStationShortAddr,u16HandStationPanId,flash_read_buffer,read_length);
		}
		#endif
		break;
    }
	case 0xE2://write open
    {

        u16HandStationShortAddr = psFrame->sSrcAddr.uAddr.u16Short;
        u16HandStationPanId = psFrame->sSrcAddr.u16PanId;
		receiveAck(0,0xe2);
		TimerUtil_eStopTimer(TOF_EXCITE_EVENT);
		EventUtil_vUnsetEvent(TOF_EXCITE_EVENT);
		TimerUtil_eSetTimer(TOF_EXCITE_EVENT,300000);
    	PrintfUtil_vPrintf("write open\r\n");
		u8WriteData = TRUE;
		vAHI_DioWakeEnable(0,TOF_EXCITER_CLK_PIN);
		break;
    }
	case 0xE3://write close
    {
        u16HandStationShortAddr = psFrame->sSrcAddr.uAddr.u16Short;
        u16HandStationPanId = psFrame->sSrcAddr.u16PanId;
    	PrintfUtil_vPrintf("write close\r\n");
		PrintfUtil_vPrintf("read\r\n");
		receiveAck(0,0xe3);
    	EventUtil_vSetEvent(TOF_READCARD_EVENT);
		u8ExciteTure = FALSE;
		u8WriteData = FALSE;
		vAHI_DioWakeEnable(TOF_EXCITER_CLK_PIN,0);
		break;
    }
	case 0xE4://重传机制
    {
        u16HandStationShortAddr = psFrame->sSrcAddr.uAddr.u16Short;
        u16HandStationPanId = psFrame->sSrcAddr.u16PanId;
		receivedAck = TRUE;
    	PrintfUtil_vPrintf("receiveAck\r\n");
    	EventUtil_vSetEvent(TOF_READCARD_EVENT);
		break;
    }
    case TOF_STATION_CHECKIN_ACK:	// station received card's checkin, that is, checkin successed.
    {
        DBG(
            PrintfUtil_vPrintf("checkin success\n");
        )
        checkin_ack = TRUE;
        EventUtil_vSetEvent(TOF_BUZZER_EVENT);	// when checkin successed, buzzer one time
        break;
    }
    default:
        break;
	}
	}
    #endif
}

/****************************************************************************
 *
 * NAME: vProcessAppEvent
 *
 * DESCRIPTION:
 * 	Proccess the application event queues
 *
 * PARAMETERS:      None
 *
 * RETURNS:
 * void
 *
 ****************************************************************************/
PRIVATE void vProcessAppEvent()
{
    uint32 u32EventID = EventUtil_u32ReadEvents();
    switch (u32EventID)
    {
    case TOF_OAD_EVENT:
    {
        vOADInvalidateSWImage();
        vAHI_SwReset();
        EventUtil_vUnsetEvent(u32EventID);
        break;
    }

    case TOF_CHECK_HELP_EVENT:
    {
        vCardCast(HAND_STATION_WRITE_ACK,0x0000,0xFFFF,0,u16SeqNum);
	if(u8AckCnt == 0)
	{
		TimerUtil_eSetTimer(TOF_CHECK_HELP_EVENT,5);
		u8AckCnt ++;
	}

        EventUtil_vUnsetEvent(u32EventID);
        break;
    }

    case TOF_RETREAT_EVENT:
    {
        vSetLed(JN_RED, TRUE, TOF_LED_RED_MS);
        vBuzzer(TOF_BUZZER_TIMEOUT);
        if(bMsgSending)
        {
            if(bProtectCnt++ > 4)
            {
                vAHI_SwReset();
            }
        }
        else
        {
            bProtectCnt = 0;
        }

        if(bRetreatAck == FALSE)
        {
            TimerUtil_eSetTimer(TOF_RETREAT_EVENT, TOF_RETREAT_PERIOD_MS);
        }
        EventUtil_vUnsetEvent(u32EventID);
        break;
    }

    case TOF_BUZZER_EVENT:
    {
        uint16 u16Tmp = SysUtil_u16GenRndNum() % 10;
	TimerUtil_eSetTimer(TOF_EXCITE_EVENT, u16Tmp);
        EventUtil_vUnsetEvent(u32EventID);
        break;
    }

    case TOF_STOP_BUZZER_EVENT:
    {
        vStopBuzzer();
        EventUtil_vUnsetEvent(u32EventID);
        break;
    }
    case TOF_LED_RED_OFF_EVENT:
    {
        vSetLed(JN_RED, FALSE, 0);
        EventUtil_vUnsetEvent(u32EventID);
        break;
    }

    case TOF_LED_GREEN_OFF_EVENT:
    {
        vSetLed(JN_GREEN, FALSE, 0);
        EventUtil_vUnsetEvent(u32EventID);
        break;
    }

    case TOF_CHECKIN_EVENT:		// when checkin successed, it will unset this event
    {
        DBG(PrintfUtil_vPrintf("checkin event:%d\n",u8RssiChannel);)            
        vSetCardChannel(u8RssiChannel);
        vCardCast(TOF_CARD_CHECKIN, 0x0000, 0xFFFF, 0, u16SeqNum);

        if(++u8CheckinCnt < TOF_CARD_CHECKIN_TIMES)
            TimerUtil_eSetTimer(TOF_CHECKIN_EVENT, 30);	// checkin every 20 ms

        EventUtil_vUnsetEvent(u32EventID);
        break;
    }

    case TOF_EXCITE_TIMEOUT_EVENT:
    {
        u8ExciteTure = FALSE;
        //u8WriteData = FALSE;
        //checkin_ack = FALSE;
        vAHI_WatchdogStart(12);
        EventUtil_vSetEvent(TOF_LISTEN_EVENT);
        TimerUtil_eSetCircleTimer(TOF_WATCHDOG_EVENT, 1000);    // feed watch dog every 1s
        //vAHI_DioWakeEnable(TOF_EXCITER_CLK_PIN,0);
        EventUtil_vUnsetEvent(u32EventID);
        break;
    }

    case TOF_HELP_EVENT:
    {
        DBG(
            PrintfUtil_vPrintf("help: %d, eCardTofState: %d\n", u8HelpCnt, eCardTofState);
        )
        if(u8HelpCnt++ < TOF_CARD_HELP_TIMES)
        {
            TimerUtil_eSetTimer(TOF_HELP_EVENT, TOF_CARD_HELP_PERIOD_MS);

            // if it's doing TOF, ignore the alarm this time
            if(eCardTofState != E_TOF_STATE_WAIT_STATION_TOF)
                EventUtil_vSetEvent(TOF_ALARM_EVENT);

            // if help successed flash green led, else flash red led and send alarm to air
            if((bHelped == FALSE) || (u8HelpCnt+10 <= TOF_CARD_HELP_TIMES))
            {
                vBuzzer(TOF_BUZZER_TIMEOUT);
                vSetLed(JN_RED, TRUE, TOF_LED_RED_MS);
            }
            else
                vSetLed(JN_GREEN, TRUE, TOF_LED_GREEN_MS);
        }
        else  //if sended help for 20 times, reset the flag for the next send
        {
            bHelped = FALSE;
        }

        EventUtil_vUnsetEvent(u32EventID);
        break;
    }

    case TOF_ALARM_EVENT:
    {
        eCardTofState = E_TOF_STATE_ALARM;
        vSetCardChannel(u8RssiChannel);

        // use 2 listen timeout, to wait station's rssi / available cmd to send alarm msg, and station will send app_ack when receive this msg
        TimerUtil_eSetTimer(TOF_ALARM_TIMEOUT_EVENT, TOF_ALARM_TIMEOUT);
        EventUtil_vUnsetEvent(u32EventID);
        break;
    }

    case TOF_ALARM_TIMEOUT_EVENT:
    {
        vSetCardChannel(u8BroadcastChannel);

        // card can't find rssi / available msg, maybe there's not non_rssi_station,
        // need to broadcast to all station, the joined station will send app_ack when receive this msg
        //vCardCast(TOF_CARD_ALARM, 0x0000, 0xFFFF, 0, u16SeqNum);
        EventUtil_vUnsetEvent(u32EventID);
        break;
    }

    case TOF_STATION_EVENT:
    {
        eCardTofState = E_TOF_STATE_WAIT_STATION_TOF;
        if(eCardRssiState == TRUE)
            eCardRssiState = FALSE;

        vSetCardChannel(u8StationChannel);
        DBG(
            PrintfUtil_vPrintf("tof processing tick: %d\n", u32AHI_TickTimerRead());
        )
//vCardCast(0xAA, 0xF000, u16SeqNum, 0, 0); // for test: record the wakeup time

        TimerUtil_eSetTimer(TOF_SYNC_EVENT, TOF_PENDING_TIMEOUT);	// if wait station to do tof timeout, need sync to the station
        EventUtil_vUnsetEvent(u32EventID);
        break;
    }

    case TOF_SYNC_EVENT:
    {
        DBG(
            PrintfUtil_vPrintf("sync event\n");
        )
        EventUtil_vSetEvent(TOF_LISTEN_EVENT);
        EventUtil_vUnsetEvent(u32EventID);
        break;
/*
        eCardTofState = E_TOF_STATE_SYNC;
        u8LostTimes++;
        if(u8LostTimes >= TOF_STATION_CARD_MAX_FAILED)
        {
            if((TRUE == bHelpSync) && (u8HelpCnt == 0))
             EventUtil_vSetEvent(TOF_HELP_EVENT);
            else
                EventUtil_vSetEvent(TOF_LISTEN_EVENT);
        }
        else
        {
            vSetCardChannel(u8BroadcastChannel);

            if((TRUE == bHelpSync) && (u8HelpCnt == 0))
             TimerUtil_eSetTimer(TOF_HELP_EVENT, TOF_SYNC_TIMEOUT);
            else
                TimerUtil_eSetTimer(TOF_LISTEN_EVENT, TOF_SYNC_TIMEOUT);
        }

        EventUtil_vUnsetEvent(u32EventID);
        break;*/
    }

    case TOF_LISTEN_EVENT:
    {
        eCardTofState = E_TOF_STATE_LISTEN;
        vSetCardChannel(u8BroadcastChannel);
        DBG(
            PrintfUtil_vPrintf("LISTEN tick: %d\n", u32AHI_TickTimerRead());
        );
        //reinit some adjust time
        u16WakeDelta = TOF_CARD_SYNC_DELTA_MAX;
        u16SleepDelta = TOF_CARD_SYNC_DELTA_MAX;

        LQ_StationPanid.u8ListenCount++;
        TimerUtil_eSetTimer(TOF_LISTEN_TIMEOUT_EVENT, TOF_LISTEN_TIMEOUT);
        EventUtil_vUnsetEvent(u32EventID);
        break;
    }

    case TOF_LISTEN_TIMEOUT_EVENT:
    {
        LQ_StationPanid.StationPanid = LQ_StationPanidNext.StationPanid;
        LQ_StationPanid.StationShortAddr = LQ_StationPanidNext.StationShortAddr;
        LQ_StationPanid.u8LinkQuality = LQ_StationPanidNext.u8LinkQuality;

        LQ_StationPanidNext.u8LinkQuality = 0;
        LQ_StationPanidNext.StationPanid = 0;
        LQ_StationPanidNext.StationShortAddr = 0;

        if(((0 == LQ_StationPanid.StationPanid) && (LQ_StationSearchStatus == LQ_STATION_SEARCH_STATUS_HUNGRY))||
                (++LQ_StationPanid.u8ListenTimeOutCount > 1) || (u8ListenIdleTime >= 2))
        {
            DBG(
                PrintfUtil_vPrintf("listen timeout:%d\n",LQ_StationPanid.u8ListenTimeOutCount);)
            eCardTofState = E_TOF_STATE_IDLE;
            if(bRSSIed)
                EventUtil_vSetEvent(TOF_STATION_DONE_EVENT);
            else
            {
				vSendRssi();
                TimerUtil_eSetTimer(TOF_STATION_DONE_EVENT, TOF_RSSI_TIMEOUT);
            }
        }
        else
        {
            EventUtil_vSetEvent(TOF_LISTEN_EVENT);
        }

        // 如果上周期没收到任何信号或已经连续2个周期没入网(不包括收到busy,idle重新listen的情况)
        // 本周期转入 hungry 状态，收到任意一个基站信号即立即入网
        if(LQ_StationPanid.StationPanid == 0 || LQ_StationPanid.u8ListenTimeOutCount == 1)
        {
            LQ_StationSearchStatus = LQ_STATION_SEARCH_STATUS_HUNGRY;
        }

        EventUtil_vUnsetEvent(u32EventID);
        break;
    }

    case TOF_RSSI_EVENT:
    {
        DBG(
            PrintfUtil_vPrintf("rssi\n");
        )
        eCardTofState = E_TOF_STATE_RSSI;
        vSendRssi();
        EventUtil_vUnsetEvent(u32EventID);
        break;
    }

    case TOF_REQUEST_EVENT:
    {
        eCardTofState = E_TOF_STATE_REQUEST_STATION;
        vSetCardChannel(u8BroadcastChannel);
        DBG(
            PrintfUtil_vPrintf("tof request\n");
        )
        vCardCast(TOF_CARD_REQUEST, u16StationShortAddr, u16StationPanId, 0, u16SeqNum);

        if(LQ_StationPanid.u8ListenCount < 10)
            TimerUtil_eSetTimer(TOF_LISTEN_EVENT, TOF_REQUEST_TIMEOUT);
        else
        {
            eCardTofState = E_TOF_STATE_IDLE;
            LQ_StationPanid.u8ListenCount = 0;
            if(bRSSIed)
                EventUtil_vSetEvent(TOF_STATION_DONE_EVENT);
            else
            {
                vSendRssi();
                TimerUtil_eSetTimer(TOF_STATION_DONE_EVENT, TOF_RSSI_TIMEOUT);
            }
        }

        EventUtil_vUnsetEvent(u32EventID);
        break;
    }

    case TOF_LOCATOR_EVENT:
    {
    	u32LocatorRecTick = u32AHI_TickTimerRead();
    	eCardTofState = E_TOF_STATE_WAIT_LOCATOR;
        vSetCardChannel(u8LocatorChannel);
        DBG(
            PrintfUtil_vPrintf("wait locator\n");
        )
//vCardCast(0xAA, 0xF000, u16SeqNum, 0, 0); // for test: record the wakeup time

        TimerUtil_eSetTimer(TOF_LOCATOR_DONE_EVENT, u32WaitLocatorTimeout);
        EventUtil_vUnsetEvent(u32EventID);
        break;
    }

    case TOF_STATION_DONE_EVENT:
    {
        eCardTofState = E_TOF_STATE_STATION_FINISH;

        if (FALSE == bTofConnected) // tof failed
        {
            u8NeedLocator = 0;
            u16StationMs = tofStationPeriodMs - (uint16)((u32AHI_TickTimerRead()/16000)%tofStationPeriodMs) + u16WakeDelta;

            if (u8LostTimes < TOF_STATION_CARD_MAX_FAILED)
                u8LostTimes++;
        }
        else
        {
            u8LostTimes = 0;
            u16WakeDelta = (uint16)(u32StationRecTick/16000) - (uint16)u8StationRunMs;

            // u16WakeDelta should smaller than TOF_CARD_SYNC_DELTA_MAX in normal wakeup
            // but DIO wakeup will larger than it, in this case not need to decrease u16SleepDelta because DIO wakeup's wake delta is not determined
            if((u16WakeDelta > TOF_CARD_SYNC_DELTA_MIN))
                u16SleepDelta -= MIN((u16WakeDelta - TOF_CARD_SYNC_DELTA_MIN)/2, 4);
            else if((u16WakeDelta < TOF_CARD_SYNC_DELTA_MIN))
                u16SleepDelta += MAX((TOF_CARD_SYNC_DELTA_MIN - u16WakeDelta)/2, 1);

            u16SleepDelta = MAX(TOF_CARD_SYNC_DELTA_MIN, u16SleepDelta);

            // need to -(uint16)((u32AHI_TickTimerRead() - u32StationRecTick)/16000),
            // because if alarm status, card need change to common channel to send alarm msg when it receive station_finish (u32StationRecTick)

			DBG(
            PrintfUtil_vPrintf("u8StationRunMs: %d,u32StationRecTick:%d,tick:%d\n", u8StationRunMs,u32StationRecTick,u32AHI_TickTimerRead());)
            u8CardRunMs = (uint16)u8StationRunMs + (uint16)((u32AHI_TickTimerRead() - u32StationRecTick)/16000);
            u16StationMs = tofStationPeriodMs - u8CardRunMs;
        }
        DBG(
            PrintfUtil_vPrintf("runtime: %d   %d  %d\n", u8CardRunMs, u16WakeDelta,u16SleepDelta);
        )

        EventUtil_vSetEvent(TOF_SLEEP_EVENT);
        EventUtil_vUnsetEvent(u32EventID);
        break;
    }

    case TOF_LOCATOR_DONE_EVENT:
    {
        //u8NeedLocator = 0;

        if(u8NeedLocator > 2)
        {
            u8NeedLocator = u8NeedLocator - 2;
        }
        else if(u8NeedLocator > 0)
        {
            u8NeedLocator = 0;
        }

        eCardTofState = E_TOF_STATE_LOCATOR_FINISH;
        //u8CardRunMs = (uint16)(u32AHI_TickTimerRead()/16000);
        u8CardRunMs = (uint16)((u32AHI_TickTimerRead() - u32LocatorRecTick)/16000);
        u16StationMs -= u8CardRunMs;

        DBG(PrintfUtil_vPrintf("Loctime:%d\n",u8CardRunMs);)

        EventUtil_vSetEvent(TOF_SLEEP_EVENT);
        EventUtil_vUnsetEvent(u32EventID);
        break;
    }

    case TOF_SLEEP_EVENT:
    {
        bool_t bOkToSleep;
        uint32 dio;
        dio = u32AHI_DioReadInput();
        if((0 == ( dio & TOF_HELP_PIN))							// the help button is press
                || ((u8CardStatus & CARD_STATUS_RETREAT) && (bRetreatAck == FALSE)) 	// user didn't press OK button when retreat
                || (u8HelpCnt < TOF_CARD_HELP_TIMES) || u8ExciteTure)								// still helping
        {
            bOkToSleep = FALSE;

            if((u8HelpCnt < TOF_CARD_HELP_TIMES)) u8HelpCntError++;
            if(u8HelpCntError>10) u8HelpCnt=TOF_CARD_HELP_TIMES+1;

            // In help status, ignore locator because card need to change to common channel to receive help_ack
            if (u8HelpCnt < TOF_CARD_HELP_TIMES || u8ExciteTure)
            {
                u8NeedLocator = 0;
            }
        }
        else
            bOkToSleep = TRUE;

        DBG(
            PrintfUtil_vPrintf("Sleepevent: %d OK:%d, bhelped:%d bHelpSync:%d  st:%d ack%d: cnt:%d, cnterr:%d dio:%X,Ex:%d\n",
                               u16StationMs,bOkToSleep, bHelped, bHelpSync, u8CardStatus, bRetreatAck, u8HelpCnt, u8HelpCntError, dio,u8ExciteTure);
        )

        // calculate retreat ack times(do we care accurate time?)
        if((u8CardStatus & CARD_STATUS_RETREAT_ACK)&&
            (bRetreatAck == TRUE)&&(u8RetreatAckCnt < TOF_CARD_RETREAT_ACK_TIMES))
        {
            u8RetreatAckCnt++;
        }
        else if(u8CardStatus & CARD_STATUS_RETREAT_ACK)
        {
            u8CardStatus &= (~CARD_STATUS_RETREAT_ACK);
        }

        if(u8NeedLocator > 0)
        {
            if(u8LocToCardIndex == 0xFF)
            {
                vGetLocatorMs();
                if(u16LocatorMs > u8CardRunMs)
                {
                    u16LocatorMs -= u8CardRunMs;
                    while(u16LocatorMs > tofStationPeriodMs)
                        u16LocatorMs -= tofStationPeriodMs;
                    if ((u16LocatorMs > (u16SleepDelta + 20)) && bOkToSleep)
                    {
                        //3:卡实际休眠时间比设定的时间多3ms
                        u16StationMs -= (u16LocatorMs - u16SleepDelta + 9);
                        u32WakeupEvent = TOF_LOCATOR_EVENT;
                        u32WaitLocatorTimeout = TOF_PENDING_TIMEOUT;
                        u16SeqNum--;	// because when wakeup, u16SeqNum++;

                        DBG(
                            PrintfUtil_vPrintf("loc sleep:%d %d \n\n", u16LocatorMs - u16SleepDelta, u16SleepDelta);
                        )

                        vCardSleep(u16LocatorMs - u16SleepDelta);
                    }
                    else
                    {
                        u32WaitLocatorTimeout = TOF_PENDING_TIMEOUT;
                        EventUtil_vSetEvent(TOF_LOCATOR_EVENT);
                    }
                }
                else
                {
                    u32WaitLocatorTimeout = TOF_PENDING_TIMEOUT;
                    EventUtil_vSetEvent(TOF_LOCATOR_EVENT);
                }
            }
            else
            {
                if(u8NeedLocator == 1)
                    u16LocatorMs = (TOF_SLOT_LOC_INT - (u16CardSlot%TOF_SLOT_LOC_INT)) * TOF_SLOT_MS;
                else
                    u16LocatorMs = (TOF_SLOT_LOC_INT - ((u16CardSlot+TOF_SLOT_LOC_INT/2)%TOF_SLOT_LOC_INT)) * TOF_SLOT_MS;

                uint16 u16LocToCardTimes = 0;

    #define WAIT_LOC_TIMEOUT     (2* (TOF_SLOT_APPR_MS_2 + 5) + 10)
                //0~TOF_SLOT_LOC_INT 有3个空slot 辅站占用2个slot
                // if(u8NeedLocator) 则u8LocToCardIndex > 0
                if(u8LocToCardIndex <= TOF_SLOT_LOC_INT-5)
                {
                     u16LocToCardTimes = u16LocatorMs + (u8LocToCardIndex -1) * (TOF_SLOT_APPR_MS_2 + 5) + 5;
                }
                else
                {
                     u32WaitLocatorTimeout =  TOF_LOCATOR_PERIOD_MS + u16SleepDelta;
                     DBG(PrintfUtil_vPrintf("Error: %d \n",u8LocToCardIndex);)
                }

                if(u16LocToCardTimes > (tofStationPeriodMs - (u16StationMs%tofStationPeriodMs)))
                {
                    u16LocToCardTimes -= (tofStationPeriodMs - (u16StationMs%tofStationPeriodMs));

                    while(u16LocToCardTimes > tofStationPeriodMs)
                        u16LocToCardTimes -= tofStationPeriodMs;

                    if ((u16LocToCardTimes > u16SleepDelta) && bOkToSleep)
                    {
                        u16StationMs -=  (u16LocToCardTimes - u16SleepDelta+ 6);
                        u32WakeupEvent = TOF_LOCATOR_EVENT;
                        u32WaitLocatorTimeout = WAIT_LOC_TIMEOUT;
                        u16SeqNum--;    // because when wakeup, u16SeqNum++;

                        vCardSleep(u16LocToCardTimes - u16SleepDelta);
                    }
                    else
                    {
                        u32WaitLocatorTimeout = WAIT_LOC_TIMEOUT;
                        EventUtil_vSetEvent(TOF_LOCATOR_EVENT);
                    }
                }
                else
                {
                    u32WaitLocatorTimeout =  WAIT_LOC_TIMEOUT;
                    EventUtil_vSetEvent(TOF_LOCATOR_EVENT);
                }
            }
        }

        // not need locator
        else
        {
            u32WakeupEvent = (u8LostTimes == 0) ? TOF_STATION_EVENT : TOF_LISTEN_EVENT;
            if(u8LostTimes >= TOF_STATION_CARD_MAX_FAILED)
            {
                if(u8ListenIdleTime < 2)
                {
                    u8ListenIdleTime++;
                }
            }
            else
            {
                u8ListenIdleTime = 0;
            }

            // just for safe check, to ensure the station's coming time is within (0, TOF_STATION_PERIOD_MS]
            while(u16StationMs > tofStationPeriodMs)
                u16StationMs -= tofStationPeriodMs;

            if(bOkToSleep)
            {
                if (u16StationMs <= 5 + u16SleepDelta) //&& (bTofConnected))
                {
                    DBG(
                        PrintfUtil_vPrintf("no time to sleep: %d\n\n", u16StationMs);
                    )
                    //not need to increase u16SeqNum

                    if(u8LocCnt != 255)
                    {
                        u8LocCnt++;
                    }
                    u16SeqNum++;
                    vInitParam();
                    vAHI_TickTimerWrite(0);
                    EventUtil_vSetEvent(u32WakeupEvent);
                }
                else
                {
                    DBG(
                        PrintfUtil_vPrintf("go to sleep: %d %d \n\n", u16StationMs - u16SleepDelta, u16SleepDelta);
                    )
                    vCardSleep(u16StationMs - u16SleepDelta);
                }
            }
            else
            {
                u16SeqNum++;
                vInitParam();

                // the wake up envent is coming, just wait for this event
                if (u16StationMs <= 5 + u16SleepDelta)
                    EventUtil_vSetEvent(u32WakeupEvent);
                else
                    TimerUtil_eSetTimer(u32WakeupEvent, u16StationMs - u16SleepDelta - 5);
            }
        }

        EventUtil_vUnsetEvent(u32EventID);
        break;
    }

    case TOF_WATCHDOG_EVENT:
    {
        // if protect counter overflow, restart system
        if(u8ProtectCounter++ > 30)
        {
            vAHI_SwReset();
        }
        if(FALSE == bTofConnected)
        {
            if(u16StationConnectPrt++ > 600)
                vAHI_SwReset();
        }
        else
        {
            u16StationConnectPrt = 0;
        }
        vAHI_WatchdogRestart();
        EventUtil_vUnsetEvent(u32EventID);
        break;
    }

    case TOF_RSSI_SEND_TIMEOUT_EVENT:
    {
        vCardSleep(15000);
        EventUtil_vUnsetEvent(u32EventID);
        break;
    }

    default:
    {
        EventUtil_vUnsetEvent(u32EventID);
        break;
    }
    }
}

/****************************************************************************
 *
 * NAME: vCardCast
 *
 * DESCRIPTION:
 * 	Card send msg to air
 *
 * PARAMETERS:
 *				u8CmdType - msg type
 *				u16DstAddr / u16DstPanId - destination
 *				u8TxOptions - retry or not
 *				u16value - optional value
 *
 * RETURNS:
 * void
 *
 ****************************************************************************/
PRIVATE void vCardCast(uint8 u8CmdType, uint16 u16DstAddr, uint16 u16DstPanId, uint8 u8TxOptions, uint16 u16value)
{
    bMsgSending = TRUE;

    MacUtil_SendParams_s sParams;
    app_eDev_Data_t* pDev_hdr;
    sParams.u8Radius		= 1;
    sParams.u16DstAddr	= u16DstAddr;
    sParams.u16DstPanId 	= u16DstPanId;
    sParams.u16ClusterId 	= 0;
    sParams.u8DeliverMode	= MAC_UTIL_UNICAST;

    RfTofWrapper_tu RfTofData;
    RfTofData.tof_head.protocoltype = APP_PROTOCOL_TYPE_CARD;
    RfTofData.tof_head.msgtype = u8CmdType;

    switch (u8CmdType)
    {
    case TOF_CARD_REQUEST:
	{
		RfTofData.rf_tof_card_data.u16SeqNum = u16value;
        RfTofData.rf_tof_card_data.u8CardStatus = u8CardStatus;
        RfTofData.rf_tof_card_data.u8ExciterIDorAccStatus = u8CardPeriod;	// this is for test, u8Reserved can be any value

        //if(u16Battery>0)
        //{
            RfTofData.rf_tof_card_data.u16OadVersion= 0xFFFF;
            RfTofData.rf_tof_card_data.u16Battery= 0xFFFF;//(u16Battery<<5) + u8RssiChannel;
            //u16SendVersionCnt = 16;   //16 X 5S
        //}

        //DBG(PrintfUtil_vPrintf("ver T%d C%d V%x B%d\n",u8CmdType, u16SendVersionCnt, OAD_CARD_VERSION,u16Battery););
        RfTofData.tof_head.len = sizeof(rf_tof_card_data_ts) - sizeof(app_header_t);
		break;
	}
    case TOF_CARD_RSSI:
    //case TOF_CARD_ALARM:
    //case TOF_CARD_CHECKIN:
	//case TOF_CARD_EXCITE:
    {
        RfTofData.rf_tof_card_data.u16SeqNum = u16value;
        RfTofData.rf_tof_card_data.u8CardStatus = u8CardStatus;
        RfTofData.rf_tof_card_data.u8ExciterIDorAccStatus = u8CardMotion;	// this is for test, u8Reserved can be any value

        if(u16Battery>0)
        {
            RfTofData.rf_tof_card_data.u16OadVersion= u16CardVersion;
            RfTofData.rf_tof_card_data.u16Battery= (u16Battery<<5) + u8RssiChannel;
            //u16SendVersionCnt = 16;   //16 X 5S
            //RfTofData.rf_tof_card_base_data.u8Erpid
        }

        //DBG(PrintfUtil_vPrintf("ver T%d C%d V%x B%d\n",u8CmdType, u16SendVersionCnt, OAD_CARD_VERSION,u16Battery););
        RfTofData.tof_head.len = sizeof(rf_tof_card_data_ts) - sizeof(app_header_t);
        break;
    }

    case TOF_CARD_CHECKIN:
    {
        RfTofData.rf_tof_card_data.u16SeqNum = u16value;
        RfTofData.rf_tof_card_data.u8CardStatus = u8CardStatus;
        RfTofData.rf_tof_card_data.u8ExciterIDorAccStatus = u8CardPeriod;	// this is for test, u8Reserved can be any value
        RfTofData.rf_tof_card_data.u16OadVersion= u16CardVersion;
        RfTofData.rf_tof_card_data.u16Battery= (u16Battery<<5) + u8RssiChannel;
        RfTofData.tof_head.len = sizeof(rf_tof_card_data_ts) - sizeof(app_header_t);
        if((rt_erpid_info.u8ErpIdLen > 0) && (rt_erpid_info.u8ErpIdLen < 20))
        {
            memcpy(RfTofData.rf_tof_card_base_data.u8Erpid,rt_erpid_info.u8ErpBuf,rt_erpid_info.u8ErpIdLen);
            RfTofData.tof_head.len += rt_erpid_info.u8ErpIdLen;

            PrintfUtil_vPrintf("report erp \n");
        }
        break;
    }

    case DEVICE_CARD_PERIOD_CHANNEL_ACK:
    {
        RfTofData.rf_tof_card_data.u16SeqNum = u16value;
        RfTofData.rf_tof_card_data.u8CardStatus = u8CardStatus;
        RfTofData.rf_tof_card_data.u8ExciterIDorAccStatus = u8CardPeriod;	// this is for test, u8Reserved can be any value
        RfTofData.rf_tof_card_data.u16OadVersion= u16CardVersion;
        RfTofData.rf_tof_card_data.u16Battery= (u16Battery<<5) + u8RssiChannel;
        RfTofData.tof_head.len = sizeof(rf_tof_card_data_ts) - sizeof(app_header_t);
        break;
    }
    #if 0
	case TOF_CARD_ALARM:
	//case TOF_CARD_EXCITE:
	{
        uint8 tx_buf[128];
        RfTofData.rf_card_info1.u16SeqNum = u8CardPeriod;   //卡的工作周期

        DBG(PrintfUtil_vPrintf("perA %d\n",u8CardPeriod);)
        RfTofData.rf_card_info1.u8CardStatus = u8CardStatus;
        RfTofData.rf_card_info1.u8ExciterIDorAccStatus = (u8ExciteAddr & 0x7f);
        RfTofData.rf_card_info1.u16OadVersion= u16CardVersion;
        RfTofData.rf_card_info1.u16Battery= u16Battery;

        DBG(PrintfUtil_vPrintf("cur %d\n",card_info_cfg.u8Frame_cur);)
        if(card_info_cfg.u8BaseInfoCnt)
        {
            DBG(PrintfUtil_vPrintf("base\n");)
            bAHI_FullFlashRead(INFO_STORE_ADDR, CARD_INFO_SIZE,read_buf);
            pDev_hdr = (app_eDev_Data_t*)read_buf;

            DBG(PrintfUtil_vPrintf("SSSS %d,%d\n",pDev_hdr->frameSeq,pDev_hdr->frameSum);)
            //PrintfUtil_vPrintMem(read_buf,pDev_hdr->len+4);
            RfTofData.rf_card_info1.info_data.len = pDev_hdr->len;
            RfTofData.rf_card_info1.info_data.frameSeq = 0;
            RfTofData.rf_card_info1.info_data.datatype = 0;
            RfTofData.rf_card_info1.info_data.frameSum = card_info_cfg.u8Frame_sum;

            RfTofData.tof_head.len = sizeof(rf_card_info1_ts) - sizeof(app_header_t) + RfTofData.rf_card_info1.info_data.len;
            memcpy(tx_buf,(uint8*)&RfTofData,sizeof(rf_card_info1_ts));
            memcpy(tx_buf + sizeof(rf_card_info1_ts),read_buf + sizeof(app_eDev_Data_t),pDev_hdr->len);
            u8LastHandle = MacUtil_vSendData(&sParams, tx_buf, RfTofData.tof_head.len+4, u8TxOptions);
            return;
        }
        else
        {
            DBG(PrintfUtil_vPrintf("no\n");)
            RfTofData.tof_head.len = sizeof(rf_card_info1_ts) - sizeof(app_header_t);
            RfTofData.rf_card_info1.info_data.len = 0;
            RfTofData.rf_card_info1.info_data.frameSeq = 0;
            RfTofData.rf_card_info1.info_data.frameSum = 0;
            RfTofData.rf_card_info1.info_data.datatype = 0;
            break;
        }
	}
    

    case CARD_REMARK:
    {
        uint8 tx_buf[128];
        RfTofData.rf_card_info1.u16SeqNum = u8CardPeriod;   //卡的工作周期
        RfTofData.rf_card_info1.u8CardStatus = u8CardStatus;
        RfTofData.rf_card_info1.u8ExciterIDorAccStatus = (u8ExciteAddr & 0x7f);
        RfTofData.rf_card_info1.u16OadVersion= u16CardVersion;
        RfTofData.rf_card_info1.u16Battery= u16Battery;
        if(card_info_cfg.u8Frame_sum && (card_info_cfg.u8Frame_cur <= card_info_cfg.u8Frame_sum))
        {
            DBG(PrintfUtil_vPrintf("remark:%d\n",card_info_cfg.u8Frame_cur);)
            bAHI_FullFlashRead(INFO_STORE_ADDR + card_info_cfg.u8Frame_cur*CARD_INFO_SIZE,CARD_INFO_SIZE,read_buf);
            pDev_hdr = (app_eDev_Data_t*)read_buf;

            DBG(PrintfUtil_vPrintf("F:%d,%d\n",pDev_hdr->frameSeq,pDev_hdr->frameSum);)
            RfTofData.rf_card_info1.info_data.len = pDev_hdr->len;
            RfTofData.rf_card_info1.info_data.frameSeq = pDev_hdr->frameSeq;
            RfTofData.rf_card_info1.info_data.frameSum = card_info_cfg.u8Frame_sum;
            RfTofData.rf_card_info1.info_data.datatype = 1;
            RfTofData.tof_head.len = sizeof(rf_card_info1_ts) - sizeof(app_header_t) + RfTofData.rf_card_info1.info_data.len;
            memcpy(tx_buf,(uint8*)&RfTofData,sizeof(rf_card_info1_ts));
            memcpy(tx_buf + sizeof(rf_card_info1_ts),read_buf + sizeof(app_eDev_Data_t),pDev_hdr->len);
            u8LastHandle = MacUtil_vSendData(&sParams, tx_buf, RfTofData.tof_head.len+4, u8TxOptions);
            return;
        }
        else
        {
            DBG(PrintfUtil_vPrintf("no remark\n");)
            break;
        }

    }
    #endif
    case CARD_LOC_INFO:
    {
        RfTofData.tof_head.len = sizeof(rf_card_distance_ts) - sizeof(app_header_t);
        RfTofData.rf_card_distance.u16LocDistance = u16value;
        RfTofData.rf_card_distance.u16StationAddr = u16StationPanId;
        break;
    }

    default:
    {
        RfTofData.tof_head.len = 0;
        break;
    }
    }

    u8LastHandle = MacUtil_vSendData(&sParams, (uint8*)&RfTofData, RfTofData.tof_head.len+4, u8TxOptions);
}

/****************************************************************************
 *
 * NAME: vSaveCardStatus
 *
 * DESCRIPTION:
 * Save CardStatus to flash
 *
 * PARAMETERS:      Name    RW  Usage
 *                  u8TofChannel  w   tof channel
 *                  u8LocChannel  W   locator channel
 * RETURNS:
 * bool TURE for success, FALSE for fail
 *
 ****************************************************************************/
PRIVATE bool vSaveCardStatus(uint8 CardStatus)
{
    bAHI_FlashInit(E_FL_CHIP_INTERNAL, NULL);
    if(!bAHI_FullFlashRead(PARAM_STORE_ADDR, sizeof(CFG_OPTION_T),(uint8*)&cfg_option))
    {
        DBG(PrintfUtil_vPrintf("Flash Read Fail\n"););
        return FALSE;
    }
    if (CardStatus == cfg_option.u8Status)
    {
        return TRUE;
    }

    if(!bAHI_FlashEraseSector(7))
    {
        DBG(PrintfUtil_vPrintf("Flash Erase Fail\n"););
        return FALSE;
    }

    cfg_option.u8Status = CardStatus;

    if(bAHI_FullFlashProgram(PARAM_STORE_ADDR, sizeof(CFG_OPTION_T),(uint8*)&cfg_option))
    {
        //DBG(PrintfUtil_vPrintf("OK!%d\n", CardStatus););
        return TRUE;
    }
    else
    {
        DBG(PrintfUtil_vPrintf("Set  CardStatus fail!%d\n", CardStatus););
        return FALSE;
    }
}



/****************************************************************************
 *
 * NAME: vSetCardChannel
 *
 * DESCRIPTION:
 * 	Set card's channel
 *
 * PARAMETERS:
 *				channel - channel to be setted
 *
 * RETURNS:
 * void
 *
 ****************************************************************************/
PRIVATE void vSetCardChannel(uint8 channel)
{
    if((u8CurChannel != channel)&&(bMsgSending == FALSE))
    {
        eAppApiPlmeSet(PHY_PIB_ATTR_CURRENT_CHANNEL, channel);
        u8CurChannel = channel;
        //DBG(PrintfUtil_vPrintf("change channel%d %d\n", u8CurChannel, bMsgSending););
    }
}

/****************************************************************************
 *
 * NAME: vSendRssi
 *
 * DESCRIPTION:
 * 	Card broadcast rssi msg in common channel
 *
 * PARAMETERS: 	None
 *
 * RETURNS:
 * void
 *
 ****************************************************************************/
PRIVATE void vSendRssi()
{
    if(bRSSIed == FALSE)
    {
        //DBG(PrintfUtil_vPrintf("BBB%d\n",u8RssiChannel);)
        vSetCardChannel(u8RssiChannel);
        if(TOF_STATION_EVENT == u32WakeupEvent)
            vCardCast(TOF_CARD_RSSI, 0x0000, 0xFFFF, 0, u16SeqNum);
        else
            vCardCast(TOF_CARD_RSSI, 0x0000, 0xFFFF, 0, u16SeqNum+1);
        bRSSIed = TRUE;
    }
}

PRIVATE void vOnlySendRssi()
{
    vSetCardChannel(u8RssiChannel);
    vCardCast(TOF_CARD_RSSI, 0x0000, 0xFFFF, 0, u16SeqNum);
    eCardRssiState = TRUE;
    u32WakeupEvent = TOF_RSSI_SEND_TIMEOUT_EVENT;
    TimerUtil_eSetTimer(TOF_RSSI_SEND_TIMEOUT_EVENT,10);
    
}

/****************************************************************************
 *
 * NAME: vSendAlarm
 *
 * DESCRIPTION:
 * 	Card wait for station's RSSI / AVAILABLE msg to send alarm to the station
 *
 * PARAMETERS:
 *				u16ShortAddr - station's short addr
 *				u16PanId - station's pan ID
 *
 * RETURNS:
 * void
 *
 ****************************************************************************/
PRIVATE void vSendAlarm(uint16 u16ShortAddr, uint16 u16PanId)
{
    TimerUtil_eStopTimer(TOF_ALARM_TIMEOUT_EVENT);	// need to stop alarm timeout event
    EventUtil_vUnsetEvent(TOF_ALARM_TIMEOUT_EVENT);
    eCardTofState = E_TOF_STATE_WAIT_ALARM_ACK;
    vSetCardChannel(u8BroadcastChannel);
    //vCardCast(TOF_CARD_ALARM, u16ShortAddr, u16PanId, MAC_TX_OPTION_ACK, u16SeqNum);
}

/****************************************************************************
 *
 * NAME: vSyncStation
 *
 * DESCRIPTION:
 * 	Card wait station's signal msg to sync with station
 *
 * PARAMETERS:
 *				psAppPkt - station's signal msg
 *
 * RETURNS:
 * void
 *
 ****************************************************************************/
PRIVATE void vSyncStation(RfTofWrapper_tu* psAppPkt)
{
    uint16 u16SleepMs =((TOF_SLOT_MS * ((u16CardSlot  +TOF_SLOT_NUM - psAppPkt->rf_tof_station_signal.u16CurSlot)%tofSlotNum) - (uint16)(psAppPkt->rf_tof_station_signal.u8RunMs)) % tofStationPeriodMs);
	u16SleepDelta = TOF_CARD_SYNC_DELTA_MAX;
    //发现u16SleepMs有休眠时间大于定位周期的情况，但不知道什么原因造成的，所以加下面这段，确保u16SleepMs不超过定位周期
    while(u16SleepMs > tofStationPeriodMs)
        u16SleepMs -= tofStationPeriodMs;

    if(TRUE == bHelpSync)
    {
        bHelpSync = FALSE;
        TimerUtil_eStopTimer(TOF_HELP_EVENT);
        EventUtil_vUnsetEvent(TOF_HELP_EVENT);
        EventUtil_vSetEvent(TOF_HELP_EVENT);

        if (u16SleepMs <= 5 + 10)
            EventUtil_vSetEvent(TOF_STATION_EVENT);
        else
            TimerUtil_eSetTimer(TOF_STATION_EVENT, u16SleepMs - 10);
    }
    else
    {
        // sync timeout event is TOF_LISTEN_EVENT, need stop
        TimerUtil_eStopTimer(TOF_LISTEN_EVENT);
        EventUtil_vUnsetEvent(TOF_LISTEN_EVENT);

        // only sync to station, not need to consider locator (do we care lost locator one time?)
        if(u32WakeupEvent == TOF_LOCATOR_EVENT)
            u32WakeupEvent = TOF_STATION_EVENT;

        // not time to sleep, need init parameters and reset timer tick to be 0
        uint32 dio;
        dio = u32AHI_DioReadInput();
        if (u16SleepMs <= 5 + u16SleepDelta)
        {
            vInitParam();
            vAHI_TickTimerWrite(0);
            EventUtil_vSetEvent(u32WakeupEvent);
        }
        else if(0 == ( dio & TOF_HELP_PIN) 							// the help button is press
                || ((u8CardStatus & CARD_STATUS_RETREAT) && (bRetreatAck == FALSE)) 	// user didn't press OK button when retreat
                || (u8HelpCnt < TOF_CARD_HELP_TIMES) || (u8ExciteTure))
        {
            vInitParam();
            vAHI_TickTimerWrite(0);
            TimerUtil_eSetTimer(u32WakeupEvent, u16SleepMs - u16SleepDelta);
        }
        else
        {
            DBG(PrintfUtil_vPrintf("SleepMs:%d,Delta:%d,Period:%d\n",u16SleepMs,u16SleepDelta,tofStationPeriodMs);)
            vCardSleep(u16SleepMs - u16SleepDelta - TOF_RSSI_TIMEOUT);
        }
    }
}


/****************************************************************************
 *
 * NAME: vCardTypeCheck
 *
 * DESCRIPTION:
 * 	classify card period type and check
 *
 * PARAMETERS:
 *        card period type
 *
 * RETURNS:
 * void
 *
 ****************************************************************************/
PRIVATE void vCardTypeCheck(uint8 PeriodType)
{
    if(PeriodType == CARD_TOF_1S)   //1s card
    {
        tofSlotNum = TOF_SLOT_NUM/15;
        tofStationPeriodMs = TOF_STATION_PERIOD_MS/15;
        u8CardStatus |= CARD_STATUS_TOF1S;
        u16CardHelpTimeout = CARD_1S_HELP_STATUS_TIMEOUT;
        u16CardMotionTimeout = CARD_1S_MOTION_CNT;
        DBG(PrintfUtil_vPrintf("CARD_1S: %d \n",u16CardShortAddr);)
    }
    else if(PeriodType == CARD_TOF_5S)    //5s card
    {
        tofSlotNum = TOF_SLOT_NUM/3;
        tofStationPeriodMs = TOF_STATION_PERIOD_MS/3;
        u8CardStatus &= (~(CARD_STATUS_TOF1S | CARD_STATUS_TOF15S));
        u16CardHelpTimeout = CARD_1S_HELP_STATUS_TIMEOUT;
        u16CardMotionTimeout = CARD_5S_MOTION_CNT;
        DBG(PrintfUtil_vPrintf("CARD_5S: %d \n",u16CardShortAddr);)
    }
    else if(PeriodType == CARD_TOF_15S)                                  //15s card
    {
        tofSlotNum = TOF_SLOT_NUM;
        tofStationPeriodMs = TOF_STATION_PERIOD_MS;
        u16CardHelpTimeout = CARD_5S_HELP_STATUS_TIMEOUT;
        u8CardStatus |= CARD_STATUS_TOF15S;
        u16CardMotionTimeout = CARD_15S_MOTION_CNT;
		DBG(PrintfUtil_vPrintf("CARD_15S: %d\n",u16CardShortAddr);)
    }
    else
    {
        DBG(PrintfUtil_vPrintf("PeriodType: %d\n",PeriodType);)

        TimerUtil_vInit();
        while(1)
        {
            vAHI_DioSetOutput(0, JN_RED);
            TimerUtil_vDelay(100, E_TIMER_UNIT_MILLISECOND);
            vAHI_DioSetOutput(JN_RED, 0);
            TimerUtil_vDelay(500, E_TIMER_UNIT_MILLISECOND);

            vAHI_DioSetOutput(0, JN_GREEN);
            TimerUtil_vDelay(100, E_TIMER_UNIT_MILLISECOND);
            vAHI_DioSetOutput(JN_GREEN, 0);
            TimerUtil_vDelay(500, E_TIMER_UNIT_MILLISECOND);
        }
    }

    DBG(PrintfUtil_vPrintf("channel: %d\n",u8BroadcastChannel);)
    DBG(PrintfUtil_vPrintf("rssi channel: %d\n",u8RssiChannel);)
    u16StationMs = tofStationPeriodMs;
    u16SleepDelta = TOF_CARD_SYNC_DELTA_MAX;
}

PRIVATE void vCardShortAddrCheck(uint16 u16Addr,uint32 u32BCD)
{
    uint8 tmp, i;
    for(i=0; i < 4; i++)
    {
        tmp = StringUtil_BCD2uint8((uint8)((u32BCD >> (i*8))&0xFF));
        if(tmp == 0xFF)
            break;
    }
    if((u16Addr == 0) || (tmp == 0xFF) || (u16Addr > 65000))
    {
        DBG(PrintfUtil_vPrintf("ShortAddr err: %d\n",u32BCD);)
        TimerUtil_vInit();
        while(1)
        {
            vAHI_DioSetOutput(0, JN_RED);
            TimerUtil_vDelay(100, E_TIMER_UNIT_MILLISECOND);
            vAHI_DioSetOutput(JN_RED, 0);
            TimerUtil_vDelay(500, E_TIMER_UNIT_MILLISECOND);

            vAHI_DioSetOutput(0, JN_GREEN);
            TimerUtil_vDelay(100, E_TIMER_UNIT_MILLISECOND);
            vAHI_DioSetOutput(JN_GREEN, 0);
            TimerUtil_vDelay(500, E_TIMER_UNIT_MILLISECOND);
        }
    }
}



/****************************************************************************
 *
 * NAME: vInitParam
 *
 * DESCRIPTION:
 * 	Card init parameters
 *
 * PARAMETERS: 	None
 *
 * RETURNS:
 * void
 *
 ****************************************************************************/
PRIVATE void vInitParam()
{
    bTofConnected = FALSE;
    eCardTofState = E_TOF_STATE_IDLE;
    //u8NeedLocator = 0;
    bRSSIed = FALSE;
    LQ_StationPanid.StationPanid = 0;
    LQ_StationPanid.StationShortAddr = 0;
    LQ_StationPanid.u8LinkQuality = 0;
    LQ_StationPanid.u8ListenTimeOutCount = 0;
    LQ_StationPanid.u8ListenCount = 0;
    LQ_StationPanidNext.u8LinkQuality= 0;
    LQ_StationPanidNext.StationPanid = 0;
    LQ_StationPanidNext.StationShortAddr = 0;
    LQ_StationSearchStatus = LQ_STATION_SEARCH_STATUS_NORMAL;
}

/****************************************************************************
 *
 * NAME: vCheckStationStatus
 *
 * DESCRIPTION:
 * 	Check station's status
 *
 * PARAMETERS:
 *				u8StationStatus - station's status
 *
 * RETURNS:
 * void
 *
 ****************************************************************************/
PRIVATE void vCheckStationStatus(uint8 u8StationStatus)
{
    if (u8StationStatus == STATION_STATUS_RETREAT)
    {
        u8CardStatusNormal = 0;
        if(!(u8CardStatus & CARD_STATUS_RETREAT))
        {
            EventUtil_vSetEvent(TOF_RETREAT_EVENT);
            u8CardStatus |= CARD_STATUS_RETREAT;
            bRetreatAck = FALSE;
            vSaveCardStatus(u8CardStatus);
        }
    }
    else if (u8StationStatus == STATION_STATUS_NORMAL)
    {
        if((u8CardStatus & CARD_STATUS_RETREAT) && (u8CardStatusNormal++ > MAX_STATION_STATUS_NORMAL))
        {
            u8CardStatusNormal = MAX_STATION_STATUS_NORMAL+1;
            u8CardStatus &= (~(CARD_STATUS_RETREAT|CARD_STATUS_RETREAT_ACK));
            TimerUtil_eStopTimer(TOF_RETREAT_EVENT);
            EventUtil_vUnsetEvent(TOF_RETREAT_EVENT);
            vSaveCardStatus(u8CardStatus);
        }
        //bRetreatAck = FALSE;
    }
    //else STATION_STATUS_SINGLE_RETREAT
}

/****************************************************************************
 *
 * NAME: vProcessIncomingMlme
 *
 ****************************************************************************/
PRIVATE void vProcessIncomingMlme(MAC_MlmeDcfmInd_s *psMlmeInd)
{
    switch (psMlmeInd->u8Type)
    {
    case MAC_MLME_IND_ASSOCIATE: /* Incoming association request */
        break;

    case MAC_MLME_DCFM_SCAN: /* Incoming scan results */
        break;

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
 * NAME: vHandleMcpsDataDcfm
 *
 ****************************************************************************/
PRIVATE void vHandleMcpsDataDcfm(MAC_McpsDcfmInd_s *psMcpsInd)
{
    if(psMcpsInd->uParam.sDcfmData.u8Handle == u8LastHandle)
    {
        bMsgSending = FALSE;
        if(eCardRssiState)
        {
            eCardRssiState = FALSE;
            TimerUtil_eStopTimer(u32WakeupEvent);
            EventUtil_vUnsetEvent(u32WakeupEvent);
            EventUtil_vSetEvent(u32WakeupEvent);
        }
    }
}


/****************************************************************************
 *
 * NAME: vSetLed
 *
 * DESCRIPTION:
 * 	Set a certain Led to be on or off
 *
 * PARAMETERS:
 *				u32Pin - LED's pin
 *				bOnOff - TRUE(On) or FALSE(off)
 *				u16OnMs - the led on last u16OnMs if bOnOff == TRUE
 *
 * RETURNS:
 * void
 *
 ****************************************************************************/
PRIVATE void vSetLed(uint32 u32Pin, bool_t bOnOff, uint16 u16OnMs)
{
    if(bOnOff)
        vAHI_DioSetOutput(0, u32Pin);
    else
        vAHI_DioSetOutput(u32Pin, 0);
    if(u16OnMs > 0)
    {
        if(JN_RED == u32Pin)
            TimerUtil_eSetTimer(TOF_LED_RED_OFF_EVENT, u16OnMs);
        else if(JN_GREEN == u32Pin)
            TimerUtil_eSetTimer(TOF_LED_GREEN_OFF_EVENT, u16OnMs);
    }
}

PRIVATE void vGetLocatorMs(void)
{
    if(eCardTofState == E_TOF_STATE_STATION_FINISH)
    {
        if(u8NeedLocator == 1)    //locator 0
        {
            u16LocatorMs = (TOF_SLOT_LOC_INT + 2 - (u16CardSlot%TOF_SLOT_LOC_INT)) * TOF_SLOT_MS+((u16CardSlot%TOF_SLOT_LOC_INT) - 2)*18;
            //u16LocatorMs = (TOF_SLOT_LOC_INT + 2 - (u16CardSlot%TOF_SLOT_LOC_INT)) * TOF_SLOT_MS;
        }
        else if(u8NeedLocator == 2)  //locator 1
        {
            if(u16CardSlot%TOF_SLOT_LOC_INT < TOF_SLOT_LOC_OFFSET)
            {
                u16LocatorMs = (TOF_SLOT_LOC_INT + 2 - (u16CardSlot%TOF_SLOT_LOC_INT))*TOF_SLOT_MS + (TOF_SLOT_LOC_INT - TOF_SLOT_LOC_OFFSET -2 + (u16CardSlot%TOF_SLOT_LOC_INT))*18;
            }
            else
            {
                u16LocatorMs = (TOF_SLOT_LOC_INT + 2 - (u16CardSlot%TOF_SLOT_LOC_INT))*TOF_SLOT_MS + ((u16CardSlot%TOF_SLOT_LOC_INT) - TOF_SLOT_LOC_OFFSET)*18;
            }
        }
        else if(u8NeedLocator == 3)    //locator 0 and locator 1
        {
            if(u16CardSlot%TOF_SLOT_LOC_INT < TOF_SLOT_LOC_OFFSET)
            {
                u16LocatorMs = (TOF_SLOT_LOC_INT + 2 - (u16CardSlot%TOF_SLOT_LOC_INT)) * TOF_SLOT_MS+((u16CardSlot%TOF_SLOT_LOC_INT) -2)*18;
                u16NextLocatorMs = (TOF_SLOT_LOC_INT - TOF_SLOT_LOC_OFFSET)*18;
            }
            else
            {
                u16LocatorMs = (TOF_SLOT_LOC_INT + 2 - (u16CardSlot%TOF_SLOT_LOC_INT))*TOF_SLOT_MS + ((u16CardSlot%TOF_SLOT_LOC_INT) - TOF_SLOT_LOC_OFFSET)*18;
                u16NextLocatorMs = (TOF_SLOT_LOC_OFFSET -2)*18;
            }
        }
    }
    else if(eCardTofState == E_TOF_STATE_LOCATOR_FINISH)
    {
        u16LocatorMs = u16NextLocatorMs + u16SleepDelta;
    }
}


/****************************************************************************
 *
 * NAME: vBuzzer
 *
 * DESCRIPTION:
 * 	Start buzzer and last for u32Ms
 *
 * PARAMETERS:
 *				u32Ms - buzzer ms
 *
 * RETURNS:
 * void
 *
 ****************************************************************************/
PRIVATE void vBuzzer(uint32 u32Ms)
{
    TimerUtil_eSetTimer(TOF_STOP_BUZZER_EVENT, u32Ms);
    vAHI_TimerDIOControl(TOF_BUZZER_TIMER, TRUE);
    vAHI_TimerEnable(TOF_BUZZER_TIMER, 12, FALSE, FALSE, TRUE);
	vAHI_TimerConfigureOutputs(TOF_BUZZER_TIMER,TRUE,TRUE);
    vAHI_TimerClockSelect(TOF_BUZZER_TIMER, FALSE, TRUE);
    vAHI_TimerStartRepeat(TOF_BUZZER_TIMER, 1, 2);
    u8ProtectCounter = 0;
}

/****************************************************************************
 *
 * NAME: vStopBuzzer
 *
 * DESCRIPTION:
 * 	stop the buzzer
 *
 * PARAMETERS: 	None
 *
 * RETURNS:
 * void
 *
 ****************************************************************************/
PRIVATE void vStopBuzzer()
{
    vAHI_DioSetPullup(0, TOF_BUZZER_PIN);
    vAHI_TimerDisable(TOF_BUZZER_TIMER);

    vAHI_DioSetDirection(0,TOF_BUZZER_PIN);
    vAHI_DioSetOutput(0,TOF_BUZZER_PIN);
}



/*PRIVATE void vCardMotionDetect()
{
	static teMMA845x_Data vmma845xData;

    static bool_t mma845x_invalid = FALSE;
    uint8 value;
    uint8 PlStauts;

    if (mma845x_invalid)
        return;

    vAHI_DioWakeEnable(0, TOF_MOTION_IT_PIN);

	value = mma845xData.u8IntSrc = MMA845x_ReadReg(0x0C);     //INT_SOURCE

    if(0xFF == value)
    {
        if(MMA845x_ChipCheck())
        {
            mma845x_invalid = TRUE;
            u8CardMotion &= ~APP_CARD_IS_WITH_ACCEL;
            return;
        }
        else
        {
            mma845xData.u8IntSrc = MMA845x_ReadReg(0x0C);     //INT_SOURCE
            mma845x_invalid = FALSE;
            u8CardMotion |= APP_CARD_IS_WITH_ACCEL;
        }
    }

	mma845xData.u8FFMtSrc = MMA845x_ReadReg(0x16);    //FF_MT_SRC
	//mma845xData.u8Sysmod = MMA845x_ReadReg(0x0B);     //SYSMOD
	mma845xData.u8TransientSrc = MMA845x_ReadReg(0x1E);
	//mma845xData.u8XbyteH = MMA845x_ReadReg(0x01);
	//mma845xData.u8YbyteH = MMA845x_ReadReg(0x03);
	//mma845xData.u8ZbyteH = MMA845x_ReadReg(0x05);

	PlStauts = MMA845x_ReadReg(0x10);   //PL Status Register

	//if((DEFF(mma845xData.u8XbyteH,vmma845xData.u8XbyteH) > D_VALUE))
		//|| (DEFF(mma845xData.u8YbyteH,vmma845xData.u8YbyteH) > D_VALUE)
		//|| (DEFF(mma845xData.u8ZbyteH,vmma845xData.u8ZbyteH) > D_VALUE))
	//{
		//u8CardMotion |= APP_CARD_MOTION;
        //PrintfUtil_vPrintf("MO1:%d,%d\n",mma845xData.u8XbyteH,vmma845xData.u8XbyteH);
	//}
	//else if((mma845xData.u8TransientSrc & 0x40) || (mma845xData.u8FFMtSrc & 0x80))
    if((mma845xData.u8TransientSrc & 0x40) || (mma845xData.u8FFMtSrc & 0x80))
	{
		u8CardMotion |= APP_CARD_MOTION;
	}
	else
	{
		u8CardMotion &= ~APP_CARD_MOTION;
	}

	vmma845xData = mma845xData;
	if((PlStauts & 0x04) && (!(PlStauts & 0x40)))
	{
		u8CardMotion &= ~APP_CARD_ORIENTATION;
	}
	else
	{
		u8CardMotion |= APP_CARD_ORIENTATION;
	}

    if((APP_CARD_MOTION & u8CardMotion) || ((u8CardStatus & CARD_STATUS_RETREAT) && (bRetreatAck == FALSE)))
    {
        u16CardMotionCnt = 0;
    }
    else
    {
        if(u16CardMotionCnt < u16CardMotionTimeout)
            u16CardMotionCnt++;
    }
	DBG(PrintfUtil_vPrintf("Card PLStatus %d,%d\n",u8CardMotion,u16CardMotionCnt);)
}*/


PRIVATE void vDioCallBack(uint32 u32Device, uint32 u32ItemBitmap)
{
    if(u32Device == E_AHI_DEVICE_SYSCTRL)
    {
        if((bHelped == FALSE) && (u8HelpCnt >= TOF_CARD_HELP_TIMES) && (TOF_HELP_PIN & u32ItemBitmap))
        {
            DBG(
                PrintfUtil_vPrintf("help DIO\n");
            );
            //TimerUtil_eSetTimer(TOF_CHECK_HELP_EVENT, TOF_HELP_BUTTON_MS);
        }

        else if (TOF_OK_PIN & u32ItemBitmap)
        {
            DBG(
                PrintfUtil_vPrintf("OK DIO\n");
            );
            if((u8CardStatus & CARD_STATUS_RETREAT) && bRetreatAck == FALSE)
            {
                u8RetreatAckCnt = 0;
                u8CardStatus |= CARD_STATUS_RETREAT_ACK;
            }
            bRetreatAck = TRUE;
        }
        else if (TOF_CHECKIN_PIN & u32ItemBitmap)
        {
            u8CheckinCnt = 0;
            u8LocCnt = 255;     //读卡时，用来上报定位数据给手持机，需要区分读卡还是写卡，写卡不需上报
            EventUtil_vSetEvent(TOF_CHECKIN_EVENT);
        }

    }
}


PRIVATE void vCardSleep(uint32 u32SleepMs)
{
    //if(((eCardTofState==E_TOF_STATE_STATION_FINISH)&&(u16CardMotionCnt == u16CardMotionTimeout))||((u16CardMotionCnt == u16CardMotionTimeout)&& (FALSE == bTofConnected)))

    if(u8LocCnt == 255)
    {
        u8LocCnt = u8MaxTofCnt - 1;
        u8ReportDistance = TRUE;
    }

    if((eCardTofState==E_TOF_STATE_STATION_FINISH)||(FALSE == bTofConnected) || u8LocCnt > u8MaxTofCnt)
    {
    	if(u8NeedLocator > 0)
		    u16SeqNum++;

        if((u8LocCnt < u8MaxTofCnt) && (u8LocCnt != 0))
        {
            goto stop;
        }

        u16SleepCnt++;
        u8SleepMode = 1;
        //7天仓库模式
        if(u8CardPeriod == 168)
            u32SleepMs = 1200000;
        else
        u32SleepMs = 15000;
        
 #if 0

        //if(u8CardPeriod == 253)    //1分钟
        //{
            //u32SleepMs = 1500;
            //u16SleepCnt = 65000;
        //}
        //else if(u8CardPeriod == 254)   //5分钟
        //{
            //u32SleepMs = 1500;
            //u16SleepCnt = 65000;
        //}
        //else
        //{
            //u32SleepMs = 3600000*u8CardPeriod;
            //u32SleepMs = 1500;
            //u32SleepMs = 3000;
            //DBG(PrintfUtil_vPrintf("SLEEp %d\n",u16SleepCnt);)
        //}
            //u32SleepMs = 10000;
 #endif
        u32WakeupEvent = TOF_LISTEN_EVENT;
        u16StationConnectPrt = 0;
        vInitParam();
        u8NeedLocator = 0;
        u8LocCnt = 0;
        vAHI_DioSetDirection(TOF_MOTION_IT_PIN, 0);
        vAHI_DioWakeEnable(TOF_MOTION_IT_PIN, 0);
        vAHI_DioWakeEdge(TOF_MOTION_IT_PIN, 0);
    }
    else
    {
        stop:
        u8LocCnt++;
        u8SleepMode = 0;
    }

    u8CardStatus &= ~CARD_STATUS_EXCITER;

    vSetLed(JN_GREEN, FALSE, 0);
    vSetLed(JN_RED, FALSE, 0);
    u32Timeout = 0;
	u8iCount = 0;

    //u8CardStatus &= ~CARD_STATUS_EXCITER;
    //vAHI_DioSetPullup(0, TOF_BUZZER_PIN);
    //vAHI_TimerDisable(TOF_BUZZER_TIMER);
    vAHI_DioSetDirection(0,TOF_BUZZER_PIN);
    vAHI_DioSetOutput(0,TOF_BUZZER_PIN);

    vAHI_UartDisable(E_AHI_UART_0);
    vAHI_DioWakeEnable(0, TOF_OK_PIN);	// need disable OK button's wakeup ability when sleep
    SleepUtil_Sleep(E_AHI_WAKE_TIMER_0, u32SleepMs, E_AHI_SLEEP_OSCON_RAMON);
}

PRIVATE uint8 vParse_card_info(uint8* pdata,uint8 len)
{
    uint8 i;
    uint8 ret = 0;
    app_header_t * papp_hdr = (app_header_t*)pdata;
    app_eDev_Data_t* pdata_hdr = (app_eDev_Data_t*) (papp_hdr + 1);

    //PrintfUtil_vPrintMem(pdata,len);

    app_eDev_Data_t* p;
    memcpy(write_buf1,(uint8*) pdata_hdr,pdata_hdr->len + sizeof(app_eDev_Data_t));

    if(card_info_cfg.u8Frame_sum || card_info_cfg.u8BaseInfoCnt)
    {
        //DBG(PrintfUtil_vPrintf("Num:%d,%d\n",card_info_cfg.u8Frame_sum,card_info_cfg.u8BaseInfoCnt);)
        if(card_info_cfg.u8BaseInfoCnt == 0)
        {
            bAHI_FullFlashRead(INFO_STORE_ADDR,(card_info_cfg.u8Frame_sum+1)*CARD_INFO_SIZE,flash_info);
        }
        else
        {
            bAHI_FullFlashRead(INFO_STORE_ADDR,(card_info_cfg.u8Frame_sum+card_info_cfg.u8BaseInfoCnt)*CARD_INFO_SIZE,flash_info);
        }
    }

    //同时修改基本信息和备注信息

    //DBG(PrintfUtil_vPrintf("f1:%d,%d\n",pdata_hdr->len,papp_hdr->len);)
    if(papp_hdr->len > (pdata_hdr->len + sizeof(app_eDev_Data_t)))
    {
        if(pdata_hdr->datatype == 0)    //第一条是基本数据类型
        {
            //DBG(PrintfUtil_vPrintf("Base\n");)
            if(pdata_hdr->len == 0)    //删除基本数据类型
            {
                DBG(PrintfUtil_vPrintf("--Base\n");)
                card_info_cfg.u8BaseInfoCnt = 0;
            }
            else                      //修改或增加基本信息
            {
                //DBG(PrintfUtil_vPrintf("w2\n");)
                card_info_cfg.u8BaseInfoCnt = 1;
                memcpy(flash_info,write_buf1,CARD_INFO_SIZE);
            }
        }
        pdata_hdr = (app_eDev_Data_t*) (pdata + sizeof(app_header_t) + pdata_hdr->len + sizeof(app_eDev_Data_t));
        memcpy(write_buf1,(uint8*) pdata_hdr,pdata_hdr->len + sizeof(app_eDev_Data_t));

        goto NEXT_MSG;
    }
    else
    {
NEXT_MSG:
        if(pdata_hdr->datatype == 0)    //基本数据类型
        {
            if(pdata_hdr->len == 0)    //删除基本数据类型
            {
                card_info_cfg.u8BaseInfoCnt = 0;
                //DBG(PrintfUtil_vPrintf("--w5\n");)
            }
            else                      //修改或增加基本信息
            {
                //DBG(PrintfUtil_vPrintf("w3\n");)
                card_info_cfg.u8BaseInfoCnt = 1;
                memcpy(flash_info,write_buf1,CARD_INFO_SIZE);
                bAHI_FlashEraseSector(6);
                bAHI_FullFlashProgram(INFO_STORE_ADDR,(card_info_cfg.u8Frame_sum+card_info_cfg.u8BaseInfoCnt)*CARD_INFO_SIZE,flash_info);
                ret = 1;
            }
        }
        else if(pdata_hdr->len == 0)    //删除一条数据
        {
            //DBG(PrintfUtil_vPrintf("-:%d\n",pdata_hdr->frameSeq);)
            if(card_info_cfg.u8Frame_sum > 0 && pdata_hdr->frameSeq <= card_info_cfg.u8Frame_sum)
            {
                memcpy(flash_info_temp,flash_info,pdata_hdr->frameSeq *CARD_INFO_SIZE);
                memcpy(flash_info_temp + pdata_hdr->frameSeq *CARD_INFO_SIZE,flash_info+(pdata_hdr->frameSeq+1)*CARD_INFO_SIZE,(card_info_cfg.u8Frame_sum - pdata_hdr->frameSeq)*CARD_INFO_SIZE);
                card_info_cfg.u8Frame_sum--;
                for(i=0;i<card_info_cfg.u8Frame_sum;i++)
                {
                    //DBG(PrintfUtil_vPrintf("-1\n");)
                    p = (app_eDev_Data_t*)(flash_info_temp + (i + 1)*CARD_INFO_SIZE);
                    p->frameSum = card_info_cfg.u8Frame_sum;
                    if(p->frameSeq > pdata_hdr->frameSeq)
                        p->frameSeq--;
                }
                bAHI_FlashEraseSector(6);
                bAHI_FullFlashProgram(INFO_STORE_ADDR,(card_info_cfg.u8Frame_sum+1)*CARD_INFO_SIZE,flash_info_temp);
            }
	    ret = 1;
        }
        else if((pdata_hdr->frameSeq > pdata_hdr->frameSum) && (pdata_hdr->frameSeq > card_info_cfg.u8Frame_sum))    // 增加一条数据
        {
            //DBG(PrintfUtil_vPrintf("+%d\n",pdata_hdr->frameSeq);)
            card_info_cfg.u8Frame_sum++;
            memcpy(flash_info + card_info_cfg.u8Frame_sum*CARD_INFO_SIZE,write_buf1,CARD_INFO_SIZE);
            for(i=0;i<card_info_cfg.u8Frame_sum;i++)
            {
                p = (app_eDev_Data_t*)(flash_info + (i + 1)*CARD_INFO_SIZE);
                p->frameSum++;
            }

            //DBG(PrintfUtil_vPrintf("sum %d\n",card_info_cfg.u8Frame_sum);)

            if(card_info_cfg.u8Frame_sum > 126)
            {
                //card_info_cfg.u8Frame_sum--;
                //DBG(PrintfUtil_vPrintf("ccc %d\n",card_info_cfg.u8Frame_sum);)
                memcpy(flash_info_temp,flash_info,1 *CARD_INFO_SIZE);
                memcpy(flash_info_temp + 1 *CARD_INFO_SIZE,flash_info+(1+1)*CARD_INFO_SIZE,(card_info_cfg.u8Frame_sum - 1)*CARD_INFO_SIZE);
                card_info_cfg.u8Frame_sum--;
                for(i=0;i<card_info_cfg.u8Frame_sum;i++)
                {
                    //DBG(PrintfUtil_vPrintf("-1\n");)
                    p = (app_eDev_Data_t*)(flash_info_temp + (i + 1)*CARD_INFO_SIZE);
                    p->frameSum = card_info_cfg.u8Frame_sum;
                    if(p->frameSeq > 1)
                        p->frameSeq--;
                }
                bAHI_FlashEraseSector(6);
                bAHI_FullFlashProgram(INFO_STORE_ADDR,(card_info_cfg.u8Frame_sum+1)*CARD_INFO_SIZE,flash_info_temp);
            }
            else
            {
                bAHI_FlashEraseSector(6);
                bAHI_FullFlashProgram(INFO_STORE_ADDR,(card_info_cfg.u8Frame_sum+1)*CARD_INFO_SIZE,flash_info);
            }
            ret = 1;
        }
        else       //修改某条数据
        {
            //DBG(PrintfUtil_vPrintf("=:%d\n",pdata_hdr->frameSeq);)
            memcpy(flash_info + pdata_hdr->frameSeq*CARD_INFO_SIZE,write_buf1,CARD_INFO_SIZE);
            bAHI_FlashEraseSector(6);
            bAHI_FullFlashProgram(INFO_STORE_ADDR,(card_info_cfg.u8Frame_sum+1)*CARD_INFO_SIZE,flash_info);
            ret = 1;
        }

        //card_info_cfg.u8Frame_sum
    }
    return ret;

}

PRIVATE void vFetchCardVersion(uint8* p,uint8 u8LenVer)
{
    u16CardVersion = OAD_CARD_VERSION;
#if 0
    uint8 i;
    uint32 u32ver = 0;
    for(i=0;i<u8LenVer;i++)
    {
        if((*p <= '9') && (*p >= '0'))
        {
            u32ver = u32ver * 10;
            u32ver += (*p - '0');
        }
        p++;
    }

    PrintfUtil_vPrintf("ve: %d\n",u32ver);

    if(u32ver < 10000)
    {
        u16CardVersion = (u32ver % 1000) * 10;    // 1407
    }
    else if(u32ver > 100000)
    {
        uint32 u32version = 0;

        u32version = (u32ver % 100)/10;
        u32version = u32version*10000;

        //PrintfUtil_vPrintf("u32version: %d\n",u32version);
        u32ver = (u32ver % 100000);

        u32ver = (u32ver /100)*10 + (u32ver%10);   //去掉十位数


        u32version += u32ver;

        //u32version = (u32ver % 10)*10000;   //1407.1RC1
        //u32ver = (u32ver % 100000);
        //u32version += (u32ver / 10);
        u16CardVersion = u32version;

        //PrintfUtil_vPrintf("ver: %d\n",u16CardVersion);

    }
    else if((u32ver >=10000) && (u32ver < 100000))
    {
        u16CardVersion = u32ver % 10000;
    }
    else
    {
        u16CardVersion = 0xFFFF;
        DBG(PrintfUtil_vPrintf("ver error\n"););
    }

    DBG(PrintfUtil_vPrintf("ver %d\n",u16CardVersion););

    //u16CardVersion
#endif
}

PRIVATE void vCheckVddValue(void)
{
    vAHI_DioSetOutput(E_AHI_DIO1_INT,0);          //开始采样

    uint32 u32Vol = SysUtil_u32GetExtVoltage();

    vAHI_DioSetOutput(0,E_AHI_DIO1_INT);         //关闭采样，省电
    // y = 0.0024x - 0.0111
    //u16Battery = (u32Vol*23 + 150)/300 ;   // unit: 0.1v

    u16Battery = ((u32Vol*4*23+1024)*10)/1023 ;   // unit: 0.1v
    DBG(PrintfUtil_vPrintf("Vol: %d,%d\n", u32Vol,u16Battery);)
    if(TOF_VDD_3000 > u16Battery)
    {
        u8LowPowerConf++;
        u8HighPowerConf = 0;
        if((bNoPwd == FALSE) && (u8LowPowerConf > 4))
        {
            u8LowPowerConf = 0;
            bReportNoPwd = FALSE;
            bNoPwd = TRUE;
        }
    }
    else
    {
        u8LowPowerConf = 0;
        u8HighPowerConf++;

        if(u8HighPowerConf > 4)
        {
            u8HighPowerConf = 0;
            bReportNoPwd = TRUE;
            bNoPwd = FALSE;
        }
    }
    if(bNoPwd == FALSE)
    {
        u8CardStatus &= (~CARD_STATUS_NOPWD);
    }
    else
    {
        u8CardStatus |= CARD_STATUS_NOPWD;
    }

}

/*
如果返回true说明获取到了默认配置，否则将需要进行配置才能启动
*/
PRIVATE bool vGet_bootparam(void)
{
    char ch, passwd[5]= "asdf";
    int i = 0, delay, dels = 0; // 10S

    TimerUtil_vInit();

    delay = 9;
    if (!delay)
        delay = 1;
    /* wait passwd */
    while (delay >= 0)
    {
        PrintfUtil_vPrintf("\r");
        PrintfUtil_vPrintf("wait time: %d", delay);
        delay--;
        dels = 1000;

        while ((ch = u8UartRead(E_AHI_UART_0)) != 0)
        {
            if (passwd[i] != ch)
            {
                i = 0;
            }
            else if (i + 1 > 3)
            {
                return FALSE;
            }
            i = ((i + 1) & 3);
        }
        TimerUtil_vDelay(500, E_TIMER_UNIT_MILLISECOND);
        dels--;
    }
    PrintfUtil_vPrintf("\n");
    return TRUE;
}

PRIVATE void vBootsh_show_menu(void)
{
    PrintfUtil_vPrintf("\n******************************************* \n");
    PrintfUtil_vPrintf("*        Startup configuration options       *\n");
    PrintfUtil_vPrintf("*******************************************   \n");
    PrintfUtil_vPrintf("* options:                                   *\n");
    PrintfUtil_vPrintf("*         cardnum=?  (Base short address     *\n");
    PrintfUtil_vPrintf("*         channel=?  (Base boardcast channel)*\n");
    PrintfUtil_vPrintf("*         cardtype=?  (Base 1s、5s、15s)     *\n");
    PrintfUtil_vPrintf("*         rssich=?  (rssi channel)     *\n");
    PrintfUtil_vPrintf("*         saveenv (save configuration)       *\n");
    PrintfUtil_vPrintf("*         show    (show config)              *\n");
    PrintfUtil_vPrintf("*         reboot  (reboot)                   *\n");
    PrintfUtil_vPrintf("*         help    (display this window)      *\n");
    PrintfUtil_vPrintf("*         erase                              *\n");
    PrintfUtil_vPrintf("* example:                                   *\n");
    PrintfUtil_vPrintf("*         bootsh>>cardnum=25555              *\n");
    PrintfUtil_vPrintf("*         bootsh>>channel=25                 *\n");
    PrintfUtil_vPrintf("*         bootsh>>cardtype=0                 *\n");
    PrintfUtil_vPrintf("******************************************   *\n");
}

PRIVATE void _cardnum_process(char *option, const char *val_pos, uint32* pVal)
{
    int id;

    if (!option || !val_pos)
        return;

    if ((id = atoi(val_pos)) < 0 )
        PrintfUtil_vPrintf("%s\b value error\n", option);
    else
    {
        if (((id > 65000) || (id == 0)) && (!strcasecmp(option, OPTION_CARDNUM)))
        {
            PrintfUtil_vPrintf("%s\b value error\n", option);
            return;
        }

        cfg_option.u16ShortAddr= id;

        PrintfUtil_vPrintf("%s\b value ok\n", option);
    }
}

PRIVATE void _channel_process(char *option, const char *val_pos, uint32* pVal)
{
    int id;

    if (!option || !val_pos)
        return;

    if ((id = atoi(val_pos)) < 0)
        PrintfUtil_vPrintf("%s\b value error\n", option);
    else
    {
        if ((id < 11 || id > 26) && !strcasecmp(option, OPTION_CHANNEL))
        {
            PrintfUtil_vPrintf("%s\b value error\n", option);
            return;
        }

        cfg_option.u8Channel = id;
        PrintfUtil_vPrintf("%s\b value ok\n", option);
    }
}

PRIVATE void _cardtype_process(char *option, const char *val_pos, uint32* pVal)
{
    int id;

    if (!option || !val_pos)
        return;

    if ((id = atoi(val_pos)) < 0)
        PrintfUtil_vPrintf("%s\b value error\n", option);
    else
    {
        if (id > 2 && !strcasecmp(option, OPTION_CARDTYPE))
        {
            PrintfUtil_vPrintf("%s\b value error\n", option);
            return;
        }

        cfg_option.u8CardType = id;
        PrintfUtil_vPrintf("%s\b value ok\n", option);
    }
}

PRIVATE void _rssich_process(char *option, const char *val_pos, uint32* pVal)
{
    int id;
    if (!option || !val_pos)
        return;

    if ((id = atoi(val_pos)) < 0)
        PrintfUtil_vPrintf("%s\b value error\n", option);
    else
    {
        if ((id < 11 || id > 26) && !strcasecmp(option, OPTION_RSSICH))
        {
            PrintfUtil_vPrintf("%s\b value error\n", option);
            return;
        }

        cfg_option.u8RssiCh = id;
        PrintfUtil_vPrintf("%s\b value ok\n", option);
    }
}



PRIVATE void _save_configuration(char *option, const char *val_pos, uint32* pVal)
{
    if (!val_pos || *val_pos != ';')
        return;

    cfg_option.u8Status = u8CardStatus;

    bAHI_FlashInit(E_FL_CHIP_INTERNAL, NULL);

    if(!bAHI_FlashEraseSector(7))
    {
        PrintfUtil_vPrintf("Flash Erase Fail\n");
        return;
    }

    if(bAHI_FullFlashProgram(PARAM_STORE_ADDR, sizeof(CFG_OPTION_T),(uint8*)&cfg_option))
    {
        PrintfUtil_vPrintf("finish saving  !\n");

        u16CardShortAddr = cfg_option.u16ShortAddr;
        tofPeriodType = 1;//cfg_option.u8CardType;
        u8BroadcastChannel = cfg_option.u8Channel;
        u8RssiChannel = cfg_option.u8RssiCh;
        return;
    }
    else
    {
        PrintfUtil_vPrintf("Set  CardStatus fail!\n");
        return;
    }
}

void _reboot_process(char *option, const char *val_pos, uint32* pVal)
{
    if (!val_pos || *val_pos != ';')
        return;

    PrintfUtil_vPrintf("reboot station...\n\n");
    vAHI_SwReset();
}

void _show_cfg(char *option, const char *val_pos, uint32 *pVal)
{
    if (!val_pos || *val_pos != ';')
        return;
    PrintfUtil_vPrintf("\ncardnum=%d\n",u16CardShortAddr);
    PrintfUtil_vPrintf("\ncardtype=%d\n",tofPeriodType);
    PrintfUtil_vPrintf("\nchannel=%d\n",u8BroadcastChannel);
    PrintfUtil_vPrintf("\nrssi channel=%d\n",u8RssiChannel);
}

void _help_process(char *option, const char *val_pos, uint32* pVal)
{
    if (!val_pos || *val_pos != ';')
        return;

    vBootsh_show_menu();
}
void _erase_process(char *option, const char *val_pos, uint32* pVal)
{
    if (!val_pos || *val_pos != ';')
        return;
    bAHI_FlashInit(E_FL_CHIP_INTERNAL, NULL);
	bAHI_FlashEraseSector(3);
	bAHI_FlashEraseSector(4);
	bAHI_FlashEraseSector(5);
	bAHI_FlashEraseSector(6);
	bAHI_FlashEraseSector(7);
	PrintfUtil_vPrintf("erase\r\n");
}

PRIVATE void vBootsh_run_line(char* cmd, int cmdlen)
{
    char *pos, *pre;
    int i, cmd_len;
    int cmd_cnt = 0;

    BOOTSH_CMD_T cmd_process[] = {
        {OPTION_CARDNUM, _cardnum_process, NULL},
        {OPTION_CHANNEL, _channel_process, NULL},
        {OPTION_CARDTYPE, _cardtype_process, NULL},
        {OPTION_RSSICH, _rssich_process, NULL},
        {OPTION_SAVEENV, _save_configuration, NULL},
        {OPTION_REBOOT, _reboot_process, NULL},
        {OPTION_ERASE, _erase_process, NULL},
        {OPTION_SHOW, _show_cfg, NULL},
        {OPTION_HELP, _help_process, NULL}
    };

    if (!cmd || !cmdlen)
    {
        return;
    }

    while (*cmd != '\0' && cmdlen > 0)
    {
        if (*cmd == ';' || *cmd == ' ')
        {
            cmd++;cmdlen--;
        }
        else
        {
            break;
        }
    }

    pos = cmd;
    cmd_len = cmdlen;
    cmd_cnt = sizeof(cmd_process)/sizeof(BOOTSH_CMD_T);

    while (cmdlen > 0)
    {
        for (i=0; i<cmd_cnt; i++)
        {
            int option_len = strlen(cmd_process[i].cmd);

            if ((cmdlen >= option_len) && !strncasecmp(pos, cmd_process[i].cmd, option_len))
            {
                // 偏移到值域
                pos += option_len;

                cmd_process[i].pfn(cmd_process[i].cmd, pos, cmd_process[i].pVal);

                // 偏移到下一条命令
                pre = strchr(pos, ';');

                if (pre)
                {
                    pre++;
                    // 去掉上一条命令的长度
                    cmdlen -= pre - pos;
                    pos = pre;
                }
                break;
            }
        }
        if (cmd_len != cmdlen)
        {
            cmd_len = cmdlen;
        }
        else// 无法解析
        {
            if (!pos &&  *pos )
                PrintfUtil_vPrintf("%s command error\n", pos);
            break;
        }
    }
}

PRIVATE bool vGet_flashparam(void)
{
    if(!bAHI_FullFlashRead(PARAM_STORE_ADDR, sizeof(CFG_OPTION_T),(uint8*)&cfg_option))
    {
        PrintfUtil_vPrintf("Flash Read Fail\n");
        return FALSE;
    }


    u16CardShortAddr = cfg_option.u16ShortAddr;
    tofPeriodType = cfg_option.u8CardType;
    u8BroadcastChannel = cfg_option.u8Channel;
    u8RssiChannel = cfg_option.u8RssiCh;
    return TRUE;
}

PRIVATE void vGet_MACAddress(void)
{
    MacUtil_vReadExtAddress(&psMacAddr);
    uint32 tmp32;
    tmp32 = StringUtil_BCD2uint32(psMacAddr.u32H);
    if(u16CardShortAddr > 65000)
    {
		u16CardShortAddr = tmp32%100000;
		vCardShortAddrCheck(u16CardShortAddr,psMacAddr.u32H);
    }
    if(tofPeriodType > 2)
    	tofPeriodType = ((psMacAddr.u32L) >> 24) & 0xFF;
    if(u8BroadcastChannel < 11 || u8BroadcastChannel > 26)
    {
		uint8 channel = ((psMacAddr.u32L) >> 16) & 0xFF;
		if(channel >=11 && channel <=26)
		{
			u8BroadcastChannel = channel;
		}
		else
		{
			u8BroadcastChannel = DEFAULT_CHANNEL_BROADCAST;
		}
    }

    if(u8RssiChannel < 11 || u8RssiChannel > 26)
    {
		uint8 channel = ((psMacAddr.u32L) >> 8) & 0xFF;
		if(channel >=11 && channel <=26)
		{
			u8RssiChannel = channel;
		}
		else
		{
			u8RssiChannel = DEFAULT_CHANNEL_LOCATOR;    //占用辅站频道
		}
    }

}

#if 0
/****************************************************************************
 *
 * NAME: vCardCast
 *
 * DESCRIPTION:
 * 	Card send msg to air
 *
 * PARAMETERS:
 *				u8CmdType - msg type
 *				u16DstAddr / u16DstPanId - destination
 *				u8TxOptions -write open retry or not
 *				u16value - optional value
 *
 * RETURNS:
 * void
 *
 ****************************************************************************/
PRIVATE void vDeviceCardCast( uint16 u16DstAddr, uint16 u16DstPanId, char * sendBuffer,uint32 len)
{
    bMsgSending = TRUE;
    MacUtil_SendParams_s sParams;
    sParams.u8Radius		= 1;
    sParams.u16DstAddr		= u16DstAddr;
    sParams.u16DstPanId 	= u16DstPanId;
    sParams.u16ClusterId 	= 0;
    sParams.u8DeliverMode	= MAC_UTIL_UNICAST;

    //sendBuffer[1] = msgType;

    u8LastHandle = MacUtil_vSendData(&sParams, (uint8*)sendBuffer, len, 0);
}


PRIVATE void ldata_state_init(void)
{

    if(!bAHI_FullFlashRead(PARAM_STORE_ADDR+sizeof(CFG_OPTION_T), sizeof(data_rw_state),(uint8*)&ldata_state))
    {
        PrintfUtil_vPrintf("Flash Read Fail\n");
        return FALSE;
    }
	if(ldata_state.current_write_block==0xFFFFFFFF)
	memset(&ldata_state, 0, sizeof(ldata_state));
}


PRIVATE int dump_location_data( unsigned char* p_data, int data_len)
{
	unsigned int  current_write;
    if ((p_data == NULL)||(data_len == 0))
	return FALSE;
    current_write = ldata_state.current_write_block;
    //PrintfUtil_vPrintf("%c %c %c %c %c %c\r\n",p_data[4],p_data[5],p_data[6],p_data[7],p_data[8],p_data[9]);
	if(ldata_state.block_state[current_write].w_pos + data_len < BLOCK_SIZE )
	{
		do{
		PrintfUtil_vPrintf("write current_write=%d w_pos=%d data_len=%d p_data=%d\r\n",current_write,ldata_state.block_state[current_write].w_pos,data_len,p_data[6]);
		if(!bAHI_FullFlashRead(INFO_STORE_ADDR+current_write*BLOCK_SIZE, ldata_state.block_state[current_write].w_pos, flash_write_buffer));
		{
			PrintfUtil_vPrintf("FlashRead fail\r\n");
		}
		memcpy(flash_write_buffer+ldata_state.block_state[current_write].w_pos,p_data,data_len);
		if(!bAHI_FlashEraseSector(current_write+3))
		{
			PrintfUtil_vPrintf("Flash Erase Fail\n");
		}
		if(!bAHI_FullFlashProgram(INFO_STORE_ADDR+current_write*BLOCK_SIZE, data_len+ldata_state.block_state[current_write].w_pos, flash_write_buffer))
		{
			PrintfUtil_vPrintf("FlashProgram fail\r\n");
		}

		if(!bAHI_FullFlashRead(INFO_STORE_ADDR+current_write*BLOCK_SIZE,data_len+ldata_state.block_state[current_write].w_pos, flash_check_buffer))
		{
			PrintfUtil_vPrintf("FlashRead fail\r\n");
		}
		}while(memcmp(flash_write_buffer,flash_check_buffer,data_len));
		PrintfUtil_vPrintf("Program over\r\n");

		ldata_state.block_state[current_write].w_pos += data_len;
		ldata_state.block_state[current_write].state = 1;
	}
	else
	{
		current_write = (++current_write) % BLOCK_CNT;
		if(current_write == 0)
            PrintfUtil_vPrintf("write full\r\n");
		ldata_state.current_write_block=current_write;
		ldata_state.block_state[current_write].state=0;
		ldata_state.block_state[current_write].w_pos=0;
		ldata_state.block_state[current_write].r_pos=0;
		bAHI_FlashEraseSector(current_write+3);

		bAHI_FullFlashProgram(INFO_STORE_ADDR+current_write*BLOCK_SIZE + ldata_state.block_state[current_write].w_pos, data_len,p_data);
		ldata_state.block_state[current_write].w_pos += data_len;
		ldata_state.block_state[current_write].state = 1;
	}
	do{
		if(!bAHI_FullFlashRead(PARAM_STORE_ADDR,sizeof(CFG_OPTION_T)+sizeof(data_rw_state) , flash_write_buffer));
		{
			PrintfUtil_vPrintf("FlashRead fail\r\n");
		}
		memcpy(flash_write_buffer+sizeof(CFG_OPTION_T),(uint8*)(&ldata_state),sizeof(data_rw_state));
		if(!bAHI_FlashEraseSector(7))
		{
			PrintfUtil_vPrintf("pFlash Erase Fail\n");
		}
		if(!bAHI_FullFlashProgram(PARAM_STORE_ADDR, sizeof(CFG_OPTION_T)+sizeof(data_rw_state), flash_write_buffer))
		{
			PrintfUtil_vPrintf("pFlashProgram fail\r\n");
		}

		if(!bAHI_FullFlashRead(PARAM_STORE_ADDR,sizeof(CFG_OPTION_T)+sizeof(data_rw_state), flash_check_buffer))
		{
			PrintfUtil_vPrintf("pFlashRead fail\r\n");
		}
	}while(memcmp(flash_write_buffer,flash_check_buffer,sizeof(CFG_OPTION_T)+sizeof(data_rw_state)));


    return data_len;
}


PRIVATE int load_location_data(unsigned char* p_buf, int buf_len)
{
    app_header_t* pkt_hdr = NULL;
    unsigned int pkt_hdr_size = 4;
    unsigned int pkt_size = 0;
	int i;
	unsigned int  current_read;

    if ((p_buf == NULL) || (buf_len == 0))
    {
        return FALSE;
    }
	current_read = ldata_state.current_write_block+1;
	current_read %= BLOCK_CNT;

	for(i = 0;i < BLOCK_CNT;i++)
	{
		if(ldata_state.block_state[current_read].state != 0)
		{
			PrintfUtil_vPrintf("current_read=%d\r\n",current_read);

			if(ldata_state.block_state[current_read].r_pos < ldata_state.block_state[current_read].w_pos )
			{
				PrintfUtil_vPrintf("r_pos=%d\r\n",ldata_state.block_state[current_read].r_pos);

				if(!bAHI_FullFlashRead(INFO_STORE_ADDR+current_read*BLOCK_SIZE + ldata_state.block_state[current_read].r_pos, 4, p_buf))
				{
					PrintfUtil_vPrintf("FlashRead fail\r\n");
				}
				pkt_hdr = (app_header_t*)p_buf;
				pkt_size = pkt_hdr_size + pkt_hdr->len;
				PrintfUtil_vPrintf("pkt_hdr_size=%d\r\n",pkt_hdr_size);
				PrintfUtil_vPrintf("pkt_hdr->len=%d\r\n",pkt_hdr->len);
				if(!bAHI_FullFlashRead(INFO_STORE_ADDR+current_read*BLOCK_SIZE + ldata_state.block_state[current_read].r_pos + pkt_hdr_size, pkt_hdr->len,p_buf + pkt_hdr_size))
				{
					PrintfUtil_vPrintf("FlashRead fail\r\n");
				}
				ldata_state.block_state[current_read].r_pos += pkt_size;
				break;
			}
			else
			{
				//ldata_state.block_state[current_read].state = 0;
				//ldata_state.block_state[current_read].w_pos = 0;
				ldata_state.block_state[current_read].r_pos = 0;
			}
		}
		if(current_read == (BLOCK_CNT - 1))
			current_read = 0;
		else
			current_read++;
	}

	if(i == BLOCK_CNT)
		return FALSE;
	else
		return pkt_size;
}
#endif


/*void receiveAck(uint8 sequnum,uint8 ackmsg)
{
	ACK_PACKET_T ackAppPkt;
	ackAppPkt.tof_head.protocoltype = 3;
	ackAppPkt.tof_head.msgtype = 0xe4;
	ackAppPkt.tof_head.len = 2;
	ackAppPkt.sequnum = sequnum;
	ackAppPkt.ackmsg = ackmsg;
	vDeviceCardCast(u16HandStationShortAddr,u16HandStationPanId,(char *)&ackAppPkt,6);
}*/

uint8 calcrc_1byte(uint8 abyte)
{
    uint8 i,crc_1byte;
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


/****************************************************************************/
/***        END OF FILE                                                   ***/
/****************************************************************************/





