/**************************************************************************************************
  Filename:       ChargeED_Osal.c
  Revised:        $Date: 2010/07/07 02:25:00 $
  Revision:       $Revision: 1.2 $

  Description:    This file contains function that allows user setup tasks

**************************************************************************************************/

/**************************************************************************************************
 *                                            INCLUDES
 **************************************************************************************************/
#include "hal_types.h"
#include "OSAL.h"
#include "OSAL_Tasks.h"
#include "OSAL_Custom.h"
#include "OnBoard.h"
#include "mac_api.h"

/* HAL */
#include "hal_drivers.h"

/* Application */
#include "ChargeED.h"

/*********************************************************************
 * GLOBAL VARIABLES
 */

const pTaskEventHandlerFn tasksArr[] = {
  macEventLoop,
  Hal_ProcessEvent,
  ChargeED_ProcessEvent,
};

const uint8 tasksCnt = sizeof( tasksArr ) / sizeof( tasksArr[0] );
uint16 tasksEvents_Mem[8];
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

  tasksEvents = (uint16 *)tasksEvents_Mem;//(uint16 *)osal_mem_alloc( sizeof( uint16 ) * tasksCnt);
  osal_memset( tasksEvents, 0, (sizeof( uint16 ) * tasksCnt));

  macTaskInit( taskID++ );
  Hal_Init( taskID++ );
  ChargeED_Init( taskID++ );
}

/**************************************************************************************************
**************************************************************************************************/
