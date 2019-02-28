/**************************************************************************************************
  Filename:       LocationProfile.h
  Revised:        $Date: 2009/04/27 19:02:49 $
  Revision:       $Revision: 1.2 $

  Description:    Z-Stack Location Profile Application.


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

#ifndef LOCATIONPROFILE_H
#define LOCATIONPROFILE_H

// Uncomment the definition of the LocationProfile device to build.
#ifdef RTR_NWK
#define LOCATION_REFNODE
#else
#define LOCATION_BLINDNODE
#endif
//#define LOCATION_DONGLE

/*********************************************************************
 * INCLUDES
 */

#if defined ( LOCATION_REFNODE )
  #include "RefNode.h"
#endif

#if defined ( LOCATION_BLINDNODE )
  #define   CC2431
  #include "BlindNode.h"
#endif

#if defined ( LOCATION_DONGLE )
  #include "LocDongle.h"
  #include "LocationEngine.h"
#endif

/*********************************************************************
 * CONSTANTS
 */

// These endpoints are only for example and can be changed as desired.
#define LOCATION_REFNODE_ENDPOINT                 210
#define LOCATION_BLINDNODE_ENDPOINT               211
#define LOCATION_DONGLE_ENDPOINT                  203

#define LOCATION_PROFID                           0xC003

#define LOCATION_REFNODE_DEVICEID                 0x0010
#define LOCATION_BLINDNODE_DEVICEID               0x0011
#define LOCATION_DONGLE_DEVICEID                  0x0012
#define LOCATION_DEVICE_VERSION                   0
#define LOCATION_FLAGS                            0

// Cluster IDs
#define LOCATION_XY_RSSI_REQUEST                  0x0011
#define LOCATION_XY_RSSI_RESPONSE                 0x0012
#define LOCATION_BLINDNODE_FIND_REQUEST           0x0013
#define LOCATION_BLINDNODE_FIND_RESPONSE          0x0014
//#define LOCATION_REFNODE_CONFIG                   0x0015
//#define LOCATION_BLINDNODE_CONFIG                 0x0016
//#define LOCATION_REFNODE_REQUEST_CONFIG           0x0017
//#define LOCATION_BLINDNODE_REQUEST_CONFIG         0x0018

#define LOCATION_RSSI_BLAST                       0x0019

#define LOC_DEFAULT_X_Y                           0xFFFF
#define LOC_DEFAULT_MODE                          NODE_MODE_AUTO

#define LOC_DEFAULT_A                             (39 << 1)
#define LOC_DEFAULT_N                             16

// This profile uses the MSG AF service.

// LOCATION_XY_RSSI_REQUEST - message format
#define REFNODE_REQ_MSGS_IDX                      0

// LOCATION_REFNODE_CONFIG - message format
#define REFNODE_CONFIG_X_LO_IDX                   0
#define REFNODE_CONFIG_X_HI_IDX                   1
#define REFNODE_CONFIG_Y_LO_IDX                   2
#define REFNODE_CONFIG_Y_HI_IDX                   3

#define REFNODE_CONFIG_LEN                        4

// LOCATION_BLINDNODE_CONFIG - message format
#define BLINDNODE_CONFIG_A_IDX                    0
#define BLINDNODE_CONFIG_N_IDX                    1
#define BLINDNODE_CONFIG_MODE_IDX                 2
#define BLINDNODE_CONFIG_COLLECT_TIME_LO_IDX      3   // in 100 millisecond increments
#define BLINDNODE_CONFIG_COLLECT_TIME_HI_IDX      4
#define BLINDNODE_CONFIG_CYCLE_LO_IDX             5   // in 100 millisecond increments
#define BLINDNODE_CONFIG_CYCLE_HI_IDX             6
#define BLINDNODE_CONFIG_REPORT_SADDR_LO_IDX      7   // if auto mode
#define BLINDNODE_CONFIG_REPORT_SADDR_HI_IDX      8
#define BLINDNODE_CONFIG_REPORT_EP_IDX            9
#define BLINDNODE_CONFIG_MIN_REFNODES_IDX         10

#define BLINDNODE_CONFIG_LEN                      11

// Values for REFNODE_CONFIG_MODE_IDX and BLINDNODE_CONFIG_MODE_IDX
#define NODE_MODE_POLLED   0     // Only send responses to requests
#define NODE_MODE_AUTO     1     // Automatically send REFNODE response
                                 //   Depends on the cycle time (in seconds)

// LOCATION_XY_RSSI_RESPONSE - message format
#define LOCATION_XY_RSSI_X_LO_IDX                   0
#define LOCATION_XY_RSSI_X_HI_IDX                   1
#define LOCATION_XY_RSSI_Y_LO_IDX                   2
#define LOCATION_XY_RSSI_Y_HI_IDX                   3
#define LOCATION_XY_RSSI_RSSI_IDX                   4

#define LOCATION_XY_RSSI_LEN                        5

// LOCATION_BLINDNODE_RESPONSE - message format
#define BLINDNODE_RESPONSE_X_LO_IDX                 0
#define BLINDNODE_RESPONSE_X_HI_IDX                 1
#define BLINDNODE_RESPONSE_Y_LO_IDX                 2
#define BLINDNODE_RESPONSE_Y_HI_IDX                 3
#define BLINDNODE_RESPONSE_Z_LO_IDX                 4
#define BLINDNODE_RESPONSE_Z_HI_IDX                 5
#define BLINDNODE_RESPONSE_LEN                      6
#if 0
#define BLINDNODE_RESPONSE_STATUS_IDX               0
#define BLINDNODE_RESPONSE_X_LO_IDX                 1
#define BLINDNODE_RESPONSE_X_HI_IDX                 2
#define BLINDNODE_RESPONSE_Y_LO_IDX                 3
#define BLINDNODE_RESPONSE_Y_HI_IDX                 4
#define BLINDNODE_RESPONSE_Z_LO_IDX                 5
#define BLINDNODE_RESPONSE_Z_HI_IDX                 6
#define BLINDNODE_RESPONSE_NUMREFNODES_IDX          5
#define BLINDNODE_RESPONSE_REF_SHORTADDR_LO_IDX     6
#define BLINDNODE_RESPONSE_REF_SHORTADDR_HI_IDX     7
#define BLINDNODE_RESPONSE_REF_X_LO_IDX             8
#define BLINDNODE_RESPONSE_REF_X_HI_IDX             9
#define BLINDNODE_RESPONSE_REF_Y_LO_IDX             10
#define BLINDNODE_RESPONSE_REF_Y_HI_IDX             11
#define BLINDNODE_RESPONSE_REF_RSSI                 12
#define BLINDNODE_RESPONSE_LEN                      13
#endif

// Values for Blind Node Response Status field
#define BLINDNODE_RSP_STATUS_SUCCESS                0
#define BLINDNODE_RSP_STATUS_NOT_ENOUGH_REFNODES    1

// LOCATION_BLINDNODE_BLAST

// NV Items
#define LOC_NV_REFNODE_CONFIG       0x1001
#define LOC_NV_BLINDNODE_CONFIG     0x1002

/*********************************************************************
 * MACROS
 */

/*********************************************************************
 * GLOBAL VARIABLES
 */

/*********************************************************************
 * FUNCTIONS
 */

/*
 * Handles all key events for a LocationProfile device.
 */
void LocationHandleKeys( unsigned char keys );

#endif  // #ifndef LOCATIONPROFILE_H

/*********************************************************************
*********************************************************************/
