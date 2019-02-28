#ifndef _3G_PROTOCOL_H_
#define _3G_PROTOCOL_H_

#include "mbusproto.h"
#include "app_protocol.h"

typedef uint_16 pan_addr_t;

enum
{
    COMM_TERM_CMD,
    COMM_CALL_RECORD,		// UAPP_COMM_LOG
    COMM_ALERT_POWER,		// UAPP_UCMD_NOPOWER
    COMM_ALERT_URGENT,		// UAPP_UCMD_ALERT
    COMM_REPORT_RSSI,		// UAPP_RSSI_INFO
    COMM_PAN_STATUS,		// UAPP_UART_LINK_STATUS
    COMM_TERM_SMS,			// UAPP_TERM_SMS
    COMM_SMS_ACK,			// UAPP_SMS_RSP
    COMM_TERM_STATUS,		// UAPP_PHONE_STATUS
    COMM_PAN_RESTART,		// UAPP_RESTART_INFO
    COMM_ZC_RESTART,		// UAPP_ZC_RESTART_INFO
    COMM_GAS_REPORT,        // UAPP_GAS_REPORT
    COMM_GAS_ALARM,         // UAPP_GAS_ALARM
    COMM_CMD_RES,			// UAPP_OPERATE_CMD
    COMM_SBS_VERSION,		// UAPP_FILE_VERSION
    COMM_FILE_MD5,			// UAPP_FILEUP_INFO
    COMM_GAS_SMS,
    COMM_GAS_SMS_ACK,
    COMM_LICENSE_REP,
    COMM_LICENSE_CMD,
    COMM_UP_CMD,
    COMM_LOCNODE_SET,		// UAPP_LOCNODE_SET
    COMM_LOCRSSI_REPORT,	// UAPP_LOCRSSI_REPORT, deprecated
    COMM_SENSOR_RPT,	    //UAPP_CO_REPORT
    COMM_CARD_RPT,
    COMM_REPORT_RSSI2,		// new protocol to report rssi
    COMM_RETREAT_LOG,
    COMM_NEW_SENSOR_RPT,
    COMM_VERS_ELEC,         // UAPP_UCMD_VERS_ELEC
    COMM_CARD_LINK_RPT,     // UAPP_CARD_LINK_RPT
    COMM_SENSOR_LINK_RPT,   //ARM_MBUS_SENSOR_LINK_RPT
    COMM_PAN_RUNNING_STATE,

    COMM_DOWN_CMD = 200,	// UAPP_DCMD_RETREAT
    COMM_DOWN_TIME,

    COMM_FORCE_CLOSE = 300, // used by SC
    COMM_GROUP_SC_CMD,
    COMM_GROUP_PAN_CMD,
    COMM_GROUP_WS_CMD,
    COMM_GROUP_GW_CMD,
    COMM_ERRRATE,
    COMM_VERSION_REQ,
    COMM_VERSION_RSP,
    COMM_GATEWAY_ID_REP,
    COMM_ROUTE_QRY,
    COMM_ROUTE_RSP,

    COMM_TRANS_CMD = 800,
    COMM_ERROR_RATE_TEST,
    COMM_ERROR_RATE_TEST_CHANNEL,

	//for sensor substation 3.0
	COMM_SENSOR_QUERY_LINK = 3000,
	COMM_SENSOR_QUERY_CONFIG,
	COMM_SENSOR_DOWN_CONFIG,
	COMM_SENSOR_UP_SITEID,

	COMM_POWER_SENSOR_RPT,
};

//card
typedef struct {
	pan_addr_t src_pan;
	uint_16 dev_id;
	time_t timestamp;
	uint_32 datalen;
	//followed by data;
	//mbus_card_t card_data;
} comm_card_rpt_t;

//card link
typedef struct {
    pan_addr_t src_pan;
    time_t timestamp;
    uint_16 datelen;
    //follow uint_16 array
} comm_card_link_rpt_t;

//new sensor
typedef struct{
	pan_addr_t src_pan;
	uint_16 dev_id;
	time_t timestamp;
	mbus_sensor_packed_t new_sensor_data;
}comm_sensor_data_rpt_t;
typedef struct power_information_t
{
	uint_8 ATEXT;
	uint_8 BTEXT;
	uint_16 ITEXT;
	uint_16 VTEXT;
	// followed by data
	
}power_informati_t_Temp;

// sensor v3.0
typedef struct {
    pan_addr_t src_pan;
    uint_16 channel;
    uint_16 dev_id;     // the sensor substation id
	time_t timestamp;
    uint_16 datalen;
    // followed by int_8 array config data
}comm_sensor_config_t;

//sensor link report
typedef struct{
	time_t timestamp;
	int  channel;
	pan_addr_t src_pan;
	uint_16 count;
	//follow uint_16 array
}comm_sensor_link_rpt_t;

// ARM restart structure
typedef struct {
	pan_addr_t pan;
	int mode;
	char msg[256];
} pan_restart_t;

// ARM running state
enum {
    ARM_NORMAL = 0,
    ARM_WARNING,
    ARM_ERROR,
};

// ARM running state structure
typedef struct {
	pan_addr_t pan;
	int mode;
	char msg[256];
} pan_running_state_t;

typedef struct {
	pan_addr_t panid;
	uint_8 seq;
} version_req_t;

typedef struct {
	pan_addr_t panid;
	time_t timestamp;
	uint_8 seq;
	uint_16 size;
	// followed by version
} version_rsp_t;

#define ERROR_RATE_TEST_DATA_LEN 88

/** msg type **/
enum
{
	/* Mobile msg between arm */
	ARM_MP_JOIN_NOTIFY = 0x01,
	ARM_MP_LEAVE_NOTIFY,
	ARM_MP_POLL,
	ARM_MP_VOICE,
	ARM_MP_SMS,
	ARM_MP_ARMID,
	ARM_MP_CELLSWITCH,
	ARM_MP_CMD_UP,
	ARM_MP_CMD_DOWN,
	ARM_MP_FORCE_CLOSE,
	ARM_MP_GROUP_CMD,

	ARM_MPRF_REPORT,
	ARM_MP_STATUS,
	ARM_MP_CALL_RECORD,
	ARM_MP_GATEWAY_ID,

	/*added by wanghaiyang*/
	ARM_MP_SWITCH_PEER,
	ARM_MP_PAN_MAP_INFO,
	ARM_MP_ERROR_RATE_CHECK,

	/* Card msg between arm */
	ARM_TOF_ALARM = 0x40,
	ARM_TOF_ALARM_ACK,
	ARM_TOF_CHECKIN,
	ARM_TOF_RSSI,
	ARM_TOF_DISTANCE,
	ARM_TOF_LOCATOR,
	ARM_TOF_REPORT,
	APP_TOF_SET,
	ARM_MODULE_RESTART,
	ARM_TOF_MORE_DISTANCE,
	ARM_TOF_GAS_RSSI,
	ARM_TOF_GAS_DENSITY,
	ARM_TOF_GAS_ALARM,
	ARM_TOF_GAS_ALARM_ACK,



	/* download msg between arm */
	ARM_FILEUP_INFO = 0x60,
	ARM_FILEUP_PACK_REQ,
	ARM_FILEDOWN_INFO,
	ARM_FILEDOWN_PACK,

	ARM_RESTART,
	ARM_VERSION,
	ARM_LINK_STATUS,
	ARM_LINK_NEW_STATUS,

	ARM_TRANS_CMD,
	ARM_SYNC_TIME,
	ARM_TRANS_CMD_RSP,
	ARM_VERSION_REQ,
	ARM_VERSION_RSP,
	ARM_FILE_MD5,
	ARM_TRANS_CARDREADER_CMD,

	/*license msg */
	ARM_LICENSE_UPDATE = 0X80,
	ARM_LICENSE_REPORT,
	ARM_LICENSE_CMD,
	TRANS_ERROR_RATE_TEST,
	ARM_ERRRATE,
	ARM_ERRRATE_RSP,

	ARM_SANY_DATA = 0X90,

	ARM_MBUS_CARD_RPT = 0Xa0,
	ARM_MBUS_SENSOR_RPT,
	ARM_MBUS_SENSOR_DATA_RPT,
	ARM_MBUS_SENSOR_LINK_RPT,
	ARM_GROUP_CMD,

	//new rssi msg
	ARM_TOF_RSSI_TYPE_TOF_STATION,
	ARM_TOF_RSSI_TYPE_MPSTATION,

    // for sensor v3.0
    ARM_MBUS_QUERY_SENSOR_LINK = 0Xb0,
    ARM_MBUS_QUERY_SENSOR_CONFIG,
    ARM_MBUS_SET_SENSOR_CONFIG,

    ARM_MBUS_CARD_READER_LINK_RPT = 0xc0,
};

typedef struct
{
	app_header_t hdr;
	unsigned char retreat[256]; //cardreader retreat cmd
} arm_trans_cardreader_cmd_t;

typedef struct {
	app_header_t	hdr;
	pan_addr_t		panid;
	pan_addr_t		jn_id;
	uint_16			short_addr;
	uint_16			ack_type;
} app_armTofAlarmAck_t;

#endif /* _3G_PROTOCOL_H_ */

