#include <ioCC2430.h>
#include "bsmac_timer.h"

/*******************************************************************************
** Local macro and variable
*******************************************************************************/
// Timer1 operation define
#define TIMER1_RESET            do {T1CNTL = 0x00;} while (0)
#define TIMER1_START            do {T1CTL |= 0x02;} while (0)
#define TIMER1_STOP             do {T1CTL &= 0xFC;} while (0)
#define TIMER1_INT_ENABLE       do {IEN1 |= 0X02;} while (0)
#define TIMER1_INT_CLEAR        do {IRCON &= 0XFD;} while (0)
#define TIMER1_CH0_INT_ENABLE   do {T1CCTL0 |= 0x40;} while (0)
#define TIMER1_CH0_INT_DISABLE  do {T1CCTL0 &= ~0x40;} while (0)
#define TIMER1_CH1_INT_ENABLE   do {T1CCTL1 |= 0x40;} while (0)
#define TIMER1_CH1_INT_DISABLE  do {T1CCTL1 &= ~0x40;} while (0)
#define TIMER1_CH0_INT_CLEAR    do {T1CTL &= ~0x20;} while (0)
#define TIMER1_CH1_INT_CLEAR    do {T1CTL &= ~0x40;} while (0)

static bsmac_timer_cb tcb = 0;
static unsigned char counter_dmarx=0;
static unsigned char counter_live=0;
static unsigned char enable_listen = 0;

/*******************************************************************************
** Description:
    initial mac timer(here use time 1)
*******************************************************************************/
unsigned char bsmac_timer_init(bsmac_timer_cb cb)
{
    // get callback handle
    if (cb == 0) {
        return BSMAC_TIMER_INIT_CB_INVALID;
    }
    tcb = cb;
    
    // init timer1
    PERCFG |= 0x40;         	// Timer1 use alt2
    CLKCON |= 0x38;         	// use 250k for timer ticket    
    T1CTL = 0x04;           	// Tick freq/4 = 32k

    T1CCTL0 |= 0x04;            // ch0 in compare mode  
    T1CC0L = 0x00;
    T1CC0H = 0x20;                 // 256ms

    T1CCTL1 |= 0x04;            // ch1 in compare mode      
    T1CC1L = 0x00;
    T1CC1H = 0x80;                 // expire time = 8*256*0.5ms = 1024ms
    
    // init timer1 interrupt    
    TIMER1_INT_ENABLE;          // timer1 interrupt enable    
    TIMER1_CH0_INT_ENABLE;      // channel 0 int enable(for keep live and link maintain)
    TIMER1_CH1_INT_DISABLE;     // channel 1 int disable(for rx dma hang detection)
       
    // start timer 1
    TIMER1_START;
    
    return BSMAC_TIMER_INIT_SUCCESS;
}

/*******************************************************************************
** Description:
    restart mac timer
*******************************************************************************/
void bsmac_timer_restart(void)
{
    TIMER1_STOP;
    TIMER1_RESET;
    TIMER1_START;
}

/*******************************************************************************
** Description:
    start listen rx dma to detect whether it's hanging
*******************************************************************************/
void bsmac_timer_start_listen_rxdma(void)
{
    //TIMER1_CH1_INT_ENABLE;
    enable_listen = 1;
}

/*******************************************************************************
** Description:
    stop listen rx dma
*******************************************************************************/
void bsmac_timer_stop_listen_rxdma(void)
{
    //TIMER1_CH1_INT_DISABLE;
    enable_listen = 0;
    counter_dmarx = 0;
}

void bsmac_timer_reset_live(void)
{
    counter_live = 0;
}


/*******************************************************************************
** Description:
    timer 1 interrupt handler
*******************************************************************************/
#pragma vector = T1_VECTOR
 __near_func __interrupt void  T1_ISR(void)
{
    unsigned char rs = 0;
    
    //TIMER1_STOP;                  
    TIMER1_INT_CLEAR;      
//    TIMER1_START;
    counter_live++;
    rs = T1CTL;
    if ((rs & 0x20) != 0) { // channel 0 int
        TIMER1_CH0_INT_CLEAR;
        
        if(counter_live>1)
            tcb(BSMAC_TIMER_CH0_INT);

        if(enable_listen) counter_dmarx++;
        if(counter_dmarx > 4) 
        {
            tcb(BSMAC_TIMER_CH1_INT);
            bsmac_timer_stop_listen_rxdma();
        }
        
    } else if ((rs & 0x40) != 0) {  // channel 1 int
        TIMER1_CH1_INT_CLEAR;
        tcb(BSMAC_TIMER_CH1_INT);
    }
    
    //TIMER1_START;
}    




