
/**************************************************************************************************
 *                                           Includes
 **************************************************************************************************/

/* Hal Driver includes */
#include "hal_types.h"
//#include "hal_key.h"
#include "hal_timer.h"
#include "hal_drivers.h"
#include "hal_led.h"
#include "hal_adc.h"
#include "hal_lcd.h"
#ifndef LOCSYS_NODEROLE_STATION
#include "hal_flash.h"
#endif

/* OS includes */
#include "OSAL.h"
#include "OSAL_Tasks.h"
#include "OSAL_PwrMgr.h"

/* Application Includes */
#include "OnBoard.h"

/* MAC Application Interface */
#include "mac_api.h"
#include "mac_main.h"

#include "LocSys_Nwk.h"

/* Application */
#include "LocSys.h"

#include "LocSys_Util.h"

/* utils */
#include "macutil.h"
#include "App_cfg.h"
#include "delay.h"
#include "OSAL_Nv.h"
#include "ZComdef.h"
#ifdef WATCHDOG
#include "WatchdogUtil.h"
#endif

/* check node role definition */
#if !defined(LOCSYS_NODEROLE_STATION)   && \
    !defined(LOCSYS_NODEROLE_ROUTER)    && \
    !defined(LOCSYS_NODEROLE_CARD)      && \
    !defined(LOCSYS_NODEROLE_HANDHELD)
#error "no noderole defined!"
#endif

#if (defined(LOCSYS_NODEROLE_STATION) && defined(LOCSYS_NODEROLE_ROUTER  )) || \
    (defined(LOCSYS_NODEROLE_STATION) && defined(LOCSYS_NODEROLE_CARD    )) || \
    (defined(LOCSYS_NODEROLE_STATION) && defined(LOCSYS_NODEROLE_HANDHELD)) || \
    (defined(LOCSYS_NODEROLE_ROUTER ) && defined(LOCSYS_NODEROLE_CARD    )) || \
    (defined(LOCSYS_NODEROLE_ROUTER ) && defined(LOCSYS_NODEROLE_HANDHELD)) || \
    (defined(LOCSYS_NODEROLE_CARD   ) && defined(LOCSYS_NODEROLE_HANDHELD))
#error "multiple noderole defined!"
#endif

#if defined(LOCSYS_NODEROLE_STATION)
#define NWK_STATION     TRUE
#define NWK_ROUTER      FALSE
#define NWK_CARD        FALSE
#define NWK_HANDHELD    FALSE
#elif defined(LOCSYS_NODEROLE_ROUTER)
#define NWK_STATION     FALSE
#define NWK_ROUTER      TRUE
#define NWK_CARD        FALSE
#define NWK_HANDHELD    FALSE
#elif defined(LOCSYS_NODEROLE_CARD)
#define NWK_STATION     FALSE
#define NWK_ROUTER      FALSE
#define NWK_CARD        TRUE
#define NWK_HANDHELD    FALSE
#elif defined(LOCSYS_NODEROLE_HANDHELD)
#define NWK_STATION     FALSE
#define NWK_ROUTER      FALSE
#define NWK_CARD        FALSE
#define NWK_HANDHELD    TRUE
#endif

uint32 current;
uint32 current320us;

#define NWK_INCREASE_BASETIME(c) do{while(c - SyncInfo.BaseTime >= NWK_WINDOW_INTERVAL) SyncInfo.BaseTime += NWK_WINDOW_INTERVAL;}while(0)

#ifdef SLEEPTIME_ACCOUNTING
uint32 sleep32kticks = 0;
uint16 sleeps = 0;
uint32 starttime = 0;
uint32 sleeptimebase = 0;
#endif
/**************************************************************************************************
 *                                           Constant
 **************************************************************************************************/
#define NWK_LED_RXON                    HAL_LED_1

#define NWK_PANID                       0xAAAA
#define NWK_HANDLE_RESERVED             0xFF

#define NWK_PROFILER_ADDR               0xFFF1
#define NWK_PROFILER_PERIOD             30000u

/* Size table for MAC structures */
const CODE uint8 Nwk_cbackSizeTable [] =
{
  0,                                   /* unused */
  sizeof(macMlmeAssociateInd_t),       /* MAC_MLME_ASSOCIATE_IND */
  sizeof(macMlmeAssociateCnf_t),       /* MAC_MLME_ASSOCIATE_CNF */
  sizeof(macMlmeDisassociateInd_t),    /* MAC_MLME_DISASSOCIATE_IND */
  sizeof(macMlmeDisassociateCnf_t),    /* MAC_MLME_DISASSOCIATE_CNF */
  sizeof(macMlmeBeaconNotifyInd_t),    /* MAC_MLME_BEACON_NOTIFY_IND */
  sizeof(macMlmeOrphanInd_t),          /* MAC_MLME_ORPHAN_IND */
  sizeof(macMlmeScanCnf_t),            /* MAC_MLME_SCAN_CNF */
  sizeof(macMlmeStartCnf_t),           /* MAC_MLME_START_CNF */
  sizeof(macMlmeSyncLossInd_t),        /* MAC_MLME_SYNC_LOSS_IND */
  sizeof(macMlmePollCnf_t),            /* MAC_MLME_POLL_CNF */
  sizeof(macMlmeCommStatusInd_t),      /* MAC_MLME_COMM_STATUS_IND */
  sizeof(macMcpsDataCnf_t),            /* MAC_MCPS_DATA_CNF */
  sizeof(macMcpsDataInd_t),            /* MAC_MCPS_DATA_IND */
  sizeof(macMcpsPurgeCnf_t),           /* MAC_MCPS_PURGE_CNF */
  sizeof(macEventHdr_t)                /* MAC_PWR_ON_CNF */
};

/**************************************************************************************************
 *                                        Local Variables
 **************************************************************************************************/


#define LOCSYS_RSSI_THRESHOLD           -85
const CODE uint8 rssi2cost[] = {
  255u, 233u, 212u, 192u, 175u, 159u, 145u, 131u,
  119u, 109u, 99u,  90u,  82u,  74u,  67u,  61u,
  56u,  51u,  46u,  42u,  38u,  35u,  31u,  29u,
  26u,  24u,  21u,  20u,  18u,  16u,  15u,  13u,
  12u,  11u,  10u,  9u,   8u,   8u,   7u,   6u,
  6u,   5u,   5u,   4u,   4u,   4u,   3u,   3u,
  3u,   2u,   2u,   2u,   2u,   2u,   1u,   1u
};
#define LOCSYS_RSSI_TABLE_SIZE (sizeof(rssi2cost)/sizeof(uint8))

enum {
  DEV_CARD,
  DEV_ROUTER,
  DEV_STATION,
};

typedef struct{
  uint16        PanId;
  uint8         LogicalChannel;
  uint8         BeaconOrder;
  uint8         SupperFrameOrder;
  uint16        ShortAddr;
  sAddrExt_t    ExtAddr;
  sAddrExt_t    DestExtAddr;
  uint8         state;
  uint8         DataHandle;
  uint32        DataSendTime;
}DevInfo_t;

static DevInfo_t Nwk_devInfo;

static uint8 Nwk_dsn = 0;
#if (NWK_STATION)
static uint8 Nwk_broadcast_dsn = 0;
#endif

/* TRUE and FALSE value */
bool Nwk_MACTrue  = TRUE;
bool Nwk_MACFalse = FALSE;

/* Rx on bitmap */
#define NWK_RXON_ID_NORMAL                  0
#define NWK_RXON_ID_WAIT_BEACON             1
#define NWK_RXON_ID_STATION                 2
#define NWK_RXON_ID_CARD                    3
#define NWK_RXON_ID_APP                     4
static uint16 Rxon_bitmap = 0;

/* Task ID */
uint8 Nwk_TaskId;

/* Event IDs */
#define NWK_EVENT_TICK_UPDATE               0x0001
#define NWK_EVENT_WINDOW_OPEN               0x0002
#define NWK_EVENT_WINDOW_CLOSE              0x0004
#define NWK_EVENT_UPDATE_DEPTH              0x0008
#define NWK_EVENT_FEED_WATCHDOG             0x0010
#define NWK_EVENT_RELAY_BEACON              0x0040
#define NWK_EVENT_WAIT_BEACON_TIMEOUT       0x0080


#define NWK_EVENT_PROFILER                  0x2000
#define NWK_EVENT_DUMMY                     0x4000

/* MSG handle */
#define NWK_MSG_DEVICESTART                 0x80

/**************************************************************************************************
 *                                     Local Function Prototypes
 **************************************************************************************************/
/* Setup routines */
void Nwk_DeviceStartup(void);

uint8 Nwk_McpsDataReq(uint8* data, uint8 dataLength, uint8 srcAddrMode, uint16 dstShortAddr, uint8 txOpt);


typedef struct {
  uint16 nbAddr;
  uint8 dsn;
  uint32 recvtime;
}timesync_entry_t;

typedef struct {
  uint8 dsn;
  uint8 sendcount;
  uint8 available;
  uint32 recvtime;
  uint8* buf;
}trans_broadcast_entry_t;

#define MAX_TIMESYNC_ENTRIES                8
#define MAX_TREE_DEPTH                      20
#define MAX_TRANS_DATA_ENTRIES              8
#define MAX_TRANS_BROADCAST_ENTRIES         (1u << 3)
#define TRANS_BROADCAST_ENTRY_MASK          (MAX_TRANS_BROADCAST_ENTRIES - 1)
#define MAX_FAIL_TO_PARENT_COUNT            10

typedef struct {
  uint16    address;
  uint8     depth;
  uint8     cost;
  uint32    updatedepthtime;
  uint16    accucost;
  uint16    parent;
  uint8     failcount;
  uint32    successtime;
  uint8     ticks;
  uint8     windowopen;
  uint16    pathtoroot[MAX_TREE_DEPTH];
  timesync_entry_t          TmSyncTbl[MAX_TIMESYNC_ENTRIES];
  trans_broadcast_entry_t   TransTbl[MAX_TRANS_BROADCAST_ENTRIES];
}nwk_info_t;

static nwk_info_t NwkInfo;


typedef struct {
  uint8     beacon;
  uint8     beacon_req;
  uint8     update_depth;
  uint8     check_parent;
  uint8     data[MAX_TRANS_DATA_ENTRIES];
  uint8     broadcast[MAX_TRANS_BROADCAST_ENTRIES];
}handle_set_t;
handle_set_t Nwk_HdlSet;


/* in miliseconds */
#define NWK_RELAY_BEACON_DELAY_MASK         0x3F
#define NWK_TRANS_BRDCAST_DURATION          1000u
#define NWK_DUMMY_EVENT_PERIOD              10000u
#define NWK_WINDOW_ERROR                    20u
#define NWK_TICK_PERIOD                     1000ul
#define NWK_CARD_SEND_PERIOD_TICK           20u
#define NWK_WINDOW_DURATION_TICK            6u
#define NWK_WINDOW_INTERVAL_TICK            (NWK_CARD_SEND_PERIOD_TICK * 3 + NWK_WINDOW_DURATION_TICK - 2)
#define NWK_WINDOW_INTERVAL                 (NWK_WINDOW_INTERVAL_TICK * NWK_TICK_PERIOD)
#define NWK_WINDOW_DURATION                 (NWK_WINDOW_DURATION_TICK * NWK_TICK_PERIOD)
#define NWK_BEACON_REQ_PERIOD               (NWK_WINDOW_DURATION / 2)
#define NWK_MAX_SYNC_LOST_TIME              (NWK_WINDOW_INTERVAL * 4)
#define NWK_WAIT_BEACON_DURATION            50u
#define NWK_FEED_WATCHDOG_PERIOD            300u
#define NWK_MAX_NO_DATA_SEND_DURATION       (NWK_WINDOW_INTERVAL * 5)

typedef struct{
  bool      SyncLost;
  uint32    BaseTime;
  uint32    SyncTime;
  uint32    BaseTime320us;

  uint8     nsend;
  uint16    dstAddr;
}time_sync_info_t;
time_sync_info_t SyncInfo;

/*frame type*/
enum{
  NWK_FRM_DATA,
  NWK_FRM_ACK,
  NWK_FRM_BEACON,
  NWK_FRM_BCNREQ,
  NWK_FRM_DEPTH,
  NWK_FRM_CHECKPARENT,
  NWK_FRM_DEBUG_SLEEP	= 7,
};

typedef struct{
  nwk_frm_hdr_t hdr;
}nwk_ack_t;


typedef struct{
  uint8         dsn;
  uint16        adjust;
}adjust_info_t;

typedef struct{
  nwk_frm_hdr_t hdr;
  uint8         adjcnt;
  adjust_info_t adjinfo[3];
}nwk_beacon_t;
nwk_beacon_t Nwk_beacon;

typedef struct{
  nwk_frm_hdr_t hdr;
}nwk_beacon_req_t;

typedef struct{
  nwk_frm_hdr_t hdr;
  uint8         depth;
  uint16        accucost;
  uint16        pathtoroot[0];
}nwk_depth_t;

typedef struct{
  nwk_frm_hdr_t hdr;
}nwk_checkparent_t;

typedef struct{
  nwk_frm_hdr_t hdr;
  uint16		sleepcount;
  uint16		sleeprate;
}nwk_debugsleep_t;

void Nwk_ProcessDataInd(macCbackEvent_t *pData);
void Nwk_ProcessDataCnf(macCbackEvent_t *pData);

void Nwk_TurnOnRx(uint8 Id);
void Nwk_TurnOffRx(uint8 Id);

void Nwk_UpdateDepth(uint16 dstAddr);
void Nwk_ProcessUpdateDepthFrm(nwk_depth_t *frm, uint8 cost);
void Nwk_SendBeacon(uint16 dstAddr);
void Nwk_ProcessBeaconFrm(nwk_beacon_t *beacon, uint32 timestamp);
void Nwk_SendBeaconReq(void);
void Nwk_ProcessBeaconReq(nwk_beacon_req_t * bcnreq);
void Nwk_ProcessDataFrm(macCbackEvent_t* pData);
void Nwk_Reset(void);
void NwkInfo_Init(void);
void Nwk_CheckParent(void);

void Nwk_TaskInit(uint8 taskId)
{
  /* Initialize the task id */
  Nwk_TaskId = taskId;

#if(defined WATCHDOG) && (WATCHDOG==TRUE)
  StartWatchDog(DOGTIMER_INTERVAL_1S);
#endif

  /* initialize MAC features */
  MAC_InitDevice();
  MAC_InitCoord();

  /* Reset the MAC */
  MAC_MlmeResetReq(TRUE);

  osal_pwrmgr_device(PWRMGR_BATTERY);
  osal_pwrmgr_task_state(taskId, PWRMGR_CONSERVE);

  osal_event_hdr_t *event = (osal_event_hdr_t*)osal_msg_allocate(sizeof(osal_event_hdr_t));
  if(event){
    event->event = NWK_MSG_DEVICESTART;
    event->status = SUCCESS;
    osal_msg_send(Nwk_TaskId, (uint8*)event);
  }else{
    SystemReset();
  }

  #ifdef SLEEPTIME_ACCOUNTING
  Util_start_reload_timer(Nwk_TaskId, NWK_EVENT_PROFILER, NWK_PROFILER_PERIOD);
  #endif
}

#if (NWK_ROUTER)
extern void Nwk_RestartCB(void);
#endif

uint16 Nwk_ProcessEvent(uint8 taskId, uint16 events)
{  
  current = osal_GetSystemClock();
  current320us = Util_GetTime320us();

  if(events & NWK_EVENT_DUMMY){
    #if (!NWK_HANDHELD)
    //if no data to send such long time, there must be something abnormal
    if(current - Nwk_devInfo.DataSendTime > NWK_MAX_NO_DATA_SEND_DURATION){
      SystemReset();
    }
    #endif

    #if (NWK_ROUTER)
    Nwk_RestartCB();
    #endif

    return events ^ NWK_EVENT_DUMMY;
  }

#if(defined WATCHDOG) && (WATCHDOG==TRUE)
  if(events & NWK_EVENT_FEED_WATCHDOG){
    FeedWatchDog();
    return events ^ NWK_EVENT_FEED_WATCHDOG;
  }
#endif

  if (events & SYS_EVENT_MSG)
  {
    uint8* pMsg;
    macCbackEvent_t* pData;

    while ((pMsg = osal_msg_receive(Nwk_TaskId)) != NULL)
    {
      switch ( *pMsg )
      {
        case NWK_MSG_DEVICESTART:{
          Nwk_StateChangeCB(DEV_STATE_HOLD);
          break;
        }
        case MAC_MLME_START_CNF:
          pData = (macCbackEvent_t *) pMsg;

          if(pData->startCnf.hdr.status == MAC_SUCCESS){
            Nwk_StateChangeCB(DEV_STATE_START);
          }else{
            Nwk_StateChangeCB(DEV_STATE_STARTFAIL);
          }
          break;
      
        case MAC_MCPS_DATA_CNF:
          pData = (macCbackEvent_t *) pMsg;

          Nwk_ProcessDataCnf(pData);

          mac_msg_deallocate((uint8**)&pData->dataCnf.pDataReq);
          break;

        case MAC_MCPS_DATA_IND:
          pData = (macCbackEvent_t*)pMsg;

          /* ignore broadcast data from other PAN */
          if(pData->dataInd.mac.dstPanId != Nwk_devInfo.PanId)
            break;

          Nwk_ProcessDataInd(pData);
          
          break;
      }

      /* Deallocate */
      if(*pMsg == NWK_MSG_DEVICESTART)
        osal_msg_deallocate(pMsg);
      else
        mac_msg_deallocate((uint8**)&pMsg);
    }

    return events ^ SYS_EVENT_MSG;
  }

  if (events & NWK_EVENT_TICK_UPDATE) {
    
    #if (NWK_STATION)
    if(SyncInfo.SyncLost){
      SyncInfo.SyncLost = FALSE;
      SyncInfo.BaseTime = current;
    }
    SyncInfo.SyncTime = current;

    NWK_INCREASE_BASETIME(current);

    //snap to the nearest tick
    uint8 tick = (current - SyncInfo.BaseTime + NWK_TICK_PERIOD / 2) / NWK_TICK_PERIOD;

    if(tick & 0x01){//send beacon per dual seconds on odd ticks
      Nwk_SendBeacon(NWK_BROADCAST_ADDR);
    }
    if(tick == 0){
      Nwk_StateChangeCB(DEV_STATE_ACTIVE);
      Util_start_reload_timer(Nwk_TaskId, NWK_EVENT_UPDATE_DEPTH, NWK_TICK_PERIOD + (osal_rand() & 0xFF));
    }
    if(tick == NWK_WINDOW_DURATION_TICK - 1) {
      Nwk_StateChangeCB(DEV_STATE_INACTIVE);
    }
    if(tick == NWK_WINDOW_DURATION_TICK){
      Util_stop_timer(Nwk_TaskId, NWK_EVENT_UPDATE_DEPTH);
    }
    tick++;
    uint16 timeout = SyncInfo.BaseTime + tick * NWK_TICK_PERIOD - current;
    Util_start_timerEx(Nwk_TaskId, NWK_EVENT_TICK_UPDATE, timeout);
    #endif

    #if (NWK_ROUTER)
    if(!SyncInfo.SyncLost && current - SyncInfo.SyncTime > NWK_MAX_SYNC_LOST_TIME){
      SyncInfo.SyncLost = TRUE;
      Nwk_StateChangeCB(DEV_STATE_INACTIVE);
    }

    if(SyncInfo.SyncLost){
      Util_start_timerEx(Nwk_TaskId, NWK_EVENT_TICK_UPDATE, NWK_BEACON_REQ_PERIOD);

      //send beacon req, turn on rx to recv beacon and set timeout
      Nwk_TurnOnRx(NWK_RXON_ID_WAIT_BEACON);
      Util_start_timerEx(Nwk_TaskId, NWK_EVENT_WAIT_BEACON_TIMEOUT, NWK_WAIT_BEACON_DURATION);
      Nwk_SendBeaconReq();
    }else if(NwkInfo.windowopen){
      Util_start_timerEx(Nwk_TaskId, NWK_EVENT_TICK_UPDATE, NWK_TICK_PERIOD);

      if(NwkInfo.ticks == 0){
        Nwk_StateChangeCB(DEV_STATE_ACTIVE);
      }else if(NwkInfo.ticks == NWK_WINDOW_DURATION_TICK - 1){
        Nwk_StateChangeCB(DEV_STATE_INACTIVE);
      }
      //no routing to parent
      if(NwkInfo.parent == NWK_INVALID_ADDRESS){
        //to scan parent
        NwkInfo.depth = NWK_INVALID_DEPTH;
        Nwk_UpdateDepth(NWK_BROADCAST_ADDR);
      }else{
        if(current - NwkInfo.successtime > NWK_TICK_PERIOD){
          Nwk_CheckParent();
        }
      }
      NwkInfo.ticks++;
    }
    #endif
    
    return events ^ NWK_EVENT_TICK_UPDATE;
  }

  if (events & NWK_EVENT_WAIT_BEACON_TIMEOUT) {
    Nwk_TurnOffRx(NWK_RXON_ID_WAIT_BEACON);
    return events ^ NWK_EVENT_WAIT_BEACON_TIMEOUT;
  }

  if (events & NWK_EVENT_RELAY_BEACON) {
    Nwk_SendBeacon(NWK_BROADCAST_ADDR);
    return events ^ NWK_EVENT_RELAY_BEACON;
  }

  if (events & NWK_EVENT_WINDOW_OPEN) {
    if(!SyncInfo.SyncLost){
      Nwk_TurnOnRx(NWK_RXON_ID_NORMAL);
      NwkInfo.windowopen = TRUE;
      NwkInfo.ticks = 0;

      Util_start_timerEx(Nwk_TaskId, NWK_EVENT_TICK_UPDATE, NWK_WINDOW_ERROR);

      Util_start_timerEx(Nwk_TaskId, NWK_EVENT_WINDOW_CLOSE, NWK_WINDOW_DURATION);
    }
    return events ^ NWK_EVENT_WINDOW_OPEN;
  }

  if (events & NWK_EVENT_WINDOW_CLOSE) {
    Nwk_TurnOffRx(NWK_RXON_ID_NORMAL);
    NwkInfo.windowopen = FALSE;

    if(!SyncInfo.SyncLost){
      Util_stop_timer(Nwk_TaskId, NWK_EVENT_TICK_UPDATE);

      NWK_INCREASE_BASETIME(current);
      Util_start_timerEx(Nwk_TaskId, NWK_EVENT_WINDOW_OPEN, SyncInfo.BaseTime + NWK_WINDOW_INTERVAL - NWK_WINDOW_ERROR - current);
    }
    return events ^ NWK_EVENT_WINDOW_CLOSE;
  }

  #if (NWK_ROUTER || NWK_STATION)
  if (events & NWK_EVENT_UPDATE_DEPTH) {
    Nwk_UpdateDepth(NWK_BROADCAST_ADDR);
    return events ^ NWK_EVENT_UPDATE_DEPTH;
  }
  #endif

  #ifdef SLEEPTIME_ACCOUNTING
  if (events & NWK_EVENT_PROFILER) {
    starttime += (sleep32kticks >> 15) * 1000;
    sleep32kticks &= 0x7FFF;
    uint32 sleeptime = starttime + (sleep32kticks * 125 + 4095) / 4096;

    nwk_debugsleep_t dbgslp;
	osal_memset(&dbgslp.hdr, 0, 2);
	dbgslp.hdr.type		= NWK_FRM_DEBUG_SLEEP;
    dbgslp.sleeprate 	= (uint16)((1.0 - (float)sleeptime / (current - sleeptimebase)) * 10000);
    dbgslp.sleepcount	= sleeps;
    Nwk_McpsDataReq((uint8*)&dbgslp, sizeof(dbgslp), SADDR_MODE_SHORT, NWK_PROFILER_ADDR, 0);
    return events ^ NWK_EVENT_PROFILER;
  }
  #endif

  return 0;
}

void MAC_CbackEvent(macCbackEvent_t *pData)
{
  macCbackEvent_t *pMsg = NULL;

  uint8 len = Nwk_cbackSizeTable[pData->hdr.event];

  switch (pData->hdr.event)
  {
    case MAC_MLME_BEACON_NOTIFY_IND:

      len += sizeof(macPanDesc_t) + pData->beaconNotifyInd.sduLength +
             MAC_PEND_FIELDS_LEN(pData->beaconNotifyInd.pendAddrSpec);
      if ((pMsg = (macCbackEvent_t *) osal_msg_allocate(len)) != NULL)
      {
        /* Copy data over and pass them up */
        osal_memcpy(pMsg, pData, sizeof(macMlmeBeaconNotifyInd_t));
        pMsg->beaconNotifyInd.pPanDesc = (macPanDesc_t *) ((uint8 *) pMsg + sizeof(macMlmeBeaconNotifyInd_t));
        osal_memcpy(pMsg->beaconNotifyInd.pPanDesc, pData->beaconNotifyInd.pPanDesc, sizeof(macPanDesc_t));
        pMsg->beaconNotifyInd.pSdu = (uint8 *) (pMsg->beaconNotifyInd.pPanDesc + 1);
        osal_memcpy(pMsg->beaconNotifyInd.pSdu, pData->beaconNotifyInd.pSdu, pData->beaconNotifyInd.sduLength);
      } else {
        SystemReset();
      }
      break;

    case MAC_MCPS_DATA_IND:
      pMsg = pData;
      break;

    default:
      if ((pMsg = (macCbackEvent_t *) osal_msg_allocate(len)) != NULL)
      {
        osal_memcpy(pMsg, pData, len);
      } else {
        SystemReset();
      }
      break;
  }

  if (pMsg != NULL)
  {
    osal_msg_send(Nwk_TaskId, (uint8*)pMsg);
  } 
}

uint8 MAC_CbackCheckPending(void)
{
  return (0);
}

void Nwk_DeviceStartup()
{
  MAC_MlmeResetReq(TRUE);

  MAC_MlmeSetReq(MAC_EXTENDED_ADDRESS, &Nwk_devInfo.ExtAddr);

  MAC_MlmeSetReq(MAC_SHORT_ADDRESS, &Nwk_devInfo.ShortAddr);

  MAC_MlmeSetReq(MAC_PAN_ID, &Nwk_devInfo.PanId);

  MAC_MlmeSetReq(MAC_LOGICAL_CHANNEL, &Nwk_devInfo.LogicalChannel);

  MAC_MlmeSetReq(MAC_BEACON_ORDER, &Nwk_devInfo.BeaconOrder);

  MAC_MlmeSetReq(MAC_SUPERFRAME_ORDER, &Nwk_devInfo.SupperFrameOrder);

  MAC_MlmeSetReq(MAC_ASSOCIATED_PAN_COORD, &Nwk_MACTrue);

  MAC_MlmeSetReq(MAC_RX_ON_WHEN_IDLE, &Nwk_MACFalse);

  #if (NWK_STATION)
  uint8 tmp8 = 0;
  MAC_MlmeSetReq(MAC_BEACON_PAYLOAD_LENGTH, &tmp8);

  MAC_MlmeSetReq(MAC_ASSOCIATION_PERMIT, &Nwk_MACFalse);
  
  macMlmeStartReq_t   startReq;

  startReq.startTime = 0;
  startReq.panId = Nwk_devInfo.PanId;
  startReq.logicalChannel = Nwk_devInfo.LogicalChannel;
  startReq.beaconOrder = 0xF;
  startReq.superframeOrder = 0xF;
  startReq.panCoordinator = TRUE;
  startReq.batteryLifeExt = FALSE;
  startReq.coordRealignment = FALSE;
  startReq.realignSec.securityLevel = FALSE;
  startReq.beaconSec.securityLevel = FALSE;

  MAC_MlmeStartReq(&startReq);
  #endif
}

uint8 Nwk_McpsDataReq(uint8* data, uint8 dataLength, uint8 srcAddrMode, uint16 dstShortAddr, uint8 txOpt)
{
  macMcpsDataReq_t  *pData;

  if ((pData = MAC_McpsDataAlloc(dataLength, MAC_SEC_LEVEL_NONE, MAC_KEY_ID_MODE_NONE)) != NULL)
  {
    pData->mac.srcAddrMode = srcAddrMode;
    if(dstShortAddr == MAC_ADDR_USE_EXT){
      pData->mac.dstAddr.addrMode = SADDR_MODE_EXT;
      sAddrExtCpy(pData->mac.dstAddr.addr.extAddr, Nwk_devInfo.DestExtAddr);
    }else{
      pData->mac.dstAddr.addrMode = SADDR_MODE_SHORT;
      pData->mac.dstAddr.addr.shortAddr = dstShortAddr;
    }
    pData->mac.dstPanId = Nwk_devInfo.PanId;
    pData->mac.msduHandle = Nwk_devInfo.DataHandle++;
    if(Nwk_devInfo.DataHandle == NWK_HANDLE_RESERVED) Nwk_devInfo.DataHandle++;
    if(dstShortAddr == MAC_SHORT_ADDR_BROADCAST) txOpt &= ~MAC_TXOPTION_ACK;
    pData->mac.txOptions = txOpt;

    osal_memcpy (pData->msdu.p, data, dataLength);

    MAC_McpsDataReq(pData);
    Nwk_devInfo.DataSendTime = current;
    return NWK_SUCCESS;
  } else {
    return NWK_NO_MEMORY;
  }
}

void Nwk_ProcessDataInd(macCbackEvent_t* pData)
{
  uint8 *p  = pData->dataInd.msdu.p;
  uint8 len = pData->dataInd.msdu.len;
  nwk_frm_hdr_t *hdr    = (nwk_frm_hdr_t*)p;
  uint32 timestamp      = pData->dataInd.mac.timestamp;
  
  if(len != sizeof(nwk_frm_hdr_t) + hdr->len) return;
  
  switch(hdr->type){
    #if (NWK_ROUTER)
    case NWK_FRM_BEACON:{
      Nwk_ProcessBeaconFrm((nwk_beacon_t*)p, timestamp);
      break;
    }
    #endif
    
    #if (NWK_ROUTER || NWK_STATION)
    case NWK_FRM_DEPTH:{
      if(pData->dataInd.mac.rssi >= LOCSYS_RSSI_THRESHOLD){
        uint8 cost = pData->dataInd.mac.rssi - LOCSYS_RSSI_THRESHOLD >= LOCSYS_RSSI_TABLE_SIZE ? 1 : rssi2cost[pData->dataInd.mac.rssi - LOCSYS_RSSI_THRESHOLD];
        Nwk_ProcessUpdateDepthFrm((nwk_depth_t*)p, cost);
      }
      break;
    }
    #endif
    
    #if (NWK_ROUTER || NWK_STATION)
    case NWK_FRM_BCNREQ:{
      Nwk_ProcessBeaconReq((nwk_beacon_req_t*)p);
      break;
    }
    #endif
    
    case NWK_FRM_DATA:{
      Nwk_ProcessDataFrm(pData);
      break;
    }
        
    default:
      break;
  }
}

void Nwk_ProcessDataCnf(macCbackEvent_t *pData)
{
  uint8 handle      = pData->dataCnf.msduHandle;
  uint8 status      = pData->dataCnf.hdr.status;
  uint32 timestamp  = pData->dataCnf.timestamp;
  int8 rssi         = pData->dataCnf.rssi;
  if(handle == Nwk_HdlSet.beacon){
    Nwk_HdlSet.beacon = NWK_HANDLE_RESERVED;

    if(status == MAC_SUCCESS){
      if(SyncInfo.nsend-- == 0) return;
        
      Nwk_beacon.adjinfo[Nwk_beacon.adjcnt].dsn = Nwk_beacon.hdr.dsn;
      Nwk_beacon.adjinfo[Nwk_beacon.adjcnt].adjust = UTIL_320US_TO_MS(Util_Sub320us(timestamp, SyncInfo.BaseTime320us));
      Nwk_beacon.adjcnt++;
      
      Nwk_beacon.hdr.dsn = Nwk_dsn;

      Nwk_HdlSet.beacon = Nwk_devInfo.DataHandle;
      uint8 st = Nwk_McpsDataReq((uint8*)&Nwk_beacon, sizeof(nwk_beacon_t), SADDR_MODE_SHORT, SyncInfo.dstAddr, 0);
      if(st == NWK_SUCCESS){
        Nwk_dsn++;
      }else{
        Nwk_HdlSet.beacon = NWK_HANDLE_RESERVED;
      }
    } else {
      Nwk_HdlSet.beacon = Nwk_devInfo.DataHandle;
      uint8 st = Nwk_McpsDataReq((uint8*)&Nwk_beacon, sizeof(nwk_beacon_t), SADDR_MODE_SHORT, SyncInfo.dstAddr, 0);
      if(st != NWK_SUCCESS){
        Nwk_HdlSet.beacon = NWK_HANDLE_RESERVED;
      }
    }
    return;
  }

  for(uint8 i = 0; i < MAX_TRANS_BROADCAST_ENTRIES; ++i){
    if(handle == Nwk_HdlSet.broadcast[i]){
      Nwk_HdlSet.broadcast[i] = NWK_HANDLE_RESERVED;
      trans_broadcast_entry_t *tte = &NwkInfo.TransTbl[i];
      if(status == MAC_SUCCESS){
        if(tte->sendcount-- > 0){
          nwk_data_t *frm = (nwk_data_t*)tte->buf;
          Nwk_HdlSet.broadcast[i] = Nwk_devInfo.DataHandle;
          uint8 st = Nwk_McpsDataReq(tte->buf, sizeof(nwk_frm_hdr_t) + frm->hdr.len, SADDR_MODE_SHORT, MAC_SHORT_ADDR_BROADCAST, 0);
          if(st != NWK_SUCCESS){
            Nwk_HdlSet.broadcast[i] = NWK_HANDLE_RESERVED;
            osal_mem_free(tte->buf);
            tte->buf = NULL;
          }
        }else{
          osal_mem_free(tte->buf);
          tte->buf = NULL;
        }
      }else{
        nwk_data_t *frm = (nwk_data_t*)tte->buf;
        Nwk_HdlSet.broadcast[i] = Nwk_devInfo.DataHandle;
        uint8 st = Nwk_McpsDataReq(tte->buf, sizeof(nwk_frm_hdr_t) + frm->hdr.len, SADDR_MODE_SHORT, MAC_SHORT_ADDR_BROADCAST, 0);
        if(st != NWK_SUCCESS){
          Nwk_HdlSet.broadcast[i] = NWK_HANDLE_RESERVED;
          osal_mem_free(tte->buf);
          tte->buf = NULL;
        }
      }
      return;
    }
  }

  for(uint8 i = 0; i < MAX_TRANS_DATA_ENTRIES; ++i){
    if(handle == Nwk_HdlSet.data[i]){
      Nwk_HdlSet.data[i] = NWK_HANDLE_RESERVED;
      if(status == MAC_SUCCESS){
        NwkInfo.failcount = 0;
        NwkInfo.successtime = current;
      }else if(status == MAC_NO_ACK){
        NwkInfo.failcount++;
        if(NwkInfo.failcount >= MAX_FAIL_TO_PARENT_COUNT){
          NwkInfo.parent = NWK_INVALID_ADDRESS;
          NwkInfo.depth = NWK_INVALID_DEPTH;
          Nwk_UpdateDepth(NWK_BROADCAST_ADDR);
        }
      }
      return;
    }
  }

  if(handle == Nwk_HdlSet.check_parent){
    Nwk_HdlSet.check_parent = NWK_HANDLE_RESERVED;
    if(status == MAC_SUCCESS){
      NwkInfo.failcount = 0;
      NwkInfo.successtime = current;
    }else if(status == MAC_NO_ACK){
      NwkInfo.failcount++;
      if(NwkInfo.failcount >= MAX_FAIL_TO_PARENT_COUNT || current - NwkInfo.successtime > NWK_WINDOW_INTERVAL){
        NwkInfo.parent  = NWK_INVALID_ADDRESS;
        NwkInfo.depth   = NWK_INVALID_DEPTH;
        Nwk_UpdateDepth(NWK_BROADCAST_ADDR);
      }
    }

  }
  
  return;
}

void Nwk_TurnOnRx(uint8 Id)
{
  Rxon_bitmap |= (uint16)1 << Id;
  MAC_MlmeSetReq(MAC_RX_ON_WHEN_IDLE, &Nwk_MACTrue);
  Util_start_reload_timer(Nwk_TaskId, NWK_EVENT_FEED_WATCHDOG, NWK_FEED_WATCHDOG_PERIOD);
  #if (NWK_ROUTER)
  HalLedSet(NWK_LED_RXON, HAL_LED_MODE_ON);
  #endif
}

void Nwk_TurnOffRx(uint8 Id)
{
  Rxon_bitmap &= ~((uint16)1 << Id);
  if (Rxon_bitmap == 0){
    MAC_MlmeSetReq(MAC_RX_ON_WHEN_IDLE, &Nwk_MACFalse);
    Util_stop_timer(Nwk_TaskId, NWK_EVENT_FEED_WATCHDOG);
    #if (NWK_ROUTER)
    HalLedSet(NWK_LED_RXON, HAL_LED_MODE_OFF);
    #endif
  }
}

void Nwk_SetDeviceInfo(deviceInfo_t* info)
{
  #if (NWK_STATION)

  Nwk_devInfo.ShortAddr = NWK_STATION_ADDRESS;
  Nwk_devInfo.LogicalChannel = info->Channel;
  osal_memcpy((uint8*)Nwk_devInfo.ExtAddr, (uint8*)info->ExtAddr, 8);

  #else

  HalFlashRead(HAL_FLASH_IEEE_PAGE, HAL_FLASH_IEEE_OSET, (uint8 *)Nwk_devInfo.ExtAddr, HAL_FLASH_IEEE_SIZE);
  Nwk_devInfo.ShortAddr = BUILD_UINT16(Nwk_devInfo.ExtAddr[EXT_MACADDR_DEVID_LBYTE], Nwk_devInfo.ExtAddr[EXT_MACADDR_DEVID_HBYTE]);
  Nwk_devInfo.LogicalChannel = Nwk_devInfo.ExtAddr[EXT_MACADDR_CHANNEL];

  if(Nwk_devInfo.LogicalChannel < 11 || Nwk_devInfo.LogicalChannel > 26)
    Nwk_devInfo.LogicalChannel = 11;

  /* if wrong address */
  if(Nwk_devInfo.ShortAddr == 0 || Nwk_devInfo.ShortAddr == 0xFFFF){
    while(1){
      HalLedSet(HAL_LED_ALL, HAL_LED_MODE_ON);
      
      #if(defined WATCHDOG) && (WATCHDOG==TRUE)
      FeedWatchDog();
      #endif
      DelayMs(500);
      HalLedSet(HAL_LED_ALL, HAL_LED_MODE_OFF);
      #if(defined WATCHDOG) && (WATCHDOG==TRUE)
      FeedWatchDog();
      #endif
      DelayMs(500);
    }
  }
  
  #endif

  Nwk_devInfo.PanId             = NWK_PANID;
  Nwk_devInfo.BeaconOrder       = 15;
  Nwk_devInfo.SupperFrameOrder  = 15;
  Nwk_devInfo.DataHandle        = 0;
  Nwk_devInfo.DataSendTime      = osal_GetSystemClock();

}

void Nwk_Reset()
{
  HalLedSet(HAL_LED_ALL, HAL_LED_MODE_OFF);
  NwkInfo_Init();

  #if (NWK_STATION)
  SyncInfo.SyncLost = TRUE;

  Nwk_TurnOnRx(NWK_RXON_ID_STATION);
  #endif
  
  #if (NWK_ROUTER)
  SyncInfo.SyncLost = TRUE;
  #endif

  Nwk_StateChangeCB(DEV_STATE_RESET);

  /* 2530 card has a bug that the sleep time more than about 20sec will not wake up on time,
   * this is a workaround to avoid this */
  Util_start_reload_timer(Nwk_TaskId, NWK_EVENT_DUMMY, NWK_DUMMY_EVENT_PERIOD);

  #if (!NWK_CARD)
  Util_start_timerEx(Nwk_TaskId, NWK_EVENT_TICK_UPDATE, 100);
  #endif
}


void Nwk_UpdateDepth(uint16 dstAddr)
{
  uint16 size = 0;
  if(NwkInfo.depth != 0 && NwkInfo.depth != NWK_INVALID_DEPTH){
    size += sizeof(uint16) * NwkInfo.depth;
  }
  nwk_depth_t *frm = osal_mem_alloc(sizeof(nwk_depth_t) + size);
  if(frm){
    osal_memset(frm, 0, 2);
    frm->hdr.type     = NWK_FRM_DEPTH;
    frm->hdr.srcAddr  = Nwk_devInfo.ShortAddr;
    frm->hdr.dstAddr  = dstAddr;
    frm->hdr.dsn      = Nwk_dsn++;
    frm->hdr.len      = sizeof(nwk_depth_t) + size - sizeof(nwk_frm_hdr_t);
    frm->depth        = NwkInfo.depth;
    frm->accucost     = NwkInfo.accucost;
    if(size != 0){
      osal_memcpy((uint8*)frm->pathtoroot, (uint8*)NwkInfo.pathtoroot, size);
    }
    Nwk_HdlSet.update_depth = Nwk_devInfo.DataHandle;
    uint8 st = Nwk_McpsDataReq((uint8*)frm, sizeof(nwk_depth_t) + size, SADDR_MODE_SHORT, dstAddr, MAC_TXOPTION_ACK);
    if(st != NWK_SUCCESS){
      Nwk_HdlSet.update_depth = NWK_HANDLE_RESERVED;
    }
    osal_mem_free(frm);
  }
}

void Nwk_ProcessUpdateDepthFrm(nwk_depth_t *frm, uint8 cost)
{
  if(NwkInfo.parent == NWK_INVALID_ADDRESS){//lost connection
    if(frm->depth == NWK_INVALID_DEPTH){
      return;
    }else{
      for(uint8 i = frm->depth; i > 0; --i){
        if(Nwk_devInfo.ShortAddr == frm->pathtoroot[i - 1]){//from descendant
          return;
        }
      }
      //candidate parent
      NwkInfo.parent    = frm->hdr.srcAddr;
      NwkInfo.depth     = frm->depth + 1;
      NwkInfo.cost      = cost;
      NwkInfo.accucost  = frm->accucost + cost;
      if(frm->depth != 0) osal_memcpy((uint8*)&NwkInfo.pathtoroot[0], (uint8*)&frm->pathtoroot[0], frm->depth * sizeof(uint16));
      NwkInfo.pathtoroot[frm->depth] 	= NwkInfo.parent;
      NwkInfo.updatedepthtime			= current;
      osal_set_event(Nwk_TaskId, NWK_EVENT_UPDATE_DEPTH);

      Nwk_StateChangeCB(DEV_STATE_ACTIVE);
    }
  }else{//connected
    if(frm->depth == NWK_INVALID_DEPTH){//
      for(uint8 i = NwkInfo.depth; i > 0 ; --i){
        if(frm->hdr.srcAddr == NwkInfo.pathtoroot[i - 1]){//from ancestor
          NwkInfo.depth     = NWK_INVALID_DEPTH;
          NwkInfo.parent    = NWK_INVALID_ADDRESS;
          osal_set_event(Nwk_TaskId, NWK_EVENT_UPDATE_DEPTH);
          
          Nwk_StateChangeCB(DEV_STATE_INACTIVE);
          return;
        }
      }

      //this is a scanning for active node, make response
      Nwk_UpdateDepth(frm->hdr.srcAddr);
      return;
    }else{//update path to root
      #if (!NWK_STATION)
      if(frm->hdr.srcAddr == NwkInfo.parent){//path to root update
        //path to root not change
        if(frm->depth + 1 == NwkInfo.depth && 
          osal_memcmp((uint8*)&frm->pathtoroot[0], (uint8*)&NwkInfo.pathtoroot[0], frm->depth * sizeof(uint16))){
          NwkInfo.cost      = cost;
          NwkInfo.accucost  = frm->accucost + cost;
          Util_start_timerEx(Nwk_TaskId, NWK_EVENT_UPDATE_DEPTH, 1 + (osal_rand() & 0x1F));
          return;
        }

        //path to root changed
        NwkInfo.depth       = frm->depth + 1;
        NwkInfo.cost        = cost;
        NwkInfo.accucost    = frm->accucost + cost;
        if(frm->depth != 0) osal_memcpy((uint8*)&NwkInfo.pathtoroot[0], (uint8*)&frm->pathtoroot[0], frm->depth * sizeof(uint16));
        NwkInfo.pathtoroot[frm->depth] 	= NwkInfo.parent;
        NwkInfo.updatedepthtime			= current;
        osal_set_event(Nwk_TaskId, NWK_EVENT_UPDATE_DEPTH);
        return;
      }else{
        for(uint8 i = frm->depth; i > 0; --i){
          if(Nwk_devInfo.ShortAddr == frm->pathtoroot[i - 1]){//from descendant
            return;
          }
        }
        if(current - NwkInfo.updatedepthtime > NWK_WINDOW_DURATION && (frm->accucost + cost < NwkInfo.accucost || (frm->accucost + cost == NwkInfo.accucost && frm->depth + 1 > NwkInfo.depth))){
          NwkInfo.parent    = frm->hdr.srcAddr;
          NwkInfo.depth     = frm->depth + 1;
          NwkInfo.cost      = cost;
          NwkInfo.accucost  = frm->accucost + cost;
          if(frm->depth != 0) osal_memcpy((uint8*)&NwkInfo.pathtoroot[0], (uint8*)&frm->pathtoroot[0], frm->depth * sizeof(uint16));
          NwkInfo.pathtoroot[frm->depth] 	= NwkInfo.parent;
          NwkInfo.updatedepthtime			= current;
          osal_set_event(Nwk_TaskId, NWK_EVENT_UPDATE_DEPTH);
          return;
        }
        return;
      }
      #endif
    }
  }
}


void Nwk_SendBeacon(uint16 dstAddr)
{
  NWK_INCREASE_BASETIME(current);
  SyncInfo.BaseTime320us = Util_Sub320us(current320us, UTIL_MS_TO_320US(current - SyncInfo.BaseTime));
  SyncInfo.nsend    = NWK_BROADCAST_RETRANS;
  SyncInfo.dstAddr  = dstAddr;

  osal_memset(&Nwk_beacon, 0, 2);
  Nwk_beacon.hdr.type       = NWK_FRM_BEACON;
  Nwk_beacon.hdr.srcAddr    = Nwk_devInfo.ShortAddr;
  Nwk_beacon.hdr.dstAddr    = dstAddr;
  Nwk_beacon.hdr.dsn        = Nwk_dsn++;
  Nwk_beacon.hdr.len        = sizeof(nwk_beacon_t) - sizeof(nwk_frm_hdr_t);
  Nwk_beacon.adjcnt         = 0;

  Nwk_HdlSet.beacon = Nwk_devInfo.DataHandle;
  uint8 st = Nwk_McpsDataReq((uint8*)&Nwk_beacon, sizeof(nwk_beacon_t), SADDR_MODE_SHORT, dstAddr, 0);
  if(st != NWK_SUCCESS){
    Nwk_HdlSet.beacon = NWK_HANDLE_RESERVED;
  }
}

void Nwk_ProcessBeaconFrm(nwk_beacon_t *beacon, uint32 timestamp)
{
  if(current - SyncInfo.SyncTime < NWK_TICK_PERIOD && SyncInfo.SyncLost == FALSE){
    return;
  }

  //extend the wait beacon duration
  if(SyncInfo.SyncLost){
    Util_start_timerEx(Nwk_TaskId, NWK_EVENT_WAIT_BEACON_TIMEOUT, NWK_WAIT_BEACON_DURATION);
  }
    
  uint8 idx = 0;
  uint8 freeidx = MAX_TIMESYNC_ENTRIES;
  for(; idx < MAX_TIMESYNC_ENTRIES; ++idx){
    //delete timeout entry
    if(current - NwkInfo.TmSyncTbl[idx].recvtime > NWK_TICK_PERIOD / 2)
      NwkInfo.TmSyncTbl[idx].nbAddr = NWK_INVALID_ADDRESS;

    if(NwkInfo.TmSyncTbl[idx].nbAddr == beacon->hdr.srcAddr)
      break;
    if(freeidx == MAX_TIMESYNC_ENTRIES && NwkInfo.TmSyncTbl[idx].nbAddr == NWK_INVALID_ADDRESS)
      freeidx = idx;
  }

  //no available entry
  if(idx == MAX_TIMESYNC_ENTRIES && freeidx == MAX_TIMESYNC_ENTRIES)
    return;
  
  if(idx == MAX_TIMESYNC_ENTRIES){
    idx = freeidx;
    NwkInfo.TmSyncTbl[idx].nbAddr   = beacon->hdr.srcAddr;
    NwkInfo.TmSyncTbl[idx].dsn      = beacon->hdr.dsn;
    NwkInfo.TmSyncTbl[idx].recvtime = current - UTIL_320US_TO_MS(Util_Sub320us(current320us, timestamp));
  }else{
    timesync_entry_t *tse = &NwkInfo.TmSyncTbl[idx];  
    for(uint8 cnt = 0; cnt < beacon->adjcnt; ++cnt){
      if(beacon->adjinfo[cnt].dsn == tse->dsn){
        SyncInfo.BaseTime   = tse->recvtime - beacon->adjinfo[cnt].adjust;
        SyncInfo.SyncTime   = current;
        for(uint8 idx = 0; idx < MAX_TIMESYNC_ENTRIES; ++idx){
          NwkInfo.TmSyncTbl[idx].nbAddr = NWK_INVALID_ADDRESS;
        }

        //relay broadcast beacon
        if(beacon->hdr.dstAddr == NWK_BROADCAST_ADDR){
          //delay a random time to avoid collision
          Util_start_timerEx(Nwk_TaskId, NWK_EVENT_RELAY_BEACON, 1 + (osal_rand() & NWK_RELAY_BEACON_DELAY_MASK));
        }

        if(SyncInfo.SyncLost){
          SyncInfo.SyncLost = FALSE;
          //todo, arrange windows
          NWK_INCREASE_BASETIME(current);
          if(current - SyncInfo.BaseTime < NWK_WINDOW_DURATION){
            Util_start_timerEx(Nwk_TaskId, NWK_EVENT_WINDOW_OPEN, 1);
          }else{
            Util_start_timerEx(Nwk_TaskId, NWK_EVENT_WINDOW_OPEN, SyncInfo.BaseTime + NWK_WINDOW_INTERVAL - current);
          }
        }
        break;
      }
    }
  }

}

void Nwk_SendBeaconReq()
{
  nwk_beacon_req_t bcnreq;
  osal_memset(&bcnreq, 0, 2);
  bcnreq.hdr.type       = NWK_FRM_BCNREQ;
  bcnreq.hdr.srcAddr    = Nwk_devInfo.ShortAddr;
  bcnreq.hdr.dstAddr    = NWK_BROADCAST_ADDR;
  bcnreq.hdr.dsn        = Nwk_dsn++;
  bcnreq.hdr.len        = sizeof(nwk_beacon_req_t) - sizeof(nwk_frm_hdr_t);
  Nwk_HdlSet.beacon_req = Nwk_devInfo.DataHandle;
  uint8 st = Nwk_McpsDataReq((uint8*)&bcnreq, sizeof(nwk_beacon_req_t), SADDR_MODE_SHORT, MAC_SHORT_ADDR_BROADCAST, 0);
  if(st != NWK_SUCCESS){
    Nwk_HdlSet.beacon_req = NWK_HANDLE_RESERVED;
  }
}
void Nwk_ProcessBeaconReq(nwk_beacon_req_t* bcnreq)
{
  if(!SyncInfo.SyncLost && current - SyncInfo.SyncTime < NWK_WINDOW_INTERVAL + NWK_WINDOW_DURATION)
    Nwk_SendBeacon(bcnreq->hdr.srcAddr);
}
void NwkInfo_Init()
{
  NwkInfo.address = Nwk_devInfo.ShortAddr;
  #if (NWK_STATION)
  NwkInfo.accucost  = 0;
  NwkInfo.depth     = 0;
  NwkInfo.parent    = Nwk_devInfo.ShortAddr;
  #endif
  
  #if (NWK_ROUTER)
  NwkInfo.depth     = NWK_INVALID_DEPTH;
  NwkInfo.parent    = NWK_INVALID_ADDRESS;
  #endif

  #if (NWK_ROUTER || NWK_STATION)
  for(uint8 i = 0; i < MAX_TIMESYNC_ENTRIES; ++i){
    NwkInfo.TmSyncTbl[i].nbAddr = NWK_INVALID_ADDRESS;
  }
  for(uint8 i = 0; i < MAX_TRANS_BROADCAST_ENTRIES; ++i){
    NwkInfo.TransTbl[i].available = TRUE;
  }
  #endif

  //reset all hdl
  osal_memset((uint8*)&Nwk_HdlSet, NWK_HANDLE_RESERVED, sizeof(Nwk_HdlSet));
}

void Nwk_ProcessDataFrm(macCbackEvent_t* pData)
{    
  nwk_data_t *frm = (nwk_data_t*)pData->dataInd.msdu.p;
  uint8 indicatedata = FALSE;
  switch(frm->hdr.delivermode){
    case DM_CRD_TO_ALL:{
      #if (NWK_ROUTER || NWK_HANDHELD)
      indicatedata = TRUE;
      #endif
      break;
    }
    case DM_RTR_TO_CRD:
    case DM_HHD_TO_CRD:{
      #if (NWK_CARD)
      indicatedata = TRUE;
      #endif
      break;
    }

    case DM_RTR_TO_STN:{
      #if (NWK_STATION)
      indicatedata = TRUE;
      #endif

      #if (NWK_ROUTER)
      if(NwkInfo.parent != NWK_INVALID_ADDRESS){
        for(uint8 i = 0; i < MAX_TRANS_DATA_ENTRIES; ++i){
          if(Nwk_HdlSet.data[i] == NWK_HANDLE_RESERVED){
            Nwk_HdlSet.data[i] = Nwk_devInfo.DataHandle;
            uint8 st = Nwk_McpsDataReq((uint8*)frm, sizeof(nwk_frm_hdr_t) + frm->hdr.len, SADDR_MODE_SHORT, NwkInfo.parent, MAC_TXOPTION_ACK);
            if(st != NWK_SUCCESS){
              Nwk_HdlSet.data[i] = NWK_HANDLE_RESERVED;
            }
            break;
          }
        }
      }else{
        Nwk_UpdateDepth(NWK_BROADCAST_ADDR);
      }
      #endif
      
      break;
    }
    case DM_HHD_TO_STN:{
      #if (NWK_STATION)
      indicatedata = TRUE;
      #endif
      break;
    }
    case DM_CRD_TO_HHD:
    case DM_STN_TO_HHD:{
      #if (NWK_HANDHELD)
      indicatedata = TRUE;
      #endif
      break;
    }
    case DM_CRD_TO_RTR:{
      #if (NWK_ROUTER)
      indicatedata = TRUE;
      #endif
      break;
    }
    case DM_STN_TO_RTR:{
      #if (NWK_ROUTER)
      if(frm->hdr.dstAddr == NwkInfo.address || frm->hdr.dstAddr == NWK_BROADCAST_ADDR){
        indicatedata = TRUE;
      }

      if(frm->hdr.srcrtg){
        if(frm->hdr.dstAddr != NwkInfo.address){
          nwk_srcrtg_ext_t *srcrtg = (nwk_srcrtg_ext_t*)(&frm->hdr + 1);
          if(srcrtg->routing[srcrtg->idx] == NwkInfo.address){
            srcrtg->idx++;
            if(srcrtg->idx < srcrtg->cnt){
              Nwk_McpsDataReq((uint8*)frm, sizeof(nwk_frm_hdr_t) + frm->hdr.len, SADDR_MODE_SHORT, srcrtg->routing[srcrtg->idx], MAC_TXOPTION_ACK);
            }
          }
        }
      }else if(frm->hdr.dstAddr != NwkInfo.address || frm->hdr.dstAddr == NWK_BROADCAST_ADDR){
        uint8 idx = (frm->hdr.dsn & TRANS_BROADCAST_ENTRY_MASK);
        trans_broadcast_entry_t *tte = &NwkInfo.TransTbl[idx];
        if(tte->available ||
           current - tte->recvtime > NWK_TRANS_BRDCAST_DURATION ||
           Util_SequenceAfter8(frm->hdr.dsn, tte->dsn)){
          if(tte->buf) osal_mem_free(tte->buf);
          if((tte->buf = osal_mem_alloc(sizeof(nwk_frm_hdr_t) + frm->hdr.len)) != NULL){
            tte->available  = FALSE;
            tte->dsn        = frm->hdr.dsn;
            tte->recvtime   = current;
            tte->sendcount  = NWK_BROADCAST_RETRANS;
            osal_memcpy(tte->buf, (uint8*)frm, sizeof(nwk_frm_hdr_t) + frm->hdr.len);
            Nwk_HdlSet.broadcast[idx] = Nwk_devInfo.DataHandle;
            uint8 st = Nwk_McpsDataReq(tte->buf, sizeof(nwk_frm_hdr_t) + frm->hdr.len, SADDR_MODE_SHORT, MAC_SHORT_ADDR_BROADCAST, 0);
            if(st != NWK_SUCCESS){
              Nwk_HdlSet.broadcast[idx] = NWK_HANDLE_RESERVED;
              osal_mem_free(tte->buf);
              tte->buf = NULL;
              tte->available = TRUE;
            }
          }else{
            tte->available = TRUE;
          }
        }
      }
      #endif
      break;
    }
    case DM_TUNNELLING:{
      #if (!NWK_ROUTER)
      if(frm->hdr.dstAddr == NwkInfo.address || frm->hdr.dstAddr == NWK_BROADCAST_ADDR){
        indicatedata = TRUE;
      }
      #endif

      #if (NWK_ROUTER)
      if(frm->hdr.dstAddr == NWK_STATION_ADDRESS){
        if(NwkInfo.parent != NWK_INVALID_ADDRESS){
          for(uint8 i = 0; i < MAX_TRANS_DATA_ENTRIES; ++i){
            if(Nwk_HdlSet.data[i] == NWK_HANDLE_RESERVED){
              Nwk_HdlSet.data[i] = Nwk_devInfo.DataHandle;
              uint8 st = Nwk_McpsDataReq((uint8*)frm, sizeof(nwk_frm_hdr_t) + frm->hdr.len, SADDR_MODE_SHORT, NwkInfo.parent, MAC_TXOPTION_ACK);
              if(st != NWK_SUCCESS){
                Nwk_HdlSet.data[i] = NWK_HANDLE_RESERVED;
              }
              break;
            }
          }
        }else{
          Nwk_UpdateDepth(NWK_BROADCAST_ADDR);
        }
      }else if(frm->hdr.dstAddr != NwkInfo.address || frm->hdr.dstAddr == NWK_BROADCAST_ADDR){
        uint8 idx = (frm->hdr.dsn & TRANS_BROADCAST_ENTRY_MASK);
        trans_broadcast_entry_t *tte = &NwkInfo.TransTbl[idx];
        if(tte->available ||
           current - tte->recvtime > NWK_TRANS_BRDCAST_DURATION ||
           Util_SequenceAfter8(frm->hdr.dsn, tte->dsn)){
          if(tte->buf) osal_mem_free(tte->buf);
          if((tte->buf = osal_mem_alloc(sizeof(nwk_frm_hdr_t) + frm->hdr.len)) != NULL){
            tte->available  = FALSE;
            tte->dsn        = frm->hdr.dsn;
            tte->recvtime   = current;
            tte->sendcount  = NWK_BROADCAST_RETRANS;
            osal_memcpy(tte->buf, (uint8*)frm, sizeof(nwk_frm_hdr_t) + frm->hdr.len);
            Nwk_HdlSet.broadcast[idx] = Nwk_devInfo.DataHandle;
            uint8 st = Nwk_McpsDataReq(tte->buf, sizeof(nwk_frm_hdr_t) + frm->hdr.len, SADDR_MODE_SHORT, MAC_SHORT_ADDR_BROADCAST, 0);
            if(st != NWK_SUCCESS){
              Nwk_HdlSet.broadcast[idx] = NWK_HANDLE_RESERVED;
              osal_mem_free(tte->buf);
              tte->buf = NULL;
              tte->available = TRUE;
            }
          }else{
            tte->available = TRUE;
          }
        }
      }
      #endif
      break;
    }
    case DM_TUNNELLING2:{
      if(frm->hdr.dstAddr == NwkInfo.address || frm->hdr.dstAddr == NWK_BROADCAST_ADDR){
        indicatedata = TRUE;
      }
      break;
    }
    default:
      break;
  }

  if(indicatedata){
    Nwk_ComingDataCB(frm, pData->dataInd.mac.rssi);
  }
}

uint8* Nwk_DataReqBufAlloc(uint8 len)
{
  nwk_data_t* data = osal_mem_alloc(sizeof(nwk_data_t) + len);
  if(data){
    data->hdr.len = len;
    data++;
  }
  return (uint8*)data;
}
void Nwk_DataReqBufFree(void *p)
{
  osal_mem_free((nwk_data_t*)p - 1);
}

uint8 Nwk_DataReq(nwkDataReq_t *pData)
{
  uint8 status = NWK_SUCCESS;
  uint8 mactxopt = MAC_TXOPTION_ACK;
  nwk_data_t* frm = ((nwk_data_t*)pData->p) - 1;

  frm->hdr.srcAddr      = NwkInfo.address;
  frm->hdr.dstAddr      = pData->dstAddr;
  frm->hdr.len          = pData->len;
  frm->hdr.delivermode  = pData->delivermode;
  frm->hdr.type         = NWK_FRM_DATA;
  frm->hdr.dsn          = pData->seqnum;
  frm->hdr.srcrtg       = !!(pData->txopt & NWK_TXOPTION_SRCROUTING);
  frm->hdr.withpath     = !!(pData->txopt & NWK_TXOPTION_WITHPATH);

  #if (!NWK_STATION)
  if(frm->hdr.srcrtg){
    return NWK_INVALID_PARAM;
  }
  #endif

  #if (!NWK_ROUTER)
  if(frm->hdr.withpath){
    return NWK_INVALID_PARAM;
  }
  #endif

  switch(pData->delivermode){
    case DM_DEBUG:{
      mactxopt = 0;
      Nwk_McpsDataReq((uint8*)frm, sizeof(nwk_frm_hdr_t) + frm->hdr.len, SADDR_MODE_SHORT, frm->hdr.dstAddr, mactxopt);
      break;
    }
    
    #if (NWK_CARD)
    case DM_CRD_TO_ALL:
    case DM_CRD_TO_RTR:
    case DM_CRD_TO_HHD:{
      mactxopt = (frm->hdr.dstAddr == NWK_BROADCAST_ADDR) ? 0 : MAC_TXOPTION_ACK;
      status = Nwk_McpsDataReq((uint8*)frm, sizeof(nwk_frm_hdr_t) + frm->hdr.len, SADDR_MODE_SHORT, frm->hdr.dstAddr, mactxopt);
      break;
    }
    #endif  

    #if (NWK_HANDHELD)
    case DM_HHD_TO_CRD:
    case DM_HHD_TO_STN:{
      mactxopt = (frm->hdr.dstAddr == NWK_BROADCAST_ADDR) ? 0 : MAC_TXOPTION_ACK;
      status = Nwk_McpsDataReq((uint8*)frm, sizeof(nwk_frm_hdr_t) + frm->hdr.len, SADDR_MODE_SHORT, frm->hdr.dstAddr, mactxopt);
      break;
    }
    #endif
    
    #if (NWK_ROUTER)
    case DM_RTR_TO_CRD:{
      frm->hdr.withpath = 0;
      mactxopt = (frm->hdr.dstAddr == NWK_BROADCAST_ADDR) ? 0 : MAC_TXOPTION_ACK;
      status = Nwk_McpsDataReq((uint8*)frm, sizeof(nwk_frm_hdr_t) + frm->hdr.len, SADDR_MODE_SHORT, frm->hdr.dstAddr, mactxopt);
      break;
    }
    case DM_RTR_TO_STN:{
      if(pData->dstAddr != NWK_STATION_ADDRESS){
        status = NWK_INVALID_PARAM;
        break;
      }
      if(NwkInfo.parent == NWK_INVALID_ADDRESS){
        status = NWK_CONN_LOST;
        break;
      }
      
      uint8 idx = 0;
      for(idx = 0; idx < MAX_TRANS_DATA_ENTRIES; ++idx){
        if(Nwk_HdlSet.data[idx] == NWK_HANDLE_RESERVED){
          Nwk_HdlSet.data[idx] = Nwk_devInfo.DataHandle;
          status = Nwk_McpsDataReq((uint8*)frm, sizeof(nwk_frm_hdr_t) + frm->hdr.len, SADDR_MODE_SHORT, NwkInfo.parent, MAC_TXOPTION_ACK);
          if(status != NWK_SUCCESS){
            Nwk_HdlSet.data[idx] = NWK_HANDLE_RESERVED;
          }
          break;
        }
      }
      if(idx == MAX_TRANS_DATA_ENTRIES){
        status = NWK_BUSY;
        break;
      }
      break;
    }
    #endif

    #if (NWK_STATION)
    case DM_TUNNELLING2:{
      mactxopt = (frm->hdr.dstAddr == NWK_BROADCAST_ADDR) ? 0 : MAC_TXOPTION_ACK;
      status = Nwk_McpsDataReq((uint8*)frm, sizeof(nwk_frm_hdr_t) + frm->hdr.len, SADDR_MODE_SHORT, frm->hdr.dstAddr, mactxopt);
      break;
    }
    case DM_TUNNELLING:{
      if(frm->hdr.srcrtg){
        status = NWK_INVALID_PARAM;
        break;
      }
      //NOTE: no break;
    }
    case DM_STN_TO_RTR:{        
      if(frm->hdr.srcrtg){
        nwk_srcrtg_ext_t *srcrtg = (nwk_srcrtg_ext_t*)(frm + 1);
        status = Nwk_McpsDataReq((uint8*)frm, sizeof(nwk_frm_hdr_t) + frm->hdr.len, SADDR_MODE_SHORT, srcrtg->routing[srcrtg->idx], MAC_TXOPTION_ACK);
      }else{
        uint8 idx = (Nwk_broadcast_dsn & TRANS_BROADCAST_ENTRY_MASK);
        trans_broadcast_entry_t *tte = &NwkInfo.TransTbl[idx];
        if(tte->available || current - tte->recvtime > NWK_TRANS_BRDCAST_DURATION){
          if(tte->buf) osal_mem_free(tte->buf);
          tte->buf = osal_mem_alloc(sizeof(nwk_frm_hdr_t) + frm->hdr.len);
          if(tte->buf){
            osal_memcpy(tte->buf, frm, sizeof(nwk_frm_hdr_t) + frm->hdr.len);
            tte->available  = FALSE;
            tte->dsn        = Nwk_broadcast_dsn;
            tte->recvtime   = current;
            tte->sendcount  = NWK_BROADCAST_RETRANS;
            Nwk_HdlSet.broadcast[idx] = Nwk_devInfo.DataHandle;
            status = Nwk_McpsDataReq(tte->buf, sizeof(nwk_frm_hdr_t) + frm->hdr.len, SADDR_MODE_SHORT, MAC_SHORT_ADDR_BROADCAST, 0);
            if(status == NWK_SUCCESS){
              Nwk_broadcast_dsn++;
            }else{
              osal_mem_free(tte->buf);
              tte->buf = NULL;
              Nwk_HdlSet.broadcast[idx] = NWK_HANDLE_RESERVED;
              tte->available = TRUE;
            }
          }else{
            status = NWK_NO_MEMORY;
          }
        }else{
          status = NWK_BUSY;
        }
      }
      break;
    }
    #endif
    
    default:
      status = NWK_INVALID_PARAM;
      break;
  }

  return status;
}

void Nwk_RxOnReq(uint8 appid)
{
  Nwk_TurnOnRx(NWK_RXON_ID_APP + appid);
}

void Nwk_RxOffReq(uint8 appid)
{
  Nwk_TurnOffRx(NWK_RXON_ID_APP + appid);
}

void Nwk_Start()
{
  Nwk_DeviceStartup();
  Nwk_Reset();
}

uint16 Nwk_PanId()
{
  return Nwk_devInfo.PanId;
}

uint16 Nwk_ShortAddr()
{
  return NwkInfo.address;
}

#if (NWK_ROUTER)
uint8 Nwk_Depth()
{
  return NwkInfo.depth;
}
#endif

#if (NWK_ROUTER)
uint8 Nwk_Path(uint16 * buf, uint8 depth)
{
  if(depth != NwkInfo.depth)
    return NWK_FAILURE;
  osal_memcpy((uint8*)buf, (uint8*)NwkInfo.pathtoroot, sizeof(uint16) * depth);
  return NWK_SUCCESS;
}
#endif

uint8 Nwk_ExtHdrLen(nwk_data_t *frm)
{
  uint8 extlen = 0;
  if(frm->hdr.srcrtg){
    nwk_srcrtg_ext_t *srcrtg = (nwk_srcrtg_ext_t*)frm->payload;
    extlen = sizeof(nwk_srcrtg_ext_t) + sizeof(uint16) * srcrtg->cnt;
  }
  if(frm->hdr.withpath){
    nwk_path_ext_t *path = (nwk_path_ext_t*)frm->payload;
    extlen = sizeof(nwk_path_ext_t) + sizeof(uint16) * path->depth;
  }
  return extlen;
}

void Nwk_CheckParent()
{
  if(NwkInfo.parent == NWK_INVALID_ADDRESS) return;
  nwk_checkparent_t frm;
  osal_memset(&frm, 0, 2);
  frm.hdr.type = NWK_FRM_CHECKPARENT;
  frm.hdr.dstAddr   = NwkInfo.parent;
  frm.hdr.srcAddr   = NwkInfo.address;
  frm.hdr.dsn       = Nwk_dsn++;
  frm.hdr.len       = 0;
  Nwk_HdlSet.check_parent = Nwk_devInfo.DataHandle;
  if(NWK_SUCCESS != Nwk_McpsDataReq((uint8*)&frm, sizeof(nwk_frm_hdr_t) + frm.hdr.len, SADDR_MODE_SHORT, NwkInfo.parent, MAC_TXOPTION_ACK)){
    Nwk_HdlSet.check_parent = NWK_HANDLE_RESERVED;
  }
}
/**************************************************************************************************
 **************************************************************************************************/

