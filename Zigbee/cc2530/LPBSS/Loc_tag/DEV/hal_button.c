/*******************************************************************************
  Filename:     hal_button.c
  Revised:        $Date: 16:32 2012年5月9日
  Revision:       $Revision: 1.0 $
  Description:  BSP button library

*******************************************************************************/

/*******************************************************************************
* INCLUDES
*/
#include <hal_mcu.h>
#include <hal_button.h>




/*******************************************************************************
* CONSTANTS
*/

/*******************************************************************************
* LOCAL DATA
*/

static HAL_BUTTON_CALLBACK_PFN s_pfnISR_BUTTON_1;
static HAL_BUTTON_CALLBACK_PFN s_pfnISR_WAKEUP;


/*******************************************************************************
* GLOBAL FUNCTIONS
*/

/*******************************************************************************
* @fn          HAL_BUTTON_Init
*
* @brief       配置DAT为中断输入上升沿触发
*
*
* @param    input - pfnISR 中断回调函数
*
* @return     none
*/
VOID HAL_BUTTON_Init(HAL_BUTTON_CALLBACK_PFN pfnISR)
{
    HAL_BUTTON_1_SEL    &= ~(HAL_BUTTON_1_BIT);    /* Set pin function to GPIO */
    HAL_BUTTON_1_DIR    &= ~(HAL_BUTTON_1_BIT);    /* Set pin direction to Input */
    HAL_BUTTON_1_INP0   |= (HAL_BUTTON_1_BIT);     // p0_4为上拉输入
    //HAL_BUTTON_1_INP2   &= ~(HAL_BUTTON_1_INP2BIT);
    HAL_BUTTON_1_PICTL  &= ~(HAL_BUTTON_1_EDGEBIT);   // 上升沿触发中断

    s_pfnISR_BUTTON_1 = pfnISR;
	IEN0 |= BV(7);
    /* Interrupt configuration:
     * - Enable interrupt generation at the port
     * - Enable CPU interrupt
     * - Clear any pending interrupt
     */
    HAL_BUTTON_1_ICTL |= HAL_BUTTON_1_ICTLBIT;
    HAL_BUTTON_1_IEN |= HAL_BUTTON_1_IENBIT;
    HAL_BUTTON_1_PXIFG = ~(HAL_BUTTON_1_BIT);
}


/*******************************************************************************
* @fn          HAL_Wakeup_Init
*
* @brief
*
*
* @param    input - pfnISR 中断回调函数
*
* @return     none
*/
VOID HAL_Wakeup_Init(HAL_BUTTON_CALLBACK_PFN pfnISR)
{
    HAL_WAKEUP_SEL &= ~(HAL_WAKEUP_BIT);    /* Set pin function to GPIO */
    HAL_WAKEUP_DIR &= ~(HAL_WAKEUP_BIT);    /* Set pin direction to Input */

    //PICTL &= ~(HAL_WAKEUP_EDGEBIT);    /* Clear the edge bit */
    HAL_BUTTON_1_PICTL  |= HAL_BUTTON_1_EDGEBIT;   // 下边沿触发中断

    s_pfnISR_WAKEUP = pfnISR;

    /* Interrupt configuration:
     * - Enable interrupt generation at the port
     * - Enable CPU interrupt
     * - Clear any pending interrupt
     */
    HAL_WAKEUP_ICTL |= HAL_WAKEUP_ICTLBIT;
    HAL_WAKEUP_IEN |= HAL_WAKEUP_IENBIT;
    HAL_WAKEUP_PXIFG = ~(HAL_WAKEUP_BIT);
    HAL_WAKEUP_INPX |= HAL_WAKEUP_INPXBIT;
    //HAL_WAKEUP_INPX &= ~HAL_WAKEUP_INPXBIT;
    //HAL_WAKEUP_INP2 |= HAL_WAKEUP_INP2BIT;
}
/*******************************************************************************
 *                      INTERRUPT SERVICE ROUTINE
 ******************************************************************************/

/*******************************************************************************
 * @fn      halKeyPort0Isr
 *
 * @brief   Port0 ISR
 *
 * @param
 *
 * @return
 ******************************************************************************/
HAL_ISR_FUNCTION( halKeyPort0Isr, P0INT_VECTOR )
{
    if (HAL_BUTTON_1_PXIFG & HAL_BUTTON_1_BIT)
    {
        if (s_pfnISR_BUTTON_1)
        {
            (*s_pfnISR_BUTTON_1)();
        }
    }
/*
    if (HAL_WAKEUP_PXIFG & HAL_WAKEUP_BIT)
    {
          if (s_pfnISR_WAKEUP)
          {
              (*s_pfnISR_WAKEUP)();
          }
    }
*/
    /*
    Clear the CPU interrupt flag for Port_0
    PxIFG has to be cleared before PxIF
    */
    HAL_BUTTON_1_PXIFG = 0;
    HAL_BUTTON_PORT_0_IF = 0;
}

/**************************************************************************************************
 * @fn      halKeyPort1Isr
 *
 * @brief   Port1 ISR
 *
 * @param
 *
 * @return
HAL_ISR_FUNCTION(halKeyPort1Isr, P1INT_VECTOR)
{
  HAL_ENTER_ISR();

  //CLEAR_SLEEP_MODE();
  HAL_EXIT_ISR();
}
 **************************************************************************************************/

