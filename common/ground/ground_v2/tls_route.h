#ifndef _TLS_ROUTE_H_
#define _TLS_ROUTE_H_

#if defined ( __CC_ARM ) 
    #define __PACKED __attribute__((__packed__))
#elif defined ( __GNUC__ )
    #define __PACKED __attribute__((__packed__))
#endif

/*
**  network panid
**  use type check, (if type error that get a warning)
**************************************************************/
#define     DEVICE_CARD_PANID               0xFFF6
#define     STAFF_CARD_PANID                0xFFF7
#define     READ_STATION_PANID    	        0xFFF8
#define     POS_STATION_PANID               0xFFF9

#define     NWK_BROADCAST_ADDR              0xFFFF

#define     LPBSS_MAX_CARD_CMD_RECORDS      100

#define     LPBSS_NWK_MODULE_DEVTYPE_GET(module)            ((module) & 0x0F)
#define     LPBSS_NWK_MODULE_DEVMODEL_GET(module)           (((module) & 0xF0) >> 4)
#define     LPBSS_NWK_MODULE_DEVTYPE_SET(module, type)      ((module & 0xf0) | (0x0f & (type)))
#define     LPBSS_NWK_MODULE_DEVMODEL_SET(module, model)    ((module & 0x0f) | (0xf0 & (model << 4)))

/*
**  LPBSS Nwk header + LPBSS  Payload
*/
typedef struct
{
    unsigned short 	u16SrcAddr;       // 源设备地址
    unsigned short 	u16DstAddr;       // 目的设备地址
    unsigned char 	u8SrcModule;     // 源设备类型
    unsigned char 	u8DstModule;     // 目的设备类型
    unsigned short	u16Len;           // 后面LPBSS payload 数据长度
}__PACKED LPBSS_Nwk_Header_T;

//device Type
typedef enum
{
    ANALYSIS_DEVICE_ID = 0x00,        // analysis programme
    STATION_CONTROL_DEVICE_ID = 0x01, // station control
    READSTATION_DEVICE_ID = 0x02,     // read-card station
    STAFF_CARD_DEVICE_ID = 0x03,      // Staff card
    DEVICE_CARD_DEVICE_ID = 0x04,     // device card
    HAND_MACHINE_DEVICE_ID = 0x05,     // hand-machine
    NANO_LOCATOR_DEVICE_ID = 0x06
}LPBSS_Device_Type_E;

typedef struct
{
	unsigned char u8Msg;
	unsigned char u8Padding;
	unsigned short u16len;
}__PACKED LPBSS_Msg_Header_T;

//Msg type
enum msg_type_e
{
    LPBSS_VERSION  = 0x00,   //station'sversion(staff station and device station)

    LPBSS_CARD_LOC_DATA     = 0x01,

    //LPBSS_DEV_LOC    = 0x11,
    LPBSS_DEV_INFO   = 0x12,
    LPBSS_DEV_SET    = 0x13,
    LPBSS_DEV_CARD_SCH = 0x14,    //include:serch or cancel serch
    LPBSS_CARD_VERSION =0x15,

    LPBSS_JMP_TOPO = 0x30,

    LPBSS_SET_LOC_FREQ = 0x31,      // reserve
    LPBSS_DISTANCE_RESULT = 0x32,   // reserve
    LPBSS_BASIC_DATA = 0x33,        // reserve

    LPBSS_URGENT_RETREAT = 0x34,
    LPBSS_CANCLE_RETREAT = 0x35,
    LPBSS_URGENT_RSP = 0x36,
    LPBSS_URGENT_CNF = 0x37,
    LPBSS_HELP_REQ = 0x38,
    LPBSS_HELP_RSP = 0x39,
    LPBSS_MISC_DATA = 0x40,
};

/********************************END*****************************************/
#endif
