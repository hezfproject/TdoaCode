/**************************************************************************************************
  Filename:      MPDep.h
  Revised:        $Date: 2011/01/12 00:42:07 $
  Revision:       $Revision: 1.3 $

  Description:    Dependend interface for MP and so on.
  **************************************************************************************************/
#ifndef MP_DEP_H
#define MP_DEP_H

/*********************************************************************
* INCLUDES
*/
#include "hal_types.h"

/*************************************************************************************************
  *MACROS
  */
#define MINEAPP_POWER_LONGPRESS_TIMEOUT 1000

/*********************************************************************
* FUNCTIONS
*/
bool MP_TestLongPress(uint16 key,uint16 TimeOut);
bool MP_vdd_check( void );
void MP_PowerOFF(void);
void MP_ShutDown(void);
void MP_LongDelay(uint16 timeout, uint8 cnt);
bool MP_IsPowerOn(void);
void MP_SetPowerOn(bool powerOn);

#endif

