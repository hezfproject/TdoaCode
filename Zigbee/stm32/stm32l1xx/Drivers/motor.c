#include "stm32l1xx.h"
#include <stm32l1xx_gpio.h>

void MOTOR_Configuration(void)
{
	GPIO_InitTypeDef GPIO_InitStructure;

    //振动马达
	GPIO_InitStructure.GPIO_Pin = GPIO_Pin_11;
	GPIO_InitStructure.GPIO_Mode = GPIO_Mode_OUT;		//上拉输入
	GPIO_InitStructure.GPIO_OType = GPIO_OType_PP;
    GPIO_InitStructure.GPIO_PuPd = GPIO_PuPd_DOWN;
	GPIO_InitStructure.GPIO_Speed = GPIO_Speed_2MHz;	//2M时钟速度
	GPIO_Init(GPIOA, &GPIO_InitStructure);

    //GPIO_ResetBits(GPIOA, GPIO_Pin_11);
    GPIO_SetBits(GPIOA, GPIO_Pin_11);
}

void MOTOR_Start(void)
{

    //GPIO PA11拉低
    GPIO_ResetBits(GPIOA, GPIO_Pin_11);
}

void MOTOR_Stop(void)
{
    //GPIO PA11拉高
    GPIO_SetBits(GPIOA, GPIO_Pin_11);
}


