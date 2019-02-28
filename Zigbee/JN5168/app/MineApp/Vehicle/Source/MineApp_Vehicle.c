
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
#include "bsmac.h"


/****************************************************************************/
/***        Macro Definitions                                             ***/
/****************************************************************************/

#if (defined DEBUG_CARD)
#define DBG(x) do{x} while(0);
#else
#define DBG(x)
#endif

#define TOF_VDD_3800		380	// 3.8v
#define TOF_VDD_3900		390	// 3.9v
#define TOF_VDD_4000		400	// 4.0v
#define TOF_VDD_4100		410	// 4.1v

#define TOF_OK_PIN			E_AHI_DIO13_INT
#define TOF_HELP_PIN		E_AHI_DIO5_INT
//#define TOF_CHECKIN_PIN	E_AHI_DIO10_INT
//#define TOF_BUZZER_PIN		E_AHI_DIO13_INT
#define TOF_EXCITER_SAD_PIN     E_AHI_DIO17_INT
#define TOF_EXCITER_CLK_PIN     E_AHI_DIO16_INT
#define TOF_MOTION_IT_PIN       E_AHI_DIO0_INT
//#define TOF_VIB_PIN       		E_AHI_DIO5_INT
#define TOF_EXCITER_WAKEUP_PIN  E_AHI_DIO4_INT


#define TOF_BUZZER_TIMER	E_AHI_TIMER_3	// for buzzer's input
#define TOF_SHAKER_TIMER	E_AHI_TIMER_1	// for buzzer's input

#define TOF_OAD_EVENT					BIT(3)	// the card's OAD event

#define TOF_LED_RED_OFF_EVENT			BIT(5)	// the card's red LED off
#define TOF_LED_GREEN_OFF_EVENT		BIT(6)	// the card's green LED off
#define TOF_CHECK_VDD_EVENT         BIT(7)
#define TOF_VDD_ACTION_EVENT        BIT(8)
#define TOF_SLEEP_EVENT				BIT(9)	// sleep
#define TOF_CHECK_HELP_EVENT			BIT(10)	// check if the help button is press or not
#define TOF_HELP_EVENT					BIT(11)	// the card is helping

#define TOF_LISTEN_EVENT				BIT(12)	// card listen: to search station
#define TOF_LISTEN_TIMEOUT_EVENT		BIT(13)	// no station broadcast

#define TOF_CHANGE_CARD_TYPE_EVENT      BIT(14)
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
#define TOF_SPEED_EVENT				BIT(27)	// card buzzer
#define TOF_WARN_EVENT			BIT(28)	// 提示音
#define TOF_RETREAT_EVENT				BIT(29)	// station retreat
#define TOF_WATCHDOG_EVENT			BIT(30)	// feed watch dog event
#define TOF_SELECT_STATION_EVENT    BIT(31)

#define TOF_OK_BUTTON_MS	        3000
#define TOF_HELP_BUTTON_MS			2000	// when press help button for 1s, will trick the help event
#define TOF_CARD_HELP_TIMES			20		// help 20 times (1 second per time)
#define TOF_CARD_CHECKIN_TIMES		4		// card re-checkin 4 times
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
#define CARD_15S_MOTION_CNT           120        //30minutes
#define CARD_5S_MOTION_CNT            360       //2minutes
#define CARD_1S_MOTION_CNT            1800      //2minutes

#define MAX_STATION_STATUS_NORMAL   1
#define PARAM_STORE_ADDR 	0x38000

#define D_VALUE  4
#define DEFF(x,y)   ((x)>=(y)?((x)-(y)):((y)-(x)))

#define OPTION_CARDNUM     "cardnum="
#define OPTION_CHANNEL     "channel="
#define OPTION_CARDTYPE    "cardtype="
#define OPTION_RSSICH      "rssich="
#define OPTION_REBOOT   "reboot"
#define OPTION_SHOW     "show"
#define OPTION_HELP     "help"
#define OPTION_SAVEENV  "saveenv"

#define SELECT_STATION_200    0
#define SELECT_STATION_350    1
#define SELECT_STATION_450    2
#define SELECT_STATION_550    3
#define SELECT_STATION_650    4
#define SELECT_STATION_750    5
#define SELECT_STATION_850    6
#define SELECT_STATION_950    7
#define SELECT_STATION_1050   8


/****************************************************************************/
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
    E_TOF_STATE_CHANGE_PERIOD,      // card wait change work period
    E_TOF_STATE_SELECT_STATION,     //card select station
} teCardState;

typedef enum
{
    CARD_TOF_5S = 0,
    CARD_TOF_1S = 1,
    CARD_TOF_15S = 2,
} teCardTofPeriod;

typedef enum
{
    UART_CMD_HELP = 1,
    UART_CMD_RETREAT = 2,
    UART_CMD_CANCEL_RETREAT = 3,
    UART_CMD_SPEED = 4,
    UART_CMD_HELP_ACK = 5,
    UART_CMD_MOTION = 6,
    UART_CMD_UNMOTION = 7,
    UART_CMD_CARD_NUM = 8,
} teCardUartCmd;


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
    char sync[3];
    uint_8 cmd;
    uint_16 slv_id;
    uint_16 data_len;
    // followed by TLVs
}__PACKED mbus_hdr_t;


typedef struct
{
    char  sync[3];
    uint8 u8cmd1;
    uint8 u8cmd2;
    uint8 u8cmd3;
}teUartCmd;

PRIVATE teUartCmd uartcmd;

PRIVATE uint8 u8UartState;


PRIVATE teMMA845x_Data mma845xData;

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
PRIVATE void vCardMotionDetect();
PRIVATE void vProcessSysEvent();
PRIVATE void vProcessAppEvent();

PRIVATE void vCardCast(uint8 u8CmdType, uint16 u16DstAddr, uint16 u16DstPanId, uint8 u8TxOptions, uint16 u16value);
PRIVATE void vSendRssi();
PRIVATE void vSendAlarm(uint16 u16ShortAddr, uint16 u16PanId);

PRIVATE void vCardSleep(uint32 u32SleepMs);
PRIVATE void vCardLocatorSleep(uint32 u32SleepMs);
PRIVATE void vSetLed(uint32 u32Pin, bool_t bOnOff, uint16 u16OnMs);
PRIVATE void vSetCardChannel(uint8 channel);
PRIVATE void vSyncStation(RfTofWrapper_tu* psAppPkt);
//PRIVATE void vBuzzer(uint32 u32Ms);
//PRIVATE void vStopBuzzer();
PRIVATE void vCheckStationStatus(uint8 u8StationStatus);
PRIVATE void vDioCallBack(uint32 u32Device, uint32 u32ItemBitmap);
PRIVATE bool  vSaveCardStatus(uint8 CardStatus);
PRIVATE void vGetLocatorMs(void);
PRIVATE void vCardTypeCheck(uint8 PeriodType);
PRIVATE void vFetchCardVersion(uint8* p,uint8 u8LenVer);
PRIVATE void vCardShortAddrCheck(uint16 u16Addr,uint32 u32BCD);
PRIVATE void vCheckVddValue(void);
PRIVATE void vInitialBaud(void);

PRIVATE bool vGet_bootparam(void);
PRIVATE void vBootsh_show_menu(void);
PRIVATE void vBootsh_run_line(char* cmd, int cmdlen);
PRIVATE bool vGet_flashparam(void);
PRIVATE void vGet_MACAddress(void);
//PRIVATE void vStartShaker(void);
//PRIVATE void vStopShaker();
//PRIVATE void vGenerateShakeWave(void);
uint8 calcrc_1byte(uint8 abyte);
PRIVATE void vCheckVddAction();
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

PRIVATE uint8 u8StationChannel;		// channel for station do tof with card
PRIVATE uint8 u8LocatorChannel;		// channel for locator do tof with card
PRIVATE uint8 u8BroadcastChannel;	// channel for station to send signal msg
PRIVATE uint8 u8CurChannel;			// card's current channel
PRIVATE uint8 u8RssiChannel;

//PRIVATE uint8 u8RssiChannel = DEFAULT_CHANNEL_RSSI;
//PRIVATE uint8 u8ChannnelPoll = 0;

PRIVATE uint8 u8RedLightFlashNum = 0;

PRIVATE uint32 u32WakeupEvent = TOF_LISTEN_EVENT;	// card wakeup event
PRIVATE uint8 u8NeedLocator = 0;		// if need locator
PRIVATE uint8 u8LocToCardIndex = 0xFF;		// seqnum of TOF with card

PRIVATE uint8 u8VddCnt =0;         //couter for vdd buzzer
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
PRIVATE uint8 u8ExciteTure = FALSE; 					// couter for excite
PRIVATE uint8 u8ExciteProtect = FALSE;

PRIVATE bool_t bMsgSending = FALSE;    // if the msg is still sending or not
PRIVATE uint8 bProtectCnt = 0;
PRIVATE uint32 u32ShakeCnt = 0;
PRIVATE bool_t bVddSync = FALSE;      //check the buzzer ever ranging enought time
PRIVATE uint8 u8KeyNum = 0;


PRIVATE uint8 u8ExciteAddr = 0;
PRIVATE uint8 u8iCount = 0;
PRIVATE uint8 u8LowPowerConf = 0;    //low power confirm
PRIVATE uint8 u8HighPowerConf = 0;    //high power confirm

PRIVATE uint32 u32Timeout = 0;

PRIVATE teCardState eCardTofState;					// card's state
PRIVATE teCardState eCardRssiState = FALSE;					// card's state

PRIVATE uint8 u8CardStatus = CARD_STATUS_NORMAL;	// card's status, should be normal, help, nopwd, retreat
PRIVATE uint8 u8CardStatus1 = 0;
PRIVATE uint8 u8SpeedNum = 0;

PRIVATE uint8 u8HelpAckCnt = 0;


PRIVATE uint8 u8CardMotion = APP_CARD_IS_WITH_ACCEL;
PRIVATE uint8 u8VehcleCard = 0;
PRIVATE uint8 u8LastVehcleCard = 0;
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
PRIVATE uint8 tofPeriodType = CARD_TOF_5S;
PRIVATE uint16 tofSlotNum = TOF_SLOT_NUM;
PRIVATE uint16 tofStationPeriodMs = TOF_STATION_PERIOD_MS;
PRIVATE uint16 u16CardHelpTimeout = CARD_5S_HELP_STATUS_TIMEOUT;
PRIVATE uint16 u16CardMotionTimeout = CARD_5S_MOTION_CNT;

PRIVATE uint16 u16CardMotionCnt = 0;
PRIVATE uint16 u16LastCardMotionCnt = 0;

PRIVATE uint8 u8CardStatusNormal = 0;

PRIVATE uint8 u8HelpCntError = 0;
PRIVATE uint32 u16SendVersionCnt = 0;

PRIVATE uint8 au8UartRxBuffer[100];

PRIVATE uint16 u16CardVersion = 0xFFFF;
PRIVATE uint32 u32BootDelayCnt = 0;


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

PRIVATE uint8 u8SelectStationStatus = 0;
PRIVATE uint16 SelectStation[9]={200,350,450,550,650,750,850,950,1050};
PRIVATE uint8  u8StationLinkQuality;    /**< Link quality of received frame */


PRIVATE BOOTSH_T _bootsh;
PRIVATE BOOTSH_T *bootsh = &_bootsh;


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
        //vStartShaker();
        EventUtil_vSetEvent(TOF_RETREAT_EVENT);
        u8CardStatus |= CARD_STATUS_RETREAT;
    }
    EventUtil_vSetEvent(TOF_LISTEN_EVENT);
    TimerUtil_eSetCircleTimer(TOF_WATCHDOG_EVENT, 1000);	// feed watch dog every 1s
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

	MMA845x_Init();

    As3933_SpiInit();
    config_As3933();


    vAHI_DioSetDirection(0, JN_RED | JN_GREEN);
    vSetLed(JN_GREEN, FALSE, 0); //TRUE, TOF_LED_GREEN_MS);
    vSetLed(JN_RED, FALSE, 0);

    //vAHI_DioSetDirection(0, TOF_VIB_PIN);
    //vAHI_DioSetOutput(TOF_VIB_PIN,0);

    vAHI_DioSetDirection(TOF_HELP_PIN | TOF_OK_PIN, 0);
    vAHI_DioWakeEnable(TOF_HELP_PIN | TOF_OK_PIN, 0);
    vAHI_DioWakeEdge(0, TOF_HELP_PIN | TOF_OK_PIN);

    vAHI_DioSetPullup(TOF_HELP_PIN,0);
    vAHI_DioSetPullup(TOF_OK_PIN,0);


    //vAHI_DioSetPullup(0, TOF_BUZZER_PIN);

	vAHI_DioSetDirection(TOF_EXCITER_WAKEUP_PIN, 0);
	vAHI_DioSetPullup(0,TOF_EXCITER_WAKEUP_PIN);

	vAHI_DioSetDirection(TOF_EXCITER_SAD_PIN,0);
    vAHI_DioSetPullup(0,TOF_EXCITER_SAD_PIN);
	//vAHI_DioInterruptEnable(0,TOF_EXCITER_SAD_PIN);
	//vAHI_DioSetPullup(TOF_EXCITER_SAD_PIN, 0);

	vAHI_DioSetDirection(TOF_EXCITER_CLK_PIN,0);
	vAHI_DioWakeEnable(TOF_EXCITER_CLK_PIN,0);
	vAHI_DioWakeEdge(TOF_EXCITER_CLK_PIN,0);
	//vAHI_DioSetPullup(TOF_EXCITER_CLK_PIN);


	//vAHI_DioInterruptEnable(TOF_EXCITER_CLK_PIN,0);
	//vAHI_DioInterruptEdge(TOF_EXCITER_CLK_PIN,0);
	vAHI_DioSetPullup(0,TOF_EXCITER_CLK_PIN);

    vAHI_DioSetDirection(E_AHI_DIO0_INT,0);
    vAHI_DioSetPullup(0,E_AHI_DIO0_INT);


    vAHI_DioSetDirection(0,E_AHI_DIO1_INT);
    vAHI_DioSetOutput(0,E_AHI_DIO1_INT);

    vAHI_WakeTimerEnable(E_AHI_WAKE_TIMER_0, TRUE);

    //vAHI_UartSetRTSCTS(E_AHI_UART_1,FALSE);     // uart1:2-wire mode,DIO18 is RTS
    vAHI_DioSetDirection(0,E_AHI_DIO10_INT);
    vAHI_DioSetOutput(0,E_AHI_DIO10_INT);
    vInitialBaud();


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

    uartcmd.sync[0] = 'Y';
    uartcmd.sync[1] = 'I';
    uartcmd.sync[2] = 'R';

    u16CardPanId = TOF_CARD_NWK_ADDR;
    vCardTypeCheck(tofPeriodType);

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

PRIVATE void vBsMac_rx_callback(unsigned char *pbuf, unsigned char len)
{

}



/****************************************************************************
 *
 * NAME: vInitialBaud
 *
 * DESCRIPTION:
 * Read mac address, stored Baud from flash and initial Uart Baud
 *
 * PARAMETERS:
 * void
 *
 * RETURNS:
 * void
 *
 ****************************************************************************/

PRIVATE void vInitialBaud(void)
{
    uint8 u8TmpUartBaud = 3;//((psMacAddr.u32L) >> 24) & 0xFF;
    uint8 u8BsmacUartBaud;
    switch(u8TmpUartBaud)
    {
    case 1:       // 500K
        u8BsmacUartBaud = BSMAC_UART_BAUD_500k;
        break;

    case 2:       // 100K
        u8BsmacUartBaud = BSMAC_UART_BAUD_100k;
        break;

    case 3:       // 115200
        u8BsmacUartBaud = BSMAC_UART_BAUD_115200;
        break;

    default:      // 460800
        u8BsmacUartBaud = BSMAC_UART_BAUD_460800;
    }

    //initial both uart 0 and 1
    Jbsmac_u8Init(vBsMac_rx_callback, 1, u8BsmacUartBaud, u16StationPanId, BSMAC_DEVICE_TYPE_LOC);
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
    uint32 battery;

	/* Disable JTAG */
	vAHI_SetJTAGdebugger(FALSE, FALSE);

	/* Wait for 32Khz clock to run */
	while((bAHI_Clock32MHzStable() == FALSE) &&  ++u32Timeout < 10000);

    (void)u32AppQApiInit(NULL, NULL, vDioCallBack);
    (void)u32AHI_Init();

    // need enable card's OK button when wakeup (disable when sleep)
    //vAHI_DioWakeEnable(TOF_OK_PIN, 0);
    //vAHI_DioWakeEdge(0, TOF_OK_PIN);
    vAHI_UartSetRTSCTS(E_AHI_UART_0,FALSE);
    DBG(vAHI_UartSetRTSCTS(E_AHI_UART_0,FALSE);
        PrintfUtil_u8Init(E_AHI_UART_0, PRINTF_BAUD_RATE_1M,au8UartRxBuffer,sizeof(au8UartRxBuffer));
        PrintfUtil_vPrintf("V: %d, Seq: %d\n", OAD_CARD_VERSION, u16SeqNum);)


    vAHI_UartSetRTSCTS(E_AHI_UART_1,FALSE);     // uart1:2-wire mode,DIO18 is RTS
    vInitialBaud();
    u8ProtectCounter = 0;
    u16SeqNum++;
    bHelped = FALSE;
    bHelpSync = FALSE;
    //bMsgSending = FALSE;
    vInitParam();

    vCardDeviceStartup();

    //PrintfUtil_vPrintf("mssss: %d\n",(uint16)((u32AHI_TickTimerRead()/16000)));
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

    //battery = SysUtil_u32GetBatteryVoltage();

    //DBG(PrintfUtil_vPrintf("Bat:%d\n", battery);)

    // LED flash when wakeup
    // if card is wakeup for locator, not need to flash
    if(u32WakeupEvent != TOF_LOCATOR_EVENT && (u8ExciteProtect == FALSE))
    {

        if (bNoPwd)
            vSetLed(JN_RED, TRUE, TOF_LED_RED_MS);
        else
            vSetLed(JN_GREEN, TRUE, TOF_LED_GREEN_MS);
    }
    //////////////////////////////////////////////////
    //vAHI_DioSetPullup(0, TOF_BUZZER_PIN);	// need pulldown this pin to save battery pwd
    //vAHI_DioSetPullup(0, TOF_BUZZER_PIN);
    if(u32WakeStatus & TOF_HELP_PIN)
    {
        // just to wait station coming, not need to consider locator
        TimerUtil_eSetTimer(TOF_CHECK_HELP_EVENT, TOF_HELP_BUTTON_MS);	// from now on, if 1s later the button is still press, HELP!
        eCardTofState = E_TOF_STATE_WAIT_STATION_TOF;
        vSetCardChannel(u8StationChannel);
        TimerUtil_eSetTimer(TOF_LISTEN_EVENT, tofStationPeriodMs);	// if in this period, there's not station coming, it must be lost, need to listen
    }
    else if(u32WakeStatus & TOF_OK_PIN)
    {
        DBG(PrintfUtil_vPrintf("OK\n"););
        u8KeyNum++;
		TimerUtil_eSetTimer(TOF_CHECK_VDD_EVENT, TOF_OK_BUTTON_MS);
    }

	else if((u32WakeStatus & TOF_EXCITER_CLK_PIN))
	{
		//u8ExciteCnt = 0;
		//EventUtil_vSetEvent(TOF_EXCITE_EVENT);

        DBG(PrintfUtil_vPrintf("CLK\n");)
		if(u8LostTimes == 0)
			TimerUtil_eSetTimer(TOF_SYNC_EVENT, TOF_CHECKIN_TIMEOUT+TOF_BUZZER_TIMEOUT);
		else
			TimerUtil_eSetTimer(TOF_LISTEN_EVENT, TOF_CHECKIN_TIMEOUT+TOF_BUZZER_TIMEOUT);
	}
    else if(TOF_STATION_EVENT == u32WakeupEvent)
    {
        //FIXME, for beta only:share E_TOF_STATE_WAIT_STATION_TOF state
        eCardTofState = E_TOF_STATE_WAIT_STATION_TOF;
        eCardRssiState = TRUE;
		vSendRssi();
        //timeout
        TimerUtil_eSetTimer(u32WakeupEvent,15);
    }
#if 0
    else if(u16CardMotionCnt == u16CardMotionTimeout)
    {
        //todo:test
        //vSetCardChannel(u8BroadcastChannel);
        //vCardCast(TOF_CARD_REQUEST, 0x0000, 0xFFFF, 0, u16SeqNum);
        //vSendRssi();
        //TimerUtil_eSetTimer(TOF_SLEEP_EVENT, TOF_RSSI_TIMEOUT+2);
    	EventUtil_vSetEvent(TOF_LISTEN_EVENT);
    }
#endif
    else
    {
        EventUtil_vSetEvent(u32WakeupEvent);
    }

    if(u32WakeupEvent != TOF_LOCATOR_EVENT)
    	vCardMotionDetect();
    TimerUtil_eSetCircleTimer(TOF_WATCHDOG_EVENT, 1000);	// feed watch dog every 1s
    //vStartShaker();
    //vAHI_WatchdogStop();
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
    if((APP_PROTOCOL_TYPE_CARD != psAppPkt->tof_head.protocoltype)
            || (psFrame->u8SduLength != sizeof(app_header_t) + psAppPkt->tof_head.len))
        return;

    switch (psAppPkt->tof_head.msgtype)
    {
    case TOF_CARD_OAD:
    {
        if((psAppPkt->tof_head.len == 4)
                //&& (psAppPkt->rf_tof_oad_data.u8DeviceType == DEVICE_TYPE_CARD15S)
                && (psAppPkt->rf_tof_oad_data.u16Version > OAD_CARD_VERSION))
        {
            TimerUtil_vStopAllTimer();
            EventUtil_vResetAllEvents();

            // when OAD, buzzer & red LED about 2 seconds to indicate user
            //vBuzzer(1800);
            vSetLed(JN_RED, TRUE, 1800);
            TimerUtil_eSetTimer(TOF_OAD_EVENT, 2000);
        }

        break;
    }

    case TOF_STATION_CHECKIN_ACK:	// station received card's checkin, that is, checkin successed.
    {
        DBG(
            PrintfUtil_vPrintf("checkin success\n");
        )
        TimerUtil_eStopTimer(TOF_CHECKIN_EVENT);
        EventUtil_vUnsetEvent(TOF_CHECKIN_EVENT);
        //EventUtil_vSetEvent(TOF_BUZZER_EVENT);	// when checkin successed, buzzer one time
        break;
    }

	case TOF_STATION_EXCITE_ACK:
	{
		DBG(PrintfUtil_vPrintf("excite success\n");)
		u8CardStatus &= ~CARD_STATUS_EXCITER;
		u8ExciteTure = FALSE;

		TimerUtil_eStopTimer(TOF_EXCITE_EVENT);
		EventUtil_vUnsetEvent(TOF_EXCITE_EVENT);
		//EventUtil_vSetEvent(TOF_BUZZER_EVENT);	// when checkin successed, buzzer one time
		break;

	}

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
        //基站在满载的情况下，在卡还没发出RSSI帧前，继续监听，确保卡在收到基站RSSI帧后再休眠
        /*if ((eCardTofState == E_TOF_STATE_LISTEN) && (bRSSIed == FALSE))
        {

            TimerUtil_eStopTimer(TOF_LISTEN_TIMEOUT_EVENT);
            EventUtil_vUnsetEvent(TOF_LISTEN_TIMEOUT_EVENT);

            EventUtil_vSetEvent(TOF_LISTEN_EVENT);

        }*/
        if ((eCardTofState == E_TOF_STATE_SYNC) && (psFrame->sSrcAddr.u16PanId == u16StationPanId) && (psFrame->sSrcAddr.uAddr.u16Short == u16StationShortAddr))
        {
            vSyncStation(psAppPkt);
        }
        //u8ListenIdleTime = 0;

        break;
    }

    case TOF_STATION_RSSI:
    {
        if((eCardTofState == E_TOF_STATE_ALARM) &&
                ((CARD_STATUS_HELP & u8CardStatus) ||
                (CARD_STATUS_NOPWD & u8CardStatus) ||
                (CARD_STATUS_EXCITER & u8CardStatus) ||
                (CARD_STATUS_RETREAT_ACK & u8CardStatus)))
        {
            vSendAlarm(psFrame->sSrcAddr.uAddr.u16Short, psFrame->sSrcAddr.u16PanId);
        }

        if ((eCardTofState == E_TOF_STATE_LISTEN) && (bRSSIed == FALSE))
        {
            eCardTofState = E_TOF_STATE_RSSI;

            uint16 u16Tmp = SysUtil_u16GenRndNum() % ((uint16)(psAppPkt->rf_tof_station_signal.u8AvailableMs));
            if(u16Tmp == 0)
                EventUtil_vSetEvent(TOF_RSSI_EVENT);
            else
                TimerUtil_eSetTimer(TOF_RSSI_EVENT, u16Tmp);
        }
        else if ((eCardTofState == E_TOF_STATE_SYNC) && (psFrame->sSrcAddr.u16PanId == u16StationPanId) && (psFrame->sSrcAddr.uAddr.u16Short == u16StationShortAddr))
        {
            vSyncStation(psAppPkt);
        }

        vCheckStationStatus(psAppPkt->rf_tof_station_signal.u8StationStatus);

        break;
    }

    case TOF_STATION_AVAILABLE:
    {
        uint32 u32MyEvent = 0;
        u8ListenIdleTime = 0;

		if((eCardTofState == E_TOF_STATE_ALARM) &&
                ((CARD_STATUS_HELP & u8CardStatus) ||
                (CARD_STATUS_NOPWD & u8CardStatus) ||
                (CARD_STATUS_NOPWD & u8CardStatus) ||
                (CARD_STATUS_EXCITER & u8CardStatus)))
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

        else if((eCardTofState == E_TOF_STATE_SELECT_STATION) && (u16StationPanId != psFrame->sSrcAddr.u16PanId))
        {
            if(psFrame->u8LinkQuality > u8StationLinkQuality)
            {
                DBG(PrintfUtil_vPrintf("select station %d\n",u16StationPanId);)
                TimerUtil_eStopTimer(TOF_STATION_DONE_EVENT);
                EventUtil_vUnsetEvent(TOF_STATION_DONE_EVENT);
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
        }
        if(u32MyEvent > 0)
        {
            uint16 u16Tmp = SysUtil_u16GenRndNum() % ((uint16)(psAppPkt->rf_tof_station_signal.u8AvailableMs));
            if(u16Tmp == 0)
                EventUtil_vSetEvent(u32MyEvent);
            else
                TimerUtil_eSetTimer(u32MyEvent, u16Tmp);
        }

        vCheckStationStatus(psAppPkt->rf_tof_station_signal.u8StationStatus);

        break;
    }

    case TOF_STATION_FINISH:
    {
        DBG(PrintfUtil_vPrintf("station_fin: %d,%d\n", eCardTofState,u16CardSlot);)
        if((eCardTofState == E_TOF_STATE_WAIT_STATION_TOF) || (eCardTofState == E_TOF_STATE_ALARM))
        {
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
            u8StationLinkQuality = psFrame->u8LinkQuality;

            // if card receive station_finish, need to report alarm status,
            // else, card must send out rssi msg, which include alarm status already
            if((u8CardStatus & (CARD_STATUS_HELP |CARD_STATUS_EXCITER)) ||
               ((u8CardStatus & CARD_STATUS_RETREAT) &&(u8CardStatus & CARD_STATUS_RETREAT_ACK)
                &&(bRetreatAck == TRUE) && (u8RetreatAckCnt < TOF_CARD_RETREAT_ACK_TIMES)))
            {
                EventUtil_vSetEvent(TOF_ALARM_EVENT);
                TimerUtil_eSetTimer(TOF_STATION_DONE_EVENT, TOF_ALARM_TIMEOUT+10);
            }
            else
            {
                //u8CardStatus &= (~CARD_STATUS_RETREAT_ACK);

                DBG(PrintfUtil_vPrintf("dist: %d\n", psAppPkt->rf_tof_station_finish.u16Dist2Station);)
                if(psAppPkt->rf_tof_station_finish.u16Dist2Station > SelectStation[u8SelectStationStatus])
                {
                    u8SelectStationStatus = psAppPkt->rf_tof_station_finish.u16Dist2Station/100 - 1;
                    if(u8SelectStationStatus > SELECT_STATION_1050)
                        u8SelectStationStatus = SELECT_STATION_1050;
                    EventUtil_vSetEvent(TOF_SELECT_STATION_EVENT);
                }
                else
                {
                    EventUtil_vSetEvent(TOF_STATION_DONE_EVENT);
                    if(u8SelectStationStatus>SELECT_STATION_350)
                    {
                        if(psAppPkt->rf_tof_station_finish.u16Dist2Station < SelectStation[u8SelectStationStatus-2])
                            u8SelectStationStatus = u8SelectStationStatus - 1;
                    }
                    else
                    {
                        if(psAppPkt->rf_tof_station_finish.u16Dist2Station < SelectStation[SELECT_STATION_200])
                            u8SelectStationStatus = SELECT_STATION_350;
                    }
                }
            }

            vCheckStationStatus(psAppPkt->rf_tof_station_finish.u8StationStatus);
        }

        break;
    }

    /*
    case TOF_STATION_WAIT:
    {
    	break;
    	if((eCardTofState == E_TOF_STATE_WAIT_STATION_TOF) || (eCardTofState == E_TOF_STATE_ALARM))
    	{
    		TimerUtil_eStopTimer(TOF_SYNC_EVENT);
    		EventUtil_vUnsetEvent(TOF_SYNC_EVENT);

    		// for wakeup from help DIO
    		TimerUtil_eStopTimer(TOF_LISTEN_EVENT);
    		EventUtil_vUnsetEvent(TOF_LISTEN_EVENT);

    		//eCardTofState = E_TOF_STATE_STATION_WAIT;
    		u8StationRunMs 	= psAppPkt->rf_tof_station_finish.u8RunMs;
    		u8NeedLocator	= psAppPkt->rf_tof_station_finish.u8LocN;
    		bTofConnected 	= TRUE;
    		u32StationRecTick = u32AHI_TickTimerRead();
    	}

    	break;
    }
    */

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

                    u8SelectStationStatus = SELECT_STATION_350;
                    u8StationChannel = psAppPkt->rf_tof_station_accept.u8StationChannel;
                    u8LocatorChannel = psAppPkt->rf_tof_station_accept.u8LocatorCardChannel;
                    u16CardSlot = psAppPkt->rf_tof_station_accept.tsAccptData[u8Tmp].u16SlotIndex;

                    u16StationMs = ((u16CardSlot + TOF_SLOT_NUM - psAppPkt->rf_tof_station_accept.u16CurSlot)*TOF_SLOT_MS - (uint16)(psAppPkt->rf_tof_station_accept.u8RunMs)) % tofStationPeriodMs;

                    EventUtil_vSetEvent(TOF_SLEEP_EVENT);

                    DBG(PrintfUtil_vPrintf("accept,%d- slot: %d, run: %d, sleep: %d\n", sizeof(psAppPkt->rf_tof_station_accept.tsAccptData[u8Tmp].u16SlotIndex),u16CardSlot, psAppPkt->rf_tof_station_accept.u8RunMs, u16StationMs);)
                    break;
                }
            }
        }

        break;
    }

    // the station receive card's help
    case TOF_STATION_HELP_ACK:
    {
        //if(u8HelpCnt <=( TOF_CARD_HELP_TIMES-1))
        //{
        //    bHelped = TRUE;
        //}
        break;
    }

    // help success: app received card's help
    case TOF_APP_HELP_ACK:
    {
        /*if(u8HelpCnt <= (TOF_CARD_HELP_TIMES-1))
        {
            bHelped = TRUE;
        }*/

        u8HelpAckCnt = 5;

        u8HelpCnt = TOF_CARD_HELP_TIMES;
        u8CardStatus &= (~CARD_STATUS_HELP);
        vAHI_DioSetOutput(E_AHI_DIO10_INT,0);

        u8UartState = UART_CMD_HELP_ACK;
        TimerUtil_eSetTimer(TOF_WARN_EVENT,60);

        u8CardStatus1 = 1;
        u8SpeedNum = 0;
        //u8Uart_StartTx(E_AHI_UART_1,(uint8*)(&uartcmd),4);
        //EventUtil_vSetEvent(TOF_BUZZER_EVENT);
        break;
    }

    /*case TOF_STATION_NOPWD_ACK:
    {
        //bReportNoPwd = TRUE;
        //u8CardStatus &= (~CARD_STATUS_NOPWD);
        //TimerUtil_eStopTimer(TOF_STATION_DONE_EVENT);
        //EventUtil_vSetEvent(TOF_STATION_DONE_EVENT);
        break;
    }*/

    ///////////////////////////////////////////////////////////
    // locator msg
    case TOF_LOCATOR_FINISH:	// The locator finish TOF
    {
        if(eCardTofState == E_TOF_STATE_WAIT_LOCATOR)
        {
            DBG(
            PrintfUtil_vPrintf("locator finish:%d,%d\n",psFrame->sSrcAddr.uAddr.u16Short,psAppPkt->tof_head.len);)
            eCardTofState = E_TOF_STATE_LOCATOR_FINISH;
            TimerUtil_eStopTimer(TOF_LOCATOR_DONE_EVENT);
            EventUtil_vSetEvent(TOF_LOCATOR_DONE_EVENT);
        }
        break;
    }

    case CHANGE_PERIOD:        //手持机修改卡类型
    {
        if(eCardTofState == E_TOF_STATE_CHANGE_PERIOD)
        {
            vCardCast(CARD_TYPE_CHEACK_ACK,psFrame->sSrcAddr.uAddr.u16Short,psFrame->sSrcAddr.u16PanId,0,0);
            TimerUtil_vDelay(10, E_TIMER_UNIT_MILLISECOND);
            if(cfg_option.u8CardType != psAppPkt->rf_card_type.cardtype)
            {
                cfg_option.u8CardType = psAppPkt->rf_card_type.cardtype;
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

    case HAND_STATION_RSSI_CHANNEL:
    {
        if(eCardTofState == E_TOF_STATE_CHANGE_PERIOD)
        {
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

    case TOF_STATION_RSSI_CHANNEL:
    {
        if(psAppPkt->rf_tof_station_signal.u8AvailableMs != cfg_option.u8RssiCh)
        {
            cfg_option.u8RssiCh =  psAppPkt->rf_tof_station_signal.u8AvailableMs;
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
        break;
    }

    default:
        break;
    }
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
        if(0 == (u32AHI_DioReadInput() & TOF_HELP_PIN))	// the help button is still pressed
        {
            u8CardStatus |= CARD_STATUS_HELP;
            u8HelpCnt = 0;
            u8HelpCntError = 0;
            bHelpSync = TRUE;
            vAHI_DioSetOutput(E_AHI_DIO10_INT,0);
            u8UartState = UART_CMD_HELP;
            TimerUtil_eSetTimer(TOF_WARN_EVENT,60);

            EventUtil_vSetEvent(TOF_SYNC_EVENT);
        }
        EventUtil_vUnsetEvent(u32EventID);
        break;
    }

    case TOF_RETREAT_EVENT:
    {
        vSetLed(JN_RED, TRUE, TOF_LED_RED_MS);
        //vBuzzer(TOF_BUZZER_TIMEOUT);
        vAHI_DioSetOutput(E_AHI_DIO10_INT,0);
        u8UartState = UART_CMD_RETREAT;
        TimerUtil_eSetTimer(TOF_WARN_EVENT,60);
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

    case TOF_SPEED_EVENT:
    {
        vAHI_DioSetOutput(E_AHI_DIO10_INT,0);
        u8UartState = UART_CMD_SPEED;
        TimerUtil_eSetTimer(TOF_WARN_EVENT,60);
        DBG(PrintfUtil_vPrintf("SSS\n");)
        if(u8SpeedNum++ < 4)
        {
            DBG(PrintfUtil_vPrintf("Num:%d\n",u8SpeedNum);)
            TimerUtil_eSetTimer(TOF_SPEED_EVENT, 500);
        }
        else
        {

            u8SpeedNum = 0;
            u8CardStatus1 = 0;
            EventUtil_vSetEvent(TOF_STATION_DONE_EVENT);
            DBG(PrintfUtil_vPrintf("Num:%d\n",u8SpeedNum);)
        }
        EventUtil_vUnsetEvent(u32EventID);
        break;
    }

    /*case TOF_STOP_BUZZER_EVENT:
    {
        //vStopBuzzer();
        EventUtil_vUnsetEvent(u32EventID);
        break;
    }*/
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

        DBG(
            PrintfUtil_vPrintf("checkin event\n");
        )
        vSetCardChannel(u8BroadcastChannel);
        vCardCast(TOF_CARD_CHECKIN, 0x0000, 0xFFFF, 0, u16SeqNum);

        if(++u8CheckinCnt < TOF_CARD_CHECKIN_TIMES)
            TimerUtil_eSetTimer(TOF_CHECKIN_EVENT, 20);	// checkin every 20 ms

        EventUtil_vUnsetEvent(u32EventID);
        break;
    }

	case TOF_EXCITE_EVENT:
	{
		DBG(
					PrintfUtil_vPrintf("excite event:%d\n",u8ExciteCnt);
				)
		vSetCardChannel(u8RssiChannel);
		vCardCast(TOF_CARD_ALARM, 0x0000, 0xFFFF, 0, u16SeqNum);
        if(++u8ExciteCnt < 4)
            TimerUtil_eSetTimer(TOF_EXCITE_EVENT, 5);	// checkin every 20 ms
        else
        {
            u8CardStatus &= ~CARD_STATUS_EXCITER;
            u8ExciteProtect = FALSE;

			//TimerUtil_eSetTimer(TOF_SYNC_EVENT, 20);
			u8ExciteTure = FALSE;
        }
#if 0
		if(++u8ExciteCnt < 3)
		{
            TimerUtil_eSetTimer(TOF_EXCITE_EVENT, 200);
			 // if it's doing TOF, ignore the alarm this time
            if(eCardTofState != E_TOF_STATE_WAIT_STATION_TOF)
                EventUtil_vSetEvent(TOF_ALARM_EVENT);

		}
        else
        {
			u8CardStatus &= ~CARD_STATUS_EXCITER;

			TimerUtil_eSetTimer(TOF_SYNC_EVENT, 20);
			u8ExciteTure = FALSE;
        }
#endif
		EventUtil_vUnsetEvent(u32EventID);
		break;
	}

    case TOF_EXCITE_TIMEOUT_EVENT:
    {
        u8ExciteTure = FALSE;
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
            vAHI_DioSetOutput(E_AHI_DIO10_INT,0);
            u8UartState = UART_CMD_HELP;
            TimerUtil_eSetTimer(TOF_WARN_EVENT,60);
            // if it's doing TOF, ignore the alarm this time
            if(eCardTofState != E_TOF_STATE_WAIT_STATION_TOF)
                EventUtil_vSetEvent(TOF_ALARM_EVENT);

            // if help successed flash green led, else flash red led and send alarm to air
            if((bHelped == FALSE) || (u8HelpCnt+10 <= TOF_CARD_HELP_TIMES))
            {
                //vBuzzer(TOF_BUZZER_TIMEOUT);
                vSetLed(JN_RED, TRUE, 250);
            }
            else
                vSetLed(JN_GREEN, TRUE, TOF_LED_GREEN_MS);


            //vAHI_DioSetOutput(E_AHI_DIO10_INT,0);
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
        vCardCast(TOF_CARD_ALARM, 0x0000, 0xFFFF, 0, u16SeqNum);
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
        break;
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
                (++LQ_StationPanid.u8ListenTimeOutCount > 2) || (u8ListenIdleTime >= 10))
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
        if(LQ_StationPanid.StationPanid == 0 || LQ_StationPanid.u8ListenTimeOutCount == 2)
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

    case TOF_SELECT_STATION_EVENT:
    {
        DBG(PrintfUtil_vPrintf("Select\n");)
        eCardTofState = E_TOF_STATE_SELECT_STATION;
        vSetCardChannel(u8BroadcastChannel);
        TimerUtil_eSetTimer(TOF_STATION_DONE_EVENT, TOF_LISTEN_TIMEOUT);
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
            //vSendRssi();
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

			DBG(PrintfUtil_vPrintf("u8StationRunMs: %d,u32StationRecTick:%d,tick:%d\n", u8StationRunMs,u32StationRecTick,u32AHI_TickTimerRead());

        )
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
        if((0 == ( dio & TOF_HELP_PIN)) || (1 == ( dio & TOF_EXCITER_CLK_PIN))							// the help button is press
                || ((u8CardStatus & CARD_STATUS_RETREAT) && (bRetreatAck == FALSE)) 	// user didn't press OK button when retreat
                || (u8HelpCnt < TOF_CARD_HELP_TIMES) || u8ExciteTure
                 || u8CardStatus1)								// still helping
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

        //bOkToSleep = TRUE;

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

                        vCardLocatorSleep(u16LocatorMs - u16SleepDelta);
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

                        vCardLocatorSleep(u16LocToCardTimes - u16SleepDelta);
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
			if(TOF_STATION_EVENT == u32WakeupEvent)
            {
                vSetCardChannel(u8BroadcastChannel);
            }
			if(u8LostTimes >= TOF_STATION_CARD_MAX_FAILED)
            {
                if(u8ListenIdleTime < 10)
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
                    //u16SeqNum++;
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
        //uint8 buf[10]={'1','2','3','4','5','6','7','8','9','0'};
        //u8Uart_StartTx(E_AHI_UART_1,buf,9);

        //PrintfUtil_vPrintf("w:\n");

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
	case TOF_CHECK_VDD_EVENT:
	{
		if(0 == (u32AHI_DioReadInput() & TOF_OK_PIN))
		{
			bVddSync=FALSE;
			u8VddCnt=0;
			/*if(u8KeyNum >= 3)
			{
				PrintfUtil_vPrintf("VDDC\n");
				u8KeyNum = 0;
				EventUtil_vSetEvent(TOF_VDD_ACTION_EVENT);
			}
			else*/
			{
			    PrintfUtil_vPrintf("TYPE\n");
			    vSetCardChannel(u8BroadcastChannel);
			    eCardTofState = E_TOF_STATE_CHANGE_PERIOD;
			    u8RedLightFlashNum = 0;
			    EventUtil_vSetEvent(TOF_CHANGE_CARD_TYPE_EVENT);
			}
		}
        else
        {
            EventUtil_vSetEvent(TOF_LISTEN_EVENT);
        }

		EventUtil_vUnsetEvent(u32EventID);
		break;
	 }
	 case TOF_VDD_ACTION_EVENT:
	 {
		 //vCheckVddAction();
		 //if(bVddSync ==TRUE)

		 
		 vAHI_DioSetOutput(E_AHI_DIO10_INT,0);
		 u8UartState = UART_CMD_CARD_NUM;
		 TimerUtil_eSetTimer(TOF_WARN_EVENT,60);
		 
		 
//		 	EventUtil_vSetEvent(TOF_LISTEN_EVENT);
		 	//TimerUtil_eSetTimer(TOF_LISTEN_EVENT, TOF_SYNC_TIMEOUT);
		 //else
		 	//EventUtil_vSetEvent(TOF_VDD_ACTION_EVENT);
		 EventUtil_vUnsetEvent(u32EventID);
		 break;
	 }

     case TOF_CHANGE_CARD_TYPE_EVENT:
     {
        if(u8RedLightFlashNum++ <40)
        {
            vSetLed(JN_RED, TRUE, 10);
            TimerUtil_eSetTimer(TOF_CHANGE_CARD_TYPE_EVENT, 500);
        }
        else
        {
            u8RedLightFlashNum = 0;
            eCardTofState = E_TOF_STATE_LISTEN;
            EventUtil_vSetEvent(TOF_LISTEN_EVENT);
        }
        EventUtil_vUnsetEvent(u32EventID);
        break;
     }

     case TOF_WARN_EVENT:
     {
        vAHI_DioSetOutput(0,E_AHI_DIO10_INT);
        uartcmd.u8cmd1 = u8UartState;
		if(UART_CMD_CARD_NUM == u8UartState)
		{
			uartcmd.u8cmd2 = u16CardShortAddr&0xFF;
        	uartcmd.u8cmd3 = (u16CardShortAddr>>8)&0xFF;
		}
		else
		{
	        uartcmd.u8cmd2 = u8UartState;
	        uartcmd.u8cmd3 = u8UartState;
		}
        u8Uart_StartTx(E_AHI_UART_1,(uint8*)(&uartcmd),6);

        u8ExciteTure = FALSE;

        EventUtil_vSetEvent(TOF_LISTEN_EVENT);

        DBG(PrintfUtil_vPrintf("Warm %d\n",u8UartState);)

        u8CardStatus1 = 0;

        //if((u8HelpAckCnt > 0) && (u8UartState == UART_CMD_HELP_ACK))
        if((u8HelpAckCnt > 0))
        {
            u8HelpAckCnt--;
            TimerUtil_eSetTimer(TOF_WARN_EVENT, 20);
        }
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
    case TOF_CARD_RSSI:
    //case TOF_CARD_ALARM:
    case TOF_CARD_CHECKIN:
	//case TOF_CARD_EXCITE:
    {
        RfTofData.rf_tof_card_data.u16SeqNum = u16value;
        RfTofData.rf_tof_card_data.u8CardStatus = u8CardStatus;
        RfTofData.rf_tof_card_data.u8ExciterIDorAccStatus = u8CardMotion;	// this is for test, u8Reserved can be any value

        if(u16SendVersionCnt ==0 && u16Battery>0)   //at most send every 2 minutes
        {
            RfTofData.rf_tof_card_data.u16OadVersion= u16CardVersion;
            RfTofData.rf_tof_card_data.u16Battery= (u16Battery<<5) + u8RssiChannel;
            u16SendVersionCnt = 16;   //16 X 5S

        }
        else
        {
            RfTofData.rf_tof_card_data.u16OadVersion = 0;
            RfTofData.rf_tof_card_data.u16Battery = 0;
        }
        //DBG(PrintfUtil_vPrintf("ver T%d C%d V%x B%d\n",u8CmdType, u16SendVersionCnt, OAD_CARD_VERSION,u16Battery););
        RfTofData.tof_head.len = sizeof(rf_tof_card_data_ts) - sizeof(app_header_t);
        break;
    }
	case TOF_CARD_ALARM:
	//case TOF_CARD_EXCITE:
	{
		RfTofData.rf_tof_card_data.u16SeqNum = u16value;
		RfTofData.rf_tof_card_data.u8CardStatus = u8CardStatus;
		RfTofData.rf_tof_card_data.u8ExciterIDorAccStatus = u8ExciteAddr;	// this is for test, u8Reserved can be any value

		RfTofData.rf_tof_card_data.u16OadVersion= u16CardVersion;
		RfTofData.rf_tof_card_data.u16Battery= u16Battery;

		//DBG(PrintfUtil_vPrintf("ver T%d C%d V%x B%d\n",u8CmdType, u16SendVersionCnt, OAD_CARD_VERSION,u16Battery););
		RfTofData.tof_head.len = sizeof(rf_tof_card_data_ts) - sizeof(app_header_t);
		break;
	}

    case CARD_TYPE_CHEACK_ACK:
    case HAND_STATION_RSSI_CHANNEL_ACK:
    {
        RfTofData.tof_head.len = 0;
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
        vSetCardChannel(u8RssiChannel);
        vCardCast(TOF_CARD_RSSI, 0x0000, 0xFFFF, 0, u16SeqNum);
        bRSSIed = TRUE;
    }
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
    vCardCast(TOF_CARD_ALARM, u16ShortAddr, u16PanId, MAC_TX_OPTION_ACK, u16SeqNum);
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
    if(u8StationStatus == STATION_STATUS_RETREAT)
    {
        u8CardStatusNormal = 0;
        if(!(u8CardStatus & CARD_STATUS_RETREAT))
        {
            EventUtil_vSetEvent(TOF_RETREAT_EVENT);
            vAHI_DioSetOutput(E_AHI_DIO10_INT,0);
            u8UartState = UART_CMD_RETREAT;

            //TimerUtil_eSetTimer(TOF_CHECK_HELP_EVENT, TOF_HELP_BUTTON_MS);
            TimerUtil_eSetTimer(TOF_WARN_EVENT,60);
            u8CardStatus |= CARD_STATUS_RETREAT;
            bRetreatAck = FALSE;
            vSaveCardStatus(u8CardStatus);
        }
    }
    else if(u8StationStatus == STATION_STATUS_NORMAL)
    {
        if((u8CardStatus & CARD_STATUS_RETREAT) && (u8CardStatusNormal++ > MAX_STATION_STATUS_NORMAL))
        {
            u8CardStatusNormal = MAX_STATION_STATUS_NORMAL+1;
            u8CardStatus &= (~(CARD_STATUS_RETREAT|CARD_STATUS_RETREAT_ACK));
            TimerUtil_eStopTimer(TOF_RETREAT_EVENT);
            vAHI_DioSetOutput(E_AHI_DIO10_INT,0);
            u8UartState = UART_CMD_CANCEL_RETREAT;
            TimerUtil_eSetTimer(TOF_WARN_EVENT,60);
            EventUtil_vUnsetEvent(TOF_RETREAT_EVENT);
            //vAHI_DioSetOutput(E_AHI_DIO10_INT,0);
            //vStopShaker();
            vSaveCardStatus(u8CardStatus);
        }
        //bRetreatAck = FALSE;
    }
    else if(u8StationStatus == STATION_STATUS_SPEED)
    {
        DBG(PrintfUtil_vPrintf("Speed\n");)
        vAHI_DioSetOutput(E_AHI_DIO10_INT,0);
        EventUtil_vSetEvent(TOF_SPEED_EVENT);
        u8CardStatus1 = 1;
        u8SpeedNum = 0;
    }
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
/*PRIVATE void vBuzzer(uint32 u32Ms)
{
    //TimerUtil_eSetTimer(TOF_STOP_BUZZER_EVENT, u32Ms);
    vAHI_TimerDIOControl(TOF_BUZZER_TIMER, TRUE);
    vAHI_TimerEnable(TOF_BUZZER_TIMER, 12, FALSE, FALSE, TRUE);
	vAHI_TimerConfigureOutputs(TOF_BUZZER_TIMER,TRUE,TRUE);
    vAHI_TimerClockSelect(TOF_BUZZER_TIMER, FALSE, TRUE);
    vAHI_TimerStartRepeat(TOF_BUZZER_TIMER, 1, 2);
    u8ProtectCounter = 0;
}*/
/*PRIVATE void vBuzzer1(uint32 u32Ms,uint32 k)
{
    //TimerUtil_eSetTimer(TOF_STOP_BUZZER_EVENT, u32Ms);
    vAHI_TimerDIOControl(TOF_BUZZER_TIMER, TRUE);
    vAHI_TimerEnable(TOF_BUZZER_TIMER, 12, FALSE, FALSE, TRUE);
	vAHI_TimerConfigureOutputs(TOF_BUZZER_TIMER,TRUE,TRUE);
    vAHI_TimerClockSelect(TOF_BUZZER_TIMER, FALSE, TRUE);
    vAHI_TimerStartRepeat(TOF_BUZZER_TIMER, 1, k);
    u8ProtectCounter = 0;
}*/

/****************************************************************************
 *
 * NAME: vShaker
 *
 * DESCRIPTION:
 * 	Start shake and last for u32Ms
 *
 * PARAMETERS:
 *				u32Ms - shake ms
 *
 * RETURNS:
 * void
 *
 ****************************************************************************/
/*PRIVATE void vStartShaker(void)
{
	vAHI_TimerEnable(TOF_SHAKER_TIMER, 14, TRUE, TRUE, FALSE);
    vAHI_TimerClockSelect(TOF_SHAKER_TIMER, FALSE, TRUE);
    vAHI_TimerStartRepeat(TOF_SHAKER_TIMER, 3000, 1000);
    vAHI_Timer1RegisterCallback((PR_HWINT_APPCALLBACK)vGenerateShakeWave);
}*/

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
/*PRIVATE void vStopBuzzer()
{
    //vAHI_DioSetPullup(0, TOF_BUZZER_PIN);
    //vAHI_TimerDisable(TOF_BUZZER_TIMER);

    //vAHI_DioSetDirection(0,TOF_BUZZER_PIN);
    //vAHI_DioSetOutput(0,TOF_BUZZER_PIN);
}*/
/****************************************************************************
 *
 * NAME: vStopShaker
 *
 * DESCRIPTION:
 * 	stop the shaker
 *
 * PARAMETERS: 	None
 *
 * RETURNS:
 * void
 *
 ****************************************************************************/
/*
PRIVATE void vStopShaker()
{

    //vAHI_DioSetPullup(0, TOF_VIB_PIN);
    //vAHI_TimerDisable(TOF_SHAKER_TIMER);

    //vAHI_DioSetDirection(0,TOF_VIB_PIN);
    //vAHI_DioSetOutput(TOF_VIB_PIN,0);
}*/

PRIVATE void vCardMotionDetect()
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
            u16CardMotionCnt = 0;
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
	mma845xData.u8XbyteH = MMA845x_ReadReg(0x01);
	mma845xData.u8YbyteH = MMA845x_ReadReg(0x03);
	//mma845xData.u8ZbyteH = MMA845x_ReadReg(0x05);

    //DBG(PrintfUtil_vPrintf("FFM %d,Trans %d\n",mma845xData.u8FFMtSrc,mma845xData.u8TransientSrc);)

	PlStauts = MMA845x_ReadReg(0x10);   //PL Status Register

	if((DEFF(mma845xData.u8XbyteH,vmma845xData.u8XbyteH) > D_VALUE)
		|| (DEFF(mma845xData.u8YbyteH,vmma845xData.u8YbyteH) > D_VALUE))
		//|| (DEFF(mma845xData.u8ZbyteH,vmma845xData.u8ZbyteH) > D_VALUE))
	{
		u8CardMotion |= APP_CARD_MOTION;
        //PrintfUtil_vPrintf("MO1:%d,%d\n",mma845xData.u8XbyteH,vmma845xData.u8XbyteH);
	}
	//else if((mma845xData.u8TransientSrc & 0x40) || (mma845xData.u8FFMtSrc & 0x80))
    else if((mma845xData.u8TransientSrc & 0x40) || (mma845xData.u8FFMtSrc & 0x80))
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
    if(u16CardMotionCnt < u16CardMotionTimeout)
    {
        u8VehcleCard = 1;    //动
    }
    else
    {
        u8VehcleCard = 0;    //静
    }

#if 0
    if(u8LastVehcleCard != u8VehcleCard)
    {
        if(u8VehcleCard == 0)
        {
            vAHI_DioSetOutput(E_AHI_DIO10_INT,0);
            //DBG(PrintfUtil_vPrintf("JIng\n");)
            u8UartState = UART_CMD_UNMOTION;

            //u8ExciteTure = TRUE;
            u8CardStatus1 = 1;
            u8HelpAckCnt = 5;
            TimerUtil_eSetTimer(TOF_WARN_EVENT,60);
        }
        else
        {
            vAHI_DioSetOutput(E_AHI_DIO10_INT,0);
            //DBG(PrintfUtil_vPrintf("dong\n");)
            u8UartState = UART_CMD_MOTION;
            //u8ExciteTure = TRUE;
            u8CardStatus1 = 1;
            u8HelpAckCnt = 5;
            TimerUtil_eSetTimer(TOF_WARN_EVENT,60);
        }
    }
    u8LastVehcleCard = u8VehcleCard;
#endif

    ////u16LastCardMotionCnt = u16CardMotionCnt;


	DBG(PrintfUtil_vPrintf("Card PLStatus %d,%d\n",u8CardMotion,u16CardMotionCnt);)
}

PRIVATE void vProcessExcite()
{
	static uint16 u16HeadMs;
	static uint16 u16TailMs;
	static uint16 u16ExciterID = 0;
	uint8 u8HighExciterID;
	uint8 u8LowExciterID;

	uint16 CycleLenth;
	u16HeadMs = (uint16)(u32AHI_TickTimerRead()/1600);

	if(u8iCount == 0)
	{
		u8iCount++;
		u16TailMs = u16HeadMs;
		u16ExciterID = 0;
		CycleLenth = 0;
        //DBG(PrintfUtil_vPrintf("O\n"););
	}
	else
	{
		CycleLenth = u16HeadMs - u16TailMs;
		u16TailMs = u16HeadMs;
		//DBG(PrintfUtil_vPrintf("S2:%d\n",CycleLenth););
	}
	//PrintfUtil_vPrintf("CycleLenth=%d!\n",CycleLenth);
	if(CycleLenth > 20)
	{
		u8iCount = 1;
		u16TailMs = u16HeadMs;
		u16ExciterID = 0;
		//DBG(PrintfUtil_vPrintf("S12\n"););
	}

	//uint32 u32SDAStatus = u32AHI_DioWakeStatus();
	uint32 u32SDAdio = u32AHI_DioReadInput();
	if(u32SDAdio & TOF_EXCITER_SAD_PIN)
	{
		u16ExciterID |= 1<<(16-u8iCount);
        //DBG(PrintfUtil_vPrintf("set:%d\n",u8iCount););
	}

	u8iCount++;

	//DBG(PrintfUtil_vPrintf("C:%d\n",u8iCount););
	if(u8iCount == 17)
	{
		u8iCount = 0;
		u8HighExciterID = (uint8)((u16ExciterID>>8)&0xff);
		u8LowExciterID = (uint8)(u16ExciterID&0xff);
		//if(calcrc_1byte(u8HighExciterID)!=u8LowExciterID || (u16ExciterID==0))
		if(calcrc_1byte(u8HighExciterID)!=u8LowExciterID || (u16ExciterID==0) || u8HighExciterID!=u8ExciteAddr)
		{
			u8ExciteAddr = u8HighExciterID;
			EventUtil_vSetEvent(TOF_SLEEP_EVENT);
			return;
		}
		//DBG(PrintfUtil_vPrintf("ID0:%d\n",(uint8)(u16ExciterID>>8)););
		u8ExciteAddr = u8HighExciterID;
		u16ExciterID = 0;
		u8ExciteCnt = 0;
		u8CardStatus |= CARD_STATUS_EXCITER;
		u8ExciteTure = TRUE;
		EventUtil_vSetEvent(TOF_EXCITE_EVENT);

		//EventUtil_vSetEvent(TOF_BUZZER_EVENT);	// when excite successed, buzzer one time
		vSetLed(JN_RED, TRUE, TOF_LED_RED_MS);
	}
}




PRIVATE void vDioCallBack(uint32 u32Device, uint32 u32ItemBitmap)
{
    if(u32Device == E_AHI_DEVICE_SYSCTRL)
    {
        if((bHelped == FALSE) && (u8HelpCnt >= TOF_CARD_HELP_TIMES) && (TOF_HELP_PIN & u32ItemBitmap))
        {
            DBG(
                PrintfUtil_vPrintf("help DIO\n");
            );
            TimerUtil_eSetTimer(TOF_CHECK_HELP_EVENT, TOF_HELP_BUTTON_MS);
        }
        else if (TOF_OK_PIN & u32ItemBitmap)
        {
            DBG(PrintfUtil_vPrintf("OK DIO\n"););
            u8KeyNum++;
			TimerUtil_eSetTimer(TOF_CHECK_VDD_EVENT, TOF_OK_BUTTON_MS);

            if((u8CardStatus & CARD_STATUS_RETREAT) && bRetreatAck == FALSE)
            {
                u8RetreatAckCnt = 0;
                u8CardStatus |= CARD_STATUS_RETREAT_ACK;
            }
			else
			{
				if(u8KeyNum >= 3)
				{
					PrintfUtil_vPrintf("VDDC\n");
					u8KeyNum = 0;
					EventUtil_vSetEvent(TOF_VDD_ACTION_EVENT);
				}
			}
				//EventUtil_vSetEvent(TOF_CHECK_VDD_EVENT);
            //vStopShaker();
            bRetreatAck = TRUE;
			//TimerUtil_eSetTimer(TOF_LISTEN_EVENT, 50);
        }

		else if(TOF_EXCITER_CLK_PIN & u32ItemBitmap)
		{
            //if(eCardTofState != E_TOF_STATE_WAIT_STATION_TOF)
            vProcessExcite();
			u8ProtectCounter = 0;
            u8ExciteProtect = TRUE;

            TimerUtil_eStopTimer(TOF_EXCITE_TIMEOUT_EVENT);	// if request timeout, listen again
            EventUtil_vUnsetEvent(TOF_EXCITE_TIMEOUT_EVENT);
            TimerUtil_eSetTimer(TOF_EXCITE_TIMEOUT_EVENT,1500);
		}

        else if(TOF_MOTION_IT_PIN & u32ItemBitmap)
        {
            u16CardMotionCnt = 0;
            DBG(PrintfUtil_vPrintf("MOTION DIO\n");)
        }
    }
}

PRIVATE void vCardLocatorSleep(uint32 u32SleepMs)
{
    vSetLed(JN_GREEN, FALSE, 0);
    vSetLed(JN_RED, FALSE, 0);
    u32Timeout = 0;
	u8iCount = 0;
    u8ExciteProtect = FALSE;
    u8KeyNum = 0;
    //vAHI_DioSetPullup(0, TOF_BUZZER_PIN);
    //vAHI_TimerDisable(TOF_BUZZER_TIMER);
    //vAHI_DioSetDirection(0,TOF_BUZZER_PIN);
    //vAHI_DioSetOutput(0,TOF_BUZZER_PIN);

    vAHI_UartDisable(E_AHI_UART_0);
//	 vAHI_DioWakeEnable(0, TOF_OK_PIN);  // need disable OK button's wakeup ability when sleep
    SleepUtil_Sleep(E_AHI_WAKE_TIMER_0, u32SleepMs, E_AHI_SLEEP_OSCON_RAMON);
}

PRIVATE void vCardSleep(uint32 u32SleepMs)
{
    if(((eCardTofState==E_TOF_STATE_STATION_FINISH)&&(u16CardMotionCnt == u16CardMotionTimeout))||((u16CardMotionCnt == u16CardMotionTimeout)&& (FALSE == bTofConnected)))
    {

    	if(u8NeedLocator > 0)
		u16SeqNum++;

        u32SleepMs = 120000;
        u32WakeupEvent = TOF_LISTEN_EVENT;
        u16StationConnectPrt = 0;
        vInitParam();
        u8NeedLocator = 0;
        vAHI_DioSetDirection(TOF_MOTION_IT_PIN, 0);
        vAHI_DioWakeEnable(TOF_MOTION_IT_PIN, 0);
        vAHI_DioWakeEdge(TOF_MOTION_IT_PIN, 0);
    }

    vAHI_DioSetOutput(0,E_AHI_DIO10_INT);
    //vAHI_DioSetOutput(0,E_AHI_DIO10_INT);
    vSetLed(JN_GREEN, FALSE, 0);
    vSetLed(JN_RED, FALSE, 0);
    u32Timeout = 0;
	u8iCount = 0;
    u8ExciteProtect = FALSE;
    u8KeyNum = 0;
    //vAHI_DioSetPullup(0, TOF_BUZZER_PIN);
    //vAHI_TimerDisable(TOF_BUZZER_TIMER);
    //vAHI_DioSetDirection(0,TOF_BUZZER_PIN);
    //vAHI_DioSetOutput(0,TOF_BUZZER_PIN);

    vAHI_UartDisable(E_AHI_UART_0);
    vAHI_UartDisable(E_AHI_UART_1);
    vAHI_DioSetDirection(0,E_AHI_DIO14_INT);
    vAHI_DioSetDirection(0,E_AHI_DIO15_INT);

    vAHI_DioSetOutput(0,E_AHI_DIO14_INT);
    vAHI_DioSetOutput(E_AHI_DIO15_INT,0);


//	 vAHI_DioWakeEnable(0, TOF_OK_PIN);  // need disable OK button's wakeup ability when sleep
    SleepUtil_Sleep(E_AHI_WAKE_TIMER_0, u32SleepMs, E_AHI_SLEEP_OSCON_RAMON);
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

    //PrintfUtil_vPrintf("ve: %d\n",u32ver);

    if(u32ver < 10000)
    {
        u16CardVersion = (u32ver % 1000) * 10;    // 1407
    }
    else if(u32ver > 100000)
    {
        uint32 u32version = 0;
        u32version = (u32ver % 10)*10000;   //1407_RC1.1
        u32ver = (u32ver % 100000);
        u32version += (u32ver / 10);
        u16CardVersion = u32version;
    }
    else if((u32ver >=10000) && (u32ver < 100000))
    {
        u16CardVersion = u32ver % 10000;
    }
    else
    {
        DBG(PrintfUtil_vPrintf("ver error\n"););
    }

    //u16CardVersion
#endif

}

PRIVATE void vCheckVddValue(void)
{

	//if((u8CardStatus == CARD_STATUS_NORMAL) && ((u16SeqNum & 0x000F) == 0))
    //if((u16SeqNum & 0x000F) == 0) // sample every 16*5 seconds
    {
        vAHI_DioSetOutput(E_AHI_DIO1_INT,0);          //开始采样

        uint32 u32Vol = SysUtil_u32GetExtVoltage();

        vAHI_DioSetOutput(0,E_AHI_DIO1_INT);         //关闭采样，省电
        // y = 0.0024x - 0.0111
        //u16Battery = (u32Vol*23 + 150)/300 ;   // unit: 0.1v

        u16Battery = ((u32Vol*4*23+1024)*10)/1023 ;   // unit: 0.1v
        DBG(
            PrintfUtil_vPrintf("Vol: %d,%d\n", u32Vol,u16Battery);
        )
        if(TOF_VDD_3800 > u16Battery)
        {
            u8LowPowerConf++;
            u8HighPowerConf = 0;
            if((bNoPwd == FALSE) && (u8LowPowerConf > 2))
            {
                u8LowPowerConf = 0;
                bReportNoPwd = FALSE;
                bNoPwd = TRUE;
            }
        }

        if(TOF_VDD_3900 <= u16Battery)
        {
            u8LowPowerConf = 0;
            u8HighPowerConf++;

            if(u8HighPowerConf > 2)
            {
                u8HighPowerConf = 0;
                bReportNoPwd = TRUE;
                bNoPwd = FALSE;
            }
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


PRIVATE void vCheckVddAction(void)
{
//     PrintfUtil_vPrintf("-------OK-----u16Battery =%d\n",u16Battery);
	if(TOF_VDD_3800 > u16Battery)
	{
		DBG(PrintfUtil_vPrintf("--------belowe than 3.8-----\n");)
		bVddSync = TRUE;
	}
	else if(TOF_VDD_3900 > u16Battery)
	{
		//vBuzzer1(TOF_BUZZER_TIMEOUT,2);
		vSetLed(JN_RED, TRUE, TOF_LED_RED_MS);
		bVddSync = TRUE;
	}
	else if(TOF_VDD_4000 > u16Battery)
	{
		if(u8VddCnt++ < 2)
		{
			TimerUtil_eSetTimer(TOF_VDD_ACTION_EVENT, TOF_CARD_HELP_PERIOD_MS);
			//vBuzzer1(TOF_BUZZER_TIMEOUT,5);
			vSetLed(JN_RED, TRUE, TOF_LED_RED_MS);
		}
		if(u8VddCnt ==2)
			bVddSync = TRUE;
	}
	else if(TOF_VDD_4100 > u16Battery)
	{
		if(u8VddCnt++ < 3)
		{
			TimerUtil_eSetTimer(TOF_VDD_ACTION_EVENT, TOF_CARD_HELP_PERIOD_MS);
			//vBuzzer1(TOF_BUZZER_TIMEOUT,4);
			vSetLed(JN_RED, TRUE, TOF_LED_RED_MS);
		}
		if(u8VddCnt ==3)
			bVddSync = TRUE;
	}
	else
	{
		if(u8VddCnt++ < 4)
		{
			TimerUtil_eSetTimer(TOF_VDD_ACTION_EVENT, TOF_CARD_HELP_PERIOD_MS);
			//vBuzzer1(TOF_BUZZER_TIMEOUT,3);
			vSetLed(JN_RED, TRUE, TOF_LED_RED_MS);
	}
	if(u8VddCnt ==4)
		bVddSync = TRUE;
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
        tofPeriodType = cfg_option.u8CardType;
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
//void vGenerateShakeWave(void)
//{
	//if(u32ShakeCnt%2)
	//vAHI_DioSetOutput(0,TOF_VIB_PIN);
	//else
	//vAHI_DioSetOutput(TOF_VIB_PIN,0);
	//u32ShakeCnt++;
//}
/********************************************************/
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





