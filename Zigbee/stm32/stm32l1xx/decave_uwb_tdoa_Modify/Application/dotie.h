#include"type_def.h"
#include "app_protocol.h"

#define array_max 20
#define Tick_max 1099511627775             //0xffffffffff �����tick��
#define standard_num_max    (5)  //ͬһ����׼�ܹ����յ������Ļ�׼��վ������
#define send_max   (50)          //������� TimeStamp_mgr �ĸ��� 
/*
typedef struct Rxmsg_data 
{
	uint64 own_timestamp;
	uint16 seqnum;      
	uint16 cardid;     //���⿨ID 
	uint8 speedtype;
	uint8 status;
	int8 i8Rssi;
	uint8 u8DevType;
}Rx_msg;               //��ǩ�Լ���׼��ǩ����������Ϣ

typedef struct standard_msg_data 
{
	uint32 own_timestamp_H;
	uint32 own_timestamp_L;
	uint16 seqnum;      
	uint16 cardid;     //���⿨ID 
}Rx_standard_msg;


typedef struct AVG_data           //��վʱ���
{
	uint64 avg_TIE;
	uint64 Max_tie;
	uint64 Min_tie;
	int count;              //����ƽ������Ŀ
}AVG_TIE;

typedef struct msgarray          //�Ի�׼��ǩ�� 
{
	Rx_standard_msg pre_quick;    //ǰһ��ʱ���
	Rx_standard_msg last_quick;    //���ڵĺ�һ��ʱ���
	uwb_tdoa_TimeStamp_ts send_msg[array_max];	//���⿨�������Ϣ
	uint16 n;                    //���췢֮���յ��Ĵ��������
	uint16 lost;              //���Ի�׼��ǩ��ʧʹ�ã���ʱ����
	uint16 start_seq;         //��׼��ǩ��ʼ��seq
	uint8 type;               //0Ϊδ����1Ϊ������������� 
}Msg_array;

typedef struct sendpack
{
	uint16 n;
//	TimeStamp_mgr send_msg[send_max];
	uwb_tdoa_TimeStamp_ts send_msg[send_max];
}Send_Pack;




void clear_Array(Msg_array *msg_array);

void build_unity(Rx_msg rx_msg,Msg_array *msg_array,int k);

int check_standardID(Rx_msg rx_msg,Msg_array msg_arr[]);

*/

/***********************************************************************
����:send_pack �����ݷ��ͳ�ȥ������������ݹ���ֱ�ӷ��ͳ�ȥ�����Ƚ�С
	����뵽pack�д���һ����ʱ���ͳ�ȥ
����:
	pack :�����͵İ��ռ�
	msg_array :�����뵽pack������
	time :����pack���Ĵ���
����: ��
***********************************************************************/

//void send_pack(Send_Pack *pack,Msg_array *msg_array,int *con_times);



