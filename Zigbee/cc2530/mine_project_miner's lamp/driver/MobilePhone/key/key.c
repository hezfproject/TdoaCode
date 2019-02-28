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
void ReadKeyChip(uint8 cmd, uint8 *pdata);
void WriteKeyChip(uint8 cmd, uint8 data);

// Initial BackLight
//void InitialBackLight(void);
/****************************************************************
*** Read key-scan chip
*****************************************************************/

void ReadKeyChip(uint8 cmd, uint8 *pdata)
{
    return;
    int8 i = 0;
    uint8 data = 0;
    uint8 intState;
    intState = EA;
    EA = 0;

    // Change SDA(P1.5) back to output
    KEY_SETSDA_OUTPUT();

    // disable interrupt on sda(p1.5)
    KEY_INT_DISABLE();

    DelayUs(5);

    KEY_SDA = 1;
    KEY_SCL = 1;
    DelayUs(5);
    KEY_SDA = 0;    // send start signal
    DelayUs(5);
    KEY_SCL = 0;
    DelayUs(5);

    for(i = 7; i >= 0; i--)
    {
        KEY_SDA = (cmd >> i) & 1;   // send command
        DelayUs(5);
        KEY_SCL = 1;
        DelayUs(5);
        KEY_SCL = 0;
        DelayUs(5);
    }

    // Change SDA(P1.5) to input
    KEY_SETSDA_INPUT();

    KEY_SDA = 1;    // send "1", wait acknoledge
    DelayUs(5);
    KEY_SCL = 1;
    DelayUs(5);
    KEY_SCL = 0;
    DelayUs(5);


    for(i = 7; i >= 0; i--)
    {
        KEY_SCL = 1;
        DelayUs(5);
        data |= KEY_SDA << i;
        KEY_SCL = 0;
        DelayUs(5);
    }

    // Change SDA(P1.5) back to output
    KEY_SETSDA_OUTPUT();

    KEY_SCL = 1;
    KEY_SDA = 1;
    DelayUs(5);

    // enable interrupt on sda(p1.5)
    KEY_SETSDA_INPUT();     // set P1.5 to input
    KEY_SDA_CLEARIFG(); // clear ifg of P1.5
    KEY_INT_ENABLE();

    *pdata = data;
    EA = intState;
}


/****************************************************************
*** Get key from key-scan chip
*****************************************************************/
/****************************************************************
*** Get key from key-scan chip
*****************************************************************/

uint8 GetKey(void)
{
    uint8 key_data;

    //ReadKeyChip(0x6f, &key_data);
    if(P2_0)
    {
        return  HAL_KEY_INVALID;
    }
    else
    {
        return HAL_KEY_SELECT;
    }
}

/****************************************************************
*** Write key-scan chip
*****************************************************************/
void WriteKeyChip(uint8 cmd, uint8 data)
{
    return;

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
    //P1SEL &= ~BV(5);            // set P1.5 to general io
    //P1DIR &= ~BV(5);            // set P1.5 to input
    //P1IFG &= ~BV(5);            // clear ifg of P1.5

    // init P1.3 to output, used as SCL
    //P1SEL &= ~0x08;         // set P1.3 to general io
    //P1DIR |= 0x08;          // set P1.3 to output

    // init P1.5 to output, used as SDA
    //P1SEL &= ~BV(5);            // set P1.5 to general io
    //P1DIR |= BV(5);         // set P1.5 to output

    P2SEL &= ~BV(0);    //按键P2.0
    P2DIR &= ~BV(0);
    PICTL |= BV(3);  // 下边沿触发中断
    IEN0 |= BV(7);   //中断enable
    P2IEN |= BV(0);
    IEN2 |= BV(1);
    P2IFG &= ~(BV(0));


    P1SEL &= ~BV(6);
    P1DIR |= BV(6);
    P1_6 = 0;

    P1SEL &= ~BV(7);     // P2_4 used as gpio
    P1DIR |= BV(7);      // P2_4 used as output
    P1_7 =0;

    P1SEL &= ~(BV(0) | BV(2));   //P1.0 P1.2  gpio
    P1DIR |= (BV(0) | BV(2));

    P1_0 = 0;
    P1_2 = 0;

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
    return;

}

void KeyIntoSleep(void)
{
    return;

}

void WaitKeySleep(uint16 TimeOut)
{
    return;

}
void KeyReset(void)
{
    return;

}


/* enable key by time */
void KeyEnable(void)
{
    return;

}

void backlight_ctrl(bool enable)
{
    return;

}

void shake_ctrl(bool enable)
{
    return;

}

