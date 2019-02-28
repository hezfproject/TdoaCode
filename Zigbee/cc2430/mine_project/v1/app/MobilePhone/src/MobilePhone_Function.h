/**************************************************************************************************
Filename:       MP_MP_fuction.h
Revised:        $Date: 2011/04/12 01:45:50 $
Revision:       $Revision: 1.8 $

Description:    A Mine Application task except voice function of mobile phone.
**************************************************************************************************/

#ifndef MP_MP_FUCTION_H
#define MP_MP_FUCTION_H

#include "hal_types.h"

/*************************************************************************************************
*CONSTANTS
*/
extern   uint8 MP_Function_TaskID;

/*************************************************************************************************
*MACROS
*/
#define MP_FUNC_UPDATE_EVENT                    0x0001
#define MP_FUNC_MENULIB_EVENT                   0x0002
#define MP_FUNC_RESET_EVENT			     0x0004
#define MP_FUNC_CONTINUESPRESS_TEST_EVENT  0x0008
#define MP_FUNC_PERIODIC_SLEEP_EVENT       0x0010
#define MP_FUNC_SCANPREP_EVENT                  0x0020
#define MP_FUNC_PERIODIC_SCAN_EVENT        0x0040  // periodically do channel scan to get nearby stations
#define MP_FUNC_PERIODIC_SCANEND_EVENT  0x0080  // periodically do channel scan to get nearby stations
#define MP_FUNC_POLL_EVENT            	             0x0100       // must be done after channel scan finish
#define MP_FUNC_JOIN_SWITCH_EVENT            0x0200       // must be done after channel scan finish
/*********************************************************************
* FUNCTIONS
*/
void MP_Function_Init( uint8 task_id );
uint16 MP_Function_ProcessEvent( uint8 task_id, uint16 events);
void MP_Function_HandleKeys( uint16 keys, uint8 shifts);
void MP_StartMenuLibEvt (uint16 timeout);
void MP_StopMenuLibEvt (void);

#endif

