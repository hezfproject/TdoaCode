/*******************************************************************************
  Filename:     bsp_led.c
  Revised:        $Date: 15:47 2012年5月9日
  Revision:       $Revision: 1.0 $
  Description:  BSP Led library

*******************************************************************************/

/*******************************************************************************
* INCLUDES
*/
#include <bsp_led.h>
/*******************************************************************************
* Macro
*/
#ifndef LED_ON_TIME
#define LED_ON_TIME 5
#endif

/*******************************************************************************
* CONSTANTS
*/

/*******************************************************************************
* GLOBAL FUNCTIONS
*/

/*******************************************************************************
* @fn          BSP_LED_Init
*
* @brief       LED初始化接口
*
* @param       none
*
*
* @return      none
*/
VOID BSP_LED_Init(VOID)
{
    BSP_LED_RED_OFF();
    BSP_LED_GREEN_OFF();
}

/*******************************************************************************
* @fn          BSP_LED_ALL_Flash
*
* @brief       LED闪烁接口
*
* @param       none
*
*
* @return      none
*/
VOID BSP_LED_ALL_Flash(VOID)
{
    BSP_LED_RED_TOGGLE();
    BSP_LED_GREEN_TOGGLE();
    HAL_LED_BLINK_DELAY();
#ifdef OPEN_WTD
    BSP_WATCHDOG_Feed();
#endif
    BSP_LED_RED_TOGGLE();
    BSP_LED_GREEN_TOGGLE();
    HAL_LED_BLINK_DELAY();
#ifdef OPEN_WTD
    BSP_WATCHDOG_Feed();
#endif
}

/*******************************************************************************
* @fn          BSP_LED_RED_Flash
*
* @brief       LED红色闪烁接口
*
* @param       none
*
*
* @return      none
*/
VOID BSP_LED_RED_Flash(VOID)
{
    BSP_LED_RED_TOGGLE();
    BSP_SLEEP_Enter(LED_ON_TIME);
#ifdef OPEN_WTD
    BSP_WATCHDOG_Feed();
#endif
    BSP_LED_RED_TOGGLE();
}

/*******************************************************************************
* @fn          BSP_LED_GREEN_Flash
*
* @brief       LED绿色闪烁接口
*
* @param       none
*
*
* @return      none
*/
VOID BSP_LED_GREEN_Flash(VOID)
{
    BSP_LED_GREEN_TOGGLE();
    //BSP_SLEEP_Enter(LED_ON_TIME);

#ifdef OPEN_WTD
    BSP_WATCHDOG_Feed();
#endif
    BSP_LED_GREEN_TOGGLE();
}

VOID BSP_LED_Set(LED_E leds, LED_STATE_E state)
{
    if (leds == LED_ALL)
    {
        if (state == LED_ON)
        {
            BSP_LED_RED_ON();
            BSP_LED_GREEN_ON();
        }
        else
        {
            BSP_LED_RED_OFF();
            BSP_LED_GREEN_OFF();
        }
        return;
    }
    if (leds & LED_RED)
    {
        if (state == LED_ON)
            BSP_LED_RED_ON();
        else
            BSP_LED_RED_OFF();
    }
    else if (leds & LED_GREEN)
    {
        if (state == LED_ON)
            BSP_LED_GREEN_ON();
        else
            BSP_LED_GREEN_OFF();
    }
}

