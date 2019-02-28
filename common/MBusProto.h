/*
 * MBusProto.h
 * This header file is designed for definitions of type and data format
 * which are used for transceiving data on uart 485 using ModBus protocol.
 * CRC Algorithm is enclosed in this file.
 *
 * Data format:
 * Master header
 * |----------------------------------------------------------------------|
 * | SEQ | DATA FLAG | CMD | DST ID | LENGTH(IF ANY) | DATA(IF ANY) | CRC |
 * |----------------------------------------------------------------------|
 *
 * Slave header
 * |----------------------------------------------------------------------------------------|
 * | SYNC | SEQ | PADDING BIT | CMD | DST ID | LENGTH(TLV total bytes) | TLV | TLV... | CRC |
 * |----------------------------------------------------------------------------------------|
 *
 * TLV format:
 * |-----------------------------|
 * | TYPE | LENGTH | VALUE(DATA) |
 * |-----------------------------|
 */

#ifndef _MBUSPROTO_H_
#define _MBUSPROTO_H_

#include "CommonTypes.h"

#define MBUS_SYNC "YIRI"
#define MBUS_SYNC_SIZE	4
#define MBUS_PROTO_VERSION 0x01

#define MBUS_IDMAX 0x00FE
#define MBUS_IDMIN 0x0001
#define MBUS_IDCENTER 0x0000
#define MBUS_IDBROADCAST 0xFFFF

#define MBUS_BAUDRATE 115200 //bit per second
#define MBUS_FRAME_INTERVAL 50 // millisecond
#define MBUS_FRAME_TIMEOUT 20 // millisecond
#define MBUS_READ_TIMEOUT 100 // millisecond
#define MBUS_FRAME_MAX_LEN  896

#ifdef WIN32
#pragma pack(push) //align
#pragma pack(1)
#endif


/*
 * CMD of MBUS
 */
typedef enum {
	MBUS_CMD_CLR                    = 0x1,	// request card readers to clear buffer
	MBUS_CMD_QRY                    = 0x2,	// query
	MBUS_CMD_RSP                    = 0x3,  // respond
	MBUS_CMD_RETREAT                = 0x4,  // retreat
	MBUS_CMD_CANCEL_RETREAT         = 0x5,  // cancel retreat
	MBUS_CMD_QUERY_SENSOR_LINK_INFO = 0x6,
	MBUS_CMD_QUERY_SENSOR_CONFIG    = 0x7,
	MBUS_CMD_SET_SENSOR_CONFIG      = 0x8,
	MBUS_CMD_SYN_SLAVE_TIME         = 0x9,
	MBUS_CMD_VERSION_QRY         	= 0xA,
	MBUS_CMD_VERSION_RSP         	= 0xB,
	MBUS_CMD_ALARM_ACK              = 0xC,
	MBUS_CMD_SET_CONFIG_MODE        = 0xD,
	
	MBUS_CMD_EXECUTE                = 0x10,
	MBUS_CMD_EXECUTE_SUCCESS        = 0x11,
	MBUS_CMD_EXECUTE_FAIL           = 0x12,

	MBUS_CMD_EXCEPCTION             = 0xF0,	// exception
} eMBusCmd;

/*
 * CMD DATA STRUCTURE
 */
typedef struct mbus_cmd_t{
    eMBusCmd cmd_no;
    uint_16 dev_id;     // the dest sensor station id
    uint_16 para_len;
    // followed by the para data
} __PACKED mbus_cmd_t;

/*
 * TYPE of TLV
 */
typedef enum {
	MBUS_TLV_CARD_READER = 0x1,

	MBUS_TLV_SENSOR_READER_V2 = 0x10,
	MBUS_TLV_SENSOR_READER_V3_CONFIG = 0x11,
	MBUS_TLV_REAL_TIME = 0x12,
	MBUS_TLV_CARD_ALARM_HELP_ACK = 0x13,

	MBUS_TLV_VERSION_CARD_READER = 0x21,
	MBUS_TLV_VERSION_SENSOR_READER_V3 = 0x22,

	/*Sensor reader, different readers have different TLV types*/
	MBUS_TLV_MIC2000_DATA = 0x80,
	MBUS_TLV_MIC2000_SET = 0x81,
} eMBusTlvType;

/*
 * TYPE of data
 */
typedef enum {
	MBUS_TYPE_ALARM = 1,
	MBUS_TYPE_RSSI = 2,
	MBUS_TYPE_NOPOWER = 3
} eMBusRspType;

/*
 * data structure
 *
 * In sensor v3.0
 * type: the sensor alarm type
 * out_type: the decimal place of the value
 * value: the monitoring physical quantities
 */
typedef struct mbus_sensor_t {
    uint_8 type;
    uint_8 out_type;
    uint_16 value;
} __PACKED mbus_sensor_t;

typedef struct mbus_sensor_packed_t {
      mbus_sensor_t  mbus_sensor [16];
} __PACKED mbus_sensor_packed_t;

/*
 * sensor alarm type
 */
enum {
    ALARM_FREE = 0,
    ALARM_SWITCH_ON = 1,
    ALARM_SWITCH_OFF = 2,
    ALARM_ANALOG = 3,
    ALARM_OUTRANGE = 4,
    ALARM_OFFLINE = 5,
};

//
#define MBUS_GET_MASTER_SEQ(fc) 			((uint_8)(fc)&0x01)
#define MBUS_GET_MASTER_DATAFLAG(fc) 		(((uint_8)(fc)&0x02) >> 1)
#define MBUS_GET_MASTER_VERSION(fc)		(((uint_8)(fc)&0xFC) >> 2)

#define MBUS_SET_MASTER_SEQ(fc, v) 			{fc&=(uint_8)(0xFE); fc|= ((uint_8)v & 0x1);}
#define MBUS_SET_MASTER_DATAFLAG(fc, v)	{fc&=(uint_8)(0xFD); fc|= (((uint_8)v & 0x1) << 1);}
#define MBUS_SET_MASTER_VERSION(fc, v) 	{fc&=(uint_8)(0x03); fc|= (((uint_8)v & 0x3F) << 2);}

typedef struct mbus_hdr_mstr_t {
	uint_8 frame_control;
	/*
	//bit field is not portable
	uint_8 seq:1;
	bool data_flag:1;
	uint_8 version:6;
	*/
	uint_8 cmd;
	uint_16 slv_id;
	// followed by uint_16 data_len and TLVs
} __PACKED mbus_hdr_mstr_t;

#define MBUS_GET_SLAVE_SEQ(fc) 			((uint_8)(fc)&0x01)
#define MBUS_GET_SLAVE_VERSION(fc)		(((uint_8)(fc)&0xFC) >> 2)

#define MBUS_SET_SLAVE_SEQ(fc, v) 		{fc&=(uint_8)(0xFE); fc|= ((uint_8)v & 0x1);}
#define MBUS_SET_SLAVE_VERSION(fc, v)	{fc&=(uint_8)(0x03); fc|= (((uint_8)v & 0x3F) << 2);}

typedef struct mbus_hdr_slv_t {
	char sync[4];
	uint_8 frame_control;
	/*
	//bit field is not portable
	uint_8 seq:1;
	uint_8 :1;  // padding bit
	uint_8 version:6;
	*/
	uint_8 cmd;
	uint_16 slv_id;
	uint_16 data_len;
	// followed by TLVs
} __PACKED mbus_hdr_slv_t;

typedef struct mbus_tlv_t {
	uint_8 type;
	uint_16 len;
	// followed by data
	//mbus_sensor_packed_t mbus_sensor_packed;
} __PACKED mbus_tlv_t;

typedef struct mbus_alarm_t {
	uint_16 card_num;
	uint_8 value;
} __PACKED mbus_alarm_t;

typedef struct mbus_cardrssi_t {
	uint_16 card_num;
	uint_16 seq;
	int_8 rssi;
} __PACKED mbus_cardrssi_t;

typedef struct mbus_nopower_t {
	uint_16 card_num;
	uint_8 value;
} __PACKED mbus_nopower_t;

typedef struct mbus_card_t {
	uint_8 type;    // MBusRspType
	union {
		mbus_alarm_t alm;
		mbus_cardrssi_t cardrssi;
		mbus_nopower_t nopower;
	} __PACKED data;
} __PACKED mbus_card_t;

typedef int_32 mbus_time_t;

#ifdef WIN32
#pragma pack(pop)
#endif

#endif

