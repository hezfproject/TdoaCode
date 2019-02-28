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
*MIC2000 from ����������Ƽ�
****************************************************************************/

/* always 1 */
#define MIC2000_COLLECTOR_ADDR   0x01

/* Regiters Addresses - 16bit*/
#define MIC2000_REGADDR_SENSOR_STATUS                            0x00D8         /*8BYTES; ̽����״̬��8BYTES	;״ֵ̬   		8*1�ֽ�ʮ��������������Ӧ1-8��̽����*/
#define MIC2000_REGADDR_SENSOR_NUM                   0x0103         /*1BYTE   ; 01-08 */
#define MIC2000_REGADDR_DENSITY                           0x0x08        /*24BYTES	;��������Ũ��ֵ 8*3�ֽ�BCD����Ӧ1-8��̽����*/
#define MIC2000_REGADDR_ALARM_H                          0x0120         /*24BYTES	;�߶α����趨ֵ	8*3�ֽ�BCD����Ӧ1-8��̽����*/
#define MIC2000_REGADDR_ALARM_L                          0x0138         /*24BYTES	;�Ͷα����趨ֵ	8*3�ֽ�BCD����Ӧ1-8��̽����*/
#define MIC2000_REGADDR_FULL_RANGE                    0x0150         /*24BYTES; ������ֵ	    8*3�ֽ�BCD����Ӧ1-8��̽����*/
#define MIC2000_REGADDR_SENSOR_TYPE                  0x0168          /*8BYTES; �����������	8*1�ֽ�ʮ��������������Ӧ1-8��̽����*/
#define MIC2000_REGADDR_DECIMAL_POINT_POS      0x0170         /*8BYTES	;С����λ��0-4	8*1�ֽ�ʮ��������������Ӧ1-8��̽����*/
#define MIC2000_REGADDR_UNIT                                 0x0178         /*8BYTES  ;��λֵ   1-3  	8*1�ֽ�ʮ��������������Ӧ1-8��̽����*/
#define MIC2000_REGADDR_SHIELDING                       0x0180          /*8BYTES	;�Ƿ�����     	8*1�ֽ�ʮ��������������Ӧ1-8��̽����*/
#define MIC2000_REGADDR_RELAY                               0x0188         /*8BYTES	;�̵������0-8	8*1�ֽ�ʮ��������������Ӧ1-8��̽����*/
#define MIC2000_REGADDR_TARGET_POINT                0x0190          /*24BYTES	;Ŀ����趨ֵ   8*3�ֽڸ���������Ӧ1-8��̽����*/


/*����ͷΪ����4 ��9 ���ֽڵ�0xFE*/
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
    unsigned char sensor_status[8]; /*̽����״̬��8BYTES	;״ֵ̬   		8*1�ֽ�ʮ��������������Ӧ1-8��̽����*/
    unsigned char density[24];          /*��������Ũ��ֵ 8*3�ֽ�BCD����Ӧ1-8��̽����*/
    unsigned char alarmH[24];          /*�߶α����趨ֵ	8*3�ֽ�BCD����Ӧ1-8��̽����*/
    unsigned char alarmL[24];          /*�Ͷα����趨ֵ	8*3�ֽ�BCD����Ӧ1-8��̽����*/
    unsigned char fullRange[24];        /*������ֵ	    8*3�ֽ�BCD����Ӧ1-8��̽����*/
    unsigned char sensor_type[8];   /*�����������	8*1�ֽ�ʮ��������������Ӧ1-8��̽����*/
    unsigned char decimal_point_pos[8];   /*С����λ��0-4	8*1�ֽ�ʮ��������������Ӧ1-8��̽����*/
    unsigned char unit[8];                  /*��λֵ   0-2  	8*1�ֽ�ʮ��������������Ӧ1-8��̽����*/
    unsigned char shielding[8];         /*�Ƿ�����     	8*1�ֽ�ʮ��������������Ӧ1-8��̽����*/
    unsigned char relay[8];                /*�̵������0-8	8*1�ֽ�ʮ��������������Ӧ1-8��̽����*/
    unsigned char target_point[24];    /*Ŀ����趨ֵ   8*3�ֽڸ���������Ӧ1-8��̽����*/
}MIC2000_data_t;


/****************************************************************************/

#endif 


