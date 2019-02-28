#ifndef WATCHDOG_UTIL_H
#define WATCHDOG_UTIL_H

#include "hal_types.h"
#include  "hal_defs.h"
#include "ioCC2430.h"

#define DOGTIMER_INTERVAL_1S          0
#define DOGTIMER_INTERVAL_250MS    1
#define DOGTIMER_INTERVAL_15MS      2
#define DOGTIMER_INTERVAL_2MS        3

#define SLEEP_RST_POS		3

#define WDCTL_INT_POS		0
#define WDCTL_MODE_POS	        2
#define WDCTL_EN_POS		3
#define WDCTL_CLR_POS		4

#define RESET_FLAG_POWERON	0
#define RESET_FLAG_EXTERNAL	1
#define RESET_FLAG_WATCHDOG	2

#define STARTWATCHDOG(interval)  st(WDCTL &= ~(0x01<<WDCTL_MODE_POS);WDCTL &= ~(0x03<<WDCTL_INT_POS);WDCTL |= (interval & 0x03)<<WDCTL_INT_POS; WDCTL |= (0x01<<WDCTL_EN_POS); )

#define FEEDWATCHDOG()			\
{		\
uint8 RS0,RS1;RS0 = RS1 = WDCTL;		\
RS0 &= ~(0x0F<<WDCTL_CLR_POS);RS0 |=   (0x0A<<WDCTL_CLR_POS);		\
RS1 &= ~(0x0F<<WDCTL_CLR_POS);RS1 |=   (0x05<<WDCTL_CLR_POS);		\
WDCTL = RS0;WDCTL = RS1;  \
}
       
extern bool IsWatchDogOn(void);
extern uint8 GetResetFlag(void);
extern void  StartWatchDog(uint8 interval);
//__near_func
extern void  FeedWatchDog(void);
extern void ChangeWatchDogInterval(uint8 interval);

#endif
