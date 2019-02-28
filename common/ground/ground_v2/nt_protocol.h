#ifndef _NANO_PROTOCOL_H_
#define _NANO_PROTOCOL_H_

#if defined ( __CC_ARM )
    #define __PACKED __attribute__((__packed__))
#elif defined ( __GNUC__ )
    #define __PACKED __attribute__((__packed__))
#endif

/* ����������ݰ� */
typedef struct dist_info
{
    unsigned short anchor_addr;
    unsigned short tenfold_dist;
    unsigned char rssi;
} __PACKED dist_info_t;

/* ��λ����������TOA�����߲��� */
typedef struct
{
    unsigned int    timestamp;
    unsigned char   numrec;

    /* under follow misc-payload,
        size = numrec * (NT_dist_info_t)
    */
    // dist_info_t     dist_info[];
} __PACKED locate_result_t;

typedef enum
{
    CARD_STATUS_NOMAL = 0x00,        // ����
    CARD_STATUS_SEARCH = 0x01,       // ���ҵ�
    CARD_STATUS_HUNGER = 0x02,       // �͵�
    CARD_STATUS_IMPEL = 0x04,        // ����
    CARD_STATUS_HELP = 0x08,         // ���

    CARD_STATUS_MOTIONLESS = 0x10,   // long sleep
    CARD_STATUS_REBOOT = 0x20,       // system reboot
    CARD_STATUS_ATINVALID = 0x40      // acceleration transducer invalid
} card_status_e;

/* �豸��ʶ��Ϣ */
typedef struct
{
    unsigned short              device_address;
    unsigned short              device_type;
} __PACKED device_t;

/* �豸������Ϣ */
typedef struct
{
    device_t                    device;
    unsigned short              seqnum;
    unsigned char               battery;              // unit: 0.05v
    unsigned char               status;
} __PACKED basic_data_t;

/* ����ĵ������� */
typedef struct
{
    basic_data_t    basic_data;
    unsigned int    urgent_id;	// ������
} __PACKED urgent_confirm_t;

/* �汾��Ϣ�ĵ������ݱ�ǩ */
typedef struct
{
    device_t        device;
    unsigned char   version_len;
} __PACKED version_tag_t;

/* �豸�Ļ�����Ϣ�Ͷ�λ���ݻ�ϵİ� */
typedef struct
{
    basic_data_t        basic_data;
    locate_result_t     locate_data;
} __PACKED misc_data_t;

/* �����ϰ���һ���ϱ� */
typedef struct
{
    LPBSS_Msg_Header_T  msg_hdr;
    misc_data_t         misc_data;
} __PACKED misc_report_t;

/* ����֪ͨ */
typedef struct
{
    LPBSS_Msg_Header_T  msg_hdr;
    unsigned int        urgent_id;	    // ������
    unsigned short      number;         // ��վ����
    // device_t            device_list[];  // ��վ��ַ
} __PACKED urgent_notify_t;

/* �����ϱ� */
typedef struct
{
    LPBSS_Msg_Header_T  msg_hdr;
    urgent_confirm_t    urgent_confirm;
} __PACKED urgent_report_t;

/* �����Ӧ */
typedef struct
{
    LPBSS_Msg_Header_T  msg_hdr;
    device_t            device;         // ����ַ
} __PACKED help_response_t;

// ����ϱ�
typedef struct
{
    LPBSS_Msg_Header_T  msg_hdr;
    basic_data_t        basic_data;     // ������
} __PACKED help_request_t;

typedef struct
{
    LPBSS_Msg_Header_T  msg_hdr;
    version_tag_t       version;
} __PACKED version_report_t;

/*******************************************************************************
* ���ͻ�վ֮���Э������
*/
#define RANGING_DATA_LEN            (5)

#define MIN_NEIGHBORS_NUM           (4)
#define MAX_NEIGHBORS_NUM           (5)

#define MIN_RANGING_NUM             (MIN_NEIGHBORS_NUM)
#define MAX_RANGING_NUM             (5)

// radio address
#define NT_STATION_ADDRESS          (0x0000)
#define NT_GROUP_ADDRESS            (0xFFF0)
#define NT_BROADCAST_ADDRESS        (0xFFFF)
#define NT_INVALID_ADDRESS          (0xFFFF)

#define NT_ANCHOR_ADDRESS_MIN       (1)
#define NT_ANCHOR_ADDRESS_MAX       (65000)

#define NT_TAG_ADDRESS_MIN          (1)
#define NT_TAG_ADDRESS_MAX          (65000)

// search wait time
#define NT_SEARCH_DURATION          (50)

// toa result protocl
#define NT_FRM_HDR_SIZE             (sizeof(NT_frm_hdr_t))
#define NT_FRM_SIZE_MAX             (128)

typedef enum
{
    FT_BEACON_REQ       = 1,
    FT_BEACON_RSP       = 2,
    FT_CONFIG           = 3,
    FT_TOA_RESULT       = 4,
    FT_DISTANCE_RESULT  = 5,
    FT_BASIC_DATA       = 6,
    FT_MISC_DATA        = 7,
    FT_SYSTEM_VERSION   = 8,
    FT_DESCR_DATA       = 9,
    FT_URGENT_NOTIFY    = 10,
    FT_URGENT_CANCEL    = 11,
    FT_URGENT_RSP       = 12,
    FT_URGENT_CNF       = 13,
    FT_HELP_REQ         = 14,
    FT_HELP_RSP         = 15,
} NT_frm_type;

typedef struct
{
    unsigned char   frm_type;
    unsigned char   device_type;
    unsigned short  seqnum;
    unsigned short  srcaddr;
    unsigned short  dstaddr;
    unsigned char   len;
} __PACKED NT_frm_hdr_t;

/* ����ĵ������� */
typedef struct
{
    basic_data_t    basic_data;
    unsigned int    urgent_id;	// ������
} __PACKED NT_urgent_confirm_t;

/* �豸�Ļ�����Ϣ�Ͷ�λ���ݻ�ϵİ� */
typedef struct
{
    NT_frm_hdr_t    frm_hdr;
    misc_data_t     misc_data;
} __PACKED NT_misc_data_t;

/* ����֪ͨ */
typedef struct
{
    NT_frm_hdr_t    frm_hdr;
    unsigned int    urgent_id;  // ������
    device_t        device;     // ����ַ
} __PACKED NT_urgent_notify_t;

/* �����ϱ� */
typedef struct
{
    NT_frm_hdr_t        frm_hdr;
    urgent_confirm_t    urgent_confirm;
} __PACKED NT_urgent_report_t;

/* �����Ӧ */
typedef struct
{
    NT_frm_hdr_t    frm_hdr;
    device_t        device;     // ����ַ
} __PACKED NT_help_response_t;

/* ������� */
typedef struct
{
    NT_frm_hdr_t    frm_hdr;
    basic_data_t    basic_data;
} __PACKED NT_help_request_t;

typedef struct
{
    NT_frm_hdr_t    frm_hdr;
    version_tag_t   version;
} __PACKED NT_version_report_t;

#endif
