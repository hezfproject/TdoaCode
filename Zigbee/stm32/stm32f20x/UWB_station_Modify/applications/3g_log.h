#ifndef _3G_LOG_H_
#define _3G_LOG_H_

#include "rtthread.h"

#include <time.h>

enum {
    LOG_INFO = 0,
    LOG_CRITICAL,
    LOG_WARNING,
    LOG_ERROR,
};

#define LOG_LEVEL LOG_INFO

#ifdef LOG_DEBUG
#define DEBUG_LOG(fmt, args...)    \
    do {    \
        rt_kprintf(fmt, ##args);    \
    } while (0)
#else
#define DEBUG_LOG(fmt, args...)
#endif

#define LOG(log_level, fmt, args...)    \
    do {    \
        if ((log_level) >= LOG_LEVEL) {  \
            rt_kprintf(fmt, ##args);    \
        }   \
    } while (0)

#define TIME_LOG(log_level, fmt, args...)    \
    do {    \
        if ((log_level) >= LOG_LEVEL) {     \
            rt_kprintf("%u ", time(RT_NULL));   \
            rt_kprintf(fmt, ##args);    \
        }   \
    } while (0)

#define ERROR_LOG(fmt, args...)    \
    do {    \
        rt_kprintf("func %s line %d err: ",__func__, __LINE__);      \
        rt_kprintf(fmt, ##args);    \
    } while (0)

void mem_printf(const char* p_title, const char* p_mem, int size);

#endif  // _3G_LOG_H_
