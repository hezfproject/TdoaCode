#ifndef _MCU_H_
#define _MCU_H_

#include "cc_def.h"
#include "CommonTypes.h"

#define WATCHDOG_ENABLE
//#define CONFIG_IWDG_HW
//#define USING_CODE_SECURITY

#define BASE_TIME_TICK          1000
#define SYSTICK_CLOCK_KHz        ( SystemCoreClock / 8 / 1000 )
#define SysTickReloadValue        ( SYSTICK_CLOCK_KHz * BASE_TIME_TICK)

uint_32 GetSysClock(void);

void ElapsedSysClock(uint_32 tick);

//void SysTick_Configuration(void);

void set_flash_read_protection(void);

void unset_flash_read_protection(void);

void chip_core_init(void);

void motion_detect_init(void);

// open nanotorn rx interrupt
void radio_RxInt_init(void);

//void radio_RxInt_on(void);

// disable nanotorn rx interrupt
//void radio_RxInt_off(void);

void chip_interface_init(void);

void chip_lowpower_ready(void);

void chip_lowpower_finish(void);

void WatchdogReset(void);

uint_32 interrupt_disable(void);

void interrupt_enable(uint_32 reg);
void portSetTickCnt(uint_32 tick);

uint_32 portGetTickCnt(void);
void portResetTickCnt(void);





#endif

