#include "board.h"
#include "printf_util.h"
#include "sleep.h"
#include "config.h"
#include "com_config.h"
#include "spi_nano.h"
#include "stm32l1xx.h"
#include "as3933.h"
#include "Instance.h"

void motion_detect_init(void)
{
    GPIO_InitTypeDef   GPIO_InitStructure;

    GPIO_InitStructure.GPIO_Pin = GPIO_Pin_1;
    GPIO_InitStructure.GPIO_Mode = GPIO_Mode_IN;
    GPIO_InitStructure.GPIO_Speed = GPIO_Speed_2MHz;
//    GPIO_InitStructure.GPIO_OType = GPIO_OType_PP;
    GPIO_InitStructure.GPIO_PuPd  = GPIO_PuPd_DOWN;
    GPIO_Init(GPIOB, &GPIO_InitStructure);

    /* Connect EXTI8 Line to PB1 pin */
    SYSCFG_EXTILineConfig(EXTI_PortSourceGPIOB, EXTI_PinSource1);

    MMA845x_Init();
}


void motion_detect_int_open(void)
{
    EXTI_InitTypeDef   EXTI_InitStructure;

    /* Clear the EXTI line 8 pending bit */
    EXTI_ClearITPendingBit(EXTI_Line1);
	
    /* Configure EXTI9 line */
    EXTI_InitStructure.EXTI_Line = EXTI_Line1;
    EXTI_InitStructure.EXTI_Mode = EXTI_Mode_Interrupt;
    EXTI_InitStructure.EXTI_Trigger = EXTI_Trigger_Rising;
    EXTI_InitStructure.EXTI_LineCmd = ENABLE;
    EXTI_Init(&EXTI_InitStructure);
}

void motion_detect_int_close(void)
{
    EXTI_InitTypeDef   EXTI_InitStructure;
    /* Configure EXTI8 line */
    EXTI_InitStructure.EXTI_Line = EXTI_Line1;
    EXTI_InitStructure.EXTI_Mode = EXTI_Mode_Interrupt;
    EXTI_InitStructure.EXTI_Trigger =EXTI_Trigger_Rising;// EXTI_Trigger_Rising;
    EXTI_InitStructure.EXTI_LineCmd = DISABLE;
    EXTI_Init(&EXTI_InitStructure);
}




void interrupt_init(void)
{
    GPIO_InitTypeDef GPIO_InitStructure;
    EXTI_InitTypeDef EXTI_InitStructure;
    NVIC_InitTypeDef NVIC_InitStructure;

    // Enable GPIO used as DECA IRQ for interrupt
    GPIO_InitStructure.GPIO_Pin = GPIO_Pin_0;
    GPIO_InitStructure.GPIO_Mode = GPIO_Mode_IN;
    GPIO_InitStructure.GPIO_PuPd = GPIO_PuPd_NOPULL;
    GPIO_Init(GPIOB, &GPIO_InitStructure);

    /* Connect EXTI Line to GPIO Pin */
    SYSCFG_EXTILineConfig(GPIO_PortSourceGPIOB, GPIO_PinSource0);

    /* Configure EXTI line */
    EXTI_InitStructure.EXTI_Line = EXTI_Line0;
    EXTI_InitStructure.EXTI_Mode = EXTI_Mode_Interrupt;
    EXTI_InitStructure.EXTI_Trigger = EXTI_Trigger_Rising;  //MPW3 IRQ polarity is high by default
    EXTI_InitStructure.EXTI_LineCmd = ENABLE;
    EXTI_Init(&EXTI_InitStructure);

    /* Set NVIC Grouping to 16 groups of interrupt without sub-grouping */
    #if 0
    NVIC_PriorityGroupConfig(NVIC_PriorityGroup_4);

    /* Enable and set EXTI Interrupt to the lowest priority */
    NVIC_InitStructure.NVIC_IRQChannel = EXTI0_IRQn;
    NVIC_InitStructure.NVIC_IRQChannelPreemptionPriority = 15;
    NVIC_InitStructure.NVIC_IRQChannelSubPriority = 0;
    NVIC_InitStructure.NVIC_IRQChannelCmd = ENABLE;

    NVIC_Init(&NVIC_InitStructure);

    /* Enable the RTC Interrupt */
    /*NVIC_InitStructure.NVIC_IRQChannel = RTC_IRQn;
    NVIC_InitStructure.NVIC_IRQChannelPreemptionPriority = 10;
    NVIC_InitStructure.NVIC_IRQChannelSubPriority = 0;
    NVIC_InitStructure.NVIC_IRQChannelCmd = ENABLE;

    NVIC_Init(&NVIC_InitStructure);*/
    #endif


}


void peripheral_int_open(void)
{
    NVIC_InitTypeDef   NVIC_InitStructure;
	 
    NVIC_InitStructure.NVIC_IRQChannelPreemptionPriority = 0x0F;
    NVIC_InitStructure.NVIC_IRQChannelSubPriority = 0x0F;
    NVIC_InitStructure.NVIC_IRQChannelCmd = ENABLE;

    /* Enable and set EXTI15 Interrupt to the 9 priority */
    NVIC_InitStructure.NVIC_IRQChannel = EXTI15_10_IRQn;
    NVIC_Init(&NVIC_InitStructure);
    /* Enable and set EXTI5 Interrupt to the 9 priority */
    NVIC_InitStructure.NVIC_IRQChannel = EXTI9_5_IRQn;
    NVIC_Init(&NVIC_InitStructure);

    NVIC_InitStructure.NVIC_IRQChannel = EXTI3_IRQn;
    NVIC_Init(&NVIC_InitStructure);

	 NVIC_InitStructure.NVIC_IRQChannelPreemptionPriority = 0x01;
    NVIC_InitStructure.NVIC_IRQChannelSubPriority = 0x01;
    NVIC_InitStructure.NVIC_IRQChannelCmd = ENABLE;
	NVIC_InitStructure.NVIC_IRQChannel = EXTI1_IRQn;
    NVIC_Init(&NVIC_InitStructure);
}

void board_init()
{
    chip_core_init();
    chip_interface_init();

    LED_Configuration();
	UART_Configuration();


#ifndef TDOA_INST_MODE_ANCHOR    //作为卡时需要开启，即非基站时开启
    MOTOR_Configuration();
    BUTTON_Configuration();
    ADC_Configuration();
    motion_detect_init();
	SPI2_Configuration();
	init_as3933();
#else
	led_station_init();
#endif

    peripheral_int_open();
    interrupt_init();
    SPI_Configuration();
    
   	(void)Dw1000Init();  //Dw1000模块初始化 Tdoa工程使用

    //WatchdogReset();
}

