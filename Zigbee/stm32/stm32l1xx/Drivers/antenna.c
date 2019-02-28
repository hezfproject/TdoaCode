#include "stm32l1xx.h"
#include "cc_def.h"
#include "CommonTypes.h"
#include "antenna.h"

//L：PIFA天线 H：chip天线
void ANTENNA_Configuration(void)
{
	GPIO_InitTypeDef GPIO_InitStructure;

	GPIO_InitStructure.GPIO_Pin = GPIO_Pin_13;
	GPIO_InitStructure.GPIO_Mode = GPIO_Mode_OUT;
	GPIO_InitStructure.GPIO_OType = GPIO_OType_PP;
    GPIO_InitStructure.GPIO_PuPd = GPIO_PuPd_DOWN;
	GPIO_InitStructure.GPIO_Speed = GPIO_Speed_40MHz;
	GPIO_Init(GPIOC, &GPIO_InitStructure);

    GPIO_SetBits(GPIOC, GPIO_Pin_13);
}

void ANTENNA_Choose_Pifa(void)
{
    GPIO_SetBits(GPIOC, GPIO_Pin_13);
}

void ANTENNA_Choose_On_Chip(void)
{
    GPIO_ResetBits(GPIOC, GPIO_Pin_13);
}

void ANTENNA_Alter(void)
{
    uint_8 value = GPIO_ReadOutputDataBit(GPIOC, GPIO_Pin_13);//读出PC13的状态

    if (value)
    {
        GPIO_ResetBits(GPIOC, GPIO_Pin_13);
    }
    else
    {
        GPIO_SetBits(GPIOC, GPIO_Pin_13);
    }
}



