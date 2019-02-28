#ifndef _PROTOCOL_H_
#define _PROTOCOL_H_

#define NT_STATION_ADDRESS		0x0000
#define NT_ANCHOR_ADDRESS_MIN	0x0001
#define NT_ANCHOR_ADDRESS_MAX	0x0100
#define NT_TAG_ADDRESS_MIN		0x0102
#define NT_TAG_ADDRESS_MAX		0x0200
#define NT_REFTAG_ADDRESS_MIN	0x0201
#define NT_REFTAG_ADDRESS_MAX	0x0201

#define NT_INVALID_ADDRESS		0xFFFF
#define NT_BROADCAST_ADDRESS	0xFFFF
#define NT_ALLANCHOR_ADDRESS	0xFFFD
#define NT_ALLTAG_ADDRESS		0xFFFC
#define NT_GROUP_ADDRESS		0xFFF0

#define NT_MAX_DEPTH			10
#define NT_INVALID_DEPTH		0xFF

#define NT_SOF					0xFE
#define NT_EOF					0x7F

#define NT_UCMD_NWK_FRM			0
#define NT_UCMD_SET_ADDR		1
#define NT_UCMD_DEBUG_PRINT		2
#define NT_UCMD_SLEEP			3
#define NT_UCMD_SNIFFER_FRM		4
#define NT_UCMD_CONFIG			5
#define NT_DISTANCES_REPORT		6

typedef struct
{
	uint8		sof;
	uint8		len;			/* exclude hdr/tail */
	uint8		cmd;
	uint8		frm[];
} NT_Uart_hdr_t;

typedef struct
{
	uint8		chk;
	uint8		eof;
} NT_Uart_tail_t;

typedef struct
{
	uint8		tagrate;
	uint8		anchornum;
	uint16		anchorlist[];
} NT_config_info_t;

#define NT_MAX_FRM_SIZE		128
typedef struct
{
	uint8 		type:6;
	uint8		srcrtg:1;   //源路由，指定经过的路由，路由的时候取ext_hdr里头的地址作为路由
	uint8		recordpath:1;
	uint8 		dsn;
	uint16 		srcaddr;
	uint16 		dstaddr;
	uint8		hops;
	uint8 		len;
} NT_frm_hdr_t ;

typedef struct
{
	uint8		idx;
	uint8		cnt;
	uint16		path[NT_MAX_DEPTH];
} NT_frm_ext_hdr_t;

#define NT_FRM_HDR_SIZE		(sizeof(NT_frm_hdr_t))
#define NT_FRM_EXT_HDR_SIZE	(sizeof(NT_frm_ext_hdr_t))
#define NT_MAX_DATA_SIZE	(NT_MAX_FRM_SIZE - NT_FRM_HDR_SIZE)

typedef enum
{
	FT_TOA_RESULT					= 1,
	FT_BEACON_REQ					= 2,
	FT_BEACON						= 3,
	FT_UPDATE_DEPTH 				= 4,
	FT_TAG_CFG_POLL					= 5,
	FT_TAG_CFG 						= 6,
	FT_ANCHOR_CFG_POLL				= 7,
	FT_ANCHOR_CFG 					= 8,
	FT_TAG_EVENT					= 9,
	FT_CONFIG						= 10,
	FT_TDOA_RESULT					= 11,
	FT_TDOA_BEACON					= 12,
} NT_frm_type;

typedef enum
{
	TET_LOW_POWER					= 1,
} NT_tag_event_type;

#define RANGING_DATA_LEN			5
typedef struct
{
	uint8		set1[RANGING_DATA_LEN];
	uint8		set2[RANGING_DATA_LEN];
	uint8		rssi;
} ranging_data_t;
typedef struct
{
	uint16			anchoraddr;
	ranging_data_t	rawdata;
} NT_ranging_record_t;

typedef struct
{
	NT_frm_hdr_t	hdr;
	uint32			timestamp;
	uint8			numrec;
	NT_ranging_record_t		records[];
} NT_toa_result_t;

typedef struct
{
	NT_frm_hdr_t	hdr;
} NT_search_neighbor_t;

typedef struct
{
	NT_frm_hdr_t	hdr;
	uint8			rssi;
} NT_neighbor_ack_t;

typedef struct
{
	NT_frm_hdr_t		hdr;
	uint8				action;
} NT_tag_cfg_poll_t;

#define TAG_CFG_POLL_NONE		0
#define TAG_CFG_POLL_FIRST		1
#define TAG_CFG_POLL_SECOND		2
#define TAG_CFG_POLL_TUNNEL		3

typedef struct
{
	NT_frm_hdr_t		hdr;
	NT_frm_ext_hdr_t	ext;
	uint16				tagaddr;
	uint8				action;
} NT_anchor_cfg_poll_t;


#define CFG_MASK_STATIC_RATE		0x01
#define CFG_MASK_DYNAMIC_RATE		0x02
#define CFG_MASK_SLEEP				0x04

typedef struct
{
	uint8				mask;
	uint8				speriod;
	uint8				dperiod;
	uint8				sleep;		//0.1hr
} cfg_info_t;

typedef struct
{
	NT_frm_hdr_t		hdr;
	cfg_info_t			cfginfo;
} NT_tag_cfg_t;

typedef struct
{
	NT_frm_hdr_t		hdr;
	NT_frm_ext_hdr_t	ext;
	uint16				tagaddr;
	uint8				action;
	cfg_info_t			cfginfo;
} NT_anchor_cfg_t;

typedef struct
{
	NT_frm_hdr_t	hdr;
	uint8			event;
} NT_tag_event_t;

typedef struct
{
	NT_frm_hdr_t	hdr;
	uint8			depth;
	uint16			accucost;
	uint16			path2root[];
} NT_update_depth_t;

typedef struct
{
	NT_frm_hdr_t	hdr;
	uint8			pathok;
} NT_beacon_t;

typedef struct
{
	uint32			start;
	uint32			end;
} timegap_t;
typedef struct
{
	NT_frm_hdr_t	hdr;
	uint8			count;
	timegap_t		gaps[];
} NT_time_eval_t;

typedef struct
{
	NT_frm_hdr_t		hdr;
	uint8				info[];
} NT_config_t;

#define TDOA_STEP_1		0
#define TDOA_STEP_2		1
#define TDOA_STEP_3		2
typedef struct
{
	NT_frm_hdr_t		hdr;
	uint16				tagid;
	uint8				step;
} NT_TDOA_beacon_t;

typedef struct
{
	uint16				toaoff;
	uint8				phaseoff;
	uint32				ms;
	uint16				count;
} TDOA_time_info_t;
typedef struct
{
	NT_frm_hdr_t		hdr;
	uint16				tagid;
	uint16				reftagid;
	uint8				tagdsn;
	TDOA_time_info_t	info[3];
} NT_TDOA_result_t;
#endif
