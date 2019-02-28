/**************************************************************************************************
Filename:       SleepUtil.h
Revised:        $Date: 2010/11/26 23:14:25 $
Revision:       $Revision: 1.11 $

Description:    
**************************************************************************************************/

#ifndef SLEEP_UTIL_H
#define SLEEP_UTIL_H
#include "hal_types.h"

#ifdef __cplusplus
extern "C"
{
#endif

#define CC2430_PM0            0  /* PM0, Clock oscillators on, voltage regulator on */
#define CC2430_PM1            1  /* PM1, 32.768 kHz oscillators on, voltage regulator on */
#define CC2430_PM2            2  /* PM2, 32.768 kHz oscillators on, voltage regulator off */
#define CC2430_PM3            3  /* PM3, All clock oscillators off, voltage regulator off */

/* convert msec to 320 usec units with round */
#define HAL_SLEEP_MS_TO_320US(ms)           (((((uint32) (ms)) * 100) + 31) / 32)


extern void UtilSleep(uint8 sleep_mode, uint32 timeout_ms);
extern void UtilSleepWithMac(uint8 sleep_mode, uint16 timeout_ms);
extern void UtilSleepUs(uint8 sleep_mode, uint32 timeout_us);
extern  void UtilSleepSetTimer(uint32 timeout);
extern void Util_osal_sleep( uint16 osal_timeout );

#ifdef __cplusplus
}
#endif

#endif /* SLEEP_UTIL_H */
