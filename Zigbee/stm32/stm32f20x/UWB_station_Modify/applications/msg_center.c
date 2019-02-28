#include "msg_center.h"
#include "3g_log.h"

// 消息管理的消息队列
struct rt_messagequeue msg_analyser_mq;

rt_uint8_t u8MsgPool[MSG_POOL_MAX];

// 用于在消息队列中获取一个消息
static rt_uint8_t msg_buf[MSG_ANALYSER_PKT_SIZE];

static void (*msg_analyser_handle[TRANMIT_MAX_ID])(void *) =
{
    RT_NULL,
    RT_NULL,
    RT_NULL,
    RT_NULL
};

// 注册一个ID的数据分析器句柄
rt_bool_t msg_analyser_register(TRANSMIT_ID_EM emId, void (*pfn)(void *))
{
    if (emId >= TRANMIT_MAX_ID || !pfn || msg_analyser_handle[emId])
        return RT_FALSE;

    msg_analyser_handle[emId] = pfn;

    return RT_TRUE;
}

// 取消一个ID的数据分析器句柄
rt_bool_t msg_analyser_deregister(TRANSMIT_ID_EM emId)
{
    if (emId >= TRANMIT_MAX_ID)
        return RT_FALSE;

    msg_analyser_handle[emId] = RT_NULL;

    return RT_TRUE;
}

// 消息分析器线程入口
void msg_analyser_entry(void *parameter)
{
    while (1)
    {
        if (rt_mq_recv(&msg_analyser_mq, msg_buf, sizeof(msg_buf), RT_WAITING_FOREVER)
                == RT_EOK)
        {
            const rt_uint32_t *c_pu32Type = (rt_uint32_t *)msg_buf;

            if (*c_pu32Type < TRANMIT_MAX_ID)
            {
                void (*msg_handle)(void *) = RT_NULL;

                msg_handle = msg_analyser_handle[*c_pu32Type];

                if (msg_handle != RT_NULL)
                    msg_handle(msg_buf);
            }
        }
    }
}

static rt_thread_t msg_analyser_thread = RT_NULL;

rt_bool_t start_msg_analyser_work()
{
    msg_analyser_thread = rt_thread_create("msg analyser",
        msg_analyser_entry, RT_NULL, 1024, 9, 10);

    if (msg_analyser_thread == RT_NULL)
    {
        ERROR_LOG("create msg analyser work thread failed\n");
        return RT_FALSE;
    }

    rt_thread_startup(msg_analyser_thread);

    return RT_TRUE;
}
