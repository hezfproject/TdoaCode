/*******************************************************************************
  Filename:     hal_ultra_sound.c
  Revised:      $Date: 16:32 2012年5月9日
  Revision:     $Revision: 1.0 $
  Description:  BSP ultra library

*******************************************************************************/

/*******************************************************************************
* INCLUDES
*/
#include <hal_mcu.h>
#include <hal_ultra_sound.h>

/*******************************************************************************
* Macro
*/

#define HAL_PORT2_INP         P2INP
#define HAL_PORT2_INP_P0_BIT  BV(5)  //set p0 up or down pull

#define HAL_P0_EA_ENABLE_BIT  BV(7)
#define HAL_P0_ICTL           BV(0)
#define HAL_PORT_0_IF         P0IF

/* hot view PRI (P0.5) interrupt */
#define HAL_HOT_VIEW_PORT     P0
#define HAL_HOT_VIEW_BIT      BV(5)
#define HAL_HOT_VIEW_SEL      P0SEL 
#define HAL_HOT_VIEW_DIR      P0DIR 
#define HAL_HOT_VIEW_INPX     P0INP 
#define HAL_HOT_VIEW_INPXBIT  HAL_HOT_VIEW_BIT
#define HAL_HOT_VIEW_IEN      P0IEN
#define HAL_HOT_VIEW_ICTL     PICTL
#define HAL_HOT_VIEW_P0IFG    P0IFG

/*
 * temp test
 */

#define HAL_PORT2_INP_P1_BIT  BV(6)  //set p1 up or down pull

#define HAL_P1_EA_ENABLE_BIT  BV(4)
#define HAL_P1_ICTL           BV(1)
#define HAL_PORT_1_IF         P1IF

/* hot view PRI (P1.3) interrupt */
#define HAL_HOT_VIEW_PORT_T     P1
#define HAL_HOT_VIEW_BIT_T      BV(3)
#define HAL_HOT_VIEW_SEL_T      P1SEL 
#define HAL_HOT_VIEW_DIR_T      P1DIR 
#define HAL_HOT_VIEW_INPX_T     P1INP 
#define HAL_HOT_VIEW_INPXBIT_T  HAL_HOT_VIEW_BIT_T

#define HAL_HOT_VIEW_IEN_T      P1IEN
#define HAL_HOT_VIEW_ICTL_T     PICTL
#define HAL_HOT_VIEW_P1IFG_T    P1IFG

#define _TEMP_TEST_

/*******************************************************************************
* CONSTANTS
*/

/*******************************************************************************
* LOCAL DATA
*/

static HAL_ULTRA_CALLBACK_PFN s_pfnISR_HOTVIEW;


/*******************************************************************************
* GLOBAL FUNCTIONS
*/

/*******************************************************************************
* @fn         HAL_HotView_Init
* @brief
* @param      input - pfnISR 中断回调函数
* @return     none
*/
VOID HAL_HotView_Init(HAL_ULTRA_CALLBACK_PFN pfnISR)
{

#ifndef _TEMP_TEST_
    HAL_HOT_VIEW_SEL    &= ~(HAL_HOT_VIEW_BIT);    // Set pin function to gpio   
    HAL_HOT_VIEW_DIR    &= ~(HAL_HOT_VIEW_BIT);    // Set pin direction to input 
    //HAL_HOT_VIEW_INPX   &= ~(HAL_HOT_VIEW_BIT);    // Set pin pull(up/down) mode 

    //HAL_PORT2_INP   &= ~(HAL_PORT2_INP_P0_BIT);    // Set P0 pull up mode
    HAL_HOT_VIEW_ICTL &= ~(HAL_P0_ICTL);           // Set P0 Rising edge interrupt 

    s_pfnISR_HOTVIEW = pfnISR;

    IEN0 |= HAL_P0_EA_ENABLE_BIT;                  // Set P0 EA enable bit
    HAL_HOT_VIEW_IEN  |= HAL_HOT_VIEW_BIT;         // Set P0.5 enable interrupt 
    HAL_HOT_VIEW_P0IFG  &= ~(HAL_HOT_VIEW_BIT);    // Clear P0IFG.5 flag info 
#else
    HAL_HOT_VIEW_SEL_T   &= ~(HAL_HOT_VIEW_BIT_T);    // Set pin function to gpio 
    HAL_HOT_VIEW_DIR_T   &= ~(HAL_HOT_VIEW_BIT_T);    // Set pin direction to input 
    //HAL_HOT_VIEW_INPX_T  &= ~(HAL_HOT_VIEW_BIT_T);    // Set pin pull(up/down) mode 
    //HAL_HOT_VIEW_INPX_T  |= (HAL_HOT_VIEW_BIT_T);     // Set pin 3status mode 

    //HAL_PORT2_INP   &= ~(HAL_PORT2_INP_P1_BIT);       // Set P1 pull up mode 
    //HAL_PORT2_INP   |= (HAL_PORT2_INP_P1_BIT);        // Set P1 pull down mode 
    HAL_HOT_VIEW_ICTL_T &= ~(HAL_P1_ICTL);            // Set P1 Rising edge interrupt


    s_pfnISR_HOTVIEW = pfnISR;
    // Interrupt configuration:
    //  - Enable interrupt generation at the port
    //  - Enable CPU interrupt
    //  - Clear any pending interrupt

    IEN0 |= HAL_P0_EA_ENABLE_BIT;                    // Set EA
    IEN2 |= HAL_P1_EA_ENABLE_BIT;                    // Set P1 EA enable bit 
    HAL_HOT_VIEW_IEN_T  |= HAL_HOT_VIEW_BIT_T;       // Set P1.3 enable interrupt 
    HAL_HOT_VIEW_P1IFG_T  &= ~(HAL_HOT_VIEW_BIT_T);  // Clear P0IFG.3 flag info 
#endif
}

/*******************************************************************************
 *                      INTERRUPT SERVICE ROUTINE
 ******************************************************************************/

/*******************************************************************************
 * @fn      halKeyPort0Isr
 * @brief 
 * @param
 * @return
 ******************************************************************************/
#ifndef _TEMP_TEST_
HAL_ISR_FUNCTION( halKeyPort0Isr, P0INT_VECTOR )
{
    if ( (HAL_HOT_VIEW_P0IFG & HAL_HOT_VIEW_BIT) )
    {
        if (s_pfnISR_HOTVIEW)
        {
            (*s_pfnISR_HOTVIEW)();
        }
    }

    HAL_HOT_VIEW_P0IFG &= ~(HAL_HOT_VIEW_BIT);
    HAL_PORT_0_IF       = 0;
}
#else
HAL_ISR_FUNCTION( halKeyPort1Isr, P1INT_VECTOR )
{
    if ( (HAL_HOT_VIEW_P1IFG_T & HAL_HOT_VIEW_BIT_T) )
    {
        if (s_pfnISR_HOTVIEW)
        {
            (*s_pfnISR_HOTVIEW)();
        }
    }

    HAL_HOT_VIEW_P1IFG_T &= ~(HAL_HOT_VIEW_BIT_T);
    HAL_PORT_1_IF        = 0;
}
#endif
