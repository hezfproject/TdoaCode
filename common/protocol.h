#ifndef _TRANS_INTF_H_
#define _TRANS_INTF_H_

//#include "../../../common/AppProtocol.h"
#include "CommonMacro.h"
#include "CommonTypes.h"
#include "protocol_types.h"
#include "SensorDefines.h"
#include "types.h"
#include "general_data_type_def.h"
#include "port_stat.h"
#include "cmd_type.h"
#include "MBusProto.h"

/* protocol between workstation transinterface and scheduler
format: herder + structure
header: struct CommHeader
{
int tag;
int len;
};
where tag is COMM_X_X,
structure is listed below.
*/

#ifdef __cplusplus
extern "C" {
#endif

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

		COMM_TPT_DATA = 150,    // transparent transmission
		COMM_SANY_DATA = 180,

		COMM_DOWN_CMD = 200,	// UAPP_DCMD_RETREAT
		COMM_DOWN_TIME,
		COMM_DOWN_CARDREADER_CMD,
		COMM_DOWN_UWBSET,

		//used between workstation and analysis
		COMM_DOWN_LICENSE = 250,
		COMM_DOWN_PHONE_LEVEL,
		COMM_DOWN_PHONE_OUTLINE,

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

		COMM_WAREHOUSE_TRANS_CMD = 400,
		COMM_WAREHOUSE_CMD_ACK,

		COMM_BLAST_REQUEST,     // UAPP_BLAST_REQUEST

		/* New type of cmd for transtation */
		COMM_TOF_ALARM = 600,
		COMM_TOF_ALARM_ACK,
		COMM_TOF_CHECKIN,
		COMM_TOF_RSSI,
		COMM_TOF_DISTANCE,
		COMM_TOF_LOCATOR,
		COMM_TOF_REPORT,
		COMM_LSRF_SET,
		COMM_LINK_STATUS,
		COMM_ENTIRE_LINK_STATUS,
		COMM_TOF_MORE_DISTANCE,
		COMM_TOF_GAS_ALARM,
		COMM_TOF_GAS_RSSI,
		COMM_TOF_GAS_DENSITY,
		COMM_TOF_DEV_RSSI,
		COMM_TOF_LOCATOR_DIS_SET,
		COMM_TOF_CARD_EVACUATE,
		COMM_TOF_NEW_DISTANCE,
		COMM_TOF_NEW_ALARM,
		COMM_TOF_VEHICLE_ALARM,
		COMM_TOF_VEHICLE_RSSI,
		COMM_TOF_VEHICLE_BATTERY,

		COMM_UWB_DISTANCE,
		COMM_UWB_ALARM,
		COMM_UWB_REPORT,
		COMM_UWB_CARD_EVACUATE,
		COMM_UWB_CARD_CYCLESET,

		COMM_LIGHT_LINK,
		COMM_LIGHT_SET,
		COMM_LIGHT_CHANNEL,
		COMM_LIGHT_RSSI_CHANNEL,
		COMM_LIGHT_VERSION,
		COMM_LIGHT_RSSI,

		COMM_UWB_RSSI,

		COMM_UWB_TDOA_TIMESTAMP=650,

		COMM_MP_CMD=700,
		COMM_MP_VOICE,
		COMM_MP_SMS,
		COMM_MP_SMS_ACK,
		COMM_MP_SWITCH,
		COMM_MP_LEAVE,
		COMM_SWITCH_PEER,

		COMM_TRANS_CMD = 800,
		COMM_ERROR_RATE_TEST,
		COMM_ERROR_RATE_TEST_CHANNEL,

		COMM_CELL_SWITCH = 900,
		COMM_CELL_SWITCH_PEER,
		COMM_CELL_CLEAR_PEER,

		COMM_ENTIRE_NET_GENERAL_FOR_WORKSTATION = COMM_ENTIRE_NET_GENERAL, // 1000
		COMM_ENTIRE_NET_SCHEDULAR_SMS_FOR_WORKSTATION = COMM_ENTIRE_NET_SCHEDULAR_SMS, // 2000
		COMM_ENTIRE_NET_SCHEDULAR_FORCE_CLOSE_FOR_WORKSTATION = COMM_ENTIRE_NET_SCHEDULAR_FORCE_CLOSE, // 2001

		//for sensor substation 3.0
		COMM_SENSOR_QUERY_LINK = 3000,
		COMM_SENSOR_QUERY_CONFIG,
		COMM_SENSOR_DOWN_CONFIG,
		COMM_SENSOR_UP_SITEID,

		//3g network data
		COMM_3G_DATA = 4000,
		COMM_TRANSFER_THROUGH,

        //multi substation
		COMM_PLUGIN_DOWN_CONFIG = 4050,
		COMM_CTRL_DOWN_CONFIG,
		COMM_SWITCH_STATUS_RPT,

		//use for mini-site, add in 2017/5/22
		COMM_STM32_MINISITE_POWER_RPT = 5555, //电量包Tag
		COMM_STM32_MODULE_LINK_RPT = 5556, //通断包Tag
		COMM_STM32_MODULE_TYPE_REQUEST_ACK = 5557, //模块类型请求应答包Tag, add in 2017/9/20
        COMM_STM32_MODULE_TYPE_DOWN = 5558, //模块类型下发包Tag

		//1806
		COMM_RADIO_DATA = 6000,//1806广播语音Tag
		COMM_INTEGRATED_STA_DATA = 10000, //1806安全监测数据Tag, add by liangxiao
	};

	enum
	{
		TERM_CMD_DIAL,
		TERM_CMD_FOUND,
		TERM_CMD_BUSY,
		TERM_CMD_ACCEPT,
		TERM_CMD_CLOSE,
		TERM_CMD_AUDIO,
		TERM_CMD_ACK,
	};
	enum PORT_NO
	{
		PORT_NO_0 = 0,
		PORT_NO_1,
		PORT_NO_2,
#ifdef VERSION_SECOND
		PORT_NO_3,
		PORT_NO_4,
		PORT_NO_5,
		PORT_NO_6,
		PORT_NO_7,
		PORT_NO_8,
		PORT_NO_9,
		PORT_NO_10,
		PORT_NO_11,
#endif
		PORT_NO_CNT,
	};

	enum COMM_SWITCH_TYPE
	{
		COMM_SWITCH_REQ = 0,
		COMM_SWITCH_ALLOW,
		COMM_SWITCH_DENY,
	};

    typedef struct { //version & battery
        char srcNbr[NMBRDIGIT];
		time_t timestamp;
        int battery_value;
        uint_8 macid[8];
        char version[64];
    } comm_vers_elec_t;

	typedef struct {
		char gateway_id[8];
		//int gateway_id_len;
	} comm_gateway_id;

	typedef struct {
		int is_comm_between_trans;
		int channel;
	} comm_error_rate_test_channel_t;

	typedef struct {
		int type;
		char srcnbr[NMBRDIGIT];
		char dstnbr[NMBRDIGIT];
		uint_16 src_pan;
	} comm_cmd_t;

	typedef struct {
		int type;
		uint_16 src_pan;
		uint_16 seqnum;
		char srcnbr[NMBRDIGIT];
		char dstnbr[NMBRDIGIT];
	} comm_cmd_new_t;

	typedef struct {
		uint_16 silencemap;
		uint_8 seqnum;
		uint_8 len;
		uint_16 src_pan;
		uint_16 reserved;
		char srcnbr[NMBRDIGIT];
		char dstnbr[NMBRDIGIT];
	} comm_voice_t;

	typedef struct {
		char nbr[NMBRDIGIT];
		char peer_nbr[NMBRDIGIT];
		uint_8 switch_type;
		uint_8 seqnum;
		pan_addr_t peer_pan_id; /*peer_pan_id is the other pan when voice, otherwise it's an invalid value*/
		pan_addr_t leave_pan; /*mp leave from this pan*/
		pan_addr_t join_pan; /*mp join to this pan*/
	} comm_switch_t;

	typedef struct {
		char peer_nbr[NMBRDIGIT];
		pan_addr_t peer_pan;
		pan_addr_t pan;
		uint_16 reserved;
	} comm_switch_peer_t;

	typedef struct {
		pan_addr_t dst_pan;
		char nbr[NMBRDIGIT];
		uint_16 reserved;
	} comm_leave_t;

	typedef struct { //blast
        time_t timestamp;
        uint_16 src_pan;
        uint_16 srcAddr;
        uint_16 dstAddr;
        uint_8  blastEnalbe;
        uint_8  blastCnt;
		int_8 rssi;
		int_8 padding1;
		uint_16 padding2;
	} comm_blast_t;

#define URGENT_NOPWR_REALNOPWR      0
#define URGENT_NOPWR_RECVRETREAT   0xFD
#define URGENT_NOPWR_KEYCONFIRM     0xFE
	typedef struct {
		char srcnbr[NMBRDIGIT];
		time_t timestamp;
		int val;
	} term_alert_t;

	typedef struct {
		char srcnbr[NMBRDIGIT];
		PanAddr_t srcpan;
		time_t timestamp;
		short rssi;
		unsigned seqnum;
	} term_rssi_t;

	// new format for rssi report, compatible with original
	typedef struct {
		char srcnbr[NMBRDIGIT];
		PanAddr_t srcpan;
		short rssi;
		time_t timestamp;
		unsigned seqnum;
	} term_rssi2_t;

	typedef struct {
		PanAddr_t neighbor_id;
		unsigned int neighbor_boud_rate;
	} port_neighbor_t;

	typedef struct {
		PanAddr_t srcpan;
		time_t timestamp;
#ifdef VERSION_SECOND
		port_neighbor_t port_neighbor[PORT_NO_CNT];
#else
		//////////////////////////////////////////////////////////////////////////
		PanAddr_t An;
		unsigned As;
		PanAddr_t Bn;
		unsigned Bs;
		PanAddr_t Cn;
		unsigned Cs;
		//////////////////////////////////////////////////////////////////////////
#endif
		char spi_master;
		char spi_slave;
	} pan_status_t;

	typedef struct {
		char srcnbr[NMBRDIGIT];
		char dstnbr[NMBRDIGIT];
		time_t timestamp;
		uint_16 blk;
		// followed by content
	} term_sms_t;

	typedef struct {
		char srcnbr[NMBRDIGIT]; // not used now
		char dstnbr[NMBRDIGIT];
		PanAddr_t dstpan;		// not used now
		uint_16 blk;
	} term_smsack_t;

	typedef struct {
		char srcnbr[NMBRDIGIT];
		char dstnbr[NMBRDIGIT];
		time_t timestamp;
		unsigned secs;
	} call_record_t;

	typedef struct {
		char srcnbr[NMBRDIGIT];
		char dstnbr[NMBRDIGIT];
		PanAddr_t srcpan;
		PanAddr_t dstpan;
		time_t start;
		char io;
		char status;
	} term_status_t;

    // ARM running state
    enum {
        ARM_NORMAL = 0,
        ARM_WARNING,
        ARM_ERROR,
    };

    // ARM running state structure
	typedef struct {
		PanAddr_t pan;
		int mode;
		char msg[256];
	} pan_running_state_t;

	// ARM restart structure
	typedef struct {
		PanAddr_t pan;
		int mode;
		char msg[256];
	} pan_restart_t;

	typedef struct {
		PanAddr_t pan;
		int mode;
	} zc_restart_t;

	typedef struct {
		char nbr1[NMBRDIGIT];
		char nbr2[NMBRDIGIT];
	} force_close_t;

	typedef struct {
		char type;
		short value;
	} TV_t;

	typedef struct {
		PanAddr_t devid;
		short rssi;
	} PanRssi_t;

	typedef struct {
		PanAddr_t devid;
		uint_16 seq;
		time_t timestamp;
		TV_t tv[3];
		int thres;
		PanRssi_t pr[8];
	} gas_report_t;

	typedef struct {
		PanAddr_t devid;
		PanAddr_t reppan;
		int type;
		time_t timestamp;
		int val;
	} gas_alarm_t;

	typedef struct
	{
		uint_16 srcAddr;
		time_t timestamp;
	} sany_t;

#define CMDRES_RFINS_BEGIN	1
#define CMDRES_RFINS_RES	2
#define CMDRES_RM_NRPATH	3
#define CMDRES_RM_LICENSE	4
#define CMDRES_REBOOT		5
#define CMDRES_SPI_ACK		6
#define CMDRES_RETREAT		7
#define CMDRES_RETREAT_CANCEL 8
#define CMDRES_SPI_ACK_DOWN   9  //下发撤离时，射频板的回应
#define CMDRES_SPI_ACK_CANCEL 10 //取消撤离时，射频板的回应

	typedef struct {
		PanAddr_t srcpan;
		time_t timestamp;
		int type;
		int res;
	} pan_cmdres_t;

	typedef struct {
		PanAddr_t srcpan;
		time_t timestamp;
		char version[64];
		int stat;
	} pan_version_t;

	typedef struct {
		PanAddr_t srcpan;
		time_t timestamp;
		char filename[32];
		unsigned char md5sum[16];
		int stat;
	} pan_filemd5_t;

	typedef struct {
		char srcnbr[NMBRDIGIT];
		char dstnbr[NMBRDIGIT];		// maybe panid or gasdev id
		time_t sendtime;
		int seqnum;
		unsigned len;
		// followed by message data
	} dev_sms_t;

	typedef struct {
		char srcnbr[NMBRDIGIT];
		char dstnbr[NMBRDIGIT];		// maybe panid or gasdev id
		time_t timestamp;
		int seqnum;
	} dev_smsack_t;

	typedef struct {
		PanAddr_t srcpan;
		unsigned leftdays;
		char mac[12];
	} licenserep_t;

	typedef struct {
		PanAddr_t dstpan;
		unsigned days;
		char mac[12];
	} licensecmd_t;

	// up_cmd type
	enum {
		REQ_LOCNODE,
	};

	typedef struct {
		int cmdtype;
		PanAddr_t srcpan;
	} upcmd_t;

	typedef struct {
		PanAddr_t dstPan;
		int locnum;
		// followed by locnode data
	} locnode_set_t;

	typedef struct {
		PanAddr_t devid;
		uint_16 seq;
		time_t timestamp;
		PanRssi_t pr[8];
	} locrssi_rep_t;

	typedef struct {
		PanAddr_t dstPan;
		uint_16 seq;
		uint_32 size;
		// followed by content
	} errrate_t;

	typedef struct {
		PanAddr_t panid;
		uint_8 seq;
	} version_req_t;

	typedef struct {
		PanAddr_t panid;
		time_t timestamp;
		uint_8 seq;
		uint_16 size;
		// followed by version
	} version_rsp_t;

	typedef struct {
		PanAddr_t pan;
	} route_qry_t;

	typedef struct {
		PanAddr_t pan;
		char index;
		uint_32 depth;
	} route_uart_t;

	typedef struct {
		PanAddr_t pan;
		PanAddr_t index_pan;
		uint_32 depth;
	} route_wls_t;

	typedef struct {
		PanAddr_t pan;
		uint_16 uart_cnt;
		uint_16 wls_cnt;
		uint_16 size;
		time_t timestamp;
		// followed by route_uart_t route_wls_t
	} route_rsp_t;

	/* ------------- DownCmd structure ------------------*/

	typedef struct {
		int cmd;
		PanAddr_t dstpan;
	} pan_cmd_t;

	typedef struct {
		time_t timestamp;
	} down_time_t;

	typedef struct {
		unsigned char retreat[256]; //cardreader retreat cmd
	} cardreader_cmd_t;

	/*
	* The following msg struct definition is for transtation.
	*/

	typedef struct {
		time_t		tm;
		pan_addr_t	panid;
		pan_addr_t	jn_id;
		uint_16		short_addr;
		uint_16		status;
	} comm_tof_alarm_t;

	typedef struct {
		time_t		tm;
		pan_addr_t	panid;
		pan_addr_t	moduleid;
		uint_16		u16ShortAddr;
		uint_8		u8Status;
		uint_8      u8ExciterID;
	} comm_tof_new_alarm_t;

	typedef struct {
		time_t		tm;
		pan_addr_t	panid;
		pan_addr_t	jn_id;
		uint_16		short_addr;
		uint_16		status;
	} comm_tof_checkin_t;

	typedef struct {
		pan_addr_t	panid;
		pan_addr_t	jn_id;
		uint_16		short_addr;
		uint_16		ack_type;
	} comm_tof_alarm_ack_t;

	typedef struct {
		time_t		tm;
		pan_addr_t	panid;
		pan_addr_t	jn_id;
		uint_32		len;
		// followed by tof_rssi_ts[n]
	} comm_tof_rssi_t;

	typedef struct {
		time_t		tm;
		pan_addr_t	panid;
		pan_addr_t	moduleid;
		uint_16 u16vehiche;
		uint_16 u16seqnum;
		uint_16 reseverd;
		uint_16		len;
		// followed by vehicle_rssi_ts[n]
	} comm_vehicle_rssi_t;

	typedef struct {
		time_t		tm;
		pan_addr_t	pan_id;
		pan_addr_t	module_id;
		uint_32		len;
		// followed by tof_distance_ts[n]
	} comm_tof_distance_t;

	typedef struct {
		time_t		tm;
		pan_addr_t	panid;
		pan_addr_t	jn_id;
		uint_16		short_addr;
		uint_8		dist;
		uint_8		status;
	} comm_tof_locator_t;

	typedef struct {
		uint_8 reporttype;
		uint_8 seqnum;
		pan_addr_t module_id;
		pan_addr_t pan_id;
		uint_8 rssich;
		uint_8 len;
	} comm_tof_report_t;

	typedef struct {
		uint_8 reporttype;
		uint_8 devType;
		uint_8 seqnum;
		pan_addr_t module_id;
		pan_addr_t pan_id;
		uint_8 rssich;
		uint_8 len;
	} comm_uwb_report_t;

	typedef struct {
		uint_8 tof_channel;
		uint_8 station_type;
		uint_8 loc_distance[2];
		uint_32 loc_id[2];
		uint_8 loc_channel;
		uint_8 comm_channel;
		uint_16 reserved;
		time_t tm;
		uint_16 oad_version;
		uint_16 len;
		//add version
	} comm_tof_loc_status_t;

	typedef struct {
		uint_16 oad_verison;
		uint_16 len;
		//add version
	}comm_com_status_t;


	typedef struct {
		uint_16 	   devid;
		uint_8         devtype;
		uint_8         rssich;
		uint_16        oad_ver;
		uint_16 	   battery;   // uint: 0.1v
	}  comm_loc_version_report_t;
	typedef struct 
    {
        uint_16 devid;
        uint_16 oad_ver;
        uint_16 battery;   // uint: 0.1v
        uint_16 Reserved;
    }  comm_UWBVersionReport_t;
	typedef struct {
		time_t tm;
		pan_addr_t panid;
		pan_addr_t jn_id;
		uint_16 u16Reserved;
		uint_16 u16ShortAddr;
		uint_16 u16SeqNum;
		uint_16 u16Distance;
		uint_16 u16LocDistance[2];
		uint_16 u16GasDensity;
		uint_16 u16GasThr;
		uint_8  u8GasType;     // 0x01: gas
		int_8   i8Rssi;
		uint_8  u8Direction;
		uint_8  u8Reserved1;
	} comm_tof_gas_density_t;

	typedef struct {
		time_t tm;
		pan_addr_t panid;
		pan_addr_t jn_id;
		uint_32 len;
		//followed by tof_gas_rssi_ts[n]
	} comm_tof_gas_rssi_t;

	enum
	{
		DEV_RSSI_TYPE_LOC_SITE,
		DEV_RSSI_TYPE_LOCATOR,
		DEV_RSSI_TYPE_COMM
	};
	typedef struct
	{
		pan_addr_t panid;
		uint_16 rssiType;
		time_t	tm;
		uint_16	u16ShortAddr;
		int_8	sentRssi;
		int_8   receiveRssi;
	} comm_tof_dev_rssi_t;

	// link message for 2nd generation station
	typedef struct {
		pan_addr_t src;
		unsigned short reserved;
		time_t timestamp;
		portstat_t ps[12];
	} comm_linkstat_t;

	typedef struct {
		pan_addr_t src;
		unsigned short dev_type;
		unsigned short len;
		unsigned short reserved;
		time_t timestamp;
	} comm_entire_linkstat_t;

	// DOWN CMD
	typedef struct {
		int cmd;
		pan_addr_t main_id;
		pan_addr_t module_id;
	} comm_transcmd_t;

	//CO
	typedef struct {
		pan_addr_t src_pan;
		uint_16 dev_id;
		time_t timestamp;
		MIC2000_data_t sensor_data;
	} comm_sensor_rpt_t;

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

	//multi substation
	typedef struct{
		pan_addr_t src_pan;
		uint_16 dev_id;
		time_t timestamp;
        int datalen;
        //follow comm_plugin_item_t array
	}comm_plugin_config_t;

	typedef struct{
		char    siteip[16];
		pan_addr_t srcpan;
		pan_addr_t devid;
		time_t timestamp;
		int datalen;
		//follow comm_plugin_item_t array
	}comm_composite_config_t;

    typedef struct {
        uint_16 port;
        uint_16 open;
        char    name[16];
        char    ip[16];
    }comm_plugin_item_t;

    typedef struct {
		pan_addr_t src_pan;
		uint_16 dev_id;
		time_t timestamp;
        uint_8 port;
		uint_8 status;
		uint_16 reserved;
    }comm_ctrl_config_t;

	typedef struct {
		pan_addr_t src_pan;
		uint_16 dev_id;
		time_t timestamp;
		uint_16 status;
		char macaddr[18];
	}comm_switch_rpt_t;


	//define the msg over workstation
#define CELL_SWITCH_REQ 0
#define CELL_SWITCH_RSP 1

	typedef struct
	{
		PanAddr_t dst_pan;
		uint_16 msg_type;
		uint_32 len;
		//follow by switch msg data;
	}comm_msg_switch_t;

	//cell switch peer
	typedef struct{
		PanAddr_t src_pan;
		PanAddr_t peer_pan_id;
		//termNbr_t src_nbr;
		char peer_nbr[NMBRDIGIT];
	}comm_cell_switch_peer_t;

#define GROUP_STATE_IDLE       0
#define GROUP_STATE_START      1
#define GROUP_NUMBER_LENGTH 4
	typedef struct
	{
		char priority;
		char grouptype;
		short total;
		char groupnumber[GROUP_NUMBER_LENGTH];
	}comm_group_t;

	typedef struct
	{
		int msgtype;
		int cseqnum; //align 4
		comm_group_t group;
		time_t timestamp;
	}comm_group_msg_t;
	typedef struct
	{
		int msgtype;
		//time_t timestamp;
		short total;
		short data; //seq /req item times
		char groupnumber[GROUP_NUMBER_LENGTH];
		char dstnumber[GROUP_NUMBER_LENGTH];
		char datetime[8];
	} comm_group_msg_head_t;
	//cell clear peer
	typedef struct{
		PanAddr_t dst_pan;
		char clear_nbr[NMBRDIGIT];
	}comm_cell_clear_peer_t;
	//log info
	typedef struct
	{
		time_t timestamp;
		uint_16 log_type;
		pan_addr_t src_pan;
	} comm_retreatlog_t;


	typedef struct
	{
		time_t timestamp;
		pan_addr_t srcpan;
		short padding;
		char machineid[16];
		int len;
		//data followed
	} comm_tptdata_t;

	typedef struct
	{
		std::string name;
		int value;
	} nvp_t;

	//add protocols between workstation and app
	typedef struct
	{
		char licence[128];
	} licence_t;

	typedef struct
	{
		char nbr[NMBRDIGIT];
		int level;
	} phone_role_t;

	typedef struct
	{
		char nbr[NMBRDIGIT];
	} phone_outline_t;

	typedef struct
	{
		pan_addr_t pan_num;
		pan_addr_t site_num;
		uint_8 loc_channel;
		uint_8 tof_channel;
		uint_8 comm_channel;
		uint_8 reserved;
	} set_channel_t;

	struct set_locator_distance
	{
		pan_addr_t siteNum;
		pan_addr_t locatorNum;
		uint_8 distance;
		uint_8 reserved;
	};

	struct card_evacuate_cmd
	{
		uint_16 reserved;
		uint_16 length;
		// follow pan_addr_t list
	};

	struct Light_set_cmd
	{
		uint_16 reserved;
		uint_16 length;
		// follow linghtId list
	};

	struct card_vehicle_alram_cmd
	{
		uint_16 reserved;
		uint_16 length;
		// follow pan_addr_t list
	};

	struct card_cycelSet_cmd
	{
		uint_16 reserved;
		uint_16 length;
		// follow cardNum list
	};
	enum
	{
		APP_GROUP_CMD_CALL,//呼叫
		APP_GROUP_CMD_START,//开始通话
		APP_GROUP_CMD_CLOSE,//结束
		APP_GROUP_CMD_NEW_GROUP,//新建组
		APP_GROUP_CMD_FORCE_CLOSE,//强拆
		APP_GROUP_CMD_INSERT,//强插
		APP_GROUP_CMD_FAIL,//失败
		APP_GROUP_CMD_TEMP_GROUP,//新建临时组
		APP_GROUP_CMD_REQ_GROUP_ITEM,//请求数据
		APP_GROUP_CMD_ACK,//应答
		APP_GROUP_CMD_SINGLE_CALL,//单拨
		APP_GROUP_CMD_DELETE,
		APP_GROUP_CMD_ACK_GROUP_ITEM,//收取完整数据
		APP_GROUP_CMD_CHECK_GROUP_ITEM//检查收取完整数据
	};

	typedef struct {
		int msgtype; //呼叫类型
		int iMsgLength;//消息长度
		int groupnumber;//组号
		int grouptype; //组类型 0所有部门 , 1 部门 1 班组 ,2 临时会议3强插
		int total;
		int seqnum;
		char datetime[8];
	}GroupMsgHead_t;

	typedef struct {
		int itemnumber1;//主播
		int itemnumber2;// 插号码
		int itemnumber3;//通话对方号码
	}GroupMsgBody_t;

	typedef struct
	{
		time_t timestamp;
		pan_addr_t panid;
		uint_16 len;
	} Comm_CardsTransHdr_t;

	//data type
	enum
	{
		CARD_STATUS_DATA      = 0x00,
		CARD_DESC_DATA        = 0x01,
		CARD_ROUTE_PATH       = 0x02,

		CARD_SEARCH_CMD       = 0x10,

		CARD_TUNNELLING_DATA1 = 0xF0,
		CARD_TUNNELLING_DATA2 = 0xF1,
	};
	typedef struct
	{
		pan_addr_t srcAddr;
		pan_addr_t dstAddr;
		uint_8 seqnum;
		uint_8 len;
		//follow  data  by dataType
	} CardsDataHdr_t;

	typedef struct
	{
		uint_8 prototype;
		uint_8 msgtype;
		uint_16 panid;
		time_t  occtime;
		uint_16 devnum;
		uint_16 len;
	} comm_transfer_through_t;

	typedef struct
	{
		uint_16 panid;
		uint_16 moduleid;
		time_t  occtime;
		uint_16 vehicleaddr;  
		uint_8  battery;
		uint_8  len;
	} comm_vehicle_battery_t;

	//use for mini-site, add in 2017/5/22
	typedef struct
	{
		uint_16 minisiteid; //小型化综合分站ID
		uint_16 reserved; //预留
	} stm32_MinisiteId_t;

	typedef struct
	{
		uint_16 voltage1; //1路电压, 单位0.1V
		uint_16 voltage2;
		uint_16 voltage3;
		uint_16 current1; //1路电流, 单位mA
		uint_16 current2;
		uint_16 current3;
	} stm32_MinisitePower_t; 

	typedef struct
	{
		uint_8 port; //端口号
		uint_8 state; //连接状态
	    uint_16 reserved; //预留
		uint_32 ip; //端口IP地址
	} stm32_ModuleLink_t; //8 Bytes

	typedef struct
    {
        byte type1;
        byte type2;
        byte type3;
        byte type4;
    }stm32_ModuleType_t;

#ifdef __cplusplus
};
#endif

#endif
