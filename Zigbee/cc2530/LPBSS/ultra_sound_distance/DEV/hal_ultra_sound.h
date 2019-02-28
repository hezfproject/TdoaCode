/*******************************************************************************
  Filename:     hal_ultra_sound.h
  Revised:      $Date: 10:52 2012年5月10日
  Revision:     $Revision: 1.0 $

  Description:  HAL ultrasonic and hot view head file

*******************************************************************************/
#ifndef _HAL_ULTRA_SOUND
#define _HAL_ULTRA_SOUND

/*******************************************************************************
* INCLUDES
*/
#include <types.h>

/*******************************************************************************
* Macro
*/

/* Hot view PRI is at P0.5 */
#define HAL_HOT_VIEW_PRI   P0_5
#define HAL_HOT_VIEW_POLARITY()    (ACTIVE_LOW(HAL_HOT_VIEW_PRI))
#define HAL_HOT_VIEW_ISDOWN()       HAL_HOT_VIEW_POLARITY()


/*******************************************************************************
* TYPEDEFS
*/
typedef VOID (*HAL_ULTRA_CALLBACK_PFN)(VOID);

/*******************************************************************************
* GLOBAL FUNCTIONS
*/

/*******************************************************************************
* @fn          HAL_HotView_Init
* @brief       配置 PRI (P0.5) 为中断输入下降沿触发
* @param       input - pfnISR 中断回调函数
* @return      none
*/

VOID HAL_HotView_Init(HAL_ULTRA_CALLBACK_PFN pfnISR);

#endif
