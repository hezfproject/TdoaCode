/**************************************************************************************************
Filename:       Conveyor.h
Revised:        $Date: 2011/08/10 17:43:56 $
Revision:       $Revision: 1.1 $

Description:    This file contains the the Mac Sample Application protypes and definitions

**************************************************************************************************/

#ifndef CONVEYOR_H
#define CONVEYOR_H

#include  "hal_types.h"
#include  "comdef.h"

extern uint8 sent_conveyortype;

// state  type
#define NOMAL_STOP      		0
#define DELAY_START     		1
#define MANPOWER_STOP   	2
#define TROUBLE_STOP    		3
#define NOMAL_RUN       		4
#define COMPLY_START    		5
#define COMPLY_STOP     		6

//malfunction_msg
#define  UPPER_LIMIT    0x26
#define  LOWER_LIMIT    0x27

//malfunction_type
typedef enum
{
    CONVEYOR_SPEED= 1,     //�ٶ�  
    CONVEYOR_CUMULI, 	//��ú  
    CONVEYOR_OFFCENTER, 	//��ƫ  
    CONVEYOR_SCRAM, 		//��ͣ
    CONVEYOR_TEMPERATURE, 	//�¶�
    CONVEYOR_SMOG, 		//����
    CONVEYOR_LANIATE, 	//˺��
    CONVEYOR_TENSILITY, 	//����
    CONVEYOR_BREAKAGE, 	//�ϴ�
    CONVEYOR_UNDERVOLTAGE, 	//Ƿѹ
    CONVEYOR_OVERCURRENT1, 	//����1
    CONVEYOR_OVERCURRENT2, 	//����2
    CONVEYOR_GAGE, 		//��λ   0x0D
}malfunction_type;

//Status_type



extern uint8 Get_MalfunctionType_by_number(char *pbuff,malfunction_type malfunctiontype);
extern uint8 Get_ConveyorStatus_by_number(char* pbuff,uint8 status );

#endif /* CONVEYOR_H */
