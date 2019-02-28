/**************************************************************************************************
  Filename:       AmmeterApp.c
  Revised:        $Date: 2010/05/11 03:20:18 $
  Revision:       $Revision: 1.1 $

  Description -   Ammeter Data Transfer Application.

**************************************************************************************************/
/*********************************************************************
* INCLUDES
*/
#include "AmmeterApp.h"

#if (defined AMMETER_COLLECTOR)
#include "AmmeterApp_Collector.c"
#elif (defined AMMETER_REPEATER)
#include "AmmeterApp_Repeater.c"
#elif (defined AMMETER_DATACENTER)
#include "AmmeterApp_DataCenter.c"
#endif

