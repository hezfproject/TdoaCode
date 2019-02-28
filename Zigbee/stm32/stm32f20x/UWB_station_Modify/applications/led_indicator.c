/**
 * running state indicator -- GPIO
 * system -- GPE3
 * modbus -- GPE2
 * light up -- level 0
 * light off -- level 1
 */

#include "rtthread.h"
#include "stm32f2xx_gpio.h"
#include "stm32f2xx_rcc.h"

#define STATE_INDICATOR_GPIO    GPIOE
#define MODBUS_STATE_GPIO_PIN   GPIO_Pin_2
#define SYS_STATE_GPIO_PIN      GPIO_Pin_3
#define RSSI_MODULES_STATE_GPIO_PIN      GPIO_Pin_12


void init_sys_status_indicator_lamps()
{
    GPIO_InitTypeDef GPIO_InitStructure;
	GPIO_InitStructure.GPIO_Mode  = GPIO_Mode_OUT;
	GPIO_InitStructure.GPIO_OType = GPIO_OType_PP;
	GPIO_InitStructure.GPIO_PuPd  = GPIO_PuPd_UP;
	GPIO_InitStructure.GPIO_Speed = GPIO_Speed_2MHz;
	GPIO_InitStructure.GPIO_Pin = MODBUS_STATE_GPIO_PIN | SYS_STATE_GPIO_PIN | RSSI_MODULES_STATE_GPIO_PIN;

	RCC_AHB1PeriphClockCmd(RCC_AHB1Periph_GPIOE, ENABLE);
    
	GPIO_Init(STATE_INDICATOR_GPIO, &GPIO_InitStructure);
    GPIO_SetBits(STATE_INDICATOR_GPIO, SYS_STATE_GPIO_PIN);
    GPIO_SetBits(STATE_INDICATOR_GPIO, MODBUS_STATE_GPIO_PIN);
	GPIO_SetBits(STATE_INDICATOR_GPIO, RSSI_MODULES_STATE_GPIO_PIN);

}
void light_up_sys_running_indicator()
{
    GPIO_ResetBits(STATE_INDICATOR_GPIO, SYS_STATE_GPIO_PIN);
}

void light_off_sys_running_indicator()
{
    GPIO_SetBits(STATE_INDICATOR_GPIO, SYS_STATE_GPIO_PIN);
}

void light_up_modbus_data_indicator()
{
    GPIO_ResetBits(STATE_INDICATOR_GPIO, MODBUS_STATE_GPIO_PIN);
}

void light_off_modbus_data_indicator()
{
    GPIO_SetBits(STATE_INDICATOR_GPIO, MODBUS_STATE_GPIO_PIN);
}
void light_up_rssi_modules_data_indicator()
{
    GPIO_ResetBits(STATE_INDICATOR_GPIO, RSSI_MODULES_STATE_GPIO_PIN);
}

void light_off_rssi_modules_data_indicator()
{
    GPIO_SetBits(STATE_INDICATOR_GPIO, RSSI_MODULES_STATE_GPIO_PIN);
}

/**
 * 系统运行状态指示函数，系统线程切换时调用，为调度器钩子函数
 */
void hook_system_running_state(rt_thread_t from, rt_thread_t to)
{
    GPIO_ToggleBits(STATE_INDICATOR_GPIO, SYS_STATE_GPIO_PIN);
}

static void _chang_sys_lamp_state(void *arg)
{
    //GPIO_ToggleBits(STATE_INDICATOR_GPIO, SYS_STATE_GPIO_PIN);
}

/*
 * 使用系统软件定时器改变系统运行指示灯状态，定时周期为1.5s
 */

#define STATE_LAMP_BLINK_CYCLE  (1.5)

void startup_sys_running_state_lamp_timer()
{
    static struct rt_timer sys_status_timer;
    rt_timer_init(&sys_status_timer, "sys running status lamp timer", 
        _chang_sys_lamp_state, RT_NULL, 
        RT_TICK_PER_SECOND * STATE_LAMP_BLINK_CYCLE, 
        RT_TIMER_FLAG_PERIODIC);
    rt_timer_start(&sys_status_timer);
}

