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
			len=osal_strlen((char*)"速度");
			strncpy((char*)pbuff,(char *)"速度",len);
			break;
			
 		case CONVEYOR_CUMULI:
			len=osal_strlen((char*)"堆煤");
			strncpy((char*)pbuff,(char *)"堆煤",len);			
			break;
			
 		case CONVEYOR_OFFCENTER:
			len=osal_strlen((char*)"跑偏");
			strncpy((char*)pbuff,(char *)"跑偏",len);			
			break;
			
 		case CONVEYOR_SCRAM:
			len=osal_strlen((char*)"急停");
			strncpy((char*)pbuff,(char *)"急停",len);			
			break;
			
 		case CONVEYOR_TEMPERATURE:
			len=osal_strlen((char*)"温度");
			strncpy((char*)pbuff,(char *)"温度",len);			
			break;
			
 		case CONVEYOR_SMOG:
			len=osal_strlen((char*)"烟雾");
			strncpy((char*)pbuff,(char *)"烟雾",len);			
			break;
			
 		case CONVEYOR_LANIATE:
			len=osal_strlen((char*)"撕裂");
			strncpy((char*)pbuff,(char *)"撕裂",len);			
			break;
			
 		case CONVEYOR_TENSILITY:
			len=osal_strlen((char*)"张力");
			strncpy((char*)pbuff,(char *)"张力",len);			
			break;
			
 		case CONVEYOR_BREAKAGE:
			len=osal_strlen((char*)"断带");
			strncpy((char*)pbuff,(char *)"断带",len);			
			break;		

 		case CONVEYOR_UNDERVOLTAGE:
			len=osal_strlen((char*)"欠压");
			strncpy((char*)pbuff,(char *)"欠压",len);			
			break;		

 		case CONVEYOR_OVERCURRENT1:
			len=osal_strlen((char*)"过流1");
			strncpy((char*)pbuff,(char *)"过流1",len);			
			break;		

 		case CONVEYOR_OVERCURRENT2:
			len=osal_strlen((char*)"过流2");
			strncpy((char*)pbuff,(char *)"过流2",len);			
			break;			

 		case CONVEYOR_GAGE:
			len=osal_strlen((char*)"料位");
			strncpy((char*)pbuff,(char *)"料位",len);			
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
			len=osal_strlen((char*)"正常停车");
			strncpy((char*)pbuff,(char *)"正常停车",len);
			break;
			
 		case 1:
			len=osal_strlen((char*)"延时启车");
			strncpy((char*)pbuff,(char *)"延时启车",len);			
			break;
			
 		case 2:
			len=osal_strlen((char*)"人工停车");
			strncpy((char*)pbuff,(char *)"人工停车",len);			
			break;
			
 		case 3:
			len=osal_strlen((char*)"故障停车");
			strncpy((char*)pbuff,(char *)"故障停车",len);			
			break;
			
 		case 4:
			len=osal_strlen((char*)"正常运行");
			strncpy((char*)pbuff,(char *)"正常运行",len);			
			break;
			
 		case 5:
			len=osal_strlen((char*)"顺煤流起车");
			strncpy((char*)pbuff,(char *)"顺煤流起车",len);			
			break;
			
 		case 6:
			len=osal_strlen((char*)"顺煤流停车");
			strncpy((char*)pbuff,(char *)"顺煤流停车",len);			
			break;
			
 		case 7:
			len=osal_strlen((char*)"无线停车");
			strncpy((char*)pbuff,(char *)"无线停车",len);			
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


