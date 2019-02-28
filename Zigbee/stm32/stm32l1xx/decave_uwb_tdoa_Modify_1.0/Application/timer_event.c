#include "stm32l1xx.h"
#include "mcu.h"
#include "timer_event.h"
#include <string.h>

#ifndef EVENT_TIMER_MAX
#error "timer_event.h EVENT_TIMER_MAX Is undefined"
#elif EVENT_TIMER_MAX > 32
#error "timer_event.h EVENT_TIMER_MAX Should be less than 32"
#endif

#define true    (True)
#define false   (False)

#define MAX_TIMER_TICK  (0xFFFFFFFF/2)

#define ENTER_CRITICAL_SECTION()  interrupt_disable()

#define EXIT_CRITICAL_SECTION(reg)  interrupt_enable(reg)

#define MIN(a, b)   ((a) < (b) ? (a) : (b))

#define PARAM_CHECK_RET(param, ret)     \
    do{                                 \
        if (param)                      \
            return (ret);               \
    }while(0)

typedef struct
{
    uint_32 tick;
    uint_32 remain_tick;
    uint_32 reload_tick;
} event_timer;

static uint_32   reload_events;
static uint_32   bits_events;
static uint_32   min_timer;

static uint_8    timer_cnt;

static event_timer timer_list[EVENT_TIMER_MAX];

static __INLINE uint_8 _event_timer_idx(uint_32 event)
{
    uint_8 i;

    for (i=0; i<EVENT_TIMER_MAX; i++)
    {
        if ((1 << i) & event)
            break;
    }

    return i;
}

/*
* 查找一个事件是否被发生
*/
Bool event_timer_take(uint_32 event)
{
    Bool ret;
    uint_32 reg;

    if (!event || (event & ((~event) + 1)) != event)
    {
        return false;
    }

    reg = ENTER_CRITICAL_SECTION();

    if (event & bits_events)
    {
        bits_events ^= event;
        ret = true;
    }
    else
    {
        ret = false;
    }

    EXIT_CRITICAL_SECTION(reg);

    return ret;
}

/*
* 查找所有事件是否被发生
*/
uint_32 event_timer_take_all(void)
{
    uint_32 ret;
    uint_32 reg;

    reg = ENTER_CRITICAL_SECTION();
    ret = bits_events;
    bits_events = 0;
    EXIT_CRITICAL_SECTION(reg);

    return ret;
}

/*
* 定时器事件模块的初始化
*/
void event_timer_init(void)
{
    memset(timer_list, 0, sizeof(timer_list));
    timer_cnt = bits_events = reload_events = 0;
}

Bool event_timer_set(uint_32 event)
{
    uint_32 reg;

    if (!event || (event & ((~event) + 1)) != event)
    {
        return false;
    }

    reg = ENTER_CRITICAL_SECTION();

    bits_events |= event;

    EXIT_CRITICAL_SECTION(reg);

    return true;
}

Bool event_timer_unset(uint_32 event)
{
    uint_32 reg;

    if (!event || (event & ((~event) + 1)) != event)
    {
        return false;
    }

    reg = ENTER_CRITICAL_SECTION();

    bits_events &= ~event;

    EXIT_CRITICAL_SECTION(reg);

    return true;
}

/*
* 添加一个正在运行的定时器, tick < MAX_TIMER_TICK
*/
Bool event_timer_add(uint_32 event, uint_32 tick)
{
    uint_8 idx;
    uint_32 reg;

    if (!tick)
    {
        return event_timer_set(event);
    }

    if ((!event || (event & ((~event) + 1)) != event)
        || MAX_TIMER_TICK <= tick)
    {
        return false;
    }

    idx = _event_timer_idx(event);

    if (idx >= EVENT_TIMER_MAX)
        return false;

    reg = ENTER_CRITICAL_SECTION();

    if (!timer_list[idx].remain_tick)
        timer_cnt++;

    timer_list[idx].tick = 0;
    timer_list[idx].remain_tick = tick;

    EXIT_CRITICAL_SECTION(reg);

    return true;
}

/*
* 添加一个正在运行的定时器,自动重新加载, 0 < tick < MAX_TIMER_TICK
*/
Bool event_timer_add_reload(uint_32 event, uint_32 tick)
{
    uint_32 reg;
    uint_8 idx;

    if ((!event || (event & ((~event) + 1)) != event)
        || !tick || MAX_TIMER_TICK <= tick)
    {
        return false;
    }

    idx = _event_timer_idx(event);

    if (idx >= EVENT_TIMER_MAX)
        return false;

    reg = ENTER_CRITICAL_SECTION();

    if (!timer_list[idx].remain_tick)
    {
        timer_cnt++;
        timer_list[idx].tick = 0;
        timer_list[idx].remain_tick = tick;
    }

    timer_list[idx].reload_tick = tick;
    reload_events |= event;

    EXIT_CRITICAL_SECTION(reg);

    return true;
}

/*
* 取消一个正在运行的定时器
*/
Bool event_timer_del(uint_32 event)
{
    uint_8 idx;
    uint_32 reg;

    if (!event || (event & ((~event) + 1)) != event)
    {
        return false;
    }

    idx = _event_timer_idx(event);

    if (idx >= EVENT_TIMER_MAX)
        return false;

    reg = ENTER_CRITICAL_SECTION();

    if (timer_list[idx].remain_tick)
        timer_cnt--;
    timer_list[idx].tick = 0;
    timer_list[idx].remain_tick = 0;
    reload_events &= ~event;
    bits_events &= ~event;

    EXIT_CRITICAL_SECTION(reg);

    return true;
}

/*
* 定时器模块的主循环
*/
void event_timer_update(void)
{
    uint_8 i;
    uint_32 reg;
    uint_32 currTick, elapseTick;

    min_timer = 0xFFFFFFFF;

    if (bits_events != 0)
        min_timer = 0;

    currTick = GetSysClock();

    for (i=0; i<EVENT_TIMER_MAX; i++)
    {
        event_timer *pos = timer_list + i;

        reg = ENTER_CRITICAL_SECTION();

        if (!pos->remain_tick)
        {
            EXIT_CRITICAL_SECTION(reg);
            continue;
        }

        if (!pos->tick)
        {
            pos->tick = pos->remain_tick + currTick;
        }

        elapseTick = currTick - pos->tick;

        if (elapseTick < MAX_TIMER_TICK)
        {
            min_timer = 0;
            bits_events |= (1 << i);

            if (reload_events & (1 << i))
            {
                pos->tick = pos->reload_tick + currTick;
            }
            else
            {
                pos->tick = 0;
                pos->remain_tick = 0;
                timer_cnt = timer_cnt > 0 ? timer_cnt - 1 : 0;
            }
        }
        else if (min_timer > 0)
        {
            min_timer = MIN(min_timer, pos->tick - currTick);
        }

        EXIT_CRITICAL_SECTION(reg);
    }
}

uint_32 event_timer_next_tick(void)
{
    event_timer_update();

    return min_timer;
}

