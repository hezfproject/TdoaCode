
/****************************************************************************/
/***        Include files                                                 ***/
/****************************************************************************/
#include <jendefs.h>
#include <AppHardwareApi.h>
#include <AppQueueApi.h>
#include <mac_sap.h>
#include <mac_pib.h>
#include <Utilities.h>
#include <AppApiTof.h>
#include <OAD.h>

#include "config.h"
#include "app_protocol.h"
#include "JN5148_util.h"

/****************************************************************************/
/***        Macro Definitions                                             ***/
/****************************************************************************/

#if (defined DEBUG_LEVEL && DEBUG_LEVEL >= 1)
#define DBG(x) {x}
#else
#define DBG(x)
#endif

#define ABS(x)     ((x) >= 0 ? (x) : (-1)*(x))

#define EVENT_LOC_WAIT_SIGNAL        BIT(1)    // wait station's signal
#define EVENT_LOC_SIGNAL_TIMEOUT     BIT(2)    // wait station's signal timeout
#define EVENT_LOC_REQUEST            BIT(3)    // request to join
#define EVENT_LOC_REQ_TIMEOUT        BIT(4)    // request timeout
#define EVENT_LOC_PENDING            BIT(5)    // wait station to exchange data
#define EVENT_LOC_SEND_CARD_DIST     BIT(6)    // report card distance to station
#define EVENT_LOC_PROCESS_TOF        BIT(7)    // do TOF with card
#define EVENT_LOC_TOF_TIMEOUT        BIT(8)    // TOF timeout
#define EVENT_LOC_WAIT_TOF           BIT(9)    // wait the TOF finish
#define EVENT_LOC_SEND_SHORTADD      BIT(11)
#define EVENT_LOC_CLEAR              BIT(12)
#define EVENT_LOC_SET                BIT(13)
#define EVENT_HALF1SECOND_TICK           BIT(14)
#define EVENT_OFF_LED                BIT(15)
#define EVENT_LOC_WATCHDOG           BIT(30)    // feed watch dog

#define SEND_SHORTADDR_PIN           E_AHI_DIO0_INT
#define RESENT_LOCADDR_COUNT         (20*60) //20MIN

#define PERIOD_SET                   30
#define PERIOD_CLEAR                 60
#define HALF_PERIOD_SET              15
#define HALF_PERIOD_CLEAR            30


//  1: --|__|--|__|--                           period:30ms

//  0: --|_____|-----|_______|--------|_____    period:60ms


#define MAX_READINGS                2
#define TOF_LOC_REQUEST_TIMEOUT    (TOF_SLOT_MS*2)

#define HARD_WATCHDOG_PIN            E_AHI_DIO18_INT


/****************************************************************************/
/***        Type Definitions                                              ***/
/****************************************************************************/
typedef enum
{
    E_STATE_START,
    E_STATE_WAIT_SIGNAL,        // wait station's signal
    E_STATE_WAIT_AVAILABLE,    // find station's signal and wait to join
    E_STATE_REQUEST_JOIN,        // locator request to join
    E_STATE_PENDING,            // locator wait station to exchange data
    E_STATE_REPORT_DATA,        // locator report card distances to station
    E_STATE_TOF_PROCESSING,    // locator do TOF with card
    E_STATE_TOF_FINISH,        // tof is finish
    E_STATE_TOF_IDLE,            // locator is idle
} teState;


/****************************************************************************/
/***        Local Function Prototypes                                     ***/
/****************************************************************************/
PRIVATE void vColdInitSystem(void);
PRIVATE void vInitAhiDio(uint32 u32AhiDio , uint32 u32OnOrOff);
PRIVATE void vStartDevice(void);
PRIVATE void vProcessSysEventQueues(void);
PRIVATE void vProcessIncomingMlme(MAC_MlmeDcfmInd_s *psMlmeInd);
PRIVATE void vProcessIncomingMcps(MAC_McpsDcfmInd_s *psMcpsInd);
PRIVATE void vProcessIncomingHwEvent(AppQApiHwInd_s *psAHI_Ind);
PRIVATE void vHandleMcpsDataInd(MAC_McpsDcfmInd_s *psMcpsInd);
PRIVATE void vHandleMcpsDataDcfm(MAC_McpsDcfmInd_s *psMcpsInd);
PRIVATE void vProcessAppEventQueues(void);

PRIVATE void vProcessLocatorCmd(MAC_McpsIndData_s *psMcpsIndData);
PRIVATE void vLocProcessAirMsg(MAC_McpsIndData_s *psMcpsInd);
PRIVATE void vLocTofCallback(eTofReturn eStatus);
PRIVATE void vProcessTofData();
PRIVATE void vLocatorCast(uint8 cmd_type);
PRIVATE void vSendCardFinish();
PRIVATE void vSetLocChannel(uint8 channel);
PRIVATE void vUart0_rx_callback(uint32 u32DeviceId, uint32 u32ItemBitmap);

PRIVATE void vSyncLocator(uint16 u16LocShortAdd);
PRIVATE void vLoctoCardProcess(void);


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
PRIVATE uint16 u16LocatorShortAddr;
PRIVATE uint16 u16StationShortAddr;
PRIVATE uint16 u16StationPanId;

PRIVATE uint16 u16BoundStationPanId =0;

PRIVATE uint8  u8StationChannel;        // station to send card info channel
PRIVATE uint8  u8CardChannel;            // to do tof with card's channel
PRIVATE uint8  u8BroadcastChannel;    // station send signal msg channel
PRIVATE uint8  u8CurChannel = 0x00;    // current channel

PRIVATE uint16 u16LocSlot;            // my slot index in station
PRIVATE uint8  u8StationRunMs;            // station ran ms in the current slot
PRIVATE uint32 u32RecTick;
//PRIVATE uint32 u32SRec = 0;
PRIVATE uint16 u16CardAddr[TOF_SLOT_LOC_INT];    // record the cards' addr list which need locator (maximun is TOF_SLOT_LOC_INT)
PRIVATE uint16 u16CardDist[TOF_SLOT_LOC_INT];    // record the tof distance between the card(from u16CardAddr) & locator, the order is same to u16CardAddr
PRIVATE uint8  u8CardTofIndex;    // current tof index
PRIVATE uint8  u8CardTofCount;

PRIVATE bool_t bConnected = FALSE;            // to record if locator is connected with station
PRIVATE uint8  u8DisconnectNum = 0;     // 40 times disconnect,restart LOC

PRIVATE uint16 u16ReSentLocAddNum = 0;      // 20 minutes resend LOC's shortaddress

PRIVATE uint8  u8SyncAddrCount = 0;
PRIVATE int_8  i8LocatorRecRssi = INVALID_RSSI;      //to record locator receive rssi

PRIVATE uint8  u8LastHandle = 0xFF;
PRIVATE eTofReturn eTofStatus = -1;
PRIVATE tsAppApiTof_Data asTofData[MAX_READINGS];

PRIVATE teState eLocState;
PRIVATE bool_t  bDetailedPrint=FALSE;
//PRIVATE uint8   u8RecCardInfoCount = 0;

/****************************************************************************/
/***        Exported Functions                                            ***/
/****************************************************************************/
/****************************************************************************
 ****************************************************************************/
PUBLIC void AppColdStart(void)
{
    // Watch dog is enabled by default
    vColdInitSystem();

    vOADInitClient(TRUE);
    vStartDevice();

    DBG(
        PrintfUtil_u8Init(E_AHI_UART_0, PRINTF_BAUD_RATE_1M);
        DebugUtil_vInit(E_AHI_UART_0, vUart0_rx_callback);
        PrintfUtil_vPrintf("cold start\n");
    )

#if (defined SUPPORT_HARD_WATCHDOG)
    vInitAhiDio(HARD_WATCHDOG_PIN,0);//init hard watch dog GPIO
#endif
    vInitAhiDio(SEND_SHORTADDR_PIN,0);
    vInitAhiDio(JN_RED,1);           //turn on red light

    EventUtil_vSetEvent(EVENT_LOC_SEND_SHORTADD);
    TimerUtil_eSetCircleTimer(EVENT_HALF1SECOND_TICK,500);
    TimerUtil_eSetCircleTimer(EVENT_LOC_WATCHDOG, 502);    // feed watch dog every 500 ms

    while (1)
    {
        TimerUtil_vUpdate();
        vProcessSysEventQueues();
        vProcessAppEventQueues();
    }
}

/****************************************************************************
 ****************************************************************************/
PUBLIC void AppWarmStart(void)
{
    AppColdStart();
}

/****************************************************************************/
/***        Local Functions                                               ***/
/****************************************************************************/
PRIVATE void vColdInitSystem(void)
{
    (void)u32AppQApiInit(NULL, NULL, NULL);
    (void)u32AHI_Init();

    vAHI_HighPowerModuleEnable(TRUE, TRUE);
    vAHI_WakeTimerEnable(E_AHI_WAKE_TIMER_0, TRUE);
    vAppApiTofInit(TRUE);

    MacUtil_vReadExtAddress(&psMacAddr);

    uint32 tmp32;
    tmp32 = StringUtil_BCD2uint32(psMacAddr.u32H);

    u16LocatorShortAddr = tmp32%100000;

    uint8 channel = ((psMacAddr.u32L) >> 16) & 0xFF;

    if(channel >=11 && channel <=26)
    {
        u8BroadcastChannel = channel;
    }
    else
    {
        u8BroadcastChannel = DEFAULT_CHANNEL_BROADCAST;
    }

    u8CurChannel = u8BroadcastChannel;

    MacUtil_Setting_s sMacUtil;
    sMacUtil.u16SrcPanId     = TOF_LOCNODE_NWK_ADDR;
    sMacUtil.u16SrcShortAddr = u16LocatorShortAddr;
    sMacUtil.u16Profile_id     = 0x2001; //for backward compatable
    sMacUtil.u8Dst_endpoint     = 0x21;
    sMacUtil.u8NodeType     = 0;
    sMacUtil.u8Src_endpoint     = 0;
    MacUtil_vInit(&sMacUtil);
}

PRIVATE void vInitAhiDio(uint32 u32AhiDio , uint32 u32OnOrOff)
{
    if(u32AhiDio <= E_AHI_DIO19_INT && u32AhiDio >= E_AHI_DIO0_INT)
    {
        vAHI_DioSetDirection(0, u32AhiDio);
        if(u32OnOrOff)
        {
            vAHI_DioSetOutput(0,u32AhiDio);
        }
        else
        {
            vAHI_DioSetOutput(u32AhiDio,0);
        }
    }
    else
    {
        DBG(PrintfUtil_vPrintf( "Error Dio: %d\n", u32AhiDio);)
    }
}

PRIVATE void vProcessSysEventQueues(void)
{
    MAC_MlmeDcfmInd_s *psMlmeInd;
    MAC_McpsDcfmInd_s *psMcpsInd;
    AppQApiHwInd_s    *psAHI_Ind;

    /* Check for anything on the MCPS upward queue */
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

    /* Check for anything on the MLME upward queue */
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

    /* Check for anything on the AHI upward queue */
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
 * NAME: vProcessAppEventQueues
 *
 ****************************************************************************/
PRIVATE void vProcessAppEventQueues(void)
{
    uint32 event=EventUtil_u32ReadEvents();
    if(event)
    {
        switch (event)
        {
        case EVENT_LOC_REQUEST:
        {
            DBG(
                PrintfUtil_vPrintf( "request: %d\n", u32AHI_TickTimerRead());
            )
            vSetLocChannel(u8BroadcastChannel);

            MacUtil_SendParams_s sMacParam;
            sMacParam.u16ClusterId = 0;
            sMacParam.u16DstAddr = u16StationShortAddr;
            sMacParam.u16DstPanId = u16StationPanId;
            sMacParam.u8DeliverMode = MAC_UTIL_UNICAST;
            sMacParam.u8Radius = 1;

            //PrintfUtil_vPrintf( "add: %d,Panid: %d\n",u16StationShortAddr,u16StationPanId );

            RfTofWrapper_tu RfTofData;
            RfTofData.tof_head.protocoltype = APP_PROTOCOL_TYPE_CARD;
            RfTofData.tof_head.msgtype = TOF_LOCATOR_REQUEST;
            RfTofData.tof_head.len = 0;

            MacUtil_vSendData(&sMacParam, (uint8*)&RfTofData, 4, MAC_TX_OPTION_ACK);

            eLocState = E_STATE_REQUEST_JOIN;
            TimerUtil_eSetTimer(EVENT_LOC_REQ_TIMEOUT, TOF_LOC_REQUEST_TIMEOUT);
            EventUtil_vUnsetEvent(event);
            break;
        }

        case EVENT_LOC_REQ_TIMEOUT:
        {
            DBG(
                PrintfUtil_vPrintf( "request timeout: %d\n", u32AHI_TickTimerRead());
            )
            eLocState = E_STATE_WAIT_AVAILABLE;
            EventUtil_vUnsetEvent(event);
            break;
        }

        case EVENT_LOC_PENDING:
        {
            DBG(
             //   PrintfUtil_vPrintf( "pending\n");
            )
            eLocState = E_STATE_PENDING;
            vSetLocChannel(u8StationChannel);
            EventUtil_vUnsetEvent(EVENT_LOC_PENDING);
            break;
        }

        case EVENT_LOC_SEND_CARD_DIST:
        {
            DBG(
              //  PrintfUtil_vPrintf( "report: %d\n",eLocState);
            )
            vLocatorCast(TOF_LOCATOR_CARDS_DIST);
            eLocState = E_STATE_TOF_IDLE;
            EventUtil_vUnsetEvent(event);
            break;
        }

        case EVENT_LOC_PROCESS_TOF:
        {
            //PrintfUtil_vPrintf("-");
            vLoctoCardProcess();
            EventUtil_vUnsetEvent(event);
            break;
        }

        case EVENT_LOC_TOF_TIMEOUT:
        {
            TimerUtil_eStopTimer(EVENT_LOC_WAIT_TOF);//add it
            EventUtil_vUnsetEvent(EVENT_LOC_WAIT_TOF);
            if(eTofStatus == -1)
            {
                vAppApiTofInit(FALSE);
                vAppApiTofInit(TRUE);
            }
            eTofStatus = -1;
            //eLocState = E_STATE_PENDING;

            EventUtil_vSetEvent(EVENT_LOC_PROCESS_TOF);
            EventUtil_vUnsetEvent(event);
            //DBG(PrintfUtil_vPrintf( "%d-%d time out\n", u8CardTofIndex,u16CardAddr[u8CardTofIndex]);)
            break;
        }

        case EVENT_LOC_WAIT_TOF:
        {
            if(eTofStatus != -1)
            {
                eLocState = E_STATE_TOF_FINISH;

                vProcessTofData();

                //eTofStatus = -1;

                vSendCardFinish();

                EventUtil_vUnsetEvent(event);
            }
            //do not clear event
            break;
        }

        case EVENT_HALF1SECOND_TICK:
        {
            if(bConnected)
            {
                static uint8 u8Idx = 0;
                if(u8Idx++%2)
                {
                    //PrintfUtil_vPrintf( "on\n");
                    vAHI_DioSetOutput(0, JN_RED);   //turn on red light
                    TimerUtil_eSetTimer(EVENT_OFF_LED, 50);
                }
            }

            // 20s内收不到一个定位finish 则重传地址
            if(u8DisconnectNum++ >= TOF_STATION_LOCATOR_MAX_FAILED)
            {
                if(bConnected)
                {
                    DBG(PrintfUtil_vPrintf( "resent2\n");)
                    EventUtil_vSetEvent(EVENT_LOC_SEND_SHORTADD);
                    u16BoundStationPanId = 0;
                    eLocState = E_STATE_TOF_IDLE;
                    bConnected = FALSE;
                }
                //  40s内收不到一个定位finish 则重起
                if(u8DisconnectNum >= TOF_STATION_LOCATOR_MAX_FAILED*2)
                {
                    DBG(PrintfUtil_vPrintf( "reset: %d\n",u8DisconnectNum);)
                    vAHI_SwReset();          //loc will restart if failed 20+20 times
                }
            }

            EventUtil_vUnsetEvent(event);
            break;
        }

        case EVENT_LOC_CLEAR:
        {
           vAHI_DioSetOutput(0,SEND_SHORTADDR_PIN);
           if(u8SyncAddrCount >= 15)
           {
               u8SyncAddrCount = 0;
               if(u16BoundStationPanId == 0 || bConnected == FALSE)
               {
                    DBG(PrintfUtil_vPrintf( "resent1\n");)
                    TimerUtil_eSetTimer(EVENT_LOC_SEND_SHORTADD,4000);
               }
               //pull up for save power
               TimerUtil_eSetTimer(EVENT_LOC_SET,5);
           }
           EventUtil_vUnsetEvent(event);
           break;
        }

        case EVENT_LOC_SET:
        {
           vAHI_DioSetOutput(SEND_SHORTADDR_PIN,0);
           EventUtil_vUnsetEvent(event);
           break;
        }

        case EVENT_LOC_SEND_SHORTADD:
        {
           u16ReSentLocAddNum = 0;
           vSetLocChannel(u8BroadcastChannel);
           vSyncLocator(u16LocatorShortAddr);
           EventUtil_vUnsetEvent(event);
           break;
        }

        case EVENT_LOC_WATCHDOG:
        {
            //DBG(PrintfUtil_vPrintf("W:%d\n",eLocState);)
            #if (defined SUPPORT_HARD_WATCHDOG)
            vFeedHardwareWatchDog();
            #endif
            vAHI_WatchdogRestart();
            EventUtil_vUnsetEvent(event);
            break;
        }

        case EVENT_OFF_LED:
        {
            vAHI_DioSetOutput(JN_RED,0);   //turn off red light
            //PrintfUtil_vPrintf("off\n");
            EventUtil_vUnsetEvent(event);
            break;
        }

        default:
        {
            EventUtil_vUnsetEvent(event);
            break;
        }
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
}

/****************************************************************************
 *
 * NAME: vProcessIncomingData
 *
 ****************************************************************************/
PRIVATE void vProcessIncomingMcps(MAC_McpsDcfmInd_s *psMcpsInd)
{
    /* Only handle incoming data events one device has been started as a
       coordinator */
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
 * NAME: vHandleMcpsDataDcfm
 *
 ****************************************************************************/
PRIVATE void vHandleMcpsDataDcfm(MAC_McpsDcfmInd_s *psMcpsInd)
{
    if((eLocState == E_STATE_TOF_FINISH) && (psMcpsInd->uParam.sDcfmData.u8Handle == u8LastHandle))
    {
        //eLocState = E_STATE_TOF_IDLE;
        //EventUtil_vSetEvent(EVENT_LOC_PROCESS_TOF);
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

    if(APP_PROTOCOL_TYPE_CARD == psAppPkt->tof_head.protocoltype)
    {
        vLocProcessAirMsg(&psMcpsInd->uParam.sIndData);
        return;
    }

    if( APP_PROTOCOL_TYPE_CMD  == psAppPkt->tof_head.protocoltype )
    {
        vProcessLocatorCmd(&psMcpsInd->uParam.sIndData);
        return;
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
PRIVATE void vStartDevice(void)
{
    TimerUtil_vInit();

    eLocState = E_STATE_START;

    s_pvMac = pvAppApiGetMacHandle();
    s_psMacPib = MAC_psPibGetHandle(s_pvMac);

    MAC_vPibSetPanId(s_pvMac, TOF_LOCNODE_NWK_ADDR);
    MAC_vPibSetShortAddr(s_pvMac, u16LocatorShortAddr);

    MAC_vPibSetRxOnWhenIdle(s_pvMac, TRUE, FALSE);
    MAC_vPibSetMaxCsmaBackoffs(s_pvMac, 2);
    MAC_vPibSetMinBe(s_pvMac, 1);

    s_psMacPib->u8MaxFrameRetries     =1;
    s_psMacPib->sCoordExtAddr.u32H    = 0x1111;    // fake
    s_psMacPib->sCoordExtAddr.u32L     = 0x2222;    // fake
    s_psMacPib->u16CoordShortAddr     = 0x0000;    // fake
    s_psMacPib->u8SecurityMode         = 0;

    eAppApiPlmeSet(PHY_PIB_ATTR_CURRENT_CHANNEL, u8CurChannel);
}


PRIVATE void vLocTofCallback(eTofReturn eStatus)
{
    eTofStatus = eStatus;
}

PRIVATE void vProcessLocatorCmd(MAC_McpsIndData_s *psMcpsIndData)
{
    MAC_RxFrameData_s *psFrame;
    psFrame = &psMcpsIndData->sFrame;
    RfTofWrapper_tu* psAppPkt = (RfTofWrapper_tu*)(psFrame->au8Sdu);

    switch(psAppPkt->tof_head.msgtype)
    {
    case TOF_LOCATOR_OAD:
    {
        if((psAppPkt->tof_head.len == 4)
                && (psFrame->sDstAddr.u16PanId == TOF_LOCNODE_NWK_ADDR)
                &&(psFrame->sDstAddr.uAddr.u16Short == u16LocatorShortAddr)
                && (psAppPkt->rf_tof_oad_data.u8DeviceType == DEVICE_TYPE_LOCARTOR)
                && (psAppPkt->rf_tof_oad_data.u16Version > OAD_LOCATOR_VERSION))
        {
            TimerUtil_vStopAllTimer();
            EventUtil_vResetAllEvents();

            LedUtil_vFlashAll(500, 10);

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
 * NAME: vLocProcessAirMsg
 *
 ****************************************************************************/

PRIVATE void vLocProcessAirMsg(MAC_McpsIndData_s *psMcpsIndData)
{
    MAC_RxFrameData_s *psFrame;
    psFrame = &psMcpsIndData->sFrame;

    RfTofWrapper_tu* psAppPkt = (RfTofWrapper_tu*)(psFrame->au8Sdu);

    if(APP_PROTOCOL_TYPE_CARD != psAppPkt->tof_head.protocoltype)
        return;

    rf_tof_msg_te tTofCmdType = psAppPkt->tof_head.msgtype;
    if(tTofCmdType == TOF_STATION_LOC_INFO)
    {
        DBG(PrintfUtil_vPrintf( "loc info\n");)
        u16BoundStationPanId = psFrame->sSrcAddr.u16PanId;
        TimerUtil_eStopTimer(EVENT_LOC_SEND_SHORTADD);
        EventUtil_vUnsetEvent(EVENT_LOC_SEND_SHORTADD);
        eLocState = E_STATE_WAIT_SIGNAL;
        return;
    }
    //FIXME, for beta only
    if(u16BoundStationPanId != psFrame->sSrcAddr.u16PanId
        &&!(psFrame->sSrcAddr.u16PanId ==0xffff&&tTofCmdType==TOF_STATION_CARDS_INFO))
        return;
    i8LocatorRecRssi = SysUtil_vConvertLQI2Dbm(psFrame->u8LinkQuality);

    switch(tTofCmdType)
    {
    case TOF_STATION_ILDE:
    case TOF_STATION_BUSY:
    {
        if ((eLocState == E_STATE_WAIT_SIGNAL)
                && (psAppPkt->rf_tof_station_signal.u8LocIdle&HAVE_IDLE_LOC_SLOT))
        {
            DBG(PrintfUtil_vPrintf( "idle and busy\n");)
            eLocState = E_STATE_WAIT_AVAILABLE;
        }

        break;
    }

    case TOF_STATION_AVAILABLE:        // for card and locator to join
    case TOF_STATION_RSSI:            // only for locator to join, and for card to send RSSI
    {
        if (((eLocState == E_STATE_WAIT_SIGNAL) || (eLocState == E_STATE_WAIT_AVAILABLE))
                && (psAppPkt->rf_tof_station_signal.u8LocIdle&HAVE_IDLE_LOC_SLOT))
        {
            eLocState = E_STATE_REQUEST_JOIN;
            EventUtil_vSetEvent(EVENT_LOC_REQUEST);

            u16StationShortAddr = psFrame->sSrcAddr.uAddr.u16Short;
            u16StationPanId = psFrame->sSrcAddr.u16PanId;
        }

        break;
    }

    case TOF_STATION_ACCEPT:
    {
        if (eLocState == E_STATE_REQUEST_JOIN)    // to avoid receive TOF_STATION_CARDS_INFO again
        {
            if(psAppPkt->rf_tof_station_accept.tsAccptData[0].u16ShortAddr == u16LocatorShortAddr)
            {

                //TimerUtil_vStopAllTimer();
                //EventUtil_vResetAllEvents();
                TimerUtil_eStopTimer(EVENT_LOC_REQ_TIMEOUT);
                EventUtil_vUnsetEvent(EVENT_LOC_REQ_TIMEOUT);

                u8DisconnectNum = 0;
                bConnected = TRUE;

                u8StationChannel = psAppPkt->rf_tof_station_accept.u8StationChannel;
                u8CardChannel = psAppPkt->rf_tof_station_accept.u8LocatorCardChannel;
                u16LocSlot = psAppPkt->rf_tof_station_accept.tsAccptData[0].u16SlotIndex;
                DBG(PrintfUtil_vPrintf( "accept:%d  %d\n",u16LocSlot,u32AHI_TickTimerRead());)

                EventUtil_vSetEvent(EVENT_LOC_PENDING);
                break;
            }
        }

        break;
    }

    case TOF_STATION_CARDS_INFO:
    {
        if (eLocState == E_STATE_PENDING)    // to avoid receive TOF_STATION_CARDS_INFO again
        {
            u32RecTick = u32AHI_TickTimerRead();
            u8StationRunMs = psAppPkt->rf_tof_station_card_info.u8RunMs;

            u8DisconnectNum = 0;
            //DBG(PrintfUtil_vPrintf( "card info1: %d %d\n",u8StationRunMs,u32AHI_TickTimerRead()/16000);)

            if (psAppPkt->rf_tof_station_card_info.u8CardNum == (TOF_SLOT_LOC_INT - 2))    // max card num is TOF_SLOT_LOC_INT-2
            {
                memcpy(u16CardAddr, psAppPkt->rf_tof_station_card_info.u16CardAddr, (TOF_SLOT_LOC_INT - 2)*2);
                u8CardTofCount = 0;

                uint16 runTime = (uint16)(u32AHI_TickTimerRead() - u32RecTick)/16000 + u8StationRunMs;

                //u32SRec = u32AHI_TickTimerRead();

                //； station--3ms-->locator
                if(runTime < (TOF_SLOT_MS*2 - 1))
                {
                      //PrintfUtil_vPrintf( "do tof to card1: %d  %d\n",runTime,u8StationRunMs);
                      TimerUtil_eSetTimer(EVENT_LOC_PROCESS_TOF,TOF_SLOT_MS*2 - 1- runTime);
                }
                else
                {
                      //PrintfUtil_vPrintf( "do tof to card2:%d %d\n",runTime,u8StationRunMs);
                      PrintfUtil_vPrintf("time pass \n");
                      EventUtil_vSetEvent(EVENT_LOC_PROCESS_TOF);
                }
            }
            else
            {
                TimerUtil_eSetTimer(EVENT_LOC_PENDING,1);
                DBG(PrintfUtil_vPrintf( "card len error: %d \n",psAppPkt->rf_tof_station_card_info.u8CardNum);)
            }
            eLocState = E_STATE_REPORT_DATA;

        }
        if(u16LocSlot == 1)//#1 locator
        {
            //退避#0 locator
            TimerUtil_eSetTimer(EVENT_LOC_SEND_CARD_DIST, 4);
        }
        else //#0 locator
        {
            EventUtil_vSetEvent(EVENT_LOC_SEND_CARD_DIST);
        }
        break;
    }

    case TOF_LOC_VERSION_INFO:
    {
        vLocatorCast(TOF_LOC_VERSION_INFO);
        break;
    }

    case TOF_STATION_FINISH:
    {
        DBG(PrintfUtil_vPrintf( "finish\n");)
        bConnected = TRUE;
        u8DisconnectNum = 0;
        break;
    }

    default:
        break;
    }
}


/****************************************************************************
 *
 * NAME: vProcessTofData
 *
 ****************************************************************************/
PRIVATE void vProcessTofData()
{
    if (eTofStatus == TOF_SUCCESS)
    {
        int16 dist = i16GetTofDistance(asTofData, MAX_READINGS);

        if (dist != INVALID_TOF_DISTANCE)
        {
            u16CardDist[u8CardTofIndex] = MAX(0, dist);    // u8CardTofIndex++ when call tof
            //DBG(PrintfUtil_vPrintf( "%d - %d: %d \n",u8CardTofIndex,u16CardAddr[u8CardTofIndex],u16CardDist[u8CardTofIndex]);)
            //PrintfUtil_vPrintf( "ok\n");
        }
        else
        {
            DBG(
                PrintfUtil_vPrintf( "%d < 0\n", u16CardAddr[u8CardTofIndex]);
            )
        }
    }
    else
    {
        DBG(
            PrintfUtil_vPrintf( "%d failed: %d\n", u16CardAddr[u8CardTofIndex], eTofStatus);
        )
    }
}


/****************************************************************************
 *
 * NAME: vLocatorCast
 *        1. Send card's L-C distance to station. Only send the distance, not need to send their
 *        addr because station know the addr (the distance order is same to addr list)
 *        2.Send Locator Version to station
 *
 ****************************************************************************/
PRIVATE void vLocatorCast(uint8 cmd_type)
{
    MacUtil_SendParams_s sParams;
    sParams.u8Radius        = 1;
    sParams.u16DstAddr    = u16StationShortAddr;
    sParams.u16DstPanId     = u16StationPanId;
    sParams.u16ClusterId     = 0;
    sParams.u8DeliverMode    = MAC_UTIL_UNICAST;

    RfTofWrapper_tu RfData;

    RfData.tof_head.protocoltype = APP_PROTOCOL_TYPE_CARD;
    RfData.tof_head.msgtype = cmd_type;

    switch(cmd_type)
    {
    case TOF_LOCATOR_CARDS_DIST:
    {
        RfData.tof_head.len = ((TOF_SLOT_LOC_INT-2+2)/2)*4;  // better than (TOF_SLOT_LOC_INT-2)*2+2 for 32CPU
        RfData.rf_tof_locator_card_info.u8CardNum = TOF_SLOT_LOC_INT-2;
        RfData.rf_tof_locator_card_info.i8Rssi = i8LocatorRecRssi;

        memcpy(RfData.rf_tof_locator_card_info.u16CardDist, u16CardDist, (TOF_SLOT_LOC_INT-2)*2);
        MacUtil_vSendData(&sParams, (uint8*)&RfData, RfData.tof_head.len+4, 0);
        break;
    }

    case TOF_LOC_VERSION_INFO:
    {
        RfData.tof_head.len = strlen(VERSION) + sizeof(RfData.rt_tof_loc_version.u16LocOAD);
        RfData.rt_tof_loc_version.u16LocOAD = OAD_LOCATOR_VERSION;
        memcpy(RfData.rt_tof_loc_version.u8LocVersion,VERSION,strlen(VERSION));
        MacUtil_vSendData(&sParams, (uint8*)&RfData, RfData.tof_head.len+4, 0);
        break;
    }

    default:
        break;

    }
}


/****************************************************************************
 *
 * NAME: vSendCardFinish
 *         Send finish to card
 *
 ****************************************************************************/
PRIVATE void vSendCardFinish(void)
{
    MacUtil_SendParams_s sParams;
    sParams.u8Radius        = 1;
    sParams.u16DstAddr    = u16CardAddr[u8CardTofIndex];
    sParams.u16DstPanId     = TOF_CARD_NWK_ADDR;
    sParams.u16ClusterId     = 0;
    sParams.u8DeliverMode    = MAC_UTIL_UNICAST;

    RfTofWrapper_tu RfData;
    RfData.tof_head.protocoltype = APP_PROTOCOL_TYPE_CARD;
    RfData.tof_head.msgtype = TOF_LOCATOR_FINISH;
    RfData.tof_head.len = 0;  //u16CardDist[u8CardTofIndex]

    u8LastHandle =  MacUtil_vSendData(&sParams, (uint8*)&RfData, 4, 0);    // not ack to shorter locator's time
}

PRIVATE void vSetLocChannel(uint8 channel)
{
    if(u8CurChannel != channel)
    {
        u8CurChannel = channel;
        eAppApiPlmeSet(PHY_PIB_ATTR_CURRENT_CHANNEL, channel);
    }
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
        case 'L':
            bDetailedPrint = TRUE;
            break;
        case 'l':
            bDetailedPrint = FALSE;
            break;
        default:
            break;
        }
    }
}

void vSyncLocator(uint16 data)
{
    uint16 sData;

    sData=data;
    if(u8SyncAddrCount == 1)
    {
        vAHI_DioSetOutput(0, JN_RED);   //turn on red light
    }
    else if(u8SyncAddrCount == 2)
    {
        vAHI_DioSetOutput(JN_RED,0);   //turn off red light
    }

    vAHI_DioSetOutput(0,SEND_SHORTADDR_PIN);
    if(u8SyncAddrCount <= 15)
    {
        DBG(PrintfUtil_vPrintf("SendAddr: %d\n", u8SyncAddrCount);)
        if((sData>>u8SyncAddrCount)&0x01)//;1
        {
             TimerUtil_eSetTimer(EVENT_LOC_SET,HALF_PERIOD_SET);
             if(u8SyncAddrCount == 15)//if LOC Add send finish
             {
                TimerUtil_eSetTimer(EVENT_LOC_CLEAR,HALF_PERIOD_SET*2);
             }
             else //else send next bit
             {
                TimerUtil_eSetTimer(EVENT_LOC_SEND_SHORTADD,PERIOD_SET);
             }
        }
        else//;0
        {
             TimerUtil_eSetTimer(EVENT_LOC_SET,HALF_PERIOD_CLEAR);
             if(u8SyncAddrCount == 15)
             {
                TimerUtil_eSetTimer(EVENT_LOC_CLEAR,HALF_PERIOD_CLEAR*2);
             }
             else
             {
                TimerUtil_eSetTimer(EVENT_LOC_SEND_SHORTADD,PERIOD_CLEAR);
             }
        }
        u8SyncAddrCount++;
    }
    else
    {
        DBG(PrintfUtil_vPrintf("SendAddr error: %d\n", u8SyncAddrCount);)
    }

}

PRIVATE void vLoctoCardProcess(void)
{
    if (u8CardTofCount == 0) // first card, need init all card's dist
    {
        uint8 u8tmp;
        //PrintfUtil_vPrintf( "clear\n");
        for(u8tmp = 0; u8tmp < TOF_SLOT_LOC_INT; u8tmp++)
        {
            u16CardDist[u8tmp] = INVALID_TOF_DISTANCE;
        }
    }
    if (u8CardTofCount < TOF_SLOT_LOC_INT-2)
    {
        uint16 u16RunMs =(uint16)((u32AHI_TickTimerRead() - u32RecTick)/16000);
        // check if have enough time to do TOF
        if(u8StationRunMs + u16RunMs + TOF_SLOT_APPR_MS_2 >= TOF_LOCATOR_PERIOD_MS)
        {
            // not time left to finish the tof processing, go to EVENT_LOC_PENDING to wait station's next card info
            vSetLocChannel(u8StationChannel);
            EventUtil_vSetEvent(EVENT_LOC_PENDING);
            DBG(PrintfUtil_vPrintf( "no time left:%d\n",u8CardTofCount);)
        }
        else
        {
            uint8 u8lagSlot = 0;
            if(u16LocSlot == 1)//#1 locator
            {
                u8lagSlot = 10;
            }

            u8CardTofIndex = (u8CardTofCount + u8lagSlot)%(TOF_SLOT_LOC_INT - 2);
            if(u16CardAddr[u8CardTofIndex] != 0)
            {
                MAC_Addr_s sAddr;
                sAddr.u8AddrMode = 2;
                sAddr.u16PanId = TOF_CARD_NWK_ADDR;
                sAddr.uAddr.u16Short = u16CardAddr[u8CardTofIndex];
                vSetLocChannel(u8CardChannel);
                if(FALSE == bAppApiGetTof(asTofData, &sAddr, MAX_READINGS, API_TOF_FORWARDS, vLocTofCallback))
                    DBG(PrintfUtil_vPrintf( "APP Tof fail\n");)
                eLocState = E_STATE_TOF_PROCESSING;

                EventUtil_vSetEvent(EVENT_LOC_WAIT_TOF);
            }

            //u32SRec = u32AHI_TickTimerRead();

            u8CardTofCount++;

            TimerUtil_eSetTimer(EVENT_LOC_TOF_TIMEOUT, TOF_SLOT_APPR_MS_2 + 5);
        }
    }
    else
    {
        EventUtil_vSetEvent(EVENT_LOC_PENDING);
        //DBG(PrintfUtil_vPrintf( "TOF e: %d  %d\n",(u32AHI_TickTimerRead()-u32RecTick)/16000,u8CardTofCount);)
    }
}

/****************************************************************************/
/***        END OF FILE                                                   ***/
/****************************************************************************/





