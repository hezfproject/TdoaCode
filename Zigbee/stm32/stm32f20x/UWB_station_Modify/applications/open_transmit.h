#ifndef _OPEN_TRANSMIT_H_
#define _OPEN_TRANSMIT_H_

#include <rtthread.h>

//�˿ںŶ���
typedef enum
{
    COM1_TRANSMIT_ID = 0,
    COM2_TRANSMIT_ID = 1,
    COM3_TRANSMIT_ID = 2,

    ETH0_TRANSMIT_ID = 3,
    TRANMIT_MAX_ID
}TRANSMIT_ID_EM;

/**
 * ע��һ���˿ڵ����ݷ��ͽӿ�
 *
 *fromID    �������    �������ݵĶ˿ں�
 *toID      �������    �������ݵĶ˿ں�
 *pfn       �������    ���ݷ��ͽӿھ��
 *
 *����ֵrt_bool_t �ɹ�ΪRT_TRUE��ʧ��ΪRT_FALSE
 */
rt_bool_t transmit_register(TRANSMIT_ID_EM fromID, TRANSMIT_ID_EM toID,
                                rt_int32_t (*pfn)(void* message));

/**
 * ����һ���˿ڵ����ݷ��ͽӿ�
 *
 *fromID    �������    �������ݵĶ˿ں�
 *toID      �������    �������ݵĶ˿ں�
 *
 *
 *����ֵrt_bool_t �ɹ�ΪRT_TRUE��ʧ��ΪRT_FALSE
 */
rt_bool_t transmit_deregister(TRANSMIT_ID_EM fromID, TRANSMIT_ID_EM toID);

/**
 * ����һ�����õ����ݴ��ͽӿڣ����Դ��κ�һ���Զ���Ķ˿ڷ��͵���һ���˿� 
 *
 *fromID    �������    �������ݵĶ˿ں�
 *toID      �������    �������ݵĶ˿ں�
 *message   �������    ���͵����ݰ����
 *
 *����ֵrt_int32_t ��ʾ���͵��ֽ�����-1��ʾ����ʧ��
 */
rt_int32_t transmit_sendto(TRANSMIT_ID_EM fromID, TRANSMIT_ID_EM toID, void *message);

#endif
