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
//���ļ�����TDOA����ʵ������ ������ʷԭ��ʹ���˿�������⿨��ѧϰ��ʱ��Ҫ��config.h�еĺ�DEC_UWB_ANCHOR����
//���л�վ����ʱ����Ҫ��config.h�еĺ�DEC_UWB_ANCHOR����
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
/*************** ʵ���豸���� TDOA_INST_MODE_E *****************/
#define   TDOA_INST_MODE_ANCHOR         //��վ
//#define   TDOA_INST_MODE_TAG_TEST       //���⿨
//#define   TDOA_INST_MODE_TAG_STANDARD   //ѧϰ��

/*********************�豸��������*****************************/
#define DEV_TAG_ANCHOR_ADDRESS          30003    //��վID
#define DEV_TAG_TEST_ADDRESS            10009    //���⿨ID   10009
#define DEV_TAG_STANDARD_ADDRESS        10007    //�췢��ID
#define SLOW_SPEED_TAG_SEND_TIME  		1000     //���⿨�������ڼ����⿨��Ϣ�������� 400
#define QUICK_SPEED_TAG_SEND_TIME   	150      //���⿨�������ڼ����⿨��Ϣ�������� 45
#define DEVICE_UART_SEND_TIME   	    1000     //���ڻ�������Ϣ�������� 1000 ���ܴ��ڿ�����ѯ����


#ifdef TDOA_INST_MODE_ANCHOR 
#define TDOA_INST_MODE_TYPE             TDOA_INST_ANCHOR        //��վ
#define DEV_ADDRESS_CARDID        		DEV_TAG_ANCHOR_ADDRESS //�豸��ID���� �豸��16λ�̵�ַ
#endif

#ifdef TDOA_INST_MODE_TAG_TEST 
#define TDOA_INST_MODE_TYPE             TDOA_INST_TAG_TEST        //���⿨ ��Ҫ����config.h�еĺ�DEC_UWB_ANCHOR
#define DEV_ADDRESS_CARDID        		DEV_TAG_TEST_ADDRESS      //�豸��ID���� �豸��16λ�̵�ַ
#endif

#ifdef TDOA_INST_MODE_TAG_STANDARD 
#define TDOA_INST_MODE_TYPE             TDOA_INST_TAG_STANDARD  //ѧϰ��
#define DEV_ADDRESS_CARDID        		DEV_TAG_STANDARD_ADDRESS      //�豸��ID���� �豸��16λ�̵�ַ
#endif

/*********************�豸��ص�����*****************************/
#define   PRINTF_vReportCardDistance_SendData     //���͵�stm����Ϣ

//#define DWT_TIME_UNITS          (1.0/499.2e6/128.0) //= 15.65e-12 s һ��dw1000 tick��ʱ���С
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
#define RTLS_TDOA_BLINK_SEND                (0x28)          //tdoa blink��Ϣ������
#define RTLS_RETREAT_ACK_SEND               (0x2B)          //press the buttun to stop retreat


//�豸ʵ�����ͽ���֡��Ӧ�ĳ��ȶ���
#define TAG_POLL_MSG_LEN                    1				// FunctionCode(1),
#define ANCH_RESPONSE_MSG_LEN               9               // FunctionCode(1), RespOption (1), OptionParam(2), Measured_TOF_Time(5)
#define TAG_FINAL_MSG_LEN                   16              // FunctionCode(1), Poll_TxTime(5), Resp_RxTime(5), Final_TxTime(5)
#define RANGINGINIT_MSG_LEN					7				// FunctionCode(1), Tag Address (2), Response Time (2) * 2

#define STANDARD_FRAME_SIZE         127  //��׼�ܹ�֡�ṹ���� ��չ����Ϊ1024�ֽ�

#define ADDR_BYTE_SIZE_L            (8)  //����ַʹ�ó���
#define ADDR_BYTE_SIZE_S            (2)  //�̵�ַʹ�ó���

#define FRAME_CONTROL_BYTES         2    //�շ�֡�Ŀ���λ����
#define FRAME_SEQ_NUM_BYTES         1    //�շ�֡�����к�ռ�ó���
#define FRAME_PANID                 2    //�շ�֡��Panid����
#define FRAME_CRC					2	 //�շ�֡��CRCУ�鳤��
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

//�豸ʹ�ܺ�δʹ�ܱ�־
#define TDOA_EABLE      1
#define TDOA_DISEABLE   0

#define TDOA_UART_SEND_MAX_TAG_NUM  	  8               //���ڷ���һ������෢�͵Ŀ�����

#define TDOA_MSG_TESTCARD_MAX_NUM  	  20                  //�����Ϣ�д��⿨�������ֵ
#define TDOA_STANDCARD_MAX_NUM       (5)                  //ͬһ����׼�ܹ����յ������Ļ�׼��վ������
#define TDOA_PACK_SEND_MAX_NUM       (50)                 //������� TimeStamp_mgr �ĸ��� ����������ڷ���ǰ��Ҫ���а�����ͳ��
#define TDOA_LOST_POLL_PACK_COUNT     5                   //��վ�Ķ�����

#define TDOA_DW_TICK_MAX             1099511627775       //0xffffffffff �����tick��

#define TDOA_INST_PANID_TAG  		  0x2222              //���⿨��Panid
#define TDOA_INST_PANID_TAG_STANDARD  0x1111              //ѧϰ����Panid
#define TDOA_INST_PANID_ANCHOR        0x1111              //��վ��panid

//��Ƶ��ʱĬ��ֵ
#define DWT_PRF_64M_RFDLY   (515.6f)
#define DWT_PRF_16M_RFDLY   (515.0f)
extern const uint16 gau16RfDelays[2];

#define TDOA_INST_MODE_SPEED_SLOW    0x0                 //��ѯ����֡�б�Ǵ��⿨
#define TDOA_INST_MODE_SPEED_QUCIK   0x1				 //��ѯ����֡�б�ǿ췢��


#define TDOA_INST_ACK_REQUESTED               (1)        //����ȷ��֡ Request an ACK frame
#define TDOA_INST_SEND_POLL_DEST_ADDR        (0xFFFF)    //(0xFFFF)     //���⿨�Ϳ췢����Ŀ�ĵ�ַ����Ϊ�㲥��ַ
#define TDOA_INST_FRAME_TYPE_BIT              0          //����֡���� ����Ƿ�����ѯ��Ϣ����
#define TDOA_INST_FRAME_SPEED_TYPE_BIT        1          //����֡�м�¼��ǩ������������λ
#define TDOA_INST_FRAME_PANID_BIT_H           2          //����֡�м�¼�豸panid�ĸ�λ��������
#define TDOA_INST_FRAME_PANID_BIT_L           3          //����֡�м�¼�豸panid�ĵ�λ��������

#define TDOA_INST_FRAME_SEQNUM_BIT_H          4          //����֡�м�¼��Ϣ���кŵĸ�λ��������
#define TDOA_INST_FRAME_SEQNUM_BIT_L          5          //����֡�м�¼��Ϣ���кŵĵ�λ��������
#define TDOA_INST_FRAME_ARRAY_DATA_LEN        6          //�¼�����֡����������ռ�õ����ݳ���
#define TDOA_SEND_CARD_SAVE_START_INDEX       15         //���ʹ洢���⿨������Ϣ���׵�ַ  

 
#define TDOA_CHANNEL_CONFIG_MODE_LEN 9    //ͨ������ģʽ����
#define TDOA_CHANNEL_CONFIG_MODE_ONE 0    //ͨ������ģʽ����


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

/***************�豸���յ��¼�������****************/
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

/***************�豸���͵��������� �������ִ��⿨�Ϳ췢��****************/
typedef enum InstModeSpeedType
{
	INST_MODE_TX_SPEED_SLOW = 1,  //���Ϊ���⿨
	INST_MODE_TX_SPEED_QUICK,     //���Ϊ�췢��

	INST_MODE_TX_SPEED_BUTT
} TDOA_INST_MODE_SPEED_TYPE_E;

//��Ϣ����Ƶ�׽ṹ
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

/*******�豸ʵ������������Ϣ*******/
typedef struct TdoaInstanceBaseData
{
	TDOA_INST_MODE_E    		eTdoaInstMode;	   //�豸���� ���⿨���췢������վ
    TDOA_INST_STATES_E   		eTdoaInstState;    //�豸��ǰ������״̬
    TDOA_INST_EVENT_TYPE_E      eDwEventType;      //�豸ʵ���¼����� �������֡�Ƿ���ȷ�ɴ���
	TDOA_INST_MODE_SPEED_TYPE_E eTxSpeedType;	   //���ڱ���Ǵ��⿨���ǿ췢�� ���⿨:0 �췢��:1 

	uint16 u16SeqNum;                       //���⿨���췢����Ҫ��¼���к�
    uint16 u16OwnAddress;                   //�豸�ĵ�ַ
    uint16 u16DestAddress;                  //�豸�ĵ�ַ
	uint16 u16PanId ;                       //�豸��panid �������͵Ŀ���Ҫ����ͬ��panid��ͨ��
	uint64 u64RxTimeStamp;		            //��Ҫ��¼���մ��⿨�Ϳ췢������Ϣʱ��ʱ���
	uint8  u8RxAutoreEnable;                //ʹ���Զ����տ���
	uint8  u8StationStatus;                 //station's status �澯״̬���߻ָ���λ״̬
    uint8  u8InstEvent[2];           		//this holds any TX/RX events - at the moment this is an array of 2 but should be changed to a queue
    uint8  u8InstEventCnt;
	int8   i8Rssi;                          //�豸�Ĺ���ֵ
	int8   i8LostPollPackCount;             //��վ��ʧPoll���Ĵ��� �ﵽ5�ξ͸�λ��վ
	
}TDOA_INSTANCE_BASE_DATA_S;

/********��DW1000���ջ�������д�Ľṹ��*******/
//802.15.4a֡�ṹ
//���͸�������������ǰ�������ݷ�װ�Ľṹ
//�ɽ��������������ռĴ�����ȡ�Ľ������ݽṹ
/**************************************************
 ************u8MessageData������װ��ʽ*************
 ��һ���ֽ�    ��Ϣ��������
 �ڶ����ֽ�    �췢����������־
 ���������ֽ�  �豸��panid ʹ�ô�������д������
 ��������ֽ�  ��Ϣ�����к� ʹ�ô�������д������
 ��ʮ���ֽ�    ѧϰ�����մ��⿨������
 ��ʮ���ֽں�  ѧϰ�����մ��⿨�����Լ���Ӧ���յ�������ֵ

**************************************************/
typedef struct
{
    uint8 u8FrameCtrl[FRAME_CONTROL_BYTES];               //  frame control bytes 00-01
    uint8 u8SeqNum;                               	      //  cur slot  ���к�ͨ�����ݵĵڶ�����λ���ͳ�ȥ
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
	uint32	u32Power; //���߷��͹�������
}TDOA_INST_TX_CONFIG_S;

/**********ͨ��ģʽ�ṹ��*****************/
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

 /*******����DW1000��ͨ��ģʽ******/
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
 
 //�豸����־�ļ� ����RSSI�ļ�����Ϣ
 typedef struct
 {
	 uint32 u32IcId;				  //оƬId
 
	 dwt_rxdiag_t stDwRxDiag;		  //�豸���յ�����־��Ϣ
 
#if DECA_LOG_ENABLE==1
	 int		 iAccumLogging ;							   // log data to a file, used to indicate that we are currenty logging (file is open)
	 FILE		 *accumLogFile ;							   // file
#endif
 
 }TDOA_DW_DEVICE_LOG_DATA_S;

typedef struct
{
	TDOA_INSTANCE_BASE_DATA_S stInstBaseData;  //�豸ʵ������������Ϣ

	//DW1000�����ýṹ�� �������͵��豸��Ҫ����ͬ��ͨ����ͨ��
	TDOA_DW_CHANNEL_CONFIG_S    stDwChannelCfg;	//DW1000 ͨ�����ýṹ�� ԭ��dwt_config_t ԭ��dwt_local_data_t dw1000local���޸�

	//���ͺͽ��ջ���������
	uint16 u16TxMsgLength;
	uint16 u16RxMsgLength;
    TDOA_DW_TRX_MSG_S stTxMsgToPack; //���⿨��ѧϰ������������ͽṹ
    TDOA_DW_TRX_MSG_S stRxMsgFromDw; //��վ��DW1000���������ջ������Ĵ�����ȡ����֡�ṹ

	//���߷��书��Ƶ������
	uint16 u16AntennaTxDelay;                     //���߷�����ʱ
	TDOA_INST_TX_CONFIG_S       stDwInstTxCfg;    //Ӳ�������������� ��Ҫ���������߲���
	
	//�豸����־��Ϣ �����豸�Ľ���RSSIֵ
	TDOA_DW_DEVICE_LOG_DATA_S   stDwDeviceLogDate;
	
   // uint8    macdata_msdu[MAX_MAC_MSG_DATA_LEN];        //

	//�Է��͵�������Ϣ���еȴ���������ȷ�� �����Ƿ���Ҫͨ����ʱ�ȴ����ȴ���������ȷ��
    uint8 u8WaitAck;
} TDOA_INSTANCE_DATA_S ;

/*******���⿨���췢�����ǰ�Ľ�����Ϣ�ṹ*******/
typedef struct TdoaInstRxMsgToCard 
{
	uint64 u64RxCardTimestamp;     //��¼��վ�յ����⿨���췢����Ϣʱ��ʱ���
	uint16 u16Seqnum;            //��¼���⿨���췢��������Ϣ�����к�
	uint16 u16CardId;            //��¼���⿨���췢���Ŀ�ID
	uint8  u16Speedtype;		  //��¼���ķ����������� ���ڱ�Ǵ��⿨���췢��  TDOA_INST_MODE_SPEED_TYPE_E
	uint8  u16Status;
	int8   i8Rssi;
	uint8  u8DevType;
}TDOA_INST_RXMSG_TO_CARDMSG_S;               //��ǩ�Լ���׼��ǩ����������Ϣ

/*******ѧϰ�����յ����⿨����ֵ�Ĵ洢�ṹ*******/
typedef struct TdoaInstRssiData 
{
	uint16 u16TestCardId;                    //���⿨����
	int8   i8Rssi;                       //���յ����⿨������ֵ
}TDOA_INST_RSSI_DATA_S;     //���⿨����ֵ��Ϣ�洢�ṹ

typedef struct TdoaInstRelationTable
{
	uint16 u16TestCardId;                    //���⿨����
	uint16 u16StdCardId;                     //ѧϰ������
	int8   i8Rssi;                          //���յ����⿨������ֵ
}TDOA_INST_RELATION_TABLE_S;           //ѧϰ������⿨�Ĺ�ϵ��

typedef struct TdoaInstBetterSelsct
{
    TDOA_INST_RELATION_TABLE_S stBetterTable[TDOA_MSG_TESTCARD_MAX_NUM];
    uint8 u8CardCount;   //��¼��ǰ�յ��Ŀ�����
    uint8 u8Dealtime;   //���ݵ����Ĵ��� ���ݵ�������ѧϰ���Ĵ�������г�ʼ��ѧϰ������⿨�����Ϣ�ṹ ��֤���ݵ�ʵʩ��
}TDOA_INST_BETTER_SELECT_TABLE_S;               //��ǩ�Լ���׼��ǩ����������Ϣ

typedef struct TdoaInstSaveRssi 
{
    TDOA_INST_RSSI_DATA_S stTestRssiData[TDOA_MSG_TESTCARD_MAX_NUM];
    uint8 u8CardCount;   //��¼��ǰ�յ��Ŀ�����
    uint8 u8Dealtime;   //���ݵ����Ĵ��� ���ݵ��������������⿨����������г�ʼ��ѧϰ���洢���⿨�Ĵ洢�ṹ ��֤���ݵ�ʵʩ��
    uint8 u8LastDealtime; //��һ�ε���ļ�¼���� ������û�и��� ���վ��Ϊ�췢���Ѿ����� ���Խ��Ŀ췢����Ϣ�������
    uint16 u16StdCardId; //ѧϰ������
}TDOA_INST_SAVE_RSSI_MSG_S;               //��ǩ�Լ���׼��ǩ����������Ϣ

typedef struct TdoaInstStandardDate 
{
    TDOA_INST_SAVE_RSSI_MSG_S  stStdCardMsg[TDOA_STANDCARD_MAX_NUM];
    uint8 u8StdCardCount;  //��¼��ǰ�յ��Ŀ����� ���ڲ����������ļ���
    //uint8 u8StdCardCountReal;  //��¼��ǰ�յ��Ŀ����� ��ʵ�յ�������
}TDOA_INST_STANDARD_CARD_MSG_S;               //��վ�洢���ѧϰ������Ϣ

/***********������ݽṹ*******************/
//�췢������ṹ����Ϣ
typedef struct TdoaStandardMsg 
{
	uint32 u32RxCardTimestampH;
	uint32 u32RxCardTimestampL;
	uint16 u16Seqnum;      
	uint16 u16CardId;     //���⿨ID 
}TDOA_STANDARD_CARD_MSG_S;

//���⿨����ṹ����Ϣ
typedef struct 
{
	uint16 u16TestCardID;            //�����ǩID
	uint16 u16StandardCardID;          //��׼��ǩID
	uint16 u16StationID;                //��վID           sys_option.u32BsId;
	uint16 u16Cardseqnum;              //�����ǩ���к�
	uint32 u32SQANHTieH;               //���������һ�ο췢��ʱ�����
	uint32 u32SQANHTieL;               //���������һ�ο췢��ʱ�����
	uint32 u32QQANHTieH;               //������һ�ο췢��ȥ��һ�ο췢��ʱ�����
	uint32 u32QQANHTieL; 
	uint8  u8DevType;	  // 
	uint8  u8Status;
	uint8  i8Rssi;
	uint8  u8Reserved;
}TDOA_UWB_TIMESTAMP_PACK_S;

//����ṹ��Ϣ
typedef struct TdoaTimestampMsg          //�Ի�׼��ǩ�� 
{
	TDOA_STANDARD_CARD_MSG_S  stPreQuickCardMsg;     //ǰһ��ʱ���
	TDOA_STANDARD_CARD_MSG_S  stLastQuickCardMsg;    //���ڵĺ�һ��ʱ���
	TDOA_UWB_TIMESTAMP_PACK_S stTdoaMsg[TDOA_MSG_TESTCARD_MAX_NUM];	 //���⿨�������Ϣ �������⿨���췢����������Ϣ�Ļ�վ�������̼�¼����Чʱ�����ֵ
	uint16 u16TdoaMsgNum;                            //���췢֮���յ��Ĵ��������
	uint16 u16StandCardLost;              //���Ի�׼��ǩ��ʧʹ�ã���ʱ����
	uint16 u16StandCardStartSeq;         //��׼��ǩ��ʼ��seq
	uint8  u16MsgSendFlag;               //0Ϊδ����1Ϊ������������� 
}TDOA_UWB_TIMESTAMP_MSG_ARRAY_S;

//�������ռ�������ʱ������ṹ��Ϣ
typedef struct TdoaMsgPack          //�Ի�׼��ǩ�� 
{
	uint16 u16PackSendCount;                                         //���ʹ��м����ʱ������ݵİ�����
	TDOA_UWB_TIMESTAMP_PACK_S stTdoaMsg[TDOA_PACK_SEND_MAX_NUM];	 //���⿨�������Ϣ �������⿨���췢����������Ϣ�Ļ�վ�������̼�¼����Чʱ�����ֵ
}TDOA_UWB_MSG_PACK_SEND_S;

/*********************************��������******************************************/
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

