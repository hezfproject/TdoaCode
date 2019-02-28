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
#include "AppProtocol.h"
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


    /* common */
    uint8    Channel;
    uint16   CoordPanID;

    /* panID I want to associate, if 0xFFFF, all pan ID is allowed */
    uint16    DesireCoordPanID;

    sAddrExt_t ExitAddr;

    /* this two is the same, one in uint16 and one in string */
    uint16       	nmbr;
    termNbr_t	termNbr;

    /* poll params*/
    bool    hascoordlink;
    int8     currentRssi;
} MP_DevInfo_t;

typedef struct
{
    uint16               peernmbr;          /* peer numbers */
    termNbr_t  peer_termnbr;
    uint16              seqnum;            // the last voice  seqnum
    uint16              cmdseqnum;      /* if calling cmd_seqnum=dial_seqnum, if called, cmd_seqnum=peer dial_seqnum */
    uint32              peer_tick;          /* the timer tick of peers last voice packet */

    /* signal retry counts */
    uint8               dialup_cnt;
    uint8               accept_cnt;
    uint8               close_cnt;

    uint8                retrying_bitmap;
    bool   		      IsTalkWithGateWay;	
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
/*********************************************************************
* FUNCTIONS
*/
bool MP_JudgeStatus(uint8 WorkStatus);
void MP_SetStatus(uint8 WorkStatus);
void MP_StartTalk(void);
void MP_EndTalk(void);
void MP_ResetAudio(void);
uint8 MP_Rssi2Level(int8 rssi);

uint8 MP_SendCmd(uint8  cmdtype, const termNbr_t *dstnmbr, uint16 seqnum);
uint8 MP_SendSignalToCoord(uint8 *p, uint8 len, bool retrans );
void MP_start_timerEx( uint8 taskID, uint16 event_id, uint16 timeout_value );
void MP_set_event(uint8 task_id, uint16 event_flag);

bool MP_IsNwkOn(void);
void MP_ClearCellInfo(void);
bool MP_IsCellInfoEmpty(void);
void MP_UpdateCellInfo(void);
void MP_SortCellInfo(void);
void MP_VoiceScanShedule(void);
void MP_SetPeerNum(const termNbr_t *pnbr);//const app_mpCmd_t *pCmd)
void MP_StopSignalRetrys(void);
bool MP_termNmbrCheck(termNbr_t *ptermNbr);

#endif

