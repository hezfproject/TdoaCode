/*****************************************************************************
 *
 * MODULE:              Definitions specific to a particular processor
 *
 * COMPONENT:           $RCSfile: MicroSpecific.h,v $
 *
 * VERSION:             $Name: HEAD $
 *
 * REVISION:            $Revision: 1.1 $
 *
 * DATED:               $Date: 2011/01/14 22:35:27 $
 *
 * STATUS:              $State: Exp $
 *
 * AUTHOR:              CJG
 *
 * DESCRIPTION:
 * Definitions for a specific processor, i.e. functions that can only be
 * resolved by op codes
 *
 * CHANGE HISTORY:
 *
 * $Log: MicroSpecific.h,v $
 * Revision 1.1  2011/01/14 22:35:27  chenxun
 * no message
 *
 * Revision 1.16  2009/02/11 17:48:24  cjg
 * Version suitable for:
 * JN5121 (v3)
 * JN5139
 * JN5139J01
 * JN5139T01
 * JN5147
 * JN5148
 *
 * Requires JENNIC_CHIP_JNxxxx to be defined so that chip version can be
 * unambiguously determined
 *
 * Revision 1.15  2007/05/31 08:47:16  cjg
 * Added back in the or32 macros
 *
 * Revision 1.14  2007/05/25 16:47:17  cjg
 * Added second define for JN513xR1
 *
 * Revision 1.13  2007/04/13 12:52:27  we1
 * Added define for Cacti
 *
 * Revision 1.12  2007/04/02 15:51:22  we1
 * Version for Cacti
 * Uses Osmium instructions
 *
 * Revision 1.11  2007/01/25 10:31:33  cjg
 * Added definition for JN513x-002
 *
 * Revision 1.10  2006/11/21 14:26:38  cjg
 * Added interrupt store and restore (from branch 1.7.1.1)
 *
 * Revision 1.9  2006/07/26 14:38:10  we1
 * ROM size changed from 160k to 192k
 *
 * Revision 1.8  2006/07/26 09:38:24  we1
 * added rom MACRO's
 *
 * Revision 1.7  2006/06/01 09:15:20  cjg
 * Added JN513x settings
 *
 * Revision 1.6  2006/05/18 06:15:48  cjg
 * Added define for JN5131
 *
 * Revision 1.5  2005/11/28 17:28:57  cjg
 * Interrupt handler changes
 *
 * Revision 1.4  2005/11/11 11:24:32  rcc
 * New ISR infrastructure to save RAM
 *
 * Revision 1.3  2005/10/07 09:48:23  cjg
 * Added tick timer support
 *
 * Revision 1.2  2005/08/05 15:08:14  rcc
 * Added VSR definitions
 *
 * Revision 1.1.1.1  2005/06/10 09:26:40  rcc
 * Initial release
 *
 * Revision 1.1.1.1  2005/06/10 09:13:44  rcc
 * Initial release
 *
 * Revision 1.2  2005/05/04 16:11:02  rcc
 * Added defines for numbers and ISR location for ZED002
 *
 * Revision 1.1  2005/04/11 07:28:48  cjg
 * First version
 *
 *
 * LAST MODIFIED BY:    $Author: chenxun $
 *                      $Modtime: $
 *
 ****************************************************************************
 *
 *  (c) Copyright JENNIC Ltd 2005
 *
 ****************************************************************************/

#ifndef  MICRO_SPECIFIC_INCLUDED
#define  MICRO_SPECIFIC_INCLUDED

#if defined __cplusplus
extern "C" {
#endif

/****************************************************************************/
/***        Include Files                                                 ***/
/****************************************************************************/
#include <jendefs.h>

/****************************************************************************/
/***        Macro Definitions                                             ***/
/****************************************************************************/

#ifdef EMBEDDED

#ifndef USER_VSR_HANDLER
#error Build not supported!
#endif

#if defined JENNIC_CHIP_JN5121
#define MICRO_ROM_SIZE 0x00010000
#define JENNIC_CPU_BA1
#elif defined JENNIC_CHIP_JN5139
#define MICRO_ROM_SIZE 0x00030000
#define JENNIC_CPU_BA1
#elif defined JENNIC_CHIP_JN5139J01
#define MICRO_ROM_SIZE 0x00030000
#define JENNIC_CPU_BA1
#elif defined JENNIC_CHIP_JN5139T01
#define MICRO_ROM_SIZE 0x00030000
#define JENNIC_CPU_BA1
#elif defined JENNIC_CHIP_JN5147
#define MICRO_ROM_SIZE 0x00020000
#define JENNIC_CPU_BA2
#elif defined JENNIC_CHIP_JN5148
#define MICRO_ROM_SIZE 0x00020000
#define JENNIC_CPU_BA2
#elif defined JENNIC_CHIP_JN5148T01
#define MICRO_ROM_SIZE 0x00020000
#define JENNIC_CPU_BA2
#define JENNIC_HW_PRIORITY_INTERRUPTS
#elif defined JENNIC_CHIP_JN5148T01_HDK
#define MICRO_ROM_SIZE 0x00020000
#define JENNIC_CPU_BA2
#define JENNIC_HW_PRIORITY_INTERRUPTS
#else
#error Chip not supported!
#endif

/* MicroInt.h relies on defines above */
//#include "MicroInt.h"

#ifdef CHIP_RELEASE_1
 #define MICRO_VSR_BASE 0xf0016800
 #define MICRO_ISR_BASE 0xf0017008
#else
 #ifdef CHIP_RELEASE_2
  #define MICRO_VSR_BASE 0xf0017f80
  #define MICRO_ISR_BASE 0xf001700c
 #else
  #ifdef CHIP_RELEASE_3
   #define MICRO_VSR_BASE 0xf0017f80
   #define MICRO_ISR_BASE 0xf001702c
  #else
   #define MICRO_VSR_BASE 0x04000000
  #endif
 #endif
#endif

#define MICRO_ROM_BASE 0x00000000

#define MICRO_VSR_NUM_TICK      5
#define MICRO_VSR_NUM_EXT       8

#define MICRO_IEE_MASK          (1 << 2)
#define MICRO_TEE_MASK          (1 << 1)


#ifdef JENNIC_CPU_BA2

#define MICRO_ISR_NUM_AUDIOFIFO 0
#define MICRO_ISR_NUM_I2S       1
#define MICRO_ISR_NUM_SYSCTRL   2
#define MICRO_ISR_NUM_BBC       3
#define MICRO_ISR_NUM_AES       4
#define MICRO_ISR_NUM_PHY       5
#define MICRO_ISR_NUM_UART0     6
#define MICRO_ISR_NUM_UART1     7
#define MICRO_ISR_NUM_TMR0      8
#define MICRO_ISR_NUM_TMR1      9
#define MICRO_ISR_NUM_I2C       10
#define MICRO_ISR_NUM_SPIM      11
#define MICRO_ISR_NUM_INTPER    12
#define MICRO_ISR_NUM_ANPER     13
#define MICRO_ISR_NUM_TMR2      14
#define MICRO_ISR_NUM_TICK_TMR  15

#else /* JENNIC_CPU */

/* For user VSR, tick timer is at index 0, system controller at 1, BBC at 2,
   etc. for efficiency */
#define MICRO_ISR_NUM_TICK_TMR  0
#define MICRO_ISR_NUM_SYSCTRL   1
#define MICRO_ISR_NUM_BBC       2
#define MICRO_ISR_NUM_AES       3
#define MICRO_ISR_NUM_PHY       4
#define MICRO_ISR_NUM_UART0     5
#define MICRO_ISR_NUM_UART1     6
#define MICRO_ISR_NUM_TMR0      7
#define MICRO_ISR_NUM_TMR1      8
#ifdef CHIP_RELEASE_1
#define MICRO_ISR_NUM_TMR2      9
#else
#define MICRO_ISR_NUM_I2C       9
#endif
#define MICRO_ISR_NUM_SPIM      10
#define MICRO_ISR_NUM_INTPER    11
#define MICRO_ISR_NUM_ANPER     12

#endif /* JENNIC_CPU */

#define MICRO_ISR_MASK_TICK_TMR (1 << MICRO_ISR_NUM_TICK_TMR)
#define MICRO_ISR_MASK_SYSCTRL  (1 << MICRO_ISR_NUM_SYSCTRL)
#define MICRO_ISR_MASK_BBC      (1 << MICRO_ISR_NUM_BBC)
#define MICRO_ISR_MASK_AES      (1 << MICRO_ISR_NUM_AES)
#define MICRO_ISR_MASK_PHY      (1 << MICRO_ISR_NUM_PHY)
#define MICRO_ISR_MASK_UART0    (1 << MICRO_ISR_NUM_UART0)
#define MICRO_ISR_MASK_UART1    (1 << MICRO_ISR_NUM_UART1)
#define MICRO_ISR_MASK_TMR0     (1 << MICRO_ISR_NUM_TMR0)
#define MICRO_ISR_MASK_TMR1     (1 << MICRO_ISR_NUM_TMR1)
#define MICRO_ISR_MASK_I2C      (1 << MICRO_ISR_NUM_I2C)
#define MICRO_ISR_MASK_SPIM     (1 << MICRO_ISR_NUM_SPIM)
#define MICRO_ISR_MASK_INTPER   (1 << MICRO_ISR_NUM_INTPER)
#define MICRO_ISR_MASK_ANPER    (1 << MICRO_ISR_NUM_ANPER)


#ifdef JENNIC_CPU_BA2

#define MICRO_ENABLE_INTERRUPTS();                                          \
        asm volatile ("b.ei;" : : );

#define MICRO_DISABLE_INTERRUPTS();                                         \
        asm volatile ("b.di;" : : );

#define MICRO_DISABLE_AND_SAVE_INTERRUPTS(u32Store);                        \
    {                                                                       \
        asm volatile ("b.mfspr %0, r0, 17;" :"=r"(u32Store) : );            \
        asm volatile ("b.di;" : : );                                        \
    }
#define MICRO_RESTORE_INTERRUPTS(u32Store);                                 \
        asm volatile ("b.mtspr r0, %0, 17;" : :"r"(u32Store));

#ifdef JENNIC_HW_PRIORITY_INTERRUPTS
extern void vAHI_InterruptSetPriority(uint16 u16Mask, uint8 u8Level);
#define MICRO_ENABLE_TICK_TIMER_INTERRUPT();                                \
    vAHI_InterruptSetPriority(MICRO_ISR_MASK_TICK_TMR,8);

#define MICRO_SET_PIC_ENABLE(A);                                            \
    vAHI_InterruptSetPriority(A,8);

#define MICRO_SET_ACTIVE_INT_LEVEL(A);                                      \
    asm volatile ("b.mtspr r0, %0, 0x4810;" : :"r"(A));

#define MICRO_GET_ACTIVE_INT_LEVEL()                                        \
    ({                                                                      \
        register uint32 __result;                                           \
        asm volatile ("b.mfspr %0, r0, 0x4810;" : "=r"(__result) :);        \
        __result;                                                           \
    })
#else /* JENNIC_HW_PRIORITY_INTERRUPTS */
#define MICRO_ENABLE_TICK_TIMER_INTERRUPT();                                \
    {                                                                       \
        register uint32 ruCtrlReg;                                          \
        asm volatile ("b.mfspr %0, r0, 17;" :"=r"(ruCtrlReg) : );           \
        ruCtrlReg |= 2;                                                     \
        asm volatile ("b.mtspr r0, %0, 17;" : :"r"(ruCtrlReg));             \
    }
#define MICRO_SET_PIC_ENABLE(A);                                            \
    {                                                                       \
        register uint32 ruCtrlReg;                                          \
        asm volatile ("b.mfspr %0, r0, %1;" :"=r"(ruCtrlReg) : "i"(0x4800));\
        ruCtrlReg |= A;                                                     \
        asm volatile ("b.mtspr r0, %0, %1;" : :"r"(ruCtrlReg), "i"(0x4800));\
    }

#define MICRO_CLEAR_PIC();                                                  \
    asm volatile ("b.mtspr r0, r0, 0x4802;" : : );

#endif /* JENNIC_HW_PRIORITY_INTERRUPTS */

#else /* JENNIC_CPU */

#define MICRO_ENABLE_INTERRUPTS();                                          \
    {                                                                       \
        register uint32 ruCtrlReg;                                          \
        asm volatile ("l.mfspr %0, r0, 17;" :"=r"(ruCtrlReg) : );           \
        ruCtrlReg |= 4;                                                     \
        asm volatile ("l.mtspr r0, %0, 17;" : :"r"(ruCtrlReg));             \
    }

#define MICRO_DISABLE_AND_SAVE_INTERRUPTS(u32Store);                        \
    {                                                                       \
        register uint32 ruCtrlReg;                                          \
        asm volatile ("l.mfspr %0, r0, 17;" :"=r"(u32Store) : );            \
        ruCtrlReg = u32Store & 0xfffffff9;                                  \
        asm volatile ("l.mtspr r0, %0, 17;" : :"r"(ruCtrlReg));             \
    }

#define MICRO_RESTORE_INTERRUPTS(u32Store);                                 \
        asm volatile ("l.mtspr r0, %0, 17;" : :"r"(u32Store));

#define MICRO_ENABLE_TICK_TIMER_INTERRUPT();                                \
    {                                                                       \
        register uint32 ruCtrlReg;                                          \
        asm volatile ("l.mfspr %0, r0, 17;" :"=r"(ruCtrlReg) : );           \
        ruCtrlReg |= 2;                                                     \
        asm volatile ("l.mtspr r0, %0, 17;" : :"r"(ruCtrlReg));             \
    }

#define MICRO_SET_PIC_ENABLE(A);                                            \
    {                                                                       \
        register uint32 ruCtrlReg;                                          \
        asm volatile ("l.mfspr %0, r0, %1;" :"=r"(ruCtrlReg) : "i"(0x4800));\
        ruCtrlReg |= A;                                                     \
        asm volatile ("l.mtspr r0, %0, %1;" : :"r"(ruCtrlReg), "i"(0x4800));\
    }
#define MICRO_CLEAR_PIC();                                                  \
    asm volatile ("l.mtspr r0, r0, 0x4802;" : : );

#endif /* JENNIC_CPU */

/* Handler registration */

#define MICRO_SET_INT_HANDLER(INT, FUNC);                                   \
    ((void **)(*(uint32 *)0x44))[(INT)] = (void *)(FUNC);

#define MICRO_GET_INT_HANDLER(INT)                                          \
    (((void **)(*(uint32 *)0x44))[(INT)])

#define MICRO_SET_VSR_HANDLER(INT, FUNC);                                   \
    *(void **)(MICRO_VSR_BASE + ((INT) << 2)) = (void *)(FUNC);

#else /* EMBEDDED */

#define MICRO_ENABLE_INTERRUPTS();
#define MICRO_DISABLE_INTERRUPTS();
#define MICRO_ENABLE_TICK_TIMER_INTERRUPT();
#define MICRO_SET_PIC_ENABLE(A);
#define MICRO_CLEAR_PIC();
#define MICRO_SET_INT_HANDLER(INT, FUNC);
#define MICRO_SET_VSR_HANDLER(INT, FUNC);

#endif /* EMBEDDED */

/****************************************************************************/
/***        Type Definitions                                              ***/
/****************************************************************************/

/****************************************************************************/
/***        Exported Functions                                            ***/
/****************************************************************************/

/****************************************************************************/
/***        Exported Variables                                            ***/
/****************************************************************************/

#if defined __cplusplus
}
#endif

#endif  /* MICRO_SPECIFIC_INCLUDED */

/****************************************************************************/
/***        END OF FILE                                                   ***/
/****************************************************************************/

