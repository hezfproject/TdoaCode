#ifndef _SEMAPHORE_H_
#define _SEMAPHORE_H_

#include "hal_mcu.h"

inline static uint32 ENTER_CRITICAL_SECTION(void)
{
    uint32 reg;

    reg = EA;

    EA = 0;

    return reg;
}

inline static void  EXIT_CRITICAL_SECTION(uint32 reg)
{
    EA = reg;
}

#define _sem_take_retv(sem, re)             \
    do{                                     \
        uint32 reg;                         \
                                            \
        reg = ENTER_CRITICAL_SECTION();     \
                                            \
        if (!sem)                           \
        {                                   \
            EXIT_CRITICAL_SECTION(reg);     \
            return re;                      \
        }                                   \
                                            \
        sem = 0;                            \
        EXIT_CRITICAL_SECTION(reg);         \
    }while(0)

#define _sem_take_ret(sem)                  \
    do{                                     \
        uint32 reg;                         \
                                            \
        reg = ENTER_CRITICAL_SECTION();     \
                                            \
        if (!sem)                           \
        {                                   \
            EXIT_CRITICAL_SECTION(reg);     \
            return;                         \
        }                                   \
                                            \
        sem = 0;                            \
        EXIT_CRITICAL_SECTION(reg);         \
    }while(0)

#define _sem_release(sem)                   \
    do{                                     \
        uint32 reg;                         \
                                            \
        reg = ENTER_CRITICAL_SECTION();     \
        sem = 1;                            \
        EXIT_CRITICAL_SECTION(reg);         \
    }while(0)

#endif

