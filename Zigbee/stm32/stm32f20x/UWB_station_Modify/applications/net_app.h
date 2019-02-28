#ifndef _NET_APP_H_
#define _NET_APP_H_

#include <rtthread.h>

typedef struct
{
    char url[20];
    int port;
} server_info;


#define NET_POOL_MAX    (1024 * 16)

extern rt_uint8_t u8NetPool[NET_POOL_MAX];
extern struct rt_messagequeue net_mq;

extern unsigned char net_recv_fine_flag;
extern unsigned char net_send_fine_flag;

rt_bool_t start_net_work(void);
void net_thread_entry(void *parameter);
void app_display_ipaddr(rt_uint32_t IPaddress);
void net_report_restart_msg(void);
void net_report_running_state_msg(int state_mode, const char* p_msg, int msg_len);

#endif
