#include "hal_beeper.h"

void HalBeepInit(void)
{
    P2DIR |= (0x01 << BEEPER_BIT);  //set to output
    PERCFG |= (0x01 << 4); // set timer4 location
    BEEPER = 0;
}

void HalBeepBegin(void)
{
    P2SEL |= (0x01 << BEEPER_BIT);  // set to peripheral

    T4CCTL0 = (0x00 << 6) | (0x02 << 3) | (0x01 << 2) ;
    T4CC0 = 0x5D;
    T4CTL = (0x06 << 5) | (0x00 << 3) | (0x02 << 0);

    T4CTL &= ~(0x01 << 2); //clear
    T4CTL |= (0x01 << 4);  //start
}

void HalBeepStop(void)
{
    P2SEL &= ~(0x01 << BEEPER_BIT);
    BEEPER = 0;
}

