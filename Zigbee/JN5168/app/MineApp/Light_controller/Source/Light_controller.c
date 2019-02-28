
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
//#include "bsmac.h"
#include "RssiUart.h"
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
//#define LED_UART0         E_AHI_DIO9_INT
#define LED_UART1         E_AHI_DIO8_INT
#define LED_RF             E_AHI_DIO9_INT  //DIO 14 is used for i2c,remove it to unused DIO16



#define TOF_BUZZER_PIN  E_AHI_DIO13_INT

#define TOF_BUZZER_TIMER	 E_AHI_TIMER_3	// for buzzer's input

#define CS5460_RST_PIN		E_AHI_DIO1_INT
#define CS5460_CLK_PIN             E_AHI_DIO17_INT
#define CS5460_SDO_PIN            E_AHI_DIO5_INT
#define CS5460_SDI_PIN             E_AHI_DIO4_INT
#define CS5460_CS_PIN     		E_AHI_DIO10_INT

#define LIGHT_KEY_PIN           E_AHI_DIO16_INT


#define E_SPI_CLOCK_2M  8 // 8*16M/64
#define E_SPI_MSB_FIRST FALSE





//FIX me, DIO14 is used for I2C, Use DIO16 instead
//#define LED_LINK         E_AHI_DIO14_INT

/* app envents */
//#define	STATION_PORT_QUERY_EVENT	   BIT(1)
#define STATION_REPORT_STATUS_EVENT    BIT(2)
#define STATION_REPORT_RESET_EVENT    BIT(3)
#define STATION_REPORT_RSSI_EVENT    BIT(4)
#define STATION_REPORT_DATA_EVENT       BIT(5)
#define STATION_REPORT_CARDVERSION   BIT(6)
#define STATION_BUZZER_EVENT				BIT(27)	// card buzzer
#define STATION_STOP_BUZZER_EVENT			BIT(28)	// stop buzzer




#define PARAM_STORE_ADDR 	0x38000
#define   STATION_FEED_WATCHDOG_EVENT  BIT(10)
#define MOBILEPHONE_NWK_ADDR		    0xFFF4
#define AIR_BUF_SIZE					16
#define HI_UINT16(a) (((a) >> 8) & 0xFF)
#define LO_UINT16(a) ((a) & 0xFF)
#define CONVERT_ENDIAN(x)	SysUtil_vConvertEndian(&(x), sizeof(x))
#define MEMCOPY(x, y)	memcpy(&(x), &(y), sizeof(x))


#define OPTION_CARDNUM     "cardnum="
#define OPTION_CHANNEL     "channel="
//#define OPTION_CARDTYPE    "cardtype="
//#define OPTION_RSSICH      "rssich="
#define OPTION_REBOOT   "reboot"
#define OPTION_SHOW     "show"
#define OPTION_HELP     "help"
#define OPTION_SAVEENV  "saveenv"

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
//PRIVATE void vProcessStationCmd(MAC_McpsIndData_s *psMcpsInd);
//PRIVATE bool_t bArmReady();
PRIVATE void vUartRxCallBack(unsigned char *pbuf, unsigned char len);
PRIVATE void MP_Report_ResetCause(uint8 reporttype,uint8 devtype);
PRIVATE void  vStackCheck(void);
PRIVATE void vSyncLEDStatus(void);
PRIVATE void reset_report_rssi(void);
//PRIVATE void update_station_rssi(uint16 u16Short,int8 receivei8Rssi,int8 senti8Rssi,uint8 msgtype);
PRIVATE void vReportCardRssi(void);
//PRIVATE void vReportCardExcite(uint16 u16ShortAddr, uint8 u8CardStatus,uint8 u8ExciteID);
PRIVATE void vConvertEndian16(uint16* pu16Data);
//PRIVATE void vTransferThroughUpData();
PRIVATE bool  vSaveChannel(uint8 u8Channel);
PRIVATE bool bChannelValid(uint8 u8BroadcastChannel);
PRIVATE void vBuzzer(uint32 u32Ms);
PRIVATE void vStopBuzzer();
//PRIVATE void CS5460_SpiInit(void);
PRIVATE void Reset_CS5460(void);
PRIVATE uint32 CS5460_Read_Register(uint8 com);







//bool  vSaveResetType(uint8 u8reportType, uint8 u8TypeInfo);
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
PRIVATE uint8 u8ReportStatusNum=0;

PRIVATE uint8 u8RestartCounter=0;

/*use u32 for alignment*/
PRIVATE uint32 u32CommonBuf[128];

//PRIVATE uint8 u8ThroughUpBuf[128];


PRIVATE uint8 u8ThroughBuf[128];

PRIVATE uint32 u32Data2Arm[30];    // the data send to arm, max len is 114 (BSMAC_MAX_TX_PAYLOAD_LEN), 30*4 = 120 is enough (28 is enough)
PRIVATE struct nwkhdr* pnwk_data = (struct nwkhdr*)u32Data2Arm;


PRIVATE uint32 u32LastAirRxTimeMs = 0;
PRIVATE bool_t  bResetFromWarmStart = 0;
PRIVATE uint16 u16RssiNum = 0;
PRIVATE app_vehicle_rssi_ts app_rssi_data;
//PRIVATE app_transfer_through_ts app_transfer_through_data;

//PRIVATE bool bReported = FALSE;

PRIVATE app_LSVersionReport_t tsVersionReport[APP_TOF_VERSION_MAX_NUM];
PRIVATE uint8  			      tsVersionReportLen;

PRIVATE app_rssi_report  rssiReport;
PRIVATE int_8 i8MpRcvRssi;
PRIVATE uint8 u8StationPort;

PRIVATE uint8 au8UartRxBuffer[100];

PRIVATE uint16 u16IrmsTable[86][2]={
        {470,55},
        {504,60},
        {558,70},
        {648,80},
        {720,90},
        {800,100},
        {840,110},
        {905,120},
        {981,130},
        {1051,140},
        {1127,150},
        {1170,160},
        {1186,170},
        {1210,180},
        {1234,190},
        {1254,200},
        {1272,210},
        {1306,220},
        {1320,230},
        {1335,240},
        {1350,250},
        {1363,260},
        {1376,270},
        {1384,280},
        {1390,290},
        {1395,300},
        {1405,310},
        {1410,320},
        {1412,330},
        {1423,340},
        {1428,350},
        {1437,360},
        {1440,370},
        {1453,380},
        {1457,390},
        {1459,400},
        {1463,410},
        {1468,420},
        {1471,430},
        {1477,440},
        {1482,450},
        {1485,460},
        {1489,470},
        {1493,480},
        {1494,490},
        {1496,500},
        {1504,510},
        {1507,520},
        {1510,530},
        {1512,540},
        {1514,550},
        {1516,560},
        {1518,570},
        {1521,580},
        {1524,590},
        {1527,600},
        {1530,610},
        {1535,620},
        {1537,630},
        {1539,640},
        {1540,650},
        {1542,660},
        {1543,670},
        {1544,680},
        {1547,690},
        {1548,700},
        {1549,710},
        {1553,720},
        {1555,730},
        {1556,740},
        {1558,750},
        {1559,760},
        {1561,770},
        {1564,790},
        {1565,800},
        {1570,820},
        {1572,840},
        {1575,860},
        {1578,880},
        {1580,900},
        {1585,920},
        {1586,940},
        {1588,960},
        {1591,980},
        {1601,1050},
        {1612,1100},
};
PRIVATE uint32 current_vol;
PRIVATE uint32 Vrms;
PRIVATE uint32 Irms;



PRIVATE uint8 u8IsRssiStation = 0;   //默认不支持上报rssi

PRIVATE uint8 u8MaxLightLevel = 10;
PRIVATE uint8 u8MinLightLevel = 1;

PRIVATE int8 i8Rssi_Threshold = -70;

PRIVATE uint8 u8LightTimeout = 0;

PRIVATE uint8 u8PwmParameter[11] = {100,96, 88, 80,71,63,55,47,39,31,22};



PRIVATE uint8 u8LightStatus;

typedef struct
{
	//uint8 u8Status;
	//uint8 u8CardType;
	uint8 u8Channel;
	uint8 u8Reserved;
	uint8 u8MaxLightLevel;
	uint8 u8MinLightLevel;
	int8 i8Rssi_Threshold;
	uint8 u8IsRssiStation;
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


#define REPORT_RSSI_TIME   (5*60*1000)
#define  LIGHT_ON_TIMEOUT  18

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
	vCheckSetting();

	Reset_CS5460();

	PrintfUtil_vPrintf("cs5460\n");

	vBuzzer(1000);
	TimerUtil_eSetTimer(STATION_REPORT_STATUS_EVENT, 5000);

	TimerUtil_eSetTimer(STATION_REPORT_DATA_EVENT, 100);

	TimerUtil_eSetCircleTimer(STATION_FEED_WATCHDOG_EVENT, 300);

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


PRIVATE void  _save_parameter(void)
{
	bAHI_FlashInit(E_FL_CHIP_INTERNAL, NULL);

	cfg_option.u8MinLightLevel = u8MinLightLevel;
	cfg_option.u8MaxLightLevel = u8MaxLightLevel;
	cfg_option.u8IsRssiStation = u8IsRssiStation;
	cfg_option.i8Rssi_Threshold = i8Rssi_Threshold;

	if(!bAHI_FlashEraseSector(7))
	{
		PrintfUtil_vPrintf("Flash Erase Fail\n");
		return;
	}

	if(bAHI_FullFlashProgram(PARAM_STORE_ADDR, sizeof(CFG_OPTION_T),(uint8*)&cfg_option))
	{
		PrintfUtil_vPrintf("finish saving  !\n");
		return;
	}
	else
	{
		PrintfUtil_vPrintf("Set  parameter fail!\n");
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

	if((cfg_option.i8Rssi_Threshold < -5) && (cfg_option.i8Rssi_Threshold > -90))
		i8Rssi_Threshold = cfg_option.i8Rssi_Threshold;

	if(cfg_option.u8IsRssiStation == 0 || cfg_option.u8IsRssiStation ==1)
		u8IsRssiStation = cfg_option.u8IsRssiStation;

	if(cfg_option.u8MaxLightLevel <11 )
		u8MaxLightLevel = cfg_option.u8MaxLightLevel;

	if(cfg_option.u8MinLightLevel <11 )
		u8MinLightLevel = cfg_option.u8MinLightLevel;

	u16StationPanId = cfg_option.u16ShortAddr;
	u8Channel = cfg_option.u8Channel;

	PrintfUtil_vPrintf("Threshold:%i,IsRssiStation:%d,MaxLightLevel:%d,MinLightLevel:%d\n",cfg_option.i8Rssi_Threshold,cfg_option.u8IsRssiStation,cfg_option.u8MaxLightLevel,cfg_option.u8MinLightLevel);

	PrintfUtil_vPrintf("Threshold:%i,IsRssiStation:%d,MaxLightLevel:%d,MinLightLevel:%d\n",i8Rssi_Threshold,u8IsRssiStation,u8MaxLightLevel,u8MinLightLevel);
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
		u8Channel = DEFAULT_CHANNEL_LOCATOR;
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


	vAHI_DioSetDirection(0, E_AHI_DIO12_INT);   //init hardware watchdog GPIO
	vAHI_DioSetOutput(E_AHI_DIO12_INT,0);

	vAHI_DioSetDirection(0, CS5460_RST_PIN);
	vAHI_DioSetOutput(0,CS5460_RST_PIN);

	vAHI_DioSetDirection(0,CS5460_CLK_PIN);
	vAHI_DioSetDirection(0,CS5460_SDO_PIN);
	//vAHI_DioSetDirection(0,CS5460_SDI_PIN);
	vAHI_DioSetDirection(0,CS5460_CS_PIN);

	vAHI_DioSetDirection(CS5460_SDI_PIN, 0);
	vAHI_DioSetPullup(CS5460_SDI_PIN,0);

	vAHI_DioSetDirection(0, LIGHT_KEY_PIN);

	vAHI_DioSetOutput(LIGHT_KEY_PIN,0);

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

	// Enable high power modules, tof function, timerUtil
	vAHI_HighPowerModuleEnable(TRUE, TRUE);
	TimerUtil_vInit();

	vAHI_WakeTimerEnable(E_AHI_WAKE_TIMER_0, TRUE);

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

	PrintfUtil_vPrintf("StationPanId:%d\n",u16StationPanId);

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

		PrintfUtil_vPrintf("++++\n");
		PrintfUtil_vPrintf("ChannelBBBBB:%d\n", u8Channel);
	}

	#endif


	PrintfUtil_vPrintf("ChannelAAAA:%d\n", u8Channel);

	app_rssi_data.app_tof_head.protocoltype = APP_PROTOCOL_TYPE_LIGHT;
	app_rssi_data.app_tof_head.msgtype = APP_LIGHT_MSG_CARD_RSSI;

	app_rssi_data.u16station_addr = u16StationPanId;
	app_rssi_data.u16seqnum = 1;

	pnwk_data->type = NWK_DATA;
	pnwk_data->ttl = 1;
	pnwk_data->src = u16StationPanId;
	vConvertEndian16(&(pnwk_data->src));

	vAHI_UartSetRTSCTS(E_AHI_UART_1,FALSE);     // uart1:2-wire mode,DIO18 is RTS
	vUart_Init(UART_BAUDRATE_115200, vUartTxCallBack, vUartRxCallBack);



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
		case STATION_FEED_WATCHDOG_EVENT:
		{
#if (defined SUPPORT_HARD_WATCHDOG)
			vFeedHardwareWatchDog();
#endif
            static uint8 i;
            uint8 j;
			if(i++%3)
			{
				vAHI_WatchdogRestart();
				if(i%2)
				{
					current_vol  = CS5460_Read_Register(11);
					current_vol = current_vol /10000;

                    if(current_vol > 1612)
                        Irms = current_vol*25/6 -5616;
                    else if(current_vol<470)
                        Irms = current_vol*11/94;
                    else
                    {
                        for(j=0;j<85;j++)
                        {
                            if(current_vol >=u16IrmsTable[j][0] && current_vol <= u16IrmsTable[j+1][0])
                            {
                                Irms = u16IrmsTable[j][1] +(current_vol - u16IrmsTable[j][0])*(u16IrmsTable[j+1][1]-u16IrmsTable[j][1])/(u16IrmsTable[j+1][0]-u16IrmsTable[j][0]);
                                //PrintfUtil_vPrintf("ok\n");
                                break;
                            }
                        }
                    }

                    if(Irms > 100000)
                        Irms = 0;
					//PrintfUtil_vPrintf("Irms:%d,%d\n",Irms,current_vol);
				}

				else
				{
					current_vol  = CS5460_Read_Register(12);
					current_vol = current_vol /10000;

					if(current_vol > 161)
						Vrms = (current_vol*5)/9 -4;
					else
						Vrms = (current_vol*5)/7 -30;
					//PrintfUtil_vPrintf("Vrms:%d,Irms:%d\n",Vrms,Irms);
					//vAHI_DioSetOutput(0,LIGHT_KEY_PIN);
				}
			}
			EventUtil_vUnsetEvent(event);

			break;
		}
		case STATION_REPORT_RESET_EVENT:
		{

		}

		case STATION_REPORT_STATUS_EVENT:
		{
			app_Light_Report_t *pStationReport = (app_Light_Report_t *)u32CommonBuf;
			uint16 *pCrc = (uint16 *)(pStationReport + 1);

			pStationReport->sync_head.sync[0] = 'Y';
			pStationReport->sync_head.sync[1] = 'I';
			pStationReport->sync_head.sync[2] = 'R';
			pStationReport->sync_head.data_len = sizeof(app_Light_Report_t) -sizeof(light_sync_hdr_t);

			pStationReport->app_light_head.len = sizeof(app_Light_Report_t) -sizeof(light_sync_hdr_t) - sizeof(app_header_t);

			pStationReport->app_light_head.msgtype = APP_LIGHT_MSG_REPORT;
			pStationReport->app_light_head.protocoltype = APP_PROTOCOL_TYPE_LIGHT;
			pStationReport->u8Irms = Irms>>2;
			pStationReport->u8Vrms = Vrms>>1;
			pStationReport->u8Channel = u8Channel;

			*pCrc = CRC16(u32CommonBuf, sizeof(app_Light_Report_t), 0xFFFF);

			uint16 u16SendLen = sizeof(app_Light_Report_t)+ sizeof(uint16);

			if(UART_SUCCESS!=u8Uart_StartTx((uint8 *)u32CommonBuf,u16SendLen))
			{
			    EDBG(PrintfUtil_vPrintf("Err: Uart Send Data Failed\n"););
			}
			TimerUtil_eSetTimer(STATION_REPORT_STATUS_EVENT, 5000);
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
				if(UART_SUCCESS!=u8Uart_StartTx((uint8 *)(u32CommonBuf),sizeof(struct nwkhdr) +sizeof(app_rssi_report)))
				{
				    EDBG(PrintfUtil_vPrintf("Err: Uart Send Data Failed\n"););
				}
				 //Jbsmac_eWriteData((uint8 *)(u32CommonBuf),sizeof(struct nwkhdr) +sizeof(app_rssi_report));
				 reset_report_rssi();
			}
			TimerUtil_eSetTimer(STATION_REPORT_RSSI_EVENT, REPORT_RSSI_TIME);
			EventUtil_vUnsetEvent(STATION_REPORT_RSSI_EVENT);
			break;
		}

		case STATION_REPORT_CARDVERSION:
		{
			vReportCardVersion();
			TimerUtil_eSetTimer(STATION_REPORT_CARDVERSION, 60000);
			EventUtil_vUnsetEvent(STATION_REPORT_CARDVERSION);
			break;
		}

		case STATION_REPORT_DATA_EVENT:
		{
			//DBG(PrintfUtil_vPrintf("report event\n"););
			vReportCardRssi();
			TimerUtil_eSetTimer(STATION_REPORT_DATA_EVENT, 1000);
			EventUtil_vUnsetEvent(STATION_REPORT_DATA_EVENT);

			break;
		}

		case STATION_STOP_BUZZER_EVENT:
		{
			vStopBuzzer();
			vBuzzer(1000);
        		EventUtil_vUnsetEvent(STATION_STOP_BUZZER_EVENT);
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


void vProcessCardVersion(uint16 u16DevId, uint8 u8DevType, uint16 u16OadVersion, uint16 u16Battery)
{
	uint8 i;
	bool bfind = FALSE;

	if(u16DevId==0 || u16OadVersion==0 || u16Battery==0)
	{
		EDBG(PrintfUtil_vPrintf("version error: %d, %d, %d\n",u16DevId,u16OadVersion,u16Battery););
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
			tsVersionReport[i].battery = u16Battery>>5;
			tsVersionReport[i].rssich = (u16Battery & 0x1F);
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
				tsVersionReport[i].battery = u16Battery>>5;
				tsVersionReport[i].rssich = (u16Battery & 0x1F);
				tsVersionReportLen++;
				bfind = TRUE;
				break;
			}
		}
	}

	if(tsVersionReportLen >= APP_TOF_VERSION_MAX_NUM)
	{
		//TimerUtil_eSetTimer(STATION_REPORT_CARDVERSION, 1);
	}
}

#if 0
PRIVATE void vStationCast(uint8 u8CmdType, uint16 u16DstAddr, uint16 u16DstPanId, uint8 u8TxOptions, uint8 u8Val)
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

	switch (u8CmdType)
	{

	case TOF_STATION_CHECKIN_ACK:
	case TOF_STATION_EXCITE_ACK:
	{
		RfTofData.tof_head.len = 0;
		break;
	}

	case TOF_STATION_RSSI_CHECK_ACK:
	{
		RfTofData.rt_tof_station_rssi.i8Rssi = i8MpRcvRssi;
		RfTofData.rt_tof_station_rssi.u8StationPort = u8StationPort;
		RfTofData.tof_head.len = 2;
		break;
	}

	default:
		break;
	}

	MacUtil_vSendData(&sParams, (uint8*)&RfTofData, RfTofData.tof_head.len+sizeof(app_header_t), u8TxOptions);
}
#endif

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

	RfTofWrapper_tu* psAppPkt = (RfTofWrapper_tu*)(psFrame->au8Sdu);


	switch (psAppPkt->tof_head.protocoltype)
	{
	case APP_PROTOCOL_TYPE_CARD:
	{
		u32LastAirRxTimeMs = Hal_GetTimeMs();
		if((psFrame->sDstAddr.u16PanId == 0xFFFF) || (psFrame->sDstAddr.u16PanId == u16StationPanId))
		{
			switch(psAppPkt->tof_head.msgtype)
			{
			case TOF_CARD_RSSI:
			{
				if(psFrame->u8SduLength != sizeof(rf_tof_card_data_ts)) return;

				if( SysUtil_vConvertLQI2Dbm(psFrame->u8LinkQuality) > i8Rssi_Threshold )
				{
					u8LightTimeout =LIGHT_ON_TIMEOUT;
					//PrintfUtil_vPrintf("cardnum %d\n",psFrame->sSrcAddr.uAddr.u16Short);
					LedUtil_vToggle(LED_RF);
				}

				//支持rssi上报
				if(u8IsRssiStation)
				{
					app_rssi_data.vehicle_rssi[u16RssiNum].u16ShortAddr = psFrame->sSrcAddr.uAddr.u16Short;
					app_rssi_data.vehicle_rssi[u16RssiNum].uu16SeqNum = psAppPkt->rf_tof_card_data.u16SeqNum;
					app_rssi_data.vehicle_rssi[u16RssiNum].u8Reserved = 0;
					app_rssi_data.vehicle_rssi[u16RssiNum].i8Rssi = SysUtil_vConvertLQI2Dbm(psFrame->u8LinkQuality);
					//PrintfUtil_vPrintf("cardnum %d\n",psFrame->sSrcAddr.uAddr.u16Short);

					
					u16RssiNum++;
					if(6 == u16RssiNum)
					{
						vReportCardRssi();
						//bReported = TRUE;
					}
				}

				break;
			}

			default:
			{
				break;
			}

			}
		}
		break;
	}

	default:
	{
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
		EDBG(PrintfUtil_vPrintf("air msg  len ERR! len:%d\n", psFrame->u8SduLength););
		return;
	}

	app_header_t *pheader = (app_header_t *)(psFrame->au8Sdu);

	DBG(
	if(pheader->msgtype == MP_VOICE)
	{
		app_mpVoice_t* pVoice = (app_mpVoice_t*)(pheader + 1);
		static uint32 cnt;
		if(cnt++ % 20 == 0)
		{
		    PrintfUtil_vPrintf("seq:%d,%d\n",pVoice->seqnum,pVoice->hdr.dstaddr);
		}
	}
	);

	uint16 headerlen = pheader->len;
	CONVERT_ENDIAN(headerlen);

	if(headerlen + sizeof(app_header_t) != psFrame->u8SduLength)
	{
		EDBG(PrintfUtil_vPrintf("app_header len ERR!\n"););
		return;
	}

	//update_station_rssi(u16StationPanId,SysUtil_vConvertLQI2Dbm(psFrame->u8LinkQuality),INVALID_RSSI,MP_RSSI_MPSTATION);

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
			if(srcaddr != psFrame->sSrcAddr.u16PanId)
			{
			 	PrintfUtil_vPrintf("shortaddr ERR! s addr: %d, p addr: %d\n", psFrame->sSrcAddr.uAddr.u16Short, srcaddr);
			}
		}
		);

		if((psFrame->u8SduLength + sizeof(struct nwkhdr)) <= BSMAC_MAX_TX_PAYLOAD_LEN)
		{
			struct nwkhdr *pNwkHdr = (struct nwkhdr *)u32CommonBuf;
			pNwkHdr->type = NWK_DATA;
			pNwkHdr->ttl = 255;

			/*vAHI_StartRandomNumberGenerator(E_AHI_RND_SINGLE_SHOT, E_AHI_INTS_DISABLED);
			random_time = (u16AHI_ReadRandomNumber() & 10);

			memcpy((uint8 *)u32CommonBufDown,(uint8 *)pheader,headerlen + sizeof(app_header_t));
			u8LengthDown = headerlen + sizeof(app_header_t);

			TimerUtil_eSetTimer(STATION_DOWN_RADIO_DATA_EVENT, random_time);

			Hal_SendDataToAir((uint8 *)pheader, headerlen + sizeof(app_header_t), MOBILEPHONE_NWK_ADDR, 0xFFFF, TRUE);*/

			pNwkHdr->src = u16StationPanId;
			SysUtil_vConvertEndian(&pNwkHdr->src, sizeof(pNwkHdr->src));

			pNwkHdr->dst = u16ArmId;
			SysUtil_vConvertEndian(&pNwkHdr->dst, sizeof(pNwkHdr->dst));

			pNwkHdr->len =  psFrame->u8SduLength;
			SysUtil_vConvertEndian(&pNwkHdr->len, sizeof(pNwkHdr->len));

			memcpy(((uint8 *)u32CommonBuf) + sizeof(struct nwkhdr), (uint8 *)(psFrame->au8Sdu), psFrame->u8SduLength);
			if(UART_SUCCESS!=u8Uart_StartTx((uint8 *)(u32CommonBuf), psFrame->u8SduLength + sizeof(struct nwkhdr)))
			{
			    EDBG(PrintfUtil_vPrintf("Err: Uart Send Data Failed\n"););
			}
			//Jbsmac_eWriteData((uint8 *)(u32CommonBuf), psFrame->u8SduLength + sizeof(struct nwkhdr));
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
			PrintfUtil_vPrintf("MP_SCAN!\n");
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
			psFrame->sSrcAddr.u16PanId, 0x0000, TRUE);
		}
		else
		{
		 	EDBG(PrintfUtil_vPrintf("scan err!\n"););
		}
	}

	else if(pheader->msgtype == VEHICLE_BATTERY_VER)
	{
		if((psFrame->u8SduLength + sizeof(struct nwkhdr)) <= BSMAC_MAX_TX_PAYLOAD_LEN)
		{
			struct nwkhdr *pNwkHdr = (struct nwkhdr *)u32CommonBuf;
			app_header_t *pHeader = (app_header_t *)(pNwkHdr + 1);

			uint16 len;

			app_Vehicle_Battery_VER_t *pStationReport = (app_Vehicle_Battery_VER_t *)(pHeader + 1);

			pHeader->msgtype = APP_VEHICLE_BATTERY_VER;
			pHeader->protocoltype = APP_PROTOCOL_TYPE_MOBILE;

			pStationReport->battery = *(((uint8 *)(psFrame->au8Sdu)) + sizeof(app_header_t));
			pStationReport->vehicle_card_addr = psFrame->sSrcAddr.u16PanId;
			SysUtil_vConvertEndian(&pStationReport->vehicle_card_addr, sizeof(pStationReport->vehicle_card_addr));

			pStationReport->len = psFrame->u8SduLength - 5;

			pHeader->len = sizeof(app_Vehicle_Battery_VER_t) + pStationReport->len;

			pNwkHdr->type = NWK_DATA;
			pNwkHdr->ttl = 255;

			pNwkHdr->src = u16StationPanId;
			SysUtil_vConvertEndian(&pNwkHdr->src, sizeof(pNwkHdr->src));

			pNwkHdr->dst = u16ArmId;
			SysUtil_vConvertEndian(&pNwkHdr->dst, sizeof(pNwkHdr->dst));

			pNwkHdr->len = pHeader->len + sizeof(app_header_t);

			len = pNwkHdr->len + sizeof(struct nwkhdr);
			SysUtil_vConvertEndian(&pNwkHdr->len, sizeof(pNwkHdr->len));

			SysUtil_vConvertEndian(&pHeader->len, sizeof(pHeader->len));

			memcpy(((uint8 *)pStationReport) + sizeof(app_Vehicle_Battery_VER_t), ((uint8 *)(psFrame->au8Sdu))+5, psFrame->u8SduLength - 5);
			if(UART_SUCCESS!=u8Uart_StartTx((uint8 *)(u32CommonBuf), len))
			{
			    EDBG(PrintfUtil_vPrintf("Err: Uart Send Data Failed\n"););
			}
			//Jbsmac_eWriteData((uint8 *)(u32CommonBuf), len);
		}
	}
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

	if(APP_PROTOCOL_TYPE_LIGHT == pheader->protocoltype)
	{

		switch(pheader->msgtype)
		{
		case APP_LIGHT_MSG_REPORT_ACK:
		{
			app_Light_ReportAck_t * pReportAck = (app_Light_ReportAck_t *)pbuf;
			//if(pReportAck->u8MaxLightLevel >10 || pReportAck->u8MinLightLevel>10)
			//{
				//EDBG(PrintfUtil_vPrintf("lightlevel error %d,%d\n", pReportAck->u8MaxLightLevel,pReportAck->u8MinLightLevel););
				//return;
			//}


			if(u8MaxLightLevel != pReportAck->u8MaxLightLevel || u8MinLightLevel != pReportAck->u8MinLightLevel
				|| i8Rssi_Threshold != pReportAck->i8Rssi_Threshold || u8IsRssiStation != pReportAck->u8IsRssiStation)
			{
				bool flag = 0;
				if(pReportAck->u8MaxLightLevel < 11)
				{
					if(u8MaxLightLevel != pReportAck->u8MaxLightLevel)
					{
						u8MaxLightLevel = pReportAck->u8MaxLightLevel;
						flag = 1;
						PrintfUtil_vPrintf("max:%d",u8MaxLightLevel);
					}
				}
				if(pReportAck->u8MinLightLevel < 11)
				{
					if(u8MinLightLevel != pReportAck->u8MinLightLevel)
					{
						u8MinLightLevel = pReportAck->u8MinLightLevel;
						flag = 1;
						PrintfUtil_vPrintf("min:%d",u8MinLightLevel);
					}
				}
				if(pReportAck->i8Rssi_Threshold <0 && pReportAck->i8Rssi_Threshold > -90)
				{
					if(i8Rssi_Threshold != pReportAck->i8Rssi_Threshold)
					{
						i8Rssi_Threshold = pReportAck->i8Rssi_Threshold;
						flag = 1;
						PrintfUtil_vPrintf("Rssi_Threshold:%i",i8Rssi_Threshold);
					}
				}
				if(pReportAck->u8IsRssiStation == 0 || pReportAck->u8IsRssiStation ==1)
				{
					if(u8IsRssiStation != pReportAck->u8IsRssiStation)
					{
						u8IsRssiStation = pReportAck->u8IsRssiStation;
						flag = 1;
						PrintfUtil_vPrintf("IsRssiStation:%d",u8IsRssiStation);
					}
				}
				if(flag)
					_save_parameter();
			}
			break;
		}

		case APP_LIGHT_MSG_SET:
		{
			app_Light_ChannelSeting_t * pChannelSeting= (app_Light_ChannelSeting_t *)pbuf;
			cfg_option.u8Channel = pChannelSeting->u8Channel;
			_save_parameter();
			vAHI_SwReset();
			break;
		}

		default:
		{
			break;
		}
		}

	}
}

/*PRIVATE bool_t bArmReady()
{

}*/

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
	if(UART_SUCCESS!=u8Uart_StartTx((uint8 *)u32CommonBuf, sizeof(struct nwkhdr) + sizeof(app_header_t) + sizeof(app_LSrfReport_t)))
	{
	    EDBG(PrintfUtil_vPrintf("Err: Uart Send Data Failed\n"););
	}
	//Jbsmac_eWriteData((uint8 *)u32CommonBuf, sizeof(struct nwkhdr) + sizeof(app_header_t) + sizeof(app_LSrfReport_t));
}

/****************************************************************************
 *
 * NAME: vWriteData2Stm
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
PRIVATE void vWriteData2Stm(uint8* pbuf)
{
	app_header_t* pAppHead = (app_header_t*) pbuf;
	uint16 u16App_len = pAppHead->len;
	uint16 u16CardIndex;

	switch (pAppHead->msgtype)
	{
	case APP_LIGHT_MSG_CARD_RSSI:
	{
		app_vehicle_rssi_ts* pRssi_data = (app_vehicle_rssi_ts*)pbuf;
		uint16 u16CardLen = (u16App_len - 4) /6; //sizeof(vehicle_rssi_ts);

		//vConvertEndian16(&(pRssi_data->u16station_addr));
		//vConvertEndian16(&(pRssi_data->u16seqnum));
		for (u16CardIndex = 0; u16CardIndex < u16CardLen; u16CardIndex++)
		{
			vConvertEndian16(&(pRssi_data->vehicle_rssi[u16CardIndex].u16ShortAddr));
			vConvertEndian16(&(pRssi_data->vehicle_rssi[u16CardIndex].uu16SeqNum));
		}
		break;
	}

	#if 0
	case APP_TOF_MSG_ALARM:
	case APP_TOF_MSG_NEW_ALARM:
	{
		app_tof_alarm_ts* pAlarm_data = (app_tof_alarm_ts*) pbuf;
		vConvertEndian16(&(pAlarm_data->u16ShortAddr));
		break;
	}
	#endif

	default:
		break;
	}

	light_sync_hdr_t *psync_hdr = (light_sync_hdr_t *)u8ThroughBuf;
	psync_hdr->data_len = pAppHead->len + sizeof(app_header_t);

	uint16 *pCrc = (uint16 *)((uint8 *)(psync_hdr + 1)+psync_hdr->data_len);

	psync_hdr->sync[0] = 'Y';
	psync_hdr->sync[1] = 'I';
	psync_hdr->sync[2] = 'R';


	memcpy((uint8 *)(psync_hdr+1),pbuf,psync_hdr->data_len);

	*pCrc = CRC16((uint8 *)u8ThroughBuf,psync_hdr->data_len+4, 0xFFFF);

	if(UART_SUCCESS!=u8Uart_StartTx((uint8*)u8ThroughBuf,psync_hdr->data_len+sizeof(light_sync_hdr_t)+2))
	{
	    EDBG(PrintfUtil_vPrintf("Err: Uart Send Data Failed\n"););
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
		app_rssi_data.app_tof_head.len = u16RssiNum*6 +4;
		vWriteData2Stm((uint8*)&app_rssi_data);

		app_rssi_data.u16seqnum++;

		u16RssiNum = 0;
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
	TimerUtil_eSetTimer(STATION_STOP_BUZZER_EVENT, u32Ms);
	vAHI_TimerDIOControl(TOF_BUZZER_TIMER, TRUE);
	vAHI_TimerEnable(TOF_BUZZER_TIMER, 8, FALSE, FALSE, TRUE);
	vAHI_TimerConfigureOutputs(TOF_BUZZER_TIMER,TRUE,TRUE);
	vAHI_TimerClockSelect(TOF_BUZZER_TIMER, FALSE, TRUE);

	if(u8LightTimeout > 0)
	{
		u8LightTimeout--;
	}

	if((u8LightTimeout > 0))
	{
		if(u8MaxLightLevel > 0 && u8MaxLightLevel <11)
		{
			vAHI_TimerStartRepeat(TOF_BUZZER_TIMER, u8PwmParameter[u8MaxLightLevel], 100);
			 vAHI_DioSetOutput(LIGHT_KEY_PIN,0);
		}
		else
			vAHI_DioSetOutput(0,LIGHT_KEY_PIN);
	}
	else
	{
		PrintfUtil_vPrintf("disapper\n");
		if(u8MinLightLevel > 0 && u8MinLightLevel <11)
		{
			vAHI_TimerStartRepeat(TOF_BUZZER_TIMER, u8PwmParameter[u8MinLightLevel], 100);
			vAHI_DioSetOutput(LIGHT_KEY_PIN,0);
		}
		else
			vAHI_DioSetOutput(0,LIGHT_KEY_PIN);
	}
	//u8ProtectCounter = 0;
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

	vAHI_DioSetDirection(0,TOF_BUZZER_PIN);
	vAHI_DioSetOutput(0,TOF_BUZZER_PIN);
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
		if(UART_SUCCESS!=u8Uart_StartTx((uint8 *)u32CommonBuf,
		              sizeof(struct nwkhdr) + sizeof(app_header_t) + sizeof(app_LSrfReport_t)+tsVersionReportLen*sizeof(app_LSVersionReport_t)))
		{
		    EDBG(PrintfUtil_vPrintf("Err: Uart Send Data Failed\n"););
		}
		//Jbsmac_eWriteData((uint8 *)u32CommonBuf,
		              //sizeof(struct nwkhdr) + sizeof(app_header_t) + sizeof(app_LSrfReport_t)+tsVersionReportLen*sizeof(app_LSVersionReport_t));
		DBG(
		PrintfUtil_vPrintf("Send card version \n");
		PrintfUtil_vPrintMem((uint8 *)u32CommonBuf, sizeof(struct nwkhdr) + sizeof(app_header_t) + sizeof(app_LSrfReport_t)+tsVersionReportLen*sizeof(app_LSVersionReport_t));
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
PRIVATE bool  vSaveChannel(uint8 u8Channel)
{
	uint8 u8rssichannel;
	if(!bAHI_FullFlashRead(PARAM_STORE_ADDR, sizeof(uint8),(uint8*)&u8rssichannel))
	{
		PrintfUtil_vPrintf("Flash Read Fail\n");
		return FALSE;
	}

	if(!bAHI_FlashEraseSector(7))
	{
		PrintfUtil_vPrintf("Flash Erase Fail\n");
		return FALSE;
	}

	if(bAHI_FullFlashProgram(PARAM_STORE_ADDR, sizeof(uint8),(uint8*)&u8Channel))
	{
		PrintfUtil_vPrintf("Set  channel Success!%d\n", u8Channel);
		return TRUE;
	}
	else
	{
		PrintfUtil_vPrintf("Set  channel fail!%d\n",u8Channel);
		return FALSE;
	}
}

#if 0

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
#endif

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

PRIVATE void vSyncLEDStatus(void)
{


}

PRIVATE void vConvertEndian16(uint16* pu16Data)
{
	SysUtil_vConvertEndian(pu16Data, sizeof(uint16));
}


PRIVATE void reset_report_rssi(void)
{
	rssiReport.app_rssi_head.protocoltype =0;
	rssiReport.receiveRssi = INVALID_RSSI;
	rssiReport.sentRssi = INVALID_RSSI;
}

/*PRIVATE void update_station_rssi(uint16 u16Short,int8 receivei8Rssi,int8 senti8Rssi,uint8 msgtype)
{
	rssiReport.app_rssi_head.protocoltype = APP_PROTOCOL_TYPE_MOBILE;
	rssiReport.app_rssi_head.msgtype = msgtype;
	rssiReport.app_rssi_head.len = sizeof(app_rssi_report)-sizeof(app_header_t);
	SysUtil_vConvertEndian(&rssiReport.app_rssi_head.len, sizeof(rssiReport.app_rssi_head.len));
	rssiReport.u16ShortAddr =  u16Short;
	SysUtil_vConvertEndian(&rssiReport.u16ShortAddr, sizeof(rssiReport.u16ShortAddr));
	rssiReport.receiveRssi  =  receivei8Rssi;
	rssiReport.sentRssi=  senti8Rssi;
}*/

PRIVATE void CS5460_Write_Byte(uint8 data)
{
	uint8 q = 0;
	for(q=0; q<8; q++)
	{
		if((data&0x80)==0x80)
		{
			vAHI_DioSetOutput(CS5460_SDO_PIN,0);
		}else
		{
			vAHI_DioSetOutput(0,CS5460_SDO_PIN);
		}
		TimerUtil_vDelay(20,E_TIMER_UNIT_MICROSECOND);
		data<<=1;
		vAHI_DioSetOutput(CS5460_CLK_PIN,0);
		TimerUtil_vDelay(20,E_TIMER_UNIT_MICROSECOND);
		vAHI_DioSetOutput(0,CS5460_CLK_PIN);
		TimerUtil_vDelay(20,E_TIMER_UNIT_MICROSECOND);
	}
	vAHI_DioSetOutput(CS5460_SDO_PIN,0);;
	TimerUtil_vDelay(20,E_TIMER_UNIT_MICROSECOND);
}

PRIVATE void CS5460_Write_com(uint8 com)
{
	vAHI_DioSetOutput(0,CS5460_CS_PIN);
	TimerUtil_vDelay(20,E_TIMER_UNIT_MICROSECOND);
	CS5460_Write_Byte(com);
	vAHI_DioSetOutput(CS5460_CS_PIN,0);
	TimerUtil_vDelay(20,E_TIMER_UNIT_MICROSECOND);
}

PRIVATE uint32 CS5460_Read_Register(uint8 com)
{
	uint32 data = 0;
	uint8 q = 0;
	vAHI_DioSetOutput(0,CS5460_CS_PIN);
	TimerUtil_vDelay(20,E_TIMER_UNIT_MICROSECOND);
	CS5460_Write_Byte(com<<1);
	for(q=0;q<24;q++)
	{
		data<<=1;
		vAHI_DioSetOutput(CS5460_CLK_PIN,0);
		TimerUtil_vDelay(20,E_TIMER_UNIT_MICROSECOND);
		//if(CS5460_SDO_READ)
		if(u32AHI_DioReadInput() & CS5460_SDI_PIN)
		{
			data++;
		}
		vAHI_DioSetOutput(0,CS5460_CLK_PIN);
		TimerUtil_vDelay(20,E_TIMER_UNIT_MICROSECOND);
	}
	vAHI_DioSetOutput(CS5460_CS_PIN,0);
	return data;
}

PRIVATE void CS5460_Write_Register(uint8 com, uint32 data)
{
	uint8 temp = 0;
	vAHI_DioSetOutput(0,CS5460_CS_PIN);
	TimerUtil_vDelay(20,E_TIMER_UNIT_MICROSECOND);
	CS5460_Write_Byte(0x40|(com<<1));
	temp=(data>>16);
	CS5460_Write_Byte(temp);
	temp=(data>>8);
	CS5460_Write_Byte(temp);
	temp=data;
	CS5460_Write_Byte(temp);
	vAHI_DioSetOutput(CS5460_CS_PIN,0);
}

PRIVATE void Reset_CS5460()
{
	vAHI_DioSetOutput(0,CS5460_RST_PIN);
	TimerUtil_vDelay(40,E_TIMER_UNIT_MICROSECOND);

	vAHI_DioSetOutput(CS5460_RST_PIN,0);

	TimerUtil_vDelay(40,E_TIMER_UNIT_MICROSECOND);

	vAHI_DioSetOutput(CS5460_CLK_PIN,0);
	vAHI_DioSetOutput(CS5460_SDO_PIN,0);

	CS5460_Write_Register(0,1);

	TimerUtil_vDelay(40,E_TIMER_UNIT_MICROSECOND);
	//CS5460_Write_Register(5,800);
	//CS5460_Write_com(0XE8);
	CS5460_Write_com(0XE8);

	TimerUtil_vDelay(40,E_TIMER_UNIT_MICROSECOND);
}









