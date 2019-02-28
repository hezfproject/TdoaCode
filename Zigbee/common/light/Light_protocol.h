/****************************************************************************
Filename:       light_protocol.h
Revised:        $Date: 2017/04/20 00:59:21 $
Revision:       $Revision: 1.66 $
Description:
*****************************************************************************/
#ifndef __LIGHT_PROTOCOL_H__
#define __LIGHT_PROTOCOL_H__

#define RF_LIGHT_NWK_ADDR			0xFFF4


typedef enum
{
	APP_DATA_TYPE_DEPTH,
	APP_DATA_TYPE_VOL_CUR,
	APP_DATA_TYPE_VOL_CUR_TEMP,
	APP_DATA_TYPE_PARAMETER_SET,
	APP_DATA_TYPE_DISACTIVE,
	APP_DATA_TYPE_DOWNSTREAM,
	APP_DATA_TYPE_UPSTREAM,				
} ll_Data_type_t;


typedef struct
{
	unsigned char protocol_type;
	unsigned char data_type;
	unsigned char depth;
	unsigned char len;
}ll_Data_hdr_t;

typedef struct
{
	ll_Data_hdr_t data_head;
	unsigned  short u16accucost;
	unsigned  short u16parent;
	uint_16 u16Pathtoroot[8];	
}rf_light_depth_data_ts;


typedef struct
{
	unsigned char u8vrms;
	unsigned char u8irms;
	unsigned  short u16panid;
	unsigned  short u16parent;
}rf_light_vol_cur_upstream_ts;


typedef struct
{
	ll_Data_hdr_t data_head;
	unsigned char data_buf[122];
}rf_light_stream_ts;



typedef struct
{
	ll_Data_hdr_t data_head;
	unsigned char u8vrms;
	unsigned char u8irms;
	unsigned  short u16panid;
	unsigned  short u16parent;
}rf_light_vol_cur_data_ts;

typedef struct
{
	ll_Data_hdr_t data_head;
	uint_16 u16DestAddr;    
	int_8 i8RssiAssignment;        
	uint_8 u8MaxLightLevel;
	//uint_16 u16Reserved;
	uint_8 u8IsRssiStation;
	uint_8 u8Reserved;
	uint_8 u8MinLightLevel;
	uint_8 u8Len;
	uint_16 u16Pathtoroot[8];	
}rf_light_parmeter_data_ts;

typedef union
{
	ll_Data_hdr_t data_head;
	rf_light_vol_cur_data_ts light_vol_cur_data;
	rf_light_depth_data_ts light_depth_data;
	rf_light_parmeter_data_ts light_parmeter_data;
	rf_light_stream_ts light_stream;
}RfDataWrapper_tu;


#endif
