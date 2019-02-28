#include <rtthread.h>
#include <stm32f2xx.h>
#include "stm32_flash.h"

/**
  * @brief  Gets the sector of a given address
  * @param  None
  * @retval The sector of a given address
  */
uint32_t GetSector(uint32_t Address)
{
  uint32_t sector = 0;

  if((Address < ADDR_FLASH_SECTOR_1) && (Address >= ADDR_FLASH_SECTOR_0))
  {
    sector = FLASH_Sector_0;
  }
  else if((Address < ADDR_FLASH_SECTOR_2) && (Address >= ADDR_FLASH_SECTOR_1))
  {
    sector = FLASH_Sector_1;
  }
  else if((Address < ADDR_FLASH_SECTOR_3) && (Address >= ADDR_FLASH_SECTOR_2))
  {
    sector = FLASH_Sector_2;
  }
  else if((Address < ADDR_FLASH_SECTOR_4) && (Address >= ADDR_FLASH_SECTOR_3))
  {
    sector = FLASH_Sector_3;
  }
  else if((Address < ADDR_FLASH_SECTOR_5) && (Address >= ADDR_FLASH_SECTOR_4))
  {
    sector = FLASH_Sector_4;
  }
  else if((Address < ADDR_FLASH_SECTOR_6) && (Address >= ADDR_FLASH_SECTOR_5))
  {
    sector = FLASH_Sector_5;
  }
  else if((Address < ADDR_FLASH_SECTOR_7) && (Address >= ADDR_FLASH_SECTOR_6))
  {
    sector = FLASH_Sector_6;
  }
  else if((Address < ADDR_FLASH_SECTOR_8) && (Address >= ADDR_FLASH_SECTOR_7))
  {
    sector = FLASH_Sector_7;
  }
  else if((Address < ADDR_FLASH_SECTOR_9) && (Address >= ADDR_FLASH_SECTOR_8))
  {
    sector = FLASH_Sector_8;
  }
  else if((Address < ADDR_FLASH_SECTOR_10) && (Address >= ADDR_FLASH_SECTOR_9))
  {
    sector = FLASH_Sector_9;
  }
  else if((Address < ADDR_FLASH_SECTOR_11) && (Address >= ADDR_FLASH_SECTOR_10))
  {
    sector = FLASH_Sector_10;
  }
  else/*(Address < FLASH_END_ADDR) && (Address >= ADDR_FLASH_SECTOR_11))*/
  {
    sector = FLASH_Sector_11;
  }

  return sector;
}

rt_bool_t flash_read(u32 address, u8* buffer, u16 size)
{
    s32 i;

    if (!buffer || !size)
        return RT_FALSE;

    if (!IS_FLASH_ADDRESS(address) || !IS_FLASH_ADDRESS(address + size))
        return RT_FALSE;

    for (i=0; i<size; i++)
    {
        buffer[i] = *(u8*)(address + i);
    }

    return RT_TRUE;
}

rt_bool_t flash_save(u32 address, u8* buffer, u16 size)
{
    s32 i, j, num_word;
    u32 start_sector, end_sector;
    u32 *pdata, status = FLASH_COMPLETE;

    if (!buffer || !size)
        return RT_FALSE;

    if (!IS_FLASH_ADDRESS(address) || !IS_FLASH_ADDRESS(address + size))
        return RT_FALSE;

    FLASH_Unlock();

    start_sector = GetSector(address);
    end_sector = GetSector(address + size);

    for (i=start_sector; i<=end_sector; i++)
    {
        FLASH_EraseSector(i, VoltageRange_3);
    }

    num_word = (size + 3) / 4;
    pdata = (u32*)buffer;

    for (i=0; i<num_word; i++)
    {
        j = 0;
        do {
            status = FLASH_ProgramWord(address, *pdata);
            j++;
        } while (status != FLASH_COMPLETE && j < 5);

        if (status != FLASH_COMPLETE)
        {
            break;
        }

        address += 4;
        pdata++;
    }

    FLASH_Lock();

    return status != FLASH_COMPLETE ? RT_FALSE : RT_TRUE;
}

