#ifndef HAL_ALARM_H
#define HAL_ALARM_H

#include "hal_types.h"
/**************************************************************************************************
 * MACROS
 **************************************************************************************************/
#define MP_MAX_ALARMNUM   8

/**************************************************************************************************
 * MACROS
 **************************************************************************************************/
typedef enum
{
    MP_ALARM_INITNWK = 0,  /* the timeout menu display "searching nwk" to "no nwk" */
    MP_ALARM_JOIN,              /* the timeout for nwkstate NWK_JOINING, NWK_CELLSWITCHINHal_alarmG*/
    MP_ALARM_WAKE,         /* the timeout for static change to wake and wait for another cmd */
   // MP_ALARM_RING,                 /* thie timeout for ring */
} mp_alarm_type_t;

/**************************************************************************************************
 * MACROS
 **************************************************************************************************/

uint8 HAL_AlarmSet ( uint8 alarmtype, uint16 timeout );

void HAL_AlarmUnSet ( uint8 alarmtype );

bool HAL_AlarmIsSeting ( uint8 alarmtype, uint16 timeout );

void HAL_AlarmPoll ( void );

#endif 
