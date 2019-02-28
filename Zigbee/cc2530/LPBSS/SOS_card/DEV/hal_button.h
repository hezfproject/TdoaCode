/*******************************************************************************
  Filename:     hal_button.h
  Revised:      $Date: 10:52 2012��5��10��
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
/* SW_6 is at P0.2 */
#define HAL_BUTTON_1_PIN    P1_6
#define HAL_BUTTON_1_POLARITY()    (ACTIVE_LOW(HAL_BUTTON_1_PIN))
#define HAL_BUTTON_1_ISDOWN()       HAL_BUTTON_1_POLARITY()


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
* @brief       ���ð���1Ϊ�ж������½��ش���
*
*
* @param    input - pfnISR �жϻص�����
*
* @return     none
*/
VOID HAL_BUTTON_Init(HAL_BUTTON_CALLBACK_PFN pfnISR);
VOID HAL_Wakeup_Init(HAL_BUTTON_CALLBACK_PFN pfnISR);

#endif