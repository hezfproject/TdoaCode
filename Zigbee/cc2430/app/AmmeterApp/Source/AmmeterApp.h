/**************************************************************************************************
  Filename:       AmmeterApp.h
  Revised:        $Date: 2010/05/11 03:20:18 $
  Revision:       $Revision: 1.1 $

  Description:    This file contains the Ammeter Transfer Application definitions.

**************************************************************************************************/

#ifndef AMMETERAPP_H
#define AMMETERAPP_H

#ifdef __cplusplus
extern "C"
{
#endif

/*********************************************************************
 * INCLUDES
 */
#if (defined AMMETER_COLLECTOR)
#include "AmmeterApp_Collector.h"
#elif (defined AMMETER_REPEATER)
#include "AmmeterApp_Repeater.h"
#elif (defined AMMETER_DATACENTER)
#include "AmmeterApp_DataCenter.h"
#endif

/*********************************************************************
*********************************************************************/

#ifdef __cplusplus
}
#endif

#endif
