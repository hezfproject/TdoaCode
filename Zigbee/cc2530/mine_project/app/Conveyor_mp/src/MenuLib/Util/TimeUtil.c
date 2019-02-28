#include "hal_mcu.h"
#include "hal_types.h"
#include "hal_defs.h"
#include "TimeUtil.h"
#include "TimeUtilChinese.h"
#include "string.h"

#define CONV_ITOA(x)    ('0'+x) 
/* ------------------------------------------------------------------------------------------------
*                                      Variables
* -----------------------------------------------------------------------------------------------*/

static Time_t  SyncedTime = {9,0,0};
static uint32   SyncedTicks = 0;
static uint32    SyncSTCnt = 0;

static Time_t   CurrentTime = {9,0,0};
static Date_t   CurrentDate = {TIME_INIT_YEAR,TIME_INIT_MONTH,TIME_INIT_DAY};

static uint8    Timeformat = 0;

static  const uint8  day_list[] = {0,31,28,31,30,31,30,31,31,30,31,30,31};

static const char  monday[] = {MONDAY};
static const char  tuesday[] = {TUESDAY};
static const char  wednesday[] = {WEDNESDAY};
static const char  thursday[] = {THURSDAY};
static const char  friday[] = {FRIDAY};
static const char  saturday[] = {SATURDAY};
static const char  sunday[] = {SUNDAY};

static const char *  const Time_WeekList[] = {
	monday,
	tuesday,
	wednesday,
	thursday,
	friday,
	saturday,
	sunday,
};

#ifdef 	MENU_TIMEUPDATE_CTL
static bool Time_autoUpdate = TRUE;
#endif
/* ------------------------------------------------------------------------------------------------
*                                      function definations
* -----------------------------------------------------------------------------------------------*/
static void UpdateTime(uint16 delta_sec);
static void UpdateDate(void);
static bool IsLeapYear(uint16 year);
/* ------------------------------------------------------------------------------------------------
*                                      functions
* -----------------------------------------------------------------------------------------------*/
/*
void  ResetTimeDate(void)
{
	Time_t time;
	time.hour = 9;
	time.min = 0;
	time.sec = 0;
	SetTime(time);
	SetTimeFormat(TIME_FORMAT_12);

	Date_t date;
	date.year = TIME_INIT_YEAR;
	date.mon = TIME_INIT_MONTH;
	date.day = TIME_INIT_DAY;
	SetDate(date);    
}
*/
bool SetTime(Time_t time)
{

	if(time.hour > 23 || time.min>59 || time.sec>59)
		return FALSE;

	halIntState_t intState;
	HAL_ENTER_CRITICAL_SECTION(intState);
	((uint8 *) &SyncSTCnt)[0] = ST0;
	((uint8 *) &SyncSTCnt)[1] = ST1;
	((uint8 *) &SyncSTCnt)[2] = ST2;
	((uint8 *) &SyncSTCnt)[3] = 0;
	HAL_EXIT_CRITICAL_SECTION(intState);
	SyncedTime = time;
	return TRUE;
}

void  SyncTime()
{
	uint32 ticks,delta_tick;

	halIntState_t intState;
	HAL_ENTER_CRITICAL_SECTION(intState);
	((uint8 *) &ticks)[0] = ST0;
	((uint8 *) &ticks)[1] = ST1;
	((uint8 *) &ticks)[2] = ST2;
	((uint8 *) &ticks)[3] = 0;
	HAL_EXIT_CRITICAL_SECTION(intState);

	delta_tick = ticks;
	if(delta_tick <= SyncSTCnt)
	{
		delta_tick += 0x1000000;
	}
	delta_tick -= SyncSTCnt;

	uint32 delta_sec = delta_tick>>15; // convert ticks to second.  One tick takes  1/32768 second 

	delta_tick &= 0x7FFF; // caculate  ticks under 1s

	SyncedTicks+=delta_tick;

	if(SyncedTicks & ~0x7FFF) // update ticks under 1s
	{
		delta_sec += SyncedTicks>>15;
		SyncedTicks &=0x7FFF;
	}

	UpdateTime(delta_sec);

	SyncedTime = CurrentTime;
	SyncSTCnt = ticks;

	return ;
}

Time_t GetTime(void)
{
	SyncTime();
	return CurrentTime;
}
void SetTimeFormat(uint8 val)
{
	Timeformat = val;
}
uint8  GetTimeFormat(void)
{
	return Timeformat;
}
void GetCharFromTime(uint8* p,Time_t time)
{
	uint8 data;
	if(Timeformat== TIME_FORMAT_12)
	{
		if(time.hour == 0 )
		{
			time.hour = 12;
		}
		else if(time.hour > 12)
		{
			time.hour -= 12;
		}
	}
	data = time.hour/10;
	*p++ = CONV_ITOA(data);
	data = time.hour%10;
	*p++ = CONV_ITOA(data);    
	*p++ = ':';
	data = time.min/10;
	*p++ = CONV_ITOA(data);
	data = time.min%10;
	*p++ = CONV_ITOA(data);  
	*p++ = '\0';
}

void GetFullCharFromTime(uint8* p,Time_t time)
{
	GetCharFromTime(p, time);
	p+= 5;
	if(Timeformat == TIME_FORMAT_12)
	{
		*p++ = ' ';
		if(time.hour < 12)
		{   
			*p++= 'a';
			*p++= 'm';
		}
		else
		{
			*p++= 'p';
			*p++= 'm';

		}
		*p++ = '\0';
	}
}

void  GetTimeChar(uint8* p)
{
	Time_t time;
	time = GetTime();
	GetCharFromTime(p,time);

}


void GetFullTimeChar(uint8* p)
{
	Time_t time;
	time = GetTime();
	GetFullCharFromTime(p,time);

	return;
}

bool SetDate(Date_t date)
{
	if(date.year > TIME_MAX_YEAR || date.year < TIME_MIN_YEAR)
	{
		return FALSE;
	}
	if(date.mon ==0 || date.mon > 12) 
		return FALSE;

	uint8 max_day = day_list[date.mon];
	if(date.mon==2 && IsLeapYear(CurrentDate.year))
	{
		max_day = 29;
	}
	if(date.day> max_day || date.day==0) 
		return FALSE;

	CurrentDate = date;
	return TRUE;
}

Date_t  GetDate(void)
{
	return CurrentDate;
}

void GetCharFromDate(uint8* p, Date_t date)
{
	uint8 data;
	uint16 num = date.year;

	for(int8 i=3;i>=0;i--)
	{
		*(p+i) = CONV_ITOA(num%10);
		num /=10;
	}
	p+= 4;

	*p++ = '.';    
	data = date.mon/10;
	*p++ = CONV_ITOA(data);    
	data = date.mon%10;
	*p++ = CONV_ITOA(data);    
	*p++ = '.';
	data = date.day/10;
	*p++ = CONV_ITOA(data);    
	data = date.day%10;
	*p++ = CONV_ITOA(data);    
	*p++ = '\0';

}

void  GetDateChar(uint8* p)
{
	Date_t date;
	date = GetDate();
	GetCharFromDate(p,date);

}

uint8 GetWeek(void) 
{
	uint16 y = CurrentDate.year;
	uint16 m = CurrentDate.mon;
	uint16 d = CurrentDate.day;
	uint8 week;

	if(m <= 2)
	{
		y--;
		m += 12;
	}
	week = (d+2*m+3*(m+1)/5+y+y/4-y/100+y/400)%7;   //algorithm from google, http://www.cnblogs.com/sashow/archive/2009/05/21/1486316.html
	return week;
}
void GetWeekChar(uint8 *p)
{
	uint8 week = GetWeek();
	strcpy ((char*)p, (char *)Time_WeekList[week]);
}

uint8 GetMaxDayofMonth(uint16 year, uint16 mon)
{
	uint8 max_day = day_list[mon];
	if(mon==2 && IsLeapYear(year))
	{
		max_day = 29;
	}
	return max_day;
}

#ifdef 	MENU_TIMEUPDATE_CTL
void SetTimeAutoUpdate(bool val)
{
	Time_autoUpdate = val;
}
bool GetTimeAutoUpdate(void)
{
	return Time_autoUpdate;
}
#endif
static void UpdateTime(uint16  delta_sec)
{  
	CurrentTime = SyncedTime;
	CurrentTime.sec += delta_sec;
	if(CurrentTime.sec > 59)
	{
		uint16  yu_1      = (CurrentTime.sec%60);
		uint16  shang_1 = (CurrentTime.sec/60);

		CurrentTime.sec   = yu_1;
		CurrentTime.min += shang_1;

		if(CurrentTime.min > 59)
		{
			uint16  yu_2      = (CurrentTime.min%60);
			uint16  shang_2 = (CurrentTime.min/60);

			CurrentTime.min   = yu_2;
			CurrentTime.hour += shang_2;
			if(CurrentTime.hour > 23)
			{
				CurrentTime.hour = 0;
				UpdateDate();
			}
		}
	}
}

static void UpdateDate()
{
	uint8 max_day = day_list[CurrentDate.mon];
	if(CurrentDate.mon==2 && IsLeapYear(CurrentDate.year))
	{
		max_day = 29;
	}
	CurrentDate.day++;
	if(CurrentDate.day > max_day)
	{
		CurrentDate.day = 1;
		CurrentDate.mon++;
		if(CurrentDate.mon > 12)
		{
			CurrentDate.mon = 1;
			CurrentDate.year++;
		}
	}
}

static bool IsLeapYear(uint16 year)
{
	return (year%400==0)||(year%4==0&&year%100!=0);
}


