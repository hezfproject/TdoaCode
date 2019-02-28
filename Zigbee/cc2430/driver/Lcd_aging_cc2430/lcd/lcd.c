/***********************************************
lcd.c 
    Revised:        $Date: 2009/07/18 23:37:13 $
    Revision:       $Revision: 1.1 $

    Description:    This file contains the LCD definitions.
************************************************/
/*********************************************************************
	The low-level LCD interfaces for mobile phone are defined in the files.
*********************************************************************/
#include "lcd.h"
#include "keyboard.h"
#include "hal_key_cfg.h"

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

// LCD control signals define
#define LCD_RS          P0_1
#define LCD_RW          P0_2
#define LCD_E           P0_3
// LCD data type define
#define LCD_CMD         (0x00)
#define LCD_DATA        (0x01)

// LCD command define
#define LCDDISPLAY_0            (0X30)
#define LCDDISPLAY_1            (0X31)
#define LCDDISPLAY_2            (0x32)
#define LCDDISPLAY_3            (0X33)
#define LCDDISPLAY_4            (0X34)
#define LCDDISPLAY_5            (0X35)
#define LCDDISPLAY_6            (0X36)
#define LCDDISPLAY_7            (0X37)
#define LCDDISPLAY_8            (0X38)
#define LCDDISPLAY_9            (0X39)
#define LCDDISPLAY_CLEAR_ALL    (0X01)
#define LCDDISPLAY_CHGTO1_LINE  (0X80)
#define LCDDISPLAY_CHGTO2_LINE  (0XC0)
#define MAXPOS   8
#define MAXLINE   2
static int8  pos = 0;    //当前显示的数字位置, from 0 to 7.
static int8  line = 0;  //current line number, from 0 to 1.
/***************************
// Local subroutine define
****************************/
// Write data or command to LCD
void WriteLCD(uint8 type, uint8 data);
void Delay_lcd(uint32 n);

/*******************************************************************************
// Delay
*******************************************************************************/
void Delay_lcd(uint32 n)
{
	uint32 tt;
	for(tt = 0;tt<n;tt++);
	for(tt = 0;tt<n;tt++);
	for(tt = 0;tt<n;tt++);
	for(tt = 0;tt<n;tt++);
	for(tt = 0;tt<n;tt++);
}

/*******************************************************************************
//       Initial LCD
*******************************************************************************/
void InitialLCD(void)
{
	P0IFG = 0; //clear interrupt if possible.
}
/*******************************************************************************
//       Write LCD
*******************************************************************************/
void WriteLCD(uint8 type, uint8 data)
{
    P1DIR = 0XFF;       //0 :input,1 : output
    LCD_E = 0;        
    LCD_RS = type;      // 1:data, 0:cmd
    LCD_RW = 0;         //0:write,1:read
    P1 = data;
    Delay_lcd(1);
    LCD_E = 1;
    Delay_lcd(60);
    LCD_E = 0;
    Delay_lcd(40);
    P0IFG &= ~0x0E;
}

/*******************************************************************************
// Start LCD
*******************************************************************************/
void StartLCD(void) 
{
    P1SEL = 0X0 ;                           //P1 port set as general purpose I/O
    P1DIR = 0XFF ;                          //P1 port set as output port
    P0SEL &= 0XF1 ;                         // P0_1 , P0_2 ,P0 _3 set as general purpose I/O
    P0DIR |= 0XE ;                          //P0_1 ,P0_2 ,P0_3 set as output port

    P0SEL &= 0XDF;                          //把P0_5设为通用I/O口,P0_5是背光使能,低有效
    P0DIR |= 0X20;                          //把P0_5设为输出口
    #if 0
    // init timer1
    PERCFG |= 0x40;                         // Timer1 use alt2
    CLKCON |= 0x38;                         // use 32M/128=1/4M for timer ticket
    T1CTL = 0x0c;                           // Tick freq/128=1/512M=2k
    T1CC0L = 0x00;      
    T1CC0H = 0x14;                          // expire time = 3s
    // enable timer1 interrupt
    TIMER1_INT_ENABLE;                      //timer1 interrupt enable
    #endif
    
    //WriteLCD(CMD, XXX_MODE)
    WriteLCD(LCD_CMD, 0x38);                    // set to 8-bit mode,2-line
    WriteLCD(LCD_CMD, 0x0E);                    // display on, cursor appear    
    WriteLCD(LCD_CMD, 0x1C);                    // Shift display to the right. Cursor follows the display shift
    WriteLCD(LCD_CMD,LCDDISPLAY_CLEAR_ALL);     // clear display
    WriteLCD(LCD_CMD,0X02);                     // return home
    WriteLCD(LCD_CMD,0X06);                     // Entry mode set
    
    P0IFG = 0;
}

/*******************************************************************************
// Dispaly key on LCD
*******************************************************************************/
void LCDDisplay(int16 key)
{                
    if (line >= MAXLINE && pos >= MAXPOS) //we have overwrite the lcd. no space now.
		return;
	
    // display key on LCD
    if (key == KEY_0)                   {
       WriteLCD(LCD_DATA,LCDDISPLAY_0);
    }   else if (key == KEY_1)          {
       WriteLCD(LCD_DATA,LCDDISPLAY_1);
    }   else if (key == KEY_2)          {
       WriteLCD(LCD_DATA,LCDDISPLAY_2);
    }   else if (key == KEY_3)          {
       WriteLCD(LCD_DATA,LCDDISPLAY_3);
    }   else if (key == KEY_4)          {
       WriteLCD(LCD_DATA,LCDDISPLAY_4);
    }   else if (key == KEY_5)          {
       WriteLCD(LCD_DATA,LCDDISPLAY_5);
    }   else if (key == KEY_6)          {
       WriteLCD(LCD_DATA,LCDDISPLAY_6);
    }   else if (key == KEY_7)          {
       WriteLCD(LCD_DATA,LCDDISPLAY_7);
    }   else if (key == KEY_8)          {
       WriteLCD(LCD_DATA,LCDDISPLAY_8);
    }   else if (key == KEY_9)          {
       WriteLCD(LCD_DATA,LCDDISPLAY_9);
    }   else if (key == KEY_YES)        {
    }   else if (key == KEY_NO)         {
       if (pos > 0)
       {
       	WriteLCD (LCD_CMD,0X10);             //shift the cursor to the left
       	WriteLCD (LCD_DATA,0X20);            //write a space
       	WriteLCD (LCD_CMD,0X10);             //shift the cursor to the left
       }
	else if (line > 0)
	{
		pos = MAXPOS - 1;
		WriteLCD(LCD_CMD,LCDDISPLAY_CHGTO1_LINE);
	}
    }   else if (key == KEY_INVALID)    {
    }
    
  
    // control cursor shift on LCD       
    if ((key != KEY_INVALID) && (key != KEY_YES)) {
        if(key != KEY_NO) {
            pos = pos + 1;
        } else if(key == KEY_NO) {
            pos = pos - 1;
        }
        if (pos == 8 && line < MAXLINE) {
	     pos = 0;
            WriteLCD(LCD_CMD,LCDDISPLAY_CHGTO2_LINE);
	     line++;
        } else if (line == 0X2) {
            //WriteLCD(LCD_CMD,0X02);  //return home
            //pos = 0;
        }
    }  
}

