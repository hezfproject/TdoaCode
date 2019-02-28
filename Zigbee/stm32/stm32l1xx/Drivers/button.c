#include "stm32l1xx.h"
#include "cc_def.h"
#include "CommonTypes.h"
#include "timer_event.h"
#include "button.h"

#define BTN_HELP_PIN            GPIO_Pin_3
#define BTN_HELP_GPIO           GPIOB
#define BTN_HELP_EXTI_PIN       EXTI_PinSource3
#define BTN_HELP_EXTI_GPIO      EXTI_PortSourceGPIOB
#define BTN_HELP_EXTI_LINE      EXTI_Line3
#define BTN_HELP_EXTI_IRQ       EXTI9_5_IRQnEXTI3_IRQn

#define BTN_CFRM_PIN            GPIO_Pin_6
#define BTN_CFRM_GPIO           GPIOB
#define BTN_CFRM_EXTI_PIN       EXTI_PinSource6
#define BTN_CFRM_EXTI_GPIO      EXTI_PortSourceGPIOB
#define BTN_CFRM_EXTI_LINE      EXTI_Line6
#define BTN_CFRM_EXTI_IRQ       EXTI9_5_IRQn

void BUTTON_Configuration(void)
{
    GPIO_InitTypeDef   GPIO_InitStructure;
    EXTI_InitTypeDef   EXTI_InitStructure;

    /* Configure PB15 pin as input floating */
    GPIO_InitStructure.GPIO_Mode = GPIO_Mode_IN;
    GPIO_InitStructure.GPIO_PuPd = GPIO_PuPd_NOPULL;
    GPIO_InitStructure.GPIO_Pin = BTN_HELP_PIN;
    GPIO_Init(BTN_HELP_GPIO, &GPIO_InitStructure);
    /* Configure PB5 pin as input floating */
    GPIO_InitStructure.GPIO_Pin = BTN_CFRM_PIN;
    GPIO_Init(BTN_CFRM_GPIO, &GPIO_InitStructure);

    /* Connect EXTI15 Line to PB3 pin */
    SYSCFG_EXTILineConfig(BTN_HELP_EXTI_GPIO, BTN_HELP_EXTI_PIN);
    /* Connect EXTI5 Line to PB6 pin */
    SYSCFG_EXTILineConfig(BTN_CFRM_EXTI_GPIO, BTN_CFRM_EXTI_PIN);

    EXTI_InitStructure.EXTI_Mode = EXTI_Mode_Interrupt;
    EXTI_InitStructure.EXTI_Trigger = EXTI_Trigger_Rising;
    EXTI_InitStructure.EXTI_LineCmd = ENABLE;
    /* Configure EXTI3 line */
    EXTI_InitStructure.EXTI_Line = BTN_HELP_EXTI_LINE;
    EXTI_Init(&EXTI_InitStructure);
    /* Configure EXT6 line */
    EXTI_InitStructure.EXTI_Line = BTN_CFRM_EXTI_LINE;
    EXTI_Init(&EXTI_InitStructure);
}

uint_8 BUTTON_KeyCode_Read(void)
{
    uint_8 key_code = 0;

   	if (GPIO_ReadInputDataBit(BTN_CFRM_GPIO, BTN_CFRM_PIN))
    {
		key_code |= BTN_CFRM;  //0x2
    }
    else if (GPIO_ReadInputDataBit(BTN_HELP_GPIO, BTN_HELP_PIN))
    {
		key_code |= BTN_HELP;
    }


    return key_code;
}

