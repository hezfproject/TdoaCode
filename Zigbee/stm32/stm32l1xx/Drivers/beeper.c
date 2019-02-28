#include "stm32l1xx.h"
#include <stm32l1xx_gpio.h>

#include "cc_def.h"
#include "CommonTypes.h"

void BEEP_Configuration(void)
{
	GPIO_InitTypeDef GPIO_InitStructure;
	TIM_TimeBaseInitTypeDef  TIM_TimeBaseStructure;
	TIM_OCInitTypeDef  TIM_OCInitStructure;

    /* ---------------------------GPIO Configuration -----------------------------*/
    GPIO_InitStructure.GPIO_Mode  = GPIO_Mode_AF;
    GPIO_InitStructure.GPIO_Speed = GPIO_Speed_40MHz;
    GPIO_InitStructure.GPIO_OType = GPIO_OType_PP;
    GPIO_InitStructure.GPIO_PuPd  = GPIO_PuPd_UP;
    GPIO_InitStructure.GPIO_Pin = GPIO_Pin_5;
    GPIO_Init(GPIOB, &GPIO_InitStructure);

    GPIO_PinAFConfig(GPIOB, GPIO_PinSource5, GPIO_AF_TIM3);

    /* ---------------------------------------------------------------------------
    TIM9 Configuration: generate a PWM signal:
    The TIM11CLK frequency is set to SystemCoreClock (Hz).
    SystemCoreClock is set to 32 MHz for Ultra Low Power Medium-Density Devices.
    TIM3 prescaler is set to 0
    ---------------------------------------------------------------------------- */
    RCC_APB1PeriphClockCmd(RCC_APB1Periph_TIM3, ENABLE);

    /* Time base configuration */
    TIM_TimeBaseStructure.TIM_Period =12307  ;// ;13912 //32M/2300=13913
    TIM_TimeBaseStructure.TIM_Prescaler = 0;
    TIM_TimeBaseStructure.TIM_ClockDivision = 0;
    TIM_TimeBaseStructure.TIM_CounterMode = TIM_CounterMode_Up;

    TIM_TimeBaseInit(TIM3, &TIM_TimeBaseStructure);

    /* PWM1 Mode configuration: Channel2 */
    TIM_OCInitStructure.TIM_OCMode = TIM_OCMode_PWM1;
    TIM_OCInitStructure.TIM_OutputState = TIM_OutputState_Enable;
    TIM_OCInitStructure.TIM_Pulse =6153 ;//;6956	//(13913-1)/2
    TIM_OCInitStructure.TIM_OCPolarity = TIM_OCPolarity_High;

    TIM_OC2Init(TIM3, &TIM_OCInitStructure);

    TIM_OC2PreloadConfig(TIM3, TIM_OCPreload_Enable);

    TIM_ARRPreloadConfig(TIM3, ENABLE);

    /* TIM9 enable counter */
    TIM_Cmd(TIM3, DISABLE);
}


void BEEP_Start(void)
{
    BEEP_Configuration();
    TIM_Cmd(TIM3,ENABLE);
}

void BEEP_Stop(void)
{
    TIM_Cmd(TIM3,DISABLE);
    RCC_APB2PeriphClockCmd(RCC_APB1Periph_TIM3, DISABLE);
}

