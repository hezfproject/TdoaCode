/******************************************************************************
 Copyright 2012
******************************************************************************/
#include "stm32l1xx.h"
#include "mem.h"
#include "debug.h"
#include "board.h"
//#include "timer_event.h"
#include "instance.h"

#include "compiler.h"
#include "port.h"
#include "printf_util.h"
#include "config.h"

#ifdef __CC_ARM
extern int Image$$RW_IRAM1$$ZI$$Limit;
#else
#error "not armcc"
#endif


#define STM32_HEAP_BEGIN    (void *)(&Image$$RW_IRAM1$$ZI$$Limit)
#define STM32_SRAM_SIZE     16
#define STM32_HEAP_END      (void *)(0x20000000 + STM32_SRAM_SIZE * 1024)

int main(void)
{	
	PrintfUtil_u8Init();
	NVIC_DisableIRQ(EXTI0_IRQn);
	DBG(PrintfUtil_vPrintf("\n ************** main start*************\n");)

	board_init();
	portResetTickCnt();

	Application();
}

