#include "timer.h"
#include <htc.h>
 

void start_Timer1(void)
{
//7:T1GINV  6:TMR1GE  5-4:TICKPS  3:T1OSCEN  2:T1SYNC  1:TMR1CS 0:TMR1ON
    T1CON = 0;
    T1CON |= BV(3)|BV(1)|BV(2);// 预分频系数为1，外部时钟  不同步外部时钟
    TMR1IE = 1;
    PEIE =1;
    GIE = 1;
    TMR1H = 0xee; 
    TMR1L = 0x22; 
    TMR1ON = 1;
}

void init_Timer0(void)
{
    OPTION = 0xC8;
    TMR0 = TIMER0FOOD;
    GIE  = 1;
}


void start_Timer0(void)
{
    T0IE = 1;

}

void stop_Timer0(void)
{
    T0IE = 0;

}

