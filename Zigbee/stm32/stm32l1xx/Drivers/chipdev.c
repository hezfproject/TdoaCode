#include "stm32l1xx.h"
#include "cc_def.h"
#include "CommonTypes.h"
#include "nanotron.h"

/*******************************************************************************
*   ntrxdrv.h
*******************************************************************************/

void NtrxSSN_Lo(void)
{
    //Clears the selected data port bits.
    GPIO_ResetBits(NTRX_SSN_PORT, NTRX_SSN_PIN) ;
}

void NtrxSSN_Hi(void)
{
    GPIO_SetBits(NTRX_SSN_PORT, NTRX_SSN_PIN) ;
}

void NtrxResetOn(void)
{
    GPIO_ResetBits(NTRX_RST_PORT, NTRX_RST_PIN) ;
}

void NtrxResetOff(void)
{
    GPIO_SetBits(NTRX_RST_PORT, NTRX_RST_PIN) ;
}

void NtrxCtrlInit(void)
{
    GPIO_InitTypeDef GPIO_InitStructure ;

    RCC_AHBPeriphClockCmd(RCC_AHBPeriph_GPIO_SSN | RCC_AHBPeriph_GPIO_RST
                           | RCC_AHBPeriph_GPIO_DIO, ENABLE);

    GPIO_StructInit(&GPIO_InitStructure);

    GPIO_InitStructure.GPIO_Pin = NTRX_SSN_PIN;
    GPIO_InitStructure.GPIO_Speed = GPIO_Speed_40MHz;
    GPIO_InitStructure.GPIO_Mode = GPIO_Mode_OUT;
    GPIO_InitStructure.GPIO_OType = GPIO_OType_PP;
    GPIO_Init(NTRX_SSN_PORT, &GPIO_InitStructure);

    GPIO_InitStructure.GPIO_Pin = NTRX_RST_PIN;
    GPIO_InitStructure.GPIO_Speed = GPIO_Speed_40MHz;
    GPIO_InitStructure.GPIO_Mode = GPIO_Mode_OUT;
    GPIO_InitStructure.GPIO_OType = GPIO_OType_PP;
    GPIO_Init(NTRX_RST_PORT, &GPIO_InitStructure);

    GPIO_InitStructure.GPIO_Pin = NTRX_DIO0_PIN | NTRX_DIO14_POWER;
    GPIO_InitStructure.GPIO_Speed = GPIO_Speed_40MHz;
    GPIO_InitStructure.GPIO_Mode = GPIO_Mode_OUT;
    GPIO_InitStructure.GPIO_OType = GPIO_OType_PP;
    GPIO_Init(NTRX_DIO_PORT, &GPIO_InitStructure);

    // PB1 interrupt
    GPIO_InitStructure.GPIO_Pin = GPIO_Pin_1;
    GPIO_InitStructure.GPIO_Mode = GPIO_Mode_IN;
    GPIO_InitStructure.GPIO_OType = GPIO_OType_OD;
    GPIO_Init(NTRX_DIO_PORT, &GPIO_InitStructure);

    NtrxWakeupDo();
    NtrxOpenPA();
    NtrxSSN_Lo();
}

