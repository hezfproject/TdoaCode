/*
 * unistd.c
 *
 *  Created on: Apr 14, 2011
 *      Author: Ekawahyu Susilo
 */

#include "compiler.h"
#include "usleep.h"
#include "port.h"

clock_t rt_hw_tick_get_millisecond(void);
clock_t rt_hw_tick_get_microsecond(void);


#define _clock(x) rt_hw_tick_get_millisecond(x)//clock(x)
#define _MICRO_CLOCK   rt_hw_tick_get_microsecond()
#define _MILL_CLOCK 	rt_hw_tick_get_millisecond()
unsigned __weak sleep(unsigned seconds)
{
	clock_t t0 = _MILL_CLOCK;//_clock();
	clock_t dt = seconds * CLOCKS_PER_SEC;

	while (_MILL_CLOCK - t0  < dt);
	return 0;
}

int __weak usleep(useconds_t useconds)
{
	clock_t t0 = _MICRO_CLOCK;//_clock();
	//clock_t dt = useconds / (1000000/CLOCKS_PER_SEC);
	clock_t dt = useconds;

	while (_MICRO_CLOCK - t0  < dt);
	return 0;
}

/**
 * This fucntion returns milliseconds since system passed
 */
clock_t rt_hw_tick_get_millisecond(void)
{
	clock_t tick;
	clock_t value;

#define TICK_MS (1000/CLOCKS_PER_SEC)

	tick = portGetTickCnt();
	value = tick * TICK_MS + (SysTick->LOAD - SysTick->VAL) * TICK_MS / SysTick->LOAD;

	return value;
}

/**
 * This fucntion returns microseconds since system passed
 */
clock_t rt_hw_tick_get_microsecond(void)
{
	clock_t tick;
	clock_t value;

#define TICK_US	(1000000/CLOCKS_PER_SEC)

	tick = portGetTickCnt();
	value = tick * TICK_US + (SysTick->LOAD - SysTick->VAL) * TICK_US / SysTick->LOAD;

	return value;
}



