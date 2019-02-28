/*******************************************************************************
  Filename:     hal_button.c
  Revised:        $Date: 16:32 2012��5��9��
  Revision:       $Revision: 1.0 $
  Description:  BSP button library

*******************************************************************************/

/*******************************************************************************
* INCLUDES
*/
#include <hal_mcu.h>
#include <hal_button.h>

/*******************************************************************************
* Macro
*/
/* CPU port interrupt */
#define HAL_BUTTON_PORT_1_IF P1IF

#define HAL_BUTTON_1_BIT    BV(7)
#define HAL_BUTTON_1_SEL    P1SEL
#define HAL_BUTTON_1_DIR    P1DIR
#define HAL_BUTTON_1_INP0   P1INP
#define HAL_BUTTON_1_INP2   P2INP
#define HAL_BUTTON_1_INP2BIT BV(6)

#define HAL_BUTTON_2_BIT    BV(6)
#define HAL_BUTTON_2_SEL    P1SEL
#define HAL_BUTTON_2_DIR    P1DIR
#define HAL_BUTTON_2_INP2   P2INP
#define HAL_BUTTON_2_INP2BIT BV(6)

#define MMA_845XQ_INT_BIT      BV(4)
#define MMA_845XQ_INT_SEL	   P1SEL
#define MMA_845XQ_INT_DIR	   P1DIR
#define MMA_845XQ_INT_INP1	   P1INP


#define HAL_BUTTON_CHARGE_BIT    BV(0)
#define HAL_BUTTON_CHARGE_SEL    P0SEL
#define HAL_BUTTON_CHARGE_DIR    P0DIR
#define HAL_BUTTON_CHARGE_INP0   P0INP
#define HAL_BUTTON_CHARGE_INP2   P2INP
#define HAL_BUTTON_CHARGE_INP2BIT BV(5)



/* edge interrupt */
#define HAL_BUTTON_1_EDGEBIT  BV(2)

/* SW_6 interrupts */
#define HAL_BUTTON_1_IEN      IEN2  /* CPU interrupt mask register */
#define HAL_BUTTON_1_IENBIT   BV(4) /* Mask bit for all of Port_1 */
#define HAL_BUTTON_1_ICTL     P1IEN /* Port Interrupt Control register */
#define HAL_BUTTON_1_ICTLBIT  BV(7) /* P1IEN - P1.7 enable/disable bit */
#define HAL_BUTTON_1_PXIFG    P1IFG /* Interrupt flag at source */
#define HAL_BUTTON_1_PICTL    PICTL /* ����ѡ��Ĵ��� */


/* edge interrupt */
#define HAL_BUTTON_CHARGE_EDGEBIT  BV(0)
/* SW_6 interrupts */
#define HAL_BUTTON_CHARGE_IEN      IEN1  /* CPU interrupt mask register */
#define HAL_BUTTON_CHARGE_IENBIT   BV(5) /* Mask bit for all of Port_1 */
#define HAL_BUTTON_CHARGE_ICTL     P0IEN /* Port Interrupt Control register */
#define HAL_BUTTON_CHARGE_ICTLBIT  BV(0) /* P0IEN - P0.6 enable/disable bit */
#define HAL_BUTTON_CHARGE_PXIFG    P0IFG /* Interrupt flag at source */
#define HAL_BUTTON_CHARGE_PICTL    PICTL /* ����ѡ��Ĵ��� */


/* SW_6 interrupts */
#define MMA_845XQ_INT_IEN      IEN2  /* CPU interrupt mask register */
#define MMA_845XQ_INT_IENBIT   BV(4) /* Mask bit for all of Port_1 */
#define MMA_845XQ_INT_ICTL     P1IEN /* Port Interrupt Control register */
#define MMA_845XQ_INT_ICTLBIT  BV(4) /* P1IEN - P1.4enable/disable bit */
#define MMA_845XQ_INT_PXIFG    P1IFG /* Interrupt flag at source */
#define MMA_845XQ_INT_PICTL    PICTL /* ����ѡ��Ĵ��� */



/* edge interrupt */
#define HAL_BUTTON_2_EDGEBIT  BV(2)

/* SW_6 interrupts */
#define HAL_BUTTON_2_IEN      IEN2  /* CPU interrupt mask register */
#define HAL_BUTTON_2_IENBIT   BV(4) /* Mask bit for all of Port_1 */
#define HAL_BUTTON_2_ICTL     P1IEN /* Port Interrupt Control register */
#define HAL_BUTTON_2_ICTLBIT  BV(6) /* P1IEN - P1.1 enable/disable bit */
#define HAL_BUTTON_2_PXIFG    P1IFG /* Interrupt flag at source */
#define HAL_BUTTON_2_PICTL    PICTL /* ����ѡ��Ĵ��� */


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
* CONSTANTS
*/

/*******************************************************************************
* LOCAL DATA
*/

static HAL_BUTTON_CALLBACK_PFN s_pfnISR_BUTTON_1;
static HAL_BUTTON_CALLBACK_PFN s_pfnISR_BUTTON_0;


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
VOID HAL_BUTTON_Init(HAL_BUTTON_CALLBACK_PFN pfnISR)
{
    HAL_BUTTON_1_SEL    &= ~(HAL_BUTTON_1_BIT);    /* Set pin function to GPIO */
    HAL_BUTTON_1_DIR    &= ~(HAL_BUTTON_1_BIT);    /* Set pin direction to Input */
    HAL_BUTTON_1_INP0   &= ~(HAL_BUTTON_1_BIT);     // p1_7Ϊ��������
    HAL_BUTTON_1_INP2   &= ~(HAL_BUTTON_1_INP2BIT);
    HAL_BUTTON_1_PICTL  |= HAL_BUTTON_1_EDGEBIT;   // �±��ش����ж�

    HAL_BUTTON_2_SEL    &= ~(HAL_BUTTON_2_BIT);    /* Set pin function to GPIO */
    HAL_BUTTON_2_DIR    &= ~(HAL_BUTTON_2_BIT);    /* Set pin direction to Input */
    HAL_BUTTON_2_INP2   &= ~(HAL_BUTTON_2_INP2BIT);
    HAL_BUTTON_2_PICTL  |= HAL_BUTTON_2_EDGEBIT;   // �±��ش����ж�



    s_pfnISR_BUTTON_1 = pfnISR;
    /* Interrupt configuration:
     * - Enable interrupt generation at the port
     * - Enable CPU interrupt
     * - Clear any pending interrupt
     */
    IEN0 |= HAL_BUTTON_1_BIT;
    HAL_BUTTON_1_ICTL |= HAL_BUTTON_1_ICTLBIT;
    HAL_BUTTON_1_IEN |= HAL_BUTTON_1_IENBIT;
    HAL_BUTTON_1_PXIFG &= ~(HAL_BUTTON_1_BIT);



    //MMA_845XQ_INT_ICTL |= MMA_845XQ_INT_ICTLBIT;
    //MMA_845XQ_INT_IEN |= MMA_845XQ_INT_IENBIT;
    //MMA_845XQ_INT_PXIFG &= ~(MMA_845XQ_INT_BIT);

    HAL_BUTTON_2_ICTL |= HAL_BUTTON_2_ICTLBIT;
    HAL_BUTTON_2_IEN |= HAL_BUTTON_2_IENBIT;
    HAL_BUTTON_2_PXIFG &= ~(HAL_BUTTON_2_BIT);
}


/*******************************************************************************
* @fn          HAL_Wakeup_Init
*
* @brief
*
*
* @param    input - pfnISR �жϻص�����
*
* @return     none
*/
VOID HAL_BUTTON0_Init(HAL_BUTTON_CALLBACK_PFN pfnISR)
{

    HAL_BUTTON_CHARGE_SEL &= ~(HAL_BUTTON_CHARGE_BIT);
    HAL_BUTTON_CHARGE_DIR    &= ~(HAL_BUTTON_CHARGE_BIT);    /* Set pin direction to Input */
    HAL_BUTTON_CHARGE_INP0   |= (HAL_BUTTON_CHARGE_BIT);     // p0_0Ϊ��������
    //HAL_BUTTON_CHARGE_INP2   != (HAL_BUTTON_CHARGE_INP2BIT);
    HAL_BUTTON_CHARGE_PICTL  &= ~(HAL_BUTTON_CHARGE_EDGEBIT);   // �����ش����ж�

    s_pfnISR_BUTTON_0 = pfnISR;
    IEN0 |= HAL_BUTTON_1_BIT;

    /* Interrupt configuration:
     * - Enable interrupt generation at the port
     * - Enable CPU interrupt
     * - Clear any pending interrupt
     */
    HAL_BUTTON_CHARGE_ICTL |= HAL_BUTTON_CHARGE_ICTLBIT;
    HAL_BUTTON_CHARGE_IEN |= HAL_BUTTON_CHARGE_IENBIT;
    HAL_BUTTON_CHARGE_PXIFG &= ~(HAL_BUTTON_CHARGE_BIT);
}

/*******************************************************************************
* @fn          CheckBandDisconnect
*
* @brief
*
* @param    none
*
* @return   true :disconnect;false:connect
*/
bool CheckBandDisconnect(void)
{
    if(P1_6 == 1)
    {
        return true;
    }
    else
    {
       return false;
    }
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
 * @return601377
 ******************************************************************************/
HAL_ISR_FUNCTION( halKeyPort0Isr, P0INT_VECTOR )
{
    if (HAL_BUTTON_CHARGE_PXIFG & HAL_BUTTON_CHARGE_BIT)
    {
        if (s_pfnISR_BUTTON_0)
        {
            (*s_pfnISR_BUTTON_0)();
        }
    }

    /*if (HAL_WAKEUP_PXIFG & HAL_WAKEUP_BIT)
    {
          if (s_pfnISR_WAKEUP)
          {
              (*s_pfnISR_WAKEUP)();
          }
    }*/

    /*
    Clear the CPU interrupt flag for Port_0
    PxIFG has to be cleared before PxIF
    */

    P0IFG = 0;
    P0IF = 0;
    //HAL_BUTTON_1_PXIFG = 0;
    //HAL_BUTTON_PORT_1_IF = 0;
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
HAL_ISR_FUNCTION( halKeyPort1Isr, P1INT_VECTOR )
{
    if ((HAL_BUTTON_1_PXIFG & HAL_BUTTON_1_BIT)
		|| HAL_BUTTON_2_PXIFG & HAL_BUTTON_2_BIT)
    {
        if (s_pfnISR_BUTTON_1)
        {
            (*s_pfnISR_BUTTON_1)();
        }
    }

    /*if (HAL_WAKEUP_PXIFG & HAL_WAKEUP_BIT)
    {
          if (s_pfnISR_WAKEUP)
          {
              (*s_pfnISR_WAKEUP)();
          }
    }*/

    /*
    Clear the CPU interrupt flag for Port_0
    PxIFG has to be cleared before PxIF
    */

    //P0IFG = 0;
    //P0IF = 0;
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

