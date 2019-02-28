
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
#include "numtrans.h"
#include "LightUart.h"
#include "JN5148_util.h"
#include "Hal_Air.h"
#include "string.h"
#include "Light_protocol.h"

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
//#define LED_UART0         E_AHI_DIO9_INT
#define LED_UART1         E_AHI_DIO8_INT
#define LED_RF             E_AHI_DIO9_INT  //DIO 14 is used for i2c,remove it to unused DIO16



#define TOF_BUZZER_PIN  E_AHI_DIO13_INT

#define TOF_BUZZER_TIMER	 E_AHI_TIMER_3	// for buzzer's input


#define E_SPI_CLOCK_2M  8 // 8*16M/64
#define E_SPI_MSB_FIRST FALSE





//FIX me, DIO14 is used for I2C, Use DIO16 instead
//#define LED_LINK         E_AHI_DIO14_INT

/* app envents */
#define LIGHT_REPORTACK_STATUS_EVENT    BIT(2)
#define LIGHT_REPORT_RESET_EVENT    BIT(3)
#define LIGHT_REPORT_RSSI_EVENT    BIT(4)
#define LIGHT_REPORT_DATA_EVENT       BIT(5)
#define LIGHT_REPORT_LIGHTVERSION   BIT(6)
#define LIGHT_LISTEN_EVENT     BIT(7)
#define LIGHT_LISTEN_TIMEOUT_EVENT   BIT(8)
#define LIGHT_REPORT_DEPTH_EVENT   BIT(9)
#define LIGHT_REPORT_VOL_CUR_EVENT   BIT(11)
#define LIGHT_RANDOM_REPORT_EVENT   BIT(12)
#define LIGHT_DOWN_CAST_EVENT   BIT(13)
#define LIGHT_CHANGE_CHANNEL_EVENT  BIT(14)
#define LIGHT_CAST_CHANNEL_SETTING_EVENT  BIT(15)
#define LIGHT_CAST_DEPTH_EVENT BIT(16)
#define LIGHT_LED_EVENT BIT(17)
#define LIGHT_LED_OFF_EVENT BIT(18)





#define STATION_BUZZER_EVENT				BIT(27)	// card buzzer
#define STATION_STOP_BUZZER_EVENT			BIT(28)	// stop buzzer




#define PARAM_STORE_ADDR 	0x38000
#define   LIGHT_FEED_WATCHDOG_EVENT  BIT(10)
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

typedef enum
{
    E_LIGHT_STATE_IDLE,					// nothing to do
    E_LIGHT_STATE_LISTEN,				// 离网时监听基站信号
    E_LIGHT_STATE_WORK,
    //E_LIGHT_STATE_SYNC,					// card sync
    //E_LIGHT_STATE_RSSI,					// card send rssi
    //E_LIGHT_STATE_REQUEST_STATION,	// card send join request
    //E_LIGHT_STATE_STATION_ACCEPT,		// station accept card's join request
    //E_LIGHT_STATE_WAIT_STATION_TOF,	// wait station to do tof
    //E_LIGHT_STATE_STATION_FINISH,		// station finish tof with card
    //E_LIGHT_STATE_STATION_WAIT,		// station finish tof with card
    //E_LIGHT_STATE_WAIT_LOCATOR,		// wait locator to do tof
    //E_LIGHT_STATE_LOCATOR_FINISH,		// locator finish tof with card
    //E_LIGHT_STATE_ALARM,				// card alarm
    //E_LIGHT_STATE_WAIT_ALARM_ACK,		// card wait alarm's ack
    //E_LIGHT_STATE_CHANGE_PERIOD,      // card wait change work period
    //E_LIGHT_STATE_SELECT_STATION,     //card select station
} teLightState;

typedef struct
{
    //uint8 u8Status;
    //uint8 u8CardType;
    uint8 u8Channel;
    uint8 u8Reserved;	
    uint16 u16ShortAddr;
    
    //uint8 u8RssiCh;
}CFG_OPTION_T;

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

PRIVATE BOOTSH_T _bootsh;
PRIVATE BOOTSH_T *bootsh = &_bootsh;

PRIVATE CFG_OPTION_T cfg_option;
PRIVATE uint8 u8LastHandle;



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
//PRIVATE void vProcessStationCmd(MAC_McpsIndData_s *psMcpsInd);
PRIVATE void vUartRxCallBack(unsigned char *pbuf, unsigned char len);
PRIVATE void  vStackCheck(void);
//PRIVATE void vSyncLEDStatus(void);
//PRIVATE void reset_report_rssi(void);
//PRIVATE void update_station_rssi(uint16 u16Short,int8 receivei8Rssi,int8 senti8Rssi,uint8 msgtype);
//PRIVATE void vReportCardRssi(void);
PRIVATE void vConvertEndian16(uint16* pu16Data);
//PRIVATE void vTransferThroughUpData();
PRIVATE void  vSaveChannel(uint8 u8Channel);
PRIVATE bool bChannelValid(uint8 u8BroadcastChannel);

PRIVATE void vLightCast(uint8 u8CmdType, uint16 u16DstAddr, uint16 u16DstPanId, uint8 u8TxOptions, uint8 u8Val);
PRIVATE bool bReportedDepth(void);

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
//PRIVATE bool_t bIsStationUp = FALSE;


//PRIVATE uint8 u8LiveSeqNum = 0;
//PRIVATE uint8 u8LastLiveAckSeq = 0;

//PRIVATE uint8 u8RestartCounter=0;

/*use u32 for alignment*/
PRIVATE uint32 u32CommonBuf[128];

PRIVATE uint8 u8CommonRfBuf[128];

PRIVATE uint8 u8UpstreamBuf[128];


PRIVATE rf_light_vol_cur_upstream_ts* pvol_cur_upstream = (rf_light_vol_cur_upstream_ts*)u8CommonRfBuf;
PRIVATE bool_t bMsgSending = FALSE;    // if the msg is still sending or not

PRIVATE uint32 u32Data2Arm[30];    // the data send to arm, max len is 114 (BSMAC_MAX_TX_PAYLOAD_LEN), 30*4 = 120 is enough (28 is enough)
PRIVATE struct nwkhdr* pnwk_data = (struct nwkhdr*)u32Data2Arm;


PRIVATE uint32 u32LastAirRxTimeMs = 0;
PRIVATE bool_t  bResetFromWarmStart = 0;
//PRIVATE uint16 u16RssiNum = 0;
PRIVATE app_tof_rssi_ts app_rssi_data;
//PRIVATE app_transfer_through_ts app_transfer_through_data;

//PRIVATE bool bReported = FALSE;

//PRIVATE app_LSVersionReport_t tsVersionReport[APP_TOF_VERSION_MAX_NUM];
//PRIVATE uint8  			      tsVersionReportLen;

PRIVATE app_rssi_report  rssiReport;
//PRIVATE int_8 i8MpRcvRssi;
//PRIVATE uint8 u8StationPort;

PRIVATE uint8 au8UartRxBuffer[100];

#define LN_MIN_RSSI -120

#define LN_CELL_THREHOLD1   (-35)
#define LN_CELL_THREHOLD2   (-60)
#define MP_CELL_DIFFRSSI	    6   
#define COOR_INVALIDADDR 0xFFFF

#define NWK_INVALID_DEPTH               0xFF

#define UART_INVALID_VALUE             0xFF
//#define NWK_STATION_ADDRESS             0x0
#define NWK_INVALID_ADDRESS             0xFFFF

#define NWK_WINDOW_DURATION        6000

#define OPTION_CARDNUM     "cardnum="
#define OPTION_CHANNEL     "channel="
//#define OPTION_CARDTYPE    "cardtype="
//#define OPTION_RSSICH      "rssich="
#define OPTION_REBOOT   "reboot"
#define OPTION_SHOW     "show"
#define OPTION_HELP     "help"
#define OPTION_SAVEENV  "saveenv"


//PRIVATE uint32 current_vol;

//PRIVATE uint8 u8Pwmpare = 0;

PRIVATE uint8 u8IsRssiStation = 0;   //默认不支持上报rssi

PRIVATE uint8 u8MaxLightLevel = UART_INVALID_VALUE;
PRIVATE uint8 u8MinLightLevel = UART_INVALID_VALUE;

//PRIVATE uint16 u16FatherNode = 0;
//PRIVATE uint16 u16NodeCost = 0xFFFF;;

PRIVATE uint8 u8ListenCnt= 0;


PRIVATE int8 i8Rssi_Threshold = 0;
//PRIVATE int8 i8CurrentRssi ;
PRIVATE uint8 u8Vrms;
PRIVATE uint8 u8Irms;
PRIVATE uint8 u8RssiChannel;
//PRIVATE uint16 u16FatherNode;
PRIVATE uint8 u8LightState = E_LIGHT_STATE_LISTEN;

PRIVATE bool bIsCharging = FALSE;

PRIVATE uint8 u8NewLocChannel;



rf_light_parmeter_data_ts light_parmeter;

app_light_channel_ts light_channel;


uint32 u32CastType = 0;

typedef struct {
  uint16    address;
  uint8     depth;
  uint8     cost;
  uint16    accucost;
  uint16    parent;
  uint16     failcount;
  uint16    successtime;
  uint32 updatedepthtime;
  uint16    pathtoroot[MAX_TREE_DEPTH];
}nwk_info_t;

#define LIGHT_RSSI_THRESHOLD           -85
const  uint8 rssi2cost[] = {
  255u, 233u, 212u, 192u, 175u, 159u, 145u, 131u,
  119u, 109u, 99u,  90u,  82u,  74u,  67u,  61u,
  56u,  51u,  46u,  42u,  38u,  35u,  31u,  29u,
  26u,  24u,  21u,  20u,  18u,  16u,  15u,  13u,
  12u,  11u,  10u,  9u,   8u,   8u,   7u,   6u,
  6u,   5u,   5u,   4u,   4u,   4u,   3u,   3u,
  3u,   2u,   2u,   2u,   2u,   2u,   1u,   1u
};
#define LIGHT_RSSI_TABLE_SIZE (sizeof(rssi2cost)/sizeof(uint8))    //56


PRIVATE nwk_info_t NwkInfo;

PRIVATE nwk_info_t NwkInfoTemp;


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
	uint_8 random_time;
	vInitSystem();
	vStartCoordinator();
	vCheckSetting();
	TimerUtil_eSetCircleTimer(LIGHT_FEED_WATCHDOG_EVENT, 10000);

	//EventUtil_vSetEvent(LIGHT_LISTEN_EVENT);

	random_time = SysUtil_u16GenRndNum()%20;

	EventUtil_vSetEvent(LIGHT_REPORT_LIGHTVERSION);
	EventUtil_vSetEvent(LIGHT_REPORT_DEPTH_EVENT);
	EventUtil_vSetEvent(LIGHT_LED_EVENT);
	//TimerUtil_eSetCircleTimer(LIGHT_REPORT_DEPTH_EVENT,40000+random_time);
	
	TimerUtil_eSetCircleTimer(LIGHT_REPORT_VOL_CUR_EVENT,60000+random_time);

	/* update last air rx time*/
	Hal_TimePoll();
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
	PrintfUtil_vPrintf("warm start. \n\r");
#endif
	AppColdStart();
}

/****************************************************************************/
/***        Local Functions                                               ***/
/****************************************************************************/
PRIVATE void vProcessPoll(void)
{
	//Jbsmac_vPoll();
	TimerUtil_vUpdate();
	Hal_TimePoll();
	Hal_AirPoll();
	//vStackCheck();
	//#if (defined DEBUG_COM_APP || defined DEBUG_ERROR)
	//i2c_vPrintPoll();
	//#endif
}

PRIVATE void vUartTxCallBack(void)
{
    // must delay 1ms after send
    //JN5148 finished before  485
    TimerUtil_vDelay(1000, E_TIMER_UNIT_MICROSECOND);
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
    //PrintfUtil_vPrintf("*         cardtype=?  (Base 1s、5s、15s)     *\n");
    //PrintfUtil_vPrintf("*         rssich=?  (rssi channel)     *\n");
    PrintfUtil_vPrintf("*         saveenv (save configuration)       *\n");
    PrintfUtil_vPrintf("*         show    (show config)              *\n");
    PrintfUtil_vPrintf("*         reboot  (reboot)                   *\n");
    PrintfUtil_vPrintf("*         help    (display this window)      *\n");
    PrintfUtil_vPrintf("* example:                                   *\n");
    PrintfUtil_vPrintf("*         bootsh>>cardnum=25555              *\n");
    PrintfUtil_vPrintf("*         bootsh>>channel=25                 *\n");
    //PrintfUtil_vPrintf("*         bootsh>>cardtype=0                 *\n");
    PrintfUtil_vPrintf("******************************************   *\n");
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
    PrintfUtil_vPrintf("\ncardnum=%d\n",u16StationPanId);
    //PrintfUtil_vPrintf("\ncardtype=%d\n",tofPeriodType);
    PrintfUtil_vPrintf("\nchannel=%d\n",u8Channel);
    //PrintfUtil_vPrintf("\nrssi channel=%d\n",u8RssiChannel);
}

void _help_process(char *option, const char *val_pos, uint32* pVal)
{
    if (!val_pos || *val_pos != ';')
        return;

    vBootsh_show_menu();
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
        if (((id > 29999) || (id < 20000)) && (!strcasecmp(option, OPTION_CARDNUM)))
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

PRIVATE void _save_configuration(char *option, const char *val_pos, uint32* pVal)
{
    if (!val_pos || *val_pos != ';')
        return;


    bAHI_FlashInit(E_FL_CHIP_INTERNAL, NULL);

    if(!bAHI_FlashEraseSector(7))
    {
        PrintfUtil_vPrintf("Flash Erase Fail\n");
        return;
    }

    if(bAHI_FullFlashProgram(PARAM_STORE_ADDR, sizeof(CFG_OPTION_T),(uint8*)&cfg_option))
    {
        PrintfUtil_vPrintf("finish saving  !\n");

        u16StationPanId = cfg_option.u16ShortAddr;

        u8Channel = cfg_option.u8Channel;
        return;
    }
    else
    {
        PrintfUtil_vPrintf("Set  CardStatus fail!\n");
        return;
    }
}

PRIVATE bool vGet_flashparam(void)
{
    if(!bAHI_FullFlashRead(PARAM_STORE_ADDR, sizeof(CFG_OPTION_T),(uint8*)&cfg_option))
    {
        PrintfUtil_vPrintf("Flash Read Fail\n");
        return FALSE;
    }

    u16StationPanId = cfg_option.u16ShortAddr;
    u8Channel = cfg_option.u8Channel;
    return TRUE;
}

PRIVATE void vBootsh_run_line(char* cmd, int cmdlen)
{
    char *pos, *pre;
    int i, cmd_len;
    int cmd_cnt = 0;

    BOOTSH_CMD_T cmd_process[] = {
        {OPTION_CARDNUM, _cardnum_process, NULL},
        {OPTION_CHANNEL, _channel_process, NULL},
        //{OPTION_CARDTYPE, _cardtype_process, NULL},
        //{OPTION_RSSICH, _rssich_process, NULL},
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

PRIVATE void vGet_MACAddress(void)
{
    MacUtil_vReadExtAddress(&psMacAddr);
    uint32 tmp32;
    tmp32 = StringUtil_BCD2uint32(psMacAddr.u32H);
    if(u16StationPanId > 65000)
    {
		u16StationPanId = tmp32%100000;
		//vCardShortAddrCheck(u16CardShortAddr,psMacAddr.u32H);
    }
   
    if(u8Channel < 11 || u8Channel > 26)
    {
		uint8 channel = ((psMacAddr.u32L) >> 16) & 0xFF;
		if(channel >=11 && channel <=26)
		{
			u8Channel = channel;
		}
		else
		{
			u8Channel = 19;
		}
    }
}





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
PRIVATE void vInitSystem(void)
{
	(void)u32AppQApiInit(NULL, NULL, NULL); //vHwDeviceIntCallback);
	(void)u32AHI_Init();

	vAHI_UartSetRTSCTS(E_AHI_UART_0,FALSE);
	PrintfUtil_u8Init(E_AHI_UART_0, PRINTF_BAUD_RATE_1M,au8UartRxBuffer,sizeof(au8UartRxBuffer));
	PrintfUtil_vPrintf("cold start\n");

	bAHI_FlashInit(E_FL_CHIP_INTERNAL, NULL);	
	

	vAHI_DioSetDirection(0, E_AHI_DIO18_INT);   //init hardware watchdog GPIO
	vAHI_DioSetOutput(E_AHI_DIO18_INT,0);

	vAHI_DioSetPullup(0, TOF_BUZZER_PIN);

	vAHI_DioSetDirection(0, LED_RF);
	

	LedUtil_bRegister(LED_RF);
	//LedUtil_bRegister(LED_UART0);
	//LedUtil_bRegister(LED_UART1);

	ErrorUtil_vRegisterLed0(LED_RF);
	//ErrorUtil_vRegisterLed1(LED_UART0);
	//ErrorUtil_vRegisterLed2(LED_UART1);

	/* if restart from warm start, continue the led status*/
	/*if(bResetFromWarmStart)
	{
		vSyncLEDStatus();
	}*/
	/*
	vAHI_ProtocolPower(TRUE);
	vAHI_BbcSetHigherDataRate(E_AHI_BBC_CTRL_DATA_RATE_500_KBPS);
	vAHI_BbcSetInterFrameGap(50);
	*/


	NwkInfo.parent = NWK_INVALID_ADDRESS;
	NwkInfo.depth = NWK_INVALID_DEPTH;

	NwkInfoTemp.accucost = 0xFFFF;
	NwkInfoTemp.parent = NWK_INVALID_ADDRESS;
	NwkInfoTemp.depth = NWK_INVALID_DEPTH;
	
	app_rssi_data.app_tof_head.protocoltype = APP_PROTOCOL_TYPE_CARD;
	app_rssi_data.app_tof_head.msgtype = APP_TOF_MSG_RSSI;


	//app_transfer_through_data.transfer_hdr.hdr.protocoltype = APP_PROTOCOL_TYPE_THROUGH;
	//app_transfer_through_data.transfer_hdr.hdr.msgtype= APP_TRANSFER_THROUGH;
	//app_transfer_through_data.transfer_hdr.hdr.len=58;


	// Enable high power modules, tof function, timerUtil
	vAHI_HighPowerModuleEnable(TRUE, TRUE);
	TimerUtil_vInit();

	bMsgSending = FALSE;

	//vAHI_WakeTimerEnable(E_AHI_WAKE_TIMER_0, TRUE);

	if(!vGet_bootparam())
	{
	        vBootsh_show_menu();

	        cfg_option.u16ShortAddr = u16StationPanId;
	        cfg_option.u8Channel = u8Channel;

	        if(!vGet_flashparam())
	        {
	            cfg_option.u16ShortAddr = u16StationPanId;
	            cfg_option.u8Channel = u8Channel;
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

	PrintfUtil_vPrintf("ChannelBBBBB:%d\n", u8Channel);


	#if 0
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
		u8Channel = DEFAULT_CHANNEL_LOCATOR;
	}

	//if(bAHI_FullFlashRead(PARAM_STORE_ADDR, sizeof(uint8),(uint8*)&channel)
	        //&& bChannelValid(channel))
	{
		//u8Channel = channel;

		PrintfUtil_vPrintf("ChannelBBBBB:%d\n", u8Channel);
	}

	u8Channel = 13;
#endif

	pnwk_data->type = NWK_DATA;
	pnwk_data->ttl = 1;
	pnwk_data->src = u16StationPanId;
	vConvertEndian16(&(pnwk_data->src));

	vAHI_UartSetRTSCTS(E_AHI_UART_1,FALSE);     // uart1:2-wire mode,DIO18 is RTS
	vUart_Init(UART_BAUDRATE_115200, vUartTxCallBack, vUartRxCallBack);

	
	PrintfUtil_vPrintf("panid:%d\n", u16StationPanId);

	/*if(!bResetFromWarmStart)
	{
		vStartingLEDindicator();
	}*/

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

	
	MacUtil_Setting_s sMacUtil;
	sMacUtil.u16SrcPanId 		= u16StationPanId;
	sMacUtil.u16SrcShortAddr 	= u16StationShortAddr;
	sMacUtil.u16Profile_id 		= 0x2001; //for backward compatable
	sMacUtil.u8Dst_endpoint 	= 0x21;
	sMacUtil.u8NodeType 		= 0;
	sMacUtil.u8Src_endpoint 	= 0;
	MacUtil_vInit(&sMacUtil);
	
	Hal_AirInit (u16StationPanId, u16StationShortAddr);

	// memset(&tsAirDataInfo, 0, sizeof(tsAirDataInfo));
	// tsAirDataInfo.isSending = false;
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
	    || u16StationPanId < 20000 || u16StationPanId > 29999)
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
	while(event && count++<32)
	{
		switch (event)
		{
		case LIGHT_FEED_WATCHDOG_EVENT:
		{
#if (defined SUPPORT_HARD_WATCHDOG)
			vFeedHardwareWatchDog();
#endif
			vAHI_WatchdogRestart();
			EventUtil_vUnsetEvent(event);	

			break;
		}

		case LIGHT_REPORTACK_STATUS_EVENT:
		{
			app_Light_ReportAck_t *pStationReportAck = (app_Light_ReportAck_t *)u32CommonBuf;
			uint16 *pCrc = (uint16 *)(pStationReportAck + 1);

			//PrintfUtil_vPrintf("report ack\n");

			pStationReportAck->sync_head.sync[0] = 'Y';
			pStationReportAck->sync_head.sync[1] = 'I';
			pStationReportAck->sync_head.sync[2] = 'R';
			pStationReportAck->sync_head.data_len = sizeof(app_Light_ReportAck_t) -sizeof(light_sync_hdr_t);
			
			pStationReportAck->app_tof_head.len = sizeof(app_Light_ReportAck_t) - sizeof(light_sync_hdr_t) -sizeof(app_header_t);

			pStationReportAck->app_tof_head.msgtype = APP_LIGHT_MSG_REPORT_ACK;
			pStationReportAck->app_tof_head.protocoltype = APP_PROTOCOL_TYPE_LIGHT;
			pStationReportAck->u8MaxLightLevel = u8MaxLightLevel;
			pStationReportAck->u8MinLightLevel = u8MinLightLevel;
			pStationReportAck->i8Rssi_Threshold = i8Rssi_Threshold;
			pStationReportAck->u8IsRssiStation = u8IsRssiStation;
			*pCrc = CRC16(u32CommonBuf, sizeof(app_Light_ReportAck_t), 0xFFFF);

			uint16 u16SendLen = sizeof(app_Light_ReportAck_t)+ sizeof(uint16);

			//PrintfUtil_vPrintMem(u32CommonBuf,u16SendLen);

			if(UART_SUCCESS!=u8Uart_StartTx((uint8 *)u32CommonBuf,u16SendLen))
			{
			 	EDBG(PrintfUtil_vPrintf("Err: Uart Send Data Failed\n"););
			}
			//TimerUtil_eSetTimer(STATION_REPORT_STATUS_EVENT, 2000);
			EventUtil_vUnsetEvent(LIGHT_REPORTACK_STATUS_EVENT);
			break;
		}

		case LIGHT_REPORT_RSSI_EVENT:
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
				if(UART_SUCCESS!=u8Uart_StartTx((uint8 *)(u32CommonBuf),sizeof(struct nwkhdr) +sizeof(app_rssi_report)))
				{
				    EDBG(PrintfUtil_vPrintf("Err: Uart Send Data Failed\n"););
				}
				 //Jbsmac_eWriteData((uint8 *)(u32CommonBuf),sizeof(struct nwkhdr) +sizeof(app_rssi_report));
				 //reset_report_rssi();
			}
			TimerUtil_eSetTimer(LIGHT_REPORT_RSSI_EVENT, REPORT_RSSI_TIME);
			EventUtil_vUnsetEvent(LIGHT_REPORT_RSSI_EVENT);
			break;
		}

		case LIGHT_REPORT_DEPTH_EVENT:
		{
			PrintfUtil_vPrintf("parent %d\n",NwkInfo.parent);
			if(NwkInfo.parent != NWK_INVALID_ADDRESS)
			{
				if(bReportedDepth())
					vLightCast(APP_DATA_TYPE_DEPTH,0x0000,0xFFFF,0,0);
			}

			if((Hal_GetTimeMs()  - NwkInfo.updatedepthtime) > 70000 && (NwkInfo.parent != NWK_INVALID_ADDRESS))
			{
				NwkInfo.parent = NWK_INVALID_ADDRESS;
				NwkInfo.depth = NWK_INVALID_DEPTH;
				PrintfUtil_vPrintf("lose connect\n");				
				vLightCast(APP_DATA_TYPE_DEPTH,0x0000,0xFFFF,0,0);

				u8ListenCnt = 0;
				//u16FatherNode = 0;
				NwkInfoTemp.accucost = 0xFFFF;
				NwkInfoTemp.parent = NWK_INVALID_ADDRESS;
				NwkInfoTemp.depth = NWK_INVALID_DEPTH;
			}

			if((Hal_GetTimeMs()  - NwkInfo.updatedepthtime) > 120000)
			{
				PrintfUtil_vPrintf("updatetime %d\n",NwkInfo.updatedepthtime);
				vAHI_SwReset();
			}
			TimerUtil_eSetTimer(LIGHT_REPORT_DEPTH_EVENT, 30000);
			EventUtil_vUnsetEvent(event);
			break;
		}
		
		case LIGHT_REPORT_VOL_CUR_EVENT:
		{
			PrintfUtil_vPrintf("Report1 Vol %d\n",NwkInfo.parent);
			if(NwkInfo.parent != NWK_INVALID_ADDRESS)
			{
				PrintfUtil_vPrintf("Report Vol %d\n",NwkInfo.parent);
				vLightCast(APP_DATA_TYPE_VOL_CUR,0x0000,NwkInfo.parent,MAC_TX_OPTION_ACK,0);
				//vLightCast(APP_DATA_TYPE_VOL_CUR,0x0000,0x898F,MAC_TX_OPTION_ACK,0);
			}
			EventUtil_vUnsetEvent(event);
			break;
		}

		case LIGHT_REPORT_LIGHTVERSION:
		{
			PrintfUtil_vPrintf("ver:%d\n",u8RssiChannel);
			app_light_ver_channel_ts* pheard = (app_light_ver_channel_ts*)(u8UpstreamBuf);
			pheard->app_light_head.protocoltype = APP_PROTOCOL_TYPE_LIGHT;
			pheard->app_light_head.msgtype = APP_LIGHT_MSG_VER_CHANNEL;
			pheard->app_light_head.len = 8;
			pheard->light_ver_channel.u8light_channel = u8Channel;
			pheard->light_ver_channel.u8rssi_channel = u8RssiChannel;
			pheard->light_ver_channel.u16light_oad = OAD_CARD_VERSION;
			pheard->light_ver_channel.u16rssi_oad = OAD_CARD_VERSION;
			pheard->light_ver_channel.u16light_addr = u16StationPanId;
			vLightCast(APP_DATA_TYPE_UPSTREAM,0x0000,NwkInfo.parent,1,0);
			TimerUtil_eSetTimer(LIGHT_REPORT_LIGHTVERSION, 300000);
			//TimerUtil_eSetTimer(LIGHT_REPORT_LIGHTVERSION, 3000);
			EventUtil_vUnsetEvent(LIGHT_REPORT_LIGHTVERSION);
			break;
		}

		case LIGHT_REPORT_DATA_EVENT:
		{
			EventUtil_vUnsetEvent(LIGHT_REPORT_DATA_EVENT);
			break;
		}

		case LIGHT_LISTEN_EVENT:
		{
			u8LightState = E_LIGHT_STATE_LISTEN;
			TimerUtil_eSetTimer(LIGHT_LISTEN_TIMEOUT_EVENT, 5000);
			EventUtil_vUnsetEvent(LIGHT_LISTEN_EVENT);
			break;
		}
 
		case LIGHT_RANDOM_REPORT_EVENT:
		{
			vLightCast(u32CastType,0x0000,0xFFFF,0,0);
			EventUtil_vUnsetEvent(LIGHT_RANDOM_REPORT_EVENT);
			break;
		}
		case LIGHT_DOWN_CAST_EVENT:
		{
			if(light_parmeter. u16DestAddr == 0xFFFF)
				vLightCast(APP_DATA_TYPE_PARAMETER_SET,0x0000,0xFFFF,0,0);
			else
				vLightCast(APP_DATA_TYPE_PARAMETER_SET,0x0000,light_parmeter.u16Pathtoroot[light_parmeter.u8Len/2-1],0,0);
			EventUtil_vUnsetEvent(LIGHT_DOWN_CAST_EVENT);
			break;
		}

		case LIGHT_CHANGE_CHANNEL_EVENT:
		{
			bIsCharging = FALSE;
			if(bChannelValid(u8NewLocChannel))
			{
				u8Channel = u8NewLocChannel;
				vSaveChannel(u8NewLocChannel);
				vAHI_SwReset();
			}		
			EventUtil_vUnsetEvent(LIGHT_CHANGE_CHANNEL_EVENT);
			break;
		}

		case LIGHT_CAST_CHANNEL_SETTING_EVENT:
		{
            if(light_channel.u16DestAddr == 0xFFFF)
                vLightCast(APP_DATA_TYPE_DOWNSTREAM,0x0000,0xFFFF,0,0);
            else
                vLightCast(APP_DATA_TYPE_DOWNSTREAM,0x0000,light_channel.u16Pathtoroot[light_channel.u8Len/2-1],0,0);
            EventUtil_vUnsetEvent(LIGHT_CAST_CHANNEL_SETTING_EVENT);
            break;
		}

		case LIGHT_CAST_DEPTH_EVENT:
		{
			if(!bMsgSending)
			{
				vLightCast(APP_DATA_TYPE_VOL_CUR_TEMP,0x0000,NwkInfo.parent,MAC_TX_OPTION_ACK,0);
				EventUtil_vUnsetEvent(LIGHT_CAST_DEPTH_EVENT);	
			}
			//else, not need to unset event 
			break;
		}
		case LIGHT_LED_EVENT:
		{
    		//vSetLed(JN_GREEN, FALSE, 0); //TRUE, TOF_LED_GREEN_MS);
    		//vSetLed(JN_RED, FALSE, 0);

			vAHI_DioSetOutput(0,LED_RF);
			TimerUtil_eSetTimer(LIGHT_LED_OFF_EVENT, 10);
			

			if(NwkInfo.parent != NWK_INVALID_ADDRESS)
			{
				TimerUtil_eSetTimer(LIGHT_LED_EVENT, 1000);
			}
			else
			{
				TimerUtil_eSetTimer(LIGHT_LED_EVENT, 5000);
			}
			EventUtil_vUnsetEvent(LIGHT_LED_EVENT);
			break;
			
		}
		case LIGHT_LED_OFF_EVENT:
		{
			vAHI_DioSetOutput(LED_RF,0);
			EventUtil_vUnsetEvent(LIGHT_LED_OFF_EVENT);
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
			DBG(PrintfUtil_vPrintf("defmcps %d\n",psMcpsInd->u8Type ););
			break;
		}
	}
}


PRIVATE void vLightCast(uint8 u8CmdType, uint16 u16DstAddr, uint16 u16DstPanId, uint8 u8TxOptions, uint8 u8Val)
{
	bMsgSending = TRUE;
	//vAHI_HighPowerModuleEnable(FALSE, FALSE);
	MacUtil_SendParams_s sParams;
	sParams.u8Radius        = 1;
	sParams.u16DstAddr    = u16DstAddr;
	sParams.u16DstPanId     = u16DstPanId;
	sParams.u16ClusterId     = 0;
	sParams.u8DeliverMode    = MAC_UTIL_UNICAST;

	RfDataWrapper_tu RfLightData;
	RfLightData.data_head.protocol_type = APP_PROTOCOL_TYPE_LIGHT;
	RfLightData.data_head.data_type = u8CmdType;
	RfLightData.data_head.depth = NwkInfo.depth;

	switch (u8CmdType)
	{
	case APP_DATA_TYPE_DEPTH:
	{
		if(NwkInfo.depth != 0 && NwkInfo.depth != NWK_INVALID_DEPTH)
		{
			RfLightData.light_depth_data.data_head.len = 4+sizeof(uint16) * NwkInfo.depth;
			memcpy((uint8*)RfLightData.light_depth_data.u16Pathtoroot, (uint8*)NwkInfo.pathtoroot, sizeof(uint16) * NwkInfo.depth);
		}
		else
		{
			RfLightData.light_depth_data.data_head.len = 4;
		}
			
		RfLightData.light_depth_data.u16accucost = NwkInfo.accucost;
		RfLightData.light_depth_data.u16parent = NwkInfo.parent;
		break;
	}

	case APP_DATA_TYPE_VOL_CUR:
	{
		RfLightData.light_vol_cur_data.data_head.len= 6;
		RfLightData.light_vol_cur_data.u16parent = NwkInfo.parent;
		RfLightData.light_vol_cur_data.u16panid = u16StationPanId;
		RfLightData.light_vol_cur_data.u8vrms = u8Vrms;
		RfLightData.light_vol_cur_data.u8irms = u8Irms;
                //PrintfUtil_vPrintf("I :%d\n",RfLightData.light_vol_cur_data.u8irms);
		break;
	}

	case APP_DATA_TYPE_VOL_CUR_TEMP:
	{
		RfLightData.light_vol_cur_data.data_head.len = 6;
		RfLightData.light_vol_cur_data.u16panid = pvol_cur_upstream->u16panid;
		RfLightData.light_vol_cur_data.u16parent = pvol_cur_upstream->u16parent;
		RfLightData.light_vol_cur_data.u8irms = pvol_cur_upstream->u8irms;
		RfLightData.light_vol_cur_data.u8vrms = pvol_cur_upstream->u8vrms;
		break;
	}

	case APP_DATA_TYPE_PARAMETER_SET:
	{
		RfLightData.light_parmeter_data.data_head.len = 8+light_parmeter.u8Len;
		RfLightData.light_parmeter_data.u16DestAddr = light_parmeter.u16DestAddr;
		RfLightData.light_parmeter_data.u8IsRssiStation = light_parmeter.u8IsRssiStation;
		RfLightData.light_parmeter_data.u8Reserved = light_parmeter.u8Reserved;
		RfLightData.light_parmeter_data.u8Len = light_parmeter.u8Len;
		RfLightData.light_parmeter_data.u8MaxLightLevel = light_parmeter.u8MaxLightLevel;
		RfLightData.light_parmeter_data.u8MinLightLevel = light_parmeter.u8MinLightLevel;
		RfLightData.light_parmeter_data.i8RssiAssignment = light_parmeter.i8RssiAssignment;

		//RfLightData.light_parmeter_data.u16Pathtoroot = light_parmeter.u16Pathtoroot;
		if(light_parmeter.u8Len > 0)
			memcpy((uint8*)RfLightData.light_parmeter_data.u16Pathtoroot,(uint8*)light_parmeter.u16Pathtoroot,light_parmeter.u8Len);
		break;
	}

	case APP_DATA_TYPE_DOWNSTREAM:
	{
		RfLightData.light_stream.data_head.len =8+ light_channel.u8Len;
		memcpy(RfLightData.light_stream.data_buf,(uint8*)(&light_channel),RfLightData.light_stream.data_head.len);
		break;
	}

	case APP_DATA_TYPE_UPSTREAM:
	{
		app_header_t* pheader = (app_header_t*)(u8UpstreamBuf);

		RfLightData.data_head.len = pheader->len + sizeof(app_header_t);
		memcpy((&RfLightData.data_head.len)+1,u8UpstreamBuf,pheader->len + sizeof(app_header_t));
		break;
	}

	default:
		break;
	}

	u8LastHandle = MacUtil_vSendData(&sParams, (uint8*)&RfLightData, RfLightData.data_head.len+sizeof(ll_Data_hdr_t), u8TxOptions);
}


/****************************************************************************
 *
 * NAME: vHandleMcpsDataDcfm
 *
 ****************************************************************************/
PRIVATE void vHandleMcpsDataDcfm(MAC_McpsDcfmInd_s *psMcpsInd)
{
 	//hal_ProcessDataCnf (&psMcpsInd->uParam.sDcfmData);

	if(psMcpsInd->uParam.sDcfmData.u8Handle == u8LastHandle)
    {
        bMsgSending = FALSE;
		//vAHI_HighPowerModuleEnable(TRUE, TRUE);
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
	uint8 i;
	uint8 random;
	psFrame = &psMcpsInd->uParam.sIndData.sFrame;

	RfDataWrapper_tu* psAppPkt = (RfDataWrapper_tu*)(psFrame->au8Sdu);
	
	 if (APP_PROTOCOL_TYPE_LIGHT == psAppPkt->data_head.protocol_type)
	 {
		 if((psFrame->sDstAddr.u16PanId == 0xFFFF) || (psFrame->sDstAddr.u16PanId == u16StationPanId))
		 {
			 switch(psAppPkt->data_head.data_type)
			 {
			  case APP_DATA_TYPE_DEPTH:
			  {
				uint8 cost = SysUtil_vConvertLQI2Dbm(psFrame->u8LinkQuality) - LIGHT_RSSI_THRESHOLD >= LIGHT_RSSI_TABLE_SIZE ? 1 : rssi2cost[SysUtil_vConvertLQI2Dbm(psFrame->u8LinkQuality) - LIGHT_RSSI_THRESHOLD];
				//lost connection
				if(NwkInfo.parent == NWK_INVALID_ADDRESS)
				{
					if(psAppPkt->data_head.depth == NWK_INVALID_DEPTH)
					{
						PrintfUtil_vPrintf("invalid net\n");
						return;
					}
					else
					{
						for(i = psAppPkt->data_head.depth; i > 0; --i)
						{
							if(u16StationPanId == psAppPkt->light_depth_data.u16Pathtoroot[i])
							{//from descendant
								PrintfUtil_vPrintf("from descendant\n");
								return;
							}
						}
						if(u8IsRssiStation && psAppPkt->data_head.depth > 1)
							return;

						PrintfUtil_vPrintf("rssi %i,%d\n",SysUtil_vConvertLQI2Dbm(psFrame->u8LinkQuality),psFrame->sSrcAddr.u16PanId);

						if(NwkInfoTemp.accucost > (psAppPkt->light_depth_data.u16accucost+cost))
						{
							NwkInfoTemp.parent = psFrame->sSrcAddr.u16PanId;
							NwkInfoTemp.depth     = psAppPkt->data_head.depth +1;
							NwkInfoTemp.cost      = cost;
							NwkInfoTemp.updatedepthtime = Hal_GetTimeMs();
							NwkInfoTemp.accucost  = psAppPkt->light_depth_data.u16accucost + cost;
							if(psAppPkt->data_head.depth != 0)
							memcpy((uint8*)&NwkInfoTemp.pathtoroot[0], (uint8*)&psAppPkt->light_depth_data.u16Pathtoroot[0], psAppPkt->data_head.depth * sizeof(uint16));
							NwkInfoTemp.pathtoroot[psAppPkt->data_head.depth] 	= NwkInfoTemp.parent;
							//PrintfUtil_vPrintf("aaa:%d",psFrame->sSrcAddr.u16PanId);
						}

						if(SysUtil_vConvertLQI2Dbm(psFrame->u8LinkQuality) > (-40))
						{
						
							PrintfUtil_vPrintf("connecting:%d\n",psFrame->sSrcAddr.u16PanId);

							NwkInfo.parent = psFrame->sSrcAddr.u16PanId;
							NwkInfo.depth     = psAppPkt->data_head.depth +1;
							NwkInfo.cost      = cost;
							NwkInfo.updatedepthtime = Hal_GetTimeMs();
							NwkInfo.accucost  = psAppPkt->light_depth_data.u16accucost + cost;
							if(psAppPkt->data_head.depth != 0)
							memcpy((uint8*)&NwkInfo.pathtoroot[0], (uint8*)&psAppPkt->light_depth_data.u16Pathtoroot[0], psAppPkt->data_head.depth * sizeof(uint16));
							NwkInfo.pathtoroot[psAppPkt->data_head.depth] 	= NwkInfo.parent;
                            u8ListenCnt = 0;
							NwkInfoTemp.accucost = 0xFFFF;
						}
						//u8ListenCnt++;
						else if(u8ListenCnt++ >10)
						{
							NwkInfo = NwkInfoTemp;
							PrintfUtil_vPrintf("connecting1:%d\n",NwkInfo.parent);
							u8ListenCnt = 0;
						}
						
						
					}
				}
				
				else //connected
				{
					if(psAppPkt->data_head.depth == NWK_INVALID_DEPTH)
					{//
						for(i = NwkInfo.depth; i > 0 ; --i)
						{
							if(psFrame->sSrcAddr.u16PanId == NwkInfo.pathtoroot[i - 1])
							{//from ancestor
								PrintfUtil_vPrintf("from ancestor\n");
								NwkInfo.depth     = NWK_INVALID_DEPTH;
								NwkInfo.parent    = NWK_INVALID_ADDRESS;
								u8ListenCnt = 0;
								//u16FatherNode = 0;								
								random = SysUtil_u16GenRndNum()%20; 
								u32CastType = APP_DATA_TYPE_DEPTH;
								 TimerUtil_eSetTimer(LIGHT_RANDOM_REPORT_EVENT, random);
								//vLightCast(APP_DATA_TYPE_DEPTH,0x0000,0xFFFF,0,0);
								return;
							}
						}
					}
					else //update path to root
					{
						if(NwkInfo.parent == psFrame->sSrcAddr.u16PanId)
						{
							if(psAppPkt->data_head.depth +1 == NwkInfo.depth &&( !memcmp((uint8*)&psAppPkt->light_depth_data.u16Pathtoroot[0], (uint8*)&NwkInfo.pathtoroot[0], psAppPkt->data_head.depth * sizeof(uint16))))
							{ 
								PrintfUtil_vPrintf("update\n");
								NwkInfo.cost      = cost;
      								NwkInfo.accucost  = psAppPkt->light_depth_data.u16accucost + cost;
								NwkInfo.updatedepthtime			= Hal_GetTimeMs();
								return;							 
							}
							//path to root changed
							
					        NwkInfo.depth       = psAppPkt->light_depth_data.data_head.depth + 1;
					        NwkInfo.cost        = cost;
					        NwkInfo.accucost    = psAppPkt->light_depth_data.u16accucost + cost;
					        if(psAppPkt->light_depth_data.data_head.depth != 0) 
							memcpy((uint8*)&NwkInfo.pathtoroot[0], (uint8*)&psAppPkt->light_depth_data.u16Pathtoroot[0], psAppPkt->data_head.depth * sizeof(uint16));
							
					        NwkInfo.pathtoroot[psAppPkt->data_head.depth] 	= NwkInfo.parent;
					        NwkInfo.updatedepthtime			= Hal_GetTimeMs();
							random = SysUtil_u16GenRndNum()%20;
							u32CastType = APP_DATA_TYPE_DEPTH;							
							TimerUtil_eSetTimer(LIGHT_RANDOM_REPORT_EVENT, random);
						        //vLightCast(APP_DATA_TYPE_DEPTH,0x0000,0xFFFF,0,0);
							return;
						}
                        else
                        {  
                            if((Hal_GetTimeMs() - NwkInfo.updatedepthtime > NWK_WINDOW_DURATION) && ((psAppPkt->light_depth_data.u16accucost + cost) > (NwkInfo.accucost+10)))
                            {
								PrintfUtil_vPrintf("path to root changed\n");
								NwkInfo.parent = psFrame->sSrcAddr.u16PanId;
    	      					NwkInfo.depth     = psAppPkt->data_head.depth +1;
    	      					NwkInfo.cost      = cost;
    							NwkInfo.updatedepthtime = Hal_GetTimeMs();
          						NwkInfo.accucost  = psAppPkt->light_depth_data.u16accucost+ cost;
                            }
                        }
					}					
					/*else
					{
						if((Hal_GetTimeMs() - NwkInfo.updatedepthtime > NWK_WINDOW_DURATION) && (psAppPkt->light_depth_data.u16accucost + cost < NwkInfo.accucost && (psAppPkt->light_depth_data.u16accucost + cost == NwkInfo.accucost && psAppPkt->data_head.depth + 1 < NwkInfo.depth)))
						{
							NwkInfo.parent = psFrame->sSrcAddr.u16PanId;
		      					NwkInfo.depth     = psAppPkt->data_head.depth +1;
		      					NwkInfo.cost      = cost;
							NwkInfo.updatedepthtime = Hal_GetTimeMs();
      							NwkInfo.accucost  = psAppPkt->light_depth_data.u16accucost+ cost;
						}
					}*/
				}
				
				break;
			  }

			  case APP_DATA_TYPE_VOL_CUR:
			  case APP_DATA_TYPE_VOL_CUR_TEMP:
			  {
				pvol_cur_upstream->u16panid = psAppPkt->light_vol_cur_data.u16panid;
				pvol_cur_upstream->u16parent = psAppPkt->light_vol_cur_data.u16parent;
				pvol_cur_upstream->u8irms = psAppPkt->light_vol_cur_data.u8irms;
				pvol_cur_upstream->u8vrms = psAppPkt->light_vol_cur_data.u8vrms;
				PrintfUtil_vPrintf("child:%d\n",psFrame->sSrcAddr.u16PanId);

				random = SysUtil_u16GenRndNum()%30;							
				TimerUtil_eSetTimer(LIGHT_CAST_DEPTH_EVENT, random);
				//vLightCast(APP_DATA_TYPE_VOL_CUR_TEMP,0x0000,NwkInfo.parent,MAC_TX_OPTION_ACK,0);				
				break;
			  }

			  case APP_DATA_TYPE_PARAMETER_SET:
			  {
				light_parmeter = psAppPkt->light_parmeter_data;
				if(light_parmeter.u8Len > 1)
					light_parmeter.u8Len = light_parmeter.u8Len -2;

				if(light_parmeter.u16DestAddr == u16StationPanId || light_parmeter.u16DestAddr == 0xffff)
				{
					i8Rssi_Threshold = light_parmeter.i8RssiAssignment;
					u8MaxLightLevel = light_parmeter.u8MaxLightLevel;
					u8MinLightLevel = light_parmeter.u8MinLightLevel;
					u8IsRssiStation = light_parmeter.u8IsRssiStation;

					PrintfUtil_vPrintf("i8rssi %d,Max:%d,Min:%d\n",light_parmeter.i8RssiAssignment,light_parmeter.u8MaxLightLevel,light_parmeter.u8MinLightLevel);
					PrintfUtil_vPrintf("light_parmeter.u8IsRssiStation:%d,%d\n",light_parmeter.u8IsRssiStation,light_parmeter.u8Reserved);
					
					TimerUtil_eStopTimer(LIGHT_REPORT_LIGHTVERSION);
					EventUtil_vUnsetEvent(LIGHT_REPORT_LIGHTVERSION);
					random = SysUtil_u16GenRndNum()%20;	
					TimerUtil_eSetTimer(LIGHT_REPORT_LIGHTVERSION,random);
					EventUtil_vSetEvent(LIGHT_REPORTACK_STATUS_EVENT);
				}
				if(light_parmeter.u16DestAddr != u16StationPanId && NwkInfo.depth > psAppPkt->data_head.depth)
				{
					random = SysUtil_u16GenRndNum()%30;							
					TimerUtil_eSetTimer(LIGHT_DOWN_CAST_EVENT, random);
					//vLightCast(APP_DATA_TYPE_PARAMETER_SET,0x0000,light_parmeter.u16Pathtoroot[light_parmeter.u8Len/2],0,0);
				}
				
				break;
			  }

			  case APP_DATA_TYPE_UPSTREAM:
			  {
				if(NwkInfo.parent != NWK_INVALID_ADDRESS)
				{
					PrintfUtil_vPrintf("cast rssi %d,parent %d\n",psFrame->sSrcAddr.u16PanId,NwkInfo.parent);
					memcpy(u8UpstreamBuf,&psAppPkt->light_vol_cur_data.u8vrms,psAppPkt->data_head.len);
					vLightCast(APP_DATA_TYPE_UPSTREAM,0x0000,NwkInfo.parent,0,0);
				}
				break;
			  }
			  
			  case APP_DATA_TYPE_DOWNSTREAM:
			  {
				app_light_channel_ts *pheader =  (app_light_channel_ts *)psAppPkt->light_stream.data_buf;
				if(pheader->u16DestAddr == u16StationPanId || pheader->u16DestAddr== 0xffff)
				{
					if(pheader->app_light_head.msgtype == APP_LIGHT_MSG_NODE_CHANNEL)
					{
						if(pheader->u8channel != u8RssiChannel && (!bIsCharging))
						{
							bIsCharging = TRUE;
							u8NewLocChannel = pheader->u8channel;
							PrintfUtil_vPrintf("recv light ch\n");
							TimerUtil_eSetTimer(LIGHT_CHANGE_CHANNEL_EVENT, 60000);
						}

						//TimerUtil_eSetTimer(LIGHT_DOWN_CAST_EVENT, 60000);
					}
					else if(pheader->app_light_head.msgtype == APP_LIGHT_MSG_RSSI_CHANNEL)
					{
						if(pheader->u8channel > 10 &&  pheader->u8channel <26)
						{
							app_Light_ChannelSeting_t *pChannelSeting = (app_Light_ChannelSeting_t *)u32CommonBuf;
							uint16 *pCrc = (uint16 *)(pChannelSeting + 1);
							pChannelSeting->sync_head.sync[0] = 'Y';
							pChannelSeting->sync_head.sync[1] = 'I';
							pChannelSeting->sync_head.sync[2] = 'R';
							pChannelSeting->sync_head.data_len = sizeof(app_Light_ChannelSeting_t) -sizeof(light_sync_hdr_t);
							pChannelSeting->app_tof_head.len = sizeof(app_Light_ChannelSeting_t) - sizeof(light_sync_hdr_t) -sizeof(app_header_t);
							pChannelSeting->app_tof_head.msgtype = APP_LIGHT_MSG_SET;
							pChannelSeting->app_tof_head.protocoltype = APP_PROTOCOL_TYPE_LIGHT;
							pChannelSeting->u8Channel = pheader->u8channel;
							pChannelSeting->u8Reserved = 0;
							pChannelSeting->u16Reserved = 0;
							*pCrc = CRC16(u32CommonBuf, sizeof(app_Light_ChannelSeting_t), 0xFFFF);

							PrintfUtil_vPrintf("recv rssi ch\n");

							uint16 u16SendLen = sizeof(app_Light_ChannelSeting_t)+ sizeof(uint16);
							if(UART_SUCCESS!=u8Uart_StartTx((uint8 *)u32CommonBuf,u16SendLen))
							{
							 	EDBG(PrintfUtil_vPrintf("Err: Uart Send Data Failed\n"););
							}
						}
						
					}
				}
				 if(pheader->u16DestAddr != u16StationPanId &&NwkInfo.depth > psAppPkt->data_head.depth)
				{
					random = SysUtil_u16GenRndNum()%30;
					if(pheader->u8Len > 0)
						pheader->u8Len = pheader->u8Len -2;
					light_channel = *pheader;
					TimerUtil_eSetTimer(LIGHT_CAST_CHANNEL_SETTING_EVENT, random);
				}
				break;
			  }

			  default:
			  	break;
			   
			 }
		 }
	 }

	 /*
	 else if((APP_PROTOCOL_TYPE_CARD == psAppPkt->data_head.protocol_type) && (u8IsRssiStation == 1))
	 {
		RfTofWrapper_tu* psTofAppPkt = (RfTofWrapper_tu*)(psFrame->au8Sdu);
		 if((psFrame->sDstAddr.u16PanId == 0xFFFF) || (psFrame->sDstAddr.u16PanId == u16StationPanId))
        	{
			switch(psTofAppPkt->tof_head.msgtype)
			{
			case TOF_CARD_RSSI:
			{
			if(psFrame->u8SduLength != sizeof(rf_tof_card_data_ts)) return;

			//app_rssi_data.tof_rssi[u16RssiNum].u16ShortAddr = psFrame->sSrcAddr.uAddr.u16Short;
			//app_rssi_data.tof_rssi[u16RssiNum].u16SeqNum = psAppPkt->rf_tof_card_data.u16SeqNum;
			//app_rssi_data.tof_rssi[u16RssiNum].i8Rssi = SysUtil_vConvertLQI2Dbm(psFrame->u8LinkQuality);
			//app_rssi_data.tof_rssi[u16RssiNum].u8RssiType = APP_TOF_CARD_RSSI;
			//app_rssi_data.tof_rssi[u16RssiNum].u8Reserved = 0;
			//app_rssi_data.tof_rssi[u16RssiNum].u8Accel = psAppPkt->rf_tof_card_data.u8ExciterIDorAccStatus;

			u16RssiNum++;
			if(APP_MAX_CARD_NUM == u16RssiNum)
			{
			    //vReportCardRssi();
			    //bReported = TRUE;
			}

			break;
			}

			}

		 }
	 }*/
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


PRIVATE bool bReportedDepth(void)
{
	if(u8IsRssiStation)
	{
		if(NwkInfo.depth >1)
			return FALSE;
	}
	else
	{
		if(NwkInfo.depth >7)
			return FALSE;
	}
	return TRUE;
}



#if 0
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
		        && (psAppPkt->rf_tof_oad_data.u8DeviceType == DEVICE_TYPE_STATION))
		        //&& (psAppPkt->rf_tof_oad_data.u16Version > OAD_COM_STATION_VERSION))
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
	    DBG(PrintfUtil_vPrintf("defcmd%d\n",psAppPkt->tof_head.msgtype););
	    break;
	}
}
#endif

PRIVATE void vUartRxCallBack(unsigned char *pbuf, unsigned char len)
{
	if(len < (sizeof(light_sync_hdr_t) + sizeof(app_header_t)) || len > BSMAC_MAX_TX_PAYLOAD_LEN)
	{
		EDBG(PrintfUtil_vPrintf("uart data len  ERR! length %d\n", len););
		return;
	}
	app_header_t* pheader = (app_header_t*)(pbuf + sizeof(light_sync_hdr_t));
	//uint8 		*pPayload =	(uint8 *)(pheader + 1);
	
	if(APP_PROTOCOL_TYPE_LIGHT == pheader->protocoltype)
	{
		switch(pheader->msgtype)		
		{
		case APP_LIGHT_MSG_REPORT:
		{
			app_Light_Report_t * papp_Report_t = (app_Light_Report_t*)pbuf;
			u8Vrms = papp_Report_t->u8Vrms;
			u8Irms = papp_Report_t->u8Irms;	
			u8RssiChannel = papp_Report_t->u8Channel;
			EventUtil_vSetEvent(LIGHT_REPORTACK_STATUS_EVENT);

			break;
		}

		case APP_LIGHT_MSG_CARD_RSSI:
		{
			if(NwkInfo.parent != NWK_INVALID_ADDRESS)
			{
				//PrintfUtil_vPrintf("uart rssi data\n");
				memcpy(u8UpstreamBuf,pheader,pheader->len + sizeof(app_header_t));
				vLightCast(APP_DATA_TYPE_UPSTREAM,0x0000,NwkInfo.parent,0,0);
				
			}
			break;
		}
		
		default:
		{
			break;
		}
		}

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
		PrintfUtil_vPrintf("fail  channel %d \n",u8BroadcastChannel);
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
PRIVATE void  vSaveChannel(uint8 u8Channel)
{
    bAHI_FlashInit(E_FL_CHIP_INTERNAL, NULL);

    if(!bAHI_FlashEraseSector(7))
    {
        PrintfUtil_vPrintf("Flash Erase Fail\n");
        return;
    }

    cfg_option.u8Channel = u8Channel;

    if(bAHI_FullFlashProgram(PARAM_STORE_ADDR, sizeof(CFG_OPTION_T),(uint8*)&cfg_option))
    {
        PrintfUtil_vPrintf("Set  channel Success!%d\n", u8Channel);

        u16StationPanId = cfg_option.u16ShortAddr;

        u8Channel = cfg_option.u8Channel;
        return;
    }
    else
    {
        PrintfUtil_vPrintf("Set  channel fail!%d\n",u8Channel);
        return;
    }
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
		EDBG(PrintfUtil_vPrintf("Flash Read Fail\n"););
		return FALSE;
	}

	if(!bAHI_FlashEraseSector(7))
	{
		EDBG(PrintfUtil_vPrintf("Flash Erase Fail\n"););
		return FALSE;
	}
	Resetparam.u8reportType = u8reportType;
	Resetparam.u8TypeInfo = u8TypeInfo;

	if(bAHI_FullFlashProgram(RESET_PARAM_STORE_ADDR, sizeof(ResetParam_t),(uint8*)&Resetparam))
	{
		DBG(PrintfUtil_vPrintf("Set  u8reportType Success!%d %d\n", Resetparam.u8reportType, Resetparam.u8TypeInfo););
		return TRUE;
	}
	else
	{
		EDBG(PrintfUtil_vPrintf("Set  u8reportType fail!%d %d\n", Resetparam.u8reportType, Resetparam.u8TypeInfo););
		return FALSE;
	}
}

PRIVATE void  vStackCheck(void)
{
	uint32 u32CurrentTimeMs = Hal_GetTimeMs();

	//if over 1 min did not received any signal  from air, then sleep 1ms,
	//sleep is useless,  just want to run to warm start and  reset stack,
	// At the same time keep ram on, make the system go to work rapidly.

	if(u32CurrentTimeMs -  u32LastAirRxTimeMs > 180*1000)
	{
		EDBG(PrintfUtil_vPrintf("No Air! %d %d\n ", u32LastAirRxTimeMs, u32CurrentTimeMs););
		//EDBG(i2c_vPrintPoll();); //print datas in buffer first
		//SleepUtil_Sleep(E_AHI_WAKE_TIMER_0, 1, E_AHI_SLEEP_OSCON_RAMON);
		vAHI_SwReset();
	}
}



PRIVATE void vConvertEndian16(uint16* pu16Data)
{
	SysUtil_vConvertEndian(pu16Data, sizeof(uint16));
}







