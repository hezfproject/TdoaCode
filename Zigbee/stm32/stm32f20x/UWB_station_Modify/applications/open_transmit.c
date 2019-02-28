#include "open_transmit.h"

//�˿ڵ��˿ڵľ��ӳ��
rt_int32_t (*transmit_handle[TRANMIT_MAX_ID][TRANMIT_MAX_ID])(void *message) =
{
    // COM1 TO COM1        COM1 TO COM2       COM1 TO COM3       COM1 TO ETH0
    {RT_NULL,           RT_NULL,            RT_NULL,            RT_NULL},

    // COM2 TO COM1        COM2 TO COM2       COM2 TO COM3       COM2 TO ETH0
    {RT_NULL,           RT_NULL,            RT_NULL,            RT_NULL},

    // COM3 TO COM1        COM3 TO COM2       COM3 TO COM3       COM3 TO ETH0
    {RT_NULL,           RT_NULL,            RT_NULL,            RT_NULL},

    // ETH0 TO COM1        ETH0 TO COM2       ETH0 TO COM3       ETH0 TO ETH0
    {RT_NULL,           RT_NULL,            RT_NULL,            RT_NULL},
};

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
                                rt_int32_t (*pfn)(void* message))
{
    if (fromID >= TRANMIT_MAX_ID || toID >= TRANMIT_MAX_ID || !pfn)
        return RT_FALSE;

    transmit_handle[fromID][toID] = pfn;

    return RT_TRUE;
}

/**
 * ����һ���˿ڵ����ݷ��ͽӿ�
 *
 *fromID    �������    �������ݵĶ˿ں�
 *toID      �������    �������ݵĶ˿ں�
 *
 *
 *����ֵrt_bool_t �ɹ�ΪRT_TRUE��ʧ��ΪRT_FALSE
 */
rt_bool_t transmit_deregister(TRANSMIT_ID_EM fromID, TRANSMIT_ID_EM toID)
{
    if (fromID >= TRANMIT_MAX_ID || toID >= TRANMIT_MAX_ID)
        return RT_FALSE;

    transmit_handle[fromID][toID] = RT_NULL;

    return RT_TRUE;
}

/**
 * ����һ�����õ����ݴ��ͽӿڣ����Դ��κ�һ���Զ���Ķ˿ڷ��͵���һ���˿�
 *
 *fromID    �������    �������ݵĶ˿ں�
 *toID      �������    �������ݵĶ˿ں�
 *message   �������    ���͵����ݰ����
 *
 *����ֵrt_int32_t ��ʾ���͵��ֽ�����-1��ʾ����ʧ��
 */
rt_int32_t transmit_sendto(TRANSMIT_ID_EM fromID, TRANSMIT_ID_EM toID, void *message)
{
    if (fromID >= TRANMIT_MAX_ID || toID >= TRANMIT_MAX_ID
        || !transmit_handle[fromID][toID])
        return -1;

    return transmit_handle[fromID][toID](message);
}

