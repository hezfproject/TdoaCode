/*******************************************************************************
  Filename:       hal_timer.c
  Revised:        $Date: 18:32 2012年5月7日
  Revision:       $Revision: 1.0 $

  Description:    定时器2模块文件

*******************************************************************************/

/*******************************************************************************
* INCLUDES
*/
#include <types.h>
#include <hal_mcu.h>
#include <hal_timer.h>

/*******************************************************************************
* CONSTANTS
*/

#define HAL_PWM_PORT_T     P2
#define HAL_PWM_BIT        BV(0)
#define HAL_PWM_SEL        P2SEL 
#define HAL_PWM_DIR        P2DIR 

#define HAL_PWM_PERCFG     PERCFG
#define HAL_PWM_PCFG_BIT   BV(4) //select timer4

/*******************************************************************************
* LOCAL VARIABLES
*/
static TIMER_CALLBACK_PFN s_pfnISR_T4;
static unsigned long volatile happened_count = 0;

/*******************************************************************************
* LOCAL FUNCTIONS DECLARATION
*/

/*******************************************************************************
* @fn      hal_timer4_Init
* @brief   初始化T4定时器，每次产生两个周期的方波
* @param   none
* @return  none
*/
static VOID hal_timer4_Init(VOID)
{
    //port init
    HAL_PWM_SEL |= HAL_PWM_BIT; //set P2_0 is peripheral
    HAL_PWM_DIR |= HAL_PWM_BIT; //set P2_0 is output

    //percfg init
    HAL_PWM_PERCFG |= HAL_PWM_PCFG_BIT; //select timer4, select place 2
    HAL_PWM_SEL    |= 0x08;             //0000 1000， select timer4, 

    //timer init
    T4CC0   = 0x64;  //count value
    T4CCTL0 = 0x34;  //compare mode

    T4CTL   = 0x43;  //start & stop set, and 4fp, count mode

    T4IE    = 1;     //timer 4 interrupt enable
}

/*******************************************************************************
* @fn      HAL_Timer4_Start
* @brief   启动T4定时器每次产生两个方波
* @param   input - pfnT4器中断处理回调函数
* @return  none
*/
VOID HAL_Timer4_Start(TIMER_CALLBACK_PFN pfnT4)
{
    s_pfnISR_T4 = pfnT4;

    hal_timer4_Init();

    T4CTL |= BV(2); //set CLR = 1
    T4CTL |= BV(3); //set overflow interrupt
    T4CTL |= BV(4); //set START = 1;

    happened_count = 0;
}

VOID hal_timer4_interrupt(VOID)
{
    happened_count++;
    if (1 == happened_count) {
        T4CTL &= ~BV(2); //clear CLR
        T4CTL &= ~BV(4); //clear START;
        happened_count = 0;
    }
}

VOID HAL_Timer4_Stop(VOID)
{
    T4CTL &= ~BV(2); //clear CLR
    T4CTL &= ~BV(4); //clear START;
    happened_count = 0;
    T4CTL |= BV(4); //set START = 1;
}

/*******************************************************************************
* @fn          McuTimer4Isr
* @brief       Interrupt service routine for timer4, the OS timer.
* @param       none
* @return      none
*/
HAL_ISR_FUNCTION(Timer4Isr, T4_VECTOR)
{
    if ( (TIMIF & BV(3)) )
    {
        if (s_pfnISR_T4)
            (*s_pfnISR_T4)();
        // clear the interrupt flag
        TIMIF &= ~BV(3);
    }
}




