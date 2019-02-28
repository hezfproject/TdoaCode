
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

#include "GasLocate_protocol_V2.h"
#include "config.h"
#include "app_protocol.h"
#include "JN5148_util.h"
#include "printf_util.h"
#include "gas_card_uart.h"
#include "crc.h"

#include "system_util.h"

#include "timer_util.h"
/****************************************************************************/
/***        Macro Definitions                                             ***/
/****************************************************************************/

#if (defined DEBUG_GASCARDLOCATOR)
#define DBG(x) do{x} while(0);
#else
#define DBG(x) do{x} while(0);
#endif
#define CONVERT_ENDIAN(x)	SysUtil_vConvertEndian(&(x), sizeof(x))

#define TOF_VDD_3600		1547	// 3.6v
#define TOF_VDD_3800		1630	// 3.8v

#define TOF_OK_PIN			E_AHI_DIO15_INT
#define TOF_HELP_PIN		E_AHI_DIO14_INT
#define TOF_CHECKIN_PIN	E_AHI_DIO10_INT
//#define TOF_BUZZER_PIN		E_AHI_DIO13_INT

#define TOF_BUZZER_TIMER	E_AHI_TIMER_1	// for buzzer's input

#define TOF_OAD_EVENT					BIT(3)	// the card's OAD event
#define TOF_REQADDR_EVENT			    BIT(4)	// req address after restart

#define TOF_REQVER_EVENT			BIT(5)	// TX the card's VERSION
//#define TOF_LED_GREEN_OFF_EVENT		BIT(6)	// the card's green LED off
#define TOF_CHECK_VDD_EVENT			BIT(7)	// check battery
#define TOF_SLEEP_EVENT				BIT(9)	// sleep
#define TOF_CHECK_HELP_EVENT			BIT(10)	// check if the help button is press or not
#define TOF_HELP_EVENT					BIT(11)	// the gasnode is helping
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
//#define TOF_BUZZER_EVENT				BIT(27)	// card buzzer
//#define TOF_STOP_BUZZER_EVENT			BIT(28)	// stop buzzer
#define TOF_WATCHDOG_EVENT			BIT(30)	// feed watch dog event

//#define TOF_HELP_BUTTON_MS			1000	// when press help button for 1s, will trick the help event
#define TOF_CARD_HELP_TIMES			20		// help 10 times (1 second per time)
#define TOF_CARD_CHECKIN_TIMES		4		// card re-checkin 4 times

#define TOF_GasNode_SYNC_DELTA_MAX		40		// card wakeup at most TOF_CARD_SYNC_DELTA_MAX ms before station do TOF
#define TOF_GasNode_SYNC_DELTA_MIN		10		// card wakeup at least TOF_CARD_SYNC_DELTA_MIN ms before station do TOF

#define TOF_RETREAT_PERIOD_MS			500 		// card flash and buzzer every 500ms
#define TOF_CARD_HELP_PERIOD_MS		500 		// LED flash every 500ms
#define TOF_LED_RED_MS					10		// red led on TOF_LED_RED_MS ms
#define TOF_LED_GREEN_MS				10		// green led on TOF_LED_RED_MS ms

#define TOF_CHECKIN_TIMEOUT			(TOF_SLOT_MS*2+3)	// checkin timeout
#define TOF_LISTEN_TIMEOUT				(TOF_SLOT_MS*2+3)	// listen timeout
#define TOF_ALARM_TIMEOUT				(TOF_SLOT_MS*6+3)	// alarm timeout
#define TOF_SYNC_TIMEOUT				(TOF_SLOT_MS*6+3)	// when lost station, or wakeup from button, need sync with station
#define TOF_RSSI_TIMEOUT				3					// need 3ms to send rssi
#define TOF_PENDING_TIMEOUT			(20+TOF_GasNode_SYNC_DELTA_MAX)		// timeout: waiting for station to do tof
#define TOF_REQUEST_TIMEOUT			20					// send request to join's timeout
#define TOF_BUZZER_TIMEOUT			200					// buzzer timeout

#define MAX_STATION_STATUS_NORMAL   1
#define PARAM_STORE_ADDR 	0x70000

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
    uint16 StationPanid;
    uint16 StationShortAddr;
    uint8      u8LinkQuality;    /**< Link quality of received frame */
}RxFrameData_List;

RfTofWrapper_tu* psAppPkt_Temp = NULL;


RxFrameData_List LQ_StationPanid;
uart_locate_t uart_locate;
//Uart_Header_t uart_header;
uart_request_t uart_request;
uart_ack_t uart_ack;
PRIVATE uart_version_t uart_version;





/****************************************************************************/
/***        Local Function Prototypes                                     ***/
/****************************************************************************/
PRIVATE void vInitGasNodeSystem(void);
PRIVATE void vInitParam();
PRIVATE void vProcessIncomingMlme(MAC_MlmeDcfmInd_s *psMlmeInd);
PRIVATE void vProcessIncomingMcps(MAC_McpsDcfmInd_s *psMcpsInd);
PRIVATE void vProcessIncomingHwEvent(AppQApiHwInd_s *psAHI_Ind);
PRIVATE void vHandleMcpsDataInd(MAC_McpsDcfmInd_s *psMcpsInd);
PRIVATE void vHandleMcpsDataDcfm(MAC_McpsDcfmInd_s *psMcpsInd);
PRIVATE void vGasNodeDeviceStartup();
PRIVATE void vProcessSysEvent();
PRIVATE void vProcessAppEvent();

PRIVATE void vGasNodeCast(uint8 u8CmdType, uint16 u16DstAddr, uint16 u16DstPanId, uint8 u8TxOptions, uint16 u16value);
PRIVATE void vSendRssi();
PRIVATE void vSendAlarm(uint16 u16ShortAddr, uint16 u16PanId);
PRIVATE void vUart1_Sent_Ack(uint8 u8AckType);
PRIVATE void vGasNodeSleep(uint16 u16SleepMs);
//PRIVATE void vSetLed(uint32 u32Pin, bool_t bOnOff, uint16 u16OnMs);
PRIVATE void vSetGasNodeChannel(uint8 channel);
PRIVATE void vSyncStation(RfTofWrapper_tu* psAppPkt);
//PRIVATE void vBuzzer(uint32 u32Ms);
//PRIVATE void vStopBuzzer();
PRIVATE void vCheckStationStatus(uint8 u8StationStatus);
//PRIVATE void vDioCallBack(uint32 u32Device, uint32 u32ItemBitmap);
//PRIVATE bool  vSaveGasNodeStatus(uint8 CardStatus);
PRIVATE void vConvertEndian16(uint16* pu16Data);
PRIVATE void vUart1_Sent_Req(uint8 u8Req);
PRIVATE void vUart1_Sent_Loc(uint8 u8Loc,uint16 LocateDistance);
PRIVATE void vUart1_Sent_Ver(uint8 u8Ver);
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
//PRIVATE MAC_ExtAddr_s psMacAddr;

// gasnode's short addr / pan ID, and station's short addr / pan ID
PRIVATE uint16 u16GasNodeShortAddr;
PRIVATE uint16 u16GasNodePanId;
PRIVATE uint16 u16StationShortAddr;
PRIVATE uint16 u16StationPanId;

PRIVATE uint8 u8StationChannel;		// channel for station do tof with card
PRIVATE uint8 u8LocatorChannel;		// channel for locator do tof with card
PRIVATE uint8 u8BroadcastChannel;	// channel for station to send signal msg
PRIVATE uint8 u8CurChannel;			// gasnode's current channel

PRIVATE uint32 u32WakeupEvent = TOF_LISTEN_EVENT;	// card wakeup event
PRIVATE uint8 u8NeedLocator = 0;		// if need locator

PRIVATE uint8 u16GasNodeSlot;			// the station slot index for me, use this to quickly sync with station if need
PRIVATE uint16 u16SeqNum = 0;		// gasnode's sequence number
PRIVATE uint16 u16JN2TISeqNum = 0;    // jennic to 2440 sequence number
PRIVATE uint8 u8LostTimes = TOF_STATION_CARD_MAX_FAILED;	// to record the times that card lost station

PRIVATE bool_t bTofConnected = FALSE;		// connect with station or not
PRIVATE bool_t bRSSIed = FALSE;			// weather card send rssi msg during the sleep period
//PRIVATE uint16 u16Battery = 0;				// battery power, unit is  0.1v
//PRIVATE bool_t bReportNoPwd = FALSE;		// report no power success or not
PRIVATE bool_t bRetreatAck = FALSE;		// if station retreat, use need to press OK button to stop buzzer
//PRIVATE bool_t bHelped = FALSE;			// station receive gasnode's help or not
PRIVATE bool_t bHelpSync = FALSE;			// is the sync is from gasnode help event

PRIVATE uint8 u8HelpCnt = TOF_CARD_HELP_TIMES;	// couter for help times
PRIVATE uint8 u8CheckinCnt = 0; 					// couter for checkin

PRIVATE teCardState eCardTofState;					// gasnode's state
PRIVATE uint8 u8GasNodeStatus = GASNODE_STATUS_NORMAL;	// gasnode's status, should be normal, help, nopwd, retreat
PRIVATE uint8 u8GasNodeStatusTemp;
PRIVATE uint8 u8LastHandle;				// to record the last handle of the sending msg
PRIVATE uint16 u16GasDensity = 0xFFFF;
PRIVATE uint16 u16GasThr = 0xFFFF;

PRIVATE uint32 u32WaitLocatorTimeout;	// timeout for wait locator to do TOF
PRIVATE uint32 u32LocatorRecTick = 0;		// to record the beginning tick of wait locator
PRIVATE uint32 u32StationRecTick = 0;		// to record the tick that received station's finish msg
//PRIVATE uint16 u16StationConnectPrt = 0;
PRIVATE uint16 u16StationMs = TOF_STATION_PERIOD_MS;		// the coming time of my next slot
PRIVATE uint16 u16LocatorMs;				// the coming time of the locator
PRIVATE uint8	u8StationRunMs;				// the station has ran ms in current slot
PRIVATE uint8	u16SleepDelta;
PRIVATE uint8	u16WakeDelta = TOF_GasNode_SYNC_DELTA_MAX;
PRIVATE uint8 u8ProtectCounter = 0;
//PRIVATE uint8 u8AwayStationTimes = 0;
PRIVATE uint8 u8GasNodeStatusNormal = 0;

PRIVATE bool   bSyncedAddr = FALSE;
PRIVATE bool   bSyncedVer = FALSE;
PRIVATE uint8 u8HelpCntError = 0;
PRIVATE uint32 u16SendVersionCnt = 0;

PRIVATE signed char i8DispalyRssi = -127;


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
    vInitGasNodeSystem();
    vInitParam();
    vGasNodeDeviceStartup();

    EventUtil_vSetEvent(TOF_REQVER_EVENT);
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
 * NAME: vInitGasNodeSystem
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
PRIVATE void vInitGasNodeSystem(void)
{
    (void)u32AppQApiInit(NULL, NULL, NULL);
    (void)u32AHI_Init();

    DBG(
        PrintfUtil_u8Init(E_AHI_UART_0, PRINTF_BAUD_RATE_1M);
        PrintfUtil_vPrintf("cold start\n");
    )

    bAHI_FlashInit(E_FL_CHIP_ST_M25P40_A, NULL);

    u16StationMs = TOF_STATION_PERIOD_MS;
    u16SleepDelta = TOF_GasNode_SYNC_DELTA_MAX;

    strncpy((uint8*)uart_version.version.version, VERSION,16);
    

    vAHI_WakeTimerEnable(E_AHI_WAKE_TIMER_0, TRUE);

    /*MacUtil_vReadExtAddress(&psMacAddr);

    uint32 tmp32;
    tmp32 = StringUtil_BCD2uint32(psMacAddr.u32H);

    u16GasNodeShortAddr = tmp32%100000;

    u16GasNodePanId = TOF_CARD_NWK_ADDR;

    uint8 channel = ((psMacAddr.u32L) >> 16) & 0xFF;

    if(channel >=11 && channel <=26)
    {
        u8BroadcastChannel = channel;
    }
    else
    {
        u8BroadcastChannel = DEFAULT_CHANNEL_BROADCAST;
    }

    bAccurateMode = ((psMacAddr.u32L >> 24) & 0xFF)? TRUE : FALSE;

    u8CurChannel = u8BroadcastChannel;*/
//	u8LocatorChannel = ((psMacAddr.u32L) >> 16) & 0xFF;

    // ***************************************************/
    MacUtil_Setting_s sMacUtil;
    sMacUtil.u16SrcPanId		= TOF_CARD_NWK_ADDR;
    sMacUtil.u8Dst_endpoint 	= 0x21;
    sMacUtil.u8Src_endpoint 	= 0x20;   // ??
    sMacUtil.u16Profile_id 		= 0x2001; //0x2001; //for backward compatable
    sMacUtil.u8NodeType 		= 0x02;
    sMacUtil.u16SrcShortAddr 	= u16GasNodeShortAddr;

    MacUtil_vInit(&sMacUtil);

    //TX1EN
    vAHI_DioSetDirection(0, E_AHI_DIO11_INT);
    vAHI_DioSetOutput(E_AHI_DIO11_INT, 0);
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
    (void)u32AppQApiInit(NULL, NULL, NULL);
    (void)u32AHI_Init();
    DBG(
        PrintfUtil_u8Init(E_AHI_UART_0, PRINTF_BAUD_RATE_1M);
        PrintfUtil_vPrintf("V: %d, Seq: %d\n", OAD_CARD_VERSION, u16SeqNum);
    )
	vUart_Init(E_AHI_UART_1, UART_BAUTRATE_115200, vUart1_txCallBack, vUart1_rxCallBack);

    // need enable card's OK button when wakeup (disable when sleep)

    u8ProtectCounter = 0;
    u16SeqNum++;
    //bHelped = FALSE;
    bHelpSync = FALSE;
    vInitParam();
    vGasNodeDeviceStartup();

    if(u16SendVersionCnt>0)
    {
        u16SendVersionCnt--;
    }
    if(u32WakeupEvent != TOF_LOCATOR_EVENT)
    {
        vUart1_Sent_Req(REQ);
        i8DispalyRssi = -127;
    }
    EventUtil_vSetEvent(u32WakeupEvent);
    
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
 * NAME: vGasNodeDeviceStartup
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
PRIVATE void vGasNodeDeviceStartup()
{
    vAppApiTofInit(FALSE);
    vAppApiTofInit(TRUE);

    vAHI_HighPowerModuleEnable(TRUE, TRUE);
    TimerUtil_vInit();

    s_pvMac = pvAppApiGetMacHandle();
    s_psMacPib = MAC_psPibGetHandle(s_pvMac);

    MAC_vPibSetRxOnWhenIdle(s_pvMac, TRUE, FALSE);
    MAC_vPibSetMaxCsmaBackoffs(s_pvMac, 2);
    MAC_vPibSetMinBe(s_pvMac, 1);

    MAC_vPibSetPanId(s_pvMac, u16GasNodePanId);
    MAC_vPibSetShortAddr(s_pvMac, u16GasNodeShortAddr);

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

PRIVATE void vConvertEndian16(uint16* pu16Data)
{
    SysUtil_vConvertEndian(pu16Data, sizeof(uint16));
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
                && (psAppPkt->rf_tof_oad_data.u8DeviceType == DEVICE_TYPE_CARD)
                && (psAppPkt->rf_tof_oad_data.u16Version > OAD_CARD_VERSION))
        {
            TimerUtil_vStopAllTimer();
            EventUtil_vResetAllEvents();

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

    /*case TOF_STATION_RSSI:
    {
        if((eCardTofState == E_TOF_STATE_ALARM) && ((GASNODE_STATUS_HELP & u8GasNodeStatus)||(GASNODE_STATUS_FIRE & u8GasNodeStatus)||
            (GASNODE_STATUS_WATER & u8GasNodeStatus)||(GASNODE_STATUS_TOPBOARD & u8GasNodeStatus)||(GASNODE_STATUS_OTHER& u8GasNodeStatus)))
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

        if(u8GasNodeStatus & GASNODE_STATUS_RETREAT)
        {
            vUart1_Sent_Loc(LOCATE,0xFFFF);
        }

        break;
    }*/
    
    case TOF_STATION_RSSI:
    case TOF_STATION_AVAILABLE:
    {
        if ((eCardTofState == E_TOF_STATE_LISTEN) || (eCardTofState == E_TOF_STATE_RSSI))
        {
            if(0 == LQ_StationPanid.StationPanid)
            {
                LQ_StationPanid.StationPanid = psFrame->sSrcAddr.u16PanId;
                LQ_StationPanid.StationShortAddr =  psFrame->sSrcAddr.uAddr.u16Short;
                LQ_StationPanid.u8LinkQuality = psFrame->u8LinkQuality;
            }
            else if( LQ_StationPanid.u8LinkQuality < psFrame->u8LinkQuality)
            {
                LQ_StationPanid.StationPanid = psFrame->sSrcAddr.u16PanId;
                LQ_StationPanid.StationShortAddr =  psFrame->sSrcAddr.uAddr.u16Short;
                LQ_StationPanid.u8LinkQuality = psFrame->u8LinkQuality;
            }
        }
        if((eCardTofState == E_TOF_STATE_ALARM) && ((GASNODE_STATUS_HELP & u8GasNodeStatus)||(GASNODE_STATUS_FIRE & u8GasNodeStatus)||
            (GASNODE_STATUS_WATER & u8GasNodeStatus)||(GASNODE_STATUS_TOPBOARD & u8GasNodeStatus)||(GASNODE_STATUS_OTHER & u8GasNodeStatus)))
            vSendAlarm(psFrame->sSrcAddr.uAddr.u16Short, psFrame->sSrcAddr.u16PanId);

        if ((eCardTofState == E_TOF_STATE_SYNC) && (psFrame->sSrcAddr.u16PanId == u16StationPanId) && (psFrame->sSrcAddr.uAddr.u16Short == u16StationShortAddr))
        {
            vSyncStation(psAppPkt);
        }

        i8DispalyRssi = SysUtil_vConvertLQI2Dbm(psFrame->u8LinkQuality);
        vCheckStationStatus(psAppPkt->rf_tof_station_signal.u8StationStatus);
        if(u8GasNodeStatus & GASNODE_STATUS_RETREAT)
        {
            vUart1_Sent_Loc(LOCATE,0xFFFF);
        }
        break;
    }

    case TOF_STATION_FINISH:
    {
        DBG(
            PrintfUtil_vPrintf("station_fin: %d\n", eCardTofState);
        )

        vGasNodeCast(TOF_GASDENSITY, u16StationShortAddr, u16StationPanId,MAC_TX_OPTION_ACK, 0);

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

            // if card receive station_finish, need to report alarm status,
            // else, card must send out rssi msg, which include alarm status already
            if(u8GasNodeStatus & (GASNODE_STATUS_HELP | GASNODE_STATUS_FIRE |GASNODE_STATUS_WATER|GASNODE_STATUS_TOPBOARD|GASNODE_STATUS_OTHER))
            {
                EventUtil_vSetEvent(TOF_ALARM_EVENT);
                TimerUtil_eSetTimer(TOF_STATION_DONE_EVENT, TOF_ALARM_TIMEOUT+10);
            }
            else
                EventUtil_vSetEvent(TOF_CHECK_VDD_EVENT);
        }
        //vSetGasNodeChannel(u8BroadcastChannel);

        uart_locate.locate.i8Rssi = SysUtil_vConvertLQI2Dbm(psFrame->u8LinkQuality);
        i8DispalyRssi = uart_locate.locate.i8Rssi;
        vCheckStationStatus(psAppPkt->rf_tof_station_finish.u8StationStatus);
        vUart1_Sent_Loc(LOCATE,psAppPkt->rf_tof_station_finish.u16Dist2Station);
        break;
    }

    case TOF_STATION_ACCEPT:	// The station accept some card's join request
    {
        if(eCardTofState == E_TOF_STATE_REQUEST_STATION)
        {
            if(psAppPkt->rf_tof_station_accept.tsAccptData[0].u16ShortAddr == u16GasNodeShortAddr)
            {
                //TimerUtil_eStopTimer(TOF_LISTEN_EVENT);	// if request timeout, listen again
                //EventUtil_vUnsetEvent(TOF_LISTEN_EVENT);
                TimerUtil_eStopTimer(TOF_CHECK_VDD_EVENT);                     // if request timeout, go to sleep
                EventUtil_vUnsetEvent(TOF_CHECK_VDD_EVENT);
                TimerUtil_eStopTimer(TOF_STATION_DONE_EVENT);
                EventUtil_vUnsetEvent(TOF_STATION_DONE_EVENT);

                eCardTofState 	= E_TOF_STATE_STATION_ACCEPT;
                bTofConnected 	= TRUE;
                u8LostTimes		= 0;
                u16LocatorMs 	= 0;
                u8NeedLocator	= 0;
                u16SleepDelta 	= TOF_GasNode_SYNC_DELTA_MAX;

                u8StationChannel = psAppPkt->rf_tof_station_accept.u8StationChannel;
                u8LocatorChannel = psAppPkt->rf_tof_station_accept.u8LocatorCardChannel;
                u16GasNodeSlot = psAppPkt->rf_tof_station_accept.tsAccptData[0].u16SlotIndex;

                u16StationMs = ((u16GasNodeSlot + TOF_SLOT_NUM - psAppPkt->rf_tof_station_accept.u16CurSlot)*TOF_SLOT_MS - (uint16)(psAppPkt->rf_tof_station_accept.u8RunMs)) % TOF_STATION_PERIOD_MS;

                EventUtil_vSetEvent(TOF_SLEEP_EVENT);

                DBG(
                    PrintfUtil_vPrintf("accept, slot: %d, run: %d, sleep: %d,CurSolt:%d\n", u16GasNodeSlot, psAppPkt->rf_tof_station_accept.u8RunMs, u16StationMs,psAppPkt->rf_tof_station_accept.u16CurSlot);
                )
            }
        }

        break;
    }

    case TOF_GASNODE_HELP_ACK:
    {
        vUart1_Sent_Ack(STATUS_HELP);
        u8GasNodeStatus &= (~GASNODE_STATUS_HELP);
        break;
    }

    case TOF_GASNODE_FIRE_ACK:
    {
        vUart1_Sent_Ack(STATUS_FIRE);
        u8GasNodeStatus &= (~GASNODE_STATUS_FIRE);
        break;
    }

    case TOF_GASNODE_WATER_ACK:
    {
        vUart1_Sent_Ack(STATUS_WATER);
        u8GasNodeStatus &= (~GASNODE_STATUS_WATER);
        break;
    }

    case TOF_GASNODE_TOPBOARD_ACK:
    {
        vUart1_Sent_Ack(STATUS_TOPBOARD);
        u8GasNodeStatus &= (~GASNODE_STATUS_TOPBOARD);
        break;
    }

    case TOF_GASNODE_OTHER_ACK:
    {
        vUart1_Sent_Ack(STATUS_OTHER);
        u8GasNodeStatus &= (~GASNODE_STATUS_OTHER);
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
        if(u8GasNodeStatus&(GASNODE_STATUS_HELP|GASNODE_STATUS_FIRE|GASNODE_STATUS_WATER|GASNODE_STATUS_TOPBOARD|GASNODE_STATUS_OTHER))	// the help button is still pressed
        {
            u8HelpCnt = 0;
            u8HelpCntError = 0;
            bHelpSync = TRUE;
            EventUtil_vSetEvent(TOF_SYNC_EVENT);
        }
        EventUtil_vUnsetEvent(u32EventID);
        break;
    }

    case TOF_CHECKIN_EVENT:		// when checkin successed, it will unset this event
    {
        DBG(
            PrintfUtil_vPrintf("checkin event\n");
        )
        vSetGasNodeChannel(u8BroadcastChannel);
        vGasNodeCast(TOF_CARD_CHECKIN, 0x0000, 0xFFFF, 0, u16SeqNum);

        if(++u8CheckinCnt < TOF_CARD_CHECKIN_TIMES)
            TimerUtil_eSetTimer(TOF_CHECKIN_EVENT, 20);	// checkin every 20 ms

        EventUtil_vUnsetEvent(u32EventID);
        break;
    }

    case TOF_ALARM_EVENT:
    {
        eCardTofState = E_TOF_STATE_ALARM;
        vSetGasNodeChannel(u8BroadcastChannel);

        // use 2 listen timeout, to wait station's rssi / available cmd to send alarm msg, and station will send app_ack when receive this msg
        TimerUtil_eSetTimer(TOF_ALARM_TIMEOUT_EVENT, TOF_ALARM_TIMEOUT);
        EventUtil_vUnsetEvent(u32EventID);
        break;
    }

    case TOF_ALARM_TIMEOUT_EVENT:
    {
        vSetGasNodeChannel(u8BroadcastChannel);

        // card can't find rssi / available msg, maybe there's not non_rssi_station,
        // need to broadcast to all station, the joined station will send app_ack when receive this msg
        vGasNodeCast(TOF_GASNODE_ALARM, 0x0000, 0xFFFF, 0, u16SeqNum);
        EventUtil_vUnsetEvent(u32EventID);
        break;
    }

    case TOF_STATION_EVENT:
    {
        eCardTofState = E_TOF_STATE_WAIT_STATION_TOF;
        vSetGasNodeChannel(u8StationChannel);
        //DBG(
            //PrintfUtil_vPrintf("tof processing tick: %d\n", u32AHI_TickTimerRead());
        //)
//vGasNodeCast(0xAA, 0xF000, u16SeqNum, 0, 0); // for test: record the wakeup time

        TimerUtil_eSetTimer(TOF_SYNC_EVENT, TOF_PENDING_TIMEOUT+17);	// if wait station to do tof timeout, need sync to the station
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
        }

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
            vSetGasNodeChannel(u8BroadcastChannel);

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
        vSetGasNodeChannel(u8BroadcastChannel);
        DBG(
            PrintfUtil_vPrintf("LISTEN tick: %d\n", u32AHI_TickTimerRead());
        );
        //reinit some adjust time
        u16WakeDelta = TOF_GasNode_SYNC_DELTA_MAX;
        u16SleepDelta = TOF_GasNode_SYNC_DELTA_MAX;

        TimerUtil_eSetTimer(TOF_LISTEN_TIMEOUT_EVENT, TOF_LISTEN_TIMEOUT);
        EventUtil_vUnsetEvent(u32EventID);
        break;
    }

    case TOF_LISTEN_TIMEOUT_EVENT:
    {
        uint32 u32MyEvent = 0;
        if(0 == LQ_StationPanid.StationPanid)
        {
            DBG(
            PrintfUtil_vPrintf("listen timeout\n");)
            eCardTofState = E_TOF_STATE_IDLE;
            if(bRSSIed)
                EventUtil_vSetEvent(TOF_CHECK_VDD_EVENT);
            else
            {
                vSendRssi();
                TimerUtil_eSetTimer(TOF_CHECK_VDD_EVENT, TOF_RSSI_TIMEOUT);
            }
        }
        else
        {
            if((eCardTofState == E_TOF_STATE_REQUEST_STATION) // this show that the station accept another card and send available msg again
                    && (LQ_StationPanid.StationPanid == u16StationPanId))
            {
                TimerUtil_eStopTimer(TOF_LISTEN_EVENT);	// if request timeout, listen again, so need to stop it
                EventUtil_vUnsetEvent(TOF_LISTEN_EVENT);
                u32MyEvent = TOF_REQUEST_EVENT;
            }
            else if ((eCardTofState == E_TOF_STATE_LISTEN) || (eCardTofState == E_TOF_STATE_RSSI))
            {
                //TimerUtil_eStopTimer(TOF_LISTEN_TIMEOUT_EVENT);
                //EventUtil_vUnsetEvent(TOF_LISTEN_TIMEOUT_EVENT);
                TimerUtil_eStopTimer(TOF_RSSI_EVENT);
                EventUtil_vUnsetEvent(TOF_RSSI_EVENT);

                u16StationShortAddr = LQ_StationPanid.StationShortAddr;
                u16StationPanId = LQ_StationPanid.StationPanid;
                u32MyEvent = TOF_REQUEST_EVENT;
            }
            else if ((eCardTofState == E_TOF_STATE_SYNC) && (LQ_StationPanid.StationPanid == u16StationPanId) && (LQ_StationPanid.StationShortAddr == u16StationShortAddr))
            {
                vSyncStation(psAppPkt_Temp);
            }

            if(u32MyEvent > 0)
            {
                uint16 u16Tmp = SysUtil_u16GenRndNum() % ((uint16)(psAppPkt_Temp->rf_tof_station_signal.u8AvailableMs));
                if(u16Tmp == 0)
                    EventUtil_vSetEvent(u32MyEvent);
                else
                    TimerUtil_eSetTimer(u32MyEvent, u16Tmp);
            }
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
    //case TOF_RSSI_EVENT:
    case TOF_REQUEST_EVENT:
    {
        eCardTofState = E_TOF_STATE_REQUEST_STATION;
        vSetGasNodeChannel(u8BroadcastChannel);
        DBG(
            PrintfUtil_vPrintf("tof request\n");
        )
        DBG(PrintfUtil_vPrintf("StationAdd: %d,StationPID: %d\n",u16StationShortAddr,u16StationPanId);)
        vGasNodeCast(TOF_GASNODE_REQUEST, u16StationShortAddr, u16StationPanId, 0, u16SeqNum);

        TimerUtil_eSetTimer(TOF_CHECK_VDD_EVENT, TOF_REQUEST_TIMEOUT);
        EventUtil_vUnsetEvent(u32EventID);
        break;
    }

    case TOF_LOCATOR_EVENT:
    {
        eCardTofState = E_TOF_STATE_WAIT_LOCATOR;
        vSetGasNodeChannel(u8LocatorChannel);
        DBG(
            PrintfUtil_vPrintf("wait locator\n");
        )
//vGasNodeCast(0xAA, 0xF000, u16SeqNum, 0, 0); // for test: record the wakeup time
        u32LocatorRecTick = u32AHI_TickTimerRead();

        TimerUtil_eSetTimer(TOF_LOCATOR_DONE_EVENT, u32WaitLocatorTimeout);
        EventUtil_vUnsetEvent(u32EventID);
        break;
    }

    case TOF_CHECK_VDD_EVENT:
    {
        EventUtil_vSetEvent(TOF_STATION_DONE_EVENT);
        EventUtil_vUnsetEvent(u32EventID);
        break;
    }

    case TOF_STATION_DONE_EVENT:
    {
        eCardTofState = E_TOF_STATE_STATION_FINISH;

        if (FALSE == bTofConnected) // tof failed
        {
            //PrintfUtil_vPrintf("Disnec\n");
            u8NeedLocator = 0;
            u16StationMs = TOF_STATION_PERIOD_MS - (uint16)((u32AHI_TickTimerRead()/16000)%TOF_STATION_PERIOD_MS) + u16WakeDelta;

            if (u8LostTimes < TOF_STATION_CARD_MAX_FAILED)
                u8LostTimes++;
        }
        else
        {
            u8LostTimes = 0;
            u16WakeDelta = (uint16)(u32StationRecTick/16000) - (uint16)u8StationRunMs;
            // u16WakeDelta should smaller than TOF_GasNode_SYNC_DELTA_MAX in normal wakeup
            // but DIO wakeup will larger than it, in this case not need to decrease u16SleepDelta because DIO wakeup's wake delta is not determined
            if((u16WakeDelta > TOF_GasNode_SYNC_DELTA_MIN) && (u16WakeDelta < TOF_GasNode_SYNC_DELTA_MAX))
                u16SleepDelta -= MIN((u16WakeDelta - TOF_GasNode_SYNC_DELTA_MIN)/2, 4);
            else if((u16WakeDelta < TOF_GasNode_SYNC_DELTA_MIN))
                u16SleepDelta += MAX((TOF_GasNode_SYNC_DELTA_MIN - u16WakeDelta)/2, 1);

            u16SleepDelta = MAX(TOF_GasNode_SYNC_DELTA_MIN, u16SleepDelta);

            // need to -(uint16)((u32AHI_TickTimerRead() - u32StationRecTick)/16000),
            // because if alarm status, card need change to common channel to send alarm msg when it receive station_finish (u32StationRecTick)
            u16StationMs = TOF_STATION_PERIOD_MS  - (uint16)u8StationRunMs - (uint16)((u32AHI_TickTimerRead() - u32StationRecTick)/16000);
        }
        DBG(
            PrintfUtil_vPrintf("station done: %d, sleep delta: %d, u16WakeDelta: %d, bTofConnected: %d\n", u16StationMs, u16SleepDelta, u16WakeDelta, bTofConnected);
           )

        EventUtil_vSetEvent(TOF_SLEEP_EVENT);
        //TimerUtil_eSetTimer(TOF_SLEEP_EVENT, 20);
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

        if(u8GasNodeStatus & (GASNODE_STATUS_RETREAT|GASNODE_STATUS_HELP|GASNODE_STATUS_FIRE|GASNODE_STATUS_WATER
            |GASNODE_STATUS_TOPBOARD|GASNODE_STATUS_OTHER))  // user didn't press OK button when retreat// still helping
        {
            bOkToSleep = FALSE;

            if((u8HelpCnt < TOF_CARD_HELP_TIMES)) u8HelpCntError++;
            if(u8HelpCntError>5) u8HelpCnt=TOF_CARD_HELP_TIMES+1;

            // In help status, ignore locator because card need to change to common channel to receive help_ack
            if (u8HelpCnt < TOF_CARD_HELP_TIMES)
                u8NeedLocator = 0;
        }
        else
            bOkToSleep = TRUE;

        DBG(
            PrintfUtil_vPrintf("Sleepevent: %d OK:%d,st:%d ack%d: cnt:%d, cnterr:%d\n",
                               u16StationMs,bOkToSleep, u8GasNodeStatus, bRetreatAck, u8HelpCnt, u8HelpCntError);
        )

        if(u8NeedLocator > 0)
        {
            
            if(u8NeedLocator == 1)
                u16LocatorMs = (TOF_SLOT_LOC_INT - (u16GasNodeSlot%TOF_SLOT_LOC_INT)) * TOF_SLOT_MS;
            else
                u16LocatorMs = (TOF_SLOT_LOC_INT - ((u16GasNodeSlot+TOF_SLOT_LOC_INT/2)%TOF_SLOT_LOC_INT)) * TOF_SLOT_MS;
            

            if(u16LocatorMs > (TOF_STATION_PERIOD_MS - (u16StationMs%TOF_STATION_PERIOD_MS)))
            {
                u16LocatorMs -= (TOF_STATION_PERIOD_MS - (u16StationMs%TOF_STATION_PERIOD_MS));
                //PrintfUtil_vPrintf("LOC:%d,Delta:%d\n",u16LocatorMs,u16SleepDelta);
                if ((u16LocatorMs > (u16SleepDelta+20)) && bOkToSleep)
                {
                    //PrintfUtil_vPrintf("LOC33\n");
                    u16StationMs -= u16LocatorMs - u16SleepDelta + 8;
                    u32WakeupEvent = TOF_LOCATOR_EVENT;
                    u32WaitLocatorTimeout = TOF_LOCATOR_PERIOD_MS + u16SleepDelta+20;
                    u16SeqNum--;	// because when wakeup, u16SeqNum++;
                    vGasNodeSleep(u16LocatorMs - u16SleepDelta-20);
                }
                else
                {
                    //PrintfUtil_vPrintf("LOC44\n");
                    u32WaitLocatorTimeout = TOF_LOCATOR_PERIOD_MS + u16LocatorMs+20;
                    EventUtil_vSetEvent(TOF_LOCATOR_EVENT);
                }
            }
            else
            {
                u32WaitLocatorTimeout = TOF_LOCATOR_PERIOD_MS + u16LocatorMs+20;
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
                if (u16StationMs <= 20 + u16SleepDelta) //&& (bTofConnected))
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
                    vGasNodeSleep(u16StationMs - u16SleepDelta-17);
                }
            }
            else
            {
                vUart1_Sent_Req(REQ);
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
        DBG(PrintfUtil_vPrintf("W\n");)
        vAHI_WatchdogRestart();
        EventUtil_vUnsetEvent(u32EventID);
        break;
    }

    case TOF_REQADDR_EVENT:
    {
        if(bSyncedAddr)
        {
            u8CurChannel = u8BroadcastChannel;
            DBG(PrintfUtil_vPrintf("channel: %d\n",u8BroadcastChannel);)
            MacUtil_Setting_s sMacUtil;
            sMacUtil.u16SrcPanId		= TOF_CARD_NWK_ADDR;
            sMacUtil.u8Dst_endpoint 	= 0x21;
            sMacUtil.u8Src_endpoint 	= 0x20;   // ??
            sMacUtil.u16Profile_id 		= 0x2001; //0x2001; //for backward compatable
            sMacUtil.u8NodeType 		= 0x02;
            sMacUtil.u16SrcShortAddr 	= u16GasNodeShortAddr;

            MacUtil_vInit(&sMacUtil);
            vInitParam();
            vGasNodeDeviceStartup();
            TimerUtil_eSetCircleTimer(TOF_WATCHDOG_EVENT, 1000);	// feed watch dog every 1s
            //DBG(PrintfUtil_vPrintf("OK\n");)
            EventUtil_vSetEvent(TOF_LISTEN_EVENT);

        }
        else
        {
            vUart1_Sent_Req(REQ);
            //DBG(PrintfUtil_vPrintf("fail\n");)
            TimerUtil_eSetTimer(TOF_REQADDR_EVENT, 1000);
        }
        EventUtil_vUnsetEvent(u32EventID);
        break;
    }

    case TOF_REQVER_EVENT:
    {
        if(bSyncedVer)
        {
            EventUtil_vSetEvent(TOF_REQADDR_EVENT);
        }
        else
        {
            vUart1_Sent_Ver(RADIO_VERSION);
            TimerUtil_eSetTimer(TOF_REQVER_EVENT,1000);
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
 * NAME: vGasNodeCast
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
PRIVATE void vGasNodeCast(uint8 u8CmdType, uint16 u16DstAddr, uint16 u16DstPanId, uint8 u8TxOptions, uint16 u16value)
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
    //case TOF_GASNODE_REQUEST:
    //case TOF_CARD_RSSI:
    case TOF_CARD_CHECKIN:
    {
        RfTofData.rf_tof_card_data.u16SeqNum = u16value;
        RfTofData.rf_tof_card_data.u8CardStatus = u8GasNodeStatus;
        RfTofData.rf_tof_card_data.u8Reserved = u8HelpCnt;	// this is for test, u8Reserved can be any value

        if(u16SendVersionCnt ==0)   //at most send every 2 minutes
        {
            RfTofData.rf_tof_card_data.u16OadVersion = OAD_CARD_VERSION;
            RfTofData.rf_tof_card_data.u16Battery= 0;//no use u16Battery;
            u16SendVersionCnt = 2*60/5;
        }
        else
        {
            RfTofData.rf_tof_card_data.u16OadVersion = 0;
            RfTofData.rf_tof_card_data.u16Battery = 0;
        }
        DBG(PrintfUtil_vPrintf("ver T%d C%d V%x\n",u8CmdType, u16SendVersionCnt, OAD_CARD_VERSION););
        RfTofData.tof_head.len = sizeof(rf_tof_card_data_ts) - sizeof(app_header_t);
        break;
    }

    case TOF_GASNODE_RSSI:
    case TOF_GASNODE_REQUEST:
    {
        RfTofData.rf_tof_gas_rssi.u16SeqNum = u16value;
        RfTofData.rf_tof_gas_rssi.u8CardStatus = u8GasNodeStatus;
        RfTofData.rf_tof_gas_rssi.u16GasDensity = u16GasDensity;
        RfTofData.rf_tof_gas_rssi.u16GasThr = u16GasThr;
        RfTofData.rf_tof_gas_rssi.u8Reserved = u8HelpCnt;
        if(u16SendVersionCnt ==0)   //at most send every 2 minutes
        {
            RfTofData.rf_tof_gas_rssi.u16OadVersion = OAD_CARD_VERSION;
            RfTofData.rf_tof_gas_rssi.u16Battery= 0;
            u16SendVersionCnt = 2*60/5;
        }
        else
        {
            RfTofData.rf_tof_gas_rssi.u16OadVersion = 0;
            RfTofData.rf_tof_gas_rssi.u16Battery = 0;
        }
        RfTofData.tof_head.len = sizeof(rf_tof_gas_rssi_ts) - sizeof(app_header_t);

        break;
    }
    case TOF_GASDENSITY:
    case TOF_GASNODE_ALARM:
    {
        RfTofData.rf_tof_gas_density.u16GasDensity = u16GasDensity;
        RfTofData.rf_tof_gas_density.u16GasThr = u16GasThr;
        RfTofData.rf_tof_gas_density.u8CardStatus = u8GasNodeStatus;
        RfTofData.tof_head.len = sizeof(rf_tof_gas_density_ts) - sizeof(app_header_t);
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
 * NAME: vSaveGasNodeStatus
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
/*bool  vSaveGasNodeStatus(uint8 CardStatus)
{
    uint8 CardStatusTemp;
    if(!bAHI_FullFlashRead(PARAM_STORE_ADDR, sizeof(CardStatusTemp),(uint8*)&CardStatusTemp))
    {
        DBG(PrintfUtil_vPrintf("Flash Read Fail\n"););
        return FALSE;
    }
    if (CardStatus == CardStatusTemp)
    {
        return TRUE;
    }

    if(!bAHI_FlashEraseSector(7))
    {
        DBG(PrintfUtil_vPrintf("Flash Erase Fail\n"););
        return FALSE;
    }

    if(bAHI_FullFlashProgram(PARAM_STORE_ADDR, sizeof(CardStatus),(uint8*)&CardStatus))
    {
        //DBG(PrintfUtil_vPrintf("OK!%d\n", CardStatus););
        return TRUE;
    }
    else
    {
        DBG(PrintfUtil_vPrintf("Set  CardStatus fail!%d\n", CardStatus););
        return FALSE;
    }
}*/



/****************************************************************************
 *
 * NAME: vSetGasNodeChannel
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
PRIVATE void vSetGasNodeChannel(uint8 channel)
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
        vSetGasNodeChannel(u8BroadcastChannel);
        vGasNodeCast(TOF_GASNODE_RSSI, 0x0000, 0xFFFF, 0, u16SeqNum);
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
    vSetGasNodeChannel(u8BroadcastChannel);
    vGasNodeCast(TOF_GASNODE_ALARM, u16ShortAddr, u16PanId, MAC_TX_OPTION_ACK, u16SeqNum);
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
    uint16 u16SleepMs = (TOF_SLOT_MS * (u16GasNodeSlot + TOF_SLOT_NUM - psAppPkt->rf_tof_station_signal.u16CurSlot - 1) - (uint16)(psAppPkt->rf_tof_station_signal.u8RunMs)) % TOF_STATION_PERIOD_MS;

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
        else if((u8GasNodeStatus & GASNODE_STATUS_RETREAT) && (bRetreatAck == FALSE)) 	// user didn't press OK button when retreat
                
        {
            vInitParam();
            vAHI_TickTimerWrite(0);
            TimerUtil_eSetTimer(u32WakeupEvent, u16SleepMs - u16SleepDelta);
        }
        else
        {
            vGasNodeSleep(u16SleepMs - u16SleepDelta);
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
    LQ_StationPanid.StationPanid = 0;
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
        u8GasNodeStatusNormal = 0;
        if(!(u8GasNodeStatus & GASNODE_STATUS_RETREAT))
        {
            u8GasNodeStatus |= GASNODE_STATUS_RETREAT;
            bRetreatAck = FALSE;
        }
    }
    else if (u8StationStatus == STATION_STATUS_NORMAL)
    {
        if((u8GasNodeStatus & GASNODE_STATUS_RETREAT) && (u8GasNodeStatusNormal++ > MAX_STATION_STATUS_NORMAL))
        {
            u8GasNodeStatusNormal = MAX_STATION_STATUS_NORMAL+1;
            u8GasNodeStatus &= (~GASNODE_STATUS_RETREAT);
            //vSaveGasNodeStatus(u8GasNodeStatus);
        }
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


PRIVATE void vGasNodeSleep(uint16 u16SleepMs)
{
    vAHI_UartDisable(E_AHI_UART_0);
    vAHI_UartDisable(E_AHI_UART_1);
    //vAHI_UartDisable(E_AHI_UART_1);
    //vAHI_DioWakeEnable(0, TOF_OK_PIN);	// need disable OK button's wakeup ability when sleep
    SleepUtil_Sleep(E_AHI_WAKE_TIMER_0, u16SleepMs, E_AHI_SLEEP_OSCON_RAMON);
}

PRIVATE void vUart1_txCallBack(void)
{
    ;//bDisReporting = FALSE;
}
PRIVATE void vUart1_rxCallBack(uint8* p, uint16 len)
{
    DBG(PrintfUtil_vPrintf("U rx %d\n", len););
    uint16 crc;
    if(p==NULL || len < sizeof(Uart_Header_t))
    {
        DBG(PrintfUtil_vPrintf("error\n");)
        return;
    }

    //DBG(PrintfUtil_vPrintf("A\n"););
    Uart_Header_t* pHeader = (Uart_Header_t*)p;
    
    /*if((pHeader->header_h != (unsigned char)UART_PREAMBLE_H)||(pHeader->header_l != (unsigned char)UART_PREAMBLE_L))
    {
        DBG(PrintfUtil_vPrintf("error\n");)
        return;
    }*/

    crc = CRC16(((unsigned char *)(pHeader)) + 4,len-4, 0xffff);
    
    
    CONVERT_ENDIAN(pHeader->crc);
    if(crc == pHeader->crc)
    {
        //DBG(PrintfUtil_vPrintf("H:%d,L:%d\n",pHeader->hdr.header_h,pHeader->hdr.header_l);)
        //if((pHeader->hdr.header_h == UART_PREAMBLE_H)&&(pHeader->hdr.header_l == UART_PREAMBLE_L))
        //DBG(PrintfUtil_vPrintf("B\n"););
        {
            if(pHeader->cmdtype == REPORT)
            {
                uart_report_t* pReport = (uart_report_t*)p;
                CONVERT_ENDIAN(pReport->report.shortAddr);
                CONVERT_ENDIAN(pReport->report.gasDensity);
                CONVERT_ENDIAN(pReport->report.alarmGasDensity);
                bSyncedAddr = TRUE;
                u16GasDensity = pReport->report.gasDensity;
                u16GasThr = pReport->report.alarmGasDensity;
                u16GasNodeShortAddr = pReport->report.shortAddr;
                u16GasNodePanId = TOF_CARD_NWK_ADDR;
                if(pReport->report.broadcastChannel>=11 && pReport->report.broadcastChannel<=26)
                {
                    u8BroadcastChannel = pReport->report.broadcastChannel;
                }
                else
                {
                    u8BroadcastChannel = DEFAULT_CHANNEL_BROADCAST;
                }
                u8GasNodeStatus = pReport->report.upStatus;
                if((u8GasNodeStatus != GASNODE_STATUS_NORMAL)&& (u8GasNodeStatus != u8GasNodeStatusTemp))
                {
                    TimerUtil_eSetTimer(TOF_CHECK_HELP_EVENT, 500);
                }
                u8GasNodeStatusTemp = u8GasNodeStatus;
            }
            else if(pHeader->cmdtype == RADIO_VERSION_ACK)
            {
                bSyncedVer = TRUE;
                //DBG(PrintfUtil_vPrintf("C\n"););
            }
        }
    }
    else
    {
        DBG(PrintfUtil_vPrintf("crc error\n");)
    }
}

PRIVATE void vUart1_Sent_Ack(uint8 u8AckType)
{
    uart_ack.hdr.header_h = UART_PREAMBLE_H;
    uart_ack.hdr.header_l = UART_PREAMBLE_L;
    uart_ack.hdr.cmdtype = REPORT_ACK;
    uart_ack.hdr.len = sizeof(Ack_Information_t);
    uart_ack.ack.downStatus = u8AckType;
    uart_ack.hdr.crc = CRC16(((unsigned char *)(&uart_ack))+4, sizeof(uart_ack_t)-4, 0xFFFF);
    vConvertEndian16(&(uart_ack.hdr.crc));
    u8Uart_StartTx(E_AHI_UART_1, (uint8*)&uart_ack, sizeof(uart_ack_t));
}

PRIVATE void vUart1_Sent_Req(uint8 u8Req)
{
    uart_request.hdr.header_h = UART_PREAMBLE_H;
    uart_request.hdr.header_l = UART_PREAMBLE_L;
    uart_request.hdr.cmdtype = u8Req;
    uart_request.hdr.len = sizeof(Req_Information_t);
    uart_request.hdr.padding =0xFFFF;
    if(u8GasNodeStatus & GASNODE_STATUS_RETREAT)
        uart_request.request.downStatus = DOWNSTATUS_RETREAT;
    else
        uart_request.request.downStatus = DOWNSTATUS_NOMAL;
    
    uart_request.request.i8Rssi = i8DispalyRssi;
    
    //vConvertEndian16(&(uart_request.hdr.padding));
    uart_request.hdr.crc = CRC16(((unsigned char *)(&uart_request))+4, sizeof(uart_request_t)-4, 0xFFFF);
    vConvertEndian16(&(uart_request.hdr.crc));
    u8Uart_StartTx(E_AHI_UART_1, (uint8*)&uart_request, sizeof(uart_request_t));
}

PRIVATE void vUart1_Sent_Ver(uint8 u8Ver)
{
    uart_version.hdr.header_h = UART_PREAMBLE_H;
    uart_version.hdr.header_l = UART_PREAMBLE_L;
    uart_version.hdr.cmdtype = u8Ver;
    uart_version.hdr.len = sizeof(Version_Information_t);
    uart_version.hdr.crc = CRC16(((unsigned char *)(&uart_version))+4, sizeof(uart_version_t)-4, 0xFFFF);
    vConvertEndian16(&(uart_version.hdr.crc));
    u8Uart_StartTx(E_AHI_UART_1, (uint8*)&uart_version, sizeof(uart_version_t));
}

PRIVATE void vUart1_Sent_Loc(uint8 u8Loc,uint16 LocateDistance)
{
    uart_locate.hdr.header_h = UART_PREAMBLE_H;
    uart_locate.hdr.header_l = UART_PREAMBLE_L;
    uart_locate.hdr.cmdtype = u8Loc;
    uart_locate.hdr.len = sizeof(Locate_Information_t);
    uart_locate.hdr.padding = 0xFFFF;
    if(0xFFFF == LocateDistance)
    {
        uart_locate.locate.locateIsvalid = 0;
    }
    else
    {
        uart_locate.locate.locateIsvalid = 1;
    }
    if(u8GasNodeStatus & GASNODE_STATUS_RETREAT)
    {
        uart_locate.locate.downStatus = DOWNSTATUS_RETREAT;
    }
    else
    {
        uart_locate.locate.downStatus = DOWNSTATUS_NOMAL;
    }
    u16JN2TISeqNum++;
    uart_locate.locate.seqnum = u16JN2TISeqNum;
    uart_locate.locate.stationPandID = u16StationPanId;
    uart_locate.locate.stationShortAddr = u16StationShortAddr;//u16GasNodeShortAddr;
    uart_locate.locate.locateDistance = LocateDistance;
    uart_locate.locate.padding = 0xFF;
    vConvertEndian16(&(uart_locate.locate.seqnum));
    //vConvertEndian16(&(uart_locate.locate.padding));
    vConvertEndian16(&(uart_locate.hdr.padding));
    vConvertEndian16(&(uart_locate.locate.locateDistance));
    vConvertEndian16(&(uart_locate.locate.stationShortAddr));
    vConvertEndian16(&(uart_locate.locate.stationPandID));
    uart_locate.hdr.crc = CRC16((unsigned char *)(&uart_locate)+4, sizeof(uart_locate_t)-4, 0xFFFF);
    vConvertEndian16(&(uart_locate.hdr.crc));
    u8Uart_StartTx(E_AHI_UART_1, (uint8*)&uart_locate,sizeof(uart_locate_t));
}
/****************************************************************************/
/***        END OF FILE                                                   ***/
/****************************************************************************/





