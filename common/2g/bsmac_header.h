#ifndef BSMAC_HEADER_H
#define BSMAC_HEADER_H

/*****************************************************************************
*
* BSmac related definitions here
*
*****************************************************************************/

#define BSMAC_HEADER_LEN              (12)
#define BSMAC_FOOTER_LEN              (2)
#define BSMAC_RX_LEN_DEFAULT          (128)
#define BSMAC_TX_LEN_DEFAULT          (128)
#define BSMAC_MAX_TX_PAYLOAD_LEN         (BSMAC_TX_LEN_DEFAULT-BSMAC_HEADER_LEN - BSMAC_FOOTER_LEN)
#define UWB_RX_LEN_DEFAULT    (512)

// mac header
#define BSMAC_PREAMBLE_H              (0x4d)
#define BSMAC_PREAMBLE_L              (0x41)
#define BSMAC_FRAME_TYPE_DATA         (0x00)          // data frame
#define BSMAC_FRAME_TYPE_ACK          (0x01)          // ack frame
#define BSMAC_FRAME_TYPE_DATA_ACK     (0x02)          // ack data data frame
#define BSMAC_FRAME_TYPE_LIVE         (0x03)          // keep-alive frame
#define BSMAC_DEVICE_TYPE_BS_EP       (0x00)          // device is bs
#define BSMAC_DEVICE_TYPE_BS_OP       (0x01)          // device is bs, with optical port
#define BSMAC_DEVICE_TYPE_COM         (0x02)          // device is communication module
#define BSMAC_DEVICE_TYPE_LOC         (0x03)          // device is location module


/*******************************************************************************
// Mac header:
// preamble(0x4D41, 2bytes)
// control0(1byte, bit[2:0]:mac frame type, bit[3]:rx rdy, bit[6:4]:device type, bit7:priority)
// reserved(1byte)
// framecnt(2bytes)
// src_addr(2bytes)
// dest_addr(2bytes)
// length(2bytes)
// Mac footer:
// crc(2B)
|-------------------------------------------------------------------------------
|preamble|control|reserved|framecnt|src_addr|dest_addr|length|  payload   |crc  |
|(2B)    |(1B)   |(1B)    |(2B)    |(2B)    |(2B)     |(2B)  |(length-2 B)|(2B) |
|-------------------------------------------------------------------------------
*******************************************************************************/

#define BSMAC_EXCUTE(x)      do { x } while (__LINE__ == -1)

#define BSMAC_FRAMETYPE_MASK (0x07)
#define BSMAC_RDY_MASK (0x08)
#define BSMAC_DEVICETYPE_MASK (0x70)
#define BSMAC_PRIORITY_MASK (0x80)

#define BSMAC_RDY_POS (3)
#define BSMAC_DEVICETYPE_POS (4)
#define BSMAC_PRIORITY_POS (7)

#define BSMAC_GET_FRAMETYPE(c) ((c) & BSMAC_FRAMETYPE_MASK)
#define BSMAC_GET_RDY(c) (((c) & BSMAC_RDY_MASK) >> BSMAC_RDY_POS)
#define BSMAC_GET_DEVICETYPE(c) (((c) & BSMAC_DEVICETYPE_MASK) >> BSMAC_DEVICETYPE_POS)
#define BSMAC_GET_PRIORITY(c) (((c) & BSMAC_PRIORITY_MASK) >> BSMAC_PRIORITY_POS)

#define BSMAC_SET_FRAMETYPE(c, v) BSMAC_EXCUTE((c) &= ~(BSMAC_FRAMETYPE_MASK); (c) |= (v&BSMAC_FRAMETYPE_MASK);)
#define BSMAC_SET_RDY(c, v)              BSMAC_EXCUTE((c) &= ~(BSMAC_RDY_MASK); (c) |= (((v) << BSMAC_RDY_POS)&BSMAC_RDY_MASK);)
#define BSMAC_SET_DEVICETYPE(c, v) BSMAC_EXCUTE((c) &= ~(BSMAC_DEVICETYPE_MASK); (c) |= (((v) << BSMAC_DEVICETYPE_POS)&BSMAC_DEVICETYPE_MASK);)
#define BSMAC_SET_PRIORITY(c, v)      BSMAC_EXCUTE((c) &= ~(BSMAC_PRIORITY_MASK); (c) |= (((v) << BSMAC_PRIORITY_POS)&BSMAC_PRIORITY_MASK);)

typedef unsigned char 	uint_8;
typedef unsigned short 	uint_16;
typedef unsigned char 	uint8;
typedef unsigned short 	uint16;

typedef struct
{
    unsigned char preamble_H;
    unsigned char preamble_L;
    /* bit filed depends on endian
    unsigned char frame_type    :3;
    unsigned char ready              :1;
    unsigned char device_type   :2;
    unsigned char filler               :1; //filler
    unsigned char priority           :1;
    */
    unsigned char frame_control;
    unsigned char reserverd;
    unsigned char frame_count_H;
    unsigned char frame_count_L;
    unsigned char src_addr_H;
    unsigned char src_addr_L;
    unsigned char dst_addr_H;
    unsigned char dst_addr_L;
    unsigned char data_len_H;
    unsigned char data_len_L;
}bsmac_header_t;


typedef struct
{
    uint_8 u8PreambleH;
    uint_8 u8PreambleL;
    /* bit filed depends on endian
    unsigned char frame_type    :3;
    unsigned char ready              :1;
    unsigned char device_type   :2;
    unsigned char filler               :1; //filler
    unsigned char priority           :1;
    */
    uint_8 u8frameControl;
    uint_8 u8Reserverd;
    uint_8 u8FrameCountH;
    uint_8 u8FrameCountL;
    uint_8 u8SrcAddrH;
    uint_8 u8SrcAddrL;
    uint_8 u8DstAddrH;
    uint_8 u8DstAddrL;
    uint_8 u8DataLenH;
    uint_8 u8DataLenL;
}UWB_BSMAC_PACKET_HEADER_S;

typedef struct 
{
	uint_8		u8Type;
	uint_8		u8Ttl;
	uint_16	    u16Src;           //pan_addr_t src
	uint_16 	u16Dst;
	uint_16		u16Len;
	/* The options start here */
}UWB_NET_PACKET_HEADER_S;

typedef struct
{

	/*******************************************************
	 注:此结构体先后顺序不能乱,不然可能会导致不能正确解包 
	 *******************************************************/
	UWB_BSMAC_PACKET_HEADER_S   stBsmacPackHead;    
	UWB_NET_PACKET_HEADER_S     stNetPackHead;
	uint_8 u8PackDataBuff[300];
	//CRC校验位
   // uint8 u8TxCrcH;
   // uint8 u8TxCrcL;
}UWB_BSMAC_BUILD_PACK_S;

#endif


