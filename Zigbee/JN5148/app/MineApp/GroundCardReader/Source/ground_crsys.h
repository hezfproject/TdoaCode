#ifndef _GROUND_CARDREADER_H_
#define _GROUND_CARDREADER_H_

/****************************************************************************/
/***        Include files                                                 ***/
/****************************************************************************/
#include "lpbssnwk.h"
#include "MbusProto.h"
#include "RadioProto.h"
#include "version.h"

#include "ground_systime.h"
#include "ground_uart.h"
#include "ground_track.h"
#include "ground_485.h"
#ifdef USE_JMP_NET
#include "ground_jmp.h"
#endif
#include "ground_packed.h"
#include "ground_radio.h"

/****************************************************************************/
/***        Macro Definitions                                             ***/
/****************************************************************************/
/* Utility */
#define CONVERT_ENDIAN(x)   SysUtil_vConvertEndian(&(x), sizeof(x))

/* LEDS */
#define LED_LINK            E_AHI_DIO8_INT
#define LED_RF              E_AHI_DIO9_INT
#define LED_RUN             E_AHI_DIO10_INT
#define HARDWARE_DOG        E_AHI_DIO16_INT

#define CARD_SEARCH_MAX     (LPBSS_MAX_CARD_CMD_RECORDS + 1)
#define CARD_CANCEL_IDX     (0)

#define CARD_SMS_MAX        (10)
#define CARD_SMS_SIZE       (60)
#define CARD_SMS_TIMEOUT    (32 * _1S)

/* app envents */
#define SYSTEM_TICK_EVENT                   0x0001
#define BUS485_SEND_DATA_EVENT              0x0002
#define BUS485_PARSER_EVENT                 0x0004
#define BUS485_SEND_ADDR_EVENT              0x0008
#define CARDREADER_OFF_LED_RF_EVENT         0x0010
#define CARDREADER_OFF_LED_LINK_EVENT       0x0020
#define CARDREADER_LED_TOGGLE_EVENT         0x0040
#define JMP_TICK_EVENT                      0x0080
#define CARDREADER_STETIME_TIMEOUT          0x0100
#define CARDREADER_SEND_SMS_EVENT           0x0200
#define CARDREADER_SEND_WRIST_BACKOF        0x0400

#define TIME_SERVICE_TIMEOUT    (21 * _1S)

#define BUS485_LINK_SYNC_CYCLE              (_5S)
#define SYS_VER_REPORT_CYCLE                (60*60*1000)

/****************************************************************************/
/***        Type Definitions                                              ***/
/****************************************************************************/
typedef enum
{
    E_STATE_IDLE,
    E_STATE_STARTED,
} state_e;

typedef struct
{
    uint16 u16SysAddr;
    uint16 u16SysPanid;

    uint8 u8SysModel;
    uint8 u8SysChannel;
    uint8 u8StationState;
    uint8 u8CmdType;

    uint8 u8LastHandle;
    bool_t bMsgSending;    // if the msg is still sending or not
    uint8 u8LoseLinkCnt;

    uint32 u32LastAirRxTimeMs;
    uint32 u32LastFeedDog;
    uint32 u32LastScanCardInfo;
    uint32 u32LastSyncBus485;
    uint32 u32LastSyncVer;
}SYS_MGR_T;

/*******************************************************************************
****ÏÂÐÐ»º³åÇø
*******************************************************************************/
typedef struct
{
    uint8 u8CardCnt;
    bool_t bRecordActive[SET_CARD_INFO_NUM];
    uint32 u32RecordTime[SET_CARD_INFO_NUM];
    uint16 u16CardAddrList[SET_CARD_INFO_NUM];
    uint8 aau8CardSet[SET_CARD_INFO_NUM][BUS485_MAX_RX_LEN];
}card_set_t;

typedef struct
{
    bool_t bRecordActive;
    uint16  u16Year;
    uint8  u8Month;
    uint8  u8Day;
    uint8  u8Hour;
    uint8  u8Minute;
    uint8  u8Second;
}card_time_t;

typedef struct
{
    uint8 u8SearchCnt;
    bool_t bRecordActive[CARD_SEARCH_MAX];
    uint32 u32RecordTime[CARD_SEARCH_MAX];
    LPBSS_card_search_record_t astCardSet[CARD_SEARCH_MAX];
}card_search_t;

#define SMS_READY   2
#define SMS_WAIT    1
#define SMS_OVER    0

typedef struct
{
    uint8 u8FreeIdx;
    uint8 u8PoolIdx;
    uint8 u8SmsCnt;
    uint8 u8SmsState[CARD_SMS_MAX];
    bool_t bRecordActive[CARD_SMS_MAX];
    uint16 u16RecordAddress[CARD_SMS_MAX];
    uint32 u32RecordTime[CARD_SMS_MAX];
    uint8 astCardSms[CARD_SMS_MAX][BUS485_MAX_RX_LEN];
} __PACKED card_sms_t;

/****************************************************************************/
/***        Exported Functions                                            ***/
/****************************************************************************/
PUBLIC uint16 app_get_sysver(uint8 * const pu8Buf, uint16 size);

PUBLIC void app_cmd_search(LPBSS_Msg_Header_t *pstMsg, uint16 size);

PUBLIC void app_cmd_setime(LPBSS_Msg_Header_t *pstMsg, uint16 size);

PUBLIC void app_cmd_setinfo(LPBSS_Msg_Header_t *pstMsg, uint16 size);

PUBLIC void app_packet_parser(LPBSS_Msg_Header_t *pstMsg, uint16 size);

PUBLIC void app_led_ontimer(uint32, uint32);

/****************************************************************************/
/***        Exported Variables                                            ***/
/****************************************************************************/
extern card_search_t g_stSearchCardBuf;
extern card_set_t g_stSetCardBuf;
extern card_time_t g_stStationTime;
extern card_sms_t g_stSendSmsBuf;

extern SYS_MGR_T g_stSysMgr;

#endif

