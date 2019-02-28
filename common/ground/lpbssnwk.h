#ifndef _LPBSSNWK_H_
#define _LPBSSNWK_H_

#ifndef __PACKED
  #if (defined __ARM32__) || (defined __CC_ARM) || (defined JENNIC_CHIP) || (defined __GNUC__)
    #define __PACKED __attribute__((__packed__))
  #else
    #define __PACKED
  #endif
#endif

// one byte alignment in Win32 and Wince app
#if (defined(WIN32) || defined(WINCE))
#pragma pack(push)
#pragma pack(1)
#endif

/*
**  network panid
**  use type check, (if type error that get a warning)
**************************************************************/
#define     DEVICE_CARD_PANID               0xFFF6
#define     STAFF_CARD_PANID                0xFFF7
#define     READ_STATION_PANID    	        0xFFF8
#define     POS_STATION_PANID               0xFFF9
#define     SOS_CARD_PANID                  0xFFFA
#define     MATTER_ALERT_PANID              0xFFFB


#define NWK_BROADCAST_ADDR              0xFFFF

#define LPBSS_MAX_CARD_CMD_RECORDS 100

/*
**  LPBSS Nwk header + LPBSS  Payload
*/
typedef struct
{
    unsigned short 	u16SrcAddr;       // Դ�豸��ַ
    unsigned short 	u16DstAddr;       // Ŀ���豸��ַ
    unsigned char 	u8SrcDevType;     // Դ�豸����
    unsigned char 	u8DstDevType;     // Ŀ���豸����
    unsigned char  u8Model;           // ������������(��λд��mac addr����ģ����������ǵ������ߵĻ�վ)
    unsigned char  u8Padding1;
    unsigned short u16Padding2;
    unsigned short	u16Len;           // ����LPBSS payload ���ݳ���
}__PACKED LPBSS_nwk_header_t;

//device Type
typedef enum
{
    ANALYSIS_DEVICE_ID = 0x00,        // analysis programme
    STATION_CONTROL_DEVICE_ID = 0x01, // station control
    READSTATION_DEVICE_ID = 0x02,     // read-card station
    STAFF_CARD_DEVICE_ID = 0x03,      // Staff card
    DEVICE_CARD_DEVICE_ID = 0x04,     // device card
    HAND_MACHINE_DEVICE_ID = 0x05,    // hand-machine
    WRIST_CARD_DEVICE_ID = 0x06,      // wrist card
    STM32F107POE_DEVICE_ID = 0x07,
    MATESS_ALERTOR_DEVICE_ID = 0x08,   //����
    SOS_DEVICE_ID = 0x09,              //������
    BADGE_DEVICE_ID = 0x0A,            //����
    ASSET_CARD_DEVICE_ID = 0x0B,        //�ʲ���λ��
}LPBSS_device_ID_e;


typedef  struct
{
	unsigned char u8Msg;
	unsigned char u8Padding;
	unsigned short u16len;
}__PACKED LPBSS_Msg_Header_t;

//Msg type
enum
{
    LPBSS_VERSION  = 0x00,   //station'sversion(staff station and device station)

    LPBSS_CARD_LOC_DATA     = 0x01,

    //LPBSS_DEV_LOC    = 0x11,
    LPBSS_DEV_INFO   = 0x12,
    LPBSS_DEV_SET    = 0x13,
    LPBSS_DEV_CARD_SCH = 0x14,    //include:serch or cancel serch
    LPBSS_CARD_VERSION = 0x15,
    LPBSS_CARD_TIME    = 0x16,
	LPBSS_CARD_SMS     = 0x17,

    LPBSS_JMP_TOPO = 0x30
};

// upload card(device or staff)rssi
typedef  struct
{
    unsigned short u16ShortAddr;
    unsigned short u16Seqnum;
	unsigned char u8Device;
    signed char  Rssi;
    unsigned char u8Battery;
    unsigned char u8SoftVer;
    unsigned char u8Status;
    unsigned char u8IsCardAlarm;  // 1:card's alarming;0:cancal alarm
    unsigned char u8Model;
    unsigned char u8Padding;
} __PACKED LPBSS_card_data_t;

typedef  struct
{
    unsigned short u16ShortAddr;
    unsigned short u16Seqnum;
	unsigned char u8Device;      //�ʲ���
    signed char  Rssi;
    unsigned char u8Battery;
    unsigned char u8Status;
    unsigned short u16ExciteID;
    unsigned char u8IsCardAlarm;  // 1:card's alarming;0:cancal alarm
    unsigned char u8Model;
} __PACKED ASSET_card_data_t;


typedef  struct
{
    unsigned short u16ShortAddr;
	unsigned char u8Device;
    unsigned char u8Len;
    unsigned short u16padding;
    unsigned char  u8VerOffset;
    unsigned char  u8ReleaseOffset;

}__PACKED LPBSS_card_ver_t;


typedef struct
{
    unsigned short  year;
    unsigned char   month;
    unsigned char   day;
    unsigned char   hour;
    unsigned char   minute;
    unsigned char   second;
    unsigned char   padding;
}__PACKED LPBSS_card_time_t;

typedef struct
{
	 unsigned char		   u8dstType;	 //; 0: station    1:card
	 unsigned char		   u8padding;
	 unsigned short 	   u16dstID;
	 unsigned short 	   u16seqnum;
	 unsigned short 	   u16len;
	//SMS content is followed.
}  LPBSS_SMS_Header_t;

// �豸����Ϣ�ֳ�����:�����Ϣbasic��������Ϣ
typedef struct
{
    unsigned char  u8WorkType;
    unsigned char  u8padding1;
    unsigned char  u8padding2;
    unsigned char   u8Len;
} __PACKED LPBSS_dev_card_info_t;


//upload or download device card's info
typedef struct
{
    unsigned short u16ShortAddr;
    unsigned short u16Seqnum;
	unsigned char u8Device;
	unsigned char u8IsChangeInfo;
    //uint_8 u8Len;
    unsigned short u16CRC;

    LPBSS_dev_card_info_t DevCardInfo;
}__PACKED LPBSS_devicecard_info_t;

typedef struct
{
    unsigned char u8WorkType;
    unsigned char u8Padding;     //oxff
}__PACKED LPBSS_card_type_t;


typedef union
{
    unsigned short  u16DstAddr;   //0~65000
    LPBSS_card_type_t CardSearchType;
}__PACKED LPBSS_card_search_t;


typedef struct
{
	LPBSS_card_search_t CardSearch;
    unsigned char u8Cmdtype;
    unsigned char u8SearchType;
}__PACKED LPBSS_card_search_record_t;

//search card cmdtype
enum
{
    LPBSS_SEARCH_CARD_ALERT         = 0x01,
    LPBSS_SEARCH_CARD_CANCEL_ALERT  = 0x02,
};

//SearchType
enum
{
    LPBSS_SEARCH_CARD_BY_ADDR = 0,
    LPBSS_SEARCH_CARD_BY_TYPE = 1,
};

/*******************************************************************************
*    ��LF��  <======>  �տ���|�ֳֻ�(LF����)  <========> app
*/
/*
 *    MARCO
 */
#define  MAC_MAX_FRAME_SIZE        102
#define  WIRE_SYNC "YIRI"
#define  WIRE_SYNC_SIZE	4
#define  MAX_EDEV_DATA_LEN      (MAC_MAX_FRAME_SIZE -sizeof(app_eDev_hdr_t)-sizeof(app_eDev_Data_t)-1)


typedef enum
{
    // start   0x80
    DETECT = 0x80,
    DETECT_ACK,
    WRITE_CARD,
    REPORT_VERSION,

    //MSGWirelessType
    READ_DATA = 0x90,
    READ_DATA_ACK,
    READY,
    WRITE_DATA,
    WRITE_DATA_ACK,
    REMOTE_DATA,
    SHORT_DISTANCE_CHECK,
    LONG_DISTANCE_CHECK,


    CHARGE_PERIOD,     //�޸Ĺ�������  0x98
    READ_INFO,          //��������
    WRITE_INFO,         //д������
    WRITE_CHANNEL,
    CHARGE_CARD_TYPE,   //�޸Ŀ�ƿ�����:5�뿨��1�뿨��15�뿨

    CARD_TYPE_CHECK,    //5s����1s����15s��
    STATION_RSSI_CHECK,

    CHARGE_CARD_TYPE_ACK,
    GET_STATION_RSSI,      //A0   �����ֳֻ�Ƶ��ͬʱ���Ի�վ�ź�
    GET_STATION_RSSI_ACK,

    SET_RSSI_CHANNNEL,
    SET_RSSI_CHANNNEL_ACK,
    POS_RSSI_CHANNNEL,
    CARD_LOC_DISTANCE,
	STATION_CHANNEL,      //appͨ����msg��ȡ�ֳֻ�Ƶ�����ֳ��ϱ�Ƶ��Ҳ��ͨ����msg
	SET_STATION_CHANNEL,  //�����ֳֻ�Ƶ��

	TEST_STATION_RSSI = 0xAA, //ͬA0�����Ի�վ�źŵ����޸�Ƶ��
	TEST_STATION_RSSI_ACK,

    DEVICE_CARD_READ = 0xAC,
    DEVICE_CARD_READ_ACK =0xAD,
    DEVICE_CARD_SETTING = 0xAE,  
    DEVICE_CARD_SETTING_ACK = 0xAF,
    DEVICE_CARD_ERPID = 0xB0,
} LPBSS_LF_WR_MSGType ; //__PACKED

typedef enum
{
    BASEINFO= 0x00,
    UNKNOWN,
    REMARKINFO,
} DataType ;

/******************************************************/

typedef struct
{
    unsigned char   sync[WIRE_SYNC_SIZE];
}app_mDev_hdr_t;

typedef struct
{
    unsigned short  padding;
    unsigned short  crc;
}app_mDev_fdr_t;

typedef struct
{
    unsigned char   MSGType;
    unsigned char   eDevType;
    unsigned short  eDevID;
    unsigned short  mDevID;
    unsigned short  dataLen;
}app_eDev_hdr_t;

typedef struct
{
    signed char   rssi;
    unsigned char   rssi_channel;
    unsigned short   battery;
    unsigned char   period;
    unsigned short   ver_oad;
    unsigned char    erp_id[20];
}app_report_hdr_t;

typedef struct
{
    unsigned char   frameSum;
    unsigned char   frameSeq;
    unsigned char   datatype;
    unsigned char   len;
    //data
}app_eDev_Data_t;

//|app_mDev_hdr_t|app_eDev_hdr_t|app_eDev_Data_t|app_mDev_fdr_t|

/*****************************************************************/

typedef struct
{
    unsigned char   worktype;
    unsigned short  len;
    //DATA
}app_eDev_BaseInfo_t;

typedef struct
{
    unsigned char  type;
    unsigned char  len;
} __PACKED app_rfTlv_t;


#if (defined(WIN32) || defined(WINCE))
#pragma pack(pop)
#endif

/********************************END*****************************************/
#endif
