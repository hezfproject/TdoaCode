#include <iocc2530.h>
#include "lcd_serial.h"
#include "delay.h"
#include "OSAL.h"
#include "hal_sleep.h"
#include "version.h"
#include "MobilePhone_MenuLib.h"
#if (defined WATCHDOG) &&(WATCHDOG==TRUE)
#include "WatchDogUtil.h"
#endif


//static  const   uint8 bat_mode_0[BATTERY_GRAPH_LEN] = {0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x7F, 0xFF, 0x40, 0x01, 0x40, 0x01, 0xC0, 0x01, 0xC0, 0x01, 0x40, 0x01, 0x40, 0x01, 0x7F, 0xFF, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00};
//static  const   uint8 bat_mode_1[BATTERY_GRAPH_LEN] = {0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x7F, 0xFF, 0x40, 0x01, 0x40, 0x09, 0xC0, 0x09, 0xC0, 0x09, 0x40, 0x09, 0x40, 0x01, 0x7F, 0xFF, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00};
//static  const   uint8 bat_mode_2[BATTERY_GRAPH_LEN] = {0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x7F, 0xFF, 0x40, 0x01, 0x40, 0x25, 0xC0, 0x25, 0xC0, 0x25, 0x40, 0x25, 0x40, 0x01, 0x7F, 0xFF, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00};
//static  const   uint8 bat_mode_3[BATTERY_GRAPH_LEN] = {0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x7F, 0xFF, 0x40, 0x01, 0x41, 0x25, 0xC1, 0x25, 0xC1, 0x25, 0x41, 0x25, 0x40, 0x01, 0x7F, 0xFF, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00};
//static  const   uint8 bat_mode_4[BATTERY_GRAPH_LEN] = {0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x7F, 0xFF, 0x40, 0x01, 0x49, 0x25, 0xC9, 0x25, 0xC9, 0x25, 0x49, 0x25, 0x40, 0x01, 0x7F, 0xFF, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00};
//static  const   uint8 SMS_ICON_data[BATTERY_GRAPH_LEN]   = {0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x7F, 0xFC, 0x60, 0x0C, 0x50, 0x14, 0x48, 0x24, 0x44, 0x44, 0x42, 0x84, 0x41, 0x04, 0x7F, 0xFC, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00};
enum
{
    SIGNAL_GRAPHIC = 1,
    BATTERY_GRAPHIC
};
/* handle bat or sms icon draw*/
//static  uint8 const  * graphicdraw_p;

/* Process side bar */
#define  BAR_START   LCD_LINE_HIGH
#define  BAR_LEN      3*LCD_LINE_HIGH
#define  BAR_BLACK   0xFC
#define  BAR_WHITE   0x00
#define  MAX_BARINV_LEN  BAR_LEN

//static  uint8 barinv_beg;
//static  uint8 barinv_len;

/*Lcd Backlight */
//static uint8    backlight_ctl = BACKLIGHT_CTL_10S;
/***************************
// Local subroutine define
****************************/
//static void LcdTestFont();

// Set cursor position
static void LcdSetCurXY(uint8 x, uint8 y);
// Get cursor position
//static UINT8_XY LcdGetCurXY(void);
// Print Font
//static void LcdPrintFont(uint8 *pdata, uint8 len);
// write lcd reg
static void LcdWriteReg(uint8 addr, uint8 data);
// read lcd reg
static uint8 LcdReadReg(uint8 addr);
// write lcd display memory
static void LcdWriteMem(uint8 data);
// read lcd display memory
//static uint8 LcdReadMem(void);

//static uint8 LCDConvertChar(uint8 aChar);

//static void LcdTestFont(void);

static void LCD_BatSmsIconDraw(uint16 len, uint8, uint8);
/****************************************************************
*** lcd clear
*****************************************************************/
void LCD_Clear(uint8 x_start, uint8 y_start, uint8 x_end, uint8 y_end)
{
    return;
   
}

/****************************************************************
*** clear lcd first line
*****************************************************************/
void LCD_Sig_Bat_Clear(uint8 index)
{
    return;
    
}


void LCD_ShowCursor(uint8 x, uint8 y)
{
    return;
   
}
void LCD_CloseCursor(void)
{
    return;
   
}

/****************************************************************
*** clear one line of lcd
*****************************************************************/
void LCD_Line_Clear(uint8 line_id)
{
    return;


}
void LCD_ListLine_Clear(uint8 line)
{
    LCD_Clear(0, line, LCD_LINE_WIDTH - 1, line);
}
/****************************************************************
*** clear lcd display
*****************************************************************/
void LcdClearDisplay(void)
{
    return;


}
/****************************************************************
*** Write Lcd register
*****************************************************************/

void LcdWriteReg(uint8 addr, uint8 data)
{
    return;
    
}

/****************************************************************
*** Read lcd register
*****************************************************************/
uint8 LcdReadReg(uint8 addr)
{
    return 0;
   
}
/****************************************************************
*** Write lcd memory
*****************************************************************/

void LcdWriteMem(uint8 data)
{
    return;
   
}

#if 0
/****************************************************************
*** Read lcd memory
*****************************************************************/
uint8 LcdReadMem(void)
{
    int8 i = 0;
    uint8 data = 0;

    LCD_CS = 0;

    DelayUs(20);

    LCD_SCK = 0;
    LCD_SDA = 1;    // RW = 1, read LCD
    DelayUs(1);
    LCD_SCK = 1;
    DelayUs(1);


    LCD_SCK = 0;
    LCD_SDA = 1;    // RS = 1
    DelayUs(1);
    LCD_SCK = 1;
    DelayUs(1);

    P1DIR &= ~0x40; // set SDA(P1.6) to input

    for(i = 7; i >= 0; i--)
    {
        LCD_SCK = 0;
        data |= LCD_SDA << i;
        DelayUs(1);
        LCD_SCK = 1;
        DelayUs(1);
    }

    LCD_CS = 1;

    P1DIR |= 0x40;  // set SDA(P1.6) back to output
    return data;
}
#endif

/*******************************************************************************
//       Initial LCD
*******************************************************************************/
void InitialLcd(void)
{
    return;
  

}

/****************************************************************
*** Set cursor position
*****************************************************************/
void LcdSetCurXY(uint8 x, uint8 y)
{
    return;

}
#if 0
UINT8_XY LcdGetCurXY(void)
{
    UINT8_XY xy = {0, 0};
    return xy;
}

/****************************************************************
*** Print font to lcd
*****************************************************************/
void LcdPrintFont(uint8 *pdata, uint8 len)
{
    uint8 i = 0;

    LcdWriteReg(PWRR, 0x02);
    LcdWriteReg(MWMR, 0x47);
    LcdWriteReg(SWRYR, 0x00);

    LcdSetCurXY(0, 0);

    for(i = 0; i < len; i++)
    {
        LcdWriteMem(pdata[i]);
        DelayMs(1);
    }

}

/****************************************************************
*** Font testing
*****************************************************************/
void LcdTestFont()
{
    unsigned int ii;
    unsigned char a[16] = {"翌日科技有限公司"};
//    unsigned char b[16]={"    中文字库    "};
    LcdWriteReg(PWRR, 0x02);
    LcdWriteReg(MWMR, 0x43);    // 0x47);
    LcdWriteReg(SWRYR, 0x00);
    LcdWriteReg(XCUR, 0x00);
    LcdWriteReg(YCUR, 0x00);

    LcdSetCurXY(0, 0);
    for(ii = 0; ii < 16; ii++)
    {
        LcdWriteMem(a[ii]);
//      KeyDelay(1000);
        DelayMs(1);
    }

    LcdSetCurXY(0, 16);
    for(ii = 0; ii < 16; ii++)
    {
        LcdWriteMem(a[ii]);
//      KeyDelay(1000);
        DelayMs(1);
    }

    LcdSetCurXY(0, 32);
    for(ii = 0; ii < 16; ii++)
    {
        LcdWriteMem(a[ii]);
//      KeyDelay(1000);
        DelayMs(1);
    }

    LcdSetCurXY(0, 48);
    for(ii = 0; ii < 16; ii++)
    {
        LcdWriteMem(a[ii]);
//      KeyDelay(1000);
        DelayMs(1);
    }

    LcdWriteReg(MWMR, 0x00);
}
uint8 LCDConvertChar(uint8 aChar)
{
    uint8 lChar = 0;

    return lChar;
}

void LCDWriteString(char* pStr, uint8 x, uint8 y)
{
    //uint8 strlen = osal_strlen(pStr);
    //UINT8_XY xy = LcdGetCurXY();

}

void LCDWriteChar(uint8 aChar)
{
    UINT8_XY xy;
    if(aChar <= '9' && aChar >= '0')
    {
        LcdWriteReg(MWMR, 0x52);
        LcdWriteMem('0');
        LcdWriteMem(aChar);
    }
    xy = LcdGetCurXY();
    if(xy.y >= LCD_MAX_Y)
    {
        if(xy.x < LCD_MAX_X - 1)
            LcdSetCurXY(xy.x + 1, 0);
        else
            LcdSetCurXY(0, 0);
    }
}
#endif

void LCD_BigAscii_Print(uint8 value, uint8 x, uint8 y)
{
    return;

}

void LCD_Memory_Print(uint8* mem, uint8 len, uint8 x, uint8 y)
{
    return;
   
}

void LCD_Str_Print(uint8 *pdata, uint8 x, uint8 y, bool direction)
{
    return;
   
}

uint8 ascii_scan(uint8*p , uint8 len)
{
    return 0;

}

/*********************************************************************
* @fn      LCD_SMS_Print
*
* @brief   It is called when there are chinese or other double byte charaters in the mem
*
* @param
*
* @return
*/
uint8 LCD_SMS_Print(uint8* mem, uint8 len, uint8 x, uint8 y)
{
    return 0;
   
}

void LCD_Str_Print_Pixel(uint8 *pdata, uint8 x, uint8 y)
{
    return;
   
}
uint8 LCD_ID_Show(uint8 id, uint8 x, uint8 y)
{
    return 0;
    
}

void LCD_ListLine_Inv(uint8 line)
{
    return;
    //LCD_Char_Inv(0, line, LCD_LINE_WIDTH - 1);
}
void LCD_Char_Inv(uint8 x, uint8 y, uint8 len)
{
    return;
    
}
void LCD_Clear_Inv(void)
{
    return;
    //LcdWriteReg(BLTR, 0x00);
}

uint8 LCD_Signal_Show(uint8 index)
{
    return 0;
    
}

/* Note: buffer is in graphicdraw_p, do not pass by input */
void LCD_BatSmsIconDraw(uint16 len, uint8 x, uint8 y)
{
    return;
}
void LCD_SMS_ICON_Show(uint8 x, uint8 line_id)
{
    return;
    
}
uint8 LCD_Battery_Show(uint8 index)
{
    return 0;
    
}


void LCD_ProgBar_open(void)
{
    return;
  
}

void LCD_ProgBar_update(uint8 sel_item, uint8 total_item)
{
    return;
    
}

void LCDSetBackLightCtl(uint8 ctl)
{
    return;
   
}

uint8 LCDGetBackLightCtl(void)
{
    return 0;
    //return backlight_ctl;
}

void LCDDisplayOff()
{
    return;
}
void LCDIntoSleep()
{
    return;
   
}
void LCDWakeUp()
{
    return;
    //LcdWriteReg(PWRR, 0x02);
    //return;
}

void do_Sleep(uint32 timeout_ms)
{
    return;
    //halSleep_immediately( timeout_ms);
}

