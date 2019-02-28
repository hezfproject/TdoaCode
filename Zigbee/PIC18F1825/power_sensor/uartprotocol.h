
/*
 *uartprotocol.h
 *
 *  Created on: 2014-10-31
 *      Author: yfm
 */

#ifndef UART_PROTOCOL_H_
#define UART_PROTOCOL_H_

typedef struct power_information_t
{
	uint8 ATEXT;
	uint8 BTEXT;
	uint16 ITEXT;
	uint16 VTEXT;
	// followed by data
	
}power_informati_t_Temp;


typedef struct uartprotocol_t 
{
	uint8 type;
	uint16 len;
	// followed by data
	power_informati_t_Temp power_information__packed;
}uartprotocol_t_Temp;


typedef struct uartprotocol_hdr_slv_t 
{
	int8 sync[4];
	uint8 frame_control;

	uint8 cmd;
	uint16 slv_id;
	uint16 data_len;
	// followed by TLVs
	uartprotocol_t_Temp uartprotocol_tlv;
}uartprotocol_hdr_slv_t_Temp;











#endif /* UART_PROTOCOL_H_ */





