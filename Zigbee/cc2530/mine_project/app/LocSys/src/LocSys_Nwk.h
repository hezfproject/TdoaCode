#ifndef LOCSYS_NWK_H
#define LOCSYS_NWK_H

#ifdef __cplusplus
extern "C" {
#endif

#include "hal_types.h"
#include "saddr.h"

#define NWK_BROADCAST_RETRANS       3


typedef struct{
  uint8         type:3;
  uint8         delivermode:5;
  uint8         srcrtg:1;
  uint8         withpath:1;
  uint8         dummy:6;
  uint16        srcAddr;
  uint16        dstAddr;
  uint8         dsn;
  uint8         len;
}nwk_frm_hdr_t;

typedef struct{
  uint8         cnt;
  uint8         idx;
  uint16        routing[0];
}nwk_srcrtg_ext_t;

typedef struct{
  uint8         depth;
  uint16        path[0];
}nwk_path_ext_t;

typedef struct{
  nwk_frm_hdr_t hdr;
  uint8         payload[0];
}nwk_data_t;

#define NWK_DATA_OFFSET                 (sizeof(nwk_data_t))
#define NWK_MAX_DATA_FRAME_SIZE         (MAC_MAX_FRAME_SIZE - NWK_DATA_OFFSET)

#define NWK_INVALID_DEPTH               0xFF


#define NWK_STATION_ADDRESS             0x0
#define NWK_INVALID_ADDRESS             0xFFFF
#define NWK_BROADCAST_ADDR              0xFFFF

/* Status */
#define NWK_SUCCESS                 0x00
#define NWK_FAILURE                 0x01
#define NWK_CONN_LOST               0x02
#define NWK_SYNC_LOST               0x03
#define NWK_NO_MEMORY               0x04
#define NWK_INVALID_PARAM           0x05
#define NWK_BUSY                    0x06

typedef enum
{
  DEV_STATE_HOLD,                       //station need app to start
  DEV_STATE_START,                      //station started
  DEV_STATE_STARTFAIL,                  //station start fail
  DEV_STATE_RESET,                      //nwk re-initialized
  DEV_STATE_ACTIVE,                     //enable data
  DEV_STATE_INACTIVE,                   //disable data
}devStates_t;

enum{
  DM_RTR_TO_CRD,
  DM_CRD_TO_RTR,
  DM_RTR_TO_STN,
  DM_STN_TO_RTR,
  DM_CRD_TO_HHD,
  DM_HHD_TO_CRD,
  DM_HHD_TO_STN,
  DM_STN_TO_HHD,
  DM_CRD_TO_ALL,
  DM_TUNNELLING,
  DM_TUNNELLING2,
  DM_DEBUG,
};

#define NWK_TXOPTION_SRCROUTING     0x01
#define NWK_TXOPTION_WITHPATH       0x02
typedef struct
{
  uint8             delivermode;
  uint16            dstAddr;
  uint8             seqnum;
  uint8             len;
  uint8            *p;                      //there must be NWK_DATA_OFFSET byte reserved before p,  
                                            //use Nwk_DataReqBufAlloc() to allocate p is a good manner,
                                            //use Nwk_DataReqBufFree() to free
  uint8             txopt;
  nwk_srcrtg_ext_t *srcrtg;
} nwkDataReq_t;

typedef struct
{
  sAddrExt_t        ExtAddr;
  uint8             Channel;
}deviceInfo_t;

extern uint32 current;
extern uint32 current320us;

extern void Nwk_TaskInit(uint8 taskId);

extern uint16 Nwk_ProcessEvent(uint8 taskId, uint16 events);

extern void Nwk_SetDeviceInfo(deviceInfo_t* info);

extern uint8 Nwk_DataReq(nwkDataReq_t *pData);

extern void Nwk_RxOnReq(uint8 appid);

extern void Nwk_RxOffReq(uint8 appid);

extern void Nwk_ComingDataCB(nwk_data_t *frm, int8 rssi);

extern void Nwk_StateChangeCB(devStates_t state);

extern uint8* Nwk_DataReqBufAlloc(uint8 len);

extern void Nwk_DataReqBufFree(void *p);

extern void Nwk_Start(void);

extern uint16 Nwk_PanId(void);

extern uint16 Nwk_ShortAddr(void);

extern uint8 Nwk_Depth(void);

extern uint8 Nwk_Path(uint16 *buf, uint8 depth);

extern uint8 Nwk_ExtHdrLen(nwk_data_t *frm);

#ifdef __cplusplus
};
#endif

#endif

