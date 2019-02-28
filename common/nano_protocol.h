#ifndef _NANO_PROTOCOL_H_
#define _NANO_PROTOCOL_H_

/* __packed keyword used to decrease the data type alignment to 1-byte */
#if defined (__CC_ARM)         /* ARM Compiler */
  #define __packed  __packed
#else
  #define __packed
#endif

typedef unsigned char   uint8;
typedef signed   char   int8;
typedef unsigned short  uint16;
typedef signed   short  int16;
typedef unsigned int    uint32;
typedef signed   int    int32;

#define NT_MAX_DEPTH                10

#define MIN_NEIGHBORS_NUM	        1 //3
#define MAX_NEIGHBORS_NUM	        8

#define MIN_RANGING_NUM             MIN_NEIGHBORS_NUM
#define MAX_RANGING_NUM             6

// radio address
#define NT_STATION_ADDRESS          0x0000
#define NT_GROUP_ADDRESS            0xFFF0
#define NT_BROADCAST_ADDRESS        0xFFFF
#define NT_INVALID_ADDRESS          0xFFFF

#define NT_ANCHOR_ADDRESS_MIN       0x0001
#define NT_ANCHOR_ADDRESS_MAX       0x0100

#define NT_TAG_ADDRESS_MIN          0x0101
#define NT_TAG_ADDRESS_MAX          0x0200

// search wait time
#define SEARCH_NEIGHBOR_DURATION	50

// toa result protocl
#define NT_FRM_HDR_SIZE		        (sizeof(NT_frm_hdr_t))
#define NT_SOF                      0xFE
#define NT_EOF                      0x7F
#define NT_CMD_NWK_FRM              0

typedef enum
{
    FT_TOA_RESULT       = 1,
    FT_BEACON_REQ       = 2,
    FT_BEACON           = 3,
    FT_UPDATE_DEPTH     = 4,
    FT_TAG_CFG_POLL     = 5,
    FT_TAG_CFG          = 6,
    FT_ANCHOR_CFG_POLL  = 7,
    FT_ANCHOR_CFG       = 8,
    FT_TAG_EVENT        = 9,
    FT_CONFIG           = 10,
    FT_TDOA_RESULT      = 11,
    FT_TDOA_BEACON      = 12,
    FT_RANGING_RESULT   = 13,
    NT_SEQNUM_COUNT     = 14,
    NT_SEQNUM_LOSE      = 15,
} NT_frm_type;

__packed typedef struct
{
    uint8   type: 6;
    uint8   srcrtg: 1;  //源路由，指定经过的路由，路由的时候取ext_hdr里头的地址作为路由
    uint8   recordpath: 1;
    uint8   dsn;
    uint16  srcaddr;
    uint16  dstaddr;
    uint8   hops;
    uint8   len;
} NT_frm_hdr_t;

__packed typedef struct
{
    uint8   idx;
    uint8   cnt;
    uint16  path[NT_MAX_DEPTH];
} NT_frm_ext_hdr_t;

__packed typedef struct
{
	uint8		sof;
	uint8		len;			/* exclude hdr/tail */
	uint8		cmd;
} NT_Uart_hdr_t;

__packed typedef struct
{
	uint8		chk;
	uint8		eof;
} NT_Uart_tail_t;

__packed struct CmdHeader
{
    int type;
    int length;
};

#endif
