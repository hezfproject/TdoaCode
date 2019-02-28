#ifndef WATCHDOG_UTIL_H
#define WATCHDOG_UTIL_H

#include "hal_types.h"
#include  "hal_defs.h"
#include "ioCC2530.h"
#include "hal_mcu.h"

#define DOGTIMER_INTERVAL_1S          0
#define DOGTIMER_INTERVAL_250MS    1
#define DOGTIMER_INTERVAL_15MS      2
#define DOGTIMER_INTERVAL_2MS        3

#define SLEEP_RST_POS		3

#define WDCTL_INT_POS		0
#define WDCTL_MODE_POS	    2
#define WDCTL_EN_POS		3
#define WDCTL_CLR_POS		4

#define RESET_FLAG_POWERON	0
#define RESET_FLAG_EXTERNAL	1
#define RESET_FLAG_WATCHDOG	2

#define STARTWATCHDOG(interval)  st(WDCTL &= ~(0x01<<WDCTL_MODE_POS);WDCTL &= ~(0x03<<WDCTL_INT_POS);WDCTL |= (interval & 0x03)<<WDCTL_INT_POS; WDCTL |= (0x01<<WDCTL_EN_POS); )

#define FeedWatchDog() WD_KICK()

       
extern bool IsWatchDogOn(void);
extern void  StartWatchDog(uint8 interval);
extern void ChangeWatchDogInterval(uint8 interval);

#endif
