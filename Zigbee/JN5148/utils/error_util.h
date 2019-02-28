#ifndef ERROR_UTIL_H
#define ERROR_UTIL_H

PUBLIC bool_t ErrorUtil_vRegisterLed0(uint32 u32LedDIO0);
PUBLIC bool_t ErrorUtil_vRegisterLed1(uint32 u32LedDIO1);
PUBLIC bool_t ErrorUtil_vRegisterLed2(uint32 u32LedDIO2);

PUBLIC void ErrorUtil_vFatalHalt1(uint8 errorCode);
PUBLIC void ErrorUtil_vFatalHalt2(uint8 errorCode);
PUBLIC void ErrorUtil_vFatalHalt3(uint8 errorCode);

#endif
