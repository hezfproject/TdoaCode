
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
#include "string.h"

/****************************************************************************/
/***        Macro Definitions                                             ***/
/****************************************************************************/

#if (defined DEBUG_LOC_APP)
#define DBG(x) do{x}while(0);
#else
#define DBG(x)
#endif

// for errors
#if (defined DEBUG_ERROR)
#define EDBG(x) do{x} while (0);
#else
#define EDBG(x)
#endif

#define LED_UART0         E_AHI_DIO9_INT
#define LED_UART1         E_AHI_DIO8_INT
#define LED_RF        	  E_AHI_DIO16_INT

#define CTRL_HARD_WATCHDOG E_AHI_DIO18_INT
#define CTRL_ADDR_SYNC_0   E_AHI_DIO0_INT
#define CTRL_ADDR_SYNC_1   E_AHI_DIO12_INT


#define    STATION_SLOT_EVENT                BIT(13)    // to deal with slot timer
#define    STATION_SEND_CARD_INFO_EVENT    	 BIT(14)    // send card info to locator to indicate which cards need locator
#define    STATION_TOF2CARD_EVENT            BIT(15)    // tof with card
#define    STATION_WAIT_TOF2CARD_EVENT       BIT(16)    // wait tof with card finish
#define    STATION_CLAIM_EVENT               BIT(17)    // station claim IDLE or BUSY
#define    STATION_AVAILABLE_EVENT           BIT(18)    // station available, card can join
#define    STATION_RSSI_EVENT                BIT(19)    // card card send rssi
#define    STATION_ACCEPT_EVENT              BIT(20)    // station deal with card join request
#define    STATION_LOCTOF_TIMEOUT_EVENT      BIT(21)    // when loc join, need tof to determine loc's distance
#define    STATION_TOF_REPORT_EVENT          BIT(22)    // station report card's distance / rssi
#define    STATION_PORT_QUERY_EVENT          BIT(23)    // query if station up
#define    STATION_REPORT_STATUS             BIT(24)    // report station status
#define    STATION_REPORT_CARDVERSION        BIT(25)    // report card version
//#define    STATION_REPORT_GASDENSITY       BIT(26)    //
#define    STATION_REPORT_STATION_RSSI       BIT(27)    //report locator's rssi
#define    STATION_REPORT_RESET_EVENT        BIT(29)    //report station reset
//#define    STATION_REQUEST_LOC_VER           BIT(28)    //request locator's version
#define    STATION_WATCHDOG_EVENT            BIT(30)    // feed watch dog
#define    STATION_LOCTOF_CHECK_EVENT        BIT(31)    // check the L-C tof if it is done


#define     STATION_ACCEPT_CARD_MAX_NUM     5    // when station available, only 5 card can join per time
#define     STATION_TOF_MAX_READINGS        2
#define     STATION_TOF_MIN_DISTANCE        15     // when the distance between card and station is small than 15 meters, not need locator
#define     STATION_TOF_LOC_DIST_NUM        10     // when locator join, need STATION_TOF_LOC_DIST_NUM times tof to determine locator's distance
#define     STATION_TOF_LOC_DIST_TRY_NUM    30     // The mac locator join tof time

// 辅站号码同步
#define    LOC_ADD_SET_PERIOD               30       // 1's period:30ms
#define    LOC_ADD_CLEAR_PERIOD             60       // 0's period:60ms

//define slot type
#define SLOT_CARD              0
#define SLOT_LOCATOR       	   1
#define SLOT_IDLE              2
//#define SLOT_GAS_TOF         3
//#define SLOT_GAS_DENSITY     4

#define SINGLE_CARD_RETREAT_SIZE    100      //support single card retreat number
#define SINGLE_CARD_RETREAT_TIMEOUT 60       //

#define CARD_SPEED_REMIND_SIZE    50      //support single card retreat number
#define CARD_SPEED_REMIND_TIMEOUT 60       //

#define STATION_LOG    "Station:" 			 // station tag:use to report station's version
#define LOCATOR0_LOG    "LOC0:"       	 // locator tag:use to report locator's version
#define LOCATOR1_LOG    "LOC1:"
#define VERSION_LENGTH  100

#define ABS(x)     ((x) >= 0 ? (x) : (-1)*(x))

#define PARAM_STORE_ADDR 	0x70000

#define REPORT_STATION_RSSI_TIME     (5*60)
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
    uint8 b1Used:1;         // 0: idle, 1: used
    uint8 b2SlotType:3;     //
    uint8 u8LostNum;        // if u8LostNum >= 2, release this slot
    uint8 u8DeviceType;     //
    uint16 u16SeqNum;       // sequence number
    uint16 u16ShortAddr;    // short addr of the device
    uint16 u16PanID;        // Pan id
    uint16 tsTof;           // TOF result
    uint16 u16LocCardDistance[2]; // card2Loc,1 for locaotr #1, 2 for locator #2
    int8  i8Rssi;          //定位卡和主站之间的RSSI
    int8  i8LocRssi[2];    //定位卡和辅站之间的RSSI
    uint8 u8AppHelpAck;    // to record that if app send help ack to station or not. >0: yes, 0: not
    uint8 u8AppHelpAckType;
} tsTofSlot;


typedef struct
{
    uint8 bUsed;					// 0, 1
    uint8 u8LostNum;
    uint16 u16PanID;        		// Pan id
    uint16 u16ShortAddr;			// locator ID
    uint16 u16SyncAddress;			// 同步地址线上的 address
    uint16 u16SeqNum;       		// sequence number
    uint16 u16TofDist;				// 主辅站之间的
    uint16 u16LocVerCnt;
    uint16 u16LocOADVersion;
    char  u8LocVer[52];
} tsLocator_t;

typedef struct
{
    uint16 u16RetreatCard[SINGLE_CARD_RETREAT_SIZE];
    uint8 u8CardCount;
    uint8 u8Duration;
} tsSingleCardRetreat;

typedef struct
{
    uint16 u16SpeedCard[CARD_SPEED_REMIND_SIZE];
    uint8 u8CardCount;
    uint8 u8Duration;
} tsCardSpeedRemind;


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
/****************************************************************************/
/***        Local Function Prototypes                                     ***/
/****************************************************************************/
PRIVATE void vCheckSetting(void);
PRIVATE void vStartingLEDindicator();
PRIVATE void vInitSlots(void);
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
PRIVATE void vStopTofIndex(uint16 u16Index);
PRIVATE void vLocTof(uint8 u8Loc);
PRIVATE void vLocTofDone(void);
PRIVATE void vCheckCardLost(uint16 u16Index);

PRIVATE void vStationCast(uint8 u8CmdType, uint16 u16DstAddr, uint16 u16DstPanId, uint8 u8TxOptions, uint8 u8Val);
PRIVATE void vSendCardInfo();
PRIVATE void vUpdateAvailSlot(uint16 u16CurrSlotIdx);
PRIVATE void vProcessLocatorCardDist(uint16 *pCardDist, uint8 u8Len, uint8 u8Loc);
PRIVATE uint16 u16GetCardBaseIndex(uint16 u16SlotIndex, tof_device_type_te u8DeviceType);

//PRIVATE uint8 u8GetIdleGasSlot(uint16* pu16Index);
PRIVATE uint16 u16NextSlotIndex(uint16 u16index);
PRIVATE uint32 u32NextSlotTick(uint32 u32tick);
PRIVATE void vSetStationChannel(uint8 channel);

PRIVATE void vCheckCardStatus(uint16 u16ShortAddr, uint8 u8Status,uint8 u8ExciteID);
//PRIVATE void vCheckGasNodeStatus(uint16 u16ShortAddr, uint8 u8Status);

PRIVATE bool bArmReady();
PRIVATE void vBsMac_rx_callback(unsigned char *pbuf, unsigned char len);
PRIVATE void vReportCardDistance();
PRIVATE void vReportCardRssi();
PRIVATE void vReportCardAlarm(uint16 u16ShortAddr, uint8 u8CardStatus,uint8 u8ExciteID);
//PRIVATE void vReportCardExcite(uint16 u16ShortAddr, uint8 u8CardStatus,uint8 u8ExciteID);

PRIVATE void vConvertEndian16(uint16* pu16Data);
PRIVATE void vWriteData2Arm(uint8* pbuf);
PRIVATE void vInitialChannel(void);
PRIVATE void vInitialBaud(void);
PRIVATE bool vSaveChannel(uint8 u8TofChannel, uint8 u8LocChannel);
PRIVATE bool bChannelValid(uint8 u8BroadcastChannel, uint8 u8TofChannel, uint8 u8LocatorCardChannel);
PRIVATE void vDioCallBack(uint32 u32Device, uint32 u32ItemBitmap);

PRIVATE void vCheckCardRssi(uint16 u16ShortAddr, uint16 u16SeqNum, uint8 u8LinkQuality, uint8 RssiType,uint8 u8AccStatus);
PRIVATE int8 i8GetTofRssi(void);

PRIVATE void vReportLive(void);
PRIVATE void vReportStatus(void);
PRIVATE void vReportCardVersion(void);
PRIVATE void vReportStationRssi(uint8 u8Loc);
PRIVATE void vReportReset(void);

PRIVATE void vSlotProcessCard(uint16 u16SlotIndex);
PRIVATE void vSlotProcessGas(uint16 u16SlotIndex);
PRIVATE void vSlotProcessLocator(uint16 u16SlotIndex);
PRIVATE void vSlotProcessIdle(uint16 u16SlotIndex);

PRIVATE void vLocatorAddrSync(uint8 u8LocIdx);

PRIVATE void vResetReportRssi(uint8 u8Loc);
PRIVATE void vUpdateStationRssi(uint8 u8Loc, uint16 u16Short,int8 i8RecvRssi,int8 i8SendRssi);

PRIVATE void vReleaseSlot(uint16 u16Index);

PRIVATE uint16 u16GetIdleSlot_TOF15s(uint16 u16Start, uint16 u16ReqShortAddr, uint16* pu16Index,uint16 u16CardSlot);
PRIVATE uint16 u16GetIdleSlot_TOF5s(uint16 u16Start, uint16 u16ReqShortAddr, uint16* pu16Index,uint16 u16CardSlot);
PRIVATE uint16 u16GetIdleSlot_TOF1s(uint16 u16Start, uint16 u16ReqShortAddr, uint16* pu16Index,uint16 u16CardSlot);
PRIVATE void bAllocCardSlot(uint8 device_type,uint16 u16PanID,uint16 u16ShortAddr,uint16 u16AllocIndex,uint16 u16SeqNum);



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

PRIVATE uint8 u8CurChannel;            // to record station's current channel
PRIVATE uint8 u8TofChannel;            // station use this channel to do tof with card, and exchange info with loc
PRIVATE uint8 u8BroadcastChannel;      // station use this channel to send signal msg: idle, busy, rssi, available
PRIVATE uint8 u8LocatorCardChannel;    // this channel is for loc to do tof with card
PRIVATE uint8 u8ExtAddr[8];

PRIVATE tof_station_status_te tStationStatus = STATION_STATUS_NORMAL;


PRIVATE tsSingleCardRetreat SingleCardRetreat;

PRIVATE tsCardSpeedRemind CardSpeedRemind;
PRIVATE uint16 u16AcceptLocSlot;
PRIVATE uint16 u16AcceptGasSlot;

// slot 相关变量
PRIVATE tsTofSlot tsSlot[TOF_SLOT_NUM];
PRIVATE uint32 u32SlotTick;        	// current slot tick
PRIVATE uint16 u16SlotIndex;        // current slot index

// tof 相关
PRIVATE uint32 u32TofSlotTick;     	// the slot tick which is doing tof
PRIVATE uint16 u16TofSlotIndex;     // the slot index which is doing tof
PRIVATE uint16 u16TofPendingAddr;     	// the pending card which will do tof
PRIVATE eTofReturn eTofStatus = -1;
PRIVATE bool_t bTofBusy = FALSE;
PRIVATE tsAppApiTof_Data asTofData[STATION_TOF_MAX_READINGS];
PRIVATE uint16 TofFailDistance[7]= {INVALID_TOF_DISTANCE,FAILED_TOF_DISTANCE,VALID_TOF_DISTANCE,TOF_NOT_STARTED_DISTANCE,
                                    TOF_TX_ERROR_DISTANCE,TOF_RX_ERROR_DISTANCE,TOF_TIMEOUT_DISTANCE
                                   };

// 定位辅站
PRIVATE tsLocator_t tsLocator[2];

// available相关
PRIVATE uint8 u8AvailSlotType = 0;  // slot 发出的available类型
// 类型包括 HAVE_IDLE_LOC_SLOT，HAVE_IDLE_ONES_SLOT,HAVE_IDLE_FIVES_SLOT,HAVE_IDLE_FIFTEEN_SLOT
//PRIVATE uint8 u8AvailSlotIdx1S = 0;  // 分配出的一秒卡时隙
//PRIVATE uint8 u8AvailSlotIdx5S = 0;  // 分配出的五秒卡时隙
//PRIVATE uint8 u8AvailSlotIdx15S = 0;  // 分配出的十五秒卡时隙

// station cast相关变量
PRIVATE bool_t bMsgSending = FALSE;    // if the msg is still sending or not
PRIVATE uint8 u8LastHandle;
PRIVATE uint8 u8Retries = 0;
PRIVATE uint8 u8AcceptNum = 0;
PRIVATE uint8 u8AcceptNumTemp = 0;

PRIVATE tsCardInfo tsAccepted[STATION_ACCEPT_CARD_MAX_NUM];

// 系统状态
PRIVATE teState teStaionState = E_STATE_IDLE;


// 主辅站定位卡信息交互
PRIVATE uint8 u8ReSendCardInfo = 0;    		// to record the retry times that send card info to loc
PRIVATE bool_t bSendCardInfoDone = TRUE;    // 整体标志，和两个辅站之间该做的数据收发已经结束。(未入网的辅站不算)
PRIVATE bool_t bRxCardDistanceDone[2];    		// receive card's distance from loc

//PRIVATE bool bIsReciveGasDensity = FALSE;


PRIVATE uint32 u32Data2Arm[30];    // the data send to arm, max len is 114 (BSMAC_MAX_TX_PAYLOAD_LEN), 30*4 = 120 is enough (28 is enough)
PRIVATE struct nwkhdr* pnwk_data = (struct nwkhdr*)u32Data2Arm;
PRIVATE uint16 u16ArmId;
PRIVATE uint16 u16RssiNum = 0;
//PRIVATE uint16 u16GasRssiNum = 0;
//PRIVATE bool   bSupportedgas = FALSE;          // 0:not supported gasnod;1:supported gasnode
PRIVATE app_tof_rssi_ts app_tof_more_rssi_data;
//PRIVATE app_tof_gas_rssi_ts app_tof_gas_rssi_data;
//PRIVATE app_tof_gasdensity_ts app_tof_gasdensity_data;

PRIVATE uint8 u8StaLogVerLen;
PRIVATE uint8 u8StaLocVersin[VERSION_LENGTH];

// u16IdleSlot: these slots should never be used, they are for other use (station signal)
PRIVATE uint16 u16IdleSlot[TOF_SLOT_IDLE_NUM] = {6, 12, 20};

/*use u32 for alignment*/
PRIVATE uint32 u32CommonBuf[128];
PRIVATE uint8 u8LiveSeqNum = 0;
PRIVATE uint8 u8LastLiveAckSeq = 0;
PRIVATE uint8 u8ReportStatusNum = 0;
PRIVATE bool bIsStationUp = FALSE;
PRIVATE uint8 u8RestartCounter=0;

PRIVATE bool_t bIsTofSuccess = FALSE;

PRIVATE app_LSVersionReport_t tsVersionReport[APP_TOF_VERSION_MAX_NUM];
PRIVATE uint8  			      tsVersionReportLen;

PRIVATE app_rssi_report  rssiReport[2];

PRIVATE int_8 i8StationRcvRssi;

/****************************************************************************/
/***        Function protos                                            ***/
/****************************************************************************/
void vProcessCardVersion(uint16 u16DevId, uint8 u8DevType, uint16 u16OadVersion, uint16 u16Battery);
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
    vAHI_WatchdogStart(12);           //Timeout Period =(2^(12-1)+1)X8 ms = 16.4s
    vInitStationSystem();
    vStartCoordinator();

#if (defined DEBUG_LOC_APP || defined DEBUG_ERROR)
    i2c_printf_init();
    i2c_vPrintf("LOC System started. \n\r");
    i2c_vPrintf("station id %d\n", u16StationPanId);
    if(bAHI_WatchdogResetEvent()) i2c_vPrintf("Watchdog reset !!!\n");
#endif
    vOADInitClient(TRUE);

    vInitSlots();

    vResetReportRssi(0);
    vResetReportRssi(1);

    TimerUtil_eSetCircleTimer(STATION_SLOT_EVENT, TOF_SLOT_MS);
    TimerUtil_eSetCircleTimer(STATION_WATCHDOG_EVENT, 500);    // feed watch dog every 500 milliseconds

    EventUtil_vSetEvent(STATION_PORT_QUERY_EVENT);
    EventUtil_vSetEvent(STATION_REPORT_STATUS);
    EventUtil_vSetEvent(STATION_REPORT_CARDVERSION);
    EventUtil_vSetEvent(STATION_REPORT_RESET_EVENT);
    EventUtil_vSetEvent(STATION_REPORT_STATION_RSSI);

    vAHI_TickTimerWrite(0);

    while (1)
    {
        TimerUtil_vUpdate();
        Jbsmac_vPoll();
        vProcessSysEventQueues();
        vProcessAppEventQueues();
#if (defined DEBUG_LOC_APP || defined DEBUG_ERROR)
        i2c_vPrintPoll();
#endif
    }
}

/****************************************************************************
 *
 * NAME: AppWarmStart
 *
 ****************************************************************************/
PUBLIC void AppWarmStart(void)
{
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
    (void)u32AppQApiInit(NULL, NULL, vDioCallBack); //vHwDeviceIntCallback);
    (void)u32AHI_Init();

    /* 口线初始化 */
    vAHI_DioSetDirection(0, CTRL_HARD_WATCHDOG);   //init hardware watchdog GPIO
    vAHI_DioSetOutput(CTRL_HARD_WATCHDOG,0);

    vAHI_DioSetDirection(CTRL_ADDR_SYNC_0,0);
    vAHI_DioInterruptEnable(CTRL_ADDR_SYNC_0,0);
    vAHI_DioInterruptEdge(0,CTRL_ADDR_SYNC_0);

    vAHI_DioSetDirection(CTRL_ADDR_SYNC_1,0);
    vAHI_DioInterruptEnable(CTRL_ADDR_SYNC_1,0);
    vAHI_DioInterruptEdge(0,CTRL_ADDR_SYNC_1);

    /*vAHI_ProtocolPower(TRUE);
    vAHI_BbcSetHigherDataRate(E_AHI_BBC_CTRL_DATA_RATE_500_KBPS);
    vAHI_BbcSetInterFrameGap(50);
    */

    // Enable high power modules, tof function, timerUtil
    vAHI_HighPowerModuleEnable(TRUE, TRUE);
    vAppApiTofInit(TRUE);
    TimerUtil_vInit();
    vAHI_WakeTimerEnable(E_AHI_WAKE_TIMER_0, TRUE);

    vInitialChannel();

    //init global variable,avoid warmstart global variable no init
    bSendCardInfoDone = TRUE;

    bTofBusy = FALSE;
    bMsgSending = FALSE;

    //u8AvailSlotType = 0;
    u8AvailSlotType |= HAVE_IDLE_FIVES_SLOT | HAVE_IDLE_LOC_SLOT | HAVE_IDLE_ONES_SLOT | HAVE_IDLE_FIFTEEN_SLOT;

// for arm
    pnwk_data->type = NWK_DATA;
    pnwk_data->ttl = 1;
    pnwk_data->src = u16StationPanId;
    vConvertEndian16(&(pnwk_data->src));

    app_tof_more_rssi_data.app_tof_head.protocoltype = APP_PROTOCOL_TYPE_CARD;
    app_tof_more_rssi_data.app_tof_head.msgtype = APP_TOF_MSG_RSSI;
    //app_tof_gas_rssi_data.app_tof_head.protocoltype = APP_PROTOCOL_TYPE_CARD;
    //app_tof_gas_rssi_data.app_tof_head.msgtype = APP_TOF_MSG_GAS_RSSI;

    LedUtil_bRegister(LED_RF);
    LedUtil_bRegister(LED_UART0);
    LedUtil_bRegister(LED_UART1);

    ErrorUtil_vRegisterLed0(LED_RF);
    ErrorUtil_vRegisterLed1(LED_UART0);
    ErrorUtil_vRegisterLed2(LED_UART1);

    vCheckSetting();
    vAHI_UartSetRTSCTS(E_AHI_UART_1,FALSE);     // uart1:2-wire mode,DIO18 is RTS
    vInitialBaud();

    vStartingLEDindicator();
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

    // 基站版本
    u8StaLogVerLen = strlen(STATION_LOG);
    memcpy(u8StaLocVersin,STATION_LOG,strlen(STATION_LOG));
    u8StaLogVerLen += strlen(VERSION);
    if(u8StaLogVerLen < VERSION_LENGTH)
        memcpy(u8StaLocVersin + strlen(STATION_LOG),VERSION,strlen(VERSION));

    /*u8StaLogVerLen += strlen(LOCATOR0_LOG);
    if(u8StaLogVerLen < VERSION_LENGTH)
        memcpy(u8StaLocVersin + strlen(STATION_LOG)+strlen(VERSION),LOCATOR0_LOG,strlen(LOCATOR0_LOG));*/
}

PRIVATE void vCheckSetting(void)
{
    //channels check
    if(!bChannelValid(u8BroadcastChannel,  u8TofChannel,  u8LocatorCardChannel)
            || u16StationPanId < 30000 || u16StationPanId > 39999)
    {
        EDBG(i2c_vPrintf("Check Setting Failed!\n"););
        EDBG(i2c_vPrintf("u16StationPanId %d u8BroadcastChannel %d u8TofChannel %d u8LocatorCardChannel %d \n",
                         u16StationPanId, u8BroadcastChannel, u8TofChannel, u8LocatorCardChannel););

        ErrorUtil_vFatalHalt3(35);
    }
}

PRIVATE void vStartingLEDindicator()
{
    LedUtil_vFlashAll(1000,3);
    LedUtil_vFlashAll(3000,1);
}

// Slot 初始化
PRIVATE void vInitSlots(void)
{
    uint16 i, j, t ;
    memset((uint8*)tsSlot, 0, TOF_SLOT_NUM*sizeof(tsTofSlot));

    for (i = 0; i < TOF_SLOT_NUM; i++)
    {
        tsSlot[i].b1Used = 0;
        tsSlot[i].b2SlotType = SLOT_CARD;
        tsSlot[i].u8DeviceType = DEVICE_TYPE_CARD5S;
        tsSlot[i].u16LocCardDistance[0] = INVALID_TOF_DISTANCE;
        tsSlot[i].u16LocCardDistance[1] = INVALID_TOF_DISTANCE;

        t = i % TOF_SLOT_LOC_INT; 	    // 分配辅站slot
        if(t < 2)                       // 每 25 slot 的 0和1 slot分配给辅站
        {
            tsSlot[i].b2SlotType = SLOT_LOCATOR;
        }
        else
        {
            for(j=0; j<TOF_SLOT_IDLE_NUM; j++)  // 分配IDLE slot
            {
                if(t == u16IdleSlot[j])
                {
                    tsSlot[i].b2SlotType = SLOT_IDLE;
                    break;
                }
            }
        }
    }

    u16SlotIndex = 0;
    u32SlotTick = 0;
    u16TofSlotIndex = 0;
    u32TofSlotTick = 0;
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
        case STATION_PORT_QUERY_EVENT:
        {
            /* initial i2c printf every 3*64 seconds */
            /* it will encrease the crc error */
#if (defined DEBUG_LOC_APP || defined DEBUG_ERROR)
            static uint16 cnt;
            if(++cnt % 64 ==0)
            {
                i2c_vPrintPoll(); //print datas in buffer first
                i2c_printf_init();
                i2c_vPrintf("Channel %d %d %d \n", u8BroadcastChannel, u8TofChannel, u8LocatorCardChannel);
            }
#endif
            int8 port = Jbsmac_i8GetUpPort();

            // for up port, must link to arm, for down port, link to uart
            if( (port == E_AHI_UART_0 && bIsStationUp)
                    || (port != E_AHI_UART_0 && Jbsmac_u8GetLinkStatus(0) > 0))
            {
                LedUtil_vOn(LED_UART0);
            }
            else
            {
                LedUtil_vOff(LED_UART0);
            }
            if( (port == E_AHI_UART_1 && bIsStationUp)
                    || (port != E_AHI_UART_1 && Jbsmac_u8GetLinkStatus(1) > 0))
            {
                LedUtil_vOn(LED_UART1);
            }
            else
            {
                LedUtil_vOff(LED_UART1);
            }

            if(bArmReady() == FALSE)
            {
                EDBG(i2c_vPrintf("ComPort: not connected\n"););
            }
            else
            {
                EDBG(
                    if(bIsStationUp)
                    i2c_vPrintf("up %d\n", u16ArmId);
                    else
                        i2c_vPrintf("down %d\n", u16ArmId);
                    );
                vReportLive();
            }
            uint8 tmp = (u8LiveSeqNum > u8LastLiveAckSeq) ? (u8LiveSeqNum - u8LastLiveAckSeq) : (u8LiveSeqNum + 256 - u8LastLiveAckSeq);

            if(tmp > 3)
            {
                bIsStationUp = FALSE;
                u8RestartCounter++;
            }

            //if can not connect, restart after about 2min
            if(u8RestartCounter > 40)
            {
                //reset
                EDBG(i2c_vPrintf("restart: can not connect\n"););
                EDBG(i2c_vPrintPoll();); //print datas in buffer first
                vAHI_SwReset();
            }

            //reset LEDs state
            LedUtil_vOff(LED_RF);

            TimerUtil_eSetTimer(STATION_PORT_QUERY_EVENT, 3000);
            EventUtil_vUnsetEvent(STATION_PORT_QUERY_EVENT);
            break;
        }
        case STATION_CLAIM_EVENT:
        {
            uint16 u16RunMs = (uint16)((u32AHI_TickTimerRead() - u32SlotTick)/16000);

            DBG(
                if(bDetailedPrint)
                {
                    i2c_vPrintf("Claim %d: %d %d %d %d\n", u16SlotIndex, bMsgSending, bTofBusy, bSendCardInfoDone, u16RunMs);
                }
            )

            // all things are done
            if((!bMsgSending) && (!bTofBusy) && bSendCardInfoDone)
            {
                // should has at least 3 ms to send msg
                if(u16RunMs <= TOF_SLOT_MS - 3)
                {
                    // if next slot is used, just claim, else send available/Rssi msg to save time
                    if(0 < tsSlot[u16NextSlotIndex(u16SlotIndex)].b1Used)
                    {
                        vSetStationChannel(u8BroadcastChannel);
                        if(u8AvailSlotType > 0)    // has idle slot
                        {
                            vStationCast(TOF_STATION_ILDE, 0xFFFF, TOF_CARD_NWK_ADDR, 0, 0);
                        }
                        else
                        {
                            vStationCast(TOF_STATION_BUSY, 0xFFFF, TOF_CARD_NWK_ADDR, 0, 0);
                        }
                    }
                    else
                    {
                        if(u8AvailSlotType > 0) // has idle slot
                        {
                            EventUtil_vSetEvent(STATION_AVAILABLE_EVENT);
                        }
                        else
                        {
                            EventUtil_vSetEvent(STATION_RSSI_EVENT);
                        }
                    }
                }

                EventUtil_vUnsetEvent(event);
            }

            // next slot is coming, should unset this event
            else if(u16RunMs > TOF_SLOT_MS - 3)
            {
                EventUtil_vUnsetEvent(event);
            }
            // else, not need to unset event
            break;
        }

        case STATION_AVAILABLE_EVENT:
        {
            uint8 u8RunMs = (uint8)((u32AHI_TickTimerRead() - u32SlotTick)/16000);
            if((!bMsgSending) && (!bTofBusy) && bSendCardInfoDone)
            {
                // station (re)send accpted msg at: 12 / 15 / 18 ms of this slot
                // assume station tx available msg to card need 2ms, and rx request msg from card need 2ms too
                // then card has at most 12 - 2 -2 = 8 ms to random send request msg
                // subtract the cost of station (u8RunMs) comes: 8 - u8RunMs
                // if next slot is NOT used, means that this is send at the end of a slot (come from STATION_CLAIM_EVENT)
                if(((u8RunMs <= 4) && (0 == tsSlot[u16SlotIndex].b1Used))         // 4 ms to send avalaible msg (include resends)
                        || (0 == tsSlot[u16NextSlotIndex(u16SlotIndex)].b1Used))     // send at the end of a slot
                {
                    vSetStationChannel(u8BroadcastChannel);
                    teStaionState = E_STATE_AVAILABLE;
                    u8Retries = 0;
                    u8AcceptNum = 0;

                    uint8 u8RndRangeTmp;
                    if(u8RunMs <= 4)
                        u8RndRangeTmp = 8 - u8RunMs;
                    else
                        u8RndRangeTmp = TOF_SLOT_MS + 8 - u8RunMs;

                    vStationCast(TOF_STATION_AVAILABLE, 0xFFFF, TOF_CARD_NWK_ADDR, 0, u8RndRangeTmp);
                    TimerUtil_eSetTimer(STATION_ACCEPT_EVENT, u8RndRangeTmp + 4);
                }
                EventUtil_vUnsetEvent(event);
            }

            // the next used slot is coming
            else if((u8RunMs > TOF_SLOT_MS - 3) && (0 != tsSlot[u16NextSlotIndex(u16SlotIndex)].b1Used))
                EventUtil_vUnsetEvent(event);

            // else not need to unset this event

            break;
        }

        case STATION_RSSI_EVENT:
        {
            uint8 u8RunMs = (uint8)((u32AHI_TickTimerRead() - u32SlotTick)/16000);
            if(((u8RunMs <= 15) && (0 == tsSlot[u16SlotIndex].b1Used))
                    || (0 == tsSlot[u16NextSlotIndex(u16SlotIndex)].b1Used))     // send at the end of a slot
            {
                uint8 u8RndRangeTmp;
                // station need at least 2 ms to send RSSI cmd
                if(0 == tsSlot[u16NextSlotIndex(u16SlotIndex)].b1Used)
                    u8RndRangeTmp = TOF_SLOT_MS + 20 - 2 - u8RunMs;
                else
                    u8RndRangeTmp = 20 - 2 - u8RunMs;

                vSetStationChannel(u8BroadcastChannel);
                teStaionState = E_STATE_RSSI;
                vStationCast(TOF_STATION_RSSI, 0xFFFF, TOF_CARD_NWK_ADDR, 0, u8RndRangeTmp);
            }

            EventUtil_vUnsetEvent(event);
            break;
        }

        case STATION_ACCEPT_EVENT:
        {
            //DBG(i2c_vPrintf("AC\n");)
            uint32 tick= u32AHI_TickTimerRead();
            uint16 u16RunMs = (uint16)((tick - u32SlotTick)/16000);

            u8AcceptNumTemp = 0;
            teStaionState = E_STATE_ACCEPT;
            if ((u16RunMs <= TOF_SLOT_MS - 2)) // need 2 ms to finish the accept msg
            {
                if(u8AcceptNum > 0)
                {
                    vSetStationChannel(u8BroadcastChannel);
                    vStationCast(TOF_STATION_ACCEPT, 0xFFFF, TOF_CARD_NWK_ADDR, 0, 0);
                    u8Retries++;
                }
                else
                {
                    bMsgSending = FALSE;
                    bTofBusy = FALSE;

                    EventUtil_vSetEvent(STATION_CLAIM_EVENT);
                }
            }
            EventUtil_vUnsetEvent(event);
            break;
        }

        case STATION_SLOT_EVENT:
        {
            /* 更新当前slot状态 */
            u16SlotIndex = u16NextSlotIndex(u16SlotIndex);
            u32SlotTick = u32NextSlotTick(u32SlotTick);

            // time out check: if the tof is last for more than 1 slots, it's timeout, should stop it
            if(bTofBusy == TRUE)
                vStopTofIndex(u16TofSlotIndex);
            else
                vCurrentTofDone();
            // end of time out check

            // station report card's distance/rssi at 15ms of the loc's #2 slot
            if (u16SlotIndex % TOF_SLOT_LOC_INT == 1)
            {
                TimerUtil_eSetTimer(STATION_TOF_REPORT_EVENT, 15);
            }

            /* do not serve if ARM is not ready*/
#ifndef DEBUG_WOC
            if(!bIsStationUp)
            {
                EventUtil_vUnsetEvent(event);
                break;
            }
#endif

            if(tsSlot[u16SlotIndex].b2SlotType == SLOT_CARD)
            {
                vSlotProcessCard(u16SlotIndex);
            }
            else if (tsSlot[u16SlotIndex].b2SlotType == SLOT_LOCATOR)
            {
                vSlotProcessLocator(u16SlotIndex);
            }
            else if (tsSlot[u16SlotIndex].b2SlotType == SLOT_IDLE)
            {
                vSlotProcessIdle(u16SlotIndex);
            }
            else
            {
                vSlotProcessIdle(u16SlotIndex);
            }

            EventUtil_vUnsetEvent(event);
            break;
        }

        case STATION_SEND_CARD_INFO_EVENT:
        {
            // 在0ms, 7ms, 14ms 分别重传三次
            if((bSendCardInfoDone == FALSE) && (u8ReSendCardInfo++ < 2))
            {
                vSendCardInfo();
                TimerUtil_eSetTimer(STATION_SEND_CARD_INFO_EVENT, 12);
            }
            else if(bSendCardInfoDone==FALSE && u8ReSendCardInfo >= 2)
            {
                uint8 u8Loc;

                //如果2个辅站都失败?
                // 如果两次发送未收完整，也算 send card info done
                bSendCardInfoDone = TRUE;
                if(tsLocator[0].bUsed && bRxCardDistanceDone[0]==FALSE)
                {
                    u8Loc = 0;
                }
                else if(tsLocator[1].bUsed && bRxCardDistanceDone[1]==FALSE)
                {
                    u8Loc = 1;
                }
                else
                {
                    EventUtil_vUnsetEvent(event);
                    break;
                }

                tsLocator[u8Loc].u8LostNum++;
                i2c_vPrintf("lost %d \n", tsLocator[u8Loc].u8LostNum);

                if(tsLocator[u8Loc].u8LostNum >= TOF_STATION_LOCATOR_MAX_FAILED)
                {
                    tsLocator[u8Loc].bUsed = FALSE;
                    tsLocator[u8Loc].u8LostNum= 0;
                    tsLocator[u8Loc].u16PanID= 0;
                    tsLocator[u8Loc].u16ShortAddr = 0;
                    tsLocator[u8Loc].u16TofDist = INVALID_TOF_DISTANCE;
                    vUpdateAvailSlot(u16SlotIndex);

                    EDBG(
                        i2c_vPrintf("locator %d out: %d\n", u8Loc, tsLocator[u8Loc].u16ShortAddr);
                    );
                }
            }
            EventUtil_vUnsetEvent(event);
            break;
        }

        case STATION_TOF2CARD_EVENT:
        {
            MAC_Addr_s sAddr;
            sAddr.u8AddrMode = 2;
            sAddr.u16PanId = TOF_CARD_NWK_ADDR;
            sAddr.uAddr.u16Short = u16TofPendingAddr;
            eTofStatus = -1;

            vSetStationChannel(u8TofChannel);
            if (FALSE == bAppApiGetTof(asTofData, &sAddr, STATION_TOF_MAX_READINGS, API_TOF_FORWARDS, vTofCallback))
            {
                vCheckCardLost(u16TofSlotIndex);
                vCurrentTofDone();
                EventUtil_vSetEvent(STATION_CLAIM_EVENT);
            }
            else
            {
                bTofBusy = TRUE;
                EventUtil_vSetEvent(STATION_WAIT_TOF2CARD_EVENT);
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
                EventUtil_vUnsetEvent(event);

                // if there's at least 3ms left, send claim msg
                if((TOF_SLOT_MS - ((u32AHI_TickTimerRead() - u32SlotTick) / 16000) > 3))
                    EventUtil_vSetEvent(STATION_CLAIM_EVENT);
            }
            // don't unset event
            break;
        }

        case STATION_WATCHDOG_EVENT:
        {
            //DBG(i2c_vPrintf("W\n"););
            vAHI_WatchdogRestart();

#if (defined SUPPORT_HARD_WATCHDOG)
            vFeedHardwareWatchDog();
#endif
            //calculate single card retreat timeout
            if(SingleCardRetreat.u8CardCount > 0)
            {
                if(--SingleCardRetreat.u8Duration == 0)
                {
                    SingleCardRetreat.u8CardCount = 0;
                }
            }
            else if(tStationStatus == STATION_STATUS_SINGLE_RETREAT)
            {
                tStationStatus = STATION_STATUS_NORMAL;
            }

            if(CardSpeedRemind.u8CardCount > 0)
            {
                if(--CardSpeedRemind.u8Duration == 0)
                {
                    CardSpeedRemind.u8CardCount = 0;
                }
            }

            EventUtil_vUnsetEvent(event);
            break;
        }
        case STATION_LOCTOF_TIMEOUT_EVENT:
        {
            /* time is out ,kill loctof check */
            EventUtil_vUnsetEvent(STATION_LOCTOF_CHECK_EVENT);
            EventUtil_vUnsetEvent(event);
            break;
        }
        case STATION_LOCTOF_CHECK_EVENT:
        {
            /* if tof done */
            if(eTofStatus != -1)
            {
                vLocTofDone();
                EventUtil_vUnsetEvent(event);
            }
            // do not unset event until finished or timeout
            //  EventUtil_vUnsetEvent(event);
            break;
        }
        case STATION_TOF_REPORT_EVENT:
        {
            vReportCardDistance();
            vReportCardRssi();
            // vReportGasNodeDensity();

            EventUtil_vUnsetEvent(event);
            break;
        }
        case STATION_REPORT_STATUS:
        {
            vReportStatus();
            TimerUtil_eSetTimer(STATION_REPORT_STATUS, 120000);
            EventUtil_vUnsetEvent(STATION_REPORT_STATUS);
            break;
        }
        case STATION_REPORT_CARDVERSION:
        {
            vReportCardVersion();
            TimerUtil_eSetTimer(STATION_REPORT_CARDVERSION, 60000);
            EventUtil_vUnsetEvent(STATION_REPORT_CARDVERSION);
            break;
        }

        case STATION_REPORT_STATION_RSSI:
        {
            static uint16 xCnt;
            if(xCnt++ % 2 == 0)
            {
                vReportStationRssi(0);
            }
            else
            {
                vReportStationRssi(1);
            }
            TimerUtil_eSetTimer(STATION_REPORT_STATION_RSSI, 60000);
            EventUtil_vUnsetEvent(event);
            break;
        }

        case STATION_REPORT_RESET_EVENT:
        {
            vReportReset();
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
            EventUtil_vSetEvent(STATION_AVAILABLE_EVENT);

        // if broadcast the accept msg, resend at most 3 times
        else if ((teStaionState == E_STATE_ACCEPT) && (u8Retries < 3))
        {
            EventUtil_vSetEvent(STATION_ACCEPT_EVENT);
        }
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
        //Do not serve if station is not up
#ifndef DEBUG_WOC
        if(!bIsStationUp) return;
#endif
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

            LedUtil_vOff(LED_RF);
            LedUtil_vOff(LED_UART0);
            LedUtil_vOff(LED_UART1);

            for(i=0; i<10; i++)
            {
                TimerUtil_vDelay(500, E_TIMER_UNIT_MILLISECOND);
                LedUtil_vToggle(LED_RF);
                LedUtil_vToggle(LED_UART0);
                LedUtil_vToggle(LED_UART1);
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

inline uint16 vStationMod(uint16 number,uint16 modNum)
{
    uint16 sNumber = number;
    //sNumber = number;
    while(sNumber >= modNum)
    {
        sNumber = sNumber - modNum;
    }
    return sNumber;
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

    LedUtil_vToggle(LED_RF);

    switch(psAppPkt->tof_head.msgtype)
    {
    case TOF_CARD_CHECKIN:
    {
        if(psFrame->u8SduLength != sizeof(rf_tof_card_data_ts))
        {
            sys_assert(0);
            break;
        }

        vStationCast(TOF_STATION_CHECKIN_ACK, psMcpsIndData->sFrame.sSrcAddr.uAddr.u16Short, \
                     psMcpsIndData->sFrame.sSrcAddr.u16PanId, MAC_TX_OPTION_ACK, 0);

        vCheckCardStatus(psMcpsIndData->sFrame.sSrcAddr.uAddr.u16Short,\
            (psAppPkt->rf_tof_card_data.u8CardStatus & (~CARD_STATUS_EXCITER)),psAppPkt->rf_tof_card_data.u8ExciterIDorAccStatus);
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

            vWriteData2Arm((uint8*)&app_tof_checkin_data);
        }

        break;
    }

    /*case TOF_CARD_EXCITE:
    {
        if(psFrame->u8SduLength != sizeof(rf_tof_card_data_ts))
        {
            sys_assert(0);
            break;
        }

        vStationCast(TOF_STATION_EXCITE_ACK, psMcpsIndData->sFrame.sSrcAddr.uAddr.u16Short, \
                     psMcpsIndData->sFrame.sSrcAddr.u16PanId, MAC_TX_OPTION_ACK, 0);

        vReportCardExcite(psMcpsIndData->sFrame.sSrcAddr.uAddr.u16Short,\
            psAppPkt->rf_tof_card_data.u8CardStatus,psAppPkt->rf_tof_card_data.u8ExciterIDorAccStatus);
        break;
    }*/

    case TOF_CARD_REQUEST:   // 定位卡入网协议
    {
		uint8 req_type;
		if(psFrame->u8SduLength != sizeof(rf_tof_card_data_ts))
        {
            sys_assert(0);
            break;
        }
        vCheckCardStatus(psMcpsIndData->sFrame.sSrcAddr.uAddr.u16Short
                         ,(psAppPkt->rf_tof_card_data.u8CardStatus &(~CARD_STATUS_EXCITER)),psAppPkt->rf_tof_card_data.u8ExciterIDorAccStatus);

		// record rssi once receive card request
		if(psAppPkt->rf_tof_card_data.u16Battery == 0xFFFF && psAppPkt->rf_tof_card_data.u16OadVersion == 0xFFFF)
			req_type = APP_TOF_DEVICE_CARD_REQ;
		else
			req_type = APP_TOF_CARD_REQ;
		
        vCheckCardRssi(psMcpsIndData->sFrame.sSrcAddr.uAddr.u16Short
                       ,psAppPkt->rf_tof_card_data.u16SeqNum
                       ,psFrame->u8LinkQuality,
                       req_type,psAppPkt->rf_tof_card_data.u8ExciterIDorAccStatus);

        u8AcceptNumTemp++;

        if(((teStaionState == E_STATE_AVAILABLE) || (teStaionState == E_STATE_ACCEPT))
                && (STATION_ACCEPT_CARD_MAX_NUM > u8AcceptNum)&& (STATION_ACCEPT_CARD_MAX_NUM >= u8AcceptNumTemp))
        {
            uint16 u16tmp = 0;
            uint16 u16AllocIndex = 0;
            if(psAppPkt->rf_tof_card_data.u8CardStatus & CARD_STATUS_TOF1S)  //1s卡
            {
                //uint32 u32CurTick = u32AHI_TickTimerRead();
                u16tmp = u16GetIdleSlot_TOF1s(u16SlotIndex,psMcpsIndData->sFrame.sSrcAddr.uAddr.u16Short, &u16AllocIndex,psAppPkt->rf_tof_card_data.u16SeqNum);
                if(u16tmp > 0)
                {
                    bAllocCardSlot(CARD_STATUS_PERIOD_1S,psMcpsIndData->sFrame.sSrcAddr.u16PanId,psMcpsIndData->sFrame.sSrcAddr.uAddr.u16Short,
                                        u16AllocIndex,psAppPkt->rf_tof_card_data.u16SeqNum);
                }
            }
            else if(psAppPkt->rf_tof_card_data.u8CardStatus & CARD_STATUS_TOF15S)
            {
                //uint32 u32CurTick = u32AHI_TickTimerRead();
                u16tmp = u16GetIdleSlot_TOF15s(u16SlotIndex, psMcpsIndData->sFrame.sSrcAddr.uAddr.u16Short, &u16AllocIndex,psAppPkt->rf_tof_card_data.u16SeqNum);
                if(u16tmp > 0)
                {
                    bAllocCardSlot(CARD_STATUS_PERIOD_15S,psMcpsIndData->sFrame.sSrcAddr.u16PanId,psMcpsIndData->sFrame.sSrcAddr.uAddr.u16Short,
                    u16AllocIndex,psAppPkt->rf_tof_card_data.u16SeqNum);
                }
            }
            else
            {
                u16tmp = u16GetIdleSlot_TOF5s(u16SlotIndex,psMcpsIndData->sFrame.sSrcAddr.uAddr.u16Short, &u16AllocIndex,psAppPkt->rf_tof_card_data.u16SeqNum);
                if(u16tmp > 0)
                {
                    bAllocCardSlot(CARD_STATUS_PERIOD_5S,psMcpsIndData->sFrame.sSrcAddr.u16PanId,psMcpsIndData->sFrame.sSrcAddr.uAddr.u16Short,
                    u16AllocIndex,psAppPkt->rf_tof_card_data.u16SeqNum);
                }
            }
            //uint32 u32CurTick = u32AHI_TickTimerRead();
            vUpdateAvailSlot(u16SlotIndex);
            //i2c_vPrintf("UP:%d\n",u32AHI_TickTimerRead()-u32CurTick);
        }

        break;
    }

    case TOF_CARD_RSSI:
    {
        // check len: rf_tof_card_data's len is 12
        if(psFrame->u8SduLength != sizeof(rf_tof_card_data_ts))
        {
            sys_assert(0);
            break;
        }

        vCheckCardStatus(psMcpsIndData->sFrame.sSrcAddr.uAddr.u16Short,
                         (psAppPkt->rf_tof_card_data.u8CardStatus & (~CARD_STATUS_EXCITER)),psAppPkt->rf_tof_card_data.u8ExciterIDorAccStatus);

#if 0
        if(teStaionState == E_STATE_RSSI)
            vCheckCardRssi(psMcpsIndData->sFrame.sSrcAddr.uAddr.u16Short
                       ,psAppPkt->rf_tof_card_data.u16SeqNum
                       ,psFrame->u8LinkQuality
                       ,APP_TOF_CARD_RSSI,psAppPkt->rf_tof_card_data.u8ExciterIDorAccStatus);
#endif
        break;
    }

    case TOF_GASNODE_RSSI:
    {
        break;
    }

    case TOF_CARD_ALARM:
    {
        // check len: rf_tof_card_data's len is 12
        if(psFrame->u8SduLength != sizeof(rf_tof_card_data_ts))
        {
            sys_assert(0);
            break;
        }

        vCheckCardStatus(psMcpsIndData->sFrame.sSrcAddr.uAddr.u16Short,\
            psAppPkt->rf_tof_card_data.u8CardStatus,psAppPkt->rf_tof_card_data.u8ExciterIDorAccStatus);
        break;
    }

    case TOF_GASNODE_ALARM:
    {
        break;
    }

    case TOF_GASNODE_REQUEST:
    {
        break;
    }

    case TOF_GASDENSITY:
    {
        break;
    }

    case TOF_LOCATOR_REQUEST:
    {
        // check len: locator request's len is 4
        if(psFrame->u8SduLength != 4)
        {
            sys_assert(0);
            break;
        }

        if(tsLocator[0].u16SyncAddress == psMcpsIndData->sFrame.sSrcAddr.uAddr.u16Short
                && tsLocator[0].u16SyncAddress != 0)
        {
            u16AcceptLocSlot = 0;
        }
        else if(tsLocator[1].u16SyncAddress == psMcpsIndData->sFrame.sSrcAddr.uAddr.u16Short
                && tsLocator[1].u16SyncAddress != 0)
        {
            u16AcceptLocSlot = 1;
        }
        else
        {
            EDBG(i2c_vPrintf("loc req error %d !:%d\r\n",psMcpsIndData->sFrame.sSrcAddr.uAddr.u16Short,tsLocator[0].u16SyncAddress););

            break;
        }

        tsLocator[u16AcceptLocSlot].bUsed = TRUE;
        tsLocator[u16AcceptLocSlot].u8LostNum = 0;
        tsLocator[u16AcceptLocSlot].u16ShortAddr = psMcpsIndData->sFrame.sSrcAddr.uAddr.u16Short;
        tsLocator[u16AcceptLocSlot].u16PanID = psMcpsIndData->sFrame.sSrcAddr.u16PanId;
        tsLocator[u16AcceptLocSlot].u16SeqNum = 0;
        tsLocator[u16AcceptLocSlot].u16TofDist = INVALID_TOF_DISTANCE;
        tsLocator[u16AcceptLocSlot].u16LocVerCnt = 1200;    //辅站入网时就让报版本

        vStationCast(TOF_STATION_ACCEPT, psMcpsIndData->sFrame.sSrcAddr.uAddr.u16Short,
                     psMcpsIndData->sFrame.sSrcAddr.u16PanId, MAC_TX_OPTION_ACK, 1); // locator request: u8Val = 1

        DBG(
            i2c_vPrintf("locator %x join %d\n", psMcpsIndData->sFrame.sSrcAddr.uAddr.u16Short, u16AcceptLocSlot);
        )

        break;
    }

    // locator report card's distance
    case TOF_LOCATOR_CARDS_DIST:
    {
        uint8 u8Loc;

        // 主辅站一次交换数据必须为 23个
        if(psFrame->u8SduLength != psAppPkt->tof_head.len + sizeof(app_header_t)
                || psFrame->u8SduLength != sizeof(rf_tof_locator_card_info_ts)
                || psAppPkt->rf_tof_locator_card_info.u8CardNum != TOF_SLOT_LOC_INT-2)
        {
            sys_assert(0);
            break;
        }

        if(tsLocator[0].bUsed && psMcpsIndData->sFrame.sSrcAddr.uAddr.u16Short == tsLocator[0].u16ShortAddr && bRxCardDistanceDone[0] == FALSE)
        {
            bRxCardDistanceDone[0] = TRUE;
            tsLocator[0].u8LostNum = 0;
            u8Loc = 0;

            vProcessLocatorCardDist(psAppPkt->rf_tof_locator_card_info.u16CardDist
                                ,psAppPkt->rf_tof_locator_card_info.u8CardNum, u8Loc);

            vUpdateStationRssi(u8Loc, psMcpsIndData->sFrame.sSrcAddr.uAddr.u16Short
                           ,psAppPkt->rf_tof_locator_card_info.i8Rssi
                           ,SysUtil_vConvertLQI2Dbm(psFrame->u8LinkQuality));
        }
        else if(tsLocator[1].bUsed && psMcpsIndData->sFrame.sSrcAddr.uAddr.u16Short == tsLocator[1].u16ShortAddr && bRxCardDistanceDone[1] == FALSE)
        {
            bRxCardDistanceDone[1] = TRUE;
            tsLocator[1].u8LostNum = 0;
            u8Loc = 1;
            vProcessLocatorCardDist(psAppPkt->rf_tof_locator_card_info.u16CardDist
                                ,psAppPkt->rf_tof_locator_card_info.u8CardNum, u8Loc);

            // 主站到辅站的 RSSI 更新
            vUpdateStationRssi(u8Loc, psMcpsIndData->sFrame.sSrcAddr.uAddr.u16Short
                           ,psAppPkt->rf_tof_locator_card_info.i8Rssi
                           ,SysUtil_vConvertLQI2Dbm(psFrame->u8LinkQuality));
        }
        /*else
        {
            //i2c_vPrintf("addr:%d,1used:%d,2used:%d\n",psMcpsIndData->sFrame.sSrcAddr.uAddr.u16Short,tsLocator[0].bUsed,tsLocator[1].bUsed);
            sys_assert(0);
            break;
        }*/

        // 主辅站数据更新是否完全结束
        if(!((tsLocator[0].bUsed && bRxCardDistanceDone[0] == FALSE)
                || (tsLocator[1].bUsed && bRxCardDistanceDone[1] == FALSE)))
        {
            bSendCardInfoDone = TRUE;  // 整体标志
            TimerUtil_eStopTimer(STATION_SEND_CARD_INFO_EVENT);
            EventUtil_vUnsetEvent(STATION_SEND_CARD_INFO_EVENT);
        }


        // EventUtil_vSetEvent(STATION_CLAIM_EVENT);

        break;
    }

    case TOF_LOC_VERSION_INFO:
    {
        if(psFrame->u8SduLength != psAppPkt->tof_head.len + 4) break;
        //i2c_vPrintf("VER:%d\n",psMcpsIndData->sFrame.sSrcAddr.uAddr.u16Short);
        if(tsLocator[0].bUsed && psMcpsIndData->sFrame.sSrcAddr.uAddr.u16Short == tsLocator[0].u16ShortAddr)
        {
            tsLocator[0].u16LocOADVersion = psAppPkt->rt_tof_loc_version.u16LocOAD;
            if(psAppPkt->tof_head.len - sizeof(psAppPkt->rt_tof_loc_version.u16LocOAD) < 50)
            {
                memcpy(tsLocator[0].u8LocVer,psAppPkt->rt_tof_loc_version.u8LocVersion,
                    psAppPkt->tof_head.len - sizeof(psAppPkt->rt_tof_loc_version.u16LocOAD));
                tsLocator[0].u8LocVer[psAppPkt->tof_head.len - sizeof(psAppPkt->rt_tof_loc_version.u16LocOAD)]=0;
            }
        }
        else if(tsLocator[1].bUsed && psMcpsIndData->sFrame.sSrcAddr.uAddr.u16Short == tsLocator[1].u16ShortAddr)
        {
            tsLocator[1].u16LocOADVersion = psAppPkt->rt_tof_loc_version.u16LocOAD;
            if(psAppPkt->tof_head.len - sizeof(psAppPkt->rt_tof_loc_version.u16LocOAD) < 50)
            {
                memcpy(tsLocator[1].u8LocVer,psAppPkt->rt_tof_loc_version.u8LocVersion,
                    psAppPkt->tof_head.len - sizeof(psAppPkt->rt_tof_loc_version.u16LocOAD));
                tsLocator[1].u8LocVer[psAppPkt->tof_head.len - sizeof(psAppPkt->rt_tof_loc_version.u16LocOAD)]=0;
            }
        }
        break;
    }

    case TOF_STATION_RSSI_CHECK:
    {
        //if(psAppPkt->tof_head.len == 0)
        {
            i8StationRcvRssi = SysUtil_vConvertLQI2Dbm(psFrame->u8LinkQuality);
            vStationCast(TOF_STATION_RSSI_CHECK_ACK, 0xFFFF, 0xFFFF, 0, 0);
        }
        break;
    }
    default:
        break;
    }

    // process card version and battery
    if(psAppPkt->tof_head.msgtype == TOF_CARD_RSSI|| 
		psAppPkt->tof_head.msgtype == TOF_CARD_ALARM || psAppPkt->tof_head.msgtype == TOF_CARD_CHECKIN )
    {
        if(psFrame->u8SduLength == sizeof(rf_tof_card_data_ts) && psAppPkt->rf_tof_card_data.u16OadVersion >0 )
        {
            vProcessCardVersion(psMcpsIndData->sFrame.sSrcAddr.uAddr.u16Short, DEVICE_TYPE_CARD5S, psAppPkt->rf_tof_card_data.u16OadVersion, psAppPkt->rf_tof_card_data.u16Battery);
        }
    }
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
    int8 i8Rssi;
    uint8 u8NeedLoc = 0;
    int16 i16TofDist;

    LedUtil_vToggle(LED_RF);

    switch (eTofStatus)
    {
    case TOF_SUCCESS:
    {
        i16TofDist = i16GetTofDistance(asTofData, STATION_TOF_MAX_READINGS);
        i16TofDist = MAX(0, i16TofDist);
        i8Rssi = i8GetTofRssi();

        tsSlot[u16TofSlotIndex].i8Rssi = i8Rssi;

        //DBG(i2c_vPrintf("%x: RS %i\n", tsSlot[u16TofSlotIndex].u16ShortAddr, i8Rssi););

        //定位距离有效
        if (i16TofDist < MIN_INVALID_TOF_DISTANCE)   //means distance valid
        {
            tsSlot[u16TofSlotIndex].tsTof = (uint16)i16TofDist;
            tsSlot[u16TofSlotIndex].u8LostNum = 0;
        }
        else
        {
            vCheckCardLost(u16TofSlotIndex);
        }

        break;
    }
    default:
    {
        if(tsSlot[u16TofSlotIndex].b2SlotType == SLOT_CARD && eTofStatus <= TOF_RX_ERROR)
        {
            tsSlot[u16TofSlotIndex].tsTof = TofFailDistance[eTofStatus];
        }
        vCheckCardLost(u16TofSlotIndex);

        break;
    }
    }

    //fixed me:  if TOF not start, seems that the card can't receive msg any more, so not need to send msg to card, and not need locator
    if(TOF_NOT_STARTED != eTofStatus)
    {
        bIsTofSuccess = TRUE;
        if(tsLocator[0].bUsed)       //locator 0
        {
            u8NeedLoc |= 0x01;
        }
        if(tsLocator[1].bUsed)       //locator 1
        {
            u8NeedLoc |= 0x02;
        }

        vStationCast(TOF_STATION_FINISH, u16TofPendingAddr, TOF_CARD_NWK_ADDR, MAC_TX_OPTION_ACK, u8NeedLoc);
        DBG(
            //i2c_vPrintf("%d: Fin to %x \n", u16TofSlotIndex, u16TofPendingAddr);
        )
    }
    else
    {
        DBG(
            //i2c_vPrintf("%d: TOF not started %x \n", u16TofSlotIndex, u16TofPendingAddr);
        )
    }
}

int8 i8GetTofRssi(void)
{
    //Calculate RSSI
    int8 n=0, lc=0, rc=0;
    int16 rs=0, ls=0;
    int8 lrssi = INVALID_RSSI, rrssi = INVALID_RSSI;
    for(; n < STATION_TOF_MAX_READINGS; n++)
    {
        if(asTofData[n].s8RemoteRSSI != 0)
        {
            rs += asTofData[n].s8RemoteRSSI;
            rc++;
        }

        if(asTofData[n].s8LocalRSSI != 0)
        {
            ls += asTofData[n].s8LocalRSSI;
            lc++;
        }
    }

    if(lc>0)
    {
        lrssi = -7 - (108-(int8)(ls/lc));
    }

    if(rc>0)
    {
        rrssi = -7 - (108-(int8)(rs/rc));
    }

    return rrssi;
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
        RfTofData.rf_tof_station_accept.u16CurSlot = u16SlotIndex;
        RfTofData.rf_tof_station_accept.u8RunMs = (uint16)((u32AHI_TickTimerRead() - u32SlotTick) / 16000);

        if(u8Val == 1) // this is for locator's accept
        {
            RfTofData.rf_tof_station_accept.u8AcceptNum = 1;
            RfTofData.rf_tof_station_accept.tsAccptData[0].u16ShortAddr = u16DstAddr;
            RfTofData.rf_tof_station_accept.tsAccptData[0].u16SlotIndex = u16AcceptLocSlot;
        }
        else if(u8Val == 2) // this is for gasnode's accept
        {
            RfTofData.rf_tof_station_accept.u8AcceptNum = 1;
            RfTofData.rf_tof_station_accept.tsAccptData[0].u16ShortAddr = u16DstAddr;
            RfTofData.rf_tof_station_accept.tsAccptData[0].u16SlotIndex = u16AcceptGasSlot;
        }
        else        // for card's accept
        {
            uint8 u8Tmp;
            for (u8Tmp = 0; u8Tmp < u8AcceptNum; u8Tmp++)
            {
                RfTofData.rf_tof_station_accept.tsAccptData[u8Tmp].u16ShortAddr = tsAccepted[u8Tmp].u16ShortAddr;
                RfTofData.rf_tof_station_accept.tsAccptData[u8Tmp].u16SlotIndex = tsAccepted[u8Tmp].u16SlotIndex;
            }

            RfTofData.rf_tof_station_accept.u8AcceptNum = u8AcceptNum;
        }

        RfTofData.tof_head.len = (RfTofData.rf_tof_station_accept.u8AcceptNum + 2)*4;
        break;
    }

    case TOF_STATION_AVAILABLE:
    case TOF_STATION_RSSI:
    case TOF_STATION_BUSY:
    case TOF_STATION_ILDE:
    {
        RfTofData.rf_tof_station_signal.u8AvailableMs = u8Val;    // card will send request in random (0 ~ u8Val) ms
        RfTofData.rf_tof_station_signal.u8StationStatus = tStationStatus;
        RfTofData.rf_tof_station_signal.u16CurSlot = u16SlotIndex;
        RfTofData.rf_tof_station_signal.u8RunMs = (uint8)((u32AHI_TickTimerRead() - u32SlotTick) / 16000);
        RfTofData.rf_tof_station_signal.u8LocIdle = u8AvailSlotType;
        RfTofData.tof_head.len = 6;    //this is not a 4-byte struct !!!!!!!!!!!
        break;
    }

    case TOF_STATION_FINISH:
    {
        RfTofData.rf_tof_station_finish.u8RunMs = (uint8)((u32AHI_TickTimerRead() - u32SlotTick) / 16000);

        RfTofData.rf_tof_station_finish.u8LocN = u8Val;
        RfTofData.rf_tof_station_finish.u8Reserved = 0xFF;

        // 单卡撤离
        if(SingleCardRetreat.u8CardCount > 0)
        {
            uint8 i = 0;
            for(; i < SingleCardRetreat.u8CardCount; i++)
            {
                if(u16DstAddr == SingleCardRetreat.u16RetreatCard[i])
                {
                    RfTofData.rf_tof_station_finish.u8StationStatus = STATION_STATUS_RETREAT;
                    break;
                }
            }
            if(i >= SingleCardRetreat.u8CardCount)
                RfTofData.rf_tof_station_finish.u8StationStatus = STATION_STATUS_NORMAL;
        }
        else
            RfTofData.rf_tof_station_finish.u8StationStatus = tStationStatus;

        if(CardSpeedRemind.u8CardCount > 0)
        {
            uint8 i = 0;
            for(; i < CardSpeedRemind.u8CardCount; i++)
            {
                if(u16DstAddr == CardSpeedRemind.u16SpeedCard[i])
                {
                    RfTofData.rf_tof_station_finish.u8StationStatus |= STATION_STATUS_SPEED;
                    break;
                }
            }
        }

        if(!bIsTofSuccess)
        {
            RfTofData.rf_tof_station_finish.u16Dist2Station = INVALID_TOF_DISTANCE;
        }
        else
        {
            RfTofData.rf_tof_station_finish.u16Dist2Station = tsSlot[u16TofSlotIndex].tsTof;
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
            uint16 ii;
            for(ii = 0; ii < TOF_SLOT_NUM; ii++)
            {
                if((tsSlot[ii].u16ShortAddr == u16DstAddr) && (tsSlot[ii].u8AppHelpAck > 0))
                {
                    RfTofData.tof_head.msgtype = TOF_APP_HELP_ACK;
                    tsSlot[ii].u8AppHelpAck --;
                    break;
                }
            }
        }
        break;
    }

    case TOF_GASNODE_HELP_ACK:
    case TOF_GASNODE_FIRE_ACK:
    case TOF_GASNODE_WATER_ACK:
    case TOF_GASNODE_TOPBOARD_ACK:
    case TOF_GASNODE_OTHER_ACK:
    case TOF_LOC_VERSION_INFO:
    {
        RfTofData.tof_head.len = 0;
        break;
    }

    case TOF_STATION_WAIT:
    case TOF_STATION_LOC_INFO:
        RfTofData.tof_head.len = 0;
        break;

    case TOF_STATION_RSSI_CHECK_ACK:
        RfTofData.rt_tof_station_rssi.i8Rssi = i8StationRcvRssi;
        RfTofData.rt_tof_station_rssi.u8StationPort = 0;
        RfTofData.tof_head.len = 2;
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
 * Station send the card's info to loc.
 *
 * PARAMETERS:      None
 *
 * RETURNS:
 * void
 *
 ****************************************************************************/
PRIVATE void vSendCardInfo()
{
    uint16 i;
    // 防止已经过了多个slot, 计算该节的第一个slot
    uint16 u16CurrIntIndex = (u16SlotIndex/TOF_SLOT_LOC_INT)*TOF_SLOT_LOC_INT;
    uint16 u16LastIntIndex = (u16CurrIntIndex + TOF_SLOT_NUM - TOF_SLOT_LOC_INT) % TOF_SLOT_NUM;

    MacUtil_SendParams_s 	sParams;
    sParams.u8Radius        = 1;
    sParams.u16DstAddr    	= 0xFFFF;					//向辅站广播
    sParams.u16DstPanId     = TOF_LOCNODE_NWK_ADDR;
    sParams.u16ClusterId     = 0;
    sParams.u8DeliverMode    = MAC_UTIL_BROADCAST;

    RfTofWrapper_tu RfTofdata;
    RfTofdata.rf_tof_station_card_info.tof_head.protocoltype = APP_PROTOCOL_TYPE_CARD;
    RfTofdata.rf_tof_station_card_info.tof_head.msgtype = TOF_STATION_CARDS_INFO;
    RfTofdata.rf_tof_station_card_info.tof_head.len = sizeof(rf_tof_station_card_info_ts) - sizeof(app_header_t);

    // 时间是从本节的第一个slot的起始地址开始
    RfTofdata.rf_tof_station_card_info.u8RunMs = (uint8)((u32AHI_TickTimerRead() - u32SlotTick) / 16000) + (u16SlotIndex - u16CurrIntIndex)*TOF_SLOT_MS;
    RfTofdata.rf_tof_station_card_info.u8CardNum = TOF_SLOT_LOC_INT-2;

    for(i=2; i<TOF_SLOT_LOC_INT; i++)
    {
        if(tsSlot[u16LastIntIndex + i].b2SlotType == SLOT_CARD && tsSlot[u16LastIntIndex + i].b1Used)
        {
            RfTofdata.rf_tof_station_card_info.u16CardAddr[i-2] =  tsSlot[u16LastIntIndex + i].u16ShortAddr;
        }
        else
        {
            RfTofdata.rf_tof_station_card_info.u16CardAddr[i-2] = 0;  // 用0来表示无效地址，无需tof
        }
    }

    vSetStationChannel(u8TofChannel);
    MacUtil_vSendData(&sParams, (uint8*)&RfTofdata, RfTofdata.rf_tof_station_card_info.tof_head.len + sizeof(app_header_t), 0);
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
PRIVATE void vStopTofIndex(uint16 u16Index)
{
    if (bTofBusy)
    {
        EventUtil_vUnsetEvent(STATION_WAIT_TOF2CARD_EVENT);
        vAppApiTofInit(FALSE);
        vAppApiTofInit(TRUE);
        vCurrentTofDone();
    }

    vCheckCardLost(u16Index);

    // fixme
    // if the card is still doing TOF, it seems always can't receive any package, so need locator is not use here
    // and not need to send TOF_STATION_FINISH to tsSlot[u16TofSlotIndex].u16ShortAddr
    // need check the reason
    if ((tsSlot[u16Index].b1Used) && (u16Index != u16TofSlotIndex))    // no time left
    {
        bIsTofSuccess = FALSE;
        vStationCast(TOF_STATION_FINISH, tsSlot[u16Index].u16ShortAddr, TOF_CARD_NWK_ADDR, MAC_TX_OPTION_ACK, 0);
    }
}


/****************************************************************************
 *
 * NAME: vProcessLocatorCardDist
 *
 * DESCRIPTION:
 * 辅站回复定位卡测距结果，根据结果更新主站的slot里面保存的定位信息
 *
 * PARAMETERS:
 *                 u8Len - card number
 *                 u8Loc - which locator
 *
 * RETURNS:
 * void
 *
 ****************************************************************************/
PRIVATE void vProcessLocatorCardDist(uint16 *pCardDist, uint8 u8Len, uint8 u8Loc)    // u8Loc: 0, 1
{
    uint16 i;

    uint16 u16LastIntIndex;	  //上一 "节"的第一个slot
    uint16 u16CurrIntIndex;   //当前 "节"的第一个slot, 理论上等于u16SlotIndex，为了防止时间已经走过了一个slot

    sys_assert(pCardDist != NULL);
    sys_assert(u8Len == TOF_SLOT_LOC_INT-2);
    sys_assert(u8Loc < 2);

    u16CurrIntIndex = (u16SlotIndex/TOF_SLOT_LOC_INT)*TOF_SLOT_LOC_INT;
    u16LastIntIndex = (u16CurrIntIndex + TOF_SLOT_NUM - 2*TOF_SLOT_LOC_INT) % TOF_SLOT_NUM;

    for(i=2; i<TOF_SLOT_LOC_INT; i++)
    {
        if(tsSlot[u16LastIntIndex + i].b2SlotType == SLOT_CARD)  // 不管是否 used都做更新
        {
            tsSlot[u16LastIntIndex + i].u16LocCardDistance[u8Loc] = *pCardDist;
        }
        pCardDist++;
    }
    return;
}

/****************************************************************************
 *
 * NAME: u16NextSlotIndex
 *
 * DESCRIPTION:
 * Get next slot index
 *
 * PARAMETERS:      u16index - current slot index
 *
 * RETURNS:
 *         uint16: next slot index
 *
 ****************************************************************************/
PRIVATE uint16 u16NextSlotIndex(uint16 u16index)
{
    uint16 u16tmp = u16index;
    u16tmp++;
    if(u16tmp >= TOF_SLOT_NUM)
        u16tmp = 0;

    return u16tmp;
}

PRIVATE uint32 u32NextSlotTick(uint32 u32tick)
{
    return u32tick+TOF_SLOT_TICK;
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
PRIVATE void vCheckCardLost(uint16 u16Index)
{
    uint16 u16BaseIndex;
    uint16 i;

    u16BaseIndex = u16GetCardBaseIndex(u16Index, tsSlot[u16Index].u8DeviceType);

    tsSlot[u16Index].u8LostNum = ++tsSlot[u16BaseIndex].u8LostNum;
    if(tsSlot[u16Index].u8LostNum >= TOF_STATION_CARD_MAX_FAILED)
    {
        switch(tsSlot[u16Index].u8DeviceType)
        {
        case DEVICE_TYPE_CARD1S:
        {
            for(i=0; i<TOF_SLOT_NUM/TOF_SLOT_LOC_PERIOD; i++)
            {
                vReleaseSlot(u16BaseIndex + i* TOF_SLOT_LOC_PERIOD);
            }
            break;
        }
        case DEVICE_TYPE_CARD5S:
        {
            for(i=0; i<(TOF_SLOT_NUM/TOF_SLOT_LOC_PERIOD)/5; i++)
            {
                vReleaseSlot(u16BaseIndex + i* (TOF_SLOT_LOC_PERIOD*5));
            }
            break;
        }
        case DEVICE_TYPE_CARD15S:
        {
            vReleaseSlot(u16Index);
            break;
        }
        }
        // 更新目前的 avail slot 情况，供CLIAM 使用
        vUpdateAvailSlot(u16Index);
    }
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
PRIVATE void vCheckCardStatus(uint16 u16ShortAddr, uint8 u8Status,uint8 u8ExciteID)
{
    app_tof_alarm_type_te teStatus = APP_TOF_ALARM_NONE;

    if(u8Status & CARD_STATUS_HELP)
    {
        teStatus = u8Status;
        vStationCast(TOF_STATION_HELP_ACK, u16ShortAddr, TOF_CARD_NWK_ADDR, 0, 0);
    }
    else if((u8Status & CARD_STATUS_NOPWD) || (u8Status & CARD_STATUS_RETREAT_ACK))
    {
        teStatus = u8Status;
        //vStationCast(TOF_STATION_NOPWD_ACK, u16ShortAddr, TOF_CARD_NWK_ADDR, 0, 0);
    }
    else if(u8Status & CARD_STATUS_EXCITER)
    {
        teStatus = u8Status;
        vStationCast(TOF_STATION_EXCITE_ACK, u16ShortAddr, TOF_CARD_NWK_ADDR, 0, 0);
    }
    else
    {
        return;
    }

    vReportCardAlarm(u16ShortAddr,teStatus,u8ExciteID);
}

PRIVATE void vCheckCardRssi(uint16 u16ShortAddr, uint16 u16SeqNum, uint8 u8LinkQuality, uint8 RssiType,uint8 u8AccStatus)
{
    app_tof_more_rssi_data.tof_rssi[u16RssiNum].u16ShortAddr = u16ShortAddr;
    app_tof_more_rssi_data.tof_rssi[u16RssiNum].u16SeqNum = u16SeqNum;
    app_tof_more_rssi_data.tof_rssi[u16RssiNum].i8Rssi = SysUtil_vConvertLQI2Dbm(u8LinkQuality);
    app_tof_more_rssi_data.tof_rssi[u16RssiNum].u8RssiType = RssiType;//APP_TOF_CARD_RSSI;
    app_tof_more_rssi_data.tof_rssi[u16RssiNum].u8Accel = u8AccStatus;

    u16RssiNum++;
    if(u16RssiNum >= APP_MAX_CARD_NUM)
    {
        vReportCardRssi();
    }
    return;
}

/****************************************************************************
 *
 * NAME: vCheckGasNodeStatus
 *
 * DESCRIPTION:
 * Check the certain GasNode's status: help, SOS type or normal
 *
 * PARAMETERS:
 *                u16ShortAddr - the checked card's addr
 *                 u8Status - the card's status bitmap
 *
 * RETURNS:
 *         void
 *
 ****************************************************************************/
/*PRIVATE void vCheckGasNodeStatus(uint16 u16ShortAddr, uint8 u8Status)
{
    app_tof_alarm_type_te teStatus = APP_TOF_ALARM_NONE;
    // help has priority higher than nopwd
    if(u8Status & GASNODE_STATUS_HELP)
    {
        teStatus = APP_TOF_ALARM_GASNODE_URGENT;
        //vStationCast(TOF_STATION_HELP_ACK, u16ShortAddr, TOF_CARD_NWK_ADDR, 0, 0);
    }

    else if(u8Status & GASNODE_STATUS_FIRE)
    {
        teStatus = APP_TOF_ALARM_GASNODE_FIRE;
        //vStationCast(TOF_STATION_NOPWD_ACK, u16ShortAddr, TOF_CARD_NWK_ADDR, 0, 0);
    }
    else if(u8Status & GASNODE_STATUS_WATER)
    {
        teStatus = APP_TOF_ALARM_GASNODE_WATER;
    }
    else if(u8Status & GASNODE_STATUS_TOPBOARD)
    {
        teStatus = APP_TOF_ALARM_GASNODE_TOPBOARD;
    }
    else if(u8Status & GASNODE_STATUS_OTHER)
    {
        teStatus = APP_TOF_ALARM_GASNODE_OTHERS;
    }
    else
    {
        return;
    }

    vReportGasNodeAlarm(u16ShortAddr, teStatus);
}*/


/****************************************************************************
 *
 * NAME: bArmReady
 *
 * DESCRIPTION:
 * Check arm is ready or not every time when send data to arm.
 *
 * PARAMETERS:      None
 *
 * RETURNS:
 *         TRUE - got the arm id
 *        FALSE - has not get the arm id
 *
 ****************************************************************************/
PRIVATE bool bArmReady()
{
    int8  port = Jbsmac_i8GetUpPort();
    if(port >=0  && Jbsmac_u8GetLinkStatus(port) > 0)
    {
        u16ArmId = Jbsmac_u16GetArmid();
        return TRUE;
    }
    else
    {
        return FALSE;
    }

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
    )
    if(len < sizeof(struct nwkhdr) + sizeof(app_header_t))
        return;

    RfTofWrapper_tu* psAppPkt = (RfTofWrapper_tu*)(pbuf + sizeof(struct nwkhdr));

    // not for me
    if(APP_PROTOCOL_TYPE_CARD != psAppPkt->tof_head.protocoltype)
        return;

    switch(psAppPkt->tof_head.msgtype)
    {
    case APP_TOF_MSG_RETREAT:
    {
        tStationStatus = STATION_STATUS_RETREAT;
        SingleCardRetreat.u8CardCount = 0;
        break;
    }

    case APP_TOF_MSG_LINK:
    {
        EventUtil_vSetEvent(STATION_REPORT_STATUS);
        break;
    }

    case APP_TOF_MSG_CANCEL_RETREAT:
    {
        tStationStatus = STATION_STATUS_NORMAL;
        SingleCardRetreat.u8CardCount = 0;
        break;
    }

    case APP_TOF_MSG_ALARM_ACK:
    {
        app_tof_alarm_ack_ts* app_tof_alarm_ack = (app_tof_alarm_ack_ts*)psAppPkt;
        vConvertEndian16(&(app_tof_alarm_ack->u16ShortAddr));
        DBG(
            i2c_vPrintf("alarm ack: %x\n", app_tof_alarm_ack->u16ShortAddr);
        )
        if(app_tof_alarm_ack->u8AckType == APP_TOF_ALARM_CARD_HELP_ACK)
        {
            uint16 ii;
            for(ii = 0; ii < TOF_SLOT_NUM; ii++)
            {
                if(tsSlot[ii].u16ShortAddr == app_tof_alarm_ack->u16ShortAddr)
                {
                    tsSlot[ii].u8AppHelpAck = 3; // send app's ack to card in STATION_FIN 3 times
                    break;
                }
            }
        }
        break;
    }

    /*case APP_TOF_MSG_GAS_ALARM_ACK:
    {
        app_tof_alarm_ack_ts* app_tof_alarm_ack = (app_tof_alarm_ack_ts*)psAppPkt;
        vConvertEndian16(&(app_tof_alarm_ack->u16ShortAddr));
        uint8 ii;
        for(ii = 47; ii < TOF_SLOT_NUM; ii+=50)
        {
            if(tsSlot[ii].u16ShortAddr == app_tof_alarm_ack->u16ShortAddr||tsSlot[ii+1].u16ShortAddr == app_tof_alarm_ack->u16ShortAddr)
            {
                tsSlot[ii].u8AppHelpAck = 3; // send app's ack to gasnode in STATION_FIN 3 times
                tsSlot[ii+1].u8AppHelpAck = 3;
                tsSlot[ii].u8AppHelpAckType = app_tof_alarm_ack->u8AckType;
                tsSlot[ii+1].u8AppHelpAckType = app_tof_alarm_ack->u8AckType;
                break;
            }
        }
        break;
    }*/

    case APP_TOF_MSG_REPORT_ACK:
    {
        app_LSrfReport_t *prfReport = (app_LSrfReport_t *)((app_header_t *)psAppPkt + 1);
        if( prfReport->reporttype == APP_LS_REPORT_LIVE)
        {
            DBG(
                //i2c_vPrintf("live ack\n");
            )
            u8LastLiveAckSeq = prfReport->seqnum;
            bIsStationUp = TRUE;
            u8RestartCounter = 0;
        }
        break;
    }
    case MODULE_ERROR_RATE_TEST:
    {
        DBG(
            i2c_vPrintf("ERROR_RATE_TEST\n", len);
        )
        struct nwkhdr *pNwkHdr = (struct nwkhdr *)u32CommonBuf;
        app_header_t *pHeader = (app_header_t *)(pNwkHdr + 1);

        pNwkHdr->type = NWK_DATA;
        pNwkHdr->ttl = 1;
        pNwkHdr->src = u16StationPanId;
        SysUtil_vConvertEndian(&pNwkHdr->src, sizeof(pNwkHdr->src));

        pNwkHdr->dst = u16ArmId;
        SysUtil_vConvertEndian(&pNwkHdr->dst, sizeof(pNwkHdr->dst));

        pNwkHdr->len =  sizeof(app_header_t);
        SysUtil_vConvertEndian(&pNwkHdr->len, sizeof(pNwkHdr->len));

        pHeader->len = 0;
        SysUtil_vConvertEndian(&pHeader->len, sizeof(pHeader->len));

        pHeader->msgtype = MODULE_ERROR_RATE_TEST;
        pHeader->protocoltype = APP_PROTOCOL_TYPE_CARD;
        Jbsmac_eWriteData((uint8 *)u32CommonBuf, sizeof(struct nwkhdr) + sizeof(app_header_t));
        break;
    }
    case APP_TOF_MSG_SET:
    {
        DBG(i2c_vPrintf("msg set \n"););
        DBG(i2c_vPrintMem((uint8*)psAppPkt, len - sizeof(struct nwkhdr)););

        app_header_t * pheader = (app_header_t *)psAppPkt;
        app_LSrfSet_t * prfSet = (app_LSrfSet_t *)(pheader + 1);

        SysUtil_vConvertEndian((uint8*)&pheader->len, sizeof(pheader->len));
        SysUtil_vConvertEndian((uint8*)&prfSet->hdr.srcaddr, sizeof(prfSet->hdr.srcaddr));
        SysUtil_vConvertEndian((uint8*)&prfSet->hdr.dstaddr, sizeof(prfSet->hdr.dstaddr));
        SysUtil_vConvertEndian((uint8*)&prfSet->seqnum, sizeof(prfSet->seqnum));
        SysUtil_vConvertEndian((uint8*)&prfSet->crc, sizeof(prfSet->crc));

        if((prfSet->hdr.dstaddr == u16StationPanId || prfSet->hdr.dstaddr == 0xFFFF)
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
                uint8 u8NewLocChannel = u8LocatorCardChannel;

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
                    else if(pTlv->type == APP_LS_TLV_TYPE_LOC_DISTANCE_0)
                    {
                    }
                    else if(pTlv->type == APP_LS_TLV_TYPE_LOC_DISTANCE_1)
                    {
                    }
                    /*else if(pTlv->type == APP_LS_TLV_TYPE_LOC_UNCOMPATIBLE_GAS)
                    {
                        if(bSupportedgas)
                        {
                            EDBG(i2c_vPrintf("set gas\n "););
                            EDBG(i2c_vPrintPoll();); //print datas in buffer first

                            bSupportedgas = FALSE;
                            SleepUtil_Sleep(E_AHI_WAKE_TIMER_0, 1, E_AHI_SLEEP_OSCON_RAMON);
                        }
                    }

                    else if(pTlv->type == APP_LS_TLV_TYPE_LOC_COMPATIBLE_GAS)
                    {
                        if(bSupportedgas == FALSE)
                        {
                            EDBG(i2c_vPrintf("set gas\n "););
                            EDBG(i2c_vPrintPoll();); //print datas in buffer first
                            bSupportedgas = TRUE;
                            SleepUtil_Sleep(E_AHI_WAKE_TIMER_0, 1, E_AHI_SLEEP_OSCON_RAMON);
                        }
                    }*/
                    else if(pTlv->type == APP_LS_TLV_TYPE_LOC_COMPATIBLE_SPEED)
                    {
                    }
                    else if(pTlv->type == APP_LS_TLV_TYPE_LOC_UNCOMPATIBLE_SPEED)
                    {
                    }
                    else if(pTlv->type == APP_LS_TLV_TYPE_SINGLE_CARD_RETREAT)
                    {
                        if(tStationStatus != STATION_STATUS_RETREAT)
                        {
                            if(pTlv->len < SINGLE_CARD_RETREAT_SIZE*2)
                            {
                                memcpy(&SingleCardRetreat.u16RetreatCard,pdata,pTlv->len);
                                SingleCardRetreat.u8CardCount = pTlv->len / 2;
                                SingleCardRetreat.u8Duration = SINGLE_CARD_RETREAT_TIMEOUT;
                                tStationStatus = STATION_STATUS_SINGLE_RETREAT;
                                uint8 i;
                                for(i=0; i<SingleCardRetreat.u8CardCount; i++)
                                    SysUtil_vConvertEndian(&SingleCardRetreat.u16RetreatCard[i], sizeof(SingleCardRetreat.u16RetreatCard[i]));
                            }
                        }
                    }
                    else if(pTlv->type == APP_LS_TLV_TYPE_SPEED_REMIND)
                    {
                        if(pTlv->len < CARD_SPEED_REMIND_SIZE*2)
                        {
                            memcpy(&CardSpeedRemind.u16SpeedCard,pdata,pTlv->len);
                            CardSpeedRemind.u8CardCount = pTlv->len / 2;
                            CardSpeedRemind.u8Duration = SINGLE_CARD_RETREAT_TIMEOUT;
                            uint8 i;
                            for(i=0; i<CardSpeedRemind.u8CardCount; i++)
                                SysUtil_vConvertEndian(&CardSpeedRemind.u16SpeedCard[i], sizeof(CardSpeedRemind.u16SpeedCard[i]));
                        }

                    }
                    len -= (sizeof(app_rfTlv_t)  + pTlv->len);
                    pTlv = (app_rfTlv_t *)(pdata  + pTlv->len);
                }

                if(changedChannel && bChannelValid(u8BroadcastChannel, u8NewTofChannel, u8NewLocChannel))
                {
                    vSaveChannel(u8NewTofChannel, u8NewLocChannel);
                    // reset after set
                    vAHI_SwReset();
                }
            }
            else
            {
                EDBG(i2c_vPrintf("CRC failed %d %d\n", prfSet->crc, crc););
            }
        }
        else
        {
            EDBG(i2c_vPrintf("head error \n"););
        }
        break;
    }
    default:
        break;
    }
}

/****************************************************************************
 *
 * NAME: vReportCardDistance
 *
 * DESCRIPTION:
 * Station report card distance to ARM, the max card number is APP_MAX_CARD3_NUM per time,
 * if the card number is larger than APP_MAX_CARD3_NUM, will send several times.
 *
 * PARAMETERS:      None
 *
 * RETURNS:
 *         void
 *
 ****************************************************************************/
PRIVATE void vReportCardDistance()
{
    // 本节第一个slot
    uint16 u16IntBeginSlot = (u16SlotIndex/TOF_SLOT_LOC_INT)*TOF_SLOT_LOC_INT;

    uint16 u16CardBeginSlot = ((u16IntBeginSlot + TOF_SLOT_NUM - TOF_SLOT_LOC_INT*2) % TOF_SLOT_NUM);
    uint16 u16CardNum = 0;

    app_new_tof_distance_ts app_tof_distance_data;

    app_tof_distance_data.app_tof_head.protocoltype = APP_PROTOCOL_TYPE_CARD;
    app_tof_distance_data.app_tof_head.msgtype = APP_TOF_MSG_NEW_DISTANCE;

    uint16 i;
    for(i = u16CardBeginSlot; i < u16CardBeginSlot + TOF_SLOT_LOC_INT; i++)
    {
        tsTofSlot *slot1 = &tsSlot[i%TOF_SLOT_NUM]; //*slot2;
        tof_new_distance_ts  *distance_data = &app_tof_distance_data.tof_distance[u16CardNum];

        if(slot1->b1Used && slot1->b2SlotType == SLOT_CARD)
        {
            app_tof_distance_data.tof_distance[u16CardNum].u16ShortAddr  = slot1->u16ShortAddr;
            app_tof_distance_data.tof_distance[u16CardNum].u16SeqNum     = slot1->u16SeqNum;
            app_tof_distance_data.tof_distance[u16CardNum].u16Distance   = slot1->tsTof;
            app_tof_distance_data.tof_distance[u16CardNum].i8Rssi        = slot1->i8Rssi;
            app_tof_distance_data.tof_distance[u16CardNum].u16LocCardDistance[0] = slot1->u16LocCardDistance[0];
            app_tof_distance_data.tof_distance[u16CardNum].u16LocCardDistance[1] = slot1->u16LocCardDistance[1];

            app_tof_distance_data.tof_distance[u16CardNum].u8DevType = APP_TOF_CARD_5S;
            if(slot1->u8DeviceType == DEVICE_TYPE_CARD1S)
            {
                distance_data->u8DevType = APP_TOF_CARD_1S;
            }
            else if(slot1->u8DeviceType == DEVICE_TYPE_CARD15S)
            {
                distance_data->u8DevType = APP_TOF_CARD_15S;
            }
            else
            {
                distance_data->u8DevType = APP_TOF_CARD_5S;
            }
            u16CardNum++;
        }

        if(APP_MAX_CARD3_NUM == u16CardNum)
        {
            //DBG(i2c_vPrintf("slot:%d df#:%d\n", u16SlotIndex, u16CardNum);)
            app_tof_distance_data.app_tof_head.len = u16CardNum*sizeof(tof_new_distance_ts);

            if(bIsStationUp)
                vWriteData2Arm((uint8*)&app_tof_distance_data);

            u16CardNum = 0;
        }
    }

    if(0 < u16CardNum)
    {
        //DBG(i2c_vPrintf("slot:%d d#:%d\n", u16SlotIndex, u16CardNum);)
        app_tof_distance_data.app_tof_head.len = u16CardNum*sizeof(tof_new_distance_ts);

        if(bIsStationUp)
            vWriteData2Arm((uint8*)&app_tof_distance_data);
        u16CardNum = 0;
    }
}

/****************************************************************************
 *
 * NAME: vReportCardRssi
 *
 * DESCRIPTION:
 * Station report card rssi to ARM, the max card number is APP_MAX_CARD_NUM per time,
 * if the card number is larger than APP_MAX_CARD_NUM, will send several times.
 *
 * PARAMETERS:      None
 *
 * RETURNS:
 *         void
 *
 ****************************************************************************/
PRIVATE void vReportCardRssi()
{
    if(0 < u16RssiNum)
    {
        //DBG(i2c_vPrintf("slot:%d r#:%d\n\n", u16SlotIndex, u16RssiNum);)
        app_tof_more_rssi_data.app_tof_head.len = u16RssiNum*sizeof(tof_rssi_ts);

        if(bIsStationUp)
            vWriteData2Arm((uint8*)&app_tof_more_rssi_data);

        u16RssiNum = 0;
    }
}

/****************************************************************************
 *
 * NAME: vReportGasNodeRssi
 *
 * DESCRIPTION:
 * Station report GasNode rssi to ARM, the max GasNode number is APP_MAX_GAS_NUM per time,
 * if the GasNode number is larger than APP_MAX_CARD_NUM, will send several times.
 *
 * PARAMETERS:      None
 *
 * RETURNS:
 *         void
 *
 ****************************************************************************/
/*PRIVATE void vReportGasNodeRssi()
{
    if(0 < u16GasRssiNum)
    {
        //DBG(i2c_vPrintf("slot:%d r#:%d\n\n", u16SlotIndex, u16GasRssiNum);)
        app_tof_gas_rssi_data.app_tof_head.len = u16GasRssiNum*sizeof(tof_gas_rssi_ts);

        if(bIsStationUp)
            vWriteData2Arm((uint8*)&app_tof_gas_rssi_data);

        u16GasRssiNum = 0;
    }
}*/

/*PRIVATE void vReportGasNodeDensity()
{

    if(bIsReciveGasDensity == TRUE)
    {
        bIsReciveGasDensity = FALSE;
        vWriteData2Arm((uint8*)&app_tof_gasdensity_data);
    }
}*/

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
PRIVATE void vReportCardAlarm(uint16 u16ShortAddr, uint8 u8CardStatus,uint8 u8ExciteID)
{
//i2c_vPrintf("%d:alarm %x\n\n", u16SlotIndex, u16ShortAddr);
    app_tof_alarm_ts app_tof_alarm_data;

    app_tof_alarm_data.app_tof_head.protocoltype = APP_PROTOCOL_TYPE_CARD;
    app_tof_alarm_data.app_tof_head.msgtype = APP_TOF_MSG_NEW_ALARM;
    app_tof_alarm_data.app_tof_head.len = 4;
    app_tof_alarm_data.u16ShortAddr = u16ShortAddr;
    app_tof_alarm_data.u8Status = u8CardStatus;
    app_tof_alarm_data.u8ExciterID = u8ExciteID;

    if(bIsStationUp)
        vWriteData2Arm((uint8*)&app_tof_alarm_data);
}

/*PRIVATE void vReportCardExcite(uint16 u16ShortAddr, uint8 u8CardStatus,uint8 u8ExciteID)
{
    app_tof_alarm_ts app_tof_alarm_data;

    app_tof_alarm_data.app_tof_head.protocoltype = APP_PROTOCOL_TYPE_CARD;
    app_tof_alarm_data.app_tof_head.msgtype = APP_TOF_MSG_NEW_ALARM;
    app_tof_alarm_data.app_tof_head.len = 4;
    app_tof_alarm_data.u16ShortAddr = u16ShortAddr;
    app_tof_alarm_data.u8Status = u8CardStatus;
    app_tof_alarm_data.u8ExciterID = u8ExciteID;

    if(bIsStationUp)
        vWriteData2Arm((uint8*)&app_tof_alarm_data);
}*/



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

    /*case APP_TOF_MSG_DISTANCE:
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
    }*/
    case APP_TOF_MSG_NEW_DISTANCE:
    {
        app_new_tof_distance_ts* pDistance_data = (app_new_tof_distance_ts*)pbuf;
        uint16 u16CardLen = u16App_len / sizeof(tof_new_distance_ts);
        for (u16CardIndex = 0; u16CardIndex < u16CardLen; u16CardIndex++)
        {
            vConvertEndian16(&(pDistance_data->tof_distance[u16CardIndex].u16ShortAddr));
            vConvertEndian16(&(pDistance_data->tof_distance[u16CardIndex].u16SeqNum));
            vConvertEndian16(&(pDistance_data->tof_distance[u16CardIndex].u16Distance));
            vConvertEndian16(&(pDistance_data->tof_distance[u16CardIndex].u16LocCardDistance[0]));
            vConvertEndian16(&(pDistance_data->tof_distance[u16CardIndex].u16LocCardDistance[1]));
        }
        break;
    }

    case APP_TOF_MSG_NEW_ALARM:
    {
        app_tof_alarm_ts* pAlarm_data = (app_tof_alarm_ts*) pbuf;
        vConvertEndian16(&(pAlarm_data->u16ShortAddr));
        break;
    }

    case APP_TOF_MSG_GAS_ALARM:
    {
        app_tof_alarm_ts* pAlarm_data = (app_tof_alarm_ts*) pbuf;
        vConvertEndian16(&(pAlarm_data->u16ShortAddr));
        break;
    }

    case APP_TOF_MSG_GAS_DENSITY:
    {
        app_tof_gasdensity_ts* pDensity_data = (app_tof_gasdensity_ts*)pbuf;
        //vConvertEndian16(&(pDensity_data->app_tof_head.len));
        vConvertEndian16(&(pDensity_data->u16ShortAddr));
        vConvertEndian16(&(pDensity_data->u16SeqNum));
        vConvertEndian16(&(pDensity_data->u16Distance));
        vConvertEndian16(&(pDensity_data->u16LocDistance[0]));
        vConvertEndian16(&(pDensity_data->u16LocDistance[1]));
        vConvertEndian16(&(pDensity_data->u16GasDensity));
        vConvertEndian16(&(pDensity_data->u16GasThr));
        break;
    }

    case APP_TOF_MSG_GAS_RSSI:
    {
        app_tof_gas_rssi_ts* pGasRssi_data = (app_tof_gas_rssi_ts*)pbuf;
        uint16 u16CardLen = u16App_len / sizeof(tof_gas_rssi_ts);
        for (u16CardIndex = 0; u16CardIndex < u16CardLen; u16CardIndex++)
        {
            vConvertEndian16(&(pGasRssi_data->tof_gas_rssi[u16CardIndex].u16ShortAddr));
            vConvertEndian16(&(pGasRssi_data->tof_gas_rssi[u16CardIndex].u16SeqNum));
            vConvertEndian16(&(pGasRssi_data->tof_gas_rssi[u16CardIndex].u16GasDensity));
            vConvertEndian16(&(pGasRssi_data->tof_gas_rssi[u16CardIndex].u16GasThr));
        }
        break;
    }
    case   APP_TOF_MSG_RSSI_TOF_STATION:
    {
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

    //uint8 u8Data[120];
    //memcpy(u8Data, (uint8*)pnwk_data, nwk_len);

    /*
    DBG(
           uint16 u16Tmp;
        for(u16Tmp = 0; u16Tmp < nwk_len; u16Tmp++)
            i2c_vPrintf("%x  ", u8Data[u16Tmp]);
        i2c_vPrintf("\nLen: %d\n", nwk_len);
        i2c_vPrintf("\nDst: %d\n", pnwk_data->dst);
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
                    /*DBG(
                        i2c_vPrintf("%x\t%d\t%d\t%d\t%d\t%d, \t%d\t%d\n", i+CARD_BASE, u32StatTofNum[i], u32StatTofSuccess[i], u32StatTofFailed[i], u32StatTofSkip[i], u32StatTofUnCom[i], u32StatTofLocatorNum[i], u32StatTofLocatorSuccess[i]);
                    )*/
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
 * NAME: vLocTof
 *
 * DESCRIPTION:
 * When the loc join in, station need do TOF with it several times to determine the distance of S-L.
 *
 * PARAMETERS:      None
 *
 * RETURNS:
 *         void
 *
 ****************************************************************************/
PRIVATE void vLocTof(uint8 u8Loc)
{
    MAC_Addr_s sAddr;
    sys_assert(u8Loc < 2);

    sAddr.u16PanId = TOF_LOCNODE_NWK_ADDR;
    sAddr.u8AddrMode = 2;
    sAddr.uAddr.u16Short = tsLocator[u8Loc].u16ShortAddr;

    u16TofSlotIndex = u16SlotIndex;
    u32TofSlotTick = u32SlotTick;
    u16TofPendingAddr = tsLocator[u8Loc].u16ShortAddr;

    eTofStatus = -1;
    vSetStationChannel(u8TofChannel);

    if (bAppApiGetTof(asTofData, &sAddr, STATION_TOF_MAX_READINGS, API_TOF_FORWARDS, vTofCallback))
    {
        /* start to check loc tof result */
        EventUtil_vSetEvent(STATION_LOCTOF_CHECK_EVENT);

        EventUtil_vUnsetEvent(STATION_LOCTOF_TIMEOUT_EVENT);
        TimerUtil_eSetTimer(STATION_LOCTOF_TIMEOUT_EVENT, 15);
        // eTofStatus will not be -1 when TOF finished, to double safe guard, use timer to avoid dead-loop
    }
}


/****************************************************************************
 *
 * NAME: vLocTofDone
 *
 * DESCRIPTION:
 * process  the tof result between S-L
 *
 * PARAMETERS:      None
 *
 * RETURNS:
 *         void
 *
 ****************************************************************************/
void vLocTofDone(void)
{

    uint8 u8Loc;

    if(tsLocator[0].bUsed && tsLocator[0].u16ShortAddr == u16TofPendingAddr)
    {
        u8Loc = 0;
    }
    else if(tsLocator[1].bUsed && tsLocator[1].u16ShortAddr == u16TofPendingAddr)
    {
        u8Loc = 1;
    }
    else
    {
        return;
    }

    if(eTofStatus == TOF_SUCCESS)
    {
        int16 i16TofDist = i16GetTofDistance(asTofData, STATION_TOF_MAX_READINGS);
        if(i16TofDist < MIN_INVALID_TOF_DISTANCE)
        {
            tsLocator[u8Loc].u16TofDist = MAX(i16TofDist, 0);
        }
        //else
        //{
        //}
    }
    //else
    //{
    //}

    vStationCast(TOF_STATION_FINISH, tsLocator[u8Loc].u16ShortAddr, TOF_LOCNODE_NWK_ADDR, MAC_TX_OPTION_ACK, 0);
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
        EDBG(i2c_vPrintf("Flash Read Fail\n"););
        return FALSE;
    }

    if(!bAHI_FlashEraseSector(7))
    {
        EDBG(i2c_vPrintf("Flash Erase Fail\n"););
        return FALSE;
    }
    tsparam.u8TofChannel = u8TofChannel;
    tsparam.u8LocChannel = u8LocChannel;

    if(bAHI_FullFlashProgram(PARAM_STORE_ADDR, sizeof(tsParam_t),(uint8*)&tsparam))
    {
        EDBG(i2c_vPrintf("Set  channel Success!%d %d\n", tsparam.u8TofChannel, tsparam.u8LocChannel););
        return TRUE;
    }
    else
    {
        EDBG(i2c_vPrintf("Set  channel fail!%d %d\n", tsparam.u8TofChannel, tsparam.u8LocChannel););
        return FALSE;
    }
}

/****************************************************************************
 *
 * NAME: bChannelValid
 *
 * DESCRIPTION:
 * caculate channel is valid or not
 *
 * RETURNS:
 * bool TURE for valid, FALSE for not valid
 *
 ****************************************************************************/
bool bChannelValid(uint8 u8BroadcastChannel, uint8 u8TofChannel, uint8 u8LocatorCardChannel)
{
    //fix me:
    //the 26 channel have some bug and do not use in this project
    if(u8BroadcastChannel>=11 && u8BroadcastChannel<=25
            && u8TofChannel>=11 && u8TofChannel<=25
            && u8LocatorCardChannel>=11 && u8LocatorCardChannel<=25
            && u8BroadcastChannel!=u8TofChannel
            && u8BroadcastChannel!=u8LocatorCardChannel
            && u8TofChannel!= u8LocatorCardChannel)
    {
        return TRUE;
    }
    else
    {
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
    //u16LocatorAddress = u16StationPanId + 10000;

    //FLASH
    bAHI_FlashInit(E_FL_CHIP_ST_M25P40_A, NULL);


    uint8 u8TmpBroadChannel = ((psMacAddr.u32L) >> 16) & 0xFF;
    uint8 u8TmpTofChannel = ((psMacAddr.u32L) >> 8) & 0xFF;
    uint8 u8TmpLocatorCardChannel  =(psMacAddr.u32L) & 0xFF;

    if(u8TmpBroadChannel>=11 && u8TmpBroadChannel<=26)
    {
        u8BroadcastChannel = u8TmpBroadChannel;
    }
    else
    {
        u8BroadcastChannel = DEFAULT_CHANNEL_BROADCAST;
    }

    if(bAHI_FullFlashRead(PARAM_STORE_ADDR, sizeof(tsParam_t),(uint8*)&tsparam)
            && bChannelValid(u8BroadcastChannel, tsparam.u8TofChannel, tsparam.u8LocChannel))
    {
        u8TofChannel = tsparam.u8TofChannel;
        u8LocatorCardChannel = tsparam.u8LocChannel;
    }
    else
    {
        if(bChannelValid( u8BroadcastChannel,  u8TmpTofChannel, u8TmpLocatorCardChannel))
        {
            u8TofChannel = u8TmpTofChannel;
            u8LocatorCardChannel = u8TmpLocatorCardChannel;
        }
        else
        {
            u8TofChannel = DEFAULT_CHANNEL_TOF;
            u8LocatorCardChannel = DEFAULT_CHANNEL_LOCATOR;
        }
        EDBG(i2c_vPrintf("Flash Read fail %d %d, Use default %d %d\n",
                         tsparam.u8TofChannel, tsparam.u8LocChannel, u8TofChannel, u8LocatorCardChannel););
    }
    u8CurChannel = u8BroadcastChannel;
}

/****************************************************************************
 *
 * NAME: u8SlotIsShardOrVergin
 *
 * DESCRIPTION:
 * Read mac address, stored Baud from flash and initial Uart Baud
 *
 * PARAMETERS:
 *                    column:
 *                    pu16Index: record 15s card slot index,only 15s card used
 * RETURNS:
 * void
 *
 ****************************************************************************/
uint8 u8SlotIsShardOrVergin(uint16 column)
{
    uint8 j,k;
    uint8  sum_1s = 0;
    uint8  sum_5S = 0;
    uint8  ret = 0;
    uint16 u16Index;

    if(tsSlot[column].b2SlotType != SLOT_CARD)
        return ret;

    for(j=0; j<5; j++)
    {
        sum_5S = 0;
        for(k=0;k<3;k++)
        {
            u16Index =column + (TOF_SLOT_LOC_PERIOD*j)+ (250*k);

            if(tsSlot[u16Index].b1Used)
            {
                sum_1s++;
                sum_5S++;
            }
        }

        if(sum_5S == 0)    //有5秒卡空位:即5秒卡跟15秒卡都有
        {
            ret |= 0x02;
        }
        else if(sum_5S < 3)    //有15秒卡空位
        {
            ret |= 0x01;
        }
    }

    if(sum_1s == 0)           //有1秒卡空位:即3种卡都有空位
    {
        ret |= 0x04;
    }
    return ret;
}

uint8 u8SlotIs15ShardOrVergin(uint16 column,uint16* pu16Index)
{
    uint8 j,k;
    uint8  sum_1s = 0;
    uint8  sum_5S = 0;
    uint8  ret = 0;
    uint16 u16Index;
    uint16 u16ShareIndex = 0;

    if(tsSlot[column].b2SlotType != SLOT_CARD)
        return ret;

    for(j=0; j<5; j++)
    {
        sum_5S = 0;
        for(k=0;k<3;k++)
        {
            u16Index =column + (TOF_SLOT_LOC_PERIOD*j)+ (250*k);

            if(tsSlot[u16Index].b1Used)
            {
                sum_1s++;
                sum_5S++;
            }
            else
            {
                u16ShareIndex = u16Index;
            }
        }

        if(sum_5S == 0)    //有5秒卡空位:即5秒卡跟15秒卡都有
        {
            ret |= 0x02;
        }
        else if(sum_5S < 3)    //有15秒卡空位
        {
            ret |= 0x01;
            *pu16Index = u16ShareIndex;
        }
    }

    if(sum_1s == 0)           //有1秒卡空位:即3种卡都有空位
    {
        ret |= 0x04;
    }
    return ret;
}



uint8 u8SlotIs5ShardOrVergin(uint16 column)
{
    uint8 j,k;
    uint8  sum_1s = 0;
    uint8  sum_5S = 0;
    uint8  ret = 0;
    uint16 u16Index;

    if(tsSlot[column].b2SlotType != SLOT_CARD)
        return ret;

    for(j=0; j<5; j++)
    {
        sum_5S = 0;
        for(k=0;k<3;k++)
        {
            u16Index =column + (TOF_SLOT_LOC_PERIOD*j)+ (250*k);

            if(tsSlot[u16Index].b1Used)
            {
                sum_1s++;
                sum_5S++;
            }
        }
        if(sum_5S == 0 && sum_1s > 0)    //有5秒卡空位
        {
            return 0x02;
        }
    }

    if(sum_1s == 0)           //有1秒卡空位
    {
        ret |= 0x04;
    }
    return ret;
}

uint8 u8SlotIs1ShardOrVergin(uint16 column)
{
    uint8 j,k;
    uint16 u16Index;

    if(tsSlot[column].b2SlotType != SLOT_CARD)
        return 0;

    for(j=0; j<5; j++)
    {
        for(k=0;k<3;k++)
        {
            u16Index =column + (TOF_SLOT_LOC_PERIOD*j)+ (250*k);

            if(tsSlot[u16Index].b1Used)
            {
                return 0;
            }
        }
    }
    return 0x04;
}


PRIVATE uint16 u16GetIdleSlot_TOF15s(uint16 u16Start, uint16 u16ReqShortAddr, uint16* pu16Index,uint16 u16CardSlot)
{
    uint16 i,j,k;
    uint16 column_15s = 0;
    uint16 column_5s = 0;
    uint16 column_1s = 0;
	uint8 column_15s_find = 0;
    uint8 column_5s_find = 0;
    uint8 column_1s_find = 0;
    uint8 u8tmp;
    uint16 u16Index_15s;
    uint16 u16Index;
    uint16 u16column;

	//当前列
    uint8 current_column = vStationMod(u16Start,TOF_SLOT_LOC_PERIOD);

    for(i=0; i<TOF_SLOT_LOC_PERIOD; i++)
    {
        // 从当前列的下一列开始遍历
        u16column = vStationMod((current_column + 1 + i),TOF_SLOT_LOC_PERIOD);

        // 寻找当前列的下一个碎片列，如果已经有碎片列了，则不再查找
        if(!column_15s_find)
        {
            u8tmp = u8SlotIs15ShardOrVergin(u16column,&u16Index_15s);
            if(u8tmp & 0x01)     //碎片列
            {
                column_15s = u16Index_15s;
                column_15s_find = 1;
            }
            else if(!column_5s_find && (u8tmp & 0x02)) //5秒完整列
            {
                column_5s = u16column;
                column_5s_find = 1;
            }
            else if(!column_1s_find && (u8tmp & 0x04))    //1秒卡完整列
            {
                column_1s = u16column;
                column_1s_find = 1;
            }
        }

        // 寻找原卡号本身还存在的slot
        for(j=0; j<5; j++)
        {
            for(k=0;k<3;k++)
            {
                u16Index =u16column + (TOF_SLOT_LOC_PERIOD*j)+ (250*k);
                if(tsSlot[u16Index].b1Used == 1
                    &&tsSlot[u16Index].b2SlotType == SLOT_CARD
                    &&tsSlot[u16Index].u8DeviceType == DEVICE_TYPE_CARD15S
                    &&tsSlot[u16Index].u16ShortAddr == u16ReqShortAddr)
                {
                    // 如果原卡号找到了，那么把原卡号所在的slot分配出去
    	            *pu16Index = u16Index;
    	            return 2;
                }
            }
        }
    }
    // 如果有带碎片的列，那么把当前列的后一个带碎片的列较快的那个碎片分配出去。
    if(column_15s_find)
    {
        if((!tsSlot[column_15s].b1Used)
                      && tsSlot[column_15s].b2SlotType == SLOT_CARD)
        {
            *pu16Index = column_15s;
            return 1;
        }
    }
    // 如果没有带碎片的列, 那么就把当前列的后一个完整列的最快的碎片分配出去
    else if(column_5s_find)
    {
        for(j=0; j<5; j++)
        {
            //row = (j + current_row) % THE_MULTIPLE;

            for(k=0;k<3;k++)
            {
                u16Index =column_5s + (TOF_SLOT_LOC_PERIOD*j)+ (250*k);
                if((!tsSlot[u16Index].b1Used)
                      && tsSlot[u16Index].b2SlotType == SLOT_CARD)
                {
                    *pu16Index = u16Index;
                    return 1;
                }
            }
        }
    }
    else if(column_1s_find)
    {
        for(j=0; j<5; j++)
        {
            //row = (j + current_row) % THE_MULTIPLE;

            for(k=0;k<3;k++)
            {
                u16Index =column_1s + (TOF_SLOT_LOC_PERIOD*j)+ (250*k);
                if((!tsSlot[u16Index].b1Used)
                      && tsSlot[u16Index].b2SlotType == SLOT_CARD)
                {
                    *pu16Index = u16Index;
                    return 1;
                }
            }
        }
    }
    // 分配失败
    //else
    {
        pu16Index = NULL;
        return 0;
    }
}

PRIVATE uint16 u16GetIdleSlot_TOF5s(uint16 u16Start, uint16 u16ReqShortAddr, uint16* pu16Index,uint16 u16CardSlot)
{
    uint16 i,j,k;
    uint16 column_5s = 0;
    uint16 column_1s = 0;
    uint8 column_5s_find = 0;
    uint8 column_1s_find = 0;
    uint8 u8tmp;
    uint16 u16Index;
    uint16 u16column;

	//当前列
    uint8 current_column = vStationMod(u16Start+1,TOF_SLOT_LOC_PERIOD);

    for(i=0; i<TOF_SLOT_LOC_PERIOD; i++)
    {
        // 从当前列的下一列开始遍历
        u16column = vStationMod((current_column + 1 + i),TOF_SLOT_LOC_PERIOD);

        // 寻找当前列的下一个碎片列，如果已经有碎片列了，则不再查找
        if(!column_5s_find)
        {
            u8tmp = u8SlotIs5ShardOrVergin(u16column);

            if((u8tmp & 0x02)) //5秒完整列
            {
                column_5s = u16column;
                column_5s_find = 1;
            }
            else if(!column_1s_find && (u8tmp & 0x04))    //1秒卡完整列
            {
                column_1s = u16column;
                column_1s_find = 1;
            }
        }

        // 寻找原卡号本身还存在的slot
        for(k=0;k<5;k++)
        {
            u16Index =u16column + (TOF_SLOT_LOC_PERIOD*k);
            if(tsSlot[u16Index].b1Used == 1
                //&&tsSlot[u16Index].b2SlotType == SLOT_CARD
                &&tsSlot[u16Index].u8DeviceType == DEVICE_TYPE_CARD5S
                &&tsSlot[u16Index].u16ShortAddr == u16ReqShortAddr)
            {
                // 如果原卡号找到了，那么把原卡号所在的slot分配出去
	            *pu16Index = u16Index;
	            return 2;
            }
        }
    }

    // 如果没有带碎片的列, 那么就把当前列的后一个完整列的最快的碎片分配出去

    bool findFlag = TRUE;
    if(column_5s_find)
    {
        for(j=0; j<5; j++)
        {
            findFlag = TRUE;
            for(k=0;k<3;k++)
            {
                u16Index =column_5s + (TOF_SLOT_LOC_PERIOD*j)+ (250*k);
                if(tsSlot[u16Index].b1Used
                      || tsSlot[u16Index].b2SlotType != SLOT_CARD)
                {
                    findFlag = FALSE;
                    break;
                }
            }
            if(findFlag)
            {
                *pu16Index = u16Index;
                return 1;
            }
        }
    }
    else if(column_1s_find)
    {
        for(j=0; j<5; j++)
        {
            findFlag = TRUE;
            for(k=0;k<3;k++)
            {
                u16Index =column_1s + (TOF_SLOT_LOC_PERIOD*j)+ (250*k);
                if((tsSlot[u16Index].b1Used)
                      || tsSlot[u16Index].b2SlotType != SLOT_CARD)
                {
                    findFlag = FALSE;
                    break;
                }
            }
            if(findFlag)
            {
                *pu16Index = u16Index;
                return 1;
            }
        }
    }
    // 分配失败
    //else
    {
        pu16Index = NULL;
        return 0;
    }
}


PRIVATE uint16 u16GetIdleSlot_TOF1s(uint16 u16Start, uint16 u16ReqShortAddr, uint16* pu16Index,uint16 u16CardSlot)
{
    uint16 i;
    uint16 column_1s = 0;
    uint8 column_1s_find = 0;
    uint8 u8tmp;
    uint16 u16column;

    //  当前行，当前列
    uint8 current_column = vStationMod(u16Start+1,TOF_SLOT_LOC_PERIOD);

    for(i=0; i<TOF_SLOT_LOC_PERIOD; i++)
    {
        // 从当前列的下一列开始遍历
        u16column = vStationMod((current_column + 1 + i),TOF_SLOT_LOC_PERIOD);

        // 寻找当前列的下一个碎片列，如果已经有碎片列了，则不再查找
        if(!column_1s_find)
        {
            u8tmp = u8SlotIs1ShardOrVergin(u16column);

            if(u8tmp & 0x04)    //1秒卡完整列
            {
                column_1s = u16column;
                column_1s_find = 1;
            }
        }

        // 寻找原卡号本身还存在的slot
        if(tsSlot[u16column].b1Used == 1
            //&&tsSlot[u16column].b2SlotType == SLOT_CARD
            &&tsSlot[u16column].u8DeviceType == DEVICE_TYPE_CARD1S
            &&tsSlot[u16column].u16ShortAddr == u16ReqShortAddr)
        {
            // 如果原卡号找到了，那么把原卡号所在的slot分配出去
            *pu16Index = u16column;
            return 2;
        }
    }

    // 如果没有带碎片的列, 那么就把当前列的后一个完整列的最快的碎片分配出去

    if(column_1s_find)
    {
        *pu16Index = column_1s;
        return 1;
    }
    // 分配失败
    //else
    {
        pu16Index = NULL;
        return 0;
    }
}

void bAllocCardSlot(uint8 device_type,uint16 u16PanID,uint16 u16ShortAddr,uint16 u16AllocIndex,uint16 u16SeqNum)
{
    uint16 index1;
    uint8 i,j;
    switch (device_type)
    {
    case CARD_STATUS_PERIOD_5S:
    {
        for(i=0;i<3;i++)
        {
            index1 = vStationMod((u16AllocIndex + i*250),750);
            tsSlot[index1].b1Used   = 1;
            tsSlot[index1].u8LostNum    = 0;
            tsSlot[index1].u8DeviceType = DEVICE_TYPE_CARD5S;
            tsSlot[index1].u16SeqNum    = u16SeqNum;
            tsSlot[index1].u16PanID = u16PanID;
            tsSlot[index1].u16ShortAddr     = u16ShortAddr;
            tsSlot[index1].tsTof   = INVALID_TOF_DISTANCE;    // init current tof value
            tsSlot[index1].u16LocCardDistance[0] = INVALID_TOF_DISTANCE;//init current LC for 1#
            tsSlot[index1].u16LocCardDistance[1] = INVALID_TOF_DISTANCE;//init current LC for 2#
            tsSlot[index1].i8Rssi = INVALID_RSSI;
            tsSlot[index1].i8LocRssi[0] = INVALID_RSSI;
            tsSlot[index1].i8LocRssi[1] = INVALID_RSSI;
            tsSlot[index1].u8AppHelpAck    = 0;            // init the app's help ack
        }
        tsAccepted[u8AcceptNum].u16ShortAddr = u16ShortAddr;
        tsAccepted[u8AcceptNum].u16SlotIndex = u16AllocIndex;
        u8AcceptNum++;
        break;
    }

    case CARD_STATUS_PERIOD_15S:
    {

        tsSlot[u16AllocIndex].b1Used    = 1;
        tsSlot[u16AllocIndex].u8LostNum     = 0;
        tsSlot[u16AllocIndex].u8DeviceType = DEVICE_TYPE_CARD15S;
        tsSlot[u16AllocIndex].u16SeqNum = u16SeqNum;
        tsSlot[u16AllocIndex].u16PanID  = u16PanID;
        tsSlot[u16AllocIndex].u16ShortAddr     = u16ShortAddr;
        tsSlot[u16AllocIndex].tsTof   = INVALID_TOF_DISTANCE;    // init current tof value
        tsSlot[u16AllocIndex].u16LocCardDistance[0] = INVALID_TOF_DISTANCE;//init current LC for 1#
        tsSlot[u16AllocIndex].u16LocCardDistance[1] = INVALID_TOF_DISTANCE;//init current LC for 2#
        tsSlot[u16AllocIndex].i8Rssi = INVALID_RSSI;
        tsSlot[u16AllocIndex].i8LocRssi[0] = INVALID_RSSI;
        tsSlot[u16AllocIndex].i8LocRssi[1] = INVALID_RSSI;
        tsSlot[u16AllocIndex].u8AppHelpAck    = 0;            // init the app's help ack

        tsAccepted[u8AcceptNum].u16ShortAddr = u16ShortAddr;
        tsAccepted[u8AcceptNum].u16SlotIndex = u16AllocIndex;
        u8AcceptNum++;
        break;
    }

    case CARD_STATUS_PERIOD_1S:
    {

        for(i=0;i<5;i++)
        {
            for(j=0;j<3;j++)
            {
                index1 =u16AllocIndex + (TOF_SLOT_LOC_PERIOD*i)+ (250*j);
                tsSlot[index1].b1Used   = 1;
                tsSlot[index1].u8LostNum    = 0;
                tsSlot[index1].u8DeviceType = DEVICE_TYPE_CARD1S;
                tsSlot[index1].u16SeqNum    = u16SeqNum;
                tsSlot[index1].u16PanID = u16PanID;
                tsSlot[index1].u16ShortAddr     = u16ShortAddr;
                tsSlot[index1].tsTof   = INVALID_TOF_DISTANCE;    // init current tof value
                tsSlot[index1].u16LocCardDistance[0] = INVALID_TOF_DISTANCE;//init current LC for 1#
                tsSlot[index1].u16LocCardDistance[1] = INVALID_TOF_DISTANCE;//init current LC for 2#
                tsSlot[index1].i8Rssi = INVALID_RSSI;
                tsSlot[index1].i8LocRssi[0] = INVALID_RSSI;
                tsSlot[index1].i8LocRssi[1] = INVALID_RSSI;
                tsSlot[index1].u8AppHelpAck    = 0;            // init the app's help ack
            }
        }
        tsAccepted[u8AcceptNum].u16ShortAddr = u16ShortAddr;
        tsAccepted[u8AcceptNum].u16SlotIndex = index1;
        u8AcceptNum++;
        break;
    }
    default:
        break;
    }

}


// 空闲 slot分配
// 更新 u8AvailSlotType,
// 输入 u16CurrSlotIdx的原因是要尽量分配里当前帧最近的空闲帧
void vUpdateAvailSlot(uint16 u16CurrSlotIdx)
{
    uint8 u8tmp,i;
    uint8 card_1s_num = 0;
    uint8 card_5s_num = 0;
    uint8 card_15s_num = 0;
    //uint8 vergin_column_num = 0;

    u8AvailSlotType = 0;

    // loc slot 更新
    if(!(tsLocator[0].bUsed && tsLocator[1].bUsed))
    {
        u8AvailSlotType |= HAVE_IDLE_LOC_SLOT;
    }

    // card slot 更新
    for(i=2; i<TOF_SLOT_LOC_PERIOD; i++)
    {
        u8tmp = u8SlotIsShardOrVergin(i);
        if(u8tmp & 0x04)//1秒卡
        {
            card_1s_num++;
        }
        else if(u8tmp & 0x02)//5秒卡
        {
            card_5s_num++;
        }
        else if(u8tmp & 0x01)                //15秒卡
        {
            card_15s_num++;
        }

        if(card_1s_num)
            break;
    }

    if(card_1s_num)
    {
        u8AvailSlotType |= HAVE_IDLE_ONES_SLOT | HAVE_IDLE_FIVES_SLOT | HAVE_IDLE_FIFTEEN_SLOT;
    }
    else if(card_5s_num)
    {
        u8AvailSlotType |= HAVE_IDLE_FIVES_SLOT | HAVE_IDLE_FIFTEEN_SLOT;
    }
    else if(card_15s_num)
    {
        u8AvailSlotType |= HAVE_IDLE_FIFTEEN_SLOT;
    }
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
    uint8 u8TmpUartBaud = ((psMacAddr.u32L) >> 24) & 0xFF;
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
    Jbsmac_u8Init(vBsMac_rx_callback, 2, u8BsmacUartBaud, u16StationPanId, BSMAC_DEVICE_TYPE_LOC);
}

PRIVATE void vDioCallBack(uint32 u32Device, uint32 u32ItemBitmap)
{
    if(u32Device == E_AHI_DEVICE_SYSCTRL)
    {
        if(u32ItemBitmap & CTRL_ADDR_SYNC_0)
        {
            vLocatorAddrSync(0);
        }
        if(u32ItemBitmap & CTRL_ADDR_SYNC_1)
        {
            vLocatorAddrSync(1);
        }
    }
}

// 辅站地址同步
PRIVATE void vLocatorAddrSync(uint8 u8LocIdx)
{
    static uint8  u8iCount[2];
    static uint16 u16HeadMs;
    static uint16 u16TailMs[2];
    static uint16 u16LocatorAddrTemp[2];

    uint16 CycleLenth;

    sys_assert(u8LocIdx==0 || u8LocIdx==1);

    u16HeadMs = (uint16)(u32AHI_TickTimerRead()/16000);

    if(u8iCount[u8LocIdx] == 0)
    {
        u8iCount[u8LocIdx]++;
        u16TailMs[u8LocIdx] = u16HeadMs;
        u16LocatorAddrTemp[u8LocIdx]=0;
        CycleLenth = 0;
    }
    else
    {
        CycleLenth = u16HeadMs - u16TailMs[u8LocIdx];
    }

    if((LOC_ADD_CLEAR_PERIOD+10) < CycleLenth)
    {
        u8iCount[u8LocIdx] = 1;
        u16TailMs[u8LocIdx] = u16HeadMs;
        u16LocatorAddrTemp[u8LocIdx]=0;
    }

    else if((LOC_ADD_SET_PERIOD-10) <= CycleLenth && CycleLenth <= (LOC_ADD_SET_PERIOD+10))
    {
        u16LocatorAddrTemp[u8LocIdx] |= (0x01<<(u8iCount[u8LocIdx]-1));
        u16TailMs[u8LocIdx] = u16HeadMs;
        u8iCount[u8LocIdx]++;
    }
    else if((LOC_ADD_CLEAR_PERIOD-10) <= CycleLenth && CycleLenth <= (LOC_ADD_CLEAR_PERIOD+10))
    {
        u16LocatorAddrTemp[u8LocIdx] &= ~(0x01<<(u8iCount[u8LocIdx]-1));
        u16TailMs[u8LocIdx] = u16HeadMs;
        u8iCount[u8LocIdx]++;
    }

    if(u8iCount[u8LocIdx] >= 17)
    {
        if(tsLocator[u8LocIdx].bUsed == 0)
        {
            tsLocator[u8LocIdx].u16SyncAddress = u16LocatorAddrTemp[u8LocIdx];

            vSetStationChannel(u8BroadcastChannel);
            vStationCast(TOF_STATION_LOC_INFO,u16LocatorAddrTemp[u8LocIdx],TOF_LOCNODE_NWK_ADDR, MAC_TX_OPTION_ACK, 0);
        }

        if(tsLocator[u8LocIdx].u16SyncAddress != u16LocatorAddrTemp[u8LocIdx])
        {
            tsLocator[u8LocIdx].bUsed = 0;
        }

        u8iCount[u8LocIdx] = 0;
    }
}

void vProcessCardVersion(uint16 u16DevId, uint8 u8DevType, uint16 u16OadVersion, uint16 u16Battery)
{
    uint8 i;
    bool bfind = FALSE;

    if(u16DevId==0 || u16OadVersion==0 || u16Battery==0)
    {
        EDBG(i2c_vPrintf("version error: %d, %d, %d\n",u16DevId,u16OadVersion,u16Battery););
        return;
    }
    /* if already exist */
    for(i=0; i<APP_TOF_VERSION_MAX_NUM; i++)
    {
        if(tsVersionReport[i].devid!=0 && tsVersionReport[i].devid == u16DevId)
        {
            tsVersionReport[i].devid = u16DevId;
            tsVersionReport[i].devtype = u8DevType;
            tsVersionReport[i].oad_ver = u16OadVersion;
            tsVersionReport[i].battery = u16Battery;
            bfind = TRUE;
            break;
        }
    }

    /* else fill a new position */
    if(!bfind)
    {
        for(i=0; i<APP_TOF_VERSION_MAX_NUM; i++)
        {
            if(tsVersionReport[i].devid ==0)
            {
                tsVersionReport[i].devid = u16DevId;
                tsVersionReport[i].devtype = u8DevType;
                tsVersionReport[i].oad_ver = u16OadVersion;
                tsVersionReport[i].battery = u16Battery;
                tsVersionReportLen++;
                bfind = TRUE;
                break;
            }
        }
    }

    if(tsVersionReportLen >= APP_TOF_VERSION_MAX_NUM)
    {
        TimerUtil_eSetTimer(STATION_REPORT_CARDVERSION, 1);
    }
}

PRIVATE void vResetReportRssi(uint8 u8Loc)
{
    rssiReport[u8Loc].app_rssi_head.protocoltype =0;
    rssiReport[u8Loc].app_rssi_head.msgtype = 0;
    rssiReport[u8Loc].app_rssi_head.len = 0;
    rssiReport[u8Loc].u16ShortAddr = 0;
    rssiReport[u8Loc].receiveRssi = INVALID_RSSI;
    rssiReport[u8Loc].sentRssi = INVALID_RSSI;
}

PRIVATE void vUpdateStationRssi(uint8 u8Loc, uint16 u16Short,int8 i8RecvRssi,int8 i8SendRssi)
{
    rssiReport[u8Loc].app_rssi_head.protocoltype = APP_PROTOCOL_TYPE_STATION;
    rssiReport[u8Loc].app_rssi_head.msgtype = APP_TOF_MSG_RSSI_TOF_STATION;
    rssiReport[u8Loc].app_rssi_head.len = sizeof(app_rssi_report)-sizeof(app_header_t);
    rssiReport[u8Loc].u16ShortAddr =  u16Short;
    rssiReport[u8Loc].receiveRssi  =  i8RecvRssi;
    rssiReport[u8Loc].sentRssi=  i8SendRssi;

    SysUtil_vConvertEndian(&rssiReport[u8Loc].u16ShortAddr, sizeof(rssiReport[u8Loc].u16ShortAddr));
}

// 定位卡时隙处理
PRIVATE void vSlotProcessCard(uint16 u16SlotIndex)
{
    if(tsSlot[u16SlotIndex].b1Used)
    {
        // 标准时隙
        uint16 u16BaseIndex;
        u16BaseIndex = u16GetCardBaseIndex(u16SlotIndex, tsSlot[u16SlotIndex].u8DeviceType);

        // 更新seqnum
        tsSlot[u16BaseIndex].u16SeqNum++;

        // 清除当前卡的定位距离和rssi
        tsSlot[u16SlotIndex].i8Rssi= INVALID_RSSI;
        tsSlot[u16SlotIndex].tsTof = INVALID_TOF_DISTANCE;
        tsSlot[u16SlotIndex].u16LocCardDistance[0] = INVALID_TOF_DISTANCE;
        tsSlot[u16SlotIndex].u16LocCardDistance[1] = INVALID_TOF_DISTANCE;
        tsSlot[u16SlotIndex].u16SeqNum =  tsSlot[u16BaseIndex].u16SeqNum;

        uint32 u32CurTick = u32AHI_TickTimerRead();
        uint16 u16RunMs = (uint16)((u32CurTick - u32SlotTick) /16000);

        // tof 大概需要13ms，如果时间不够则放弃
        if ((TOF_SLOT_APPR_MS_2 > TOF_SLOT_MS - u16RunMs) && (tsSlot[u16NextSlotIndex(u16SlotIndex)].b1Used))
        {
            // no time enough left to finish this tof, and the next slot is occupied, should skip this tof, and let the card know the tof is finished
            // maybe last tof is doing now, so don't call vCurrentTofDone()
            vStopTofIndex(u16SlotIndex);
            EventUtil_vSetEvent(STATION_CLAIM_EVENT);
        }
        else
        {
            // start tof
            teStaionState = E_STATE_TOF;
            u16TofSlotIndex = u16SlotIndex;
            u32TofSlotTick = u32SlotTick;
            u16TofPendingAddr = tsSlot[u16SlotIndex].u16ShortAddr;

            EventUtil_vSetEvent(STATION_TOF2CARD_EVENT);
        }
    }
    else
    {
        if(((teStaionState != E_STATE_AVAILABLE) && (teStaionState != E_STATE_RSSI))     // to avoid send available or RSSI too close
                            && (bSendCardInfoDone))                // this is for the locator slot #2
        EventUtil_vSetEvent(STATION_CLAIM_EVENT);
    }
    return;
}


// 瓦检仪时隙处理
PRIVATE void vSlotProcessGas(uint16 u16SlotIndex)
{
}

//定位辅站时隙处理
PRIVATE void vSlotProcessLocator(uint16 u16SlotIndex)
{
    teStaionState = E_STATE_LOCATOR;
    sys_assert(u16SlotIndex % TOF_SLOT_LOC_INT < 2);

// 如果辅站均已入网，且 定位完毕，则两个时隙都用于信息交互
// 如果辅站尚未完全入网， 则时隙0 用于信息交互， 时隙1 用于 辅站入网

    if(u16SlotIndex % TOF_SLOT_LOC_INT == 0)
    {
        // 为调试，不管有无辅站都发出来
        //if(u8LocDist[0]>0 || u8LocDist[1]>0)
        if(tsLocator[0].bUsed || tsLocator[1].bUsed)
        {
            u8ReSendCardInfo = 0;
            bSendCardInfoDone = FALSE;
            bRxCardDistanceDone[0] = FALSE;
            bRxCardDistanceDone[1] = FALSE;
            EventUtil_vSetEvent(STATION_SEND_CARD_INFO_EVENT);
        }
        else
        {
            EventUtil_vSetEvent(STATION_CLAIM_EVENT);
        }
    }
    else   	// 第二个时隙给辅站入网，以及TOF
    {
        // 每 15秒 做辅站tof, 其余时隙用来做 claim
        if(u16SlotIndex == 1 && tsLocator[0].bUsed)
        {
            vLocTof(0);
            tsLocator[0].u16SeqNum++;
        }
        else if(u16SlotIndex == 26 && tsLocator[1].bUsed)
        {
            vLocTof(1);
            tsLocator[1].u16SeqNum++;
        }

        //每10分钟request一次辅站版本
        else if((tsLocator[0].u16LocVerCnt++ > 1200) && tsLocator[0].bUsed)
        {
            tsLocator[0].u16LocVerCnt = 0;
            vSetStationChannel(u8TofChannel);
            vStationCast(TOF_LOC_VERSION_INFO,tsLocator[0].u16ShortAddr,tsLocator[0].u16PanID,0,0);
        }
        else if((tsLocator[1].u16LocVerCnt++ > 1200) && tsLocator[1].bUsed)
        {
            tsLocator[1].u16LocVerCnt = 0;
            vSetStationChannel(u8TofChannel);
            vStationCast(TOF_LOC_VERSION_INFO,tsLocator[1].u16ShortAddr,tsLocator[1].u16PanID,0,0);
        }
        else
        {
            EventUtil_vSetEvent(STATION_CLAIM_EVENT);
        }
    }
}

//IDLE Slot 处理
PRIVATE void vSlotProcessIdle(uint16 u16SlotIndex)
{
    EventUtil_vSetEvent(STATION_CLAIM_EVENT);
}

// 获取定位卡的"基准时隙"
// 定位卡的 Seqnum, LostNum等都保存在基准时隙中
// 1秒卡为 0-49
// 5秒卡为 0-249
// 15秒卡为 0-749
PRIVATE uint16 u16GetCardBaseIndex(uint16 u16SlotIndex, tof_device_type_te u8DeviceType)
{
    uint16  u16BaseIndex = u16SlotIndex;

    if(u8DeviceType == DEVICE_TYPE_CARD1S)
    {
        u16BaseIndex =  u16SlotIndex % TOF_SLOT_LOC_PERIOD;
    }
    else if(u8DeviceType == DEVICE_TYPE_CARD5S)
    {
        u16BaseIndex =  u16SlotIndex % (TOF_SLOT_LOC_PERIOD*5);
    }
    else if(u8DeviceType == DEVICE_TYPE_CARD15S)
    {
        u16BaseIndex =  u16SlotIndex;
    }
    else
    {
        EDBG(i2c_vPrintf("u8DeviceType error! %x\n", u8DeviceType););
    }

    return u16BaseIndex;
}

// 释放slot
void vReleaseSlot(uint16 u16Index)
{
    // 不释放 slottype, seqnum
    tsSlot[u16Index].b1Used = 0;
    tsSlot[u16Index].u8LostNum = 0;
    tsSlot[u16Index].u16ShortAddr = 0;
    tsSlot[u16Index].u16PanID = 0;
    tsSlot[u16Index].tsTof= INVALID_TOF_DISTANCE;
    tsSlot[u16Index].i8Rssi = INVALID_RSSI;
    tsSlot[u16Index].u16LocCardDistance[0] = INVALID_TOF_DISTANCE;
    tsSlot[u16Index].u16LocCardDistance[1] = INVALID_TOF_DISTANCE;
    tsSlot[u16Index].i8LocRssi[0] = INVALID_RSSI;
    tsSlot[u16Index].i8LocRssi[1] = INVALID_RSSI;
    tsSlot[u16Index].u8AppHelpAck = 0;
}


void vReportLive(void)
{
    struct nwkhdr *pNwkHdr = (struct nwkhdr *)u32CommonBuf;
    app_header_t *pHeader = (app_header_t *)(pNwkHdr + 1);
    app_LSrfReport_t *pStationReport = (app_LSrfReport_t *)(pHeader + 1);

    pNwkHdr->type = NWK_DATA;
    pNwkHdr->ttl = 1;

    pNwkHdr->src = u16StationPanId;
    SysUtil_vConvertEndian(&pNwkHdr->src, sizeof(pNwkHdr->src));

    pNwkHdr->dst = u16ArmId;
    SysUtil_vConvertEndian(&pNwkHdr->dst, sizeof(pNwkHdr->dst));

    pNwkHdr->len =  sizeof(app_header_t) + sizeof(app_LSrfReport_t);
    SysUtil_vConvertEndian(&pNwkHdr->len, sizeof(pNwkHdr->len));

    pHeader->len = sizeof(app_LSrfReport_t);
    SysUtil_vConvertEndian(&pHeader->len, sizeof(pHeader->len));

    pHeader->msgtype = APP_TOF_MSG_REPORT;
    pHeader->protocoltype = APP_PROTOCOL_TYPE_CARD;

    pStationReport->hdr.dstaddr = 0;
    SysUtil_vConvertEndian(&pStationReport->hdr.dstaddr, sizeof(pStationReport->hdr.dstaddr));
    pStationReport->hdr.srcaddr = u16StationShortAddr;
    SysUtil_vConvertEndian(&pStationReport->hdr.srcaddr, sizeof(pStationReport->hdr.srcaddr));
    pStationReport->len = 0;
    SysUtil_vConvertEndian(&pStationReport->len, sizeof(pStationReport->len));

    pStationReport->reporttype = APP_LS_REPORT_LIVE;
    pStationReport->devtype = BSMAC_DEVICE_TYPE_LOC;
    pStationReport->seqnum = u8LiveSeqNum++;

    Jbsmac_eWriteData((uint8 *)u32CommonBuf, sizeof(struct nwkhdr) + sizeof(app_header_t) + sizeof(app_LSrfReport_t));
    DBG(
        i2c_vPrintf("live %d %d\n", u8LiveSeqNum, u8LastLiveAckSeq);
    );
}

void vReportStatus(void)
{
    if(bIsStationUp)
    {
        struct nwkhdr *pNwkHdr = (struct nwkhdr *)u32CommonBuf;
        app_header_t *pHeader = (app_header_t *)(pNwkHdr + 1);
        app_LSrfReport_t *pStationReport = (app_LSrfReport_t *)(pHeader + 1);
        app_LSLocstatus_t* pStatus = (app_LSLocstatus_t*)(pStationReport + 1);

        uint8 u8VersionLen = u8StaLogVerLen + strlen(LOCATOR0_LOG) + strlen(tsLocator[0].u8LocVer)
                              + strlen(LOCATOR1_LOG) + strlen(tsLocator[1].u8LocVer) + 1;
        if(sizeof(struct nwkhdr) + sizeof(app_header_t) + sizeof(app_LSrfReport_t)+sizeof(app_LSLocstatus_t)+u8VersionLen > 128)
        {
            EDBG(i2c_vPrintf("RPT len err!\n"););
            return;
        }

        /* network header */
        pNwkHdr->type = NWK_DATA;
        pNwkHdr->ttl = 1;

        pNwkHdr->src = u16StationPanId;
        SysUtil_vConvertEndian(&pNwkHdr->src, sizeof(pNwkHdr->src));

        pNwkHdr->dst = u16ArmId;
        SysUtil_vConvertEndian(&pNwkHdr->dst, sizeof(pNwkHdr->dst));

        pNwkHdr->len =  sizeof(app_header_t) + sizeof(app_LSrfReport_t)+sizeof(app_LSLocstatus_t) + u8VersionLen;
        SysUtil_vConvertEndian(&pNwkHdr->len, sizeof(pNwkHdr->len));

        /* app header */
        pHeader->len = sizeof(app_LSrfReport_t)+sizeof(app_LSLocstatus_t)+u8VersionLen;
        SysUtil_vConvertEndian(&pHeader->len, sizeof(pHeader->len));

        pHeader->msgtype = APP_TOF_MSG_REPORT;
        pHeader->protocoltype = APP_PROTOCOL_TYPE_CARD;

        /* rf report */
        pStationReport->hdr.dstaddr = u16ArmId;
        SysUtil_vConvertEndian(&pStationReport->hdr.dstaddr, sizeof(pStationReport->hdr.dstaddr));
        pStationReport->hdr.srcaddr = u16StationPanId;
        SysUtil_vConvertEndian(&pStationReport->hdr.srcaddr, sizeof(pStationReport->hdr.srcaddr));

        pStationReport->len = sizeof(app_LSLocstatus_t) + u8VersionLen;
        SysUtil_vConvertEndian(&pStationReport->len, sizeof(pStationReport->len));

        pStationReport->reporttype = APP_LS_REPORT_STATUS_LOC;
        pStationReport->devtype = BSMAC_DEVICE_TYPE_LOC;
        pStationReport->seqnum = u8ReportStatusNum++;

        /* Loc status  */
        pStatus->station_type = APP_LS_TOF_STATION;
        pStatus->tof_channel= u8TofChannel;
        pStatus->loc_distance[0] = tsLocator[0].u16TofDist;
        pStatus->loc_distance[1] = tsLocator[1].u16TofDist;
        pStatus->loc_id[0] = tsLocator[0].u16ShortAddr;
        SysUtil_vConvertEndian(&pStatus->loc_id[0], sizeof(pStatus->loc_id[0]));

        pStatus->loc_id[1] = tsLocator[1].u16ShortAddr;
        SysUtil_vConvertEndian(&pStatus->loc_id[1], sizeof(pStatus->loc_id[1]));

        pStatus->loc_channel = u8LocatorCardChannel;
        pStatus->comm_channel = u8BroadcastChannel;

        pStatus->port[0].neighborid = Jbsmac_u16GetPeerAddr(0);
        pStatus->port[0].livestat = Jbsmac_u8GetLinkStatus(0);
        Jbsmac_vGetErrCnt(0, &pStatus->port[0].total_cnt,&pStatus->port[0].lost_cnt);

        pStatus->port[1].neighborid = Jbsmac_u16GetPeerAddr(1);
        pStatus->port[1].livestat = Jbsmac_u8GetLinkStatus(1);
        Jbsmac_vGetErrCnt(1, &pStatus->port[1].total_cnt , &pStatus->port[1].lost_cnt);

        EDBG(
            i2c_vPrintf("Sts rpt\n");
            i2c_vPrintf("P0 %d %d %d %d\n",pStatus->port[0].neighborid, pStatus->port[0].livestat, pStatus->port[0].total_cnt , pStatus->port[0].lost_cnt);
            i2c_vPrintf("P1 %d %d %d %d\n",pStatus->port[1].neighborid, pStatus->port[1].livestat, pStatus->port[1].total_cnt , pStatus->port[1].lost_cnt);
            //i2c_vPrintMem((uint8 *)u32CommonBuf, sizeof(struct nwkhdr) + sizeof(app_header_t) + sizeof(app_LSrfReport_t)+sizeof(app_LSComstatus_t));
        );

        EDBG(
            uint32 err_rate[2];
            err_rate[0] = (uint32)pStatus->port[0].lost_cnt*1000/pStatus->port[0].total_cnt;
            err_rate[1] = (uint32)pStatus->port[1].lost_cnt*1000/pStatus->port[1].total_cnt;
            i2c_vPrintf("ErrRate P0: %d.%d%%  P1 %d.%d%%\n",
                        err_rate[0]/10, err_rate[0]-err_rate[0]/10, err_rate[1]/10, err_rate[1]-err_rate[1]/10);
        );
        SysUtil_vConvertEndian(&pStatus->port[0].neighborid, sizeof(pStatus->port[0].neighborid));
        SysUtil_vConvertEndian(&pStatus->port[0].livestat, sizeof(pStatus->port[0].livestat));
        SysUtil_vConvertEndian(&pStatus->port[0].total_cnt, sizeof(pStatus->port[0].total_cnt));
        SysUtil_vConvertEndian(&pStatus->port[0].lost_cnt, sizeof(pStatus->port[0].lost_cnt));
        SysUtil_vConvertEndian(&pStatus->port[1].neighborid, sizeof(pStatus->port[1].neighborid));
        SysUtil_vConvertEndian(&pStatus->port[1].livestat, sizeof(pStatus->port[1].livestat));
        SysUtil_vConvertEndian(&pStatus->port[1].total_cnt, sizeof(pStatus->port[1].total_cnt));
        SysUtil_vConvertEndian(&pStatus->port[1].lost_cnt, sizeof(pStatus->port[1].lost_cnt));

        //pStatus->loc_oad_version = u16LocOADVersion;
        // fix me:test
        if(tsLocator[0].bUsed)
        {
            pStatus->loc_oad_version = tsLocator[0].u16LocOADVersion;
        }
        else
        {
            pStatus->loc_oad_version = tsLocator[1].u16LocOADVersion;
        }
        pStatus->oad_version = OAD_LOC_STATION_VERSION;
        pStatus->len = u8VersionLen;

        SysUtil_vConvertEndian(&pStatus->loc_oad_version, sizeof(pStatus->loc_oad_version));
        SysUtil_vConvertEndian(&pStatus->oad_version, sizeof(pStatus->oad_version));
        SysUtil_vConvertEndian(&pStatus->len, sizeof(pStatus->len));

        uint32 bias;
        //station version
        memcpy((uint8*)(pStatus+1),u8StaLocVersin,u8StaLogVerLen);

        //loc0 version
        bias = u8StaLogVerLen;
        memcpy(((uint8*)(pStatus+1) + bias),LOCATOR0_LOG,strlen(LOCATOR0_LOG));
        memcpy(((uint8*)(pStatus+1) + bias + strlen(LOCATOR0_LOG)),
                      tsLocator[0].u8LocVer,strlen(tsLocator[0].u8LocVer));

        //loc1 version
        bias += strlen(LOCATOR0_LOG) + strlen(tsLocator[0].u8LocVer);
        memcpy(((uint8*)(pStatus+1) + bias),LOCATOR1_LOG,strlen(LOCATOR1_LOG));
        memcpy(((uint8*)(pStatus+1) + bias + strlen(LOCATOR1_LOG)),
                         tsLocator[1].u8LocVer,strlen(tsLocator[1].u8LocVer));


        /*memcpy((uint8*)(pStatus+1),u8StaLocVersin,u8VersionLen - 1);
        memcpy(((uint8*)(pStatus+1))+ u8VersionLen - 1,'0',1);*/
        uint16 u16SendLen = sizeof(struct nwkhdr) + sizeof(app_header_t) + sizeof(app_LSrfReport_t)+sizeof(app_LSLocstatus_t)+u8VersionLen;

        EDBG(
            i2c_vPrintf("Ver %s \n", VERSION);
        );

        Jbsmac_eWriteData((uint8 *)u32CommonBuf, u16SendLen);
        //DBG(
        //    i2c_vPrintf("Send status report\n");
        //    i2c_vPrintMem((uint8 *)u32CommonBuf, u16SendLen);
        // );
    }

}
void vReportCardVersion(void)
{
    if(bIsStationUp && tsVersionReportLen>0 && tsVersionReportLen<=APP_TOF_VERSION_MAX_NUM)
    {
        struct nwkhdr *pNwkHdr = (struct nwkhdr *)u32CommonBuf;
        app_header_t *pHeader = (app_header_t *)(pNwkHdr + 1);
        app_LSrfReport_t *pStationReport = (app_LSrfReport_t *)(pHeader + 1);
        app_LSVersionReport_t *pVersionReport = (app_LSVersionReport_t *)(pStationReport+1);

        /* nwk header */
        pNwkHdr->type = NWK_DATA;
        pNwkHdr->ttl = 1;
        pNwkHdr->src = u16StationPanId;
        SysUtil_vConvertEndian(&pNwkHdr->src, sizeof(pNwkHdr->src));
        pNwkHdr->dst = u16ArmId;
        SysUtil_vConvertEndian(&pNwkHdr->dst, sizeof(pNwkHdr->dst));
        pNwkHdr->len =  sizeof(app_header_t) + sizeof(app_LSrfReport_t)
                        +tsVersionReportLen*sizeof(app_LSVersionReport_t);
        SysUtil_vConvertEndian(&pNwkHdr->len, sizeof(pNwkHdr->len));


        /*app header */
        pHeader->msgtype = APP_TOF_MSG_REPORT;
        pHeader->protocoltype = APP_PROTOCOL_TYPE_CARD;
        pHeader->len = sizeof(app_LSrfReport_t) + tsVersionReportLen*sizeof(app_LSVersionReport_t);
        SysUtil_vConvertEndian(&pHeader->len, sizeof(pHeader->len));

        /* rf report header */
        pStationReport->hdr.dstaddr = u16ArmId;
        SysUtil_vConvertEndian(&pStationReport->hdr.dstaddr, sizeof(pStationReport->hdr.dstaddr));
        pStationReport->hdr.srcaddr = u16StationPanId;
        SysUtil_vConvertEndian(&pStationReport->hdr.srcaddr, sizeof(pStationReport->hdr.srcaddr));

        static uint8 u8ReportNum;
        pStationReport->reporttype = APP_LS_REPORT_VERSION;
        pStationReport->devtype = BSMAC_DEVICE_TYPE_LOC;
        pStationReport->seqnum = u8ReportNum++;

        pStationReport->len = tsVersionReportLen*sizeof(app_LSVersionReport_t);
        SysUtil_vConvertEndian(&pStationReport->len, sizeof(pStationReport->len));

        uint8 i;
        for(i=0; i<tsVersionReportLen; i++)
        {
            SysUtil_vConvertEndian(&tsVersionReport[i].devid, sizeof(tsVersionReport[i].devid));
            SysUtil_vConvertEndian(&tsVersionReport[i].oad_ver, sizeof(tsVersionReport[i].oad_ver));
            SysUtil_vConvertEndian(&tsVersionReport[i].battery, sizeof(tsVersionReport[i].battery));

        }
        memcpy((uint8*)pVersionReport, (uint8*)tsVersionReport, tsVersionReportLen*sizeof(app_LSVersionReport_t));
        Jbsmac_eWriteData((uint8 *)u32CommonBuf,
                          sizeof(struct nwkhdr) + sizeof(app_header_t) + sizeof(app_LSrfReport_t)+tsVersionReportLen*sizeof(app_LSVersionReport_t));
        DBG(
            i2c_vPrintf("Send card version \n");
            i2c_vPrintMem((uint8 *)u32CommonBuf, sizeof(struct nwkhdr) + sizeof(app_header_t) + sizeof(app_LSrfReport_t)+tsVersionReportLen*sizeof(app_LSVersionReport_t));
        );

        /* clear the buffer after send */
        memset((uint8*)tsVersionReport, 0, APP_TOF_VERSION_MAX_NUM*sizeof(app_LSVersionReport_t));
        tsVersionReportLen = 0;

    }
    else
    {
        tsVersionReportLen = 0;
    }

}

void vReportStationRssi(uint8 u8Loc)
{
    if(tsLocator[u8Loc].bUsed && rssiReport[u8Loc].app_rssi_head.msgtype == APP_TOF_MSG_RSSI_TOF_STATION)
    {
        vWriteData2Arm((uint8*)&rssiReport[u8Loc]);
        vResetReportRssi(u8Loc);
    }
}

void vReportReset(void)
{
    if(bIsStationUp)
    {
        static uint8 u8ReportResetCauseSeq;
        struct nwkhdr *pNwkHdr = (struct nwkhdr *)u32CommonBuf;
        app_header_t *pHeader = (app_header_t *)(pNwkHdr + 1);
        app_LSrfReport_t *pStationReport = (app_LSrfReport_t *)(pHeader + 1);


        pNwkHdr->type = NWK_DATA;
        pNwkHdr->ttl = 1;

        pNwkHdr->src = u16StationPanId;
        SysUtil_vConvertEndian(&pNwkHdr->src, sizeof(pNwkHdr->src));

        pNwkHdr->dst = u16ArmId;
        SysUtil_vConvertEndian(&pNwkHdr->dst, sizeof(pNwkHdr->dst));

        pNwkHdr->len = sizeof(app_header_t) + sizeof(app_LSrfReport_t);
        SysUtil_vConvertEndian(&pNwkHdr->len, sizeof(pNwkHdr->len));

        pHeader->len = sizeof(app_LSrfReport_t);
        SysUtil_vConvertEndian(&pHeader->len, sizeof(pHeader->len));

        pHeader->msgtype = APP_TOF_MSG_REPORT;
        pHeader->protocoltype = APP_PROTOCOL_TYPE_CARD;

        pStationReport->hdr.dstaddr = 0;
        SysUtil_vConvertEndian(&pStationReport->hdr.dstaddr, sizeof(pStationReport->hdr.dstaddr));

        pStationReport->hdr.srcaddr = u16StationPanId;
        SysUtil_vConvertEndian(&pStationReport->hdr.srcaddr, sizeof(pStationReport->hdr.srcaddr));

        pStationReport->len = 0;
        //SysUtil_vConvertEndian(&pStationReport->len, sizeof(pStationReport->len));

        pStationReport->reporttype = 2;
        pStationReport->devtype = 3;
        pStationReport->seqnum = u8ReportResetCauseSeq++;

        Jbsmac_eWriteData((uint8 *)u32CommonBuf, sizeof(struct nwkhdr) + sizeof(app_header_t) + sizeof(app_LSrfReport_t));
        //EventUtil_vUnsetEvent(event);
    }
    else
    {
        TimerUtil_eSetTimer(STATION_REPORT_RESET_EVENT, 1000);
    }
}


/****************************************************************************/
/***        END OF FILE                                                   ***/
/****************************************************************************/





