/*******************************************************************************
  Filename:     hal_sleep.c
  Revised:      $Date: 18:21 2012年5月7日
  Revision:     $Revision: 1.0 $

  Description:  HAL radio interface header file

*******************************************************************************/

/*******************************************************************************
* INCLUDES
*/
#include <types.h>
#include <hal_mcu.h>
#include <hal_sleep.h>

/*******************************************************************************
* CONSTANTS AND DEFINES
*/

#define U32_OFFSET(U32VAL, OFFSET)  (UINT8)(((U32VAL) >> ((OFFSET) * 8)) & 0xFF)
#define HAL_SLEEP_20_SEC             20000

/* PM0, Clock oscillators on, voltage regulator on */
/* PM1, 32.768 kHz oscillators on, voltage regulator on */
/* PM2, 32.768 kHz oscillators on, voltage regulator off */
/* PM3, All clock oscillators off, voltage regulator off */
#define CC2530_PM0          0
#define CC2530_PM1          1
#define CC2530_PM2          2
#define CC2530_PM3          3

/* sleep and external interrupt port masks */
#define STIE_BV             BV(5)
#define P0IE_BV             BV(5)
#define P1IE_BV             BV(4)
#define P2IE_BV             BV(1)

#define UINT32_NDX0         0
#define UINT32_NDX1         1
#define UINT32_NDX2         2
#define UINT32_NDX3         3

/*******************************************************************************
* Macros FUNCTIONS
*/

#define HAL_SLEEP_MODE_SETUP(POWER_MODE)                \
    st(                                                 \
        SLEEPCMD &= ~3;         /* clear mode bits */   \
        SLEEPCMD |= POWER_MODE; /* set mode bits   */   \
        while (!(STLOAD & 1));                          \
    )

#define HAL_SLEEP_INT_CFG(ien0, ien1, ien2)             \
    st(                                                 \
        /* sleep timer interrupt control */             \
        /* clear sleep interrupt flag */                \
        STIF = 0;                                       \
        /* enable sleep timer interrupt */              \
        IEN0 |= STIE_BV;                                \
                                                        \
        /* backup interrupt enable registers before sleep */    \
        ien0  = IEN0;    /* backup IEN0 register */             \
        ien1  = IEN1;    /* backup IEN1 register */             \
        ien2  = IEN2;    /* backup IEN2 register */             \
        IEN0 &= STIE_BV; /* disable IEN0 except STIE */         \
        IEN1 &= P0IE_BV; /* disable IEN1 except P0IE */         \
        IEN2 &= (P1IE_BV|P2IE_BV); /* disable IEN2 except P1IE, P2IE */\
    )

#define HAL_SLEEP_INT_RESTORE(ien0, ien1, ien2)         \
    st(                                                 \
        /* restore interrupt enable registers */        \
        IEN0 = ien0;        /* restore IEN0 register */ \
        IEN1 = ien1;        /* restore IEN1 register */ \
        IEN2 = ien2;        /* restore IEN2 register */ \
        IEN0 &= ~STIE_BV;   /* disable sleep int */     \
    )

/* read the sleep timer; ST0 must be read first */
#define HAL_SLEEP_READ_SLEEP_TIMER(u32Value)            \
    st(                                                 \
        ((UINT8 *) &u32Value)[UINT32_NDX0] = ST0;       \
        ((UINT8 *) &u32Value)[UINT32_NDX1] = ST1;       \
        ((UINT8 *) &u32Value)[UINT32_NDX2] = ST2;       \
        ((UINT8 *) &u32Value)[UINT32_NDX3] = 0;         \
    )

/* set sleep timer compare; ST0 must be written last */
#define HAL_SLEEP_WRITE_SLEEP_TIMER(u32Value)           \
    st(                                                 \
        ST2 = ((UINT8 *) &u32Value)[UINT32_NDX2];       \
        ST1 = ((UINT8 *) &u32Value)[UINT32_NDX1];       \
        ST0 = ((UINT8 *) &u32Value)[UINT32_NDX0];       \
    )

/*******************************************************************************
* LOCAL VARIABLES
*/
static UINT32 s_u32PreSleepTimer;       // 记录上一次休眠的时间长度

/*******************************************************************************
* LOCAL FUNCTIONS
*/

/*******************************************************************************
* @fn          hal_sleep_ToGo
*
* @brief       开始休眠，一个指令周期后暂停寻址
*
*
* @param    none
*
* @return      none
*/
static VOID hal_sleep_ToGo(VOID)
{
    PCON = 1;
    asm("NOP");
}

/*******************************************************************************
* @fn          hal_sleep_SetTimer
*
* @brief       设置休眠定时器的值，以当前值为BASE，进行设置OFFSET时间
*
*
* @param    input - u32SleepTime 需要休眠的时间,单位MS
*
* @return     none
*/
static VOID hal_sleep_SetTimer(UINT32 u32SleepTime)
{
    UINT32 u32Ticks;

    HAL_SLEEP_READ_SLEEP_TIMER(u32Ticks);

    // save current sleep timer
    s_u32PreSleepTimer = u32Ticks;

    // set sleep time offset
    u32Ticks += u32SleepTime;

    HAL_SLEEP_WRITE_SLEEP_TIMER(u32Ticks);
}

/*******************************************************************************
* GLOBAL FUNCTIONS
*/

/*******************************************************************************
* @fn          HAL_SLEEP_Adjust
*
* @brief       校准休眠期间系统时钟.
*
*
* @param    none
*
* @return      休眠的时间
*/
UINT32 HAL_SLEEP_Adjust(VOID)
{
    UINT32 u32Ticks;

    HAL_SLEEP_READ_SLEEP_TIMER(u32Ticks);

    return (((u32Ticks - s_u32PreSleepTimer) & 0x00FFFFFF) * 125) >> 12;
}

/*******************************************************************************
* @fn          HAL_SLEEP_Enter
*
* @brief       进入休眠
*
*
* @param    input - u32SleepMs 休眠的时间长度，单位MS
*
* @return      none
*/
VOID HAL_SLEEP_Enter(UINT32 u32SleepMs)
{
    UINT8 ien0, ien1, ien2;
    UINT8 POWER_MODE;
    UINT32 u32SleepTime;

    if (u32SleepMs > HAL_SLEEP_20_SEC)
    {
        POWER_MODE = CC2530_PM3;
    }
    else
    {
        POWER_MODE = CC2530_PM2;
    }

    // 换算成32kHz的tick
    u32SleepTime = (u32SleepMs << 12) / 125;

    HAL_SLEEP_MODE_SETUP(POWER_MODE);           // 设置休眠模式

    hal_sleep_SetTimer(u32SleepTime);           // 设置休眠时间

    HAL_SLEEP_INT_CFG(ien0, ien1, ien2);        // 备份并关闭部分中断

    HAL_ENABLE_INTERRUPTS();
    hal_sleep_ToGo();                           // 开始休眠
    //HAL_CLOCK_STABLE();                         // 等待晶振稳定

    HAL_DISABLE_INTERRUPTS();

    HAL_SLEEP_INT_RESTORE(ien0, ien1, ien2);    // 回复休眠前的中断

    HAL_ENABLE_INTERRUPTS();
}

/*******************************************************************************
 * @fn          halSleepTimerIsr
 *
 * @brief       Sleep timer ISR.
 *
 * input parameters
 *
 * None.
 *
 * output parameters
 *
 * None.
 *
 * @return      None.
 */
HAL_ISR_FUNCTION(halSleepTimerIsr, ST_VECTOR)
{
  HAL_ENTER_ISR();

  STIF = 0;

  HAL_EXIT_ISR();
}
