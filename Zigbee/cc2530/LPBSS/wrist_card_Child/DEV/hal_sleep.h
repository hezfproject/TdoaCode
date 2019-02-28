/*******************************************************************************
  Filename:     hal_sleep.h
  Revised:      $Date: 18:21 2012年5月7日
  Revision:     $Revision: 1.0 $
  
  Description:  HAL radio interface header file

*******************************************************************************/

#ifndef _HAL_SLEEP_H
#define _HAL_SLEEP_H

/*******************************************************************************
* GLOBAL FUNCTIONS
*/

/*******************************************************************************
* @fn          HAL_SLEEP_Enter
*
* @brief       进入休眠
*
*
* @param    input - u32SleepMs 休眠的时间长度，单位MS
*
* @return      none
*/
VOID    HAL_SLEEP_Enter(UINT32 u32SleepMs);

/*******************************************************************************
* @fn          HAL_SLEEP_Adjust
*
* @brief       校准休眠期间系统时钟.
*
*
* @param    none
*
* @return      休眠的时间
*/
UINT32  HAL_SLEEP_Adjust(VOID);

#endif
