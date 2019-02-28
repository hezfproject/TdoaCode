#ifndef _3G_THREAD_H_
#define _3G_THREAD_H_

#define THREAD_MSLEEP(millisecond)  \
    do {    \
        int sleep_tick = (millisecond) * RT_TICK_PER_SECOND / 1000;    \
        if (sleep_tick <= 0)    \
            sleep_tick = 1;    \
        rt_thread_delay(sleep_tick);   \
    } while (0)

#endif  // _3G_THREAD_H_
