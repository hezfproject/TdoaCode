/**************************************************************************************************
  Filename:       MineApp_Router.h
  Revised:        $Date: 2009/09/27 17:44:27 $
  Revision:       $Revision: 1.1 $

  Description:    Mine Application of Router(for none star-network).
  **************************************************************************************************/
  
#ifndef MINEAPP_ROUTER_H
#define MINEAPP_ROUTER_H

#define MINEAPP_URGENT_TIME_EVENT              0x0001
#define MINEAPP_SLEEPPERIOD_TIME_EVENT     0x0002
#define MINEAPP_SIGNALSTRENGTH_EVENT        0x0004
#define MINEAPP_ROUTER_PROBENWK_EVENT    0x0008

#define MINEAPP_URGENT_TIMEOUT                  10          /* Urgent Signal Keep for query, in minutes*/
#define MINEAPP_SLEEPPERIOD_TIMEOUT         10           /* Mode Signal Keep for query, in minutes*/ 

#include "hal_types.h"
void MineApp_Init( uint8 task_id );
uint16 MineApp_ProcessEvent( uint8 task_id, uint16 events);
#endif

