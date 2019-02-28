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
#define TAG_POLL_MSG_LEN                   9//;3				// FunctionCode(1), Temp (1), Volt (1), tick(4), seqnum(2)
#define ANCH_RESPONSE_MSG_LEN               4               // FunctionCode(1), RespOption (1), OptionParam(2)
#define TAG_FINAL_MSG_LEN                   16              // FunctionCode(1), Poll_TxTime(5), Resp_RxTime(5), Final_TxTime(5)
#define TOF_REPORT_MSG_LEN                  15//6               // FunctionCode(1), Measured_TOF_Time(5)   tick(4)  polltick(4) anchorRangeWith(1)
#define RANGINGINIT_MSG_LEN					 10//5				// FunctionCode(1), Tag Address (2), Response Time (2)  slot(1) tick(4)

#define STANDARD_FRAME_SIZE         88
#define MAX_MAC_MSG_DATA_LEN                (STANDARD_FRAME_SIZE)//(TAG_FINAL_MSG_LEN) //max message len of the above
#define MAX_CAR_CARD_CNT            20



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

#define FRAME_SOURCE_ADDRESS_S        (ADDR_BYTE_SIZE_S)
#define FRAME_DEST_ADDRESS_S          (ADDR_BYTE_SIZE_S)
#define FRAME_SOURCE_ADDRESS_L        (ADDR_BYTE_SIZE_L)
#define FRAME_DEST_ADDRESS_L          (ADDR_BYTE_SIZE_L)
#define FRAME_CRTL_AND_ADDRESS_L    (FRAME_DEST_ADDRESS_L + FRAME_SOURCE_ADDRESS_L + FRAME_CTRLP) //21 bytes for 64-bit addresses)
#define FRAME_CRTL_AND_ADDRESS_S    (FRAME_DEST_ADDRESS_S + FRAME_SOURCE_ADDRESS_S + FRAME_CTRLP) //9 bytes for 16-bit addresses)
#define FRAME_CRTL_AND_ADDRESS_LS	(FRAME_DEST_ADDRESS_L + FRAME_SOURCE_ADDRESS_S + FRAME_CTRLP) //15 bytes for 1 16-bit address and 1 64-bit address)
#define MAX_USER_PAYLOAD_STRING_LL     (STANDARD_FRAME_SIZE-FRAME_CRTL_AND_ADDRESS_L-TAG_FINAL_MSG_LEN-FRAME_CRC) //127 - 21 - 16 - 2 = 88
#define MAX_USER_PAYLOAD_STRING_SS     (STANDARD_FRAME_SIZE-FRAME_CRTL_AND_ADDRESS_S-TAG_FINAL_MSG_LEN-FRAME_CRC) //127 - 9 - 16 - 2 = 100
#define MAX_USER_PAYLOAD_STRING_LS     (STANDARD_FRAME_SIZE-FRAME_CRTL_AND_ADDRESS_LS-TAG_FINAL_MSG_LEN-FRAME_CRC) //127 - 15 - 16 - 2 = 94


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

#define MAX_CARD_CYCLE_SEC      (5)      //the max time period; 5s card =5  ;15s card =15
#define TOF_SLOT_LOC_PERIOD      40    // slot分配的最小周期，1s = 40 slot :40
#define EVERY_SLOT_TIME          30  //MS
#define CARD_1S_SEC_TIME         1200
#define CARD_5S_SEC_TIME         6000
#define WAKE_UP_REDAY_TIME       15   //MS, when wakeup card into the event need some time to ready
#define CARD_WAKE_UP_MAX         50
#define LOST_TOF_MAX_NUM          2    //the max consecutive  lost number can tolerate
#define TOF_TO_SUB_MIN_DIST     (0.0)   //when the main station ditance less then this value ,should do tof to sub station
#define ANCHOR_TOF_PRO            1     //the card in tof process
#define ANCHOR_BLINK_PRO          0     //the card in blink process
#define ANCHOR_TOF_CHANNEL        1     //the card in tof process
#define ANCHOR_BLINK_CHANNEL      3     //the card in blink process
#define ALL_TOF_TIME              2     //Do 2 consecutive tof with sub and main station

#define QUICK_SPEED_TAG_SEND_TIME       45


#define SUM_SLOT_COUNT     (MAX_CARD_CYCLE_SEC*TOF_SLOT_LOC_PERIOD)

#define MAX_SET_CARD_CNT    30
#define MAX_ALARM_NUM       50
typedef enum instanceModes{LISTENER,SUB_STA, TAG, ANCHOR, TAG_TDOA, TDOA_INST_TAG_STANDARD, NUM_MODES} INST_MODE;

typedef enum cardtypeModes{L_IDLE=0,CARD_1S=1,CARD_5S=5,OTHER=6} SLOTLIST_MODE;

typedef enum slotstatus{IDLE=0,USED_TOF=1,USED_UART=2,USED_BLINK=3 } SLOT_STATUS;


//function codes
#define RTLS_DEMO_MSG_RNG_INIT				(0x20)			// Ranging initiation message
#define RTLS_DEMO_MSG_TAG_POLL              (0x21)          // Tag poll message
#define RTLS_DEMO_MSG_ANCH_RESP             (0x10)          // Anchor response to poll
#define RTLS_DEMO_MSG_TAG_FINAL             (0x29)          // Tag final massage back to Anchor (0x29 because of 5 byte timestamps needed for PC app)
#define RTLS_DEMO_MSG_ANCH_TOFR             (0x2A)          // Anchor TOF Report message
#define RTLS_TOF_MSG_TAG_ACK                (0x22)          //ACK 基站响应卡的测距初始化消息
#define RTLS_MSG_HELP_CALL                  (0x23)          //card ask help 
#define RTLS_MSG_HELP_RESP                  (0x24)          //station rev help and resp
#define RTLS_EVACUATE_ASK_SEND              (0x25)          //evacute alarm 
#define RTLS_EXCIT_ASK_SEND                 (0x26)          //exicit 
#define RTLS_EXCIT_ACK_SEND                 (0x27)          //exicit 
#define RTLS_TDOA_BLINK_SEND                (0x28)          //TDOA  same as the resp
#define RTLS_RETREAT_ACK_SEND               (0x2B)          //press the buttun to stop retreat
#define RTLS_CAR_REVTOFCARD_MSG             (0x2C)          //the car's card send the human card msg
#define RTLS_TDOA_MSG_CARD_POLL             (0x2D)          // Tag poll message



#define BLINK_BYTE   0xC5
#define DK1000_WAKEUP_TIME   (35*CLOCKS_PER_MILLI)

//application data message byte offsets
#define FCODE                               0               // Function code is 1st byte of messageData
#define PTXT                                1
#define RRXT                                6
#define FTXT                                11
#define TOFR                                1
#define RES_R1                              1               // Response option octet 0x02 (1),
#define RES_R2                              2               // Response option paramter 0x00 (1) - used to notify Tag that the report is coming
#define RES_R3                              3               // Response option paramter 0x00 (1),
#define RES_T1                              3               // Ranging request response delay low byte
#define RES_T2                              4               // Ranging request response delay high byte
#define RES_SLOT                            5
#define RES_TICK1                           6
#define RES_TICK2                           7
#define RES_TICK3                           8
#define RES_TICK4                           9

#define POLL_TEMP                           1               // Poll message TEMP octet
#define POLL_VOLT                           2               // Poll message Voltage octet

#define MAX_NUMBER_OF_POLL_RETRYS 			2//20

#define ACK_REQUESTED                       (1)             // Request an ACK frame
#define BLINK_ALL_RESP                     (0)              //all idle card nedd response
#define BLINK_5S_RESP                       (2)             //only 5s card need response
#define BLINK_NO_RESP                       (3)             //rev blink but not need send ranging init

#define CARD_STATUS                          1              //card current status
#define SLOT_MSG                             2              //1s card or 5s card

#define allot_slot_1                         3              //blink the first allot addr's place
enum inst_states
{
    TA_INIT, //0

    TA_TXE_WAIT,                //1/
    TA_TXPOLL_WAIT_SEND,        //2/
    TA_TXFINAL_WAIT_SEND,       //3/
    TA_TXRESPONSE_WAIT_SEND,    //4/
    TA_TXREPORT_WAIT_SEND,      //5
    TA_TX_WAIT_CONF,            //6

    TA_RXE_WAIT,                //7
    TA_RX_WAIT_DATA,            //8

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
    TA_RETREAT_ACK_SEND,           //20
    TA_TDOA_END_TOF_START          //21
} ;

//Listener = in this mode, the instance only receives frames, does not respond
//Tag = Exchanges DecaRanging messages (Poll-Response-Final) with Anchor and enabling Anchor to calculate the range between the two instances
//Anchor = see above

// This file defines data and functions for access to Parameters in the Device

// -------------------------------------------------------------------------------------------------------------------
// structure to hold device's logging data


typedef struct
{
	uint32 icid;

	dwt_rxdiag_t diag;

#if DECA_LOG_ENABLE==1
    int         accumLogging ;                                // log data to a file, used to indicate that we are currenty logging (file is open)
	FILE        *accumLogFile ;                               // file
#endif

} devicelogdata_t ;

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

#if 0
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
    //uint8 destAddr[ADDR_BYTE_SIZE];           	//  07-08 or using 64 bit addresses (07 - 14)
    uint8 sourceAddr[ADDR_BYTE_SIZE];           	//  07-08 or using 64 bit addresses (07 - 14)
    uint8 messageData[MAX_USER_PAYLOAD_STRING] ;    //  15-124 (application data and any user payload)
    uint8 fcs[2] ;                              	//  125-126  we allow space for the CRC as it is logically part of the message. However ScenSor TX calculates and adds these bytes.
} srd_msg_bcast ;
#endif

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

#define RTD_MED_SZ          8      // buffer size for mean of 8

typedef struct
{
	INST_MODE mode;				//instance mode (tag, anchor or listener)
    int testAppState ;
    int nextState ;
    int previousState ;
    int done ;

	dwt_config_t    configData ;
	dwt_txconfig_t  configTX ;
    uint32 rxTimeouts ;
//#if (DEEP_SLEEP == 1)	
	int txmsgcount;
	int	rxmsgcount;
//#endif
    uint8 shouldDoDelayedRx ;
#if (DR_DISCOVERY == 0)
    instanceAddressConfig_t payload ;
#endif

    uint16 seqnum; //add seqnum in poll segment, current handle seqnum
    srd_msg msg ; // simple 802.15.4a frame structure (used for tx message)
//	iso_IEEE_EUI64_blink_msg blinkmsg ; // simple 802.15.4a frame structure (used for tx blink message)
    srd_msg rxmsg ; //holds received frame (after a good RX frame event)
//	ack_msg rxackmsg ; //holds received ACK frame 
//	union {
//	iso_IEEE_EUI64_blink_msg rxblinkmsg;
//	iso_IEEE_EUI64_blinkdw_msg rxblinkmsgdw;
//	}blinku;

	uint16 tagShortAdd ;
	uint16 psduLength ;

	union {

		uint64 txTimeStamp ;		   // last tx timestamp
		uint64 tagPollTxTime ;		   // tag's poll timestamp
	  uint64 anchorRespTxTime ;
	}txu;

	union {

		uint64 rxTimeStamp ;		   // last rx timestamp
		uint64 anchorRespRxTime ;	   // receive time of response
	}rxu;
	uint16 rxLength ;
	uint8 ackreq;

	uint16 txantennaDelay ;

    uint64 tagPollRxTime ;         // time we received the Poll

	uint64 delayedReplyTime;

    uint64 tof ;
	uint64 m_tof ;
    uint16 relpyAddress ;

	double clockOffset ;

    double adist[RTD_MED_SZ] ;
 //   double adist4[4] ;
    double longTermRangeSum ;
    int longTermRangeCount ;
    int tofindex ;
    int tofcount ;
 //   uint8 lastReportSN ;


//    int last_update ;           // detect changes to status report

	devicelogdata_t devicelogdata;

//	uint8    dispClkOffset ;								// Display Clock Offset

    uint8    macdata_msdu[MAX_MAC_MSG_DATA_LEN];        //

    uint8    frame_sn;
	uint16   panid ;
//    double idistmax;
//    double idistmin;

    double idistance ; // instantaneous distance
    int newrange;
	int oldrange;     //if need send the pre diastance ,it =1

	uint32 lastReportTime;

	uint32 tagSleepTime_us; //in milliseconds
	uint32 tagBlinkSleepTime_us;
	uint32 anchReportTimeout_us;
    uint32 endSleepTime_ms ;
	uint32 endDelayedTime_ms ;

    uint8 instToSleep ;
    uint8 anchorListIndex ;
    uint8 sendTOFR2Tag ;	//sends report to TAG else forwards to Forwarding Address
    uint8 tag2rxReport ;    // tag should get ready to rx report after final message is sent

    uint64 rxOnDelay ;
	uint64 fixedReplyDelay ;
	uint64 fixedReportDelay ;		//TX delay when sending the (ToF) report
	double fixedReplyDelay_ms ;

	uint8 newReportSent;
//    uint8 sentSN;
//    uint8 ackdSN;
//    uint8 recvSN;
    uint8 dataxlen ;
    uint8 wait4ack ;
    uint8 ackexpected ;
    uint8 stoptimer;
    uint8 dwevent[2]; //this holds any TX/RX events - at the moment this is an array of 2 but should be changed to a queue
    uint8 dweventCnt;
    uint8 instancetimer_en;
    uint32 instancetimer; //(TagTimeoutTimer) --- this timer is used to timeout Tag when in deep sleep so it can send the next poll message

	uint32 slottimer;
//	uint32 toftickcount;
//	uint32 toftickcount1;
//	uint64 toftickcount_64;
	uint8 deviceissleeping; //this disabled reading/writing to DW1000 while it is in sleep mode 
							//(DW1000 will wake on chip select so need to disable and chip select line activity)

	uint8 rxautoreenable;
	uint16 lp_osc_cal ;
	uint16 blinktime ;
	uint8 eui64[8];
	uint8 tagToRangeWith;	//it is the index of the tagList array which contains the address of the Tag we are raning with
	uint8 tagCnt;
	uint8 TagSlot;
//	uint8 stawakeuptick;
//	uint64 tagList[TAG_LIST_SIZE];
	uint64 get_anchoraddr;
	uint16 shortaddr16;         //own addr
	uint16 rev_shortaddr16;     //rev source addr
//	int count_waitdata;
	int is_newstatus;
	uint8 is_intoflist;
	uint8 cardstatus;
	uint8 cur_slot;           //current slot
	SLOT_STATUS cur_slot_msg;
	uint8 next_idle_1sslot ;    //station use,if used in  USED_TOF ,this equal the next wakeup slot ; if USED_BLINK ,this slot used to allot the rev card
	uint8 next_idle_5sslot ;
	uint8 rev_card_type;         //rev card is 1s or 5s
//	uint8 only_blink_slot;       //is in the only blink slot or not ,1 yes , 0 no
	uint8 slotlist_full;         //all the idle slot have allot to the card ,is full; 1:full    0:not full
	uint8 have_rev_pollOblink;   //this slot have rev poll or blink
	uint16 cur_slot_seq;         //current slot's seqnum
	uint16 up_revrpoll_time;  //from slot start to rev first poll ,used to compensation the sleep time
	uint32 slot_starttick;
	uint8 station_status;         //station's status 
	uint8 shouldrepvbat;          //1:shoud send Battery and version
	uint8 tdoarepvbat;
	uint16 cardbattery;               //card's battery
	uint8 curslot_column;          //the station's current slot's column
	uint8 curslot_row;             //the station's current slot's cow
	uint8 change_devtype;          //change card's devtype or not
	uint8 revpolltype;             //whether the card rev the poll or not
	int8  i8rssi;
	uint8  help_excit_send;    //the alarm msg have been send to  the station,1:yes  ,0: no ; so is_send=0x11, help &0x01  ;excit &0x10
	uint8 new_inblink;
	uint8 car_revcard;
	
	//TDOA扩展
	uint8 u8TxSpeedType;
	uint16 u16shortaddr16dest;  
} instance_data_t ;


/*
typedef struct
{
    double idist ;
    double mean4 ; 
    double mean ; 
    int numAveraged ;
	uint32 lastRangeReportTime ;
} currentRangeingInfo_t ;
*/
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
    uint32 m_distance;             //main sation distance
	uint32 s_distance;             //sub station distance
} slot_msg_t;

typedef struct
{
    SLOTLIST_MODE slotmode;    // 0x00: idle, ;0x01: 1s card ;0x02:5s card ;
    uint8 num_of5s;          //the number of 5s card have used
    slot_msg_t allocslot[MAX_CARD_CYCLE_SEC];  //every row have MAX_CARD_CYCLE_SEC slot 
} alloc_Slot_t;

typedef struct
{
	uint8 u8cardaddr[ADDR_BYTE_SIZE];   //the slot's card addr
	uint16 u16seqnum;                   //current seq
	uint32 m_distance;                  //current main station distance
	uint32 s_distance;                  //current sub station distance
} uart_distmsg_t;

typedef struct
{
	uint8 u8cardaddr[ADDR_BYTE_SIZE];   //the slot's card addr
	uint8 status;                   //card's status
	uint8 devtype;                  //devtype
} Car_cardsmsg_t;

typedef struct
{
	uint8 u8CardCnt;
	uint32 m_distance;             //main sation distance
	uint32 s_distance;             //sub station distance
    Car_cardsmsg_t cardmsg[MAX_CAR_CARD_CNT];
    
} ts_Car_cardlist;

typedef struct
{
	uint8 count;
	uint16 addr[MAX_ALARM_NUM];
}alarm_addrlist_t;

typedef struct
{
	uint16 alarmaddr;
	uint8  alarmstatus;
	uint8  excitid;
}Sub_alarm_msg_t;

typedef struct
{
    uint16 u16CardNum[MAX_SET_CARD_CNT];
    uint8 u8CardCnt;
} tsCardTypeSet;

typedef struct
{
    uint8 channel ;
    uint8 prf ;
    uint8 datarate ;
    uint8 preambleCode ;
    uint8 preambleLength ;
    uint8 pacSize ;
    uint8 nsSFD ;
} chConfig_t ;

static chConfig_t chConfig[9] ={
//mode 1 - S1: 7 off, 6 off, 5 off
	{
		1,              // channel    //tof channel
		DWT_PRF_16M,    // prf
		DWT_BR_110K,    // datarate
		3,             // preambleCode
		DWT_PLEN_128,	// preambleLength
		DWT_PAC32,		// pacSize
		1		// non-standard SFD
	},

	 //mode 2
	{
		2,              // channel  //blink channel
		DWT_PRF_16M,    // prf
		DWT_BR_6M8,    // datarate
		3,             // preambleCode
		DWT_PLEN_128,	// preambleLength
		DWT_PAC8,		// pacSize
		0		// non-standard SFD
	},
	//mode 3
	{
		2,              // channel
		DWT_PRF_64M,    // prf
		DWT_BR_110K,    // datarate
		9,             // preambleCode
		DWT_PLEN_1024,	// preambleLength
		DWT_PAC32,		// pacSize
		1		// non-standard SFD
	},

	//mode 4
	{
		2,              // channel
		DWT_PRF_64M,    // prf
		DWT_BR_6M8,    // datarate
		9,             // preambleCode
		DWT_PLEN_128,	// preambleLength
		DWT_PAC8,		// pacSize
		0		// non-standard SFD
	},

	//mode 5
	{
		5,              // channel
		DWT_PRF_16M,    // prf
		DWT_BR_110K,    // datarate
		3,             // preambleCode
		DWT_PLEN_1024,	// preambleLength
		DWT_PAC32,		// pacSize
		1		// non-standard SFD
	},

	{
		5,              // channel
		DWT_PRF_16M,    // prf
		DWT_BR_110K,    // datarate
		3,             // preambleCode
		DWT_PLEN_64,	// preambleLength
		DWT_PAC32,		// pacSize
		1		// non-standard SFD
	},

	{
		5,              // channel
		DWT_PRF_16M,    // prf
		DWT_BR_6M8,    // datarate
		3,             // preambleCode
		DWT_PLEN_128,	// preambleLength
		DWT_PAC8,		// pacSize
		0		// non-standard SFD
	},

	//mode 7
	{
		5,              // channel
		DWT_PRF_64M,    // prf
		DWT_BR_110K,    // datarate
		9,             // preambleCode
		DWT_PLEN_1024,	// preambleLength
		DWT_PAC32,		// pacSize
		1		// non-standard SFD
	},

	//mode 8
	{
		5,              // channel
		DWT_PRF_64M,    // prf
		DWT_BR_6M8,    // datarate
		9,             // preambleCode
		DWT_PLEN_128,	// preambleLength
		DWT_PAC8,		// pacSize
		0		// non-standard SFD
	}
};

/**************************************** TDOA部分定义 start ***********************************************/
#define ADDR_BYTE_SIZE_L            (8)  //长地址使用长度
#define ADDR_BYTE_SIZE_S            (2)  //短地址使用长度

//设备使能和未使能标志
#define TDOA_EABLE      1
#define TDOA_DISEABLE   0

#define TDOA_UART_SEND_MAX_TAG_NUM  	  8               //串口发送一次性最多发送的卡数据

#define TDOA_MSG_TESTCARD_MAX_NUM  	  20                  //组包信息中待测卡数量最大值
#define TDOA_STANDCARD_MAX_NUM       (5)                  //同一个基准能够接收到的最多的基准基站的数量
#define TDOA_PACK_SEND_MAX_NUM       (50)                 //发送最大 TimeStamp_mgr 的个数 组包后往串口发送前需要进行包数量统计

#define TDOA_DW_TICK_MAX             1099511627775       //0xffffffffff 即最大tick数

#define TDOA_INST_PANID_TAG  		 0xdeca              //实例的Panid
#define TDOA_INST_PANID_ANCHOR       0xeeee              //基站的panid

//射频延时默认值
#define DWT_PRF_64M_RFDLY   (515.6f)
#define DWT_PRF_16M_RFDLY   (515.0f)
extern const uint16 gau16RfDelays[2];

#define TDOA_INST_MODE_SPEED_SLOW    0x0                 //轮询发送帧中标记待测卡
#define TDOA_INST_MODE_SPEED_QUCIK   0x1				 //轮询发送帧中标记快发卡


#define TDOA_INST_ACK_REQUESTED               (1)        //请求确认帧 Request an ACK frame
#define TDOA_INST_SEND_POLL_DEST_ADDR        (0xFFFF)     //待测卡和快发卡的目的地址设置为广播地址
#define TDOA_INST_FRAME_TYPE_BIT              0          //数据帧类型 标记是否是轮询消息数据
#define TDOA_INST_FRAME_SPEED_TYPE_BIT        1          //数据帧中记录标签卡的速率类型位
#define TDOA_INST_FRAME_SEQNUM_BIT_H          2          //数据帧中记录消息序列号的高位部分数据
#define TDOA_INST_FRAME_SEQNUM_BIT_L          3          //数据帧中记录消息序列号的低位部分数据
#define TDOA_INST_FRAME_ARRAY_DATA_LEN        6          //事件发送帧中数据数组占用的数据长度
 
#define TDOA_CHANNEL_CONFIG_MODE_LEN 9    //通道配置模式数量
#define TDOA_CHANNEL_CONFIG_MODE_ONE 0    //通道配置模式数量

/*********************设备参数设置*****************************/
//#define DEV_ADDRESS_CARDID        		10001  //设备卡ID设置 设备的16位短地址
#define SLOW_SPEED_TAG_SEND_TIME  		1000   //待测卡触发周期即待测卡信息发送周期 400
#define QUICK_SPEED_TAG_SEND_TIME   	45     //待测卡触发周期即待测卡信息发送周期 45
#define DEVICE_UART_SEND_TIME   	    1000   //串口缓冲区信息发送周期 1000 不能大于卡的轮询周期
#define SECOND_TO_MS_UNIT  		        1000   //秒和毫秒的转换

#define TDOA_SLOW_CARD_SEND_COUNT         30    //待测卡发送的tdoa消息次数 发送30秒
#define TDOA_QUICK_CARD_SEND_COUNT        (SECOND_TO_MS_UNIT/QUICK_SPEED_TAG_SEND_TIME ) * (TDOA_SLOW_CARD_SEND_COUNT - 10)    //快发卡发送的tdoa消息次数

#define TOF_CARD_TO_STATION_NUM  3 //与卡进行tof测距的基站数量


typedef enum inst_statesNew
{
    TDOA_INIT, //0
	TDOA_RX_WAIT_DATA,			  //8
/*		
    TA_TXE_WAIT,                //1/
    TA_TXPOLL_WAIT_SEND,        //2/
    TA_TXFINAL_WAIT_SEND,       //3/
    TA_TXRESPONSE_WAIT_SEND,    //4/
    TA_TXREPORT_WAIT_SEND,      //5
    TA_TX_WAIT_CONF,            //6

    TA_RXE_WAIT,                //7

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
    TA_TDOA_WAIT_SEND             //19   
 */   
 
} TDOA_INST_STATES_E;

typedef enum instanceModesNew
{
	TDOA_INST_TAG_TEST, 
	TDOA_INST_ANCHOR, 
//	TDOA_INST_TAG_STANDARD,
	
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

typedef struct
{
	uint16 u16DestAddr;          //目标基站地址
	uint16 u16StaRespFinalFrame; //目标基站地址的是否进行最后一帧响应的标志
}TOF_INST_CARD_STATA_S;

typedef struct
{
	TOF_INST_CARD_STATA_S    stTofCardState[TOF_CARD_TO_STATION_NUM];
	uint16 u16StaCount;
	uint16 u16CarsTofStart;  //是否允许开启卡开启tof测距 若为true则允许开启 若为false则不允许开启
}TOF_INST_CARD_DATA_S;

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
	INST_MODE_TX_SPEED_SLOW = 0,  //标记为待测卡
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
	
}TDOA_INSTANCE_BASE_DATA_S;

/********对DW1000接收缓冲区读写的结构体*******/
//802.15.4a帧结构
//发送给接收器缓冲区前进行数据封装的结构
//由接收器缓冲区接收寄存器读取的接收数据结构
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
instance_data_t* TdoaGetOldLocalInstStructurePtr(void);

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
void TdoaCardMsdBuildUnity(TDOA_INST_RXMSG_TO_CARDMSG_S *pstCardRxMsg, 
						   TDOA_UWB_TIMESTAMP_MSG_ARRAY_S* pstTdoaMsgArray, 
						   int iMsgArrayNum);
int TdoaCheckRxMsgStandardID(TDOA_INST_RXMSG_TO_CARDMSG_S *pstCardRxMsg);
void TdoaRxSlowCardMsgProc(TDOA_INST_RXMSG_TO_CARDMSG_S *pstCardRxMsg);
void TdoaRxQuickCardMsgProc(TDOA_INST_RXMSG_TO_CARDMSG_S *pstCardRxMsg);
void TdoaMsgPackInsetUartBuff(TDOA_UWB_TIMESTAMP_MSG_ARRAY_S* pstTdoaMsgArray);
void TdoaDataBuffClear(void);

//void init_anchor(TDOA_INSTANCE_DATA_S *inst);
/**************************************** TDOA部分定义 end ***********************************************/

//-------------------------------------------------------------------------------------------------------------
//
//	Functions used in logging/dispalying range and status data
//
//-------------------------------------------------------------------------------------------------------------

// function to calculate and report the Time of Flight to the GUI/display
uint16 instance_get_cardid(void);

void reportTOF(instance_data_t *inst);

// clear the status/ranging data 
void instanceclearcounts(void) ;
uint64* instgettaglist(void);
void instcleartaglist(void);
void instsettagtorangewith(int tagID);

//void instancegetcurrentrangeinginfo(currentRangeingInfo_t *info) ;

// enable reading of the accumulator - used for displaying channel impulse response
void instancesetaccumulatorreadenable(int enable) ;      // enable reading of accumulator every 'period' frames
void instance_set_alarmlist(uint16 addr,uint8 type,uint8 status);
void instance_set_helpstatus(uint16 addr);
uint8 instance_get_retreatstatus(void);


void dec_sleep_wakeup(void);

void instancerxon(int delayed, uint64 delayedReceiveTime);

void clear_inblinkmsg(void);

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

uint8 instance_setpower_rssi(uint8 type);

void instance_init_slotlist(void);

void instance_init_cardslot(uint8 cardtype);

void instance_clear_substa(void);

uint8 instance_get_listslotmsg(uint8 cur_slot);

uint16 get_curslot_destaddr(uint8 curslot) ;

uint8 get_curslot_cardmsg(uint16 *cardid); 

uint8 instance_get_distancelist(uwb_tof_distance_ts *distance, uwb_tof_distance_ts *predistance ,uint8 type);
#ifdef DEC_UWB_SUB
uint8 instance_get_car_cardlist(uwb_tof_distance_ts *distance,Car_cardsmsg_t *car_card);
#endif
void instance_set_AnchorPanid(uint8 type);

void txhelp_call_send(void);

void tdoa_send(void);
// configure the instance and DW1000 device
void instance_config(instanceConfig_t *config) ;  

void instance_change_channel(uint8 channel);

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
void instance_rxcallback(const dwt_callback_data_t *rxd);
void instance_txcallback(const dwt_callback_data_t *txd);

// sets the Tag sleep delay time (the time Tag "sleeps" between each ranging attempt)
void instancesettagsleepdelay(uint32);
// sets the Tag/Anchor reply delay time (the time Tag/Anchor wait before sending the reply message (Final/Response))
void instancesetreplydelay(double delayms);
void instancesetblinkreplydelay(double delayms); //delay in ms

// set/get the instance roles e.g. Tag/Anchor/Listener
void instancesetrole(int mode) ;                // 
uint8 instancegetrole(void) ;
// get the DW1000 device ID (e.g. 0xDECA0130 for MP)
uint32 instancereaddeviceid(void) ;                                 // Return Device ID reg, enables validation of physical device presence

int instance_getevent(void);

double instance_get_adist(void);

double instance_get_idist(void);

double instance_get_m_idist(void);

double instance_get_ldist(void);

uint16 instance_get_seqnum(void);

void instance_set_seqnum(void);

uint8 instance_get_helpstatus(void);

void instance_reset_helpstatus(void);

uint8 instance_rev_poll(void);

void instance_set_idle(void);

void instance_cardnewslot_init(void);
uint16 instance_get_sleeptick(void);

uint16 instance_get_ifrevsig(void);

uint16 instance_get_uptick(void);

int instancenewrange(void);
int instanceoldrange(void);

int instance_new_Car(void);

void instance_set_car_rev(void);

int instance_get_anchoraddr(void);
	
//void insertbuff(instance_data_t *inst, int start);

int instance_get_lcount(void) ;

int instance_get_rxf(void);

int instance_get_txf(void); //get number of Txed frames
int instance_get_status(void);
void instance_set_status(int eventstatus);
void instance_set_event(int type);
void card_rev_poll_tontinue_count(void);

void instance_set_card_status(uint8 cardstatus);    

uint8 instance_get_card_status(void) ;

void instance_set_slot_starttick(uint32 tick);

uint32 instance_get_slot_starttick(void);

void instance_set_sta_status(uint8 status);

void instance_reset_sta_status(uint8 status);

uint8 instance_get_sta_status(uint8 status);

void instance_set_vbat(uint16 vdd);

void instance_change_devtype(void);

uint8 instance_getchange_devtype(void);

void instance_set_revpolltype(uint8 type);

uint8 instance_get_revpolltype(void);


void instance_set_helpexcit(uint8 type);

int8 instance_get_rssi(void);

uint8 instance_get_inblinkmsg(void);

int8 instance_get_powerlever(void);

void dec_sleep_wakeup(void);

#define DWT_PRF_64M_RFDLY   (515.6f)
#define DWT_PRF_16M_RFDLY   (515.0f)
extern const uint16 rfDelays[2];
#ifdef __cplusplus
}
#endif

#endif





