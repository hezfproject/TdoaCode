#ifndef __LCD__
#define __LCD__
#include "hal_types.h"
#include <iocc2530.h>
#include "MobilePhone_Dep.h"

// data type define
/*#ifndef uint8
#define uint8   unsigned char
#endif
#ifndef uint16
#define uint16  unsigned short
#endif
#ifndef uint32
#define uint32  unsigned int
#endif
#ifndef int8
#define int8    signed char
#endif
#ifndef int16
#define int16   short
#endif
#ifndef int32
#define int32   int
#endif
*/
typedef enum
{
      NO_SIG,
       QUA_SIG,
	HALF_SIG,
	 BIG_SIG,
	FULL_SIG,
	MAX_SIG
}SIGNAL;

typedef enum
{
       NO_BAT,
       QUA_BAT,
	HALF_BAT,
	 BIG_BAT,
       FULL_BAT,
	MAX_BAT
}BATTERY;

typedef struct
{
	uint8 x;
	uint8 y;
} UINT8_XY;

typedef enum 
{
    BACKLIGHT_CTL_OFF,
    BACKLIGHT_CTL_10S,    
    BACKLIGHT_CTL_20S,    
    BACKLIGHT_CTL_30S,    
//    BACKLIGHT_CTL_ALWAYSON 
}BACKLIGHT_CTL;

/***************************
// Local macro define
****************************/
// LCD interface define
#define LCD_CS          P1_7
#define LCD_SDA         P1_6
//#define LCD_SCK           P1_5
#define LCD_SCK         P2_0


// LCD register define
#define DWFR            0x00
#define PWRR            0x01
#define SYSR            0x02
#define MWMR            0x03
#define CURCR           0x04
#define XCUR            0x05
#define YCUR            0x06
#define KEYR            0x07
#define KSDR            0x07
#define SWSXR           0x08
#define SWSYR           0x09
#define SWRXR           0x0a
#define SWRYR           0x0b
#define SCOR            0x0c
#define ASCR            0x0d
#define SCCR            0x0e
#define ISR             0x0f
#define CSTR            0x10
#define DRCRA           0x11
#define DRCRB           0x12
#define BLTR            0x13
#define IODR            0x14
#define IODAR           0x15
#define ELCR            0x16
#define CGMI            0x17
#define CGMD            0x18


#define SIGNAL_ASCII 0x80
#define BATTERY_ASCII 0x85
#define BATTERY_GRAPH_LEN 32

#define LCD_MAX_X  4
#define LCD_MAX_Y  16
#define MCU_MAX_X  5
#define MCU_MAX_Y  18

#define LCD_SUCCESS 0
#define INVALID_INDEX 1

#define LCD_LINE_HIGH   16 //the unit is pixel
#define LCD_LINE_WIDTH 16 //the unit is byte

// Initial Lcd
void InitialLcd(void);
//write an ascii char
void LCDWriteChar(uint8 aChar);
//write string to lcd.
void LCDWriteString(char* str, uint8 x, uint8 y);

void LCD_Clear(uint8, uint8, uint8, uint8);

// Clear the graphic of Signal and battery
void LCD_Sig_Bat_Clear(uint8);
//Clear one line of LCD
void LCD_Line_Clear(uint8 line_id);
void LCD_ListLine_Clear(uint8 line);

// Clear display
void LcdClearDisplay(void);

uint8 ascii_scan(uint8*p ,uint8 len);
void LCD_Memory_Print(uint8*, uint8, uint8, uint8);
void LCD_BigAscii_Print(uint8, uint8, uint8);
void LCD_Str_Print(uint8*, uint8, uint8, bool);
uint8 LCD_ID_Show(uint8, uint8, uint8);
void LCD_Str_Print_Pixel(uint8 *pdata, uint8 x, uint8 y);

void LCD_ShowCursor(uint8 x, uint8 y);

void LCD_CloseCursor(void);

void LCD_ListLine_Inv(uint8 line);

void LCD_Line_Inv(uint8);

 void LCD_Char_Inv(uint8 x, uint8 y, uint8 len);
 void LCD_Clear_Inv(void);
uint8 LCD_Signal_Show(uint8);

uint8 LCD_Battery_Show(uint8);
void  LCD_SMS_ICON_Show(uint8, uint8);
//uint8 LCD_Line_Print(uint8* mem, uint8 len, uint8 x, uint8 y);

uint8 LCD_SMS_Print(uint8* mem, uint8 len, uint8 x, uint8 y);

void LCD_ProgBar_open(void);

void  LCD_ProgBar_update(uint8 sel_item, uint8 total_item);

void LCDSetBackLightCtl(uint8 ctl);
uint8 LCDGetBackLightCtl(void);


void LCDDisplayOff(void);

void LCDIntoSleep(void);

void LCDWakeUp(void);

void do_Sleep(uint32 timeout_ms);

void LCD_GraphicMode_Write( uint8 const *p, uint8 len,uint8 x, uint8 y,uint8 width);
void LCD_BigCharPrint(char data, uint8 x, uint8 y);
#endif  // __LCD__

