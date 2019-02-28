#include "hal_capture.h"
#include "hal_led.h"

#define     _3_5ms  875     //4.85mS    3.5*250         //3.5ms
#define     _6_5ms  1625        //                      //6.5ms

#define     _0_6ms  150     //0.95mS                    //0.6ms
#define     _1_3ms  325                                 //1.3ms

#define     _2_35ms  588                                 //2.35ms

#define CLOSEPORT0INT(emPin) \
do{\
    P0IEN &= ~ BV(emPin);\
}while(0)

#define SETUPTIMER_1()\
do{\
    T1IE = 0;\
    T1CTL = 0x0D;\
    T1CNTH = 0;\
    T1CNTL = 0;\
}while(0)

#define STOPTIMER_1()\
do{\
    T1CTL &= ~3;\
}while(0)

void Port0Init(BITNUM emPin)
{
    P0SEL &= ~ BV(emPin); /* 选择为通用 */
    P0DIR &= ~ BV(emPin); /* 设置为输入 */
    P0INP &= ~ BV(emPin); /* 上拉/下拉*/
    P2INP &= ~ BV(5);     /* 上拉*/
    PICTL |= BV(0);       /* 下降沿触发中断*/
    P0IE = 1;             /* 使能端口中断*/
    P0IEN |= BV(emPin);   /* 使能引脚中断*/
    P0IFG &= ~ BV(emPin); /* 清除中断标志*/
}

typedef enum
{
    SPACE,
    START,
    ORIGINAL,
    VERIFY
} PASER_STATE;

volatile PASER_STATE emCurrState = SPACE;
volatile uint8 u8BitCnt;
volatile uint8 u8Original;
volatile uint8 u8Verify;

bool GetBit(uint16 u16ClkCnt, volatile uint8* u8Bits)
{
    if (u16ClkCnt > _0_6ms && u16ClkCnt <= _1_3ms)
    {
        u8BitCnt++;
        *u8Bits = *u8Bits << 1;
    }
    else if(u16ClkCnt > _1_3ms && u16ClkCnt < _2_35ms)
    {
        u8BitCnt++;
        *u8Bits = (*u8Bits << 1) | 1;
    }
    else
        return FALSE;
    return TRUE;
}

uint8 errorCnt = 0;

void halCaptureProc(void)
{
    uint8 u8ClkCntL = T1CNTL;
    uint8 u8ClkCntH = T1CNTH;
    uint16 u16ClkCnt = (u8ClkCntH << 8) | u8ClkCntL;

    T1CNTL = 0;

    if (PORTINTJUDGE(DATA_CAPTURE, DATA_CAPTURE_A))
    {
        switch (emCurrState)
        {
        case START:
            if (u16ClkCnt >= _3_5ms && u16ClkCnt <= _6_5ms)
            {
                emCurrState = ORIGINAL;
                u8BitCnt = 0;
                u8Original = 0;
                u8Verify = 0;
                return;
            }

            if (errorCnt++ < 60)
                return;
            errorCnt = 0;
            break;
        case ORIGINAL:
            if (GetBit(u16ClkCnt, &u8Original))
            {
                if (8 == u8BitCnt)
                {
                    emCurrState = VERIFY;
                    //u8BitCnt = 0;
                    u8Original = ~u8Original;
                }
                return;
            }
            break;
        case VERIFY:
            if (GetBit(u16ClkCnt, &u8Verify))
            {
                if (16 == u8BitCnt)
                {
                    uint8 u8v = u8Verify, u8o = u8Original;
                    if (u8v == u8o)
                        HalLedBlink(HAL_LED_2, 4, 60, 500);
                }
                else
                    return;
            }
        }
    }
    else if (PORTINTJUDGE(DATA_CAPTURE, DATA_CAPTURE_B))
    {
        if (SPACE == emCurrState)
        {
            emCurrState = START;

            CLOSEPORT0INT(DATA_CAPTURE_B);
            Port0Init(DATA_CAPTURE_A);

            SETUPTIMER_1();
            return;
        }
    }

    CLOSEPORT0INT(DATA_CAPTURE_A);
    Port0Init(DATA_CAPTURE_B);
    STOPTIMER_1();
    emCurrState = SPACE;
}

bool halCaptureInit(void)
{
    Port0Init(DATA_CAPTURE_B);
    RegisterP012IntProc(PORT0, BIT4, halCaptureProc);
    return RegisterP012IntProc(PORT0, BIT5, halCaptureProc);
}

