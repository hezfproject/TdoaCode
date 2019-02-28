/**************************************************************************************************
Filename:       Conveyor.c
Revised:        $Date: 2011/08/10 17:43:56 $
Revision:       $Revision: 1.1 $

Description:    This file contains the sample application that can be use to test
the functionality of the MAC, HAL and low level.

**************************************************************************************************/

#include "Conveyor.h"
#include "OSAL.h"
#include "string.h"
uint8 sent_conveyortype=0xff;

uint8 Get_MalfunctionType_by_number(char* pbuff,malfunction_type malfunctiontype )
{

	uint8 len=0;  //len  must  less  than  10
	if(malfunctiontype>CONVEYOR_GAGE)return INVALIDPARAMETER;
	switch(malfunctiontype)
		{
 		case CONVEYOR_SPEED:
			len=osal_strlen((char*)"�ٶ�");
			strncpy((char*)pbuff,(char *)"�ٶ�",len);
			break;
			
 		case CONVEYOR_CUMULI:
			len=osal_strlen((char*)"��ú");
			strncpy((char*)pbuff,(char *)"��ú",len);			
			break;
			
 		case CONVEYOR_OFFCENTER:
			len=osal_strlen((char*)"��ƫ");
			strncpy((char*)pbuff,(char *)"��ƫ",len);			
			break;
			
 		case CONVEYOR_SCRAM:
			len=osal_strlen((char*)"��ͣ");
			strncpy((char*)pbuff,(char *)"��ͣ",len);			
			break;
			
 		case CONVEYOR_TEMPERATURE:
			len=osal_strlen((char*)"�¶�");
			strncpy((char*)pbuff,(char *)"�¶�",len);			
			break;
			
 		case CONVEYOR_SMOG:
			len=osal_strlen((char*)"����");
			strncpy((char*)pbuff,(char *)"����",len);			
			break;
			
 		case CONVEYOR_LANIATE:
			len=osal_strlen((char*)"˺��");
			strncpy((char*)pbuff,(char *)"˺��",len);			
			break;
			
 		case CONVEYOR_TENSILITY:
			len=osal_strlen((char*)"����");
			strncpy((char*)pbuff,(char *)"����",len);			
			break;
			
 		case CONVEYOR_BREAKAGE:
			len=osal_strlen((char*)"�ϴ�");
			strncpy((char*)pbuff,(char *)"�ϴ�",len);			
			break;		

 		case CONVEYOR_UNDERVOLTAGE:
			len=osal_strlen((char*)"Ƿѹ");
			strncpy((char*)pbuff,(char *)"Ƿѹ",len);			
			break;		

 		case CONVEYOR_OVERCURRENT1:
			len=osal_strlen((char*)"����1");
			strncpy((char*)pbuff,(char *)"����1",len);			
			break;		

 		case CONVEYOR_OVERCURRENT2:
			len=osal_strlen((char*)"����2");
			strncpy((char*)pbuff,(char *)"����2",len);			
			break;			

 		case CONVEYOR_GAGE:
			len=osal_strlen((char*)"��λ");
			strncpy((char*)pbuff,(char *)"��λ",len);			
			break;					

		default:
			return FAILURE;

		}
	  *(pbuff+len)='\0';	
	 return SUCCESS;
}

#define   MAX_STATUS_NUM        7

uint8 Get_ConveyorStatus_by_number(char* pbuff,uint8 status )
{
	uint8 len=0;  //len  must  less  than  10
	if(status>MAX_STATUS_NUM)   return INVALIDPARAMETER;
	switch(status)
		{
 		case 0:
			len=osal_strlen((char*)"����ͣ��");
			strncpy((char*)pbuff,(char *)"����ͣ��",len);
			break;
			
 		case 1:
			len=osal_strlen((char*)"��ʱ����");
			strncpy((char*)pbuff,(char *)"��ʱ����",len);			
			break;
			
 		case 2:
			len=osal_strlen((char*)"�˹�ͣ��");
			strncpy((char*)pbuff,(char *)"�˹�ͣ��",len);			
			break;
			
 		case 3:
			len=osal_strlen((char*)"����ͣ��");
			strncpy((char*)pbuff,(char *)"����ͣ��",len);			
			break;
			
 		case 4:
			len=osal_strlen((char*)"��������");
			strncpy((char*)pbuff,(char *)"��������",len);			
			break;
			
 		case 5:
			len=osal_strlen((char*)"˳ú����");
			strncpy((char*)pbuff,(char *)"˳ú����",len);			
			break;
			
 		case 6:
			len=osal_strlen((char*)"˳ú��ͣ��");
			strncpy((char*)pbuff,(char *)"˳ú��ͣ��",len);			
			break;
			
 		case 7:
			len=osal_strlen((char*)"����ͣ��");
			strncpy((char*)pbuff,(char *)"����ͣ��",len);			
			break;
			
		default:
			return FAILURE;

		}
	  *(pbuff+len)='\0';	
	 return SUCCESS;
}




/**************************************************************************************************
*                                           Includes
**************************************************************************************************/


