
/****************************************************************************/
/***        Include files                                                 ***/
/****************************************************************************/
#include <jendefs.h>
#include <AppHardwareApi.h>
#include <AppQueueApi.h>
#include <mac_sap.h>
#include <mac_pib.h>
#include <Utilities.h>
#include <OAD.h>

#include "config.h"
#include "app_protocol.h"
#include "numtrans.h"
#include "bsmac.h"
#include "JN5148_util.h"
#include "Hal_Air.h"
#include "string.h"

/****************************************************************************/
/***        Macro Definitions                                             ***/
/****************************************************************************/

#if (defined DEBUG_COM_APP)
#define DBG(x) do{x}while(0)
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
#define LED_RF             E_AHI_DIO16_INT  //DIO 14 is used for i2c,remove it to unused DIO16



//FIX me, DIO14 is used for I2C, Use DIO16 instead
//#define LED_LINK         E_AHI_DIO14_INT

/* app envents */
#define	STATION_PORT_QUERY_EVENT	   BIT(1)
#define STATION_REPORT_STATUS_EVENT    BIT(2)
#define STATION_REPORT_RESET_EVENT    BIT(3)
#define STATION_REPORT_RSSI_EVENT    BIT(4)

#define   STATION_FEED_WATCHDOG_EVENT  BIT(10)

//#define   STATION_AIRSENT_EVENT                 BIT(2)

#define MOBILEPHONE_NWK_ADDR		    0xFFF4
#define AIR_BUF_SIZE					16
#define HI_UINT16(a) (((a) >> 8) & 0xFF)
#define LO_UINT16(a) ((a) & 0xFF)
#define CONVERT_ENDIAN(x)	SysUtil_vConvertEndian(&(x), sizeof(x))
#define MEMCOPY(x, y)	memcpy(&(x), &(y), sizeof(x))

/****************************************************************************/
/***        Type Definitions                                              ***/
/****************************************************************************/
typedef enum
{
    E_STATE_IDLE,
    E_STATE_COORDINATOR_STARTED,
    E_STATE_AVAILABLE,
    E_STATE_RSSI,
    E_STATE_ACCEPT,
    E_STATE_TOF,
    E_STATE_LOCATOR,
    E_STATE_BUSY,
} teState;

typedef struct
{
    uint16 destAddr;
    uint16 msduLen;
    uint8 msdu[128];
} tsAirData_t;

typedef struct
{
    tsAirData_t   tsAirData[AIR_BUF_SIZE];
    uint8   sendIdx;
    uint8   fillIdx;
    bool_t  isSending;
    uint32 sendingTick;
} tsAirDataInfo_t;

/****************************************************************************/
/***        Local Function Prototypes                                     ***/
/****************************************************************************/
PRIVATE void vInitSystem(void);
PRIVATE void vCheckSetting(void);
PRIVATE void vStartingLEDindicator();
PRIVATE void vProcessPoll(void);
PRIVATE void vStartCoordinator(void);
PRIVATE void vProcessSysEventQueues(void);
PRIVATE void vProcessIncomingMlme(MAC_MlmeDcfmInd_s *psMlmeInd);
PRIVATE void vProcessIncomingMcps(MAC_McpsDcfmInd_s *psMcpsInd);
PRIVATE void vProcessIncomingHwEvent(AppQApiHwInd_s *psAHI_Ind);
PRIVATE void vHandleMcpsDataInd(MAC_McpsDcfmInd_s *psMcpsInd);
PRIVATE void vHandleMcpsDataDcfm(MAC_McpsDcfmInd_s *psMcpsInd);
PRIVATE void vProcessAppEventQueues(void);
PRIVATE void vProcessPhoneData(MAC_McpsIndData_s *psMcpsInd);
PRIVATE void vProcessStationCmd(MAC_McpsIndData_s *psMcpsInd);
PRIVATE bool_t bArmReady();
PRIVATE void vBsMac_rx_callback(unsigned char *pbuf, unsigned char len);
PRIVATE void MP_Report_ResetCause(uint8 reporttype,uint8 devtype);
PRIVATE void  vStackCheck(void);
PRIVATE void vSyncLEDStatus(void);
PRIVATE void reset_report_rssi(void);
PRIVATE void update_station_rssi(uint16 u16Short,int8 receivei8Rssi,int8 senti8Rssi,uint8 msgtype);
PRIVATE void vInitialMpStationBaud(void);

bool  vSaveResetType(uint8 u8reportType, uint8 u8TypeInfo);
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
PRIVATE uint8 u8Channel;
PRIVATE uint8 u8ExtAddr[8];

PRIVATE uint16 u16ArmId = 0xFFFF;
PRIVATE teState teStaionState = E_STATE_IDLE;
PRIVATE bool_t bIsStationUp = FALSE;

PRIVATE uint8 u8LiveSeqNum = 0;
PRIVATE uint8 u8LastLiveAckSeq = 0;

PRIVATE uint8 u8RestartCounter=0;

/*use u32 for alignment*/
PRIVATE uint32 u32CommonBuf[128];

PRIVATE uint32 u32LastAirRxTimeMs = 0;
PRIVATE bool_t  bResetFromWarmStart = 0;

PRIVATE app_rssi_report  rssiReport;
#define REPORT_RSSI_TIME   (5*60*1000)

//PRIVATE tsAirDataInfo_t  tsAirDataInfo
#define RESET_PARAM_STORE_ADDR 	0x70000

typedef struct
{
    uint8 u8reportType;
    uint8 u8TypeInfo;
} ResetParam_t;

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
    vInitSystem();
    vStartCoordinator();
    EDBG(i2c_vPrintf("COM station started. \n\r"););

    vCheckSetting();

    //FLASH
    bAHI_FlashInit(E_FL_CHIP_ST_M25P40_A, NULL);

    // if reset from warm start, do not flash LED and send report restart
    if(!bResetFromWarmStart)
    {
       // vStartingLEDindicator();
        TimerUtil_eSetTimer(STATION_REPORT_RESET_EVENT, 3000);
    }
    TimerUtil_eSetTimer(STATION_PORT_QUERY_EVENT, 1000);
    TimerUtil_eSetTimer(STATION_REPORT_STATUS_EVENT, 1000);

    TimerUtil_eSetCircleTimer(STATION_FEED_WATCHDOG_EVENT, 500);

    /* update last air rx time*/
    Hal_TimePoll( );
    u32LastAirRxTimeMs  = Hal_GetTimeMs();

    while (1)
    {
        vProcessPoll();
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
    bResetFromWarmStart = TRUE;
#if (defined DEBUG_COM_APP || defined DEBUG_ERROR)
    i2c_printf_init();
    i2c_vPrintf("warm start. \n\r");
#endif
    AppColdStart();
}

/****************************************************************************/
/***        Local Functions                                               ***/
/****************************************************************************/

PRIVATE void vProcessPoll(void)
{
    Jbsmac_vPoll();
    TimerUtil_vUpdate();
    Hal_TimePoll( );
    Hal_AirPoll();
    vStackCheck();
#if (defined DEBUG_COM_APP || defined DEBUG_ERROR)
    i2c_vPrintPoll();
#endif
}
/****************************************************************************
 *
 * NAME: vInitSystem
 *
 ****************************************************************************/
PRIVATE void vInitSystem(void)
{
    (void)u32AppQApiInit(NULL, NULL, NULL); //vHwDeviceIntCallback);
    (void)u32AHI_Init();

    //Watchdog is default running with 16s timer
    //vAHI_WatchdogStart(12);

    vOADInitClient(TRUE);

#if (defined DEBUG_COM_APP || defined DEBUG_ERROR)
    i2c_printf_init();
#endif

    vAHI_DioSetDirection(0, E_AHI_DIO18_INT);   //init hardware watchdog GPIO
    vAHI_DioSetOutput(E_AHI_DIO18_INT,0);

    LedUtil_bRegister(LED_RF);
    LedUtil_bRegister(LED_UART0);
    LedUtil_bRegister(LED_UART1);

    ErrorUtil_vRegisterLed0(LED_RF);
    ErrorUtil_vRegisterLed1(LED_UART0);
    ErrorUtil_vRegisterLed2(LED_UART1);

    /* if restart from warm start, continue the led status*/
    if(bResetFromWarmStart)
    {
        vSyncLEDStatus();
    }
    /*
    vAHI_ProtocolPower(TRUE);
    vAHI_BbcSetHigherDataRate(E_AHI_BBC_CTRL_DATA_RATE_500_KBPS);
    vAHI_BbcSetInterFrameGap(50);
    */

    // Enable high power modules, tof function, timerUtil
    vAHI_HighPowerModuleEnable(TRUE, TRUE);
    TimerUtil_vInit();

    vAHI_WakeTimerEnable(E_AHI_WAKE_TIMER_0, TRUE);

    /*init address*/
    MacUtil_vReadExtAddress(&psMacAddr);

    uint32 tmp32;
    tmp32 = StringUtil_BCD2uint32(psMacAddr.u32H);


    u16StationShortAddr = 0x0000;	//psMacAddr.u32L & 0xFFFF;
    u16StationPanId = tmp32%100000;

    uint8 channel = ((psMacAddr.u32L) >> 16) & 0xFF;

    if(channel >=11 && channel <=26)
    {
        u8Channel = channel;
    }
    else
    {
        u8Channel = DEFAULT_CHANNEL_MOBILE;
    }

    vAHI_UartSetRTSCTS(E_AHI_UART_1,FALSE);     // uart1:2-wire mode,DIO18 is RTS
    vInitialMpStationBaud();

    if(!bResetFromWarmStart)
    {
        vStartingLEDindicator();
    }

    memcpy(u8ExtAddr, (void *)&psMacAddr.u32L, 4);
    memcpy(u8ExtAddr + 4, (void *)&psMacAddr.u32H, 4);
    SysUtil_vConvertEndian(u8ExtAddr, 4);
    SysUtil_vConvertEndian(u8ExtAddr + 4, 4);

    /* Set up the MAC handles. Must be called AFTER u32AppQApiInit() */
    s_pvMac = pvAppApiGetMacHandle();
    s_psMacPib = MAC_psPibGetHandle(s_pvMac);

    /* Set Pan ID and short address in PIB (also sets match registers in hardware) */
    MAC_vPibSetPanId(s_pvMac, u16StationPanId);
    MAC_vPibSetShortAddr(s_pvMac, u16StationShortAddr);

    /* Enable receiver to be on when idle */
    MAC_vPibSetRxOnWhenIdle(s_pvMac, TRUE, FALSE);

    /* Allow nodes to associate */
    s_psMacPib->bAssociationPermit = 0;
    s_psMacPib->bAutoRequest = 0;
    s_psMacPib->bGtsPermit = FALSE;
    s_psMacPib->u16CoordShortAddr = 0x00;

    /* change csma sets*/
    s_psMacPib->u8MaxFrameRetries = 1;
    MAC_vPibSetMaxCsmaBackoffs(s_pvMac, 2);
    MAC_vPibSetMinBe(s_pvMac, 0);

    /*
    MacUtil_Setting_s sMacUtil;
    sMacUtil.u16SrcPanId 		= u16StationPanId;
    sMacUtil.u16SrcShortAddr 	= u16StationShortAddr;
    sMacUtil.u16Profile_id 		= 0x2001; //for backward compatable
    sMacUtil.u8Dst_endpoint 	= 0x21;
    sMacUtil.u8NodeType 		= 0;
    sMacUtil.u8Src_endpoint 	= 0;
    MacUtil_vInit(&sMacUtil);
    */
    Hal_AirInit (u16StationPanId, u16StationShortAddr);


    // memset(&tsAirDataInfo, 0, sizeof(tsAirDataInfo));
    // tsAirDataInfo.isSending = false;

}


/****************************************************************************
 *
 * NAME: vInitialMpStationBaud
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

PRIVATE void vInitialMpStationBaud(void)
{
    uint8 u8TmpUartBaud = ((psMacAddr.u32L) >> 24) & 0xFF;
    uint8 u8BsmacUartBaud;
    switch(u8TmpUartBaud)
    {
        case 1:      //500K
            u8BsmacUartBaud = BSMAC_UART_BAUD_500k;
        break;

        case 2:       //100K
            u8BsmacUartBaud = BSMAC_UART_BAUD_100k;
        break;

        case 3:       //115200
            u8BsmacUartBaud = BSMAC_UART_BAUD_115200;
        break;

        default:      //460800
            u8BsmacUartBaud = BSMAC_UART_BAUD_460800;
    }

    //initial both uart 0 and 1
    if(bResetFromWarmStart)
        Jbsmac_u8WarmStartInit(vBsMac_rx_callback, 2, u8BsmacUartBaud, u16StationPanId, BSMAC_DEVICE_TYPE_COM);
    else
        Jbsmac_u8Init(vBsMac_rx_callback, 2, u8BsmacUartBaud, u16StationPanId, BSMAC_DEVICE_TYPE_COM);
}


/****************************************************************************
 *
 * NAME: vCheckSetting
 * Check channel and address setting
 *
 ****************************************************************************/
PRIVATE void vCheckSetting(void)
{
    //channels check
    if(u8Channel < 11 || u8Channel > 25
            || u16StationPanId < 20000 || u16StationPanId > 29999
      )
    {
        ErrorUtil_vFatalHalt3(0);
    }
}


/****************************************************************************
 *
 * NAME: vStartingLEDindicator
 * Flash all LEDs at start
 *
 ****************************************************************************/

PRIVATE void vStartingLEDindicator()
{
    LedUtil_vFlashAll(1000,3);
    LedUtil_vFlashAll(3000,1);
}


/****************************************************************************
 *
 * NAME: vProcessSysEventQueues
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
 ****************************************************************************/
PRIVATE void vProcessAppEventQueues(void)
{
    uint32 event = EventUtil_u32ReadEvents();
    uint8 count=0;
    while(event && count++<32)
    {
        switch (event)
        {
        case STATION_FEED_WATCHDOG_EVENT:
        {
            #if (defined SUPPORT_HARD_WATCHDOG)
            vFeedHardwareWatchDog();
            #endif

            vAHI_WatchdogRestart();
            EventUtil_vUnsetEvent(event);
            break;
        }
        case STATION_REPORT_RESET_EVENT:
        {
            if(bIsStationUp)
            {
                ResetParam_t resetParam;
                if(!bAHI_FullFlashRead(RESET_PARAM_STORE_ADDR, sizeof(ResetParam_t),(uint8*)&resetParam))
                {
                    EDBG(i2c_vPrintf("Flash Read Fail\n"););
                }
                DBG(i2c_vPrintf("ResetRpt: %d %d\n",resetParam.u8reportType,resetParam.u8TypeInfo););
                if((resetParam.u8reportType==APP_LS_REPORT_WATCHDOG_RESTART)||(resetParam.u8reportType==APP_LS_REPORT_POWERON))
                {
                    MP_Report_ResetCause(resetParam.u8reportType,resetParam.u8TypeInfo);
                }
                vSaveResetType(APP_LS_REPORT_POWERON,0);
                EventUtil_vUnsetEvent(event);
            }
            else
            {
                TimerUtil_eSetTimer(STATION_REPORT_RESET_EVENT, 1000);
                EventUtil_vUnsetEvent(event);
            }
            break;
        }
        case STATION_PORT_QUERY_EVENT:
        {
#if (defined DEBUG_COM_APP || defined DEBUG_ERROR)
            /* initial i2c printf every 3*64 seconds */
            /* it will encrease the crc error */
            static uint16 cnt;
            if(++cnt % 64 ==0)
            {
                i2c_vPrintPoll(); //print datas in buffer first
                i2c_printf_init();
            }
#endif
            vSyncLEDStatus();

            if(bArmReady() == FALSE)
            {
                EDBG(i2c_vPrintf("ComPort: not connected\n"););
            }
            else
            {
                EDBG(
                    if(bIsStationUp)	i2c_vPrintf("up %d\n", u16ArmId);
                    else i2c_vPrintf("down %d\n", u16ArmId);
                    );
                //i2c_vPrintf("MI%d,MAP%d, Avg%d, Cnt%d\n",MaxIntTime, MaxIntMap, RealIntTime, RealIntCnt);
                struct nwkhdr *pNwkHdr = (struct nwkhdr *)u32CommonBuf;
                app_header_t *pHeader = (app_header_t *)(pNwkHdr + 1);
                app_LSrfReport_t *pStationReport = (app_LSrfReport_t *)(pHeader + 1);

                //Send Live to APP
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

                pHeader->msgtype = MPRF_REPORT;
                pHeader->protocoltype = APP_PROTOCOL_TYPE_MOBILE;

                pStationReport->hdr.dstaddr = 0;
                SysUtil_vConvertEndian(&pStationReport->hdr.dstaddr, sizeof(pStationReport->hdr.dstaddr));

                pStationReport->hdr.srcaddr = u16StationShortAddr;
                SysUtil_vConvertEndian(&pStationReport->hdr.srcaddr, sizeof(pStationReport->hdr.srcaddr));

                pStationReport->len = 0;
                SysUtil_vConvertEndian(&pStationReport->len, sizeof(pStationReport->len));

                pStationReport->reporttype = APP_LS_REPORT_LIVE;
                pStationReport->devtype = BSMAC_DEVICE_TYPE_COM;
                pStationReport->seqnum = u8LiveSeqNum++;

                Jbsmac_eWriteData((uint8 *)u32CommonBuf, sizeof(struct nwkhdr) + sizeof(app_header_t) + sizeof(app_LSrfReport_t));

                // DBG(i2c_vPrintMem((uint8 *)u32CommonBuf, sizeof(struct nwkhdr) + sizeof(app_header_t) + sizeof(app_mprfReport_t)););
            }

            DBG(i2c_vPrintf("SEQ: %d %d\n", u8LiveSeqNum, u8LastLiveAckSeq););

            uint8 tmp = u8LiveSeqNum - u8LastLiveAckSeq;
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
                vSaveResetType(APP_LS_REPORT_WATCHDOG_RESTART,REPORT_SPI_RESTART);
                vAHI_SwReset();
            }

            //reset LEDs state
            LedUtil_vOff(LED_RF);

            TimerUtil_eSetTimer(STATION_PORT_QUERY_EVENT, 3000);
            EventUtil_vUnsetEvent(STATION_PORT_QUERY_EVENT);
            break;
        }
        case STATION_REPORT_STATUS_EVENT:
        {
            if(bIsStationUp)
            {
                struct nwkhdr *pNwkHdr = (struct nwkhdr *)u32CommonBuf;
                app_header_t *pHeader = (app_header_t *)(pNwkHdr + 1);
                app_LSrfReport_t *pStationReport = (app_LSrfReport_t *)(pHeader + 1);
                app_LSComstatus_t* pStatus = (app_LSComstatus_t*) (pStationReport+1);

                uint8 u8VersionLen = strlen(VERSION) + 1;

                if(sizeof(struct nwkhdr) + sizeof(app_header_t) + sizeof(app_LSrfReport_t)+sizeof(app_LSComstatus_t)+u8VersionLen > 128)
                {
                    EDBG(i2c_vPrintf("RPT len err!\n"););
                    break;
                }

                /* network header */
                pNwkHdr->type = NWK_DATA;
                pNwkHdr->ttl = 1;

                pNwkHdr->src = u16StationPanId;
                SysUtil_vConvertEndian(&pNwkHdr->src, sizeof(pNwkHdr->src));

                pNwkHdr->dst = u16ArmId;
                SysUtil_vConvertEndian(&pNwkHdr->dst, sizeof(pNwkHdr->dst));

                pNwkHdr->len =  sizeof(app_header_t) + sizeof(app_LSrfReport_t)+sizeof(app_LSComstatus_t)+u8VersionLen;
                SysUtil_vConvertEndian(&pNwkHdr->len, sizeof(pNwkHdr->len));

                /* app header */
                pHeader->len = sizeof(app_LSrfReport_t)+sizeof(app_LSComstatus_t)+u8VersionLen;
                SysUtil_vConvertEndian(&pHeader->len, sizeof(pHeader->len));

                pHeader->msgtype = APP_TOF_MSG_REPORT;
                pHeader->protocoltype = APP_PROTOCOL_TYPE_MOBILE;

                /* rf report  */
                pStationReport->hdr.dstaddr = u16ArmId;
                SysUtil_vConvertEndian(&pStationReport->hdr.dstaddr, sizeof(pStationReport->hdr.dstaddr));
                pStationReport->hdr.srcaddr = u16StationPanId;
                SysUtil_vConvertEndian(&pStationReport->hdr.srcaddr, sizeof(pStationReport->hdr.srcaddr));

                pStationReport->len = sizeof(app_LSComstatus_t) + u8VersionLen;
                SysUtil_vConvertEndian(&pStationReport->len, sizeof(pStationReport->len));

                static uint8 u8ReportStatusNum;
                pStationReport->reporttype = APP_LS_REPORT_STATUS_COM;
                pStationReport->devtype = BSMAC_DEVICE_TYPE_COM;
                pStationReport->seqnum = u8ReportStatusNum++;

                /* com status  */
                pStatus->port[0].neighborid = Jbsmac_u16GetPeerAddr(0);
                pStatus->port[0].livestat = Jbsmac_u8GetLinkStatus(0);
                Jbsmac_vGetErrCnt(0, &pStatus->port[0].total_cnt , &pStatus->port[0].lost_cnt);

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

                CONVERT_ENDIAN(pStatus->port[0].neighborid);
                CONVERT_ENDIAN(pStatus->port[0].livestat);
                CONVERT_ENDIAN(pStatus->port[0].total_cnt);
                CONVERT_ENDIAN(pStatus->port[0].lost_cnt);

                CONVERT_ENDIAN(pStatus->port[1].neighborid);
                CONVERT_ENDIAN(pStatus->port[1].livestat);
                CONVERT_ENDIAN(pStatus->port[1].total_cnt);
                CONVERT_ENDIAN(pStatus->port[1].lost_cnt);

                pStatus->oad_version = OAD_COM_STATION_VERSION;
                pStatus->len = u8VersionLen;
                CONVERT_ENDIAN(pStatus->oad_version);
                CONVERT_ENDIAN(pStatus->len);

                strcpy((uint8*)(pStatus+1), VERSION);

                EDBG(
                    i2c_vPrintf("Ver %s \n", VERSION);
                );

                Jbsmac_eWriteData((uint8 *)u32CommonBuf, sizeof(struct nwkhdr) + sizeof(app_header_t) + sizeof(app_LSrfReport_t)+sizeof(app_LSComstatus_t)+u8VersionLen);
            }

            TimerUtil_eSetTimer(STATION_REPORT_STATUS_EVENT, 120000);
            EventUtil_vUnsetEvent(STATION_REPORT_STATUS_EVENT);
            break;
        }
        case STATION_REPORT_RSSI_EVENT:
        {
            if(rssiReport.app_rssi_head.protocoltype == APP_PROTOCOL_TYPE_MOBILE)
                {
                     struct nwkhdr *pNwkHdr = (struct nwkhdr *)u32CommonBuf;

                     //Send Live to APP
                     pNwkHdr->type = NWK_DATA;
                     pNwkHdr->ttl = 1;

                     pNwkHdr->src = u16StationPanId;
                     SysUtil_vConvertEndian(&pNwkHdr->src, sizeof(pNwkHdr->src));

                     pNwkHdr->dst = u16ArmId;
                     SysUtil_vConvertEndian(&pNwkHdr->dst, sizeof(pNwkHdr->dst));

                     pNwkHdr->len =   sizeof(app_rssi_report);
                     SysUtil_vConvertEndian(&pNwkHdr->len, sizeof(pNwkHdr->len));

                     memcpy(((uint8 *)u32CommonBuf+sizeof(struct nwkhdr)) , (uint8 *)(&rssiReport), sizeof(app_rssi_report));
                     Jbsmac_eWriteData((uint8 *)(u32CommonBuf),sizeof(struct nwkhdr) +sizeof(app_rssi_report));
                     reset_report_rssi();
                }
                TimerUtil_eSetTimer(STATION_REPORT_RSSI_EVENT, REPORT_RSSI_TIME);
                EventUtil_vUnsetEvent(STATION_REPORT_RSSI_EVENT);
                break;
            }

        default:
            break;
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
    if (teStaionState >= E_STATE_COORDINATOR_STARTED)
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
            DBG(i2c_vPrintf("defmcps %d\n",psMcpsInd->u8Type ););
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
    hal_ProcessDataCnf (&psMcpsInd->uParam.sDcfmData);
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

    app_header_t *psAppPkt = (app_header_t *)(psFrame->au8Sdu);

    switch (psAppPkt->protocoltype)
    {
    case APP_PROTOCOL_TYPE_MOBILE:
    {
        u32LastAirRxTimeMs = Hal_GetTimeMs();

        if(!bIsStationUp)
        {
            DBG(i2c_vPrintf("AR: not handled\n"););
            return;
        }

        vProcessPhoneData(&psMcpsInd->uParam.sIndData);

        LedUtil_vToggle(LED_RF);
        break;
    }

    case APP_PROTOCOL_TYPE_CMD:
    {
        vProcessStationCmd(&psMcpsInd->uParam.sIndData);
        break;
    }

    default:
    {
        DBG(
            static uint16 Print_count;
            Print_count++;
            if((Print_count>200)||(psAppPkt->protocoltype != APP_PROTOCOL_TYPE_CARD))
    {
        Print_count=0;
        i2c_vPrintf("defpro: %d\n",psAppPkt->protocoltype);
        }
        );
        break;
    }
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

    teStaionState = E_STATE_COORDINATOR_STARTED;

    sMlmeReqRsp.u8Type = MAC_MLME_REQ_START;
    sMlmeReqRsp.u8ParamLength = sizeof(MAC_MlmeReqStart_s);
    sMlmeReqRsp.uParam.sReqStart.u16PanId = u16StationPanId;
    sMlmeReqRsp.uParam.sReqStart.u8Channel = u8Channel;
    sMlmeReqRsp.uParam.sReqStart.u8BeaconOrder = 0x0F;
    sMlmeReqRsp.uParam.sReqStart.u8SuperframeOrder = 0x0F;
    sMlmeReqRsp.uParam.sReqStart.u8PanCoordinator = TRUE;
    sMlmeReqRsp.uParam.sReqStart.u8BatteryLifeExt = FALSE;
    sMlmeReqRsp.uParam.sReqStart.u8Realignment = FALSE;
    sMlmeReqRsp.uParam.sReqStart.u8SecurityEnable = FALSE;

    vAppApiMlmeRequest(&sMlmeReqRsp, &sMlmeSyncCfm);
}

PRIVATE void vProcessPhoneData(MAC_McpsIndData_s *psMcpsIndData)
{

    MAC_RxFrameData_s *psFrame;
    psFrame = &psMcpsIndData->sFrame;
    if(psFrame->u8SduLength < sizeof(app_header_t) || psFrame->u8SduLength > MAC_MAX_DATA_PAYLOAD_LEN)
    {
        EDBG(i2c_vPrintf("air msg  len ERR! len:%d\n", psFrame->u8SduLength););
        return;
    }

    app_header_t *pheader = (app_header_t *)(psFrame->au8Sdu);

    DBG(
        if(pheader->msgtype == MP_VOICE)
{
    static uint32 cnt;
    if(cnt++ % 20 == 0)
        {
            i2c_vPrintf("AR voice\n");
        }
    }
    else
    {
        i2c_vPrintf("AR\n");
        //i2c_vPrintMem(psFrame->au8Sdu, psFrame->u8SduLength);
    }
    );

    uint16 headerlen = pheader->len;
    CONVERT_ENDIAN(headerlen);

    if(headerlen + sizeof(app_header_t) != psFrame->u8SduLength)
    {
        EDBG(i2c_vPrintf("app_header len ERR!\n"););
        return;
    }

     update_station_rssi(u16StationPanId,SysUtil_vConvertLQI2Dbm(psFrame->u8LinkQuality),INVALID_RSSI,MP_RSSI_MPSTATION);

    if(pheader->msgtype == MP_JOIN_NOTIFY)
    {
         TimerUtil_eSetTimer(STATION_REPORT_RSSI_EVENT, 1000);
    }

    if(pheader->msgtype < MP_MP2MP_CMDEND)
    {
        /* check if the src addr is equal as the wireless srcaddr */
        EDBG
        (
            if(pheader->msgtype < MP_MP2ARM_CMDEND)
    {
        app_mpheader_t *pmpheader = (app_mpheader_t *)(pheader + 1);
            uint16 srcaddr = pmpheader->srcaddr;
            CONVERT_ENDIAN(srcaddr);
            if(srcaddr != psFrame->sSrcAddr.uAddr.u16Short)
            {
                i2c_vPrintf("shortaddr ERR! s addr: %d, p addr: %d\n", psFrame->sSrcAddr.uAddr.u16Short, srcaddr);
            }
        }
        );

        if((psFrame->u8SduLength + sizeof(struct nwkhdr)) <= BSMAC_MAX_TX_PAYLOAD_LEN)
        {
            struct nwkhdr *pNwkHdr = (struct nwkhdr *)u32CommonBuf;
            pNwkHdr->type = NWK_DATA;
            pNwkHdr->ttl = 255;

            pNwkHdr->src = u16StationPanId;
            SysUtil_vConvertEndian(&pNwkHdr->src, sizeof(pNwkHdr->src));

            pNwkHdr->dst = u16ArmId;
            SysUtil_vConvertEndian(&pNwkHdr->dst, sizeof(pNwkHdr->dst));

            pNwkHdr->len =  psFrame->u8SduLength;
            SysUtil_vConvertEndian(&pNwkHdr->len, sizeof(pNwkHdr->len));

            memcpy(((uint8 *)u32CommonBuf) + sizeof(struct nwkhdr), (uint8 *)(psFrame->au8Sdu), psFrame->u8SduLength);

            DBG(
                if(pheader->msgtype != MP_VOICE)
        {
            i2c_vPrintf("US\n");
            }
            );
            Jbsmac_eWriteData((uint8 *)(u32CommonBuf), psFrame->u8SduLength + sizeof(struct nwkhdr));
        }
    }
    else if (pheader->msgtype == MP_SCAN)
    {

        if(!bIsStationUp)
        {
            return;
        }

        DBG(
            static uint16 Print_count;
            if(Print_count++>1000)
    {
        Print_count=0;
        i2c_vPrintf("MP_SCAN!\n");
        }
        );
        mp_Scan_t *pArmid = (mp_Scan_t *)(pheader + 1) ;

        if(pArmid->scantype == APP_SCAN_TYPE_REQ)
        {
            app_header_t *app_hdr = (app_header_t *)u32CommonBuf;
            mp_Scan_t *mp_Armid = (mp_Scan_t *)(app_hdr + 1);

            app_hdr->protocoltype = APP_PROTOCOL_TYPE_MOBILE;
            app_hdr->msgtype = MP_SCAN;
            app_hdr->len = sizeof(mp_Scan_t);
            SysUtil_vConvertEndian(&app_hdr->len, sizeof(app_hdr->len));

            mp_Armid->scantype = APP_SCAN_TYPE_ACK;
            mp_Armid->seqnum = pArmid->seqnum;
            mp_Armid->armid = u16ArmId;	//??????????????
            SysUtil_vConvertEndian(&mp_Armid->armid, sizeof(mp_Armid->armid));

            //vAirFillSendBuf(psFrame->sSrcAddr.uAddr.u16Short, (uint8 *)app_hdr, sizeof(app_header_t) + sizeof(mp_Scan_t));
            Hal_SendDataToAir ( (uint8 *)app_hdr, sizeof(app_header_t) + sizeof(mp_Scan_t),
                                MOBILEPHONE_NWK_ADDR, psFrame->sSrcAddr.uAddr.u16Short, TRUE);
        }
        else
        {
            EDBG(i2c_vPrintf("scan err!\n"););
        }
    }
}

PRIVATE void vProcessStationCmd(MAC_McpsIndData_s *psMcpsIndData)
{
    MAC_RxFrameData_s *psFrame;
    psFrame = &psMcpsIndData->sFrame;
    RfTofWrapper_tu* psAppPkt = (RfTofWrapper_tu*)(psFrame->au8Sdu);

    switch(psAppPkt->tof_head.msgtype)
    {
    case TOF_COM_STATION_OAD:
    {
        if((psAppPkt->tof_head.len == 4)
                && (psFrame->sDstAddr.u16PanId == u16StationPanId)
                &&(psFrame->sDstAddr.uAddr.u16Short == 0)
                && (psAppPkt->rf_tof_oad_data.u8DeviceType == DEVICE_TYPE_STATION)
                && (psAppPkt->rf_tof_oad_data.u16Version > OAD_COM_STATION_VERSION))
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
            vSaveResetType(APP_LS_REPORT_WATCHDOG_RESTART,REPORT_OAD_RESTART);
            vAHI_SwReset();

        }
        break;
    }
    default:
        DBG(i2c_vPrintf("defcmd%d\n",psAppPkt->tof_head.msgtype););
        break;
    }

}



PRIVATE void vBsMac_rx_callback(unsigned char *pbuf, unsigned char len)
{
    if(len < (sizeof(struct nwkhdr) + sizeof(app_header_t)) || len > BSMAC_MAX_TX_PAYLOAD_LEN)
    {
        EDBG(i2c_vPrintf("uart data len  ERR! length %d\n", len););
        return;
    }

    struct nwkhdr *pNwkHdr = (struct nwkhdr *)(pbuf);
    app_header_t *pheader =  (app_header_t *)((uint8 *)pbuf + sizeof(struct nwkhdr));
    uint8 		*pPayload =	(uint8 *)(pheader + 1);
    uint16 datalen = pNwkHdr->len;
    CONVERT_ENDIAN(datalen);

    DBG(

        if(pheader->msgtype == MP_VOICE)
{
    static uint32 cnt;
    if(cnt++ % 20 == 0)
        {
            i2c_vPrintf(" UR voice\n");
        }
    }
    else
    {
        i2c_vPrintf("UR\n" );
        //i2c_vPrintMem(pbuf,  len);
    }

    );

    if(pheader->protocoltype != APP_PROTOCOL_TYPE_MOBILE || len != datalen + sizeof(struct nwkhdr) )
    {
        EDBG(i2c_vPrintf("protocol type %d or header len %d ERR!\n", pheader->protocoltype, len););
        return;
    }

    /* mp2arm,  send to  mobile directly, always have headr app_mpheader_t */
    if(pheader->msgtype > MP_MP2ARM_CMDSTART && pheader->msgtype < MP_MP2MP_CMDEND)
    {
        uint16 u16DstAddr;
        if(pheader->msgtype > MP_MP2ARM_CMDSTART && pheader->msgtype < MP_MP2ARM_CMDEND)
        {
            app_mpheader_t *pmpheader = (app_mpheader_t *)pPayload;
            u16DstAddr	= pmpheader->dstaddr;
            CONVERT_ENDIAN(u16DstAddr);
        }
        /*MP2MP*/
        else if(pheader->msgtype > MP_MP2MP_CMDSTART && pheader->msgtype < MP_MP2MP_CMDEND)
        {
            app_termNbr_t *srcnbr = (app_termNbr_t *)pPayload;
            app_termNbr_t *dstnbr = srcnbr + 1;

            char str[20];
            num_term2str(str, dstnbr);
            u16DstAddr	= SysUtil_u16atou(str);
        }

        //  vAirFillSendBuf( u16DstAddr, (uint8 *)pheader, datalen);

        Hal_SendDataToAir((uint8 *)pheader, datalen, MOBILEPHONE_NWK_ADDR, u16DstAddr, TRUE);

    }
    /* mprf vs arm */
    else if(pheader->msgtype == MPRF_REPORT_ACK)
    {
        app_LSrfReport_t *prfReport = (app_LSrfReport_t *)pPayload;
        if( prfReport->reporttype == APP_LS_REPORT_LIVE)
        {
            u8LastLiveAckSeq = prfReport->seqnum;
            bIsStationUp = TRUE;
            u8RestartCounter = 0;
        }
    }

    else if(pheader->msgtype == MODULE_ERROR_RATE_TEST)
    {
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
        pHeader->protocoltype = APP_PROTOCOL_TYPE_MOBILE;

        Jbsmac_eWriteData((uint8 *)u32CommonBuf, sizeof(struct nwkhdr) + sizeof(app_header_t));
    }

    #if 0

    /* support error rate test */
    DBG(
        if(pheader->msgtype == MODULE_ERROR_RATE_TEST)
{
    pan_addr_t tmp;
    tmp = pNwkHdr->src;
    pNwkHdr->src = pNwkHdr->dst;
    pNwkHdr->dst = tmp;
    DBG(
        i2c_vPrintf("S err test:\n");
        //i2c_vPrintMem((uint8 *)pbuf, len);
    );
        Jbsmac_eWriteData((uint8 *)pbuf, len);
    }
    );
    #endif
}

PRIVATE bool_t bArmReady()
{
    int8 port = Jbsmac_i8GetUpPort();

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

void MP_Report_ResetCause(uint8 reporttype,uint8 devtype)
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

    pNwkHdr->len =  sizeof(app_header_t) + sizeof(app_LSrfReport_t);
    SysUtil_vConvertEndian(&pNwkHdr->len, sizeof(pNwkHdr->len));

    pHeader->len = sizeof(app_LSrfReport_t);
    SysUtil_vConvertEndian(&pHeader->len, sizeof(pHeader->len));

    pHeader->msgtype = MPRF_REPORT;
    pHeader->protocoltype = APP_PROTOCOL_TYPE_MOBILE;

    pStationReport->hdr.dstaddr = 0;
    SysUtil_vConvertEndian(&pStationReport->hdr.dstaddr, sizeof(pStationReport->hdr.dstaddr));

    pStationReport->hdr.srcaddr = u16StationShortAddr;
    SysUtil_vConvertEndian(&pStationReport->hdr.srcaddr, sizeof(pStationReport->hdr.srcaddr));

    pStationReport->len = 0;
    SysUtil_vConvertEndian(&pStationReport->len, sizeof(pStationReport->len));

    pStationReport->reporttype = reporttype;
    pStationReport->devtype = devtype;
    pStationReport->seqnum = u8ReportResetCauseSeq++;

    Jbsmac_eWriteData((uint8 *)u32CommonBuf, sizeof(struct nwkhdr) + sizeof(app_header_t) + sizeof(app_LSrfReport_t));
}

/****************************************************************************
 *
 * NAME: vSaveResetType
 *
 * DESCRIPTION:
 * Save ResetType to flash
 *
 * PARAMETERS:      Name    RW  Usage
 *                  u8reportType  w   reset type
 *                  u8TypeInfo  W   reset type info
 * RETURNS:
 * bool TURE for success, FALSE for fail
 *
 ****************************************************************************/
bool  vSaveResetType(uint8 u8reportType, uint8 u8TypeInfo)
{

    ResetParam_t Resetparam;

    if(!bAHI_FullFlashRead(RESET_PARAM_STORE_ADDR, sizeof(ResetParam_t),(uint8*)&Resetparam))
    {
        EDBG(i2c_vPrintf("Flash Read Fail\n"););
        return FALSE;
    }

    if(!bAHI_FlashEraseSector(7))
    {
        EDBG(i2c_vPrintf("Flash Erase Fail\n"););
        return FALSE;
    }
    Resetparam.u8reportType = u8reportType;
    Resetparam.u8TypeInfo = u8TypeInfo;

    if(bAHI_FullFlashProgram(RESET_PARAM_STORE_ADDR, sizeof(ResetParam_t),(uint8*)&Resetparam))
    {
        DBG(i2c_vPrintf("Set  u8reportType Success!%d %d\n", Resetparam.u8reportType, Resetparam.u8TypeInfo););
        return TRUE;
    }
    else
    {
        EDBG(i2c_vPrintf("Set  u8reportType fail!%d %d\n", Resetparam.u8reportType, Resetparam.u8TypeInfo););
        return FALSE;
    }
}

PRIVATE void  vStackCheck(void)
{
    uint32 u32CurrentTimeMs = Hal_GetTimeMs();

    //if over 1 min did not received any signal  from air, then sleep 1ms,
    //sleep is useless,  just want to run to warm start and  reset stack,
    // At the same time keep ram on, make the system go to work rapidly.

    if(u32CurrentTimeMs -  u32LastAirRxTimeMs > 60*1000)
    {
        EDBG(i2c_vPrintf("No Air! %d %d\n ", u32LastAirRxTimeMs, u32CurrentTimeMs););
        EDBG(i2c_vPrintPoll();); //print datas in buffer first
        SleepUtil_Sleep(E_AHI_WAKE_TIMER_0, 1, E_AHI_SLEEP_OSCON_RAMON);
    }
}

PRIVATE void vSyncLEDStatus(void)
{
    // for the arm neighbor: up port, must link to arm, down port, link to uart
    // for not the arm neighbor, up and down port only need link up

    if(Jbsmac_bIsArmNeighbor())
    {
        int8 port = Jbsmac_i8GetUpPort();
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
    }
    else
    {
        if( Jbsmac_u8GetLinkStatus(0) > 0)
        {
            LedUtil_vOn(LED_UART0);
        }
        else
        {
            LedUtil_vOff(LED_UART0);
        }
        if( Jbsmac_u8GetLinkStatus(1) > 0)
        {
            LedUtil_vOn(LED_UART1);
        }
        else
        {
            LedUtil_vOff(LED_UART1);
        }
    }
}

PRIVATE void reset_report_rssi(void)
{
    rssiReport.app_rssi_head.protocoltype =0;
    rssiReport.receiveRssi = INVALID_RSSI;
    rssiReport.sentRssi = INVALID_RSSI;
}

PRIVATE void update_station_rssi(uint16 u16Short,int8 receivei8Rssi,int8 senti8Rssi,uint8 msgtype)
{
        rssiReport.app_rssi_head.protocoltype = APP_PROTOCOL_TYPE_MOBILE;
        rssiReport.app_rssi_head.msgtype = msgtype;
        rssiReport.app_rssi_head.len = sizeof(app_rssi_report)-sizeof(app_header_t);
        SysUtil_vConvertEndian(&rssiReport.app_rssi_head.len, sizeof(rssiReport.app_rssi_head.len));
        rssiReport.u16ShortAddr =  u16Short;
        SysUtil_vConvertEndian(&rssiReport.u16ShortAddr, sizeof(rssiReport.u16ShortAddr));
        rssiReport.receiveRssi  =  receivei8Rssi;
        rssiReport.sentRssi=  senti8Rssi;
}

/****************************************************************************/
/***        END OF FILE                                                   ***/
/****************************************************************************/




