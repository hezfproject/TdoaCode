#include <bsp_flash.h>
#include <hal_flash.h>
#include <hal_adc.h>

BOOL BSP_FLASH_Erase(uint8 pg)
{
    if (!HalAdcCheckVdd(VDD_MIN_NV))
        return false;

    HalFlashErase(pg);

    return true;
}

/*
** 请使用ram区域的数据进行写入
** 注意使用之前做好先读取这段区域是否被擦除过，其次需要判断电量是否稳定
** 请参考其他工程的OSAL_NV_CHECK_BUS_VOLTAGE
*/
BOOL BSP_FLASH_Write(uint8 pg, uint16 offset, uint8 *buf, uint16 cnt)
{
    uint16 addr = ((uint16)pg << 9) + (offset >> 2);

    if (!buf || !cnt)
        return false;

    if (!HalAdcCheckVdd(VDD_MIN_NV))
        return false;

    HalFlashWrite(addr, buf, cnt);

    return true;
}

BOOL BSP_FLASH_Read(uint8 pg, uint16 offset, uint8 *buf, uint16 cnt)
{
    if (!buf || !cnt)
        return false;

    if (!HalAdcCheckVdd(VDD_MIN_NV))
        return false;

    HalFlashRead(pg, offset, buf, cnt);

    return true;
}

