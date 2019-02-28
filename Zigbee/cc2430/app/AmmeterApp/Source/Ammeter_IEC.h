/*
*
*
*
*
*
*/
#ifndef AMMETER_IEC_H
#define AMMETER_IEC_H

#include "ZComDef.h"

typedef enum
{
	E_IEC_FRAME_SERVER_START,
	E_IEC_FRAME_SERVER_DISC,
	E_IEC_FRAME_SERVER_ACK,
 	
	E_IEC_FRAME_CLIENT_ID,
	E_IEC_FRAME_CLIENT_UA,
	
	E_IEC_FRAME_DATA_FINISHED,		
	E_IEC_FRAME_DATA_UNFINISHED,
	E_IEC_FRAME_DATA_ERROR,
	
}teIECframeType;

typedef enum
{
	E_IEC_STATE_IDLE,
	E_IEC_STATE_BAUD_SWITCH,
	E_IEC_STATE_UART_IDLE,	
	E_IEC_STATE_UART_READING,	
	E_IEC_STATE_UART_FINISHED,	
}teIECstate;

typedef struct
{
	uint8 max_baud_rate;
	uint8 com_baud_rate;
	uint8 device_id[3];
	uint8 protocol:2;
	uint8 baud_switch:1;
	uint8 state:5;
	
}tsIEC_info;

#define D_IEC_SERVER  0
#define D_IEC_CLIENT 1

#define D_IEC_PROTOCOL_C 0
#define D_IEC_PROTOCOL_E 1

teIECframeType IEC_ParseFrame(uint8 *frameData, uint8 len, uint8 nodeType, tsIEC_info* info);
void IEC_ChangeBaudRate(uint8 port, tsIEC_info* info);

#endif

