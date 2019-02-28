#ifndef _TCP_PROTOCOL_H_
#define _TCP_PROTOCOL_H_

/// ����ӿ���������򽻻�����������
enum TCP_Tag_E
{
	TCP_Tag_DataUp = 2048,
	TCP_Tag_DataDown = 2049,
	TCP_Tag_DownSyncTime = 2050,
	TCP_Tag_HeartBeat = 2051,
	TCP_Tag_LinkMsg = 2052,
};

/// ��վ�����ϱ���Э��ͷ
typedef struct
{
	signed long tag;
	signed long len;
}TCP_Hdr_T;

/// ��վ����������Ϣͷ
typedef struct
{
	unsigned long timestamp;
    unsigned short panid;
	unsigned short datalen;
}TCP_Data_Hdr_T;

/// ͬ��ʱ������
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
