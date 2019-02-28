#ifndef _MBUS_APP_H_
#define _MBUS_APP_H_

#include "rtdef.h"

#define GET_STATION_PAN_ID() (sys_option.u32BsId + 20000)

#define MBUS_NET_MSG_BUF_SIZE    (1024)
extern struct rt_messagequeue mbus_reported_mq;
extern struct rt_messagequeue mbus_rec_mq;

void init_mbus_msg_queue(void);
void mbus_thread_entry(void *param);
rt_bool_t start_modbus_work(void);
void stop_modbus_work(void);

#endif
