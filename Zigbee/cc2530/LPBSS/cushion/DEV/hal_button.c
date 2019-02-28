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
* @brief       配置按键1为中断输入下降沿触发
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
    HAL_BUTTON_1_INP1   |= (HAL_BUTTON_1_BIT);     // p1_3为上拉输入
    HAL_BUTTON_1_INP2   &= ~(HAL_BUTTON_1_INP2BIT);  //Port 1 pullup select.
//	HAL_BUTTON_1_INP2	|= (HAL_BUTTON_1_INP2BIT);  //Port 1 pullup select.

    HAL_BUTTON_1_PICTL  |= HAL_BUTTON_1_EDGEBIT;   // 下边沿触发中断

	HAL_BUTTON_2_SEL    &= ~(HAL_BUTTON_2_BIT);    /* Set pin function to GPIO */
    HAL_BUTTON_2_DIR    &= ~(HAL_BUTTON_2_BIT);    /* Set pin direction to Input */
    HAL_BUTTON_2_INP1   |= (HAL_BUTTON_2_BIT);     // p1_4为上拉输入
//    HAL_BUTTON_2_INP2   &= ~(HAL_BUTTON_2_INP2BIT);
    HAL_BUTTON_2_PICTL  &= ~(HAL_BUTTON_2_EDGEBIT);   //上升沿触发中断

	HAL_BUTTON_5_SEL    &= ~(HAL_BUTTON_5_BIT);    /* Set pin function to GPIO */
    HAL_BUTTON_5_DIR    &= ~(HAL_BUTTON_5_BIT);    /* Set pin direction to Input */
	HAL_BUTTON_5_INP0   |= (HAL_BUTTON_5_BIT);     // p0_5为下拉输入
    
    s_pfnISR_BUTTON_1 = pfnISR;
//	IEN0 |=(1<<7);
    /* Interrupt configuration:
     * - Enable interrupt generation at the port
     * - Enable CPU interrupt
     * - Clear any pending interrupt
     */
	HAL_BUTTON_2_IEN |= HAL_BUTTON_1_IENBIT;         //IEN2 for port1
    HAL_BUTTON_1_ICTL |= HAL_BUTTON_1_ICTLBIT;       //P1IEN

    HAL_BUTTON_2_ICTL |= HAL_BUTTON_2_ICTLBIT;
//    HAL_BUTTON_2_IEN |= HAL_BUTTON_2_IENBIT;
	HAL_BUTTON_1_PXIFG &= ~(HAL_BUTTON_1_BIT);
    HAL_BUTTON_2_PXIFG &= ~(HAL_BUTTON_2_BIT);
	

	
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
    HAL_BUTTON_2_SEL    &= ~(HAL_BUTTON_2_BIT);    /* Set pin function to GPIO */
    HAL_BUTTON_2_DIR    &= ~(HAL_BUTTON_2_BIT);    /* Set pin direction to Input */
    HAL_BUTTON_2_INP1   &= ~(HAL_BUTTON_2_BIT);     // p1_4为上拉输入
    HAL_BUTTON_2_INP2   &= ~(HAL_BUTTON_2_INP2BIT);
    HAL_BUTTON_2_PICTL  &= ~(HAL_BUTTON_2_EDGEBIT);   //上升沿触发中断

    s_pfnISR_WAKEUP = pfnISR;
    /* Interrupt configuration:
     * - Enable interrupt generation at the port
     * - Enable CPU interrupt
     * - Clear any pending interrupt*/

    HAL_BUTTON_2_ICTL |= HAL_BUTTON_2_ICTLBIT;
    HAL_BUTTON_2_IEN |= HAL_BUTTON_2_IENBIT;
    HAL_BUTTON_2_PXIFG &= ~(HAL_BUTTON_2_BIT);

}
/*******************************************************************************
 *                      INTERRUPT SERVICE ROUTINE
 ******************************************************************************/

/*******************************************************************************
 * @fn      halKeyPort1Isr
 *
 * @brief   Port1 ISR
 *
 * @param
 *
 * @return
 ******************************************************************************/
HAL_ISR_FUNCTION( halKeyPort1Isr, P1INT_VECTOR )
{
    if ((HAL_BUTTON_1_PXIFG & HAL_BUTTON_1_BIT) || (HAL_BUTTON_2_PXIFG & HAL_BUTTON_2_BIT))
    {
    /*
    	if (HAL_BUTTON_1_PXIFG & HAL_BUTTON_1_BIT)
		{
			Rising_edge=false;
			Falling_edge=true;
		}
		if(HAL_BUTTON_2_PXIFG & HAL_BUTTON_2_BIT)
		{
			Rising_edge=true;
			Falling_edge=false;
		}
		*/
        if (s_pfnISR_BUTTON_1)
        {
            (*s_pfnISR_BUTTON_1)();
        }
    }

    /*
    Clear the CPU interrupt flag for Port_0
    PxIFG has to be cleared before PxIF
    */
    HAL_BUTTON_1_PXIFG = 0;
    HAL_BUTTON_PORT_1_IF = 0;
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

