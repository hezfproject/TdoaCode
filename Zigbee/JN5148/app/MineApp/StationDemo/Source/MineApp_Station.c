
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

#include <Math.h>

#include "config.h"
#include "app_protocol.h"
#include "bsmac.h"
#include "JN5148_util.h"
#include "crc.h"

/****************************************************************************/
/***        Macro Definitions                                             ***/
/****************************************************************************/

//#define DEBUG_RTL_STATION

#if (defined DEBUG_LOC_APP)
#define DBG(x) do{x}while(0);
//#define __STAT_TOF__
#define __STAT_RSSI__
#else
#define DBG(x) //do{x}while(0);

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


#define    STATION_SLOT_EVENT                BIT(13)    // to deal with slot timer
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

#define LED_RF             E_AHI_DIO8_INT
#define LED_UART        E_AHI_DIO9_INT
#define LED_LINK         E_AHI_DIO14_INT

#define SLOT_CARD             0
#define SLOT_LOCATOR       1
#define SLOT_IDLE              2

#define PARAM_STORE_ADDR 	0x70000
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

/****************************************************************************/
/***        Local Function Prototypes                                     ***/
/****************************************************************************/
PRIVATE void vStartingLEDindicator();
PRIVATE void vInitStationSystem(void);
PRIVATE void vStartCoordinator(void);
PRIVATE void vProcessSysEventQueues(void);
PRIVATE void vProcessIncomingMlme(MAC_MlmeDcfmInd_s *psMlmeInd);
PRIVATE void vProcessIncomingMcps(MAC_McpsDcfmInd_s *psMcpsInd);
PRIVATE void vProcessIncomingHwEvent(AppQApiHwInd_s *psAHI_Ind);
PRIVATE void vHandleMcpsDataInd(MAC_McpsDcfmInd_s *psMcpsInd);
PRIVATE void vHandleMcpsDataDcfm(MAC_McpsDcfmInd_s *psMcpsInd);
PRIVATE void vProcessAppEventQueues(void);

PRIVATE void vProcessTofClusterData(MAC_McpsIndData_s *psMcpsInd);
PRIVATE void vProcessStationCmd(MAC_McpsIndData_s *psMcpsIndData);

PRIVATE void vTofCallback(eTofReturn eStatus);
PRIVATE void vTofProcessing();
PRIVATE void vCurrentTofDone();
PRIVATE void vCheckCardLost(void);

PRIVATE void vStationCast(uint8 u8CmdType, uint16 u16DstAddr, uint16 u16DstPanId, uint8 u8TxOptions, uint8 u8Val);


//PRIVATE uint8 u8GetIdleSlot(uint16 u16Start, uint16 u16ReqShortAddr, uint16* pu16Index);
PRIVATE void vSetStationChannel(uint8 channel);
PRIVATE void vCheckCardStatus(uint16 u16ShortAddr, uint8 u8Status);

PRIVATE void vBsMac_rx_callback(unsigned char *pbuf, unsigned char len);
PRIVATE void vReportCardAlarm(uint16 u16ShortAddr, app_tof_alarm_type_te alarm_type);
PRIVATE void vConvertEndian16(uint16* pu16Data);
PRIVATE void vWriteData2Arm(uint8* pbuf);
PRIVATE void vInitialChannel(void);
PRIVATE bool  vSaveChannel(uint8 u8TofChannel, uint8 u8LocChannel);
PRIVATE void vUart0_rx_callback(uint32 u32DeviceId, uint32 u32ItemBitmap);
PRIVATE void vInitVariables();



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




PRIVATE eTofReturn eTofStatus = -1;
PRIVATE tsAppApiTof_Data asTofData[20];




PRIVATE bool_t bTofBusy = FALSE;
PRIVATE tof_station_status_te tStationStatus = STATION_STATUS_NORMAL;
PRIVATE tsTofSlot tsSlot;


PRIVATE uint16 u16PendingAddr;    // the pending card which will do tof

PRIVATE bool_t bMsgSending = FALSE;    // if the msg is still sending or not
PRIVATE uint8 u8LastHandle;
PRIVATE tsCardInfo tsAccepted;
PRIVATE teState teStaionState = E_STATE_IDLE;



PRIVATE uint32 u32Data2Arm[30];    // the data send to arm, max len is 114 (BSMAC_MAX_TX_PAYLOAD_LEN), 30*4 = 120 is enough (28 is enough)
PRIVATE struct nwkhdr* pnwk_data = (struct nwkhdr*)u32Data2Arm;
PRIVATE uint16 u16ArmId;
app_tof_distance_ts app_tof_rssi_data;

// u16IdleSlot: these slots should never be used, they are for other use (station signal)

/*use u32 for alignment*/

PRIVATE uint8 u8LastLiveAckSeq = 0;
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
    DBG(i2c_printf_init(););
    //PrintfUtil_u8Init(E_AHI_UART_0, PRINTF_BAUD_RATE_1M);
    //DebugUtil_vInit(E_AHI_UART_0, vUart0_rx_callback);
    // watch dog is enable by default
    vInitStationSystem();
    vStartCoordinator();

     DBG(
        // i2c_printf_init(E_AHI_UART_0, PRINTF_BAUD_RATE_1M);
        // DebugUtil_vInit(E_AHI_UART_0, vUart0_rx_callback);
        i2c_vPrintf("LOC System started. \n\r");
        i2c_vPrintf("station id %d\n", u16StationPanId);
        if(bAHI_WatchdogResetEvent()) i2c_vPrintf("Watchdog reset !!!!!!!!!!!!!!!!!!!!!!!!!!!!!\n");
    )
    vOADInitClient(TRUE);

    LedUtil_bRegister(LED_RF);
    LedUtil_bRegister(LED_UART);
    LedUtil_bRegister(LED_LINK);

    ErrorUtil_vRegisterLed0(LED_RF);
    ErrorUtil_vRegisterLed1(LED_UART);
    ErrorUtil_vRegisterLed2(LED_LINK);
    vStartingLEDindicator();
    vInitVariables();

    //TimerUtil_eSetCircleTimer(STATION_SLOT_EVENT, TOF_SLOT_MS);
    TimerUtil_eSetCircleTimer(STATION_WATCHDOG_EVENT, 1000);    // feed watch dog every 1 second
    TimerUtil_eSetCircleTimer(STATION_FIND_EVENT ,500);
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
    i2c_printf_init();
    DBG(
        i2c_vPrintf("warm start. \n\r");
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
    (void)u32AppQApiInit(NULL, NULL, NULL); //vHwDeviceIntCallback);
    (void)u32AHI_Init();

    // Enable high power modules, tof function, timerUtil
    vAHI_HighPowerModuleEnable(TRUE, TRUE);
    vAppApiTofInit(TRUE);
    TimerUtil_vInit();

    vInitialChannel();

    pnwk_data->type = NWK_DATA;
    pnwk_data->ttl = 1;
    pnwk_data->src = u16StationPanId;
    vConvertEndian16(&(pnwk_data->src));

    app_tof_rssi_data.app_tof_head.protocoltype = APP_PROTOCOL_TYPE_CARD;
    app_tof_rssi_data.app_tof_head.msgtype = APP_TOF_MSG_RSSI;
    Jbsmac_u8Init(vBsMac_rx_callback, E_AHI_UART_0,BSMAC_UART_BAUD_DIVISOR_500k, u16StationPanId, BSMAC_DEVICE_TYPE_LOC);

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
    if(flag == 0)
    {
        return -1;
    }
    else
    {
        i2c_vPrintf("NUM1= %d \n",NUM);
        return NUM;
    }
}

/*PUBLIC int16 Data_deal(tsAppTof_Data* p, int32 n)
{
    float AVER_Value = 0;
    float SUM = 0;
    int32 i;
    int32 MAX = 0;
    int32 NUM = 0;

    PrintfUtil_vPrintf("NUM= %d \n",n);
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
        if(p->TOF_Value > AVER_Value)
            p->TOF_Bias = p->TOF_Value - AVER_Value;
        else
            p->TOF_Bias = AVER_Value - p->TOF_Value;

        SUM += p->TOF_Bias * p->TOF_Bias;
    }
    SUM = sqrt(SUM/(n-1));

    for(i = 0; i < n; i++)
    {
        if(p->TOF_Bias > (SUM*grubbs_lookup[n-3]))
        {
            if(p->TOF_Bias > MAX)
            {
                MAX = p->TOF_Bias;
                NUM = i;
            }
        }
    }
    if(MAX = 0)
    {
        return -1;
    }
    else
    {
        PrintfUtil_vPrintf("NUM1= %d \n",NUM);
        return NUM;
    }
}*/

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
                i2c_vPrintf("Channel= %d \n",u8BroadcastChannel);
                vStationCast(TOF_STATION_AVAILABLE, 0xFFFF, 0xFFFF, 0, 0);
                TimerUtil_eSetTimer(STATION_FIND_EVENT, 100);
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
            //if (FALSE == bAppApiGetTof(asTofData, &sAddr, 2, tsSlot.b1TofDir, vTofCallback))
            if (FALSE == bAppApiGetTof(asTofData, &sAddr, 20, API_TOF_FORWARDS, vTofCallback))
            {
                DBG(i2c_vPrintf("BBB \n");)
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
        case STATION_WAIT_TOF2CARD_EVENT:
        {
            if (-1 != eTofStatus)
            {
                vTofProcessing();
                vCurrentTofDone();
                if(tsSlot.b1Used)
                    EventUtil_vSetEvent(STATION_TOF2CARD_EVENT);
                EventUtil_vUnsetEvent(event);
            }
            else
            {
                DBG(i2c_vPrintf("FAIL\n");)
                vAppApiTofInit(FALSE);
                vAppApiTofInit(TRUE);
                TimerUtil_eSetTimer(STATION_TOF2CARD_EVENT, 1000);
            }
            break;
        }
        case STATION_WATCHDOG_EVENT:
        {
            DBG(i2c_vPrintf("W\n"););
            vAHI_WatchdogRestart();
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

    RfTofWrapper_tu* psAppPkt = (RfTofWrapper_tu*)(psFrame->au8Sdu);

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
    RfTofWrapper_tu* psAppPkt = (RfTofWrapper_tu*)(psFrame->au8Sdu);

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
            LedUtil_vOff(LED_LINK);
            LedUtil_vOff(LED_RF);
            LedUtil_vOff(LED_UART);
            for(i=0; i<10; i++)
            {
                TimerUtil_vDelay(500, E_TIMER_UNIT_MILLISECOND);
                LedUtil_vToggle(LED_LINK);
                LedUtil_vToggle(LED_RF);
                LedUtil_vToggle(LED_UART);
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
    RfTofWrapper_tu* psAppPkt = (RfTofWrapper_tu*)(psFrame->au8Sdu);
    switch(psAppPkt->tof_head.msgtype)
    {
    case TOF_CARD_CHECKIN:
    {
        // check len: rf_tof_card_data's len is 8
        if(psFrame->u8SduLength != 8) break;

        LedUtil_vToggle(LED_RF);

        vStationCast(TOF_STATION_CHECKIN_ACK, psMcpsIndData->sFrame.sSrcAddr.uAddr.u16Short, \
                     psMcpsIndData->sFrame.sSrcAddr.u16PanId, MAC_TX_OPTION_ACK, 0);

        vCheckCardStatus(psMcpsIndData->sFrame.sSrcAddr.uAddr.u16Short, psAppPkt->rf_tof_card_data.u8CardStatus);
        DBG(
            i2c_vPrintf("%d: checkin: %x\n\n", u16SlotIndex, psMcpsIndData->sFrame.sSrcAddr.uAddr.u16Short);
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
        // check len: rf_tof_card_data's len is 8
        DBG(i2c_vPrintf("Tof Request\n");)
        if(psFrame->u8SduLength != 8)
        {
            break;
        }
        LedUtil_vToggle(LED_RF);
        {
            uint16 u16IdleIndex = 0;
            tsSlot.u8LostNum = 0;
            // if u8tmp == 2: the card didn't lost but card request again
            // u8tmp == 1 is for new card join
            //CX: always clear the slot
            tsSlot.b1Used         = 1;
            tsSlot.u16SeqNum     = psAppPkt->rf_tof_card_data.u16SeqNum;
            tsSlot.u16PanID         = psMcpsIndData->sFrame.sSrcAddr.u16PanId;
            tsSlot.u16ShortAddr     = psMcpsIndData->sFrame.sSrcAddr.uAddr.u16Short;
            tsSlot.u8LastTofDist     = 0;            // init last tof distance failed            tsSlot.tsTof[0].u16Val    = INVALID_TOF_DISTANCE;    // init current tof value
            tsSlot.tsTof[1].u16Val    = INVALID_TOF_DISTANCE;    // init last tof value
            tsSlot.tsTof[0].u8Direction = 0xFF;    // init current tof direction
            tsSlot.tsTof[1].u8Direction = 0xFF;    // init last tof direction
            tsAccepted.u16ShortAddr = psMcpsIndData->sFrame.sSrcAddr.uAddr.u16Short;
            tsAccepted.u16SlotIndex = u16IdleIndex;
            EventUtil_vSetEvent(STATION_TOF2CARD_EVENT);
        }
        break;
    }
    case TOF_CARD_ALARM:
    {
        // check len: rf_tof_card_data's len is 8
        if(psFrame->u8SduLength != 8) break;

        LedUtil_vToggle(LED_RF);

        vCheckCardStatus(psMcpsIndData->sFrame.sSrcAddr.uAddr.u16Short, psAppPkt->rf_tof_card_data.u8CardStatus);
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
        return INVALID_TOF_DISTANCE;
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

    switch (eTofStatus)
    {
    case TOF_SUCCESS:
    {
        LedUtil_vToggle(LED_RF);
        tsSlot.u8LostNum = 0;
        i16TofDist = GetTofDistance(asTofData,20);
        i2c_vPrintf("i16TofDist %i\n",i16TofDist);
        if (i16TofDist != INVALID_TOF_DISTANCE)
        {
            i16TofDist = MAX(0, i16TofDist);
            tsSlot.tsTof[0].u16Val = (uint16)i16TofDist;
        }
        else
        {
            DBG(i2c_vPrintf("AAA \n");)
            vCheckCardLost();
        }
        break;
    }
    default:
    {
        DBG(i2c_vPrintf("CCC \n");)
        vCheckCardLost();
        break;
    }
    }
    //fixed me:  if TOF not start, seems that the card can't receive msg any more, so not need to send msg to card, and not need locator
    if(TOF_NOT_STARTED != eTofStatus || tsSlot.u8LostNum < 2)
    {
        DBG(i2c_vPrintf("TOF_START\n");)
        u16PendingAddr = 0x8B5E;
        if(tsSlot.b1TofDir == API_TOF_REVERSE)//API_TOF_REVERSE)
        {
            bIsTofSuccess = TRUE;
            tsSlot.b1TofDir = API_TOF_FORWARDS;
            //tsSlot.tsTof[0].u16Val = (tsSlot.tsTof[0].u16Val +tsSlot.tsTof[1].u16Val)/2;
            DBG(i2c_vPrintf("2\n");)
            vStationCast(TOF_STATION_FINISH, u16PendingAddr, TOF_CARD_NWK_ADDR, MAC_TX_OPTION_ACK, 0);
            DBG(
                i2c_vPrintf("%d: Send Fin to %x \n", u16TofSlotIndex, u16PendingAddr);
            )
        }
        else
        {
            tsSlot.b1TofDir = API_TOF_REVERSE;
            vStationCast(TOF_STATION_WAIT, u16PendingAddr, TOF_CARD_NWK_ADDR, MAC_TX_OPTION_ACK, 0);
            //vStationCast(TOF_STATION_WAIT, u16PendingAddr, TOF_CARD_NWK_ADDR, MAC_TX_OPTION_ACK, 0);
            DBG(
                i2c_vPrintf("%d: Send Wait to %x \n", u16TofSlotIndex, u16PendingAddr);
            )
        }
    }
    else
    {
        DBG(
            i2c_vPrintf("%d: TOF not started %x \n", u16TofSlotIndex, u16PendingAddr);
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

    RfTofWrapper_tu RfTofData;
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
        if((tsSlot.b1Used == 0))
            RfTofData.rf_tof_station_signal.u8LocIdle = 1;
        else
            RfTofData.rf_tof_station_signal.u8LocIdle = 0;

        RfTofData.tof_head.len = 6;    //this is not a 4-byte struct !!!!!!!!!!!
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
        }

        RfTofData.tof_head.len = 8;
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
            //for(ii = 0; ii < TOF_SLOT_NUM; ii++)
            {
                if(tsSlot.u16ShortAddr == u16DstAddr)
                {
                    RfTofData.tof_head.msgtype = TOF_APP_HELP_ACK;
                }
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
    DBG(i2c_vPrintf("Lost\n");)
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
        i2c_vPrintf("-- from ARM: %d --\n", len);
        int8 u8I;
        for(u8I = 0; u8I < len; u8I++)
        i2c_vPrintf("%X  ", pbuf[u8I]);
        i2c_vPrintf("\n");
    )

        if(len < sizeof(struct nwkhdr) + sizeof(app_header_t))
            return;

    RfTofWrapper_tu* psAppPkt = (RfTofWrapper_tu*)(pbuf + sizeof(struct nwkhdr));

    // not for me
    if(APP_PROTOCOL_TYPE_CARD != psAppPkt->tof_head.protocoltype)
        return;

    LedUtil_vToggle(LED_UART);

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
            i2c_vPrintf("report_ack\n");
        )
        app_LSrfReport_t *prfReport = (app_LSrfReport_t *)((app_header_t *)psAppPkt + 1);
        if( prfReport->reporttype == APP_LS_REPORT_LIVE)
        {
            DBG(
                i2c_vPrintf("live report_ack\n");
            )
            u8LastLiveAckSeq = prfReport->seqnum;
            bIsStationUp = TRUE;
            u8RestartCounter = 0;
        }
        break;
    }
    case APP_TOF_MSG_SET:
    {
        DBG(i2c_vPrintf("msg set \n"););
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
                DBG(i2c_vPrintf("CRC failed %d %d\n", prfSet->crc, crc););
            }
        }
        else
        {
            DBG(i2c_vPrintf("head error \n"););
        }
        break;
    }
    default:
        break;
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
                    i2c_vPrintf("%x\t%d\t%d\t%d\t%d\t%d, \t%d\t%d\n", i+CARD_BASE, u32StatTofNum[i], u32StatTofSuccess[i], u32StatTofFailed[i], u32StatTofSkip[i], u32StatTofUnCom[i], u32StatTofLocatorNum[i], u32StatTofLocatorSuccess[i]);
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
                        i2c_vPrintf("%x\t%d\t%d\t%d\t%d\t%d, \t%d\t%d\n", i+CARD_BASE, u32StatTofNum[i], u32StatTofSuccess[i], u32StatTofFailed[i], u32StatTofSkip[i], u32StatTofUnCom[i], u32StatTofLocatorNum[i], u32StatTofLocatorSuccess[i]);
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
        DBG(i2c_vPrintf("Flash Read Fail\n"););
        return FALSE;
    }

    if(!bAHI_FlashEraseSector(7))
    {
        DBG(i2c_vPrintf("Flash Erase Fail\n"););
        return FALSE;
    }
    tsparam.u8TofChannel = u8TofChannel;
    tsparam.u8LocChannel = u8LocChannel;

    if(bAHI_FullFlashProgram(PARAM_STORE_ADDR, sizeof(tsParam_t),(uint8*)&tsparam))
    {
        DBG(i2c_vPrintf("Set  channel Success!%d %d\n", tsparam.u8TofChannel, tsparam.u8LocChannel););
        return TRUE;
    }
    else
    {
        DBG(i2c_vPrintf("Set  channel fail!%d %d\n", tsparam.u8TofChannel, tsparam.u8LocChannel););
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

    DBG(i2c_vPrintf("u16StationPanId %d, u8TmpBroadChannel %d\n",
                                   u16StationPanId, u8TmpBroadChannel);)

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

        DBG(i2c_vPrintf("U %c\n", c);)

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

/****************************************************************************/
/***        END OF FILE                                                   ***/
/****************************************************************************/




