#ifndef LED_UTIL_H
#define LED_UTIL_H

PUBLIC bool LedUtil_bRegister(uint32 u32LedDIO);
PUBLIC void LedUtil_vOn(uint32 u32LedDIO);
PUBLIC void LedUtil_vOff(uint32 u32LedDIO);
PUBLIC void LedUtil_vToggle(uint32 u32LedDIO);
PUBLIC void LedUtil_vFlash(uint32 u32LedDIO, uint16 u16PeriodMs, uint16 u16Times);
PUBLIC void LedUtil_vFlashAll(uint16 u16PeriodMs, uint16 u16Times);

#endif 
