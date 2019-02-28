/******************************************************************************
  Filename:       OnBoard.c
  Revised:        $Date: 2011/04/01 22:13:07 $
  Revision:       $Revision: 1.2 $

  Description:    This file contains the UI and control for the
                  peripherals on the EVAL development board
  Notes:          This file targets the Texas Instruments CC2530EB


  Copyright 2008-2010 Texas Instruments Incorporated. All rights reserved.

  IMPORTANT: Your use of this Software is limited to those specific rights
  granted under the terms of a software license agreement between the user
  who downloaded the software, his/her employer (which must be your employer)
  and Texas Instruments Incorporated (the "License").  You may not use this
  Software unless you agree to abide by the terms of the License. The License
  limits your use, and you acknowledge, that the Software may not be modified,
  copied or distributed unless embedded on a Texas Instruments microcontroller
  or used solely and exclusively in conjunction with a Texas Instruments radio
  frequency transceiver, which is integrated into your product. Other than for
  the foregoing purpose, you may not use, reproduce, copy, prepare derivative
  works of, modify, distribute, perform, display or sell this Software and/or
  its documentation for any purpose.

  YOU FURTHER ACKNOWLEDGE AND AGREE THAT THE SOFTWARE AND DOCUMENTATION ARE
  PROVIDED “AS IS?WITHOUT WARRANTY OF ANY KIND, EITHER EXPRESS OR IMPLIED,
  INCLUDING WITHOUT LIMITATION, ANY WARRANTY OF MERCHANTABILITY, TITLE,
  NON-INFRINGEMENT AND FITNESS FOR A PARTICULAR PURPOSE. IN NO EVENT SHALL
  TEXAS INSTRUMENTS OR ITS LICENSORS BE LIABLE OR OBLIGATED UNDER CONTRACT,
  NEGLIGENCE, STRICT LIABILITY, CONTRIBUTION, BREACH OF WARRANTY, OR OTHER
  LEGAL EQUITABLE THEORY ANY DIRECT OR INDIRECT DAMAGES OR EXPENSES
  INCLUDING BUT NOT LIMITED TO ANY INCIDENTAL, SPECIAL, INDIRECT, PUNITIVE
  OR CONSEQUENTIAL DAMAGES, LOST PROFITS OR LOST DATA, COST OF PROCUREMENT
  OF SUBSTITUTE GOODS, TECHNOLOGY, SERVICES, OR ANY CLAIMS BY THIRD PARTIES
  (INCLUDING BUT NOT LIMITED TO ANY DEFENSE THEREOF), OR OTHER SIMILAR COSTS.

  Should you have any questions regarding your right to use this Software,
  contact Texas Instruments Incorporated at www.TI.com.
******************************************************************************/

/******************************************************************************
 * INCLUDES
 */
#include "OnBoard.h"
#include "mac_radio_defs.h"
#include "ZComdef.h"
/******************************************************************************
 * MACROS
 */
#define NO_TASK_ID 0xFF

/******************************************************************************
 * TYPEDEFS
 */


/******************************************************************************
 * GLOBAL VARIABLES
 */
static uint8 registeredKeysTaskID = NO_TASK_ID;
/******************************************************************************
 * EXTERNAL VARIABLES
 */

/******************************************************************************
 * EXTERNAL FUNCTIONS
 */

/*********************************************************************
*                        "Keyboard" Support
*********************************************************************/

/*********************************************************************
 * Keyboard Register function
 *
 * The keyboard handler is setup to send all keyboard changes to
 * one task (if a task is registered).
 *
 * If a task registers, it will get all the keys. You can change this
 * to register for individual keys.
 *********************************************************************/
uint8 RegisterForKeys(uint8 task_id)
{
  // Allow only the first task
  if ( registeredKeysTaskID == NO_TASK_ID )
  {
    registeredKeysTaskID = task_id;
    return (true);
  }
  else
    return ( false );
}

/*********************************************************************
 * @fn      OnBoard_SendKeys
 *
 * @brief   Send "Key Pressed" message to application.
 *
 * @param   keys  - keys that were pressed
 *          state - shifted
 *
 * @return  status
 *********************************************************************/
uint8 OnBoard_SendKeys(uint8 keys, uint8 state)
{
    keyChange_t *msgPtr;

    if(registeredKeysTaskID != NO_TASK_ID)
    {
        // Send the address to the task
        msgPtr = (keyChange_t *)osal_msg_allocate(sizeof(keyChange_t));
        if(msgPtr)
        {
            msgPtr->hdr.event = KEY_CHANGE;
            msgPtr->state = state;
            msgPtr->keys = keys;

            osal_msg_send(registeredKeysTaskID, (uint8 *)msgPtr);
        }
        return (ZSuccess);
    }
    else
        return (ZFailure);
}

/*********************************************************************
 * @fn      _itoa
 *
 * @brief   convert a 16bit number to ASCII
 *
 * @param   num -
 *          buf -
 *          radix -
 *
 * @return  void
 *
 *********************************************************************/
void _itoa(uint16 num, char *buf, uint8 radix)
{
    char c, i;
    char *p, rst[5];

    p = rst;
    for(i = 0; i < 5; i++, p++)
    {
        c = num % radix;  // Isolate a digit
        *p = c + ((c < 10) ? '0' : '7');    // Convert to Ascii
        num /= radix;
        if(!num)
            break;
    }

    for(c = 0 ; c <= i; c++)
        *buf++ = *p--;  // Reverse character order

    *buf = '\0';
}

/******************************************************************************
 * @fn        Onboard_rand
 *
 * @brief    Random number generator
 *
 * @param   none
 *
 * @return  uint16 - new random number
 *
 */
uint16 Onboard_rand(void)
{
    return (MAC_RADIO_RANDOM_WORD());
}

/******************************************************************************
 * @fn      Onboard_soft_reset
 *
 * @brief   Effect a soft reset.
 *
 * @param   none
 *
 * @return  none
 *
 */
__near_func void Onboard_soft_reset(void)
{
    HAL_DISABLE_INTERRUPTS();
    // Abort all DMA channels to insure that ongoing operations do not
    // interfere with re-configuration.
    DMAARM = 0x80 | 0x1F;
    asm("LJMP 0x0");
}
void  SystemReset(void)
{
    HAL_DISABLE_INTERRUPTS();
    HAL_SYSTEM_RESET();
}

/******************************************************************************
 */
