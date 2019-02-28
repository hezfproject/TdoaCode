#include "stm32l1xx.h"
#include <stm32l1xx_gpio.h>

void MOTOR_Configuration(void)
{
	GPIO_InitTypeDef GPIO_InitStructure;

    //�����
	GPIO_InitStructure.GPIO_Pin = GPIO_Pin_11;
	GPIO_InitStructure.GPIO_Mode = GPIO_Mode_OUT;		//��������
	GPIO_InitStructure.GPIO_OType = GPIO_OType_PP;
    GPIO_InitStructure.GPIO_PuPd = GPIO_PuPd_DOWN;
	GPIO_InitStructure.GPIO_Speed = GPIO_Speed_2MHz;	//2Mʱ���ٶ�
	GPIO_Init(GPIOA, &GPIO_InitStructure);

    //GPIO_ResetBits(GPIOA, GPIO_Pin_11);
    GPIO_SetBits(GPIOA, GPIO_Pin_11);
}

void MOTOR_Start(void)
{

    //GPIO PA11����
    GPIO_ResetBits(GPIOA, GPIO_Pin_11);
}

void MOTOR_Stop(void)
{
    //GPIO PA11����
    GPIO_SetBits(GPIOA, GPIO_Pin_11);
}


