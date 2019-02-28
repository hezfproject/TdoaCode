#include"type_def.h"
#include "app_protocol.h"

#define array_max 20
#define Tick_max 1099511627775             //0xffffffffff 即最大tick数
#define standard_num_max    (5)  //同一个基准能够接收到的最多的基准基站的数量
#define send_max   (50)          //发送最大 TimeStamp_mgr 的个数 
/*
typedef struct Rxmsg_data 
{
	uint64 own_timestamp;
	uint16 seqnum;      
	uint16 cardid;     //待测卡ID 
	uint8 speedtype;
	uint8 status;
	int8 i8Rssi;
	uint8 u8DevType;
}Rx_msg;               //标签以及基准标签发过来的信息

typedef struct standard_msg_data 
{
	uint32 own_timestamp_H;
	uint32 own_timestamp_L;
	uint16 seqnum;      
	uint16 cardid;     //待测卡ID 
}Rx_standard_msg;


typedef struct AVG_data           //基站时间戳
{
	uint64 avg_TIE;
	uint64 Max_tie;
	uint64 Min_tie;
	int count;              //计入平均的数目
}AVG_TIE;

typedef struct msgarray          //以基准标签放 
{
	Rx_standard_msg pre_quick;    //前一个时间戳
	Rx_standard_msg last_quick;    //相邻的后一个时间错
	uwb_tdoa_TimeStamp_ts send_msg[array_max];	//待测卡的相关信息
	uint16 n;                    //两快发之间收到的待测的数量
	uint16 lost;              //测试基准标签丢失使用，暂时不用
	uint16 start_seq;         //基准标签开始的seq
	uint8 type;               //0为未满，1为满可以组包发出 
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
作用:send_pack 将数据发送出去，若传入的数据够大，直接发送出去，若比较小
	则放入到pack中待包一定大时发送出去
参数:
	pack :待发送的包空间
	msg_array :待放入到pack的数据
	time :放入pack包的次数
返回: 空
***********************************************************************/

//void send_pack(Send_Pack *pack,Msg_array *msg_array,int *con_times);



