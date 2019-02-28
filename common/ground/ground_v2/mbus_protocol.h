/*
 * MBusProto.h
 * This header file is designed for definitions of type and data format
 * which are used for transceiving data on uart 485 using ModBus protocol.
 * CRC Algorithm is enclosed in this file.
 */

#ifndef _MBUSPROTO_H_
#define _MBUSPROTO_H_

#if defined ( __CC_ARM ) 
    #define __PACKED __attribute__((__packed__))
#elif defined ( __GNUC__ )
    #define __PACKED __attribute__((__packed__))
#endif

#define MBUS_SYNC "YIR"
#define MBUS_SYNC_SIZE	3

#define MBUS_BAUDRATE 115200 //bit per second
#define MBUS_READ_TIMEOUT 400 // millisecond
#define MBUS_FRAME_INTERVAL 50 // millisecond

/**************************************************************************************************
 *                                            MBUS PROTOCOL, serve for data tranform and link managment
 **************************************************************************************************/
 
/*
 * CMD of MBUS
 */
typedef enum
{
    MBUS_CMD_CLR =     1,	// request slave to clear buffer

    MBUS_CMD_QRY =     2,	// query
    MBUS_CMD_RSP =     3,     // respond

    MBUS_CMD_ADDR_QRY =     4,	// query
    MBUS_CMD_ADDR_RSP =     5,     // respond

    MBUS_CMD_SEARCH_CARD = 6,      //download search card's record
    MBUS_CMD_CARD_INFO = 7,        //upload card's info
    MBUS_CMD_SET_INFO =8,

    MBUS_CMD_DATA = 9
} eMBusCmd;

/*
 * MBUS Header
 */
typedef struct
{
    char sync[3];
    unsigned char cmd;
    unsigned short slv_id;
    unsigned short data_len;
    // followed by TLVs
}__PACKED mbus_hdr_t;

/*
 * MBUS Fooder
 */
typedef struct
{
    unsigned short padding;
    unsigned short crc;
}__PACKED mbus_fdr_t;

/**************************************************************************************************
 *                                            TLV  PROTOCOL
 **************************************************************************************************/

typedef struct
{
    unsigned char type;
    unsigned char model;
    unsigned short len;
    // followed by data
} __PACKED mbus_tlv_t;

/*
 * TYPE of TLV
 */
typedef enum
{
    MBUS_TLV_CARD_READER = 0x1,
    MBUS_TLV_NANO_ANCHOR = 0x02,
} eMBusTlvType;

#endif

