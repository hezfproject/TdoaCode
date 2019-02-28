/**************************************************************************************************
Filename:       CR.c
Revised:        $Date: 2011/08/01 20:41:20 $
Revision:       $Revision: 1.17 $

Description:    This file contains the application that can be use to set a device as Location
node from MAC directly which will communicate with a FFD with full z-stack.

**************************************************************************************************/

/**************************************************************************************************
*                                           Includes
**************************************************************************************************/
#include <stdio.h>
#include <string.h>

/* Hal Driver includes */
#include "hal_types.h"
#include "hal_key.h"
#include "hal_timer.h"
#include "hal_drivers.h"
#include "hal_led.h"
#include "hal_assert.h"
#include "hal_uart.h"

/* OS includes */
#include "OSAL.h"
#include "OSAL_Tasks.h"
#include "OSAL_Clock.h"
//#include "OSAL_Nv.h"

/* App Protocol*/
#include "App_cfg.h"

/* Application Includes */
#include "OnBoard.h"

/* MAC Application Interface */
#include "mac_api.h"
#include "mac_main.h"
#include "mac_pib.h"
#include "ZComDef.h"
/* Application */
#include "Station.h"

/* watchdog util */
#include "watchdogutil.h"
#include "MacUtil.h"
#include "crc.h"

/* FLASH */
#include "Hal_flash.h"

#include "Lpbssnwk.h"
#include "Hal_sleep.h"
#include "Version.h"

/**************************************************************************************************
*                                           Constant
**************************************************************************************************/
//UART
#define UART_BAUDRATE 115200 //bit per second
#define UART_FRAME_TIMEOUT 30 // millisecond


#if (UART_BAUDRATE == 115200)
#define STATION_APP_BAUD  HAL_UART_BR_115200
#endif

#if (UART_BAUDRATE == 9600)
#define STATION_APP_BAUD  HAL_UART_BR_9600
#endif

#if !defined( STATION_APP_RX_MAX )
#define STATION_APP_RX_MAX  150
#endif

// When the Rx buf space is less than this threshold, invoke the Rx callback.
#if !defined( STATION_APP_THRESH )
#define STATION_APP_THRESH  130
#endif



#if !defined( STATION_APP_TX_MAX )
#define STATION_APP_TX_MAX  250
#endif

// Millisecs of idle time after a byte is received before invoking Rx callback.
#if !defined( CR_APP_IDLE )
#define CR_APP_IDLE  8
#endif


/**************************************************************************************************
*                                           Macros
**************************************************************************************************/
#define STATION_RETREAT_NORMAL                 0

#define STATION_LED_BLUE     HAL_LED_1

#define LF_SEND_CONSEL      P0SEL
#define LF_SEND_CONDIR      P0DIR
#define LF_SEND_CONPIN      P0_2
#define LF_SEND_CONBIT      BV(2)

#define  LF_DATA_MAX_LEN    (LF_DATA_LEN*5)
#define  WIRE_DATA_MAX_LEN   (sizeof(app_mDev_hdr_t)+sizeof(app_mDev_fdr_t)+ MAC_MAX_FRAME_SIZE)
#define  DATA_START_BYTE_NUM    1

#define  LINKED         BV(0)  // L:unlink  H:linked
#define  LF_0R_1W       BV(1)  // l:read    H:write


#define SHORT_DISTANCE    0
#define REMOTE_DISTANCE     1
#define POS_MODE          2
#define POS_CHECKIN       3

#define DEVICE_POS_IDLE     0
#define DEVICE_POS_READING     1
#define DEVICE_POS_SETTING     2
#define DEVICE_POS_ERP     3

#define HAL_FLASH_INFOMATION_SIZE   (HAL_NV_PAGE_CNT * HAL_FLASH_PAGE_SIZE) //1024
#define HAL_FLASH_INFOMATION_PAGE   (HAL_NV_PAGE_END - HAL_NV_PAGE_CNT + 1)
#define HAL_FLASH_INFOMATION_OSET   (0)




/*********************************************************************
* TYPEDEFS
*/



typedef enum
{
    CR_REPORT_DATA,
    CR_RESPOND,
    CR_EXCEPTION,
} eUartSendType;

typedef enum
{
    CRC_ERROR = 0,
    LEN_SHORT,
    NON_MINE,
    GOOD_WORK,
} eUartRecReturn;


typedef enum
{
	TOF_LOCATOR_REQUEST = 0x01,	//locator request to join a station
	TOF_LOCATOR_FINISH,
	TOF_LOCATOR_CARDS_DIST,

	TOF_CARD_CHECKIN,			// card send regist msg // = 4
	TOF_CARD_RSSI,				// card send RSSI msg
	TOF_CARD_REQUEST,			// card request tof to specify station
	TOF_CARD_ALARM, 			// card alarm

	TOF_STATION_AVAILABLE, 	// station is available, for locaotr or card to join // = 8
	TOF_STATION_ILDE, 			// station has idle slot, card need wait for AVAILABLE cmd to join request
	TOF_STATION_RSSI,			// station need card to send rssi info
	TOF_STATION_BUSY,			// station has not idle slot, card need wait for RSSI cmd to send rssi // = B
	TOF_STATION_ACCEPT,		// station accepts card or locator // = C
	TOF_STATION_FINISH,		//station tof finished // = D
	TOF_STATION_HELP_ACK, 	// the station receive help from cards // = E
	TOF_STATION_NOPWD_ACK, 	// the station receive nopwd from cards // = F
	TOF_STATION_CHECKIN_ACK,	// the station receive card's regist info // = 0x10
	TOF_STATION_CARDS_INFO,	// station send cards' info to locator, to indicate which cards need locating // = 0x11
	TOF_APP_HELP_ACK, 			// the station receive help ack from app, need let card know // = 0x12
	TOF_STATION_WAIT, 		// Station ask card to wait

    TOF_GASNODE_REQUEST,
    TOF_GASDENSITY,
    TOF_GASNODE_ALARM,
    TOF_GASNODE_RSSI,
    TOF_GASNODE_HELP_ACK,  //24
    TOF_GASNODE_FIRE_ACK,
    TOF_GASNODE_WATER_ACK,
    TOF_GASNODE_TOPBOARD_ACK,
    TOF_GASNODE_OTHER_ACK,

    TOF_STATION_LOC_INFO,   // Station send it's panid to locator

    TOF_LOC_VERSION_INFO,
    TOF_CARD_EXCITE,
    TOF_STATION_EXCITE_ACK,

    CARD_BASE_INFO,   //33
    CARD_REMARK,
    HANDLE_STATION_ACK,
    CHANGE_PERIOD,
    CARD_TYPE_CHEACK_ACK,
    TOF_STATION_RSSI_CHECK,
    TOF_STATION_RSSI_CHECK_ACK,   //39

    TOF_STATION_RSSI_CHANNEL = 40,

    TOF_CARD_TRANSFER_THROUGH,      //透传

    HAND_STATION_RSSI_CHANNEL,     //手持机修改rssi频道
    HAND_STATION_RSSI_CHANNEL_ACK,
    HAND_STATION_WRITE_ACK,
    CARD_LOC_INFO,
    DEVICE_CARD_PERIOD_CHANNEL =46,
    DEVICE_CARD_PERIOD_CHANNEL_ACK = 47,

	TOF_CARD_OAD = 0xF0,		// card's OAD controller send this msg to card when card do "checkin" event
	TOF_LOCATOR_OAD = 0xF2,	// locator's OAD controller send this msg to locator
	TOF_LOC_STATION_OAD = 0xF4,	// station's OAD controller send this msg to station
	TOF_RSSI_STATION_OAD = 0xF5,	// station's OAD controller send this msg to station
	TOF_CHECKIN_STATION_OAD = 0xF6,	// station's OAD controller send this msg to station
	TOF_COM_STATION_OAD = 0xF7,	// station's OAD controller send this msg to station

}rf_tof_msg_te;



typedef struct
{
    uint8 ResetFlag;
} storeParam_t;

typedef struct
{
    bool    sendingFlag;
    bool    needSentFlag;
    uint16  len;
    uint8   LfData[LF_DATA_MAX_LEN+1];
} LFDataBuff_t;

typedef struct
{
    uint8    protocoltype;       /* always be the source protocol type */
    uint8    msgtype;
    uint16    len;
} app_header_t;
typedef struct
{
  app_header_t tof_head;
  uint8 sequnum;
}ACK_PACKET_T;

typedef struct
{
  app_header_t tof_head;
  uint8 cardtype;
}CHARGE_CARD_TYPE_T;


typedef struct
{
	app_header_t tof_head;

	uint16 u16SeqNum;
	uint8 u8CardStatus;	// tof_card_status_te
	uint8 u8Reserved;
	uint16 u16OadVersion;
	uint16 u16Battery;
}rf_tof_card_data_ts;


typedef struct
{
	app_header_t tof_head;
	uint8 u8period;	// card period
}rf_tof_card_period_ts;

typedef struct
{
	app_header_t tof_head;

	uint16 u16SeqNum;
	uint8 u8CardStatus;	// tof_card_status_te & card type & exciter
	//uint8 u8Reserved;
	uint8   u8ExciterIDorAccStatus;   // corresponding to (u8CardStatus & 0x80)true:ExciterID;false:AccelerometerStatus
	uint16 u16OadVersion;
	uint16 u16Battery;
    app_eDev_Data_t info_data;
}rf_card_info1_ts;

typedef struct
{
	app_header_t tof_head;
    app_eDev_Data_t info_data;
}rf_card_info2_ts;

ACK_PACKET_T ack_packet;

CHARGE_CARD_TYPE_T charge_card_type;


typedef struct
{
    app_header_t tof_head;
    int8 i8Rssi;
    uint8 u8StationPort;
}rf_tof_station_rssi_ts;

typedef struct
{
  app_header_t tof_head;
  uint16 u16StationAddr;
  uint16 u16LocDistance;
}rf_card_distance_ts;

typedef struct
{
	app_header_t tof_head;

	uint16 u16SeqNum;
	uint8 u8CardStatus;	// tof_card_status_te & card type & exciter
	//uint8 u8Reserved;
	uint8   u8ExciterIDorAccStatus;   // corresponding to (u8CardStatus & 0x80)true:ExciterID;false:AccelerometerStatus
	uint16 u16OadVersion;
	uint16 u16Battery;
    uint8  u8Erpid[20];
}rf_tof_card_base_data_ts;

typedef struct
{
	app_header_t tof_head;
    uint8  u8Erpid[20];
}rf_tof_erp_id_data_ts;



typedef union
{
	app_header_t tof_head;
	rf_tof_card_data_ts rf_tof_card_data;
    rf_card_info1_ts  rf_card_info1;
    rf_card_info2_ts  rf_card_info2;
    rf_tof_station_rssi_ts rf_tof_station_rssi;
    rf_card_distance_ts  rf_card_distance;
    rf_tof_card_base_data_ts rf_tof_card_base_data;
    rf_tof_erp_id_data_ts rf_tof_erp_id_data;
}RfTofWrapper_tu;

typedef struct
{
	uint32 u32TimeTick;
    uint16 u16ShortAddr;
    int8   i8CardRssi;
}rt_card_info_ts;

typedef struct
{
    uint16 u16ShortAddr;
    int8   i8CardRssi[3];
}rt_station_info_ts;


typedef struct
{

    uint16 u32StationPanId;
    int8   i8RecvRssi;
    int8   i8TransmitRssi;
}rt_Station_rssi_ts;



typedef struct
{
    uint16 u16ShortAddr;
    int8   i8CardRssi;
}rt_card_rssi_ts;



/* Size table for MAC structures */
const CODE uint_8 station_cbackSizeTable[] =
{
    0,                                               /* unused */
    sizeof(macMlmeAssociateInd_t),       /* MAC_MLME_ASSOCIATE_IND */
    sizeof(macMlmeAssociateCnf_t),       /* MAC_MLME_ASSOCIATE_CNF */
    sizeof(macMlmeDisassociateInd_t),    /* MAC_MLME_DISASSOCIATE_IND */
    sizeof(macMlmeDisassociateCnf_t),    /* MAC_MLME_DISASSOCIATE_CNF */
    sizeof(macMlmeBeaconNotifyInd_t),    /* MAC_MLME_BEACON_NOTIFY_IND */
    sizeof(macMlmeOrphanInd_t),           /* MAC_MLME_ORPHAN_IND */
    sizeof(macMlmeScanCnf_t),              /* MAC_MLME_SCAN_CNF */
    sizeof(macMlmeStartCnf_t),             /* MAC_MLME_START_CNF */
    sizeof(macMlmeSyncLossInd_t),        /* MAC_MLME_SYNC_LOSS_IND */
    sizeof(macMlmePollCnf_t),                /* MAC_MLME_POLL_CNF */
    sizeof(macMlmeCommStatusInd_t),      /* MAC_MLME_COMM_STATUS_IND */
    sizeof(macMcpsDataCnf_t),            /* MAC_MCPS_DATA_CNF */
    sizeof(macMcpsDataInd_t),            /* MAC_MCPS_DATA_IND */
    sizeof(macMcpsPurgeCnf_t),           /* MAC_MCPS_PURGE_CNF */
    sizeof(macEventHdr_t)                  /* MAC_PWR_ON_CNF */
};

/***********************************************************
 *                  Global Variables
 */
bool sentFlag = false;

/***********************************************************
 *                  Local Variables
*/
/* TRUE and FALSE value */
static bool Station_MACTrue = TRUE;
static bool Station_MACFalse = FALSE;

/* Task ID */
uint8 Station_TaskId;

/* Device Info from flash */
Station_DevInfo_t Station_DevInfo;
app_header_t *tof_head_open;
app_header_t *tof_head_write;
app_header_t *tof_head_close;
app_header_t *tof_head_ack;

static uint8 comm_rx_buf[128];

app_header_t * p_app_hdr;
uint16 len_temp;


static uint16 dstID = 0xffff;
static uint16 dstDevAdr = 0xffff;
static uint16 dstAdr = 0xffff;

static uint16 dstPanID = 0xffff;


static uint8 peroid;
static uint8 RssiChannel;
//static uint8 channel;
//static bool DoubleChannel = false;
//static uint8 ChannelPoll = 0;

static uint16 saveDevID;
static uint8  mDevStatus = 0;
static uint8  HandheldStatus = 0;
static uint8  DevicePosState = DEVICE_POS_IDLE;
static uint8  eDevType = 0;
static uint8  linkCount = 0;
static uint8 u8RssiCnt = 0;

static uint8 u8ReportIdx = 0;   //上报哪5条维修记录

static uint8 u8SetRssiChannel = 0;

static uint8 u8CurChannel = 0;


static uint8 u8SetBroadcastChannel = 25;

RfTofWrapper_tu StationRssiPacket;


typedef enum
{
    E_STATE_IDLE,
    E_STATE_READ,
    E_STATE_WRITE,
    E_STATE_CHANGE,

    E_WAIT_STATION_RSSI,
    E_STATE_RSSI_CHANNEL,
    E_TEST_STATION_RSSI,
} teState;

typedef struct
{
	uint8 u8ErpIdLen;
    uint8 u8ErpBuf[20];
}rt_erpid_info_ts;

rt_erpid_info_ts rt_erpid_info;

teState teStaionState = E_STATE_IDLE;
teState old_teStaionState;


uint8 teExciteState = 0;     //关激励

static uint8  WireSendBuff[WIRE_DATA_MAX_LEN+10];
static uint8  WriteSendBuff[256];

static uint8  WriteSendBuff[256];

//static uint8  sLFBuf[LF_DATA_MAX_LEN+DATA_START_BYTE_NUM+1];
/* buf send on LF  */
//static LFDataBuff_t  LFDataBuff[2];

rt_card_info_ts card_info[40];

rt_station_info_ts station_info[10];
uint8 station_cnt = 0;
uint8 locator_cnt = 0;
uint8 idx = 0;

uint16 dstLoctor;

uint8 buf_tail;
uint8 buf_head;


/* UART*/
static uint8 uartReadBufLen1 = WIRE_DATA_MAX_LEN;
static uint8 uartReadBuf1[WIRE_DATA_MAX_LEN+10];
static uint8 uartRXBuf1[WIRE_DATA_MAX_LEN+10];

//static uint8 uartReadLen1;
uint8 receivedOpenCmd=FALSE;
uint8 receivedWriteCmd=FALSE;
uint8 receivedCloseCmd=FALSE;

/**************************************************************************************************
*                                     Local Function Prototypes
**************************************************************************************************/

static void  init_uart(uint8  uartPort , uint8 baudRate);
static void  Station_Startup(void);
static void  Station_ReadDevInfo(void);
static eUartRecReturn  Station_ParseUartCmd(uint_8* data, uint_16 len);
static void  Station_start_timerEx(uint8 taskID,
                                         uint16 event_id,
                                         uint16 timeout_value);
static void  Station_ParseDataCnf(const macMcpsDataCnf_t* pdatacnf);
static void  Station_ParseFrame(const macMcpsDataInd_t* pdataInd);
static uint8 Station_ParseCardFrame(const sAddr_t* sAddr,uint16 srcPanId,const sData_t msdu,int8 rssi);
static void SysUtil_vConvertEndian(void* pvData, uint8 u8Len);
static uint8 Station_ParseCardDeviceFrame(const sAddr_t* sAddr,uint16 srcPanId,const sData_t msdu,int8 rssi);
static uint8 Short_ParseCardFrame(const sAddr_t* sAddr,uint16 srcPanId,const sData_t msdu,int8 rssi);
static uint8 Device_ParseCardFrame(const sAddr_t* sAddr,uint16 srcPanId,const sData_t msdu,int8 rssi);
static void Station_ParseRssiFrame(const sAddr_t* sAddr,uint16 srcPanId,
                            const sData_t msdu,int8 rssi);

//static void  Station_send_by_LF(uint8 *pdata,uint16 datalen);
//static void  Data_to_1010000_code(uint8 *p01011Code,
//                                          uint8 *pdata,uint16 datalen);
static void  Station_Send_Data_by_Wire(uint8* pData, uint16 dataLen);
static void  Start_Timer4(void);
static void  Init_LF_Exciter(void);
static void  Send_Data_by_Wireless(uint8* pdata ,
                                            uint8 devType,
                                            uint16 len,
                                            uint8 DeliverMode,
                                            uint16 uniDstAddr,
                                            uint8 txOption);
//static void  Start_buzzer(void);
//static void  Stop_buzzer(void);

void Send_Data2Station(uint8* pdata ,uint16 panid,uint16 len,uint8 DeliverMode, uint16 uniDstAddr, uint8 txOption);

static uint8 Station_ParseDeviceAckFrame(const sAddr_t* sAddr,
                                    uint16 srcPanId,
                                    const sData_t msdu,
                                    int8 rssi);
static bool checkCardShortAddr(uint16 shortAddr,uint32 timeTick,int8 rssi);
static void ReportCardRssi(void);
static void ReportStationRssi(void);
static void quicksort(void);
void receiveAck(void);
uint8 calcrc_1byte(uint8 abyte);

void Station_LF_Parameter(void);
static void CardTypeCheackAck(uint16 u16addr);
static void RssiChCheackAck(uint16 u16addr);
static void DeviceCardPeriodChannelAck(uint16 u16addr);
static void vSetStationChannel(uint8 channel);
void Simulate_card_Startup(void);

extern unsigned int  pIndex;
extern unsigned int  dIndex;
extern uint8  state ;
extern uint8   PreamblePoll;
extern uint8   PatternPoll;
//extern uint8   DataPoll;
extern uint8  sendCount;
extern uint8  PatternCount;



/**************************************************************************************************
*
* @fn          Station_Init
*
* @brief       Initialize the application
*
* @param       taskId - taskId of the task after it was added in the OSAL task queue
*
* @return      none
*
**************************************************************************************************/
void Station_Init(uint_8 taskId)
{
    /* Initialize the task id */
    Station_TaskId = taskId;

    /* initialize MAC features */
    MAC_Init();
    MAC_InitCoord();

    /* Reset the MAC */
    MAC_MlmeResetReq(TRUE);

#if(defined WATCHDOG) && (WATCHDOG==TRUE)
    StartWatchDog(DOGTIMER_INTERVAL_1S);
    osal_set_event(Station_TaskId, STATION_FEEDWATCHDOG_EVENT);
#endif

    /*init devinfo */
    Station_ReadDevInfo();

    p_app_hdr = (app_header_t *)comm_rx_buf;
    p_app_hdr->msgtype = CARD_BASE_INFO;
    p_app_hdr->protocoltype = 3;//APP_PROTOCOL_TYPE_CARD;

    charge_card_type.tof_head.protocoltype = 3;//APP_PROTOCOL_TYPE_CARD;
    charge_card_type.tof_head.msgtype = 36;    //CHANGE_PERIOD
    charge_card_type.tof_head.len = 1;
    SysUtil_vConvertEndian(&charge_card_type.tof_head.len,sizeof(charge_card_type.tof_head.len));

    ack_packet.tof_head.msgtype = HANDLE_STATION_ACK;
    ack_packet.tof_head.protocoltype = 3;
    ack_packet.tof_head.len = 1;
    ack_packet.sequnum = u8ReportIdx;

    StationRssiPacket.tof_head.protocoltype = 3;
    StationRssiPacket.tof_head.msgtype = TOF_STATION_RSSI_CHECK;
    StationRssiPacket.tof_head.len = 0;

    /* turn off leds */
    //HalLedSet(STATION_LED_BLUE, HAL_LED_MODE_OFF);
    init_uart(HAL_UART_PORT_1,STATION_APP_BAUD);

    osal_memset(&card_info,0,sizeof(rt_card_info_ts)*40);
	osal_memset(&station_info,0,sizeof(rt_station_info_ts)*10);

    /* initial macutil */
    MacUtil_t MacUtil;
    MacUtil.panID = Station_DevInfo.PanId;//0xfff7;  /* default is card nwk id */
    MacUtil.dst_endpoint = MINEAPP_ENDPOINT;
    MacUtil.src_endpoint = 0x20;//MINEAPP_ENDPOINT;
    MacUtil.profile_id = 0x0100;//MINEAPP_PROFID;
    MacUtil.cluster_id = CARD_CLUSTERID;
    MacUtil.NodeType = NODETYPE_COORDINATOR;

    MAC_UTIL_INIT(&MacUtil);

    LF_SEND_CONSEL &=~ LF_SEND_CONBIT;          //  P0.2 to general io
    LF_SEND_CONDIR |= LF_SEND_CONBIT;           //  P0.2 to output
    LF_SEND_CONPIN = 0;

    //HandheldStatus = SHORT_DISTANCE;

    HandheldStatus = POS_MODE;

    /* start up*/
    Station_start_timerEx(Station_TaskId, STATION_UART_LINK_EVENT, 10);
    Station_start_timerEx(Station_TaskId, STATION_UART_READ1_EVENT,20);
    Station_start_timerEx(Station_TaskId,STATION_POLL_CHANNEL,40);
}

/**************************************************************************************************
*
* @fn          init_uart
*
* @brief       init uart
*
* @param       UART_PORT - number of  uart port
*              baudRate -
*
* @return      void
*
**************************************************************************************************/
void init_uart(uint8  uartPort , uint8 baudRate)
{
    halUARTCfg_t uartConfig;

    uartConfig.configured           = TRUE;
    uartConfig.baudRate             = baudRate;
    uartConfig.flowControl          = FALSE;
    uartConfig.flowControlThreshold = STATION_APP_THRESH;
    uartConfig.rx.maxBufSize        = STATION_APP_RX_MAX;
    uartConfig.tx.maxBufSize        = STATION_APP_TX_MAX;
    uartConfig.idleTimeout          = CR_APP_IDLE;
    uartConfig.intEnable            = TRUE;
    uartConfig.callBackFunc         = NULL;

    /*two ports. one for backup*/
    HalUARTOpen(uartPort, &uartConfig);
}

void vSetStationChannel(uint8 channel)
{
    if(u8CurChannel != channel)
    {
        u8CurChannel = channel;
        MAC_MlmeSetReq(MAC_LOGICAL_CHANNEL, &channel);
    }
}


/**************************************************************************************************
*
* @fn          Station_ProcessEvent
*
* @brief       This routine handles events
*
* @param       taskId - ID of the application task when it registered with the OSAL
*              events - Events for this task
*
* @return      16bit - Unprocessed events
*
**************************************************************************************************/
uint16 Station_ProcessEvent(uint_8 taskId, uint16 events)
{
    uint8* pMsg;
    macCbackEvent_t* pData;

#if(defined WATCHDOG) && (WATCHDOG==TRUE)
    if (events & STATION_FEEDWATCHDOG_EVENT)
    {
        Station_start_timerEx(Station_TaskId, STATION_FEEDWATCHDOG_EVENT, 200);
        FeedWatchDog();
        return events ^ STATION_FEEDWATCHDOG_EVENT;
    }
#endif

    if (events & SYS_EVENT_MSG)
    {
        while ((pMsg = osal_msg_receive(Station_TaskId)) != NULL)
        {
            pData = (macCbackEvent_t *) pMsg;

            switch (*pMsg)
            {
                case MAC_MLME_START_CNF:
                {
                    if (pData->startCnf.hdr.status == MAC_SUCCESS)
                    {
                        /* power on */
                        Station_DevInfo.State = E_STATION_STARTED;
                        //HalLedSet(STATION_LED_BLUE, HAL_LED_MODE_ON);
                    }
                    break;
                }

                case MAC_MCPS_DATA_CNF:
                {
                    /* sometimes received  a "fake" MAC_MCPS_DATA_CNF with status of MAC_INVALID_PARAMETER */
                    /* I do not know where it comes from, but it will cause systemreset when call mac_msg_deallocate() */
                    if (pData->dataCnf.hdr.status != MAC_INVALID_PARAMETER)
                    {
                        Station_ParseDataCnf(&pData->dataCnf);
                        mac_msg_deallocate((uint8 * *) &pData->dataCnf.pDataReq);
                    }
                    break;
                }

                case MAC_MCPS_DATA_IND:
                {
                    /* do not check the indication status */
                    if(pData->dataInd.hdr.status == ZSUCCESS)
                    {
                        Station_ParseFrame(&pData->dataInd);
                    }
                    break;
                }
           }
            /* Deallocate */
            mac_msg_deallocate((uint8 * *) &pMsg);
        }

        return events ^ SYS_EVENT_MSG;
    }

    if(events & STATION_STOP_BUZZER_EVENT)
    {
        //Stop_buzzer();
        return events ^ STATION_STOP_BUZZER_EVENT;
    }

    if(events & STATION_WRITE_TIMEOUT_EVENT)
    {
       // if(mDevStatus&LF_0R_1W)
       // {
       //     mDevStatus &=~ LF_0R_1W;
       // }
        return events ^ STATION_WRITE_TIMEOUT_EVENT;
    }

    if(events & STATION_RESET_SAVEDEVID_EVENT)
    {
         saveDevID = 0;
         return events ^ STATION_RESET_SAVEDEVID_EVENT;
    }

    if (events & STATION_REPORT_VERSION_EVENT)
    {
        if(mDevStatus&LINKED)
        {
            uint16 len = 0;
            app_mDev_hdr_t mDevHdr;
            app_eDev_hdr_t eDevHdr;
            //version+' '+release
            app_mDev_fdr_t mDevFdr;

            osal_memset(WireSendBuff, 0,WIRE_DATA_MAX_LEN+10);

            osal_memcpy(WireSendBuff, WIRE_SYNC,sizeof(mDevHdr.sync));
            len += sizeof(mDevHdr.sync);

            eDevHdr.MSGType = REPORT_VERSION;
            eDevHdr.eDevID = 0;
            eDevHdr.eDevType = 0;
            eDevHdr.mDevID = Station_DevInfo.ShortAddr;
            eDevHdr.dataLen = sizeof(VERSION)-1+1+sizeof(RELEASE)-1;
            osal_memcpy(WireSendBuff+len, &eDevHdr,sizeof(app_eDev_hdr_t));
            len += sizeof(app_eDev_hdr_t);


            osal_memcpy(WireSendBuff+len, VERSION,sizeof(VERSION)-1);
            len += sizeof(VERSION)-1;

            osal_memcpy(WireSendBuff+len, " ",sizeof(" ")-1);
            len += sizeof(" ")-1;

            osal_memcpy(WireSendBuff+len, RELEASE,sizeof(RELEASE)-1);
            len += sizeof(RELEASE)-1;

            mDevFdr.padding = 0xffff;
            mDevFdr.crc = CRC16(WireSendBuff,len,0xFFFF);
            osal_memcpy(WireSendBuff+len, (uint8 *)&mDevFdr, sizeof(app_mDev_fdr_t));
            len += sizeof(app_mDev_fdr_t);
            Station_Send_Data_by_Wire(WireSendBuff,len);
        }
        Station_start_timerEx(Station_TaskId, STATION_REPORT_VERSION_EVENT, 60000);
        return events ^ STATION_REPORT_VERSION_EVENT;
    }
    if (events & STATION_START_EVENT)
    {
        Station_Startup();
		//Station_start_timerEx(Station_TaskId, STATION_LF_INSPIRE_EVENT, 1);
        return events ^ STATION_START_EVENT;
    }
    if(events & STATION_LF_INSPIRE_EVENT)
    {
        if(teExciteState == 0)
        {
            teExciteState = 1;
            Station_LF_Parameter();
            Init_LF_Exciter();
    		HalTimerStart(HAL_TIMER_3,0x5);
        }
        return events ^ STATION_LF_INSPIRE_EVENT;
    }

    if(events & STATION_UART_LINK_EVENT)
    {

        uint16 len = 0;
        app_mDev_hdr_t mDevHdr;
        app_eDev_hdr_t eDevHdr;
        app_mDev_fdr_t mDevFdr;

        linkCount++;
        if(linkCount > 10)
        {
            SystemReset();
        }
        else if(linkCount > 3)
        {
            LF_SEND_CONPIN = 0;
            mDevStatus &=~ LINKED;
            HalLedSet(STATION_LED_BLUE, HAL_LED_MODE_TOGGLE);
        }

        osal_memcpy(WireSendBuff, WIRE_SYNC,sizeof(mDevHdr.sync));
        len += sizeof(mDevHdr.sync);

        eDevHdr.MSGType = DETECT;
        eDevHdr.eDevID = 0;
        eDevHdr.eDevType = 0;
        eDevHdr.mDevID = Station_DevInfo.ShortAddr;
        eDevHdr.dataLen = 0;
        osal_memcpy(WireSendBuff+len, &eDevHdr,sizeof(app_eDev_hdr_t));
        len += sizeof(app_eDev_hdr_t);
        mDevFdr.padding = 0xffff;
        mDevFdr.crc = CRC16(WireSendBuff,len,0xFFFF);
        osal_memcpy(WireSendBuff+len, (uint8 *)&mDevFdr, sizeof(app_mDev_fdr_t));
        len += sizeof(app_mDev_fdr_t);
        Station_Send_Data_by_Wire(WireSendBuff,len);

        if(mDevStatus&LINKED)
        {
            osal_start_timerEx(taskId, STATION_UART_LINK_EVENT, 1000);
            if(!Station_DevInfo.State)
            {
                Station_start_timerEx(Station_TaskId, STATION_START_EVENT, 10);
                Station_start_timerEx(Station_TaskId, STATION_REPORT_VERSION_EVENT, 1000);
            }
        }
        else
        {
            osal_start_timerEx(taskId, STATION_UART_LINK_EVENT, 1000);
        }
        return events ^ STATION_UART_LINK_EVENT;
    }
#if 0
    if(events & STATION_UART_READ1_EVENT)
    {
        uint8 len;
        static bool fIgnore = FALSE;
        eUartRecReturn ret = GOOD_WORK;

        len = HalUARTRead(HAL_UART_PORT_1, uartReadBuf1 + uartReadLen1, uartReadBufLen1 - uartReadLen1);

        uartReadLen1 += len;

        if(uartReadBuf1[0] != 'Y' )
        {
            fIgnore = TRUE;
        }

        //continue ignoring till len == 0
        if(fIgnore)
        {
            uartReadLen1 = 0;
        }

        if(len == 0)
        {
            fIgnore = FALSE;
        }

        if(uartReadLen1 <= WIRE_DATA_MAX_LEN
            &&uartReadLen1 >= (sizeof(app_mDev_fdr_t)+sizeof(app_mDev_hdr_t)+sizeof(app_eDev_hdr_t))
            &&len == 0)
        {
            ret = Station_ParseUartCmd(uartReadBuf1, uartReadLen1);
            if(ret != LEN_SHORT)
            {
                uartReadLen1 = 0;
            }
        }
        else if(uartReadLen1 > 0 && len == 0)
        {
            uartReadLen1 = 0;
        }

        osal_start_timerEx(taskId, STATION_UART_READ1_EVENT, UART_FRAME_TIMEOUT);
        return events ^ STATION_UART_READ1_EVENT;
    }

#endif

    if(events & STATION_WAIT_TIMEOUT)
    {
        if(teStaionState == E_STATE_WRITE)
        {
            teStaionState = E_STATE_READ;

            Send_Data_by_Wireless(comm_rx_buf,0,len_temp + sizeof(app_header_t),MAC_UTIL_UNICAST,dstID,0);
        }
        return events ^ STATION_WAIT_TIMEOUT;
    }

    if(events & STATION_POLL_TIMER)
    {
        if(teExciteState)
        {
            if(HandheldStatus == POS_MODE)
            {
                teExciteState = 0;
                HalTimerStop(HAL_TIMER_2);
        		HalTimerStop(HAL_TIMER_3);
            }
        }
        return events ^ STATION_POLL_TIMER;
    }

#if 1

    if(events & STATION_UART_READ1_EVENT)
    {
		static uint16 wantlen = 0,packetlencnt = 0;
        uint8 i=0;
        uint8 len;
        uint8 data;
        app_mDev_hdr_t *pHdl = (app_mDev_hdr_t *) uartRXBuf1;
        app_eDev_hdr_t *pHdlEdv = (app_eDev_hdr_t *)(pHdl + 1);
		static uint8 RxState = UART_SYNC_Y;
        len = HalUARTRead(1,uartReadBuf1,100);
        if(len > 0)
		{
            for(i=0; i<len; i++)
            {
                data = uartReadBuf1[i];
                switch(RxState)
    			{
    			case UART_SYNC_Y:
                {
    				if(data == 'Y')
    				{
    				    RxState = UART_SYNC_I;
                        pHdl->sync[0] = data;
    				}
    				else
    				{
    				    RxState = UART_SYNC_Y;
    				}
    				break;
    			}
    			case UART_SYNC_I:
                {
    				if(data == 'I')
    				{
    				    RxState = UART_SYNC_R;
                        pHdl->sync[1] = data;
    				}
    				else
    				{
        				RxState = UART_SYNC_Y;
    				}
    				break;
    			}
    			case UART_SYNC_R:
                {
    				if(data == 'R')
    				{
    				    RxState = UART_SYNC_i;
                        pHdl->sync[2] = data;
    				}
    				else
    				{
    				    RxState = UART_SYNC_Y;
    				}
    				break;
    			}
    			case UART_SYNC_i:
                {
    				if(data == 'I')
    				{
    				    RxState = UART_MSG_T;
                        pHdl->sync[3] = data;
    				}
    				else
    				{
    				    RxState = UART_SYNC_Y;
    				}
    				break;
    			}
    			case UART_MSG_T:
                    pHdlEdv->MSGType = data;
    				RxState = UART_EDEV_T;
    				break;
    			case UART_EDEV_T:
                    pHdlEdv->eDevType = data;
    				RxState = UART_EDEV_ID_F;
    				break;
    			case UART_EDEV_ID_F:
                    pHdlEdv->eDevID = data;
    				RxState = UART_EDEV_ID_S;
    				break;
    			case UART_EDEV_ID_S:
                    pHdlEdv->eDevID = (pHdlEdv->eDevID) + (data << 8);
    				RxState = UART_MDEV_ID_F;
    				break;
    			case UART_MDEV_ID_F:
                    pHdlEdv->mDevID = data;
    				RxState = UART_MDEV_ID_S;
    				break;
    			case UART_MDEV_ID_S:
                    pHdlEdv->mDevID = (pHdlEdv->mDevID) + (data << 8);
    				RxState = UART_DATA_LEN_F;
    				break;
    			case UART_DATA_LEN_F:
                    pHdlEdv->dataLen = data;
    				RxState = UART_DATA_LEN_S;
    				break;
    			case UART_DATA_LEN_S:
                    //pHdlEdv->dataLen = (pHdlEdv->dataLen << 8) + data;
                    pHdlEdv->dataLen = (pHdlEdv->dataLen) + (data << 8);

                    if(pHdlEdv->dataLen > 100)
                        RxState = UART_SYNC_Y;
                    else
                    {
                        RxState = UART_DATA_RX;
                        wantlen = pHdlEdv->dataLen + sizeof(app_mDev_fdr_t);
                    }
    				break;
    			case UART_DATA_RX:
                    uartRXBuf1[sizeof(app_mDev_hdr_t) + sizeof(app_eDev_hdr_t)+packetlencnt]=data;
                    if(++packetlencnt >= wantlen)
    				{
                        Station_ParseUartCmd(uartRXBuf1, sizeof(app_mDev_hdr_t)+ sizeof(app_eDev_hdr_t)+ wantlen);
                        packetlencnt = 0;
    					RxState = UART_SYNC_Y;
    				}
                    break;
    			default:
    				break;
    			}
            }
		}

        osal_start_timerEx(taskId, STATION_UART_READ1_EVENT, UART_FRAME_TIMEOUT);
        return events ^ STATION_UART_READ1_EVENT;
    }
#endif

    if(events & STATION_REPORT_REMOTE_EVENT)
    {
        if(HandheldStatus == REMOTE_DISTANCE)
        {
            static uint8 cnt;
            Station_start_timerEx(Station_TaskId, STATION_REPORT_REMOTE_EVENT, 1000);
            cnt++;
			ReportCardRssi();
            if(cnt%2)
            {
				vSetStationChannel(Station_DevInfo.Channel);
            }
            else
            {
				vSetStationChannel(u8SetBroadcastChannel);
            }
        }
        return events ^ STATION_REPORT_REMOTE_EVENT;

    }

    if(events & STATION_POLL_RSSI)
    {
        Send_Data2Station((uint8*)(&StationRssiPacket),dstPanID,sizeof(app_header_t),MAC_UTIL_UNICAST,0,0);
        if(u8RssiCnt++ > 50)
        {
            u8RssiCnt = 0;
        }
        else
        {
            Station_start_timerEx(Station_TaskId,STATION_POLL_RSSI,200);
        }
        if(u8RssiCnt%3)
        {
			vSetStationChannel(Station_DevInfo.Channel);
        }
        else
        {
			vSetStationChannel(u8SetBroadcastChannel);
		}
            
        Send_Data2Station((uint8*)(&StationRssiPacket),dstPanID+(10000*(u8RssiCnt%3)),sizeof(app_header_t),MAC_UTIL_UNICAST,0,0);
        return events ^ STATION_POLL_RSSI;
    }

    if(events & STATION_POLL_CHANNEL)
    {
        /*if(DoubleChannel)
        {
            if((ChannelPoll++) % 2)
            {
                channel = 16;
            }
            else
            {
                channel = 25;
            }
            MAC_MlmeSetReq(MAC_LOGICAL_CHANNEL, &channel);
        }*/
        if(HandheldStatus == SHORT_DISTANCE)
            Station_start_timerEx(Station_TaskId, STATION_LF_INSPIRE_EVENT, 10);
        osal_start_timerEx(taskId, STATION_POLL_CHANNEL, 1000);
        return events ^ STATION_POLL_CHANNEL;
    }

	if(events & STATION_SENDDATA_EVENT)
	{
    	if(receivedWriteCmd)
    	{
        	uint8 sendLength;
        	sendLength = tof_head_write->len;
        	tof_head_write->len = ((tof_head_write->len>>8)&0x00ff)|((tof_head_write->len<<8)&0xff00);
        	Send_Data_by_Wireless((uint8*)tof_head_write,eDevType,4+sendLength,MAC_UTIL_UNICAST, dstAdr,0);
    	}

    	if(receivedCloseCmd)
    	{
    	    Send_Data_by_Wireless((uint8*)tof_head_close,eDevType,4,MAC_UTIL_UNICAST, dstAdr,0);
    	}


    	if(receivedOpenCmd)
    	{
    	Send_Data_by_Wireless((uint8*)tof_head_open,eDevType,4,MAC_UTIL_UNICAST,dstAdr,0);
    	}

    	if(receivedOpenCmd||receivedCloseCmd||receivedWriteCmd)
    	{
    	Station_start_timerEx(Station_TaskId, STATION_SENDDATA_EVENT, 500);
    	}
    	else
    	{
    	osal_stop_timerEx(Station_TaskId, STATION_SENDDATA_EVENT);
    	}
    	return events ^ STATION_SENDDATA_EVENT;
	}

	if(events & LOCATOR_POLL_RSSI)
	{
		vSetStationChannel(Station_DevInfo.Channel);

		if(station_cnt > 0)
		{
			dstLoctor = station_info[idx].u16ShortAddr + (locator_cnt%2)*10000 +10000;			
			Send_Data2Station((uint8*)(&StationRssiPacket),dstLoctor,sizeof(app_header_t),MAC_UTIL_UNICAST,0,0);
		}
		if(locator_cnt++ < 4)
			Station_start_timerEx(Station_TaskId, LOCATOR_POLL_RSSI,120);
		else
		{
			if(idx < station_cnt)
			{
				idx++;
				locator_cnt = 0;
				Station_start_timerEx(Station_TaskId, LOCATOR_POLL_RSSI,50);
			}
			else
			{
				teStaionState = old_teStaionState;
				ReportStationRssi();
				Station_Startup();
			}
						
		}
		return events ^ LOCATOR_POLL_RSSI;
	}
    return 0;
}

/**************************************************************************************************
*
* @fn          MAC_CbackEvent
*
* @brief       This callback function sends MAC events to the application.
*              The application must implement this function.  A typical
*              implementation of this function would allocate an OSAL message,
*              copy the event parameters to the message, and send the message
*              to the application's OSAL event handler.  This function may be
*              executed from task or interrupt context and therefore must
*              be reentrant.
*
* @param       pData - Pointer to parameters structure.
*
* @return      None.
*
**************************************************************************************************/
void MAC_CbackEvent(macCbackEvent_t* pData)
{
    macCbackEvent_t* pMsg = NULL;

    uint_8 len = station_cbackSizeTable[pData->hdr.event];

    switch (pData->hdr.event)
    {
    case MAC_MLME_BEACON_NOTIFY_IND:
        len += sizeof(macPanDesc_t)
               + pData->beaconNotifyInd.sduLength
               + MAC_PEND_FIELDS_LEN(pData->beaconNotifyInd.pendAddrSpec);
        if ((pMsg = (macCbackEvent_t *) osal_msg_allocate(len)) != NULL)
        {
            /* Copy data over and pass them up */
            osal_memcpy(pMsg, pData, sizeof(macMlmeBeaconNotifyInd_t));
            pMsg->beaconNotifyInd.pPanDesc = (macPanDesc_t *)
                                             ((uint_8 *) pMsg
                                              + sizeof(macMlmeBeaconNotifyInd_t));
            osal_memcpy(pMsg->beaconNotifyInd.pPanDesc,
                        pData->beaconNotifyInd.pPanDesc,
                        sizeof(macPanDesc_t));
            pMsg->beaconNotifyInd.pSdu = (uint_8 *)
                                         (pMsg->beaconNotifyInd.pPanDesc + 1);
            osal_memcpy(pMsg->beaconNotifyInd.pSdu,
                        pData->beaconNotifyInd.pSdu,
                        pData->beaconNotifyInd.sduLength);
        }
        break;

    case MAC_MCPS_DATA_IND:

        pMsg = pData;
        break;

    default:
        if ((pMsg = (macCbackEvent_t *) osal_msg_allocate(len)) != NULL)
        {
            osal_memcpy(pMsg, pData, len);
        }
        break;
    }

    if (pMsg != NULL)
    {
        if (SUCCESS != osal_msg_send(Station_TaskId, (uint8 *) pMsg))
        {
            SystemReset();
        }
    }
}

/**************************************************************************************************
*
* @fn      MAC_CbackCheckPending
*
* @brief   Returns the number of indirect messages pending in the application
*
* @param   None
*
* @return  Number of indirect messages in the application
*
**************************************************************************************************/
uint_8 MAC_CbackCheckPending(void)
{
    return (0);
}


/**************************************************************************************************
*
* @fn      Station_Startup()
*
* @brief   Update the timer per tick
*
* @param   beaconEnable: TRUE/FALSE
*
* @return  None
*
**************************************************************************************************/
void Station_Startup()
{
    macMlmeStartReq_t startReq;

    if (Station_DevInfo.State == E_STATION_STARTED)
        return;

    /* Setup MAC_EXTENDED_ADDRESS */
    MAC_MlmeSetReq(MAC_EXTENDED_ADDRESS, &Station_DevInfo.extAddr);

    /* Setup MAC_SHORT_ADDRESS */
    //Station_DevInfo.ShortAddr = 0;
    MAC_MlmeSetReq(MAC_SHORT_ADDRESS, &Station_DevInfo.ShortAddr);
	stationID = Manchester_Encoding((uint8)(EXCITE_ID));
        sendData = stationID;
	stationIDcrc = Manchester_Encoding((uint8)calcrc_1byte((uint8)(EXCITE_ID)));
    /* Setup MAC_BEACON_PAYLOAD_LENGTH */
    uint_8 tmp8 = 0;
    MAC_MlmeSetReq(MAC_BEACON_PAYLOAD_LENGTH, &tmp8);

    /* Enable RX */
    MAC_MlmeSetReq(MAC_RX_ON_WHEN_IDLE, &Station_MACTrue);

    /* Setup MAC_ASSOCIATION_PERMIT */
    MAC_MlmeSetReq(MAC_ASSOCIATION_PERMIT, &Station_MACFalse);

    /* change CCA param */
    uint8 maxFrameRetries = 4;
    MAC_MlmeSetReq(MAC_MAX_FRAME_RETRIES, &maxFrameRetries);

    uint8 maxCsmaBackoff = 5;
    MAC_MlmeSetReq(MAC_MAX_CSMA_BACKOFFS, &maxCsmaBackoff);

    uint8 minBe = 4;
    MAC_MlmeSetReq(MAC_MIN_BE, &minBe);

    uint8 maxBe = 6;
    MAC_MlmeSetReq(MAC_MAX_BE, &maxBe);

    /* Fill in the information for the start request structure */
    startReq.startTime = 0;
    startReq.panId = Station_DevInfo.PanId;
    startReq.logicalChannel = Station_DevInfo.Channel;
    startReq.beaconOrder = 0xF;
    startReq.superframeOrder = 0xF;
    startReq.panCoordinator = TRUE;
    startReq.batteryLifeExt = FALSE;
    startReq.coordRealignment = FALSE;
    startReq.realignSec.securityLevel = FALSE;
    startReq.beaconSec.securityLevel = FALSE;

    /* Call start request to start the device as a coordinator */
    MAC_MlmeStartReq(&startReq);
}


void Simulate_card_Startup()
{
    macMlmeStartReq_t startReq;

    if (Station_DevInfo.State == E_STATION_STARTED)
        return;

    /* Setup MAC_EXTENDED_ADDRESS */
    MAC_MlmeSetReq(MAC_EXTENDED_ADDRESS, &Station_DevInfo.extAddr);

    /* Setup MAC_SHORT_ADDRESS */
    //Station_DevInfo.ShortAddr = 0;
	uint_16 u16ShortAddr = 0x2345;
	
    MAC_MlmeSetReq(MAC_SHORT_ADDRESS, &u16ShortAddr);
	stationID = Manchester_Encoding((uint8)(EXCITE_ID));
        sendData = stationID;
	stationIDcrc = Manchester_Encoding((uint8)calcrc_1byte((uint8)(EXCITE_ID)));
    /* Setup MAC_BEACON_PAYLOAD_LENGTH */
    uint_8 tmp8 = 0;
    MAC_MlmeSetReq(MAC_BEACON_PAYLOAD_LENGTH, &tmp8);

    /* Enable RX */
    MAC_MlmeSetReq(MAC_RX_ON_WHEN_IDLE, &Station_MACTrue);

    /* Setup MAC_ASSOCIATION_PERMIT */
    MAC_MlmeSetReq(MAC_ASSOCIATION_PERMIT, &Station_MACFalse);

    /* change CCA param */
    uint8 maxFrameRetries = 4;
    MAC_MlmeSetReq(MAC_MAX_FRAME_RETRIES, &maxFrameRetries);

    uint8 maxCsmaBackoff = 5;
    MAC_MlmeSetReq(MAC_MAX_CSMA_BACKOFFS, &maxCsmaBackoff);

    uint8 minBe = 4;
    MAC_MlmeSetReq(MAC_MIN_BE, &minBe);

    uint8 maxBe = 6;
    MAC_MlmeSetReq(MAC_MAX_BE, &maxBe);

    /* Fill in the information for the start request structure */
    startReq.startTime = 0;
    startReq.panId = 0xFFF0;//Station_DevInfo.PanId;
    startReq.logicalChannel = 25;//Station_DevInfo.Channel;
    startReq.beaconOrder = 0xF;
    startReq.superframeOrder = 0xF;
    startReq.panCoordinator = TRUE;
    startReq.batteryLifeExt = FALSE;
    startReq.coordRealignment = FALSE;
    startReq.realignSec.securityLevel = FALSE;
    startReq.beaconSec.securityLevel = FALSE;

    /* Call start request to start the device as a coordinator */
    MAC_MlmeStartReq(&startReq);
}


void Station_ReadDevInfo(void)
{
    uint16 number, number0, number1, number2, number3, number4 ;
    HalFlashRead(HAL_FLASH_IEEE_PAGE, HAL_FLASH_IEEE_OSET,
                 (uint8 *)&Station_DevInfo.extAddr, HAL_FLASH_IEEE_SIZE);

    /* DeviceID */
    number0=Station_DevInfo.extAddr[LPBSS_MAC_DEVID_L8BIT]&0x0F;
    number1=(Station_DevInfo.extAddr[LPBSS_MAC_DEVID_L8BIT]>>4)&0x0F;
    number2=Station_DevInfo.extAddr[LPBSS_MAC_DEVID_M8BIT]&0x0F;
    number3=(Station_DevInfo.extAddr[LPBSS_MAC_DEVID_M8BIT]>>4)&0x0F;
    number4=Station_DevInfo.extAddr[LPBSS_MAC_DEVID_H4BIT]&0x0F;

    number = number0 + 10*number1 + 100*number2 + 1000*number3 + 10000*number4;

    if((number0>9)||(number1>9)||(number2>9)||(number3>9)||(number4>6)
            ||!LPBSS_IS_DEVID(number))
    {
        HAL_ASSERT(false);
    }

    Station_DevInfo.PanId = number;//POS_STATION_PANID;

    /* Channel */
    uint8 Channel = Station_DevInfo.extAddr[LPBSS_MAC_CHA];

    if(!(Channel>=11 && Channel <=26))
    {
        Channel = 0x19;   //use default channel 11;
    }

    Station_DevInfo.Channel =  Channel;
	

    BSP_FLASH_Read(HAL_FLASH_INFOMATION_PAGE, 0,(uint8 *)&Channel, 1);

    if((Channel>=11 && Channel <=26))
    {
        Station_DevInfo.Channel =  Channel;
    }

	u8CurChannel = Station_DevInfo.Channel;

    /*For Card, the EXT_LPBSS_MACADDR_TYPE Byte of Exit Addr should be 0x11 */
    //HAL_ASSERT(Station_DevInfo.extAddr[LPBSS_MAC_MODEL] == EXT_LPBSS_MAC_TYPE_CARDREADER);
    Station_DevInfo.ShortAddr = 0x0000;
    Station_DevInfo.State = E_STATION_INIT;
    Station_DevInfo.retreat_flag = STATION_RETREAT_NORMAL;
    Station_DevInfo.retreat_cnt = 0;
}

static void Station_ParseDataCnf(const macMcpsDataCnf_t* pdatacnf)
{
    static uint8 fail_cnt = 0;

    if (MAC_SUCCESS == pdatacnf->hdr.status
        || MAC_NO_ACK == pdatacnf->hdr.status)
    {
        fail_cnt = 0;
    }
    else
    {
        /* if send data failed for 100 times, find reason and restart */
        if (fail_cnt++ > 100)
        {
            /* set restart reason and restart */
            SystemReset();
        }
    }
}

static void Station_ParseFrame(const macMcpsDataInd_t* pdataInd)
{

    RfTofWrapper_tu* psAppPkt = (RfTofWrapper_tu*)(pdataInd->msdu.p);

    //if(pdataInd->msdu.len != sizeof(rf_tof_card_data_ts) && (psAppPkt->tof_head.msgtype!=CARD_REMARK) && (psAppPkt->tof_head.msgtype!=0x20)&& (psAppPkt->tof_head.msgtype!=0x07)&& (psAppPkt->tof_head.msgtype!=CARD_TYPE_CHEACK_ACK)&& (psAppPkt->tof_head.msgtype!=TOF_STATION_RSSI_CHECK_ACK)&&(psAppPkt->tof_head.msgtype!=HAND_STATION_RSSI_CHANNEL_ACK))
    {
        //return;
    }

    switch(psAppPkt->tof_head.msgtype)
    {
    case 0x04:   //TOF_CARD_CHECKIN:
    {
        RfTofWrapper_tu RfTofData;
        if(HandheldStatus == POS_CHECKIN && DevicePosState == DEVICE_POS_READING)
        {
            Device_ParseCardFrame(&pdataInd->mac.srcAddr,pdataInd->mac.srcPanId,
               pdataInd->msdu,pdataInd->mac.rssi);
            
            RfTofData.tof_head.protocoltype = 3;//APP_PROTOCOL_TYPE_CARD;
            RfTofData.tof_head.msgtype = TOF_STATION_CHECKIN_ACK;    //TOF_STATION_CHECKIN_ACK
            RfTofData.tof_head.len = 0;
            Send_Data_by_Wireless((uint8*)&RfTofData,0,RfTofData.tof_head.len+sizeof(app_header_t),0,dstAdr,0);
            
            DevicePosState = DEVICE_POS_IDLE;
        }
        else if(HandheldStatus == POS_CHECKIN && DevicePosState == DEVICE_POS_SETTING)
        {
            if((dstDevAdr == pdataInd->mac.srcAddr.addr.shortAddr) && (peroid != 0 || RssiChannel != 0))
            {
                uint16 len;
                
                RfTofData.tof_head.protocoltype = 3;//APP_PROTOCOL_TYPE_CARD;
                RfTofData.tof_head.msgtype = 46;//DEVICE_CARD_PERIOD_CHANNEL;  
                RfTofData.tof_head.len = 0;
                if(RssiChannel != 0)
                {
                     RfTofData.rf_card_info2.info_data.frameSum = 0;
                     RfTofData.rf_card_info2.info_data.frameSeq = RssiChannel;
                     RfTofData.tof_head.len += 2;
                }
                if(peroid != 0)
                {
                    if(RfTofData.tof_head.len != 0)
                    {
                        RfTofData.rf_card_info2.info_data.datatype = 1;
                        RfTofData.rf_card_info2.info_data.len = peroid;
                    }
                    else
                    {
                        RfTofData.rf_card_info2.info_data.frameSum = 1;
                        RfTofData.rf_card_info2.info_data.frameSeq = peroid;
                    }
                    RfTofData.tof_head.len += 2;
                }

                len = RfTofData.tof_head.len;

                SysUtil_vConvertEndian(&RfTofData.tof_head.len,sizeof(RfTofData.tof_head.len));
               
                Send_Data_by_Wireless((uint8*)&RfTofData,0,len+sizeof(app_header_t),0,dstDevAdr,0);

                peroid = RssiChannel = 0;
            }
        }
        else if(HandheldStatus == POS_CHECKIN && DevicePosState == DEVICE_POS_ERP)
        {
            if(dstDevAdr == pdataInd->mac.srcAddr.addr.shortAddr && rt_erpid_info.u8ErpIdLen != 0)
            {
                RfTofData.tof_head.protocoltype = 3;//APP_PROTOCOL_TYPE_CARD;
                RfTofData.tof_head.msgtype = 48;//DEVICE_CARD_ERPID;  
                RfTofData.tof_head.len = rt_erpid_info.u8ErpIdLen;
                osal_memcpy(RfTofData.rf_tof_erp_id_data.u8Erpid,rt_erpid_info.u8ErpBuf,rt_erpid_info.u8ErpIdLen);
                SysUtil_vConvertEndian(&RfTofData.tof_head.len,sizeof(RfTofData.tof_head.len));
                Send_Data_by_Wireless((uint8*)&RfTofData,0,rt_erpid_info.u8ErpIdLen+sizeof(app_header_t),0,dstDevAdr,0);
                rt_erpid_info.u8ErpIdLen = 0;
            }
        }
        else if(HandheldStatus == SHORT_DISTANCE)
            Short_ParseCardFrame(&pdataInd->mac.srcAddr,pdataInd->mac.srcPanId,
               pdataInd->msdu,pdataInd->mac.rssi);
        break;
    }

    case 0x05:    //TOF_CARD_RSSI
    {
        if(HandheldStatus == REMOTE_DISTANCE)
        {
            if(pdataInd->mac.rssi > -80)
            {
                uint32 timeTick = osal_getClock();
                if(checkCardShortAddr(pdataInd->mac.srcAddr.addr.shortAddr,timeTick,pdataInd->mac.rssi) == false)
                {
                    card_info[buf_head].u16ShortAddr = pdataInd->mac.srcAddr.addr.shortAddr;
                    card_info[buf_head].i8CardRssi = pdataInd->mac.rssi;
                    card_info[buf_head].u32TimeTick = timeTick;
                    if(buf_head >= 40)
                    {
                        buf_head = 0;
                    }
                    else
                    {
                        buf_head++;
                    }
                }
            }
        }
        break;
    }
	case 0x07:    //TOF_CARD_ALARM
	case CARD_REMARK:
    {
        if((HandheldStatus == POS_MODE) && (psAppPkt->rf_tof_card_data.u8Reserved == EXCITE_ID))
        {
            if(psAppPkt->rf_tof_card_data.u8CardStatus&0x80)
        	{
                if(teStaionState == E_STATE_READ)
                {
                    psAppPkt->rf_tof_card_data.u16Battery = ((psAppPkt->rf_tof_card_data.u16Battery>>8&0xff)+(psAppPkt->rf_tof_card_data.u16Battery<<8&0xff00))/10+0x80;
                    Station_ParseCardFrame(&pdataInd->mac.srcAddr,pdataInd->mac.srcPanId,
                     pdataInd->msdu,pdataInd->mac.rssi);
                    ack_packet.sequnum = u8ReportIdx;
                    Send_Data_by_Wireless((uint8*)(&ack_packet),0,5,MAC_UTIL_UNICAST,pdataInd->mac.srcAddr.addr.shortAddr,0);
                }
                else if((teStaionState == E_STATE_WRITE) && (pdataInd->mac.srcAddr.addr.shortAddr == dstID))
                {
                    teStaionState = E_STATE_READ;

                    Send_Data_by_Wireless(comm_rx_buf,0,len_temp + sizeof(app_header_t),MAC_UTIL_UNICAST,pdataInd->mac.srcAddr.addr.shortAddr,0);
                    osal_stop_timerEx(Station_TaskId,STATION_WAIT_TIMEOUT);
                }
                else if(teStaionState == E_STATE_CHANGE)
                {
                    teStaionState = E_STATE_READ;
                    rf_tof_card_period_ts rf_card_period_data;
                    rf_card_period_data.tof_head.protocoltype = 3;//APP_PROTOCOL_TYPE_CARD;
                    rf_card_period_data.tof_head.msgtype = CHANGE_PERIOD;    //TOF_STATION_CHECKIN_ACK
                    rf_card_period_data.tof_head.len = 1;
                    SysUtil_vConvertEndian(&rf_card_period_data.tof_head.len ,sizeof(rf_card_period_data.tof_head.len));
                    rf_card_period_data.u8period = peroid;
                    peroid = 0;
                    Send_Data_by_Wireless((uint8*)&rf_card_period_data,0,sizeof(rf_tof_card_period_ts),0,dstAdr,0);
                    osal_stop_timerEx(Station_TaskId,STATION_WAIT_TIMEOUT);
                }
                else if(teStaionState == E_STATE_RSSI_CHANNEL)
                {
                    teStaionState = E_STATE_READ;
                    charge_card_type.cardtype = RssiChannel;
                    RssiChannel = 0;
                    charge_card_type.tof_head.msgtype = 42;      //修改卡频道
                    //charge_card_type.cardtype = pDevHdr->eDevType;
                    if((charge_card_type.cardtype < 26) && (charge_card_type.cardtype > 10))
                    {
                        Send_Data_by_Wireless((uint8*)(&charge_card_type),0,sizeof(CHARGE_CARD_TYPE_T),MAC_UTIL_UNICAST,dstID,0);
                    }
                    osal_stop_timerEx(Station_TaskId,STATION_WAIT_TIMEOUT);
                }


        	}
        }
        if(teExciteState)
        {
            if(HandheldStatus == POS_MODE)
            {
                teExciteState = 0;
                HalTimerStop(HAL_TIMER_2);
        		HalTimerStop(HAL_TIMER_3);
            }
            osal_stop_timerEx(Station_TaskId,STATION_POLL_TIMER);
        }

        if(HandheldStatus == SHORT_DISTANCE)
        {
            if(psAppPkt->rf_tof_card_data.u8Reserved == EXCITE_ID)
            {
                if(psAppPkt->rf_tof_card_data.u8CardStatus&0x80)
                {
                   psAppPkt->rf_tof_card_data.u16Battery = ((psAppPkt->rf_tof_card_data.u16Battery>>8&0xff)+(psAppPkt->rf_tof_card_data.u16Battery<<8&0xff00))/10+0x80;
                   Short_ParseCardFrame(&pdataInd->mac.srcAddr,pdataInd->mac.srcPanId,
                                           pdataInd->msdu,pdataInd->mac.rssi);
                }
            }
        }
        break;
    }

	case TOF_STATION_AVAILABLE:
	{
		if(teStaionState == E_TEST_STATION_RSSI)
		{
			uint8 i;
			for(i=0;i<station_cnt;i++)
			{
				if(station_info[i].u16ShortAddr == pdataInd->mac.srcPanId)
				{
					station_info[i].i8CardRssi[0] = pdataInd->mac.rssi;
					break;
				}
			}
			if(i == station_cnt)
			{
				station_info[i].u16ShortAddr = pdataInd->mac.srcPanId;
				station_info[i].i8CardRssi[0] = pdataInd->mac.rssi;
				if(station_cnt < 10)
					station_cnt++;
			}
		}
		break;
	}

    case HAND_STATION_WRITE_ACK:
    {
        Station_ParseCardFrame(&pdataInd->mac.srcAddr,pdataInd->mac.srcPanId,
                     pdataInd->msdu,pdataInd->mac.rssi);
        break;
    }

    case CARD_LOC_INFO:
    {
        SysUtil_vConvertEndian(&psAppPkt->rf_card_distance.u16LocDistance,sizeof(psAppPkt->rf_card_distance.u16LocDistance));
        SysUtil_vConvertEndian(&psAppPkt->rf_card_distance.u16StationAddr,sizeof(psAppPkt->rf_card_distance.u16StationAddr));
        Station_ParseCardFrame(&pdataInd->mac.srcAddr,pdataInd->mac.srcPanId,
                     pdataInd->msdu,pdataInd->mac.rssi);
        break;
    }

    /*case CARD_REMARK:
    {
        psAppPkt->rf_tof_card_data.u16Battery = ((psAppPkt->rf_tof_card_data.u16Battery>>8&0xff)+(psAppPkt->rf_tof_card_data.u16Battery<<8&0xff00))/10+0x80;
                    Station_ParseCardFrame(&pdataInd->mac.srcAddr,pdataInd->mac.srcPanId,
                     pdataInd->msdu,pdataInd->mac.rssi);
        break;
    }*/

	case 0x1f:    //TOF_CARD_EXCITE
    {
        if(HandheldStatus == SHORT_DISTANCE)
        {
        	Station_ParseCardDeviceFrame(&pdataInd->mac.srcAddr,pdataInd->mac.srcPanId,pdataInd->msdu,pdataInd->mac.rssi);
        }
        break;

    }

	case 0x20:    //激励ack
    {
        if(HandheldStatus == SHORT_DISTANCE)
        {
            Station_ParseDeviceAckFrame(&pdataInd->mac.srcAddr,pdataInd->mac.srcPanId,
                     pdataInd->msdu,pdataInd->mac.rssi);
        }
        break;
    }

    case CARD_TYPE_CHEACK_ACK:
    {
        CardTypeCheackAck(pdataInd->mac.srcAddr.addr.shortAddr);
        break;
    }

    case HAND_STATION_RSSI_CHANNEL_ACK:
    {
        RssiChCheackAck(pdataInd->mac.srcAddr.addr.shortAddr);
        break;
    }

    case TOF_STATION_RSSI_CHECK_ACK:
    {
		if(teStaionState == E_TEST_STATION_RSSI)
		{
			station_info[idx].i8CardRssi[psAppPkt->rf_tof_station_rssi.u8StationPort] = pdataInd->mac.rssi;						
		}
			//Station_ParseRssiFrame(&pdataInd->mac.srcAddr,pdataInd->mac.srcPanId,pdataInd->msdu,pdataInd->mac.rssi);
        break;
    }

    case DEVICE_CARD_PERIOD_CHANNEL_ACK:
    {
        if(dstID == pdataInd->mac.srcAddr.addr.shortAddr)
        {
            DeviceCardPeriodChannelAck(pdataInd->mac.srcAddr.addr.shortAddr);
        }
    }

    default:
        break;
    }
}

static void SysUtil_vConvertEndian(void* pvData, uint8 u8Len)
{
    int i;
    uint8* pu8Data = (uint8*) pvData;
    for(i=0;i<u8Len/2;i++)
    {
        uint8 tmp;
        tmp= pu8Data[i];
        pu8Data[i] = pu8Data[u8Len-i-1];
        pu8Data[u8Len-i-1] = tmp;
    }
}

static void CardTypeCheackAck(uint16 u16addr)
{
    app_eDev_hdr_t eDevHdr;
    app_mDev_fdr_t mDevFdr;

    eDevHdr.MSGType = CHARGE_CARD_TYPE_ACK;
    eDevHdr.eDevID = u16addr;
    eDevHdr.eDevType = 0;
    eDevHdr.mDevID = 0xffff;
    eDevHdr.dataLen = 0;
    osal_memcpy(WireSendBuff,WIRE_SYNC,sizeof(app_mDev_hdr_t));
    osal_memcpy(WireSendBuff+sizeof(app_mDev_hdr_t),(uint8 *)(&eDevHdr),sizeof(app_eDev_hdr_t));
    mDevFdr.padding = 0xffff;
    //mDevFdr.crc = CRC16(WireSendBuff,(sizeof(app_mDev_hdr_t)+msdu.len),0xFFFF);
    mDevFdr.crc = CRC16(WireSendBuff,(sizeof(app_mDev_hdr_t)+sizeof(app_eDev_hdr_t)),0xFFFF);
    osal_memcpy(WireSendBuff+sizeof(app_mDev_hdr_t)+sizeof(app_eDev_hdr_t),(uint8 *)&mDevFdr,sizeof(app_mDev_fdr_t));
    HalUARTWrite(HAL_UART_PORT_1,(uint8*)(&WireSendBuff),sizeof(app_mDev_hdr_t)+sizeof(app_eDev_hdr_t)+sizeof(app_mDev_fdr_t));
}

static void RssiChCheackAck(uint16 u16addr)
{
    app_eDev_hdr_t eDevHdr;
    app_mDev_fdr_t mDevFdr;

    eDevHdr.MSGType = SET_RSSI_CHANNNEL_ACK;
    eDevHdr.eDevID = u16addr;
    eDevHdr.eDevType = 0;
    eDevHdr.mDevID = 0xffff;
    eDevHdr.dataLen = 0;
    osal_memcpy(WireSendBuff,WIRE_SYNC,sizeof(app_mDev_hdr_t));
    osal_memcpy(WireSendBuff+sizeof(app_mDev_hdr_t),(uint8 *)(&eDevHdr),sizeof(app_eDev_hdr_t));
    mDevFdr.padding = 0xffff;
    //mDevFdr.crc = CRC16(WireSendBuff,(sizeof(app_mDev_hdr_t)+msdu.len),0xFFFF);
    mDevFdr.crc = CRC16(WireSendBuff,(sizeof(app_mDev_hdr_t)+sizeof(app_eDev_hdr_t)),0xFFFF);
    osal_memcpy(WireSendBuff+sizeof(app_mDev_hdr_t)+sizeof(app_eDev_hdr_t),(uint8 *)&mDevFdr,sizeof(app_mDev_fdr_t));
    HalUARTWrite(HAL_UART_PORT_1,(uint8*)(&WireSendBuff),sizeof(app_mDev_hdr_t)+sizeof(app_eDev_hdr_t)+sizeof(app_mDev_fdr_t));
}

static void DeviceCardPeriodChannelAck(uint16 u16addr)
{
    app_eDev_hdr_t eDevHdr;
    app_mDev_fdr_t mDevFdr;

    eDevHdr.MSGType = DEVICE_CARD_SETTING_ACK;
    eDevHdr.eDevID = u16addr;
    eDevHdr.eDevType = 0;
    eDevHdr.mDevID = 0xffff;
    eDevHdr.dataLen = 0;
    osal_memcpy(WireSendBuff,WIRE_SYNC,sizeof(app_mDev_hdr_t));
    osal_memcpy(WireSendBuff+sizeof(app_mDev_hdr_t),(uint8 *)(&eDevHdr),sizeof(app_eDev_hdr_t));
    mDevFdr.padding = 0xffff;
    //mDevFdr.crc = CRC16(WireSendBuff,(sizeof(app_mDev_hdr_t)+msdu.len),0xFFFF);
    mDevFdr.crc = CRC16(WireSendBuff,(sizeof(app_mDev_hdr_t)+sizeof(app_eDev_hdr_t)),0xFFFF);
    osal_memcpy(WireSendBuff+sizeof(app_mDev_hdr_t)+sizeof(app_eDev_hdr_t),(uint8 *)&mDevFdr,sizeof(app_mDev_fdr_t));
    HalUARTWrite(HAL_UART_PORT_1,(uint8*)(&WireSendBuff),sizeof(app_mDev_hdr_t)+sizeof(app_eDev_hdr_t)+sizeof(app_mDev_fdr_t));
}

static uint8 Device_ParseCardFrame(const sAddr_t* sAddr,uint16 srcPanId,const sData_t msdu,int8 rssi)
{
    uint8 ret;

    if(sAddr->addr.shortAddr == 0)
    {
        return 0;
    }
	dstAdr = sAddr->addr.shortAddr;

    app_eDev_hdr_t eDevHdr;
    app_report_hdr_t eReportHdr;
    app_mDev_fdr_t mDevFdr;
    uint16 curDevID = 0;

	RfTofWrapper_tu* psAppPkt = (RfTofWrapper_tu*)(msdu.p);

    eDevHdr.MSGType = DEVICE_CARD_READ_ACK;
    eDevHdr.eDevID = sAddr->addr.shortAddr;
    eDevHdr.eDevType = 0;
	eDevHdr.mDevID = 0;
    //eDevHdr.dataLen = sizeof(app_report_hdr_t);
    curDevID = eDevHdr.eDevID;
	RfTofWrapper_tu RfTofData;

    SysUtil_vConvertEndian(&psAppPkt->rf_tof_card_data.u16Battery,sizeof(psAppPkt->rf_tof_card_data.u16Battery));
	eReportHdr.battery = (psAppPkt->rf_tof_card_data.u16Battery & 0xFFE0)>>5;
    eReportHdr.period = psAppPkt->rf_tof_card_data.u8Reserved;
    eReportHdr.rssi = rssi;
    eReportHdr.rssi_channel = (psAppPkt->rf_tof_card_data.u16Battery & 0x1F);
    eReportHdr.ver_oad = psAppPkt->rf_tof_card_data.u16OadVersion;

    SysUtil_vConvertEndian(&psAppPkt->rf_tof_card_base_data.tof_head.len,sizeof(psAppPkt->rf_tof_card_base_data.tof_head.len));

    eDevHdr.dataLen = psAppPkt->rf_tof_card_base_data.tof_head.len -(sizeof(rf_tof_card_data_ts) - sizeof(app_header_t));

    if(eDevHdr.dataLen > 0 && eDevHdr.dataLen < 20)
    {
        osal_memcpy(eReportHdr.erp_id,psAppPkt->rf_tof_card_base_data.u8Erpid,eDevHdr.dataLen);
    }
    else
    {
        eDevHdr.dataLen = 0;
    }

    eDevHdr.dataLen += 7;

    SysUtil_vConvertEndian(&eReportHdr.ver_oad,sizeof(eReportHdr.ver_oad));

    //if(curDevID != saveDevID)
    {
        saveDevID = curDevID;
        osal_memcpy(WireSendBuff,WIRE_SYNC,sizeof(app_mDev_hdr_t));
        osal_memcpy(WireSendBuff+sizeof(app_mDev_hdr_t),(uint8 *)(&eDevHdr),sizeof(app_eDev_hdr_t));
        osal_memcpy(WireSendBuff+sizeof(app_mDev_hdr_t)+sizeof(app_eDev_hdr_t),(uint8 *)(&eReportHdr),eDevHdr.dataLen);

        mDevFdr.padding = 0xffff;
        mDevFdr.crc = CRC16(WireSendBuff,(sizeof(app_mDev_hdr_t)+sizeof(app_eDev_hdr_t)+eDevHdr.dataLen),0xFFFF);
        osal_memcpy(WireSendBuff+sizeof(app_mDev_hdr_t)+sizeof(app_eDev_hdr_t)+eDevHdr.dataLen,(uint8 *)&mDevFdr,sizeof(app_mDev_fdr_t));
        ret= HalUARTWrite(HAL_UART_PORT_1,(uint8*)(&WireSendBuff),sizeof(app_mDev_hdr_t)+sizeof(app_eDev_hdr_t)+ eDevHdr.dataLen+sizeof(app_mDev_fdr_t));

        RfTofData.tof_head.protocoltype = 3;//APP_PROTOCOL_TYPE_CARD;
        RfTofData.tof_head.msgtype = 0x10;    //TOF_STATION_CHECKIN_ACK
        RfTofData.tof_head.len = 0;
        Send_Data_by_Wireless((uint8*)&RfTofData,0,RfTofData.tof_head.len+sizeof(app_header_t),0,dstAdr,0);
        return ret;
    }
    /*
    else
    {
        saveDevID = 0;
        return 0;
    }*/
}




static uint8 Short_ParseCardFrame(const sAddr_t* sAddr,uint16 srcPanId,const sData_t msdu,int8 rssi)
{
    uint8 ret;

    if(sAddr->addr.shortAddr == 0)
    {
        return 0;
    }
	dstAdr = sAddr->addr.shortAddr;

    app_eDev_hdr_t eDevHdr;
    app_mDev_fdr_t mDevFdr;
    uint16 curDevID = 0;

	RfTofWrapper_tu* psAppPkt = (RfTofWrapper_tu*)(msdu.p);
    //eDevHdr = *((app_eDev_hdr_t*)msdu.p);
    //curDevID = eDevHdr.eDevID;

    eDevHdr.MSGType = READ_DATA;
    eDevHdr.eDevID = sAddr->addr.shortAddr;
    eDevHdr.eDevType = 0;
    //eDevHdr.mDevID = 0xffff;
	eDevHdr.mDevID = rssi;
    eDevHdr.dataLen = 0;
    curDevID = eDevHdr.eDevID;
	RfTofWrapper_tu RfTofData;

    if((psAppPkt->rf_tof_card_data.u16Battery)&0x80)
	eDevHdr.eDevType = psAppPkt->rf_tof_card_data.u16Battery;


    if(curDevID != saveDevID)
    {
        //Start_buzzer();
        saveDevID = curDevID;
        //Station_start_timerEx(Station_TaskId, STATION_STOP_BUZZER_EVENT, 150);
        //Station_start_timerEx(Station_TaskId, STATION_RESET_SAVEDEVID_EVENT, 1000);

        osal_memcpy(WireSendBuff,WIRE_SYNC,sizeof(app_mDev_hdr_t));
        osal_memcpy(WireSendBuff+sizeof(app_mDev_hdr_t),(uint8 *)(&eDevHdr),sizeof(app_eDev_hdr_t));

        mDevFdr.padding = 0xffff;
        //mDevFdr.crc = CRC16(WireSendBuff,(sizeof(app_mDev_hdr_t)+msdu.len),0xFFFF);
        mDevFdr.crc = CRC16(WireSendBuff,(sizeof(app_mDev_hdr_t)+sizeof(app_eDev_hdr_t)),0xFFFF);
        osal_memcpy(WireSendBuff+sizeof(app_mDev_hdr_t)+sizeof(app_eDev_hdr_t),(uint8 *)&mDevFdr,sizeof(app_mDev_fdr_t));
        ret= HalUARTWrite(HAL_UART_PORT_1,(uint8*)(&WireSendBuff),sizeof(app_mDev_hdr_t)+sizeof(app_eDev_hdr_t)+sizeof(app_mDev_fdr_t));
        //Stop_buzzer();

        RfTofData.tof_head.protocoltype = 3;//APP_PROTOCOL_TYPE_CARD;
        RfTofData.tof_head.msgtype = 0x10;    //TOF_STATION_CHECKIN_ACK
        RfTofData.tof_head.len = 0;
        Send_Data_by_Wireless((uint8*)&RfTofData,0,RfTofData.tof_head.len+sizeof(app_header_t),0,dstAdr,0);
        return ret;
    }
    else
    {
        saveDevID = 0;
        return 0;
    }
}

static uint8 Station_ParseCardFrame(const sAddr_t* sAddr,uint16 srcPanId,
                                    const sData_t msdu,int8 rssi)
{
    uint8 ret = 0;

    if(sAddr->addr.shortAddr == 0)
    {
        return 0;
    }
	dstAdr = sAddr->addr.shortAddr;

    app_eDev_hdr_t eDevHdr;
    app_mDev_fdr_t mDevFdr;
    uint16 curDevID = 0;

	RfTofWrapper_tu* psAppPkt = (RfTofWrapper_tu*)(msdu.p);
    //eDevHdr = *((app_eDev_hdr_t*)msdu.p);
    //curDevID = eDevHdr.eDevID;

    eDevHdr.MSGType = READ_DATA;
    eDevHdr.eDevID = sAddr->addr.shortAddr;
    eDevHdr.eDevType = 0;
    eDevHdr.mDevID = 0xffff;
    if(psAppPkt->rf_card_info1.info_data.len > 0)
        eDevHdr.dataLen = psAppPkt->rf_card_info1.info_data.len + sizeof(app_eDev_Data_t);
    else
        eDevHdr.dataLen = 0;
    curDevID = eDevHdr.eDevID;

	//RfTofWrapper_tu RfTofData;

    switch(psAppPkt->tof_head.msgtype)
    {
        case HAND_STATION_WRITE_ACK:   //TOF_CARD_CHECKIN:
        {
            eDevHdr.MSGType = WRITE_DATA_ACK;
            eDevHdr.dataLen = 0;
            osal_memcpy(WireSendBuff,WIRE_SYNC,sizeof(app_mDev_hdr_t));
            osal_memcpy(WireSendBuff+sizeof(app_mDev_hdr_t),(uint8 *)(&eDevHdr),sizeof(app_eDev_hdr_t));
            mDevFdr.padding = 0xffff;
            mDevFdr.crc = CRC16(WireSendBuff,(sizeof(app_mDev_hdr_t)+sizeof(app_eDev_hdr_t)+eDevHdr.dataLen),0xFFFF);
            osal_memcpy(WireSendBuff+sizeof(app_mDev_hdr_t)+sizeof(app_eDev_hdr_t)+eDevHdr.dataLen,(uint8 *)&mDevFdr,sizeof(app_mDev_fdr_t));
            ret= HalUARTWrite(HAL_UART_PORT_1,(uint8*)(&WireSendBuff),sizeof(app_mDev_hdr_t)+sizeof(app_eDev_hdr_t)+sizeof(app_mDev_fdr_t)+eDevHdr.dataLen);
            break;
        }

        case CARD_LOC_INFO:
        {
            eDevHdr.MSGType = CARD_LOC_DISTANCE;
            eDevHdr.eDevID = psAppPkt->rf_card_distance.u16LocDistance;
            eDevHdr.mDevID = psAppPkt->rf_card_distance.u16StationAddr;
            eDevHdr.dataLen = 0;
            osal_memcpy(WireSendBuff,WIRE_SYNC,sizeof(app_mDev_hdr_t));
            osal_memcpy(WireSendBuff+sizeof(app_mDev_hdr_t),(uint8 *)(&eDevHdr),sizeof(app_eDev_hdr_t));
            osal_memcpy(WireSendBuff+sizeof(app_mDev_hdr_t)+sizeof(app_eDev_hdr_t),(uint8 *)(&psAppPkt->rf_card_info1.info_data),eDevHdr.dataLen);
            mDevFdr.padding = 0xffff;
            mDevFdr.crc = CRC16(WireSendBuff,(sizeof(app_mDev_hdr_t)+sizeof(app_eDev_hdr_t)+eDevHdr.dataLen),0xFFFF);
            osal_memcpy(WireSendBuff+sizeof(app_mDev_hdr_t)+sizeof(app_eDev_hdr_t)+eDevHdr.dataLen,(uint8 *)&mDevFdr,sizeof(app_mDev_fdr_t));
            ret= HalUARTWrite(HAL_UART_PORT_1,(uint8*)(&WireSendBuff),sizeof(app_mDev_hdr_t)+sizeof(app_eDev_hdr_t)+sizeof(app_mDev_fdr_t)+eDevHdr.dataLen);
            break;
        }

        default:
        {
            if((psAppPkt->rf_tof_card_data.u16Battery)&0x80)
            {
        	    eDevHdr.eDevType = psAppPkt->rf_tof_card_data.u16Battery - 0x80;
                eDevHdr.mDevID = psAppPkt->rf_tof_card_data.u16SeqNum;      //卡的工作周期放在seqnum字段里
                SysUtil_vConvertEndian(&eDevHdr.mDevID,sizeof(eDevHdr.mDevID));   //卡的工作周期大小端转换
            }

            if((curDevID != saveDevID) || (HandheldStatus == POS_MODE))
            {
                //Start_buzzer();
                saveDevID = curDevID;
                //Station_start_timerEx(Station_TaskId, STATION_STOP_BUZZER_EVENT, 150);
                //Station_start_timerEx(Station_TaskId, STATION_RESET_SAVEDEVID_EVENT, 10);

                osal_memcpy(WireSendBuff,WIRE_SYNC,sizeof(app_mDev_hdr_t));
                osal_memcpy(WireSendBuff+sizeof(app_mDev_hdr_t),(uint8 *)(&eDevHdr),sizeof(app_eDev_hdr_t));
                if(eDevHdr.dataLen)
                    osal_memcpy(WireSendBuff+sizeof(app_mDev_hdr_t)+sizeof(app_eDev_hdr_t),(uint8 *)(&psAppPkt->rf_card_info1.info_data),eDevHdr.dataLen);

                mDevFdr.padding = 0xffff;
                //mDevFdr.crc = CRC16(WireSendBuff,(sizeof(app_mDev_hdr_t)+msdu.len),0xFFFF);
                mDevFdr.crc = CRC16(WireSendBuff,(sizeof(app_mDev_hdr_t)+sizeof(app_eDev_hdr_t)+eDevHdr.dataLen),0xFFFF);
                osal_memcpy(WireSendBuff+sizeof(app_mDev_hdr_t)+sizeof(app_eDev_hdr_t)+eDevHdr.dataLen,(uint8 *)&mDevFdr,sizeof(app_mDev_fdr_t));
                ret= HalUARTWrite(HAL_UART_PORT_1,(uint8*)(&WireSendBuff),sizeof(app_mDev_hdr_t)+sizeof(app_eDev_hdr_t)+sizeof(app_mDev_fdr_t)+eDevHdr.dataLen);

                //RfTofData.tof_head.protocoltype = 3;//APP_PROTOCOL_TYPE_CARD;
                //RfTofData.tof_head.msgtype = 0x10;    //TOF_STATION_CHECKIN_ACK
                //RfTofData.tof_head.len = 0;
                //Send_Data_by_Wireless((uint8*)&RfTofData,0,RfTofData.tof_head.len+sizeof(app_header_t),0,dstAdr,0);

                //return ret;
            }
            else
            {
                saveDevID = 0;
                //return 0;
            }
            break;
        }
    }
    return ret;
}

static void Station_start_timerEx(uint8 taskID,
                                  uint16 event_id,
                                  uint16 timeout_value)
{
    if (ZSuccess != osal_start_timerEx(taskID, event_id, timeout_value))
    {
        SystemReset();
    }
}

//uint_16 datal = 0;

//uint_16 recvdatal = 0;


/**************************************************************************************************
*
* @fn      Station_ParseUartCmd
*
* @brief   parse the uart coming data
*
**************************************************************************************************/
static eUartRecReturn Station_ParseUartCmd(uint_8* pdata, uint_16 len)
{
    /*CRC doesn't include the sync unit*/
    uint_16 crc =  CRC16(pdata, len - sizeof(app_mDev_fdr_t), 0xFFFF);
    uint8 cmdType;
    uint_16 dataLen =0;

    app_eDev_hdr_t* pDevHdr = (app_eDev_hdr_t*)(pdata+sizeof(app_mDev_hdr_t));

    //if(pDevHdr->mDevID != Station_DevInfo.ShortAddr) return NON_MINE;

	//pDevHdr->eDevType=0;
    cmdType = pDevHdr->MSGType;
    eDevType = pDevHdr->eDevType;
    dstID = pDevHdr->eDevID;
    dataLen = pDevHdr->dataLen;

    //if(cmdType == WRITE_INFO)
    //{
        //asm("nop");
    //}


    //if(cmdType == 0xB0)
        //asm("nop");
        

    if((dataLen + sizeof(app_mDev_hdr_t)+ sizeof(app_eDev_hdr_t)+ sizeof(app_mDev_fdr_t)) > len
         &&dataLen < MAC_MAX_FRAME_SIZE -sizeof(app_eDev_hdr_t))
    {
        return LEN_SHORT;
    }

    if(crc != *((uint_16*)(pdata + len - 2)))
    {
        return CRC_ERROR;
    }

    switch(cmdType)
    {
        case (DETECT_ACK):
        {
            HalLedSet(STATION_LED_BLUE, HAL_LED_MODE_ON);
            LF_SEND_CONPIN = 1;
            linkCount = 0;
            if(eDevType == 1)      //资产卡用磁铁激励的版本
                HandheldStatus = POS_CHECKIN;
            
            if(!(mDevStatus&LINKED))
            {
                mDevStatus |= LINKED;
            }
            break;
        }
        case(WRITE_CARD)://write
        {
            //if(!(mDevStatus&LF_0R_1W))
            //{
            //    mDevStatus |= LF_0R_1W;
            //    Station_start_timerEx(Station_TaskId, STATION_WRITE_TIMEOUT_EVENT, 5000);
            //}

 			 if(dataLen)
 			 {
 			 if(!receivedWriteCmd)
			 tof_head_write = osal_mem_alloc(dataLen+4);
			 receivedWriteCmd=TRUE;
			 tof_head_write->protocoltype = 3,//APP_PROTOCOL_TYPE_CARD
			 tof_head_write->msgtype = 0xE0;
			 tof_head_write->len = dataLen;
			 osal_memcpy((char*)(tof_head_write+1),(char *)(pDevHdr+1),dataLen);
                Station_start_timerEx(Station_TaskId, STATION_SENDDATA_EVENT, 1);
            }
            break;
        }

        case(READ_DATA_ACK):
        {
            //uint16 eDevID = ((app_eDev_hdr_t*)(pdata+sizeof(app_mDev_hdr_t)))->eDevID;
            //uint8  eDevType = ((app_eDev_hdr_t*)(pdata+sizeof(app_mDev_hdr_t)))->eDevType;
            //Send_Data_by_Wireless((uint8 *)(pdata+sizeof(app_mDev_hdr_t)),
            //                       eDevType,
            //                      (len - sizeof(app_mDev_hdr_t)- sizeof(app_mDev_fdr_t)),
            //                       MAC_UTIL_UNICAST,
            //                       eDevID,
            //                       0);
            break;
        }

        case SHORT_DISTANCE_CHECK:
        {
            HandheldStatus = SHORT_DISTANCE;
            teExciteState = 0;

			osal_stop_timerEx(Station_TaskId,STATION_REPORT_REMOTE_EVENT);
			vSetStationChannel(Station_DevInfo.Channel);
            stationID = Manchester_Encoding((uint8)(EXCITE_ID));
            sendData = stationID;
			Station_start_timerEx(Station_TaskId, STATION_LF_INSPIRE_EVENT, 1);
            osal_memset(&card_info,0,sizeof(rt_card_info_ts)*40);
            break;
        }

        case LONG_DISTANCE_CHECK:
        {
            HandheldStatus = REMOTE_DISTANCE;
            if(teExciteState)
            {
                teExciteState = 0;
                HalTimerStop(HAL_TIMER_2);
                HalTimerStop(HAL_TIMER_3);

                //u8SetRssiChannel = eDevType;
                //Station_DevInfo.extAddr[LPBSS_MAC_CHA] = u8SetRssiChannel;

                //BSP_FLASH_Write(HAL_FLASH_IEEE_PAGE, HAL_FLASH_IEEE_OSET,(uint8 *)&Station_DevInfo.extAddr, HAL_FLASH_IEEE_SIZE);

            }
			osal_stop_timerEx(Station_TaskId,STATION_REPORT_REMOTE_EVENT);
            Station_start_timerEx(Station_TaskId, STATION_REPORT_REMOTE_EVENT, 1000);
            break;
        }

        case CARD_TYPE_CHECK:
        case STATION_RSSI_CHECK:
        {
            teExciteState = 0;
            HalTimerStop(HAL_TIMER_2);
            HalTimerStop(HAL_TIMER_3);
            break;
        }

        case CHARGE_PERIOD:
        {
			vSetStationChannel(Station_DevInfo.Channel);
			HandheldStatus = POS_MODE;
            stationID = Manchester_Encoding((uint8)(EXCITE_ID + 128));
            sendData = stationID;
	        stationIDcrc = Manchester_Encoding((uint8)calcrc_1byte((uint8)(EXCITE_ID + 128)));
            Station_start_timerEx(Station_TaskId, STATION_LF_INSPIRE_EVENT, 10);

            teStaionState = E_STATE_CHANGE;

            peroid = *(uint8 *)(pDevHdr+1);
            break;
        }

        case POS_RSSI_CHANNNEL:
        {
			vSetStationChannel(Station_DevInfo.Channel);
			HandheldStatus = POS_MODE;
            stationID = Manchester_Encoding((uint8)(EXCITE_ID + 128));
            sendData = stationID;
	        stationIDcrc = Manchester_Encoding((uint8)calcrc_1byte((uint8)(EXCITE_ID + 128)));
            Station_start_timerEx(Station_TaskId, STATION_LF_INSPIRE_EVENT, 10);

            teStaionState = E_STATE_RSSI_CHANNEL;

            RssiChannel = *(uint8 *)(pDevHdr+1);
            break;
        }

        case READ_INFO:
        {
            //Station_start_timerEx(Station_TaskId, STATION_LF_INSPIRE_EVENT, 1);
            //uint8 buftt[4]={'1','2','3','4'};
            //Send_Data_by_Wireless(buftt,0,4,MAC_UTIL_UNICAST,0xFFFF,0);
            vSetStationChannel(Station_DevInfo.Channel);
            u8ReportIdx = eDevType;

            HandheldStatus = POS_MODE;

            teStaionState = E_STATE_READ;

            stationID = Manchester_Encoding((uint8)(EXCITE_ID));
            sendData = stationID;
	        stationIDcrc = Manchester_Encoding((uint8)calcrc_1byte((uint8)(EXCITE_ID)));
            Station_start_timerEx(Station_TaskId, STATION_LF_INSPIRE_EVENT, 10);

            Station_start_timerEx(Station_TaskId,STATION_POLL_TIMER,5000);
            break;
        }

        case WRITE_INFO:
        {
			vSetStationChannel(Station_DevInfo.Channel);
			HandheldStatus = POS_MODE;
            teStaionState = E_STATE_WRITE;
            //stationID = Manchester_Encoding((uint8)(EXCITE_ID+128));
            //stationID <<=16;
            //stationID |=Manchester_Encoding((uint8)calcrc_1byte((uint8)(EXCITE_ID+128)));
            stationID = Manchester_Encoding((uint8)(EXCITE_ID + 128));
            sendData = stationID;
	        stationIDcrc = Manchester_Encoding((uint8)calcrc_1byte((uint8)(EXCITE_ID + 128)));
            Station_start_timerEx(Station_TaskId, STATION_LF_INSPIRE_EVENT, 10);

            p_app_hdr->msgtype = CARD_BASE_INFO;
            p_app_hdr->len = dataLen;
            len_temp = dataLen;
            SysUtil_vConvertEndian(&p_app_hdr->len,sizeof(p_app_hdr->len));
            osal_memcpy((char*)(p_app_hdr+1),(char *)(pDevHdr+1),dataLen);
            Station_start_timerEx(Station_TaskId,STATION_WAIT_TIMEOUT,700);
            Station_start_timerEx(Station_TaskId,STATION_POLL_TIMER,5000);
            break;
        }
 
        case GET_STATION_RSSI:    //0XA0
        //case 0x79:
        {
            teStaionState = E_WAIT_STATION_RSSI;
            u8RssiCnt = 0;

            dstPanID = dstID;
            u8SetRssiChannel = eDevType;
            BSP_FLASH_Erase(HAL_FLASH_INFOMATION_PAGE);

            //Station_DevInfo.extAddr[LPBSS_MAC_CHA] = u8SetRssiChannel;

            BSP_FLASH_Write(HAL_FLASH_INFOMATION_PAGE, 0,(uint8 *)&u8SetRssiChannel, 1);

			Station_DevInfo.Channel = eDevType;


            Station_start_timerEx(Station_TaskId,STATION_POLL_RSSI,10);
            break;
        }

        case SET_RSSI_CHANNNEL:
        {
			vSetStationChannel(Station_DevInfo.Channel);
			charge_card_type.cardtype = pDevHdr->eDevType;
            charge_card_type.tof_head.msgtype = 42;      //修改卡频道
            //charge_card_type.cardtype = pDevHdr->eDevType;
            if((charge_card_type.cardtype < 26) && (charge_card_type.cardtype > 10))
            {
                Send_Data_by_Wireless((uint8*)(&charge_card_type),0,sizeof(CHARGE_CARD_TYPE_T),MAC_UTIL_UNICAST,dstID,0);
            }
            break;
        }

        case CHARGE_CARD_TYPE:
        {
            //uint16 DestAddr = pDevHdr->eDevID;
            vSetStationChannel(Station_DevInfo.Channel);
            charge_card_type.cardtype = pDevHdr->eDevType;
            charge_card_type.tof_head.msgtype = 36;      //修改卡类型
            if(charge_card_type.cardtype < 3)
            {
                Send_Data_by_Wireless((uint8*)(&charge_card_type),0,sizeof(CHARGE_CARD_TYPE_T),MAC_UTIL_UNICAST,dstID,0);
            }
            break;
        }

		case STATION_CHANNEL:
		{
			app_eDev_hdr_t eDevHdr;
    		app_mDev_fdr_t mDevFdr;

			eDevHdr.MSGType = STATION_CHANNEL;
			eDevHdr.eDevType = Station_DevInfo.Channel;
			eDevHdr.eDevID = Station_DevInfo.PanId;
			eDevHdr.mDevID = 0xffff;
			eDevHdr.dataLen = 0;

			osal_memcpy(WireSendBuff,WIRE_SYNC,sizeof(app_mDev_hdr_t));
            osal_memcpy(WireSendBuff+sizeof(app_mDev_hdr_t),(uint8 *)(&eDevHdr),sizeof(app_eDev_hdr_t));
            mDevFdr.padding = 0xffff;
            mDevFdr.crc = CRC16(WireSendBuff,(sizeof(app_mDev_hdr_t)+sizeof(app_eDev_hdr_t)+eDevHdr.dataLen),0xFFFF);
            osal_memcpy(WireSendBuff+sizeof(app_mDev_hdr_t)+sizeof(app_eDev_hdr_t)+eDevHdr.dataLen,(uint8 *)&mDevFdr,sizeof(app_mDev_fdr_t));
            HalUARTWrite(HAL_UART_PORT_1,(uint8*)(&WireSendBuff),sizeof(app_mDev_hdr_t)+sizeof(app_eDev_hdr_t)+sizeof(app_mDev_fdr_t)+eDevHdr.dataLen);
			break;
		}

		case SET_STATION_CHANNEL:
		{
			teStaionState = E_WAIT_STATION_RSSI;
            u8RssiCnt = 0;
            

            dstPanID = dstID;
            u8SetRssiChannel = eDevType;
			if(Station_DevInfo.Channel != u8SetRssiChannel)
			{
	            BSP_FLASH_Erase(HAL_FLASH_INFOMATION_PAGE);

	            //Station_DevInfo.extAddr[LPBSS_MAC_CHA] = u8SetRssiChannel;
	            BSP_FLASH_Write(HAL_FLASH_INFOMATION_PAGE, 0,(uint8 *)&u8SetRssiChannel, 1);
			}

			Station_DevInfo.Channel = eDevType;
			vSetStationChannel(Station_DevInfo.Channel);
			break;
		}

		case TEST_STATION_RSSI:
		{
			if(teStaionState != E_TEST_STATION_RSSI)
			{
				Simulate_card_Startup();
				old_teStaionState = teStaionState;
				teStaionState = E_TEST_STATION_RSSI;
				osal_memset(&station_info,0,sizeof(rt_station_info_ts)*10);
				station_cnt = 0;
				locator_cnt = 0;
				idx = 0;
				vSetStationChannel(u8SetBroadcastChannel);
				Station_start_timerEx(Station_TaskId,LOCATOR_POLL_RSSI,1000);
			}
			break;
		}

        case DEVICE_CARD_READ:
        {
            DevicePosState = DEVICE_POS_READING;
            break;
        }

        case DEVICE_CARD_SETTING:
        {
            unsigned short len = pDevHdr->dataLen;
            DevicePosState = DEVICE_POS_SETTING;

            dstDevAdr = dstID;
            app_rfTlv_t *pTlv = (app_rfTlv_t *)(pDevHdr+1);

            
            while(len >= (sizeof(app_rfTlv_t) + pTlv->len))
            {
                uint8 *pdata = (uint8*)(pTlv+1);
                if(pTlv->type == 0)
                {
                    RssiChannel = *pdata;
                }
                else if(pTlv->type == 1)   //修改手持机频道
                {
                    if(*pdata != Station_DevInfo.Channel)
                    {
                        BSP_FLASH_Erase(HAL_FLASH_INFOMATION_PAGE);
	                    //Station_DevInfo.extAddr[LPBSS_MAC_CHA] = u8SetRssiChannel;
	                    BSP_FLASH_Write(HAL_FLASH_INFOMATION_PAGE, 0,pdata, 1);

                        Station_DevInfo.Channel = *pdata;
			            vSetStationChannel(Station_DevInfo.Channel);

                        app_eDev_hdr_t eDevHdr;
                		app_mDev_fdr_t mDevFdr;

            			eDevHdr.MSGType = DEVICE_CARD_SETTING_ACK;
            			eDevHdr.eDevType = 0;
            			eDevHdr.eDevID = Station_DevInfo.PanId;
            			eDevHdr.mDevID = 0xffff;
            			eDevHdr.dataLen = 0;

            			osal_memcpy(WireSendBuff,WIRE_SYNC,sizeof(app_mDev_hdr_t));
                        osal_memcpy(WireSendBuff+sizeof(app_mDev_hdr_t),(uint8 *)(&eDevHdr),sizeof(app_eDev_hdr_t));
                        mDevFdr.padding = 0xffff;
                        mDevFdr.crc = CRC16(WireSendBuff,(sizeof(app_mDev_hdr_t)+sizeof(app_eDev_hdr_t)+eDevHdr.dataLen),0xFFFF);
                        osal_memcpy(WireSendBuff+sizeof(app_mDev_hdr_t)+sizeof(app_eDev_hdr_t)+eDevHdr.dataLen,(uint8 *)&mDevFdr,sizeof(app_mDev_fdr_t));
                        HalUARTWrite(HAL_UART_PORT_1,(uint8*)(&WireSendBuff),sizeof(app_mDev_hdr_t)+sizeof(app_eDev_hdr_t)+sizeof(app_mDev_fdr_t)+eDevHdr.dataLen);
                        break;
                        
                    }
                }
                else if(pTlv->type == 2)
                {
                    peroid = *pdata;
                }

                len -= (sizeof(app_rfTlv_t)  + pTlv->len);
                pTlv = (app_rfTlv_t *)(pdata  + pTlv->len);
            }
            
            break;
        }

        case DEVICE_CARD_ERPID:
        {
            if(pDevHdr->dataLen < 20)
            {
                rt_erpid_info.u8ErpIdLen = pDevHdr->dataLen;
                osal_memcpy(rt_erpid_info.u8ErpBuf,(char *)(pDevHdr+1),rt_erpid_info.u8ErpIdLen);
                DevicePosState = DEVICE_POS_ERP;
                dstDevAdr = dstID;
            }
            break;
        }

        /*case WRITE_CHANNEL:
        {
            channel = *(uint8 *)(pDevHdr+1);
            if((channel < 26) && (channel > 10))
            {
                MAC_MlmeSetReq(MAC_LOGICAL_CHANNEL, &channel);
                DoubleChannel = false;
            }
            else
            {
                DoubleChannel = true;
            }
            break;
        }*/

        default:
            break;
    }
    return GOOD_WORK;
}

/**************************************************************************************************
*
* @fn      void     Station_send_mbusHdr();
*
* @brief   sent   mbusHdr
*
* @return  crc
**************************************************************************************************/

void Station_Send_Data_by_Wire(uint8*  pData, uint16 dataLen)
{
    HalUARTWrite(HAL_UART_PORT_1, pData, dataLen);
}

#if 0
/**************************************************************************************************
*
* @fn      void     Station_send_by_LF();
*
* @brief
*
* @return  void
**************************************************************************************************/

static void   Station_send_by_LF(uint8 *pdata,uint16 datalen)
{
    halIntState_t his;
    HAL_ENTER_CRITICAL_SECTION(his);


    if((pdata == NULL)||(datalen == 0)||(datalen > LF_DATA_MAX_LEN))
    {
        return;
    }
    for(uint8 i=0; i<2; i++)
    {
        if(LFDataBuff[i].sendingFlag)
        {
            continue;
        }

        LFDataBuff[i].len = datalen;
        osal_memcpy(&LFDataBuff[i].LfData,pdata,datalen);
        LFDataBuff[i].needSentFlag = true;
        break;
    }
    HAL_EXIT_CRITICAL_SECTION(his);

}

/**************************************************************************************************
*
* @fn   bool Get_Bool_for_LF(void)
*
* @brief
*
* @return  bool
**************************************************************************************************/

uint8 Get_8bits_for_LF(void)
{
    static uint8 saveLen = 0;
    static uint8 buffIdx =0;
    static uint8 idx =2;
    uint8 data =0 ;
    if(saveLen == 0)
    {
        for(uint8 i=0; i<2; i++)
        {
            if(LFDataBuff[i].needSentFlag && LFDataBuff[i].len)
            {
                LFDataBuff[i].sendingFlag = true;
                idx = i;
                saveLen = LFDataBuff[i].len;
                buffIdx = 0;
                break;
            }
        }
    }

    if(idx <2)
    {
         if(saveLen)saveLen -= 1;
         data = LFDataBuff[idx].LfData[buffIdx++];
         if(saveLen ==0)
         {
             LFDataBuff[idx].needSentFlag = false;
             LFDataBuff[idx].sendingFlag = false;
             idx =2;
         }

    }
    return data;
}

#endif


uint16 Manchester_Encoding(uint8 uncode)
{
	int32 i;
	//rt_uint16_t encode=0;
	int32 index=0x80;
	uint16 encode=0;

	for(i=0;i<8;i++)
	{

		encode=(encode<<2);
		if(uncode&(index>>i))
		{
			encode|=2;
		}
		else
		{
			encode|=1;
		}
	}
	return encode;
}




#if 0

/*
 *  Get Len of LF data
 */
uint16 Get_LFdata_Len(void)
{
    return sLFBuf[0];
}
#endif
/*
 *  Start  Timer4
 */
void Start_Timer4(void)
{
    // TIMER4 channel 1
    P2DIR |= (0x01 << 0);  //set to output
    PERCFG |= (0x01 << 4); // set timer4 location
    P2_0 = 0;

    P2SEL |= (0x01 << 0);  // set to peripheral

    T4CCTL0 = (0x00 << 6) | (0x02 << 3) | (0x01 << 2) ;
    T4CC0 = 0x7F;
    T4CTL = (0x00 << 5) | (0x00 << 3) | (0x02 << 0);
    T4CTL &= ~(0x01 << 2); //clear
    T4CTL |= (0x01 << 4);  //start
}



/*
 *  Ready for Send  of  LF Data      timer4 use to make 125k
 */
void Init_LF_Exciter(void)
{
    //timer4
    Start_Timer4();

    // P0_3
    P0DIR |= (0x01 << 3);
    P0SEL &= ~(0x01 << 3);
    //HalTimerStart(HAL_TIMER_0,0x1f4);
    //HalTimerStart(HAL_TIMER_0,0x218);
}

static bool checkCardShortAddr(uint16 shortAddr,uint32 timeTick,int8 rssi)
{
    uint8 i;
    uint8 j = 41;
    for(i=0;i<40;i++)
    {
        if(shortAddr == card_info[i].u16ShortAddr)
        {
            card_info[i].u32TimeTick = timeTick;
            card_info[i].i8CardRssi = rssi;
            break;
        }
        else if(card_info[i].u16ShortAddr == 0)
        {
            j = i;
        }
    }
    if(i>39)
    {
        if(j < 41)
        {
            card_info[j].u32TimeTick = timeTick;
            card_info[j].i8CardRssi = rssi;
            card_info[j].u16ShortAddr = shortAddr;
            return true;
        }
        else
            return false;
    }
    else
    {
        return true;
    }
}

static void ReportCardRssi()
{
    uint8 i;
    uint8 j = 0;
    uint8 combuf[256];
    static uint8 seq = 0;
    rt_card_rssi_ts *p;

    app_eDev_hdr_t eDevHdr;
    app_mDev_fdr_t mDevFdr;

    eDevHdr.MSGType = REMOTE_DATA;
    eDevHdr.eDevID = 0xffff;
    eDevHdr.eDevType = seq++;
    eDevHdr.mDevID = Station_DevInfo.PanId;

    osal_memcpy(combuf,WIRE_SYNC,sizeof(app_mDev_hdr_t));
    p = (rt_card_rssi_ts *)(combuf + sizeof(app_mDev_hdr_t) + sizeof(app_eDev_hdr_t));

    quicksort();

    uint32 timeTick = osal_getClock();
    for(i=0;i<40;i++)
    {
        if(card_info[i].u16ShortAddr != 0)
        {
            if((timeTick - card_info[i].u32TimeTick) > 11)
            {
                card_info[i].u16ShortAddr = 0;
            }
            else
            {
                p->i8CardRssi = card_info[i].i8CardRssi;
                p->u16ShortAddr = card_info[i].u16ShortAddr;
                p++;
                j++;
            }
        }
    }

    eDevHdr.dataLen = sizeof(rt_card_rssi_ts) * j;
    osal_memcpy(combuf+sizeof(app_mDev_hdr_t),(uint8 *)(&eDevHdr),sizeof(app_eDev_hdr_t));
    uint8 len = sizeof(app_mDev_hdr_t)+sizeof(app_eDev_hdr_t) + eDevHdr.dataLen;

    mDevFdr.padding = 0xffff;
    mDevFdr.crc = CRC16(combuf,len,0xFFFF);
    osal_memcpy(combuf + len,(uint8 *)&mDevFdr,sizeof(app_mDev_fdr_t));
    HalUARTWrite(HAL_UART_PORT_1,(uint8*)(&combuf),len + sizeof(app_mDev_fdr_t));
}

static void quicksort(void)
{
    uint8 i,j;
    rt_card_info_ts temp;

    for(i=0;i < 39;i++)
    {
        for(j=0;j<39-i;j++)
        {
            if(card_info[j].i8CardRssi < card_info[j+1].i8CardRssi)
            {
                temp = card_info[j];
                card_info[j] = card_info[j+1];
                card_info[j+1]=temp;
            }
        }
    }
}

static void ReportStationRssi()
{
    uint8 combuf[256];
    static uint8 seq = 0;
    rt_station_info_ts *p;

    app_eDev_hdr_t eDevHdr;
    app_mDev_fdr_t mDevFdr;

    eDevHdr.MSGType = TEST_STATION_RSSI_ACK;
    eDevHdr.eDevID = 0xffff;
    eDevHdr.eDevType = seq++;
    eDevHdr.mDevID = Station_DevInfo.PanId;

    osal_memcpy(combuf,WIRE_SYNC,sizeof(app_mDev_hdr_t));
    p = (rt_station_info_ts *)(combuf + sizeof(app_mDev_hdr_t) + sizeof(app_eDev_hdr_t));

	osal_memcpy(p,station_info,station_cnt*sizeof(rt_station_info_ts));

    eDevHdr.dataLen = sizeof(rt_station_info_ts) * station_cnt;
    osal_memcpy(combuf+sizeof(app_mDev_hdr_t),(uint8 *)(&eDevHdr),sizeof(app_eDev_hdr_t));
    uint8 len = sizeof(app_mDev_hdr_t)+sizeof(app_eDev_hdr_t) + eDevHdr.dataLen;

    mDevFdr.padding = 0xffff;
    mDevFdr.crc = CRC16(combuf,len,0xFFFF);
    osal_memcpy(combuf + len,(uint8 *)&mDevFdr,sizeof(app_mDev_fdr_t));
    HalUARTWrite(HAL_UART_PORT_1,(uint8*)(&combuf),len + sizeof(app_mDev_fdr_t));
}


void Send_Data2Station(uint8* pdata ,uint16 panid,uint16 len,uint8 DeliverMode, uint16 uniDstAddr, uint8 txOption)
{
    MacParam_t param;
    param.cluster_id = CARD_CLUSTERID;
    param.radius = 0x01;

    param.panID = panid;

    if(DeliverMode == MAC_UTIL_UNICAST)
    {
        sAddr_t DstAddr;
        DstAddr.addrMode             = SADDR_MODE_SHORT;

        DstAddr.addr.shortAddr = uniDstAddr; //if an enddevice, need to set mac dstaddr to coordinator's addr.
        MAC_UTIL_McpsDataReq(pdata, len, param.panID, DstAddr, txOption);//MAC_TXOPTION_ACK);
    }
    else
    {
        sAddr_t DstAddr;
        DstAddr.addrMode             = SADDR_MODE_SHORT;  /* MAC addMode shoud be 0X02 even broadcast */
        DstAddr.addr.shortAddr      = uniDstAddr; /* Address should be 0xFF in mac header and 0xFc or 0xFF in NWK header */
        MAC_UTIL_McpsDataReq(pdata, len,param.panID, DstAddr, txOption);//MAC_TXOPTION_NO_RETRANS);
    }

}


void Send_Data_by_Wireless(uint8* pdata ,uint8 devType,uint16 len,uint8 DeliverMode, uint16 uniDstAddr, uint8 txOption)
{
    MacParam_t param;
    param.cluster_id = CARD_CLUSTERID;
    param.radius = 0x01;
    //if(devType == STAFF_CARD_DEVICE_ID)
    //{
        param.panID = 0xFFF0;
    //}
    //else
    //{
        //param.panID = DEVICE_CARD_PANID;
    //}
    if(DeliverMode == MAC_UTIL_UNICAST)
    {
        sAddr_t DstAddr;
        DstAddr.addrMode             = SADDR_MODE_SHORT;

        DstAddr.addr.shortAddr = uniDstAddr; //if an enddevice, need to set mac dstaddr to coordinator's addr.
        MAC_UTIL_McpsDataReq(pdata, len, param.panID, DstAddr, txOption);//MAC_TXOPTION_ACK);
    }
    else
    {
        sAddr_t DstAddr;
        DstAddr.addrMode             = SADDR_MODE_SHORT;  /* MAC addMode shoud be 0X02 even broadcast */
        DstAddr.addr.shortAddr      = uniDstAddr; /* Address should be 0xFF in mac header and 0xFc or 0xFF in NWK header */
        MAC_UTIL_McpsDataReq(pdata, len,param.panID, DstAddr, txOption);//MAC_TXOPTION_NO_RETRANS);
    }

}
#if 0
static void Start_buzzer(void)
{
    //halTimerSetCount (0x00, 0x105);
    // P0_4
    P0SEL |= (0x01 << 4);// set to peripheral

    T1CCTL2 = (0x00 << 6) | (0x02 << 3) | (0x01 << 2) ;
    T1CC0H = 0x29;
    T1CC0L = 0x28;
    T1CTL = 0;

    T1CTL &= ~0x03; //clear
    T1CTL |= 0x02;  //start
}

static void Stop_buzzer(void)
{
    //HalTimerStop(HAL_TIMER_3);
    T1CTL &= ~(0x03); //clear
    P0_4 = 0;
}
#endif

static void Station_ParseRssiFrame(const sAddr_t* sAddr,uint16 srcPanId,
                            const sData_t msdu,int8 rssi)
{
    if(sAddr->addr.shortAddr != 0)
    {
        return;
    }

    app_eDev_hdr_t eDevHdr;
    app_mDev_fdr_t mDevFdr;

    rt_Station_rssi_ts station_rssi;

	RfTofWrapper_tu * psAppPkt = (RfTofWrapper_tu*)(msdu.p);

    eDevHdr.MSGType = GET_STATION_RSSI_ACK;
    eDevHdr.eDevID = srcPanId;
    eDevHdr.eDevType = 0;
    eDevHdr.mDevID = 0xffff;
    eDevHdr.dataLen = 0x04;


    station_rssi.u32StationPanId = dstPanID + 10000*psAppPkt->rf_tof_station_rssi.u8StationPort;
    station_rssi.i8RecvRssi = psAppPkt->rf_tof_station_rssi.i8Rssi;
    station_rssi.i8TransmitRssi = rssi;

    if(psAppPkt->rf_tof_station_rssi.u8StationPort != 0)
    {
        asm("nop");
    }


    osal_memcpy(WireSendBuff,WIRE_SYNC,sizeof(app_mDev_hdr_t));
    osal_memcpy(WireSendBuff+sizeof(app_mDev_hdr_t),(uint8 *)(&eDevHdr),sizeof(app_eDev_hdr_t));
	osal_memcpy(WireSendBuff+sizeof(app_mDev_hdr_t)+sizeof(app_eDev_hdr_t),(uint8 *)(&station_rssi),eDevHdr.dataLen);

    mDevFdr.padding = 0xffff;
    mDevFdr.crc = CRC16(WireSendBuff,(sizeof(app_mDev_hdr_t)+sizeof(app_eDev_hdr_t))+eDevHdr.dataLen,0xFFFF);
    osal_memcpy(WireSendBuff+sizeof(app_mDev_hdr_t)+sizeof(app_eDev_hdr_t)+eDevHdr.dataLen,(uint8 *)&mDevFdr,sizeof(app_mDev_fdr_t));
    HalUARTWrite(HAL_UART_PORT_1,(uint8*)(WireSendBuff),sizeof(app_mDev_hdr_t)+sizeof(app_eDev_hdr_t)+sizeof(app_mDev_fdr_t)+eDevHdr.dataLen);
}


static uint8 Station_ParseCardDeviceFrame(const sAddr_t* sAddr,uint16 srcPanId,
                                    const sData_t msdu,
                                    int8 rssi)
{
#if 0
    uint8 ret;
    if(sAddr->addr.shortAddr == 0)
    {
        return 0;
    }
	dstAdr = sAddr->addr.shortAddr;

    app_eDev_hdr_t eDevHdr;
    app_mDev_fdr_t mDevFdr;
    //uint16 curDevID = 0;


	app_header_t * psAppPkt = (app_header_t*)(msdu.p);

    eDevHdr.MSGType = 0x84;
    eDevHdr.eDevID = sAddr->addr.shortAddr;
    eDevHdr.eDevType = 0;
    eDevHdr.mDevID = 0xffff;
    eDevHdr.dataLen = (psAppPkt->len>>8&0xff)+(psAppPkt->len<<8&0xff00);

    //curDevID = eDevHdr.eDevID;

    osal_memcpy(WireSendBuff,WIRE_SYNC,sizeof(app_mDev_hdr_t));
    osal_memcpy(WireSendBuff+sizeof(app_mDev_hdr_t),(uint8 *)(&eDevHdr),sizeof(app_eDev_hdr_t));
	osal_memcpy(WireSendBuff+sizeof(app_mDev_hdr_t)+sizeof(app_eDev_hdr_t),(uint8 *)(psAppPkt+1),eDevHdr.dataLen);

    mDevFdr.padding = 0xffff;
    //mDevFdr.crc = CRC16(WireSendBuff,(sizeof(app_mDev_hdr_t)+msdu.len),0xFFFF);
    mDevFdr.crc = CRC16(WireSendBuff,(sizeof(app_mDev_hdr_t)+sizeof(app_eDev_hdr_t))+eDevHdr.dataLen,0xFFFF);
    osal_memcpy(WireSendBuff+sizeof(app_mDev_hdr_t)+sizeof(app_eDev_hdr_t)+eDevHdr.dataLen,(uint8 *)&mDevFdr,sizeof(app_mDev_fdr_t));
    ret= HalUARTWrite(HAL_UART_PORT_1,(uint8*)(WireSendBuff),sizeof(app_mDev_hdr_t)+sizeof(app_eDev_hdr_t)+sizeof(app_mDev_fdr_t)+eDevHdr.dataLen);
	receiveAck();
       if(WireSendBuff[14]==0x32)
       {
         mDevFdr.padding = 0xffff;
       }
    return ret;
#endif
	uint8 ret;
	static uint16 current_crc;
    static uint16 receive_crc;
	static uint8 current_sequnum=0;
	static uint8 last_sequnum=0;
	static uint8 total_sequnum_cnt=0;
	static uint8 write_pos=0;
	static int equeli=0;
	if(sAddr->addr.shortAddr == 0)
	{
		return 0;
	}
	dstAdr = sAddr->addr.shortAddr;

	app_eDev_hdr_t eDevHdr;
	app_mDev_fdr_t mDevFdr;
	//uint16 curDevID = 0;

	app_header_t * psAppPkt = (app_header_t*)(msdu.p);

	eDevHdr.MSGType = 0x84;
	eDevHdr.eDevID = sAddr->addr.shortAddr;
	eDevHdr.eDevType = 0;
	eDevHdr.mDevID = 0xffff;
	eDevHdr.dataLen = (psAppPkt->len>>8&0xff)+(psAppPkt->len<<8&0xff00);

	current_crc = CRC16((uint8*)psAppPkt,eDevHdr.dataLen+6,0xFFFF);
        receive_crc =*(uint16*)((uint8*)psAppPkt+6+eDevHdr.dataLen);
	if(current_crc!=receive_crc)
	{
	  return 0;
	}

	current_sequnum=*((uint8*)psAppPkt+4+eDevHdr.dataLen);
	total_sequnum_cnt=*((uint8*)psAppPkt+5+eDevHdr.dataLen);

	if(current_sequnum==1 && (eDevHdr.dataLen+write_pos)<256)
	{
    	osal_memcpy(WriteSendBuff,WIRE_SYNC,sizeof(app_mDev_hdr_t));
    	osal_memcpy(WriteSendBuff+sizeof(app_mDev_hdr_t),(uint8 *)(&eDevHdr),sizeof(app_eDev_hdr_t));
    	write_pos=12;
	}
	if(current_sequnum==(last_sequnum+1) && current_sequnum==total_sequnum_cnt && (eDevHdr.dataLen+write_pos)<256)
	{
	    mDevFdr.padding = 0xffff;
        if(current_sequnum>1)
        {
            WriteSendBuff[write_pos]=0x2f;
            WriteSendBuff[write_pos+1]=0x2f;
            write_pos+=2;
        }
	osal_memcpy(WriteSendBuff+write_pos,(uint8 *)(psAppPkt+1),eDevHdr.dataLen);
	write_pos+=eDevHdr.dataLen;
	eDevHdr.dataLen = write_pos-12;
	osal_memcpy(WriteSendBuff+sizeof(app_mDev_hdr_t),(uint8 *)(&eDevHdr),sizeof(app_eDev_hdr_t));
    mDevFdr.crc = CRC16(WriteSendBuff,write_pos,0xFFFF);
	osal_memcpy(WriteSendBuff+write_pos,(uint8 *)&mDevFdr,sizeof(app_mDev_fdr_t));

	ret= HalUARTWrite(HAL_UART_PORT_1,(uint8*)(WriteSendBuff),sizeof(app_mDev_fdr_t)+write_pos);
		//receiveAck();
	last_sequnum=0;
	total_sequnum_cnt=0;
    current_sequnum =0;
	write_pos=0;
	}
	else if(current_sequnum==(last_sequnum+1) && (eDevHdr.dataLen+write_pos)<256)
	{
        if(current_sequnum>1)
        {
          WriteSendBuff[write_pos]=0x2f;
          WriteSendBuff[write_pos+1]=0x2f;
          write_pos+=2;
        }
	osal_memcpy(WriteSendBuff+write_pos,(uint8 *)(psAppPkt+1),eDevHdr.dataLen);
	write_pos+=eDevHdr.dataLen;
	last_sequnum=current_sequnum;
		//receiveAck();
	}
	else if(current_sequnum==(last_sequnum+1) && (eDevHdr.dataLen+write_pos)>256)
	{
	mDevFdr.padding = 0xffff;


	eDevHdr.dataLen = write_pos-12;

	osal_memcpy(WriteSendBuff+sizeof(app_mDev_hdr_t),(uint8 *)(&eDevHdr),sizeof(app_eDev_hdr_t));
    mDevFdr.crc = CRC16(WriteSendBuff,write_pos,0xFFFF);
	osal_memcpy(WriteSendBuff+write_pos,(uint8 *)&mDevFdr,sizeof(app_mDev_fdr_t));
	ret= HalUARTWrite(HAL_UART_PORT_1,(uint8*)(WriteSendBuff),sizeof(app_mDev_fdr_t)+write_pos);
	write_pos=0;

	osal_memcpy(WriteSendBuff,WIRE_SYNC,sizeof(app_mDev_hdr_t));
	osal_memcpy(WriteSendBuff+sizeof(app_mDev_hdr_t),(uint8 *)(&eDevHdr),sizeof(app_eDev_hdr_t));
	osal_memcpy(WriteSendBuff+sizeof(app_mDev_hdr_t)+sizeof(app_eDev_hdr_t),(uint8 *)(psAppPkt+1),eDevHdr.dataLen);
	write_pos=12+eDevHdr.dataLen;
	last_sequnum=current_sequnum;
		//receiveAck();
	}
	else if(last_sequnum==current_sequnum)
	{
	equeli++;
	if(equeli==5)
	{
	equeli=0;

	}

	}
	//osal_memcpy(WireSendBuff,WIRE_SYNC,sizeof(app_mDev_hdr_t));
	//osal_memcpy(WireSendBuff+sizeof(app_mDev_hdr_t),(uint8 *)(&eDevHdr),sizeof(app_eDev_hdr_t));
	//osal_memcpy(WireSendBuff+sizeof(app_mDev_hdr_t)+sizeof(app_eDev_hdr_t),(uint8 *)(psAppPkt+1),eDevHdr.dataLen);

	//mDevFdr.padding = 0xffff;
	//mDevFdr.crc = CRC16(WireSendBuff,(sizeof(app_mDev_hdr_t)+msdu.len),0xFFFF);
	//mDevFdr.crc = CRC16(WireSendBuff,(sizeof(app_mDev_hdr_t)+sizeof(app_eDev_hdr_t))+eDevHdr.dataLen,0xFFFF);
	//osal_memcpy(WireSendBuff+sizeof(app_mDev_hdr_t)+sizeof(app_eDev_hdr_t)+eDevHdr.dataLen,(uint8 *)&mDevFdr,sizeof(app_mDev_fdr_t));
	//ret= HalUARTWrite(HAL_UART_PORT_1,(uint8*)(WireSendBuff),sizeof(app_mDev_hdr_t)+sizeof(app_eDev_hdr_t)+sizeof(app_mDev_fdr_t)+eDevHdr.dataLen);

	   if(WriteSendBuff[14]==0x32)
	   {
		 mDevFdr.padding = 0xffff;
	   }
        receiveAck();
	return ret;
}


static uint8 Station_ParseDeviceAckFrame(const sAddr_t* sAddr,uint16 srcPanId,
                                    const sData_t msdu,int8 rssi)
{
    //if(sAddr->addr.shortAddr == 0)
    //{
        //return 0;
    //}
	//ACK_PACKET_T * psAppPkt = (ACK_PACKET_T*)(msdu.p);
#if 0
	switch(psAppPkt->ackmsg)
	{
	case 0xe2://openack
	if(receivedOpenCmd)
	{
	receivedOpenCmd = FALSE;
	osal_mem_free(tof_head_open);
	}
	break;
	case 0xe0://writeack
	if(receivedWriteCmd)
	{
	receivedWriteCmd = FALSE;
	osal_mem_free(tof_head_write);
	}
    break;
	case 0xe3://closeack
	if(receivedCloseCmd)
	{
	receivedCloseCmd = FALSE;
	osal_mem_free(tof_head_close);
	}
	break;
	default:
	break;

	}
#endif
	return 0;
}

void Station_LF_Parameter(void)
{
    pIndex = 0x8000;
    dIndex = 0x8000;
    state = 0;
    PreamblePoll = 0;
    PatternPoll = 0;
    //DataPoll = 0;
    sendCount = 0;
    PatternCount = 0;
}


void receiveAck(void)
{
	tof_head_ack = osal_mem_alloc(4);
	tof_head_ack->protocoltype = 3;//APP_PROTOCOL_TYPE_CARD
	tof_head_ack->msgtype = 0xE4;
	tof_head_ack->len = 0;
	Send_Data_by_Wireless((uint8*)tof_head_ack,eDevType,4,MAC_UTIL_UNICAST,dstAdr ,0);
	osal_mem_free(tof_head_ack);
}

/********************************************************/
/*CRC8校验程序*/
/********************************************************/
 uint8 calcrc_1byte(uint8 abyte)
 {
    uint8 i,crc_1byte;
    crc_1byte=0;                //设定crc_1byte初值为0
    for(i = 0; i < 8; i++)
     {
       if(((crc_1byte^abyte)&0x01))
          {
            crc_1byte^=0x18;
            crc_1byte>>=1;
            crc_1byte|=0x80;
           }
        else
           crc_1byte>>=1;
        abyte>>=1;
      }
      return crc_1byte;
 }



/**************************************************************************************************
**************************************************************************************************/
