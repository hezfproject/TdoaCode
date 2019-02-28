/*
 * MBusProto.h
 * This header file is designed for definitions of type and data format
 * which are used for transceiving data on uart 485 using ModBus protocol.
 * CRC Algorithm is enclosed in this file.
 */

#ifndef _MBUSPROTO_H_
#define _MBUSPROTO_H_

#include "CommonTypes.h"

#define MBUS_SYNC "YIR"
#define MBUS_SYNC_SIZE	3

#define MBUS_BAUDRATE 115200 //bit per second
#define MBUS_READ_TIMEOUT 400 // millisecond
#define MBUS_FRAME_INTERVAL 50 // millisecond

/**************************************************************************************************
 *                                            MBUS PROTOCOL, serve for data tranform and link managment
 **************************************************************************************************/
/*
 * MBUS Header
 */
typedef struct
{
    char sync[3];
    uint_8 cmd;
    uint_16 slv_id;
    uint_16 data_len;
    // followed by TLVs
}__PACKED mbus_hdr_t;

/*
 * MBUS Fooder
 */
typedef struct
{
    uint_16 padding;
    uint_16 crc;
}__PACKED mbus_fdr_t;

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

/***************************************************************
*   below is fault
*/
    MBUS_CMD_SEARCH_CARD = 6,      // download search card's record
    RESERVER = 7,                   // not using remove
    MBUS_CMD_SET_INFO =8,
/*
*   above is fault
**************************************************************/

    MBUS_CMD_DATA = 9,      // data down
} eMBusCmd;


/**************************************************************************************************
 *                                            TLV  PROTOCOL
 **************************************************************************************************/

typedef struct
{
    uint_8 type;
    uint_8 model;
    uint_16 len;
    // followed by data
}__PACKED mbus_tlv_t;

/*
 * TYPE of TLV
 */
typedef enum
{
    MBUS_TLV_CARD_READER = 0x1,
}__PACKED eMBusTlvType;

#endif

