/**************************************************************************************************
Filename:       MineApp_Osal.c
Revised:        $Date: 2011/06/01 22:53:46 $
Revision:       $Revision: 1.1 $

Description:    Mine Application osal task configuration.
**************************************************************************************************/
/*********************************************************************
* INCLUDES
*/
#include "hal_types.h"
#include "OSAL_Tasks.h"

#include "drivers.h"

#include "nwk.h"
#include "APS.h"
#include "ZDApp.h"

#include "GasMonitor.h"

// The order in this table must be identical to the task initialization calls below in osalInitTask.
const pTaskEventHandlerFn tasksArr[] = {
#if defined( ZMAC_F8W )
	macEventLoop,
#endif
#if !defined( NONWK )
	nwk_event_loop,
#endif
	Hal_ProcessEvent,
#if !defined( NONWK )
	APS_event_loop,
	ZDApp_event_loop,
#endif
	GasMonitor_ProcessEvent,
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

#if defined( ZMAC_F8W )
	macTaskInit( taskID++ );
#endif
#if !defined( NONWK )
	nwk_init( taskID++ );
#endif
	Hal_Init( taskID++ );
#if defined( MT_TASK )
	MT_TaskInit( taskID++ );
#endif
#if !defined( NONWK )
	APS_Init( taskID++ );
	ZDApp_Init( taskID++ );
#endif
	GasMonitor_Init( taskID++ );
}

