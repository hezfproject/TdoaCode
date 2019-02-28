// -------------------------------------------------------------------------------------------------------------------
//
//  File: instance.h - DecaWave header for application level instance
//
//  Copyright (c) 2008 - DecaWave Ltd, Dublin, Ireland.
//
//  All rights reserved.
//
//  Author: Billy Verso, October 2008
//
//本文件用于TDOA代码实例编译 由于历史原因，使用人卡编译待测卡和学习卡时需要将config.h中的宏DEC_UWB_ANCHOR开启
//进行基站编译时不需要将config.h中的宏DEC_UWB_ANCHOR开启
// -------------------------------------------------------------------------------------------------------------------

#ifndef _INSTANCE_H_
#define _INSTANCE_H_

#ifdef __cplusplus
extern "C" {
#endif

//#include "instance_sws.h"
#include "deca_types.h"
#include "deca_device_api.h"
#include "app_protocol.h"
#include "dotie.h"

#if 1
/*************** 实例设备类型 TDOA_INST_MODE_E *****************/
#define   TDOA_INST_MODE_ANCHOR         //基站
//#define   TDOA_INST_MODE_TAG_TEST       //待测卡
//#define   TDOA_INST_MODE_TAG_STANDARD   //学习卡

/*********************设备参数设置*****************************/
#define DEV_TAG_ANCHOR_ADDRESS          30003    //基站ID
#define DEV_TAG_TEST_ADDRESS            10009    //待测卡ID   10009
#define DEV_TAG_STANDARD_ADDRESS        10007    //快发卡ID
#define SLOW_SPEED_TAG_SEND_TIME  		1000     //待测卡触发周期即待测卡信息发送周期 400
#define QUICK_SPEED_TAG_SEND_TIME   	150      //待测卡触发周期即待测卡信息发送周期 45
#define DEVICE_UART_SEND_TIME   	    1000     //串口缓冲区信息发送周期 1000 不能大于卡的轮询周期


#ifdef TDOA_INST_MODE_ANCHOR 
#define TDOA_INST_MODE_TYPE             TDOA_INST_ANCHOR        //基站
#define DEV_ADDRESS_CARDID        		DEV_TAG_ANCHOR_ADDRESS //设备卡ID设置 设备的16位短地址
#endif

#ifdef TDOA_INST_MODE_TAG_TEST 
#define TDOA_INST_MODE_TYPE             TDOA_INST_TAG_TEST        //待测卡 需要开启config.h中的宏DEC_UWB_ANCHOR
#define DEV_ADDRESS_CARDID        		DEV_TAG_TEST_ADDRESS      //设备卡ID设置 设备的16位短地址
#endif

#ifdef TDOA_INST_MODE_TAG_STANDARD 
#define TDOA_INST_MODE_TYPE             TDOA_INST_TAG_STANDARD  //学习卡
#define DEV_ADDRESS_CARDID        		DEV_TAG_STANDARD_ADDRESS      //设备卡ID设置 设备的16位短地址
#endif

/*********************设备监控点设置*****************************/
#define   PRINTF_vReportCardDistance_SendData     //发送到stm的信息

//#define DWT_TIME_UNITS          (1.0/499.2e6/128.0) //= 15.65e-12 s 一个dw1000 tick的时间大小
#define IF_DESC(x)   1

#endif

#define NUM_INST            1
#define SPEED_OF_LIGHT      (299702547.0)     // in m/s in air
#define MASK_40BIT			(0x00FFFFFFFFFF)  // MP counter is 40 bits
#define MASK_TXDTS			(0x00FFFFFFFE00)  //The TX timestamp will snap to 8 ns resolution - mask lower 9 bits. 

/******************************************************************************************************************
********************* NOTES on DW (MP) features/options ***********************************************************
*******************************************************************************************************************/
#define DEEP_SLEEP (1)//To enable deep-sleep set this to 1
//DEEP_SLEEP mode can be used, for example, by a Tag instance to put the DW1000 into low-power deep-sleep mode, there are two cases:
// 1. when the Anchor is sending the range report back to the Tag, the Tag will enter sleep after a ranging exchange is finished
// once it receives a report or times out, before the next poll message is sent (before next ranging exchange is started).
// 2. when the Anchor is not sending the report the Tag will go automatically to sleep after the final message is sent

#define DEEP_SLEEP_XTAL_ON (0)
//NOTE: on the EVK1000 the DEEPSLEEP is not actually putting the DW1000 into full DEEPSLEEP mode as XTAL is kept on 


#define ENABLE_AUTO_ACK		(1)		//To enable auto-ack feature set this to 1, frame filtering also needs to be set (to allow ACK frames)
#define ACK_RESPONSE_TIME	(5)     //ACK response time is 5 us (5 symb.), the instance receiving an ACK request will send the ACK after this delay.
#define WAIT_FOR_RESPONSE_DLY	(0) //Tx to Rx delay time, the instance waiting for response will delay turning the receiver on by this amount
// The receiver will be enabled automatically after frame transmission if DWT_RESPONSE_EXPECTED is set in dwt_starttx.
// The instance requesting an ACK, can also program a delay in turning on its receiver, if it knows that the ACK will be sent after particular delay.
// Here it is set to 0 us, which will enable the receiver a.s.a.p so it is ready for the incoming ACK message. 
// The minimum ACK response time about 5us, and the IEEE standard, specifies that the ACK has to be able to be sent within 12 us of reception of an ACK request frame.

#define SNIFF_MODE	(0) //set to 1 to enable sniff mode (low-power listening mode) Sniff (PPM) mode means that the receiver
// pulses though ON and OFF state, in the ON state all the RF blocks are on/powered  and they are off in the OFF state
// The ON time is set to 2 (= 2+1 PACs) and the off time is 255us - MAX because of a bug in MPW3 (the off timer counts in clock cycles 8ns!).

//Note:	Double buffer mode can only be used with auto rx re-enable, auto rx re-enable is on by default in Listener and Anchor instances
#define DOUBLE_RX_BUFFER (0) //To enable double RX buffer set this to 1 - this only works for the Listener instance
//NOTE: this feature is really meant for a RX only instance, as TX will not be possible while double-buffer and auto rx-enable is on.

#define DR_DISCOVERY	(1) //to use discovery ranging mode (tag will blink until it receives ranging request from an anchor)

#define CORRECT_RANGE_BIAS  (1)     // Compensate for small bias due to uneven accumulator growth at close up high power

#define DEEP_SLEEP_AUTOWAKEUP (0) //to test SLEEP mode

#define PUT_TEMP_VOLTAGE_INTO_POLL   (0)     // to insert wakeup sampled TEMP/VOLTAGE into POLL message

#define MAIN_AND_SUB_RANGING         (1)    //will tof to main_station and sub_station

#define BLINK_AGAIN_IN_END           (1)    //every time after the tof or the blink process ,if there are enaught time ,send blink again

#define POLL_HAVE_BLINK_MSG          (0)    //POLL process have blink's next 1s and 5s idle slot massege ,destaddr= 0xffff,


// on last sector tail 6byte
#define DEV_ADDRESS (0x0801FF00)
/******************************************************************************************************************
*******************************************************************************************************************
*******************************************************************************************************************/

#define USING_64BIT_ADDR (0) //when set to 0 - the DecaRanging application will use 16-bit addresses


#define SIG_RX_ACK				5		// Frame Received is an ACK (length 5 bytes)
#define SIG_RX_BLINK			7		// Received ISO EUI 64 blink message
#define SIG_RX_BLINKDW			8		// Received ISO EUI 64 DW blink message
#define mytestsize         88
//lengths including the Decaranging Message Function Code byte

//#define STANDARD_FRAME_SIZE         88
//#define MAX_MAC_MSG_DATA_LEN                (STANDARD_FRAME_SIZE)//(TAG_FINAL_MSG_LEN) //max message len of the above



#if (USING_64BIT_ADDR==1)
#define ADDR_BYTE_SIZE              (2)
#else
#define ADDR_BYTE_SIZE              (2)
#endif
#define ADDR_BYTE_SIZE_SHORT        (2)

#define FRAME_CONTROL_BYTES         2
#define FRAME_SEQ_NUM_BYTES         1
#define FRAME_PANID                 2
#define FRAME_CRC					2
#define FRAME_SOURCE_ADDRESS        (ADDR_BYTE_SIZE)
#define FRAME_DEST_ADDRESS          (ADDR_BYTE_SIZE)
#define FRAME_CTRLP					(FRAME_CONTROL_BYTES + FRAME_SEQ_NUM_BYTES + FRAME_PANID) //5
#define FRAME_CRTL_AND_ADDRESS      (FRAME_DEST_ADDRESS + FRAME_SOURCE_ADDRESS + FRAME_CTRLP) //21 bytes (or 9 for 16-bit addresses)
#define MAX_USER_PAYLOAD_STRING    88//(STANDARD_FRAME_SIZE-FRAME_CRTL_AND_ADDRESS-TAG_FINAL_MSG_LEN-FRAME_CRC) //127 - 21 - 16 - 2 = 88 (or 100 for 16-bit)

#define BLINK_FRAME_CONTROL_BYTES       1
#define BLINK_FRAME_SEQ_NUM_BYTES       1
#define BLINK_FRAME_CRC					2
#define BLINK_FRAME_SOURCE_ADDRESS      8
#define BLINK_FRAME_CTRLP				(BLINK_FRAME_CONTROL_BYTES + BLINK_FRAME_SEQ_NUM_BYTES) //2
#define BLINK_FRAME_CRTL_AND_ADDRESS    (BLINK_FRAME_SOURCE_ADDRESS + BLINK_FRAME_CTRLP) //10 bytes

#define ANCHOR_LIST_SIZE			(4)
#define TAG_LIST_SIZE				(50)
#define TAG_RANGE_SLOT  			(20*CLOCKS_PER_MILLI)
#define TAG_LIST_TIME 				(TAG_RANGE_SLOT*TAG_LIST_SIZE)	

#define FIXED_REPLY_DELAY            5 //ms  //response delay time (Tag or Anchor when sending Final/Response messages respectively)
#define FIXED_LONG_REPLY_DELAY       150
#define FIXED_LONG_BLINK_RESPONSE_DELAY       (5*FIXED_LONG_REPLY_DELAY) //NOTE: this should be a multiple of FIXED_LONG_REPLY_DELAY see DELAY_MULTIPLE below
#define DELAY_MULTIPLE				(FIXED_LONG_BLINK_RESPONSE_DELAY/FIXED_LONG_REPLY_DELAY - 1)

#define WAKE_UP_REDAY_TIME       15   //MS, when wakeup card into the event need some time to ready
#define ANCHOR_TOF_PRO            3     //the card in tof process
#define ANCHOR_BLINK_PRO          3     //the card in blink process


typedef enum cardtypeModes{L_IDLE=0,CARD_1S=1,CARD_5S=5,OTHER=6} SLOTLIST_MODE;

typedef enum slotstatus{IDLE=0,USED_TOF=1,USED_UART=2,USED_BLINK=3 } SLOT_STATUS;

//Listener = in this mode, the instance only receives frames, does not respond
//Tag = Exchanges DecaRanging messages (Poll-Response-Final) with Anchor and enabling Anchor to calculate the range between the two instances
//Anchor = see above

// This file defines data and functions for access to Parameters in the Device

// -------------------------------------------------------------------------------------------------------------------
// structure to hold device's logging data


typedef struct
{
    uint8 frameCtrl[2];                         	//  frame control bytes 00-01
    uint8 seqNum;                               	//  cur slot
    uint8 panID[2];                             	//  PAN ID 03-04
    uint8 destAddr[ADDR_BYTE_SIZE];             	//  05-06 or using 64 bit addresses (05-12)
    uint8 sourceAddr[ADDR_BYTE_SIZE];           	//  07-08 or using 64 bit addresses (13-20)
    uint8 messageData[MAX_USER_PAYLOAD_STRING] ;    //  22-124 (application data and any user payload)
    uint8 fcs[2] ;                              	//  125-126  we allow space for the CRC as it is logically part of the message. However ScenSor TX calculates and adds these bytes.
} srd_msg ;

//12 octets for Minimum IEEE ID blink
typedef struct
{
    uint8 frameCtrl;                         		//  frame control bytes 00
    uint8 seqNum;                               	//  sequence_number 01
    uint8 tagID[ADDR_BYTE_SIZE];           			//  02-09 64 bit addresses
    uint8 fcs[2] ;                              	//  10-11  we allow space for the CRC as it is logically part of the message. However ScenSor TX calculates and adds these bytes.
} iso_IEEE_EUI64_blink_msg ;

//18 octets for IEEE ID blink with Temp and Vbat values
typedef struct
{
    uint8 frameCtrl;                         		//  frame control bytes 00
    uint8 seqNum;                               	//  sequence_number 01
    uint8 tagID[ADDR_BYTE_SIZE];           			//  02-09 64 bit addresses
	uint8 enchead[2];								//  10-11 2 bytes (encoded header and header extension)
	uint8 messageID;								//  12 message ID (0xD1) - DecaWave message
	uint8 temp;										//  13 temperature value
	uint8 vbat;										//  14 voltage value
	uint8 gpio;										//  15 gpio status
    uint8 fcs[2] ;                              	//  16-17  we allow space for the CRC as it is logically part of the message. However ScenSor TX calculates and adds these bytes.
} iso_IEEE_EUI64_blinkdw_msg ;


typedef struct
{
    uint8 frameCtrl[2];                         	//  frame control bytes 00-01
    uint8 seqNum;                               	//  sequence_number 02
    uint8 fcs[2] ;                              	//  03-04  CRC
} ack_msg ;


typedef struct
{
    uint8 frameCtrl[2];                         	//  frame control bytes 00-01
    uint8 seqNum;                               	//  sequence_number 02
    uint8 panID[2];                             	//  PAN ID 03-04
    uint8 destAddr[ADDR_BYTE_SIZE_SHORT];           //  05-06
    uint8 sourceAddr[ADDR_BYTE_SIZE];           	//  07-08 or using 64 bit addresses (07 - 14)
    uint8 messageData[MAX_USER_PAYLOAD_STRING] ;    //  15-124 (application data and any user payload)
    uint8 fcs[2] ;                              	//  125-126  we allow space for the CRC as it is logically part of the message. However ScenSor TX calculates and adds these bytes.
} srd_msg_bcast ;

typedef struct
{
    unsigned char channelNumber ;       // valid range is 1 to 11
    unsigned char preambleCode ;        // 00 = use NS code, 1 to 24 selects code
    unsigned char pulseRepFreq ;        // NOMINAL_4M, NOMINAL_16M, or NOMINAL_64M
    unsigned char dataRate ;            // DATA_RATE_1 (110K), DATA_RATE_2 (850K), DATA_RATE_3 (6M81)
    unsigned char preambleLen ;         // values expected are 64, (128), (256), (512), 1024, (2048), and 4096
    unsigned char pacSize ;
    unsigned char nsSFD ;
} instanceConfig_t ;


#if (DR_DISCOVERY == 0)
typedef struct
{
	uint64 forwardToFRAddress;
    uint64 anchorAddress;
	uint64 *anchorAddressList;
	int anchorListSize ;
	int anchorPollMask ;
	int sendReport ;
} instanceAddressConfig_t ;
#endif


//function codes
#define RTLS_DEMO_MSG_RNG_INIT				      (0x20)			// Ranging initiation message
#define RTLS_DEMO_MSG_TAG_POLL              (0x21)          // Tag poll message
#define RTLS_DEMO_MSG_ANCH_RESP             (0x10)          // Anchor response to poll
#define RTLS_MSG_HELP_CALL                  (0x23)          //card ask help 
#define RTLS_MSG_HELP_RESP                  (0x24)          //station rev help and resp
#define RTLS_EVACUATE_ASK_SEND              (0x25)          //evacute alarm 
#define RTLS_EXCIT_ASK_SEND                 (0x26)          //exicit 
#define RTLS_EXCIT_ACK_SEND                 (0x27)          //exicit 
#define RTLS_TDOA_BLINK_SEND                (0x28)          //tdoa blink消息包类型
#define RTLS_RETREAT_ACK_SEND               (0x2B)          //press the buttun to stop retreat


//设备实例发送接收帧相应的长度定义
#define TAG_POLL_MSG_LEN                    1				// FunctionCode(1),
#define ANCH_RESPONSE_MSG_LEN               9               // FunctionCode(1), RespOption (1), OptionParam(2), Measured_TOF_Time(5)
#define TAG_FINAL_MSG_LEN                   16              // FunctionCode(1), Poll_TxTime(5), Resp_RxTime(5), Final_TxTime(5)
#define RANGINGINIT_MSG_LEN					7				// FunctionCode(1), Tag Address (2), Response Time (2) * 2

#define STANDARD_FRAME_SIZE         127  //标准架构帧结构长度 扩展长度为1024字节

#define ADDR_BYTE_SIZE_L            (8)  //长地址使用长度
#define ADDR_BYTE_SIZE_S            (2)  //短地址使用长度

#define FRAME_CONTROL_BYTES         2    //收发帧的控制位长度
#define FRAME_SEQ_NUM_BYTES         1    //收发帧的序列号占用长度
#define FRAME_PANID                 2    //收发帧的Panid长度
#define FRAME_CRC					2	 //收发帧的CRC校验长度
#define FRAME_SOURCE_ADDRESS_S        (ADDR_BYTE_SIZE_S)
#define FRAME_DEST_ADDRESS_S          (ADDR_BYTE_SIZE_S)
#define FRAME_SOURCE_ADDRESS_L        (ADDR_BYTE_SIZE_L)
#define FRAME_DEST_ADDRESS_L          (ADDR_BYTE_SIZE_L)
#define FRAME_CTRLP					(FRAME_CONTROL_BYTES + FRAME_SEQ_NUM_BYTES + FRAME_PANID) //5
#define FRAME_CRTL_AND_ADDRESS_L    (FRAME_DEST_ADDRESS_L + FRAME_SOURCE_ADDRESS_L + FRAME_CTRLP) //21 bytes for 64-bit addresses)
#define FRAME_CRTL_AND_ADDRESS_S    (FRAME_DEST_ADDRESS_S + FRAME_SOURCE_ADDRESS_S + FRAME_CTRLP) //9 bytes for 16-bit addresses)
#define FRAME_CRTL_AND_ADDRESS_LS	(FRAME_DEST_ADDRESS_L + FRAME_SOURCE_ADDRESS_S + FRAME_CTRLP) //15 bytes for 1 16-bit address and 1 64-bit address)
#define MAX_USER_PAYLOAD_STRING_LL     (STANDARD_FRAME_SIZE-FRAME_CRTL_AND_ADDRESS_L-TAG_FINAL_MSG_LEN-FRAME_CRC) //127 - 21 - 16 - 2 = 88
#define MAX_USER_PAYLOAD_STRING_SS     (STANDARD_FRAME_SIZE-FRAME_CRTL_AND_ADDRESS_S-TAG_FINAL_MSG_LEN-FRAME_CRC) //127 - 9 - 16 - 2 = 100
#define MAX_USER_PAYLOAD_STRING_LS     (STANDARD_FRAME_SIZE-FRAME_CRTL_AND_ADDRESS_LS-TAG_FINAL_MSG_LEN-FRAME_CRC) //127 - 15 - 16 - 2 = 94

//设备使能和未使能标志
#define TDOA_EABLE      1
#define TDOA_DISEABLE   0

#define TDOA_UART_SEND_MAX_TAG_NUM  	  8               //串口发送一次性最多发送的卡数据

#define TDOA_MSG_TESTCARD_MAX_NUM  	  20                  //组包信息中待测卡数量最大值
#define TDOA_STANDCARD_MAX_NUM       (5)                  //同一个基准能够接收到的最多的基准基站的数量
#define TDOA_PACK_SEND_MAX_NUM       (50)                 //发送最大 TimeStamp_mgr 的个数 组包后往串口发送前需要进行包数量统计
#define TDOA_LOST_POLL_PACK_COUNT     5                   //基站的丢包数

#define TDOA_DW_TICK_MAX             1099511627775       //0xffffffffff 即最大tick数

#define TDOA_INST_PANID_TAG  		  0x2222              //待测卡的Panid
#define TDOA_INST_PANID_TAG_STANDARD  0x1111              //学习卡的Panid
#define TDOA_INST_PANID_ANCHOR        0x1111              //基站的panid

//射频延时默认值
#define DWT_PRF_64M_RFDLY   (515.6f)
#define DWT_PRF_16M_RFDLY   (515.0f)
extern const uint16 gau16RfDelays[2];

#define TDOA_INST_MODE_SPEED_SLOW    0x0                 //轮询发送帧中标记待测卡
#define TDOA_INST_MODE_SPEED_QUCIK   0x1				 //轮询发送帧中标记快发卡


#define TDOA_INST_ACK_REQUESTED               (1)        //请求确认帧 Request an ACK frame
#define TDOA_INST_SEND_POLL_DEST_ADDR        (0xFFFF)    //(0xFFFF)     //待测卡和快发卡的目的地址设置为广播地址
#define TDOA_INST_FRAME_TYPE_BIT              0          //数据帧类型 标记是否是轮询消息数据
#define TDOA_INST_FRAME_SPEED_TYPE_BIT        1          //数据帧中记录标签卡的速率类型位
#define TDOA_INST_FRAME_PANID_BIT_H           2          //数据帧中记录设备panid的高位部分数据
#define TDOA_INST_FRAME_PANID_BIT_L           3          //数据帧中记录设备panid的低位部分数据

#define TDOA_INST_FRAME_SEQNUM_BIT_H          4          //数据帧中记录消息序列号的高位部分数据
#define TDOA_INST_FRAME_SEQNUM_BIT_L          5          //数据帧中记录消息序列号的低位部分数据
#define TDOA_INST_FRAME_ARRAY_DATA_LEN        6          //事件发送帧中数据数组占用的数据长度
#define TDOA_SEND_CARD_SAVE_START_INDEX       15         //发送存储待测卡能量信息的首地址  

 
#define TDOA_CHANNEL_CONFIG_MODE_LEN 9    //通道配置模式数量
#define TDOA_CHANNEL_CONFIG_MODE_ONE 0    //通道配置模式数量


typedef enum inst_states
{
    TDOA_INIT, //0

    TA_TXE_WAIT,                //1/
    TA_TXPOLL_WAIT_SEND,        //2/
    TA_TXFINAL_WAIT_SEND,       //3/
    TA_TXRESPONSE_WAIT_SEND,    //4/
    TA_TXREPORT_WAIT_SEND,      //5
    TA_TX_WAIT_CONF,            //6

    TA_RXE_WAIT,                //7
    TDOA_RX_WAIT_DATA,            //8
    
    TA_SLEEP,					//9
    TA_SLEEP_DONE,				//10
	TA_TXBLINK_WAIT_SEND,		//11
    TA_TXRANGINGINIT_WAIT_SEND,  //12
    TA_TX_RANGING_ACK_SEND ,     //13
    TA_HELP_CALL_SEND ,           //14
    TA_HELP_RESP_SEND ,           //15
    TA_EVACUATE_ASK_SEND,         //16
    TA_EXCIT_WAIT_SEND,           //17
    TA_EXCIT_ACK_SEND,            //18
    TA_TDOA_WAIT_SEND,             //19
    TDOA_RX_DATA_DONE             //20
} TDOA_INST_STATES_E;

typedef enum instanceModes
{
	TDOA_INST_TAG_TEST, 
	TDOA_INST_ANCHOR, 
	TDOA_INST_TAG_STANDARD,
	
	TDOA_INST_BUTT
} TDOA_INST_MODE_E;

/***************设备接收的事件的类型****************/
typedef enum InstEventType
{
	//DWT_SIG_RX_OKAY = 0,
	DWT_SIG_RX_BLINK,
	//DWT_SIG_RX_TIMEOUT,
	//DWT_SIG_TX_AA_DONE,
	DWT_SIG_RX_UNKNOWN,
	
	DWT_SIG_RX_BUTT
} TDOA_INST_EVENT_TYPE_E;

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

/***************设备发送的速率类型 用于区分待测卡和快发卡****************/
typedef enum InstModeSpeedType
{
	INST_MODE_TX_SPEED_SLOW = 1,  //标记为待测卡
	INST_MODE_TX_SPEED_QUICK,     //标记为快发卡

	INST_MODE_TX_SPEED_BUTT
} TDOA_INST_MODE_SPEED_TYPE_E;

//消息发送频谱结构
typedef struct 
{
                uint8 u8PGdelay;

                //TX POWER
                //31:24     BOOST_0.125ms_PWR
                //23:16     BOOST_0.25ms_PWR-TX_SHR_PWR
                //15:8      BOOST_0.5ms_PWR-TX_PHR_PWR
                //7:0       DEFAULT_PWR-TX_DATA_PWR
                uint32 u32TxPwr[2]; //
}TDOA_INST_SPECTRUM_TX_CONFIG_S;

/*******设备实例基本属性信息*******/
typedef struct TdoaInstanceBaseData
{
	TDOA_INST_MODE_E    		eTdoaInstMode;	   //设备类型 待测卡、快发卡、基站
    TDOA_INST_STATES_E   		eTdoaInstState;    //设备当前的运行状态
    TDOA_INST_EVENT_TYPE_E      eDwEventType;      //设备实例事件类型 例如接收帧是否正确可处理
	TDOA_INST_MODE_SPEED_TYPE_E eTxSpeedType;	   //用于标记是待测卡还是快发卡 待测卡:0 快发卡:1 

	uint16 u16SeqNum;                       //待测卡、快发卡需要记录序列号
    uint16 u16OwnAddress;                   //设备的地址
    uint16 u16DestAddress;                  //设备的地址
	uint16 u16PanId ;                       //设备的panid 三种类型的卡需要在相同的panid内通信
	uint64 u64RxTimeStamp;		            //需要记录接收待测卡和快发卡的信息时的时间点
	uint8  u8RxAutoreEnable;                //使能自动接收开关
	uint8  u8StationStatus;                 //station's status 告警状态或者恢复复位状态
    uint8  u8InstEvent[2];           		//this holds any TX/RX events - at the moment this is an array of 2 but should be changed to a queue
    uint8  u8InstEventCnt;
	int8   i8Rssi;                          //设备的功率值
	int8   i8LostPollPackCount;             //基站丢失Poll包的次数 达到5次就复位基站
	
}TDOA_INSTANCE_BASE_DATA_S;

/********对DW1000接收缓冲区读写的结构体*******/
//802.15.4a帧结构
//发送给接收器缓冲区前进行数据封装的结构
//由接收器缓冲区接收寄存器读取的接收数据结构
/**************************************************
 ************u8MessageData数据组装格式*************
 第一个字节    消息数据类型
 第二个字节    快发卡慢发卡标志
 第三第四字节  设备的panid 使用大端序填充写入数据
 第五第六字节  消息的序列号 使用大端序填充写入数据
 第十五字节    学习卡接收待测卡的数量
 第十六字节后  学习卡接收待测卡卡号以及对应接收到的能量值

**************************************************/
typedef struct
{
    uint8 u8FrameCtrl[FRAME_CONTROL_BYTES];               //  frame control bytes 00-01
    uint8 u8SeqNum;                               	      //  cur slot  序列号通过数据的第二、三位发送出去
    uint8 u8PanID[FRAME_PANID];                           //  PAN ID 03-04
    uint8 u8DestAddr[ADDR_BYTE_SIZE_S];             	  //  05-06 or using 64 bit addresses (05-12)
    uint8 u8SourceAddr[ADDR_BYTE_SIZE_S];           	  //  07-08 or using 64 bit addresses (13-20)
    uint8 u8MessageData[MAX_USER_PAYLOAD_STRING] ;        //  22-124 (application data and any user payload)
    uint8 u8Fcs[FRAME_CRC] ;                              //  125-126  we allow space for the CRC as it is logically part of the message. However ScenSor TX calculates and adds these bytes.
} TDOA_DW_TRX_MSG_S;

typedef struct
{
	uint8	u8PGdly;
	//TX POWER
	//31:24		BOOST_0.125ms_PWR
	//23:16		BOOST_0.25ms_PWR-TX_SHR_PWR
	//15:8		BOOST_0.5ms_PWR-TX_PHR_PWR
	//7:0		DEFAULT_PWR-TX_DATA_PWR
	uint32	u32Power; //天线发送功率设置
}TDOA_INST_TX_CONFIG_S;

/**********通道模式结构体*****************/
 typedef struct
 {
	 uint8 u8ChannelNumber ;	 // valid range is 1 to 11
	 uint8 u8PulseRepFreq ; 	 // NOMINAL_4M, NOMINAL_16M, or NOMINAL_64M
	 uint8 u8DataRate ; 		 // DATA_RATE_1 (110K), DATA_RATE_2 (850K), DATA_RATE_3 (6M81)
	 uint8 u8PreambleCode ; 	 // 00 = use NS code, 1 to 24 selects code
	 uint8 u8PreambleLen ;		 // values expected are 64, (128), (256), (512), 1024, (2048), and 4096
	 uint8 u8PacSize ;
	 uint8 u8NsSFD ;
 } TDOA_INST_CHANNEL_CONFIG_S;

 /*******配置DW1000的通道模式******/
typedef struct
{
    uint8 u8ChannelNum ;           //!< channel number {1, 2, 3, 4, 5, 7 }
    uint8 u8Prf ;            //!< Pulse Repetition Frequency {DWT_PRF_16M or DWT_PRF_64M}

	uint8 u8TxPreambLength ; //!< DWT_PLEN_64..DWT_PLEN_4096
    uint8 u8RxPAC ;          //!< Acquisition Chunk Size (Relates to RX preamble length)

	uint8 u8TxCode ;         //!< TX preamble code
    uint8 u8RxCode ;         //!< RX preamble code

	uint8 u8NsSFD ;          //!< Boolean should we use non-standard SFD for better performance

	uint8 u8DataRate ;       //!< Data Rate {DWT_BR_110K, DWT_BR_850K or DWT_BR_6M8}

	uint8 u8PhrMode ;        //!< PHR mode {0x0 - standard DWT_PHRMODE_STD, 0x3 - extended frames DWT_PHRMODE_EXT}

	uint16 u16SfdTO ;        //!< SFD timeout value (in symbols)

	uint8 u8SmartPowerEn;    //!< Smart Power enable / disable
} TDOA_DW_CHANNEL_CONFIG_S ;
 
 //设备的日志文件 包含RSSI的计算信息
 typedef struct
 {
	 uint32 u32IcId;				  //芯片Id
 
	 dwt_rxdiag_t stDwRxDiag;		  //设备接收到的日志信息
 
#if DECA_LOG_ENABLE==1
	 int		 iAccumLogging ;							   // log data to a file, used to indicate that we are currenty logging (file is open)
	 FILE		 *accumLogFile ;							   // file
#endif
 
 }TDOA_DW_DEVICE_LOG_DATA_S;

typedef struct
{
	TDOA_INSTANCE_BASE_DATA_S stInstBaseData;  //设备实例基本属性信息

	//DW1000的配置结构体 三种类型的设备需要在相同的通道内通信
	TDOA_DW_CHANNEL_CONFIG_S    stDwChannelCfg;	//DW1000 通道配置结构体 原型dwt_config_t 原有dwt_local_data_t dw1000local不修改

	//发送和接收缓冲区参数
	uint16 u16TxMsgLength;
	uint16 u16RxMsgLength;
    TDOA_DW_TRX_MSG_S stTxMsgToPack; //待测卡和学习卡进行组包发送结构
    TDOA_DW_TRX_MSG_S stRxMsgFromDw; //基站由DW1000接收器接收缓冲区寄存器获取数据帧结构

	//天线发射功率频谱设置
	uint16 u16AntennaTxDelay;                     //天线发送延时
	TDOA_INST_TX_CONFIG_S       stDwInstTxCfg;    //硬件发送数据配置 主要是配置天线参数
	
	//设备的日志信息 包含设备的接收RSSI值
	TDOA_DW_DEVICE_LOG_DATA_S   stDwDeviceLogDate;
	
   // uint8    macdata_msdu[MAX_MAC_MSG_DATA_LEN];        //

	//对发送的数据消息进行等待服务器的确认 设置是否需要通过延时等待来等待服务器的确认
    uint8 u8WaitAck;
} TDOA_INSTANCE_DATA_S ;

/*******待测卡、快发卡组包前的接收信息结构*******/
typedef struct TdoaInstRxMsgToCard 
{
	uint64 u64RxCardTimestamp;     //记录基站收到待测卡、快发卡信息时的时间戳
	uint16 u16Seqnum;            //记录待测卡、快发卡发送信息的序列号
	uint16 u16CardId;            //记录待测卡、快发卡的卡ID
	uint8  u16Speedtype;		  //记录卡的发送速率类型 用于标记待测卡、快发卡  TDOA_INST_MODE_SPEED_TYPE_E
	uint8  u16Status;
	int8   i8Rssi;
	uint8  u8DevType;
}TDOA_INST_RXMSG_TO_CARDMSG_S;               //标签以及基准标签发过来的信息

/*******学习卡接收到待测卡能量值的存储结构*******/
typedef struct TdoaInstRssiData 
{
	uint16 u16TestCardId;                    //待测卡卡号
	int8   i8Rssi;                       //接收到待测卡的能量值
}TDOA_INST_RSSI_DATA_S;     //待测卡能量值信息存储结构

typedef struct TdoaInstRelationTable
{
	uint16 u16TestCardId;                    //待测卡卡号
	uint16 u16StdCardId;                     //学习卡卡号
	int8   i8Rssi;                          //接收到待测卡的能量值
}TDOA_INST_RELATION_TABLE_S;           //学习卡与待测卡的关系表

typedef struct TdoaInstBetterSelsct
{
    TDOA_INST_RELATION_TABLE_S stBetterTable[TDOA_MSG_TESTCARD_MAX_NUM];
    uint8 u8CardCount;   //记录当前收到的卡数量
    uint8 u8Dealtime;   //数据到来的次数 数据到来超过学习卡的次数则进行初始化学习卡与待测卡间的信息结构 保证数据的实施性
}TDOA_INST_BETTER_SELECT_TABLE_S;               //标签以及基准标签发过来的信息

typedef struct TdoaInstSaveRssi 
{
    TDOA_INST_RSSI_DATA_S stTestRssiData[TDOA_MSG_TESTCARD_MAX_NUM];
    uint8 u8CardCount;   //记录当前收到的卡数量
    uint8 u8Dealtime;   //数据到来的次数 数据到来次数超过待测卡的数量则进行初始化学习卡存储待测卡的存储结构 保证数据的实施性
    uint8 u8LastDealtime; //上一次到达的记录次数 若次数没有更新 则基站认为快发卡已经离线 可以将改快发卡消息数据清除
    uint16 u16StdCardId; //学习卡卡号
}TDOA_INST_SAVE_RSSI_MSG_S;               //标签以及基准标签发过来的信息

typedef struct TdoaInstStandardDate 
{
    TDOA_INST_SAVE_RSSI_MSG_S  stStdCardMsg[TDOA_STANDCARD_MAX_NUM];
    uint8 u8StdCardCount;  //记录当前收到的卡数量 存在不符合条件的计数
    //uint8 u8StdCardCountReal;  //记录当前收到的卡数量 真实收到的数据
}TDOA_INST_STANDARD_CARD_MSG_S;               //基站存储多个学习卡的消息

/***********组包数据结构*******************/
//快发卡组包结构中信息
typedef struct TdoaStandardMsg 
{
	uint32 u32RxCardTimestampH;
	uint32 u32RxCardTimestampL;
	uint16 u16Seqnum;      
	uint16 u16CardId;     //待测卡ID 
}TDOA_STANDARD_CARD_MSG_S;

//待测卡组包结构中信息
typedef struct 
{
	uint16 u16TestCardID;            //待测标签ID
	uint16 u16StandardCardID;          //基准标签ID
	uint16 u16StationID;                //基站ID           sys_option.u32BsId;
	uint16 u16Cardseqnum;              //待测标签序列号
	uint32 u32SQANHTieH;               //慢发与最近一次快发的时间戳差
	uint32 u32SQANHTieL;               //慢发与最近一次快发的时间戳差
	uint32 u32QQANHTieH;               //慢发后一次快发减去上一次快发的时间戳差
	uint32 u32QQANHTieL; 
	uint8  u8DevType;	  // 
	uint8  u8Status;
	uint8  i8Rssi;
	uint8  u8Reserved;
}TDOA_UWB_TIMESTAMP_PACK_S;

//组包结构信息
typedef struct TdoaTimestampMsg          //以基准标签放 
{
	TDOA_STANDARD_CARD_MSG_S  stPreQuickCardMsg;     //前一个时间戳
	TDOA_STANDARD_CARD_MSG_S  stLastQuickCardMsg;    //相邻的后一个时间错
	TDOA_UWB_TIMESTAMP_PACK_S stTdoaMsg[TDOA_MSG_TESTCARD_MAX_NUM];	 //待测卡的相关信息 包括待测卡、快发卡、发送信息的基站卡、过程记录的有效时间戳差值
	uint16 u16TdoaMsgNum;                            //两快发之间收到的待测的数量
	uint16 u16StandCardLost;              //测试基准标签丢失使用，暂时不用
	uint16 u16StandCardStartSeq;         //基准标签开始的seq
	uint8  u16MsgSendFlag;               //0为未满，1为满可以组包发出 
}TDOA_UWB_TIMESTAMP_MSG_ARRAY_S;

//带有最终计算结果的时间戳包结构信息
typedef struct TdoaMsgPack          //以基准标签放 
{
	uint16 u16PackSendCount;                                         //发送带有计算后时间戳数据的包数量
	TDOA_UWB_TIMESTAMP_PACK_S stTdoaMsg[TDOA_PACK_SEND_MAX_NUM];	 //待测卡的相关信息 包括待测卡、快发卡、发送信息的基站卡、过程记录的有效时间戳差值
}TDOA_UWB_MSG_PACK_SEND_S;

/*********************************函数声明******************************************/
int Dw1000Init(void);
TDOA_INSTANCE_DATA_S* TdoaGetLocalInstStructurePtr(void);
TDOA_INST_RXMSG_TO_CARDMSG_S* TdoaGetLocalCardStructurePtr(void);
TDOA_UWB_TIMESTAMP_MSG_ARRAY_S* TdoaGetLocalMsgArrayStructurePtr(void);
TDOA_UWB_MSG_PACK_SEND_S* TdoaGetLocalSendPackStructurePtr(void);

void TdoaInstChannelConfig(uint8 usModeNum);
void TdoaInstanceBaseMsginit(int iTdoaInstMode);
void AppInstanceInit(int iTdoaInstMode);
void TdoaTagInit(void);
void TdoaAnchorInit(void);
void TdoaBuildPackBuffInit(void);
void TdoaInstConfigFrameHeader(TDOA_INSTANCE_DATA_S *pstInstMsg, int iIfAckRequst);
void TdoaSetMacFrameData(TDOA_INSTANCE_DATA_S *pstInstMsg, int iFrameDataLen, int iFrameType, int iIfAck);
int TdoaInstSendTagPoolPacket(TDOA_INSTANCE_DATA_S *pstInstMsg, int iTxDelayed);
void TdoaSendTagPoll(void);
void TdoaInstRunState(void);
void TdoaRxCardMsgProc(void);
void TdoaClearMsgArray(TDOA_UWB_TIMESTAMP_MSG_ARRAY_S* pstTdoaMsgArray);
void TdoaBuildPackBuffInit(void);
void TdoaCardMsdBuildUnity(TDOA_INST_RXMSG_TO_CARDMSG_S *pstCardRxMsg, 
						   TDOA_UWB_TIMESTAMP_MSG_ARRAY_S* pstTdoaMsgArray, 
						   int iMsgArrayNum);
int TdoaCheckRxMsgStandardID(TDOA_INST_RXMSG_TO_CARDMSG_S *pstCardRxMsg);
void TdoaRxSlowCardMsgProc(TDOA_INST_RXMSG_TO_CARDMSG_S *pstCardRxMsg);
void TdoaRxQuickCardMsgProc(TDOA_INST_RXMSG_TO_CARDMSG_S *pstCardRxMsg);
void TdoaMsgPackInsetUartBuff(TDOA_UWB_TIMESTAMP_MSG_ARRAY_S* pstTdoaMsgArray);
void TdoaDataBuffClear(void);

void init_anchor(TDOA_INSTANCE_DATA_S *inst);

void TdoaInstFillTestTagRssiToSendMsgProc(TDOA_INSTANCE_DATA_S* pstInstMsg, uint8 u8StartIndex, uint8* pu8SendLen);
void TdoaInstRxTestMsgProc(void);
void TdoaInstSaveTestTagRssiProc(uint16 u16OwnAddr, uint16 u16TestTagAddr, int i8Rssi);
void TdoaStdCardMsgBuffInit(void);
void TdoaStdCardSaveRssiBuffInit(void);
void TdoaCardMsgBuffInit(void);
int TdoaInstBetterSelectStdCardCheck(uint16 u16TestCardId, uint16 u16StdCardId);  


typedef struct
{
    double idist ;
    double mean4 ; 
    double mean ; 
    int numAveraged ;
	uint32 lastRangeReportTime ;
} currentRangeingInfo_t ;

typedef struct
{
    SLOT_STATUS b1Used;          // 0: idle, 1: used
    uint8 u8DeviceType ;         /// 1:1s card ;   5:5s card   
    uint8 u8cur_slot;              //current slot number
    uint8 dest_addr[ADDR_BYTE_SIZE];            //current slot's tag addr
    uint8 status;                  //card's current tatus
    uint8 u8LostNum;               // if u8LostNum >= 2, release this slot
    uint16 u16SeqNum;              // sequence number
    uint16 sleeptick;              //this card's need sleep time
    double m_distance;             //main sation distance
	double s_distance;             //sub station distance
} slot_msg_t;

typedef struct
{
	uint8 u8cardaddr[ADDR_BYTE_SIZE];   //the slot's card addr
	uint16 u16seqnum;                   //current seq
	double m_distance;                  //current main station distance
	double s_distance;                  //current sub station distance
} uart_distmsg_t;

typedef struct
{
	uint8 count;
	uint16 addr[50];
}alarm_addrlist_t;

typedef struct
{
	uint16 alarmaddr;
	uint8  alarmstatus;
	uint8  excitid;
}Sub_alarm_msg_t;


//-------------------------------------------------------------------------------------------------------------
//
//	Functions used in logging/dispalying range and status data
//
//-------------------------------------------------------------------------------------------------------------

// function to calculate and report the Time of Flight to the GUI/display

void instancegetcurrentrangeinginfo(currentRangeingInfo_t *info) ;

// enable reading of the accumulator - used for displaying channel impulse response
void instancesetaccumulatorreadenable(int enable) ;      // enable reading of accumulator every 'period' frames
void instance_set_helpstatus(uint16 addr);
uint8 instance_get_retreatstatus(void);


void dec_sleep_wakeup(void);

//-------------------------------------------------------------------------------------------------------------
//
//	Functions used in driving/controlling the ranging application
//
//-------------------------------------------------------------------------------------------------------------

// opent the SPI Cheetah interface - called from inittestapplication()
int instancespiopen(void) ;  // Open SPI and return handle
// close the SPI Cheetah interface  
void instance_close(void);
// Call init, then call config, then call run. call close when finished
// initialise the instance (application) structures and DW1000 device
int instance_init(void);

void instance_clear_substa(void);

uint8 get_curslot_cardmsg(uint16 *cardid); 


void instance_set_AnchorPanid(uint8 type);

void txhelp_call_send(void);

// configure the instance and DW1000 device
void instance_config(instanceConfig_t *config) ;  


#if (DR_DISCOVERY == 0)
// configure the payload and MAC address
void instancesetaddresses(instanceAddressConfig_t *plconfig) ;
#endif
// configure the antenna delays
void instancesetantennadelays(double fdelay) ;                      // delay in nanoseconds
// configure whether the Anchor sends the ToF reports to Tag
void instancesetreporting(int anchorSendsTofReports) ; // configure anchor to send TOF reports to Tag

// called (periodically or from and interrupt) to process any outstanding TX/RX events and to drive the ranging application
int instance_run(uint8 type) ;       // returns indication of status report change
// calls the DW1000 interrupt handler
#define instance_process_irq(x) 	dwt_isr()  //call device interrupt handler
// configure TX/RX callback functions that are called from DW1000 ISR

// sets the Tag sleep delay time (the time Tag "sleeps" between each ranging attempt)
// sets the Tag/Anchor reply delay time (the time Tag/Anchor wait before sending the reply message (Final/Response))

// get the DW1000 device ID (e.g. 0xDECA0130 for MP)

uint16 instance_get_seqnum(void);

void instance_set_seqnum(void);

uint8 instance_get_helpstatus(void);

void instance_reset_helpstatus(void);


int instancenewrange(void);
	
void instance_set_status(int eventstatus);
void instance_set_event(int type);
void card_rev_poll_tontinue_count(void);

void instance_set_card_status(uint8 cardstatus);    

uint8 instance_get_card_status(void) ;


void instance_set_vbat(uint16 vdd);

uint8 instance_get_tataddr(void);

void instance_get_revmsg(Rx_msg *remsg);

uint64 get_AnchroTimeStamp(void);

uint64 get_TagTimeStamp(void);

uint8 get_TxspeedType(void);

uint16 get_rcvCardID(void);

void send_tdoa_poll(void);

#ifdef __cplusplus
}
#endif

#endif

