
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
#define DBG(x) do{x} while(0);

//#define DBG(x)
#endif

#define CONVERT_ENDIAN(x)	SysUtil_vConvertEndian(&(x), sizeof(x))

#define TOF_VDD_3600		1547	// 3.6v
#define TOF_VDD_3800		1630	// 3.8v

#define POWEROFF      6

#define TOF_OK_PIN			E_AHI_DIO15_INT
#define TOF_HELP_PIN		E_AHI_DIO14_INT
#define TOF_CHECKIN_PIN	E_AHI_DIO10_INT
#define TOF_BUZZER_PIN		E_AHI_DIO13_INT

#define TOF_BUZZER_TIMER	E_AHI_TIMER_1	// for buzzer's input

#define TOF_SHUT_EVENT					BIT(3)	// the card's OAD event

#define TOF_SYNCADDR_EVENT			BIT(4)	// sync address after restart

#define TOF_LED_RED_OFF_EVENT		BIT(5)	// the card's red LED off
#define TOF_LED_GREEN_OFF_EVENT		BIT(6)	// the card's green LED off
#define TOF_CHECK_VDD_EVENT			BIT(7)	// check battery
#define TOF_REPORT_EVENT				BIT(9)	// sleep
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


typedef struct
{
    unsigned char	isvalid;
    unsigned char 	seqnum;
    unsigned char   success_rate;
    int8   RSSI;
    unsigned short   stationPandID;
    unsigned short   stationShortAddr;
    unsigned short   Locate_Distance;
} Locate_Information_temp;

typedef struct
{
	Uart_Header_t hdr;
	Locate_Information_temp locate;
}uart_locate_temp;

typedef struct
{
	app_header_t tof_head;

	uint8 u8StationStatus; 	// tof_station_status_te
	uint8 u8RunMs;	// Station runs ms in this slot
	uint8 u8LocN;	// 0: not need locator, 1: need locator #1, 2: need locator #2
	uint8 u8Reserved;
    uint8 u8Rate;
	uint16 u16Dist2Station;
	uint16 u16Dsit2Locator;
} rf_tof_station_finish_ts_t;
typedef struct
{
	app_header_t tof_head;

	uint8 u8StationStatus; 	// tof_station_status_te
	uint8 u8AvailableMs;		// the available time for card to send join request (card use this to generate a random number)
	uint16 u16CurSlot;		// station's curSlot

	//fixme, this is not a 4-Byte struct
	uint8 u8RunMs;			// station run ms in this slot
	uint8 u8LocIdle; 			// 1: has idle loc slot, 0: has not
	int8  RSSI;
} rf_tof_station_signal_ts_t;

typedef union
{
	app_header_t tof_head;
	rf_tof_card_data_ts rf_tof_card_data;
	rf_tof_locator_card_info_ts rf_tof_locator_card_info;
	rf_tof_station_card_info_ts rf_tof_station_card_info;
	rf_tof_station_accept_ts rf_tof_station_accept;
	rf_tof_station_signal_ts_t rf_tof_station_signal;
	rf_tof_station_finish_ts_t rf_tof_station_finish;
	rf_tof_oad_data_ts rf_tof_oad_data;
}RfTofWrapper_tu_temp;



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
PRIVATE void vSetLed(uint32 u32Pin, bool_t bOnOff, uint16 u16OnMs);
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
PRIVATE uint8 u8CurChannel;



PRIVATE uint16 u16SeqNum = 0;		// card's sequence number
PRIVATE uint8 u8LostTimes = TOF_STATION_CARD_MAX_FAILED;	// to record the times that card lost station

PRIVATE bool_t bHelpSync = FALSE;			// is the sync is from card help event
PRIVATE bool_t bTofConnected = FALSE;		// connect with station or not
PRIVATE bool_t bReportNoPwd = FALSE;		// report no power success or not
PRIVATE bool_t bRetreatAck = FALSE;		// if station retreat, use need to press OK button to stop buzzer
PRIVATE uint8 u8CheckinCnt = 0; 					// couter for checkin
PRIVATE teCardState eCardTofState;					// card's state
PRIVATE uint8 u8CardStatus = CARD_STATUS_NORMAL;	// card's status, should be normal, help, nopwd, retreat
PRIVATE uint8 u8LastHandle;				// to record the last handle of the sending msg

PRIVATE uint16 u16Distance;
PRIVATE int8   bRSSI;
PRIVATE uint8  bRate;
PRIVATE uint8  count = 0;


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
    TimerUtil_eSetTimer(TOF_WATCHDOG_EVENT, 1000);
    TimerUtil_eSetTimer(TOF_RSSI_EVENT,500);
    TimerUtil_eSetTimer(TOF_REPORT_EVENT,1000);
    vOADInitClient(TRUE);
    PrintfUtil_vPrintf("START\n");
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
    DBG(PrintfUtil_u8Init(E_AHI_UART_0, PRINTF_BAUD_RATE_1M);
        PrintfUtil_vPrintf("cold start\n");)
    vAHI_DioSetDirection(0, JN_RED | JN_GREEN);
    vSetLed(JN_GREEN, FALSE, 0); //TRUE, TOF_LED_GREEN_MS);
    vSetLed(JN_RED, FALSE, 0);
    vAHI_DioSetDirection(TOF_HELP_PIN | TOF_CHECKIN_PIN | TOF_OK_PIN, 0);
    vAHI_DioWakeEnable(TOF_HELP_PIN | TOF_CHECKIN_PIN | TOF_OK_PIN, 0);
    vAHI_DioWakeEdge(0, TOF_HELP_PIN | TOF_CHECKIN_PIN | TOF_OK_PIN);
    vAHI_DioSetPullup(TOF_CHECKIN_PIN, TOF_BUZZER_PIN);
    vAHI_WakeTimerEnable(E_AHI_WAKE_TIMER_0, TRUE);
    MacUtil_vReadExtAddress(&psMacAddr);
    uint32 tmp32;
    tmp32 = StringUtil_BCD2uint32(psMacAddr.u32H);
    u16CardShortAddr = tmp32%100000;
    PrintfUtil_vPrintf("u16CardShortAddr:: %d\n",u16CardShortAddr);

    u16CardPanId = TOF_CARD_NWK_ADDR;
    u8BroadcastChannel = ((psMacAddr.u32L) >> 16) & 0xFF;

    PrintfUtil_vPrintf("u8BroadcastChannel:: %d \n",u8BroadcastChannel);

    //bAccurateMode = ((psMacAddr.u32H) & 0xFF)? TRUE : FALSE;

    u8CurChannel = u8BroadcastChannel;
    // ***************************************************/
    MacUtil_Setting_s sMacUtil;
    sMacUtil.u16SrcPanId		= TOF_CARD_NWK_ADDR;
    sMacUtil.u8Dst_endpoint 	= 0x21;
    sMacUtil.u8Src_endpoint 	= 0x20;   // ??
    sMacUtil.u16Profile_id 		= 0x2001; //0x2001; //for backward compatable
    sMacUtil.u8NodeType 		= 0x02;
    sMacUtil.u16SrcShortAddr 	= u16CardShortAddr;
    MacUtil_vInit(&sMacUtil);
    eAppApiPlmeSet(PHY_PIB_ATTR_CURRENT_CHANNEL,u8BroadcastChannel );
     PrintfUtil_vPrintf("CCCCC= %d\n",u8BroadcastChannel);
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

    PrintfUtil_vPrintf("WakeUp: \n");
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
    vInitParam();
    vCardDeviceStartup();
    TimerUtil_eSetCircleTimer(TOF_WATCHDOG_EVENT, 1000);	// feed watch dog every 1s

    while(1)
    {
        TimerUtil_vUpdate();
        vProcessSysEvent();
        vProcessAppEvent();
    }
}

PRIVATE void vSendRssi()
{
	vCardCast(TOF_CARD_RSSI, 0x0000, 0xFFFF, 0, u16SeqNum);
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

    //eAppApiPlmeSet(PHY_PIB_ATTR_CURRENT_CHANNEL, boardcas);
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
        //if(bSysStarted)
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
    RfTofWrapper_tu_temp* psAppPkt = (RfTofWrapper_tu_temp*)(psFrame->au8Sdu);


    PrintfUtil_vPrintf("Type %d\n",psAppPkt->tof_head.protocoltype);
    PrintfUtil_vPrintf("Length %d\n",psFrame->u8SduLength);
    // not for me or len is error
    if((APP_PROTOCOL_TYPE_CARD != psAppPkt->tof_head.protocoltype)
            || (psFrame->u8SduLength != sizeof(app_header_t) + psAppPkt->tof_head.len))
    {
        PrintfUtil_vPrintf("fail\n");
        return;
    }
    PrintfUtil_vPrintf("RX= %d\n",psAppPkt->tof_head.msgtype);
    switch (psAppPkt->tof_head.msgtype)
    {
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

    case TOF_STATION_AVAILABLE:
    {
        uint32 u32MyEvent = 0;
        PrintfUtil_vPrintf("1\n");
        TimerUtil_eStopTimer(TOF_LISTEN_TIMEOUT_EVENT);
        EventUtil_vUnsetEvent(TOF_LISTEN_TIMEOUT_EVENT);
        u16StationShortAddr = psFrame->sSrcAddr.uAddr.u16Short;
        u16StationPanId = psFrame->sSrcAddr.u16PanId;
        PrintfUtil_vPrintf("Addr:: %d\n",u16StationShortAddr);
        PrintfUtil_vPrintf("PanId:: %d\n",u16StationPanId);
        u32MyEvent = TOF_REQUEST_EVENT;
        if(u32MyEvent > 0)
        {
            EventUtil_vSetEvent(u32MyEvent);
        }
        break;
    }
    case TOF_STATION_RSSI:
    {
        bRSSI = psAppPkt->rf_tof_station_signal.RSSI;
        break;
    }

    case TOF_STATION_FINISH:
    {
        DBG(
            PrintfUtil_vPrintf("station_fin: %d\n", eCardTofState);
        )
        {
            eCardTofState = E_TOF_STATE_STATION_FINISH;
            bTofConnected 	= TRUE;
            u16Distance =  psAppPkt->rf_tof_station_finish.u16Dist2Station;
            bRate       =  psAppPkt->rf_tof_station_finish.u8Rate;
        }
        break;
    }
    default:
        break;
    }
}

/****************************************************************************
 * NAME: vProcessAppEvent
 *
 * DESCRIPTION:
 * 	Proccess the application event queues
 *
 * PARAMETERS:      None
 * RETURNS:
 * void
 ****************************************************************************/
PRIVATE void vProcessAppEvent()
{
    uint32 u32EventID = EventUtil_u32ReadEvents();
    switch (u32EventID)
    {
    case TOF_REQUEST_EVENT:
    {
        eCardTofState = E_TOF_STATE_REQUEST_STATION;
        DBG(
            PrintfUtil_vPrintf("tof request\n");
        )
        vCardCast(TOF_CARD_REQUEST, u16StationShortAddr, u16StationPanId, 0, u16SeqNum);
        EventUtil_vUnsetEvent(u32EventID);
        break;
    }
    case TOF_REPORT_EVENT:
    {
        vReportTofDistance();
        bTofConnected = FALSE;
        TimerUtil_eSetTimer(TOF_REPORT_EVENT,2000);
        EventUtil_vUnsetEvent(u32EventID);
        break;
    }

    case TOF_WATCHDOG_EVENT:
    {
        PrintfUtil_vPrintf("W\n");
        vAHI_WatchdogRestart();
        EventUtil_vUnsetEvent(u32EventID);
        break;
    }
    case TOF_RSSI_EVENT:
    {
        DBG(
            PrintfUtil_vPrintf("rssi\n");
        )
        vSendRssi();
        TimerUtil_eSetTimer(TOF_RSSI_EVENT,1000);
        EventUtil_vUnsetEvent(u32EventID);
        break;
    }
    case TOF_SYNCADDR_EVENT:
    {
        if(bSyncedAddr)
        {
            bSyncedAddr = FALSE;
            count = 0;
            PrintfUtil_vPrintf("SyncEnd\n");
            vAHI_WatchdogStop();
            TimerUtil_eSetTimer(TOF_SYNCADDR_EVENT, 60000);
            if(!bSysStarted)
            {
                bSysStarted = TRUE;
                //vAHI_WatchdogStop
                PrintfUtil_vPrintf("ASD\n");
                vInitCardSystem();
                vInitParam();
                vCardDeviceStartup();
                TimerUtil_eSetTimer(TOF_RSSI_EVENT,1000);
                TimerUtil_eSetTimer(TOF_SYNCADDR_EVENT,60000);
                TimerUtil_eSetCircleTimer(TOF_WATCHDOG_EVENT, 1000);
                TimerUtil_eSetTimer(TOF_REPORT_EVENT,2000);
            }
        }
        else
        {
            count++;
            if(count > 3 )
            {
                PrintfUtil_vPrintf("Sleep: \n");
                vAHI_UartDisable(E_AHI_UART_0);
                vAHI_UartDisable(E_AHI_UART_1);
                vAHI_DioWakeEnable(0, TOF_OK_PIN);	// need disable OK button's wakeup ability when sleep
                TimerUtil_vStopAllTimer();
                EventUtil_vResetAllEvents();
                vAHI_Sleep(E_AHI_SLEEP_DEEP);
            }
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

    RfTofWrapper_tu_temp RfTofData;
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

PRIVATE void vDioCallBack(uint32 u32Device, uint32 u32ItemBitmap)
{
    if(u32Device == E_AHI_DEVICE_SYSCTRL)
    {
        if (TOF_OK_PIN & u32ItemBitmap)
        {
            DBG(
                PrintfUtil_vPrintf("OK DIO\n");
            )
            bRetreatAck = TRUE;
        }
    }
}
PRIVATE void vReportTofDistance(void)
{
    uart_locate_temp uart_locate;
    uart_locate.hdr.header_h = UART_PREAMBLE_H;
    uart_locate.hdr.header_l = UART_PREAMBLE_L;
    uart_locate.hdr.cmdtype = LOCATE;

    uart_locate.hdr.len  = sizeof(Locate_Information_temp);
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
        bTofConnected = FALSE;
    }
    uart_locate.locate.seqnum = seqnum++;
    uart_locate.locate.stationPandID = u16StationPanId;
    uart_locate.locate.stationShortAddr =  u16StationShortAddr;
    uart_locate.locate.Locate_Distance = u16Distance;
    uart_locate.locate.RSSI            = bRSSI;
    uart_locate.locate.success_rate    = bRate;

    CONVERT_ENDIAN(uart_locate.locate.stationPandID);
    CONVERT_ENDIAN(uart_locate.locate.stationShortAddr);
    CONVERT_ENDIAN( uart_locate.locate.Locate_Distance);

    PrintfUtil_vPrintf("U tx %d\n",sizeof(uart_locate));
    PrintfUtil_vPrintMem((uint8*)&uart_locate,sizeof(uart_locate));

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
		/*DBG(PrintfUtil_vPrintf("Get ExitAddr: \n"););
		DBG(PrintfUtil_vPrintMem(puart_sync_ack->ExitAddr, 8););
        CONVERT_ENDIAN(u8ExitAddr.u32L);
        CONVERT_ENDIAN(u8ExitAddr.u32H);
		DBG(PrintfUtil_vPrintf("u8ExitAddr: %x %x\n", u8ExitAddr.u32L, u8ExitAddr.u32H););
        psMacAddr = u8ExitAddr;
        uint32 tmp32;
        tmp32 = StringUtil_BCD2uint32(psMacAddr.u32H);
        u16CardShortAddr = tmp32%100000;
        u16CardPanId = TOF_CARD_NWK_ADDR;
        u8BroadcastChannel = ((psMacAddr.u32L) >> 16) & 0xFF;
        eAppApiPlmeSet(PHY_PIB_ATTR_CURRENT_CHANNEL, u8BroadcastChannel);
        DBG(PrintfUtil_vPrintf("sAddr:%d, BChannel:%d\n",u16CardShortAddr, u8BroadcastChannel););*/
        break;
    }
    case POWEROFF:
    {
        PrintfUtil_vPrintf("Sleep: \n");
        vAHI_UartDisable(E_AHI_UART_0);
        vAHI_UartDisable(E_AHI_UART_1);

        vAHI_DioWakeEnable(0, TOF_OK_PIN);	// need disable OK button's wakeup ability when sleep
        //SleepUtil_Sleep(E_AHI_WAKE_TIMER_0, u16SleepMs, E_AHI_SLEEP_OSCON_RAMON);
        TimerUtil_vStopAllTimer();
        EventUtil_vResetAllEvents();
        //float fErrorCal = 10000.0 / u32AHI_WakeTimerCalibrate();
        //vAHI_WakeTimerStartLarge(u8Timer, (uint64)(fErrorCal * (u32SleepMS<<5))); //32000 per second;
        //vAHI_Sleep(E_AHI_WAKE_TIMER_0);
        vAHI_Sleep(E_AHI_SLEEP_DEEP);
        break;
    }
    }
}
/****************************************************************************/
/***        END OF FILE                                                   ***/
/****************************************************************************/





