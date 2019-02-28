#include <iocc2430.h>
#include "lcd_serial.h"
#include "../comm/delay.h"
#include "OSAL.h"
/***************************
// Local macro define
****************************/
// LCD interface define
#define LCD_CS			P2_3//P0_2
#define LCD_SDA			P2_4//P0_3
#define LCD_SCK			P0_0//P0_4
#define LCD_RESET       	P0_5

#define LCD_CS_BIT       3// 2
#define LCD_SDA_BIT     4// 3
#define LCD_SCK_BIT     0//  4
#define LCD_RESET_BIT  5
#define LCD_BACKLIGHT_BIT  6
// LCD register define
#define DWFR		    0x00
#define PWRR		    0x01
#define SYSR		    0x02
#define MWMR		    0x03
#define CURCR		    0x04
#define XCUR		    0x05
#define YCUR		    0x06
#define KEYR		    0x07
#define KSDR		    0x07
#define SWSXR		    0x08
#define SWSYR		    0x09
#define SWRXR		    0x0a
#define SWRYR		    0x0b
#define SCOR		    0x0c
#define ASCR		    0x0d
#define SCCR		    0x0e
#define ISR		        0x0f
#define CSTR		    0x10
#define DRCRA		    0x11
#define DRCRB		    0x12
#define	BLTR		    0x13
#define IODR		    0x14
#define IODAR		    0x15
#define ELCR		    0x16
#define CGMI		    0x17
#define CGMD		    0x18

#define SIGNAL_ASCII 0x80
#define BATTERY_ASCII 0x85
#define BATTERY_GRAPH_LEN 32

#define  BAR_START   LCD_LINE_HIGH
#define  BAR_LEN      3*LCD_LINE_HIGH
#define  BAR_BLACK   0xFC
#define  BAR_WHITE   0x00
#define  MAX_BARINV_LEN  BAR_LEN

/*********************************************************************
* static definations
*********************************************************************/
static __code const uint8 bat_mode_0[BATTERY_GRAPH_LEN] = {0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x7F,0xFF,0x40,0x01,0x40,0x01,0xC0,0x01,0xC0,0x01,0x40,0x01,0x40,0x01,0x7F,0xFF,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00};
static __code const uint8 bat_mode_1[BATTERY_GRAPH_LEN] = {0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x7F,0xFF,0x40,0x01,0x40,0x09,0xC0,0x09,0xC0,0x09,0x40,0x09,0x40,0x01,0x7F,0xFF,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00};
static __code const uint8 bat_mode_2[BATTERY_GRAPH_LEN] = {0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x7F,0xFF,0x40,0x01,0x40,0x25,0xC0,0x25,0xC0,0x25,0x40,0x25,0x40,0x01,0x7F,0xFF,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00};
static __code const uint8 bat_mode_3[BATTERY_GRAPH_LEN] = {0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x7F,0xFF,0x40,0x01,0x41,0x25,0xC1,0x25,0xC1,0x25,0x41,0x25,0x40,0x01,0x7F,0xFF,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00};
static __code const uint8 bat_mode_4[BATTERY_GRAPH_LEN] = {0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x7F,0xFF,0x40,0x01,0x49,0x25,0xC9,0x25,0xC9,0x25,0x49,0x25,0x40,0x01,0x7F,0xFF,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00};
static __code const uint8 SMS_ICON_data[BATTERY_GRAPH_LEN]   = {0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x7F,0xFC,0x60,0x0C,0x50,0x14,0x48,0x24,0x44,0x44,0x42,0x84,0x41,0x04,0x7F,0xFC,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00};

/*--  文字:  0  --*/
/*--  黑体22;  此字体下对应的点阵为：宽x高=15x29   --*/
/*--  宽度不是8的倍数，现调整为：宽度x高度=16x29  --*/
static  __code const uint8  char_0[] = {
	0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x0F,0xE0,0x1F,0xF0,
	0x3E,0xF8,0x3C,0x78,0x78,0x3C,0x78,0x3C,0x78,0x3C,0x70,0x1C,0x70,0x1C,0x70,0x1C,
	0x70,0x1C,0x70,0x1C,0x78,0x3C,0x78,0x3C,0x78,0x3C,0x3C,0x78,0x3E,0xF8,0x1F,0xF0,
	0x0F,0xE0,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00
};

/*--  文字:  1  --*/
/*--  黑体22;  此字体下对应的点阵为：宽x高=15x29   --*/
/*--  宽度不是8的倍数，现调整为：宽度x高度=16x29  --*/
static  __code const uint8  char_1[] = {
	0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x03,0x80,0x03,0x80,
	0x0F,0x80,0x1F,0x80,0x03,0x80,0x03,0x80,0x03,0x80,0x03,0x80,0x03,0x80,0x03,0x80,
	0x03,0x80,0x03,0x80,0x03,0x80,0x03,0x80,0x03,0x80,0x03,0x80,0x03,0x80,0x03,0x80,
	0x03,0x80,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00
};

/*--  文字:  2  --*/
/*--  黑体22;  此字体下对应的点阵为：宽x高=15x29   --*/
/*--  宽度不是8的倍数，现调整为：宽度x高度=16x29  --*/
static  __code const uint8  char_2[] = {
	0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x1F,0xC0,0x3F,0xE0,
	0x3D,0xF0,0x78,0xF0,0x78,0x70,0x70,0x70,0x70,0xF0,0x00,0xF0,0x00,0xF0,0x01,0xE0,
	0x03,0xC0,0x07,0x80,0x0F,0x00,0x1E,0x00,0x3C,0x00,0x78,0x00,0x78,0x00,0x7F,0xF0,
	0x7F,0xF0,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00
};

/*--  文字:  3  --*/
/*--  黑体22;  此字体下对应的点阵为：宽x高=15x29   --*/
/*--  宽度不是8的倍数，现调整为：宽度x高度=16x29  --*/
static  __code const uint8  char_3[] = {
	0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x0F,0xE0,0x3F,0xF0,
	0x3C,0xF8,0x78,0x78,0x78,0x38,0x70,0x38,0x00,0x78,0x01,0xF8,0x03,0xE0,0x03,0xF0,
	0x01,0xF8,0x00,0x78,0x00,0x38,0x70,0x38,0x78,0x38,0x78,0x78,0x7C,0xF8,0x3F,0xF0,
	0x1F,0xE0,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00
};

/*--  文字:  4  --*/
/*--  黑体22;  此字体下对应的点阵为：宽x高=15x29   --*/
/*--  宽度不是8的倍数，现调整为：宽度x高度=16x29  --*/
static  __code const uint8  char_4[] = {
	0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0xE0,0x01,0xE0,
	0x03,0xE0,0x03,0xE0,0x07,0xE0,0x07,0xE0,0x0E,0xE0,0x1E,0xE0,0x1C,0xE0,0x38,0xE0,
	0x78,0xE0,0x70,0xE0,0x7F,0xFC,0x7F,0xFC,0x00,0xE0,0x00,0xE0,0x00,0xE0,0x00,0xE0,
	0x00,0xE0,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00
};

/*--  文字:  5  --*/
/*--  黑体22;  此字体下对应的点阵为：宽x高=15x29   --*/
/*--  宽度不是8的倍数，现调整为：宽度x高度=16x29  --*/
static  __code const uint8  char_5[] = {
	0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x1F,0xF8,0x3F,0xF8,
	0x38,0x00,0x38,0x00,0x38,0x00,0x38,0x00,0x3F,0xE0,0x3F,0xF0,0x3C,0xF8,0x38,0x78,
	0x00,0x78,0x00,0x38,0x00,0x38,0x70,0x38,0x78,0x78,0x78,0x78,0x3C,0xF0,0x3F,0xE0,
	0x1F,0xC0,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00
};

/*--  文字:  6  --*/
/*--  黑体22;  此字体下对应的点阵为：宽x高=15x29   --*/
/*--  宽度不是8的倍数，现调整为：宽度x高度=16x29  --*/
static  __code const uint8  char_6[] = {
	0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x07,0xE0,0x0F,0xF0,
	0x1E,0x78,0x1C,0x78,0x3C,0x38,0x3C,0x00,0x3F,0xE0,0x3F,0xF0,0x3E,0xF8,0x3C,0x78,
	0x3C,0x78,0x38,0x38,0x38,0x38,0x38,0x38,0x3C,0x78,0x3C,0x78,0x1E,0xF8,0x0F,0xF0,
	0x07,0xE0,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00
};

/*--  文字:  7  --*/
/*--  黑体22;  此字体下对应的点阵为：宽x高=15x29   --*/
/*--  宽度不是8的倍数，现调整为：宽度x高度=16x29  --*/
static  __code const uint8  char_7[] = {
	0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x3F,0xF8,0x3F,0xF8,
	0x00,0x78,0x00,0x78,0x00,0x78,0x00,0x70,0x00,0xF0,0x00,0xF0,0x00,0xF0,0x01,0xE0,
	0x01,0xE0,0x01,0xE0,0x03,0xC0,0x03,0xC0,0x03,0xC0,0x03,0x80,0x07,0x80,0x07,0x80,
	0x07,0x80,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00
};

/*--  文字:  8  --*/
/*--  黑体22;  此字体下对应的点阵为：宽x高=15x29   --*/
/*--  宽度不是8的倍数，现调整为：宽度x高度=16x29  --*/
static  __code const uint8  char_8[] = {
	0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x0F,0xC0,0x1F,0xE0,
	0x3C,0xF0,0x3C,0xF0,0x38,0x70,0x38,0x70,0x3C,0xF0,0x3C,0xF0,0x1F,0xE0,0x1F,0xE0,
	0x3C,0xF0,0x78,0x78,0x70,0x38,0x70,0x38,0x78,0x78,0x78,0x78,0x7C,0xF8,0x3F,0xF0,
	0x1F,0xE0,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00
};

/*--  文字:  9  --*/
/*--  黑体22;  此字体下对应的点阵为：宽x高=15x29   --*/
/*--  宽度不是8的倍数，现调整为：宽度x高度=16x29  --*/
static  __code const uint8  char_9[] = {
	0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x1F,0x80,0x3F,0xC0,
	0x7D,0xE0,0x78,0xF0,0x78,0xF0,0x70,0x70,0x70,0x70,0x70,0x70,0x78,0xF0,0x7D,0xF0,
	0x3F,0xF0,0x1F,0x70,0x00,0x70,0x00,0xF0,0x70,0xF0,0x78,0xE0,0x7D,0xE0,0x3F,0xC0,
	0x1F,0x80,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00
};

/*--  文字:  %  --*/
/*--  黑体22;  此字体下对应的点阵为：宽x高=15x29   --*/
/*--  宽度不是8的倍数，现调整为：宽度x高度=16x29  --*/
static  __code const uint8  char_percent[] = {
	0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x3C,0x10,0x7E,0x30,
	0x7E,0x20,0x66,0x60,0x66,0x60,0x66,0xC0,0x66,0xC0,0x7F,0x80,0x7F,0x80,0x3D,0x78,
	0x03,0xFC,0x03,0xFC,0x06,0xCC,0x06,0xCC,0x0C,0xCC,0x0C,0xCC,0x08,0xFC,0x18,0xFC,
	0x10,0x78,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00
};

/*--  文字:  .  --*/
/*--  黑体22;  此字体下对应的点阵为：宽x高=15x29   --*/
/*--  宽度不是8的倍数，现调整为：宽度x高度=16x29  --*/
static  __code const uint8  char_dot[] = {
	0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,
	0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,
	0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x38,0x00,0x38,0x00,
	0x38,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00
};

/*--  文字:  -  --*/
/*--  黑体22;  此字体下对应的点阵为：宽x高=15x29   --*/
/*--  宽度不是8的倍数，现调整为：宽度x高度=16x29  --*/
static  __code const uint8  char_minus[] = {
0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,
0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x7F,0xF8,0x7F,0xF8,
0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,
0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00
};

/*--  文字:  ℃  --*/
/*--  黑体22;  此字体下对应的点阵为：宽x高=29x29   --*/
/*--  宽度不是8的倍数，现调整为：宽度x高度=32x29  --*/
static  __code const uint8  char_celsius[] = {
0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x06,0x00,0x00,0x00,
0x0F,0x03,0xF0,0x00,0x19,0x8F,0xFC,0x80,0x19,0x9E,0x07,0x80,0x19,0xB8,0x03,0x80,
0x0F,0x38,0x01,0x80,0x00,0x70,0x01,0x80,0x00,0x70,0x00,0x80,0x00,0x60,0x00,0x80,
0x00,0xE0,0x00,0x00,0x00,0xE0,0x00,0x00,0x00,0xE0,0x00,0x00,0x00,0xE0,0x00,0x00,
0x00,0xE0,0x00,0x00,0x00,0x60,0x00,0x00,0x00,0x70,0x00,0x00,0x00,0x70,0x00,0x80,
0x00,0x30,0x01,0x80,0x00,0x38,0x03,0x00,0x00,0x1C,0x07,0x00,0x00,0x0F,0xFC,0x00,
0x00,0x07,0xF8,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,
0x00,0x00,0x00,0x00
};

enum
{
	SIGNAL_GRAPHIC = 1,
	BATTERY_GRAPHIC
};

static  uint8 barinv_beg;
static  uint8 barinv_len;

/*Lcd Backlight */
static uint8    backlight_ctl = BACKLIGHT_CTL_10S;
/***************************
// Local subroutine define
****************************/
static void LcdTestFont();

// Set cursor position
static void LcdSetCurXY(uint8 x,uint8 y);
// Get cursor position
static UINT8_XY LcdGetCurXY(void);
// Print Font
static void LcdPrintFont(uint8 *pdata, uint8 len);
// write lcd reg
static void LcdWriteReg(uint8 addr, uint8 data);
// read lcd reg
static uint8 LcdReadReg(uint8 addr);
// write lcd display memory
static void LcdWriteMem(uint8 data);
// read lcd display memory
static uint8 LcdReadMem(void);

static uint8 LCDConvertChar(uint8 aChar);

static void LcdTestFont(void);

/****************************************************************
*** lcd clear
*****************************************************************/
void LCD_Clear(uint8 x_start, uint8 y_start, uint8 x_end, uint8 y_end)
{
	uint8 i = 0;

	LcdWriteReg(MWMR,0x52);

	LcdSetCurXY(x_start,y_start*LCD_LINE_HIGH);

	uint8 len =  (y_end-y_start)*(LCD_LINE_WIDTH+2)-x_start+x_end;

	for(i=0;i<len;i++)
	{
		LcdWriteMem(0x00);
	}
}

/****************************************************************
*** clear lcd first line
*****************************************************************/
void LCD_Sig_Bat_Clear(uint8 index)
{
	if(index == SIGNAL_GRAPHIC)
	{
		LcdWriteReg(MWMR,0x52);
		LcdSetCurXY(1,0);
		LcdWriteMem(0x00);
	}
	else if(index == BATTERY_GRAPHIC)
	{
		LcdWriteReg(MWMR,0x52);
		LcdSetCurXY(14,0);
		LcdWriteMem(0x00); 
		LcdWriteMem(0x00); 
	}
}


void LCD_ShowCursor(uint8 x, uint8 y)
{
	uint8 data;

	LcdSetCurXY(x, y*LCD_LINE_HIGH);
	data = LcdReadReg(CURCR);
	data |= 1<<0;   // cursor display on
	data |= 1<<2;   // cursor blink 
	LcdWriteReg(CURCR, data);

}
void LCD_CloseCursor(void)
{
	uint8 data;
	data = LcdReadReg(CURCR);
	data &= ~1<<0;   // cursor display off
	data &= ~1<<2;   // close cursor blink 
	LcdWriteReg(CURCR, data);
}

/****************************************************************
*** clear one line of lcd
*****************************************************************/
void LCD_Line_Clear(uint8 line_id)
{
	LCD_Clear(0, line_id, LCD_LINE_WIDTH, line_id);
	LCD_CloseCursor();
}
void LCD_ListLine_Clear(uint8 line)
{
	LCD_Clear(0, line, LCD_LINE_WIDTH-1, line);
}
/****************************************************************
*** clear lcd display
*****************************************************************/
void LcdClearDisplay(void)
{
#if 0
    int i=0,p=0;
    LcdWriteReg(BLTR,0x00);
    LcdWriteReg(MWMR,0x00);
    LcdWriteReg(XCUR,0x00);
    LcdWriteReg(YCUR,0x00);
    for(i=0;i<128;i++) {
        for(p=0;p<8;p++) {
            LcdWriteMem(0x00); 
        }   
    } 
#else
    uint8 i=0;
    LcdWriteReg(BLTR,0x00);
    LcdWriteReg(MWMR,0x52);
    LcdWriteReg(XCUR,0x00);
    LcdWriteReg(YCUR,0x00);
    for(i=0;i<72;i++) 
    {
           LcdWriteMem(0x00); 
    } 
    LCD_CloseCursor();
#endif

}
/****************************************************************
*** Write Lcd register
*****************************************************************/

void LcdWriteReg(uint8 addr, uint8 data)
{
	int8 i = 0;

	LCD_CS = 0;
	
    DelayUs(20);
    
	LCD_SCK = 0;	
	LCD_SDA = 0;	// RW = 0, write LCD
	DelayUs(2);
	LCD_SCK = 1;
	DelayUs(2);	

	LCD_SCK = 0;
	LCD_SDA = 0;	// RS = 0;
	DelayUs(2);
	LCD_SCK = 1;
	DelayUs(2);

	for (i=7; i>=0; i--) {
        LCD_SCK = 0;
        if (addr & 0x80) {
            LCD_SDA = 1;
        } else {
            LCD_SDA = 0;
        }
        addr = addr << 1;
        DelayUs(8);
        LCD_SCK = 1;
        DelayUs(2);
            
	}

	for (i=7; i>=0; i--) {
		LCD_SCK = 0;
        if (data & 0x80) {
            LCD_SDA = 1;
        } else {
            LCD_SDA = 0;
        }
        data = data << 1;
        DelayUs(8);
		LCD_SCK = 1;
        DelayUs(2);
	}
	LCD_SCK = 1;
	LCD_SDA = 1;
	LCD_CS = 1;
}

/****************************************************************
*** Read lcd register
*****************************************************************/
uint8 LcdReadReg(uint8 addr)
{
	int8 i = 0;
	uint8 data = 0;

	LCD_CS = 0;
	DelayUs(20);

	LCD_SCK = 0;	
	LCD_SDA = 1;	// RW = 1, read LCD
	DelayUs(1);
	LCD_SCK = 1;
	DelayUs(1);

	LCD_SCK = 0;
	LCD_SDA = 0;	// RS = 0;
	DelayUs(1);
	LCD_SCK = 1;
	DelayUs(1);

	for (i=7; i>=0; i--) {
		LCD_SCK = 0;
		LCD_SDA = (addr >> i) & 1;	// send addr
		DelayUs(1);
		LCD_SCK = 1;
		DelayUs(1);
	}

	P2DIR &= ~(0x01<<LCD_SDA_BIT);	// set SDA  to input

	for (i=7; i>=0; i--) {
		LCD_SCK = 0;
		DelayUs(1);
		data |= LCD_SDA << i;		// read data
		LCD_SCK = 1;
		DelayUs(1);
	}

	LCD_CS = 1;

	P2DIR |= (0x01<<LCD_SDA_BIT);	// set SDA back to output
	return data;
}
/****************************************************************
*** Write lcd memory
*****************************************************************/

void LcdWriteMem(uint8 data)
{
	int8 i = 0;

	LCD_CS = 0;
    DelayUs(20);
	
	LCD_SCK = 0;	
	LCD_SDA = 0;	// RW = 0, write LCD
    DelayUs(8);
	LCD_SCK = 1;
    DelayUs(8);
	
	LCD_SCK = 0;
	LCD_SDA = 1;	// RS = 1
    DelayUs(8);
	LCD_SCK = 1;	
    DelayUs(8);
	
	for (i=7; i>=0; i--) {
		LCD_SCK = 0;
        if (data & 0x80) {
            LCD_SDA = 1;
        } else {
            LCD_SDA = 0;
        }
        data = data << 1;        
        DelayUs(16);
		LCD_SCK = 1;
        DelayUs(8);
	}

	LCD_SCK = 1;
	LCD_SDA = 1;	
	LCD_CS = 1;
	//    DelayMs(1);
}
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
	LCD_SDA = 1;	// RW = 1, read LCD
	DelayUs(1);
	LCD_SCK = 1;
	DelayUs(1);


	LCD_SCK = 0;
	LCD_SDA = 1;	// RS = 1
	DelayUs(1);
	LCD_SCK = 1;	
	DelayUs(1);

	P2DIR &= ~(0x01<<LCD_SDA_BIT);	// set SDA  to input

	for (i=7; i>=0; i--) {
		LCD_SCK = 0;
		data |= LCD_SDA << i;
		DelayUs(1);
		LCD_SCK = 1;
		DelayUs(1);
	}

	LCD_CS = 1;

	P2DIR |= (0x01<<LCD_SDA_BIT);	// set SDA  back to output
	return data;
}

/*******************************************************************************
//       Initial LCD
*******************************************************************************/
void InitialLcd(void)
{
	//P0SEL &=~ (0x01<<LCD_CS_BIT | 0x01<<LCD_SDA_BIT |0x01<<LCD_SCK_BIT |0x01<<LCD_RESET_BIT |0x01<<LCD_BACKLIGHT_BIT);// set LCD_CS, LCD_SDA, LCD_SCK,LCD_RESET to general io
	//P0DIR |= (0x01<<LCD_CS_BIT | 0x01<<LCD_SDA_BIT |0x01<<LCD_SCK_BIT |0x01<<LCD_RESET_BIT | 0x01<<LCD_BACKLIGHT_BIT);// set LCD_CS, LCD_SDA, LCD_SCK,LCD_RESET to output
	P0SEL &=~ (0x01<<LCD_SCK_BIT |0x01<<LCD_RESET_BIT |0x01<<LCD_BACKLIGHT_BIT);// set LCD_SCK,LCD_RESET to general io
	P0DIR |= (0x01<<LCD_SCK_BIT |0x01<<LCD_RESET_BIT | 0x01<<LCD_BACKLIGHT_BIT);// set  LCD_SCK,LCD_RESET to output

	P2SEL &=~ (0x01<<LCD_CS_BIT | 0x01<<LCD_SDA_BIT );// set LCD_CS, LCD_SDA to general io
	P2DIR |= (0x01<<LCD_CS_BIT | 0x01<<LCD_SDA_BIT);// set LCD_CS, LCD_SDA to output

	LCD_CS = 1;
	LCD_SCK = 1;
	LCD_SDA = 1;

	LCDReset();
	DelayMs(150);

	LcdWriteReg(0x00, 0x00);	// 驱动波形设定缓存器
	DelayMs(5);
	LcdWriteReg(0x01, 0x02);	// 开显示,关闭键盘
	DelayMs(5);
	LcdWriteReg(0x02,0x7B); 	// 128*64中文简体
	LcdWriteReg(0x03,0x00); 	// 内存输入模式缓存器，正常显示全型字模式，不加粗，不反白
	LcdWriteReg(0x04,0x00); 		// 光标控制缓存器，不显示，不闪动
	LcdWriteReg(0x05,0x00); 	// 光标X 位置缓存器
	LcdWriteReg(0x06,0x00); 	// 光标Y 位置缓存器
	LcdWriteReg(0x07,0x00); 	// 键盘扫描控制缓存器
	LcdWriteReg(SWSXR,0x00); 	// X 轴卷动起始点缓存器
	LcdWriteReg(SWSYR,0x00); 	// Y 轴卷动起始点缓存器
	LcdWriteReg(SWRXR,0x00); 	// X 轴卷动范围缓存器
	LcdWriteReg(SWRYR,0x00); 	// Y 轴卷动范围缓存器
	LcdWriteReg(0x0C,0x00); 	// 卷动位移量缓存器
	LcdWriteReg(0x0D,0x00); 	// 自动卷动控制缓存器
	LcdWriteReg(0x0E,0x00); 	// 卷动控制缓存器
	LcdWriteReg(0x0F,0x00); 	// 中断状态缓存器

	LcdWriteReg(0x10,0xef);//0xff);	//1/9Bias  最大对比度
	DelayMs(5);
	LcdWriteReg(0x12,0xfc);		//4x,最小电流驱动
	LcdWriteReg(BLTR,0x00);	
	LcdWriteReg(0x14,0x00); 	// IO 端口方向设定缓存器
	LcdWriteReg(0x15,0x00); 	// IO 端口数据缓存器
	LcdWriteReg(0x16,0x80); 	// 冷光控制缓存器
	LcdWriteReg(0x17,0x00); 	// 造字选择缓存器
	LcdWriteReg(0x18,0x00); 	// 造字数据缓存器
	LcdWriteReg(0x11,0xf8);		//全部内部的电压驱动	

	LcdClearDisplay();

}

/****************************************************************
*** Set cursor position
*****************************************************************/
void LcdSetCurXY(uint8 x,uint8 y)
{
	LcdWriteReg(XCUR,x);
	//DelayMs(1);
	DelayUs(10);
	LcdWriteReg(YCUR,y);
	//DelayMs(1);
	DelayUs(10);
}

/****************************************************************
*** Print font to lcd
*****************************************************************/
void LcdPrintFont(uint8 *pdata, uint8 len)
{
	uint8 i = 0;

	LcdWriteReg(PWRR,0x02); 
	LcdWriteReg(MWMR,0x47);
	LcdWriteReg(SWRYR,0x00);

	LcdSetCurXY(0,0);

	for(i=0; i<len; i++)
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
	unsigned char a[16]={"翌日科技有限公司"};
	//    unsigned char b[16]={"    中文字库    "};
	LcdWriteReg(PWRR,0x02); 
	LcdWriteReg(MWMR,0x43);     // 0x47);
	LcdWriteReg(SWRYR,0x00);
	LcdWriteReg(XCUR,0x00);
	LcdWriteReg(YCUR,0x00);

	LcdSetCurXY(0, 0);
	for(ii=0;ii<16;ii++)
	{
		LcdWriteMem(a[ii]);	
		//		KeyDelay(1000); 
		DelayMs(1);
	}    

	LcdSetCurXY(0, 16);
	for(ii=0;ii<16;ii++)
	{
		LcdWriteMem(a[ii]);	
		//		KeyDelay(1000); 
		DelayMs(1);
	}

	LcdSetCurXY(0, 32);
	for(ii=0;ii<16;ii++)
	{
		LcdWriteMem(a[ii]);	
		//		KeyDelay(1000); 
		DelayMs(1);
	}

	LcdSetCurXY(0, 48);
	for(ii=0;ii<16;ii++)
	{
		LcdWriteMem(a[ii]);	
		//		KeyDelay(1000); 
		DelayMs(1);
	}    

	LcdWriteReg(MWMR,0x00);
}

uint8 LCDConvertChar(uint8 aChar)
{
	uint8 lChar = 0;

	return lChar;
}

/*
void LCD_BigAscii_Print(uint8 value, uint8 x, uint8 y)
{
LcdWriteReg(MWMR,0x52);
LcdSetCurXY(x, y*LCD_LINE_HIGH);
LcdWriteMem(value);	
DelayMs(1);
}*/

void LCD_Memory_Print(uint8* mem, uint8 len, uint8 x, uint8 y)
{
	uint8 i = 0;

	if(mem == NULL)
		return;

	LcdWriteReg(PWRR,0x02); 
	LcdWriteReg(MWMR,0x43);
	LcdWriteReg(SWRYR,0x00);
	LcdWriteReg(BLTR,0x00);

	if((LCD_LINE_WIDTH*4-x-LCD_LINE_WIDTH*y)<len)
		len = LCD_LINE_WIDTH*4-x-LCD_LINE_WIDTH*y;

	LcdSetCurXY(x, y*LCD_LINE_HIGH);

	if(len <= LCD_LINE_WIDTH)
	{
		for(i=0; i<len; i++)
		{
			LcdWriteMem(mem[i]);	
			DelayMs(1);
		}
	}
	else if(len <= 2*LCD_LINE_WIDTH)
	{
		for(i=0; i<LCD_LINE_WIDTH-x; i++)
		{
			LcdWriteMem(mem[i]);	
			DelayMs(1);
		}
		LcdSetCurXY(0, (y+1)*LCD_LINE_HIGH);
		for(; i<len; i++)
		{
			LcdWriteMem(mem[i]);	
			DelayMs(1);
		}
	}
	else if(len <= 3*LCD_LINE_WIDTH)
	{
		for(i=0; i<LCD_LINE_WIDTH-x; i++)
		{
			LcdWriteMem(mem[i]);	
			DelayMs(1);
		}
		LcdSetCurXY(0, (y+1)*LCD_LINE_HIGH);
		for(; i<len; i++)
		{
			LcdWriteMem(mem[i]);	
			DelayMs(1);
		}
		LcdSetCurXY(0, (y+2)*LCD_LINE_HIGH);
		for(; i<len; i++)
		{
			LcdWriteMem(mem[i]);	
			DelayMs(1);
		}
	}
	else if(len <= 4*LCD_LINE_WIDTH)
	{
		for(i=0; i<LCD_LINE_WIDTH-x; i++)
		{
			LcdWriteMem(mem[i]);	
			DelayMs(1);
		}
		LcdSetCurXY(0, (y+1)*LCD_LINE_HIGH);
		for(; i<len; i++)
		{
			LcdWriteMem(mem[i]);	
			DelayMs(1);
		}
		LcdSetCurXY(0, (y+2)*LCD_LINE_HIGH);
		for(; i<len; i++)
		{
			LcdWriteMem(mem[i]);	
			DelayMs(1);
		}
		LcdSetCurXY(0, (y+3)*LCD_LINE_HIGH);
		for(; i<len; i++)
		{
			LcdWriteMem(mem[i]);	
			DelayMs(1);
		}
	}

}

void LCD_BigAscii_Print(uint8 value, uint8 x, uint8 y)
{
	LcdWriteReg(MWMR,0x52);
	LcdSetCurXY(x, y*LCD_LINE_HIGH);
	LcdWriteMem(value);	
	DelayMs(1);
}

void LCD_Str_Print(uint8 *pdata, uint8 x, uint8 y, bool direction)
{
	uint8 len;

	if(pdata == NULL)
		return;

	len = osal_strlen((char*)pdata);

	if( !direction )
	{
		if(len <= LCD_LINE_WIDTH)
			x = LCD_LINE_WIDTH - len;
		else if(len <= 2*LCD_LINE_WIDTH){
			x = 2*LCD_LINE_WIDTH -len;
			y = y - 1;
		}
		else if(len <= 3*LCD_LINE_WIDTH){
			x = 3*LCD_LINE_WIDTH -len;
			y = y - 2;
		}
		else if(len <= 4*LCD_LINE_WIDTH){
			x = 4*LCD_LINE_WIDTH -len;
			y = y - 3;
		}
	}

	LCD_Memory_Print(pdata, len, x, y);
}

uint8 ascii_scan(uint8*p ,uint8 len)
{
	uint8 i, j;   

	j = 0;
	for(i=0;i<len;i++)
	{
		if(p[i]>=0x80)
			j++;
	}
	return j;
}


void LCD_SMS_ICON_Show(uint8 x, uint8 line_id)
{
        uint8 y = 0;
	 uint8 const *sms_icon = (uint8 const *)SMS_ICON_data;
	 y = line_id*LCD_LINE_WIDTH;
	 LCD_GraphicMode_Write((uint8*)sms_icon,BATTERY_GRAPH_LEN,x, y,16);
	 LCD_CloseCursor();
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
	uint8 i, j, k;
	uint8 temp, finish_len, y_next_line;

	k = 0;
	y_next_line = y;
	temp = ((LCD_LINE_WIDTH -x) > len) ? len : (LCD_LINE_WIDTH -x);

	LcdWriteReg(PWRR,0x02); 
	LcdWriteReg(MWMR,0x43);
	LcdWriteReg(SWRYR,0x00);
	LcdWriteReg(BLTR,0x00);

	LcdSetCurXY(x, y*LCD_LINE_HIGH);
	do{	 
		//j = 0;
		/*
		for(i=k;i<(k+temp);i++)
		{
		if(mem[i]>=0x80)
		j++;
		}*/
		j = ascii_scan(&mem[k], temp);
		if(j%2 != 0)
		{
			for(i=k;i<(k+temp-1);i++)
			{	
				LcdWriteMem(mem[i]);
				DelayMs(1);
			}
			LcdWriteMem(0);
			DelayMs(1);
			finish_len = temp-1;
		}
		else
		{
			for(i=k;i<(k+temp);i++)
			{	
				LcdWriteMem(mem[i]);
				DelayMs(1);
			}
			finish_len = temp;
		}
		k += finish_len;

		if(++y_next_line > 3)
			break;
		len = len - finish_len;
		if(len > LCD_LINE_WIDTH)
			temp = LCD_LINE_WIDTH;
		else
			temp = len;
		LcdSetCurXY(0, y_next_line*LCD_LINE_HIGH);
	}while(len > 0);

	return k;
}

void LCD_Str_Print_Pixel(uint8 *pdata, uint8 x, uint8 y)
{	
	LcdWriteReg(PWRR,0x02); 
	LcdWriteReg(MWMR,0x43);
	LcdWriteReg(SWRYR,0x00);
	LcdWriteReg(BLTR,0x00);
	uint8 len = osal_strlen((char*)pdata);

	if(x + len > LCD_LINE_WIDTH)
		return;

	LcdSetCurXY(x, y);
	for(uint8 i=0;i<len;i++)
	{
		LcdWriteMem(pdata[i]);
		DelayMs(1);
	}
}
uint8 LCD_ID_Show(uint8 id, uint8 x, uint8 y)
{
	uint8 temp = id + '0';

	if(id <= 9)
	{
		LCD_BigAscii_Print(temp, x, y);
		return 1;
	}
	else if(id <= 99)
	{
		temp = id/10 + '0';
		LCD_BigAscii_Print(temp, x, y);
		temp = id%10 + '0';
		LCD_BigAscii_Print(temp, x+1, y);
		return 2;	  
	}
	else
	{
		temp = id/100 + '0';
		LCD_BigAscii_Print(temp, x, y);
		temp = (id%100)/10 + '0';
		LCD_BigAscii_Print(temp, x+1, y);
		temp = (id%100)%10 + '0';
		LCD_BigAscii_Print(temp, x+2, y);
		return 3;	
	}
}

void LCD_ListLine_Inv(uint8 line)
{
	LCD_Char_Inv(0, line, LCD_LINE_WIDTH-1);
}

void LCD_Area_Inv(uint8 x_start,uint8 x_end, uint8 y_start,uint8 y_end)
{
	LcdWriteReg(BLTR,0x00);
	LcdWriteReg(SWSXR,x_start);
	LcdWriteReg(SWSYR,y_start);
	LcdWriteReg(SWRXR,(x_end-x_start));
	LcdWriteReg(SWRYR,0x80 | (y_end-y_start));   
	LcdWriteReg(BLTR,0x10);
}

void LCD_Char_Inv(uint8 x, uint8 y, uint8 len)
{
	LcdWriteReg(BLTR,0x00);
	LcdWriteReg(SWSXR,x);
	LcdWriteReg(SWSYR,y*LCD_LINE_HIGH);
	LcdWriteReg(SWRXR,len-1);
	LcdWriteReg(SWRYR,0x80 | LCD_LINE_HIGH);   
	LcdWriteReg(BLTR,0x10);
}
void LCD_Clear_Inv(void)
{
	LcdWriteReg(BLTR,0x00);
}

uint8 LCD_Signal_Show(uint8 index)
{
	if(index > MAX_SIG)
		return INVALID_INDEX;

	LCD_BigAscii_Print(SIGNAL_ASCII, 0, 0);
	if(index>0)
		LCD_BigAscii_Print(SIGNAL_ASCII + index, 1, 0);
	return LCD_SUCCESS;
}
void LCD_GraphicMode_Write(uint8*p, uint8 len,uint8 x, uint8 y,uint8 width)
{
	LcdWriteReg(MWMR,0x0);	
	LcdSetCurXY(x, y);	
        uint8 w = width/8;
	for(uint8 i=0; i<len; i++)
	{
		if(i%w == 0)
			LcdSetCurXY(x,y+i/w);
		LcdWriteMem(p[i]);
		//DelayMs(1);
	  }
}
void LCD_BigCharPrint(char data, uint8 x, uint8 y)
{
	uint8 const *p = NULL;
	uint8 len;
        uint8 width = 16;
	switch(data)
	{
	case '0':
		{
			p = (uint8 const *)(char_0);
			len = sizeof(char_0);
			break;
		}
	case '1':
		{
			p = (uint8 const *)(char_1);
			len = sizeof(char_1);
			break;
		}
	case '2':
		{
			p = (uint8 const *)(char_2);
			len = sizeof(char_2);
			break;
		}
	case '3':
		{
			p = (uint8 const *)(char_3);
			len = sizeof(char_3);
			break;
		}
	case '4':
		{
			p = (uint8 const *)(char_4);
			len = sizeof(char_4);
			break;
		}
	case '5':
		{
			p = (uint8 const *)(char_5);
			len = sizeof(char_5);
			break;
		}
	case '6':
		{
			p = (uint8 const *)(char_6);
			len = sizeof(char_6);
			break;
		}
	case '7':
		{
			p = (uint8 const *)(char_7);
			len = sizeof(char_7);
			break;
		}
	case '8':
		{
			p = (uint8 const *)(char_8);
			len = sizeof(char_8);
			break;
		}
	case '9':
		{
			p = (uint8 const *)(char_9);
			len = sizeof(char_9);
			break;
		}
	case '%':
		{
			p = (uint8 const *)(char_percent);
			len = sizeof(char_percent);
			break;
		}
	case '.':
		{
			p = (uint8 const *)(char_dot);
			len = sizeof(char_dot);
			break;
		}
	case '-':
		{
			p = (uint8 const *)(char_minus);
			len = sizeof(char_minus);
			break;
		}
	case 'C':
		{
			p = (uint8 const *)(char_celsius);
			len = sizeof(char_celsius);
                     width = 32; //
			break;
		}
	case ' ':
		{
			p = NULL;
			len = 0;
		}
	}
	LCD_GraphicMode_Write((uint8*)p,len,x,y,width);
}
/*
void LCD_SMS_ICON_Show(uint8 x, uint8 line_id)
{
uint8 y = 0;
uint8 const *sms_icon = (uint8 const *)SMS_ICON_data;
y = line_id*LCD_LINE_WIDTH;
LCD_GraphicMode_Write((uint8*)sms_icon, x, y);
LCD_CloseCursor();
}
*/
uint8 LCD_Battery_Show(uint8 index)
{
	uint8 const *p = NULL;

	if(index > MAX_BAT)
		return INVALID_INDEX;

	switch(index)
	{
	case NO_BAT:
		p = (uint8 const *)bat_mode_0;
		break;
	case QUA_BAT:
		p = (uint8 const *)bat_mode_1;
		break;
	case HALF_BAT:
		p = (uint8 const *)bat_mode_2;
		break;
	case BIG_BAT:
		p = (uint8 const *)bat_mode_3;
		break;
	case FULL_BAT:
		p = (uint8 const *)bat_mode_4;
		break;
	}
	LCD_GraphicMode_Write((uint8*)p, BATTERY_GRAPH_LEN, 14, 0,16);
	return LCD_SUCCESS;
}

void LCD_ProgBar_open(void)
{      
	LcdWriteReg(MWMR,0x0);
	for(uint8 i=0; i< BAR_LEN; i++)
	{
		LcdSetCurXY(15,BAR_START+i);
		LcdWriteMem(BAR_BLACK);
	}
	barinv_beg = 0;
	barinv_len =0;
}

void LCD_ProgBar_update(uint8 sel_item, uint8 total_item)
{   
	LcdWriteReg(MWMR,0x52);

	if(sel_item < 9)
	{
		LCD_Clear(LCD_LINE_WIDTH-2, 0, LCD_LINE_WIDTH, 0);
		LcdSetCurXY(LCD_LINE_WIDTH-1,0);	
		LcdWriteMem(sel_item + '1');
	}
	else if(sel_item < 99)
	{
		uint8 temp;

		LCD_Clear(LCD_LINE_WIDTH-3, 0, LCD_LINE_WIDTH, 0);
		LcdSetCurXY(LCD_LINE_WIDTH-2,0);	
		temp = (sel_item+1)/10 + '0';
		LcdWriteMem(temp);
		temp = (sel_item+1)%10 + '0';
		LcdWriteMem(temp);
	}
	else
	{
		uint8 temp;

		LCD_Clear(LCD_LINE_WIDTH-3, 0, LCD_LINE_WIDTH, 0);
		LcdSetCurXY(LCD_LINE_WIDTH-3,0);	
		temp = (sel_item+1)/100 + '0';
		LcdWriteMem(temp);
		temp = ((sel_item+1)%100)/10 + '0';
		LcdWriteMem(temp);
		temp = ((sel_item+1)%100)%10 + '0';
		LcdWriteMem(temp);
	}


	//LcdWriteMem('1'+sel_item);

	LcdWriteReg(MWMR,0x0);

	for(uint8 i=0; i< barinv_len; i++)
	{
		LcdSetCurXY(15,BAR_START+barinv_beg+i);
		LcdWriteMem(BAR_BLACK);
	}

	barinv_len = MAX_BARINV_LEN/(total_item+1);
	if(barinv_len < 1)
	{
		barinv_len = 1;
	}
	barinv_beg = (BAR_LEN - barinv_len)*sel_item/(total_item-1);

	for(uint8 i=0; i< barinv_len; i++)
	{
		LcdSetCurXY(15,BAR_START+barinv_beg+i);
		LcdWriteMem(BAR_WHITE);
	}
	return; 
}

void LCDSetBackLightCtl(uint8 ctl)
{
	if(ctl <= BACKLIGHT_CTL_30S)//BACKLIGHT_CTL_ALWAYSON)
	{
		backlight_ctl = ctl;
	}
}

uint8 LCDGetBackLightCtl(void)
{
	return backlight_ctl;
}

void LCDDisplayOff()
{
	LcdWriteReg(PWRR, 0x00);
	return;
}
void LCDIntoSleep()
{
	LcdWriteReg(PWRR, 0x01);
	LCD_CS = 1;
	LCD_SCK = 0;   
	P2DIR &= ~(0x01<<LCD_SDA_BIT);    // LCD_SDA  input
	return;
}
void LCDWakeUp()
{
	LcdWriteReg(PWRR, 0x02);
	return;
}
#if 1
void LCDReset(void)
{
	LCD_RESET = 0;
	DelayMs(80); // 50ms
	LCD_RESET = 1;    
}
#endif
