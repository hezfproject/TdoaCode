
/****************************************************************************/
/***        Include files                                                 ***/
/****************************************************************************/
#include <jendefs.h>
#include <AppHardwareApi.h>
#include <AppQueueApi.h>
#include <mac_sap.h>
#include <mac_pib.h>
#include <Utilities.h>

#include "config.h"
#include "app_protocol.h"
#include "bsmac.h"
#include "JN5148_util.h"
#include "string.h"

/****************************************************************************/
/***        Macro Definitions                                             ***/
/****************************************************************************/
#define RSSI_CHECK_RETREAT_EVENT    BIT(3)
#define RSSI_WATCHDOG_EVENT            BIT(4)
#define RSSI_PORT_QUERY_EVENT          BIT(5)
#define RSSI_REPORT_DATA_EVENT           BIT(6)
#define RSSI_REPORT_STATUS_EVENT      BIT(7)

#define RSSI_REPORT_CARDVERSION       BIT(10)


//#define  DEBUG_FULL_LOAD   //just for test processing capacity of arm
#ifdef  DEBUG_FULL_LOAD
#define RSSI_TEST_FULL_EVENT      BIT(8)
#endif

#define LED_RF          E_AHI_DIO16_INT
#define LED_UART0        E_AHI_DIO9_INT
#define LED_UART1        E_AHI_DIO8_INT

#if (defined DEBUG_RSSI_STATION_APP)
#define DBG(x) do{x}while(0);
#else
#define DBG(x)
#endif

#if (defined DEBUG_ERROR)
#define EDBG(x) do{x} while (0);
#else
#define EDBG(x)
#endif

#define PARAM_STORE_ADDR 	0x70000


#ifdef  DEBUG_FULL_LOAD
PRIVATE uint16  debug_card_base = 1 ;
PRIVATE uint16  debug_card_num = 1 ;
PRIVATE uint16  debug_count=1;
PRIVATE uint16  debug_begin=1;
#endif

/****************************************************************************/
/***        Type Definitions                                              ***/
/****************************************************************************/
typedef enum
{
    E_STATE_IDLE,
    E_STATE_STARTED,
    E_STATE_RSSI,
    E_STATE_BUSY,
} teState;


/****************************************************************************/
/***        Local Function Prototypes                                     ***/
/****************************************************************************/
PRIVATE void vInitRssiStationSystem(void);
PRIVATE void vStartingLEDindicator(void);
PRIVATE void vStartCoordinator(void);
PRIVATE void vProcessSysEventQueues(void);
PRIVATE void vProcessIncomingMlme(MAC_MlmeDcfmInd_s *psMlmeInd);
PRIVATE void vProcessIncomingMcps(MAC_McpsDcfmInd_s *psMcpsInd);
PRIVATE void vProcessIncomingHwEvent(AppQApiHwInd_s *psAHI_Ind);
PRIVATE void vHandleMcpsDataInd(MAC_McpsDcfmInd_s *psMcpsInd);
PRIVATE void vHandleMcpsDataDcfm(MAC_McpsDcfmInd_s *psMcpsInd);
PRIVATE void vProcessAppEventQueues(void);
PRIVATE void vReportCardAlarm(uint16 u16ShortAddr, uint8 alarm_type);
PRIVATE void vReportCardExcite(uint16 u16ShortAddr, uint8 u8CardStatus,uint8 u8ExciteID);
PRIVATE void vCheckCardStatus(uint16 u16ShortAddr, uint8 u8Status);
PRIVATE void vRssiStationCast(uint8 u8CmdType, uint16 u16DstAddr, uint16 u16DstPanId, uint8 u8TxOptions, uint8 u8Val);

PRIVATE void vInitialRssiStationBaud(void);
PRIVATE bool_t bArmReady();
PRIVATE void vBsMac_rx_callback(unsigned char *pbuf, unsigned char len);
PRIVATE void vReportCardRssi();
PRIVATE void vConvertEndian16(uint16* pu16Data);
PRIVATE void vWriteData2Arm(uint8* pbuf);
PRIVATE bool bChannelValid(uint8 u8BroadcastChannel);
PRIVATE bool  vSaveChannel(uint8 u8Channel);


PRIVATE void vUart0_rx_callback(uint32 u32DeviceId, uint32 u32ItemBitmap);


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
PRIVATE uint16 u16RssiShortAddr;
PRIVATE uint16 u16RssiPanId;
PRIVATE uint8 u8RssiChannel;

PRIVATE tof_station_status_te tRssiStationStatus = STATION_STATUS_NORMAL;
PRIVATE teState teStaionState = E_STATE_IDLE;


PRIVATE uint32 u32Data2Arm[30];    // the data send to arm, max len is 114 (BSMAC_MAX_TX_PAYLOAD_LEN), 30*4 = 120 is enough (28 is enough)
PRIVATE struct nwkhdr* pnwk_data = (struct nwkhdr*)u32Data2Arm;
PRIVATE uint16 u16ArmId;
PRIVATE uint16 u16RssiNum = 0;
PRIVATE app_tof_rssi_ts app_rssi_data;

/*use u32 for alignment*/
PRIVATE uint32 u32CommonBuf[128];
PRIVATE uint8 u8LiveSeqNum = 0;
PRIVATE uint8 u8LastLiveAckSeq = 0;
PRIVATE bool bIsStationUp = FALSE;
PRIVATE uint8 u8RestartCounter=0;
PRIVATE uint8 u8ReportStatusNum=0;
PRIVATE bool bReported = FALSE;

PRIVATE bool_t bDetailedPrint = FALSE;

PRIVATE app_LSVersionReport_t tsVersionReport[APP_TOF_VERSION_MAX_NUM];
PRIVATE uint8  			      tsVersionReportLen;


#define SYNCSTATUS_COUNT  (5*60*1000/20)   //  5 min
PRIVATE uint16 syncstatus_count=0;
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
    //vAHI_WatchdogStop();
    vInitRssiStationSystem();
    vStartCoordinator();

    //PrintfUtil_u8Init(E_AHI_UART_0, PRINTF_BAUD_RATE_1M);
    //DebugUtil_vInit(E_AHI_UART_0, vUart0_rx_callback);
#if (defined DEBUG_RSSI_STATION_APP || defined DEBUG_ERROR)
    i2c_printf_init();
    //DebugUtil_vInit(E_AHI_UART_0, vUart0_rx_callback);
    i2c_vPrintf("RSSI System started. \n\r");
    i2c_vPrintf("station id %d\n", u16RssiPanId);
    i2c_vPrintf("Channel is %d\n", u8RssiChannel);
    if(bAHI_WatchdogResetEvent()) i2c_vPrintf("Watchdog reset !!!\n");
#endif

    DBG(
#ifdef  DEBUG_FULL_LOAD
        i2c_vPrintf("debug_card_base is %d\n", debug_card_base);
        i2c_vPrintf("debug_card_num is %d\n", debug_card_num);
        i2c_vPrintf("debug_count is %d\n", debug_count);
#endif
    );

    EventUtil_vSetEvent(RSSI_PORT_QUERY_EVENT);
    EventUtil_vSetEvent(RSSI_REPORT_DATA_EVENT);
    EventUtil_vSetEvent(RSSI_REPORT_STATUS_EVENT);
    EventUtil_vSetEvent(RSSI_REPORT_CARDVERSION);
    TimerUtil_eSetCircleTimer(RSSI_CHECK_RETREAT_EVENT, 20);    // check app every 20 ms
    TimerUtil_eSetCircleTimer(RSSI_WATCHDOG_EVENT, 500);        // feed watch dog every 500 milliseconds

#ifdef  DEBUG_FULL_LOAD
    TimerUtil_eSetCircleTimer(RSSI_TEST_FULL_EVENT, (uint32)5000/debug_count);
#endif

    while (1)
    {
        Jbsmac_vPoll();
        TimerUtil_vUpdate();
        vProcessSysEventQueues();
        vProcessAppEventQueues();
#if (defined DEBUG_RSSI_STATION_APP || defined DEBUG_ERROR)
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
    //PrintfUtil_u8Init(E_AHI_UART_0, PRINTF_BAUD_RATE_1M);
    /*DBG(
        i2c_vPrintf("warm start. \n\r");
    );*/

    AppColdStart();
}

/****************************************************************************/
/***        Local Functions                                               ***/
/****************************************************************************/

/****************************************************************************
 *
 * NAME: vInitRssiStationSystem
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
PRIVATE void vInitRssiStationSystem(void)
{
    (void)u32AppQApiInit(NULL, NULL, NULL); //vHwDeviceIntCallback);
    (void)u32AHI_Init();
    vAHI_DioSetDirection(0, E_AHI_DIO18_INT);   //init hardware watchdog GPIO
    vAHI_DioSetOutput(E_AHI_DIO18_INT,0);
    vAHI_HighPowerModuleEnable(TRUE, TRUE);
    TimerUtil_vInit();

    bAHI_FlashInit(E_FL_CHIP_ST_M25P40_A, NULL);
    LedUtil_bRegister(LED_RF);
    LedUtil_bRegister(LED_UART0);
    LedUtil_bRegister(LED_UART1);

    ErrorUtil_vRegisterLed0(LED_RF);
    ErrorUtil_vRegisterLed1(LED_UART0);
    ErrorUtil_vRegisterLed2(LED_UART1);

    vStartingLEDindicator();
    MacUtil_vReadExtAddress(&psMacAddr);

    uint32 tmp32;
    tmp32 = StringUtil_BCD2uint32(psMacAddr.u32H);

    u16RssiShortAddr = 0x0000;    //psMacAddr.u32L & 0xFFFF;
    u16RssiPanId = tmp32%100000;

#ifdef  DEBUG_FULL_LOAD
    debug_card_num =(uint16)((psMacAddr.u32L) & 0xFFFF);
    debug_card_base = (uint16)(((psMacAddr.u32L) >> 16) & 0xFFFF);
    if(debug_card_num>0x03E8)debug_card_num=0x03E8;
    if(debug_card_num<0x00C8)debug_card_num=0x00C8;
    debug_count=(uint16)debug_card_num/APP_MAX_CARD_NUM+1;
    debug_begin=(uint16)(debug_card_base-1);
    uint8 channel = 11;
#else
    uint8 channel = ((psMacAddr.u32L) >> 16) & 0xFF;
#endif

    if(channel >=11 && channel <=26)
    {
        u8RssiChannel = channel;
    }
    else
    {
        u8RssiChannel = DEFAULT_CHANNEL_BROADCAST;
    }

    if(bAHI_FullFlashRead(PARAM_STORE_ADDR, sizeof(channel),(uint8*)&channel)
            && bChannelValid(channel))
    {
        u8RssiChannel = channel;
    }

// for arm
    pnwk_data->type = NWK_DATA;
    pnwk_data->ttl = 1;
    pnwk_data->src = u16RssiPanId;
    vConvertEndian16(&(pnwk_data->src));

    app_rssi_data.app_tof_head.protocoltype = APP_PROTOCOL_TYPE_CARD;
    app_rssi_data.app_tof_head.msgtype = APP_TOF_MSG_RSSI;

    vAHI_UartSetRTSCTS(E_AHI_UART_1,FALSE);     // uart1:2-wire mode,DIO18 is RTS

    vInitialRssiStationBaud();

    s_pvMac = pvAppApiGetMacHandle();
    s_psMacPib = MAC_psPibGetHandle(s_pvMac);

    MAC_vPibSetPanId(s_pvMac, u16RssiPanId);
    MAC_vPibSetShortAddr(s_pvMac, u16RssiShortAddr);

    MAC_vPibSetRxOnWhenIdle(s_pvMac, TRUE, FALSE);
    MAC_vPibSetMaxCsmaBackoffs(s_pvMac, 2);
    MAC_vPibSetMinBe(s_pvMac, 1);

    s_psMacPib->bAssociationPermit = 0;
    s_psMacPib->bAutoRequest=0;
    s_psMacPib->bGtsPermit = FALSE;
    //FIXME
    s_psMacPib->u8MaxFrameRetries = 1;

    MacUtil_Setting_s sMacUtil;
    sMacUtil.u16SrcPanId         = u16RssiPanId;
    sMacUtil.u16SrcShortAddr     = u16RssiShortAddr;

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

PRIVATE void vReportCardVersion(void)
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
        pNwkHdr->src = u16RssiPanId;
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
        pStationReport->hdr.srcaddr = u16RssiPanId;
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


/****************************************************************************
 *
 * NAME: vInitialRssiStationBaud
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

PRIVATE void vInitialRssiStationBaud(void)
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
    Jbsmac_u8Init(vBsMac_rx_callback, 2, u8BsmacUartBaud, u16RssiPanId, BSMAC_DEVICE_TYPE_LOC);
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
    uint8 count=0;

    while(event && count++ < 32)
    {
        switch (event)
        {

#ifdef  DEBUG_FULL_LOAD
        case RSSI_TEST_FULL_EVENT:
        {
            DBG(i2c_vPrintf("rssi test full event!\n START:%d   NUM:%d\n", debug_card_base,debug_card_num););
            {
                MAC_McpsDcfmInd_s sMcpsInd;
                vHandleMcpsDataInd(&sMcpsInd);
                EventUtil_vUnsetEvent(event);
            }
            break;
        }
#endif

        case RSSI_CHECK_RETREAT_EVENT:
        {
            /* sent station status to card when retreat  or receve normal status
                1.alawys down sent when  retreat ,but sent five minute when receve normal status  */
            if((tRssiStationStatus == STATION_STATUS_RETREAT)||syncstatus_count)
            {
                if(syncstatus_count)syncstatus_count--;
                vRssiStationCast(TOF_STATION_RSSI, 0xFFFF, TOF_CARD_NWK_ADDR, 0, 0);
            }

            EventUtil_vUnsetEvent(event);
            break;
        }

        case RSSI_WATCHDOG_EVENT:
        {
            #if (defined SUPPORT_HARD_WATCHDOG)
            vFeedHardwareWatchDog();
            #endif
            vAHI_WatchdogRestart();
            EventUtil_vUnsetEvent(event);
            break;
        }

        case RSSI_REPORT_DATA_EVENT:
        {
            DBG(i2c_vPrintf("report event\n"););
            vReportCardRssi();

            TimerUtil_eSetTimer(RSSI_REPORT_DATA_EVENT, 500);
            EventUtil_vUnsetEvent(RSSI_REPORT_DATA_EVENT);

            break;
        }

        case RSSI_PORT_QUERY_EVENT:
        {
            /* initial i2c printf every 3*64 seconds */
            /* it will encrease the crc error */
#if (defined DEBUG_LOC_APP || defined DEBUG_ERROR)
            static uint16 cnt;
            if(++cnt % 64 ==0)
            {
                i2c_vPrintPoll(); //print datas in buffer first
                i2c_printf_init();
            }
#endif
            if(bArmReady() == FALSE)
            {
                EDBG(i2c_vPrintf("ComPort: not connected\n"););
            }
            else
            {
                EDBG(
                    if(bIsStationUp)	i2c_vPrintf("up%d\n", u16ArmId);
                    else i2c_vPrintf("down%d\n", u16ArmId);
                    );
                //i2c_vPrintf("MI%d,MAP%d, Avg%d, Cnt%d\n",MaxIntTime, MaxIntMap, RealIntTime, RealIntCnt);

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

                struct nwkhdr *pNwkHdr = (struct nwkhdr *)u32CommonBuf;
                app_header_t *pHeader = (app_header_t *)(pNwkHdr + 1);
                app_LSrfReport_t *pStationReport = (app_LSrfReport_t *)(pHeader + 1);

                pNwkHdr->type = NWK_DATA;
                pNwkHdr->ttl = 1;

                pNwkHdr->src = u16RssiPanId;
                SysUtil_vConvertEndian(&pNwkHdr->src, sizeof(pNwkHdr->src));

                pNwkHdr->dst = u16ArmId;
                SysUtil_vConvertEndian(&pNwkHdr->dst, sizeof(pNwkHdr->dst));

                pNwkHdr->len =  sizeof(app_header_t) + sizeof(app_LSrfReport_t);
                SysUtil_vConvertEndian(&pNwkHdr->len, sizeof(pNwkHdr->len));

                pHeader->len = sizeof(app_LSrfReport_t);
                SysUtil_vConvertEndian(&pHeader->len, sizeof(pHeader->len));

                pHeader->msgtype = APP_TOF_MSG_REPORT;
                pHeader->protocoltype = APP_PROTOCOL_TYPE_CARD;

                pStationReport->hdr.dstaddr = u16ArmId;
                SysUtil_vConvertEndian(&pStationReport->hdr.dstaddr, sizeof(pStationReport->hdr.dstaddr));

                pStationReport->hdr.srcaddr = u16RssiPanId;
                SysUtil_vConvertEndian(&pStationReport->hdr.srcaddr, sizeof(pStationReport->hdr.srcaddr));

                pStationReport->len = 0;
                SysUtil_vConvertEndian(&pStationReport->len, sizeof(pStationReport->len));

                pStationReport->reporttype = APP_LS_REPORT_LIVE;
                pStationReport->seqnum = u8LiveSeqNum++;

                Jbsmac_eWriteData((uint8 *)u32CommonBuf, sizeof(struct nwkhdr) + sizeof(app_header_t) + sizeof(app_LSrfReport_t));
                DBG(i2c_vPrintf("live %d %d\n", u8LiveSeqNum, u8LastLiveAckSeq););
            }

            uint8 tmp = (u8LiveSeqNum > u8LastLiveAckSeq) ? (u8LiveSeqNum - u8LastLiveAckSeq) : (u8LiveSeqNum - u8LastLiveAckSeq);

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
                vAHI_SwReset();
            }

            //reset LEDs state
            LedUtil_vOff(LED_RF);
            //LedUtil_vOff(LED_DATA);

            TimerUtil_eSetTimer(RSSI_PORT_QUERY_EVENT, 3000);
            EventUtil_vUnsetEvent(RSSI_PORT_QUERY_EVENT);
            break;
        }

        case RSSI_REPORT_STATUS_EVENT:
        {
            if(bIsStationUp)
            {
                struct nwkhdr *pNwkHdr = (struct nwkhdr *)u32CommonBuf;
                app_header_t *pHeader = (app_header_t *)(pNwkHdr + 1);
                app_LSrfReport_t *pStationReport = (app_LSrfReport_t *)(pHeader + 1);
                app_LSLocstatus_t* pStatus = (app_LSLocstatus_t*) (pStationReport+1);

                uint8 u8VersionLen = strlen(VERSION)+1;

                if(sizeof(struct nwkhdr) + sizeof(app_header_t) + sizeof(app_LSrfReport_t)+sizeof(app_LSLocstatus_t)+u8VersionLen > 128)
                {
                    EDBG(i2c_vPrintf("RPT len err!\n"););
                    break;
                }


                pNwkHdr->type = NWK_DATA;
                pNwkHdr->ttl = 1;

                pNwkHdr->src = u16RssiPanId;
                SysUtil_vConvertEndian(&pNwkHdr->src, sizeof(pNwkHdr->src));

                pNwkHdr->dst = u16ArmId;
                SysUtil_vConvertEndian(&pNwkHdr->dst, sizeof(pNwkHdr->dst));

                pNwkHdr->len =  sizeof(app_header_t) + sizeof(app_LSrfReport_t)+sizeof(app_LSLocstatus_t)+u8VersionLen;
                SysUtil_vConvertEndian(&pNwkHdr->len, sizeof(pNwkHdr->len));

                pHeader->len = sizeof(app_LSrfReport_t)+sizeof(app_LSLocstatus_t)+u8VersionLen;
                SysUtil_vConvertEndian(&pHeader->len, sizeof(pHeader->len));

                pHeader->msgtype = APP_TOF_MSG_REPORT;
                pHeader->protocoltype = APP_PROTOCOL_TYPE_CARD;

                pStationReport->hdr.dstaddr = u16ArmId;
                SysUtil_vConvertEndian(&pStationReport->hdr.dstaddr, sizeof(pStationReport->hdr.dstaddr));
                pStationReport->hdr.srcaddr = u16RssiPanId;
                SysUtil_vConvertEndian(&pStationReport->hdr.srcaddr, sizeof(pStationReport->hdr.srcaddr));

                pStationReport->len = sizeof(app_LSLocstatus_t)+u8VersionLen;
                SysUtil_vConvertEndian(&pStationReport->len, sizeof(pStationReport->len));

                pStationReport->reporttype = APP_LS_REPORT_STATUS_LOC;
                pStationReport->devtype = BSMAC_DEVICE_TYPE_LOC;
                pStationReport->seqnum = u8ReportStatusNum++;

                pStatus->station_type = APP_LS_RSSI_STATION;

                pStatus->loc_distance[0] = 0;
                pStatus->loc_distance[1] = 0;
                pStatus->loc_id[0] = 0;
                pStatus->loc_id[1] = 0;
                pStatus->comm_channel = u8RssiChannel;
                pStatus->tof_channel=0;
                pStatus->loc_channel=0;

                /* uart0  */
                pStatus->port[0].neighborid = Jbsmac_u16GetPeerAddr(0);
                pStatus->port[0].livestat = Jbsmac_u8GetLinkStatus(0);
                Jbsmac_vGetErrCnt(0, &pStatus->port[0].total_cnt , &pStatus->port[0].lost_cnt);
                SysUtil_vConvertEndian(&pStatus->port[0].neighborid, sizeof(pStatus->port[0].neighborid));
                SysUtil_vConvertEndian(&pStatus->port[0].livestat, sizeof(pStatus->port[0].livestat));
                SysUtil_vConvertEndian(&pStatus->port[0].total_cnt, sizeof(pStatus->port[0].total_cnt));
                SysUtil_vConvertEndian(&pStatus->port[0].lost_cnt, sizeof(pStatus->port[0].lost_cnt));

                /* uart1 */
                pStatus->port[1].neighborid = Jbsmac_u16GetPeerAddr(1);
                pStatus->port[1].livestat = Jbsmac_u8GetLinkStatus(1);
                Jbsmac_vGetErrCnt(1, &pStatus->port[1].total_cnt , &pStatus->port[1].lost_cnt);
                SysUtil_vConvertEndian(&pStatus->port[1].neighborid, sizeof(pStatus->port[1].neighborid));
                SysUtil_vConvertEndian(&pStatus->port[1].livestat, sizeof(pStatus->port[1].livestat));
                SysUtil_vConvertEndian(&pStatus->port[1].total_cnt, sizeof(pStatus->port[1].total_cnt));
                SysUtil_vConvertEndian(&pStatus->port[1].lost_cnt, sizeof(pStatus->port[1].lost_cnt));

                pStatus->oad_version = OAD_LOC_STATION_VERSION;
                pStatus->len = u8VersionLen;
                SysUtil_vConvertEndian(&pStatus->oad_version, sizeof(pStatus->oad_version));
                SysUtil_vConvertEndian(&pStatus->len, sizeof(pStatus->len));

                strcpy((uint8*)(pStatus+1), VERSION);

                uint16 u16SendLen = sizeof(struct nwkhdr) + sizeof(app_header_t) + sizeof(app_LSrfReport_t)+sizeof(app_LSLocstatus_t)+u8VersionLen;

                Jbsmac_eWriteData((uint8 *)u32CommonBuf,u16SendLen);

                //DBG(
                //    i2c_vPrintf("Send status report\n");
                //    //PrintfUtil_vPrintMem((uint8 *)u32CommonBuf, u16SendLen);
                //);
            }

            TimerUtil_eSetTimer(RSSI_REPORT_STATUS_EVENT, 120000);
            EventUtil_vUnsetEvent(RSSI_REPORT_STATUS_EVENT);
            break;
        }


        case RSSI_REPORT_CARDVERSION:
        {
            vReportCardVersion();
            TimerUtil_eSetTimer(RSSI_REPORT_CARDVERSION, 60000);
            EventUtil_vUnsetEvent(RSSI_REPORT_CARDVERSION);
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
#ifdef  DEBUG_FULL_LOAD
#else
            vHandleMcpsDataInd(psMcpsInd);
#endif
            break;
        case MAC_MCPS_DCFM_DATA: /* Incoming acknowledgement or ack timeout */
            vHandleMcpsDataDcfm(psMcpsInd);
            break;
        default:
            break;
        }
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
        TimerUtil_eSetTimer(RSSI_REPORT_CARDVERSION, 1);
    }
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
 * NAME: vHandleMcpsDataInd
 *
 ****************************************************************************/
PRIVATE void vHandleMcpsDataInd(MAC_McpsDcfmInd_s *psMcpsInd)
{
    MAC_RxFrameData_s *psFrame;
    psFrame = &psMcpsInd->uParam.sIndData.sFrame;

    RfTofWrapper_tu* psAppPkt = (RfTofWrapper_tu*)(psFrame->au8Sdu);

#ifdef  DEBUG_FULL_LOAD
    static uint16 virtual_u16SeqNum = 0;
    static uint8 virtual_status = 1;
    uint8 i=0;

    for(; i<APP_MAX_CARD_NUM; i++)
    {
        debug_begin++;
        virtual_status= virtual_status<<1;
        if(virtual_status&0x08)virtual_status=0x01;
        vCheckCardStatus(debug_begin, virtual_status);

        app_rssi_data.tof_rssi[u16RssiNum].u16ShortAddr = debug_begin;
        app_rssi_data.tof_rssi[u16RssiNum].u16SeqNum = virtual_u16SeqNum;
        app_rssi_data.tof_rssi[u16RssiNum].i8Rssi = 0;
        app_rssi_data.tof_rssi[u16RssiNum].u8RssiType = APP_TOF_CARD_RSSI;
        app_rssi_data.tof_rssi[u16RssiNum].u16Reserved = 0;

        //DBG(i2c_vPrintf("%X: %X, %i\n", debug_begin, virtual_u16SeqNum, virtual_status););

        u16RssiNum++;
        if(debug_begin>=(debug_card_num+debug_card_base-1))
        {
            DBG(i2c_vPrintf("finish sent\n"););
            vReportCardRssi();
            bReported = TRUE;
            debug_begin=(uint16)(debug_card_base-1);
            virtual_u16SeqNum++;

            //reset timer
            TimerUtil_eSetTimer(RSSI_REPORT_DATA_EVENT, 500);
            EventUtil_vUnsetEvent(RSSI_REPORT_DATA_EVENT);
            break;
        }

        if(APP_MAX_CARD_NUM == u16RssiNum)
        {
            DBG(i2c_vPrintf("report rssi\n"););
            vReportCardRssi();
            bReported = TRUE;

            //reset timer
            TimerUtil_eSetTimer(RSSI_REPORT_DATA_EVENT, 500);
            EventUtil_vUnsetEvent(RSSI_REPORT_DATA_EVENT);
        }

        LedUtil_vToggle(LED_RF);
    }
#else
    if (APP_PROTOCOL_TYPE_CARD == psAppPkt->tof_head.protocoltype)
    {
        //if((psAppPkt->tof_head.msgtype == TOF_CARD_RSSI)
                //&& ((psFrame->sDstAddr.u16PanId == 0xFFFF) || (psFrame->sDstAddr.u16PanId == u16RssiPanId)))    // it's not send to other station
        if((psFrame->sDstAddr.u16PanId == 0xFFFF) || (psFrame->sDstAddr.u16PanId == u16RssiPanId))
        {
            if(psAppPkt->tof_head.msgtype == TOF_CARD_RSSI)
            {
                // check len: rf_tof_card_data_ts's len
                if(psFrame->u8SduLength != sizeof(rf_tof_card_data_ts)) return;

                vCheckCardStatus(psFrame->sSrcAddr.uAddr.u16Short, psAppPkt->rf_tof_card_data.u8CardStatus & (~CARD_STATUS_EXCITER));

                DBG(i2c_vPrintf("%X: %X, %i\n", psFrame->sSrcAddr.uAddr.u16Short, psAppPkt->rf_tof_card_data.u16SeqNum, SysUtil_vConvertLQI2Dbm(psFrame->u8LinkQuality)););


                app_rssi_data.tof_rssi[u16RssiNum].u16ShortAddr = psFrame->sSrcAddr.uAddr.u16Short;
                app_rssi_data.tof_rssi[u16RssiNum].u16SeqNum = psAppPkt->rf_tof_card_data.u16SeqNum;
                app_rssi_data.tof_rssi[u16RssiNum].i8Rssi = SysUtil_vConvertLQI2Dbm(psFrame->u8LinkQuality);
                app_rssi_data.tof_rssi[u16RssiNum].u8RssiType = APP_TOF_CARD_RSSI;
                app_rssi_data.tof_rssi[u16RssiNum].u8Reserved = 0;
                app_rssi_data.tof_rssi[u16RssiNum].u8Accel = psAppPkt->rf_tof_card_data.u8ExciterIDorAccStatus;

                u16RssiNum++;
                if(APP_MAX_CARD_NUM == u16RssiNum)
                {
                    DBG(i2c_vPrintf("report rssi\n"););
                    vReportCardRssi();
                    bReported = TRUE;

                    //reset timer
                    TimerUtil_eSetTimer(RSSI_REPORT_DATA_EVENT, 500);
                    EventUtil_vUnsetEvent(RSSI_REPORT_DATA_EVENT);
                }
                LedUtil_vToggle(LED_RF);
            }
            else if(psAppPkt->tof_head.msgtype == TOF_CARD_ALARM)
            {
                vReportCardExcite(psFrame->sSrcAddr.uAddr.u16Short,\
                    psAppPkt->rf_tof_card_data.u8CardStatus,psAppPkt->rf_tof_card_data.u8ExciterIDorAccStatus);
            }

            if( psAppPkt->tof_head.msgtype == TOF_CARD_REQUEST || psAppPkt->tof_head.msgtype == TOF_CARD_RSSI
            || psAppPkt->tof_head.msgtype == TOF_CARD_ALARM || psAppPkt->tof_head.msgtype == TOF_CARD_CHECKIN )
            {
                if(psFrame->u8SduLength == sizeof(rf_tof_card_data_ts) && psAppPkt->rf_tof_card_data.u16OadVersion >0 )
                {
                    vProcessCardVersion(psFrame->sSrcAddr.uAddr.u16Short, DEVICE_TYPE_CARD5S, psAppPkt->rf_tof_card_data.u16OadVersion, psAppPkt->rf_tof_card_data.u16Battery);
                }
            }

        }
    }
#endif
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
    sMlmeReqRsp.uParam.sReqStart.u16PanId = u16RssiPanId;
    sMlmeReqRsp.uParam.sReqStart.u8Channel = u8RssiChannel;
    sMlmeReqRsp.uParam.sReqStart.u8BeaconOrder = 0x0F;
    sMlmeReqRsp.uParam.sReqStart.u8SuperframeOrder = 0x0F;
    sMlmeReqRsp.uParam.sReqStart.u8PanCoordinator = TRUE;
    sMlmeReqRsp.uParam.sReqStart.u8BatteryLifeExt = FALSE;
    sMlmeReqRsp.uParam.sReqStart.u8Realignment = FALSE;
    sMlmeReqRsp.uParam.sReqStart.u8SecurityEnable = FALSE;

    vAppApiMlmeRequest(&sMlmeReqRsp, &sMlmeSyncCfm);
}



/****************************************************************************
 *
 * NAME: vRssiStationCast
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
PRIVATE void vRssiStationCast(uint8 u8CmdType, uint16 u16DstAddr, uint16 u16DstPanId, uint8 u8TxOptions, uint8 u8Val)
{
    MacUtil_SendParams_s sParams;
    sParams.u8Radius        = 1;
    sParams.u16DstAddr    = u16DstAddr;
    sParams.u16DstPanId     = u16DstPanId;
    sParams.u16ClusterId     = 0;
    sParams.u8DeliverMode    = MAC_UTIL_UNICAST;

    RfTofWrapper_tu RfTofData;
    RfTofData.tof_head.protocoltype = APP_PROTOCOL_TYPE_CARD;
    RfTofData.tof_head.msgtype = u8CmdType;
    RfTofData.tof_head.len = 0;//rf_tof_station_accept_ts no use

    switch (u8CmdType)
    {
    case TOF_STATION_RSSI:
    {
        RfTofData.rf_tof_station_signal.u8AvailableMs = u8Val;    // card will send request in random (0 ~ u8Val) ms
        RfTofData.rf_tof_station_signal.u8StationStatus = tRssiStationStatus;
        RfTofData.rf_tof_station_signal.u16CurSlot = 0;
        RfTofData.rf_tof_station_signal.u8RunMs = 0;
        RfTofData.rf_tof_station_signal.u8LocIdle = 0;
        RfTofData.tof_head.len = 6;    //this is not a 4-byte struct !!!!!!!!!!!
        break;
    }

    default:
        break;
    }

    MacUtil_vSendData(&sParams, (uint8*)&RfTofData, RfTofData.tof_head.len+sizeof(app_header_t), u8TxOptions);
}


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
    /*DBG(
        i2c_vPrintf("UR\n", len);
        uint8 u8I;
        for(u8I = 0; u8I < len; u8I++)
        i2c_vPrintf("%X ", pbuf[u8I]);
        i2c_vPrintf("\n");
    );*/

    if(len < sizeof(struct nwkhdr) + sizeof(app_header_t))
        return;

    RfTofWrapper_tu* psAppPkt = (RfTofWrapper_tu*)(pbuf + sizeof(struct nwkhdr));

    app_header_t *pheader =  (app_header_t *)((uint8 *)pbuf + sizeof(struct nwkhdr));
    uint8 		*pPayload =	(uint8 *)(pheader + 1);

    // not for me
    if(APP_PROTOCOL_TYPE_CARD != psAppPkt->tof_head.protocoltype)
        return;

    switch(psAppPkt->tof_head.msgtype)
    {
    case APP_TOF_MSG_RETREAT:
    {
        tRssiStationStatus = STATION_STATUS_RETREAT;
        //syncstatus_count=SYNCSTATUS_COUNT;
        break;
    }

    case APP_TOF_MSG_CANCEL_RETREAT:
    {
        tRssiStationStatus = STATION_STATUS_NORMAL;
        syncstatus_count=SYNCSTATUS_COUNT;
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
            vRssiStationCast(TOF_APP_HELP_ACK, app_tof_alarm_ack->u16ShortAddr, TOF_CARD_NWK_ADDR, 0, 0);
        }
        break;
    }

    case MPRF_REPORT_ACK:
    {
        app_LSrfReport_t *prfReport = (app_LSrfReport_t *)pPayload;
        if( prfReport->reporttype == APP_LS_REPORT_LIVE)
        {
            u8LastLiveAckSeq = prfReport->seqnum;
            bIsStationUp = TRUE;
            u8RestartCounter = 0;
        }
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

        if((prfSet->hdr.dstaddr == u16RssiPanId || prfSet->hdr.dstaddr == 0xFFFF)
                && pheader->len  >=  (sizeof(app_LSrfSet_t) + sizeof(app_rfTlv_t))) //at least one tlv
        {
            uint_16 crc;
            crc = CRC16((uint8*)(prfSet + 1),pheader->len-sizeof(app_LSrfSet_t) , 0xFFFF);
            if(crc == prfSet->crc)
            {
                uint16 len = pheader->len - sizeof(app_LSrfSet_t);  // sum len of all tlvs

                app_rfTlv_t *pTlv = (app_rfTlv_t *)(prfSet+1);

                bool  changedChannel = FALSE;
                uint8 u8NewRssiChannel = u8RssiChannel;

                while(len >= (sizeof(app_rfTlv_t) + pTlv->len))
                {
                    uint8 *pdata = (uint8*)(pTlv+1);
                    if(pTlv->type == APP_LS_TLV_TYPE_RSSI_CHANNEL)
                    {
                        if(*pdata != u8RssiChannel)
                        {
                            DBG(i2c_vPrintf("SET \n"););
                            u8NewRssiChannel = *pdata;
                            changedChannel = TRUE;
                        }
                    }

                    /*else if(pTlv->type == APP_LS_TLV_TYPE_SINGLE_CARD_RETREAT)
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
                    }*/
                    len -= (sizeof(app_rfTlv_t)  + pTlv->len);
                    pTlv = (app_rfTlv_t *)(pdata  + pTlv->len);
                }

                if(changedChannel && bChannelValid(u8NewRssiChannel))
                {
                    vSaveChannel(u8NewRssiChannel);
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

    case MODULE_ERROR_RATE_TEST:
    {
        struct nwkhdr *pNwkHdr = (struct nwkhdr *)u32CommonBuf;
        app_header_t *pHeader = (app_header_t *)(pNwkHdr + 1);

        pNwkHdr->type = NWK_DATA;
        pNwkHdr->ttl = 1;

        pNwkHdr->src = u16RssiPanId;
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
    }
    //LedUtil_vToggle(LED_DATA);

    default:
        break;
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
        app_rssi_data.app_tof_head.len = u16RssiNum*sizeof(tof_rssi_ts);

        if(bIsStationUp)
            vWriteData2Arm((uint8*)&app_rssi_data);

        u16RssiNum = 0;
    }
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

    case APP_TOF_MSG_ALARM:
    case APP_TOF_MSG_NEW_ALARM:
    {
        app_tof_alarm_ts* pAlarm_data = (app_tof_alarm_ts*) pbuf;
        vConvertEndian16(&(pAlarm_data->u16ShortAddr));
        break;
    }

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

    /*DBG(
        uint16 u16Tmp;
        uint8 u8Data[120];
        memcpy(u8Data, (uint8*)pnwk_data, nwk_len);
        for(u16Tmp = 0; u16Tmp < nwk_len; u16Tmp++)
        i2c_vPrintf("%X ", u8Data[u16Tmp]);
        i2c_vPrintf("\nLen: %d\n", nwk_len);
    );*/

    Jbsmac_eWriteData((uint8*)pnwk_data, nwk_len);
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
    uint8 teStatus = APP_TOF_ALARM_NONE;

    /*  not check mismatching status */
    /*if(u8Status & CARD_STATUS_RETREAT)
    {
        if(tRssiStationStatus!=STATION_STATUS_RETREAT)
        {
            syncstatus_count=SYNCSTATUS_COUNT;
        }
    }*/

    //teStatus = u8Status;
    if(u8Status & CARD_STATUS_HELP)
    {
        //teStatus = APP_TOF_ALARM_CARD_HELP;
        teStatus = u8Status;
        vRssiStationCast(TOF_STATION_HELP_ACK, u16ShortAddr, TOF_CARD_NWK_ADDR, 0, 0);
    }
    else if(u8Status & CARD_STATUS_NOPWD)
    {
        //teStatus = APP_TOF_ALARM_CARD_NOPWD;
        teStatus = u8Status;
        vRssiStationCast(TOF_STATION_NOPWD_ACK, u16ShortAddr, TOF_CARD_NWK_ADDR, 0, 0);
    }
    else if(u8Status & CARD_STATUS_RETREAT_ACK)
    {
        //teStatus = APP_TOF_RETREAT_ACK;
        teStatus = u8Status;
    }
    else
    {
        return;
    }

    vReportCardAlarm(u16ShortAddr, teStatus);
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
PRIVATE void vReportCardAlarm(uint16 u16ShortAddr, uint8 alarm_type)
{
    app_tof_alarm_ts app_tof_alarm_data;

    app_tof_alarm_data.app_tof_head.protocoltype = APP_PROTOCOL_TYPE_CARD;
    app_tof_alarm_data.app_tof_head.msgtype = APP_TOF_MSG_NEW_ALARM;
    app_tof_alarm_data.app_tof_head.len = 4;
    app_tof_alarm_data.u16ShortAddr = u16ShortAddr;
    app_tof_alarm_data.u8Status = alarm_type;

    if(bArmReady())
        vWriteData2Arm((uint8*)&app_tof_alarm_data);
}

PRIVATE void vReportCardExcite(uint16 u16ShortAddr, uint8 u8CardStatus,uint8 u8ExciteID)
{
    app_tof_alarm_ts app_tof_alarm_data;

    app_tof_alarm_data.app_tof_head.protocoltype = APP_PROTOCOL_TYPE_CARD;
    app_tof_alarm_data.app_tof_head.msgtype = APP_TOF_MSG_NEW_ALARM;
    app_tof_alarm_data.app_tof_head.len = 4;
    app_tof_alarm_data.u16ShortAddr = u16ShortAddr;
    app_tof_alarm_data.u8Status = u8CardStatus;
    app_tof_alarm_data.u8ExciterID = u8ExciteID;

    if(bArmReady())
        vWriteData2Arm((uint8*)&app_tof_alarm_data);
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
PRIVATE bool bChannelValid(uint8 u8BroadcastChannel)
{
    //fix me:
    //the 26 channel have some bug and do not use in this project
    if(u8BroadcastChannel>=11 && u8BroadcastChannel<=25)
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
PRIVATE bool  vSaveChannel(uint8 u8Channel)
{
    uint8 u8rssichannel;
    if(!bAHI_FullFlashRead(PARAM_STORE_ADDR, 1,(uint8*)&u8rssichannel))
    {
        EDBG(i2c_vPrintf("Flash Read Fail\n"););
        return FALSE;
    }

    if(!bAHI_FlashEraseSector(7))
    {
        EDBG(i2c_vPrintf("Flash Erase Fail\n"););
        return FALSE;
    }

    if(bAHI_FullFlashProgram(PARAM_STORE_ADDR, 1,(uint8*)&u8Channel))
    {
        EDBG(i2c_vPrintf("Set  channel Success!%d\n", u8Channel););
        return TRUE;
    }
    else
    {
        EDBG(i2c_vPrintf("Set  channel fail!%d\n",u8Channel););
        return FALSE;
    }
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




