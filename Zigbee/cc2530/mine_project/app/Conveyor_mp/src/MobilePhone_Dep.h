/**************************************************************************************************
Filename:      MPDep.h
Revised:        $Date: 2011/08/10 17:43:56 $
Revision:       $Revision: 1.1 $

Description:    Dependend interface for MP and so on.
**************************************************************************************************/
#ifndef MP_DEP_H
#define MP_DEP_H

/*********************************************************************
* INCLUDES
*/
#include "hal_types.h"

/*************************************************************************************************
*MACROS
*/
#define MINEAPP_POWER_LONGPRESS_TIMEOUT 600

/*********************************************************************
* FUNCTIONS
*/
bool MP_TestLongPress(uint16 key,uint16 TimeOut);
int8 MP_CheckVddLevel(void);
void MP_PowerOFF(void);
void MP_LongDelay(uint16 timeout, uint8 cnt);
bool MP_IsPowerOn(void);
void MP_SetPowerOn(bool powerOn);
#endif

