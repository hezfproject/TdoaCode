/**************************************************************************************************
Filename:       GasLocate_protocol_V2.h
Revised:        $Date: 2012/02/06 03:01:45 $
Revision:       $Revision: 1.0 $

**************************************************************************************************/
#ifndef _LOCATE_PROTOCOL_H
#define _LOCATE_PROTOCOL_H


#define GASLOCATE_UART_MAX_STRUCT_LEN    (sizeof(uart_version_t))

#define UART_PREAMBLE_H 0x4D
#define UART_PREAMBLE_L 0x41


//#define INVALID_DATA        0


/* cmdtype */ 
typedef enum
{
  LOCATE = 1,
  REQ = 2,
  REPORT_ACK =3,
  REPORT =4,
  RADIO_VERSION=5,
  RADIO_VERSION_ACK=6,

  CMDTYPE_END,
}gas_msgtype_t;

typedef struct
{
    unsigned char  header_h;
    unsigned char  header_l;
    unsigned short   crc;
    unsigned char  cmdtype;    
    unsigned char  len;
    unsigned short padding;    
    //data is followed.
} Uart_Header_t;

/*status type*/
#define    DOWNSTATUS_NOMAL 		0
#define    DOWNSTATUS_RETREAT   1

typedef struct
{
    unsigned char     downStatus;   //if locator is not  succsess, stationPandID=LocateDistance=stationShortAddr=0xffff;
    unsigned char     locateIsvalid;
    signed char         i8Rssi;
    unsigned char    padding; 
    unsigned short   seqnum;
    unsigned short   stationPandID;  
    unsigned short   locateDistance;
    unsigned short   stationShortAddr;
} Locate_Information_t;

/*status */
#define    STATUS_NOMAL 	0x00
//0x01   no use
#define    STATUS_HELP 	                0x02
#define    STATUS_FIRE 		        0x04
#define    STATUS_WATER 		        0x08
#define    STATUS_TOPBOARD 	        0x10
#define    STATUS_OTHER 		        0x20

typedef struct
{
    unsigned char	upStatus;    //if(upStatus !=UPSTATUS_NOMAL) need  ack
    //unsigned char 	TofChannel;
    //unsigned char 	LocateChannel;
    unsigned char 	broadcastChannel;
    unsigned short 	seqnum;
    unsigned short    shortAddr;
    unsigned short    gasDensity;
    unsigned short    alarmGasDensity;
    unsigned short    padding;
} Report_Information_t;

typedef struct
{
    unsigned char  version[16];   
} Version_Information_t;

typedef struct
{
    unsigned char     downStatus;   //if locator is not  succsess, stationPandID=LocateDistance=stationShortAddr=0xffff;
    signed char         i8Rssi;
    unsigned short   seqnum;
    unsigned short   padding1; 
    unsigned short    padding2; 
} Req_Information_t;


typedef struct
{
    unsigned char	downStatus;   
    unsigned char	padding; 
    unsigned short 	seqnum;
} Ack_Information_t;

typedef struct
{
	Uart_Header_t hdr;
	Locate_Information_t locate;
}uart_locate_t;

typedef struct
{
	Uart_Header_t hdr;
	Req_Information_t request;
}uart_request_t;


typedef struct
{
	Uart_Header_t hdr;
	Ack_Information_t ack;
}uart_ack_t;

typedef struct
{
	Uart_Header_t hdr;
	Report_Information_t report;
}uart_report_t;

typedef struct
{
	Uart_Header_t hdr;
	Version_Information_t version;
}uart_version_t;
#endif
