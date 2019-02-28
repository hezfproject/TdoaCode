/**************************************************************************************************
  Filename:       hal_key.h
  Revised:        $Date: 2010/10/07 01:48:11 $
  Revision:       $Revision: 1.7 $

  Description:    This file contains the interface to the KEY defination for different board/enddevice.
**************************************************************************************************/

#ifndef HAL_KEY_CFG_H
#define HAL_KEY_CFG_H

#include "hal_board.h"

/**************************************************************************************************
 * MACROS
 **************************************************************************************************/

/**************************************************************************************************
 *                                            CONSTANTS
 **************************************************************************************************/

/* Interrupt option - Enable or disable */
#define HAL_KEY_INTERRUPT_DISABLE    0x00
#define HAL_KEY_INTERRUPT_ENABLE     0x01

/* Key state - ADC, nornal and so on.
*/
#define HAL_KEY_STATE_NORMAL          0x00
#define HAL_KEY_STATE_SHIFT           0x01

#ifdef HAL_BOARD_YIRIMP
#define HAL_KEY_SELECT         (1)
#define HAL_KEY_UP             (2)
#define HAL_KEY_BACKSPACE      (3)
#define HAL_KEY_CALL           (4)
#define HAL_KEY_LEFT           (5)
#define HAL_KEY_RIGHT          (6)
#define HAL_KEY_POWER          (7)
#define HAL_KEY_CANCEL          HAL_KEY_POWER
#define HAL_KEY_DOWN           (8)
#define HAL_KEY_1              (9)
#define HAL_KEY_2              (10)
#define HAL_KEY_3              (11)
#define HAL_KEY_4              (12)
#define HAL_KEY_5              (13)
#define HAL_KEY_6              (14)
#define HAL_KEY_7              (15)
#define HAL_KEY_8              (16)
#define HAL_KEY_9              (17)
#define HAL_KEY_STAR           (18)
#define HAL_KEY_0              (19)
#define HAL_KEY_POUND          (20)
#elif defined HAL_BOARD_GASMONITOR
#else
/* Switches (keys) */
#define HAL_KEY_SW_1 0x01  // Joystick up
#define HAL_KEY_SW_2 0x02  // Joystick right
#define HAL_KEY_SW_5 0x04  // Joystick center
#define HAL_KEY_SW_4 0x08  // Joystick left
#define HAL_KEY_SW_3 0x10  // Joystick down
#define HAL_KEY_SW_6 0x20  // Button S1 if available
#define HAL_KEY_SW_7 0x40  // Button S2 if available

/* Joystick */
#define HAL_KEY_UP     0x01  // Joystick up
#define HAL_KEY_RIGHT  0x02  // Joystick right
#define HAL_KEY_CENTER 0x04  // Joystick center
#define HAL_KEY_LEFT   0x08  // Joystick left
#define HAL_KEY_DOWN   0x10  // Joystick down
#endif

#endif

