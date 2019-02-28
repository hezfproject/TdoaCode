#ifndef _TCP_PROTOCOL_H_
#define _TCP_PROTOCOL_H_

/// 传输接口与分析程序交互的命令类型
enum TCP_Tag_E
{
	TCP_Tag_DataUp = 2048,
	TCP_Tag_DataDown = 2049,
	TCP_Tag_DownSyncTime = 2050,
	TCP_Tag_HeartBeat = 2051,
	TCP_Tag_LinkMsg = 2052,
};

/// 基站网口上报总协议头
typedef struct
{
	signed long tag;
	signed long len;
}TCP_Hdr_T;

/// 基站网口数据信息头
typedef struct
{
	unsigned long timestamp;
    unsigned short panid;
	unsigned short datalen;
}TCP_Data_Hdr_T;

/// 同步时间命令
typedef struct
{
	signed long time;
}TCP_DownSyncTime_T;

typedef struct
{
    unsigned short  panid;
    unsigned short  port;
    signed  long    timestamp;
    signed  long    datalen;
}TCP_LinkMsg_Hdr_T;

#endif
