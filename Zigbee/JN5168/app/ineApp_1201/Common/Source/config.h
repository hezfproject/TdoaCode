

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

//#define __TOF_BURST_TEST__

#ifdef __TOF_BURST_TEST__
#define	TOF_SLOT_NUM				100
#define	TOF_SLOT_IDLE_NUM			3	// the number of not use slots between 2 locator
#define	TOF_SLOT_LOC_FREQUENCE	2	// the locator wakeup 2 times duiring TOF_STATION_PERIOD_MS
#else
#define	TOF_SLOT_NUM				250
#define	TOF_SLOT_IDLE_NUM			3
#define	TOF_SLOT_LOC_FREQUENCE	5	// the locator wakeup 5 times duiring TOF_STATION_PERIOD_MS
#endif

#define	TOF_SLOT_MS				20
#define	TOF_SLOT_TICK				(TOF_SLOT_MS * 16000)
#define 	TOF_SLOT_APPR_MS_2		13	// the approximately tof processing time (ms) with max readings = 2
#define	TOF_SLOT_LOC_INT			(TOF_SLOT_NUM / TOF_SLOT_LOC_FREQUENCE)	// slots #0, #(TOF_SLOT_LOC_INT), #(2*TOF_SLOT_LOC_INT), #(3*TOF_SLOT_LOC_INT), #(4*TOF_SLOT_LOC_INT) are for locator
#define	TOF_LOCATOR_PERIOD_MS	(TOF_SLOT_LOC_INT*TOF_SLOT_MS)
#define 	TOF_STATION_PERIOD_MS	(TOF_SLOT_NUM * TOF_SLOT_MS)

#define TOF_STATION_LOCATOR_MAX_FAILED	20	// loc will disconnect if failed 10 times
#define TOF_STATION_CARD_MAX_FAILED		2	// card will disconnect if failed 2 times
#define TOF_SEND_CARD_INFO_APPR_MS		5	// S-L exchange data need at most 5 ms
#define	CARD_INFO_LEN		(TOF_SLOT_LOC_INT-1)	// S-L exchange data: at most CARD_INFO_LEN cards

#define JN_RED		E_AHI_DIO8_INT	// red LED
#define JN_GREEN	E_AHI_DIO9_INT	// green LED

#define TOF_ERROR		10			// the error of tof (10m)

#define BIT(n)		(1UL<<(n))

#define DEFAULT_CHANNEL_MOBILE 		14
#define DEFAULT_CHANNEL_BROADCAST	25
#define DEFAULT_CHANNEL_TOF 			24
#define DEFAULT_CHANNEL_LOCATOR 		20
/************************************************************************/
// TOF protocol for RF
#define RF_MAX_ACCEPT_NUM	10	// max num for card to join per time
#define RF_MAX_INFO_NUM	50	// max num for card info

typedef enum
{
	DEVICE_TYPE_CARD = 1,
	DEVICE_TYPE_LOCARTOR = 2,
	DEVICE_TYPE_STATION = 3,
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
}tof_station_status_te;

typedef enum
{
	CARD_STATUS_NORMAL 	= 0x01,
	CARD_STATUS_HELP		= 0x02,	// card alert
	CARD_STATUS_NOPWD	= 0x04,	// no power
	CARD_STATUS_RETREAT	= 0x08,	// retreat
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
	uint16 u16CardAddr[RF_MAX_INFO_NUM];	// max cards number for locator to do tof per period is RF_MAX_INFO_NUM
}rf_tof_station_card_info_ts;

typedef struct
{
	app_header_t tof_head;

	uint8 u8CardNum;		// need locator's card number
	uint8 u8Reserved;
	uint16 u16CardDist[RF_MAX_INFO_NUM];	// max cards number is RF_MAX_INFO_NUM
}rf_tof_locator_card_info_ts;

// for card to join
typedef struct
{
	app_header_t tof_head;

	uint16 u16SeqNum;
	uint8 u8CardStatus;	// tof_card_status_te
	uint8 u8Reserved;
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
    app_eDev_Data_t info_data;
}rf_card_info1_ts;

typedef struct
{
	app_header_t tof_head;
    app_eDev_Data_t info_data;
}rf_card_info2_ts;



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
    uint16 u16ShortAddr;
    uint8 u8CardType;
    uint8 u8Reserved;
}rt_tof_card_type_ts;



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

    rf_card_info1_ts  rf_card_info1;
    rf_card_info2_ts  rf_card_info2;

}RfTofWrapper_tu;

// end of TOF protocol for RF
/************************************************************************/

#if defined __cplusplus
}
#endif

#endif  /* CONFIG_H_INCLUDED */


