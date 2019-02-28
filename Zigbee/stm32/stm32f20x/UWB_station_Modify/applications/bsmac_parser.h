#ifndef _BSMAC_PARSER_H_
#define _BSMAC_PARSER_H_

#include <rtthread.h>

/* msg queue to bsmac */
extern struct rt_messagequeue bsmac_mq;

void bsmac_thread_entry(void *parameter);
rt_bool_t bsmac_get_link_status(rt_uint8_t port);
rt_uint16_t bsmac_get_peer_addr(rt_uint8_t port);

rt_bool_t start_bsmac_work(void);
void stop_bsmac_work(void);
rt_err_t  bsmac_send_packet(rt_uint8_t* p, rt_uint16_t len, rt_uint8_t port);


#endif

