#include "hal_key_cfg.h"
#include "hal_key.h"
#include "hal_audio.h"
#include "lcd_serial.h"
#include "TimeUtil.h"
#include "MenuAdjustUtil.h"
#include "MineApp_MenuLibChinese.h"
#include "MenuLib_global.h"
#include "MineApp_MP_Function.h"
#include "string.h"

#define  AMPM_AM TRUE
#define  AMPM_PM FALSE

/* Adjust Time */
static Time_t             adjusttime_time;
static uint16              adjusttime_invpos;     // 0: minutes       1:seconds    
static uint8                adjusttime_timeformat;
static bool                 adjusttime_ampm;   

/* Adjust date */
static Date_t            adjustdate_date;
static uint16             adjustdate_invpos;   // 0: year       1: month      2: day

static uint16              adjust_num;   
static uint8                adjust_numcnt;   

/*Adjust volume */
static uint8                adjust_volsetting;

/* pad lock */
static bool                padlock_en = TRUE;
static uint8               padlock_stat = PADLOCK_UNLOCKED;

static void  circle_increase(uint16 *p, uint16 min, uint16 max);
static void  circle_decrease(uint16 *p, uint16 min, uint16 max);
static inline void reset_adjustnum(void);


void   menu_setadjusttime(void )
{
	adjusttime_time = GetTime();
#ifdef MENU_CLOCKFORMAT
	adjusttime_timeformat = GetTimeFormat();
#else
	adjusttime_timeformat = TIME_FORMAT_24;
#endif
	adjusttime_invpos = 0;
	if(adjusttime_timeformat == TIME_FORMAT_12)
	{
		if(adjusttime_time.hour <12)
		{
			adjusttime_ampm = AMPM_AM;
		}
		else
		{
			adjusttime_time.hour -= 12;
			adjusttime_ampm = AMPM_PM;
		}

	}
	reset_adjustnum();

}
void   menu_setadjustdate(void )
{
	adjustdate_date = GetDate();
	adjustdate_invpos = 0;
	reset_adjustnum();
}
void    menu_adjusttime_display(void)
{
	uint8 X_START = 5;
	uint8 Y_START = 2;
	uint8 time_char[TIME_LEN];
	Time_t time = adjusttime_time;

	if(adjusttime_timeformat == TIME_FORMAT_12)
	{
		X_START--;
		if(adjusttime_ampm == AMPM_PM)
		{
			time.hour += 12;
		}    
	}
	GetFullCharFromTime(time_char,time);

	LcdClearDisplay();
	LCD_Str_Print(TIME_CHINA, 6, 0, TRUE);    
	LCD_Str_Print((uint8 *)time_char, X_START, Y_START, TRUE);
	if(adjusttime_invpos ==0)
	{
		LCD_Char_Inv(X_START, Y_START,2);
	}
	else if(adjusttime_invpos ==1)
	{
		LCD_Char_Inv(X_START+3, Y_START,2);
	}
	else 
	{
		LCD_Char_Inv(X_START+6, Y_START,2);
	}

}
void    menu_adjusttime_onkey(uint8 keys, uint8 status)
{
	switch(keys)
	{
	case HAL_KEY_0:
	case HAL_KEY_1:
	case HAL_KEY_2:
	case HAL_KEY_3:
	case HAL_KEY_4:
	case HAL_KEY_5:
	case HAL_KEY_6:
	case HAL_KEY_7:
	case HAL_KEY_8:
	case HAL_KEY_9:

		if(adjusttime_invpos == 0)
		{
			if(adjust_numcnt == 0)
			{
				adjust_num = Key2ASCII(keys) - '0';   
				adjust_numcnt++;
			}
			else
			{
				adjust_num*=10;
				adjust_num += Key2ASCII(keys) - '0';   
				circle_increase(&adjusttime_invpos,0,1);
				adjust_numcnt = 0;                  
			}
			if(adjusttime_timeformat == TIME_FORMAT_12)
			{
				if(adjust_num > 11) 
					adjust_num=11;
			}
			else
			{
				if(adjust_num > 23) 
					adjust_num=23;
			}
			adjusttime_time.hour = adjust_num;
		}        		
		else if(adjusttime_invpos == 1)
		{
			if(adjust_numcnt == 0)
			{
				adjust_num = Key2ASCII(keys) - '0';   
				adjust_numcnt++;
			}
			else
			{
				adjust_num*=10;
				adjust_num += Key2ASCII(keys) - '0';   
				circle_increase(&adjusttime_invpos,0,1);
				adjust_numcnt = 0;                  
			}
			if(adjust_num > 59) 
				adjust_num=59;

			adjusttime_time.min = adjust_num;
		}
		menu_display();
		break;

	case HAL_KEY_UP:
		if(adjusttime_invpos == 0)
		{
#ifdef MENU_CLOCKFORMAT
			if(GetTimeFormat() == TIME_FORMAT_12)
			{
				circle_increase(&adjusttime_time.hour, 0, 11);
			}
                        else
#endif
			{
				circle_increase(&adjusttime_time.hour, 0, 23);
			}
		}
		else if(adjusttime_invpos == 1)
		{
			circle_increase(&adjusttime_time.min, 0, 59);

		}
		else 
		{
			if(adjusttime_timeformat == TIME_FORMAT_12)
			{
				adjusttime_ampm = !adjusttime_ampm;
			}
		}
		menu_display();
		break;
	case HAL_KEY_DOWN:
		if(adjusttime_invpos == 0)
		{
#ifdef MENU_CLOCKFORMAT
			if(GetTimeFormat() == TIME_FORMAT_12)
			{
				circle_decrease(&adjusttime_time.hour, 0, 11);
			}
			else
#endif
			{
				circle_decrease(&adjusttime_time.hour, 0, 23);
			}

		}
		else if(adjusttime_invpos == 1)
		{
			circle_decrease(&adjusttime_time.min, 0, 59);
		}
		else
		{
			if(adjusttime_timeformat == TIME_FORMAT_12)
			{
				adjusttime_ampm = !adjusttime_ampm;
			}
		}
		menu_display();
		break;
	case HAL_KEY_LEFT:
		if(adjusttime_timeformat == TIME_FORMAT_12)
		{
			circle_decrease(&adjusttime_invpos, 0, 2);
		}
		else
		{
			circle_decrease(&adjusttime_invpos, 0, 1);
		}
		reset_adjustnum();
		menu_display();
		break;
	case HAL_KEY_RIGHT:
		if(adjusttime_timeformat == TIME_FORMAT_12)
		{
			circle_increase(&adjusttime_invpos, 0, 2);
		}
		else
		{
			circle_increase(&adjusttime_invpos, 0, 1);
		}
		reset_adjustnum();
		menu_display();
		break;
	case HAL_KEY_SELECT:
		if(adjusttime_timeformat == TIME_FORMAT_12)
		{
			if(adjusttime_ampm == AMPM_PM)
			{
				adjusttime_time.hour += 12;
			}
			SetTime(adjusttime_time);
		}
		else
		{
			SetTime(adjusttime_time);
		}
		//menu_JumpBackWithMark();
	//	strcpy((char *)g_jump_buf,SETTED_CHINA);
       // 	menu_JumptoMenu(MENU_ID_SHOWMESSAGE);
	       menu_setadjustdate();
        	menu_JumptoMenu(MENU_ID_ADJUSTDATE);	
        break;
	case HAL_KEY_BACKSPACE:
		menu_JumpBackWithMark();
		break;
	default:
		break;
	}

}
void    menu_adjustdate_display(void)
{
	const uint8 X_START = 3;
	const uint8 Y_START = 2;

	const uint8  year_pos = X_START;
	const uint8  month_pos = year_pos + 5;
	const uint8  day_pos = month_pos + 3;


	uint8 p[DATE_LEN];

	GetCharFromDate(p,adjustdate_date);
	LcdClearDisplay();
	LCD_Str_Print(DATE_CHINA, 6, 0, TRUE);    
	LCD_Str_Print((uint8 *)p, X_START, Y_START, TRUE);

	if(adjustdate_invpos ==0)  // year
	{
		LCD_Char_Inv(year_pos, Y_START,4);
	}
	else if(adjustdate_invpos ==1)
	{
		LCD_Char_Inv(month_pos, Y_START,2);
	}
	else if(adjustdate_invpos ==2)
	{
		LCD_Char_Inv(day_pos, Y_START,2);
	}

}
void    menu_adjustdate_onkey(uint8 keys, uint8 status)
{
	switch(keys)
	{
	case HAL_KEY_0:
	case HAL_KEY_1:
	case HAL_KEY_2:
	case HAL_KEY_3:
	case HAL_KEY_4:
	case HAL_KEY_5:
	case HAL_KEY_6:
	case HAL_KEY_7:
	case HAL_KEY_8:
	case HAL_KEY_9:

		if(adjustdate_invpos == 0)
		{
			if(adjust_numcnt == 0)
			{
				adjust_num = Key2ASCII(keys) - '0';   
				adjust_numcnt++;
			}
			else if(adjust_numcnt < 3)
			{
				adjust_num*=10;
				adjust_num += Key2ASCII(keys) - '0';   
				adjust_numcnt++;
			}
			else
			{
				adjust_num*=10;
				adjust_num += Key2ASCII(keys) - '0';   
				adjustdate_invpos++;
				adjust_numcnt = 0;           

				if(adjust_num > TIME_MAX_YEAR) 
					adjust_num=TIME_MAX_YEAR;
				if(adjust_num < TIME_MIN_YEAR) 
					adjust_num=TIME_MIN_YEAR;

			}
			adjustdate_date.year = adjust_num;
		}        		
		else if(adjustdate_invpos == 1)
		{
			if(adjust_numcnt == 0)
			{
				adjust_num = Key2ASCII(keys) - '0';   
				adjust_numcnt++;
			}
			else
			{
				adjust_num*=10;
				adjust_num += Key2ASCII(keys) - '0';   
				adjustdate_invpos++;
				adjust_numcnt = 0;                  
			}

			if(adjust_num > 12) 
				adjust_num=12;
			adjustdate_date.mon = adjust_num;		
		}
		else if(adjustdate_invpos == 2)
		{
			if(adjust_numcnt == 0)
			{
				adjust_num = Key2ASCII(keys) - '0';   
				adjust_numcnt++;
			}
			else
			{
				adjust_num*=10;
				adjust_num += Key2ASCII(keys) - '0';   
				adjustdate_invpos = 0;
				adjust_numcnt = 0;                  
			}
			uint8 maxday = GetMaxDayofMonth(adjustdate_date.year, adjustdate_date.mon);
			if(adjust_num > maxday) 
				adjust_num=maxday;

			adjustdate_date.day = adjust_num;		
		}
		menu_display();
		break;

	case HAL_KEY_UP:
		{
			if(adjustdate_invpos == 0)
			{
				circle_increase(&adjustdate_date.year, TIME_MIN_YEAR, TIME_MAX_YEAR);
			}
			else if(adjustdate_invpos == 1)
			{
				circle_increase(&adjustdate_date.mon, 1, 12);
			}
			else
			{
				circle_increase(&adjustdate_date.day, 1, GetMaxDayofMonth(adjustdate_date.year,adjustdate_date.mon));
			}
			menu_display();
			break;
		}
	case HAL_KEY_DOWN:
		if(adjustdate_invpos == 0)
		{
			circle_decrease(&adjustdate_date.year, TIME_MIN_YEAR, TIME_MAX_YEAR);
		}
		else if(adjustdate_invpos ==1)
		{
			circle_decrease(&adjustdate_date.mon, 1, 12);
		}
		else
		{
			circle_decrease(&adjustdate_date.day, 1, GetMaxDayofMonth(adjustdate_date.year, adjustdate_date.mon));
		}
		menu_display();
		break;
	case HAL_KEY_LEFT:
		if(adjustdate_invpos == 0)
		{
			if(adjustdate_date.year > TIME_MAX_YEAR)
				adjustdate_date.year = TIME_MAX_YEAR; 
			if(adjustdate_date.year < TIME_MIN_YEAR)
				adjustdate_date.year = TIME_MIN_YEAR; 
          }
			circle_decrease(&adjustdate_invpos, 0, 2);
			reset_adjustnum();
			menu_display();
		break;
	case HAL_KEY_RIGHT:
		if(adjustdate_invpos == 0)
		{
			if(adjustdate_date.year > TIME_MAX_YEAR)
				adjustdate_date.year = TIME_MAX_YEAR; 
			if(adjustdate_date.year < TIME_MIN_YEAR)
				adjustdate_date.year = TIME_MIN_YEAR; 
		}
		circle_increase(&adjustdate_invpos, 0, 2);
		reset_adjustnum();
		menu_display();
		break;
	case HAL_KEY_SELECT:
		SetDate(adjustdate_date);
		//menu_JumpBackWithMark();
		strcpy((char *)g_jump_buf,SETTED_CHINA);
        menu_JumptoMenu(MENU_ID_SHOWMESSAGE);
        break;
    case HAL_KEY_BACKSPACE:
		menu_JumpBackWithMark();
		break;
	default:
		break;
	}

}

void menu_adjustvol_display(void)
{
	if(FIRSTTIME_INTO_NODE())
	{
		LcdClearDisplay();
		LCD_Str_Print((uint8 *)VOLUME_CHINA, (LCD_LINE_WIDTH-osal_strlen(VOLUME_CHINA))/2, 0, TRUE); 
		set_info_t set_info;
		MP_SettingInformation_ReadFlash(&set_info);
        adjust_volsetting = set_info.sound_gain;
		AudioSetOutputGain(adjust_volsetting);
		 HalRingSetGain(RING_TYPE_SOUND, adjust_volsetting);
	}
	LCD_BigAscii_Print(0x90, 2, 2);
	LCD_BigAscii_Print(0x8B, 3, 2);

	for(uint8 i=0;i < adjust_volsetting;i++)
	{
		LCD_BigAscii_Print(0x0D, 5+2*i, 2);
		LCD_BigAscii_Print(0x0D, 5+2*i+1, 2);
	}   
	for(uint8 i = adjust_volsetting;i<4;i++)
	{
		LCD_BigAscii_Print(0x00, 5+2*i, 2);
		LCD_BigAscii_Print(0x00, 5+2*i+1, 2);
	}  
}
void menu_adjustvol_onkey(uint8 keys, uint8 status)
{
	switch(keys)
	{
	set_info_t set_info;
	case HAL_KEY_RIGHT:
		if(adjust_volsetting < MAX_GAIN_LEVEL-1)
		{
			adjust_volsetting++;
			MP_SettingInformation_ReadFlash(&set_info);
	        set_info.sound_gain = adjust_volsetting;
			MP_SettingInformation_Handout(&set_info);
			MP_SettingInformation_WriteFlash(&set_info);
		}
		NearLastNodeID = CurrentNodeID;
		menu_display();
		break;

	case HAL_KEY_LEFT:
		if(adjust_volsetting > 0)
		{
			adjust_volsetting--;
			MP_SettingInformation_ReadFlash(&set_info);
	        set_info.sound_gain = adjust_volsetting;
			MP_SettingInformation_Handout(&set_info);
			MP_SettingInformation_WriteFlash(&set_info);
		}
		NearLastNodeID = CurrentNodeID;
            menu_display();
		break;
	default:
		break;
	}
      // osal_start_timerEx(MineApp_Function_TaskID, MINEAPP_MENULIB_EVENT, 1000);
       MineApp_StartMenuLibEvt (1000);
}


void HalSetPadLockStat(uint8 val)
{
    padlock_stat = val;
}
uint8  HalGetPadLockStat(void)
{
    return padlock_stat;
}
void HalSetPadLockEnable(bool val)
{
    padlock_en = val;
}
bool HalGetPadLockEnable(void )
{
    return padlock_en;
}

/* static functions */
static void circle_increase(uint16 *p, uint16 min, uint16 max)
{
	if(*p < max)
	{
		++*p;
	}
	else
	{
		*p  =  min;
	}
}
static void circle_decrease(uint16 *p, uint16 min, uint16 max)
{
	if(*p > min)
	{
		--*p;
	}
	else
	{
		*p = max;
	}
}
inline static void reset_adjustnum()
{
	adjust_num = 0;
	adjust_numcnt= 0;
}
