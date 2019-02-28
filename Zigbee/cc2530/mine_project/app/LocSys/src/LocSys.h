/**************************************************************************************************
  Filename:       SanyRouter.h
  Revised:        $Date: 2008-06-10 11:29:36 -0700 (Tue, 10 Jun 2008) $
  Revision:       $Revision: 17192 $

  Description:    This file contains the the Mac Sample Application protypes and definitions


  Copyright 2006-2007 Texas Instruments Incorporated. All rights reserved.

  IMPORTANT: Your use of this Software is limited to those specific rights
  granted under the terms of a software license agreement between the user
  who downloaded the software, his/her employer (which must be your employer)
  and Texas Instruments Incorporated (the "License").  You may not use this
  Software unless you agree to abide by the terms of the License. The License
  limits your use, and you acknowledge, that the Software may not be modified,
  copied or distributed unless embedded on a Texas Instruments microcontroller
  or used solely and exclusively in conjunction with a Texas Instruments radio
  frequency transceiver, which is integrated into your product.  Other than for
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
**************************************************************************************************/

#ifndef MACSAMPLEAPP_H
#define MACSAMPLEAPP_H

#ifdef __cplusplus
extern "C"
{
#endif

/**************************************************************************************************
 * INCLUDES
 **************************************************************************************************/
#include "hal_types.h"

/**************************************************************************************************
 *                                        User's  Defines
 **************************************************************************************************/

#define LOCSYS_HALL_INT_ENABLED      TRUE

#define LOCSYS_KEY_INT_ENABLED       TRUE           /*
                                                 * FALSE = Key Polling
                                                 * TRUE  = Key interrupt
                                                 *
                                                 * Notes: Key interrupt will not work well with 2430 EB because
                                                 *        all the operations using up/down/left/right switch will
                                                 *        no longer work. Normally S1 + up/down/left/right is used
                                                 *        to invoke the switches but on the 2430 EB board,  the
                                                 *        GPIO for S1 is used by the LCD.
                                                 */



/**************************************************************************************************
 * CONSTANTS
 **************************************************************************************************/

/**************************************************************************************************
 * GLOBALS
 **************************************************************************************************/

/*********************************************************************
 * FUNCTIONS
 */

/*
 * Task Initialization for the Mac Sample Application
 */
extern void LocSys_Init( uint8 task_id );

/*
 * Task Event Processor for the Mac Sample Application
 */
extern uint16 LocSys_ProcessEvent(uint8 taskId, uint16 events);

/*
 * Handle keys
 */
extern void LocSys_HandleKeys( uint8 keys, uint8 shift );

extern void LocSys_HandleHall(void);

/*********************************************************************
*********************************************************************/

#ifdef __cplusplus
}
#endif

#endif /* MACSAMPLEAPP_H */
