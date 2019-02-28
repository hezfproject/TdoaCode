#include "bsp.h"
#include "timer_event.h"
#include "semaphore.h"
#include "string.h"

#define GetSysClock BSP_GetSysTick

#ifndef EVENT_TIMER_MAX
#error "timer_event.h EVENT_TIMER_MAX Is undefined"
#elif EVENT_TIMER_MAX > 32
#error "timer_event.h EVENT_TIMER_MAX Should be less than 32"
#endif

#define MAX_TIMER_TICK  (0xffffffff >> 1)

typedef struct
{
    UINT32 tick;
    UINT32 remain_tick;
    UINT32 reload_tick;
} event_timer;

static UINT32   reload_events;
static UINT32   bits_events;
static UINT32   min_timer;

static UINT8    timer_cnt;

static event_timer timer_list[EVENT_TIMER_MAX];

static inline UINT8 _event_timer_idx(UINT32 event)
{
    UINT8 i;

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
BOOL event_timer_take(UINT32 event)
{
    BOOL ret;
    UINT32 reg;

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
UINT32 event_timer_take_all(void)
{
    UINT32 ret;
    UINT32 reg;

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

BOOL event_timer_set(UINT32 event)
{
    UINT32 reg;

    if (!event || (event & ((~event) + 1)) != event)
    {
        return false;
    }

    reg = ENTER_CRITICAL_SECTION();

    bits_events |= event;

    EXIT_CRITICAL_SECTION(reg);

    return true;
}

BOOL event_timer_unset(UINT32 event)
{
    UINT32 reg;

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
BOOL event_timer_add(UINT32 event, UINT32 tick)
{
    UINT8 idx;
    UINT32 reg;

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
BOOL event_timer_add_reload(UINT32 event, UINT32 tick)
{
    UINT32 reg;
    UINT8 idx;

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
BOOL event_timer_del(UINT32 event)
{
    UINT8 idx;
    UINT32 reg;

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

    EXIT_CRITICAL_SECTION(reg);

    return true;
}

/*
* 定时器模块的主循环
*/
void event_timer_update(void)
{
    UINT8 i;
    UINT32 reg;
    UINT32 currTick, elapseTick;

    min_timer = 0xFFFFFFFF;

    if (bits_events != 0)
        min_timer = 0;

    currTick = GetSysClock();

    for (i=0; i<EVENT_TIMER_MAX; i++)
    {
        event_timer *pos = timer_list + i;

        if (!pos->remain_tick)
            continue;

        if (!pos->tick)
        {
            pos->tick = pos->remain_tick + currTick;
        }

        elapseTick = currTick - pos->tick;

        if (elapseTick < MAX_TIMER_TICK)
        {
            reg = ENTER_CRITICAL_SECTION();

            min_timer = 0;
            bits_events |= (1 << i);

            if (reload_events & (1 << i))
            {
                pos->tick = pos->reload_tick + currTick;
                EXIT_CRITICAL_SECTION(reg);
            }
            else
            {
                pos->tick = 0;
                pos->remain_tick = 0;
                timer_cnt = timer_cnt > 0 ? timer_cnt - 1 : 0;
                EXIT_CRITICAL_SECTION(reg);
            }
        }
        else if (min_timer > 0)
        {
            reg = ENTER_CRITICAL_SECTION();
            min_timer = MIN(min_timer, pos->tick - currTick);
            EXIT_CRITICAL_SECTION(reg);
        }
    }
}

UINT32 event_timer_next_tick(void)
{
    event_timer_update();

    return min_timer;
}

