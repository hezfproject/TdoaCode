#include <types.h>
#include <bsp_flash.h>

//设备卡偏移的页中，放着基本信息，卡状态等. Dev_card.h
#ifndef WORK_STATE_PG
#define WORK_STATE_PG               (FLASH_START_PG)
#endif

#ifndef WORK_TYPE_PG
#define WORK_TYPE_PG               (FLASH_START_PG + 1)
#endif

#ifndef BASE_INFO_PG
#define BASE_INFO_PG                (FLASH_START_PG + 2)
#define BASE_INFO_LEN               (1024)  //该页只能存一帧基本信息,最长89字节
#endif

//#ifdef DEV_CARD_PROJ
#define FLASH_PAGE_OFFSET   3
//#else
//#define FLASH_PAGE_OFFSET   0
//#endif

#define FLASH_CARD_INFO_HANDLE_PAGE     (FLASH_START_PG+FLASH_PAGE_OFFSET)
#define FLASH_CARD_INFO_HANDLE_OSET     (0)

//使用区和缓冲区各两页，互相转换，避免写入时覆盖原有内容。
#define FLASH_CARD_INFO_DETAIL_PAGE_1     (FLASH_START_PG+FLASH_PAGE_OFFSET+1)
#define FLASH_CARD_INFO_DETAIL_OSET_1     (0)//

#define FLASH_CARD_INFO_DETAIL_PAGE_2     (FLASH_START_PG+FLASH_PAGE_OFFSET+2)
#define FLASH_CARD_INFO_DETAIL_OSET_2     (0)//

bool app_FLASH_Erase(uint8 pg);

/*
** 请使用ram区域的数据进行写入
** 注意使用之前做好先读取这段区域是否被擦除过，其次需要判断电量是否稳定
** 请参考其他工程的OSAL_NV_CHECK_BUS_VOLTAGE
*/
bool app_FLASH_Write(uint8 pg, uint16 offset, uint8 *buf, uint16 cnt);

bool app_FLASH_Read(uint8 pg, uint16 offset, uint8 *buf, uint16 cnt);
