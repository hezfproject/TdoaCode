/**************************************************************************************************
Filename:      GasMonitorDep.h
Revised:        $Date: 2010/12/02 23:37:29 $
Revision:       $Revision: 1.7 $

Description:    Dependend interface for Gas Monitor and so on.
**************************************************************************************************/
#ifndef GASMONITOR_DEP_H
#define GASMONITOR_DEP_H

/*********************************************************************
* INCLUDES
*/
#include "hal_types.h"

/*************************************************************************************************
*MACROS
*/
#define GASMONITOR_POWER_LONGPRESS_TIMEOUT 2000
#define GASMONITOR_HELP_LONGPRESS_TIMEOUT 1000

/*********************************************************************
* FUNCTIONS
*/
bool GasMonitor_TestLongPress(uint16 keys,uint16 TimeOut);
bool GasMonitor_vdd_check( void );
void GasMonitor_PowerOFF(void);
void GasMonitor_ShutDown(void);
void GasMonitor_LongDelay(uint16 timeout, uint8 cnt);
bool GasMonitor_IsPowerOn(void);
void GasMonitor_SetPowerOn(bool powerOn);

uint8 GasMonitor_InitFlashInfo(void);
uint8 GasMonitor_WriteInfoToFlash(void);
uint8 GasMonitor_ReadInfoFromFlash(void);

void Gasmonitor_SaveParam2RAM(void);
void Gasmonitor_ReadParamFromRAM(void);
void Gasmonitor_ReadDevDateInfo(void);
void Gasmonitor_SaveDevDateInfo(void);
void Gasmonitor_UpdateSleepTime(void);
#endif

