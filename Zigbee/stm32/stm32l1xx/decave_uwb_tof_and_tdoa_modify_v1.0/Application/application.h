#ifndef _APPLICATION_H_
#define _APPLICATION_H_

#include "cc_def.h"
//#include "nanotron.h"

/* app	header */
typedef struct
{
	uint_8	  u8ProtocolType;	//Э������	  /* always be the source protocol type */
	uint_8	  u8MsgType;		//��������
	uint_16   u16MsgLen;		//������Ϣ���� Э��ͷ�� + ������ݳ���
} APP_HEADER_S;

//���⿨����ṹ����Ϣ
typedef struct 
{
	uint_16 u16TestCardID;              //�����ǩID
	uint_16 u16StandardCardID;          //��׼��ǩID
	uint_16 u16StationID;                //��վID           sys_option.u32BsId;
	uint_16 u16Cardseqnum;              //�����ǩ���к�
	uint_32 u32SQANHTieH;               //���������һ�ο췢��ʱ�����
	uint_32 u32SQANHTieL;               //���������һ�ο췢��ʱ�����
	uint_32 u32QQANHTieH;               //������һ�ο췢��ȥ��һ�ο췢��ʱ�����
	uint_32 u32QQANHTieL; 
	uint_8  u8DevType;	  				// �����ͣ�1s������5s��
	uint_8  u8Status;					//��״̬
	int_8   i8Rssi;						//�ź�ǿ��
	uint_8  u8Reserved;
}APP_UWB_TDOA_TIMESTAMP_S;

//�������㼶Լ���õ�����Э��ṹ Э��ͷ + ���ݴ�С
typedef struct     
{
	APP_HEADER_S stAppTdoaHead;
	APP_UWB_TDOA_TIMESTAMP_S stAppTdoaMsg[APP_UWBTDOA_MAX_CARD_NUM];	
} APP_UWB_TDOA_DITANCE_S;    //���뵽Э���ļ���

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
	 ע:�˽ṹ���Ⱥ�˳������,��Ȼ���ܻᵼ�²�����ȷ��� 
	 *******************************************************/
	TDOA_BSMAC_PACKET_HEADER_S   stBsmacPackHead;    
	TDOA_NET_PACKET_HEADER_S     stNetPackHead;
	uint8 u8PackDataBuff[300];
	//CRCУ��λ
   // uint8 u8TxCrcH;
   // uint8 u8TxCrcL;
}TDOA_BSMAC_BUILD_PACK_S;

void Application(void);
void Appclear_distancelist(void);
void TdoaRxCardMsgProc(void);
void TdoaSendCardReportToUart(void);
void vReportCardDistance(void);


#endif

