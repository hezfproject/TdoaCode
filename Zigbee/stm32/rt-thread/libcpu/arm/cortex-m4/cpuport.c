/*
 * File      : cpuport.c
 * This file is part of RT-Thread RTOS
 * COPYRIGHT (C) 2006 - 2012, RT-Thread Development Team
 *
 * The license and distribution terms for this file may be
 * found in the file LICENSE in this distribution or at
 * http://www.rt-thread.org/license/LICENSE
 *
 * Change Logs:
 * Date           Author       Notes
 * 2011-10-21     Bernard      the first version.
 * 2011-10-27     aozima       update for cortex-M4 FPU.
 * 2011-12-31     aozima       fixed stack align issues.
 * 2012-01-01     aozima       support context switch load/store FPU register.
 * 2012-12-11     lgnq         fixed the coding style.
 * 2012-12-23     aozima       stack addr align to 8byte.
 * 2012-12-29     Bernard      Add exception hook.
 */

#include <rtthread.h>

#define USE_FPU   /* ARMCC */ (  (defined ( __CC_ARM ) && defined ( __TARGET_FPU_VFP )) \
                  /* IAR */   || (defined ( __ICCARM__ ) && defined ( __ARMVFP__ )) \
                  /* GNU */   || (defined ( __GNUC__ ) && defined ( __VFP_FP__ ) && !defined(__SOFTFP__)) )

/* exception and interrupt handler table */
rt_uint32_t rt_interrupt_from_thread;
rt_uint32_t rt_interrupt_to_thread;
rt_uint32_t rt_thread_switch_interrupt_flag;
/* exception hook */
static rt_err_t (*rt_exception_hook)(void *context) = RT_NULL;

struct exception_stack_frame
{
    rt_uint32_t r0;
    rt_uint32_t r1;
    rt_uint32_t r2;
    rt_uint32_t r3;
    rt_uint32_t r12;
    rt_uint32_t lr;
    rt_uint32_t pc;
    rt_uint32_t psr;

#if USE_FPU
    /* FPU register */
    rt_uint32_t S0;
    rt_uint32_t S1;
    rt_uint32_t S2;
    rt_uint32_t S3;
    rt_uint32_t S4;
    rt_uint32_t S5;
    rt_uint32_t S6;
    rt_uint32_t S7;
    rt_uint32_t S8;
    rt_uint32_t S9;
    rt_uint32_t S10;
    rt_uint32_t S11;
    rt_uint32_t S12;
    rt_uint32_t S13;
    rt_uint32_t S14;
    rt_uint32_t S15;
    rt_uint32_t FPSCR;
    rt_uint32_t NO_NAME;
#endif
};

struct stack_frame
{
    /* r4 ~ r11 register */
    rt_uint32_t r4;
    rt_uint32_t r5;
    rt_uint32_t r6;
    rt_uint32_t r7;
    rt_uint32_t r8;
    rt_uint32_t r9;
    rt_uint32_t r10;
    rt_uint32_t r11;

#if USE_FPU
    /* FPU register s16 ~ s31 */
    rt_uint32_t s16;
    rt_uint32_t s17;
    rt_uint32_t s18;
    rt_uint32_t s19;
    rt_uint32_t s20;
    rt_uint32_t s21;
    rt_uint32_t s22;
    rt_uint32_t s23;
    rt_uint32_t s24;
    rt_uint32_t s25;
    rt_uint32_t s26;
    rt_uint32_t s27;
    rt_uint32_t s28;
    rt_uint32_t s29;
    rt_uint32_t s30;
    rt_uint32_t s31;
#endif

    struct exception_stack_frame exception_stack_frame;
};

rt_uint8_t *rt_hw_stack_init(void       *tentry,
                             void       *parameter,
                             rt_uint8_t *stack_addr,
                             void       *texit)
{
    struct stack_frame *stack_frame;
    rt_uint8_t         *stk;
    unsigned long       i;

    stk  = stack_addr + sizeof(rt_uint32_t);
    stk  = (rt_uint8_t *)RT_ALIGN_DOWN((rt_uint32_t)stk, 8);
    stk -= sizeof(struct stack_frame);

    stack_frame = (struct stack_frame *)stk;

    /* init all register */
    for (i = 0; i < sizeof(struct stack_frame) / sizeof(rt_uint32_t); i ++)
    {
        ((rt_uint32_t *)stack_frame)[i] = 0xdeadbeef;
    }

    stack_frame->exception_stack_frame.r0  = (unsigned long)parameter; /* r0 : argument */
    stack_frame->exception_stack_frame.r1  = 0;                        /* r1 */
    stack_frame->exception_stack_frame.r2  = 0;                        /* r2 */
    stack_frame->exception_stack_frame.r3  = 0;                        /* r3 */
    stack_frame->exception_stack_frame.r12 = 0;                        /* r12 */
    stack_frame->exception_stack_frame.lr  = (unsigned long)texit;     /* lr */
    stack_frame->exception_stack_frame.pc  = (unsigned long)tentry;    /* entry point, pc */
    stack_frame->exception_stack_frame.psr = 0x01000000L;              /* PSR */

    /* return task's current stack address */
    return stk;
}

/**
 * This function set the hook, which is invoked on fault exception handling.
 *
 * @param exception_handle the exception handling hook function.
 */
void rt_hw_exception_install(rt_err_t (*exception_handle)(void* context))
{
	rt_exception_hook = exception_handle;
}

#if 0
void rt_hw_hard_fault_exception(struct exception_stack_frame *exception_stack)
{
	extern long list_thread(void);

	if (rt_exception_hook != RT_NULL)
	{
		rt_err_t result;

		result = rt_exception_hook(exception_stack);
		if (result == RT_EOK) return;
	}

    rt_kprintf("psr: 0x%08x\n", exception_stack->psr);
    rt_kprintf(" pc: 0x%08x\n", exception_stack->pc);
    rt_kprintf(" lr: 0x%08x\n", exception_stack->lr);
    rt_kprintf("r12: 0x%08x\n", exception_stack->r12);
    rt_kprintf("r03: 0x%08x\n", exception_stack->r3);
    rt_kprintf("r02: 0x%08x\n", exception_stack->r2);
    rt_kprintf("r01: 0x%08x\n", exception_stack->r1);
    rt_kprintf("r00: 0x%08x\n", exception_stack->r0);

    rt_kprintf("hard fault on thread: %s\n", rt_thread_self()->name);

#ifdef RT_USING_FINSH
    list_thread();
#endif
    while (1);
}
#else
#define SCB_CFSR        (*(volatile const unsigned *)0xE000ED28) /* Configurable Fault Status Register */
#define SCB_HFSR        (*(volatile const unsigned *)0xE000ED2C) /* HardFault Status Register */
#define SCB_MMAR        (*(volatile const unsigned *)0xE000ED34) /* MemManage Fault Address register */
#define SCB_BFAR        (*(volatile const unsigned *)0xE000ED38) /* Bus Fault Address Register */

#define SCB_CFSR_MFSR   (*(volatile const unsigned char*)0xE000ED28)  /* Memory-management Fault Status Register */
#define SCB_CFSR_BFSR   (*(volatile const unsigned char*)0xE000ED29)  /* Bus Fault Status Register */
#define SCB_CFSR_UFSR   (*(volatile const unsigned short*)0xE000ED2A) /* Usage Fault Status Register */

static void usage_fault_track(void)
{
    rt_kprintf("usage fault:\n");
    rt_kprintf("SCB_CFSR_UFSR:0x%02X ", SCB_CFSR_UFSR);

    if(SCB_CFSR_UFSR & (1<<0))
    {
        /* [0]:UNDEFINSTR */
        rt_kprintf("UNDEFINSTR ");
    }

    if(SCB_CFSR_UFSR & (1<<1))
    {
        /* [1]:INVSTATE */
        rt_kprintf("INVSTATE ");
    }

    if(SCB_CFSR_UFSR & (1<<2))
    {
        /* [2]:INVPC */
        rt_kprintf("INVPC ");
    }

    if(SCB_CFSR_UFSR & (1<<3))
    {
        /* [3]:NOCP */
        rt_kprintf("NOCP ");
    }

    if(SCB_CFSR_UFSR & (1<<8))
    {
        /* [8]:UNALIGNED */
        rt_kprintf("UNALIGNED ");
    }

    if(SCB_CFSR_UFSR & (1<<9))
    {
        /* [9]:DIVBYZERO */
        rt_kprintf("DIVBYZERO ");
    }

    rt_kprintf("\n");
}

static void bus_fault_track(void)
{
    rt_kprintf("bus fault:\n");
    rt_kprintf("SCB_CFSR_BFSR:0x%02X ", SCB_CFSR_BFSR);

    if(SCB_CFSR_BFSR & (1<<0))
    {
        /* [0]:IBUSERR */
        rt_kprintf("IBUSERR ");
    }

    if(SCB_CFSR_BFSR & (1<<1))
    {
        /* [1]:PRECISERR */
        rt_kprintf("PRECISERR ");
    }

    if(SCB_CFSR_BFSR & (1<<2))
    {
        /* [2]:IMPRECISERR */
        rt_kprintf("IMPRECISERR ");
    }

    if(SCB_CFSR_BFSR & (1<<3))
    {
        /* [3]:UNSTKERR */
        rt_kprintf("UNSTKERR ");
    }

    if(SCB_CFSR_BFSR & (1<<4))
    {
        /* [4]:STKERR */
        rt_kprintf("STKERR ");
    }

    if(SCB_CFSR_BFSR & (1<<7))
    {
        rt_kprintf("SCB->BFAR:%08X\n", SCB_BFAR);
    }
    else
    {
        rt_kprintf("\n");
    }
}

static void mem_manage_fault_track(void)
{
    rt_kprintf("mem manage fault:\n");
    rt_kprintf("SCB_CFSR_MFSR:0x%02X ", SCB_CFSR_MFSR);

    if(SCB_CFSR_MFSR & (1<<0))
    {
        /* [0]:IACCVIOL */
        rt_kprintf("IACCVIOL ");
    }

    if(SCB_CFSR_MFSR & (1<<1))
    {
        /* [1]:DACCVIOL */
        rt_kprintf("DACCVIOL ");
    }

    if(SCB_CFSR_MFSR & (1<<3))
    {
        /* [3]:MUNSTKERR */
        rt_kprintf("MUNSTKERR ");
    }

    if(SCB_CFSR_MFSR & (1<<4))
    {
        /* [4]:MSTKERR */
        rt_kprintf("MSTKERR ");
    }

    if(SCB_CFSR_MFSR & (1<<7))
    {
        /* [7]:MMARVALID */
        rt_kprintf("SCB->MMAR:%08X\n", SCB_MMAR);
    }
    else
    {
        rt_kprintf("\n");
    }
}

static void hard_fault_track(void)
{
    if(SCB_HFSR & (1UL<<1))
    {
        /* [1]:VECTBL, Indicates hard fault is caused by failed vector fetch. */
        rt_kprintf("failed vector fetch\n");
    }

    if(SCB_HFSR & (1UL<<30))
    {
        /* [30]:FORCED, Indicates hard fault is taken because of bus fault,
                        memory management fault, or usage fault. */
        if(SCB_CFSR_BFSR)
        {
            bus_fault_track();
        }

        if(SCB_CFSR_MFSR)
        {
            mem_manage_fault_track();
        }

        if(SCB_CFSR_UFSR)
        {
            usage_fault_track();
        }
    }

    if(SCB_HFSR & (1UL<<31))
    {
        /* [31]:DEBUGEVT, Indicates hard fault is triggered by debug event. */
        rt_kprintf("debug event\n");
    }
}

/**
 * fault exception handling
 */
void rt_hw_hard_fault_exception(struct exception_stack_frame *exception_stack)
{
    if (rt_exception_hook != RT_NULL)
	{
		rt_err_t result;

		result = rt_exception_hook(exception_stack);
		if (result == RT_EOK) return;
	}

    rt_kprintf("psr: 0x%08x\n", exception_stack->psr);
    rt_kprintf(" pc: 0x%08x\n", exception_stack->pc);
    rt_kprintf(" lr: 0x%08x\n", exception_stack->lr);
    rt_kprintf("r12: 0x%08x\n", exception_stack->r12);
    rt_kprintf("r03: 0x%08x\n", exception_stack->r3);
    rt_kprintf("r02: 0x%08x\n", exception_stack->r2);
    rt_kprintf("r01: 0x%08x\n", exception_stack->r1);
    rt_kprintf("r00: 0x%08x\n", exception_stack->r0);

    hard_fault_track();

    rt_kprintf("hard fault on thread: %s\n", rt_thread_self()->name);
#ifdef RT_USING_FINSH
    list_thread();
#endif
    while (1);
}
#endif

void rt_hw_cpu_shutdown(void)
{
    rt_kprintf("shutdown...\n");

    RT_ASSERT(0);
}
