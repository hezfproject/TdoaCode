/*****************************************************************************

 *
 * MODULE:             Sensor Collector
 *
 * COMPONENT:          mac_pib.h
 * 
 * This file contains defines for all type of sensors
 *
 *
****************************************************************************/

#ifndef _SENSOR_DEFINES_H
#define _SENSOR_DEFINES_H

/*****************************************************************************
* 
*MIC2000 from 深圳逸云天科技
****************************************************************************/

/* always 1 */
#define MIC2000_COLLECTOR_ADDR   0x01

/* Regiters Addresses - 16bit*/
#define MIC2000_REGADDR_SENSOR_STATUS                            0x00D8         /*8BYTES; 探测器状态：8BYTES	;状态值   		8*1字节十六进制整数，对应1-8号探测器*/
#define MIC2000_REGADDR_SENSOR_NUM                   0x0103         /*1BYTE   ; 01-08 */
#define MIC2000_REGADDR_DENSITY                           0x0x08        /*24BYTES	;采样气体浓度值 8*3字节BCD，对应1-8号探测器*/
#define MIC2000_REGADDR_ALARM_H                          0x0120         /*24BYTES	;高段报警设定值	8*3字节BCD，对应1-8号探测器*/
#define MIC2000_REGADDR_ALARM_L                          0x0138         /*24BYTES	;低段报警设定值	8*3字节BCD，对应1-8号探测器*/
#define MIC2000_REGADDR_FULL_RANGE                    0x0150         /*24BYTES; 满量程值	    8*3字节BCD，对应1-8号探测器*/
#define MIC2000_REGADDR_SENSOR_TYPE                  0x0168          /*8BYTES; 气体名称序号	8*1字节十六进制整数，对应1-8号探测器*/
#define MIC2000_REGADDR_DECIMAL_POINT_POS      0x0170         /*8BYTES	;小数点位置0-4	8*1字节十六进制整数，对应1-8号探测器*/
#define MIC2000_REGADDR_UNIT                                 0x0178         /*8BYTES  ;单位值   1-3  	8*1字节十六进制整数，对应1-8号探测器*/
#define MIC2000_REGADDR_SHIELDING                       0x0180          /*8BYTES	;是否屏蔽     	8*1字节十六进制整数，对应1-8号探测器*/
#define MIC2000_REGADDR_RELAY                               0x0188         /*8BYTES	;继电器输出0-8	8*1字节十六进制整数，对应1-8号探测器*/
#define MIC2000_REGADDR_TARGET_POINT                0x0190          /*24BYTES	;目标点设定值   8*3字节浮点数，对应1-8号探测器*/


/*数据头为连续4 至9 个字节的0xFE*/
#define MIC2000_FRAME_PRILIMINARY 0xFE

/*Frame types*/
#define MIC2000_FRAME_QUERY   0x03
#define MIC2000_FRAME_QUERY_SUCCESS   0x05
#define MIC2000_FRAME_QUERY_ERROR   0x04
#define MIC2000_FRAME_SET   0x06
#define MIC2000_FRAME_SET_SUCCESS  0x08
#define MIC2000_FRAME_SET_ERROR  0x07

typedef struct MIC2000_data_s
{
    unsigned char sensor_num;  /*1~8*/
    unsigned char filler[3]; //padding
    unsigned char sensor_status[8]; /*探测器状态：8BYTES	;状态值   		8*1字节十六进制整数，对应1-8号探测器*/
    unsigned char density[24];          /*采样气体浓度值 8*3字节BCD，对应1-8号探测器*/
    unsigned char alarmH[24];          /*高段报警设定值	8*3字节BCD，对应1-8号探测器*/
    unsigned char alarmL[24];          /*低段报警设定值	8*3字节BCD，对应1-8号探测器*/
    unsigned char fullRange[24];        /*满量程值	    8*3字节BCD，对应1-8号探测器*/
    unsigned char sensor_type[8];   /*气体名称序号	8*1字节十六进制整数，对应1-8号探测器*/
    unsigned char decimal_point_pos[8];   /*小数点位置0-4	8*1字节十六进制整数，对应1-8号探测器*/
    unsigned char unit[8];                  /*单位值   0-2  	8*1字节十六进制整数，对应1-8号探测器*/
    unsigned char shielding[8];         /*是否屏蔽     	8*1字节十六进制整数，对应1-8号探测器*/
    unsigned char relay[8];                /*继电器输出0-8	8*1字节十六进制整数，对应1-8号探测器*/
    unsigned char target_point[24];    /*目标点设定值   8*3字节浮点数，对应1-8号探测器*/
}MIC2000_data_t;


/****************************************************************************/

#endif 


