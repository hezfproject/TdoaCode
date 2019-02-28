/**************************************************************************************************
Filename:       Lcd_aging_check.c
Revised:        $Date: 2011/10/08 15:01:45 $
Revision:       $Revision: 1.0 $

**************************************************************************************************/

#include "Lcd_aging_check.h"
#include "MenuLib_global.h"
#include "hal_types.h"
#include "key.h"
#include "WatchDogUtil.h"
#include "OnBoard.h"
#include "audio_ambe_serial.h"
#include "mac_radio_defs.h"
#include "lcd_serial.h"


static void LcdSetCurXY(uint8 x, uint8 y);
static void LcdWriteReg(uint8 addr, uint8 data);
static void LcdWriteMem(uint8 data);
static inline bool _MP_InTestLongPress(uint16 key,uint16 TimeOut);
static void Delay_with_feeddog(uint16 datax300);

#define WAITING_TIME    (3*300)

static  bool isaging=FALSE;
static  uint8 chars[33];


/****************************************************************
*** Set cursor position
*****************************************************************/
static void LcdSetCurXY(uint8 x, uint8 y)
{
    LcdWriteReg(XCUR, x);
    DelayMs(1);
    LcdWriteReg(YCUR, y);
    DelayMs(1);
}

/****************************************************************
*** Write lcd memory
*****************************************************************/

static void LcdWriteMem(uint8 data)
{
    int8 i = 0;

    LCD_CS = 0;
    DelayUs(20);

    LCD_SCK = 0;
    LCD_SDA = 0;    // RW = 0, write LCD
    DelayUs(8);
    LCD_SCK = 1;
    DelayUs(8);

    LCD_SCK = 0;
    LCD_SDA = 1;    // RS = 1
    DelayUs(8);
    LCD_SCK = 1;
    DelayUs(8);

    for(i = 7; i >= 0; i--)
    {
        LCD_SCK = 0;
        if(data & 0x80)
        {
            LCD_SDA = 1;
        }
        else
        {
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
*** Write Lcd register
*****************************************************************/

static void LcdWriteReg(uint8 addr, uint8 data)
{
    int8 i = 0;

    LCD_CS = 0;

    DelayUs(20);

    LCD_SCK = 0;
    LCD_SDA = 0;    // RW = 0, write LCD
    DelayUs(2);
    LCD_SCK = 1;
    DelayUs(2);

    LCD_SCK = 0;
    LCD_SDA = 0;    // RS = 0;
    DelayUs(2);
    LCD_SCK = 1;
    DelayUs(2);

    for(i = 7; i >= 0; i--)
    {
        LCD_SCK = 0;
        if(addr & 0x80)
        {
            LCD_SDA = 1;
        }
        else
        {
            LCD_SDA = 0;
        }
        addr = addr << 1;
        DelayUs(8);
        LCD_SCK = 1;
        DelayUs(2);

    }

    for(i = 7; i >= 0; i--)
    {
        LCD_SCK = 0;
        if(data & 0x80)
        {
            LCD_SDA = 1;
        }
        else
        {
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

static void LCD_GraphicMode_Write1( uint8 const *p, uint8 len,uint8 x, uint8 y,uint8 width)
{
	LcdWriteReg(MWMR,0x0);	
	LcdSetCurXY(x, y);	
        uint8 w = width/8;
	for(uint8 i=0; i<len; i++)
	{
#if(defined WATCHDOG) &&(WATCHDOG == TRUE)
        FeedWatchDog();
#endif	
		if(i%w == 0)
			LcdSetCurXY(x,y+i/w);
		LcdWriteMem(p[i]);
	  }
}
 
static void Lcd_set_all_screen(uint8 const * pdata)
{
        for(uint8 i=0;i<(4*15);i++)
            {
 #if(defined WATCHDOG) &&(WATCHDOG == TRUE)
        FeedWatchDog();
#endif           
                LCD_GraphicMode_Write1((uint8 const *)(pdata),32,i%15,(i/15)*LCD_LINE_WIDTH,LCD_LINE_WIDTH);
            }     
}

#if 0
static  __code const uint8  transverseline_char[] = {
	0xff,0xff,0x00,0x00,0xff,0xff,0x00,0x00,
       0xff,0xff,0x00,0x00,0xff,0xff,0x00,0x00,
	0xff,0xff,0x00,0x00,0xff,0xff,0x00,0x00,
	0xff,0xff,0x00,0x00,0xff,0xff,0x00,0x00,
};

static  __code const uint8  tverticalline_char[] = {
	0x55,0x55,0x55,0x55,0x55,0x55,0x55,0x55,
       0x55,0x55,0x55,0x55,0x55,0x55,0x55,0x55,
	0x55,0x55,0x55,0x55,0x55,0x55,0x55,0x55,
	0x55,0x55,0x55,0x55,0x55,0x55,0x55,0x55,
	//0x55,0x55,0x55,0x55,0x55,0x55,0x55,0x55,
	//0x55,0x55,0x55,0x55,0x55,0x55,0x55,0x55,
	//0x55,0x55,0x55,0x55,0x55,0x55,0x55,0x55,0x00,0x00
};

static  __code const uint8  black_char[] = {
	0xff,0xff,0xff,0xff,0xff,0xff,0xff,0xff,
       0xff,0xff,0xff,0xff,0xff,0xff,0xff,0xff,
	0xff,0xff,0xff,0xff,0xff,0xff,0xff,0xff,
	0xff,0xff,0xff,0xff,0xff,0xff,0xff,0xff,
};

static  __code const uint8  white_char[] = {
	0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,
       0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,
	0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,
	0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,
};
#endif

static uint8* set_chars(uint8 data1,uint8 data2)
{
        for(uint8 i=0;i<32;i++)
            {
 #if(defined WATCHDOG) &&(WATCHDOG == TRUE)
        FeedWatchDog();
#endif           
                if(((i%4)==0)||((i%4)==1))
                    {
                        chars[i]=data1;
                    }
                    else
                    {
                        chars[i]=data2;
                    }
            }
            chars[32]='\0';
        return chars;    
}

static void Lcd_set_transverseline_all_screen1(void)
{
        Lcd_set_all_screen((uint8 const *)set_chars(0xff,0x00));
}

static void Lcd_set_transverseline_all_screen2(void)
{
        Lcd_set_all_screen((uint8 const *)set_chars(0x00,0xff));
}

static void Lcd_set_tverticalline_all_screen(void)
{
        Lcd_set_all_screen((uint8 const *)set_chars(0x55,0x55));
        Delay_with_feeddog(WAITING_TIME);
        Lcd_set_all_screen((uint8 const *)set_chars(0xaa,0xaa));
        
}

static void Lcd_set_black_all_screen(void)
{
        Lcd_set_all_screen((uint8 const *)set_chars(0xff,0xff));        
}

static void Lcd_set_white_all_screen(void)
{
        Lcd_set_all_screen((uint8 const *)set_chars(0x00,0x00));        
}

static inline bool _MP_InTestLongPress(uint16 key,uint16 TimeOut)
{
    uint16 testInterval = 100;
    uint16 testnum = TimeOut/testInterval;
    for( uint16 i=0; i<testnum; i++)
    {
        DelayMs(testInterval);
        uint8 key_tmp = GetKey();

#if(defined WATCHDOG) &&(WATCHDOG == TRUE)
        FeedWatchDog();
#endif
        if(key_tmp != key)
        {
            return FALSE;
        }
    }
    return TRUE;
}

static void Delay_with_feeddog(uint16 datax300)
{
        uint8 count_300ms=0;
        count_300ms=datax300/300;
 #if(defined WATCHDOG) &&(WATCHDOG == TRUE)
           FeedWatchDog();
#endif        
        while(count_300ms--)
            {
            //DelayMs(100);
            //halSleep(300);
            do_Sleep(100);
            //UtilSleep(2, 300);
 #if(defined WATCHDOG) &&(WATCHDOG == TRUE)
           FeedWatchDog();
#endif
           do_Sleep(100);
 #if(defined WATCHDOG) &&(WATCHDOG == TRUE)
           FeedWatchDog();
#endif
           do_Sleep(100);
            }        
 #if(defined WATCHDOG) &&(WATCHDOG == TRUE)
           FeedWatchDog();
#endif 
}

void Lcd_aging(void)
{    

      backlight_ctrl(false);
       isaging = TRUE;
	//HAL_DISABLE_INTERRUPTS();
#if (defined WATCHDOG) &&(WATCHDOG == TRUE)
	FeedWatchDog();
#endif
	StopAudio();
	AudioIntoSleep();
	MAC_RADIO_TURN_OFF_POWER();   
	//MP_ShutDown();
       MP_turn_off_PA_LNA_use_16mRC();

       while(1) 
        {        
        
                /*   Reset LCD  */
                LcdWriteReg(PWRR, 0xC0);  

                DelayUs(60);
                //Delay_with_feeddog(WAITING_TIME);

                LCDWakeUp();
                
                /*   Initial LCD  */
                InitialLcd(); 

                Lcd_set_transverseline_all_screen1();

                Delay_with_feeddog(WAITING_TIME);

                Lcd_set_tverticalline_all_screen();

                Delay_with_feeddog(WAITING_TIME);
                
                Lcd_set_transverseline_all_screen2(); 
                
                Delay_with_feeddog(WAITING_TIME);
                
                Lcd_set_black_all_screen();

                Delay_with_feeddog(WAITING_TIME);

                Lcd_set_white_all_screen();

                //DelayMs(1000);
 #if(defined WATCHDOG) &&(WATCHDOG == TRUE)
            FeedWatchDog();
#endif
           
        }
}

void Check_LCD_aging_stop_and_reset(void)
{
         volatile static __idata bool longpress = FALSE;
          if(isaging)
           {
                   longpress = _MP_InTestLongPress(HAL_KEY_BACKSPACE,WAITING_TIME);
                   if(longpress)
                   {
                        SystemReset();
                   }         
           }
}

