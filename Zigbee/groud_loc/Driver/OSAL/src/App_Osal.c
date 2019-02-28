/**************************************************************************************************
  Filename:       App_Osal.c
**************************************************************************************************/

/**************************************************************************************************
 *                                            INCLUDES
 **************************************************************************************************/
#include <stdlib.h>
#include <string.h>

#include "OSAL.h"
#include "OSAL_Tasks.h"

/* HAL */
//#include "hal_drivers.h"


/* Application */
#include "App.h"
#include "HAL.h"

/*********************************************************************
 * GLOBAL VARIABLES
 */

// The order in this table must be identical to the task initialization calls below in osalInitTask.
const pTaskEventHandlerFn tasksArr[] =
{
  App_ProcessEvent,
  Hal_ProcessEvent, //处理按键事件
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

  tasksEvents = (uint16 *)malloc( sizeof( uint16 ) * tasksCnt); //这里tasksCnt=2
  memset( tasksEvents, 0, (sizeof( uint16 ) * tasksCnt));   

  App_Init( taskID++ );
  Hal_Init( taskID++ );
}

/*********************************************************************
*********************************************************************/

