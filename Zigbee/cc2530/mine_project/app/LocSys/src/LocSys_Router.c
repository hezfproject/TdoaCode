
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
#include "hal_flash.h"


/* OS includes */
#include "OSAL.h"
#include "OSAL_Tasks.h"
#include "OSAL_PwrMgr.h"

/* Application Includes */
#include "OnBoard.h"

/* MAC Application Interface */
#include "mac_api.h"
#include "mac_main.h"
#include "mac_pib.h"

#include "LocSys_Nwk.h"

/* Application */
#include "LocSys.h"

#include "LocSys_Util.h"

/* utils */
#include "crc.h"
#include "macutil.h"
#include "App_cfg.h"
#include "AppProtocol.h"
#include "delay.h"
#include "OSAL_Nv.h"
#include "ZComdef.h"
//#ifdef WATCHDOG
#include "WatchdogUtil.h"
//#endif

#include "AppProtocol.h"
#include "AppProtocolWrapper.h"

#include "LocSys_Protocol.h"

/**************************************************************************************************/


/* Event IDs */
#define ROUTER_EVENT_SEND_CARD_INFO             0x0001 
#define ROUTER_EVENT_SEND_CARD_RECORDS          0x0002
#define ROUTER_EVENT_DISTRIBUTE_CMD_TIMEOUT     0x0004
#define ROUTER_EVENT_SEND_ROUTER_PATH           0x0008
#define ROUTER_EVENT_SEND_ROUTER_INFO           0x0010
#define ROUTER_EVENT_LOW_POWER                  0x0020

/* (VDD/3)/(VREF/(2e7-1)) (VDD~=2.28V,VREF=1.15V) : VDD = 3.45 * LIMT / 127 */
#define ROUTER_VDD_LIMT                         0x5A        /* 0x5A = 2.445v, 0x5B = 2.472, 0x5C = 2.499 */

#define ROUTER_LED_GREEN                        HAL_LED_1
#define ROUTER_LED_RED                          HAL_LED_2

#define ROUTER_RXON_ID_SEARCH_CARD              0

#define ROUTER_HOLD_ID_LOW_POWER                0

#define ROUTER_SEND_CARD_RECORD_DELAY           (1000ul * 60) 
#define ROUTER_DISTRIBUTE_CMD_DURATION          (1000ul * 40)

#define ROUTER_RESTART_PERIOD                   (1000ul * 60 * 60 * 24)  //1 day
#define ROUTER_PREP_RESTART_DURATION            (1000ul * 60 * 3)       //3 mins

#define ROUTER_LOW_POWER_ALERT_PERIOD           30000u
#define ROUTER_LOW_POWER_ALERT_DURATION         20u

#define ROUTER_SEND_ROUTER_INFO_INTERVAL        (1000ul * 60 * 60)

#define ROUTER_SEND_CARD_RECORD_RETRIES         3

#define LOCSYS_MAX_CARD_DESC_RECORDS            5

#define ROUTER_INVALID_SEQNUM                   0xFFFF
#define ROUTER_MAX_CMD_SEQNUM                   8
typedef struct {
  uint8                 sendretries;
  uint8                 sendcount;
  uint8                 recvcount;
  uint8                 seqnum;
  uint32                firstrecvtime;
  cardStatusRecord_t*   sendbuf;
  cardStatusRecord_t*   recvbuf;
  uint8                 buf[2][NWK_MAX_DATA_FRAME_SIZE + NWK_DATA_OFFSET];

  uint8                 cmdcount;
  cardSearchRecord_t    cmdBuf[LOCSYS_MAX_CARD_CMD_RECORDS];
  uint32                cmdsendtimeout;
  uint16                recvseq[ROUTER_MAX_CMD_SEQNUM];

  uint8*                cardinfos[LOCSYS_MAX_CARD_DESC_RECORDS];
  uint32                cardinfotimeout[LOCSYS_MAX_CARD_DESC_RECORDS];
}Card_Mgr_t;

#define MAX_ROUTER_DESC_LEN                     50
typedef struct{
  cardstatus_t          routerstatus;
  uint8                 sendinfo;
  uint32                sendinfotime;

  uint8                 desclen;
  uint8                 desc[MAX_ROUTER_DESC_LEN];

  uint8                 lowpowercount;

  uint8                 preprestart;
  uint32                preprestarttime;
}router_info_t;

static uint8 Router_firstreset = TRUE;
static Card_Mgr_t Router_CardMgr;
static router_info_t Router_Info;

/* Task ID */
uint8 Router_TaskId;

devStates_t Router_NwkState;

uint8 Router_seqnum = 0;

void Router_Reset(void);

void Router_InitCardMgr(void);
void Router_SendCardStatusRecords(void);
void Router_RecordCardStatus(cardStatusRecord_t *record);
void Router_SendSearchCardCmd(uint16 dstAddr, uint8 type);
void Router_ProcessSearchCardRecordsFrm(nwk_data_t *frm);
void Router_ProcessCardInfoFrm(nwk_data_t *frm);
void Router_ProcessCardInfoAckFrm(nwk_data_t *frm);
void Router_ProcessCardStatusFrm(nwk_data_t *frm, int8 rssi);
void Router_SendCardInfo(void);
void Router_SendRouterPath(void);
void Router_SendRouterInfo(void);
void Router_DetectLowPower(void);


void LocSys_Init(uint8 taskId)
{
  /* Initialize the task id */
  Router_TaskId = taskId;

  osal_pwrmgr_device(PWRMGR_BATTERY);
  osal_pwrmgr_task_state(taskId, PWRMGR_CONSERVE);
}

uint16 LocSys_ProcessEvent(uint8 taskId, uint16 events)
{  
  current = osal_GetSystemClock();
  current320us = Util_GetTime320us();
  if(events & SYS_EVENT_MSG){
    uint8* pMsg;
    while(pMsg = osal_msg_receive(Router_TaskId)){
      osal_msg_deallocate(pMsg);
    }
    return events ^ SYS_EVENT_MSG;
  }

  if(events & ROUTER_EVENT_SEND_CARD_RECORDS){
    if(Router_NwkState == DEV_STATE_ACTIVE || 
       (Router_CardMgr.cmdcount && Util_TimeAfter(Router_CardMgr.cmdsendtimeout, current) && (Router_CardMgr.cmdsendtimeout - current > 2000u))){
      Router_SendCardStatusRecords();
      Util_start_timerEx(Router_TaskId, ROUTER_EVENT_SEND_CARD_RECORDS, 1 + (osal_rand() & 0x3FF));
    }
    return events ^ ROUTER_EVENT_SEND_CARD_RECORDS;
  }

  if(events & ROUTER_EVENT_SEND_ROUTER_PATH){
    if(Router_NwkState == DEV_STATE_ACTIVE){
      Router_SendRouterPath();
    }
    return events ^ ROUTER_EVENT_SEND_ROUTER_PATH;
  }

  if(events & ROUTER_EVENT_SEND_CARD_INFO){
    if(Router_NwkState == DEV_STATE_ACTIVE ||
       (Router_CardMgr.cmdcount && Util_TimeAfter(Router_CardMgr.cmdsendtimeout, current) && (Router_CardMgr.cmdsendtimeout - current > 2000u))){
      Router_SendCardInfo();
    }
    return events ^ ROUTER_EVENT_SEND_CARD_INFO;
  }

  if(events & ROUTER_EVENT_SEND_ROUTER_INFO){
    if(Router_NwkState == DEV_STATE_ACTIVE){
      if(!Router_Info.sendinfo && current - Router_Info.sendinfotime > ROUTER_SEND_ROUTER_INFO_INTERVAL){
        Router_Info.sendinfo = TRUE;
      }
      if(Router_Info.routerstatus.lowpower){
        Router_Info.sendinfo = TRUE;
      }
      if(Router_Info.sendinfo){
        Router_SendRouterInfo();
      }
    }
    return events ^ ROUTER_EVENT_SEND_ROUTER_INFO;
  }

  if(events & ROUTER_EVENT_DISTRIBUTE_CMD_TIMEOUT){
    Nwk_RxOffReq(ROUTER_RXON_ID_SEARCH_CARD);
    HalLedSet(ROUTER_LED_RED, HAL_LED_MODE_OFF);
    Router_CardMgr.cmdcount = 0;
    for(uint8 i = 0; i < ROUTER_MAX_CMD_SEQNUM; ++i){
      Router_CardMgr.recvseq[i] = ROUTER_INVALID_SEQNUM;
    }
    return events ^ ROUTER_EVENT_DISTRIBUTE_CMD_TIMEOUT;
  }

  if(events & ROUTER_EVENT_LOW_POWER){
    if(Router_Info.lowpowercount == 0){
      Util_stop_timer(Router_TaskId, ROUTER_EVENT_LOW_POWER);
    }else if(Router_Info.lowpowercount >= 10){
      static uint8 ledon = FALSE;
      if(ledon){
        ledon = FALSE;
        Util_EnableSleep(Router_TaskId, ROUTER_HOLD_ID_LOW_POWER);
        HalLedSet(ROUTER_LED_RED, HAL_LED_MODE_OFF);
        Util_start_timerEx(Router_TaskId, ROUTER_EVENT_LOW_POWER, ROUTER_LOW_POWER_ALERT_PERIOD);
      }else{
        ledon = TRUE;
        Util_DisableSleep(Router_TaskId, ROUTER_HOLD_ID_LOW_POWER);
        HalLedSet(ROUTER_LED_RED, HAL_LED_MODE_ON);
        Util_start_timerEx(Router_TaskId, ROUTER_EVENT_LOW_POWER, ROUTER_LOW_POWER_ALERT_DURATION);
      }
    }else{
      Router_DetectLowPower();
    }
    return events ^ ROUTER_EVENT_LOW_POWER;
  }

  return 0;
}

void LocSys_HandleHall()
{
}

void LocSys_HandleKeys(uint8 keys, uint8 shift)
{
}

void Nwk_ComingDataCB(nwk_data_t *frm, int8 rssi)
{
  uint8 datatype = frm->payload[Nwk_ExtHdrLen(frm)];
  switch(frm->hdr.delivermode){
    case DM_STN_TO_RTR:{
      switch(datatype){
        case LOCSYS_DATA_SEARCH_CARD:{
          Router_ProcessSearchCardRecordsFrm(frm);
          break;
        }
        case LOCSYS_DATA_CARD_INFO_ACK:{
          Router_ProcessCardInfoAckFrm(frm);
          break;
        }
        default:
          break;
      }
      break;
    }
    case DM_CRD_TO_ALL:
    case DM_CRD_TO_RTR:{
      switch(datatype){
        case LOCSYS_DATA_CARD_STATUS:{
          Router_ProcessCardStatusFrm(frm, rssi);
          break;
        }
        case LOCSYS_DATA_CARD_INFO:{
          Router_ProcessCardInfoFrm(frm);
          break;
        }
        default:
          break;
      }
      break;
    }
    
    default:
      break;
  }
  
}
void Nwk_StateChangeCB(devStates_t state)
{
  Router_NwkState = state;
  switch(state){
    case DEV_STATE_HOLD:{
      Nwk_SetDeviceInfo(NULL);
      Nwk_Start();
      break;
    }
    case DEV_STATE_RESET:
      Router_Reset();
      break;
    case DEV_STATE_ACTIVE:
      HalLedSet(ROUTER_LED_GREEN, HAL_LED_MODE_ON);
      Router_DetectLowPower();
      Util_start_timerEx(Router_TaskId, ROUTER_EVENT_SEND_CARD_RECORDS, 1 + (osal_rand() & 0x3FF));
      Util_start_timerEx(Router_TaskId, ROUTER_EVENT_SEND_ROUTER_PATH, 1 + (osal_rand() & 0x7FF));
      Util_start_timerEx(Router_TaskId, ROUTER_EVENT_SEND_ROUTER_INFO, 1 + (osal_rand() & 0x7FF));
      Util_start_timerEx(Router_TaskId, ROUTER_EVENT_SEND_CARD_INFO, 1 + (osal_rand() & 0x1FF));
      break;
    case DEV_STATE_INACTIVE:
      HalLedSet(ROUTER_LED_GREEN, HAL_LED_MODE_OFF);
      break;
    default:
      break;
  }
}

void Router_InitCardMgr()
{
  Router_CardMgr.sendcount      = 0;
  Router_CardMgr.recvcount      = 0;
  Router_CardMgr.sendretries    = 0;
  Router_CardMgr.seqnum         = 0;
  Router_CardMgr.sendbuf        = (cardStatusRecord_t*)&Router_CardMgr.buf[0][NWK_DATA_OFFSET + sizeof(LocSys_Data_CardStatusRecords_t)];
  Router_CardMgr.recvbuf        = (cardStatusRecord_t*)&Router_CardMgr.buf[1][NWK_DATA_OFFSET + sizeof(LocSys_Data_CardStatusRecords_t)];

  Router_CardMgr.cmdcount           = 0;
  for(uint8 i = 0; i < ROUTER_MAX_CMD_SEQNUM; ++i){
    Router_CardMgr.recvseq[i] = ROUTER_INVALID_SEQNUM;
  }

  for(uint8 i = 0; i < LOCSYS_MAX_CARD_DESC_RECORDS; ++i){
    if(!Router_firstreset && Router_CardMgr.cardinfos[i]){
      osal_mem_free(Router_CardMgr.cardinfos[i]);
    }
    Router_CardMgr.cardinfos[i] = NULL;
    Router_CardMgr.cardinfotimeout[i] = 0;
  }

  Router_firstreset = FALSE;
}

void Router_SendCardStatusRecords()
{
  if(Router_CardMgr.sendretries == 0){
    if(Router_CardMgr.recvcount && current - Router_CardMgr.firstrecvtime > ROUTER_SEND_CARD_RECORD_DELAY){
      cardStatusRecord_t* tmp        = Router_CardMgr.sendbuf;
      Router_CardMgr.sendbuf        = Router_CardMgr.recvbuf;
      Router_CardMgr.recvbuf        = tmp;
      Router_CardMgr.sendcount      = Router_CardMgr.recvcount;
      Router_CardMgr.sendretries    = ROUTER_SEND_CARD_RECORD_RETRIES;
      Router_CardMgr.recvcount      = 0;
      Router_CardMgr.seqnum++;
    }else{
      return;
    }
  }

  LocSys_Data_CardStatusRecords_t *data = (LocSys_Data_CardStatusRecords_t*)((uint8*)Router_CardMgr.sendbuf - sizeof(LocSys_Data_CardStatusRecords_t));
  data->datatype    = LOCSYS_DATA_CARD_STATUS;
  data->cnt         = Router_CardMgr.sendcount;

  nwkDataReq_t req;
  req.txopt         = 0;
  req.delivermode   = DM_RTR_TO_STN;
  req.dstAddr       = NWK_STATION_ADDRESS;
  req.len           = sizeof(LocSys_Data_CardStatusRecords_t) + sizeof(cardStatusRecord_t) * data->cnt;
  req.p             = (uint8*)data;
  req.seqnum        = Router_CardMgr.seqnum;
  if(NWK_SUCCESS == Nwk_DataReq(&req)){
    Router_CardMgr.sendretries--;
  }
}

void Router_RecordCardStatus(cardStatusRecord_t *record)
{
  if(Router_CardMgr.recvcount == 0) Router_CardMgr.firstrecvtime = current;
  Router_CardMgr.recvbuf[Router_CardMgr.recvcount++] = *record;
  if(Router_CardMgr.recvcount == LOCSYS_MAX_CARD_STATUS_RECORDS){
    cardStatusRecord_t* tmp     = Router_CardMgr.sendbuf;
    Router_CardMgr.sendbuf      = Router_CardMgr.recvbuf;
    Router_CardMgr.recvbuf      = tmp;
    Router_CardMgr.sendcount    = Router_CardMgr.recvcount;
    Router_CardMgr.sendretries  = ROUTER_SEND_CARD_RECORD_RETRIES;
    Router_CardMgr.recvcount    = 0;
    Router_CardMgr.seqnum++;

    Util_start_timerEx(Router_TaskId, ROUTER_EVENT_SEND_CARD_RECORDS, 20);
  }
}

void Router_SendSearchCardCmd(uint16 dstAddr, uint8 type)
{
  for(uint8 i = 0; i < Router_CardMgr.cmdcount; ++i){
    if(Router_CardMgr.cmdBuf[i].dstAddr == NWK_BROADCAST_ADDR || 
       dstAddr == Router_CardMgr.cmdBuf[i].dstAddr ||
       (Router_CardMgr.cmdBuf[i].bytype == 0xFF && type == Router_CardMgr.cmdBuf[i].type)){
      LocSys_Data_SearchCard_t *data = (LocSys_Data_SearchCard_t*)Nwk_DataReqBufAlloc(sizeof(LocSys_Data_SearchCard_t));
      if(data){
        data->datatype  = LOCSYS_DATA_SEARCH_CARD;
        data->cmdtype   = Router_CardMgr.cmdBuf[i].cmdtype;
        data->param     = Router_CardMgr.cmdBuf[i].param;
        
        nwkDataReq_t req;
        req.txopt       = 0;
        req.delivermode = DM_RTR_TO_CRD;
        req.dstAddr     = dstAddr;
        req.seqnum      = Router_seqnum++;
        req.len         = sizeof(LocSys_Data_SearchCard_t);
        req.p           = (uint8*)data;
        Nwk_DataReq(&req);
        Nwk_DataReqBufFree(data);
      }
      break;
    }
  }
}

void Router_ProcessCardStatusFrm(nwk_data_t *frm, int8 rssi)
{
  LocSys_Data_CardStatus_t *sfrm = (LocSys_Data_CardStatus_t*)&frm->payload[Nwk_ExtHdrLen(frm)];
  if(Router_CardMgr.cmdcount){
    Router_SendSearchCardCmd(frm->hdr.srcAddr, sfrm->type);
  }
  cardStatusRecord_t record;
  record.srcAddr    = frm->hdr.srcAddr;
  record.rssi       = rssi;
  record.status     = sfrm->status;
  Router_RecordCardStatus(&record);
}

void Router_ProcessCardInfoFrm(nwk_data_t *frm)
{
  if(Router_Info.preprestart) return;

  LocSys_Data_CardInfo_t *cardinfo = (LocSys_Data_CardInfo_t *)&frm->payload[Nwk_ExtHdrLen(frm)];
  uint8 freeidx = LOCSYS_MAX_CARD_DESC_RECORDS;
  uint8 status = NWK_FAILURE;
  for(uint8 i = 0; i < LOCSYS_MAX_CARD_DESC_RECORDS; ++i){
    if(!Router_CardMgr.cardinfos[i] && freeidx == LOCSYS_MAX_CARD_DESC_RECORDS){
      freeidx = i;
    }
    if(Router_CardMgr.cardinfos[i]){
      LocSys_Data_CardInfo_t *d = (LocSys_Data_CardInfo_t*)Router_CardMgr.cardinfos[i];
      if(d->cardaddress == cardinfo->cardaddress){
        status = NWK_SUCCESS;
        break;
      }
    }
  }
  if(status != NWK_SUCCESS && freeidx != LOCSYS_MAX_CARD_DESC_RECORDS){
    Router_CardMgr.cardinfos[freeidx] = osal_mem_alloc(sizeof(LocSys_Data_CardInfo_t) + cardinfo->desclen);
    if(Router_CardMgr.cardinfos[freeidx]){
      osal_memcpy(Router_CardMgr.cardinfos[freeidx], cardinfo, sizeof(LocSys_Data_CardInfo_t) + cardinfo->desclen);
      status = NWK_SUCCESS;
    }
  }
  
  if(status == NWK_SUCCESS){
    LocSys_Data_CardInfoAck_t *ack = (LocSys_Data_CardInfoAck_t*)Nwk_DataReqBufAlloc(sizeof(LocSys_Data_CardInfoAck_t));
    if(ack){
      ack->datatype = LOCSYS_DATA_CARD_INFO_ACK;
      ack->cardaddress = cardinfo->cardaddress;

      nwkDataReq_t req;
      req.delivermode   = DM_RTR_TO_CRD;
      req.dstAddr       = frm->hdr.srcAddr;
      req.seqnum        = frm->hdr.dsn;
      req.txopt         = 0;
      req.len           = sizeof(LocSys_Data_CardInfoAck_t);
      req.p             = (uint8*)ack;
      Nwk_DataReq(&req);
      Nwk_DataReqBufFree(ack);
    }
  }
}

void Router_ProcessCardInfoAckFrm(nwk_data_t * frm)
{
  LocSys_Data_CardInfoAck_t *ack = (LocSys_Data_CardInfoAck_t*)&frm->payload[Nwk_ExtHdrLen(frm)];
  if(ack->cardaddress == NWK_BROADCAST_ADDR){
    Router_Info.sendinfo = FALSE;
    Router_Info.sendinfotime = current;
    return;
  }
  for(uint8 i = 0; i < LOCSYS_MAX_CARD_DESC_RECORDS; ++i){
    if(!Router_CardMgr.cardinfos[i]) continue;
    LocSys_Data_CardInfo_t *info = (LocSys_Data_CardInfo_t*)Router_CardMgr.cardinfos[i];
    if(info->cardaddress == ack->cardaddress){
      osal_mem_free(Router_CardMgr.cardinfos[i]);
      Router_CardMgr.cardinfos[i] = NULL;
      return;
    }
  }
}

void Router_SendRouterPath()
{
  uint8 depth = Nwk_Depth();
  uint8 send = FALSE;
  if(depth != NWK_INVALID_DEPTH){
    uint8 len = sizeof(LocSys_Data_RouterPath_t) + sizeof(uint16) * depth;
    LocSys_Data_RouterPath_t *data = (LocSys_Data_RouterPath_t*)Nwk_DataReqBufAlloc(len);
    if(data){
      if(NWK_SUCCESS == Nwk_Path(data->path, depth)){
        data->depth     = depth;
        data->datatype  = LOCSYS_DATA_ROUTER_PATH;

        nwkDataReq_t req;
        req.txopt       = 0;
        req.delivermode = DM_RTR_TO_STN;
        req.dstAddr     = NWK_STATION_ADDRESS;
        req.seqnum      = Router_seqnum++;
        req.len         = len;
        req.p           = (uint8*)data;
        if(NWK_SUCCESS == Nwk_DataReq(&req))
          send = TRUE;
      }
      Nwk_DataReqBufFree(data);
    }
  }
  if(!send) Util_start_timerEx(Router_TaskId, ROUTER_EVENT_SEND_ROUTER_PATH, 1000);
}

void Router_SendRouterInfo()
{
  if(Nwk_Depth() == NWK_INVALID_DEPTH) return;

  uint8 extlen = sizeof(nwk_path_ext_t) + sizeof(uint16) * Nwk_Depth();
  uint8 infolen = sizeof(LocSys_Data_CardInfo_t) + Router_Info.desclen;
  nwk_path_ext_t *exthdr = (nwk_path_ext_t*)Nwk_DataReqBufAlloc(extlen + infolen);
  if(exthdr){
    exthdr->depth = Nwk_Depth();
    Nwk_Path(exthdr->path, exthdr->depth);
    LocSys_Data_CardInfo_t* info = (LocSys_Data_CardInfo_t*)((uint8*)exthdr + extlen);
    info->datatype      = LOCSYS_DATA_CARD_INFO;
    info->cardaddress   = NWK_BROADCAST_ADDR;
    info->cardstatus    = Router_Info.routerstatus;
    info->cardtype      = 0;
    info->version       = LOCSYS_ROUTER_VERSION;
    info->desclen       = Router_Info.desclen;
    osal_memcpy(info->desc, Router_Info.desc, Router_Info.desclen);

    nwkDataReq_t req;
    req.delivermode     = DM_RTR_TO_STN;
    req.txopt           = NWK_TXOPTION_WITHPATH;
    req.dstAddr         = NWK_STATION_ADDRESS;
    req.seqnum          = Router_seqnum++;
    req.len             = extlen + infolen;
    req.p               = (uint8*)exthdr;
    Nwk_DataReq(&req);
    Nwk_DataReqBufFree(exthdr);
  }
}

void Router_SendCardInfo()
{
  uint16 mintimeout = ~0u;
  if(Nwk_Depth() == NWK_INVALID_DEPTH){
    mintimeout = 100u;
  }else{
    for(uint8 i = 0; i < LOCSYS_MAX_CARD_DESC_RECORDS; ++i){
      if(!Router_CardMgr.cardinfos[i]) continue;
      if(!Util_SequenceAfter32(Router_CardMgr.cardinfotimeout[i], current)){
        LocSys_Data_CardInfo_t *info = (LocSys_Data_CardInfo_t*)Router_CardMgr.cardinfos[i];
        uint8 extlen = sizeof(nwk_path_ext_t) + sizeof(uint16) * Nwk_Depth();
        nwk_path_ext_t *exthdr = (nwk_path_ext_t*)Nwk_DataReqBufAlloc(extlen + sizeof(LocSys_Data_CardInfo_t) + info->desclen);
        if(exthdr){
          exthdr->depth = Nwk_Depth();
          Nwk_Path(exthdr->path, exthdr->depth);
          osal_memcpy((uint8*)exthdr + extlen, info, sizeof(LocSys_Data_CardInfo_t) + info->desclen);

          nwkDataReq_t req;
          req.delivermode   = DM_RTR_TO_STN;
          req.txopt         = NWK_TXOPTION_WITHPATH;
          req.dstAddr       = NWK_STATION_ADDRESS;
          req.seqnum        = Router_seqnum++;
          req.len           = extlen + sizeof(LocSys_Data_CardInfo_t) + info->desclen;
          req.p             = (uint8*)exthdr;
          if(NWK_SUCCESS == Nwk_DataReq(&req)){
            Router_CardMgr.cardinfotimeout[i] = current + 100u * Nwk_Depth();
          }
          Nwk_DataReqBufFree(exthdr);
        }
        mintimeout = 10u;
        break;
      }
      if(Router_CardMgr.cardinfotimeout[i] - current < mintimeout){
        mintimeout = Router_CardMgr.cardinfotimeout[i] - current;
      }
    }
  }
  if(mintimeout != ~0u)
    Util_start_timerEx(Router_TaskId, ROUTER_EVENT_SEND_CARD_INFO, mintimeout);
}

void Router_ProcessSearchCardRecordsFrm(nwk_data_t *frm)
{
  uint8 i = 0;
  for(; i < ROUTER_MAX_CMD_SEQNUM; ++i){
    if(Router_CardMgr.recvseq[i] == ROUTER_INVALID_SEQNUM) break;
    if((uint16)frm->hdr.dsn == Router_CardMgr.recvseq[i]) return;
  }
  if(i == ROUTER_MAX_CMD_SEQNUM) return;
  Router_CardMgr.recvseq[i] = frm->hdr.dsn;
  
  LocSys_Data_SearchCardRecords_t *recordscmd = (LocSys_Data_SearchCardRecords_t *)&frm->payload[Nwk_ExtHdrLen(frm)];
  uint8 cnt = recordscmd->cnt;
  if(Router_CardMgr.cmdcount + cnt > LOCSYS_MAX_CARD_CMD_RECORDS)
    cnt = LOCSYS_MAX_CARD_CMD_RECORDS - Router_CardMgr.cmdcount;
  osal_memcpy(&Router_CardMgr.cmdBuf[Router_CardMgr.cmdcount], recordscmd->records, cnt * sizeof(cardSearchRecord_t));
  Router_CardMgr.cmdcount += cnt;
  Nwk_RxOnReq(ROUTER_RXON_ID_SEARCH_CARD);
  HalLedSet(ROUTER_LED_RED, HAL_LED_MODE_ON);
  Router_CardMgr.cmdsendtimeout = current + ROUTER_DISTRIBUTE_CMD_DURATION;
  Util_start_timerEx(Router_TaskId, ROUTER_EVENT_DISTRIBUTE_CMD_TIMEOUT, ROUTER_DISTRIBUTE_CMD_DURATION);
}

void Router_Reset()
{
  Router_InitCardMgr();
  osal_memset(&Router_Info.routerstatus, 0, sizeof(Router_Info.routerstatus));
  Router_Info.desclen = 0;
  Router_Info.sendinfo = TRUE;
  Router_Info.lowpowercount = 0;
  Router_Info.preprestart = FALSE;
}

void Router_DetectLowPower()
{
  if(Router_Info.lowpowercount < 10){
    Router_Info.routerstatus.lowpower = !HalAdcCheckVdd(ROUTER_VDD_LIMT);
    if(Router_Info.routerstatus.lowpower){
      Router_Info.lowpowercount++;
      Util_start_timerEx(Router_TaskId, ROUTER_EVENT_LOW_POWER, 50);
    }else{
      Router_Info.lowpowercount = 0;
    }
  }else{
    Router_Info.routerstatus.lowpower = 1;
    osal_set_event(Router_TaskId, ROUTER_EVENT_LOW_POWER);
  }
}

void Nwk_RestartCB()
{
  if(current > ROUTER_RESTART_PERIOD){
    if(!Router_Info.preprestart){
      Router_Info.preprestart = TRUE;
      Router_Info.preprestarttime = current;
    }else if(current - Router_Info.preprestarttime > ROUTER_PREP_RESTART_DURATION && Router_CardMgr.cmdcount == 0){
      SystemReset();
    }
  }
}
/**************************************************************************************************
 **************************************************************************************************/
