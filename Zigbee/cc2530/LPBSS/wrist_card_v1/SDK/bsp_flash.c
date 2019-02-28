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
** ��ʹ��ram��������ݽ���д��
** ע��ʹ��֮ǰ�����ȶ�ȡ��������Ƿ񱻲������������Ҫ�жϵ����Ƿ��ȶ�
** ��ο��������̵�OSAL_NV_CHECK_BUS_VOLTAGE
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

