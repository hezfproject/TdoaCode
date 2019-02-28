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
/* SW_6 is at P0.4 */
#define HAL_BUTTON_1_PIN    P0_4
#define HAL_BUTTON_1_POLARITY()    (ACTIVE_LOW(HAL_BUTTON_1_PIN))
#define HAL_BUTTON_1_ISDOWN()       HAL_BUTTON_1_POLARITY()



/*******************************************************************************
* Macro
*/
/* CPU port interrupt */
#define HAL_BUTTON_PORT_0_IF P0IF

#define HAL_BUTTON_1_BIT    BV(4)
#define HAL_BUTTON_1_SEL    P0SEL
#define HAL_BUTTON_1_DIR    P0DIR
#define HAL_BUTTON_1_INP0   P0INP
#define HAL_BUTTON_1_INP2   P2INP
#define HAL_BUTTON_1_INP2BIT BV(5)
/* edge interrupt */
#define HAL_BUTTON_1_EDGEBIT  BV(0)

/* SW_6 interrupts */
#define HAL_BUTTON_1_IEN      IEN1  /* CPU interrupt mask register */
#define HAL_BUTTON_1_IENBIT   BV(5) /* Mask bit for all of Port_0 */
#define HAL_BUTTON_1_ICTL     P0IEN /* Port Interrupt Control register */
#define HAL_BUTTON_1_ICTLBIT  BV(4) /* P0IEN - P0.4 enable/disable bit */
#define HAL_BUTTON_1_PXIFG    P0IFG /* Interrupt flag at source */
#define HAL_BUTTON_1_PICTL    PICTL /* 边沿选择寄存器 */


/* CPU port interrupt */
//#define HAL_WAKEUP_CPU_PORT_0_IF P0IF


/* wake up at p0.6 */
#define HAL_WAKEUP_PORT   P0
#define HAL_WAKEUP_BIT    BV(6)
#define HAL_WAKEUP_SEL    P0SEL
#define HAL_WAKEUP_DIR    P0DIR
#define HAL_WAKEUP_INPX   P0INP
#define HAL_WAKEUP_INPXBIT   HAL_WAKEUP_BIT
#define HAL_WAKEUP_INP2   P2INP
#define HAL_WAKEUP_INP2BIT   BV(5)

/* edge interrupt */
#define HAL_WAKEUP_EDGEBIT  BV(0)

/* wake up interrupts */
#define HAL_WAKEUP_IEN      IEN1  /* CPU interrupt mask register */
#define HAL_WAKEUP_IENBIT   BV(5) /* Mask bit for all of Port_0 */
#define HAL_WAKEUP_ICTL     P0IEN /* Port Interrupt Control register */
#define HAL_WAKEUP_ICTLBIT  HAL_WAKEUP_BIT /* P0IEN - P0.6 enable/disable bit */
#define HAL_WAKEUP_PXIFG    P0IFG /* Interrupt flag at source */
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
VOID HAL_Wakeup_Init(HAL_BUTTON_CALLBACK_PFN pfnISR);

#endif