/**************************************************************************************************
Filename:       GasMonitor.h
Revised:        $Date: 2011/07/22 18:53:36 $
Revision:       $Revision: 1.4 $

Description:    Mine Application of station.
**************************************************************************************************/

#ifndef GASMONITOR_H
#define GASMONITOR_H

#include "AppProtocol.h"
#include "hal_types.h"

#define GASMONITOR_FEEDWATCHDOG_EVENT     0x0001
#define GASMONITOR_KEYDEJITTER_EVENT       0x0002
#define GASMONITOR_MENULIB_EVENT               0x0004
#define GASMONITOR_UPDATEGAS_EVENT	  0x0008	    
#define GASMONITOR_UPDATETIME_EVENT	  0x0010
#define GASMONITOR_GASDENSITYALERT_EVENT	  0x0020
#define GASMONITOR_POWERON_EVENT	  0x0040
#define GASMONITOR_POWEROFF_EVENT	  0x0080
#define GASMONITOR_CALLHELP_EVENT	  0x0100
#define GASMONITOR_MOTORCLOSE_EVENT	  0x0200
#define GASMONITOR_SOSALARM_EVENT        0x0400
#define GASMONITOR_ITSELFMENULIB_EVENT    0x0800
#define GASMONITOR_POWERON_ALARM_EVENT 0x1000

#define GASMONITOR_UART_READ0_EVENT     0x2000
#define GASMONITOR_UART_SEND_EVENT       0x4000

extern uint8 GasMonitor_TaskID;

void GasMonitor_Init( uint8 task_id );
uint16 GasMonitor_ProcessEvent( uint8 task_id, uint16 events);
void GasMonitor_StartMenuLibEvt (uint16 timeout);
void GasMonitor_StopMenuLibEvt (void);
uint16 GasMonitor_GetDensity(void);
uint8 GasMonitor_RSSI2Level(int8 RSSI);
void GasMonitor_SOSAlarm_startsend(GasAlarmType Alarmtype);
void GasMonitor_SOSAlarm_endsend(void);
void GasMonitor_StopItselfMenuLibEvt(void);
void GasMonitor_StartItselfMenuLibEvt (uint16 timeout);
uint16 GasMonitor_GetLocateDistance(void);
uint16 GasMonitor_GetLocateTargetName(void);

#endif

