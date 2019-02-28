#include <iocc2530.h>
#include "key.h"
#include "delay.h"
#include "WatchDogUtil.h"
#include "ioexpand.h"

/***************************
// Local macro define
****************************/
// Key-chip interface define
#define KEY_SCL         P1_3
#define KEY_SDA         P1_5

#define KEY_SETSDA_INPUT() (P1DIR &= ~BV(5))
#define KEY_SETSDA_OUTPUT() (P1DIR |= BV(5))
#define KEY_SDA_CLEARIFG() (P1IFG &= ~BV(5))
/***************************
// Local subroutine define
****************************/

// Initial BackLight
//void InitialBackLight(void);
/****************************************************************
*** Read key-scan chip
*****************************************************************/





/****************************************************************
*** Get key from key-scan chip
*****************************************************************/
/****************************************************************
*** Get key from key-scan chip
*****************************************************************/

uint8 GetKey(void)
{
    uint8 key_data;
    return  HAL_KEY_CALL;


    switch(key_data)
    {
    case(0x40):
        return  HAL_KEY_SELECT;     // KEY_SEG0_DIG0;
    case(0x41):
        return  HAL_KEY_UP;         // KEY_SEG0_DIG1;
    case(0x42):
        return  HAL_KEY_BACKSPACE;  // KEY_SEG0_DIG2;
    case(0x43):
        return  HAL_KEY_CALL;       // KEY_SEG0_DIG3;
    case(0x44):
        return  HAL_KEY_LEFT;       // KEY_SEG0_DIG4;
    case(0x45):
        return  HAL_KEY_RIGHT;      // KEY_SEG0_DIG5;
    case(0x46):
        return  HAL_KEY_POWER;      // KEY_SEG0_DIG6;
    case(0x47):
        return  HAL_KEY_DOWN;       // KEY_SEG0_DIG7;
    case(0x48):
        return  HAL_KEY_1;          // KEY_SEG1_DIG0;
    case(0x49):
        return  HAL_KEY_2;          // KEY_SEG1_DIG1;
    case(0x4a):
        return  HAL_KEY_3;          // KEY_SEG1_DIG2;
    case(0x4b):
        return  HAL_KEY_4;          // KEY_SEG1_DIG3;
    case(0x4c):
        return  HAL_KEY_5;          // KEY_SEG1_DIG4;
    case(0x4d):
        return  HAL_KEY_6;          // KEY_SEG1_DIG5;
    case(0x4e):
        return  HAL_KEY_7;          // KEY_SEG1_DIG6;
    case(0x4f):
        return  HAL_KEY_8;          // KEY_SEG1_DIG7;
    case(0x50):
        return  HAL_KEY_9;          // KEY_SEG2_DIG0;
    case(0x51):
        return  HAL_KEY_STAR;       // KEY_SEG2_DIG1;
    case(0x52):
        return  HAL_KEY_0;          // KEY_SEG2_DIG2;
    case(0x53):
        return  HAL_KEY_POUND;      // KEY_SEG2_DIG3;
    default:
        return  HAL_KEY_INVALID;
    }
}




/*******************************************************************************
// Initial Keyboard
*******************************************************************************/
/****************************************************************
*** Initial key-scan chip and key interrupt
*****************************************************************/
void InitialKey(void)
{

    // init P1.5 to interrupt, rising edge trigger
    P1SEL &= ~BV(7);            // set P1.7 to general io
    P1DIR &= ~BV(7);            // set P1.7 to input
    P1IFG &= ~BV(7);            // clear ifg of P1.7

    P1IEN |= BV(7);

    // init P1.3 to output, used as SCL
    P1SEL &= ~0x08;         // set P1.3 to general io
    P1DIR |= 0x08;          // set P1.3 to output


    P2SEL &= ~BV(0);     // P2_0 used as gpio
    P2DIR |= BV(0);      // P2_0 used as output
    P2_0 =0;

    P1SEL &=~BV(6);      //P1_6 used as uart interrupt
    P1DIR &=~BV(6);
    P1IFG &= ~BV(6);            // clear ifg of P1.6
    P1IEN |= BV(6);

    P0SEL &=~BV(6);      //P1_6 used as gpio
    P0DIR |=BV(6);
    P0_6 = 0;

    // init P1.5 to output, used as SDA
    //P1SEL &= ~BV(5);            // set P1.5 to general io
    //P1DIR |= BV(5);         // set P1.5 to output

    // init SCL and SDA to 1
    //KEY_SCL = 1;
    //KEY_SDA = 1;

    //KeyReset();
    // init key-scan chip
    //1, set key-scan chip parameters
    //WriteKeyChip(0x68, 0x22);   // 0x22);   // enable key-scan, INTM set to falling pulse
    //    CH452_Write(CH452_SYSON2);
    //2, put key-scan chip into sleep mode
    //WriteKeyChip(0x64, 0x02);

}

void InitialMisc(void)
{
    // shake controller   ioexpand p01
    //back light  P03

    uint8 p0dir, p1dir;
    ioexpand_getdir(&p0dir, &p1dir);
    p0dir &= ~(BV(1) | BV(3));                      //expand p01 and p03
    ioexpand_setdir(p0dir, p1dir);             // set dir to output

    backlight_ctrl(FALSE);
    shake_ctrl(FALSE);
}



void WaitKeySleep(uint16 TimeOut)
{
    uint16 testInterval = 100;
    uint16 testnum = TimeOut / testInterval;
    for(uint16 i = 0; i < testnum; i++)
    {
        // DelayMs(testInterval);
        uint8 key_tmp = GetKey();

#if(defined WATCHDOG) &&(WATCHDOG == TRUE)
        FeedWatchDog();
#endif
        if(key_tmp == HAL_KEY_INVALID)
            break;
    }
}
void KeyReset(void)
{
    //DelayMs(100);
    DelayUs(500);
}


/* enable key by time */
void KeyEnable(void)
{
    //KEY_INT_DISABLE();
    // init key-scan chip
    //1, set key-scan chip parameters
    //WriteKeyChip(0x68, 0x22);   // 0x22);   // enable key-scan, INTM set to falling pulse
    //    CH452_Write(CH452_SYSON2);
    //2, put key-scan chip into sleep mode
    //WriteKeyChip(0x64, 0x02);
    //KEY_INT_ENABLE();
}

void backlight_ctrl(bool enable)
{
    // backlight ctrl = p03
    uint8 p0, p1;
    ioexpand_read(&p0, &p1);
    if(enable)
    {
        p0 |= BV(3);                     //p03 enable ;
    }
    else
    {
        p0 &= ~BV(3);                     //p03 disable;
    }
    ioexpand_write(p0, p1);
}

void shake_ctrl(bool enable)
{
    // shake ctrl = p01
    uint8 p0, p1;
    ioexpand_read(&p0, &p1);
    if(enable)
    {
        p0 &= ~BV(1);                     //p01 enable ;
    }
    else
    {
        p0 |= BV(1);                     //p01 disable;
    }
    ioexpand_write(p0, p1);
}

