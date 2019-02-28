
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
#ifndef LOCSYS_NO_SPI
#include "hal_spi.h"
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
#define STATION_EVENT_SEND_BUFFERED_CMD         0x0001
#define STATION_EVENT_RFMAC                     0x0002
#define STATION_EVENT_DETECT_BLAST              0x0004
#define STATION_EVENT_REPORT_STATUS             0x0008
#define STATION_EVENT_SPI_READ                  0x0010

#define STATION_EVENT_SIMULATE_CMD              0x4000


#define STATION_PARAM_ADDR                      0x1EF0  //1EF0-1EFF

#define STATION_VDD_LIMT                        0x58    //(VDD/3)/(VREF/(2e7-1)) (VDD=2.4V,VREF=1.15V)        /* Vdd limit 2.4v*/

#define STATION_LED_GREEN                       HAL_LED_1
#define STATION_LED_RED                         HAL_LED_2

#define MAX_CMD_BUFFER_ENTRIES                  8
typedef struct{
  uint8 cnt;
  uint8* buf[MAX_CMD_BUFFER_ENTRIES];
}cmd_mgr_t;
static cmd_mgr_t Station_CmdMgr;

typedef struct
{
  uint8 ResetFlag;
}storeParam_t;

/* Task ID */
uint8 Station_TaskId;

devStates_t Station_NwkState;

uint8 Station_seqnum = 0;

deviceInfo_t Station_devinfo;

/* buf send on uart  */
static uint8 Station_SpiBuf[MINEAPP_MAX_DATA_LEN + 10];
static SPIMSGPacket_t* pSpi;    // point to spi buffer
static uint8 Station_SpiReadBuf[MINEAPP_MAX_DATA_LEN + 10];

/*spi */
static uint8  spi_errcnt;

/* rfmac status */
static bool rfmac_setted = false;

void Station_Reset();

static uint8  Station_SendDataToSpi(uint16 shortAddr, const uint8 *p, uint16 len);
static uint8 Station_SendDataToSpiEx(uint16 shortAddr, uint16 len);
static void Station_ProcessSPICB(const  SPIMSGPacket_t * rxmsg);
static uint8 Station_ProcessBroadcastCmd(LocSys_CmdHdr_t *cmdhdr);
static uint8 Station_ProcessTunnelling2Cmd(LocSys_CmdHdr_t *cmdhdr);
static void Station_SendAirFrmToSpi(nwk_data_t *frm);
static void Station_SendUnknownAirFrmToSpi(nwk_data_t *frm);
static void Station_ProcessCardInfoFrm(nwk_data_t *frm);

void LocSys_Init(uint8 taskId)
{
  /* Initialize the task id */
  Station_TaskId = taskId;

  #ifndef LOCSYS_NO_SPI
  /* initial spi */
  halSPICfg_t spiConfig;
  spiConfig.bufSize = DMA_RT_MAX;
  spiConfig.taskID  = Station_TaskId;
  spiConfig.eventID = STATION_EVENT_SPI_READ;
  HalSpiStart(&spiConfig);
  #endif

  pSpi = (SPIMSGPacket_t *) Station_SpiBuf;
  pSpi->spihdr.hdr.event        = SPI_RX_MSG;                                  // useless, just the same to v1.0
  pSpi->spihdr.srcAddr.addrMode = (AddrMode_t) Addr16Bit;
  pSpi->spihdr.srcAddr.endPoint = MINEAPP_ENDPOINT;
  pSpi->spihdr.dstAddr.addrMode = (AddrMode_t) Addr16Bit;
  pSpi->spihdr.dstAddr.endPoint = MINEAPP_ENDPOINT;
  pSpi->spihdr.transID          = INIT_TRANID;
  pSpi->spihdr.options          = INIT_OPN;
  pSpi->spihdr.radius           = 1;

  osal_pwrmgr_device(PWRMGR_BATTERY);
  osal_pwrmgr_task_state(taskId, PWRMGR_CONSERVE);

}

uint16 LocSys_ProcessEvent(uint8 taskId, uint16 events)
{  
  current = osal_GetSystemClock();
  current320us = Util_GetTime320us();
  if(events & SYS_EVENT_MSG){
    uint8* pMsg;
    while(pMsg = osal_msg_receive(Station_TaskId)){
      osal_msg_deallocate(pMsg);
    }
    return events ^ SYS_EVENT_MSG;
  }

  if(events & STATION_EVENT_SEND_BUFFERED_CMD){
    if(Station_CmdMgr.cnt){
      if(Station_NwkState == DEV_STATE_ACTIVE){
        LocSys_CmdHdr_t *cmdhdr = (LocSys_CmdHdr_t *)Station_CmdMgr.buf[Station_CmdMgr.cnt - 1];

        uint8* buf = Nwk_DataReqBufAlloc(cmdhdr->hdr.len);
        if(buf){
          osal_memcpy(buf, &cmdhdr->cmdtype, cmdhdr->hdr.len);
      
          nwkDataReq_t req;
          req.delivermode   = (cmdhdr->cmdtype == LOCSYS_DATA_TUNNELLING) ? DM_TUNNELLING : DM_STN_TO_RTR;
          req.txopt         = 0;
          req.dstAddr       = cmdhdr->hdr.dstAddr;
          req.seqnum        = cmdhdr->hdr.seqnum;
          req.len           = cmdhdr->hdr.len;
          req.p             = buf;
          uint8 st = Nwk_DataReq(&req);
          Nwk_DataReqBufFree(buf);
          if(st == NWK_SUCCESS){
            Station_CmdMgr.cnt--;
          }
        }
      }
    }
    return events ^ STATION_EVENT_SEND_BUFFERED_CMD;
  }


  #ifdef LOCSYS_NO_SPI_CMD
  if(events & STATION_EVENT_SIMULATE_CMD){
    static uint8 seqnum = 0;
    if((seqnum & 0xF) == 0){
      SPIMSGPacket_t *hdr = (SPIMSGPacket_t *)Station_SpiReadBuf; 
      app_LocSys_Cmd_t *spicmd = (app_LocSys_Cmd_t*)(hdr + 1);
      spicmd->msgtype   = LOCSYS_CMD;
      spicmd->srcAddr   = NWK_STATION_ADDRESS;
      spicmd->dstAddr   = NWK_BROADCAST_ADDR;
      spicmd->seqnum    = seqnum;
      LocSys_Data_SearchCardRecords_t *cmd = (LocSys_Data_SearchCardRecords_t*)(spicmd + 1);
      cmd->datatype = LOCSYS_DATA_SEARCH_CARD;
      if((seqnum & 0x1F) == 0){
        uint8 cnt = 2;
        cmd->cnt = cnt;
        cmd->records[0].dstAddr = 0x0007;
        cmd->records[0].cmdtype = LOCSYS_DATA_SEARCH_CARD_ALERT;
        cmd->records[1].bytype  = 0xFF;
        cmd->records[1].type    = 0x08;
        cmd->records[1].cmdtype = LOCSYS_DATA_SEARCH_CARD_ALERT;
        spicmd->len = sizeof(LocSys_Data_SearchCardRecords_t) + cnt * sizeof(cardSearchRecord_t);
      }else{
        uint8 cnt = 1;
        cmd->cnt = cnt;
        cmd->records[0].dstAddr = NWK_BROADCAST_ADDR;
        cmd->records[0].cmdtype = LOCSYS_DATA_SEARCH_CARD_CANCEL_ALERT;
        spicmd->len = sizeof(LocSys_Data_SearchCardRecords_t) + cnt * sizeof(cardSearchRecord_t);
      }

      hdr->DataLength = spicmd->len + sizeof(app_LocSys_Cmd_t);
      hdr->spihdr.hdr.event = SPI_RX_MSG;
      Station_ProcessSPICB(hdr);
    }
    seqnum++;
    return events ^ STATION_EVENT_SIMULATE_CMD;
  }
  #endif

  #ifndef LOCSYS_NO_SPI
  if(events & STATION_EVENT_SPI_READ){
    uint16 len;
    if(HalSPIRead(Station_SpiReadBuf, &len) == SUCCESS){
      Station_ProcessSPICB((SPIMSGPacket_t *) Station_SpiReadBuf);
    }
    return events ^ STATION_EVENT_SPI_READ;
  }
  #endif
  
  if(events & STATION_EVENT_RFMAC){

    #ifdef LOCSYS_NO_SPI

    sAddrExt_t extaddr = {1,2,3,4,5,6,7,8};
    sAddrExtCpy(Station_devinfo.ExtAddr, extaddr);
    Station_devinfo.Channel = 0x1A;

    rfmac_setted = TRUE;
    Util_stop_timer(Station_TaskId, STATION_EVENT_RFMAC);
    Nwk_SetDeviceInfo(&Station_devinfo);
    Nwk_Start();

    #else
    
    if(rfmac_setted){
      Nwk_Start();
    }else{
      static uint8 rfmac_seqnum;
      app_rfmac_query_t   rfmac_query;
      rfmac_query.msgtype = RFMAC_QUERY;
      rfmac_query.seq = rfmac_seqnum++;
      Station_SendDataToSpi(0, (uint8*)&rfmac_query, sizeof(rfmac_query));
      Util_start_timerEx(Station_TaskId, STATION_EVENT_RFMAC, 1000);
    }
    
    #endif
    return events ^ STATION_EVENT_RFMAC;
  }

  if(events & STATION_EVENT_REPORT_STATUS){
    app_ZC_Report_t zc_report;
    zc_report.msgtype = ZC_REPORT;
    zc_report.PanId = Nwk_PanId();

    /* get reset flag and send report */
    storeParam_t param = *(storeParam_t*) STATION_PARAM_ADDR;
    switch(ResetReason()){
      case RESET_FLAG_WATCHDOG:{
        zc_report.flag  = param.ResetFlag;
        break;
      }
      case RESET_FLAG_EXTERNAL:{
        zc_report.flag  = ZC_REPORT_EXTERNAL_RESTART;
        break;
      }
      case RESET_FLAG_POWERON:{
        zc_report.flag  = ZC_REPORT_POWERON;
        break;
      }
      default:
        break;
    }

    Station_SendDataToSpi(0, (uint8*)&zc_report, sizeof(zc_report));

    /* set  reset param to watchdog */
    param.ResetFlag = ZC_REPORT_WATCHDOG_RESTART;
    *(storeParam_t*) STATION_PARAM_ADDR = param;

    return events ^ STATION_EVENT_REPORT_STATUS;
  }

  if(events & STATION_EVENT_DETECT_BLAST){
    Util_start_timerEx(Station_TaskId, STATION_EVENT_DETECT_BLAST, 6000);

    app_SPIDetect_t spi_report;
    spi_report.msgtype = SPI_DETECT;
    for(uint8 i = 0; i < SPIDETECT_LEN; i++){
      spi_report.detectdata[i] = i;
    }
    Station_SendDataToSpi(0, (uint8*)&spi_report, sizeof(spi_report));

    #ifndef LOCSYS_NO_SPI
    /* time out: 6s*5=30s */
    if(spi_errcnt++ > 5){
      storeParam_t param;
      param.ResetFlag = ZC_REPORT_SPI_RESTART;
      *(storeParam_t*) STATION_PARAM_ADDR = param;
      SystemReset();
    }
    #endif
    return events ^ STATION_EVENT_DETECT_BLAST;
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
  uint8 unknown = FALSE;
  switch(frm->hdr.delivermode){
    case DM_RTR_TO_STN:{
      switch(datatype){
        case LOCSYS_DATA_CARD_STATUS:{
          Station_SendAirFrmToSpi(frm);
          break;
        }
        case LOCSYS_DATA_CARD_INFO:{
          Station_SendAirFrmToSpi(frm);
          Station_ProcessCardInfoFrm(frm);
          break;
        }
        case LOCSYS_DATA_ROUTER_PATH:{
          Station_SendAirFrmToSpi(frm);
          break;
        }
        default:{
          unknown = TRUE;
          break;
        }
      }
      break;
    }
    case DM_HHD_TO_STN:{
      switch(datatype){
        case LOCSYS_DATA_CARD_INFO:{
          Station_SendAirFrmToSpi(frm);
          break;
        }
        default:{
          unknown = TRUE;
          break;
        }
      }
      break;
    }
    case DM_TUNNELLING:{
      Station_SendAirFrmToSpi(frm);
      break;
    }
    
    default:{
      unknown = TRUE;
      break;
    }
  }
  if(unknown && frm->hdr.dstAddr == NWK_STATION_ADDRESS){
    Station_SendUnknownAirFrmToSpi(frm);
  }
}
void Nwk_StateChangeCB(devStates_t state)
{
  Station_NwkState = state;
  switch(state){
    case DEV_STATE_HOLD:{
      Util_start_timerEx(Station_TaskId, STATION_EVENT_RFMAC, 1000);
      break;
    }
    case DEV_STATE_START:{
      app_startNwk_t app_startNwk;
      app_startNwk.msgtype = ZB_START_NETWORK;
      app_startNwk.PANID = Nwk_PanId();
      osal_cpyExtAddr((uint8*)app_startNwk.macAddr, (uint8*)Station_devinfo.ExtAddr);
      Station_SendDataToSpi(0, (uint8*)&app_startNwk, sizeof(app_startNwk));

      /* detect spi */
      Util_start_timerEx(Station_TaskId, STATION_EVENT_DETECT_BLAST, 5);

      /* report restart status */
      Util_start_timerEx(Station_TaskId, STATION_EVENT_REPORT_STATUS, 10);
      break;
    }
    case DEV_STATE_STARTFAIL:{
      storeParam_t param;
      param.ResetFlag = ZC_REPORT_STARTNWK_FAILED_RESTART;
      *(storeParam_t*) STATION_PARAM_ADDR = param;
      SystemReset();
      break;
    }
    case DEV_STATE_RESET:
      Station_Reset();

      HalLedSet(STATION_LED_RED, HAL_LED_MODE_ON);
      break;
    case DEV_STATE_ACTIVE:
      HalLedSet(STATION_LED_GREEN, HAL_LED_MODE_ON);
      Util_start_reload_timer(Station_TaskId, STATION_EVENT_SEND_BUFFERED_CMD, 200u);
      break;
    case DEV_STATE_INACTIVE:
      HalLedSet(STATION_LED_GREEN, HAL_LED_MODE_OFF);
      Util_stop_timer(Station_TaskId, STATION_EVENT_SEND_BUFFERED_CMD);
      break;
    default:
      break;
  }
}

void Station_Reset()
{
  Station_CmdMgr.cnt = 0;
  #ifdef LOCSYS_NO_SPI_CMD
  Util_start_reload_timer(Station_TaskId, STATION_EVENT_SIMULATE_CMD, 10000u);
  #endif
}

/* copy data and send to SPI */
uint8  Station_SendDataToSpi(uint16 shortAddr, const uint8 *p, uint16 len)
{
  if(len > MAX_SPI_PAYLOAD_SIZE){
    return FAILURE;
  }
  pSpi->spihdr.srcAddr.addr.shortAddr = shortAddr;
  pSpi->spihdr.dstAddr.addr.shortAddr = 0;
  pSpi->DataLength = len;
  osal_memcpy((void *)(pSpi + 1), (void *) p, pSpi->DataLength);

  #ifdef LOCSYS_NO_SPI
  return SUCCESS;
  #else
  return HalSPIWrite((uint8 *) pSpi, pSpi ->DataLength + SPIPKTHDR_LEN);
  #endif
}

/* data is prepared, just send to SPI */
uint8 Station_SendDataToSpiEx(uint16 shortAddr, uint16 len)
{
  if(len > MAX_SPI_PAYLOAD_SIZE){
    return FAILURE;
  }
  pSpi->spihdr.srcAddr.addr.shortAddr = shortAddr;
  pSpi->DataLength = len;

  #ifdef LOCSYS_NO_SPI
  return SUCCESS;
  #else
  return HalSPIWrite((uint8 *) pSpi, pSpi ->DataLength + SPIPKTHDR_LEN);
  #endif
}

static void Station_ProcessSPICB(const  SPIMSGPacket_t* rxmsg)
{
  if(rxmsg == NULL || rxmsg->DataLength > MAX_SPI_PAYLOAD_SIZE || rxmsg->spihdr.hdr.event != SPI_RX_MSG){
    return;
  }

  APPWrapper_t* appwrapper = (APPWrapper_t*)(rxmsg + 1);

  switch(appwrapper->app_flag){
    case LOCSYS_CMD:{
      LocSys_CmdHdr_t *cmdhdr = (LocSys_CmdHdr_t*)appwrapper;
      uint8 cmdcheck = LOCSYSCNF_SUCCESS;
      switch(cmdhdr->cmdtype){
        case LOCSYS_DATA_SEARCH_CARD:{
          cmdcheck = Station_ProcessBroadcastCmd(cmdhdr);
          break;
        }
        case LOCSYS_DATA_TUNNELLING:{
          cmdcheck = Station_ProcessBroadcastCmd(cmdhdr);
          break;
        }
        case LOCSYS_DATA_TUNNELLING2:{
          cmdcheck = Station_ProcessTunnelling2Cmd(cmdhdr);
          break;
        }
        default:
          cmdcheck = LOCSYSCNF_PARAM_INVALID;
      }
      app_LocSys_Cnf_t *cnf = (app_LocSys_Cnf_t*)cmdhdr;
      cnf->msgtype = LOCSYS_CNF;
      cnf->status = cmdcheck;
      Station_SendDataToSpi(0, (uint8*)cnf, sizeof(app_LocSys_Cnf_t));
      break;
    }
    case RFMAC_SET:{
      if(rxmsg->DataLength!=sizeof(app_rfmac_set_t)){
        return;
      }
      if(rfmac_setted == false && CRC16(appwrapper->app_rfmac_set.macAddr, 8, 0xFFFF) == appwrapper->app_rfmac_set.crc
         && appwrapper->app_rfmac_set.macAddr[EXT_MACADDR_TYPE] == EXT_MACADDR_TYPE_SUBSTATION){
        osal_memcpy((uint8*)Station_devinfo.ExtAddr, (uint8*)appwrapper->app_rfmac_set.macAddr, SADDR_EXT_LEN);

        Station_devinfo.Channel = Station_devinfo.ExtAddr[EXT_MACADDR_CHANNEL];

        /* start device directly */
        rfmac_setted = true;
        Util_stop_timer(Station_TaskId, STATION_EVENT_RFMAC);
        Nwk_SetDeviceInfo(&Station_devinfo);
        Nwk_Start();
      }
      break;
    }
        
    case SPI_DETECT:{
      if(rxmsg->DataLength!=sizeof(app_SPIDetect_t)){
        return;
      }

      bool checkflag = true;

      for(uint8 i = 0; i < SPIDETECT_LEN; i++){
        if(appwrapper->app_SPIDetect.detectdata[i] != i){
          checkflag = false;
        }
      }
      if(checkflag){
        spi_errcnt = 0;
      }
      break;
    }
    case SPI_DEBUG:{
      if(rxmsg->DataLength!=(sizeof(app_SPIDebug_t) + appwrapper->app_SPIDebug.debugstrlen)){
        return;
      }

      Station_SendDataToSpi(0, (void *) appwrapper, sizeof(app_SPIDebug_t) + appwrapper->app_SPIDebug.debugstrlen);
      break;
    }
    case RF_VERSION_REQ:{
      if(rxmsg->DataLength!=(sizeof(app_rfversion_t) + appwrapper->app_rfversion.size)){
        return;
      }

      uint8 * pData = (uint8*)(pSpi + 1) + sizeof(app_rfversion_t);

      sprintf((char*)pData, "SoftVer:%s, BuildTm:%s,%s, Ch:%d", STATION_SW_VERSION, __DATE__, __TIME__, macPib.logicalChannel);

      app_rfversion_t* pHead = (app_rfversion_t*)(pSpi + 1);
      pHead->msgtype = RF_VERSION_RSP;
      pHead->seq = appwrapper->app_rfversion.seq;
      pHead->size = strlen((char *)pData);
      Station_SendDataToSpiEx(0, sizeof(app_rfversion_t) + pHead->size);
      break;
    }

    default:{
      break;
    }
  }  
}

static uint8 Station_ProcessTunnelling2Cmd(LocSys_CmdHdr_t * cmdhdr)
{
  uint8* buf = Nwk_DataReqBufAlloc(cmdhdr->hdr.len);
  if(buf){
    osal_memcpy(buf, &cmdhdr->cmdtype, cmdhdr->hdr.len);
      
    nwkDataReq_t req;
    req.delivermode   = DM_TUNNELLING2;
    req.txopt         = 0;
    req.dstAddr       = cmdhdr->hdr.dstAddr;
    req.seqnum        = cmdhdr->hdr.seqnum;
    req.len           = cmdhdr->hdr.len;
    req.p             = buf;
    uint8 st = Nwk_DataReq(&req);
    Nwk_DataReqBufFree(buf);
    if(st == NWK_SUCCESS){
      return LOCSYSCNF_SUCCESS;
    }else{
      return LOCSYSCNF_BUFFER_FULL;
    }
  }else{
    return LOCSYSCNF_BUFFER_FULL;
  }
}

static uint8 Station_ProcessBroadcastCmd(LocSys_CmdHdr_t *cmdhdr)
{
  if(Station_NwkState != DEV_STATE_ACTIVE && Station_NwkState != DEV_STATE_INACTIVE){
    return LOCSYSCNF_PARAM_INVALID;
  }
  uint8 st = NWK_SUCCESS;
  if(Station_NwkState == DEV_STATE_ACTIVE){
    uint8* buf = Nwk_DataReqBufAlloc(cmdhdr->hdr.len);
    if(buf){
      osal_memcpy(buf, &cmdhdr->cmdtype, cmdhdr->hdr.len);
      
      nwkDataReq_t req;
      req.delivermode   = (cmdhdr->cmdtype == LOCSYS_DATA_TUNNELLING) ? DM_TUNNELLING : DM_STN_TO_RTR;
      req.txopt         = 0;
      req.dstAddr       = cmdhdr->hdr.dstAddr;
      req.seqnum        = cmdhdr->hdr.seqnum;
      req.len           = cmdhdr->hdr.len;
      req.p             = buf;
      st = Nwk_DataReq(&req);
      Nwk_DataReqBufFree(buf);
      if(st == NWK_SUCCESS){
        return LOCSYSCNF_SUCCESS;
      }
      if(st != NWK_BUSY){
        return LOCSYSCNF_BUFFER_FULL;
      }
    }else{
      return LOCSYSCNF_BUFFER_FULL;
    }
  }
  if(st == NWK_BUSY || Station_NwkState == DEV_STATE_INACTIVE){
    if(Station_CmdMgr.cnt < MAX_CMD_BUFFER_ENTRIES){
      uint8 len = sizeof(app_LocSys_Cmd_t) + cmdhdr->hdr.len;
      uint8* buf = osal_mem_alloc(len);
      if(buf){
        Station_CmdMgr.buf[Station_CmdMgr.cnt] = buf;
        Station_CmdMgr.cnt++;
        osal_memcpy(buf, (uint8*)cmdhdr, len);
        return LOCSYSCNF_SUCCESS;
      }else{
        return LOCSYSCNF_BUFFER_FULL;
      }
    }else{
      return LOCSYSCNF_BUFFER_FULL;
    }
  }
  return LOCSYSCNF_PARAM_INVALID;
}

static void Station_SendUnknownAirFrmToSpi(nwk_data_t * frm)
{
  if(frm->hdr.len + 1 > LOCSYS_MAX_DATA_LEN) return;

  uint8 extlen = Nwk_ExtHdrLen(frm);
  app_LocSys_Data_t *hdr = (app_LocSys_Data_t*)(pSpi + 1);
  hdr->msgtype      = LOCSYS_DATA;
  hdr->srcAddr      = frm->hdr.srcAddr;
  hdr->dstAddr      = frm->hdr.dstAddr;
  hdr->seqnum       = frm->hdr.dsn;
  hdr->len          = frm->hdr.len - extlen + 1;

  LocSys_Data_Tunnelling_t *tunnel = (LocSys_Data_Tunnelling_t*)(hdr + 1);
  tunnel->datatype  = LOCSYS_DATA_TUNNELLING;
  uint8 *data       = (uint8*)(tunnel + 1);
  osal_memcpy(data, &frm->payload[extlen], frm->hdr.len - extlen);
  Station_SendDataToSpiEx(hdr->srcAddr, sizeof(app_LocSys_Data_t) + hdr->len);
}

static void Station_SendAirFrmToSpi(nwk_data_t *frm)
{
  if(frm->hdr.len > LOCSYS_MAX_DATA_LEN) return;

  app_LocSys_Data_t *hdr = (app_LocSys_Data_t*)(pSpi + 1);
  hdr->msgtype      = LOCSYS_DATA;
  hdr->srcAddr      = frm->hdr.srcAddr;
  hdr->dstAddr      = frm->hdr.dstAddr;
  hdr->seqnum       = frm->hdr.dsn;
  hdr->len          = frm->hdr.len - Nwk_ExtHdrLen(frm);
  
  uint8 *data       = (uint8*)(hdr + 1);
  uint8 *comingdata = &frm->payload[Nwk_ExtHdrLen(frm)];
  
  osal_memcpy(data, comingdata, hdr->len);
  Station_SendDataToSpiEx(hdr->srcAddr, sizeof(app_LocSys_Data_t) + hdr->len);
}

static void Station_ProcessCardInfoFrm(nwk_data_t *frm)
{
  if(!frm->hdr.withpath) return;
  nwk_path_ext_t *pathhdr = (nwk_path_ext_t*)frm->payload;
  uint8 pathhdrlen = sizeof(nwk_path_ext_t) + pathhdr->depth * sizeof(uint16);
  LocSys_Data_CardInfo_t *desc = (LocSys_Data_CardInfo_t*)(frm->payload + pathhdrlen);
  uint8 srcrtglen = sizeof(nwk_srcrtg_ext_t) + pathhdr->depth * sizeof(uint16);
  uint8 *data = (uint8*)Nwk_DataReqBufAlloc(srcrtglen + sizeof(LocSys_Data_CardInfoAck_t));
  if(data){
    nwk_srcrtg_ext_t* srcrtg = (nwk_srcrtg_ext_t*)data;
    srcrtg->cnt = pathhdr->depth;
    srcrtg->idx = 0;
    osal_memcpy(srcrtg->routing, &pathhdr->path[1], sizeof(uint16) * (pathhdr->depth - 1));
    srcrtg->routing[pathhdr->depth - 1] = frm->hdr.srcAddr;
    LocSys_Data_CardInfoAck_t* ack = (LocSys_Data_CardInfoAck_t*)(data + srcrtglen);
    ack->cardaddress = desc->cardaddress;
    ack->datatype = LOCSYS_DATA_CARD_INFO_ACK;

    nwkDataReq_t req;
    req.delivermode = DM_STN_TO_RTR;
    req.dstAddr     = frm->hdr.srcAddr;
    req.seqnum      = frm->hdr.dsn;
    req.txopt       = NWK_TXOPTION_SRCROUTING;
    req.len         = srcrtglen + sizeof(LocSys_Data_CardInfoAck_t);
    req.p           = (uint8*)data;
    Nwk_DataReq(&req);
    Nwk_DataReqBufFree(data);
  }
}
/**************************************************************************************************
 **************************************************************************************************/
