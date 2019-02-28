#include "stm32l1xx.h"
#include "led.h"
#include "Ntrxdrv.h"
#include "nanotron.h"
#include "mcu.h"
#include "config.h"
#include "Instance.h"

#ifndef LED_FLASH_ON_TIME
#define LED_FLASH_ON_TIME     20
#endif

void LED_Configuration(void)
{
    GPIO_InitTypeDef  GPIO_InitStructure;

    //RCC_APB2PeriphClockCmd(RCC_AHBPeriph_GPIOA|RCC_AHBPeriph_GPIOB|RCC_AHBPeriph_GPIOC, ENABLE);    //??PC,D,G????
    RCC_AHBPeriphClockCmd(RCC_AHBPeriph_GPIOA|RCC_AHBPeriph_GPIOB|RCC_AHBPeriph_GPIOC, ENABLE);

    GPIO_InitStructure.GPIO_Pin = GPIO_Pin_4;   //PD3,PD6????
    GPIO_InitStructure.GPIO_Mode = GPIO_Mode_OUT;         //????
    GPIO_InitStructure.GPIO_Speed = GPIO_Speed_40MHz;//??50MHz
    GPIO_Init(GPIOA, &GPIO_InitStructure);    //???GPIOD3,6
    GPIO_ResetBits(GPIOA,GPIO_Pin_4);    //PD3,PD6 ???

    GPIO_SetBits(GPIOA,GPIO_Pin_4);


    GPIO_InitStructure.GPIO_Pin = GPIO_Pin_12;   //PD3,PD6????
    GPIO_InitStructure.GPIO_Mode = GPIO_Mode_OUT;         //????
    GPIO_InitStructure.GPIO_Speed = GPIO_Speed_40MHz;//??50MHz
    GPIO_Init(GPIOA, &GPIO_InitStructure);    //???GPIOD3,6
    GPIO_ResetBits(GPIOA,GPIO_Pin_12);    //PD3,PD6 ???

    GPIO_InitStructure.GPIO_Pin = GPIO_Pin_15;   //PD3,PD6????
    GPIO_InitStructure.GPIO_Mode = GPIO_Mode_OUT;         //????
    GPIO_InitStructure.GPIO_Speed = GPIO_Speed_40MHz;//??50MHz
    GPIO_Init(GPIOA, &GPIO_InitStructure);    //???GPIOD3,6
    GPIO_ResetBits(GPIOA,GPIO_Pin_15);    //PD3,PD6 ???
        
    GPIO_InitStructure.GPIO_Pin = GPIO_Pin_10;   //
    GPIO_InitStructure.GPIO_Mode = GPIO_Mode_OUT;         //
    GPIO_InitStructure.GPIO_Speed = GPIO_Speed_40MHz;//
    GPIO_Init(GPIOB, &GPIO_InitStructure);    //
    GPIO_ResetBits(GPIOB,GPIO_Pin_10);    //
    GPIO_SetBits(GPIOB,GPIO_Pin_10);

#ifndef TDOA_INST_MODE_ANCHOR  //作为卡的时候需要开启，即非基站时开启
#if 1
    //PC13 PA enable
    GPIO_InitStructure.GPIO_Pin = GPIO_Pin_13;   //PD3,PD6????
    GPIO_InitStructure.GPIO_Mode = GPIO_Mode_OUT;         //????
    GPIO_InitStructure.GPIO_Speed = GPIO_Speed_40MHz;//??50MHz
    GPIO_Init(GPIOC, &GPIO_InitStructure);    //???GPIOD3,6
    GPIO_SetBits(GPIOC,GPIO_Pin_13);    //PD3,PD6 ???
#endif

	//dw1000_tcxo_en
	 GPIO_InitStructure.GPIO_Pin = GPIO_Pin_9;   //PD3,PD6????
    GPIO_InitStructure.GPIO_Mode = GPIO_Mode_OUT;         //????
    GPIO_InitStructure.GPIO_Speed = GPIO_Speed_40MHz;//??50MHz
    GPIO_Init(GPIOB, &GPIO_InitStructure);    //???GPIOD3,6
    GPIO_ResetBits(GPIOB,GPIO_Pin_9);    //PD3,PD6 ???
#endif
}


void led_station_init(void)
{
    GPIO_InitTypeDef   GPIO_InitStructure;

	GPIO_InitStructure.GPIO_Pin = GPIO_Pin_12;   
    GPIO_InitStructure.GPIO_Mode = GPIO_Mode_OUT;     
    GPIO_InitStructure.GPIO_Speed = GPIO_Speed_40MHz;
    GPIO_Init(GPIOA, &GPIO_InitStructure);
    GPIO_SetBits(GPIOA,GPIO_Pin_12);
}

void led_station_on(void)
{
	 GPIO_ResetBits(GPIOA,GPIO_Pin_12);    //PD3,PD6 ???
}

void led_station_off(void)
{
	GPIO_SetBits(GPIOA,GPIO_Pin_12);
}


/*******************************************************************************
* LED红色闪烁接口
*/
void LED_Red_Flash(void)
{
    LED_Red_On();

    Delay_ms(LED_FLASH_ON_TIME);
    //WatchdogReset();

    LED_Red_Off();
}

/*******************************************************************************
* LED绿色闪烁接口
*/
void LED_Green_Flash(void)
{
    LED_Green_On();

    Delay_ms(LED_FLASH_ON_TIME);
    //WatchdogReset();

    LED_Green_Off();
}

