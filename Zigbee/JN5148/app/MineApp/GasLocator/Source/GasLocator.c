
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

#include "config.h"
#include "app_protocol.h"
#include "JN5148_util.h"
#include "printf_util.h"
#include "system_util.h"

#include "Locate_protocol.h"
#include "gas_uart.h"
#include "timer_util.h"
/****************************************************************************/
/***        Macro Definitions                                             ***/
/****************************************************************************/

#if (defined DEBUG_GASLOCATOR)
#define DBG(x) do{x} while(0);
#else
#define DBG(x)
#endif

#define CONVERT_ENDIAN(x)	SysUtil_vConvertEndian(&(x), sizeof(x))

#define TOF_VDD_3600		1547	// 3.6v
#define TOF_VDD_3800		1630	// 3.8v

#define TOF_OK_PIN			E_AHI_DIO15_INT
#define TOF_HELP_PIN		E_AHI_DIO14_INT
#define TOF_CHECKIN_PIN	E_AHI_DIO10_INT
#define TOF_BUZZER_PIN		E_AHI_DIO13_INT

#define TOF_BUZZER_TIMER	E_AHI_TIMER_1	// for buzzer's input

#define TOF_OAD_EVENT					BIT(3)	// the card's OAD event

#define TOF_SYNCADDR_EVENT			BIT(4)	// sync address after restart

#define TOF_LED_RED_OFF_EVENT		BIT(5)	// the card's red LED off
#define TOF_LED_GREEN_OFF_EVENT		BIT(6)	// the card's green LED off
#define TOF_CHECK_VDD_EVENT			BIT(7)	// check battery
#define TOF_SLEEP_EVENT				BIT(9)	// sleep
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
#define TOF_SYNC_EVENT					BIT(25)	// card change to common channel to sync with station
#define TOF_CHECKIN_EVENT				BIT(26)	// card checkin
#define TOF_BUZZER_EVENT				BIT(27)	// card buzzer
#define TOF_STOP_BUZZER_EVENT			BIT(28)	// stop buzzer
#define TOF_RETREAT_EVENT				BIT(29)	// station retreat
#define TOF_WATCHDOG_EVENT			BIT(30)	// feed watch dog event

#define TOF_HELP_BUTTON_MS			1000	// when press help button for 1s, will trick the help event
#define TOF_CARD_HELP_TIMES			20		// help 10 times (1 second per time)
#define TOF_CARD_CHECKIN_TIMES		4		// card re-checkin 4 times

#define TOF_CARD_SYNC_DELTA_MAX		40		// card wakeup at most TOF_CARD_SYNC_DELTA_MAX ms before station do TOF
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
#define TOF_PENDING_TIMEOUT			(20+TOF_CARD_SYNC_DELTA_MAX)		// timeout: waiting for station to do tof
#define TOF_REQUEST_TIMEOUT			20					// send request to join's timeout
#define TOF_BUZZER_TIMEOUT			200					// buzzer timeout


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
} teCardState;


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
PRIVATE void vProcessSysEvent();
PRIVATE void vProcessAppEvent();

PRIVATE void vCardCast(uint8 u8CmdType, uint16 u16DstAddr, uint16 u16DstPanId, uint8 u8TxOptions, uint16 u16value);
PRIVATE void vSendRssi();
PRIVATE void vSendAlarm(uint16 u16ShortAddr, uint16 u16PanId);

PRIVATE void vCardSleep(uint16 u16SleepMs);
PRIVATE void vSetLed(uint32 u32Pin, bool_t bOnOff, uint16 u16OnMs);
PRIVATE void vSetCardChannel(uint8 channel);
PRIVATE void vSyncStation(RfTofWrapper_tu* psAppPkt);
PRIVATE void vBuzzer(uint32 u32Ms);
PRIVATE void vStopBuzzer();
PRIVATE void vCheckStationStatus(uint8 u8StationStatus);
PRIVATE void vDioCallBack(uint32 u32Device, uint32 u32ItemBitmap);

PRIVATE void vReportTofDistance(void);
PRIVATE void vUart1_txCallBack(void);
PRIVATE void vUart1_rxCallBack(uint8* p, uint16 len);

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

PRIVATE uint32 u32WakeupEvent = TOF_LISTEN_EVENT;	// card wakeup event
PRIVATE uint8 u8NeedLocator = 0;		// if need locator

PRIVATE uint8 u16CardSlot;			// the station slot index for me, use this to quickly sync with station if need
PRIVATE uint16 u16SeqNum = 0;		// card's sequence number
PRIVATE uint8 u8LostTimes = TOF_STATION_CARD_MAX_FAILED;	// to record the times that card lost station

PRIVATE bool_t bHelpSync = FALSE;			// is the sync is from card help event
PRIVATE bool_t bTofConnected = FALSE;		// connect with station or not
PRIVATE bool_t bRSSIed = FALSE;			// weather card send rssi msg during the sleep period
PRIVATE bool_t bReportNoPwd = FALSE;		// report no power success or not
PRIVATE bool_t bNoPwd =  FALSE;			// no power or not
PRIVATE bool_t bRetreatAck = FALSE;		// if station retreat, use need to press OK button to stop buzzer
PRIVATE bool_t bHelped = FALSE;			// station receive card's help or not

PRIVATE uint8 u8HelpCnt = TOF_CARD_HELP_TIMES;	// couter for help times
PRIVATE uint8 u8CheckinCnt = 0; 					// couter for checkin

PRIVATE teCardState eCardTofState;					// card's state
PRIVATE uint8 u8CardStatus = CARD_STATUS_NORMAL;	// card's status, should be normal, help, nopwd, retreat
PRIVATE uint8 u8LastHandle;				// to record the last handle of the sending msg

PRIVATE uint32 u32WaitLocatorTimeout;	// timeout for wait locator to do TOF
PRIVATE uint32 u32LocatorRecTick = 0;		// to record the beginning tick of wait locator
PRIVATE uint32 u32StationRecTick = 0;		// to record the tick that received station's finish msg
PRIVATE uint16 u16StationMs = TOF_STATION_PERIOD_MS;		// the coming time of my next slot
PRIVATE uint16 u16LocatorMs;				// the coming time of the locator
PRIVATE uint8	u8StationRunMs;				// the station has ran ms in current slot
PRIVATE uint8	u16SleepDelta;
PRIVATE uint8	u16WakeDelta = TOF_CARD_SYNC_DELTA_MAX;

PRIVATE bool bAccurateMode = FALSE;


PRIVATE uint16 u16Distance;
PRIVATE volatile bool   bDisReporting;

PRIVATE bool   bSyncedAddr = FALSE;
PRIVATE bool   bSysStarted = FALSE;
PRIVATE MAC_ExtAddr_s  u8ExitAddr;
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
    vCardDeviceStartup();
    //EventUtil_vSetEvent(TOF_LISTEN_EVENT);
    EventUtil_vSetEvent(TOF_SYNCADDR_EVENT);

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

    DBG(
        PrintfUtil_u8Init(E_AHI_UART_0, PRINTF_BAUD_RATE_1M);
        PrintfUtil_vPrintf("cold start\n");
    )

    vAHI_DioSetDirection(0, JN_RED | JN_GREEN);
    vSetLed(JN_GREEN, FALSE, 0); //TRUE, TOF_LED_GREEN_MS);
    vSetLed(JN_RED, FALSE, 0);

    vAHI_DioSetDirection(TOF_HELP_PIN | TOF_CHECKIN_PIN | TOF_OK_PIN, 0);
    vAHI_DioWakeEnable(TOF_HELP_PIN | TOF_CHECKIN_PIN | TOF_OK_PIN, 0);
    vAHI_DioWakeEdge(0, TOF_HELP_PIN | TOF_CHECKIN_PIN | TOF_OK_PIN);
    vAHI_DioSetPullup(TOF_CHECKIN_PIN, TOF_BUZZER_PIN);

    u16StationMs = TOF_STATION_PERIOD_MS;
    u16SleepDelta = TOF_CARD_SYNC_DELTA_MAX;

    vAHI_WakeTimerEnable(E_AHI_WAKE_TIMER_0, TRUE);

    // ***************************************************/
    MacUtil_Setting_s sMacUtil;
    sMacUtil.u16SrcPanId		= TOF_CARD_NWK_ADDR;
    sMacUtil.u8Dst_endpoint 	= 0x21;
    sMacUtil.u8Src_endpoint 	= 0x20;   // ??
    sMacUtil.u16Profile_id 		= 0x2001; //0x2001; //for backward compatable
    sMacUtil.u8NodeType 		= 0x02;
    sMacUtil.u16SrcShortAddr 	= u16CardShortAddr;

    MacUtil_vInit(&sMacUtil);

    vUart_Init(E_AHI_UART_1, UART_BAUTRATE_115200, vUart1_txCallBack, vUart1_rxCallBack);
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

    (void)u32AppQApiInit(NULL, NULL, vDioCallBack);
    (void)u32AHI_Init();
	vUart_Init(E_AHI_UART_1, UART_BAUTRATE_115200, vUart1_txCallBack, vUart1_rxCallBack);

	// need enable card's OK button when wakeup (disable when sleep)
    vAHI_DioWakeEnable(TOF_OK_PIN, 0);
    vAHI_DioWakeEdge(0, TOF_OK_PIN);
    DBG(
        PrintfUtil_u8Init(E_AHI_UART_0, PRINTF_BAUD_RATE_1M);
        PrintfUtil_vPrintf("V: %d, Seq: %d\n", OAD_CARD_VERSION, u16SeqNum);
    )

    u16SeqNum++;
    bHelped = FALSE;
    bHelpSync = FALSE;
    vInitParam();
    vCardDeviceStartup();

    //////////////////////////////////////////////////
    // LED flash when wakeup
    // if card is wakeup for locator, not need to flash
    if(u32WakeupEvent != TOF_LOCATOR_EVENT)
    {
        if (bNoPwd)
            vSetLed(JN_RED, TRUE, TOF_LED_RED_MS);
        else
            vSetLed(JN_GREEN, TRUE, TOF_LED_GREEN_MS);
    }
    //////////////////////////////////////////////////

    if(u32WakeStatus & TOF_HELP_PIN)
    {
        // just to wait station coming, not need to consider locator
        TimerUtil_eSetTimer(TOF_CHECK_HELP_EVENT, TOF_HELP_BUTTON_MS);	// from now on, if 1s later the button is still press, HELP!
        eCardTofState = E_TOF_STATE_WAIT_STATION_TOF;
        vSetCardChannel(u8StationChannel);
        TimerUtil_eSetTimer(TOF_LISTEN_EVENT, TOF_STATION_PERIOD_MS);	// if in this period, there's not station coming, it must be lost, need to listen
    }
    else if(u32WakeStatus & TOF_CHECKIN_PIN)
    {
        u8CheckinCnt = 0;
        EventUtil_vSetEvent(TOF_CHECKIN_EVENT);
        if(u8LostTimes == 0)
            TimerUtil_eSetTimer(TOF_SYNC_EVENT, TOF_CHECKIN_TIMEOUT+TOF_BUZZER_TIMEOUT);
        else
            TimerUtil_eSetTimer(TOF_LISTEN_EVENT, TOF_CHECKIN_TIMEOUT+TOF_BUZZER_TIMEOUT);
    }
    else
    {
        vAHI_DioSetPullup(0, TOF_BUZZER_PIN);	// need pullup this pin to save battery pwd
        EventUtil_vSetEvent(u32WakeupEvent);
    }

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
        if(bSysStarted)
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

    // not for me or len is error
    if((APP_PROTOCOL_TYPE_CARD != psAppPkt->tof_head.protocoltype)
            || (psFrame->u8SduLength != sizeof(app_header_t) + psAppPkt->tof_head.len))
        return;

    switch (psAppPkt->tof_head.msgtype)
    {
    case TOF_CARD_OAD:
    {
        if((psAppPkt->tof_head.len == 4)
                && (psAppPkt->rf_tof_oad_data.u8DeviceType == DEVICE_TYPE_CARD)
                && (psAppPkt->rf_tof_oad_data.u16Version > OAD_CARD_VERSION))
        {
            TimerUtil_vStopAllTimer();
            EventUtil_vResetAllEvents();

            // when OAD, buzzer & red LED about 2 seconds to indicate user
            vBuzzer(1800);
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
        EventUtil_vSetEvent(TOF_BUZZER_EVENT);	// when checkin successed, buzzer one time
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

        break;
    }

    case TOF_STATION_BUSY:
    {
        if ((eCardTofState == E_TOF_STATE_LISTEN) && (bRSSIed == FALSE))
        {
            TimerUtil_eStopTimer(TOF_LISTEN_TIMEOUT_EVENT);
            EventUtil_vUnsetEvent(TOF_LISTEN_TIMEOUT_EVENT);

            EventUtil_vSetEvent(TOF_LISTEN_EVENT);
        }
        else if ((eCardTofState == E_TOF_STATE_SYNC) && (psFrame->sSrcAddr.u16PanId == u16StationPanId) && (psFrame->sSrcAddr.uAddr.u16Short == u16StationShortAddr))
        {
            vSyncStation(psAppPkt);
        }

        break;
    }

    case TOF_STATION_RSSI:
    {
        if((eCardTofState == E_TOF_STATE_ALARM) && ((CARD_STATUS_HELP & u8CardStatus) || (CARD_STATUS_NOPWD & u8CardStatus)))
            vSendAlarm(psFrame->sSrcAddr.uAddr.u16Short, psFrame->sSrcAddr.u16PanId);

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

        if((eCardTofState == E_TOF_STATE_ALARM) && ((CARD_STATUS_HELP & u8CardStatus) || (CARD_STATUS_NOPWD & u8CardStatus)))
            vSendAlarm(psFrame->sSrcAddr.uAddr.u16Short, psFrame->sSrcAddr.u16PanId);

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
            TimerUtil_eStopTimer(TOF_LISTEN_TIMEOUT_EVENT);
            EventUtil_vUnsetEvent(TOF_LISTEN_TIMEOUT_EVENT);
            TimerUtil_eStopTimer(TOF_RSSI_EVENT);
            EventUtil_vUnsetEvent(TOF_RSSI_EVENT);

            u16StationShortAddr = psFrame->sSrcAddr.uAddr.u16Short;
            u16StationPanId = psFrame->sSrcAddr.u16PanId;
            u32MyEvent = TOF_REQUEST_EVENT;
        }
        else if ((eCardTofState == E_TOF_STATE_SYNC) && (psFrame->sSrcAddr.u16PanId == u16StationPanId) && (psFrame->sSrcAddr.uAddr.u16Short == u16StationShortAddr))
        {
            vSyncStation(psAppPkt);
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
        DBG(
            PrintfUtil_vPrintf("station_fin: %d\n", eCardTofState);
        )
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
            bTofConnected 	= TRUE;
            u32StationRecTick = u32AHI_TickTimerRead();

            u16Distance =  psAppPkt->rf_tof_station_finish.u16Dist2Station;

            // if card receive station_finish, need to report alarm status,
            // else, card must send out rssi msg, which include alarm status already
            if(u8CardStatus & (CARD_STATUS_HELP | CARD_STATUS_NOPWD))
            {
                EventUtil_vSetEvent(TOF_ALARM_EVENT);
                TimerUtil_eSetTimer(TOF_STATION_DONE_EVENT, TOF_ALARM_TIMEOUT+10);
            }
            else
                EventUtil_vSetEvent(TOF_CHECK_VDD_EVENT);

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

                    u8StationChannel = psAppPkt->rf_tof_station_accept.u8StationChannel;
                    u8LocatorChannel = psAppPkt->rf_tof_station_accept.u8LocatorCardChannel;
                    u16CardSlot = psAppPkt->rf_tof_station_accept.tsAccptData[u8Tmp].u16SlotIndex;

                    u16StationMs = ((u16CardSlot + TOF_SLOT_NUM - psAppPkt->rf_tof_station_accept.u16CurSlot)*TOF_SLOT_MS - (uint16)(psAppPkt->rf_tof_station_accept.u8RunMs)) % TOF_STATION_PERIOD_MS;

                    EventUtil_vSetEvent(TOF_SLEEP_EVENT);

                    DBG(
                        PrintfUtil_vPrintf("accept, slot: %d, run: %d, sleep: %d\n", u16CardSlot, psAppPkt->rf_tof_station_accept.u8RunMs, u16StationMs);
                    )
                    break;
                }
            }
        }

        break;
    }

    // the station receive card's help
    case TOF_STATION_HELP_ACK:
    {
        bHelped = TRUE;
        break;
    }

    // help success: app received card's help
    case TOF_APP_HELP_ACK:
    {
        bHelped = TRUE;
        u8CardStatus &= (~CARD_STATUS_HELP);
        EventUtil_vSetEvent(TOF_BUZZER_EVENT);
        break;
    }

    case TOF_STATION_NOPWD_ACK:
    {
        bReportNoPwd = TRUE;
        u8CardStatus &= (~CARD_STATUS_NOPWD);
        TimerUtil_eStopTimer(TOF_STATION_DONE_EVENT);
        EventUtil_vSetEvent(TOF_STATION_DONE_EVENT);

        break;
    }

    ///////////////////////////////////////////////////////////
    // locator msg
    case TOF_LOCATOR_FINISH:	// The locator finish TOF
    {
        if(eCardTofState == E_TOF_STATE_WAIT_LOCATOR)
        {
            eCardTofState = E_TOF_STATE_LOCATOR_FINISH;
            TimerUtil_eStopTimer(TOF_LOCATOR_DONE_EVENT);
            EventUtil_vSetEvent(TOF_LOCATOR_DONE_EVENT);
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
            bHelpSync = TRUE;
            EventUtil_vSetEvent(TOF_SYNC_EVENT);
        }

        EventUtil_vUnsetEvent(u32EventID);
        break;
    }

    case TOF_RETREAT_EVENT:
    {
        vSetLed(JN_RED, TRUE, TOF_LED_RED_MS);
        vBuzzer(TOF_BUZZER_TIMEOUT);

        if(bRetreatAck == FALSE)
            TimerUtil_eSetTimer(TOF_RETREAT_EVENT, TOF_RETREAT_PERIOD_MS);

        EventUtil_vUnsetEvent(u32EventID);
        break;
    }

    case TOF_BUZZER_EVENT:
    {
        vBuzzer(TOF_BUZZER_TIMEOUT);
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
            if((bHelped == FALSE) || (u8HelpCnt+6 <= TOF_CARD_HELP_TIMES))
            {
                vBuzzer(TOF_BUZZER_TIMEOUT);
                vSetLed(JN_RED, TRUE, TOF_LED_RED_MS);
            }
            else
                vSetLed(JN_GREEN, TRUE, TOF_LED_GREEN_MS);
        }

        EventUtil_vUnsetEvent(u32EventID);
        break;
    }

    case TOF_ALARM_EVENT:
    {
        eCardTofState = E_TOF_STATE_ALARM;
        vSetCardChannel(u8BroadcastChannel);

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
        vSetCardChannel(u8StationChannel);
        DBG(
            PrintfUtil_vPrintf("tof processing tick: %d\n", u32AHI_TickTimerRead());
        )
//vCardCast(0xAA, 0xF000, u16SeqNum, 0, 0); // for test: record the wakeup time

        if(bAccurateMode)
            TimerUtil_eSetTimer(TOF_SYNC_EVENT, TOF_PENDING_TIMEOUT+20);	// if wait station to do tof timeout, need sync to the station
        else
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
            if(TRUE == bHelpSync)
                EventUtil_vSetEvent(TOF_HELP_EVENT);
            else
                EventUtil_vSetEvent(TOF_LISTEN_EVENT);
        }
        else
        {
            vSetCardChannel(u8BroadcastChannel);

            if(TRUE == bHelpSync)
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
        )
        TimerUtil_eSetTimer(TOF_LISTEN_TIMEOUT_EVENT, TOF_LISTEN_TIMEOUT);
        EventUtil_vUnsetEvent(u32EventID);
        break;
    }

    case TOF_LISTEN_TIMEOUT_EVENT:
    {
        DBG(
            PrintfUtil_vPrintf("listen timeout\n");
        )
        eCardTofState = E_TOF_STATE_IDLE;
        if(bRSSIed)
            EventUtil_vSetEvent(TOF_CHECK_VDD_EVENT);
        else
        {
            vSendRssi();
            TimerUtil_eSetTimer(TOF_CHECK_VDD_EVENT, TOF_RSSI_TIMEOUT);
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

        TimerUtil_eSetTimer(TOF_LISTEN_EVENT, TOF_REQUEST_TIMEOUT);
        EventUtil_vUnsetEvent(u32EventID);
        break;
    }

    case TOF_LOCATOR_EVENT:
    {
        eCardTofState = E_TOF_STATE_WAIT_LOCATOR;
        vSetCardChannel(u8LocatorChannel);
        DBG(
            PrintfUtil_vPrintf("wait locator\n");
        )
//vCardCast(0xAA, 0xF000, u16SeqNum, 0, 0); // for test: record the wakeup time
        u32LocatorRecTick = u32AHI_TickTimerRead();

        TimerUtil_eSetTimer(TOF_LOCATOR_DONE_EVENT, u32WaitLocatorTimeout);
        EventUtil_vUnsetEvent(u32EventID);
        break;
    }

    case TOF_CHECK_VDD_EVENT:
    {
        //if((u8CardStatus == CARD_STATUS_NORMAL) && ((u16SeqNum & 0x000F) == 0))
        if((u16SeqNum & 0x000F) == 0) // sample every 16*5 seconds
        {
            uint32 u32Vol = SysUtil_u32GetExtVoltage();
            // y = 0.0024x - 0.0111
            DBG(
                PrintfUtil_vPrintf("Vol: %d\n", (u32Vol*24 - 1110)/10);
            )
            if(TOF_VDD_3600 > u32Vol)
            {
                if(bNoPwd == FALSE)
                {
                    bReportNoPwd = FALSE;
                    bNoPwd = TRUE;
                }
            }

            if(TOF_VDD_3800 < u32Vol)
            {
                bReportNoPwd = TRUE;
                bNoPwd = FALSE;
            }
        }

        // not need to send nopwd alarm at once, report it after every station_finish until receive nopwd_ack
        if((bNoPwd) && (bReportNoPwd == FALSE))
            u8CardStatus |= CARD_STATUS_NOPWD;
        else
            u8CardStatus &= (~CARD_STATUS_NOPWD);

        EventUtil_vSetEvent(TOF_STATION_DONE_EVENT);
        EventUtil_vUnsetEvent(u32EventID);
        break;
    }

    case TOF_STATION_DONE_EVENT:
    {

        eCardTofState = E_TOF_STATE_STATION_FINISH;

        if (FALSE == bTofConnected) // tof failed
        {
            u8NeedLocator = 0;
            u16StationMs = TOF_STATION_PERIOD_MS - (uint16)((u32AHI_TickTimerRead()/16000)%TOF_STATION_PERIOD_MS) + u16WakeDelta;

            if (u8LostTimes < TOF_STATION_CARD_MAX_FAILED)
                u8LostTimes++;
        }
        else
        {
            u8LostTimes = 0;
            u16WakeDelta = (uint16)(u32StationRecTick/16000) - (uint16)u8StationRunMs;

            // u16WakeDelta should smaller than TOF_CARD_SYNC_DELTA_MAX in normal wakeup
            // but DIO wakeup will larger than it, in this case not need to decrease u16SleepDelta because DIO wakeup's wake delta is not determined
            if((u16WakeDelta > TOF_CARD_SYNC_DELTA_MIN) && (u16WakeDelta < TOF_CARD_SYNC_DELTA_MAX))
                u16SleepDelta -= MIN((u16WakeDelta - TOF_CARD_SYNC_DELTA_MIN)/2, 4);
            else if((u16WakeDelta < TOF_CARD_SYNC_DELTA_MIN))
                u16SleepDelta += MAX((TOF_CARD_SYNC_DELTA_MIN - u16WakeDelta)/2, 1);

            u16SleepDelta = MAX(TOF_CARD_SYNC_DELTA_MIN, u16SleepDelta);

            // need to -(uint16)((u32AHI_TickTimerRead() - u32StationRecTick)/16000),
            // because if alarm status, card need change to common channel to send alarm msg when it receive station_finish (u32StationRecTick)
            u16StationMs = TOF_STATION_PERIOD_MS  - (uint16)u8StationRunMs - (uint16)((u32AHI_TickTimerRead() - u32StationRecTick)/16000);
        }
        DBG(
            PrintfUtil_vPrintf("station done: %d, sleep delta: %d, u16WakeDelta: %d, bTofConnected: %d, dis:%d \n", u16StationMs, u16SleepDelta, u16WakeDelta, bTofConnected, u16Distance);
        )

        EventUtil_vSetEvent(TOF_SLEEP_EVENT);
        EventUtil_vUnsetEvent(u32EventID);
        break;
    }

    case TOF_LOCATOR_DONE_EVENT:
    {
        u8NeedLocator = 0;
        eCardTofState = E_TOF_STATE_LOCATOR_FINISH;
        u16StationMs -= (uint16)((u32AHI_TickTimerRead() - u32LocatorRecTick)/16000);

        EventUtil_vSetEvent(TOF_SLEEP_EVENT);
        EventUtil_vUnsetEvent(u32EventID);
        break;
    }

    case TOF_SLEEP_EVENT:
    {
        bool_t bOkToSleep;

        /* after a tof,  report TOF result */
        vReportTofDistance();

        if((0 == (u32AHI_DioReadInput() & TOF_HELP_PIN)) 							// the help button is press
                || ((u8CardStatus & CARD_STATUS_RETREAT) && (bRetreatAck == FALSE)) 	// user didn't press OK button when retreat
                || (u8HelpCnt < TOF_CARD_HELP_TIMES))								// still helping
        {
            bOkToSleep = FALSE;

            // In help status, ignore locator because card need to change to common channel to receive help_ack
            if (u8HelpCnt < TOF_CARD_HELP_TIMES)
                u8NeedLocator = 0;
        }
        else
            bOkToSleep = TRUE;

        if(u8NeedLocator > 0)
        {
            if(u8NeedLocator == 1)
                u16LocatorMs = (TOF_SLOT_LOC_INT - (u16CardSlot%TOF_SLOT_LOC_INT)) * TOF_SLOT_MS;
            else
                u16LocatorMs = (TOF_SLOT_LOC_INT - ((u16CardSlot+TOF_SLOT_LOC_INT/2)%TOF_SLOT_LOC_INT)) * TOF_SLOT_MS;

            if(u16LocatorMs > (TOF_STATION_PERIOD_MS - (u16StationMs%TOF_STATION_PERIOD_MS)))
            {
                u16LocatorMs -= (TOF_STATION_PERIOD_MS - (u16StationMs%TOF_STATION_PERIOD_MS));
                if ((u16LocatorMs > u16SleepDelta) && bOkToSleep)
                {
                    u16StationMs -= u16LocatorMs - u16SleepDelta + 8;
                    u32WakeupEvent = TOF_LOCATOR_EVENT;
                    u32WaitLocatorTimeout = TOF_LOCATOR_PERIOD_MS + u16SleepDelta;
                    u16SeqNum--;	// because when wakeup, u16SeqNum++;
                    vCardSleep(u16LocatorMs - u16SleepDelta);
                }
                else
                {
                    u32WaitLocatorTimeout = TOF_LOCATOR_PERIOD_MS + u16LocatorMs;
                    EventUtil_vSetEvent(TOF_LOCATOR_EVENT);
                }
            }
            else
            {
                u32WaitLocatorTimeout = TOF_LOCATOR_PERIOD_MS + u16LocatorMs;
                EventUtil_vSetEvent(TOF_LOCATOR_EVENT);
            }
        }

        // not need locator
        else
        {
            u32WakeupEvent = (u8LostTimes == 0) ? TOF_STATION_EVENT : TOF_LISTEN_EVENT;

            // just for safe check, to ensure the station's coming time is within (0, TOF_STATION_PERIOD_MS]
            while(u16StationMs > TOF_STATION_PERIOD_MS)
                u16StationMs -= TOF_STATION_PERIOD_MS;

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
                DBG(
                    PrintfUtil_vPrintf("not ok to sleep: %d\n", u16StationMs);
                )
                u16SeqNum++;
                vInitParam();

                // the wake up envent is coming, just wait for this event
                if (u16StationMs <= 5 + u16SleepDelta)
                    EventUtil_vSetEvent(u32WakeupEvent);
                else
                    TimerUtil_eSetTimer(u32WakeupEvent, u16StationMs - u16SleepDelta - 5);
            }
            DBG(
                PrintfUtil_vPrintf("\n");
            )
        }

        EventUtil_vUnsetEvent(u32EventID);
        break;
    }

    case TOF_WATCHDOG_EVENT:
    {
        vAHI_WatchdogRestart();
        EventUtil_vUnsetEvent(u32EventID);
        break;
    }
    case TOF_SYNCADDR_EVENT:
    {
        if(bSyncedAddr)
        {
            PrintfUtil_vPrintf("SyncEnd\n");
            if(!bSysStarted)
            {
                bSysStarted = TRUE;
                vInitCardSystem();
                vInitParam();
                vCardDeviceStartup();
                EventUtil_vSetEvent(TOF_LISTEN_EVENT);
            }
        }
        else
        {
            uart_sync_req_t uart_sync_req;
            uart_sync_req.hdr.header_h = UART_PREAMBLE_H;
            uart_sync_req.hdr.header_l = UART_PREAMBLE_L;
            uart_sync_req.hdr.cmdtype = SYNC_REQ;
            uart_sync_req.hdr.padding = 0;
            uart_sync_req.hdr.len = 0;
            uart_sync_req.hdr.checksum = uart_sync_req.hdr.cmdtype + uart_sync_req.hdr.len;
            CONVERT_ENDIAN(uart_sync_req.hdr.len);
            CONVERT_ENDIAN(uart_sync_req.hdr.checksum);
            u8Uart_StartTx(E_AHI_UART_1, (uint8*)&uart_sync_req, sizeof(uart_sync_req));
            PrintfUtil_vPrintf("Sync\n");
            PrintfUtil_vPrintMem((uint8*)&uart_sync_req, sizeof(uart_sync_req));

            TimerUtil_eSetTimer(TOF_SYNCADDR_EVENT, 1000);
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
    case TOF_CARD_ALARM:
    case TOF_CARD_CHECKIN:
    {
        RfTofData.rf_tof_card_data.u16SeqNum = u16value;
        RfTofData.rf_tof_card_data.u8CardStatus = u8CardStatus;
        RfTofData.rf_tof_card_data.u8Reserved = u8HelpCnt;	// this is for test, u8Reserved can be any value
        RfTofData.tof_head.len = 4;
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
    if(u8CurChannel != channel)
    {
        eAppApiPlmeSet(PHY_PIB_ATTR_CURRENT_CHANNEL, channel);
        u8CurChannel = channel;
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
        vSetCardChannel(u8BroadcastChannel);
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
    uint16 u16SleepMs = (TOF_SLOT_MS * (u16CardSlot + TOF_SLOT_NUM - psAppPkt->rf_tof_station_signal.u16CurSlot - 1) - (uint16)(psAppPkt->rf_tof_station_signal.u8RunMs)) % TOF_STATION_PERIOD_MS;

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
        if (u16SleepMs <= 5 + u16SleepDelta)
        {
            vInitParam();
            vAHI_TickTimerWrite(0);
            EventUtil_vSetEvent(u32WakeupEvent);
        }
        else
        {
            vCardSleep(u16SleepMs - u16SleepDelta);
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
    u8NeedLocator = 0;
    bRSSIed = FALSE;
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
        if(!(u8CardStatus & CARD_STATUS_RETREAT))
        {
            EventUtil_vSetEvent(TOF_RETREAT_EVENT);
            u8CardStatus |= CARD_STATUS_RETREAT;
            bRetreatAck = FALSE;
        }
    }
    else if (u8StationStatus == STATION_STATUS_NORMAL)
    {
        u8CardStatus &= (~CARD_STATUS_RETREAT);
        //bRetreatAck = FALSE;
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
    vAHI_TimerClockSelect(TOF_BUZZER_TIMER, FALSE, TRUE);
    vAHI_TimerStartRepeat(TOF_BUZZER_TIMER, 1, 2);
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
}


PRIVATE void vDioCallBack(uint32 u32Device, uint32 u32ItemBitmap)
{
    if(u32Device == E_AHI_DEVICE_SYSCTRL)
    {
        if((bHelped == FALSE) && (u8HelpCnt >= TOF_CARD_HELP_TIMES) && (TOF_HELP_PIN & u32ItemBitmap))
        {
            DBG(
                PrintfUtil_vPrintf("help DIO\n");
            )
            TimerUtil_eSetTimer(TOF_CHECK_HELP_EVENT, TOF_HELP_BUTTON_MS);
        }
        else if (TOF_OK_PIN & u32ItemBitmap)
        {
            DBG(
                PrintfUtil_vPrintf("OK DIO\n");
            )
            bRetreatAck = TRUE;
        }
    }
}


PRIVATE void vCardSleep(uint16 u16SleepMs)
{
    vSetLed(JN_GREEN, FALSE, 0);
    vSetLed(JN_RED, FALSE, 0);
    vAHI_UartDisable(E_AHI_UART_0);
    vAHI_DioWakeEnable(0, TOF_OK_PIN);	// need disable OK button's wakeup ability when sleep
    SleepUtil_Sleep(E_AHI_WAKE_TIMER_0, u16SleepMs, E_AHI_SLEEP_OSCON_RAMON);
}

PRIVATE void vReportTofDistance(void)
{
    uart_locate_t uart_locate;
    uart_locate.hdr.header_h = UART_PREAMBLE_H;
    uart_locate.hdr.header_l = UART_PREAMBLE_L;
    uart_locate.hdr.cmdtype = LOCATE;

    uart_locate.hdr.len  = sizeof(Locate_Information_t);
    uart_locate.hdr.checksum  = uart_locate.hdr.len + uart_locate.hdr.cmdtype;

    CONVERT_ENDIAN(uart_locate.hdr.len);
    CONVERT_ENDIAN(uart_locate.hdr.checksum);

    static uint8 seqnum;
    if(bTofConnected && (u16Distance!= INVALID_TOF_DISTANCE))
    {
        uart_locate.locate.isvalid = TRUE;
    }
    else
    {
        uart_locate.locate.isvalid = FALSE;
    }
    uart_locate.locate.seqnum = seqnum++;
    uart_locate.locate.stationPandID = u16StationPanId;
    uart_locate.locate.stationShortAddr =  u16StationShortAddr;
    uart_locate.locate.Locate_Distance = u16Distance;

    CONVERT_ENDIAN(uart_locate.locate.stationPandID);
    CONVERT_ENDIAN(uart_locate.locate.stationShortAddr);
    CONVERT_ENDIAN( uart_locate.locate.Locate_Distance);

    DBG(PrintfUtil_vPrintf("U tx %d\n",sizeof(uart_locate)););
    DBG(PrintfUtil_vPrintMem((uint8*)&uart_locate,sizeof(uart_locate)););

    /* send to uart , block operation */
    bDisReporting = TRUE;
    u8Uart_StartTx(E_AHI_UART_1, (uint8*)&uart_locate, sizeof(uart_locate));
    //TimerUtil_vDelay(3, E_TIMER_UNIT_MILLISECOND);

    while(bDisReporting == TRUE);
}

PRIVATE void vUart1_txCallBack(void)
{
    bDisReporting = FALSE;
}

PRIVATE void vUart1_rxCallBack(uint8* p, uint16 len)
{
    DBG(PrintfUtil_vPrintf("U rx %d\n", len););

    if(p==NULL || len < sizeof(Uart_Header_t) || len>UART_MAX_RX_LEN)
    {
        return;
    }

    Uart_Header_t	* pHeader = (Uart_Header_t*)p;

    switch(pHeader->cmdtype)
    {
    case SYNC_ACK:
    {
        if(len != sizeof(uart_sync_ack_t))
        {
            return;
        }
        uart_sync_ack_t *puart_sync_ack = (uart_sync_ack_t *)p;
        memcpy((uint8*)&u8ExitAddr,  puart_sync_ack->ExitAddr,  8);

        bSyncedAddr = TRUE;

		DBG(PrintfUtil_vPrintf("Get ExitAddr: \n"););
		DBG(PrintfUtil_vPrintMem(puart_sync_ack->ExitAddr, 8););
        CONVERT_ENDIAN(u8ExitAddr.u32L);
        CONVERT_ENDIAN(u8ExitAddr.u32H);

		DBG(PrintfUtil_vPrintf("u8ExitAddr: %x %x\n", u8ExitAddr.u32L, u8ExitAddr.u32H););

#if 1
        psMacAddr = u8ExitAddr;        
		// MacUtil_vReadExtAddress(&psMacAddr);
        uint32 tmp32;
        tmp32 = StringUtil_BCD2uint32(psMacAddr.u32H);

        u16CardShortAddr = tmp32%100000;
        
        u16CardPanId = TOF_CARD_NWK_ADDR;
        u8BroadcastChannel = ((psMacAddr.u32L) >> 16) & 0xFF;

        //bAccurateMode = ((psMacAddr.u32H) & 0xFF)? TRUE : FALSE;

        u8CurChannel = u8BroadcastChannel;
        DBG(PrintfUtil_vPrintf("sAddr:%d, BChannel:%d\n",u16CardShortAddr, u8BroadcastChannel););
#endif
        break;
    }
    }

    /*
    if(p==NULL || len<sizeof(Uart_Header_t) || len>UART_MAX_RX_LEN)
    {
    	return;
    }

    MacUtil_SendParams_s sParams;
    sParams.u8Radius		= 1;
    sParams.u16DstAddr	= u16DstAddr;
    sParams.u16DstPanId 	= u16DstPanId;
    sParams.u16ClusterId 	= 0;
    sParams.u8DeliverMode	= MAC_UTIL_UNICAST;

    MacUtil_vSendData(&sParams, (uint8*)&RfTofData, RfTofData.tof_head.len+4, u8TxOptions);
    */
}


/****************************************************************************/
/***        END OF FILE                                                   ***/
/****************************************************************************/





