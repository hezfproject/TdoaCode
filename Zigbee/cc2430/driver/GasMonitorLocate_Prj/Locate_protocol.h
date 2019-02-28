/**************************************************************************************************
Filename:       Locate_protocol.h
Revised:        $Date: 2011/07/27 03:01:45 $
Revision:       $Revision: 1.7 $

**************************************************************************************************/
#ifndef _LOCATE_PROTOCOL_H
#define _LOCATE_PROTOCOL_H

#define UART_PREAMBLE_H 0x4D
#define UART_PREAMBLE_L 0x41


/* cmdtype */ 
#define    LOCATE 		1
#define    MOTOR 			2
#define    WIRELESS 		3
#define    SYNC_REQ 		4
#define    SYNC_ACK 		5

typedef struct
{
    unsigned char	isvalid;
    unsigned char 	seqnum;
    unsigned char   success_rate;
    int8   RSSI;
    unsigned short   stationPandID;
    unsigned short   stationShortAddr;
    unsigned short   Locate_Distance;
} Locate_Information_t;

typedef struct
{
    unsigned char  header_h;
    unsigned char  header_l;
    unsigned char  cmdtype;
    unsigned char padding;
    unsigned short len;
    unsigned short checksum;
    //data is followed.
} Uart_Header_t;


typedef struct
{
	Uart_Header_t hdr;
	Locate_Information_t locate;
}uart_locate_t;

typedef struct
{
	Uart_Header_t hdr;
	unsigned short DstPan;
    	unsigned short DstAddr;
    	unsigned short SrcPan;
    	unsigned short SrcAddr;
	// wireless data is followed, the length is hdr->len - 8
}uart_wireless_t;

/* request IEEE address */
typedef struct
{
	Uart_Header_t hdr;
}uart_sync_req_t;

/* give IEEE addres to 5148 */
typedef struct
{
	Uart_Header_t hdr;
	unsigned char  ExitAddr[8];
}uart_sync_ack_t;


#define MOTOR_ON 1
#define MOTOR_OFF 0
typedef struct
{
	Uart_Header_t hdr;
	unsigned char  onoff;    /* 1: on,  0: off*/
}uart_motor_t;


#endif
