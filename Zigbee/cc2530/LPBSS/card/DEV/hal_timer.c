/*******************************************************************************
  Filename:       hal_timer.c
  Revised:        $Date: 18:32 2012��5��7��
  Revision:       $Revision: 1.0 $

  Description:    ��ʱ��2ģ���ļ�

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
#define TIMER_1MS       (1000)
#define TIMER2_PERIOD   ((HAL_CPU_CLOCK_MHZ) * (TIMER_1MS))

#define TIMER2_LBYTE    (0xFF - (TIMER2_PERIOD & 0xFF))
#define TIMER2_HBYTE    (0xFF - (TIMER2_PERIOD >> 8))

/* T2CTRL */
#define LATCH_MODE            BV(3)
#define TIMER2_STATE          BV(2)
#define TIMER2_SYNC           BV(1)
#define TIMER2_RUN            BV(0)
#define TIMER2_INT_PER        BV(0)

/*******************************************************************************
* LOCAL VARIABLES
*/
static TIMER_CALLBACK_PFN s_pfnISR_T2;

/*******************************************************************************
* LOCAL FUNCTIONS DECLARATION
*/

/*******************************************************************************
* @fn      hal_timer2_Init
*
* @brief   ��ʼ��T2��ʱ��ÿ1msһ��TICK
*
* @param   none
*
* @return  none
*/
static VOID hal_timer2_Init(VOID)
{
    T2MSEL = 2;                         // period counter

    T2M0 = TIMER2_LBYTE;    // counter 1ms
    T2M1 = TIMER2_HBYTE;

    /* start timer */
    HAL_CLOCK_STABLE();
    T2CTRL |=  TIMER2_RUN;  //д1������ʱ��
    while(!(T2CTRL & TIMER2_STATE));

    /* Enable latch mode */
    //��ʱ������ʱ��������ʱ�������������������ʹ���Զ�T2M1/T2MOVF0/T2MOVF1/T2MOVF2
    T2CTRL |= LATCH_MODE;   
}

/*******************************************************************************
* @fn      HAL_TIMER2_Start
*
* @brief   ����T2��ʱ��ÿ1msһ��TICK
*
* @param   input - pfnT2 ��ʱ���жϴ���ص�����
*
* @return  none
*/
VOID HAL_TIMER2_Start(TIMER_CALLBACK_PFN pfnT2)
{
    s_pfnISR_T2 = pfnT2;

    hal_timer2_Init();

    /* enable timer overflow interrupt *///Ӧ����ʹ�������жϣ�����������ж�
    T2IRQM |= TIMER2_INT_PER;

    /* enable timer interrupts */
    T2IE = 1;//�ж�����λ
}

/*******************************************************************************
* @fn          McuTimer2Isr
*
* @brief       Interrupt service routine for timer2, the OS timer.
*
* @param       none
*
* @return      none
*/
HAL_ISR_FUNCTION(Timer2Isr, T2_VECTOR)
{
    HAL_ENTER_ISR();

    if (T2IRQF & TIMER2_INT_PER)
    {
        if (s_pfnISR_T2)
            (*s_pfnISR_T2)();
        /* clear the interrupt flag */
        T2IRQF &= ~TIMER2_INT_PER;
    }

    T2M0 = TIMER2_LBYTE;    // counter 1ms
    T2M1 = TIMER2_HBYTE;

    HAL_EXIT_ISR();
}
