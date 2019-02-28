#include "stm32l1xx.h"
#include <stdio.h>
#include "stm32l1xx_usart.h"
#include "config.h"
#include "Instance.h"

#if 0
void UART_RCC_Configuration(void)
{
    // Enable GPIOA and USART1 clocks
    RCC_AHBPeriphClockCmd(RCC_AHBPeriph_GPIOA, ENABLE);
    RCC_APB2PeriphClockCmd(RCC_APB2Periph_USART1, ENABLE);
}
#endif

void UART_Configuration(void)
{
    NVIC_InitTypeDef  NVIC_InitStructure ;
    GPIO_InitTypeDef  GPIO_InitStructure ;
    USART_InitTypeDef USART_InitStructure ;

    /* Connect PXx to USARTx_Tx */
    GPIO_PinAFConfig(GPIOA, GPIO_PinSource2, GPIO_AF_USART2);
    /* Connect PXx to USARTx_Rx */
    GPIO_PinAFConfig(GPIOA, GPIO_PinSource3, GPIO_AF_USART2);

    // USART2 Interrupt
    NVIC_InitStructure.NVIC_IRQChannel = USART2_IRQn;
    NVIC_InitStructure.NVIC_IRQChannelPreemptionPriority = 1;
    NVIC_InitStructure.NVIC_IRQChannelSubPriority = 0;
    NVIC_InitStructure.NVIC_IRQChannelCmd = ENABLE;
    NVIC_Init(&NVIC_InitStructure);

    // Configure USART1 Rx (PA.10) as input floating
    GPIO_InitStructure.GPIO_Pin = GPIO_Pin_3;
    GPIO_InitStructure.GPIO_Speed = GPIO_Speed_40MHz;
    GPIO_InitStructure.GPIO_Mode = GPIO_Mode_IN;
    GPIO_InitStructure.GPIO_PuPd = GPIO_PuPd_NOPULL;
    GPIO_Init(GPIOA, &GPIO_InitStructure);

    // Configure USART1 Tx (PA.09) as alternate function push-pull
    GPIO_InitStructure.GPIO_Pin = GPIO_Pin_2;
    GPIO_InitStructure.GPIO_Speed = GPIO_Speed_40MHz;
    GPIO_InitStructure.GPIO_Mode = GPIO_Mode_AF;
    GPIO_InitStructure.GPIO_OType = GPIO_OType_PP;
    GPIO_Init(GPIOA, &GPIO_InitStructure);

    // USART1 Configuration
    USART_InitStructure.USART_BaudRate = 115200; //CON_UART_BAUDRATE ;
    USART_InitStructure.USART_StopBits = USART_StopBits_1;
    USART_InitStructure.USART_Parity = USART_Parity_No;
    USART_InitStructure.USART_WordLength = USART_WordLength_8b;
    USART_InitStructure.USART_HardwareFlowControl = USART_HardwareFlowControl_None;
    USART_InitStructure.USART_Mode = USART_Mode_Rx | USART_Mode_Tx;
    USART_Init(USART2, &USART_InitStructure);

    // Enable the UART Receive interrupt
#if UART_BUFFER_SIZE > 0
    USART_ITConfig(USART2, USART_IT_RXNE, ENABLE);
#endif


    // Enable UART
    USART_Cmd(USART2, ENABLE);

    USART_ClearFlag(USART2,USART_FLAG_TC);

#ifdef  TDOA_INST_MODE_ANCHOR   //只允许基站进行发送
//#if 0
    /* Connect PXx to USARTx_Tx */
    RCC_APB2PeriphClockCmd(RCC_APB2Periph_USART1, ENABLE);
    GPIO_PinAFConfig(GPIOA, GPIO_PinSource9, GPIO_AF_USART1);
    /* Connect PXx to USARTx_Rx */
    GPIO_PinAFConfig(GPIOA, GPIO_PinSource10, GPIO_AF_USART1);

    // USART1 Interrupt
    NVIC_InitStructure.NVIC_IRQChannel = USART1_IRQn;
    NVIC_InitStructure.NVIC_IRQChannelPreemptionPriority = 1;
    NVIC_InitStructure.NVIC_IRQChannelSubPriority = 0;
    NVIC_InitStructure.NVIC_IRQChannelCmd = ENABLE;
    NVIC_Init(&NVIC_InitStructure);

    // Configure USART1 Rx (PA.10) as input floating
    GPIO_InitStructure.GPIO_Pin = GPIO_Pin_10;
    GPIO_InitStructure.GPIO_Speed = GPIO_Speed_40MHz;
    GPIO_InitStructure.GPIO_Mode = GPIO_Mode_AF;
    GPIO_InitStructure.GPIO_PuPd = GPIO_PuPd_NOPULL;
    GPIO_Init(GPIOA, &GPIO_InitStructure);

    // Configure USART1 Tx (PA.09) as alternate function push-pull
    GPIO_InitStructure.GPIO_Pin = GPIO_Pin_9;
    GPIO_InitStructure.GPIO_Speed = GPIO_Speed_40MHz;
    GPIO_InitStructure.GPIO_Mode = GPIO_Mode_AF;
    GPIO_InitStructure.GPIO_OType = GPIO_OType_PP;
    GPIO_InitStructure.GPIO_PuPd = GPIO_PuPd_UP;
    GPIO_Init(GPIOA, &GPIO_InitStructure);

    // USART1 Configuration
    USART_InitStructure.USART_BaudRate = 460800; //CON_UART_BAUDRATE ;
    USART_InitStructure.USART_StopBits = USART_StopBits_1;
    USART_InitStructure.USART_Parity = USART_Parity_No;
    USART_InitStructure.USART_WordLength = USART_WordLength_8b;
    USART_InitStructure.USART_HardwareFlowControl = USART_HardwareFlowControl_None;
    USART_InitStructure.USART_Mode = USART_Mode_Rx | USART_Mode_Tx;
    USART_Init(USART1, &USART_InitStructure);

    // Enable the UART Receive interrupt
//#if UART1_BUFFER_SIZE > 0
    USART_ITConfig(USART1, USART_IT_RXNE, ENABLE);
//#endif

    // Enable UART
    USART_Cmd(USART1, ENABLE);

    USART_ClearFlag(USART1,USART_FLAG_TC);
    USART_ClearFlag(USART1,USART_FLAG_RXNE);
#endif
}

#ifdef __GNUC__
/* With GCC/RAISONANCE, small printf (option LD Linker->Libraries->Small printf
   set to 'Yes') calls __io_putchar() */
#define PUTCHAR_PROTOTYPE int __io_putchar(int ch)
#else
#define PUTCHAR_PROTOTYPE int fputc(int ch, FILE *f)
#endif /* __GNUC__ */

/**
  * @brief  Retargets the C library printf function to the USART.
  * @param  None
  * @retval None
  */
PUTCHAR_PROTOTYPE
{
    /* Place your implementation of fputc here */
    /* e.g. write a character to the USART */
    //  USART_SendData(EVAL_COM2, (uint8_t) ch);
    //
    //  /* Loop until the end of transmission */
    //  while (USART_GetFlagStatus(EVAL_COM1, USART_FLAG_TC) == RESET)
    //  {}
    USART_SendData(USART2, (uint8_t)ch);
    while (USART_GetFlagStatus(USART2, USART_FLAG_TC) == RESET);

    if (ch == '\n')
    {
        USART_SendData(USART2, ('\r'));
        while (USART_GetFlagStatus(USART2, USART_FLAG_TC) == RESET);
    }

    return ch;
}
