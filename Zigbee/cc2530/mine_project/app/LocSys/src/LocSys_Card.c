
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

#include "hal_beeper.h"

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
#define CARD_EVENT_ALERT                        0x0001
#define CARD_EVENT_TICK                         0x0002
#define CARD_EVENT_BEEP_TIMEOUT                 0x0004
#define CARD_EVENT_WAIT_ROUTER_RSP_TIMEOUT      0x0008
#define CARD_EVENT_KEY_PRESS                    0x0010
#define CARD_EVENT_DISPLAY_STAGE                0x0020
#define CARD_EVENT_INFO_STABLE                  0x0040
#define CARD_EVENT_HALL_ACTIVE_TIMEOUT          0x0080
#define CARD_EVENT_LOW_POWER                    0x0100

#define CARD_NV_ID_DESC                         0x0401
#define CARD_NV_ID_STATUS                       0x0402
#define CARD_NV_ID_TYPE                         0x0403

/* (VDD/3)/(VREF/(2e7-1)) (VDD~=2.28V,VREF=1.15V) : VDD = 3.45 * LIMT / 127 */
#define CARD_VDD_LIMT                           0x54        /* 0x54 = 2.28v, 0x55 = 2.309 */

#define CARD_LED_GREEN                          HAL_LED_1
#define CARD_LED_RED                            HAL_LED_2

#define CARD_LED_SENDDATA                       CARD_LED_GREEN
#define CARD_LED_ALERT                          CARD_LED_GREEN
#define CARD_LED_LOW_POWER                      CARD_LED_RED
const uint8 CARD_LED_STAGE[4] = {CARD_LED_RED, CARD_LED_RED, CARD_LED_RED | CARD_LED_GREEN, CARD_LED_GREEN};

#define CARD_RXON_ID_HALL_ACTIVE                0
#define CARD_RXON_ID_WAIT_ROUTER_RSP            1

#define CARD_HOLD_ID_HALL_ACTIVE                0
#define CARD_HOLD_ID_ALERT                      1
#define CARD_HOLD_ID_DISPLAY_STAGE              2
#define CARD_HOLD_ID_LOW_POWER                  3

#define CARD_KEY_PRESS_TICK                     100u
#define CARD_KEY_PRESS_COUNT_SHORT              1
#define CARD_KEY_PRESS_COUNT_LONG               10
#define CARD_KEY_PRESS_COUNT_LONGLONG           50

#define CARD_DISPLAY_STAGE_TIME                 1000u
#define CARD_INFO_UNSTABLE_DURATION             (1000u * 10)

#define CARD_HALL_ACTIVE_DURATION               1000u

#define CARD_ALERT_PERIOD                       1000u
#define CARD_ALERT_DURATION                     100u
#define CARD_ALERT_TICKS                        (60u * 20)

#define CARD_LOW_POWER_ALERT_PERIOD             30000u
#define CARD_LOW_POWER_ALERT_DURATION           5u

#define CARD_TICK_PERIOD                        5000u
#define CARD_SEND_STATUS_PERIOD_TICKS           4u
#define CARD_WAIT_ROUTER_RSP_DURATION           40u

#define CARD_SEND_INFO_INTERVAL                 (1000ul * 60 * 60)
#define CARD_SEND_INFO_COUNT                    (12 * 3)

typedef struct{
  uint8         len;
  uint8         buf[LOCSYS_MAX_CARDDESC_LEN];
}card_desc_t;

typedef struct{
  uint8         sendinfo;
  uint8         sendinfocnt;
  uint32        sendinfotime;
  card_desc_t   desc;
  cardstatus_t  cardstatus;
  uint8         cardtype;
  uint16        alertticks;
  uint8         keycount;
  uint8         displaycount;
  uint16        displayinterval;
  uint8         infostable;
  uint8         lowpowercount;
  uint8         ticks;
}card_info_t;

static card_info_t Card_Info;

/* Task ID */
uint8 Card_TaskId;

devStates_t Card_NwkState;

uint8 Card_seqnum = 0;

void Card_Reset(void);

void Card_ProcessSearchCardFrm(nwk_data_t *frm);
void Card_InitCardInfo(void);
void Card_StoreCardDesc(uint8 len, uint8* desc);
void Card_ProcessSetCardInfoFrm(nwk_data_t *frm);
void Card_StartSendCardInfo(void);
void Card_SendCardInfo(uint16 dstAddr, uint8 delivermode);
void Card_SendCardStatus();
void Card_DetectLowPower(void);
void Card_ShiftStage(void);
void Card_DisplayStage(void);
void Card_ClearInfo(void);


void LocSys_Init(uint8 taskId)
{
  /* Initialize the task id */
  Card_TaskId = taskId;

  osal_pwrmgr_device(PWRMGR_BATTERY);
  osal_pwrmgr_task_state(taskId, PWRMGR_CONSERVE);
}

uint16 LocSys_ProcessEvent(uint8 taskId, uint16 events)
{  
  current = osal_GetSystemClock();
  current320us = Util_GetTime320us();
  if(events & SYS_EVENT_MSG){
    uint8* pMsg;
    while(pMsg = osal_msg_receive(Card_TaskId)){
      osal_msg_deallocate(pMsg);
    }
    return events ^ SYS_EVENT_MSG;
  }

  if(events & CARD_EVENT_TICK){
    Card_DetectLowPower();

    if(!Card_Info.sendinfo && current - Card_Info.sendinfotime > CARD_SEND_INFO_INTERVAL){
      Card_StartSendCardInfo();
    }
    
    uint8 send = FALSE;
    uint8 sendstatus = (Card_Info.ticks % CARD_SEND_STATUS_PERIOD_TICKS == 0);
    if(Card_Info.infostable && Card_Info.sendinfo){
      if(Card_Info.sendinfocnt < CARD_SEND_INFO_COUNT){
        Card_Info.sendinfocnt++;
        Card_SendCardInfo(NWK_BROADCAST_ADDR, DM_CRD_TO_RTR);
        send = TRUE;
      }else if(sendstatus){
        Card_SendCardInfo(NWK_BROADCAST_ADDR, DM_CRD_TO_RTR);
        send = TRUE;
      }
    }

    if(Card_Info.infostable && sendstatus){
      Card_SendCardStatus();
      send = TRUE;
    }

    if(send){
      Util_start_timerEx(Card_TaskId, CARD_EVENT_WAIT_ROUTER_RSP_TIMEOUT, CARD_WAIT_ROUTER_RSP_DURATION);
      Nwk_RxOnReq(CARD_RXON_ID_WAIT_ROUTER_RSP);
      HalLedSet(CARD_LED_SENDDATA, HAL_LED_MODE_ON);
    }

    Card_Info.ticks++;
    return events ^ CARD_EVENT_TICK;
  } 

  if(events & CARD_EVENT_ALERT){
    static uint8 alerton = FALSE;
    if(alerton){
      HalBeepStop();
      HalLedSet(CARD_LED_ALERT, HAL_LED_MODE_OFF);
    
      Util_EnableSleep(Card_TaskId, CARD_HOLD_ID_ALERT);
  
      alerton = FALSE;
      Util_start_timerEx(Card_TaskId, CARD_EVENT_ALERT, CARD_ALERT_PERIOD - CARD_ALERT_DURATION);
    }else{
      if(Card_Info.alertticks > 0){
        Card_Info.alertticks--;

        Util_DisableSleep(Card_TaskId, CARD_HOLD_ID_ALERT);
      
        HalBeepBegin();
        HalLedSet(CARD_LED_ALERT, HAL_LED_MODE_ON);

        alerton = TRUE;
        Util_start_timerEx(Card_TaskId, CARD_EVENT_ALERT, CARD_ALERT_DURATION);
      }else{
        Util_stop_timer(Card_TaskId, CARD_EVENT_ALERT);
      }
    }
    return events ^ CARD_EVENT_ALERT;
  }
  
  if(events & CARD_EVENT_WAIT_ROUTER_RSP_TIMEOUT){
    Nwk_RxOffReq(CARD_RXON_ID_WAIT_ROUTER_RSP);
    HalLedSet(CARD_LED_SENDDATA, HAL_LED_MODE_OFF);
    return events ^ CARD_EVENT_WAIT_ROUTER_RSP_TIMEOUT;
  }

  if(events & CARD_EVENT_KEY_PRESS){
    if(HalKeyRead() & HAL_KEY_SW_6){//key is down
      Card_Info.keycount++;
      Util_start_timerEx(Card_TaskId, CARD_EVENT_KEY_PRESS, CARD_KEY_PRESS_TICK);
    }else{
      if(Card_Info.keycount > CARD_KEY_PRESS_COUNT_LONGLONG){
        Card_ClearInfo();
        SystemReset();
      }else if(Card_Info.keycount > CARD_KEY_PRESS_COUNT_LONG){
        Card_ShiftStage();
      }else if(Card_Info.keycount >= CARD_KEY_PRESS_COUNT_SHORT){
        Card_Info.alertticks = 0;
        
        Card_DisplayStage();
      }
    }
    return events ^ CARD_EVENT_KEY_PRESS;
  }

  if(events & CARD_EVENT_DISPLAY_STAGE){
    if(Card_Info.displaycount & 0x01){
      HalBeepBegin();
      Util_DisableSleep(Card_TaskId, CARD_HOLD_ID_DISPLAY_STAGE);
      HalLedSet(CARD_LED_STAGE[Card_Info.cardstatus.stage], HAL_LED_MODE_ON);
    }else{
      HalBeepStop();
      Util_EnableSleep(Card_TaskId, CARD_HOLD_ID_DISPLAY_STAGE);
      HalLedSet(CARD_LED_STAGE[Card_Info.cardstatus.stage], HAL_LED_MODE_OFF);
    }
    if(Card_Info.displaycount--){
      Util_start_timerEx(Card_TaskId, CARD_EVENT_DISPLAY_STAGE, Card_Info.displayinterval);
    }
    return events ^ CARD_EVENT_DISPLAY_STAGE;
  }

  if(events & CARD_EVENT_INFO_STABLE){
    Card_Info.infostable = TRUE;
    return events ^ CARD_EVENT_INFO_STABLE;
  }

  if(events & CARD_EVENT_HALL_ACTIVE_TIMEOUT){
    Util_EnableSleep(Card_TaskId, CARD_HOLD_ID_HALL_ACTIVE);
    Card_Info.cardstatus.hallactive = 0;
    Nwk_RxOffReq(CARD_RXON_ID_HALL_ACTIVE);
    return events ^ CARD_EVENT_HALL_ACTIVE_TIMEOUT;
  }

  if(events & CARD_EVENT_BEEP_TIMEOUT){
    HalBeepStop();
    return events ^ CARD_EVENT_BEEP_TIMEOUT;
  }

  if(events & CARD_EVENT_LOW_POWER){
    if(Card_Info.lowpowercount == 0){
      Util_stop_timer(Card_TaskId, CARD_EVENT_LOW_POWER);
    }else if(Card_Info.lowpowercount >= 10){
      static uint8 ledon = FALSE;
      if(ledon){
        ledon = FALSE;
        Util_EnableSleep(Card_TaskId, CARD_HOLD_ID_LOW_POWER);
        HalLedSet(CARD_LED_LOW_POWER, HAL_LED_MODE_OFF);
        Util_start_timerEx(Card_TaskId, CARD_EVENT_LOW_POWER, CARD_LOW_POWER_ALERT_PERIOD);
      }else{
        ledon = TRUE;
        Util_DisableSleep(Card_TaskId, CARD_HOLD_ID_LOW_POWER);
        HalLedSet(CARD_LED_LOW_POWER, HAL_LED_MODE_ON);
        Util_start_timerEx(Card_TaskId, CARD_EVENT_LOW_POWER, CARD_LOW_POWER_ALERT_DURATION);
      }
    }else{
      Card_DetectLowPower();
    }
    return events ^ CARD_EVENT_LOW_POWER;
  }

  return 0;
}

void LocSys_HandleHall()
{
  if(!Card_Info.cardstatus.hallactive){
    Card_Info.cardstatus.hallactive = 1;
    Util_DisableSleep(Card_TaskId, CARD_HOLD_ID_HALL_ACTIVE);
    Nwk_RxOnReq(CARD_RXON_ID_HALL_ACTIVE);
    HalBeepBegin();
    Card_SendCardInfo(NWK_BROADCAST_ADDR, DM_CRD_TO_HHD);
    Util_start_timerEx(Card_TaskId, CARD_EVENT_HALL_ACTIVE_TIMEOUT, CARD_HALL_ACTIVE_DURATION);
    Util_start_timerEx(Card_TaskId, CARD_EVENT_BEEP_TIMEOUT, 20);
  }
}

void LocSys_HandleKeys(uint8 keys, uint8 shift)
{
  if ( keys & HAL_KEY_SW_6 ){
    Card_Info.keycount = 0;
    Util_start_timerEx(Card_TaskId, CARD_EVENT_KEY_PRESS, 1);
  }
}

void Nwk_ComingDataCB(nwk_data_t *frm, int8 rssi)
{
  uint8 datatype = frm->payload[Nwk_ExtHdrLen(frm)];
  switch(frm->hdr.delivermode){
    case DM_RTR_TO_CRD:{
      switch(datatype){
        case LOCSYS_DATA_SEARCH_CARD:{
          Card_ProcessSearchCardFrm(frm);
          break;
        }
        case LOCSYS_DATA_CARD_INFO_ACK:{
          Card_Info.sendinfo = FALSE;
          Card_Info.sendinfotime = current;
          break;
        }
        default:
          break;
      }
      break;
    }
    case DM_HHD_TO_CRD:{
      switch(datatype){
        case LOCSYS_DATA_SEARCH_CARD:{
          Card_ProcessSearchCardFrm(frm);
          break;
        }
        case LOCSYS_DATA_SET_CARD_INFO:{
          Card_ProcessSetCardInfoFrm(frm);
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
  Card_NwkState = state;
  switch(state){
    case DEV_STATE_HOLD:{
      Nwk_SetDeviceInfo(NULL);
      Nwk_Start();
      break;
    }
    case DEV_STATE_RESET:
      Card_Reset();
      break;
    default:
      break;
  }
}

void Card_SendCardStatus()
{
  LocSys_Data_CardStatus_t *data = (LocSys_Data_CardStatus_t*)Nwk_DataReqBufAlloc(sizeof(LocSys_Data_CardStatus_t));
  if(data){
    data->datatype  = LOCSYS_DATA_CARD_STATUS;
    data->status    = Card_Info.cardstatus;
    data->type      = Card_Info.cardtype;
    
    nwkDataReq_t req;
    req.txopt       = 0;
    req.delivermode = DM_CRD_TO_ALL;
    req.dstAddr     = NWK_BROADCAST_ADDR;
    req.seqnum      = Card_seqnum++;
    req.len         = sizeof(LocSys_Data_CardStatus_t);
    req.p           = (uint8*)data;
    Nwk_DataReq(&req);
    Nwk_DataReqBufFree(data);
  }
}

void Card_ProcessSearchCardFrm(nwk_data_t *frm)
{
  LocSys_Data_SearchCard_t *cmd = (LocSys_Data_SearchCard_t *)&frm->payload[Nwk_ExtHdrLen(frm)];
  switch(cmd->cmdtype){
    case LOCSYS_DATA_SEARCH_CARD_ALERT:
      Card_Info.alertticks = CARD_ALERT_TICKS;
      Util_start_timerEx(Card_TaskId, CARD_EVENT_ALERT, 1 + (osal_rand() & 0x3FF));
      break;
    case LOCSYS_DATA_SEARCH_CARD_CANCEL_ALERT:
      Card_Info.alertticks = 0;
      break;
    case LOCSYS_DATA_SEARCH_CARD_QUERY_INFO:
      Card_StartSendCardInfo();
      break;
    default:
      break;
  }
}

void Card_InitCardInfo()
{
  uint8 ret = osal_nv_item_init(CARD_NV_ID_DESC, sizeof(card_desc_t), NULL);
  if (ret == NV_ITEM_UNINIT){
    Card_Info.desc.len = 0;
    osal_nv_write(CARD_NV_ID_DESC, 0, sizeof(card_desc_t), &Card_Info.desc);
  }else if(ret == SUCCESS){
    osal_nv_read(CARD_NV_ID_DESC, 0, sizeof(card_desc_t), &Card_Info.desc);
  }
  
  Card_Info.infostable = TRUE;
  Card_StartSendCardInfo();

  ret = osal_nv_item_init(CARD_NV_ID_TYPE, sizeof(Card_Info.cardtype), NULL);
  if(ret == NV_ITEM_UNINIT){
    Card_Info.cardtype   = (0xFF & Nwk_ShortAddr());
    osal_nv_write(CARD_NV_ID_TYPE, 0, sizeof(Card_Info.cardtype), &Card_Info.cardtype);
  }else if(ret == SUCCESS){
    osal_nv_read(CARD_NV_ID_TYPE, 0, sizeof(Card_Info.cardtype), &Card_Info.cardtype);
  }

  ret = osal_nv_item_init(CARD_NV_ID_STATUS, sizeof(Card_Info.cardstatus), NULL);
  if(ret == NV_ITEM_UNINIT){
    Card_Info.cardstatus.lowpower = 0;
    Card_Info.cardstatus.stage = 0;
    osal_nv_write(CARD_NV_ID_STATUS, 0, sizeof(Card_Info.cardstatus), &Card_Info.cardstatus);
  }else if(ret == SUCCESS){
    osal_nv_read(CARD_NV_ID_STATUS, 0, sizeof(Card_Info.cardstatus), &Card_Info.cardstatus);
  }

  Card_Info.cardstatus.lowpower = 0;
  Card_Info.cardstatus.hallactive = 0;

  Card_Info.alertticks = 0;
  Card_Info.lowpowercount = 0;

  Card_Info.ticks = 0;
}

void Card_StoreCardDesc(uint8 len, uint8 *desc)
{
  if(len > LOCSYS_MAX_CARDDESC_LEN) return;
  if(Card_Info.desc.len == len && osal_memcmp(Card_Info.desc.buf, desc, len))
    return;
  Card_Info.desc.len = len;
  osal_memcpy(Card_Info.desc.buf, desc, len);
  uint8 ret = osal_nv_item_init(CARD_NV_ID_DESC, sizeof(card_desc_t), NULL);
  if(ret != NV_OPER_FAILED){
    osal_nv_write(CARD_NV_ID_DESC, 0, sizeof(card_desc_t), &Card_Info.desc);
  }
}

void Card_ProcessSetCardInfoFrm(nwk_data_t * frm)
{
  LocSys_Data_SetCardInfo_t *cmd = (LocSys_Data_SetCardInfo_t*)&frm->payload[Nwk_ExtHdrLen(frm)];
  uint8 valid = TRUE;
  if(valid && cmd->settype){
    if(cmd->type > LOCSYS_MAX_CARDTYPE)
      valid = FALSE;
    if(valid && NV_OPER_FAILED == osal_nv_item_init(CARD_NV_ID_TYPE, sizeof(Card_Info.cardtype), NULL))
      valid = FALSE;
  }
  if(valid && cmd->setstage){
    if(cmd->stage > LOCSYS_MAX_CARDSTAGE)
      valid = FALSE;
    if(valid && NV_OPER_FAILED == osal_nv_item_init(CARD_NV_ID_STATUS, sizeof(Card_Info.cardstatus), NULL))
      valid = FALSE;
  }
  if(valid && cmd->setdesc){
    if(cmd->desclen > LOCSYS_MAX_CARDDESC_LEN)
      valid = FALSE;
    if(valid && NV_OPER_FAILED == osal_nv_item_init(CARD_NV_ID_DESC, sizeof(Card_Info.desc), NULL))
      valid = FALSE;
  }
  if(valid){
    uint8 infochange = FALSE;
    if(cmd->settype && cmd->type != Card_Info.cardtype){
      Card_Info.cardtype = cmd->type;
      osal_nv_write(CARD_NV_ID_TYPE, 0, sizeof(Card_Info.cardtype), &Card_Info.cardtype);
      infochange = TRUE;
    }
    if(cmd->settype && cmd->stage != Card_Info.cardstatus.stage){
      Card_Info.cardstatus.stage = cmd->stage;
      osal_nv_write(CARD_NV_ID_STATUS, 0, sizeof(Card_Info.cardstatus), &Card_Info.cardstatus);
      infochange = TRUE;
    }
    if(cmd->setdesc && (cmd->desclen != Card_Info.desc.len || !osal_memcmp(cmd->desc, Card_Info.desc.buf, cmd->desclen))){
      Card_Info.desc.len = cmd->desclen;
      osal_memcpy(Card_Info.desc.buf, cmd->desc, cmd->desclen);
      osal_nv_write(CARD_NV_ID_DESC, 0, sizeof(Card_Info.desc), &Card_Info.desc);
      infochange = TRUE;
    }
    if(infochange) Card_StartSendCardInfo();
  }
  Card_SendCardInfo(frm->hdr.srcAddr, DM_CRD_TO_HHD);
}

void Card_StartSendCardInfo()
{
  Card_Info.sendinfo = TRUE;
  Card_Info.sendinfocnt = 0;
}

void Card_SendCardInfo(uint16 dstAddr, uint8 delivermode)
{
  LocSys_Data_CardInfo_t *data = (LocSys_Data_CardInfo_t*)Nwk_DataReqBufAlloc(sizeof(LocSys_Data_CardInfo_t) + Card_Info.desc.len);
  if(data){
    data->datatype      = LOCSYS_DATA_CARD_INFO;
    data->cardaddress   = Nwk_ShortAddr();
    data->cardstatus    = Card_Info.cardstatus;
    data->cardtype      = Card_Info.cardtype;
    data->version       = LOCSYS_CARD_VERSION;
    data->desclen       = Card_Info.desc.len;
    osal_memcpy(data->desc, Card_Info.desc.buf, data->desclen);
    
    nwkDataReq_t req;
    req.txopt       = 0;
    req.delivermode = delivermode;
    req.dstAddr     = dstAddr;
    req.seqnum      = Card_seqnum++;
    req.len         = sizeof(LocSys_Data_CardInfo_t) + Card_Info.desc.len;
    req.p           = (uint8*)data;
    Nwk_DataReq(&req);
    Nwk_DataReqBufFree(data);
  }
}

void Card_DetectLowPower()
{
  if(Card_Info.lowpowercount < 10){
    Card_Info.cardstatus.lowpower = !HalAdcCheckVdd(CARD_VDD_LIMT);
    if(Card_Info.cardstatus.lowpower){
      Card_Info.lowpowercount++;
      Util_start_timerEx(Card_TaskId, CARD_EVENT_LOW_POWER, 50);
    }else{
      Card_Info.lowpowercount = 0;
    }
  }else{
    Card_Info.cardstatus.lowpower = 1;
    osal_set_event(Card_TaskId, CARD_EVENT_LOW_POWER);
    Card_StartSendCardInfo();
  }
}

void Card_ShiftStage()
{
  Card_Info.cardstatus.stage++;
  if(NV_OPER_FAILED != osal_nv_item_init(CARD_NV_ID_STATUS, sizeof(Card_Info.cardstatus), NULL)){
    osal_nv_write(CARD_NV_ID_STATUS, 0, sizeof(Card_Info.cardstatus), &Card_Info.cardstatus);
  }
  Card_Info.infostable = FALSE;
  Util_start_timerEx(Card_TaskId, CARD_EVENT_INFO_STABLE, CARD_INFO_UNSTABLE_DURATION);
  Card_DisplayStage();
}

void Card_DisplayStage()
{
  HalBeepStop();
  HalLedSet(CARD_LED_GREEN, HAL_LED_MODE_OFF);
  HalLedSet(CARD_LED_RED, HAL_LED_MODE_OFF);
  
  uint8 stage = Card_Info.cardstatus.stage;
  if(stage == 0) stage = 4;
  Card_Info.displaycount = stage * 2 - 1;
  Card_Info.displayinterval = CARD_DISPLAY_STAGE_TIME / Card_Info.displaycount;
  Util_start_timerEx(Card_TaskId, CARD_EVENT_DISPLAY_STAGE, 200);
}

void Card_Reset()
{
  HalBeepInit();
  osal_nv_init(NULL);
  Card_InitCardInfo();
  Util_start_reload_timer(Card_TaskId, CARD_EVENT_TICK, CARD_TICK_PERIOD);
}

void Card_ClearInfo()
{
  Card_Info.cardstatus.stage = 0;
  Card_Info.cardtype = LOCSYS_CARDTYPE_DEFAULT;
  Card_Info.desc.len = 0;
  if(NV_OPER_FAILED != osal_nv_item_init(CARD_NV_ID_STATUS, sizeof(Card_Info.cardstatus), NULL)){
    osal_nv_write(CARD_NV_ID_STATUS, 0, sizeof(Card_Info.cardstatus), &Card_Info.cardstatus);
  }
  if(NV_OPER_FAILED != osal_nv_item_init(CARD_NV_ID_TYPE, sizeof(Card_Info.cardtype), NULL)){
    osal_nv_write(CARD_NV_ID_TYPE, 0, sizeof(Card_Info.cardtype), &Card_Info.cardtype);
  }
  if(NV_OPER_FAILED != osal_nv_item_init(CARD_NV_ID_DESC, sizeof(Card_Info.desc), NULL)){
    osal_nv_write(CARD_NV_ID_DESC, 0, sizeof(Card_Info.desc), &Card_Info.desc);
  }

  Card_StartSendCardInfo();
}
/**************************************************************************************************
 **************************************************************************************************/

