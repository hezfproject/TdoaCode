/**************************************************************************************************
  Filename:       MineApp_Station.h
  Revised:        $Date: 2010/11/04 03:28:18 $
  Revision:       $Revision: 1.21 $

  Description:    Mine Application of station.
  **************************************************************************************************/
  
#ifndef MINEAPP_STATION_H
#define MINEAPP_STATION_H

#define MINEAPP_URGENT_TIME_EVENT             0x0001
#define MINEAPP_SENDCMD_EVENT               	0x0002
#define MINEAPP_SIGNALSTRENGTH_EVENT       0x0004

#ifdef ZC_REPORT_TO_ARM
#define MINEAPP_STATUS_REPORT_EVENT        0x0008
#endif
#define MINEAPP_START_NWK_EVENT    		0x0010
#if (defined WATCHDOG) &&(WATCHDOG==TRUE)
#define MINEAPP_FEEDWATCHDOG_EVENT    	0x0020
#endif

#define MINEAPP_PROBE_SPI_EVENT    			0x0040
#define MINEAPP_WDG_RESTART_EVENT    		0x0080
#define MINEAPP_DETECT_SPI_EVENT			0x0100
#define MINEAPP_DETECT_BLAST_EVENT		0x0200
#define MINEAPP_LOC_BLAST_EVENT			0x0400
#define MINEAPP_LOC_STATISTIC_EVENT		0x0800
#define MINEAPP_ADDRLIST_REPORT_EVENT	0x1000
#define MINEAPP_ADDRLIST_HEALTH_EVENT     0x2000
#define MINEAPP_SPI_READ_EVENT     			0x4000

#define MINEAPP_URGENT_TIMEOUT                  10          /* Urgent Signal Keep for query, in minutes*/
#define MINEAPP_SENDCMD_TIMEOUT          10           /* Mode Signal Keep for query, in minutes*/ 
#ifndef MINEAPP_BLAST_TIMEOUT
#define MINEAPP_BLAST_TIMEOUT          8
#endif
#ifndef MINEAPP_SPIERR_TIMEOUT
#define MINEAPP_SPIERR_TIMEOUT          12
#endif

#include "hal_types.h"
void MineApp_Init( uint8 task_id );
uint16 MineApp_ProcessEvent( uint8 task_id, uint16 events);

#endif

