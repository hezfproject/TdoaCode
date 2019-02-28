/*
 * RS485.h
 *
 *  Created on: 2011-4-27
 *      Author: Administrator
 */

#ifndef RS485_H_
#define RS485_H_
#include "MBusProto.h"
#include <mac_sap.h>


typedef struct mbus_tlv_t_Temp {
	uint_8 type;
	uint_16 len;
	// followed by data
	mbus_sensor_packed_t mbus_sensor_packed;
} __PACKED mbus_tlv_t_Temp;


typedef struct mbus_hdr_slv_t_Temp {
	char sync[4];
	uint_8 frame_control;
	/*
	//bit field is not portable
	uint_8 seq:1;
	uint_8 :1;  // padding bit
	uint_8 version:6;
	*/
	uint_8 cmd;
	uint_16 slv_id;
	uint_16 data_len;
	// followed by TLVs
	mbus_tlv_t_Temp mbus_tlv;
} __PACKED mbus_hdr_slv_t_Temp;

extern mbus_hdr_slv_t_Temp mbus_hdr_slv;
extern MAC_ExtAddr_s psMacAddr;
extern uint16 u16StationPanId;
extern uint8 MbusSlvSensorAddr;
extern uint8 TX_flag;
extern uint8 fram_control;



extern uint16 u16Big_To_Little (uint16 value);


PUBLIC void RS485_Init (void);
PUBLIC void Send_Pack (uint8 fram_control);

#endif /* RS485_H_ */
