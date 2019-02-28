/*******************************************************************************
  Filename:     bsp_led.c
  Revised:        $Date: 15:47 2012年5月9日
  Revision:       $Revision: 1.0 $
  Description:  BSP Led library

*******************************************************************************/
#ifndef _BSP_LED_H_
#define _BSP_LED_H_

#include <hal_mcu.h>
#include <bsp.h>

/*******************************************************************************
* GLOBAL FUNCTIONS
*/
#define BSP_LED_RED_ON()        HAL_TURN_ON_LED1()
#define BSP_LED_GREEN_ON()      HAL_TURN_ON_LED2()
#define BSP_LED_RED_OFF()       HAL_TURN_OFF_LED1()
#define BSP_LED_GREEN_OFF()     HAL_TURN_OFF_LED2()
#define BSP_LED_RED_TOGGLE()    HAL_TOGGLE_LED1()
#define BSP_LED_GREEN_TOGGLE()  HAL_TOGGLE_LED2()
#define BSP_LED_ALL_OFF()       st(BSP_LED_RED_OFF(); BSP_LED_GREEN_OFF();)

typedef enum
{
    LED_RED = 1,
    LED_GREEN = 2,
    LED_ALL = 3
}LED_E;

typedef enum
{
    LED_OFF = 1,
    LED_ON = 0,
}LED_STATE_E;

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
VOID BSP_LED_Init(VOID);

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
VOID BSP_LED_ALL_Flash(VOID);


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
VOID BSP_LED_RED_Flash(VOID);

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
VOID BSP_LED_GREEN_Flash(VOID);

VOID BSP_LED_Set(LED_E leds, LED_STATE_E state);

#endif
