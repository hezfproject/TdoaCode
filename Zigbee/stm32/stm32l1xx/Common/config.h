#ifndef  __CONFIG_H_INCLUDED__
#define  __CONFIG_H_INCLUDED__

#if defined __cplusplus
extern "C" {
#endif

#include "app_protocol.h"
#include "version.h"


//#define   DEC_UWB_ANCHOR    //card 


// 设备类型
typedef enum
{
	DEVICE_TYPE_CARD5S   = 1,
	DEVICE_TYPE_LOCARTOR = 2,
	DEVICE_TYPE_STATION  = 3,
	DEVICE_TYPE_GAS      = 4,
	DEVICE_TYPE_CARD1S   = 5,
	DEVICE_TYPE_CARD15S  = 6,
}tof_device_type_te;

// 基站接受上层信息状态
typedef enum
{
	STATION_TYPE_NORMAL      = 0x0,
	STATION_TYPE_ALARM       = 0x1,    //all
	STATION_TYPE_ALARM_RESET = 0x2,    //all
	STATION_TYPE_ALARM_ANY   = 0x4,    //any card
	STATION_TYPE_DEVCH_1S    = 0x8,    //all  change to 1S card
	STATION_TYPE_DEVCH_5S    = 0x10,   //all  change to 5S card
	STATION_TYPE_EXCIT       = 0x20,
}station_status_te;

typedef union
{
	app_header_t tof_head;
}RfTofWrapper_tu;


	// end of TOF protocol for RF
/************************************************************************/

#if defined __cplusplus
}
#endif

#endif  /* CONFIG_H_INCLUDED */
	
	
	
	
	
