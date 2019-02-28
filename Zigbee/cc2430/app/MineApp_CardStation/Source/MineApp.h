/**************************************************************************************************
  Filename:       MineApp.h
  Revised:        $Date: 2011/07/26 23:45:33 $
  Revision:       $Revision: 1.1 $

  Description:    Mine Application.
  **************************************************************************************************/

#ifndef MINE_APP_H
#define MINE_APP_H

#ifdef ZDO_COORDINATOR
#define MINE_STATION
#elif (defined RTR_NWK) && (!defined HAL_AUDIO)
#define MINE_ROUTER
#elif (defined HAL_AUDIO) && (HAL_AUDIO == TRUE)
#define MINE_MP
#endif
/*********************************************************************
 * INCLUDES
 */
#if (defined MINE_STATION)
#include "MineApp_Station.h"
#elif (defined MINE_ROUTER)
#include "MineApp_Router.h"
#elif (defined MINE_MP)
#include "MineApp_MP.h"
#endif

#endif

