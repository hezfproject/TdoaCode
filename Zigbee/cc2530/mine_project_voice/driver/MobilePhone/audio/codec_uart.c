
#include <ioCC2530.h>
#include "codec_uart.h"
#include "Comdef.h"
#include "hal_mcu.h"
//#include "delay.h"

#define CODEC_UART_TX       P2_0
//#define CODEC_UART_RX     P2_3

static uint8 codec_uart_dalay = 0;   //one bit delay time, in us

inline void Codec_Uart_DelayUs(uint8 us);

void Codec_Uart_Init(void)
{
    P2SEL &= ~BV(0);     // P2_0 used as gpio
    P2DIR |= BV(0);      // P2_0 used as output

    codec_uart_dalay = 104;    // baudrate=9.6k, one bit time=104us

    CODEC_UART_TX = 1;  //stop
    Codec_Uart_DelayUs(codec_uart_dalay);
}

uint8 Codec_Uart_Puts(uint8 *p, uint8 len)
{
    if(p==NULL)
    {
        return FAILURE;
    }

    CODEC_UART_TX = 1;  //stop bit
    Codec_Uart_DelayUs(codec_uart_dalay);

    for(uint8 i=0; i<len; i++)
    {
            halIntState_t  x;
            HAL_ENTER_CRITICAL_SECTION(x);

            uint8 dat = *(p+i);
            CODEC_UART_TX = 0;  //start bit
            Codec_Uart_DelayUs(codec_uart_dalay);
            for(uint8 j=0; j<8; j++)
            {
                CODEC_UART_TX = dat & 0x01;  //data bit 0-7, LSB
                Codec_Uart_DelayUs(codec_uart_dalay);
                dat >>=1;
            }
            CODEC_UART_TX = 1;  //stop bit;
            Codec_Uart_DelayUs(codec_uart_dalay);
            HAL_EXIT_CRITICAL_SECTION(x);
    }
    return SUCCESS;
}

void Codec_Uart_Close(void)
{
    CODEC_UART_TX = 0;  //stop bit
     Codec_Uart_DelayUs(codec_uart_dalay);
}

inline void Codec_Uart_DelayUs(uint8 us)
{
    /* use timer4 for precise delay */
    T4CTL = (0x05<<5)   // 1M Hz
                |(0x01<<4)   // start
                |(0x01<<2)   // clear
                |(0x00<<0);   // mode = 0

   while(T4CNT < us);
}

