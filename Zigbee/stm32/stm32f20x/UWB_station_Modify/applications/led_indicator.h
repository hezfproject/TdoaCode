#ifndef _LED_INDICATOR_
#define _LED_INDICATOR_

#include "rtthread.h"

void init_sys_status_indicator_lamps(void);
void light_up_sys_running_indicator(void);
void light_off_sys_running_indicator(void);
void light_up_modbus_data_indicator(void);
void light_off_modbus_data_indicator(void);
void hook_system_running_state(rt_thread_t from, rt_thread_t to);
void startup_sys_running_state_lamp_timer(void);
void light_up_rssi_modules_data_indicator(void);
void light_off_rssi_modules_data_indicator(void);


#endif  // _LED_INDICATOR_
