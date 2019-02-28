/* To define protocol between transtation,
 * it has the common header defined in SOFTWARE/commin/minapp/app_protocol.h
 */

#ifndef _TRANS_PROTOCOL_H_
#define _TRANS_PROTOCOL_H_

#include "protocol_types.h"
#include "app_protocol.h"
#include "CommonMacro.h"
#include "CommonTypes.h"
#include "port_stat.h"
#include "app_group_msg.h"

#include <time.h>

#define FILE_NAME_SIZE	32
#define FILE_MD5_SIZE	16
#define FILE_PACK_SIZE	256
#define FILE_REQ_NUM	64

#define MAX_PROTOCOL_DATA_LEN 256

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
	ARM_TOF_NEW_DISTANCE,
	ARM_TOF_NEW_ALARM,  

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
/** msg type schedul workstation **/
enum
{
	/* schedul msg between ws */
	SC_WS_GROUP_CMD = 0x01,// sc to ws
};
enum
{
	/* gateway msg between ws */
	GW_WS_GROUP_CMD = 0x01,// gw to ws
	GW_WS_PHONE_LEVEL_CMD = 0X02, //outline phone level
};

//手机拨打权限控制
enum
{
	PHONE_LEVEL_CLEAR = 0x01,
	PHONE_LEVEL_CHANGE = 0x02,
};

//MBus protocol
typedef struct
{
	time_t timestamp;
	uint_16 devid;
	uint_16 len;
	// followed by mbus_data;
} app_mbus_rpt_t;

typedef struct
{
	time_t timestamp;
	uint_16 channel;
	uint_16 len;
	// followed by mbus_data;
} app_mbus_sensor_link_rpt_t;

typedef struct
{
	time_t timestamp;
	uint_16 channel;
	uint_16 len;
	// followed by mbus_data;
} app_mbus_card_reader_link_rpt_t;

typedef struct
{
    app_header_t hdr;
	pan_addr_t pan_id;
    uint_16 channel;
    pan_addr_t devid;
    time_t timestamp;
    uint_16 len;
    // followed by modbus sensor config;
} app_mbus_sensor_config_t;
//MBus protocol

typedef struct {
	char srcnbr[NMBRDIGIT];
	char dstnbr[NMBRDIGIT];
	pan_addr_t srcpan;
	pan_addr_t dstpan;
	time_t start;
	char io;
	char status;
} app_mp_status_t;

typedef struct {
	char srcnbr[NMBRDIGIT];
	char dstnbr[NMBRDIGIT];
	time_t timestamp;
	unsigned secs;
} app_call_record_t;

typedef struct {
	char gateway_id[8];
	//int gateway_id_len;
} app_gateway_id_t;

typedef struct {
	char nbr[NMBRDIGIT];
} app_termNbr_str_t;

typedef struct {
	app_header_t hdr;
	char	filename[FILE_NAME_SIZE];
	uint_8	md5[FILE_MD5_SIZE];
	uint_32	stat;
	uint_32 recnum;
} arm_fileup_info_t;

typedef struct {
	app_header_t hdr;
	char	filename[FILE_NAME_SIZE];
	uint_8	md5[FILE_MD5_SIZE];
	uint_32	packnum;
	uint_32	lastpacklen;
} arm_filedown_info_t;

typedef struct {
	app_header_t hdr;
	char	filename[FILE_NAME_SIZE];
	int		seqnum[FILE_REQ_NUM];
} arm_packreq_t;

typedef struct
{
	app_header_t hdr;
	char	filename[FILE_NAME_SIZE];
	int		seqnum;
	char	data[FILE_PACK_SIZE];
} arm_packinfo_t;

typedef struct {
	app_header_t hdr;
	int restart_type;
	char msg[256];
} app_armRestart_t;

typedef struct {
	app_header_t hdr;
	int restart_type;
	pan_addr_t module_pan;
}app_moudleRestart_t;

typedef struct {
	app_header_t hdr;
	char version[128];
} app_armVersion_t;

typedef struct {
	app_header_t hdr;
	pan_addr_t pan_id;
	uint_8 seq;
}arm_version_req_t;

typedef struct {
	app_header_t hdr;
	pan_addr_t pan_id;
	time_t timestamp;
	uint_8 seq;
	uint_16 size;
} arm_version_rsp_t;

typedef struct {
	app_header_t	hdr;
	time_t			tm;
	pan_addr_t		jn_id;
	uint_16			short_addr;
	uint_16			status;
	uint_16			reserved;
} app_armTofAlarm_t;

typedef struct {
	app_header_t	hdr;
	time_t			tm;
	pan_addr_t		panid;
	pan_addr_t		jn_id; /* id of the module */
	uint_16			short_addr;
	uint_16			status;
	uint_16			reserved;
} app_armTofCheckin_t;

typedef struct {
	app_header_t	hdr;
	pan_addr_t		panid;
	pan_addr_t		jn_id;
	uint_16			short_addr;
	uint_16			ack_type;
} app_armTofAlarmAck_t;

typedef struct {
	app_header_t	hdr;
	time_t			tm;
	pan_addr_t		jn_id;
	uint_16			reserved;
	// followed by tof_rssi_ts array
} app_armTofRssi_t;

typedef struct {
	app_header_t	hdr;
	time_t			tm;
	pan_addr_t		jn_id;
	uint_16			reserved;
	// followed by tof_gas_rssi_ts array
} app_armTofGasRssi_t;

typedef struct
{
	app_header_t hdr;
	time_t tm;
	pan_addr_t jn_id;
	uint_16 u16ShortAddr;
	uint_16 u16SeqNum;
	uint_16 u16Distance;
	uint_16 u16LocDistance[2];
	uint_16 u16GasDensity;
	uint_16 u16GasThr;
	uint_8	u8GasType;	   // 0x01: gas
	int_8	i8Rssi;
	uint_8	u8Direction;
	uint_8	u8Reserved1;
} app_armTofGasDensity_t;

typedef struct {
	app_header_t	hdr;
	time_t			tm;
	pan_addr_t		module_id; /* module id */
	pan_addr_t		pan_id; /* id in which the module is */
	// followed by tof_distance_tf array
} app_armTofDist_t;

typedef struct {
	app_header_t	hdr;
	time_t			tm;
	pan_addr_t		jn_id;
	uint_16			short_addr;
	uint_8			dist;
	uint_8			status;
} app_armTofLocator_t;


//new rssi data report
typedef struct {
	app_header_t	hdr;
	time_t			tm;
	uint_16			u16ShortAddr;
	int_8			sentRssi;
	int_8           receiveRssi;
} app_dev_rssi_t;

/* transtation maintain message */

typedef struct
{
	app_header_t	hdr;
	time_t			tm;
	portstat_t		ps[PORTS_NUM];
} arm_linkstat_t;

typedef struct
{
	app_header_t	hdr;
	time_t			tm;
	pan_addr_t		src;
	uint_8			dev_type;
	uint_8			len;
} arm_linknewstat_t;



enum {
	ARM_CMD_RETREAT,
	ARM_CMD_CANCELRETREAT,
};

typedef struct
{
	app_header_t hdr;
	unsigned short cmd;
	pan_addr_t module_id;
} arm_transcmd_t;

typedef struct {
	app_header_t hdr;
	time_t timestamp;
	pan_addr_t srcpan;
	uint_8 type;
	uint_8 res;
} arm_transcmdres_t;

typedef struct
{
	app_header_t hdr;
	time_t tm;
} arm_synctime_t;

typedef struct {
	app_header_t hdr;
	pan_addr_t srcpan;
	time_t timestamp;
	char filename[32];
	unsigned char md5sum[16];
	int stat;
} arm_transfilemd5_t;

typedef struct
{
	app_header_t hdr;
	unsigned char retreat[256]; //cardreader retreat cmd
} arm_trans_cardreader_cmd_t;

typedef struct {
	app_header_t hdr;
	pan_addr_t dstPan;
	uint_16 seq;
	uint_32 size;
	// followed by content
} arm_transerrrate_t;

typedef struct {
	app_termNbr_str_t peer_nbr;
	pan_addr_t pan;
	pan_addr_t peer_pan;
} app_arm_switch_peer_t;

typedef struct {
	app_termNbr_str_t srcnbr;
	app_termNbr_str_t dstnbr;
	pan_addr_t src_pan;
	uint_16 reserved;
	uint_16 silencemap;
	uint_8 seqnum;
	uint_8 len;
} app_armVoice_t;

typedef struct {
	app_termNbr_str_t srcnbr;
	app_termNbr_str_t dstnbr;
	time_t timestamp;
	uint_16 seqnum;
	uint_8 smstype;
	uint_8 len;
} app_armSMS_t;

typedef struct {
	app_termNbr_str_t srcnbr;
	app_termNbr_str_t dstnbr;
	pan_addr_t src_pan;
	uint_16 reserved;
	uint_16 seqnum;
	uint_8 cmdtype;
	uint_8 reserved_2;
} app_arm_cmd_t;

typedef struct {
	app_header_t hdr;
	unsigned int days;
	char mac[12];
} app_armLicenseUpdate_t;

typedef struct {
	app_header_t hdr;
	unsigned int leftdays;
	char mac[12];
} app_armLicenseReport_t;

/* value for switch_type */
#define APP_ARM_SWITCH_REQ 0
#define APP_ARM_SWITCH_ALLOW 1
#define APP_ARM_SWITCH_DENY 2

typedef struct {
	app_termNbr_str_t nbr;
	uint_8 switch_type;
	uint_8 seqnum;
	pan_addr_t peer_pan_id; /*peer_pan_id is the other pan when voice, otherwise it's an invalid value*/
	app_termNbr_str_t peer_nbr;
	pan_addr_t leave_pan; /*mp leave from this pan*/
	pan_addr_t join_pan; /*mp join to this pan*/
	int mp_status; /*the init status for this mp*/
	int io;
	time_t start_voice_time;
} app_arm_switch_t;

typedef struct {
	app_termNbr_str_t nbr;
	pan_addr_t dst_pan;
} app_arm_leave_t;

typedef struct {
	app_header_t hdr;
	int cmd;
} app_armLicenseCmd_t;

typedef struct {
	int cmd;
	app_termNbr_str_t srcnbr;
	int level;
} app_armphoneLevel_t;

typedef struct {
	uint_16 nbr;
	pan_addr_t pan_id;
	int is_add; /*1 represents join, and 0 represents leave*/
} app_mp_pan_map_info_t;

inline void	setup_app_header(app_header_t *hdr, uint_8 protocoltype, uint_8 msgtype, uint_16 len)
{
	hdr->protocoltype = protocoltype;
	hdr->msgtype = msgtype;
	hdr->len = len;
}

#endif


