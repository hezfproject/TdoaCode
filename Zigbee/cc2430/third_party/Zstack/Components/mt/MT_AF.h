/**************************************************************************************************
  Filename:       MT_AF.h
  Revised:        $Date: 2009/02/07 01:08:32 $
  Revision:       $Revision: 1.1.1.1 $

  Description:    MonitorTest functions for AF.


  Copyright 2004-2007 Texas Instruments Incorporated. All rights reserved.

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

#ifndef MT_AF_H
#define MT_AF_H

/*********************************************************************
 * INCLUDES
 */

#include "ZComDef.h"
#include "MTEL.h"
#include "AF.h"

#include "OnBoard.h"

/*********************************************************************
 * MACROS
 */
#if defined ( MT_AF_CB_FUNC )
#define AFCB_CHECK(ep,task,cbi) ( ((ep!=0) && (_afCallbackSub & (1 << (cbi & 0x0F))) ) ||   \
                                                      (task == MT_TaskID) )
#else
#define AFCB_CHECK(ep,cbi) 
#endif

/*********************************************************************
 * CONSTANTS
 */
#define SPI_AF_CB_TYPE                    0x0900
/*********************************************************************
 * TYPEDEFS
 */

/*********************************************************************
 * GLOBAL VARIABLES
 */

#if defined ( MT_AF_CB_FUNC )
extern uint16 _afCallbackSub;
#endif

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

#if defined ( MT_AF_FUNC )
/*********************************************************************
 *
 */
void MT_afCommandProcessing( uint16 cmd_id , byte len , byte *pData );
#endif

#if defined ( MT_AF_CB_FUNC )
/*
 * Process the callback subscription for AF Incoming data.
 */
void af_MTCB_IncomingData( void *pkt );

void MT_AfDataConfirm( uint8 endPoint, uint8 transID, ZStatus_t status );

#endif

/*********************************************************************
*********************************************************************/
#endif
