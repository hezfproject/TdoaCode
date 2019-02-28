#ifndef _APPLICATION_H_
#define _APPLICATION_H_

#include "cc_def.h"
//#include "nanotron.h"

/* app	header */
typedef struct
{
	uint_8	  u8ProtocolType;	//协议类型	  /* always be the source protocol type */
	uint_8	  u8MsgType;		//数据类型
	uint_16   u16MsgLen;		//发送消息长度 协议头长 + 测距数据长度
} APP_HEADER_S;

//待测卡组包结构中信息
typedef struct 
{
	uint_16 u16TestCardID;              //待测标签ID
	uint_16 u16StandardCardID;          //基准标签ID
	uint_16 u16StationID;                //基站ID           sys_option.u32BsId;
	uint_16 u16Cardseqnum;              //待测标签序列号
	uint_32 u32SQANHTieH;               //慢发与最近一次快发的时间戳差
	uint_32 u32SQANHTieL;               //慢发与最近一次快发的时间戳差
	uint_32 u32QQANHTieH;               //慢发后一次快发减去上一次快发的时间戳差
	uint_32 u32QQANHTieL; 
	uint_8  u8DevType;	  				// 卡类型，1s卡还是5s卡
	uint_8  u8Status;					//卡状态
	int_8   i8Rssi;						//信号强度
	uint_8  u8Reserved;
}APP_UWB_TDOA_TIMESTAMP_S;

//与其他层级约定好的数据协议结构 协议头 + 数据大小
typedef struct     
{
	APP_HEADER_S stAppTdoaHead;
	APP_UWB_TDOA_TIMESTAMP_S stAppTdoaMsg[APP_UWBTDOA_MAX_CARD_NUM];	
} APP_UWB_TDOA_DITANCE_S;    //放入到协议文件中

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
void Appclear_distancelist(void);
void TdoaRxCardMsgProc(void);
void TdoaSendCardReportToUart(void);
void vReportCardDistance(void);


#endif

