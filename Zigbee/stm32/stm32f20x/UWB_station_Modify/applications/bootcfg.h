#ifndef _BOOTCFG_H_
#define _BOOTCFG_H_

#include <rtthread.h>

typedef struct cfg_option_t
{
    rt_uint32_t u32BsId;
    rt_uint32_t u32BsIp;
    rt_uint32_t u32BsMK;
    rt_uint32_t u32BsGW;
    rt_uint32_t u32SvIp;
    rt_uint32_t u32SvPort;
    rt_uint32_t u32Delay;
    rt_uint32_t u32IpMode;  // 0:dhcp£¬1:static
}CFG_OPTION_T;

#define IPMODE_DHCP   0

extern CFG_OPTION_T sys_option;
extern struct rt_semaphore sys_option_sem;

extern void bootsh_init(void);
extern void bootsh_set_device(const char * device_name);
extern void bootsh_thread_entry(void * parameter);
extern rt_bool_t start_boot_work(void);

extern rt_bool_t get_sys_cfg(CFG_OPTION_T* cfg_buf);
extern rt_bool_t set_sys_cfg(const CFG_OPTION_T* cfg_buf);

#endif
