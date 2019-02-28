
/****************************************************************************/
/***        Include files                                                 ***/
/****************************************************************************/
#include <jendefs.h>
#include <AppHardwareApi.h>
#include <AppQueueApi.h>
#include <mac_sap.h>
#include <mac_pib.h>
#include <AppApiTof.h>
#include <Utilities.h>
#include <OAD.h>

#include "config.h"
#include "app_protocol.h"
#include "bsmac.h"
#include "JN5148_util.h"
#include "crc.h"
#include "Locate_protocol.h"
#include "gas_uart.h"



/****************************************************************************/
/***        Macro Definitions                                             ***/
/****************************************************************************/

//#define DEBUG_RTL_STATION

#if (defined DEBUG_LOC_APP)
#define DBG(x) do{x}while(0);
//#define __STAT_TOF__
#define __STAT_RSSI__
#else
#define DBG(x) do{x}while(0);

//#define DBG(x)
#endif

#define __NEED_LOCATOR__
//#define __TEST_SLOT__
//#define __TEST_RETREAT__

#ifdef __TEST_RETREAT__
#include <button.h>
#endif

#ifdef __STAT_TOF__
#define    ISSTATCARD(x)     ((tsSlot[(x)].u16ShortAddr>=CARD_BASE) && (tsSlot[(x)].u16ShortAddr<CARD_BASE+STAT_LEN) && ((x)%TOF_SLOT_LOC_INT>1))
#define    ISSTATCARD_2    ((u16PendingAddr>=CARD_BASE) && (u16PendingAddr<CARD_BASE+STAT_LEN))
#endif


#define    TOF_SYNCADDR_EVENT                BIT(13)    // to deal with slot timer
#define    STATION_SEND_CARD_INFO_EVENT    BIT(14)     // send card info to locator to indicate which cards need locator
#define    STATION_TOF2CARD_EVENT            BIT(15)     // tof with card
#define    STATION_WAIT_TOF2CARD_EVENT    BIT(16)     // wait tof with card finish
#define    STATION_CLAIM_EVENT                BIT(17)     // station claim IDLE or BUSY
#define    STATION_FIND_EVENT            BIT(18)    // station available, card can join
#define    STATION_RSSI_EVENT                BIT(19)    // card card send rssi
#define    STATION_ACCEPT_EVENT                BIT(20)    // station deal with card join request
#define    STATION_LOC_TOF_EVENT            BIT(21)    // when loc join, need tof to determine loc's distance
#define    STATION_TOF_REPORT_EVENT        BIT(22)    // station report card's distance / rssi
#define    STATION_PORT_QUERY_EVENT        BIT(23)    // query if station up
#define    STATION_REPORT_STATUS              BIT(24)    // query if station up

#define    STATION_WATCHDOG_EVENT            BIT(30)    // feed watch dog

#ifdef __TEST_SLOT__
#define    TEST_SLOT_EVENT    BIT(31)
#endif

#define    STATION_ACCEPT_CARD_MAX_NUM    5    // when station available, only 5 card can join per time
#define     STATION_TOF_MAX_READINGS        2
#define     STATION_TOF_MIN_DISTANCE        15     // when the distance between card and station is small than 15 meters, not need locator
#define     STATION_TOF_LOC_DIST_NUM        10    // when locator join, need STATION_TOF_LOC_DIST_NUM times tof to determine locator's distance

#define    CARD_VELOCITY_MAX                    80    // card's maximun velocity is 16m/s, 80m per period
#define    CARD_VELOCITY_MIN                    5    // card's minimun velocity is 1m/s, 5m per period

#define TOF_OK_PIN			E_AHI_DIO15_INT
#define TOF_HELP_PIN		E_AHI_DIO14_INT
#define TOF_CHECKIN_PIN	E_AHI_DIO10_INT
#define TOF_BUZZER_PIN		E_AHI_DIO13_INT


#define SLOT_CARD             0
#define SLOT_LOCATOR       1
#define SLOT_IDLE              2

#define PARAM_STORE_ADDR 	0x70000
#define POWEROFF      6
#define CONVERT_ENDIAN(x)	SysUtil_vConvertEndian(&(x), sizeof(x))

/****************************************************************************/
/***        Type Definitions                                              ***/
/****************************************************************************/
typedef enum
{
    E_STATE_IDLE,
    E_STATE_STARTED,
    E_STATE_AVAILABLE,
    E_STATE_RSSI,
    E_STATE_ACCEPT,
    E_STATE_TOF,
    E_STATE_LOCATOR,
    E_STATE_BUSY,
} teState;

typedef struct
{
    uint8 u8Direction;    // to flag card's direction, 0: near station, 1: same direction with locator #1, 2: same direction with locator #2, F: direction is not determined
    uint16 u16Val;        // TOF distance
} tsTofDist;   // for card

typedef struct
{
    uint8 b1Used:1;        // 0: idle, 1: used
    uint8 b1TofDir:1;  // a card
    uint8 b2SlotType:2;
    uint8 u8LostNum;        // if u8LostNum >= 3, release this slot
    uint16 u16SeqNum;    // sequence number
    uint16 u16ShortAddr;    // short addr of the device
    uint16 u16PanID;        // Pan id
    uint8 u8LastTofDist;    // 1: last Tof distance success, 0: last Tof distance failed
    uint8 Rate;
    int8  RSSI;

    tsTofDist tsTof[20];        // for card: to record current(tsTof[0]) and last(tsTof[1]) TOF distance, for locator: use tsTof[0] to store locator's distance
} tsTofSlot;

typedef struct
{
    uint16 u16SlotIndex;
    uint16 u16ShortAddr;
} tsCardInfo;

typedef struct
{
    uint8 u8TofChannel;
    uint8 u8LocChannel;
} tsParam_t;

typedef struct
{
    int32   TOF_Value;
    float   TOF_Bias;
}tsAppTof_Data;

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
PRIVATE void vCheckSetting(void);
PRIVATE void vStartingLEDindicator();
PRIVATE void vInitSlots(void);
PRIVATE void vInitStationSystem(void);
PRIVATE void vStartCoordinator(void);
PRIVATE void vUart1_txCallBack(void);
PRIVATE void vProcessSysEventQueues(void);
PRIVATE void vProcessIncomingMlme(MAC_MlmeDcfmInd_s *psMlmeInd);
PRIVATE void vProcessIncomingMcps(MAC_McpsDcfmInd_s *psMcpsInd);
PRIVATE void vProcessIncomingHwEvent(AppQApiHwInd_s *psAHI_Ind);
PRIVATE void vHandleMcpsDataInd(MAC_McpsDcfmInd_s *psMcpsInd);
PRIVATE void vHandleMcpsDataDcfm(MAC_McpsDcfmInd_s *psMcpsInd);
PRIVATE void vProcessAppEventQueues(void);

PRIVATE void vProcessTofClusterData(MAC_McpsIndData_s *psMcpsInd);
PRIVATE void vProcessStationCmd(MAC_McpsIndData_s *psMcpsIndData);
PRIVATE void vReportTofDistance(void);


PRIVATE void vTofCallback(eTofReturn eStatus);
PRIVATE void vTofProcessing();
PRIVATE void vCurrentTofDone();
PRIVATE void vStopTofIndex(void);
PRIVATE void vProcessLocatorCardDist(uint8 u8Len, uint8 u8Loc);
PRIVATE void vCheckCardLost(void);

PRIVATE void vStationCast(uint8 u8CmdType, uint16 u16DstAddr, uint16 u16DstPanId, uint8 u8TxOptions, uint8 u8Val);
PRIVATE void vSendCardInfo();

//PRIVATE uint8 u8GetIdleSlot(uint16 u16Start, uint16 u16ReqShortAddr, uint16* pu16Index);
PRIVATE uint16 u16NextSlotIndex(uint16 u16index);
PRIVATE uint32 u32NextSlotTick(uint32 u32tick);
PRIVATE void vSetStationChannel(uint8 channel);
PRIVATE uint8 u8GetLocIndex(uint16 u16MyIndex);
PRIVATE void vCalVelocity();
PRIVATE void vCheckCardStatus(uint16 u16ShortAddr, uint8 u8Status);
PRIVATE bool bArmReady();
PRIVATE void vBsMac_rx_callback(unsigned char *pbuf, unsigned char len);
PRIVATE void vReportCardDistance();
PRIVATE void vReportCardRssi();
PRIVATE void vReportCardAlarm(uint16 u16ShortAddr, app_tof_alarm_type_te alarm_type);
PRIVATE void vConvertEndian16(uint16* pu16Data);
PRIVATE void vWriteData2Arm(uint8* pbuf);
PRIVATE void vInitialChannel(void);
PRIVATE bool  vSaveChannel(uint8 u8TofChannel, uint8 u8LocChannel);
PRIVATE bool bChannelValid(uint8 u8BroadcastChannel, uint8 u8TofChannel, uint8 u8LocatorCardChannel);
PRIVATE void vUart0_rx_callback(uint32 u32DeviceId, uint32 u32ItemBitmap);
PRIVATE void vInitVariables();
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
PRIVATE uint16 u16StationShortAddr;
PRIVATE uint16 u16StationPanId;
PRIVATE uint16 u16LocatorAddress;
PRIVATE uint8 u8TofChannel;                // station use this channel to do tof with card, and exchange info with loc
PRIVATE uint8 u8BroadcastChannel;    // station use this channel to send signal msg: idle, busy, rssi, available
PRIVATE uint8 u8LocatorCardChannel;    // this channel is for loc to do tof with card
PRIVATE uint8 u8CurChannel;            // to record station's current channel
PRIVATE uint8 u8ExtAddr[8];

PRIVATE int16 i16TofDistTemp[2];


PRIVATE eTofReturn eTofStatus = -1;
PRIVATE tsAppApiTof_Data asTofData[20];

PRIVATE tsAppApiTof_Data Tof_SamplesData[20];


PRIVATE uint8 u8ReSendCardInfo = 0;    // to record the retry times that send card info to loc
PRIVATE bool_t bSendCardInfoDone = TRUE;    // send card info to loc success or not
PRIVATE bool_t bRxCardDistanceDone;    // receive card's distance from loc
PRIVATE bool_t bTofBusy = FALSE;
PRIVATE tof_station_status_te tStationStatus = STATION_STATUS_NORMAL;
PRIVATE tsTofSlot tsSlot;
PRIVATE uint16 u16AcceptLocSlot;

PRIVATE tsCardInfo tsCardInfoAddr[CARD_INFO_LEN];            // record the info of those cards that need loc
PRIVATE tsCardInfo tsPreCardInfoAddr[2][CARD_INFO_LEN];    // last loc period's card info, for 2 locs
PRIVATE uint16 u16CardDistance[CARD_INFO_LEN];
PRIVATE uint8 u8CardInfoLen = 0;

PRIVATE uint32 u32SlotTick;        // current slot tick
PRIVATE uint16 u16SlotIndex;        // current slot index
PRIVATE uint32 u32TofSlotTick;        // the slot tick which is doing tof
PRIVATE uint16 u16TofSlotIndex;     // the slot index which is doing tof
PRIVATE uint16 u16PendingAddr;    // the pending card which will do tof

PRIVATE bool_t bMsgSending = FALSE;    // if the msg is still sending or not
PRIVATE uint8 u8LastHandle;
PRIVATE uint8 u8Retries = 0;
PRIVATE uint8 u8AcceptNum = 0;
PRIVATE tsCardInfo tsAccepted;
PRIVATE teState teStaionState = E_STATE_IDLE;
PRIVATE uint8 u8LocTofTimes[2];    // when locator join, station need determine the distance to the locator, do tof with STATION_TOF_LOC_DIST_NUM times
PRIVATE int16 i16LocTofSum[2];    // temporary sum for average
PRIVATE uint8 u8LocDist[2];
PRIVATE uint32 u32LocID[2];

PRIVATE uint32 u32Data2Arm[30];    // the data send to arm, max len is 114 (BSMAC_MAX_TX_PAYLOAD_LEN), 30*4 = 120 is enough (28 is enough)
PRIVATE struct nwkhdr* pnwk_data = (struct nwkhdr*)u32Data2Arm;
PRIVATE uint16 u16ArmId;
PRIVATE uint16 u16RssiNum = 0;
app_tof_distance_ts app_tof_rssi_data;

// u16IdleSlot: these slots should never be used, they are for other use (station signal)
PRIVATE uint16 u16IdleSlot[TOF_SLOT_IDLE_NUM] = {6, 13, 20};

/*use u32 for alignment*/
PRIVATE uint32 u32CommonBuf[40];
PRIVATE uint8 u8LiveSeqNum = 0;
PRIVATE uint8 u8LastLiveAckSeq = 0;
PRIVATE uint8 u8ReportStatusNum = 0;
PRIVATE bool bIsStationUp = FALSE;
PRIVATE uint8 u8RestartCounter=0;

PRIVATE bool_t bIsTofSuccess = FALSE;
PRIVATE bool_t bDetailedPrint = FALSE;

#ifdef __STAT_TOF__
PRIVATE void vPrintStat();
#define STAT_LEN     200
#define CARD_BASE    0x0A00
PRIVATE volatile bool_t bRndNumGenerated = FALSE;
PRIVATE uint16 u16StatIndex;
PRIVATE uint16 u16StatCount;
PRIVATE uint32 u32StatTofNum[STAT_LEN];
PRIVATE uint32 u32StatTofFailed[STAT_LEN];
PRIVATE uint32 u32StatTofSkip[STAT_LEN];
PRIVATE uint32 u32StatTofSuccess[STAT_LEN];
PRIVATE uint32 u32StatTofUnCom[STAT_LEN];    // uncompleted, stop by vAppApiTofInit(FALSE)
PRIVATE uint32 u32StatTofLocatorNum[STAT_LEN];
PRIVATE uint32 u32StatTofLocatorSuccess[STAT_LEN];
#endif

#ifdef __TEST_RETREAT__
PRIVATE uint8 u8Keys;
#endif

PRIVATE bool   bSyncedAddr = FALSE;
PRIVATE bool   bSysStarted = FALSE;
PRIVATE MAC_ExtAddr_s  u8ExitAddr;
PRIVATE volatile bool   bDisReporting;
PRIVATE uint8 counts = 0;


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
    vInitStationSystem();
    vStartCoordinator();
    PrintfUtil_vPrintf("LOC System started. \n\r");
    PrintfUtil_vPrintf("station id %d\n", u16StationPanId);
    if(bAHI_WatchdogResetEvent()) PrintfUtil_vPrintf("Watchdog reset!!!\n");
    vOADInitClient(TRUE);
    vStartingLEDindicator();
    vInitVariables();
    TimerUtil_eSetCircleTimer(STATION_WATCHDOG_EVENT, 1000);    // feed watch dog every 1 second
    EventUtil_vSetEvent(TOF_SYNCADDR_EVENT);
    vAHI_TickTimerWrite(0);
    while (1)
    {
        TimerUtil_vUpdate();
        Jbsmac_vPoll();
        vProcessSysEventQueues();
        vProcessAppEventQueues();
    }
}

/****************************************************************************
 *
 * NAME: AppWarmStart
 *
 ****************************************************************************/
PUBLIC void AppWarmStart(void)
{
    PrintfUtil_u8Init(E_AHI_UART_0, PRINTF_BAUD_RATE_1M);
    DBG(
        PrintfUtil_vPrintf("warm start. \n\r");
    )
    AppColdStart();
}

/****************************************************************************/
/***        Local Functions                                               ***/
/****************************************************************************/

/****************************************************************************
 *
 * NAME: vInitStationSystem
 *
 * DESCRIPTION:
 * Init system when cold start, includes mac settings, tof / timer / high power mode functions
 *
 * PARAMETERS:      None
 *
 * RETURNS:
 * void
 *
 ****************************************************************************/
PRIVATE void vInitStationSystem(void)
{
    (void)u32AppQApiInit(NULL, NULL, NULL);
    (void)u32AHI_Init();
    DBG(PrintfUtil_u8Init(E_AHI_UART_0, PRINTF_BAUD_RATE_1M);
        PrintfUtil_vPrintf("cold start\n");)
    vAHI_DioSetDirection(0, JN_RED | JN_GREEN);
    vAHI_DioSetDirection(TOF_HELP_PIN | TOF_CHECKIN_PIN | TOF_OK_PIN, 0);
    vAHI_DioWakeEnable(TOF_HELP_PIN | TOF_CHECKIN_PIN | TOF_OK_PIN, 0);
    vAHI_DioWakeEdge(0, TOF_HELP_PIN | TOF_CHECKIN_PIN | TOF_OK_PIN);
    vAHI_DioSetPullup(TOF_CHECKIN_PIN, TOF_BUZZER_PIN);
    vAHI_WakeTimerEnable(E_AHI_WAKE_TIMER_0, TRUE);
    vAHI_HighPowerModuleEnable(TRUE, TRUE);  // Enable high power modules, tof function, timerUtil
    vAppApiTofInit(TRUE);
    TimerUtil_vInit();
    vInitialChannel();
    pnwk_data->type = NWK_DATA;
    pnwk_data->ttl = 1;
    pnwk_data->src = u16StationPanId;
    vConvertEndian16(&(pnwk_data->src));
    app_tof_rssi_data.app_tof_head.protocoltype = APP_PROTOCOL_TYPE_CARD;
    app_tof_rssi_data.app_tof_head.msgtype = APP_TOF_MSG_RSSI;
    Jbsmac_u8Init(vBsMac_rx_callback,E_AHI_UART_1, BSMAC_UART_BAUD_DIVISOR_500k, u16StationPanId, BSMAC_DEVICE_TYPE_LOC);
    memcpy(u8ExtAddr, (void*)&psMacAddr.u32L, 4);
    memcpy(u8ExtAddr+4, (void*)&psMacAddr.u32H, 4);
    SysUtil_vConvertEndian(u8ExtAddr, 4);
    SysUtil_vConvertEndian(u8ExtAddr+4, 4);
    /* Set up the MAC handles. Must be called AFTER u32AppQApiInit() */
    s_pvMac = pvAppApiGetMacHandle();
    s_psMacPib = MAC_psPibGetHandle(s_pvMac);
    MAC_vPibSetPanId(s_pvMac, u16StationPanId);
    MAC_vPibSetShortAddr(s_pvMac, u16StationShortAddr);
    MAC_vPibSetRxOnWhenIdle(s_pvMac, TRUE, FALSE);
    MAC_vPibSetMaxCsmaBackoffs(s_pvMac, 2);
    MAC_vPibSetMinBe(s_pvMac, 1);
    /* NOT allow nodes to associate */
    s_psMacPib->bAssociationPermit = 0;
    s_psMacPib->bAutoRequest=0;
    s_psMacPib->bGtsPermit = FALSE;
    //FIXME
    s_psMacPib->u8MaxFrameRetries = 1;
    MacUtil_Setting_s sMacUtil;
    sMacUtil.u16SrcPanId         = u16StationPanId;
    sMacUtil.u16SrcShortAddr     = u16StationShortAddr;
    // the following init are fake, because station is 802.15.4
    sMacUtil.u16Profile_id         = 0x2001; //for backward compatable
    sMacUtil.u8Dst_endpoint     = 0x21;
    sMacUtil.u8NodeType         = 0;
    sMacUtil.u8Src_endpoint     = 0;
    MacUtil_vInit(&sMacUtil);
    eAppApiPlmeSet(PHY_PIB_ATTR_CURRENT_CHANNEL, u8BroadcastChannel);
    vUart_Init(E_AHI_UART_1, UART_BAUTRATE_115200, vUart1_txCallBack, vUart1_rxCallBack);
}

PRIVATE void vStartingLEDindicator()
{
    LedUtil_vFlashAll(1000, 3);
    LedUtil_vFlashAll(3000, 1);
}

PRIVATE void vInitVariables(void)
{
    tsSlot.b1Used = 0;
    tsSlot.b2SlotType = SLOT_CARD;
    tsSlot.b1TofDir = API_TOF_FORWARDS;
    tsSlot.Rate = 0;
    tsSlot.RSSI = 0;
}

/****************************************************************************
 *
 * NAME: vProcessSysEventQueues
 *
 * DESCRIPTION:
 * Process system's event queues.
 *
 * PARAMETERS:      None
 *
 * RETURNS:
 * void
 *
 ****************************************************************************/
PRIVATE void vProcessSysEventQueues(void)
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


PRIVATE double grubbs_lookup[] = {1.15, 1.46, 1.67, 1.82, 1.94, 2.03, 2.11, 2.18, 2.23, 2.28, 2.33, 2.37, 2.41, 2.44, 2.48, 2.5, \
                                2.53, 2.56, 2.58, 2.6, 2.62, 2.64, 2.66, 2.74, 2.81, 2.87, 2.96, 3.17};

PUBLIC int16 Data_deal(tsAppTof_Data* p, int32 n)
{
    float AVER_Value = 0.0;
    float SUM = 0.0;
    float MAX = 0.0;
    int32 i;
    int32 flag = 0;
    int32 NUM = 0;
    if(n < 3)
        return -1;

    for(i = 0; i < n; i++)
    {
        AVER_Value += (float)(p->TOF_Value);
        p++;
    }
    p--;
    AVER_Value = AVER_Value/n;
    for(i = n; i >0; i--)
    {
        if(((float)p->TOF_Value) > AVER_Value)
            p->TOF_Bias = (float)(p->TOF_Value) - AVER_Value;
        else
            p->TOF_Bias = AVER_Value - (float)p->TOF_Value;

        SUM += p->TOF_Bias * p->TOF_Bias;
        p--;
    }
    SUM = sqrt(SUM/(n-1));

    for(i = 0; i < n; i++)
    {
        if(((float)(p->TOF_Bias)) > (SUM*grubbs_lookup[n-3]))
        {
            if(((float)(p->TOF_Bias)) > MAX)
            {
                MAX = ((float)p->TOF_Bias);
                flag = 1;
                NUM = i;
            }
        }
    }
    if(flag = 0)
    {
        return -1;
    }
    else
    {
        PrintfUtil_vPrintf("NUM1= %d \n",NUM);
        return NUM;
    }
}


// return meter
// need u8MaxReadings to define i32RecFlag & i32GrubFlag, wait for the malloc util
PUBLIC int16 GetTofDist_grubbs(tsAppApiTof_Data* asTofData, tsAppTof_Data* Tof_SamplesData, const uint8 u8MaxReadings)
{
    int32 i32SuccessNum = 0;
    int32 n;
    int32 NUM;
    int32 rtVal=0;
    if(u8MaxReadings <= 21 )
    {
        for(n = 0; n < u8MaxReadings; n++)
        {
            if(asTofData[n].u8Status == MAC_TOF_STATUS_SUCCESS)
            {
                Tof_SamplesData[i32SuccessNum++].TOF_Value = (int32)(asTofData[n].s32Tof*0.003);
            }
        }
        while((NUM = Data_deal(Tof_SamplesData,i32SuccessNum)) != -1)
        {
            Tof_SamplesData[NUM].TOF_Value = Tof_SamplesData[i32SuccessNum-1].TOF_Value;
            i32SuccessNum--;
        }
        if(i32SuccessNum > 0)
        {
            for(n = 0; n < i32SuccessNum; n++)
            {
                rtVal += Tof_SamplesData[n].TOF_Value;
            }
            return (int16)rtVal/i32SuccessNum/10;
        }
        else
        {
            return INVALID_TOF_DISTANCE;
        }
    }
    else
    {
        return INVALID_TOF_DISTANCE;
    }
}


/****************************************************************************
 *
 * NAME: vProcessAppEventQueues
 *
 * DESCRIPTION:
 * Process application's event queues.
 *
 * PARAMETERS:      None
 *
 * RETURNS:
 * void
 *
 ****************************************************************************/
PRIVATE void vProcessAppEventQueues(void)
{
    uint32 event = EventUtil_u32ReadEvents();
    uint8 count = 0;

    while(event && count++ < 32)
    {
        switch (event)
        {
        case STATION_FIND_EVENT :
        {
            teStaionState = E_STATE_AVAILABLE;
            if(0 == tsSlot.b1Used)
            {
                vSetStationChannel(u8BroadcastChannel);
                PrintfUtil_vPrintf("SSSS= %d \n",u16StationPanId);
                vStationCast(TOF_STATION_AVAILABLE, u16StationPanId+10000, 0xFFF0, 0, 0);
                TimerUtil_eSetTimer(STATION_FIND_EVENT, 1000);
            }
            else
            {
                TimerUtil_eSetTimer(STATION_TOF2CARD_EVENT, 20);
            }
            EventUtil_vUnsetEvent(event);
            break;
        }
        case STATION_TOF2CARD_EVENT:
        {
            MAC_Addr_s sAddr;
            sAddr.u8AddrMode = 2;
            sAddr.u16PanId =  tsSlot.u16PanID;
            sAddr.uAddr.u16Short = tsSlot.u16ShortAddr;
            eTofStatus = -1;
            if (FALSE == bAppApiGetTof(asTofData, &sAddr, 20, API_TOF_FORWARDS, vTofCallback))
            {
                PrintfUtil_vPrintf("BBB \n");
                vCheckCardLost();
                vCurrentTofDone();
            }
            else
            {
                bTofBusy = TRUE;
                TimerUtil_eSetTimer(STATION_WAIT_TOF2CARD_EVENT,1000);
            }
            EventUtil_vUnsetEvent(event);
            break;
        }

        case STATION_TOF_REPORT_EVENT:
        {
            vReportTofDistance();
            tsSlot.tsTof[0].u16Val= INVALID_TOF_DISTANCE;
            TimerUtil_eSetTimer(STATION_TOF_REPORT_EVENT, 2000);
            EventUtil_vUnsetEvent(event);
            break;
        }
        case STATION_WAIT_TOF2CARD_EVENT:
        {
            if (-1 != eTofStatus)
            {
                vTofProcessing();
                vCurrentTofDone();
                if(tsSlot.b1Used)
                    TimerUtil_eSetTimer(STATION_TOF2CARD_EVENT, 1000);
                EventUtil_vUnsetEvent(event);
            }
            else
            {
                PrintfUtil_vPrintf("FAIL\n");
                vAppApiTofInit(FALSE);
                vAppApiTofInit(TRUE);
                TimerUtil_eSetTimer(STATION_FIND_EVENT, 1000);
            }
            break;
        }
        case STATION_WATCHDOG_EVENT:
        {
            DBG(PrintfUtil_vPrintf("W\n"););
            vAHI_WatchdogRestart();
            EventUtil_vUnsetEvent(event);
            break;
        }
        case TOF_SYNCADDR_EVENT:
        {
            if(bSyncedAddr)
            {
                PrintfUtil_vPrintf("SyncEnd\n");
                bSyncedAddr = FALSE;
                counts = 0;
                TimerUtil_eSetTimer(TOF_SYNCADDR_EVENT, 60000);
                vAHI_WatchdogStop();
                if(!bSysStarted)
                {
                    bSysStarted = TRUE;
                    PrintfUtil_vPrintf("ASD\n");
                    vInitStationSystem();
                    vStartCoordinator();
                    PrintfUtil_vPrintf("LOC System started. \n\r");
                    PrintfUtil_vPrintf("station id %d\n", u16StationPanId);
                    if(bAHI_WatchdogResetEvent()) PrintfUtil_vPrintf("Watchdog reset!!!\n");
                    vOADInitClient(TRUE);
                    vStartingLEDindicator();
                    vInitVariables();
                    //AppColdStart();
                    TimerUtil_eSetTimer(TOF_SYNCADDR_EVENT,60000);
                    TimerUtil_eSetTimer(STATION_TOF_REPORT_EVENT, 1000);
                    TimerUtil_eSetCircleTimer(STATION_WATCHDOG_EVENT, 1000);
                    TimerUtil_eSetTimer(STATION_FIND_EVENT,2000);
                }
            }
            else
            {
                counts++;
                if(counts > 3 )
                {
                    PrintfUtil_vPrintf("Sleep: \n");
                    vAHI_UartDisable(E_AHI_UART_0);
                    vAHI_UartDisable(E_AHI_UART_1);
                    vAHI_DioWakeEnable(0, TOF_OK_PIN);  // need disable OK button's wakeup ability when sleep
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
            EventUtil_vUnsetEvent(event);
            break;
        }
        default:
        {
            EventUtil_vUnsetEvent(event);
            break;
        }
        }
        event = EventUtil_u32ReadEvents();
    }
}

/****************************************************************************
 *
 * NAME: vProcessIncomingMlme
 *
 ****************************************************************************/
PRIVATE void vProcessIncomingMlme(MAC_MlmeDcfmInd_s *psMlmeInd)
{
}

/****************************************************************************
 *
 * NAME: vProcessIncomingData
 *
 ****************************************************************************/
PRIVATE void vProcessIncomingMcps(MAC_McpsDcfmInd_s *psMcpsInd)
{
    if (teStaionState >= E_STATE_STARTED)
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
        // if send available msg failed, retry
        if((teStaionState == E_STATE_AVAILABLE) && (psMcpsInd->uParam.sDcfmData.u8Status != MAC_ENUM_SUCCESS))
            EventUtil_vSetEvent(STATION_FIND_EVENT);
    }
}

/****************************************************************************
 *
 * NAME: vHandleMcpsDataInd
 *
 ****************************************************************************/
PRIVATE void vHandleMcpsDataInd(MAC_McpsDcfmInd_s *psMcpsInd)
{
    MAC_RxFrameData_s *psFrame;
    psFrame = &psMcpsInd->uParam.sIndData.sFrame;

    RfTofWrapper_tu_temp* psAppPkt = (RfTofWrapper_tu*)(psFrame->au8Sdu);

    switch (psAppPkt->tof_head.protocoltype)
    {
    case APP_PROTOCOL_TYPE_CARD:
    {
        vProcessTofClusterData(&psMcpsInd->uParam.sIndData);
        break;
    }

    case APP_PROTOCOL_TYPE_CMD:
    {
        vProcessStationCmd(&psMcpsInd->uParam.sIndData);
        break;
    }

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
 * NAME: vStartCoordinator
 *
 ****************************************************************************/
PRIVATE void vStartCoordinator(void)
{
    /* Structures used to hold data for MLME request and response */
    MAC_MlmeReqRsp_s   sMlmeReqRsp;
    MAC_MlmeSyncCfm_s  sMlmeSyncCfm;
    teStaionState = E_STATE_STARTED;
    sMlmeReqRsp.u8Type = MAC_MLME_REQ_START;
    sMlmeReqRsp.u8ParamLength = sizeof(MAC_MlmeReqStart_s);
    sMlmeReqRsp.uParam.sReqStart.u16PanId = u16StationPanId;
    sMlmeReqRsp.uParam.sReqStart.u8Channel = u8CurChannel;
    sMlmeReqRsp.uParam.sReqStart.u8BeaconOrder = 0x0F;
    sMlmeReqRsp.uParam.sReqStart.u8SuperframeOrder = 0x0F;
    sMlmeReqRsp.uParam.sReqStart.u8PanCoordinator = TRUE;
    sMlmeReqRsp.uParam.sReqStart.u8BatteryLifeExt = FALSE;
    sMlmeReqRsp.uParam.sReqStart.u8Realignment = FALSE;
    sMlmeReqRsp.uParam.sReqStart.u8SecurityEnable = FALSE;
    vAppApiMlmeRequest(&sMlmeReqRsp, &sMlmeSyncCfm);
}


PRIVATE void vProcessStationCmd(MAC_McpsIndData_s *psMcpsIndData)
{
    MAC_RxFrameData_s *psFrame;
    psFrame = &psMcpsIndData->sFrame;
    RfTofWrapper_tu_temp* psAppPkt = (RfTofWrapper_tu*)(psFrame->au8Sdu);

    switch(psAppPkt->tof_head.msgtype)
    {
    case TOF_LOC_STATION_OAD:
    {
        if((psAppPkt->tof_head.len == 4)
                && (psFrame->sDstAddr.u16PanId == u16StationPanId)
                &&(psFrame->sDstAddr.uAddr.u16Short == 0)
                && (psAppPkt->rf_tof_oad_data.u8DeviceType == DEVICE_TYPE_STATION)
                && (psAppPkt->rf_tof_oad_data.u16Version > OAD_LOC_STATION_VERSION))
        {
            TimerUtil_vStopAllTimer();
            EventUtil_vResetAllEvents();
            // when OAD, buzzer & red LED about 2 seconds to indicate user
            uint8 i;
            for(i=0; i<10; i++)
            {
                TimerUtil_vDelay(500, E_TIMER_UNIT_MILLISECOND);
            }
            //Enter OAD status
            vOADInvalidateSWImage();
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
 * NAME: vProcessTofClusterData
 *
 * DESCRIPTION:
 * Process TOF protocal with card and locator.
 *
 * PARAMETERS:      MAC_McpsIndData_s *
 *
 * RETURNS:
 * void
 *
 ****************************************************************************/
PRIVATE void vProcessTofClusterData(MAC_McpsIndData_s *psMcpsIndData)
{
    MAC_RxFrameData_s *psFrame;
    psFrame = &psMcpsIndData->sFrame;
    RfTofWrapper_tu_temp* psAppPkt = (RfTofWrapper_tu*)(psFrame->au8Sdu);
    switch(psAppPkt->tof_head.msgtype)
    {
    case TOF_CARD_CHECKIN:
    {
        // check len: rf_tof_card_data's len is 8
        if(psFrame->u8SduLength != 8) break;

        vStationCast(TOF_STATION_CHECKIN_ACK, psMcpsIndData->sFrame.sSrcAddr.uAddr.u16Short, \
                     psMcpsIndData->sFrame.sSrcAddr.u16PanId, MAC_TX_OPTION_ACK, 0);

        vCheckCardStatus(psMcpsIndData->sFrame.sSrcAddr.uAddr.u16Short, psAppPkt->rf_tof_card_data.u8CardStatus);
        DBG(
            PrintfUtil_vPrintf("%d: checkin: %x\n\n", u16SlotIndex, psMcpsIndData->sFrame.sSrcAddr.uAddr.u16Short);
        )
        if(bIsStationUp)
        {
            app_tof_checkin_ts app_tof_checkin_data;
            app_tof_checkin_data.app_tof_head.protocoltype = APP_PROTOCOL_TYPE_CARD;
            app_tof_checkin_data.app_tof_head.msgtype = APP_TOF_MSG_CHECKIN;
            app_tof_checkin_data.app_tof_head.len = 4;
            app_tof_checkin_data.u16ShortAddr = psMcpsIndData->sFrame.sSrcAddr.uAddr.u16Short;
            app_tof_checkin_data.u8Status = psAppPkt->rf_tof_card_data.u8CardStatus;
        }
        break;
    }
    case TOF_CARD_REQUEST:
    {
        PrintfUtil_vPrintf("Tof Request\n");
        if(psFrame->u8SduLength != 8)
        {
            break;
        }
        uint16 u16IdleIndex = 0;
        tsSlot.u8LostNum = 0;
        tsSlot.b1Used         = 1;
        tsSlot.u16SeqNum     = psAppPkt->rf_tof_card_data.u16SeqNum;
        tsSlot.u16PanID         = psMcpsIndData->sFrame.sSrcAddr.u16PanId;
        tsSlot.u16ShortAddr     = psMcpsIndData->sFrame.sSrcAddr.uAddr.u16Short;
        if(tsSlot.u16ShortAddr != (u16StationPanId+10000))
        {
            PrintfUtil_vPrintf("Addr Error\n");
            break;
        }
        u16PendingAddr          = psMcpsIndData->sFrame.sSrcAddr.uAddr.u16Short;
        tsSlot.u8LastTofDist     = 0;            // init last tof distance failed            tsSlot.tsTof[0].u16Val    = INVALID_TOF_DISTANCE;    // init current tof value
        tsSlot.tsTof[1].u16Val    = INVALID_TOF_DISTANCE;    // init last tof value
        tsSlot.tsTof[0].u16Val    = INVALID_TOF_DISTANCE;
        tsSlot.tsTof[0].u8Direction = 0xFF;    // init current tof direction
        tsSlot.tsTof[1].u8Direction = 0xFF;    // init last tof direction
        tsAccepted.u16ShortAddr = psMcpsIndData->sFrame.sSrcAddr.uAddr.u16Short;
        tsAccepted.u16SlotIndex = u16IdleIndex;
        EventUtil_vSetEvent(STATION_TOF2CARD_EVENT);
        break;
    }
    case TOF_CARD_ALARM:
    {
        // check len: rf_tof_card_data's len is 8
        if(psFrame->u8SduLength != 8) break;

        vCheckCardStatus(psMcpsIndData->sFrame.sSrcAddr.uAddr.u16Short, psAppPkt->rf_tof_card_data.u8CardStatus);
        break;
    }
    case TOF_CARD_RSSI:
    {
        DBG(
            PrintfUtil_vPrintf("%x: %d, %i\n", psMcpsIndData->sFrame.sSrcAddr.uAddr.u16Short, psAppPkt->rf_tof_card_data.u16SeqNum, SysUtil_vConvertLQI2Dbm(psFrame->u8LinkQuality));
        )
        tsSlot.u16ShortAddr = psMcpsIndData->sFrame.sSrcAddr.uAddr.u16Short;
        tsSlot.u16SeqNum = psAppPkt->rf_tof_card_data.u16SeqNum;
        tsSlot.RSSI = SysUtil_vConvertLQI2Dbm(psFrame->u8LinkQuality);
        PrintfUtil_vPrintf("RRR= %i\n",tsSlot.RSSI);
        vStationCast(TOF_STATION_RSSI, psMcpsIndData->sFrame.sSrcAddr.uAddr.u16Short, \
                     psMcpsIndData->sFrame.sSrcAddr.u16PanId, MAC_TX_OPTION_ACK, 1); // locator request: u8Val = 1
        break;
    }

    case TOF_LOCATOR_REQUEST:
    {
        // check len: locator request's len is 4
        if(psFrame->u8SduLength != 4) break;

        //FIXME, for beta only
        if(u16LocatorAddress != psMcpsIndData->sFrame.sSrcAddr.uAddr.u16Short) break;

        if(tsSlot.u16ShortAddr == psMcpsIndData->sFrame.sSrcAddr.uAddr.u16Short)
            u16AcceptLocSlot = 0;
        else if(tsSlot.u16ShortAddr == psMcpsIndData->sFrame.sSrcAddr.uAddr.u16Short)
            u16AcceptLocSlot = TOF_SLOT_LOC_INT/2;
        else if(tsSlot.b1Used == 0)
            u16AcceptLocSlot = 0;
        else if(tsSlot.b1Used == 0)
            u16AcceptLocSlot = TOF_SLOT_LOC_INT/2;
        else
            break;
        uint16 u16Index;
        uint16 u16LocIndex;
        for (u16Index = 0; u16Index < TOF_SLOT_LOC_FREQUENCE; u16Index++)
        {
            u16LocIndex = u16Index*TOF_SLOT_LOC_INT + u16AcceptLocSlot;
            tsSlot.u16ShortAddr = psMcpsIndData->sFrame.sSrcAddr.uAddr.u16Short;
            tsSlot.u8LostNum = 0;
            tsSlot.b1Used = 1;
            tsSlot.u16PanID = psMcpsIndData->sFrame.sSrcAddr.u16PanId;
            tsSlot.u16SeqNum = 0;
        }

        u16Index = (u16AcceptLocSlot == 0) ? 0 : 1;
        u8LocTofTimes[u16Index] = 0;
        i16LocTofSum[u16Index] = 0;

        vStationCast(TOF_STATION_ACCEPT, psMcpsIndData->sFrame.sSrcAddr.uAddr.u16Short, \
                     psMcpsIndData->sFrame.sSrcAddr.u16PanId, MAC_TX_OPTION_ACK, 1); // locator request: u8Val = 1

        DBG(
            PrintfUtil_vPrintf("locator %x join %d\n", psMcpsIndData->sFrame.sSrcAddr.uAddr.u16Short, u16AcceptLocSlot);
        )

        break;
    }

    // locator report card's distance
    case TOF_LOCATOR_CARDS_DIST:
    {
        // check len: rf_tof_card_data's len is 8
        if(psFrame->u8SduLength != psAppPkt->tof_head.len + 4) break;
        // to avoid receive card info again
        if(bRxCardDistanceDone == FALSE)
        {
            bRxCardDistanceDone = TRUE;
            TimerUtil_eStopTimer(STATION_SEND_CARD_INFO_EVENT);
            EventUtil_vUnsetEvent(STATION_SEND_CARD_INFO_EVENT);
            uint16 u16LocSlot = u16SlotIndex % TOF_SLOT_LOC_INT;
            uint8 u8Loc; // which loc
            if (u16LocSlot <= 1)    // this is locator #1
            {
                u16LocSlot = 0;
                u8Loc = 0;
            }
            else                    // this is locator #2
            {
                u16LocSlot = TOF_SLOT_LOC_INT/2;
                u8Loc = 1;
            }
            if(psAppPkt->rf_tof_locator_card_info.u8CardNum > 0)
            {
                memcpy(u16CardDistance, psAppPkt->rf_tof_locator_card_info.u16CardDist, psAppPkt->rf_tof_locator_card_info.u8CardNum*2);
                vProcessLocatorCardDist(psAppPkt->rf_tof_locator_card_info.u8CardNum, u8Loc);
            }
            memcpy(tsPreCardInfoAddr[u8Loc], tsCardInfoAddr, u8CardInfoLen*sizeof(tsCardInfo));
            tsSlot.u8LostNum = 0;
            bSendCardInfoDone = TRUE;
            EventUtil_vSetEvent(STATION_CLAIM_EVENT);
        }
        // let station know that card info exchange finished
        vStationCast(TOF_STATION_FINISH, psMcpsIndData->sFrame.sSrcAddr.uAddr.u16Short, \
                     psMcpsIndData->sFrame.sSrcAddr.u16PanId, MAC_TX_OPTION_ACK, 0);
        break;
    }
    default:
        break;
    }
}

PRIVATE void sort(int32*p,int32 length)
{
    int32 i,j;
    int32 temp;
    int32 flag = TRUE;
    for(i = 0;i < length && flag;i++)
    {
        flag = FALSE;
        for(j = length -1;j >= i;j--)
        {
            if(*(p+j) > *(p+j+1))
            {
                temp = *(p+j);
                *(p+j) = *(p+j+1);
                *(p+j+1) = temp;
                flag = TRUE;
            }
        }
    }
}

PUBLIC int16 GetTofDistance(tsAppApiTof_Data* asTofData, const uint8 u8MaxReadings)
{
    int32 i32SuccessNum = 0;
    int32 n;
    int32 rtVal=0;
    int32 TOF_Value[20];

    if(u8MaxReadings <=21 )
    {
        for(n = 0; n < u8MaxReadings; n++)
        {
            if(asTofData[n].u8Status == MAC_TOF_STATUS_SUCCESS)
            {
                TOF_Value[i32SuccessNum++] = (int32)(asTofData[n].s32Tof*0.003);   // set negative data to be 0 for all success data, and set data to be decimeter
            }
        }
        tsSlot.Rate = i32SuccessNum;
        sort(TOF_Value,i32SuccessNum);
        if(i32SuccessNum > 5)
        {
            for(n = 2; n < (i32SuccessNum -2);n++)
            {
                rtVal+= TOF_Value[n];
            }
            return (int16)(rtVal/(i32SuccessNum-4)/10);
        }
        else return INVALID_TOF_DISTANCE;
    }
    else
        tsSlot.Rate = 0;
        return INVALID_TOF_DISTANCE;
}

PRIVATE void vUart1_txCallBack(void)
{
    bDisReporting = FALSE;
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
    if(tsSlot.tsTof[0].u16Val!= INVALID_TOF_DISTANCE)
    {
        uart_locate.locate.isvalid = TRUE;
    }
    else
    {
        uart_locate.locate.isvalid = FALSE;
    }
    uart_locate.locate.seqnum = seqnum++;
    uart_locate.locate.stationPandID = tsSlot.u16ShortAddr;
    uart_locate.locate.stationShortAddr =  u16PendingAddr;
    PrintfUtil_vPrintf("dest1 %d\n",tsSlot.u16ShortAddr);
    PrintfUtil_vPrintf("dest2 %d\n",u16PendingAddr);

    uart_locate.locate.Locate_Distance = tsSlot.tsTof[0].u16Val;
    uart_locate.locate.RSSI            = tsSlot.RSSI;
    uart_locate.locate.success_rate    = tsSlot.Rate;
    PrintfUtil_vPrintf("Distance %d\n",uart_locate.locate.Locate_Distance);
    PrintfUtil_vPrintf("RSSI %i\n",uart_locate.locate.RSSI);
    PrintfUtil_vPrintf("Rate %d\n",uart_locate.locate.success_rate);

    CONVERT_ENDIAN(uart_locate.locate.stationPandID);
    CONVERT_ENDIAN(uart_locate.locate.stationShortAddr);
    CONVERT_ENDIAN( uart_locate.locate.Locate_Distance);

    CONVERT_ENDIAN(uart_locate.locate.RSSI);
    CONVERT_ENDIAN(uart_locate.locate.success_rate);

    PrintfUtil_vPrintf("U tx %d\n",sizeof(uart_locate));
    PrintfUtil_vPrintMem((uint8*)&uart_locate,sizeof(uart_locate));

    /* send to uart , block operation */
    bDisReporting = TRUE;
    u8Uart_StartTx(E_AHI_UART_1, (uint8*)&uart_locate, sizeof(uart_locate));

    while(bDisReporting == TRUE);
}

/****************************************************************************
 *
 * NAME: vTofProcessing
 *
 * DESCRIPTION:
 * Process TOF with card, get the tof value, compare with last tof to decide if need loc or not.
 *
 * PARAMETERS:      None
 *
 * RETURNS:
 * void
 *
 ****************************************************************************/
PRIVATE void vTofProcessing()
{
    int16 i16TofDist;
    uint8 u8NeedLoc = 0;    // 0: not need, 1: locator #1, 2: locator #2
    switch (eTofStatus)
    {
    case TOF_SUCCESS:
    {
        tsSlot.u8LostNum = 0;
        i16TofDist = GetTofDistance(asTofData,20);
        PrintfUtil_vPrintf("i16TofDist %i\n",i16TofDist);
        if (i16TofDist != INVALID_TOF_DISTANCE)
        {
            i16TofDist = MAX(0, i16TofDist);
            //tsSlot.tsTof[0].u16Val = (uint16)i16TofDist;
            PrintfUtil_vPrintf("before %d\n",tsSlot.tsTof[0].u16Val);
            if(tsSlot.tsTof[0].u16Val >1000  || tsSlot.tsTof[0].u16Val == i16TofDist)
            {
                PrintfUtil_vPrintf("! \n");
                tsSlot.tsTof[0].u16Val = (uint16)i16TofDist;
            }
            else if(tsSlot.tsTof[0].u16Val > (uint16)i16TofDist)
            {
                PrintfUtil_vPrintf("@ \n");
                tsSlot.tsTof[0].u16Val = tsSlot.tsTof[0].u16Val - i16TofDist;
                tsSlot.tsTof[0].u16Val = tsSlot.tsTof[0].u16Val * 102;
                tsSlot.tsTof[0].u16Val = tsSlot.tsTof[0].u16Val + 128;   //四舍五入
                tsSlot.tsTof[0].u16Val = tsSlot.tsTof[0].u16Val/256;
                tsSlot.tsTof[0].u16Val = i16TofDist - tsSlot.tsTof[0].u16Val;
            }
            else
            {
                PrintfUtil_vPrintf("# \n");
                tsSlot.tsTof[0].u16Val = i16TofDist - tsSlot.tsTof[0].u16Val;
                tsSlot.tsTof[0].u16Val = tsSlot.tsTof[0].u16Val * 102;
                tsSlot.tsTof[0].u16Val = tsSlot.tsTof[0].u16Val + 128;   //四舍五入
                tsSlot.tsTof[0].u16Val = tsSlot.tsTof[0].u16Val/256;
                tsSlot.tsTof[0].u16Val = i16TofDist + tsSlot.tsTof[0].u16Val;
            }
        }
        else
        {
            PrintfUtil_vPrintf("AAA \n");
            vCheckCardLost();
        }
        PrintfUtil_vPrintf("DDD= %d\n",tsSlot.tsTof[0].u16Val);
        break;
    }
    default:
    {
        PrintfUtil_vPrintf("CCC \n");
        vCheckCardLost();
        break;
    }
    }
    //fixed me:  if TOF not start, seems that the card can't receive msg any more, so not need to send msg to card, and not need locator
    if(TOF_NOT_STARTED != eTofStatus || tsSlot.u8LostNum < 2)
    {
        PrintfUtil_vPrintf("TOF_START\n");
        //u16PendingAddr = 0x8B5E;
        if(tsSlot.b1TofDir == API_TOF_REVERSE)//API_TOF_REVERSE)
        {
            bIsTofSuccess = TRUE;
            tsSlot.b1TofDir = API_TOF_FORWARDS;
            //tsSlot.tsTof[0].u16Val = (tsSlot.tsTof[0].u16Val +tsSlot.tsTof[1].u16Val)/2;
            PrintfUtil_vPrintf("2\n");
            vStationCast(TOF_STATION_FINISH, u16PendingAddr, TOF_CARD_NWK_ADDR, MAC_TX_OPTION_ACK, 0);
            DBG(
                PrintfUtil_vPrintf("%d: Send Fin to %x \n", u16TofSlotIndex, u16PendingAddr);
            )
        }
        else
        {
            tsSlot.b1TofDir = API_TOF_REVERSE;
            vStationCast(TOF_STATION_FINISH, u16PendingAddr, TOF_CARD_NWK_ADDR, MAC_TX_OPTION_ACK, 0);
            //vStationCast(TOF_STATION_WAIT, u16PendingAddr, TOF_CARD_NWK_ADDR, MAC_TX_OPTION_ACK, 0);
            DBG(
                PrintfUtil_vPrintf("%d: Send Wait to %x \n", u16TofSlotIndex, u16PendingAddr);
            )
        }
    }
    else
    {
        DBG(
            PrintfUtil_vPrintf("%d: TOF not started %x \n", u16TofSlotIndex, u16PendingAddr);
        )
    }
}

/****************************************************************************
 *
 * NAME: vCurrentTofDone
 *
 * DESCRIPTION:
 * Init the TOF processing relatived parameters.
 *
 * PARAMETERS:      None
 *
 * RETURNS:
 * void
 *
 ****************************************************************************/
PRIVATE void vCurrentTofDone()
{
    eTofStatus = -1;
    bTofBusy = FALSE;
}

/****************************************************************************
 *
 * NAME: vTofCallback
 *
 * DESCRIPTION:
 * Tof's callback function.
 *
 * PARAMETERS:      eTofReturn
 *
 * RETURNS:
 * void
 *
 ****************************************************************************/
PRIVATE void vTofCallback(eTofReturn eStatus)
{
    eTofStatus = eStatus;
}

/****************************************************************************
 *
 * NAME: vStationCast
 *
 * DESCRIPTION:
 * Station send different msg to air.
 *
 * PARAMETERS:
 *                u8CmdType - Tof protocol
 *                u16DstAddr, u16DstPanId - destination
 *                u8TxOptions - need retry or not
 *                u8Val - optional value
 *
 * RETURNS:
 * void
 *
 ****************************************************************************/
PRIVATE void vStationCast(uint8 u8CmdType, uint16 u16DstAddr, uint16 u16DstPanId, uint8 u8TxOptions, uint8 u8Val)
{
    bMsgSending = TRUE;
    MacUtil_SendParams_s sParams;
    sParams.u8Radius        = 1;
    sParams.u16DstAddr    = u16DstAddr;
    sParams.u16DstPanId     = u16DstPanId;
    sParams.u16ClusterId     = 0;
    sParams.u8DeliverMode    = MAC_UTIL_UNICAST;

    RfTofWrapper_tu_temp RfTofData;
    RfTofData.tof_head.protocoltype = APP_PROTOCOL_TYPE_CARD;
    RfTofData.tof_head.msgtype = u8CmdType;

    switch (u8CmdType)
    {
    case TOF_STATION_ACCEPT:
    {
        RfTofData.rf_tof_station_accept.u8StationChannel = u8TofChannel;
        RfTofData.rf_tof_station_accept.u8LocatorCardChannel = u8LocatorCardChannel;
        //RfTofData.rf_tof_station_accept.u16CurSlot = u16SlotIndex;
         // for card's accept
        //RfTofData.rf_tof_station_accept.tsAccptData.u16ShortAddr = tsAccepted.u16ShortAddr;
        //RfTofData.rf_tof_station_accept.tsAccptData.u16SlotIndex = tsAccepted.u16SlotIndex;
        RfTofData.rf_tof_station_accept.u8AcceptNum = 1;
        RfTofData.tof_head.len = (RfTofData.rf_tof_station_accept.u8AcceptNum + 2)*4;
        break;
    }
    case TOF_STATION_AVAILABLE:
    case TOF_STATION_RSSI:
    case TOF_STATION_BUSY:
    case TOF_STATION_ILDE:
    {
        RfTofData.rf_tof_station_signal.u8AvailableMs = u8Val;
        RfTofData.rf_tof_station_signal.u8StationStatus = tStationStatus;
        RfTofData.rf_tof_station_signal.RSSI = tsSlot.RSSI;
        if((tsSlot.b1Used == 0))
            RfTofData.rf_tof_station_signal.u8LocIdle = 1;
        else
            RfTofData.rf_tof_station_signal.u8LocIdle = 0;

        RfTofData.tof_head.len = 7;    //this is not a 4-byte struct !!!!!!!!!!!
        break;
    }
    case TOF_STATION_FINISH:
    {
        //the second slot of the card. so add 20ms to runMs
        RfTofData.rf_tof_station_finish.u8LocN = u8Val;
        RfTofData.rf_tof_station_finish.u8StationStatus = tStationStatus;
        if(!bIsTofSuccess)
        {
            RfTofData.rf_tof_station_finish.u16Dist2Station = INVALID_TOF_DISTANCE;
        }
        else
        {
            RfTofData.rf_tof_station_finish.u16Dist2Station = tsSlot.tsTof[0].u16Val;
            RfTofData.rf_tof_station_finish.u8Rate = tsSlot.Rate;
        }
        RfTofData.tof_head.len = 9;
        break;
    }
    case TOF_STATION_HELP_ACK:
    case TOF_STATION_NOPWD_ACK:
    case TOF_STATION_CHECKIN_ACK:
    {
        RfTofData.tof_head.len = 0;
        // if station receive app's help_ack, not need to send station_ack, just send app_ack
        if(TOF_STATION_HELP_ACK == u8CmdType)
        {
            if(tsSlot.u16ShortAddr == u16DstAddr)
            {
                RfTofData.tof_head.msgtype = TOF_APP_HELP_ACK;
            }
        }
        break;
    }
    case TOF_STATION_WAIT:
        RfTofData.tof_head.len = 0;
        break;
    default:
        break;
    }

    u8LastHandle = MacUtil_vSendData(&sParams, (uint8*)&RfTofData, RfTofData.tof_head.len+sizeof(app_header_t), u8TxOptions);
}

/****************************************************************************
 *
 * NAME: vSendCardInfo
 *
 * DESCRIPTION:
 * Station send the card's info to loc, which include all the short addr of cards which need loc.
 *
 * PARAMETERS:      None
 *
 * RETURNS:
 * void
 *
 ****************************************************************************/
PRIVATE void vSendCardInfo()
{
    uint16 u16IndexTmp, u16IndexTmp2;
    uint8 u8NeedLoc;    // to record which locator
    uint16 u16LocIndex;

    if((u16SlotIndex % TOF_SLOT_LOC_INT) <= 1)
    {
        u16LocIndex = 0;
        u8NeedLoc = 1;
    }
    else
    {
        u16LocIndex = TOF_SLOT_LOC_INT/2;
        u8NeedLoc = 2;
    }

    uint8 u8RunSlots = u16SlotIndex % TOF_SLOT_LOC_INT - u16LocIndex;

    u8CardInfoLen = 0;

    //locator distance has been gotten
    if(u8LocDist[u8NeedLoc-1] != 0)
    {

        // when call vSendCardInfo, current slot index is u16SlotIndex or u16SlotIndex+1, these 2 slots are for locator
        // u16SlotTmp is start from u16SlotIndex+1, which is NOT a card, but its bNeedLoc is false
        // when retry some times, current slot index maybe u16SlotIndex+1, then u16SlotTmp is start from u16SlotIndex+2, is a card
        uint16 u16SlotTmp = u16SlotIndex+TOF_SLOT_NUM-TOF_SLOT_LOC_INT+1;
        for (u16IndexTmp = 0; u16IndexTmp < TOF_SLOT_LOC_INT - 1; u16IndexTmp++)
        {
            u16IndexTmp2 = (u16SlotTmp + u16IndexTmp) % TOF_SLOT_NUM;
            if(tsSlot.b1Used)
            {
                tsCardInfoAddr[u8CardInfoLen].u16ShortAddr = tsSlot.u16ShortAddr;
                tsCardInfoAddr[u8CardInfoLen].u16SlotIndex = u16IndexTmp2;
                u8CardInfoLen++;
            }
        }
    }

    MacUtil_SendParams_s sParams;
    sParams.u8Radius        = 1;
    sParams.u16DstAddr    = tsSlot.u16ShortAddr;
    sParams.u16DstPanId     = TOF_LOCNODE_NWK_ADDR;
    sParams.u16ClusterId     = 0;
    sParams.u8DeliverMode    = MAC_UTIL_UNICAST;

    RfTofWrapper_tu_temp RfTofdata;
    RfTofdata.rf_tof_station_card_info.tof_head.protocoltype = APP_PROTOCOL_TYPE_CARD;
    RfTofdata.rf_tof_station_card_info.tof_head.msgtype = TOF_STATION_CARDS_INFO;
    RfTofdata.rf_tof_station_card_info.tof_head.len = ((u8CardInfoLen+2)/2)*4;    // to snap to a 4-byte structure
    RfTofdata.rf_tof_station_card_info.u8RunMs = (uint8)((u32AHI_TickTimerRead() - u32SlotTick) / 16000) + u8RunSlots * TOF_SLOT_MS;
    RfTofdata.rf_tof_station_card_info.u8CardNum = u8CardInfoLen;

    if(u8CardInfoLen > 0)
    {
        uint16 u16Tmp;
        for(u16Tmp = 0; u16Tmp < u8CardInfoLen; u16Tmp++)
            RfTofdata.rf_tof_station_card_info.u16CardAddr[u16Tmp] = tsCardInfoAddr[u16Tmp].u16ShortAddr;
    }

    MacUtil_vSendData(&sParams, (uint8*)&RfTofdata, RfTofdata.rf_tof_station_card_info.tof_head.len+4, 0);
}

/****************************************************************************
 *
 * NAME: vStopTofIndex
 *
 * DESCRIPTION:
 * The certain slot's card is not able to finish TOF, need stop it.
 *
 * PARAMETERS:      u16Index - the stopped slot index
 *
 * RETURNS:
 * void
 *
 ****************************************************************************/
PRIVATE void vStopTofIndex(void)
{
    if (bTofBusy)
    {
        EventUtil_vUnsetEvent(STATION_WAIT_TOF2CARD_EVENT);
        vAppApiTofInit(FALSE);
        vAppApiTofInit(TRUE);
        vCurrentTofDone();
    }

    //vCheckCardLost();

    // fixme
    // if the card is still doing TOF, it seems always can't receive any package, so need locator is not use here
    // need check the reason
    if (tsSlot.b1Used)    // no time left
    {
        bIsTofSuccess = FALSE;
        PrintfUtil_vPrintf("3\n");
        vStationCast(TOF_STATION_FINISH, tsSlot.u16ShortAddr, TOF_CARD_NWK_ADDR, MAC_TX_OPTION_ACK, 0);
    }
}

/****************************************************************************
 *
 * NAME: vProcessLocatorCardDist
 *
 * DESCRIPTION:
 * Loc return cards' L-C distances, use these data to determine card's direction
 *
 * PARAMETERS:
 *                 u8Len - card number
 *                u8Loc - which locator
 *
 * RETURNS:
 * void
 *
 ****************************************************************************/
PRIVATE void vProcessLocatorCardDist(uint8 u8Len, uint8 u8Loc)    // u8Loc: 0, 1
{
#ifdef __NEED_LOCATOR__
    uint16 u16LocSlotIndex = (u8Loc == 0) ? 0 : TOF_SLOT_LOC_INT/2;

    uint8 u8IndexTmp;
    for(u8IndexTmp = 0; u8IndexTmp < u8Len; u8IndexTmp++)
    {
        if(u16CardDistance[u8IndexTmp] != INVALID_TOF_DISTANCE)    // locator tof success
        {
            uint16 u16CardSlotIndex = tsPreCardInfoAddr[u8Loc][u8IndexTmp].u16SlotIndex;
            if(tsSlot.tsTof[0].u16Val != INVALID_TOF_DISTANCE)    // station tof success
            {
                // SC <= SL
                if(tsSlot.tsTof[0].u16Val <= tsSlot.tsTof[0].u16Val)
                {
                    // LC <= SL, card is between S and L, same direction to Loc;
                    if(u16CardDistance[u8IndexTmp] < tsSlot.tsTof[0].u16Val)
                        tsSlot.tsTof[0].u8Direction = u8Loc + 1;

                    // else: different direction
                    else
                        tsSlot.tsTof[0].u8Direction = 2 - u8Loc;
                }

                // SC > SL
                else
                {
                    // LC <= SC, same direction to Loc
                    if(u16CardDistance[u8IndexTmp] <= tsSlot.tsTof[0].u16Val)
                        tsSlot.tsTof[0].u8Direction = u8Loc + 1;

                    // LC > SC, different direction to Loc
                    else
                        tsSlot.tsTof[0].u8Direction = 2 - u8Loc;
                }
            }
            // else
            // FIXME: if S-C failed, and L-C sucessed, it's hard to determin C's location?
        }

        // else, can't determine C's location
    }
#endif

#ifdef __STAT_TOF__
    uint8 u8Index;
    for (u8Index = 0; u8Index < u8Len; u8Index++)
    {
        if(u16CardDistance[u8Index] != INVALID_TOF_DISTANCE)
        {
            if((tsPreCardInfoAddr[u8Loc][u8Index].u16ShortAddr >= CARD_BASE) && (tsPreCardInfoAddr[u8Loc][u8Index].u16ShortAddr < CARD_BASE+STAT_LEN))
                u32StatTofLocatorSuccess[tsPreCardInfoAddr[u8Loc][u8Index].u16ShortAddr-CARD_BASE]++;
        }
    }
#endif
}

#ifdef __TEST_SLOT__
PRIVATE void vInitSlot()
{
    uint16 u16Num = 0;
    uint16 u16IndexTmp;

    while (u16Num++ <= TOF_SLOT_NUM - TOF_SLOT_LOC_INT)     // last 2 locator to let card joid
    {
        if(u16Num%(TOF_SLOT_LOC_INT/2) < 2)    // for locator
            continue;

        for (u16IndexTmp = 0; u16IndexTmp < TOF_SLOT_IDLE_NUM; u16IndexTmp++)
        {
            if(u16Num%(TOF_SLOT_LOC_INT/2) == u16IdleSlot[u16IndexTmp])
            {
                break;
            }
        }

        if(u16IndexTmp == TOF_SLOT_IDLE_NUM)
        {
            tsSlot[u16Num].b1Used = 1;
            tsSlot[u16Num].u8DeviceType = TEST_DEVICE;
        }
    }
}

PRIVATE void vPrintSlot()
{
    uint16 u16Index;
    DBG(
        for (u16Index = 0; u16Index < TOF_SLOT_NUM; u16Index++)
        PrintfUtil_vPrintf("%d: %d, %d\n", u16Index, tsSlot[u16Index].b1Used, tsSlot[u16Index].u8DeviceType);
    )
    }
#endif

/****************************************************************************
 *
 * NAME: vSetStationChannel
 *
 * DESCRIPTION:
 * Station change to a certain channel
 *
 * PARAMETERS:      channel - new channel
 *
 * RETURNS:
 *         void
 *
 ****************************************************************************/
PRIVATE void vSetStationChannel(uint8 channel)
{
    if(u8CurChannel != channel)
    {
        u8CurChannel = channel;
        eAppApiPlmeSet(PHY_PIB_ATTR_CURRENT_CHANNEL, channel);
        if(u8CurChannel == u8TofChannel)    // tof retry number
            s_psMacPib->u8MaxFrameRetries = 1;
        else        // card join retry number
            s_psMacPib->u8MaxFrameRetries = 3;
    }
}

/****************************************************************************
 *
 * NAME: vCheckCardLost
 *
 * DESCRIPTION:
 * Check the certain card's lost status, if this card lost more than TOF_STATION_CARD_MAX_FAILED times, release this slot
 *
 * PARAMETERS:      u16index - the checked slot
 *
 * RETURNS:
 *         void
 *
 ****************************************************************************/
PRIVATE void vCheckCardLost(void)
{
    PrintfUtil_vPrintf("Lost\n");
    tsSlot.b1Used = 0;
    tsSlot.u8LostNum++;
    tsSlot.u16ShortAddr = 0;
    EventUtil_vSetEvent(STATION_FIND_EVENT);
}

/****************************************************************************
 *
 * NAME: vCheckCardStatus
 *
 * DESCRIPTION:
 * Check the certain card's status: help, nopwd or normal
 *
 * PARAMETERS:
 *                u16ShortAddr - the checked card's addr
 *                 u8Status - the card's status bitmap
 *
 * RETURNS:
 *         void
 *
 ****************************************************************************/
PRIVATE void vCheckCardStatus(uint16 u16ShortAddr, uint8 u8Status)
{
    app_tof_alarm_type_te teStatus = APP_TOF_ALARM_NONE;

    // help has priority higher than nopwd
    if(u8Status & CARD_STATUS_HELP)
    {
        teStatus = APP_TOF_ALARM_CARD_HELP;
        //vStationCast(TOF_STATION_HELP_ACK, u16ShortAddr, TOF_CARD_NWK_ADDR, 0, 0);
    }
    else if(u8Status & CARD_STATUS_NOPWD)
    {
        teStatus = APP_TOF_ALARM_CARD_NOPWD;
        vStationCast(TOF_STATION_NOPWD_ACK, u16ShortAddr, TOF_CARD_NWK_ADDR, 0, 0);
    }
    else
    {
        return;
    }

    vReportCardAlarm(u16ShortAddr, teStatus);
}


/****************************************************************************
 *
 * NAME: vBsMac_rx_callback
 *
 * DESCRIPTION:
 * Deal with the msg rx from ARM
 *
 * PARAMETERS:
 *                pbuf - the rx msg buffer
 *                len - the rx msg len
 *
 * RETURNS:
 *         void
 *
 ****************************************************************************/
PRIVATE void vBsMac_rx_callback(unsigned char *pbuf, unsigned char len)
{
    DBG(
        PrintfUtil_vPrintf("-- from ARM: %d --\n", len);
        int8 u8I;
        for(u8I = 0; u8I < len; u8I++)
        PrintfUtil_vPrintf("%X  ", pbuf[u8I]);
        PrintfUtil_vPrintf("\n");
    )

        if(len < sizeof(struct nwkhdr) + sizeof(app_header_t))
            return;

    RfTofWrapper_tu_temp* psAppPkt = (RfTofWrapper_tu*)(pbuf + sizeof(struct nwkhdr));

    // not for me
    if(APP_PROTOCOL_TYPE_CARD != psAppPkt->tof_head.protocoltype)
        return;

    //LedUtil_vToggle(LED_UART);

    switch(psAppPkt->tof_head.msgtype)
    {
    case APP_TOF_MSG_RETREAT:
    {
        tStationStatus = STATION_STATUS_RETREAT;
        break;
    }

    case APP_TOF_MSG_CANCEL_RETREAT:
    {
        tStationStatus = STATION_STATUS_NORMAL;
        break;
    }

    case APP_TOF_MSG_REPORT_ACK:
    {
        DBG(
            PrintfUtil_vPrintf("report_ack\n");
        )
        app_LSrfReport_t *prfReport = (app_LSrfReport_t *)((app_header_t *)psAppPkt + 1);
        if( prfReport->reporttype == APP_LS_REPORT_LIVE)
        {
            DBG(
                PrintfUtil_vPrintf("live report_ack\n");
            )
            u8LastLiveAckSeq = prfReport->seqnum;
            bIsStationUp = TRUE;
            u8RestartCounter = 0;
        }
        break;
    }
    case APP_TOF_MSG_SET:
    {
        DBG(PrintfUtil_vPrintf("msg set \n"););
        DBG(PrintfUtil_vPrintMem((uint8*)psAppPkt, len - sizeof(struct nwkhdr)););

        app_header_t * pheader = (app_header_t *)psAppPkt;
        app_LSrfSet_t * prfSet = (app_LSrfSet_t *)(pheader + 1);

        SysUtil_vConvertEndian((uint8*)&pheader->len, sizeof(pheader->len));
        SysUtil_vConvertEndian((uint8*)&prfSet->hdr.srcaddr, sizeof(prfSet->hdr.srcaddr));
        SysUtil_vConvertEndian((uint8*)&prfSet->hdr.dstaddr, sizeof(prfSet->hdr.dstaddr));
        SysUtil_vConvertEndian((uint8*)&prfSet->seqnum, sizeof(prfSet->seqnum));
        SysUtil_vConvertEndian((uint8*)&prfSet->crc, sizeof(prfSet->crc));

        if(prfSet->hdr.dstaddr == u16StationPanId
                && pheader->len  >=  (sizeof(app_LSrfSet_t) + sizeof(app_rfTlv_t))) //at least one tlv
        {
            uint_16 crc;
            crc = CRC16((uint8*)(prfSet + 1),pheader->len-sizeof(app_LSrfSet_t) , 0xFFFF);
            if(crc == prfSet->crc)
            {
                uint16 len = pheader->len - sizeof(app_LSrfSet_t);  // sum len of all tlvs

                app_rfTlv_t *pTlv = (app_rfTlv_t *)(prfSet+1);

                bool  changedChannel = FALSE;
                uint8 u8NewTofChannel = u8TofChannel;
                uint8 u8NewLocChannel = u8LocatorCardChannel;;

                while(len >= (sizeof(app_rfTlv_t) + pTlv->len))
                {
                    uint8 *pdata = (uint8*)(pTlv+1);
                    if(pTlv->type == APP_LS_TLV_TYPE_TOF_CHANNEL)
                    {
                        if(*pdata != u8TofChannel)
                        {
                            u8NewTofChannel = *pdata;
                            changedChannel = TRUE;
                        }
                    }
                    else if(pTlv->type == APP_LS_TLV_TYPE_LOC_CHANNEL)
                    {
                        if(*pdata != u8LocatorCardChannel)
                        {
                            u8NewLocChannel = *pdata;
                            changedChannel = TRUE;
                        }
                    }
                    len -= (sizeof(app_rfTlv_t)  + pTlv->len);
                    pTlv = (app_rfTlv_t *)(pdata  + pTlv->len);
                }

                //if(changedChannel && bChannelValid(u8BroadcastChannel, u8NewTofChannel, u8NewLocChannel))
                {
                    vSaveChannel(u8NewTofChannel, u8NewLocChannel);
                    // reset after set
                    vAHI_SwReset();
                }
            }
            else
            {
                DBG(PrintfUtil_vPrintf("CRC failed %d %d\n", prfSet->crc, crc););
            }
        }
        else
        {
            DBG(PrintfUtil_vPrintf("head error \n"););
        }
        break;
    }
    default:
        break;
    }
}

/****************************************************************************
 *
 ****************************************************************************/
PRIVATE void vReportCardDistance()
{
    uint16 u16CardBeginSlot = (u16SlotIndex + TOF_SLOT_NUM - TOF_SLOT_LOC_INT*2) % TOF_SLOT_NUM + 1;    // delay 2 second (TOF_SLOT_LOC_INT*2)
    uint16 u16CardEndSlot = u16CardBeginSlot + TOF_SLOT_LOC_INT/2 - 2;

    uint16 u16CardNum = 0;

    app_tof_distance_ts app_tof_distance_data;

    app_tof_distance_data.app_tof_head.protocoltype = APP_PROTOCOL_TYPE_CARD;
    app_tof_distance_data.app_tof_head.msgtype = APP_TOF_MSG_DISTANCE;

    uint16 u16IndexTmp, u16SlotIndexTmp;
    for(u16IndexTmp = u16CardBeginSlot; u16IndexTmp < u16CardEndSlot; u16IndexTmp++)
    {
        u16SlotIndexTmp = u16IndexTmp%TOF_SLOT_NUM;
        tsTofSlot *slot1 = &tsSlot, *slot2;
        tof_distance_ts  *distance_data = &app_tof_distance_data.tof_distance[u16CardNum];
        {

            if((slot1->b1Used) &&
                    (slot1->tsTof[0].u16Val != INVALID_TOF_DISTANCE))
            {
                distance_data->u16ShortAddr    = slot1->u16ShortAddr;
                distance_data->u16SeqNum     = slot1->u16SeqNum;
                distance_data->u16Distance     = slot1->tsTof[0].u16Val;
                distance_data->u8Direction        = slot1->tsTof[0].u8Direction;
                u16CardNum++;

            }
        }

        {
            slot2 = &tsSlot;
            if((slot1->b1Used) &&
                    (slot1->b1TofDir == API_TOF_FORWARDS) && //only send at the first slot
                    (slot1->tsTof[0].u16Val != INVALID_TOF_DISTANCE || slot2->tsTof[0].u16Val != INVALID_TOF_DISTANCE ))
            {
                distance_data->u16ShortAddr    = slot1->u16ShortAddr;
                distance_data->u16SeqNum     = slot1->u16SeqNum;

                distance_data->u16Distance = INVALID_TOF_DISTANCE;
                if(slot1->tsTof[0].u16Val != INVALID_TOF_DISTANCE)
                {
                    distance_data->u16Distance = slot1->tsTof[0].u16Val;
                }

                if(slot2->tsTof[0].u16Val != INVALID_TOF_DISTANCE)
                {
                    if(distance_data->u16Distance != INVALID_TOF_DISTANCE)
                    {
                        distance_data->u16Distance += slot2->tsTof[0].u16Val;
                        distance_data->u16Distance /= 2;
                    }
                    else
                        distance_data->u16Distance = slot2->tsTof[0].u16Val;
                }

                distance_data->u8Direction        = tsSlot.tsTof[0].u8Direction;
                u16CardNum++;
            }

        }

        if(APP_MAX_CARD_NUM == u16CardNum)
        {
            DBG(PrintfUtil_vPrintf("slot:%d df#:%d\n", u16SlotIndex, u16CardNum);)
            app_tof_distance_data.app_tof_head.len = u16CardNum*sizeof(tof_distance_ts);

            if(bIsStationUp)
                vWriteData2Arm((uint8*)&app_tof_distance_data);

            u16CardNum = 0;
        }
    }

    if(0 < u16CardNum)
    {
        DBG(PrintfUtil_vPrintf("slot:%d d#:%d\n", u16SlotIndex, u16CardNum);)
        app_tof_distance_data.app_tof_head.len = u16CardNum*sizeof(tof_distance_ts);

        if(bIsStationUp)
            vWriteData2Arm((uint8*)&app_tof_distance_data);
        u16CardNum = 0;
    }
}

/****************************************************************************
 *
 *         void
 *
 ****************************************************************************/
PRIVATE void vReportCardRssi()
{
    if(0 < u16RssiNum)
    {
        DBG(PrintfUtil_vPrintf("slot:%d r#:%d\n\n", u16SlotIndex, u16RssiNum);)
        app_tof_rssi_data.app_tof_head.len = u16RssiNum*sizeof(tof_distance_ts);

        if(bIsStationUp)
            vWriteData2Arm((uint8*)&app_tof_rssi_data);

        u16RssiNum = 0;
    }
}

/****************************************************************************
 *
 * NAME: vReportCardAlarm
 *
 * DESCRIPTION:
 * Station report card's alarm.
 *
 * PARAMETERS:
 *                u16ShortAddr - card's addr
 *                alarm_type - alarm type
 *
 * RETURNS:
 *         void
 *
 ****************************************************************************/
PRIVATE void vReportCardAlarm(uint16 u16ShortAddr, app_tof_alarm_type_te alarm_type)
{
//PrintfUtil_vPrintf("%d:alarm %x\n\n", u16SlotIndex, u16ShortAddr);
    app_tof_alarm_ts app_tof_alarm_data;

    app_tof_alarm_data.app_tof_head.protocoltype = APP_PROTOCOL_TYPE_CARD;
    app_tof_alarm_data.app_tof_head.msgtype = APP_TOF_MSG_ALARM;
    app_tof_alarm_data.app_tof_head.len = 4;
    app_tof_alarm_data.u16ShortAddr = u16ShortAddr;
    app_tof_alarm_data.u8Status = alarm_type;

    if(bIsStationUp)
        vWriteData2Arm((uint8*)&app_tof_alarm_data);
}

PRIVATE void vConvertEndian16(uint16* pu16Data)
{
    SysUtil_vConvertEndian(pu16Data, sizeof(uint16));
}


/****************************************************************************
 *
 * NAME: vWriteData2Arm
 *
 * DESCRIPTION:
 * Station write the msg to ARM, need convert endian for all 16-bit data first.
 *
 * PARAMETERS:
 *                pbuf - the buffer which need write to ARM
 *
 * RETURNS:
 *         void
 *
 ****************************************************************************/
PRIVATE void vWriteData2Arm(uint8* pbuf)
{
    app_header_t* pAppHead = (app_header_t*) pbuf;
    uint16 u16App_len = pAppHead->len;
    uint16 u16CardIndex;

    switch (pAppHead->msgtype)
    {
    case APP_TOF_MSG_CHECKIN:
    {
        app_tof_checkin_ts* pCheckin_data = (app_tof_checkin_ts*)pbuf;
        vConvertEndian16(&(pCheckin_data->u16ShortAddr));
        break;
    }

    case APP_TOF_MSG_RSSI:
    {
        app_tof_rssi_ts* pRssi_data = (app_tof_rssi_ts*)pbuf;
        uint16 u16CardLen = u16App_len / sizeof(tof_rssi_ts);
        for (u16CardIndex = 0; u16CardIndex < u16CardLen; u16CardIndex++)
        {
            vConvertEndian16(&(pRssi_data->tof_rssi[u16CardIndex].u16ShortAddr));
            vConvertEndian16(&(pRssi_data->tof_rssi[u16CardIndex].u16SeqNum));
        }

        break;
    }

    case APP_TOF_MSG_DISTANCE:
    {
        app_tof_distance_ts* pDistance_data = (app_tof_distance_ts*)pbuf;
        uint16 u16CardLen = u16App_len / sizeof(tof_distance_ts);
        for (u16CardIndex = 0; u16CardIndex < u16CardLen; u16CardIndex++)
        {
            vConvertEndian16(&(pDistance_data->tof_distance[u16CardIndex].u16ShortAddr));
            vConvertEndian16(&(pDistance_data->tof_distance[u16CardIndex].u16SeqNum));
            vConvertEndian16(&(pDistance_data->tof_distance[u16CardIndex].u16Distance));
        }

        break;
    }

    case APP_TOF_MSG_ALARM:
    {
        app_tof_alarm_ts* pAlarm_data = (app_tof_alarm_ts*) pbuf;
        vConvertEndian16(&(pAlarm_data->u16ShortAddr));
        break;
    }

    /*
    case APP_TOF_MSG_REPORT:
    {
        app_tof_locator_ts* pLoc_data = (app_tof_locator_ts*) pbuf;
        vConvertEndian16(&(pLoc_data->u16ShortAddr));
        break;
    }
    */

    default:
        break;
    }

    uint16 nwk_len = pAppHead->len + sizeof(app_header_t) + sizeof(struct nwkhdr);
    pnwk_data->len = pAppHead->len + sizeof(app_header_t);
    pnwk_data->dst = u16ArmId;

    vConvertEndian16(&(pnwk_data->dst));
    vConvertEndian16(&(pnwk_data->len));
    vConvertEndian16(&(pAppHead->len));
    memcpy(pnwk_data+1, pbuf, u16App_len + sizeof(app_header_t));

    uint8 u8Data[120];
    memcpy(u8Data, (uint8*)pnwk_data, nwk_len);

    /*
    DBG(
           uint16 u16Tmp;
        for(u16Tmp = 0; u16Tmp < nwk_len; u16Tmp++)
            PrintfUtil_vPrintf("%x  ", u8Data[u16Tmp]);
        PrintfUtil_vPrintf("\nLen: %d\n", nwk_len);
        PrintfUtil_vPrintf("\nDst: %d\n", pnwk_data->dst);
    )
    */
    Jbsmac_eWriteData((uint8*)pnwk_data, nwk_len);
}


#ifdef __STAT_TOF__
PRIVATE void vPrintStat()
{
    if(u16StatCount++%(5*TOF_SLOT_NUM) == 0)
    {
        uint16 i;
        for (i = u16StatIndex; i < STAT_LEN; i++)
        {
            if(u32StatTofNum[i] > 0)
            {
                DBG(
                    PrintfUtil_vPrintf("%x\t%d\t%d\t%d\t%d\t%d, \t%d\t%d\n", i+CARD_BASE, u32StatTofNum[i], u32StatTofSuccess[i], u32StatTofFailed[i], u32StatTofSkip[i], u32StatTofUnCom[i], u32StatTofLocatorNum[i], u32StatTofLocatorSuccess[i]);
                )
                u32StatTofNum[i] = 0;
                u32StatTofFailed[i] = 0;
                u32StatTofSkip[i] = 0;
                u32StatTofSuccess[i] = 0;
                u32StatTofUnCom[i] = 0;
                u32StatTofLocatorNum[i] = 0;
                u32StatTofLocatorSuccess[i] = 0;

                break;
            }
        }

        if(i == STAT_LEN)
        {
            for(i = 0; i < u16StatIndex; i++)
            {
                if(u32StatTofNum[i] > 0)
                {
                    DBG(
                        PrintfUtil_vPrintf("%x\t%d\t%d\t%d\t%d\t%d, \t%d\t%d\n", i+CARD_BASE, u32StatTofNum[i], u32StatTofSuccess[i], u32StatTofFailed[i], u32StatTofSkip[i], u32StatTofUnCom[i], u32StatTofLocatorNum[i], u32StatTofLocatorSuccess[i]);
                    )
                    u32StatTofNum[i] = 0;
                    u32StatTofFailed[i] = 0;
                    u32StatTofSkip[i] = 0;
                    u32StatTofSuccess[i] = 0;
                    u32StatTofUnCom[i] = 0;
                    u32StatTofLocatorNum[i] = 0;
                    u32StatTofLocatorSuccess[i] = 0;

                    break;
                }
            }
        }

        u16StatIndex = i + 1;
        if(u16StatIndex >= STAT_LEN)
            u16StatIndex = 0;
    }
}
#endif


/****************************************************************************
 *
 *         the distributed locator
 *
 ****************************************************************************/
PRIVATE uint8 u8GetLocIndex(uint16 u16MyIndex)
{
    uint8 u8LocIndex = 0;
    if(u16MyIndex%2 == 0)
    {
        if(tsSlot.b1Used)
            u8LocIndex = 1;
        else if(tsSlot.b1Used)
            u8LocIndex = 2;
    }
    else
    {
        if(tsSlot.b1Used)
            u8LocIndex = 2;
        else if(tsSlot.b1Used)
            u8LocIndex = 1;
    }

    return u8LocIndex;
}

/****************************************************************************
 *
 * NAME: vSaveChannel
 *
 * DESCRIPTION:
 * Save channel to flash
 *
 * PARAMETERS:      Name    RW  Usage
 *                  u8TofChannel  w   tof channel
 *                  u8LocChannel  W   locator channel
 * RETURNS:
 * bool TURE for success, FALSE for fail
 *
 ****************************************************************************/
bool  vSaveChannel(uint8 u8TofChannel, uint8 u8LocChannel)
{
    tsParam_t tsparam;
    if(!bAHI_FullFlashRead(PARAM_STORE_ADDR, sizeof(tsParam_t),(uint8*)&tsparam))
    {
        DBG(PrintfUtil_vPrintf("Flash Read Fail\n"););
        return FALSE;
    }

    if(!bAHI_FlashEraseSector(7))
    {
        DBG(PrintfUtil_vPrintf("Flash Erase Fail\n"););
        return FALSE;
    }
    tsparam.u8TofChannel = u8TofChannel;
    tsparam.u8LocChannel = u8LocChannel;

    if(bAHI_FullFlashProgram(PARAM_STORE_ADDR, sizeof(tsParam_t),(uint8*)&tsparam))
    {
        DBG(PrintfUtil_vPrintf("Set  channel Success!%d %d\n", tsparam.u8TofChannel, tsparam.u8LocChannel););
        return TRUE;
    }
    else
    {
        DBG(PrintfUtil_vPrintf("Set  channel fail!%d %d\n", tsparam.u8TofChannel, tsparam.u8LocChannel););
        return FALSE;
    }
}

/****************************************************************************
 *
 * NAME: vInitialChannel
 *
 * DESCRIPTION:
 * Read mac address, stored channels from flash and initial channel
 *
 * PARAMETERS:
 * void
 *
 * RETURNS:
 * void
 *
 ****************************************************************************/
void vInitialChannel(void)
{
    tsParam_t tsparam;
    MacUtil_vReadExtAddress(&psMacAddr);
    u16StationShortAddr = 0x0000;    //psMacAddr.u32L & 0xFFFF;
    uint32 tmp32;
    tmp32 = StringUtil_BCD2uint32(psMacAddr.u32H);
    u16StationPanId = tmp32%100000;
    //FIXME, for beta only
    u16LocatorAddress = u16StationPanId + 10000;
    //FLASH
    bAHI_FlashInit(E_FL_CHIP_ST_M25P40_A, NULL);
    uint8 u8TmpBroadChannel = ((psMacAddr.u32L) >> 16) & 0xFF;
    PrintfUtil_vPrintf("u16StationPanId %d, u8TmpBroadChannel %d\n",
                                   u16StationPanId, u8TmpBroadChannel);
    if(u8TmpBroadChannel>=11 && u8TmpBroadChannel<=26)
    {
        u8BroadcastChannel = u8TmpBroadChannel;
    }
    else
    {
        u8BroadcastChannel = DEFAULT_CHANNEL_BROADCAST;
    }
    u8CurChannel = u8BroadcastChannel;
}

PRIVATE void vUart0_rx_callback(uint32 u32DeviceId, uint32 u32ItemBitmap)
{
    char c = 0;
    if(u32DeviceId == E_AHI_DEVICE_UART0 &&
            ((u32ItemBitmap == E_AHI_UART_INT_RXDATA) || (u32ItemBitmap == E_AHI_UART_INT_TIMEOUT)))
    {
        c = u8AHI_UartReadData(E_AHI_UART_0);

        DBG(PrintfUtil_vPrintf("U %c\n", c);)

        switch(c)
        {
        case 'T':
            TimerUtil_vSetDebugPrint(TRUE);
            break;
        case 't':
            TimerUtil_vSetDebugPrint(FALSE);
            break;
        case 'S':
            bDetailedPrint = TRUE;
            break;
        case 's':
            bDetailedPrint = FALSE;
            break;
        default:
            break;
        }
    }
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




