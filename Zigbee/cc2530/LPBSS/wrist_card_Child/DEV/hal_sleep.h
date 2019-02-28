/*******************************************************************************
  Filename:     hal_sleep.h
  Revised:      $Date: 18:21 2012��5��7��
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
* @brief       ��������
*
*
* @param    input - u32SleepMs ���ߵ�ʱ�䳤�ȣ���λMS
*
* @return      none
*/
VOID    HAL_SLEEP_Enter(UINT32 u32SleepMs);

/*******************************************************************************
* @fn          HAL_SLEEP_Adjust
*
* @brief       У׼�����ڼ�ϵͳʱ��.
*
*
* @param    none
*
* @return      ���ߵ�ʱ��
*/
UINT32  HAL_SLEEP_Adjust(VOID);

#endif
