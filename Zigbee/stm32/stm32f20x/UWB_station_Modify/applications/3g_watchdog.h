#ifndef _3G_WATCHDOG_
#define _3G_WATCHDOG_

void startup_watchdog(void);
void feed_watchdog(void);
void startup_hw_watchdog_feeding_timer(void);
void startup_iwdg_feeding_timer(void);
extern int iwdg_net_feed_flag;

#endif  // _3G_WATCHDOG_
