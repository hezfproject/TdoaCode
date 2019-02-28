/**************************************************************************************************
  Filename:       ZDConfig.c
  Revised:        $Date: 2009/02/07 01:08:32 $
  Revision:       $Revision: 1.1.1.1 $

  Description:    This file contains the configuration attributes for the Zigbee Device Object.
                  These are references to Configuration items that MUST be defined in ZDApp.c.
                  The names mustn't change.


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

/*********************************************************************
 * INCLUDES
 */
#include "ZComdef.h"
#include "AF.h"
#include "ZDObject.h"
#include "ZDConfig.h"

 /*********************************************************************
 * MACROS
 */

/*********************************************************************
 * CONSTANTS
 */

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
 * LOCAL FUNCTIONS
 */

/*********************************************************************
 * LOCAL VARIABLES
 */

NodeDescriptorFormat_t ZDO_Config_Node_Descriptor =
{
#if defined( ZDO_COORDINATOR ) && !defined( SOFT_START )
  NODETYPE_COORDINATOR,
#elif defined (RTR_NWK)
	NODETYPE_ROUTER,
#else
  NODETYPE_DEVICE,          // Logical Type
#endif
  0,                        // User Descriptor Available is set later.
  0,                        // Complex Descriptor Available is set later.
  0,			              		// Reserved
  0,	        	            // NO APS flags
  NODEFREQ_2400,            // Frequency Band
  // MAC Capabilities
#if defined (RTR_NWK)
  (
  #if defined( ZDO_COORDINATOR ) || defined( SOFT_START )
    CAPINFO_ALTPANCOORD |
  #endif
    CAPINFO_DEVICETYPE_FFD |
    CAPINFO_POWER_AC |
    CAPINFO_RCVR_ON_IDLE ),
#else
  CAPINFO_DEVICETYPE_RFD
  #if ( RFD_RCVC_ALWAYS_ON == TRUE)
    | CAPINFO_RCVR_ON_IDLE
  #endif
  ,
#endif
  { 0x00, 0x00 },           // Manfacturer Code - *YOU FILL IN*
  MAX_BUFFER_SIZE,          // Maximum Buffer Size.
  // The maximum transfer size field isn't used and spec says to set to 0.
  {0, 0},
  ( 0
#if defined( ZDO_COORDINATOR ) && ( SECURE != 0 )    
    | PRIM_TRUST_CENTER
#endif      
  )
};

NodePowerDescriptorFormat_t ZDO_Config_Power_Descriptor =
{
#if defined ( RTR_NWK )
  NODECURPWR_RCVR_ALWAYS_ON,
  NODEAVAILPWR_MAINS,       // available power sources
  NODEAVAILPWR_MAINS,       // current power source
  NODEPOWER_LEVEL_100       // Power Level
#else
  // Assume End Device
#if defined ( NWK_AUTO_POLL )
  NODECURPWR_RCVR_AUTO,
#else
  NODECURPWR_RCVR_STIM,
#endif
  NODEAVAILPWR_RECHARGE,    // available power sources
  NODEAVAILPWR_RECHARGE,    // current power source
  NODEPOWER_LEVEL_66        // Power Level
#endif
};

/*********************************************************************
*********************************************************************/


