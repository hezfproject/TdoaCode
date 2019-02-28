#include "open_transmit.h"

//端口到端口的句柄映射
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
 * 注册一个端口的数据发送接口
 *
 *fromID    输入参数    发送数据的端口号
 *toID      输入参数    接收数据的端口号
 *pfn       输入参数    数据发送接口句柄
 *
 *返回值rt_bool_t 成功为RT_TRUE，失败为RT_FALSE
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
 * 撤销一个端口的数据发送接口
 *
 *fromID    输入参数    发送数据的端口号
 *toID      输入参数    接收数据的端口号
 *
 *
 *返回值rt_bool_t 成功为RT_TRUE，失败为RT_FALSE
 */
rt_bool_t transmit_deregister(TRANSMIT_ID_EM fromID, TRANSMIT_ID_EM toID)
{
    if (fromID >= TRANMIT_MAX_ID || toID >= TRANMIT_MAX_ID)
        return RT_FALSE;

    transmit_handle[fromID][toID] = RT_NULL;

    return RT_TRUE;
}

/**
 * 这是一个公用的数据传送接口，可以从任何一个以定义的端口发送到另一个端口
 *
 *fromID    输入参数    发送数据的端口号
 *toID      输入参数    接收数据的端口号
 *message   输入参数    发送的数据包句柄
 *
 *返回值rt_int32_t 表示发送的字节数，-1表示发送失败
 */
rt_int32_t transmit_sendto(TRANSMIT_ID_EM fromID, TRANSMIT_ID_EM toID, void *message)
{
    if (fromID >= TRANMIT_MAX_ID || toID >= TRANMIT_MAX_ID
        || !transmit_handle[fromID][toID])
        return -1;

    return transmit_handle[fromID][toID](message);
}

