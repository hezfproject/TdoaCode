

#ifndef  __CONFIG_H_INCLUDED__
#define  __CONFIG_H_INCLUDED__

#if defined __cplusplus
extern "C" {
#endif

/****************************************************************************/
/***        Include Files                                                 ***/
/****************************************************************************/
#ifndef CardReader_PROJECT
#include <jendefs.h>
#endif
#include "app_protocol.h"
#include "version.h"
/****************************************************************************/
/***        Macro Definitions                                             ***/
/****************************************************************************/

#define TOF_CARD_NWK_ADDR			0xFFF0
#define TOF_LOCNODE_NWK_ADDR		0xFFF2 //for loc node.


#define	TOF_SLOT_NUM				750  // 全部slot个数，15s/20ms = 750
#define	TOF_SLOT_LOC_INT			25
#define	TOF_SLOT_LOC_PERIOD			50   // slot分配的最小周期，1s = 50 slot
#define TOF_SLOT_LOC_OFFSET         12   // 2号辅站与23张卡做tof，是从第12 slot个开始，11 slot结束(其中0、1 slot是辅站的)


#define	TOF_SLOT_IDLE_NUM			3    // IDLE slot的个数

#define	TOF_SLOT_MS					20
#define	TOF_SLOT_TICK				(TOF_SLOT_MS * 16000)

#define TOF_SLOT_APPR_MS_2		13	// the approximately tof processing time (ms) with max readings = 2
#define	TOF_LOCATOR_PERIOD_MS	(TOF_SLOT_LOC_INT*TOF_SLOT_MS) //500ms
#define TOF_STATION_PERIOD_MS	(TOF_SLOT_NUM * TOF_SLOT_MS)   // 15s

#define TOF_STATION_LOCATOR_MAX_FAILED	20	// loc will disconnect if failed 20 times
#define TOF_STATION_CARD_MAX_FAILED		2	// card will disconnect if failed 2 times
#define TOF_SEND_CARD_INFO_APPR_MS		5	// S-L exchange data need at most 5 ms

#define JN_RED		E_AHI_DIO8_INT	// red LED
#define JN_GREEN	E_AHI_DIO9_INT	// green LED

#define TOF_ERROR		10			// the error of tof (10m)

#define BIT(n)		(1UL<<(n))

#define HAVE_IDLE_LOC_SLOT     BIT(0)
#define HAVE_IDLE_FIVES_SLOT   BIT(1)
#define HAVE_IDLE_ONES_SLOT    BIT(2)
#define HAVE_IDLE_FIFTEEN_SLOT BIT(3)
#define NEW_STATION_ENVIRONMENT  BIT(4)


#define DEFAULT_CHANNEL_MOBILE 		14
#define DEFAULT_CHANNEL_BROADCAST	25
#define DEFAULT_CHANNEL_TOF 			24
#define DEFAULT_CHANNEL_LOCATOR 		20
#define DEFAULT_CHANNEL_RSSI         16
/************************************************************************/
// TOF protocol for RF
#define RF_MAX_ACCEPT_NUM	10	// max num for card to join per time

// 设备类型
typedef enum
{
	DEVICE_TYPE_CARD5S   = 1,
	DEVICE_TYPE_LOCARTOR = 2,
	DEVICE_TYPE_STATION  = 3,
	DEVICE_TYPE_GAS      = 4,
	DEVICE_TYPE_CARD1S   = 5,
	DEVICE_TYPE_CARD15S  = 6,
}tof_device_type_te;

typedef enum
{
	TOF_LOCATOR_REQUEST = 0x01,	//locator request to join a station
	TOF_LOCATOR_FINISH,
	TOF_LOCATOR_CARDS_DIST,

	TOF_CARD_CHECKIN,			// card send regist msg // = 4
	TOF_CARD_RSSI,				// card send RSSI msg
	TOF_CARD_REQUEST,			// card request tof to specify station
	TOF_CARD_ALARM, 			// card alarm

	TOF_STATION_AVAILABLE, 	// station is available, for locaotr or card to join // = 8
	TOF_STATION_ILDE, 			// station has idle slot, card need wait for AVAILABLE cmd to join request
	TOF_STATION_RSSI,			// station need card to send rssi info
	TOF_STATION_BUSY,			// station has not idle slot, card need wait for RSSI cmd to send rssi // = B
	TOF_STATION_ACCEPT,		// station accepts card or locator // = C
	TOF_STATION_FINISH,		//station tof finished // = D
	TOF_STATION_HELP_ACK, 	// the station receive help from cards // = E
	TOF_STATION_NOPWD_ACK, 	// the station receive nopwd from cards // = F
	TOF_STATION_CHECKIN_ACK,	// the station receive card's regist info // = 0x10
	TOF_STATION_CARDS_INFO,	// station send cards' info to locator, to indicate which cards need locating // = 0x11
	TOF_APP_HELP_ACK, 			// the station receive help ack from app, need let card know // = 0x12
	TOF_STATION_WAIT, 		// Station ask card to wait

    TOF_GASNODE_REQUEST,
    TOF_GASDENSITY,
    TOF_GASNODE_ALARM,
    TOF_GASNODE_RSSI,
    TOF_GASNODE_HELP_ACK,  //24
    TOF_GASNODE_FIRE_ACK,
    TOF_GASNODE_WATER_ACK,
    TOF_GASNODE_TOPBOARD_ACK,
    TOF_GASNODE_OTHER_ACK,

    TOF_STATION_LOC_INFO,   // Station send it's panid to locator

    TOF_LOC_VERSION_INFO,
    TOF_CARD_EXCITE,
    TOF_STATION_EXCITE_ACK,

    CARD_BASE_INFO,   //33
    CARD_REMARK,
    HANDLE_STATION_ACK,

    CHANGE_PERIOD,

    CARD_TYPE_CHEACK_ACK,
    TOF_STATION_RSSI_CHECK = 38,
    TOF_STATION_RSSI_CHECK_ACK,   //39
    TOF_STATION_RSSI_CHANNEL = 40,

    TOF_STATION_THROUGH      = 41,

    HAND_STATION_RSSI_CHANNEL,     //手持机修改rssi频道
    HAND_STATION_RSSI_CHANNEL_ACK,

    HAND_STATION_WRITE_ACK,
    CARD_LOC_INFO,

    DEVICE_CARD_PERIOD_CHANNEL = 46,
    DEVICE_CARD_PERIOD_CHANNEL_ACK = 47,
    DEVICE_CARD_ERPID = 48,
	TOF_CARD_OAD = 0xF0,		// card's OAD controller send this msg to card when card do "checkin" event
	TOF_LOCATOR_OAD = 0xF2,	// locator's OAD controller send this msg to locator
	TOF_LOC_STATION_OAD = 0xF4,	// station's OAD controller send this msg to station
	TOF_RSSI_STATION_OAD = 0xF5,	// station's OAD controller send this msg to station
	TOF_CHECKIN_STATION_OAD = 0xF6,	// station's OAD controller send this msg to station
	TOF_COM_STATION_OAD = 0xF7,	// station's OAD controller send this msg to station

}rf_tof_msg_te;

typedef enum
{
	STATION_STATUS_NORMAL,
	STATION_STATUS_RETREAT,
	STATION_STATUS_SINGLE_RETREAT,
	STATION_STATUS_SPEED,
}tof_station_status_te;


#define CARD_STATUS_PERIOD_BIT 5

#define CARD_STATUS_PERIOD_5S 	0
#define CARD_STATUS_PERIOD_1S 	1
#define CARD_STATUS_PERIOD_15S 	2
typedef enum
{
	CARD_STATUS_NORMAL 	= 0x01,
	CARD_STATUS_HELP	= 0x02,	// card alert
	CARD_STATUS_NOPWD	= 0x04,	// no power
	CARD_STATUS_RETREAT	= 0x08,	// retreat
	CARD_STATUS_TOF1S	= 0x10,	// T(tof)=1s
	CARD_STATUS_RETREAT_ACK = 0x20, // retreat ack
	CARD_STATUS_TOF15S	= 0x40,     //为了卡兼容1308的基站而做的改动:CARD_STATUS_TOF1S=0&CARD_STATUS_TOF15S=0,表示5秒卡

	//CARD_STATUS_TOF_PERIOD	= (0x40 | 0x20),	// 用两个bit表示定位卡周期: 0x0: 5s, 0x1: 1s, 0x2: 15s
    CARD_STATUS_EXCITER = 0x80,    //exciter
}tof_card_status_te;

typedef enum
{
    GASNODE_STATUS_NORMAL =0,
    GASNODE_STATUS_RETREAT = 0x01,
    GASNODE_STATUS_HELP		= 0x02,	// gasnode alert
	GASNODE_STATUS_FIRE	= 0x04,	// fire alarm
	GASNODE_STATUS_WATER	= 0x08,	// water alarm
	GASNODE_STATUS_TOPBOARD =0x10,  // topboard alarm
	GASNODE_STATUS_OTHER    = 0x20, //other alarm
}tof_gasnode_status_te;


// send finish to card
typedef struct
{
	app_header_t tof_head;

	uint8 u8StationStatus; 	// tof_station_status_te
	uint8 u8RunMs;	// Station runs ms in this slot
	uint8 u8LocN;	// 0: not need locator, 1: need locator #1, 2: need locator #2
	uint8 u8Reserved;
	uint16 u16Dist2Station;
	uint16 u16Dsit2Locator;
} rf_tof_station_finish_ts;

// send signal in common channel, for AVAILABLE, IDLE, RSSI, BUSY cmd
typedef struct
{
	app_header_t tof_head;

	uint8 u8StationStatus; 	// tof_station_status_te
	uint8 u8AvailableMs;		// the available time for card to send join request (card use this to generate a random number)
	uint16 u16CurSlot;		// station's curSlot

	//fixme, this is not a 4-Byte struct
	uint8 u8RunMs;			// station run ms in this slot
	uint8 u8LocIdle; 			// 1: has idle loc slot, 0: has not
} rf_tof_station_signal_ts;

typedef struct
{
	uint16 u16ShortAddr;	// accepted locator or card's addr
	uint16 u16SlotIndex;		// card or locator's slot index
} tof_station_accept_data_ts;

typedef struct
{
	app_header_t tof_head;

	uint8 u8StationChannel;	// station's channel
	uint8 u8LocatorCardChannel;	// locator and card's channel
	uint16 u16CurSlot;	// station's curSlot

	uint8 u8RunMs;	// station run MS from this slot
	uint8 u8AcceptNum;	// accepted card's number
	uint16 u16Reserved;

	tof_station_accept_data_ts tsAccptData[RF_MAX_ACCEPT_NUM];	// station at most accept 10 per time
} rf_tof_station_accept_ts;

typedef enum
{
    E_CARD_BASE_INFO,					// nothing to do
    E_CARD_REMARK_INFO,				// card listen
} CardInfoType;


typedef struct
{
    unsigned char   frameSum;
    unsigned char   frameSeq;
    unsigned char   datatype;
    unsigned char   len;
    //data
}app_eDev_Data_t;


typedef struct
{
	app_header_t tof_head;

	uint8 u8RunMs; 			// station runs ms in this slot
	uint8 u8CardNum;		// need locator's card number,allways ==23
	uint16 u16CardAddr[TOF_SLOT_LOC_INT-2];	// 主站上23个slot的定位卡ID, 未入网或不是定位卡的标记为 0
}rf_tof_station_card_info_ts;

typedef struct
{
	app_header_t tof_head;

	uint8 u8CardNum;		// need locator's card number, allways ==23
	int_8 i8Rssi;           // 主站到辅站的 RSSI, 由辅站再发给主站
	uint16 u16CardDist[TOF_SLOT_LOC_INT-2];	// 辅站上 23个slot的定位结果，无结果的为INVALID_DISTANCE
}rf_tof_locator_card_info_ts;

// for card to join
typedef struct
{
	app_header_t tof_head;

	uint16 u16SeqNum;
	uint8 u8CardStatus;	// tof_card_status_te & card type & exciter
	//uint8 u8Reserved;
	uint8   u8ExciterIDorAccStatus;   // corresponding to (u8CardStatus & 0x80)true:ExciterID;false:AccelerometerStatus
	uint16 u16OadVersion;
	uint16 u16Battery;
}rf_tof_card_data_ts;

typedef struct
{
	app_header_t tof_head;

	uint16 u16SeqNum;
	uint8 u8CardStatus;	// tof_card_status_te & card type & exciter
	//uint8 u8Reserved;
	uint8   u8ExciterIDorAccStatus;   // corresponding to (u8CardStatus & 0x80)true:ExciterID;false:AccelerometerStatus
	uint16 u16OadVersion;
	uint16 u16Battery;
    uint8  u8Erpid[20];
}rf_tof_card_base_data_ts;


typedef struct
{
	app_header_t tof_head;

	uint16 u16SeqNum;
	uint8 u8CardStatus;	// tof_card_status_te & card type & exciter
	//uint8 u8Reserved;
	uint8   u8ExciterIDorAccStatus;   // corresponding to (u8CardStatus & 0x80)true:ExciterID;false:AccelerometerStatus
	uint16 u16OadVersion;
	uint16 u16Battery;
    app_eDev_Data_t info_data;
}rf_card_info1_ts;

typedef struct
{
	app_header_t tof_head;
    app_eDev_Data_t info_data;
}rf_card_info2_ts;

typedef struct
{
  app_header_t tof_head;
  uint8 cardtype;
}rf_card_type_ts;

typedef struct
{
  app_header_t tof_head;
  uint16 u16StationAddr;
  uint16 u16LocDistance;
}rf_card_distance_ts;



// for OAD using
typedef struct
{
	app_header_t tof_head;

	uint16 u16Version;	// software version
	uint8 u8DeviceType;	// device type
	uint8 u8Reserved;
}rf_tof_oad_data_ts;

typedef struct
{
	app_header_t tof_head;

	uint16 u16GasDensity;
    uint16 u16Distance;
    uint16 u16ShortAddr;
    uint16 u16SeqNum;
    uint16 u16GasThr;
    uint8  u8CardStatus;	// tof_card_status_te
    //uint8  u8Direction;
    //uint8  u8Rssi;
    uint8  u8Reserved;
}rf_tof_gas_density_ts;

typedef struct
{
    app_header_t tof_head;
    uint16 u16SeqNum;
	uint8 u8CardStatus;	// tof_gasnode_status_te
	uint8 u8Reserved;
    uint16 u16GasDensity;
    uint16 u16GasThr;
	uint16 u16OadVersion;
	uint16 u16Battery;
}rf_tof_gas_rssi_ts;

typedef struct
{
    app_header_t tof_head;
    uint16 u16LocOAD;
    uint8 u8LocVersion[98];
}rt_tof_loc_version_ts;

typedef struct
{
    app_header_t tof_head;
    int_8 i8Rssi;
    uint8 u8StationPort;
}rt_tof_station_rssi_ts;

typedef struct
{
	app_header_t tof_head;
    uint8  u8Erpid[20];
}rf_tof_erp_id_data_ts;


typedef union
{
	app_header_t tof_head;
	rf_tof_card_data_ts rf_tof_card_data;
	rf_tof_locator_card_info_ts rf_tof_locator_card_info;
	rf_tof_station_card_info_ts rf_tof_station_card_info;
	rf_tof_station_accept_ts rf_tof_station_accept;
	rf_tof_station_signal_ts rf_tof_station_signal;
	rf_tof_station_finish_ts rf_tof_station_finish;
	rf_tof_oad_data_ts rf_tof_oad_data;
    rf_tof_gas_density_ts rf_tof_gas_density;
    rf_tof_gas_rssi_ts rf_tof_gas_rssi;
    rt_tof_loc_version_ts rt_tof_loc_version;
    rt_tof_station_rssi_ts rt_tof_station_rssi;

    rf_card_info1_ts  rf_card_info1;
    rf_card_info2_ts  rf_card_info2;
    rf_card_type_ts   rf_card_type;
    rf_card_distance_ts rf_card_distance;
    rf_tof_card_base_data_ts rf_tof_card_base_data;
    rf_tof_erp_id_data_ts   rf_tof_erp_id_data;
}RfTofWrapper_tu;

// end of TOF protocol for RF
/************************************************************************/

#if defined __cplusplus
}
#endif

#endif  /* CONFIG_H_INCLUDED */


