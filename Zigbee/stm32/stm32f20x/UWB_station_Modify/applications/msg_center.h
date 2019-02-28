#ifndef _MSG_CENTER_H_
#define _MSG_CENTER_H_

#include "ptl_nwk.h"
#include <rtthread.h>

//端口号定义
typedef enum
{
    COM1_TRANSMIT_ID = 0,
    COM2_TRANSMIT_ID = 1,
    COM3_TRANSMIT_ID = 2,
    COM4_TRANSMIT_ID = 3,

    ETH0_TRANSMIT_ID = 4,
    TRANMIT_MAX_ID
} TRANSMIT_ID_EM;

typedef struct
{
    rt_uint32_t  u32MsgId;
}MSG_CENTER_HEADER_T;

#define MSG_POOL_MAX    10240
#define MSG_HEADER_SIZE   sizeof(MSG_CENTER_HEADER_T)
#define MSG_PLD_MAX     NET2STM32_PACKET_SIZE
#define MSG_ANALYSER_PKT_SIZE   1024
#define MSG_NET_PKT_SIZE    MSG_ANALYSER_PKT_SIZE
#define MSG_COM_PKT_SIZE    (MSG_PLD_MAX)
#define MSG_MBUS_PKT_SIZE   MSG_ANALYSER_PKT_SIZE

extern rt_uint8_t u8MsgPool[MSG_POOL_MAX];
extern struct rt_messagequeue msg_analyser_mq;

// 注册一个ID的数据分析器句柄
rt_bool_t msg_analyser_register(TRANSMIT_ID_EM emId, void (*pfn)(void *));

// 取消一个ID的数据分析器句柄
rt_bool_t msg_analyser_deregister(TRANSMIT_ID_EM emId);

// 消息分析器线程入口
void msg_analyser_entry(void *parameter);
rt_bool_t start_msg_analyser_work(void);

#endif

