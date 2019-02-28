/*
 * RadioProto.h
 * This header file is designed for definitions of type and data format
 * which are used for transceiving data on Radio using 2.4G-Wireless protocol.
 *
 */
#ifndef _RADIO_PROTO_H_
#define _RADIO_PROTO_H_

/*******************************************************************************
*       RADIO PROTOCOL, Macros defined
*******************************************************************************/
#ifndef __PACKED
  #if (defined __ARM32__) || (defined __CC_ARM) || (defined JENNIC_CHIP) || (defined __GNUC__)
    #define __PACKED __attribute__((__packed__))
  #else
    #define __PACKED
  #endif
#endif

/***********************************************************
*             LF PROTOCOL
***********************************************************/
#define LF_DATA_LEN                 5
#define RECEIVE_MATCH_BYTE          0x55
#define LF_TO_CARD_LEN             (LF_DATA_LEN+1)//match data + LF data

/**********************************************************
 *
 */
#define LF_CMD_READ       0x01
#define LF_CMD_WRITE      0x02
#define LF_TYPE_STAFF     0x10
#define LF_TYPE_DEV       0x20
/**********************************************************/

#define LF_DATA_LENORMATCH   0
#define LF_DATA_CMDANDTYPE   1//H:type   L:cmd
#define LF_DATA_DEVID_H      2
#define LF_DATA_DEVID_L      3
#define LF_DATA_CARDID_H     4
#define LF_DATA_CARDID_L     5
/*
 *
 **********************************************************/

/**************************************************************************************************
*       RADIO PROTOCOL, type defined
**************************************************************************************************/
/*
**  report interval time
********************************************/
#ifndef REPORT_DELAY
#define REPORT_DELAY    2
#define REPORT_DELAY_V1  2
#define REPORT_DELAY_BADGE 10
#define REPORT_DELAY_SOS   600
#define REPORT_DELAY_CUSHION 300
#endif

/*
**  IEEE MAC address
********************************************/
/*
R: reserve
H: high 4-bit
M: middle 4-bit
L: low 4-bit
H to L form a unit
      ---------------------------------
Index | 7 | 6 | 5 | 4 | 3 | 2 | 1 | 0 |
BYTE  |H M|L H|H M|M L|R R|H L|R R|R R|
      ---------------------------------
********************************************/
#define LPBSS_MAC_RESERVE0              0
#define LPBSS_MAC_RESERVE1              1
#define LPBSS_MAC_CHA                   2
#define LPBSS_MAC_MODEL                 3
#define LPBSS_MAC_DEVID_L8BIT           4
#define LPBSS_MAC_DEVID_M8BIT           5
#define LPBSS_MAC_DEVID_H4BIT           6
#define LPBSS_MAC_BATCH_NUM_L4BIT       6
#define LPBSS_MAC_BATCH_NUM_H8BIT       7
#define LPBSS_MAC_CARD_TYPE         LPBSS_MAC_RESERVE1

/* ground system use channel 20 */
#define LPBSS_MAC_CHA_DEFAULT           20

#define LPBSS_CARD_DESC_LEN             50

/*
** device number
********************************************/
#define LPBSS_DEVID_MIN                 1
#define LPBSS_DEVID_MAX                 65000
#define LPBSS_IS_DEVID(x)  ((x)>=LPBSS_DEVID_MIN && (x)<=LPBSS_DEVID_MAX)

typedef enum
{
    STAFF_STATUS_NOMAL = 0x00,        // ����
    STAFF_STATUS_HELP = 0x01,         // ���
    STAFF_STATUS_HUNGER = 0x02,       // �͵�
    STAFF_STATUS_IMPEL = 0x04,        // ����
    STAFF_STATUS_BAND_DISCONNECT = 0x08, //����Ͽ�
}STAFF_CARD_STATUS_E;

typedef enum
{
    WRIST_STATUS_NOMAL = 0x00,        // ����
    WRIST_STATUS_HELP = 0x01,         // ���
    WRIST_STATUS_HUNGER = 0x02,       // �͵�
    WRIST_STATUS_IMPEL = 0x04,        // ����
    WRIST_STATUS_BAND_DISCONNECT = 0x08, //����Ͽ�
    WRIST_STATUS_MOTION = 0x10,          //�˶�
    WRIST_STATUS_CHARGING = 0x20,        //���
    WRIST_STATUS_DIEDAO = 0x40,        //����
    WRIST_STATUS_POLL = 0x80,        //�ȴ�����
}WRIST_CARD_STATUS_E;


typedef enum
{
    DEV_STATUS_NOMAL = 0x00,        // ����
    DEV_STATUS_SEARCH = 0x01,       // ���ҵ�
    DEV_STATUS_HUNGER = 0x02,       // �͵�
    DEV_STATUS_IMPEL = 0x04,        // ����

    DEV_STATUS_FAULT = 0x10,        // ����
    DEV_STATUS_WAIT = 0x20,         // �ȴ�ά��
    DEV_STATUS_REPAIR = 0x30        // ��ά��
}DEV_CARD_STATUS_E;

typedef enum
{
    ASSET_STATUS_NOMAL = 0x00,        // ����
    //ASSET_STATUS_SEARCH = 0x01,       // ���ҵ�
    ASSET_STATUS_HUNGER = 0x01,       // �͵�
    ASSET_STATUS_IMPEL = 0x02,        // ����
}ASSET_CARD_STATUS_E;



typedef enum
{
    SOS_STATUS_NOMAL = 0x00,        // ����
    SOS_STATUS_HELP = 0x01,         // ���
    SOS_STATUS_HUNGER = 0x02,       // �͵�
}SOS_CARD_STATUS_E;

typedef enum
{
    BADGE_STATUS_NOMAL = 0x00,        // ����
    BADGE_STATUS_HUNGER = 0x02,       // �͵�
    BADGE_STATUS_CHARGING = 0x20,        //���
    BADGE_STATUS_POLL = 0x80,        //�ȴ�����
}BADGE_CARD_STATUS_E;

typedef enum
{
    MATTESS_STATUS_SIT = 0x01,         //2��ʾ��������
    MATTESS_STATUS_HUNGER = 0x02,      //�͵�
    MATTESS_STATUS_3 = 0x04,
    MATTESS_STATUS_4 = 0x08,
    MATTESS_STATUS_5 = 0x10,
    MATTESS_STATUS_6 = 0x20,
    MATTESS_STATUS_7 = 0x40,
}MATTESS_CARD_STATUS_E;


typedef struct
{
    uint8     u8IsSearch:1;
    uint8     u8Lowpower:1;
    uint8     u8Impel:1;
    uint8     u8Reserve1:1;
    uint8     u8WorkState:2;
    uint8     u8Reserve2:2;
}__PACKED DEV_CARD_STATUS_T;

typedef union
{
    DEV_CARD_STATUS_T   stStatus;
    uint8               u8Status;
}DEV_CARD_STATUS_U;

/******************************************************************************
** �豸��ר��
*/
typedef struct
{
    uint8  u8WorkType;
}__PACKED DEV_CARD_BASIC_INFO_T;

typedef struct
{
    uint16   u16Len;
    uint8    u8Desc[];
}__PACKED DEV_CARD_DESC_INFO_T;

// �豸����Ϣ�ֳ�����:�����Ϣbasic��������Ϣ
typedef struct
{
    DEV_CARD_BASIC_INFO_T stBasicInfo;

    DEV_CARD_DESC_INFO_T stDescInfo;
}__PACKED DEV_CARD_INFO_T;

/*
**  message type
**************************************************************/
typedef enum
{
    STAFF_CARD_LOC = 0x01,  // ��ʱ�ϱ���RSSI������������豸��Ϣ

/*
**�����msgtypeֻ���豸����ʹ��
*/
    DEV_CARD_LOC = 0x11,
    DEV_CARD_INFO = 0x12,   // �ϱ���Ϣ
    DEV_CARD_SET = 0x13,    // ������Ϣ
    DEV_CARD_GET = 0x14,    // ��ѯ��Ϣ
    DEV_CARD_SCH = 0x15,    // ��λ
    DEV_CARD_CLE = 0x16,    // �ָ�������ȡ����λ
    DEV_CARD_ACK = 0x17,    // ���ֻظ�

    WRIST_CARD_LOC = 0x21,
    WRIST_CARD_TIME = 0x22,  //�������ʱ
    WRIST_CARD_SMS = 0x23,
    MATTESS_ALERTOR_LOC = 0x31,     //�봲������
    SOS_CARD_LOC      = 0x32,      //�̶�������
    BADGE_CARD_LOC   = 0x33,       //����
    ASSET_CARD_LOC   = 0x34,

/*
**����������
*/
    RADIO_JMP_PT = 0x40,    // ����������

    CARD_VERINFO = 0x7F,    // ���汾�ϱ�
/*
 * 0x80-0xff ���������Ϳ������á�
 */
}RADIO_MSGTYPE_E;

// command��
typedef struct
{
	uint8 u8MsgType;
}__PACKED RADIO_CMD_T;

/******************************************************************************
** ����Ϊ�������߻�վ�����Э��
**/
/***************************************************************
----------------------------------------------------------------------------------
|   Э��   |   ����   | ���к�     | ����汾   | ״̬λ | ��Ʒ��ʽ
|   Msg����| Battery  | SeqHSeqL   |  SoftVer   | Status | Model
----------------------------------------------------------------------------------
***************************************************************/
// ��Ա����λ��Э��
typedef struct
{
    uint8               u8MsgType;
    uint8               u8Battery;              // unit: 0.05v
    uint16              u16Seqnum;              // send data seqnum
    uint8               u8SoftVer;              // software version number
    uint8               u8Status;
    uint8               u8Model;                // Model of card
    uint16	            u16Crcsum;
}__PACKED RADIO_STAFF_CARD_LOC_T;


typedef struct
{
    uint8               u8MsgType;
    uint8               u8SoftVer;              // software version number
    uint16              u16Seqnum;              // send data seqnum
    uint8               u8Status;
    uint8               u8Model;                // Model of card
    uint16	            u16Crcsum;
} __PACKED MATTESS_ALERTOR_LOC_T;

typedef struct
{
    uint8               u8MsgType;
    uint8               u8SoftVer;              // software version number
    uint16              u16Seqnum;              // send data seqnum
    uint8               u8Status;
    uint8               u8Model;                // Model of card
    uint16	            u16Crcsum;
} CUSHION_LOC_T;



// �ʲ���λ��
typedef struct
{
    uint8               u8MsgType;
	uint8               u8Battery;              //
    uint16              u16Seqnum;              // send data seqnum
//    uint8               u8SoftVer;              // software version number
    uint8   			u8Status;
    uint8               u8Model;                // Model of card
	uint16   			u16ExciterID;
    uint16              u16Crcsum;
}__PACKED RADIO_ASSET_CARD_LOC_T;


// ��Ʒ����λ��
typedef struct
{
    uint8               u8MsgType;
    uint8               u8Battery;              // unit: 0.05v
    uint16              u16Seqnum;              // send data seqnum
    uint8               u8SoftVer;              // software version number
    DEV_CARD_STATUS_U   unStatus;
    uint8               u8Model;                // Model of card
    uint8               u8WorkType;
}__PACKED RADIO_DEV_CARD_LOC_T;

// ��Ʒ����Ϣ��
typedef struct
{
	uint8           u8MsgType;
    uint8           u8IsChange;
	uint16          u16Seqnum;
	uint8           u8Len;
    DEV_CARD_INFO_T stPayload;
}__PACKED RADIO_DEV_INFO_T;

//�������ʱ��
typedef struct
{
    uint8 u8MsgType;
    uint8 u8Month;
    uint16 u16Year;
    uint8 u8Day;
    uint8 u8Hour;
    uint8 u8Minute;
    uint8 u8Second;
}__PACKED RADIO_WRIST_TIME_T;


//���������
typedef struct
{
	uint8 	u8MsgType;//Ԥ��
 	uint16 	u16seqnum;
 	uint8 	u8len;
 	//SMS content is followed.
}__PACKED RADIO_WRIST_SMS_T;


typedef struct
{
    uint8   u8MsgType;              // CARD_VERSION = 0x7F
    uint8   u8DevType;              // device type
    uint8   u8VerInfoLen;           // pu8VerInfo[] length
    uint8   u8VerOffset;            // version info offset
    uint8   u8ReleaseOffset;        // release info offset
    uint8   pu8VerInfo[];           // version info + release info
}__PACKED CARD_VERSION_INFO_T;

/******************************************************************************
** ����Ϊ����Э��
**/
/***************************************************************/
typedef enum
{
    JMP_MT_HI,              // ɨ������
    JMP_MT_JOIN,            // �������
    JMP_MT_OK,              // ����ɹ�
    JMP_MT_SORRY,           // ����ʧ��
    JMP_MT_BYE,             // �������
    JMP_MT_DATA,            // ��ͨ���ݰ�
    JMP_MT_TOPO,
    JMP_MT_MAX
}__PACKED JMP_MT_E;

typedef struct
{
    unsigned char u8PktType;         // ������
    unsigned char u8MsgType;         // msg��������
    unsigned short u16SeqNum;        // ���к�
    unsigned char u8Direc;            // ����������
    unsigned char u8Ttl;              // ������γɻ�������
    unsigned short u16Len;
}__PACKED RADIO_JMP_HDR_T;

// msgtype JMP_MT_HI
typedef struct
{
    unsigned char u8Level;       // �ڵ�����
    unsigned char u8Weight;      // �ڵ��Ȩֵ
    unsigned short u16RootAddr;
}__PACKED JMP_MSG_HI_T;

#endif//RADIO_PROTO_H
