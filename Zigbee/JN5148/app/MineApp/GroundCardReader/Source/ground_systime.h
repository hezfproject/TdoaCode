#ifndef _SYSTIME_H_
#define _SYSTIME_H_

#include <jendefs.h>

#define _5ms    (5)
#define _10ms   (10)
#define _15ms   (15)
#define _50ms   (50)
#define _100ms  (100)
#define _500ms  (500)
#define _1S     (1000)
#define _5S     (5000)

#define TIMER_TICK_MAX  ((0xFFFFFFFF) >> 1)

PUBLIC void init_system_ms(void);
PUBLIC uint32 get_system_ms(void);

#endif

