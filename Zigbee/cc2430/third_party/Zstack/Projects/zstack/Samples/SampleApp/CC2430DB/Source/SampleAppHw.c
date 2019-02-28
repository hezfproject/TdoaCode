/**************************************************************************************************
  Filename:       SampleAppHw.c
  Revised:        $Date: 2009/02/07 01:08:33 $
  Revision:       $Revision: 1.1.1.1 $

  Description:    Describe the purpose and contents of the file.


  Copyright 2007 Texas Instruments Incorporated. All rights reserved.

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
  PROVIDED “AS IS” WITHOUT WARRANTY OF ANY KIND, EITHER EXPRESS OR IMPLIED, 
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

/*********************************************************************
 * INCLUDES
 */
#include "ZComDef.h"
#include "hal_mcu.h"
#include "hal_defs.h"

#include "SampleAppHw.h"

/*********************************************************************
 * MACROS
 */

/*********************************************************************
 * CONSTANTS
 */

#if defined( CC2430DB )
  /* NOTES:   Jumper should be between P7.1 and P7.3 on the CC2430DB.
   *          P7.1 -> CC2430 P1.3
   *          p7.3 -> CC2430 P0.4
   */
  #define JUMPERIN_BIT  0x08
  #define JUMPERIN_SEL  P1SEL
  #define JUMPERIN_DIR  P1DIR
  #define JUMPERIN_INP  P1INP
  #define JUMPERIN      P1
  
  #define JUMPEROUT_BIT 0x10
  #define JUMPEROUT_SEL P0SEL
  #define JUMPEROUT_DIR P0DIR
  #define JUMPEROUT_INP P0INP
  #define JUMPEROUT     P0

#elif defined( CC2430EB )
  /* NOTES:   Jumper should be between I/O A pin 9 and 11 on the CC2430EB.
   *          I/O A pin 9  -> CC2430 P0.2
   *          I/O A pin 11 -> CC2430 P0.3
   */
  #define JUMPERIN_BIT  0x04
  #define JUMPERIN_SEL  P0SEL
  #define JUMPERIN_DIR  P0DIR
  #define JUMPERIN_INP  P0INP
  #define JUMPERIN      P0
  
  #define JUMPEROUT_BIT 0x08
  #define JUMPEROUT_SEL P0SEL
  #define JUMPEROUT_DIR P0DIR
  #define JUMPEROUT_INP P0INP
  #define JUMPEROUT     P0

  #if defined (HAL_UART) && (HAL_UART==TRUE)
    #error The UART will not work with this configuration. The RX & TX pins are used.
  #endif
#else
  #error Unsupported board
#endif
/*********************************************************************
 * TYPEDEFS
 */

/*********************************************************************
 * GLOBAL VARIABLES
 */

/*********************************************************************
 * EXTERNAL VARIABLES
 */

/*********************************************************************
 * EXTERNAL FUNCTIONS
 */

/*********************************************************************
 * LOCAL VARIABLES
 */

/*********************************************************************
 * LOCAL FUNCTIONS
 */

/*********************************************************************
 * @fn      readCoordinatorJumper
 *
 * @brief   Checks for a jumper to determine if the device should
 *          become a coordinator
 *
 * NOTES:   Jumper should be between P7.1 and P7.3 on the CC2430DB.
 *          P7.1 -> CC2430 P1.3
 *          p7.3 -> CC2430 P0.4
 *
 * NOTES:   Jumper should be between I/O A pin 9 and 11 on the CC2430EB.
 *          I/O A pin 9  -> CC2430 P0.2
 *          I/O A pin 11 -> CC2430 P0.3
 *
 * @return  TRUE if the jumper is there, FALSE if not
 */
uint8 readCoordinatorJumper( void )
{
  uint8 jumpered;
  uint8 x, y;
  uint8 result;
  uint8 saveJumpInSEL;
  uint8 saveJumpInDIR;
  uint8 saveJumpInINP;
  uint8 saveJumpOutSEL;
  uint8 saveJumpOutDIR;
  uint8 saveJumpOutINP;
  
  jumpered = TRUE;
  
  // Setup PORTs
  saveJumpInSEL = JUMPERIN_SEL;
  saveJumpInDIR = JUMPERIN_DIR;
  saveJumpInINP = JUMPERIN_INP;
  saveJumpOutSEL = P0SEL;
  saveJumpOutDIR = P0DIR;
  saveJumpOutINP = P0INP;
  
  JUMPERIN_SEL &= ~(JUMPERIN_BIT);
  JUMPERIN_DIR &= ~(JUMPERIN_BIT);
  JUMPERIN_INP &= ~(JUMPERIN_BIT);
  JUMPEROUT_SEL &= ~(JUMPEROUT_BIT);
  JUMPEROUT_DIR |= JUMPEROUT_BIT;
  JUMPEROUT_INP &= ~(JUMPERIN_BIT);
  
  for ( x = 0; x < 8; x++ )
  {    
    if ( x & 0x01 )
    {
      JUMPEROUT |= JUMPEROUT_BIT;      
      for ( y = 0; y < 8; y++ );
      result = JUMPERIN & JUMPERIN_BIT;
      if ( result != JUMPERIN_BIT )
        jumpered = FALSE;
    }
    else
    {      
      JUMPEROUT &= ~(JUMPEROUT_BIT);      
      for ( y = 0; y < 8; y++ );
      result = JUMPERIN & JUMPERIN_BIT;
      if ( result != 0x00 )
        jumpered = FALSE;
    }
  }
  
  // Restore directions
  JUMPERIN_SEL = saveJumpInSEL;
  JUMPERIN_DIR = saveJumpInDIR;
  JUMPERIN_INP = saveJumpInINP;
  JUMPEROUT_SEL = saveJumpOutSEL;
  JUMPEROUT_DIR = saveJumpOutDIR;
  JUMPEROUT_INP = saveJumpOutINP;
  
  return ( jumpered );
}
