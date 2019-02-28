/*******************************************************************************
  Filename:       hal_hall.h
  Revised:        $Date: 2011-09-07 11:12:24

  Description:    This file contains the interface to the Hall element Service.
*******************************************************************************/

#ifndef HAL_HALL_H
#define HAL_HALL_H

#include "hal_types.h"

/*******************************************************************************
 * Type Defined
 ******************************************************************************/
typedef void (*halHallCBack_t) (void);

/*******************************************************************************
 * MACROS
 ******************************************************************************/
/* Interrupt option - Enable or disable */
#define HAL_HALL_INTERRUPT_DISABLE  0x00
#define HAL_HALL_INTERRUPT_ENABLE   0x01
#define HAL_KEY_CPU_PORT_1_IF       P1IF

#define HAL_HALL_FALLING_EDGE       1
#define HAL_HALL_EDGEBIT            BV(1)
#define HAL_HALL_EDGE               HAL_HALL_FALLING_EDGE

#define HAL_HALL_ICTL               P1IEN
#define HAL_HALL_ICTLBIT            BV(0)
#define HAL_HALL_IEN                IEN2
#define HAL_HALL_IENBIT             BV(4)
#define HAL_HALL_IFG                P1IFG
#define HAL_HALL_BIT                BV(0)
#define HAL_HALL_SEL                P1SEL
#define HAL_HALL_DIR                P1DIR

/*******************************************************************************
 * FUNCTION
 ******************************************************************************/
extern void HalHallInit(void);

extern void HalHallConfig(bool bInterruptEnable, halHallCBack_t pfnCback);

extern void HalHallProcessEvent(void);

/*******************************************************************************
*******************************************************************************/
#endif
