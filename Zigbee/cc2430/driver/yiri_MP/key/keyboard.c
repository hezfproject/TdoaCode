/***********************************************
keyboard.c 
    Revised:        $Date: 2009/07/18 23:37:13 $
    Revision:       $Revision: 1.1 $

    Description:    This file contains the keyboard definitions.
************************************************/
/*********************************************************************
	The low-level keyboard interfaces for mobile phone are defined in the files.
*********************************************************************/
#include "keyboard.h"
/***************************
// Local macro define
****************************/
// Key adc_level define
#define KEY_LEVEL_MIN   0XA          //KEY min value
#define KEY_LEVEL_0     0X14
#define KEY_LEVEL_1     0X1E
#define KEY_LEVEL_2     0X28
#define KEY_LEVEL_3     0X30
#define KEY_LEVEL_4     0X37
#define KEY_LEVEL_5     0X41
#define KEY_LEVEL_6     0X49
#define KEY_LEVEL_7     0X51
#define KEY_LEVEL_8     0X59
#define KEY_LEVEL_9     0X5E
#define KEY_LEVEL_YES   0X66
#define KEY_LEVEL_NO    0X70

//key2number
#define CHAR_0     0x30 //the ascii of '0'.
#define KEY_CHAR_0     (0x0 + CHAR_0)
#define KEY_CHAR_1     (0x1 + CHAR_0)
#define KEY_CHAR_2     (0x2 + CHAR_0)
#define KEY_CHAR_3     (0x3 + CHAR_0)
#define KEY_CHAR_4     (0x4 + CHAR_0)
#define KEY_CHAR_5     (0x5 + CHAR_0)
#define KEY_CHAR_6     (0x6 + CHAR_0)
#define KEY_CHAR_7     (0x7 + CHAR_0)
#define KEY_CHAR_8     (0x8 + CHAR_0)
#define KEY_CHAR_9     (0x9 + CHAR_0)

#ifndef NMBRDIGIT
#define NMBRDIGIT 20
#endif

uint8 NmbrIdx = 0;
uint8 Nmbr[NMBRDIGIT];

void Delay_key(uint32 n);
/*******************************************************************************
// Delay
*******************************************************************************/
void Delay_key(uint32 n)
{
	uint32 tt;
	for(tt = 0;tt<n;tt++);
	for(tt = 0;tt<n;tt++);
	for(tt = 0;tt<n;tt++);
	for(tt = 0;tt<n;tt++);
	for(tt = 0;tt<n;tt++);
}

/*******************************************************************************
// Initial Keyboard
*******************************************************************************/
void InitialKey(void)           
{
#if 1 //do we need iterrupt??
    // init P0_0 as interrupt
    P0SEL &= ~0X40;         //把P0_6设为通用I/O口
    P0DIR &= ~0X40;         //把P0_6设为input

    PICTL |= 0x10;           //P0_4 to P0_7 Interrupt as interrupt    //|= 0x08;          /* P0IENL = 1 */ //把P0_0口的中断打开
    PICTL &= 0XFE;          /* P0ICON = 0 */ //把P0_0口的中断设为上升沿有效
    P0IF = 0;               // IRCON &= ~0x20;     //P0口中断标志位清零    
    P0IE = 1;               //P0口中断允许
    EA=1;                   //CPU所有中断打开
#endif
    // init P0_7 as adc input
    ADCCFG |= 0X80;         /* ADCCFG7 = 1 */ //把P0_7设为ADC输入口
    P0IFG = 0; //clear interrupt if possible.
}

/*******************************************************************************
// Get key by ADC sampling
*******************************************************************************/
uint16 GetKey(void)
{
    uint8 adchigh_8 = 0;            //定义high_8为ADC转换的高8位
    uint8 adchigh_8_first = 0;      //高8位读第一次
    uint8 adchigh_8_second = 0;     //高8位读第二次
    int8 sub = 0;
    uint8 adc_val = 0;
    uint16 key = KEY_INVALID;
    
    // delay for better sample time
    Delay_key(11000);
    // get first sample
    ADCCON3 = 0x17;             // 8'b00(1.25)01(9bits)0111(AIN0)
    ADCCON1 = 0x73;             // 8'b01110011
    Delay_key(250);
    adchigh_8_first = ADCH;
    // get second sample
    ADCCON3 = 0x17;             // 8'b00(1.25)01(9bits)0111(AIN0)
    ADCCON1 = 0x73;             // 8'b01110011
    Delay_key(250);
    adchigh_8_second = ADCH;
    // decide if it's valid or not
    sub = adchigh_8_first - adchigh_8_second;
    if ((sub>-2) && (sub <2)) {
        adchigh_8 = adchigh_8_second;
    } else {
      adchigh_8 = 0xFF;
    }
    
    adc_val = adchigh_8;
        
    if (adc_val < KEY_LEVEL_MIN)         {
      key = KEY_INVALID;
    } else if (adc_val < KEY_LEVEL_0)    {
      key = KEY_0;
      Nmbr[NmbrIdx++] = KEY_CHAR_0;
    } else if (adc_val < KEY_LEVEL_1)    {
      key = KEY_1;
      Nmbr[NmbrIdx++] = KEY_CHAR_1;
    } else if (adc_val < KEY_LEVEL_2)    {
      key = KEY_2;
      Nmbr[NmbrIdx++] = KEY_CHAR_2;
    } else if (adc_val < KEY_LEVEL_3)    {
      key = KEY_3;
	Nmbr[NmbrIdx++] = KEY_CHAR_3;
    } else if (adc_val < KEY_LEVEL_4)    {
      key = KEY_4;
	Nmbr[NmbrIdx++] = KEY_CHAR_4;
    } else if (adc_val < KEY_LEVEL_5)    {
      key = KEY_5;
	Nmbr[NmbrIdx++] = KEY_CHAR_5;
    } else if (adc_val < KEY_LEVEL_6)    {
      key = KEY_6;
	Nmbr[NmbrIdx++] = KEY_CHAR_6;
    } else if (adc_val < KEY_LEVEL_7)    {
      key = KEY_7;
	Nmbr[NmbrIdx++] = KEY_CHAR_7;
    } else if (adc_val < KEY_LEVEL_8)    {
      key = KEY_8;
	Nmbr[NmbrIdx++] = KEY_CHAR_8;
    } else if (adc_val < KEY_LEVEL_9)    {
      key = KEY_9;
	Nmbr[NmbrIdx++] = KEY_CHAR_9;
    } else if (adc_val < KEY_LEVEL_YES)  {
      key = KEY_YES;
      Nmbr[NmbrIdx++] = '\0';
    } else if (adc_val < KEY_LEVEL_NO)   {
      key = KEY_NO;
    }
    else key = KEY_INVALID;
    
    return key;
}

uint8 GetNumbers()
{
	return NmbrIdx < NMBRDIGIT ? NmbrIdx : 0;
}

uint8 ResetNumberBuf()
{
	uint8 i = 0;
	for (; i < NMBRDIGIT; --i)
		Nmbr[i] = 0;
	NmbrIdx = 0;
        return 0;
}
