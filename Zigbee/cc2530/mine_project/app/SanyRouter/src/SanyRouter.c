/**************************************************************************************************
  Filename:       SanyRouter.c
  Revised:        $Date: 2009-06-04 12:31:30 -0700 (Thu, 04 Jun 2009) $
  Revision:       $Revision: 20092 $

  Description:    This file contains the sample application that can be use to test
                  the functionality of the MAC, HAL and low level.


  Copyright 2006-2009 Texas Instruments Incorporated. All rights reserved.

**************************************************************************************************/


/**************************************************************************************************
 *                                           Includes
 **************************************************************************************************/

/* Hal Driver includes */
#include "hal_types.h"
#include "hal_key.h"
#include "hal_timer.h"
#include "hal_drivers.h"
#include "hal_led.h"
#include "hal_adc.h"
#include "hal_lcd.h"

#ifndef LL_CC2430
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

/* Application */
#include "SanyRouter.h"
#ifndef LL_NODEROLE_ROUTERNODE
#include "SanyCollector.h"
#endif


/* utils */
#include "macutil.h"
#include "App_cfg.h"
#include "AppProtocol.h"
#include "delay.h"
#include "OSAL_Nv.h"
#include "ZComdef.h"
#ifdef WATCHDOG
#include "WatchdogUtil.h"
#endif

/* check node role definition */
#if !defined(LL_NODEROLE_COLLECTNODE) && \
    !defined(LL_NODEROLE_ROUTERNODE) && \
    !defined(LL_NODEROLE_REPORTNODE)
#error "no node role defined!"
#endif

#if (defined(LL_NODEROLE_COLLECTNODE) && defined(LL_NODEROLE_ROUTERNODE)) || \
    (defined(LL_NODEROLE_COLLECTNODE) && defined(LL_NODEROLE_REPORTNODE)) || \
    (defined(LL_NODEROLE_ROUTERNODE) && defined(LL_NODEROLE_REPORTNODE))
#error "multiple node role defined!"
#endif


#define TWO_WAY_DATA

extern uint32 macBackoffTimerCount(void);
#define MAX_BACKOFF_TIMER_COUNT ((uint32)(3 * 16) << 14)
#define LL_GetTime320us()  macBackoffTimerCount()

#ifdef SLEEPTIME_ACCOUNTING
uint32 sleep32kticks = 0;
uint16 sleeps = 0;
uint32 starttime = 0;
uint32 sleeptimebase = 0;
#endif
/**************************************************************************************************
 *                                           Constant
 **************************************************************************************************/

#define LL_APPHDL_RESERVED       0xFF          /* reserved application handle */

#define LL_PROFILER_ADDR         0x1111
#define LL_PROFILER_PERIOD       60000u

#define LL_CONSUMER_ADDR_HEAD    0x2222
#define LL_CONSUMER_ADDR_TAIL    0x3333


/* Size table for MAC structures */
const CODE uint8 ll_cbackSizeTable [] =
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
uint8 ll_Sany_Air_Channel;
#define LL_SANY_AIR_SEND_DURATION       40u

/* assign child id fail count */
#define LL_CONNECT_CHILD_FAIL_MAX 3
uint8 ll_connect_child_fail_count;
uint8 ll_connect_child_handle;
uint32 ll_connect_child_time;

typedef struct{
  uint16 PanId;
  uint8 LogicalChannel;
  uint8 BeaconOrder;
  uint8 SupperFrameOrder;
  uint16 ShortAddr;
  uint16 ParentShortAddr;
  uint16 ChildShortAddr;
  sAddrExt_t ExtAddr;
  sAddrExt_t dstExtAddr;
  uint8 ChildLQI;
  uint8 dataHandle;
  uint8 lossParent;
  uint8 sendLossParent;
  uint8 probeParentTimes;
  uint16 listenChildPeriod;
  uint32 sendTime;
}DevInfo_t;

DevInfo_t ll_devInfo;

typedef struct{
  uint8 sleepBitmap[32];
  uint32 topmask;
}Sleep_mgr_t;

Sleep_mgr_t sleep_mgr;

#define LL_SEND_DATA_INTERVAL
#define LL_MIN_DATA_SEND_SPAN      10

/* TRUE and FALSE value */
bool ll_MACTrue  = TRUE;
bool ll_MACFalse = FALSE;

/* Rx on bitmap */
static uint16 ll_Rxon_bitmap = 0;

/* Task ID */
uint8 LL_TaskId;
uint8 C2P_TaskId;
uint8 C2C_TaskId;

/* Event IDs */
#define LL_EVENT_LISTEN_CHILD           0x0001
#define LL_EVENT_LISTEN_CHILD_TIMEOUT   0x0002
#define LL_EVENT_CONNECTING_TIMEOUT     0x0004
#define LL_EVENT_RESTART                0x0008
#define LL_EVENT_PROFILER               0x0010
#define LL_FEED_WATCHDOG_EVENT          0x0020
#define LL_EVENT_DUMMY_WAKEUP           0x0040

#define LL_EVENT_SANY_AIR_SEND          0x0100
#define LL_EVENT_SANY_SEND_VER          0x0200
#define LL_EVENT_SANY_AIR_UNLOCK        0x0400



#define C2P_EVENT_ACTIVE_SCAN           0x0001
#define C2P_EVENT_ACTIVE_SCAN_TIMEOUT   0x0002
#define C2P_EVENT_CONNECT_RSP_TIMEOUT   0x0004
#define C2P_EVENT_SEND_DATA             0x0008
#define C2P_EVENT_RECV_DATA_BEGIN       0x0010
#define C2P_EVENT_RECV_DATA_TIMEOUT     0x0020
#define C2P_EVENT_START_TIMEOUT         0x0040
#define C2P_EVENT_WAIT_ACK_TIMEOUT      0x0080
#define C2P_EVENT_SEND_DATA_CONTINUE    0x0100
#define C2P_EVENT_CONNECT_RSP_BEGIN     0x0200

#define C2C_EVENT_SEND_DATA             0x0001
#define C2C_EVENT_RECV_DATA_BEGIN       0x0002
#define C2C_EVENT_RECV_DATA_TIMEOUT     0x0004
#define C2C_EVENT_START_TIMEOUT         0x0008

/* app state */
#define LL_STATE_LISTENING              0
#define LL_STATE_CONNECTING             1
#define LL_STATE_STOP_LISTEN            2
uint8 LL_state;

#define C2P_STATE_UNCONNECTED           0
#define C2P_STATE_CONNECTING            1
#define C2P_STATE_CONNECTED             2
uint8 C2P_state;

#define C2C_STATE_CONNECTED             0
#define C2C_STATE_UNCONNECTED           1
uint8 C2C_state;

/**************************************************************************************************
 *                                     Local Function Prototypes
 **************************************************************************************************/
/* Setup routines */
void LL_DeviceStartup(void);

/* MAC related routines */
void LL_McpsDataReq(uint8* data, uint8 dataLength, uint8 srcAddrMode, uint16 dstShortAddr, uint8 txOpt);

/* ListLink related constants, veriants and local functions */
#define LL_time_after(a, b) ((int32)(b) - (int32)(a) < 0)

/* make/apply adjust */
#define make_adjust(adj, base, real) \
do { \
  uint32 tmp; \
  if(LL_time_after((base), (real))){ \
    tmp = (base) - (real); \
    adj = (uint16)(tmp > 0x7fff ? 0x7fff : tmp); \
    adj |= 0x8000; \
  } else { \
    tmp = (real) - (base); \
    adj = (uint16)(tmp > 0x7fff ? 0x7fff : tmp); \
  } \
} while (0)
#define apply_adjust(val, adj) \
do { \
  if((adj) & 0x8000){ \
    val -= ((adj) ^ 0x8000); \
  } else { \
    val += (adj); \
  } \
} while(0)

#define LL_ACTIVE_SCAN_PERIOD               1000u
#define LL_LISTEN_CHILD_DURATION            (LL_ACTIVE_SCAN_PERIOD + 20)
#define LL_LISTEN_CHILD_PERIOD_0            (2 * LL_LISTEN_CHILD_DURATION)
#define LL_LISTEN_CHILD_PERIOD              (LL_ACTIVE_SCAN_PERIOD * 25)     /* 4% duty ratio */
#define LL_LISTEN_ACTIVE_RSP_DURATION       20u
#define LL_LISTEN_ACTIVE_RSP_CYCLE          4
#define LL_CONNECT_CHILD_DURATION           (LL_ACTIVE_SCAN_PERIOD * (LL_LISTEN_ACTIVE_RSP_CYCLE + 2))
#define LL_CONNECT_PARENT_DURATION          (LL_CONNECT_CHILD_DURATION + 1000u)
#define LL_WAIT_ACK_DURATION                50u
#define LL_DATA_TRANS_PERIOD                5000u
#define LL_TIMER_ERROR                      10u

/* 5 seconds per unit */
#define LL_DOWNSTREAM_UNITS                 6
#define LL_UPSTREAM_UNITS                   6

#define LL_DOWNSTREAM_TRANS_PERIOD          (LL_DATA_TRANS_PERIOD * LL_DOWNSTREAM_UNITS)
#define LL_UPSTREAM_TRANS_PERIOD            (LL_DATA_TRANS_PERIOD * LL_UPSTREAM_UNITS)

#define LL_DOWNSTREAM_TIMER_ERROR           (LL_TIMER_ERROR * LL_DOWNSTREAM_UNITS)
#define LL_UPSTREAM_TIMER_ERROR             (LL_TIMER_ERROR * LL_UPSTREAM_UNITS)

/* fix bug: when sleep time is more than 20 secs, 2530 won't wakeup in time */
#if (LL_DOWNSTREAM_TRANS_PERIOD > 20000u && LL_UPSTREAM_TRANS_PERIOD > 20000u)
#define LL_DUMMY_WAKEUP
#define LL_DUMMY_WAKEUP_PERIOD              18000u
#endif

typedef struct{
  uint8 type;
  uint8 flag;
  int16 period_adj;
  uint8 hops;
  uint8 total_frags;
  uint8 current_frag;
  uint8 dsn;
  uint8 len;
}ll_Data_hdr_t;

#define LL_DATA_BUFFER_LENGTH           MAC_MAX_FRAME_SIZE
#define LL_DATA_HEAD_LENGTH             (sizeof(ll_Data_hdr_t))
#define LL_DATA_MAX_LENGTH              (LL_DATA_BUFFER_LENGTH - LL_DATA_HEAD_LENGTH)


#define LL_DATA_FLAG_LOSS_PARENT        0x01
#define LL_DATA_FLAG_REQUEST_ACK        0x02
#define LL_DATA_FLAG_INVALID            0x04
#define LL_DATA_FLAG_DUMMY              0x08

#define LL_DATA_TYPE_ACTIVE_SCAN        0
#define LL_DATA_TYPE_ACTIVE_RSP         1
#define LL_DATA_TYPE_CONNECT_REQ        2
#define LL_DATA_TYPE_CONNECT_RSP        3
#define LL_DATA_TYPE_UPSTREAM           4
#define LL_DATA_TYPE_UPSTREAM_ACK       5
#define LL_DATA_TYPE_DOWNSTREAM         6
#define LL_DATA_TYPE_DOWNSTREAM_ACK     7


#define LL_MAX_SINGLE_TRANS_TIME        8u
#define LL_MAX_TRANS_TIME               (LL_MAX_SINGLE_TRANS_TIME * 4)
#define LL_SEND_DATA_DELAY              200u
#define LL_START_ACTIVE_SCAN_DELAY      100u
#define LL_START_LISTEN_CHILD_DELAY     100u

#define LL_LQI_SHRESHOLD                0

#define LL_SEND_FAIL_COUNT_MAX          3
#define LL_RECV_FAIL_COUNT_MAX          3
#define LL_SEND_RETRIES                 4        /* include the first time */
#define LL_ACK_CYCLE_DEFAULT            9
#define LL_MAX_DATA_FRAGMENTS           6
typedef struct {
  uint32 adjustRecv;
  uint32 nextRecv;
  uint32 sendTime;
  uint32 adjustPeriod;
  uint32 sendTime320us;
  uint32 recvTime;
  uint8 firstSend;
  uint8 sendFailCount;
  uint8 recvFailCount;
  uint8 retryTimeout;
  uint8 recvFinish;
  uint8 totalSend;
  uint8 currSend;
  uint8 totalRecv;
  uint8 totalMask;
  uint8 sendMask;
  uint8 recvMask;
  uint8 recvDSN;
  uint8 sendDSN;
  uint8 dataReady;
  uint8 partialRecv;
  uint8 partialSend;
  uint8 firstDataReceived;
  uint8 sendHandle[LL_MAX_DATA_FRAGMENTS];
  uint8 sendSuccess;
  uint8 ackCycle;
  uint8 waitCycle;
  uint8* dataBuf;
  uint8* dummyBuf;
  uint8 dummyData;
}StreamInfo_t;

uint8 _upstream_dummyBuf[LL_DATA_HEAD_LENGTH];
uint8 _downstream_dummyBuf[LL_DATA_HEAD_LENGTH];
uint8 _upstream_buf[LL_DATA_BUFFER_LENGTH * LL_MAX_DATA_FRAGMENTS];
uint8 _downstream_buf[LL_DATA_BUFFER_LENGTH];

StreamInfo_t ll_Upstream_Info;
#ifdef TWO_WAY_DATA
StreamInfo_t ll_Downstream_Info;
#endif

#define ll_bit_test(val, mask)  (!!((val) & (mask)))
#define ll_bit_set(var, mask)   (var |= (mask))
#define ll_bit_clear(var, mask) (var &= ~(mask))

void LL_ProcessData(macCbackEvent_t *pData, uint32 timestamp);
void LL_HandleDataCnf(uint8 handle, uint8 status, uint32 timestamp);

void C2P_ProcessData(macCbackEvent_t *pData, uint32 timestamp);

void C2C_ProcessData(macCbackEvent_t *pData, uint32 timestamp);

void LL_TurnOnRx(uint8 taskId);
void LL_TurnOffRx(uint8 taskId);

void LL_ConnectChild(void);
void LL_StartListen(void);
void LL_StopListen(void);

void C2C_StartData(void);
void C2C_StopData(void);

void C2P_StartActiveScan(void);
void C2P_StartData(void);
void C2P_StopData(void);

void LL_DataProduce(StreamInfo_t *stream, uint8 type);
void LL_DataConsume(StreamInfo_t *stream, uint8 type);

void LL_SetupDevInfo(void);
void LL_Restart(void);

void LL_DisableSleep(uint8 handle);
void LL_EnableSleep(uint8 handle);

void LL_Init(uint8 taskId)
{
  /* Initialize the task id */
  LL_TaskId = taskId;

  /* initialize MAC features */
  MAC_InitDevice();
  MAC_InitCoord();

  /* Initialize MAC beacon */
  MAC_InitBeaconDevice();
  MAC_InitBeaconCoord();

  /* Reset the MAC */
  MAC_MlmeResetReq(TRUE);

  LL_SetupDevInfo();

  LL_DeviceStartup();

#ifndef LL_NODEROLE_ROUTERNODE
  Sany_Init(ll_devInfo.ShortAddr);
#endif

#ifdef SLEEPTIME_ACCOUNTING
  if(SUCCESS != osal_start_timerEx(LL_TaskId, LL_EVENT_PROFILER, LL_PROFILER_PERIOD)){
    SystemReset();
  }
#endif

#if(defined WATCHDOG) && (WATCHDOG==TRUE)
  StartWatchDog(DOGTIMER_INTERVAL_1S);
#endif

  osal_pwrmgr_device(PWRMGR_BATTERY);
  osal_pwrmgr_task_state(taskId, PWRMGR_CONSERVE);

  osal_memset(&sleep_mgr, 0, sizeof(sleep_mgr));

#ifdef LL_DUMMY_WAKEUP
  osal_start_timerEx(LL_TaskId, LL_EVENT_DUMMY_WAKEUP, LL_DUMMY_WAKEUP_PERIOD);
#endif
}

void C2P_Init(uint8 taskId)
{
  C2P_TaskId = taskId;

  osal_pwrmgr_task_state(taskId, PWRMGR_CONSERVE);
}

void C2C_Init(uint8 taskId)
{
  C2C_TaskId = taskId;

  osal_pwrmgr_task_state(taskId, PWRMGR_CONSERVE);
}

uint16 LL_ProcessEvent(uint8 taskId, uint16 events)
{
#ifdef LL_DUMMY_WAKEUP
  if (events & LL_EVENT_DUMMY_WAKEUP){
  	osal_start_timerEx(LL_TaskId, LL_EVENT_DUMMY_WAKEUP, LL_DUMMY_WAKEUP_PERIOD);
    return events ^ LL_EVENT_DUMMY_WAKEUP;
  }
#endif

#if(defined WATCHDOG) && (WATCHDOG==TRUE)
  if (events & LL_FEED_WATCHDOG_EVENT){
    FEEDWATCHDOG();
    if(SUCCESS!=osal_start_timerEx(LL_TaskId, LL_FEED_WATCHDOG_EVENT, 300)){
      SystemReset();
    }
    return events ^  LL_FEED_WATCHDOG_EVENT;
  }
#endif

  if (events & SYS_EVENT_MSG)
  {
    uint8* pMsg;
    macCbackEvent_t* pData;
    ll_Data_hdr_t *hdr;

    while ((pMsg = osal_msg_receive(LL_TaskId)) != NULL)
    {
      switch ( *pMsg )
      {
        case MAC_MCPS_DATA_CNF:
          pData = (macCbackEvent_t *) pMsg;

          LL_HandleDataCnf(pData->dataCnf.msduHandle, pData->dataCnf.hdr.status, pData->dataCnf.timestamp);

          mac_msg_deallocate((uint8**)&pData->dataCnf.pDataReq);
          break;

        case MAC_MCPS_DATA_IND:
          pData = (macCbackEvent_t*)pMsg;

          /* ignore broadcast data from other PAN */
          if(pData->dataInd.mac.dstPanId != ll_devInfo.PanId)
            break;
          if(pData->dataInd.mac.srcAddr.addr.shortAddr == 0x0000)
            break;

          hdr = (ll_Data_hdr_t*)pData->dataInd.msdu.p;
          switch(hdr->type) {
            case LL_DATA_TYPE_DOWNSTREAM:
            case LL_DATA_TYPE_ACTIVE_RSP:
            case LL_DATA_TYPE_CONNECT_RSP:
            case LL_DATA_TYPE_UPSTREAM_ACK:
              C2P_ProcessData(pData, pData->dataInd.mac.timestamp);
              break;
            case LL_DATA_TYPE_UPSTREAM:
              C2C_ProcessData(pData, pData->dataInd.mac.timestamp);
              break;
            case LL_DATA_TYPE_ACTIVE_SCAN:
            case LL_DATA_TYPE_CONNECT_REQ:
              LL_ProcessData(pData, pData->dataInd.mac.timestamp);
              break;
            default:
              break;
          }

          break;
      }

      /* Deallocate */
      mac_msg_deallocate((uint8 **)&pMsg);
    }

    return events ^ SYS_EVENT_MSG;
  }

  /* begin listen in pwr saving mode */
  if (events & LL_EVENT_LISTEN_CHILD) {
    if (LL_state == LL_STATE_LISTENING) {
      LL_TurnOnRx(LL_TaskId);
      uint16 period;
      if(ll_devInfo.listenChildPeriod < LL_LISTEN_CHILD_PERIOD){
        period = ll_devInfo.listenChildPeriod;
        if(ll_devInfo.listenChildPeriod >= (LL_LISTEN_CHILD_PERIOD / 2)){
          ll_devInfo.listenChildPeriod = LL_LISTEN_CHILD_PERIOD;
        }else{
          ll_devInfo.listenChildPeriod *= 2;
        }
      }else{
        period = LL_LISTEN_CHILD_PERIOD;
      }
      if(SUCCESS != osal_start_timerEx(LL_TaskId, LL_EVENT_LISTEN_CHILD, period)){
        SystemReset();
      }
      if(SUCCESS != osal_start_timerEx(LL_TaskId, LL_EVENT_LISTEN_CHILD_TIMEOUT, LL_LISTEN_CHILD_DURATION)){
        SystemReset();
      }
    }
    return events ^ LL_EVENT_LISTEN_CHILD;
  }

  /* stop listen in pwr saving mode */
  if (events & LL_EVENT_LISTEN_CHILD_TIMEOUT) {
    if (LL_state == LL_STATE_LISTENING) {
      LL_TurnOffRx(LL_TaskId);
    }
    return events ^ LL_EVENT_LISTEN_CHILD_TIMEOUT;
  }

  /* stop current listening, connect to the best child */
  if (events & LL_EVENT_CONNECTING_TIMEOUT) {
   if (LL_state == LL_STATE_CONNECTING && (ll_devInfo.ChildLQI > LL_LQI_SHRESHOLD)) {
      LL_ConnectChild();
    } else {
      LL_StartListen();
    }
    return events ^ LL_EVENT_CONNECTING_TIMEOUT;
  }

  if (events & LL_EVENT_RESTART) {
    LL_Restart();
    return events ^ LL_EVENT_RESTART;
  }

#ifdef SLEEPTIME_ACCOUNTING
  if (events & LL_EVENT_PROFILER) {

#ifdef LL_SEND_DATA_INTERVAL
    uint32 current = osal_GetSystemClock();
    if(current - ll_devInfo.sendTime < LL_MIN_DATA_SEND_SPAN){
      if(SUCCESS != osal_start_timerEx(LL_TaskId, LL_EVENT_PROFILER, LL_MIN_DATA_SEND_SPAN - (current - ll_devInfo.sendTime))){
        SystemReset();
      }
      return events ^ LL_EVENT_PROFILER;
    }
#endif

    starttime += (sleep32kticks >> 15) * 1000;
    sleep32kticks &= 0x7FFF;
    uint32 sleeptime = starttime + (sleep32kticks * 125 + 4095) / 4096;

    uint16 rate = (uint16)((1.0 - (float)sleeptime / (osal_GetSystemClock() - sleeptimebase)) * 10000);
    uint8 buf[25] = {0};
    uint8 idx = 0;
    buf[idx++] = 0xff;
    buf[idx++] = HI_UINT16(rate);
    buf[idx++] = LO_UINT16(rate);
    buf[idx++] = HI_UINT16(sleeps);
    buf[idx++] = LO_UINT16(sleeps);
    buf[idx++] = BREAK_UINT32(sleeptime, 3);
    buf[idx++] = BREAK_UINT32(sleeptime, 2);
    buf[idx++] = BREAK_UINT32(sleeptime, 1);
    buf[idx++] = BREAK_UINT32(sleeptime, 0);
    sleeptime = osal_GetSystemClock();
    buf[idx++] = BREAK_UINT32(sleeptime, 3);
    buf[idx++] = BREAK_UINT32(sleeptime, 2);
    buf[idx++] = BREAK_UINT32(sleeptime, 1);
    buf[idx++] = BREAK_UINT32(sleeptime, 0);
    buf[idx++] = BREAK_UINT32(sleeptimebase, 3);
    buf[idx++] = BREAK_UINT32(sleeptimebase, 2);
    buf[idx++] = BREAK_UINT32(sleeptimebase, 1);
    buf[idx++] = BREAK_UINT32(sleeptimebase, 0);
    buf[idx++] = BREAK_UINT32(starttime, 3);
    buf[idx++] = BREAK_UINT32(starttime, 2);
    buf[idx++] = BREAK_UINT32(starttime, 1);
    buf[idx++] = BREAK_UINT32(starttime, 0);
    buf[idx++] = BREAK_UINT32(sleep32kticks, 3);
    buf[idx++] = BREAK_UINT32(sleep32kticks, 2);
    buf[idx++] = BREAK_UINT32(sleep32kticks, 1);
    buf[idx++] = BREAK_UINT32(sleep32kticks, 0);

    LL_McpsDataReq(buf, idx, SADDR_MODE_SHORT, LL_PROFILER_ADDR, 0);
    if(SUCCESS != osal_start_timerEx(LL_TaskId, LL_EVENT_PROFILER, LL_PROFILER_PERIOD)){
      SystemReset();
    }
    return events ^ LL_EVENT_PROFILER;
  }
#endif

#ifdef LL_NODEROLE_REPORTNODE
  if(events & LL_EVENT_SANY_SEND_VER){
    Sany_SendVersionEvent();
    if(SUCCESS != osal_start_timerEx(LL_TaskId, LL_EVENT_SANY_SEND_VER, SANY_SEND_VERSION_EVENT_PERIOD)){
      SystemReset();
    }
    return events ^ LL_EVENT_SANY_SEND_VER;
  }

  if(events & LL_EVENT_SANY_AIR_SEND){
    uint16 timeout = 0;
    if((timeout = osal_get_timeoutEx(C2C_TaskId, C2C_EVENT_SEND_DATA)) <= LL_SANY_AIR_SEND_DURATION) {
      timeout += LL_MAX_TRANS_TIME * LL_SEND_RETRIES + 20;
      if(SUCCESS != osal_start_timerEx(LL_TaskId, LL_EVENT_SANY_AIR_SEND, timeout)){
        SystemReset();
      }
      return events ^ LL_EVENT_SANY_AIR_SEND;
    }

    if((timeout = osal_get_timeoutEx(C2C_TaskId, C2C_EVENT_RECV_DATA_BEGIN)) <= LL_SANY_AIR_SEND_DURATION) {
      timeout += LL_UPSTREAM_TIMER_ERROR + LL_MAX_TRANS_TIME * LL_SEND_RETRIES + 20;
      if(SUCCESS != osal_start_timerEx(LL_TaskId, LL_EVENT_SANY_AIR_SEND, timeout)){
        SystemReset();
      }
      return events ^ LL_EVENT_SANY_AIR_SEND;
    }

    MAC_MlmeSetReq(MAC_LOGICAL_CHANNEL, &ll_Sany_Air_Channel);
    if(SUCCESS != osal_start_timerEx(LL_TaskId, LL_EVENT_SANY_AIR_UNLOCK, LL_SANY_AIR_SEND_DURATION)){
      SystemReset();
    }

    if(Sany_SendDataEvent()){//subsequent data
      if(SUCCESS != osal_start_timerEx(LL_TaskId, LL_EVENT_SANY_AIR_SEND, SANY_SEND_DATA_EVENT_INTERVAL)){
        SystemReset();
      }
    }

    return events ^ LL_EVENT_SANY_AIR_SEND;
  }

  if(events & LL_EVENT_SANY_AIR_UNLOCK) {
    MAC_MlmeSetReq(MAC_LOGICAL_CHANNEL, &ll_devInfo.LogicalChannel);
    return events ^ LL_EVENT_SANY_AIR_UNLOCK;
  }
#endif

  return 0;
}

void MAC_CbackEvent(macCbackEvent_t *pData)
{
  if(pData->hdr.event == MAC_MCPS_DATA_CNF){
    LL_EnableSleep(pData->dataCnf.msduHandle);
  }

  macCbackEvent_t *pMsg = NULL;

  uint8 len = ll_cbackSizeTable[pData->hdr.event];

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
    osal_msg_send(LL_TaskId, (uint8*)pMsg);
  }
}

uint8 MAC_CbackCheckPending(void)
{
  return (0);
}

void LL_DeviceStartup()
{
  MAC_MlmeSetReq(MAC_EXTENDED_ADDRESS, &ll_devInfo.ExtAddr);

  MAC_MlmeSetReq(MAC_SHORT_ADDRESS, &ll_devInfo.ShortAddr);

  MAC_MlmeSetReq(MAC_PAN_ID, &ll_devInfo.PanId);

  MAC_MlmeSetReq(MAC_LOGICAL_CHANNEL, &ll_devInfo.LogicalChannel);

  MAC_MlmeSetReq(MAC_BEACON_ORDER, &ll_devInfo.BeaconOrder);

  MAC_MlmeSetReq(MAC_SUPERFRAME_ORDER, &ll_devInfo.SupperFrameOrder);

  MAC_MlmeSetReq(MAC_ASSOCIATED_PAN_COORD, &ll_MACTrue);

  MAC_MlmeSetReq(MAC_RX_ON_WHEN_IDLE, &ll_MACFalse);
}

void LL_McpsDataReq(uint8* data, uint8 dataLength, uint8 srcAddrMode, uint16 dstShortAddr, uint8 txOpt)
{
  macMcpsDataReq_t  *pData;

  if ((pData = MAC_McpsDataAlloc(dataLength, MAC_SEC_LEVEL_NONE, MAC_KEY_ID_MODE_NONE)) != NULL)
  {
    pData->mac.srcAddrMode = srcAddrMode;
    if(dstShortAddr == MAC_ADDR_USE_EXT){
      pData->mac.dstAddr.addrMode = SADDR_MODE_EXT;
      sAddrExtCpy(pData->mac.dstAddr.addr.extAddr, ll_devInfo.dstExtAddr);
    }else{
      pData->mac.dstAddr.addrMode = SADDR_MODE_SHORT;
      pData->mac.dstAddr.addr.shortAddr = dstShortAddr;
    }
    pData->mac.dstPanId = ll_devInfo.PanId;
    pData->mac.msduHandle = ll_devInfo.dataHandle++;
    if(ll_devInfo.dataHandle == LL_APPHDL_RESERVED) ll_devInfo.dataHandle++;
    pData->mac.txOptions = txOpt;

    osal_memcpy (pData->msdu.p, data, dataLength);

    LL_DisableSleep(pData->mac.msduHandle);

    MAC_McpsDataReq(pData);
    ll_devInfo.sendTime = osal_GetSystemClock();
  } else {
    SystemReset();
  }

}

void LL_HandleKeys(uint8 keys, uint8 shift)
{
  if ( keys & HAL_KEY_SW_1 ){
    LL_Restart();
  }
}

/* process data from child when listening and creating connection */
void LL_ProcessData(macCbackEvent_t* pData, uint32 timestamp)
{
  uint8 *p = pData->dataInd.msdu.p;
  uint8 len = pData->dataInd.msdu.len;
  ll_Data_hdr_t *hdr = (ll_Data_hdr_t*)p;

  switch(LL_state){
    case LL_STATE_LISTENING:
      if(hdr->type == LL_DATA_TYPE_ACTIVE_SCAN && pData->dataInd.mac.mpduLinkQuality > LL_LQI_SHRESHOLD){
        /* parent ack to child */
        ll_connect_child_time = osal_GetSystemClock() + LL_CONNECT_CHILD_DURATION;
        uint16 childaddr = pData->dataInd.mac.srcAddr.addr.shortAddr;
        hdr->type = LL_DATA_TYPE_ACTIVE_RSP;
        uint32 connect_delay = ll_connect_child_time - osal_GetSystemClock();
        osal_memcpy(((uint8*)&hdr->type) + 1, (uint8*)&connect_delay, sizeof(uint32));
        LL_McpsDataReq(p, len, SADDR_MODE_SHORT, childaddr, MAC_TXOPTION_ACK);

        /* change state to connecting */
        LL_state = LL_STATE_CONNECTING;
        ll_devInfo.ChildLQI = 0;
        LL_TurnOnRx(LL_TaskId);
        if(SUCCESS != osal_start_timerEx(LL_TaskId, LL_EVENT_CONNECTING_TIMEOUT, ll_connect_child_time - osal_GetSystemClock())){
          SystemReset();
        }
      }
      break;
    case LL_STATE_CONNECTING:
      if(hdr->type == LL_DATA_TYPE_ACTIVE_SCAN) {
        /* parent ack to child */
        uint16 childaddr = pData->dataInd.mac.srcAddr.addr.shortAddr;
        hdr->type = LL_DATA_TYPE_ACTIVE_RSP;
        uint32 connect_delay = ll_connect_child_time - osal_GetSystemClock();
        osal_memcpy(((uint8*)&hdr->type) + 1, (uint8*)&connect_delay, sizeof(uint32));
        LL_McpsDataReq(p, len, SADDR_MODE_SHORT, childaddr, MAC_TXOPTION_ACK);
      } else if (hdr->type == LL_DATA_TYPE_CONNECT_REQ) {
        /* choose the best LQI child */
        uint8 *data = (uint8*)(hdr + 1);
        uint8 minLqi = MIN(data[0], pData->dataInd.mac.mpduLinkQuality);
        if(minLqi > ll_devInfo.ChildLQI) {
          ll_devInfo.ChildShortAddr = pData->dataInd.mac.srcAddr.addr.shortAddr;
          ll_devInfo.ChildLQI = minLqi;
        }
      }
      break;
    default:
      break;
  }
}

void C2P_ProcessData(macCbackEvent_t *pData, uint32 timestamp)
{
  uint8 *p = pData->dataInd.msdu.p;
  uint8 len = pData->dataInd.msdu.len;
  ll_Data_hdr_t *hdr = (ll_Data_hdr_t*)p;

  switch(C2P_state){
    case C2P_STATE_UNCONNECTED:
      if (hdr->type == LL_DATA_TYPE_ACTIVE_RSP) {
        uint32 tmp320us = LL_GetTime320us();
        uint32 recvtime = osal_GetSystemClock();
        uint8 buf[LL_DATA_HEAD_LENGTH + 1] = {0};
        ll_Data_hdr_t *hdr2 = (ll_Data_hdr_t*)buf;
        hdr2->flag = 0;
        hdr2->type = LL_DATA_TYPE_CONNECT_REQ;
        hdr2->len = 1;
        uint8 *data = (uint8*)(hdr2 + 1);
        data[0] = pData->dataInd.mac.mpduLinkQuality;
        LL_McpsDataReq(buf, LL_DATA_HEAD_LENGTH + hdr2->len, SADDR_MODE_SHORT, pData->dataInd.mac.srcAddr.addr.shortAddr, MAC_TXOPTION_ACK);
        uint32 connect_delay;
        osal_memcpy((uint8*)&connect_delay, (uint8*)&hdr->type + 1, sizeof(uint32));

        tmp320us = tmp320us + MAX_BACKOFF_TIMER_COUNT - timestamp;
        if(tmp320us >= MAX_BACKOFF_TIMER_COUNT) tmp320us -= MAX_BACKOFF_TIMER_COUNT;
        recvtime -= tmp320us * 8 / 25;

        LL_TurnOffRx(C2P_TaskId);
        C2P_state = C2P_STATE_CONNECTING;
        if(SUCCESS != osal_start_timerEx(C2P_TaskId, C2P_EVENT_CONNECT_RSP_BEGIN, recvtime + connect_delay - osal_GetSystemClock() - 100)){
          SystemReset();
        }
      }
      break;
    case C2P_STATE_CONNECTING:
      if (hdr->type == LL_DATA_TYPE_CONNECT_RSP) {
        ll_devInfo.ParentShortAddr = pData->dataInd.mac.srcAddr.addr.shortAddr;
        C2P_StartData();
        LL_StartListen();
      }
      break;
    case C2P_STATE_CONNECTED:
#ifdef TWO_WAY_DATA
      if (hdr->type == LL_DATA_TYPE_DOWNSTREAM) {
        uint32 tmp320us = LL_GetTime320us();
        uint32 current = osal_GetSystemClock();
        tmp320us = tmp320us + MAX_BACKOFF_TIMER_COUNT - timestamp;
        if(tmp320us >= MAX_BACKOFF_TIMER_COUNT) tmp320us -= MAX_BACKOFF_TIMER_COUNT;

        ll_Downstream_Info.recvTime = current - (tmp320us * 8 / 25);

        LL_TurnOffRx(C2P_TaskId);
        ll_Downstream_Info.recvFailCount = 0;

        osal_memcpy(ll_Downstream_Info.dataBuf, p, len);

        ll_devInfo.lossParent = ll_bit_test(hdr->flag, LL_DATA_FLAG_LOSS_PARENT);
        if(ll_devInfo.lossParent){
          C2P_StopData(); // set unconnect to parent state
#ifdef LL_NODEROLE_COLLECTNODE
          C2C_StopData();
          osal_set_event(LL_TaskId, LL_EVENT_RESTART);
#else /* LL_NODEROLE_COLLECTNODE */
          if(C2C_state == C2C_STATE_UNCONNECTED) { //loss both parent and child
            LL_StopListen(); //it is listening possibly
            osal_set_event(LL_TaskId, LL_EVENT_RESTART);
          }
#endif /* LL_NODEROLE_COLLECTNODE */
        }

        uint16 recv_adj = hdr->period_adj;

        if (!ll_Downstream_Info.firstDataReceived) {
          ll_Downstream_Info.firstDataReceived = TRUE;
        }

        ll_Downstream_Info.adjustRecv = ll_Downstream_Info.recvTime;
        apply_adjust(ll_Downstream_Info.adjustRecv, recv_adj);
        ll_Downstream_Info.nextRecv = ll_Downstream_Info.adjustRecv + LL_DOWNSTREAM_TRANS_PERIOD;

        ll_Downstream_Info.dataReady = TRUE;

        uint32 rxon_time = ll_Downstream_Info.nextRecv - LL_DOWNSTREAM_TIMER_ERROR;
        uint32 rxoff_time = ll_Downstream_Info.nextRecv + LL_MAX_TRANS_TIME * LL_SEND_RETRIES;

        if(SUCCESS != osal_start_timerEx(C2P_TaskId, C2P_EVENT_RECV_DATA_BEGIN, rxon_time - osal_GetSystemClock())){
          SystemReset();
        }
        if(SUCCESS != osal_start_timerEx(C2P_TaskId, C2P_EVENT_RECV_DATA_TIMEOUT, rxoff_time - osal_GetSystemClock())){
          SystemReset();
        }
      }
      break;
#else /* TWO_WAY_DATA */
      if (hdr->type == LL_DATA_TYPE_UPSTREAM_ACK) {
        LL_TurnOffRx(C2P_TaskId);
        ll_devInfo.lossParent = ll_bit_test(hdr->flag, LL_DATA_FLAG_LOSS_PARENT);
        if(ll_devInfo.lossParent){
          ll_Upstream_Info.waitCycle = LL_ACK_CYCLE_DEFAULT + 2;
          C2P_StopData(); // set unconnect to parent state
#ifdef LL_NODEROLE_COLLECTNODE
          C2C_StopData();
          osal_set_event(LL_TaskId, LL_EVENT_RESTART);
#else /* LL_NODEROLE_COLLECTNODE */
          if(C2C_state == C2C_STATE_UNCONNECTED) { //loss both parent and child
            LL_StopListen(); //it is listening possibly
            osal_set_event(LL_TaskId, LL_EVENT_RESTART);
          }
#endif /* LL_NODEROLE_COLLECTNODE */
        }
      }
      break;
#endif /* TWO_WAY_DATA */
    default:
      break;
  }
}
void C2C_ProcessData(macCbackEvent_t *pData, uint32 timestamp)
{
  uint8 *p = pData->dataInd.msdu.p;
  uint8 len = pData->dataInd.msdu.len;
  ll_Data_hdr_t *hdr = (ll_Data_hdr_t*)p;

  if (C2C_state == C2C_STATE_CONNECTED) {
    if (hdr->type == LL_DATA_TYPE_UPSTREAM) {
      uint32 tmp320us = LL_GetTime320us();
      uint32 current = osal_GetSystemClock();
#ifndef TWO_WAY_DATA
      if(ll_bit_test(hdr->flag, LL_DATA_FLAG_REQUEST_ACK)) {/* send ack */
        ll_Data_hdr_t *hdr2 = (ll_Data_hdr_t*)ll_Upstream_Info.dataBuf;
        if(ll_devInfo.lossParent){
          ll_bit_set(hdr2->flag, LL_DATA_FLAG_LOSS_PARENT);
          osal_set_event(LL_TaskId, LL_EVENT_RESTART);
        }
        hdr2->type = LL_DATA_TYPE_UPSTREAM_ACK;
        hdr2->len = 0;
        LL_McpsDataReq(ll_Upstream_Info.dataBuf, LL_DATA_HEAD_LENGTH + hdr2->len, SADDR_MODE_SHORT, ll_devInfo.ChildShortAddr, MAC_TXOPTION_ACK);

        ll_Upstream_Info.ackCycle = LL_ACK_CYCLE_DEFAULT;
      } else if(ll_devInfo.lossParent && ll_Upstream_Info.waitCycle-- == 0) {
        osal_set_event(LL_TaskId, LL_EVENT_RESTART);
      }
#endif
      if(ll_Upstream_Info.recvFinish){
        ll_Upstream_Info.recvFinish = FALSE;
        ll_Upstream_Info.dataReady = FALSE;
        ll_Upstream_Info.partialRecv = TRUE;
        ll_Upstream_Info.recvDSN++;

        ll_Upstream_Info.totalRecv = hdr->total_frags;
        ll_Upstream_Info.totalMask = (1 << hdr->total_frags) - 1;
        ll_Upstream_Info.recvMask = 0;

        tmp320us = tmp320us + MAX_BACKOFF_TIMER_COUNT - timestamp;
        if(tmp320us >= MAX_BACKOFF_TIMER_COUNT) tmp320us -= MAX_BACKOFF_TIMER_COUNT;

        ll_Upstream_Info.recvTime = current - (tmp320us * 8 / 25);

        uint16 recv_adj = hdr->period_adj;

        if (!ll_Upstream_Info.firstDataReceived) {
          ll_Upstream_Info.firstDataReceived = TRUE;
          if(SUCCESS != osal_start_timerEx(C2C_TaskId, C2C_EVENT_RECV_DATA_TIMEOUT, ll_Upstream_Info.recvTime + LL_MAX_TRANS_TIME * LL_SEND_RETRIES - osal_GetSystemClock())){
            SystemReset();
          }
        }

        ll_Upstream_Info.adjustRecv = ll_Upstream_Info.recvTime;
        apply_adjust(ll_Upstream_Info.adjustRecv, recv_adj);

        ll_Upstream_Info.nextRecv = ll_Upstream_Info.adjustRecv + LL_UPSTREAM_TRANS_PERIOD;

        uint32 rxon_time = ll_Upstream_Info.nextRecv - LL_UPSTREAM_TIMER_ERROR;

        if(SUCCESS != osal_start_timerEx(C2C_TaskId, C2C_EVENT_RECV_DATA_BEGIN, rxon_time - osal_GetSystemClock())){
          SystemReset();
        }
      }

      ll_Upstream_Info.recvFailCount = 0;

      ll_Upstream_Info.recvMask |= (1 << hdr->current_frag);
      osal_memcpy(ll_Upstream_Info.dataBuf + hdr->current_frag * LL_DATA_BUFFER_LENGTH, p, len);

      if(ll_Upstream_Info.recvMask == ll_Upstream_Info.totalMask){
        ll_Upstream_Info.dataReady = TRUE;
        ll_Upstream_Info.recvFinish = TRUE;
        LL_TurnOffRx(C2C_TaskId);
        uint32 rxoff_time = ll_Upstream_Info.nextRecv + LL_MAX_TRANS_TIME * LL_SEND_RETRIES;
        if(SUCCESS != osal_start_timerEx(C2C_TaskId, C2C_EVENT_RECV_DATA_TIMEOUT, rxoff_time - osal_GetSystemClock())){
          SystemReset();
        }
      }
    }
  }
}

void LL_HandleDataCnf(uint8 handle, uint8 status, uint32 timestamp)
{
  if (handle == ll_connect_child_handle) {
    ll_connect_child_handle = LL_APPHDL_RESERVED;
    if (LL_state == LL_STATE_CONNECTING) {
      if (status == MAC_SUCCESS) {
        /* stop listening and start C2C */
        LL_StopListen();
        C2C_StartData();
      } else {
        if (ll_connect_child_fail_count++ < LL_CONNECT_CHILD_FAIL_MAX) {
          /* try more times */
          LL_ConnectChild();
        } else {
          /* reset for listening */
          LL_StartListen();
        }
      }
    }

    return;
  }

  for(uint8 i = 0; i < ll_Upstream_Info.totalSend; ++i){
    if (handle == ll_Upstream_Info.sendHandle[i]) {
      ll_Upstream_Info.sendHandle[i] = LL_APPHDL_RESERVED;

      if (C2P_state == C2P_STATE_CONNECTED) {
        if (status == MAC_SUCCESS) {
          ll_Upstream_Info.sendFailCount = 0;
          if(!ll_Upstream_Info.partialSend){
            ll_Upstream_Info.partialSend = TRUE;

            uint32 conftime320us = timestamp + MAX_BACKOFF_TIMER_COUNT - ll_Upstream_Info.sendTime320us;
            if(conftime320us >= MAX_BACKOFF_TIMER_COUNT) conftime320us -= MAX_BACKOFF_TIMER_COUNT;
            uint32 nextsend = ll_Upstream_Info.sendTime + ll_Upstream_Info.adjustPeriod + conftime320us * 8 / 25;
            if(SUCCESS != osal_start_timerEx(C2P_TaskId, C2P_EVENT_SEND_DATA, nextsend - osal_GetSystemClock())){
              SystemReset();
            }
          }
        } else {
          if (!ll_Upstream_Info.dataReady &&
              osal_GetSystemClock() - ll_Upstream_Info.sendTime < ll_Upstream_Info.retryTimeout /*&&
              osal_GetSystemClock() - ll_devInfo.sendTime > LL_MIN_DATA_SEND_SPAN*/) {
            ll_Upstream_Info.sendHandle[i] = ll_devInfo.dataHandle;
            uint8 *p = ll_Upstream_Info.dataBuf + i * LL_DATA_BUFFER_LENGTH;
            if(ll_Upstream_Info.dummyData) p = ll_Upstream_Info.dummyBuf;
            ll_Data_hdr_t *hdr = (ll_Data_hdr_t*)p;
            LL_McpsDataReq(p, LL_DATA_HEAD_LENGTH + hdr->len, SADDR_MODE_SHORT, ll_devInfo.ParentShortAddr, MAC_TXOPTION_ACK);
          } else if (ll_Upstream_Info.sendFailCount++ > LL_SEND_FAIL_COUNT_MAX) {
            ll_devInfo.lossParent = TRUE;
            ll_Upstream_Info.waitCycle = LL_ACK_CYCLE_DEFAULT + 2;
            C2P_StopData();
#ifdef LL_NODEROLE_COLLECTNODE
            C2C_StopData();
            osal_set_event(LL_TaskId, LL_EVENT_RESTART);
#else
            if(C2C_state == C2C_STATE_UNCONNECTED) {
              LL_StopListen();
              osal_set_event(LL_TaskId, LL_EVENT_RESTART);
            }
#endif
          } else {
            ll_Upstream_Info.sendSuccess &= (~(1 << i));
          }
        }

        if(!ll_Upstream_Info.sendSuccess){
          /* reset fixed send period */
          if(SUCCESS != osal_start_timerEx(C2P_TaskId, C2P_EVENT_SEND_DATA, ll_Upstream_Info.sendTime + LL_UPSTREAM_TRANS_PERIOD - osal_GetSystemClock())){
            SystemReset();
          }
        }
      }

      return;
    }
  }

#ifdef TWO_WAY_DATA
  if (handle == ll_Downstream_Info.sendHandle[0]) {
    ll_Downstream_Info.sendHandle[0] = LL_APPHDL_RESERVED;
    if (C2C_state == C2C_STATE_CONNECTED) {
      if (status == MAC_SUCCESS) {
        if(ll_devInfo.sendLossParent) { //have notify child the loss parent
          osal_set_event(LL_TaskId, LL_EVENT_RESTART);
        }
        ll_Downstream_Info.sendFailCount = 0;

        uint32 conftime320us = timestamp + MAX_BACKOFF_TIMER_COUNT - ll_Downstream_Info.sendTime320us;
        if(conftime320us >= MAX_BACKOFF_TIMER_COUNT) conftime320us -= MAX_BACKOFF_TIMER_COUNT;
        uint32 nextsend = ll_Downstream_Info.sendTime + ll_Downstream_Info.adjustPeriod + conftime320us * 8 / 25;
        if(SUCCESS != osal_start_timerEx(C2C_TaskId, C2C_EVENT_SEND_DATA, nextsend - osal_GetSystemClock())){
          SystemReset();
        }
      } else {
        if (!ll_Downstream_Info.dataReady &&
            osal_GetSystemClock() - ll_Downstream_Info.sendTime < ll_Downstream_Info.retryTimeout /*&&
            osal_GetSystemClock() - ll_devInfo.sendTime > LL_MIN_DATA_SEND_SPAN*/) {
          ll_Downstream_Info.sendHandle[0] = ll_devInfo.dataHandle;
          ll_Data_hdr_t *hdr = (ll_Data_hdr_t*)ll_Downstream_Info.dataBuf;
          LL_McpsDataReq(ll_Downstream_Info.dataBuf, LL_DATA_HEAD_LENGTH + hdr->len, SADDR_MODE_SHORT, ll_devInfo.ChildShortAddr, MAC_TXOPTION_ACK);
        } else if (ll_Downstream_Info.sendFailCount++ > LL_SEND_FAIL_COUNT_MAX){
          if(ll_devInfo.sendLossParent) { //loss both parent and child
            osal_set_event(LL_TaskId, LL_EVENT_RESTART);
          } else { // only loss child
            C2C_StopData();
            LL_StartListen();
          }
        } else {
          /* reset fixed send period */
          if(SUCCESS != osal_start_timerEx(C2C_TaskId, C2C_EVENT_SEND_DATA, ll_Downstream_Info.sendTime + LL_DOWNSTREAM_TRANS_PERIOD - osal_GetSystemClock())){
            SystemReset();
          }
        }
      }
    }

    return;
  }
#endif /* TWO_WAY_DATA */
}

void LL_TurnOnRx(uint8 taskId)
{
  ll_Rxon_bitmap |= (uint16)1 << taskId;
  MAC_MlmeSetReq(MAC_RX_ON_WHEN_IDLE, &ll_MACTrue);
#ifdef LL_NODEROLE_ROUTERNODE
  uint8 leds = (C2P_state == C2P_STATE_CONNECTED) ? HAL_LED_3 : HAL_LED_1;
#endif

#ifdef LL_NODEROLE_REPORTNODE
    uint8 leds = (C2C_state == C2C_STATE_CONNECTED) ? HAL_LED_3 : HAL_LED_1;
#endif
  HalLedSet(leds, HAL_LED_MODE_ON);
  if(!HalAdcCheckVdd(20))
    HalLedSet(HAL_LED_2, HAL_LED_MODE_ON);
}

void LL_TurnOffRx(uint8 taskId)
{
  ll_Rxon_bitmap &= ~((uint16)1 << taskId);
  if (ll_Rxon_bitmap == 0){
//#ifdef LL_NODEROLE_ROUTERNODE
    MAC_MlmeSetReq(MAC_RX_ON_WHEN_IDLE, &ll_MACFalse);
//#endif
    HalLedSet(HAL_LED_ALL, HAL_LED_MODE_OFF);
  }
}

uint16 C2P_ProcessEvent(uint8 taskId, uint16 events)
{
  if (events & SYS_EVENT_MSG) {
    uint8* pMsg;
    while ((pMsg = osal_msg_receive(C2P_TaskId)) != NULL) {
      osal_msg_deallocate(pMsg);
    }
    return events ^ SYS_EVENT_MSG;
  }

  if (events & C2P_EVENT_ACTIVE_SCAN) {
    if (C2P_state == C2P_STATE_UNCONNECTED) {
      if(SUCCESS != osal_start_timerEx(C2P_TaskId, C2P_EVENT_ACTIVE_SCAN, LL_ACTIVE_SCAN_PERIOD)){
        SystemReset();
      }

      uint8 buf[LL_DATA_HEAD_LENGTH] = {0};
      ll_Data_hdr_t *hdr = (ll_Data_hdr_t*)buf;
      hdr->len = 0;
      hdr->flag = 0;
      hdr->type = LL_DATA_TYPE_ACTIVE_SCAN;
      LL_McpsDataReq(buf, LL_DATA_HEAD_LENGTH + hdr->len, SADDR_MODE_SHORT, MAC_SHORT_ADDR_BROADCAST, 0);
      if((ll_devInfo.probeParentTimes++ % LL_LISTEN_ACTIVE_RSP_CYCLE) == 0){
        LL_TurnOnRx(C2P_TaskId);
        if(SUCCESS != osal_start_timerEx(C2P_TaskId, C2P_EVENT_ACTIVE_SCAN_TIMEOUT, LL_LISTEN_ACTIVE_RSP_DURATION)){
          SystemReset();
        }
      }
    }
    return events ^ C2P_EVENT_ACTIVE_SCAN;
  }

  if (events & C2P_EVENT_ACTIVE_SCAN_TIMEOUT) {
    if (C2P_state == C2P_STATE_UNCONNECTED) {
      LL_TurnOffRx(C2P_TaskId);
    }
    return events ^ C2P_EVENT_ACTIVE_SCAN_TIMEOUT;
  }

  if (events & C2P_EVENT_CONNECT_RSP_BEGIN) {
    if (C2P_state == C2P_STATE_CONNECTING) {
      LL_TurnOnRx(C2P_TaskId);
      osal_start_timerEx(C2P_TaskId, C2P_EVENT_CONNECT_RSP_TIMEOUT, 300);
    }
    return events ^ C2P_EVENT_CONNECT_RSP_BEGIN;
  }

  if (events & C2P_EVENT_CONNECT_RSP_TIMEOUT) {
    if (C2P_state == C2P_STATE_CONNECTING) {
      LL_TurnOffRx(C2P_TaskId);
      LL_Restart();
    }
    return events ^ C2P_EVENT_CONNECT_RSP_TIMEOUT;
  }

  /* send upstream data */
  if (events & C2P_EVENT_SEND_DATA) {
    if (C2P_state == C2P_STATE_CONNECTED) {
      if(ll_Upstream_Info.firstSend){
        ll_Upstream_Info.firstSend = FALSE;
        ll_Upstream_Info.sendTime320us = LL_GetTime320us();
        ll_Upstream_Info.sendTime = osal_GetSystemClock();
      }

#ifdef LL_SEND_DATA_INTERVAL
      uint32 current = osal_GetSystemClock();
      if(current - ll_devInfo.sendTime < LL_MIN_DATA_SEND_SPAN){
        if(SUCCESS != osal_start_timerEx(C2P_TaskId, C2P_EVENT_SEND_DATA, LL_MIN_DATA_SEND_SPAN - (current - ll_devInfo.sendTime))){
          SystemReset();
        }
        return events ^ C2P_EVENT_SEND_DATA;
      }
#endif

      ll_Upstream_Info.firstSend = TRUE;

      uint32 basetime = osal_GetSystemClock() - LL_SEND_DATA_DELAY;

      uint16 next_adj;
      make_adjust(next_adj, basetime, ll_Upstream_Info.adjustRecv);
      /* make sure next node have send this frame before recv next frame */
      if(LL_time_after(basetime, ll_Upstream_Info.adjustRecv) &&
        basetime - ll_Upstream_Info.adjustRecv > LL_UPSTREAM_TRANS_PERIOD - LL_MAX_TRANS_TIME * LL_SEND_RETRIES - LL_SEND_DATA_DELAY){
        uint32 base = LL_UPSTREAM_TRANS_PERIOD;
        uint32 real = LL_MAX_TRANS_TIME * LL_SEND_RETRIES + LL_SEND_DATA_DELAY;
        make_adjust(next_adj, base, real);
      }

#ifdef LL_NODEROLE_REPORTNODE
      if(!ll_Upstream_Info.partialRecv) next_adj = 0;
      LL_DataConsume(&ll_Upstream_Info, LL_DATA_TYPE_UPSTREAM);
      ll_Upstream_Info.totalSend = 1;
      ll_Upstream_Info.currSend = 0;
#else /* ROUTER OR TAIL */

      ll_Data_hdr_t *hdr = (ll_Data_hdr_t*)ll_Upstream_Info.dataBuf;
      if(!ll_Upstream_Info.dataReady) {
        hdr = (ll_Data_hdr_t*)ll_Upstream_Info.dummyBuf;
        hdr->flag = 0;
        ll_bit_set(hdr->flag, LL_DATA_FLAG_INVALID);
        ll_Upstream_Info.dummyData = TRUE;
        ll_Upstream_Info.totalSend = 1;
        ll_Upstream_Info.currSend = 0;
        if(!ll_Upstream_Info.partialRecv){
          next_adj = 0;
        }
        hdr->type = LL_DATA_TYPE_UPSTREAM;
        hdr->len = 0;
        hdr->hops = 0;
      } else {
        ll_Upstream_Info.dummyData = FALSE;
        ll_Upstream_Info.totalSend = ll_Upstream_Info.totalRecv;
        ll_Upstream_Info.currSend = 0;
      }

      hdr->period_adj = next_adj;
      hdr->hops++;
      hdr->total_frags = ll_Upstream_Info.totalSend;
      hdr->current_frag = ll_Upstream_Info.currSend;

#ifndef TWO_WAY_DATA
      if(ll_Upstream_Info.ackCycle-- == 0) {
        ll_Upstream_Info.ackCycle = LL_ACK_CYCLE_DEFAULT;
        ll_bit_set(hdr->flag, LL_DATA_FLAG_REQUEST_ACK);
        LL_TurnOnRx(C2P_TaskId);
        if(SUCCESS != osal_start_timerEx(C2P_TaskId, C2P_EVENT_WAIT_ACK_TIMEOUT, LL_WAIT_ACK_DURATION)){
          SystemReset();
        }
      }
#endif

      ll_Upstream_Info.sendHandle[0] = ll_devInfo.dataHandle;
      LL_McpsDataReq((uint8*)hdr, LL_DATA_HEAD_LENGTH + hdr->len, SADDR_MODE_SHORT, ll_devInfo.ParentShortAddr, MAC_TXOPTION_ACK);
#endif /* LL_NODEROLE_REPORTNODE */

      ll_Upstream_Info.dataReady = FALSE;
      ll_Upstream_Info.partialRecv = FALSE;
      ll_Upstream_Info.partialSend = FALSE;
      ll_Upstream_Info.sendSuccess = (1<<ll_Upstream_Info.currSend);
      ll_Upstream_Info.sendDSN = ll_Upstream_Info.recvDSN;
      ll_Upstream_Info.retryTimeout = LL_MAX_TRANS_TIME * LL_SEND_RETRIES - (osal_GetSystemClock() - ll_Upstream_Info.sendTime);

      ll_Upstream_Info.adjustPeriod = LL_UPSTREAM_TRANS_PERIOD;
      apply_adjust(ll_Upstream_Info.adjustPeriod, next_adj);

#ifdef LL_NODEROLE_REPORTNODE
      uint32 nextsend = ll_Upstream_Info.sendTime + ll_Upstream_Info.adjustPeriod;
      if(SUCCESS != osal_start_timerEx(C2P_TaskId, C2P_EVENT_SEND_DATA, nextsend - osal_GetSystemClock())){
        SystemReset();
      }
#endif

      if(ll_Upstream_Info.currSend < ll_Upstream_Info.totalSend - 1) {
        osal_set_event(C2P_TaskId, C2P_EVENT_SEND_DATA_CONTINUE);
      }
    }
    return events ^ C2P_EVENT_SEND_DATA;
  }


  if (events & C2P_EVENT_SEND_DATA_CONTINUE) {
    if (C2P_state == C2P_STATE_CONNECTED) {

#ifdef LL_SEND_DATA_INTERVAL
      uint32 current = osal_GetSystemClock();
      if(current - ll_devInfo.sendTime < LL_MIN_DATA_SEND_SPAN){
        if(SUCCESS != osal_start_timerEx(C2P_TaskId, C2P_EVENT_SEND_DATA_CONTINUE, LL_MIN_DATA_SEND_SPAN - (current - ll_devInfo.sendTime))){
          SystemReset();
        }
        return events ^ C2P_EVENT_SEND_DATA_CONTINUE;
      }
#endif

      if(ll_Upstream_Info.sendDSN == ll_Upstream_Info.recvDSN){
        ll_Upstream_Info.currSend++;
        ll_Data_hdr_t *hdr = (ll_Data_hdr_t*)(ll_Upstream_Info.dataBuf + LL_DATA_BUFFER_LENGTH * ll_Upstream_Info.currSend);
        uint8 len = hdr->len;
        osal_memcpy(hdr, ll_Upstream_Info.dataBuf, LL_DATA_HEAD_LENGTH);
        hdr->current_frag = ll_Upstream_Info.currSend;
        hdr->len = len;

        ll_Upstream_Info.sendHandle[ll_Upstream_Info.currSend] = ll_devInfo.dataHandle;
        LL_McpsDataReq((uint8*)hdr, LL_DATA_HEAD_LENGTH + hdr->len, SADDR_MODE_SHORT, ll_devInfo.ParentShortAddr, MAC_TXOPTION_ACK);

        ll_Upstream_Info.sendSuccess |= (1 << ll_Upstream_Info.currSend);

        if(ll_Upstream_Info.currSend < ll_Upstream_Info.totalSend - 1) {
          osal_set_event(C2P_TaskId, C2P_EVENT_SEND_DATA_CONTINUE);
        }
      }
    }
    return events ^ C2P_EVENT_SEND_DATA_CONTINUE;
  }

#ifdef TWO_WAY_DATA
  /* turn on rx for receiving downstream data */
  if (events & C2P_EVENT_RECV_DATA_BEGIN) {
    if (C2P_state == C2P_STATE_CONNECTED){
#ifdef LL_NODEROLE_REPORTNODE
      LL_TurnOffRx(C2P_TaskId);
      LL_DataProduce(&ll_Downstream_Info, LL_DATA_TYPE_DOWNSTREAM);
      if(SUCCESS != osal_start_timerEx(C2P_TaskId, C2P_EVENT_RECV_DATA_BEGIN, LL_DOWNSTREAM_TRANS_PERIOD)){
        SystemReset();
      }
#else
      LL_TurnOnRx(C2P_TaskId);
#endif /* LL_NODEROLE_REPORTNODE */
    }
    return events ^ C2P_EVENT_RECV_DATA_BEGIN;
  }

  /* receive downstream data timeout */
  /* this will never happen if data recv */
  if (events & C2P_EVENT_RECV_DATA_TIMEOUT) {
    if (C2P_state == C2P_STATE_CONNECTED) {
      LL_TurnOffRx(C2P_TaskId);
      ll_Downstream_Info.recvFailCount++;
      ll_Downstream_Info.nextRecv += LL_DOWNSTREAM_TRANS_PERIOD;
      if(SUCCESS != osal_start_timerEx(C2P_TaskId, C2P_EVENT_RECV_DATA_BEGIN, ll_Downstream_Info.nextRecv - LL_DOWNSTREAM_TIMER_ERROR * ll_Downstream_Info.recvFailCount - osal_GetSystemClock())){
        SystemReset();
      }
      if(SUCCESS != osal_start_timerEx(C2P_TaskId, C2P_EVENT_RECV_DATA_TIMEOUT, ll_Downstream_Info.nextRecv + LL_MAX_TRANS_TIME * LL_SEND_RETRIES + LL_DOWNSTREAM_TIMER_ERROR * ll_Downstream_Info.recvFailCount - osal_GetSystemClock())){
        SystemReset();
      }
      if(ll_Downstream_Info.recvFailCount > LL_RECV_FAIL_COUNT_MAX){
        ll_devInfo.lossParent = TRUE;
        C2P_StopData();
#ifdef LL_NODEROLE_COLLECTNODE
        C2C_StopData();
        osal_set_event(LL_TaskId, LL_EVENT_RESTART);
#else
        if(C2C_state == C2C_STATE_UNCONNECTED){
          LL_StopListen();
          osal_set_event(LL_TaskId, LL_EVENT_RESTART);
        }
#endif
      }
    }
    return events ^ C2P_EVENT_RECV_DATA_TIMEOUT;
  }

  /* don't recv first data from parent, reset*/
  if (events & C2P_EVENT_START_TIMEOUT) {
    if (!ll_Downstream_Info.firstDataReceived) {
      LL_Restart();
    }
    return events ^ C2P_EVENT_START_TIMEOUT;
  }

#else /* TWO_WAY_DATA */
  if (events & C2P_EVENT_WAIT_ACK_TIMEOUT) {
    if(C2P_state == C2P_STATE_CONNECTED){
      LL_TurnOffRx(C2P_TaskId);
    }
    return events ^ C2P_EVENT_WAIT_ACK_TIMEOUT;
  }

#endif /* TWO_WAY_DATA */
  return 0;
}

uint16 C2C_ProcessEvent(uint8 taskId, uint16 events)
{
  if (events & SYS_EVENT_MSG) {
    uint8* pMsg;
    while ((pMsg = osal_msg_receive(C2C_TaskId)) != NULL) {
      osal_msg_deallocate(pMsg);
    }
    return events ^ SYS_EVENT_MSG;
  }

#ifdef TWO_WAY_DATA
  if (events & C2C_EVENT_SEND_DATA) {
    if (C2C_state == C2C_STATE_CONNECTED){
      if(ll_Downstream_Info.firstSend) {
        ll_Downstream_Info.firstSend = FALSE;
        ll_Downstream_Info.sendTime320us = LL_GetTime320us();
        ll_Downstream_Info.sendTime = osal_GetSystemClock();
      }

#ifdef LL_SEND_DATA_INTERVAL
      uint32 current = osal_GetSystemClock();
      if(current - ll_devInfo.sendTime < LL_MIN_DATA_SEND_SPAN){
        if(SUCCESS != osal_start_timerEx(C2C_TaskId, C2C_EVENT_SEND_DATA, LL_MIN_DATA_SEND_SPAN - (current - ll_devInfo.sendTime))){
          SystemReset();
        }
        return events ^ C2C_EVENT_SEND_DATA;
      }
#endif

      ll_Downstream_Info.firstSend = TRUE;

      uint32 basetime = osal_GetSystemClock() - LL_SEND_DATA_DELAY;

      uint16 next_adj;
      make_adjust(next_adj, basetime, ll_Downstream_Info.adjustRecv);
      /* make sure next node have send this frame before recv next frame */
      if(LL_time_after(basetime, ll_Downstream_Info.adjustRecv) &&
         basetime - ll_Downstream_Info.adjustRecv > LL_DOWNSTREAM_TRANS_PERIOD - LL_MAX_TRANS_TIME * LL_SEND_RETRIES - LL_SEND_DATA_DELAY){
         uint32 base = LL_DOWNSTREAM_TRANS_PERIOD;
         uint32 real = LL_MAX_TRANS_TIME * LL_SEND_RETRIES + LL_SEND_DATA_DELAY;
         make_adjust(next_adj, base, real);
      }

#ifdef LL_NODEROLE_COLLECTNODE
      if(!ll_Downstream_Info.dataReady) next_adj = 0;
      LL_DataConsume(&ll_Downstream_Info, LL_DATA_TYPE_DOWNSTREAM);
#else

      ll_Data_hdr_t *hdr = (ll_Data_hdr_t*)ll_Downstream_Info.dataBuf;

      if(!ll_Downstream_Info.dataReady) {
        ll_bit_set(hdr->flag, LL_DATA_FLAG_INVALID);
        next_adj = 0;
        hdr->type = LL_DATA_TYPE_DOWNSTREAM;
        hdr->len = 0;
        hdr->hops = 0;
      }

      hdr->period_adj = next_adj;

      if(ll_devInfo.lossParent) {
        ll_bit_set(hdr->flag, LL_DATA_FLAG_LOSS_PARENT);
        ll_devInfo.sendLossParent = TRUE;
      }

      hdr->hops++;

      ll_Downstream_Info.sendHandle[0] = ll_devInfo.dataHandle;
      LL_McpsDataReq((uint8*)hdr, LL_DATA_HEAD_LENGTH + hdr->len, SADDR_MODE_SHORT, ll_devInfo.ChildShortAddr, MAC_TXOPTION_ACK);
#endif /* LL_NODEROLE_COLLECTNODE */

      ll_Downstream_Info.dataReady = FALSE;
      ll_Downstream_Info.retryTimeout = LL_MAX_TRANS_TIME * LL_SEND_RETRIES - (osal_GetSystemClock() - ll_Downstream_Info.sendTime);

      ll_Downstream_Info.adjustPeriod = LL_DOWNSTREAM_TRANS_PERIOD;
      apply_adjust(ll_Downstream_Info.adjustPeriod, next_adj);

#ifdef LL_NODEROLE_COLLECTNODE
      uint32 nextsend = ll_Downstream_Info.sendTime + ll_Downstream_Info.adjustPeriod;
      if(SUCCESS != osal_start_timerEx(C2C_TaskId, C2C_EVENT_SEND_DATA, nextsend - osal_GetSystemClock())){
        SystemReset();
      }
#endif
    }
    return events ^ C2C_EVENT_SEND_DATA;
  }
#endif /* TWO_WAY_DATA */


  if (events & C2C_EVENT_RECV_DATA_BEGIN) {
    if (C2C_state == C2C_STATE_CONNECTED) {
#ifdef LL_NODEROLE_COLLECTNODE
      LL_TurnOffRx(C2C_TaskId);
      LL_DataProduce(&ll_Upstream_Info, LL_DATA_TYPE_UPSTREAM);
      if(SUCCESS != osal_start_timerEx(C2C_TaskId, C2C_EVENT_RECV_DATA_BEGIN, LL_UPSTREAM_TRANS_PERIOD)){
        SystemReset();
      }
#else
      LL_TurnOnRx(C2C_TaskId);
#endif /* LL_NODEROLE_COLLECTNODE */
    }
    return events ^ C2C_EVENT_RECV_DATA_BEGIN;
  }

  if (events & C2C_EVENT_RECV_DATA_TIMEOUT) {
    if (C2C_state == C2C_STATE_CONNECTED) {
      LL_TurnOffRx(C2C_TaskId);
      ll_Upstream_Info.recvFailCount++;
      if(ll_Upstream_Info.recvFinish){
        ll_Upstream_Info.nextRecv += LL_UPSTREAM_TRANS_PERIOD;
        if(SUCCESS != osal_start_timerEx(C2C_TaskId, C2C_EVENT_RECV_DATA_BEGIN, ll_Upstream_Info.nextRecv - LL_UPSTREAM_TIMER_ERROR * ll_Upstream_Info.recvFailCount - osal_GetSystemClock())){
          SystemReset();
        }
      }

      ll_Upstream_Info.recvFinish = TRUE;
      if(SUCCESS != osal_start_timerEx(C2C_TaskId, C2C_EVENT_RECV_DATA_TIMEOUT, ll_Upstream_Info.nextRecv + LL_MAX_TRANS_TIME * LL_SEND_RETRIES - osal_GetSystemClock())){
        SystemReset();
      }
      if(ll_Upstream_Info.recvFailCount > LL_RECV_FAIL_COUNT_MAX){
        if(ll_devInfo.lossParent){
          osal_set_event(LL_TaskId, LL_EVENT_RESTART);
        } else {
          C2C_StopData();
          LL_StartListen();
        }
      } else if(ll_devInfo.lossParent && ll_Upstream_Info.waitCycle-- == 0) {
        osal_set_event(LL_TaskId, LL_EVENT_RESTART);
      }
    }
    return events ^ C2C_EVENT_RECV_DATA_TIMEOUT;
  }

  /* can't recv first data from child, disconnect and start listening */
  if (events & C2C_EVENT_START_TIMEOUT) {
    if (!ll_Upstream_Info.firstDataReceived) {
      C2C_StopData();
      LL_StartListen();
    }
    return events ^ C2C_EVENT_START_TIMEOUT;
  }

  return 0;
}

void LL_ConnectChild()
{
  uint8 buf[LL_DATA_HEAD_LENGTH] = {0};
  ll_Data_hdr_t *hdr = (ll_Data_hdr_t*)buf;
  hdr->flag = 0;
  hdr->type = LL_DATA_TYPE_CONNECT_RSP;
  hdr->len = 0;
  hdr->dsn = ll_devInfo.dataHandle;
  ll_connect_child_handle = ll_devInfo.dataHandle;
  LL_McpsDataReq(buf, LL_DATA_HEAD_LENGTH + hdr->len, SADDR_MODE_SHORT, ll_devInfo.ChildShortAddr, MAC_TXOPTION_ACK);
}

void C2C_StartData()
{
  //change state
  C2C_state = C2C_STATE_CONNECTED;

  //reset upstream info
  ll_Upstream_Info.dataReady = FALSE;
  ll_Upstream_Info.partialRecv = FALSE;
  ll_Upstream_Info.recvFinish = TRUE;
  ll_Upstream_Info.recvFailCount = 0;
  ll_Upstream_Info.firstDataReceived = FALSE;

  //trun on rx for first data from child
  LL_TurnOnRx(C2C_TaskId);
  if(SUCCESS != osal_start_timerEx(C2C_TaskId, C2C_EVENT_START_TIMEOUT, 4 * LL_UPSTREAM_TRANS_PERIOD)){
    SystemReset();
  }

  //trigger collect node to produce data
  osal_set_event(C2C_TaskId, C2C_EVENT_RECV_DATA_BEGIN);

#ifdef TWO_WAY_DATA
  //reset downstream info
  ll_Downstream_Info.sendFailCount = 0;
  ll_Downstream_Info.firstSend = TRUE;
  for(uint8 i = 0; i < LL_MAX_DATA_FRAGMENTS; ++i){
    ll_Downstream_Info.sendHandle[i] = LL_APPHDL_RESERVED;
  }

  if(SUCCESS != osal_start_timerEx(C2C_TaskId, C2C_EVENT_SEND_DATA, LL_SEND_DATA_DELAY)){
    SystemReset();
  }
#endif

}

void C2C_StopData()
{
  LL_TurnOffRx(C2C_TaskId);
  C2C_state = C2C_STATE_UNCONNECTED;
#ifdef TWO_WAY_DATA
  for(uint8 i = 0; i < LL_MAX_DATA_FRAGMENTS; ++i){
    ll_Downstream_Info.sendHandle[i] = LL_APPHDL_RESERVED;
  }
#endif
}

void LL_StartListen()
{
  LL_TurnOffRx(LL_TaskId);
#ifdef LL_NODEROLE_COLLECTNODE
  LL_StopListen();
  C2C_StartData();
#else
  ll_connect_child_fail_count = 0;
  ll_connect_child_handle = LL_APPHDL_RESERVED;
  ll_devInfo.listenChildPeriod = LL_LISTEN_CHILD_PERIOD_0;
  LL_state = LL_STATE_LISTENING;
  if(SUCCESS != osal_start_timerEx(LL_TaskId, LL_EVENT_LISTEN_CHILD, LL_START_LISTEN_CHILD_DELAY)){
    SystemReset();
  }
#endif /* LL_NODEROLE_COLLECTNODE */
}

void LL_StopListen()
{
  LL_state = LL_STATE_STOP_LISTEN;
  ll_connect_child_handle = LL_APPHDL_RESERVED;
  LL_TurnOffRx(LL_TaskId);
}

void C2P_StartActiveScan()
{
  C2P_state = C2P_STATE_UNCONNECTED;
  if(SUCCESS != osal_start_timerEx(C2P_TaskId, C2P_EVENT_ACTIVE_SCAN, LL_START_ACTIVE_SCAN_DELAY)){
    SystemReset();
  }
}


void C2P_StartData()
{
  //change state
  C2P_state = C2P_STATE_CONNECTED;

  //turn off rx which may on while connecting
  LL_TurnOffRx(C2P_TaskId);

#ifdef TWO_WAY_DATA
  //reset downstream info
  ll_Downstream_Info.dataReady = FALSE;
  ll_Downstream_Info.partialRecv = FALSE;
  ll_Downstream_Info.recvFinish = TRUE;
  ll_Downstream_Info.recvFailCount = 0;
  ll_Downstream_Info.firstDataReceived = FALSE;

  //turn on rx for first data from parent
  LL_TurnOnRx(C2P_TaskId);
  if(SUCCESS != osal_start_timerEx(C2P_TaskId, C2P_EVENT_START_TIMEOUT, 3 * LL_DOWNSTREAM_TRANS_PERIOD)){
    SystemReset();
  }

  //only for report node to produce downstream data
  osal_set_event(C2P_TaskId, C2P_EVENT_RECV_DATA_BEGIN);
#endif

  //reset upstream info
  ll_Upstream_Info.sendFailCount = 0;
  ll_Upstream_Info.partialSend = FALSE;
  ll_Upstream_Info.firstSend = TRUE;
  for(uint8 i = 0; i < LL_MAX_DATA_FRAGMENTS; ++i){
    ll_Upstream_Info.sendHandle[i] = LL_APPHDL_RESERVED;
  }
  ll_Upstream_Info.ackCycle = 0;

  if(SUCCESS != osal_start_timerEx(C2P_TaskId, C2P_EVENT_SEND_DATA, LL_SEND_DATA_DELAY)){
    SystemReset();
  }
}

void C2P_StopData()
{
  for(uint8 i = 0; i < LL_MAX_DATA_FRAGMENTS; ++i){
    ll_Upstream_Info.sendHandle[i] = LL_APPHDL_RESERVED;
  }
  C2P_state = C2P_STATE_UNCONNECTED;
}

void LL_DataProduce(StreamInfo_t *stream, uint8 type)
{
#ifndef LL_NODEROLE_ROUTERNODE
  static uint8 dsn = 0;
  uint8 fragNum;
  if(type == LL_DATA_TYPE_UPSTREAM && Sany_ReadData()) {
    fragNum = (Sany_BufLen + LL_DATA_MAX_LENGTH - 1) / LL_DATA_MAX_LENGTH;
    for(uint8 i = 0; i < fragNum; ++i){
      uint8 len = (i == fragNum - 1) ? Sany_BufLen - i * LL_DATA_MAX_LENGTH : LL_DATA_MAX_LENGTH;
      ll_Data_hdr_t *hdr = (ll_Data_hdr_t*)(stream->dataBuf + i * LL_DATA_BUFFER_LENGTH);
      uint8 *data = (uint8*)(hdr + 1);
      osal_memcpy(data, Sany_UartBuf + i * LL_DATA_MAX_LENGTH, len);

      hdr->flag = 0;
      hdr->type = type;
      hdr->dsn = dsn;
      hdr->len = len;
      hdr->hops = 0;
      hdr->period_adj = 0;
      hdr->total_frags = fragNum;
      hdr->current_frag = i;
    }
  }else{
    fragNum = 1;
    ll_Data_hdr_t *hdr = (ll_Data_hdr_t*)(stream->dataBuf);

    hdr->flag = 0;
    ll_bit_set(hdr->flag, LL_DATA_FLAG_DUMMY);
    hdr->type = type;
    hdr->dsn = dsn;
    hdr->len = 0;
    hdr->hops = 0;
    hdr->period_adj = 0;
    hdr->total_frags = fragNum;
    hdr->current_frag = 0;
  }

  stream->recvFailCount = 0;
  stream->recvTime = stream->adjustRecv = osal_GetSystemClock();
  uint16 transperiod = (type == LL_DATA_TYPE_DOWNSTREAM) ? LL_DOWNSTREAM_TRANS_PERIOD : LL_UPSTREAM_TRANS_PERIOD;
  stream->nextRecv = stream->recvTime + transperiod;
  stream->dataReady = TRUE;
  stream->recvFinish = TRUE;
  stream->recvDSN = dsn;
  stream->firstDataReceived = TRUE;
  stream->totalRecv = fragNum;

  dsn++;
#endif
}

void LL_DataConsume(StreamInfo_t *stream, uint8 type)
{
#ifndef LL_NODEROLE_ROUTERNODE
  ll_Data_hdr_t *hdr = (ll_Data_hdr_t*)stream->dataBuf;

  if(type == LL_DATA_TYPE_UPSTREAM && stream->dataReady && !ll_bit_test(hdr->flag, LL_DATA_FLAG_INVALID)) {
    Sany_BufLen = 0;
    for(uint8 i = 0; i < hdr->total_frags; ++i){
      ll_Data_hdr_t *hdr0 = (ll_Data_hdr_t*)(stream->dataBuf + i * LL_DATA_BUFFER_LENGTH);
      uint8 *data = (uint8*)(hdr0 + 1);
      osal_memcpy(Sany_UartBuf + Sany_BufLen, data, hdr0->len);
      Sany_BufLen += hdr0->len;
    }
    Sany_PrepSend();
    if(SUCCESS != osal_start_timerEx(LL_TaskId, LL_EVENT_SANY_AIR_SEND, 1)){
      SystemReset();
    }
  } else {

  }

#ifdef SLEEPTIME_ACCOUNTING
  {
    static uint32 recvNum = 0;
    static uint16 dummyNum = 0;
    static uint16 failNum = 0;

    if(!stream->dataReady)
      failNum++;
    else if(ll_bit_test(hdr->flag, LL_DATA_FLAG_INVALID))
      dummyNum++;
    else
      recvNum++;

    uint8 idx = 0;
    stream->dataBuf[idx++] = stream->dataReady ? hdr->hops : 0;;

    stream->dataBuf[idx++] = HI_UINT16(failNum);
    stream->dataBuf[idx++] = LO_UINT16(failNum);

    stream->dataBuf[idx++] = HI_UINT16(dummyNum);
    stream->dataBuf[idx++] = LO_UINT16(dummyNum);

    stream->dataBuf[idx++] = BREAK_UINT32(recvNum, 3);
    stream->dataBuf[idx++] = BREAK_UINT32(recvNum, 2);
    stream->dataBuf[idx++] = BREAK_UINT32(recvNum, 1);
    stream->dataBuf[idx++] = BREAK_UINT32(recvNum, 0);

    uint16 dstAddr = (type == LL_DATA_TYPE_UPSTREAM) ? LL_CONSUMER_ADDR_HEAD : LL_CONSUMER_ADDR_TAIL;
    LL_McpsDataReq(stream->dataBuf, idx, SADDR_MODE_SHORT, dstAddr, 0);
  }
#endif

  stream->dataReady = FALSE;
#endif
}

void LL_SetupDevInfo()
{
#ifdef LL_CC2430
  osal_nv_item_init(ZCD_NV_EXTADDR, Z_EXTADDR_LEN,NULL);
  osal_nv_read(ZCD_NV_EXTADDR, 0, Z_EXTADDR_LEN, ll_devInfo.ExtAddr);
#else
  HalFlashRead(HAL_FLASH_IEEE_PAGE, HAL_FLASH_IEEE_OSET, (uint8 *)&ll_devInfo.ExtAddr, HAL_FLASH_IEEE_SIZE);
#endif

  ll_devInfo.ShortAddr = BUILD_UINT16(ll_devInfo.ExtAddr[EXT_MACADDR_DEVID_LBYTE], ll_devInfo.ExtAddr[EXT_MACADDR_DEVID_HBYTE]);
  ll_Sany_Air_Channel = ll_devInfo.ExtAddr[EXT_MACADDR_CHANNEL];

  if(ll_Sany_Air_Channel < 11 || ll_Sany_Air_Channel > 26)
    ll_Sany_Air_Channel = 11;

  ll_devInfo.LogicalChannel = ll_Sany_Air_Channel + 5;
  if(ll_devInfo.LogicalChannel > 26) ll_devInfo.LogicalChannel = ll_devInfo.LogicalChannel - 26 + 10;

  /* if wrong address */
  if(ll_devInfo.ShortAddr == 0 || ll_devInfo.ShortAddr == 0xFFFF){
    while(1){
      HalLedSet(HAL_LED_ALL, HAL_LED_MODE_ON);

#if(defined WATCHDOG) && (WATCHDOG==TRUE)
      FEEDWATCHDOG();
#endif
      DelayMs(500);
      HalLedSet(HAL_LED_ALL, HAL_LED_MODE_OFF);
#if(defined WATCHDOG) && (WATCHDOG==TRUE)
      FEEDWATCHDOG();
#endif
      DelayMs(500);
    }
  }

  ll_devInfo.PanId = SANY_NWK_ADDR;
  ll_devInfo.BeaconOrder = 15;
  ll_devInfo.SupperFrameOrder = 15;
  ll_devInfo.dataHandle = 0;

  ll_Upstream_Info.dataBuf = _upstream_buf;
  ll_Upstream_Info.dummyBuf = _upstream_dummyBuf;
#ifdef TWO_WAY_DATA
  ll_Downstream_Info.dataBuf = _downstream_buf;
  ll_Downstream_Info.dummyBuf = _downstream_dummyBuf;
#endif
}

void LL_Restart()
{
  HalLedSet(HAL_LED_3, HAL_LED_MODE_OFF);
#ifdef LL_NODEROLE_REPORTNODE
  C2C_StopData();
  C2P_StopData();
  C2P_StartData();
  LL_StartListen();
#else
  C2C_StopData();
  C2P_StopData();
  LL_StopListen();
  C2P_StartActiveScan();
#endif
  ll_devInfo.lossParent = FALSE;
  ll_devInfo.sendLossParent = FALSE;
}

void LL_DisableSleep(uint8 handle)
{
  uint8 idx = (handle >> 3);
  uint8 bit = (handle & 0x07);
  sleep_mgr.sleepBitmap[idx] |= (1 << bit);
  uint32 bit1 = 1;
  sleep_mgr.topmask |= (bit1 << idx);
  osal_pwrmgr_device(PWRMGR_ALWAYS_ON);
  //HalLedSet(HAL_LED_2, HAL_LED_MODE_ON);
}

void LL_EnableSleep(uint8 handle)
{
  uint8 idx = (handle >> 3);
  uint8 bit = (handle & 0x07);
  sleep_mgr.sleepBitmap[idx] &= ~(1 << bit);
  if(sleep_mgr.sleepBitmap[idx] == 0){
    uint32 bit1 = 1;
    sleep_mgr.topmask &= ~(bit1 << idx);
    if(sleep_mgr.topmask == 0){
//#ifdef LL_NODEROLE_ROUTERNODE
      osal_pwrmgr_device(PWRMGR_BATTERY);
//#endif
      HalLedSet(HAL_LED_ALL, HAL_LED_MODE_OFF);
    }
  }
}
/**************************************************************************************************
 **************************************************************************************************/
