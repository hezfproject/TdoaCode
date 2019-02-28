/****************************************************************************
Filename:       app_protocol.h
Revised:        $Date: 2011/08/26 00:59:21 $
Revision:       $Revision: 1.66 $
Description:
*****************************************************************************/
#ifndef __APP_PROTOCOL_H__
#define __APP_PROTOCOL_H__

#include "../CommonTypes.h"
#include "bsmac_header.h"
#include "../nwk_protocol.h"
#include "app_protocol.h"

#ifdef __cplusplus
extern "C" {
#endif

#define ERROR_RATE_TEST_DATA_LEN 88
    /*********************************************************************
    * DEFINES
    */

    /* max lens, including app header and payload */
#define APP_MOBILE_MAX_DATA_SIZE     102
#define APP_SMS_MAX_LEN               (APP_MOBILE_MAX_DATA_SIZE - sizeof(app_mpSMS_t) -sizeof(app_header_t))

    /* number string length */
#define APP_NMBRDIGIT    8

    /* mobile and card do not know the addrs of ARM, use 0 instead */
#define APP_ARMSHORTADDR     0x00
#define APP_ARMTERMADDR     "0"

#define APP_INVALIDARMADDR 0xFFFF

    /* TOF */
#define INVALID_TOF_DISTANCE 32766
#define FAILED_TOF_DISTANCE  32765    //TOF fail
#define VALID_TOF_DISTANCE  32764    //TOF no valid sequences
#define TOF_NOT_STARTED_DISTANCE 32763   //TOF not start
#define TOF_TX_ERROR_DISTANCE    32762   //TOF TX error
#define TOF_RX_ERROR_DISTANCE    32761    //TOF RX error
#define TOF_TIMEOUT_DISTANCE     32760    //TOF timeout
#define INVALID_LOCATOR_TOF_DISTANCE 32750  // init distance of locator and card
#define MIN_INVALID_TOF_DISTANCE 30000      //minimum invalid distance

#define INVALID_RSSI -127

#define APP_TOF_VERSION_MAX_NUM  10

    /************************************************************************/
    /* commom app protocol                                                   */
    /************************************************************************/
    /* number types */
    typedef enum
    {
        APP_TERMNBRTYPE_NONE,              // when do not know the type

        APP_TERMNBRTYPE_MOBILE,
        APP_TERMNBRTYPE_GATEWAY,
        APP_TERMNBRTYPE_GROUP,
        APP_TERMNBRTYPE_OUTSIDE,
    }
    app_nbr_type_t;

    /* termNmbr, only for number display*/
    typedef struct
    {
        char nbr[APP_NMBRDIGIT];
    }  app_termNbr_t;



    /* protocal type */
    /*最低两位置1，使sniffer不按Zigbee解析，方便调试*/
    typedef enum
    {
        APP_PROTOCOL_TYPE_CARD=3,
        APP_PROTOCOL_TYPE_MOBILE=7,                     /* protocol for Mobile */
        APP_PROTOCOL_TYPE_STATION=11,                     /* protocol between ARM */
		APP_PROTOCOL_TYPE_LIGHT = 13,
		APP_PROTOCOL_TYPE_SCHEDULE=15,                     /* protocol between Sheduler */
		APP_PROTOCOL_TYPE_RADIO = 0x17,                      /* protocol between Radio */
		APP_PROTOCOL_TYPE_UWB_CARD = 0x1F,                       /* protocol for uwb */
		APP_PROTOCOL_TYPE_GATEWAY=0x3F,                     /* protocol between gateway */
        APP_PROTOCOL_TYPE_CMD=0x7F,                    		/* OAD*/
        //APP_PROTOCOL_TYPE_THROUGH=19,					   /* protocol for transfer through */
        APP_PROTOCOL_TYPE_SENSOR_STATION = 0x3f,
    } app_protocol_type_t;

    /* app  header */ 
    typedef struct
    {
        uint_8    protocoltype;       /* always be the source protocol type */
        uint_8    msgtype;
        uint_16    len;
    } app_header_t;
    
        /* app  header */
    typedef struct
    {
        uint_8    u8ProtocolType;   //协议类型    /* always be the source protocol type */
        uint_8    u8MsgType;        //数据类型
        uint_16   u16MsgLen;        //发送消息长度  测距数据长度
    } APP_HEADER_S;

	typedef enum
	{
        COMM_DEFAULT = 0,
       	COMM_MANUAL_OUTAGE = 111, //手动断电
        COMM_MANUAL_RESET = 2, //手动复电
        COMM_AUTO_OUTAGE_CONTROL = 3, //自动断电
        COMM_AUTO_RESET_CONTROL = 4, //自动复电
        COMM_SENSOR_INFO = 5, //传感器基本信息包
        COMM_POLL_SENSOR = 6, //传感器轮询包
        COMM_VALUE_POSITION_SENSOR = 7, //传感器value位置包
        COMM_OUTAGE_SENSOR = 8, //传感器断电包
        COMM_RESET_SENSOR = 9, //传感器复电包
       	COMM_DOWN_ANALOG_UNTRALIMIT_CONFIG = 10, //下发模拟量超限报警/断电配置
        COMM_DOWN_SWITCH_STATUS_CONFIG = 11, //下发开关量状态异常报警/断电配置
        COMM_DOWN_CH4_LOCK_CONFIG = 12, //下发甲烷风电闭锁配置
        //COMM_LIGHT_WARN_CONTROL = 13, //声光报警
        COMM_DOWN_COORDINATED_CONFIG = 14, //下发联动配置包
        COMM_DOWN_COORDINATED_CONFIG_DELETE = 15, //下发联动配置删除命令
        COMM_DOWN_SENSOR_WARN = 16, //传感器报警
		COMM_DOWN_RETREAT = 17, //撤离
		COMM_DOWN_RETREAT_CANCEL = 18, //撤离取消
		COMM_DOWN_BROADCAST = 19,  //广播
        COMM_UP_SITE_INFO = 100, //分站信息上报
       	COMM_UP_SENSOR_DATA = 101, //传感器数据上报

		COMM_COORDINATED_EVACUATE = 102,     //联动撤离
		COMM_COORDINATED_BROADCAST = 103,  //联动广播
		
		COMM_SENSOR_STATUS_REPORT  = 120,    //传感器状态上报
		COMM_3G_STATUS_REPORT  = 121,    //地位板状态上报
		COMM_RADIO_STATUS_REPORT  = 122,    //广播板数据上报
		
        COMM_DOWN_TIME_SYNC = 200, //时间同步  
	}app_recv_sensor_msgtype;


	typedef enum
	{
        RADIO_EVACUATE_CANCLE_ALARM = 0,    //取消撤离
		RADIO_EVACUATE_ALARM = 1,		//撤离

		RADIO_APP_ALARM = 10,			//上层广播
	}radio_recv_baseboard_type;


    /************************************************************************/
    /* mobile protocols                                                           */
    /************************************************************************/
    /* mobile msgtype */
    typedef enum
    {
        /* mobile vs arm */
        /*start with short address*/
        MP_MP2ARM_CMDSTART = 0,
        MP_JOIN_NOTIFY,
        MP_LEAVE_NOTIFY,
        MP_POLL,
        MP_CELLSWITCH,
        MP_TIME,
        MP_VOICE,  //to save air-payload, use short address only
        MP_ERROR_RATE_CHECK,
        MP_MP2ARM_CMDEND = 0x1F,

        /*Start with term_nbr*/
        MP_MP2MP_CMDSTART= 0x20,
        MP_CMD_UP,
        MP_CMD_DOWN,
        MP_SMS,

        CARD_CMD_PERIOD,
        MP_MP2MP_CMDEND = 0x2F,

        /* mobile vs rf */
        MP_SCAN = 0x40,
        MP_SET_CHANNEL = 0x41,
		AUDIO_VOICE = 0x55,
        MP_MP2RF_CMDEND = 0x7F,

        /* mobile rfboard vs arm */
        MPRF_REPORT = 0x80,
        MPRF_REPORT_ACK = 0x81,

        MP_RSSI_MPSTATION = 0x90,
        MP_RSSI_TOF_STATION = 0x91,

        MODULE_ERROR_RATE_TEST,
        VEHICLE_BATTERY_VER = 0x93,

        MP_TRANSFER_THROUGH = 0xF0,

    } app_mp_msgType_t;

    /* mobile header, only mobile2Arm protocol need this header */
    typedef struct
    {
        uint_16    srcaddr;
        uint_16    dstaddr;
    } app_mpheader_t;

    typedef struct
    {
        int len;
        //data for test
    } app_error_rate_check_t;

    /* protocol flow */
    /* app_JoinNwk.req  -> app_JoinNwk.success */
    /*                    -> app_JoinNwk.denied */

#define     APP_MP_JOINNWK_REQ           1
#define     APP_MP_JOINNWK_SUCCESS   2
#define     APP_MP_JOINNWK_DENIED    3
    typedef struct
    {
        app_mpheader_t     hdr;
        uint_8          joinnwktype;
        uint_8          seqnum;
        uint_16         armid;    // when joinnwktype==req, it is the last arm id; when joinnwktype==success, it is the current armid
    }  app_mpJoinNwk_t;


    /* protocol flow */
    /* app_LeaveNwk -> app_ack*/
    typedef struct
    {
        app_mpheader_t     hdr;
        uint_16              seqnum;
        uint_16              reserved;
    }  app_mpLeaveNwk_t;

    typedef  enum
    {
        MP_UP_DIALUP = 0x00,
        MP_UP_FOUND, // send by called party to ZC then to caller, to tell it the called party exist in the network.
        MP_UP_BUSY, //send by called party to ZC then to caller, to tell it the called party is busy now.
        MP_UP_ACCEPT,// send by terminal to ZC when user push accept button.
        MP_UP_CLOSE, // send by terminal to ZC when user push close button.

        MP_DOWN_CALL = 0x10, // to called party
        MP_DOWN_FOUND, // to caller, to tell it the called party exist in the network
        MP_DOWN_BUSY,  // to caller, to tell it the called party is busy
        MP_DOWN_NOTFOUND, // to caller, to tell it the network cannot find the called party
        MP_DOWN_ACCEPT, // to caller, to tell it the called party accept the dialing
        MP_DOWN_CLOSE, // to both, to tell it that the peer close the connection
    } app_mp_cmd_t;

    /* protocol flow */
    /* app_Cmd_t -> app_ack*/
    typedef struct
    {
        app_termNbr_t     srcnbr;        // if cmd_up, dest number; if cmd_down, income number
        app_termNbr_t     dstnbr;
        uint_8               cmdtype;
        uint_8               reserved;
        uint_16          seqnum;

    }  app_mpCmd_t;

    /* protocol flow */
    /* app_Voice_t */
    typedef struct
    {
        app_mpheader_t    hdr;
        uint_16  silencemap;
//    uint_16    armid; /* the previous armid and represent an invalid armid if setting to APP_INVALIDARMADDR */
//    uint_16 reserved;
        uint_8    seqnum;
        uint_8    len;
        //data is followed.
    } app_mpVoice_t;

    /* protocol flow */
    /* app_MP_Poll.req -> app_MP_Poll.ack */
#define     APP_MPPOLL_TYPE_REQ  1
#define     APP_MPPOLL_TYPE_ACK  2

#define     APP_MPPOLL_FLAG_NONE  0
#define     APP_MPPOLL_FLAG_START 1
#define     APP_MPPOLL_FLAG_END   2
#define     APP_MPPOLL_FLAG_REJOIN 3
#define     APP_MPPOLL_FLAG_REQTIME 4

    typedef struct
    {
        app_mpheader_t       hdr;
//    uint_16    armid; /* the previous armid and represent an invalid armid if setting to APP_INVALIDARMADDR */
//    uint_16 reserved;
        uint_8            polltype;
        uint_8            flag;
        uint_16        seqnum;
    } app_mpPoll_t;


    /* protocol flow */
    /* app_cellswitch.request  -> app_cellswitch.allowed -> app_mpAck_t */
    /* app_JoinNwk.req -> app_JoinNwk.success -> app_LeaveNwk.req -> app_ack */
    /* seqnum is the same when once cell switch negotiation*/
#define     APP_MP_CELLSWITCH_REQUEST         1
#define     APP_MP_CELLSWITCH_ALLOWED          2
#define     APP_MP_CELLSWITCH_DENIED          3

    typedef struct
    {
        app_mpheader_t     hdr;
        uint_8              switchtype;
        uint_8             seqnum;
        uint_16               newarmid;
        uint_16            newrfid;
    } app_mpCellSwitch_t;


    /* protocol flow */
    /* app_mpPoll_t.flag==REQTIME -> app_mpTime_t */
    typedef struct
    {
        app_mpheader_t     hdr;
        uint_16  year;
        uint_8   month;
        uint_8   day;
        uint_8   hour;
        uint_8   minute;
        uint_8   second;
        uint_8   seqnum;
    } app_mpTime_t;


    /* protocol flow */
    /* app_SMS.content -> app_SMS.ack */
#define APP_MP_SMSTYPE_CONTENT  1
#define APP_MP_SMSTYPE_ACK       2

    typedef struct
    {
        app_termNbr_t     srcnbr;        // if cmd_up, dest number; if cmd_down, income number
        app_termNbr_t     dstnbr;
        uint_16        seqnum;
        uint_8         smstype;
        uint_8         len; /*len is 0 when ack*/

        //SMS content is followed.
    }  app_mpSMS_t;

    /* protocol flow */
    /* app_mpArmid_t.req -> app_mpArmid_t.ack */
#define     APP_SCAN_TYPE_REQ  1
#define     APP_SCAN_TYPE_ACK  2
    typedef struct
    {
        uint_8        scantype;
        uint_8        seqnum;
        uint_16             armid;        // only used in ack
    } mp_Scan_t;

    /************************************************************************/
    /*                                                                                    */
    /************************************************************************/

    /************************************************************************/

#define APP_MAX_CARD_NUM        ((BSMAC_MAX_TX_PAYLOAD_LEN-sizeof(app_header_t)-sizeof(struct nwkhdr))/sizeof(tof_distance_ts))    // cut app_header and nwk_header, and then divided by 8
#define APP_MAX_CARD2_NUM        ((BSMAC_MAX_TX_PAYLOAD_LEN-sizeof(app_header_t)-sizeof(struct nwkhdr))/sizeof(tof_more_distance_ts))    // cut app_header and nwk_header, and then divided by 12
#define APP_MAX_GAS_NUM        ((BSMAC_MAX_TX_PAYLOAD_LEN-sizeof(app_header_t)-sizeof(struct nwkhdr))/sizeof(tof_gas_rssi_ts))    // cut app_header and nwk_header, and then divided by 12
#define APP_MAX_CARD3_NUM      ((BSMAC_MAX_TX_PAYLOAD_LEN-sizeof(app_header_t)-sizeof(struct nwkhdr))/sizeof(tof_new_distance_ts))
#define APP_UWB_MAX_CARD_NUM   20
#define APP_UWBTDOA_MAX_CARD_NUM   10

#define APP_MAX_LIGHT_NUM    12
#define MAX_TREE_DEPTH           8
/* for JN vs ARM to determine msg type */
    typedef enum
    {
        APP_TOF_MSG_CHECKIN,        // for card checkin
        APP_TOF_MSG_RSSI,
        APP_TOF_MSG_DISTANCE,
        APP_TOF_MSG_ALARM,        // card alarm:correspond to app_tof_alarm_type_te
        APP_TOF_MSG_ALARM_ACK,    // arm send alarm ack
        ABANDENED,        // no use any more
        APP_TOF_MSG_RETREAT,        // ARM send retreat msg
        APP_TOF_MSG_CANCEL_RETREAT,    // ARM cancel retreat msg
        APP_TOF_MSG_MORE_DISTANCE,   //added from Release1201,  to instead APP_TOF_MSG_DISTANCE, report both SC and LC distance
        APP_TOF_MSG_GAS_RSSI,
        APP_TOF_MSG_GAS_DENSITY,
        APP_TOF_MSG_GAS_ALARM,
        APP_TOF_MSG_GAS_ALARM_ACK,
        APP_TOF_MSG_NEW_DISTANCE,
        APP_TOF_MSG_NEW_ALARM,        //support report multiple alarm:correspond to app_tof_card_status_te

		APP_LIGHT_MSG_DEPTH,
		APP_LIGHT_MSG_PARAMETER,
		APP_LIGHT_MSG_RSSI,
		APP_LIGHT_MSG_NODE_CHANNEL,
		APP_LIGHT_MSG_RSSI_CHANNEL,
		APP_LIGHT_MSG_VERSION,

		APP_RADIO_MSG_DATA,		
		APP_RADIO_MSG_DAIUP,
		APP_RADIO_MSG_FOUND,
		APP_RADIO_MSG_LINK,
		APP_RADIO_MSG_ACCEPT,
		APP_TOF_SENSOR_ALARM,
		APP_TOF_SENSOR_CANCEL_ALARM,
		APP_RADIO_ALARM,
		APP_RADIO_CANCEL_ALARM,
        APP_TOF_MSG_REPORT = 0x80,
        APP_TOF_MSG_REPORT_ACK = 0x81,
        APP_TOF_MSG_SET = 0x82,
        APP_TOF_MSG_LINK = 0x83,

        APP_TOF_MSG_RSSI_MPSTATION=0x90,
        APP_TOF_MSG_RSSI_TOF_STATION=0x91,
        APP_VEHICLE_BATTERY_VER = 0x92,

		APP_SYNC_TIME = 0xA0,

        APP_QUERY_CONFIG = 0xB0,
        APP_SET_CONFIG = 0xB1,
        APP_REPORT_CONFIG = 0xB2,
		APP_REPORT_SW_VERSION = 0xB3,
		APP_UPGRADE_STATON_SW = 0xB4,
		APP_CANCEL_UPGRADE_STATON_SW = 0xB5,
		APP_SET_STATION_CHANNEL = 0xB6,

		APP_TRANSFER_THROUGH = 0xF0,
		APP_VEHICLE_CARD_RSSI = 0xF1,
    } app_tof_msg_te;

    /* app_mprfReport_t */
    typedef enum
    {
        APP_LS_REPORT_POWERON =                      1,
        APP_LS_REPORT_WATCHDOG_RESTART   =        2,
        APP_LS_REPORT_LIVE    =                         3,
        APP_LS_REPORT_STATUS_LOC =               4,
        APP_LS_REPORT_STATUS_COM =               5,
        APP_LS_REPORT_VERSION = 				6,
    } app_ls_reportType_te;

    typedef enum
    {
        APP_LS_TOF_STATION  =  1,
        APP_LS_RSSI_STATION = 2,
        APP_LS_CHECKIN_STATAION = 3,
    } app_ls_stationType_te;

    typedef struct
    {
        app_mpheader_t hdr;
        uint_8 reporttype;
        uint_8         devtype;        //0x02: COM  0x03:LOC
        uint_8         seqnum;
        uint_8         len;
        //data is followed if len is not 0
    }  app_LSrfReport_t;

    /* port status included in app_LSComstatus_t and app_LSLocstatus_t*/
    typedef struct
    {
        uint_16 neighborid;
        uint_16 lost_cnt;
        uint_16 total_cnt;
        uint_16 livestat;
    } module_port_t;

    typedef struct
    {
        module_port_t    port[2];
        uint_16   oad_version;
        uint_16   len;
        // version is added here by string
    } app_LSComstatus_t;

    typedef struct
    {
        uint_8        tof_channel;  //LS_channel
        uint_8        station_type;
        uint_8        loc_distance[2];
        uint_32      loc_id[2];
        uint_8        loc_channel;
        uint_8        comm_channel;
        uint_16      loc_oad_version;
        module_port_t port[2];
        uint_16   oad_version;
        uint_16   len;
        // version is added here by string
    } app_LSLocstatus_t;

    typedef struct
    {
        uint_16        vehicle_card_addr;  //车卡卡号
        uint_8         battery;
        uint_8         len;
        // version is added here by string
    } app_Vehicle_Battery_VER_t;

    typedef struct
    {
        uint_16 	 devid;
        uint_8         devtype;   // 1: card  2: Locator
        uint_8         rssich;    // card rssi channel
        uint_16       oad_ver;
        uint_16 	 battery;   // uint: 0.1v
    }  app_LSVersionReport_t;

    typedef struct
    {
        app_mpheader_t hdr;
        uint_16         seqnum;
        uint_16         crc;
        //tlvs is followed
    }  app_LSrfSet_t;

    typedef struct
    {
        uint_8  type;
        uint_8  len;
    } __PACKED app_rfTlv_t;


    typedef enum
    {
        APP_LS_TLV_TYPE_TOF_CHANNEL = 1,     //uint8
        APP_LS_TLV_TYPE_LOC_CHANNEL, 	 //uint8
        APP_LS_TLV_TYPE_LOC_DISTANCE_0,  //uint8
        APP_LS_TLV_TYPE_LOC_DISTANCE_1,  //uint8
        APP_LS_TLV_TYPE_LOC_COMPATIBLE_GAS,   // support gasnode
        APP_LS_TLV_TYPE_LOC_UNCOMPATIBLE_GAS,   // not support gasnode
        APP_LS_TLV_TYPE_LOC_COMPATIBLE_SPEED,  //support speed
        APP_LS_TLV_TYPE_LOC_UNCOMPATIBLE_SPEED, // not support speed
        APP_LS_TLV_TYPE_SINGLE_CARD_RETREAT,
        APP_LS_TLV_TYPE_RSSI_CHANNEL,
		APP_LS_TLV_TYPE_SPEED_REMIND,
    } app_tlv_type_te;

    /* for TOF distance's direction */
    typedef enum
    {
        APP_TOF_DIRECTION_Z = 0,        // zero: near station
        APP_TOF_DIRECTION_P = 1,        // positive direction
        APP_TOF_DIRECTION_N = 2,        // negative direction
        APP_TOF_DIRECTION_F = 0xFF,    // fail to determine direction
    } app_tof_direction_te;

    /* for card or locator's status */
    typedef enum
    {
        APP_TOF_ALARM_NONE,            // normal, NOT used
        APP_TOF_ALARM_CARD_HELP,        // card alert
        APP_TOF_ALARM_CARD_NOPWD,    // no power
        APP_TOF_ALARM_LOC_NOPWD,    // locator no power
        APP_TOF_ALARM_CARD_HELP_ACK,    // when arm receive card HELP alarm msg, need to send ack, ack structure see app_tof_alarm_ack_ts

        //GasNode
        APP_TOF_ALARM_GASNODE_URGENT,
        /*GasNode accident alarms */
        APP_TOF_ALARM_GASNODE_FIRE,
        APP_TOF_ALARM_GASNODE_WATER,
        APP_TOF_ALARM_GASNODE_TOPBOARD,
        APP_TOF_ALARM_GASNODE_OTHERS,

        APP_TOF_ALARM_GASNODE_HELP_ACK,
        APP_TOF_ALARM_GASNODE_FIRE_ACK,
        APP_TOF_ALARM_GASNODE_WATER_ACK,
        APP_TOF_ALARM_GASNODE_TOPBOARD_ACK,
        APP_TOF_ALARM_GASNODE_OTHER_ACK,
        APP_TOF_RETREAT_ACK,
    } app_tof_alarm_type_te;

    typedef enum
    {
    	APP_CARD_STATUS_NORMAL 	= 0x01,
    	APP_CARD_STATUS_HELP	= 0x02,	// card alert
    	APP_CARD_STATUS_NOPWD	= 0x04,	// no power
    	APP_CARD_STATUS_RETREAT	= 0x08,	// retreat
    	APP_CARD_STATUS_RETREAT_ACK = 0x20, // retreat ack
    	APP_CARD_STATUS_EXCITER   = 0x80, //exciter
    }app_tof_card_status_te;

    typedef enum
    {
        APP_TOF_CARD_RSSI,
        APP_TOF_CARD_REQ,
        APP_TOF_DEVICE_CARD_REQ,
    }app_tof_card_rssi_type_te;

    typedef enum
    {
        APP_CARD_MOTION = 0x01,
        APP_CARD_ORIENTATION = 0x02,
        APP_CARD_FREEFALL =0x04,
        APP_CARD_IS_WITH_ACCEL = 0x80,// card is with accelerometer or not
    }app_tof_card_accel_te;

    /* for TOF card's type */
    typedef enum
    {
        APP_TOF_CARD_5S = 0,
        APP_TOF_CARD_1S = 1,
        APP_TOF_CARD_15S = 2,
    } app_tof_cardType_te;

    typedef struct
    {
        uint_16 u16ShortAddr;
        uint_16 u16SeqNum;
        int_8 i8Rssi;
        uint_8 u8RssiType;
		uint_8 u8Accel;
		uint_8 u8Reserved;
    } tof_rssi_ts;

    typedef struct
    {
        uint_16 u16ShortAddr;
        int_8 i8Rssi;
        uint_8 u8Reserved;
        uint_16 uu16SeqNum;
    } __PACKED vehicle_rssi_ts;

    typedef struct
    {
        uint_16 u16ShortAddr;
        uint_16 u16SeqNum;
        uint_16 u16GasDensity;
        uint_16 u16GasThr;
		uint_8 u8GasType;     //0x01 gas
        int_8 i8Rssi;
        uint_16 u16Reserved;
    } tof_gas_rssi_ts;

    typedef struct
    {
        uint_16 u16ShortAddr;
        uint_16 u16SeqNum;
        uint_16 u16Distance;
        int_8 i8Rssi;
        uint_8 u8Direction;    // app_tof_direction_te
    } tof_distance_ts;

    typedef struct
    {
        uint_16 u16ShortAddr;
        uint_16 u16SeqNum;
        uint_16 u16Distance;
        int_8 i8Rssi;
        uint_8 u8Direction;
        uint_16 u16LocCardDistance;
        uint_8 u8DevType;
        uint_8 u8Reserved;
    } tof_more_distance_ts;


    typedef struct
    {
        uint_16 u16ShortAddr;
        uint_16 u16SeqNum;
        uint_16 u16Distance;
        int_8 i8Rssi;
        uint_8 u8DevType;
        uint_16 u16LocCardDistance[2];
    } tof_new_distance_ts;


// ----------- ARM to RF msg -------------
    typedef struct
    {
        app_header_t app_tof_head;

        uint_16 u16ShortAddr;    // the alarmed card or locator's addr
        uint_8 u8AckType;        // app_tof_alarm_type_te, currently ONLY used for card help
        uint_8 u8Reserved;
    } app_tof_alarm_ack_ts;

// -----------  following are RF to ARM msg --------------
    /* card's alarm */
    typedef struct
    {
        app_header_t app_tof_head;

        uint_16 u16ShortAddr;    // card or locator's addr
        uint_8 u8Status;            // app_tof_alarm_type_te
        uint_8 u8Reserved;
    } app_tof_checkin_ts;

    /* card's alarm */
    typedef struct
    {
        app_header_t app_tof_head;

        uint_16 u16ShortAddr;    // card or locator's addr
        uint_8 u8Status;            // app_tof_alarm_type_te
        //uint_8 u8Reserved;
        uint_8  u8ExciterID;
    } app_tof_alarm_ts;

    /* card's rssi */
    typedef struct
    {
        app_header_t app_tof_head;

        tof_rssi_ts tof_rssi[APP_MAX_CARD_NUM];    // the length(app_tof_head.len) of this array is alterable, max length is APP_MAX_CARD_NUM
    } app_tof_rssi_ts;


    typedef struct
    {
        app_header_t app_tof_head;
        uint_16 u16station_addr;
        uint_16 u16seqnum;
        vehicle_rssi_ts vehicle_rssi[10];    // the length(app_tof_head.len) of this array is alterable, max length is APP_MAX_CARD_NUM
    } app_vehicle_rssi_ts;

    /*gasnode's rssi */
    typedef struct
    {
        app_header_t app_tof_head;
        tof_gas_rssi_ts tof_gas_rssi[APP_MAX_GAS_NUM];
    }app_tof_gas_rssi_ts;

    /* card's distance */
    typedef struct
    {
        app_header_t app_tof_head;

        tof_distance_ts tof_distance[APP_MAX_CARD_NUM];    // the length(app_tof_head.len) of this array is alterable, max length is APP_MAX_CARD_NUM
    } app_tof_distance_ts;


    /* card's distance */
    typedef struct
    {
        app_header_t app_tof_head;

        tof_new_distance_ts tof_distance[APP_MAX_CARD3_NUM];    // the length(app_tof_head.len) of this array is alterable, max length is APP_MAX_CARD_NUM
    } app_new_tof_distance_ts;

    #define GAS_INVALID_DENSITY      10001
    #define GAS_LOST_DENSITY         10002

    typedef struct
    {
        app_header_t app_tof_head;
        uint_16 u16ShortAddr;
        uint_16 u16SeqNum;
        uint_16  u16Distance;
        uint_16 u16LocDistance[2];
        uint_16  u16GasDensity;
        uint_16  u16GasThr;
		uint_8   u8GasType;     // 0x01: gas
        int_8    i8Rssi;
        uint_8   u8Direction;
        uint_8   u8Reserved1;
        uint_16  u16Reserved2;
    }app_tof_gasdensity_ts;

    /* card's distance, report Loc and Card distance */
    typedef struct
    {
        app_header_t app_tof_head;

        tof_more_distance_ts tof_distance[APP_MAX_CARD2_NUM];    // the length(app_tof_head.len) of this array is alterable, max length is APP_MAX_CARD_NUM
    } app_tof_more_distance_ts;

    /* report locator's status */
typedef struct
{
	app_header_t app_tof_head;

	uint_16 u16ShortAddr;
	uint_8 u8Distance;        // locator's distance
	uint_8 u8Status;            // app_tof_alarm_te
} app_tof_locator_ts;

// end of TOF protocol for ARM
    /************************************************************************/
typedef struct
{
	app_header_t app_rssi_head;

	uint_16 u16ShortAddr;
	int_8 sentRssi;
	int_8 receiveRssi;
} app_rssi_report;

typedef struct
{
	int_32 timestamp;
	uint_16 panid;
	uint_16 moduleid;
} app_SMT32_Data;

typedef struct
{
	app_header_t hdr;
	int_32 timestamp;
} app_synctime_t;

typedef struct
{
	uint_32 bs_id;
	uint_32 bs_ip;
	uint_32 bs_mk;
	uint_32 bs_gw;
	uint_32 svr_ip;
	uint_32 svr_port;
	uint_32 delay;
	uint_32 ip_mode;  // 0:dhcp，1:static
}stm32_config_t;

typedef struct
{
	app_header_t hdr;
	stm32_config_t cfg;
} app_stm32_config_t;

typedef struct
{
	app_header_t hdr;
	uint_8 sw_version[256];
} app_station_sw_version_t;

typedef struct
{
	uint_16 device_addr;
	uint_16 len;
} app_device_through_ts;

typedef struct
{
	app_header_t hdr;
	app_device_through_ts device_through_data;
} app_transfer_through_t;

	
//UWB  protocol
typedef enum
{
	APP_UWB_MSG_CHECKIN,        // for card checkin
	APP_UWB_MSG_RSSI,
	APP_UWB_MSG_DISTANCE,
	APP_UWB_MSG_ALARM,        // card alarm:correspond to app_tof_alarm_type_te
	APP_UWB_MSG_ALARM_ACK,    // arm send alarm ack
	APP_UWB_MSG_RETREAT,        // ARM send retreat msg
	APP_UWB_MSG_CANCEL_RETREAT,    // ARM cancel retreat msg
	APP_UWB_MSG_REQ_LOC_DISTANCE,
	APP_UWB_MSG_CARD_VER_BATTRY,         //card's version and battery
	APP_UWB_MSG_STATION_VER_LINK,        // station version and link 
	APP_UWB_MSG_TDOA,                 //tdoa timestamps
	APP_UWB_MSG_GAIN_SETTING,
	APP_UWB_MSG_REPORT = 0x80,
	APP_UWB_MSG_REPORT_ACK = 0x81,
	APP_UWB_MSG_SET = 0x82,
	APP_UWB_MSG_LINK = 0x83,
}app_uwb_msg_te;

typedef enum
{
	APP_UWB_ALARM_NONE,            // normal, NOT used
	APP_UWB_ALARM_CARD_HELP,        // card alert
	APP_UWB_ALARM_CARD_NOPWD,    // no power
	APP_UWB_ALARM_CARD_HELP_ACK,    // when arm receive card HELP alarm msg, need to send ack, ack structure see app_tof_alarm_ack_ts
} app_uwb_alarm_type_te;


typedef enum
{
	UWB_CARD_STATUS_NORMAL = 0x00,        // 正常
	UWB_CARD_STATUS_NOPWD = 0x01,       // 低电
	UWB_CARD_STATUS_IMPEL = 0x02,        // 激励
	UWB_CARD_STATUS_HELP = 0x04,         // 求救
	UWB_CARD_STATUS_RETREAT = 0x08,         // 撤离
	UWB_CARD_STATUS_RETREAT_ACK = 0x10,    //撤离ack
	UWB_CARD_STATUS_ACTIVE = 0x20,      // 运动的
	UWB_CARD_STATUS_ORIENTATION = 0x40,   //卡的姿态
	UWB_CARD_STATUS_IS_WITH_ACCEL = 0x80,  //是否有加速度传感器
}app_uwb_card_status_te;


typedef struct
{
	app_header_t app_uwb_head;

	uint_16 u16ShortAddr;	// the alarmed card or locator's addr
	uint_8 u8AckType;		// app_tof_alarm_type_te, currently ONLY used for card help
	uint_8 u8Reserved;
	} app_uwb_alarm_ack_ts;


typedef struct
{
	uint_16 u16ShortAddr;
	uint_16 u16SeqNum;
	uint_32 u32StationDistance;
	uint_32 u32LocDistance;
	uint_8 u8DevType;	  // 
	uint_8 u8Status;
	int_8  i8Rssi;
	uint_8 u8Reserved;
} uwb_tof_distance_ts;

typedef struct
{
	uint_16 u16ShortAddr;
	uint_16 u16SeqNum;
	uint_8 u8DevType;	  // 
	uint_8 u8Status;
	int_8  i8Rssi;
	uint_8 u8Reserved;
} uwb_rssi_ts;

typedef struct 
{
	uint_16 beTest_cardID;            //待测标签ID
	uint_16 standard_cardID;          //基准标签ID
	uint_16 stationID;                //基站ID           sys_option.u32BsId;
	uint_16 Card_seqnum;              //待测标签序列号
	uint_32 S_QANH_Tie_H;             //慢发与最近一次快发的时间戳差
	uint_32 S_QANH_Tie_L;             //慢发与最近一次快发的时间戳差
	uint_32 Q_QANH_Tie_H;             //慢发后一次快发减去上一次快发的时间戳差
	uint_32 Q_QANH_Tie_L; 
	uint_8 u8DevType;	                // 卡类型，1s卡还是5s卡
	uint_8 u8Status;                  //卡状态
	int_8  i8Rssi;                    //信号强度
	uint_8 u8Reserved;
}uwb_tdoa_TimeStamp_ts;

typedef struct     
{
	app_header_t app_tof_head;
	uwb_tdoa_TimeStamp_ts tdoa_msg[APP_UWBTDOA_MAX_CARD_NUM];	
} app_uwb_tdoa_distance_ts;    //放入到协议文件中

//待测卡组包结构中信息
typedef struct 
{
	uint_16 u16TestCardID;              //待测标签ID
	uint_16 u16StandardCardID;          //基准标签ID
	uint_16 u16StationID;                //基站ID           sys_option.u32BsId;
	uint_16 u16Cardseqnum;              //待测标签序列号
	uint_32 u32SQANHTieH;               //慢发与最近一次快发的时间戳差
	uint_32 u32SQANHTieL;               //慢发与最近一次快发的时间戳差
	uint_32 u32QQANHTieH;               //慢发后一次快发减去上一次快发的时间戳差
	uint_32 u32QQANHTieL; 
	uint_8  u8DevType;	  				// 卡类型，1s卡还是5s卡
	uint_8  u8Status;					//卡状态
	int_8   i8Rssi;						//信号强度
	uint_8  u8Reserved;
}APP_UWB_TDOA_TIMESTAMP_S;
//与其他层级约定好的数据协议结构 协议头 + 数据大小
typedef struct     
{
	APP_HEADER_S stAppHead;
	APP_UWB_TDOA_TIMESTAMP_S stAppTdoaMsg[APP_UWBTDOA_MAX_CARD_NUM];	
} APP_UWB_TDOA_DITANCE_S;    //放入到协议文件中

/* card's distance */
typedef struct
{
	app_header_t app_tof_head;
	uwb_tof_distance_ts tof_distance[APP_UWB_MAX_CARD_NUM];	  // the length(app_tof_head.len) of this array is alterable, max length is APP_MAX_CARD_NUM
} app_uwb_tof_distance_ts;


/* card's distance */
typedef struct
{
	app_header_t app_tof_head;
	uwb_rssi_ts rssi_data[APP_UWB_MAX_CARD_NUM];	  // the length(app_tof_head.len) of this array is alterable, max length is APP_MAX_CARD_NUM
} app_uwb_rssi_ts;

typedef struct
{
	app_header_t app_tof_head;
	uint_16 u16ShortAddr;	 // card or locator's addr
	uint_8 u8Status;			// app_tof_alarm_type_te
	//uint_8 u8Reserved;
	uint_8	u8ExciterID;
} app_uwb_alarm_ts;

typedef struct
{
	uint_16	   devid;
	uint_16	   oad_ver;
	uint_16	   battery;   // uint: 0.1v
	uint_16     Reserved;
}  app_UWBVersionReport_t;

typedef enum
 {
	 APP_LS_TLV_TYPE_UWB_CARD_1 = 1,	  //uint8
	 APP_LS_TLV_TYPE_UWB_CARD_5,	  //uint8
	 APP_LS_TLV_TYPE_UWB_CARD_RETREAT,   //单卡撤离
 } app_uwb_tlv_type_te;




typedef enum
{
	APP_LIGHT_MSG_REPORT,		//
	APP_LIGHT_MSG_REPORT_ACK,
	APP_LIGHT_MSG_CARD_RSSI,
	APP_LIGHT_MSG_SET,	
	APP_LIGHT_MSG_COULOMETRY,	 
	APP_LIGHT_MSG_VER_CHANNEL,
} app_light_msg_te;

typedef struct
{
    char sync[3];
    uint_8 data_len;
    // followed by TLVs
}__PACKED light_sync_hdr_t;



typedef struct
{
	light_sync_hdr_t sync_head;
	app_header_t app_tof_head;
	uint_8      u8MaxLightLevel;
	uint_8	u8MinLightLevel;
	int_8         i8Rssi_Threshold;
	uint_8  u8IsRssiStation;
}  app_Light_ReportAck_t;

typedef struct
{
	light_sync_hdr_t sync_head;
	app_header_t app_tof_head;
	uint_8      u8Channel;
	uint_8      u8Reserved;
	uint_16    u16Reserved;
}  app_Light_ChannelSeting_t;


typedef struct
{
	light_sync_hdr_t sync_head;
	app_header_t app_light_head;
	uint_8        u8Vrms;
	uint_8        u8Irms;	
	uint_8       u8Channel;
	uint_8     u8Reserved;
}  app_Light_Report_t;

typedef struct
{
	uint_8 u8vrms;
	uint_8  u8irms;
	uint_16 u16panid;
	uint_16 u16parent;
	uint_16 u16Reserved;
} light_depth_vol_cur_ts;

typedef struct
{
	app_header_t app_light_head;
	light_depth_vol_cur_ts light_depth_vol_cur[APP_MAX_LIGHT_NUM];    // the length(app_tof_head.len) of this array is alterable, max length is APP_MAX_CARD_NUM
} app_light_depth_vol_cur_ts;

typedef struct
{
	uint_8 u8light_channel;
	uint_8  u8rssi_channel;
	uint_16 u16light_oad;
	uint_16 u16rssi_oad;
	uint_16 u16light_addr;
} light_ver_channel_ts;

typedef struct
{
	app_header_t app_light_head;
	light_ver_channel_ts light_ver_channel;    // the length(app_tof_head.len) of this array is alterable, max length is APP_MAX_CARD_NUM
} app_light_ver_channel_ts;

// ----------- ARM to RF msg -------------
typedef struct
{
	app_header_t app_light_head;
	uint_16 u16DestAddr;    
	int_8 i8RssiAssignment;        
	uint_8 u8MaxLightLevel;
	//uint_16 u16Reserved;
	uint_8 u8IsRssiStation;
	uint_8 u8Reserved;
	uint_8 u8MinLightLevel;
	uint_8 u8Len;
	uint_16 u16Pathtoroot[MAX_TREE_DEPTH];
} app_light_parameter_ts;

typedef struct
{
	app_header_t app_light_head;
	uint_16 u16DestAddr;    
	uint_8 u8channel;        
	uint_8 u8Len;
	uint_16 u16Pathtoroot[MAX_TREE_DEPTH];
} app_light_channel_ts;


typedef struct
{
    uint_16 u16tag;
    uint_16 u16seqnum;
} TxFrameHdr_te;

typedef struct
{
    uint_16 u16type;
    uint_16 u16value;
} SensorData_te;

typedef enum
{
	SENSOR_TYPE_CURRENT = 1,
    SENSOR_TYPE_FREQUENCY = 2,
} SensorType_te;

typedef enum
{
	SENSOR_TAG =125,
} ChannelTag_te;

typedef struct
{
    app_header_t tof_head;
    TxFrameHdr_te channel_hdr;
    SensorData_te sensor_data[6];
} TxFrameSensorData_te;

#ifdef __cplusplus
}
#endif

#endif




