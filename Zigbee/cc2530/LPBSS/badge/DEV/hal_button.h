/*******************************************************************************
  Filename:     hal_button.h
  Revised:      $Date: 10:52 2012年5月10日
  Revision:     $Revision: 1.0 $

  Description:  HAL button header file

*******************************************************************************/
#ifndef _HAL_BUTTON_H_
#define _HAL_BUTTON_H_

/*******************************************************************************
* INCLUDES
*/
#include <types.h>

/*******************************************************************************
* Macro
*/
/* SW_6 is at P1.7 */
#define HAL_BUTTON_1_PIN    P1_6
#define HAL_BUTTON_1_POLARITY()    (ACTIVE_LOW(HAL_BUTTON_1_PIN))
#define HAL_BUTTON_1_ISDOWN()       HAL_BUTTON_1_POLARITY()

/* SW_7 is at P1.1 */
#define HAL_BUTTON_2_PIN    P1_7
#define HAL_BUTTON_2_POLARITY()    (ACTIVE_LOW(HAL_BUTTON_2_PIN))
#define HAL_BUTTON_2_ISDOWN()       HAL_BUTTON_2_POLARITY()


/*******************************************************************************
* TYPEDEFS
*/
typedef VOID (*HAL_BUTTON_CALLBACK_PFN)(VOID);

/*******************************************************************************
* GLOBAL FUNCTIONS
*/

/*******************************************************************************
* @fn          HAL_BUTTON_Init
*
* @brief       配置按键1为中断输入下降沿触发
*
*
* @param    input - pfnISR 中断回调函数
*
* @return     none
*/
VOID HAL_BUTTON_Init(HAL_BUTTON_CALLBACK_PFN pfnISR);
VOID HAL_BUTTON0_Init(HAL_BUTTON_CALLBACK_PFN pfnISR);
bool CheckBandDisconnect(void);

#endif