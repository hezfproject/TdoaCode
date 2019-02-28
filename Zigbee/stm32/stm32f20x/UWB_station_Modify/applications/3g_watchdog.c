/*
 * 3g_watchdog.c
 *
 */

#include <rtthread.h>
#include <rthw.h>

#include "stm32f2xx_iwdg.h"
#include "stm32f2xx_gpio.h"
#include "stm32f2xx_rcc.h"
#include "stm32f2xx_flash.h"

#include "net_app.h"
#include "3g_log.h"

int iwdg_net_feed_flag = 0; // �����߳�ιiwdg����ʶ��Ϊ1��ʼ�������߳�ι��

/*
 * hw_watchdog Ӳ�����Ź���ʹ��GPIOE1��ʹ�õ�ƽ����ι��������Ϊ1.6s
 */

#define HW_WATCHDOG_GPIO        GPIOE
#define HW_WATCHDOG_GPIO_PIN    GPIO_Pin_1

static void _init_hw_watchdog()
{
    GPIO_InitTypeDef GPIO_InitStructure;
	GPIO_InitStructure.GPIO_Mode  = GPIO_Mode_OUT;
	GPIO_InitStructure.GPIO_OType = GPIO_OType_PP;
	GPIO_InitStructure.GPIO_PuPd  = GPIO_PuPd_UP;
	GPIO_InitStructure.GPIO_Speed = GPIO_Speed_2MHz;
	GPIO_InitStructure.GPIO_Pin = HW_WATCHDOG_GPIO_PIN;

	RCC_AHB1PeriphClockCmd(RCC_AHB1Periph_GPIOE, ENABLE);

	GPIO_Init(HW_WATCHDOG_GPIO, &GPIO_InitStructure);
    GPIO_ToggleBits(HW_WATCHDOG_GPIO, HW_WATCHDOG_GPIO_PIN);
}

static void _feed_hw_watchdog()
{
    GPIO_ToggleBits(HW_WATCHDOG_GPIO, HW_WATCHDOG_GPIO_PIN);
}

static void _hw_watchdog_feeder(void *arg)
{
    _feed_hw_watchdog();
}

enum {
    HW_IWDG = 0,    // IWDGӲ�����Ź����ԣ�ϵͳ�ӵ���Զ�����
    SW_IWDG,
};

/*
 * ����IWDG���Ź�����ΪӲ�����Ź���������Ź�
 */
static void _config_iwdg(int iwdg_config)
{
    uint8_t ob_iwdg = 0;
    uint8_t ob_stop = 0;
    uint8_t ob_stdby = 0;
    uint8_t user_opt = 0;
    uint8_t changed = 0;

    FLASH_OB_Unlock();

    user_opt = FLASH_OB_GetUser();

    if (iwdg_config == HW_IWDG) {
        if (user_opt & 0x1) {
            ob_iwdg = OB_IWDG_HW;
            changed = 1;
        }
    } else if (iwdg_config == SW_IWDG){
        if (!(user_opt & 0x1)) {
            ob_iwdg = OB_IWDG_SW;
            changed = 1;
        }
    }

    if (changed) {
        if (user_opt & 0x2) {
            ob_stop = OB_STOP_RST;
        } else {
            ob_stop = OB_STOP_NoRST;
        }
        if (user_opt & 0x4) {
            ob_stdby = OB_STDBY_RST;
        } else {
            ob_stdby = OB_STDBY_NoRST;
        }
        do {
            FLASH_OB_UserConfig(ob_iwdg, ob_stop, ob_stdby);
        } while (FLASH_OB_Launch() != FLASH_COMPLETE);
    }

    FLASH_OB_Lock();
}

#define WATCHDOG_MS_INTERVAL  6000

/*
 * ����STM32�ڲ��������Ź�IWDG��ι������Ϊ6s
 */
static void _startup_iwdg()
{
    RCC_LSICmd(ENABLE);
    while (RCC_GetFlagStatus(RCC_FLAG_LSIRDY) == RESET);

    IWDG_WriteAccessCmd(IWDG_WriteAccess_Enable);
    IWDG_SetPrescaler(IWDG_Prescaler_64);
    IWDG_SetReload(WATCHDOG_MS_INTERVAL / 2);
    IWDG_ReloadCounter();
    IWDG_Enable();
}

static void _feed_iwdg()
{
    IWDG_ReloadCounter();
}

void startup_watchdog()
{
#ifdef ENABLE_HW_WATCHDOG
    _init_hw_watchdog();
#else
    GPIO_InitTypeDef GPIO_InitStructure;
	GPIO_InitStructure.GPIO_Mode  = GPIO_Mode_OUT;
	GPIO_InitStructure.GPIO_OType = GPIO_OType_PP;
	GPIO_InitStructure.GPIO_PuPd  = GPIO_PuPd_UP;
	GPIO_InitStructure.GPIO_Speed = GPIO_Speed_2MHz;
	GPIO_InitStructure.GPIO_Pin = HW_WATCHDOG_GPIO_PIN;

	RCC_AHB1PeriphClockCmd(RCC_AHB1Periph_GPIOE, ENABLE);

	GPIO_Init(HW_WATCHDOG_GPIO, &GPIO_InitStructure);
    GPIO_ResetBits(HW_WATCHDOG_GPIO, HW_WATCHDOG_GPIO_PIN);
#endif

#ifdef CONFIG_IWDG_HW
    _config_iwdg(HW_IWDG);
#else
    _config_iwdg(SW_IWDG);
#endif

#ifdef ENABLE_IWDG
    _startup_iwdg();
#endif
}

void feed_watchdog()
{
#ifdef ENABLE_IWDG
    _feed_iwdg();
#endif
}

/*
 * ʹ��ϵͳ�����ʱ��ιӲ��������ʱ������Ϊ0.5s
 */
void startup_hw_watchdog_feeding_timer()
{
#ifdef ENABLE_HW_WATCHDOG
    static struct rt_timer feed_hw_wtd_timer;
    rt_timer_init(&feed_hw_wtd_timer, "hardware watchdog feeding timer",
        _hw_watchdog_feeder, RT_NULL, RT_TICK_PER_SECOND / 2, RT_TIMER_FLAG_PERIODIC);
    rt_timer_start(&feed_hw_wtd_timer);
    _feed_hw_watchdog();
#endif
}

/*
 * ʹ��ϵͳ�����λ�������߳�����������ʶ��
 * �������һ���߳����в���������ι������ϵͳ
 * ��ʱ������Ϊ4s��ϵͳ��20s��δ�ָ�����������ϵͳ
 */

static void _iwdg_feeding_timer_handler(void* arg)
{
    int fine = 0;
    rt_base_t level = 0;
    static int cnt = 5;

    level = rt_hw_interrupt_disable();
    if (net_recv_fine_flag && net_send_fine_flag)
    {
        fine = 1;
        cnt = 5;
    }
    else
    {
        cnt--;
    }
    net_recv_fine_flag = 0;
    net_send_fine_flag = 0;
    rt_hw_interrupt_enable(level);

    if (fine || (cnt > 0))
    {
        _feed_iwdg();
    }
}

void startup_iwdg_feeding_timer()
{
#ifdef ENABLE_IWDG
    static struct rt_timer feed_iwdg_timer;
    rt_timer_init(&feed_iwdg_timer, "iwdg feeding timer",
        _iwdg_feeding_timer_handler, RT_NULL, RT_TICK_PER_SECOND * 4,
        RT_TIMER_FLAG_PERIODIC);
    rt_timer_start(&feed_iwdg_timer);
    _feed_iwdg();
#endif
}
