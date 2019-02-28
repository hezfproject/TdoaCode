#include "stm32l1xx.h"
#include "sleep.h"
//#include "ntrxdrv.h"
#include "nanotron.h"
#include "mcu.h"
extern Bool isMoving;



void RTC_WKUP_IRQHandler(void)
{
    EXTI_ClearITPendingBit(EXTI_Line20);
    RTC_ClearITPendingBit(RTC_IT_WUT);
}

/**
  * @brief  Configures RTC clock source and prescaler.
  * @param  None
  * @retval : None
  */



static __INLINE void RestoreSysClk(void)
{
    /* Enable HSE */
    RCC_HSEConfig(RCC_HSE_ON);

    /* Wait till HSE is ready */
    while (RCC_GetFlagStatus(RCC_FLAG_HSERDY) == RESET)
    {}

    /* Enable PLL */
    RCC_PLLCmd(ENABLE);

    /* Wait till PLL is ready */
    while (RCC_GetFlagStatus(RCC_FLAG_PLLRDY) == RESET)
    {}

    /* Select PLL as system clock source */
    RCC_SYSCLKConfig(RCC_SYSCLKSource_PLLCLK);

    /* Wait till PLL is used as system clock source */
    while (RCC_GetSYSCLKSource() != 0x0C)
    {}
}

void SleepInit(void)
{
    /* Enable PWR and BKP clock */
    RCC_APB1PeriphClockCmd(RCC_APB1Periph_PWR, ENABLE);
    LCD_Cmd(DISABLE);
    /* Configure RTC clock source and prescaler */
    RTC_Configuration();
}

void Sleep(uint32 ms)
{
    uint32 sleep_tick;

    WatchdogReset();

    sleep_tick = CONVER_MS_TO_SLEEPTICK(ms); // time convert tick

    if (sleep_tick > 0xFFFF)                // IS_RTC_WAKEUP_COUNTER()
        sleep_tick = 0xFFFF;
    if (!sleep_tick)
        return;

    EXTI_ClearITPendingBit(EXTI_Line20);
    RTC_ClearITPendingBit(RTC_IT_WUT);

    /* Enable Wakeup Counter */
    RTC_WakeUpCmd(DISABLE);

    RTC_SetWakeUpCounter(sleep_tick);

    /* Enables Wakeup Counter */
    RTC_WakeUpCmd(ENABLE);

    /* Enter Stop Mode */
    PWR_EnterSTOPMode(PWR_Regulator_LowPower, PWR_STOPEntry_WFI);//PWR_STOPEntry_WFE

    /* Disables Wakeup Counter */
    RTC_WakeUpCmd(DISABLE);

    /* After wake-up from STOP reconfigure the system clock */
    // 3.5ms
    RestoreSysClk();

    // 8us
    SysTick_Configuration();

    WatchdogReset();
}


static void sleep_each(uint32 sleepms)
{

	int ms=sleepms;
    if (ms <= SLEEP_TIME_MIN)
    {
        return;
    }

    // sleep ready + wakeup about 14ms
    ms -= SLEEP_TIME_MIN;



    dwt_entersleep();
	vCheckBattery();


    chip_lowpower_ready();

    // goto sleep
    // 8us + sleep ms + 3.5ms + 8us
    if(ms >20000)
    {
	    while(ms>=20000 && isMoving == False)
	    {
	    	Sleep(20000);
			ms= ms-20000;
	    }
		if(ms>14&& isMoving == False)
			Sleep(ms);
    }
	else
		Sleep(ms);
    // init module IO
    // up and down 3-statement about 4us
    chip_lowpower_finish();
	
    ElapsedSysClock((ms+14));
}




void system_powersave_sleep(uint32 ms)
{
    Sleep(ms);
}

void system_powersave_wakeup(void)
{
    interrupt_disable();
    NVIC_SystemReset();
}


//×îÉÙÐÝÃß27ms
void system_sleep(uint32 ms)
{
    uint32 sleep_time;
/*    if (ms > MAX_SLEEP_EACH)
    {
        sleep_time = MAX_SLEEP_EACH;
    }
    else
    {
        sleep_time = ms;
    }
*/
    sleep_each(ms);
}










