/**************************************************************************************************
  Filename:       OSAL_AmmeterApp.c
  Revised:        $Date: 2010/05/11 03:20:18 $
  Revision:       $Revision: 1.1 $

  Description:    This file contains all the settings and other functions
                  that the user should set and change.

**************************************************************************************************/

/*********************************************************************
 * INCLUDES
 */
#include "ZComDef.h"
#include "hal_drivers.h"
#include "OSAL.h"
#include "OSAL_Tasks.h"
#include "OSAL_Custom.h"

#if defined ( MT_TASK )
  #include "MTEL.h"
#endif

#include "nwk.h"
#include "APS.h"
#include "ZDApp.h"

#include "AmmeterApp.h"

/*********************************************************************
 * GLOBAL VARIABLES
 */

// The order in this table must be identical to the task initialization calls below in osalInitTask.
const pTaskEventHandlerFn tasksArr[] = {
  macEventLoop,
  nwk_event_loop,
  Hal_ProcessEvent,
#if defined( MT_TASK )
  MT_ProcessEvent,
#endif
  APS_event_loop,
  ZDApp_event_loop,
  AmmeterApp_ProcessEvent
};

const uint8 tasksCnt = sizeof( tasksArr ) / sizeof( tasksArr[0] );
uint16 *tasksEvents;

/*********************************************************************
 * FUNCTIONS
 *********************************************************************/

/*********************************************************************
 * @fn      osalInitTasks
 *
 * @brief   This function invokes the initialization function for each task.
 *
 * @param   void
 *
 * @return  none
 */
void osalInitTasks( void )
{
  uint8 taskID = 0;

  tasksEvents = (uint16 *)osal_mem_alloc( sizeof( uint16 ) * tasksCnt);
  osal_memset( tasksEvents, 0, (sizeof( uint16 ) * tasksCnt));

  macTaskInit( taskID++);
  nwk_init( taskID++ );
  Hal_Init( taskID++ );
#if defined( MT_TASK )
  MT_TaskInit( taskID++);
#endif
  APS_Init( taskID++ );
  ZDApp_Init( taskID++ );
  AmmeterApp_Init( taskID );
}

/*********************************************************************
*********************************************************************/
