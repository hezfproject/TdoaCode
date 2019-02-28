/**************************************************************************************************

Filename:       MineApp.c
Revised:        $Date: 2009/09/27 17:44:27 $
Revision:       $Revision: 1.17 $


Description:    Mine Application.
**************************************************************************************************/
/*********************************************************************
* INCLUDES
*/
#include "MineApp.h"

#if (defined MINE_STATION)
#include "MineApp_Station.c"
#elif (defined MINE_ROUTER)
#include "MineApp_Router.c"
#elif (defined MINE_MP)
#include "MineApp_MP1.c"
#endif

