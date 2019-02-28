/*******************************************************************************
  Filename:       hal_mcu.c
  Revised:        $Date: 18:20 2012��5��7��
  Revision:       $Revision: 1.0 $

  Description:    Declarations for the CC2530

*******************************************************************************/

/*******************************************************************************
* Includes
*/
#include <hal_mcu.h>
#include <defs.h>
#include <types.h>

/*******************************************************************************
* CONSTANTS AND DEFINES
*/
/* ----------- Cache Prefetch control ---------- */
#define PREFETCH_ENABLE()     st( FCTL = 0x08; )
#define PREFETCH_DISABLE()    st( FCTL = 0x04; )

/* SLEEPCMD bit definitions */
#define OSC_PD     BV(2)        /* Idle Osc: powered down=1 */

/* SLEEPSTA bit definitions */
#define XOSC_STB   BV(6)        /* XOSC: powered, stable=1 */
#define HFRC_STB   BV(5)        /* HFRCOSC: powered, stable=1 */

/* ADCCON1 */
#define RCTRL1                        BV(3)
#define RCTRL0                        BV(2)
#define RCTRL_BITS                    (RCTRL1 | RCTRL0)
#define RCTRL_CLOCK_LFSR              RCTRL0

/*******************************************************************************
* GLOBAL FUNCTIONS
*/
UINT16 HAL_MCU_Random(VOID)
{
  UINT16 random_word;

  /* clock the random generator to get a new random value */
  ADCCON1 = (ADCCON1 & ~RCTRL_BITS) | RCTRL_CLOCK_LFSR;

  /* read random word */
  random_word = (RNDH << 8);
  random_word += RNDL;

  /* return new randomized value from hardware */
  return(random_word);
}

/*******************************************************************************
* @fn          HAL_MCU_XOSC_Init
*
* @brief       оƬ��ʼ��
*
* @param       none
*
* @return      none
*/
VOID HAL_MCU_XOSC_Init(VOID)
{
    UINT16 i;

    /* turn on 16MHz RC and 32MHz XOSC */
    SLEEPCMD &= ~OSC_PD;

    /* wait for 32MHz XOSC stable */
    while (!(SLEEPSTA & XOSC_STB));

    /* chip bug workaround */
    asm("NOP");

    /* Require 63us delay for all revs */
    for (i=0; i<504; i++)
        asm("NOP");

    /* Select 32MHz XOSC and the source for 32K clock */
    CLKCONCMD = (CLKCONCMD_32MHZ | OSC_32KHZ);

    /* Wait for the change to be effective */
    // while (CLKCONSTA != (CLKCONCMD_32MHZ | OSC_32KHZ));
    HAL_CLOCK_STABLE();

    /* turn off 16MHz RC */
    SLEEPCMD |= OSC_PD;
    // �ر�У׼
    SLEEPCMD |= BV(7);
    /* Turn on cache prefetch mode */
    PREFETCH_ENABLE();

    /* set direction for GPIO outputs  */
    LED1_DIR |= LED1_BV;
    LED2_DIR |= LED2_BV;

#ifdef USE_CC2591
    /* Set PA/LNA HGM control P0_7 */
    P0DIR |= BV(7);
#endif



    P0SEL &=~(0x41);   //�����IO�ڳ�ʼ��
    P0DIR &=~(0x41);
    P0INP |= 0xC1;

    //P1SEL &=~(0xF2);   //������IO�ڳ�ʼ��
    //P1DIR &=~(0xF2);
    //P1INP |= 0xF2;

    P1SEL &= ~(0x40);    //������IO�ڳ�ʼ��
    P1DIR &= ~(0x40);
    P1INP |= (0x40);

    P1SEL &= ~(0x20);    //P1_5   OLED ����
    P1DIR |= 0x20;

    P1SEL &= ~(0x80);    //p1_7  ���
    P1DIR &= ~(0x80);
    P1INP |= (0x80);


    P1SEL &=~(0x0C);   //I2C
    P1DIR |=(0x0C);
    P1INP |= 0x0C;

    P1SEL &= 0xED;     //PA: P1.1,P1.4
    P1DIR |= 0x12;
    P1_1 = 0;
    P1_4 = 0;
}

/*******************************************************************************
* @fn          HAL_WATCHDOG_Feed
*
* @brief       ι������
*
* @param       none
*
* @return      none
*/
VOID HAL_WATCHDOG_Feed(VOID)
{
    WDCTL = (0xA0 | WDCTL & 0x0F);
    WDCTL = (0x50 | WDCTL & 0x0F);
}

/*******************************************************************************
* @fn          HAL_WATCHDOG_Start
*
* @brief       �������Ź���������ʱ1��
*
* @param       none
*
* @return      none
*/
VOID HAL_WATCHDOG_Start(UINT8 u8IntTime)
{
    WDCTL &= ~(0x01 << WDCTL_MODE_POS);         // set mode to watchdog mode
    WDCTL &= ~(0x03 << WDCTL_INT_POS);          // set time interval
    WDCTL |= (u8IntTime & 3) << WDCTL_INT_POS;
    WDCTL |= (0x01 << WDCTL_EN_POS);            // Enable
}

/*******************************************************************************
* @fn          HAL_WaitUs
*
* @brief       ΢���ʱ
*
* @param       input- u32Usec ��ʱ��ʱ�䣬��λ΢��
*
* @return      none
*/
#pragma optimize=none
VOID HAL_WaitUs(UINT32 u32Usec)
{
    u32Usec >>= 1;
    while (u32Usec--)
    {
        NOP();
        NOP();
        NOP();
        NOP();
        NOP();
        NOP();
        NOP();
        NOP();
        NOP();
        NOP();
        NOP();
        NOP();
        NOP();
        NOP();
        NOP();
        NOP();
        NOP();
    }
}
