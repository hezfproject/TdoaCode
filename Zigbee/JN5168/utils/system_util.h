#ifndef SYSTEM_UTIL_H
#define SYSTEM_UTIL_H
#include "i2c_printf_util.h"

#define sys_assert(x)   if (!(x)) do{i2c_vPrintf("assert error at %s:%d \n", __FUNCTION__, __LINE__);} while (0)
PUBLIC void SysUtil_vConvertEndian(void* pvData, uint8 u8Len);
PUBLIC int8 SysUtil_vConvertLQI2Dbm(uint8 u8Lqi);

PUBLIC uint32 SysUtil_u32GetIntVoltage();
PUBLIC uint32 SysUtil_u32GetExtVoltage();
PUBLIC uint16 SysUtil_u16GenRndNum();
PUBLIC void vSysCtrlRndCallback();
PUBLIC uint16 SysUtil_u16atou(const char *str);

PUBLIC uint32 SysUtil_u32GetBatteryVoltage();

#endif
