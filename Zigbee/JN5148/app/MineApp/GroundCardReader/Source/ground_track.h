#ifndef _GROUND_TRACK_H_
#define _GROUND_TRACK_H_

#include "JN5148_util.h"

#ifdef DEBUG_ERROR
#define DBG_ERR(fmt, ...)                           \
    PrintfUtil_vPrintf("%s:%d\t"fmt, __FUNCTION__, __LINE__, ##__VA_ARGS__)
#else
#define DBG_ERR(fmt, ...)
#endif

#ifdef DEBUG_WARN
#define DBG_WARN(fmt, ...)                          \
    PrintfUtil_vPrintf("%s:%d\t"fmt, __FUNCTION__, __LINE__, ##__VA_ARGS__)
#else
#define DBG_WARN(fmt, ...)
#endif

#ifdef DEBUG_LOG
#define DBG_LOG(fmt, ...)                           \
    PrintfUtil_vPrintf("%s:%d\t"fmt, __FUNCTION__, __LINE__, ##__VA_ARGS__)
#else
#define DBG_LOG(fmt, ...)
#endif

#define DBG_JUDGE(c, fmt, ...)                      \
    do {                                            \
        if (c)                                      \
        {                                           \
            DBG_LOG(fmt, ##__VA_ARGS__);            \
        }                                           \
    } while(0)

#define DBG_JUDGE_RET(c, fmt, ...)                  \
    do {                                            \
        if (c)                                      \
        {                                           \
            DBG_WARN(fmt, ##__VA_ARGS__);           \
            return;                                 \
        }                                           \
    } while(0)

#define DBG_JUDGE_RETV(c, v, fmt, ...)              \
    do {                                            \
        if (c)                                      \
        {                                           \
            DBG_WARN(fmt, ##__VA_ARGS__);           \
            return (v);                             \
        }                                           \
    } while(0)

/*
*   Assert
*/
#define ASSERT(c)                                   \
    do{                                             \
        if (!(c))                                   \
        {                                           \
            PrintfUtil_vPrintf("ASSERT:%s:%d\n",    \
                __FUNCTION__, __LINE__);            \
            LedUtil_vFlashAll(1000, 0xFFFF);        \
        }                                           \
    } while(0)

#define ASSERT_RETV(c, v)                           \
    do{                                             \
        if (!(c))                                   \
        {                                           \
            PrintfUtil_vPrintf("ASSERT:%s:%d\n",    \
                __FUNCTION__, __LINE__);            \
            return (v);                             \
        }                                           \
    } while(0)

#define ASSERT_RET(c)                               \
    do{                                             \
        if (!(c))                                   \
        {                                           \
            PrintfUtil_vPrintf("ASSERT:%s:%d\n",    \
                __FUNCTION__, __LINE__);            \
            return;                                 \
        }                                           \
    } while(0)

#endif

