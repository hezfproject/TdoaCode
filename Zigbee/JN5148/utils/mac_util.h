#ifndef MAC_UTIL_H
#define MAC_UTIL_H

#ifdef __cplusplus
extern "C"
{
#endif

#include <mac_sap.h>

#ifndef __PACKED 
#define __PACKED __attribute__((__packed__))
#endif

#define MAC_UTIL_BROADCAST_SHORTADDR_DEVZCZR     0xFFFC
#define MAC_UTIL_BROADCAST_SHORTADDR_DEVALL       0xFFFF

#define MAC_UTIL_UNICAST      0
#define MAC_UTIL_BROADCAST   1

#define Z06_NWK_HDR_LEN 8
#define Z06_APS_HDR_LEN 8

#define MAC_MAX_DATA_PAYLOAD_LEN_SHORT MAC_MAX_PHY_PKT_SIZE-13         //with short address mode and FCS
#define Z06_MAX_DATA_PAYLOAD_LEN_SHORT MAC_MAX_DATA_PAYLOAD_LEN_SHORT-Z06_NWK_HDR_LEN-Z06_APS_HDR_LEN         //with short address mode 


typedef struct
{
	uint16  u16SrcPanId;
	uint16  u16SrcShortAddr;
	uint8  u8Dst_endpoint;
	uint8  u8Src_endpoint;
	uint16 u16Profile_id;
	uint8 u8NodeType;
} MacUtil_Setting_s;

typedef struct
{
	uint16  u16DstPanId;
	uint16  u16DstAddr;
	uint16  u16ClusterId;
	uint8    u8Radius;
	uint8    u8DeliverMode;
} MacUtil_SendParams_s;

typedef struct
{
	uint16 u16FCF;
	uint16 u16DstAddr;
	uint16 u16SrcAddr;
	uint8 u8BroadcastRadius;
	uint8 u8BroadcastSeq;	
}__PACKED Z06_NWK_Hdr_s;

typedef struct
{
	uint8 u8FCF;
	uint8 u8DstEndPoint;    
	uint16 u16ClusterID;
	uint16 u16ProfileID;
	uint8 u8SrcEndPoint;
	uint8 u8ApsCounter;
}__PACKED Z06_APS_Hdr_s;

typedef struct
{
    uint8 u8MsduHandle;
    uint8 u8NwkSeqNum;
    uint8 u8ApsSeqNum;    
}SeqNum_s;


PUBLIC void MacUtil_vInit(MacUtil_Setting_s *psParams);
PUBLIC uint8 MacUtil_vSendData(MacUtil_SendParams_s *psParams, uint8* pu16Data, uint8 u8Len, uint8 u8TxOptions);
PUBLIC SeqNum_s MacUtil_vSendDataZ06(MacUtil_SendParams_s *psParams, uint8* pu16Data, uint8 u8Len, uint8 u8TxOptions);
PUBLIC uint8* MacUtil_pu8GetApsPayloadZ06(uint8* pu8MacPayload);
PUBLIC void MacUtil_vRemoveChild(MAC_ExtAddr_s  sExtAddr, uint16 u16PanId);

PUBLIC uint16 MacUtil_u16GetClusterIdZ06(uint8* u8MacPayload);
PUBLIC void MacUtil_vReadExtAddress(MAC_ExtAddr_s *psExtAddress);

#ifdef __cplusplus
}
#endif

#endif
