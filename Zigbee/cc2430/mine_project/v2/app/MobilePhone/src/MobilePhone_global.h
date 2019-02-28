/**************************************************************************************************
Filename:       MP_global.h
Revised:        $Date: 2011/04/12 01:45:50 $
Revision:       $Revision: 1.11 $

Description:   User definable global Parameters.
**************************************************************************************************/
#ifndef MP_GLOBAL_H
#define MP_GLOBAL_H
/*********************************************************************
* INCLUDES
*/
#include "hal_types.h"
#include "sAddr.h"
#include "string.h"
#include "app_protocol.h"
#include "Mac_api.h"
#include "MobilePhone_cfg.h"
#include "Hal_Alarm.h"
/*************************************************************************************************
*MACROS
*/

/*Describe a MP status:
*                                                                   ->idle
* For calling side: idle->calling->calling-wait->|
*                                                                   ->audio->idle
*                                             ->idle
* For called side: idle->called->|
*                                             ->audio->idle
* For Search Nwk: idle->SearchNwk->idle
*/
#define WORK_STATUS_IDLE                      (0x00)
#define WORK_STATUS_AUDIO                   (0x01)
#define WORK_STATUS_CALLING                (0x02)
#define WORK_STATUS_CALLINGWAIT         (0x03)
#define WORK_STATUS_CALLED                  (0x04)
#define WORK_STATUS_SM_SENDING          (0x05)
#define WORK_STATUS_FOUND                     (0x06)
#define WORK_STATUS_WAKE                    (0x07)

typedef struct
{
    bool abnormalRst;
    bool backLightOn;
} StoreParam_t;


typedef struct
{
    /* common */
    uint8    Channel;
    uint16   CoordPanID;
    uint16   armid;

    /* panID I want to associate, if 0xFFFF, all pan ID is allowed */
    uint16    DesireCoordPanID;

    sAddrExt_t ExitAddr;

    /* this two is the same, one in uint16 and one in string */
    uint16       	nmbr;
    app_termNbr_t	termNbr;

    /* poll params*/
    bool    hascoordlink;
    int8     currentRssi;
} MP_DevInfo_t;

typedef struct
{
    uint16               peernmbr;          /* peer numbers */
    app_termNbr_t  peer_termnbr;
    uint8                 seqnum;            // the last voice  seqnum
    uint16              cmdseqnum;      /* if calling cmd_seqnum=dial_seqnum, if called, cmd_seqnum=peer dial_seqnum */
    uint32              peer_tick;          /* the timer tick of peers last voice packet */

    /* signal retry counts */
    uint8               dialup_cnt;
    uint8               accept_cnt;
    uint8               close_cnt;

    uint8                retrying_bitmap;
} MP_AudioInfo_t;

#define MP_DIALUP_RETRY_BIT 0x01;
#define MP_ACCEPT_RETRY_BIT 0x02;
#define MP_CLOSE_RETRY_BIT 0x04;

typedef enum
{
    NWK_DETAIL_INIT,
    //NWK_DETAIL_JOINASKINGARMID,
    NWK_DETAIL_JOINASSOCING,
    //NWK_DETAIL_CELLASKINGARMID,
    NWK_DETAIL_CELLSWITCHING,
    NWK_DETAIL_ENDDEVICE,
} MP_nwkdetail_type_t;

typedef struct
{
    uint8 nwkState;			/* state machine of nwk status */
} MP_NwkInfo_t;

typedef struct
{
    bool  matched;    // flag if matched the condition this time
    uint8 cnt;	 //  time count of continuesly matched the cell switch condiontion
    int8  rssi;
    uint16 panid;
} MP_cell_info_t;

typedef struct
{
    bool 		isscaning;        /* have send out scan, waiting scan ack */
    bool        isinshedule;     /* alowed for scan, waiting for time shedule in voice */
} MP_ScanInfo_t;

typedef struct
{
    uint8		join_idx;
    MP_cell_info_t	  CellInfo[MP_MAC_MAXSCAN_RESULTS];
} MP_CellCfg_info_t;

typedef struct
{
    uint8 scan_seqnum;
    uint8 join_seqnum;
    uint16 dialup_seqnum;
    uint16 poll_seqnum;
} MP_SeqNums_t;

/* the same as defination in jn5148/config.h */
#ifdef MENU_OAD_UPDATE
typedef enum
{
    DEVICE_TYPE_CARD = 1,
    DEVICE_TYPE_LOCARTOR = 2,
    DEVICE_TYPE_STATION = 3,
} tof_device_type_te;

typedef enum
{
    TOF_LOCATOR_REQUEST = 0x01, //locator request to join a station
    TOF_LOCATOR_FINISH,
    TOF_LOCATOR_CARDS_DIST,

    TOF_CARD_CHECKIN,           // card send regist msg // = 4
    TOF_CARD_RSSI,              // card send RSSI msg
    TOF_CARD_REQUEST,           // card request tof to specify station
    TOF_CARD_ALARM,             // card alarm

    TOF_STATION_AVAILABLE,  // station is available, for locaotr or card to join // = 8
    TOF_STATION_ILDE,           // station has idle slot, card need wait for AVAILABLE cmd to join request
    TOF_STATION_RSSI,           // station need card to send rssi info
    TOF_STATION_BUSY,           // station has not idle slot, card need wait for RSSI cmd to send rssi // = B
    TOF_STATION_ACCEPT,     // station accepts card or locator // = C
    TOF_STATION_FINISH,     //station tof finished // = D
    TOF_STATION_HELP_ACK,   // the station receive help from cards // = E
    TOF_STATION_NOPWD_ACK,  // the station receive nopwd from cards // = F
    TOF_STATION_CHECKIN_ACK,    // the station receive card's regist info // = 0x10

    TOF_STATION_CARDS_INFO, // station send cards' info to locator, to indicate which cards need locating // = 0x11

    TOF_APP_HELP_ACK,           // the station receive help ack from app, need let card know // = 0x12
    TOF_STATION_WAIT,       // Station ask card to wait

    TOF_CARD_OAD = 0xF0,        // card's OAD controller send this msg to card when card do "checkin" event
    TOF_LOCATOR_OAD = 0xF2, // locator's OAD controller send this msg to locator
    TOF_LOC_STATION_OAD = 0xF4, // station's OAD controller send this msg to station
    TOF_RSSI_STATION_OAD = 0xF5,    // station's OAD controller send this msg to station
    TOF_CHECKIN_STATION_OAD = 0xF6, // station's OAD controller send this msg to station
    TOF_COM_STATION_OAD = 0xF7, // station's OAD controller send this msg to station
} rf_tof_msg_te;

typedef struct
{
    app_header_t tof_head;

    uint16 u16Version;  // software version
    uint8 u8DeviceType; // device type
    uint8 u8Reserved;
} rf_tof_oad_data_ts;

typedef struct
{
    uint16  panid;
    uint16  shortAddr;
    uint16  version;
    uint8    msgidx;
    uint8    msgtype;
    uint16   sendcnt;
} MP_OadUpdate_t;
#endif
/*************************************************************************************************
*MACROS
*/
#define ON_AUDIO() MP_JudgeStatus(WORK_STATUS_AUDIO)
#define IS_IDLE() MP_JudgeStatus(WORK_STATUS_IDLE)
#define ON_CALLING() MP_JudgeStatus(WORK_STATUS_CALLING)
#define ON_CALLINGWAIT() MP_JudgeStatus(WORK_STATUS_CALLINGWAIT)
#define ON_CALLED() MP_JudgeStatus(WORK_STATUS_CALLED)
#define ON_SM_SENDING() MP_JudgeStatus(WORK_STATUS_SM_SENDING)
#define ON_WAKE() MP_JudgeStatus(WORK_STATUS_WAKE)
#define ON_FOUND() MP_JudgeStatus(WORK_STATUS_FOUND)

#define SET_ON_AUDIO() MP_SetStatus(WORK_STATUS_AUDIO)
#define SET_ON_CALLING() MP_SetStatus(WORK_STATUS_CALLING)
#define SET_ON_CALLINGWAIT() MP_SetStatus(WORK_STATUS_CALLINGWAIT)
#define SET_ON_CALLED() MP_SetStatus(WORK_STATUS_CALLED)
#define SET_ON_IDLE() st(MP_SetStatus(WORK_STATUS_IDLE);HAL_AlarmUnSet(MP_ALARM_WAKE);) //when set idle, unset the wake alarm
#define SET_ON_SM_SENDING() MP_SetStatus(WORK_STATUS_SM_SENDING)
#define SET_ON_WAKE() st(MP_SetStatus(WORK_STATUS_WAKE);HAL_AlarmSet(MP_ALARM_WAKE,10000);) // set max wake timeout 10s
#define SET_ON_FOUND() MP_SetStatus(WORK_STATUS_FOUND)

#define    RSSI_MIN   -81   //  (MAC_RADIO_RECEIVER_SENSITIVITY_DBM + MAC_SPEC_ED_MIN_DBM_ABOVE_RECEIVER_SENSITIVITY)
#define    RSSI_MAX   10   // MAC_RADIO_RECEIVER_SATURATION_DBM

#define CONV_LQI_TO_RSSI( lqi )  ((int8)((int16)lqi*((int16)RSSI_MAX - (int16)RSSI_MIN)/MAC_SPEC_ED_MAX + (int16)RSSI_MIN))

/*************************************************************************************************
* global variables
*/
/* common device info */

extern MP_DevInfo_t  MP_DevInfo;
extern MP_AudioInfo_t MP_AudioInfo;
extern MP_NwkInfo_t MP_NwkInfo;

/* scan info */
extern MP_ScanInfo_t      MP_ScanInfo;
extern MP_CellCfg_info_t MP_CellInfo;

/* seqnum management */
extern MP_SeqNums_t MP_seqnums;
/* OAD update */
#ifdef MENU_OAD_UPDATE
extern MP_OadUpdate_t MP_OadUpdate;
#endif
/*********************************************************************
* FUNCTIONS
*/
bool MP_JudgeStatus(uint8 WorkStatus);
void MP_SetStatus(uint8 WorkStatus);
void MP_StartTalk(void);
void MP_EndTalk(void);
void MP_ResetAudio(void);
uint8 MP_LQI2Level(uint8 LQI);

uint8 MP_SendCmd(uint8  cmdtype, const app_termNbr_t *dstnmbr, uint16 seqnum);
uint8 MP_SendSignalToCoord(const uint8 *p, uint8 len, uint8 msgtype, bool retrans );
uint8 MP_SendSignalToAllCoord(const uint8 *p, uint8 len, uint8 msgtype, bool retrans );
uint8 MP_SendSignalToAllDev(const uint8 *p, uint8 len, uint8 msgtype, bool retrans );
void MP_start_timerEx( uint8 taskID, uint16 event_id, uint16 timeout_value );
void MP_set_event(uint8 task_id, uint16 event_flag);

bool MP_IsNwkOn(void);
void MP_ClearCellInfo(void);
bool MP_IsCellInfoEmpty(void);
void MP_UpdateCellInfo(void);
void MP_SortCellInfo(void);
void MP_VoiceScanShedule(void);
void MP_SetPeerNum(const app_termNbr_t *pnbr);//const app_mpCmd_t *pCmd)
void MP_StopSignalRetrys(void);

#endif

