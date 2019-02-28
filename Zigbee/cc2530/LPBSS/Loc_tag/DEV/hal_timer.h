/*******************************************************************************
  Filename:       hal_timer.h
  Revised:        $Date: 18:32 2012年5月7日
  Revision:       $Revision: 1.0 $

  Description:    定时器2模块头文件

*******************************************************************************/
#ifndef _HAL_TIMER_H
#define _HAL_TIMER_H

/*******************************************************************************
* INCLUDES
*/
#include <types.h>

/*******************************************************************************
* TYPEDEFS
*/
typedef VOID (*TIMER_CALLBACK_PFN)(VOID);

/*******************************************************************************
* GLOBAL FUNCTIONS DECLARATION
*/

/*******************************************************************************
* @fn          HAL_TIMER2_Start
*
* @brief       Initialise TIMER-2.
*
*
* @param    INPUT   pfnT2 -User timer process callback,
*                                       this is a os tick timer
*
* @return      none
*/
VOID HAL_TIMER2_Start(TIMER_CALLBACK_PFN pfnT2);

#endif
