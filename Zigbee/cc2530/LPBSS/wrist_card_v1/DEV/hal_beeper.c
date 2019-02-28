#include "hal_beeper.h"

void HalBeepInit(void)
{
    P2DIR |= (0x01 << BEEPER_BIT);  //set to output
    PERCFG |= (0x01 << 4); // set timer4 location
    //BEEPER = 0;   //现在用波形控制的蜂鸣器可以去掉该行，若换用其他方式的蜂鸣器，要调用HalBeepStop()关闭
}

void HalBeepBegin(void)
{
    P2SEL |= (0x01 << BEEPER_BIT);  // set to peripheral

    T4CCTL0 = (0x00 << 6) | (0x02 << 3) | (0x01 << 2) ;
    T4CC0 = 0x5D;                   // 0x64~2.5k, 0x7D~2.0K, 5D/5C~2.7k
    T4CTL = (0x06 << 5) | (0x00 << 3) | (0x02 << 0);

    T4CTL &= ~(0x01 << 2); //clear
    T4CTL |= (0x01 << 4);  //start
}

void HalBeepStop(void)
{
    P2SEL &= ~(0x01 << BEEPER_BIT);
    T4CTL &= ~(0x01 << 4);  //stop
    BEEPER = 0;
}

