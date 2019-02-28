#ifndef _APPLICATION_H_
#define _APPLICATION_H_

#include "cc_def.h"
//#include "nanotron.h"



typedef struct
{
    uint8 u8PreambleH;
    uint8 u8PreambleL;
    /* bit filed depends on endian
    unsigned char frame_type    :3;
    unsigned char ready              :1;
    unsigned char device_type   :2;
    unsigned char filler               :1; //filler
    unsigned char priority           :1;
    */
    uint8 u8frameControl;
    uint8 u8Reserverd;
    uint8 u8FrameCountH;
    uint8 u8FrameCountL;
    uint8 u8SrcAddrH;
    uint8 u8SrcAddrL;
    uint8 u8DstAddrH;
    uint8 u8DstAddrL;
    uint8 u8DataLenH;
    uint8 u8DataLenL;
}TDOA_BSMAC_PACKET_HEADER_S;

typedef struct 
{
	uint8		u8Type;
	uint8		u8Ttl;
	uint16	    u16Src;           //pan_addr_t src
	uint16 	    u16Dst;
	uint16		u16Len;
	/* The options start here */
}TDOA_NET_PACKET_HEADER_S;

typedef struct
{
	/*******************************************************
	 注:此结构体先后顺序不能乱,不然可能会导致不能正确解包 
	 *******************************************************/
	TDOA_BSMAC_PACKET_HEADER_S   stBsmacPackHead;    
	TDOA_NET_PACKET_HEADER_S     stNetPackHead;
	uint8 u8PackDataBuff[300];
	//CRC校验位
   // uint8 u8TxCrcH;
   // uint8 u8TxCrcL;
}TDOA_BSMAC_BUILD_PACK_S;

void Application(void);
void DecaTdoaEquipEventSet(void);
void TdoaMsgPackInsetUartBuff(TDOA_UWB_TIMESTAMP_MSG_ARRAY_S* pstTdoaMsgArray);
void TdoaDataBuffClear(void);


#endif

