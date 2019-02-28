/*******************************************************************************
  Filename:       hal_hall.c
  Revised:        $Date: 2011-09-07 11:12:24

  Description:    This file contains the interface to the Hall element Service.
*******************************************************************************/
#include "hal_hall.h"
#include "hal_mcu.h"
#include "osal.h"
#include "hal_drivers.h"

/*******************************************************************************
 * Variable Definition
 ******************************************************************************/
static halHallCBack_t s_pfnInterruptCback;


/*******************************************************************************
 * FUNCTION
 ******************************************************************************/
static void halProcessHallInterrupt(void)
{
    bool valid = FALSE;

    if (HAL_HALL_IFG & HAL_HALL_BIT)  /* Interrupt Flag has been set */
    {
        HAL_HALL_IFG = ~(HAL_HALL_BIT); /* Clear Interrupt Flag */
        valid = TRUE;
    }

    if (valid)
    {
        osal_set_event(Hal_TaskID, HAL_HALL_EVENT);
    }
}

void HalHallInit(void)
{
    HAL_HALL_SEL &= ~(HAL_HALL_BIT);    /* Set pin function to GPIO */
    HAL_HALL_DIR &= ~(HAL_HALL_BIT);    /* Set pin direction to Input */
    s_pfnInterruptCback = NULL;
}

void HalHallConfig(bool bInterruptEnable, halHallCBack_t pfnCback)
{
    /* Determine if interrupt is enable or not */
    if (bInterruptEnable && pfnCback)
    {
        /* Rising/Falling edge configuratinn */
        PICTL &= ~(HAL_HALL_EDGEBIT);    /* Clear the edge bit */
        /* For falling edge, the bit must be set. */
#if (HAL_HALL_EDGE == HAL_HALL_FALLING_EDGE)
        PICTL |= HAL_HALL_EDGEBIT;
#endif

        /* Interrupt configuration:
         * - Enable interrupt generation at the port
         * - Enable CPU interrupt
         * - Clear any pending interrupt
         */
        HAL_HALL_ICTL |= HAL_HALL_ICTLBIT;
        HAL_HALL_IEN |= HAL_HALL_IENBIT;
        HAL_HALL_IFG = ~(HAL_HALL_BIT);
        s_pfnInterruptCback = pfnCback;
    }
}

void HalHallProcessEvent(void)
{
    if (NULL == s_pfnInterruptCback)
        return;
    (s_pfnInterruptCback)();
}

/**************************************************************************************************
 * @fn      halKeyPort1Isr
 *
 * @brief   Port1 ISR
 *
 * @param
 *
 * @return
 **************************************************************************************************/
HAL_ISR_FUNCTION(HallPort0Isr, P0INT_VECTOR)
{
    HAL_ENTER_ISR();

    if (HAL_HALL_IFG & HAL_HALL_BIT)
    {
        halProcessHallInterrupt();
    }

    /*
      Clear the CPU interrupt flag for Port_0
      PxIFG has to be cleared before PxIF
    */
    HAL_HALL_IFG = 0;
    HAL_HALL_CPU_PORT_0_IF = 0;

    CLEAR_SLEEP_MODE();
    HAL_EXIT_ISR();
}

/*******************************************************************************
*******************************************************************************/
