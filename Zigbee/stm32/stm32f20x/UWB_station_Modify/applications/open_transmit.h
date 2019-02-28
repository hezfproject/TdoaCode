#ifndef _OPEN_TRANSMIT_H_
#define _OPEN_TRANSMIT_H_

#include <rtthread.h>

//端口号定义
typedef enum
{
    COM1_TRANSMIT_ID = 0,
    COM2_TRANSMIT_ID = 1,
    COM3_TRANSMIT_ID = 2,

    ETH0_TRANSMIT_ID = 3,
    TRANMIT_MAX_ID
}TRANSMIT_ID_EM;

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
                                rt_int32_t (*pfn)(void* message));

/**
 * 撤销一个端口的数据发送接口
 *
 *fromID    输入参数    发送数据的端口号
 *toID      输入参数    接收数据的端口号
 *
 *
 *返回值rt_bool_t 成功为RT_TRUE，失败为RT_FALSE
 */
rt_bool_t transmit_deregister(TRANSMIT_ID_EM fromID, TRANSMIT_ID_EM toID);

/**
 * 这是一个公用的数据传送接口，可以从任何一个以定义的端口发送到另一个端口 
 *
 *fromID    输入参数    发送数据的端口号
 *toID      输入参数    接收数据的端口号
 *message   输入参数    发送的数据包句柄
 *
 *返回值rt_int32_t 表示发送的字节数，-1表示发送失败
 */
rt_int32_t transmit_sendto(TRANSMIT_ID_EM fromID, TRANSMIT_ID_EM toID, void *message);

#endif
