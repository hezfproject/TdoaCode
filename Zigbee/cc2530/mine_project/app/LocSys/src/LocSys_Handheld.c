
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
#define HANDHELD_EVENT_BLINK                    0x0001

#define HANDHELD_VDD_LIMT                       0x58    //(VDD/3)/(VREF/(2e7-1)) (VDD=2.4V,VREF=1.15V)        /* Vdd limit 2.4v*/

#define HANDHELD_LED_GREEN                      HAL_LED_1
#define HANDHELD_LED_RED                        HAL_LED_2

#define HANDHELD_HOLD_ID_BLINK                  0

#define HANDHELD_RXON_ID_RW                     0

typedef struct{
  uint8             settype:1;
  uint8             setstage:1;
  uint8             setdesc:1;
  uint8             dummy:5;
  uint8             type;
  uint8             stage;
  uint8             desclen;
  uint8             desc[LOCSYS_MAX_CARDDESC_LEN];
  uint16            dstAddr;
}writer_mgr_t;

static writer_mgr_t WriterMgr;

typedef struct{
  uint8             dummy;
}reader_mgr_t;

typedef struct{
  uint8             blinkcount;
}handheld_info_t;

static handheld_info_t Handheld_Info;

/* Task ID */
uint8 Handheld_TaskId;

devStates_t Handheld_NwkState;

uint8 Handheld_seqnum = 0;

void Handheld_Reset(void);
void Handheld_ProcessCardInfoFrm(nwk_data_t *frm);

void LocSys_Init(uint8 taskId)
{
  /* Initialize the task id */
  Handheld_TaskId = taskId;

  osal_pwrmgr_device(PWRMGR_BATTERY);
  osal_pwrmgr_task_state(taskId, PWRMGR_CONSERVE);
}

uint16 LocSys_ProcessEvent(uint8 taskId, uint16 events)
{  
  current = osal_GetSystemClock();
  current320us = Util_GetTime320us();
  if(events & SYS_EVENT_MSG){
    uint8* pMsg;
    while(pMsg = osal_msg_receive(Handheld_TaskId)){
      osal_msg_deallocate(pMsg);
    }
    return events ^ SYS_EVENT_MSG;
  }

  if(events & HANDHELD_EVENT_BLINK){
    if(Handheld_Info.blinkcount > 0){
      static uint8 ledon = FALSE;
      if(ledon){
        ledon = FALSE;
        Handheld_Info.blinkcount--;
        HalLedSet(HANDHELD_LED_RED, HAL_LED_MODE_OFF);
        Util_EnableSleep(Handheld_TaskId, HANDHELD_HOLD_ID_BLINK);
        Util_start_timerEx(Handheld_TaskId, HANDHELD_EVENT_BLINK, 300);
      }else{
        ledon = TRUE;
        Util_DisableSleep(Handheld_TaskId, HANDHELD_HOLD_ID_BLINK);
        HalLedSet(HANDHELD_LED_RED, HAL_LED_MODE_ON);
        Util_start_timerEx(Handheld_TaskId, HANDHELD_EVENT_BLINK, 100);
      }
    }
    return events ^ HANDHELD_EVENT_BLINK;
  }

  return 0;
}

void LocSys_HandleHall()
{
}

void LocSys_HandleKeys(uint8 keys, uint8 shift)
{
  if ( keys & HAL_KEY_SW_6 ){
  }
}

void Nwk_ComingDataCB(nwk_data_t *frm, int8 rssi)
{
  uint8 datatype = frm->payload[Nwk_ExtHdrLen(frm)];
  switch(frm->hdr.delivermode){
    case DM_CRD_TO_ALL:
    case DM_CRD_TO_HHD:{
      switch(datatype){
        case LOCSYS_DATA_CARD_STATUS:{
          break;
        }
        case LOCSYS_DATA_CARD_INFO:{
          Handheld_ProcessCardInfoFrm(frm);
          break;
        }
        default:
          break;
      }
      break;
    }
    case DM_STN_TO_HHD:{
      break;
    }
    case DM_TUNNELLING:
    case DM_TUNNELLING2:{
      Handheld_Info.blinkcount = 10;
      osal_set_event(Handheld_TaskId, HANDHELD_EVENT_BLINK);
      break;
    }
    
    default:
      break;
  }
  
}
void Nwk_StateChangeCB(devStates_t state)
{
  Handheld_NwkState = state;
  switch(state){
    case DEV_STATE_HOLD:{
      Nwk_SetDeviceInfo(NULL);
      Nwk_Start();
      break;
    }
    case DEV_STATE_RESET:
      Handheld_Reset();
      break;
    case DEV_STATE_ACTIVE:
      HalLedSet(HANDHELD_LED_GREEN, HAL_LED_MODE_ON);
      break;
    case DEV_STATE_INACTIVE:
      HalLedSet(HANDHELD_LED_GREEN, HAL_LED_MODE_OFF);
      break;
    default:
      break;
  }
}

void Handheld_Reset()
{
  WriterMgr.settype = 1;
  WriterMgr.setstage = 1;
  WriterMgr.setdesc = 1;
  WriterMgr.type = 0x0A;
  WriterMgr.stage = 0;
  const uint8 desc[] = {1, 2, 3, 4, 5, 6, 7, 8};
  WriterMgr.desclen = sizeof(desc);
  osal_memcpy(WriterMgr.desc, desc, WriterMgr.desclen);
  WriterMgr.dstAddr = NWK_BROADCAST_ADDR;
  Nwk_RxOnReq(HANDHELD_RXON_ID_RW);

  Handheld_Info.blinkcount = 0;
}

void Handheld_ProcessCardInfoFrm(nwk_data_t *frm)
{
#if 0
  uint8 sz = 3;
  uint8 *data = Nwk_DataReqBufAlloc(sz);
  if(data){
    uint16 srcAddr = frm->hdr.srcAddr;
    data[0] = srcAddr / 10000;
    srcAddr %= 10000;
    data[1] = srcAddr / 100;
    srcAddr %= 100;
    data[2] = srcAddr;

    data[0] = data[0] / 10 * 16 + data[0] % 10;
    data[1] = data[1] / 10 * 16 + data[1] % 10;
    data[2] = data[2] / 10 * 16 + data[2] % 10;
    
    nwkDataReq_t req;
    req.delivermode   = DM_DEBUG;
    req.dstAddr       = NWK_BROADCAST_ADDR;
    req.seqnum        = Handheld_seqnum++;
    req.txopt         = 0;
    req.len           = sz;
    req.p             = (uint8*)data;
    Nwk_DataReq(&req);
    Nwk_DataReqBufFree(data);
  }
  Handheld_Info.blinkcount = 4;
  osal_set_event(Handheld_TaskId, HANDHELD_EVENT_BLINK);
  return;
#endif

  LocSys_Data_CardInfo_t *info = (LocSys_Data_CardInfo_t*)&frm->payload[Nwk_ExtHdrLen(frm)];
  uint8 equal = TRUE;
  if(WriterMgr.settype && WriterMgr.type != info->cardtype)
    equal = FALSE;
  if(WriterMgr.setstage && WriterMgr.stage != info->cardstatus.stage)
    equal = FALSE;
  if(WriterMgr.setdesc && (WriterMgr.desclen != info->desclen || !osal_memcmp(WriterMgr.desc, info->desc, WriterMgr.desclen)))
    equal = FALSE;
  if(!equal && info->cardstatus.hallactive){
    uint8 size = sizeof(LocSys_Data_SetCardInfo_t) + (WriterMgr.setdesc ? WriterMgr.desclen : 0);
    LocSys_Data_SetCardInfo_t *data = (LocSys_Data_SetCardInfo_t*)Nwk_DataReqBufAlloc(size);
    if(data){
      data->datatype    = LOCSYS_DATA_SET_CARD_INFO;
      data->settype     = WriterMgr.settype;
      data->type        = WriterMgr.type;
      data->setstage    = WriterMgr.setstage;
      data->stage       = WriterMgr.stage;
      data->setdesc     = WriterMgr.setdesc;
      data->desclen     = WriterMgr.desclen;
      if(data->setdesc && data->desclen > 0){
        osal_memcpy(data->desc, WriterMgr.desc, WriterMgr.desclen);
      }

      nwkDataReq_t req;
      req.delivermode   = DM_HHD_TO_CRD;
      req.dstAddr       = frm->hdr.srcAddr;
      req.seqnum        = Handheld_seqnum++;
      req.txopt         = 0;
      req.len           = size;
      req.p             = (uint8*)data;
      Nwk_DataReq(&req);
      Nwk_DataReqBufFree(data);
    }
  }else if(equal){
    uint8 size = sizeof(LocSys_Data_CardInfo_t) + info->desclen;
    LocSys_Data_CardInfo_t *data = (LocSys_Data_CardInfo_t*)Nwk_DataReqBufAlloc(size);
    if(data){
      osal_memcpy(data, info, size);

      nwkDataReq_t req;
      req.delivermode   = DM_HHD_TO_STN;
      req.dstAddr       = NWK_STATION_ADDRESS;
      req.seqnum        = Handheld_seqnum++;
      req.txopt         = 0;
      req.len           = size;
      req.p             = (uint8*)data;
      Nwk_DataReq(&req);
      Nwk_DataReqBufFree(data);
    }
    Handheld_Info.blinkcount = 4;
    osal_set_event(Handheld_TaskId, HANDHELD_EVENT_BLINK);
  }
}

/**************************************************************************************************
 **************************************************************************************************/
