#ifndef _BSP_FLASH_H_
#define _BSP_FLASH_H_

#include <types.h>
#include <hal_flash.h>

#define FLASH_START_PG  HAL_FLASH_INFOMATION_PAGE
#define FLASH_SIZE      HAL_FLASH_INFOMATION_SIZE

/* ----------- Minimum safe bus voltage ---------- */
// Vdd/3 / Internal Reference X ENOB --> (Vdd / 3) / 1.15 X 127
#define VDD_2_0  74   // 2.0 V required to safely read/write internal flash.
#define VDD_2_7  100  // 2.7 V required for the Numonyx device.
#define VDD_MIN_RUN   VDD_2_0
#define VDD_MIN_NV   (VDD_2_0+4)  // 5% margin over minimum to survive a page erase and compaction.


BOOL BSP_FLASH_Erase(uint8 pg);

/*
** 注意使用之前做好先读取这段区域是否被擦除过，其次需要判断电量是否稳定
** 请参考其他工程的OSAL_NV_CHECK_BUS_VOLTAGE
*/
BOOL BSP_FLASH_Write(uint8 pg, uint16 offset, uint8* buf, uint16 cnt);

BOOL BSP_FLASH_Read(uint8 pg, uint16 offset, uint8 *buf, uint16 cnt);

#endif

