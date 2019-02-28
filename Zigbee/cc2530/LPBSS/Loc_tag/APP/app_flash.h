#include <types.h>
#include <bsp_flash.h>

//�豸��ƫ�Ƶ�ҳ�У����Ż�����Ϣ����״̬��. Dev_card.h
#ifndef WORK_STATE_PG
#define WORK_STATE_PG               (FLASH_START_PG)
#endif

#ifndef WORK_TYPE_PG
#define WORK_TYPE_PG               (FLASH_START_PG + 1)
#endif

#ifndef BASE_INFO_PG
#define BASE_INFO_PG                (FLASH_START_PG + 2)
#define BASE_INFO_LEN               (1024)  //��ҳֻ�ܴ�һ֡������Ϣ,�89�ֽ�
#endif

//#ifdef DEV_CARD_PROJ
#define FLASH_PAGE_OFFSET   3
//#else
//#define FLASH_PAGE_OFFSET   0
//#endif

#define FLASH_CARD_INFO_HANDLE_PAGE     (FLASH_START_PG+FLASH_PAGE_OFFSET)
#define FLASH_CARD_INFO_HANDLE_OSET     (0)

//ʹ�����ͻ���������ҳ������ת��������д��ʱ����ԭ�����ݡ�
#define FLASH_CARD_INFO_DETAIL_PAGE_1     (FLASH_START_PG+FLASH_PAGE_OFFSET+1)
#define FLASH_CARD_INFO_DETAIL_OSET_1     (0)//

#define FLASH_CARD_INFO_DETAIL_PAGE_2     (FLASH_START_PG+FLASH_PAGE_OFFSET+2)
#define FLASH_CARD_INFO_DETAIL_OSET_2     (0)//

bool app_FLASH_Erase(uint8 pg);

/*
** ��ʹ��ram��������ݽ���д��
** ע��ʹ��֮ǰ�����ȶ�ȡ��������Ƿ񱻲������������Ҫ�жϵ����Ƿ��ȶ�
** ��ο��������̵�OSAL_NV_CHECK_BUS_VOLTAGE
*/
bool app_FLASH_Write(uint8 pg, uint16 offset, uint8 *buf, uint16 cnt);

bool app_FLASH_Read(uint8 pg, uint16 offset, uint8 *buf, uint16 cnt);
