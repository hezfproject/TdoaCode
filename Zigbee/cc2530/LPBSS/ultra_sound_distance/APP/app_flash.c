#include <bsp_flash.h>
#include <string.h>
#include <mem.h>
#include <hal_mcu.h>
//#include <hal_flash.h>
//#include <hal_adc.h>

//#define  OSAL_NV_CHECK_BUS_VOLTAGE  (HalAdcCheckVdd( VDD_MIN_NV ))

bool app_FLASH_Erase(uint8 pg)
{
    return BSP_FLASH_Erase(pg);
}

/*
 ** 请使用ram区域的数据进行写入
 ** 注意使用之前做好先读取这段区域是否被擦除过，其次需要判断电量是否稳定
 */
bool app_FLASH_Write(uint8 pg, uint16 offset, uint8 *buf, uint16 cnt)
{
    uint8* pgBuf = NULL;
    bool rs;
    UINT8 u8Critical;

    if (!buf || !cnt||(offset>=HAL_FLASH_PAGE_SIZE)||(cnt>HAL_FLASH_PAGE_SIZE))
        return false;

    HAL_ENTER_CRITICAL_SECTION(u8Critical);

    pgBuf = rt_malloc(HAL_FLASH_PAGE_SIZE+10);
    if (!pgBuf)
        return false;

#if 0
    if(offset)
    {
        rs = BSP_FLASH_Read(pg, 0, pgBuf, offset);        
        if(!rs)
        {
            rt_free(pgBuf);
            return false;
        }    
    }
#else
    rs = BSP_FLASH_Read(pg, 0, pgBuf, HAL_FLASH_PAGE_SIZE);
    if(!rs)
    {
        rt_free(pgBuf);
        return false;
    }  

#endif

    if(offset+cnt>HAL_FLASH_PAGE_SIZE)
    {
        rt_free(pgBuf);
        return false;
    }

    memcpy(pgBuf+offset, buf, cnt);

    HAL_EXIT_CRITICAL_SECTION(u8Critical);

    rs = BSP_FLASH_Erase(pg);
    if(!rs)
    {
        rt_free(pgBuf);
        return false;
    }
#if 0    
    rs = BSP_FLASH_Write(pg, 0, pgBuf, offset+cnt);
#else
    rs = BSP_FLASH_Write(pg, 0, pgBuf, HAL_FLASH_PAGE_SIZE/HAL_FLASH_WORD_SIZE);
#endif
    if(!rs)
    {
        rt_free(pgBuf);
        return false;
    }
    rt_free(pgBuf);

    return true;
}

bool app_FLASH_Read(uint8 pg, uint16 offset, uint8 *buf, uint16 cnt)
{
    return BSP_FLASH_Read(pg, offset, buf, cnt);
}

