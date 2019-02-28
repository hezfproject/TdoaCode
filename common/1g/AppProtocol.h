/****************************************************************************
Filename:       AppProtocol.h
Revised:        $Date: 2011/08/15 18:50:43 $
Revision:       $Revision: 1.185 $
Description:    Define the data frame between zigbee and ARM.
*****************************************************************************/
#ifndef APP_PROTOCOL_H
#define APP_PROTOCOL_H
/*****************************************************************************
Update the hdr between zigbee and ARM.

The following defines the data frames in zigbee network.
                 --------------------
ZigbeeAPP:      |APPHeader|APPpayload|
                ---------------------
                    1byte     variable

protocol description:


-----------------------PROTOCOL between zigbee and ARM--------------------------

Because ARM and zigbee communicate with two SPI, so an extra header is needed to decide the
protocol between them.
The SPIMSGPacket_t is at header and ZigbeeApp data is following it.
the data format between ARM and Zigbee Coordinator is:
        SPIMSGPacket_t + ZigbeeAPP
------------------------------------------------------------------------------

******************************************************************************/

#include "CommonTypes.h"
#include "CommonMacro.h"
#include "app_group_msg.h"

#ifdef __cplusplus
extern "C" {
#endif

#define DEBUG_STRLEN  72
    /****************************config for ZB and ARM begin *************************/
//unit:s
#define TIMEOUT_CONNECTION  30
#define TIMEOUT_LEAVE_NWK   150
#define TIMEOUT_REJOIN_NWK  24//120

#define NWKCapability             20 // the same to MAX_CHLDRN_CNT of nwk_global.h in z-stack.
    /****************************config for ZB and ARM end ***************************/
//Define the 2-byte phone number.
#define GATEWAYNMBR 0
#define INVALIDNMMBR 0xFFFF

#define GATEWAYMIN 9000
#define GATEWAYMAX 9999

#define GATEWAYDELIMITER '0'

#define LOCNODENUM   5
#define REPORTFREQUENCE 5 //every 5 seconds report gas density.
#define CALLINGHELP_MAXCNT 10
    /*********************************************************************
     * MACROS
     */

    /*********************************************************************
     * CONSTANTS
     */
#define APPHDR_LEN sizeof(MSGType)
#define SPIPKTHDR_LEN sizeof(SPIMSGPacket_t)

//the following is for compatible with z-stack AF/APS layer.
//From AF.h.
#define SPI_ACK_REQUEST                     0x10
#define SPI_DISCV_ROUTE                     0x20
#define SPI_EN_SECURITY                     0x40
#define SPI_SKIP_ROUTING                    0x80

//A invalid network Id used by all cards
#define CARD_NWK_ADDR                         0xFFF0
#define CONTROLLER_NWK_ADDR              0xFFF1
#define LOCNODE_NWK_ADDR              0xFFF2 //for loc node.
#define CARDREADER_NWK_ADDR        0xFFF3
#define MOBILEPHONE_NWK_ADDR        0xFFF4
#define SANY_NWK_ADDR				0xFFF5
#define BLAST_NWK_ADDR              0xFFF6

//Mobile phone work interval(unit:s), a phone's rf will open/close alternately if not talking.
//Now we assume it will rest 2s every times.
#define MP_WORK_INTERVAL   3
#define MP_WORK_TIMEOUT     300

//The necessary macro constant.
#define MAX_DEPTH 10
#define INIT_TRANID 0
#define INIT_OPN SPI_DISCV_ROUTE

#define MAX_DATA_SIZE 85 //from z-stack.
//SMS max length.
#define SMS_MAX_LEN (MAX_DATA_SIZE - sizeof(app_SMS_t))
#define GASSMS_MAX_LEN (MAX_DATA_SIZE - sizeof(app_gassms_t))

#define SPIDETECT_LEN  32  //spi detect data len, from 0 to len-1

//for error rate test
#define ERROR_RATE_TEST_DATA_LEN 88

//From ZComDef.h
    enum
    {
        _AddrNotPresent = 0,
        _AddrGroup = 1,
        _Addr16Bit = 2,
        _Addr64Bit = 3,
        _AddrBroadcast = 15
    };


//#define termNbr_t     uint_16
//#define TERM_NBR_CENTER   termNbr_t(0)
//#define TERM_NBR_ALL  termNbr_t(~0)

    typedef struct
    {
        char Nmbr[NMBRDIGIT];
    } __PACKED termNbr_t;

    typedef enum
    {
        LOCATION = 1,
        GASREPORT,
        GASTHRESH,
        VOICE,
        MODE,
        URGENT,
        NODESLEEP,
        SSREQ,
        SSIND,
        PWRCTL,             // 10
        ZB_START_NETWORK,   // It's a rsp of  TRY_CONNECTION to ARM, notify it the fixed panid in mac addr.
        ZB_JOIN_NOTIFY, //If a zbnode come into zigbee network, notify ARM.
        ZB_LEAVE_NOTIFY, //notify ARM i'm leaving the zigbee network.
        ZB_DATA_TRANSMIT,   // data transmitting.
        ZB_DIAL_UP, //zbnode dial up, ZB -> ARM, deprecated.
        //ZB_DIAL_ACK, //deprecated.
        //ZB_HANG_UP, //deprecated.
        ZB_CMD_UP,
        ZC_CMD_DOWN,
        //SSRSP, //a piece of rssi report, deprecated.
        DEPUTY_JOIN, //The parent-node delegates an end device to tx a  join_req to another station.
        DEPUTYJOIN_RSP, //the result of deputy_join.
        TRY_CONNECTION, //ARM try and build connection  between ZC and ARM.  20
        SIGNAL_STRENGTH,
        MP_SSREQ, //identify MP RSSI request
        MP_SSIND,//identify MP RSSI indication.
        TIME_SSIND,  // time sync indication
        CLEARNWK, //clear dead link in the wireless network.from ARM to ZC.
        ZB_REJOIN_NOTIFY, //ARM cann't find an end point and send a rejoin command to sync it, from ARM to end device.
        SMS,//short message.
        SMSRsp, //from MP to ARM.
        MP_POLL_REQ,   //Mobile phone poll for data, up command
        MP_POLL_ACK,   // Mobile phone poll ack, None indicates no data, Start indicates has some data to trans: 30
        ZC_REPORT, //Report the ZC status to ARM.
        CROSSPAN,  // Transfer data cross pan.
        SIGNAL_ACK, //Identify the recver receive the signal succcessfully.
        ZC_DETECT,  // To detect if ZC is in normall state, used by ARM
        SPI_DETECT, //Invalidate SPI work status.
        SPI_DEBUG,  //Debug SPI for ARM.
        RF_DEBUG,  //Get RF board informations by command

        LOCNODE_CAST,
        GASALARM,
        GASALARM_ACK = 40,

        AMMETER_QUERY,      // query ammeter ID of each collector
        AMMETER_TABLE_RET,  // return from collector
        AMMETER_DATA,       // wireless data between ZC / collector
        AMMETER_DATA_ACK,   // ACK of each wireless packet
        AMMETER_ROUTE_REPORT, // report rout information to ARM

        GASDEV_SMS,         // gas device short message
        GASDEV_SMS_ACK,     // short message ack

        AMMETER_DATA_LAST,   //FIXME, need to restructure the protocal
        AMMETER_DATA_CMDLINE ,

        CHARGEED_SSREQ  = 50,
        CHARGEED_SSRSP,
        CHARGEED_SSIND,

        LOCNODE_INFO_REQ,
        LOCNODE_INFO_SET,

        LOCNODE_ALARM,

        MP_SWITCH_REQ,

        DATA_CROSS_PAN,
        ROUTE_INFO,

        RF_VERSION_REQ,
        RF_VERSION_RSP,

        RFMAC_QUERY,
        RFMAC_SET,
        RF_REPORT,
        CARD_SPI_CMD,
        CARD_STATUS,                     // for card to report version, battery, and other status

        /* if mobilephone poll flag= POLL_FLAG_REQARMVERSION, arm report version to mobilephone */
        REPORT_STATION,


        /*  protocols for timac mobilephone and rf board */
        TIMAC_JOIN_NOTIFY = 0x80,
        TIMAC_LEAVE_NOTIFY,
        TIMAC_MP_SCAN,

        SANY_DATA = 0xA0,
        SANY_STATUS,
        
        BLAST_REQUEST = 0xA8,

        CONVEYOR_CMD = 0xB0,
        CONVEYOR_INFO,

        /* protocal of warehouse goods locating system */
        LOCSYS_DATA = 0xC0,
        LOCSYS_CMD,
        LOCSYS_CNF,

        EXCITER_FUN = 0xC3,

    }  __PACKED MSGType;

    typedef enum
    {
        MAC_TREE_DATA = 0x00,
        MAC_TREE_DATA_LAST,
        MAC_TREE_DATA_CMDLINE,
        MAC_TREE_DATA_ACK,

    } __PACKED MAC_MSGType;

    typedef enum
    {
        SLEEPTIME,
        POLL_INTERVAL,
        POLL_TIMEOUT
    } __PACKED SleepType;

    typedef struct
    {
        MSGType msgtype;
        uint_8 LOCX_L;
        uint_8 LOCX_H;
        uint_8 LOCY_L;
        uint_8 LOCY_H;
        uint_8 LOCZ_L;
        uint_8 LOCZ_H;
    } __PACKED app_Loc_t;

    typedef struct
    {
        MSGType msgtype;
        uint_8 len;
        //uint_8 *data;  //gas_type(1 byte)+gas_density(1byte)+...
        //data is followed.
    } __PACKED app_Gas_t;

    typedef struct
    {
        MSGType msgtype;
        uint_8 len;
        uint_16 blk;
        uint_16 srcnbr;
        uint_16 dstnbr;
        //uint_8* data;
        //data is followed.
    } __PACKED app_Voice_t;

#define VERSION_MAX_CHARS   14
    typedef struct
    {
        MSGType msgtype;   //REPORT_STATION
        uint_16 srcPanID;
        uint_16 dstnbr;
        uint_8  len; //len <=14
        uint_8 version[VERSION_MAX_CHARS];
    } __PACKED app_Report_t;

    typedef struct
    {
        MSGType msgtype;
        uint_8 blastEnable;
        uint_8 blastCnt;
        uint_16 srcAddr;
        uint_16 dstAddr;
        uint_16 crc;
        int_8   rssi;
    } __PACKED app_BlastRequest_t;

    typedef enum
    {
        RETREAT, //Retreat signal sending out from control center.
        CANCELRETREAT, //Cancel retreat signal sending out from control center.
        NOPWR, //Battery power lack signal sending out from card.
        ALERT, //Alert signal sending out from cardholder by pushdown button.
        ALERTACK, //ACK signal of ALERT, from coord to card
        URGENT_NONE = 0xFF,
    } __PACKED UrgentType;

    typedef enum
    {
        SSREQ_OUT = 0x01,  //0x01 card->fixed node.
        SSREQ_POLL,  //0x02 Request message card ->fixed node
        SSREQ_NODATA,  //0x03 Ack message fixed node -> card

        SSREQ_CHANGETOF = 0xF0, // station tell cards to change to tof mode
    } __PACKED SSreqType;

    /* for DISHUIYAN fking requirement*/

#define URGENT_NOPWR_REALNOPWR      0
#define URGENT_NOPWR_RECVRETREAT   0xFD
#define URGENT_NOPWR_KEYCONFIRM     0xFE

    typedef struct
    {
        MSGType msgtype;
        uint_8 urgenttype;
        uint_8 value;              //if no power, fill battery, unit is 0.1v
    } __PACKED app_Urgent_t;

#define SPI_RETREAT				0
#define SPI_CANCEL_RETREAT		1
#define SPI_RETREAT_ACK			2
#define SPI_CANCEL_RETREAT_ACK	3

    // only used by card retreat/cancelretreat.
    typedef struct
    {
        MSGType	msgtype;
        uint_8	cmdtype;
        uint_16	crc;
    } __PACKED app_spicmd_t;
    typedef struct
    {
        MSGType msgtype;
        SleepType sleeptype;
        uint_16    value;
        //uint_16  sleeptime;
        //uint_8    poll_interval;
        //uint_8    poll_timeout;
    } __PACKED app_Sleep_t;

    typedef struct
    {
        MSGType msgtype;
        uint_8 workmode;
    } __PACKED app_Mode_t;

    typedef app_Gas_t app_GasThresh_t;

    typedef struct
    {
        MSGType msgtype;
        uint_8 reqtype;
        uint_16 NWK_ADDR;
        uint_16  seqnum;
    } __PACKED app_ssReq_t;



    typedef struct
    {
        uint_16 NWK_ADDR; //subnet address.
        uint_16 NODE_ADDR; //nwk address of node in a subnet.
        uint_16 seqnum;
        int_8 rssi;
    } __PACKED rssi_pkt;

    typedef struct
    {
        MSGType msgtype;
        //int_8 RSSI;
        rssi_pkt rssipkt;
    } __PACKED app_ssInd_t;

    typedef struct
    {
        MSGType msgtype;
        uint_8 reqtype;
        uint_16 NWK_ADDR;
        uint_16  seqnum;
        termNbr_t nmbr;
    } __PACKED app_MPssReq_t;

    typedef struct
    {
        MSGType msgtype;
        termNbr_t nmbr;
        rssi_pkt rssipkt;
    } __PACKED app_MPssInd_t;

    typedef struct
    {
        MSGType msgtype;
        uint_8 workpower; //unit dbm
    } __PACKED app_PWRCTL_t;

    typedef struct
    {
        MSGType msgtype;
        //uint_8 addmp[10];//PANID+64bit macaddr of ZC.
        uint_16 PANID;
        uint_8 macAddr[8];
    } __PACKED app_startNwk_t;

    typedef struct
    {
        MSGType msgtype;
        //uint_16 nodeAddr;
        uint_8 add[8];//64bit macaddr of the node.
        uint_8 capabilityInfo;
        termNbr_t srcnbr;
    } __PACKED app_JoinNwk_t;

    typedef struct //do we need send leave command time by time??
    {
        MSGType msgtype;
        uint_8 macAddr[8];
        termNbr_t srcnbr;
    } __PACKED app_LeaveNwk_t;

    #define POLL_FLAG_REQTIME 0x01  /* flags in poll_req, int bit wise*/
    #define POLL_FLAG_REQARMVERSION   0x02

    typedef struct
    {
        MSGType msgtype;
        termNbr_t nbr;
		uint_8 flag;
    } __PACKED app_MP_Poll_Req_t;



#define     MP_POLL_FLAG_NONE  0
#define     MP_POLL_FLAG_START 1
#define     MP_POLL_FLAG_END    2
#define     MP_POLL_FLAG_REJOIN 3

    typedef struct
    {
        MSGType msgtype;
        termNbr_t nbr;
        uint_8      flag;
    } __PACKED app_MP_Poll_Ack_t;

// C -> T command

    typedef enum CmdType
    {
        CMD_UP_DIALUP,
        CMD_UP_FOUND, // send by called party to ZC then to caller, to tell it the called party exist in the network.
        CMD_UP_BUSY, //send by called party to ZC then to caller, to tell it the called party is busy now.
        CMD_UP_ACCEPT,// send by terminal to ZC when user push accept button.
        CMD_UP_CLOSE, // send by terminal to ZC when user push close button.

        CMD_DOWN_CALL, // to called party
        CMD_DOWN_FOUND, // to caller, to tell it the called party exist in the network
        CMD_DOWN_BUSY,  // to caller, to tell it the called party is busy
        CMD_DOWN_NOTFOUND, // to caller, to tell it the network cannot find the called party
        CMD_DOWN_ACCEPT, // to caller, to tell it the called party accept the dialing
        CMD_DOWN_CLOSE, // to both, to tell it that the peer close the connection
    } __PACKED CmdType_t;

// T -> C command
    typedef struct
    {
        MSGType msgtype;
        CmdType_t cmd;
        termNbr_t srcnbr;
        termNbr_t dstnbr;
    } __PACKED app_CmdUp_t;

    typedef struct
    {
        MSGType msgtype;
        CmdType_t cmd;
        termNbr_t srcnbr;
        termNbr_t dstnbr;
    } __PACKED app_CmdDown_t;

    typedef struct
    {
        MSGType msgtype;
        termNbr_t srcnbr;
        termNbr_t dstnbr;
    } __PACKED app_Dialup_t;

    typedef struct
    {
        MSGType     msgtype;
        termNbr_t   srcnbr;
        uint_16     newpan;
    } __PACKED app_switchreq_t;
//typedef struct
//{
//  MSGType msgtype;
//} __PACKED app_DialAccept_t;

//typedef struct
//{
//  MSGType msgtype;
//} __PACKED app_Hangup_t;

//typedef struct
//{
//  MSGType msgtype;
//  uint_8 len;
    //uint_8* data;//several rssi_pkt.
    //data is followed.
//} __PACKED app_ssRsp_t;//Loc node will forward the rssi to arm and so on.

    typedef struct
    {
        MSGType msgtype;
        uint_8 macAddr[8];
        termNbr_t deputyNmbr; // the number of deputed node.
        uint_8 capabilityInfo; //bitmap.
    } __PACKED app_DeputyJoin_t;

    typedef struct
    {
        uint_8 extendedPANID[8];
        uint_8 extendedCoordAddr[8];
        uint_16 PanId;
        uint_8 Channel;
        uint_16 CoordAddr;
    } __PACKED NeighborPAN_t;

    typedef struct
    {
        NeighborPAN_t PANInfo;
        uint_16 shortAddr;
    } __PACKED NodeAddr;

    typedef enum
    {
        PERMIT_JOIN,
        NWK_FULL
    } __PACKED RsType_t;

    typedef struct
    {
        MSGType msgtype;
        uint_8 rsptype;
        termNbr_t deputyNmbr;
        NodeAddr nodeAddr; //The pre-distribute address for the node in the neighbor PAN before joining.
    } __PACKED app_DeputyJoinRsp_t;

    typedef struct
    {
        MSGType msgtype;
    } __PACKED app_TryConnection_t;

    typedef struct
    {
        MSGType msgtype;
        uint_8 len;
        // payload is attached here
    } __PACKED app_SignalStrength_t;

    typedef struct
    {
        MSGType msgtype;
        uint_16 year;
        uint_8   month;
        uint_8   day;
        uint_8   hour;
        uint_8   minute;
        uint_8   second;
    } __PACKED app_TimessInd_t;

    typedef struct
    {
        MSGType msgtype;
        uint_16 shortAddr;
        uint_8 macAddr[8];
        termNbr_t nmbr;
    } __PACKED app_ClearNWK_t;

    typedef struct
    {
        MSGType msgtype;
        termNbr_t nbr;
    } __PACKED app_RejoinNwk_t; //From arm to End device.

    typedef struct
    {
        MSGType msgtype;
        uint_16 blk;
        uint_8 len;
        termNbr_t nmbr;
        //SMS content is followed.
    } __PACKED app_SMS_t;

    typedef struct
    {
        MSGType msgtype;
        termNbr_t srcnmbr;
        uint_16 blk;
    } __PACKED app_SMSRsp_t;

#define ZC_REPORT_POWERON               1
#define ZC_REPORT_WATCHDOG_RESTART   2
#define ZC_REPORT_EXTERNAL_RESTART      3
#define ZC_REPORT_SPI_RESTART           4
#define ZC_REPORT_STARTNWK_FAILED_RESTART           5
#define ZC_REPORT_BLAST_ERR_RESTART         6
#define ZC_REPORT_MEMORY_ERR_RESTART        7
#define ZC_REPORT_PARTBLAST_ERR_RESTART     8

    typedef struct
    {
        MSGType msgtype;
        uint_8 flag;
        uint_16 PanId;

        // if flag = 6£¬Additional information
    } __PACKED app_ZC_Report_t;

    typedef struct
    {
        unsigned char msgtype;
        uint_16 srcPan;
        uint_16 dstPan;
        uint_16 seqnum; // do not change for each split packet
        uint_8 packnum;
        uint_8 packseq;
        uint_16 len;
        //data is followed.
    } __PACKED app_CrossPan_t;

    typedef struct
    {
        MSGType msgtype;
    } __PACKED app_ZcDetect_t;

    typedef enum SignalType
    {
        DIALUP_ACK = 1,
        CMD_UP_ACCEPT_ACK,
        CMD_UP_FOUND_ACK,
        CMD_UP_CLOSE_ACK,
        CMD_DOWN_CALL_ACK,
        CMD_DOWN_FOUND_ACK,
        CMD_DOWN_BUSY_ACK,
        CMD_DOWN_NOTFOUND_ACK,
        CMD_DOWN_ACCEPT_ACK,
        CMD_DOWN_CLOSE_ACK,
        CMD_UP_BUSY_ACK,
        DEPUTYJOIN_RSP_ACK,
    } __PACKED SignalType_t;

    typedef struct
    {
        MSGType msgtype;
        uint_8 signaltype;
        termNbr_t nbr;
    } __PACKED app_Signal_ACK_t;

    typedef struct
    {
        MSGType msgtype;
        uint_8 detectdata[SPIDETECT_LEN];
    } __PACKED app_SPIDetect_t;

    typedef struct
    {
        MSGType msgtype;
        uint_8  debugstrlen;
        //data is following.
        //uint_8 debugstr[DEBUG_STRLEN];
    } __PACKED app_SPIDebug_t;


#define   RFDEBUG_CMD_REQ       1
#define   RFDEBUG_CMD_ACK        2

    typedef enum
    {
        RFDEBUG_TYPE_NIB = 1,
        RFDEBUG_TYPE_MACPIB,
        RFDEBUG_TYPE_ASSOCDEVS,
        RFDEBUG_TYPE_READMEM,
        RFDEBUG_TYPE_WTIREMEM,
        RFDEBUG_TYPE_RESETNLME,
        RFDEBUG_TYPE_RESETMLME,
        RFDEBUG_TYPE_RESETMACLOW,
    } RFDebug_type_t;

    typedef struct
    {
        MSGType msgtype;
        uint_8     type;
        uint_8     cmd;
        uint_16     startAddr;
        uint_16     Addrlen;
        uint_8       seq;
        uint_8  len;
        //data is following.
    } __PACKED app_RFDebug_t;

    typedef struct
    {
        uint_16 LocNode_ID;
        int_8 rssi;
    } __PACKED LocPos_t;

    typedef enum
    {
        NONE_TYPE = 0,
        GAS_CH4 = 1,
        GAS_TEMPR,
    } __PACKED ReportType;

    typedef struct
    {
        MSGType msgtype;
        uint_16 DevID;
        uint_16 dstPan;
        ReportType GasType;
        uint_16 GasDensity;
        ReportType Type2;
        uint_16 value2;
        ReportType Type3;
        uint_16 value3;
        uint_16 seq;
        uint_16 GasThr;
        int_8 LocCnt;//count of LocPos_t.
        //LocPos_t data is following, and the size is LocCnt*sizeof(LocPos_t).
    } __PACKED app_GasReport_t;

    typedef struct
    {
        MSGType msgtype;
        uint_8 vol; //unit: 0.1v, for locNode range is 0-50, for station is 0xFF
        uint_8 seq;
        uint_16 DevID;
    } __PACKED app_LocNodeCast_t;


    typedef enum
    {
        //LOCNODE_LOW_BATTERY = 1,  // move to LOCNODE_ALARM
        GASNODE_URGENT = 2,
        GASNODE_POWERON,
        GASNODE_POWEROFF,

        /* accident alarms */
        GASNODE_ALARM_WATER,
        GASNODE_ALARM_FIRE,
        GASNODE_ALARM_TOPBOARD,
        GASNODE_ALARM_OTHERS,

    } __PACKED GasAlarmType;

#define GASNODE_POWEROFF_SHUTDOWN   0
#define GASNODE_POWEROFF_NOPOWER     1
#define GASNODE_POWEROFF_OVERDENSITY 2

    typedef struct
    {
        MSGType msgtype;
        uint_16 DevID;
        uint_16 DstPan;
        uint_16 seq;
        GasAlarmType AlarmType;
        uint_8 value;
    } __PACKED app_GasAlarm_t;

    typedef struct
    {
        MSGType msgtype;
        uint_16 DevID;
        uint_16 SrcPan;
        uint_16 seq;
        GasAlarmType AlarmType;
    } __PACKED app_GasAlarm_ack_t;

    typedef struct
    {
        MSGType msgtype;
        uint_16 srcpan;
        uint_16 dstpan; // 0xFF for broadcast
        uint_8  amid;   // 0xFF for query all
    } __PACKED app_amquery_t;

    typedef struct
    {
        MSGType msgtype;
        uint_16 srcpan;
        uint_16 dstpan;
        uint_8  amnum;  // ammeter number followed
        // ammeter ID followed;
    } __PACKED app_amret_t;

    typedef app_CrossPan_t app_amdata_t;
    typedef app_CrossPan_t app_amdata_cmd_t;

    typedef struct
    {
        //FIXME, not good practice
        unsigned char msgtype;
        uint_16 srcpan;
        uint_16 dstpan;
        uint_16 seqnum;
        uint_8 packnum;
        uint_8 packseq;
    } __PACKED app_CrossPanAck_t;

    typedef app_CrossPanAck_t app_amack_t;

#define AMMETER_ROUTERPT_TYPE_MSG   0
#define AMMETER_ROUTERPT_TYPE_BEGIN  1
#define AMMETER_ROUTERPT_TYPE_END   2

    typedef struct
    {
        MSGType msgtype;
        uint_8    reporttype;   // 0:message  1:begin  2:end
        uint_16  nextHopAddress;
        uint_16  dstAddress;
        uint_8    expiryTime;
    } __PACKED app_am_routerpt_t;

    typedef struct
    {
        MSGType msgtype;
        uint_8 len;         // message content length
        uint_16 srcpan;
        uint_16 dstpan;
        uint_16 seqnum;
        termNbr_t nmbr;
        char content[1];    // content of message
    } __PACKED app_gassms_t;

    typedef struct
    {
        MSGType msgtype;
        uint_16 srcpan;
        uint_16 dstpan;
        uint_16 seqnum;
        termNbr_t nmbr;
    } __PACKED app_gassms_ack_t;

    typedef struct
    {
        MSGType msgtype;
        uint_8 reqtype;
        uint_16 srcPan;
        uint_16 seqnum;
        uint_8  LocCnt;//count of LocPos_t.
        //LocPos_t data is following, and the size is locnum*sizeof(LocPos_t).
    } __PACKED app_chargeed_ssReq_t;

    typedef struct
    {
        MSGType msgtype;
        uint_16   srcPan;
        uint_8  locnode_num;
        uint_16  seqnum;
        uint_8  urgent_type;  //UrgentType
        uint_8  urgent_value;
    } __PACKED app_chargeed_ssRsp_t;

    typedef struct
    {
        MSGType msgtype;
        rssi_pkt rssipkt;
        uint_8  LocCnt;//count of LocPos_t.
        //LocPos_t data is following, and the size is LocCnt*sizeof(LocPos_t).
    } __PACKED app_chargeed_ssInd_t;

    typedef struct
    {
        MSGType msgtype;
        uint_16 DevID;
        uint_16 seqnum;
    } __PACKED app_locnode_info_req_t;

    typedef struct
    {
        MSGType msgtype;
        uint_16 DevID;
        uint_16 seqnum;
        uint_8  LocCnt; //count of locnode
        // the locNode IDs are attached here,type is uint_16
    } __PACKED app_locnode_info_set_t;

    typedef enum
    {
        LOCNODE_LOW_BATTERY = 1,  // move to LOCNODE_ALARM
    } __PACKED LocNodeAlarmType;

    typedef app_CrossPan_t app_dataCrossPan_t;
    typedef app_Gas_t app_RouteInfo_t;


    typedef struct
    {
        MSGType msgtype;
        uint_16 DevID;
        uint_16 DstPan;
        uint_8 seq;
        LocNodeAlarmType AlarmType;
        uint_16 LocNodeID;
        uint_8 value;
    } __PACKED app_LocNodeAlarm_t;

    typedef struct
    {
        MSGType msgtype;
        uint_8 seq;
        uint_16 size;
        // followed by version info
    } __PACKED app_rfversion_t;

    typedef struct
    {
        MSGType msgtype;
        uint_8 seq;
    } __PACKED app_rfmac_query_t;

    typedef struct
    {
        MSGType msgtype;
        uint_8 seq;
        uint_8 macAddr[8];
        uint_16 crc;  // CRC for macAddr
    } __PACKED app_rfmac_set_t;


#define APP_RFREPORT_TYPE_ADDRLIST   1

#define APP_RFREPORT_TAG_MSG    1
#define APP_RFREPORT_TAG_BEGIN  2

    typedef struct
    {
        MSGType msgtype;
        uint_8 type;
        uint_8 tag;
        uint_8 seqnum;
        uint_8 len;
        // content  is attached here
    } __PACKED app_rfreport_t;


/* for card type */
#define APP_CARD_TYPE_BATTERY_2430  0
#define APP_CARD_TYPE_BATTERY_2530  1
#define APP_CARD_TYPE_CHARGE_2430   2
#define APP_CARD_TYPE_CHARGE_2530   3

    typedef struct
    {
        MSGType msgtype;
        uint_8 cardtype;
        uint_8 batteryvalue;          // uint is 0.1v
        uint_8 primaryIEEEaddr[8];
        uint_8 seqnum;
        uint_8 len;
        // software version is attached here
    } __PACKED app_card_status_t;


typedef struct
{
	MSGType msgtype;
	uint_16 srcAddr;
	uint_16 dstAddr;
	uint_16 seqnum;
	uint_8 packnum;
	uint_8 packseq;
	uint_16 len;
	//data is followed.
} __PACKED app_SanyData_t;

typedef struct
{
	//above is data
	uint_16 data_len;       //it is the can bus data length
	uint_16 crc_code;
}__PACKED app_SanyCRC_t;
#define APP_SANY_STATUS_VERSION 1

typedef struct
{
	MSGType msgtype;
	uint_8 statustype;
	uint_16 srcAddr;
	uint_16 dstAddr;
	uint_16 len;
	//data is followed.
} __PACKED app_SanyStatus_t;

#define     APP_TIMAC_JOINNWK_REQ       1
#define     APP_TIMAC_JOINNWK_SUCCESS   2
#define     APP_TIMAC_JOINNWK_DENIED    3
#define     INVALIDARMADDR 0xFFFF

#define     APP_TIMAC_STATUS_IDLE	     1
#define     APP_TIMAC_STATUS_ONCMD	     2
#define     APP_TIMAC_STATUS_ONVOICE     3


    typedef struct
    {
        MSGType  msgtype;
        uint_8       joinnwktype;
        uint_8       seqnum;
        uint_16     panid; // when joinnwktype==req, it is the last panid; when joinnwktype==success, it is the current panid
        uint_8	    status; // mobile status, idle, oncmd, onvoice
        termNbr_t srcnbr;
    } __PACKED app_Timac_JoinNwk_t;

    typedef struct
    {
        MSGType msgtype;
        uint_8       seqnum;
        termNbr_t srcnbr;
    } __PACKED app_Timac_LeaveNwk_t;

    /* app_mpArmid_t.req -> app_mpArmid_t.ack */
#define     APP_SCAN_TYPE_REQ  1
#define     APP_SCAN_TYPE_ACK  2
    typedef struct
    {
        MSGType  msgtype;
        uint_8        scantype;
        uint_8        seqnum;
    } __PACKED app_Timac_Mp_Scan_t;

    typedef enum
    {
        spiAddrNotPresent = _AddrNotPresent,
        spiAddr16Bit      = _Addr16Bit,
        spiAddrGroup      = _AddrGroup,
        spiAddrBroadcast  = _AddrBroadcast
    } __PACKED AddrMode_t;

    typedef struct
    {
        union
        {
            uint_16  shortAddr;
        } __PACKED addr;
        AddrMode_t addrMode;
        uint_8 endPoint;
    } __PACKED spiAddrType_t;  //the same to afAddrType_t.

    typedef struct
    {
        uint_8  event;
        uint_8  status;
    } __PACKED spi_event_hdr_t; //the same to osal_event_hdr_t.

    typedef struct
    {
        spi_event_hdr_t hdr;//osal msg identifier.
        spiAddrType_t srcAddr; //filled by Zigbee node.
        spiAddrType_t dstAddr; //need filled by ARM.
        uint_8 transID; //A pointer to a byte which can be modified and which will
        //be used as the transaction sequence number of the msg.
        uint_8 options; //Valid bit mask of Tx options.
        uint_8 radius; //transmit radius by hops.
    } __PACKED spi_hdr_t;

    typedef struct
    {
        spi_hdr_t spihdr;
        uint_16 DataLength; //app data length.
        //app content for ARM is followed: MSGType+realdata.
    } __PACKED SPIMSGPacket_t;


typedef enum{
	CMDTYPE_STOP=1,
	CMDTYPE_DEMAND,
} __PACKED ConveyorCmdType_t;
typedef struct
{
	MSGType 		msgtype;
	unsigned char 	num;
	unsigned char 	cmdtype;
	unsigned char 	reserve;
	unsigned short	blk;
	unsigned short	crc;
} __PACKED app_Conveyor_cmd_t;

typedef enum {
	INFOTYPE_STOP_ACK,
	INFOTYPE_START,
	INFOTYPE_STARTED,
	INFOTYPE_MALFUNCTION,
	INFOTYPE_STATUS,
	INFOTYPE_NOCONVEYOR,
	INFOTYPE_NOREACTION,
} __PACKED ConveyorInfoType_t;
typedef struct
{
	MSGType 	msgtype;
	unsigned char num;
	unsigned char infotype;
	unsigned char status;
	unsigned char malfunction_type;
	unsigned char malfunction_msg;
	unsigned char speed;
	unsigned char tensility;
	unsigned short reserve1;
	unsigned short reserve2;
	unsigned short blk;
	unsigned short crc;
} __PACKED app_Conveyor_info_t;

/* begin protocal of warehouse goods locating system */
typedef struct
{
	MSGType msgtype;
	uint_16 srcAddr;
	uint_16 dstAddr;
	uint_8  seqnum;
	uint_8  len;
} __PACKED app_LocSys_Data_t;

typedef struct
{
	MSGType msgtype;
	uint_16 srcAddr;
	uint_16 dstAddr;
	uint_8  seqnum;
	uint_8  len;
} __PACKED app_LocSys_Cmd_t;

enum
{
	LOCSYSCNF_SUCCESS,
	LOCSYSCNF_BUFFER_FULL,
	LOCSYSCNF_PARAM_INVALID,
};
typedef struct
{
	MSGType msgtype;
	uint_16 srcAddr;
	uint_16 dstAddr;
	uint_8  seqnum;
	uint_8  status;
} __PACKED app_LocSys_Cnf_t;

typedef struct {
	uint_8	   type;
	uint_8	   ttl;
	uint_16    src;
	uint_16    dst;
	uint_16    nwklen;
	//------------
	uint_8	   protocoltype;
	uint_8	   msgtype;
	uint_16    len;
	//------------
	uint_16    u16ShortAddr;
	uint_8	   u8Status;
	uint_8	   u8ExciterID;
}__PACKED app_Exciter_Info_t;


/* end protocal of warehouse manage system */

#ifdef __cplusplus
}
#endif


//The macro should be the same with the msg type in cc2430 for spi.(in ZComDef.h)
//as the pre code for spi communication between ARM and cc2430.(the first byte of a buffer.)
#define SPI_PRE  0xE1

//simply fetch the size of pkt.
//Define an array in appprotocolwrapper instead of the macro to improve access rate.
/*#define GET_PKT_SIZE(SIZE, TYPE) \
do {\
    if ((TYPE) == LOCATION) \
        (SIZE) = sizeof(app_Loc_t); \
    else if ((TYPE) == GASREPORT)  \
        (SIZE) = MAX_VARLEN; \
    else if ((TYPE) == GASTHRESH)  \
        (SIZE) = MAX_VARLEN; \
    else if ((TYPE) == VOICE)  \
        (SIZE) = MAX_VARLEN; \
    else if ((TYPE) == MODE)  \
        (SIZE) = sizeof(app_Mode_t); \
    else if ((TYPE) == URGENT)  \
        (SIZE) = sizeof(app_Urgent_t); \
    else if ((TYPE) == NODESLEEP)  \
        (SIZE) = sizeof(app_Sleep_t); \
    else if ((TYPE) == SSREQ)  \
        (SIZE) = sizeof(app_ssReq_t); \
    else if ((TYPE) == SSIND)  \
        (SIZE) = sizeof(app_ssInd_t); \
    else if ((TYPE) == PWRCTL)  \
        (SIZE) = sizeof(app_PWRCTL_t); \
    else if ((TYPE) == ZB_START_NETWORK)  \
        (SIZE) = sizeof(app_startNwk_t); \
    else if (TYPE == ZB_JOIN_NOTIFY)  \
        (SIZE) = sizeof(app_JoinNwk_t); \
    else if ((TYPE) == ZB_LEAVE_NOTIFY)  \
        (SIZE) = sizeof(app_LeaveNwk_t); \
    else if ((TYPE) == ZB_DIAL_UP)  \
        (SIZE) = sizeof(app_Dialup_t); \
    else if ((TYPE) == ZB_HANG_UP)  \
        (SIZE) = sizeof(app_Hangup_t); \
}while(0)
*/

#endif

