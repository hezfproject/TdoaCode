/*******************************************************************************
  Filename:       hal_timer.c
  Revised:        $Date: 18:32 2012年5月7日
  Revision:       $Revision: 1.0 $

  Description:    定时器2模块文件

*******************************************************************************/

/*******************************************************************************
* INCLUDES
*/
#include <types.h>
#include <hal_mcu.h>
#include <hal_timer.h>

/*******************************************************************************
* CONSTANTS
*/
#define TIMER_1MS       (1000)
#define TIMER2_PERIOD   ((HAL_CPU_CLOCK_MHZ) * (TIMER_1MS))

#define TIMER2_LBYTE    (0xFF - (TIMER2_PERIOD & 0xFF))
#define TIMER2_HBYTE    (0xFF - (TIMER2_PERIOD >> 8))

/* T2CTRL */
#define LATCH_MODE            BV(3)
#define TIMER2_STATE          BV(2)
#define TIMER2_SYNC           BV(1)
#define TIMER2_RUN            BV(0)
#define TIMER2_INT_PER        BV(0)

/*******************************************************************************
* LOCAL VARIABLES
*/
static TIMER_CALLBACK_PFN s_pfnISR_T2;

/*******************************************************************************
* LOCAL FUNCTIONS DECLARATION
*/

/*******************************************************************************
* @fn      hal_timer2_Init
*
* @brief   初始化T2定时器每1ms一次TICK
*
* @param   none
*
* @return  none
*/
static VOID hal_timer2_Init(VOID)
{
    T2MSEL = 2;                         // period counter

    T2M0 = TIMER2_LBYTE;    // counter 1ms
    T2M1 = TIMER2_HBYTE;

    /* start timer */
    HAL_CLOCK_STABLE();
    T2CTRL |=  TIMER2_RUN;  //写1启动定时器
    while(!(T2CTRL & TIMER2_STATE));

    /* Enable latch mode */
    //定时器到期时，锁定定时器和整个溢出计数器，使可以读T2M1/T2MOVF0/T2MOVF1/T2MOVF2
    T2CTRL |= LATCH_MODE;
}

/*******************************************************************************
* @fn      HAL_TIMER2_Start
*
* @brief   启动T2定时器每1ms一次TICK
*
* @param   input - pfnT2 定时器中断处理回调函数
*
* @return  none
*/
VOID HAL_TIMER2_Start(TIMER_CALLBACK_PFN pfnT2)
{
    s_pfnISR_T2 = pfnT2;

    hal_timer2_Init();

    /* enable timer overflow interrupt *///应该是使能周期中断，而不是溢出中断
    T2IRQM |= TIMER2_INT_PER;

    /* enable timer interrupts */
    T2IE = 1;//中断屏蔽位
}

/*******************************************************************************
* @fn          McuTimer2Isr
*
* @brief       Interrupt service routine for timer2, the OS timer.
*
* @param       none
*
* @return      none
*/
HAL_ISR_FUNCTION(Timer2Isr, T2_VECTOR)
{
    HAL_ENTER_ISR();

    if (T2IRQF & TIMER2_INT_PER)
    {
        if (s_pfnISR_T2)
            (*s_pfnISR_T2)();
        /* clear the interrupt flag */
        T2IRQF &= ~TIMER2_INT_PER;
    }

    T2M0 = TIMER2_LBYTE;    // counter 1ms
    T2M1 = TIMER2_HBYTE;

    HAL_EXIT_ISR();
}


#define CONV_ITOA(x)    ('0'+x)
/* ------------------------------------------------------------------------------------------------
*                                      Variables
* -----------------------------------------------------------------------------------------------*/

static Time_t  SyncedTime = {9,0,0};
static UINT32   SyncedTicks = 0;
static UINT32    SyncSTCnt = 0;

static Time_t   CurrentTime = {9,0,0};
static Date_t   CurrentDate = {TIME_INIT_YEAR,TIME_INIT_MONTH,TIME_INIT_DAY};

static UINT8    Timeformat = 0;

static __code const UINT8  day_list[] = {0,31,28,31,30,31,30,31,31,30,31,30,31};

#if 0
static const char __code monday[] = {"星期一"};
static const char __code tuesday[] = {"星期二"};
static const char __code wednesday[] = {"星期三"};
static const char __code thursday[] = {"星期四"};
static const char __code friday[] = {"星期五"};
static const char __code saturday[] = {"星期六"};
static const char __code sunday[] = {"星期天"};

static const char __code* __code const Time_WeekList[] = {
	monday,
	tuesday,
	wednesday,
	thursday,
	friday,
	saturday,
	sunday,
};
#endif

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
		return false;

	halIntState_t intState;
	HAL_ENTER_CRITICAL_SECTION(intState);
	((UINT8 *) &SyncSTCnt)[0] = ST0;
	((UINT8 *) &SyncSTCnt)[1] = ST1;
	((UINT8 *) &SyncSTCnt)[2] = ST2;
	((UINT8 *) &SyncSTCnt)[3] = 0;
	HAL_EXIT_CRITICAL_SECTION(intState);
	SyncedTime = time;
	return true;
}

void  SyncTime()
{
	UINT32 ticks,delta_tick;

	halIntState_t intState;
	HAL_ENTER_CRITICAL_SECTION(intState);
	((UINT8 *) &ticks)[0] = ST0;
	((UINT8 *) &ticks)[1] = ST1;
	((UINT8 *) &ticks)[2] = ST2;
	((UINT8 *) &ticks)[3] = 0;
	HAL_EXIT_CRITICAL_SECTION(intState);

	delta_tick = ticks;
	if(delta_tick <= SyncSTCnt)
	{
		delta_tick += 0x1000000;
	}
	delta_tick -= SyncSTCnt;

	UINT32 delta_sec = delta_tick>>15; // convert ticks to second.  One tick takes  1/32768 second

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
void SetTimeFormat(UINT8 val)
{
	Timeformat = val;
}
uint8  GetTimeFormat(void)
{
	return Timeformat;
}
void GetCharFromTime(UINT8* p,Time_t time)
{
	UINT8 data;
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

void GetFullCharFromTime(UINT8* p,Time_t time)
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

void  GetTimeChar(UINT8* p)
{
	Time_t time;
	time = GetTime();
	GetCharFromTime(p,time);

}


void GetFullTimeChar(UINT8* p)
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
		return false;
	}
	if(date.mon ==0 || date.mon > 12)
		return false;

	UINT8 max_day = day_list[date.mon];
	if(date.mon==2 && IsLeapYear(CurrentDate.year))
	{
		max_day = 29;
	}
	if(date.day> max_day || date.day==0)
		return false;

	CurrentDate = date;
	return true;
}

Date_t  GetDate(void)
{
	return CurrentDate;
}

void GetCharFromDate(UINT8* p, Date_t date)
{
	UINT8 data;
	UINT16 num = date.year;

	for(INT32 i=3;i>=0;i--)
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

void  GetDateChar(UINT8* p)
{
	Date_t date;
	date = GetDate();
	GetCharFromDate(p,date);

}

uint8 GetWeek(void)
{
	UINT16 y = CurrentDate.year;
	UINT16 m = CurrentDate.mon;
	UINT16 d = CurrentDate.day;
	UINT8 week;

	if(m <= 2)
	{
		y--;
		m += 12;
	}
	week = (d+2*m+3*(m+1)/5+y+y/4-y/100+y/400)%7;   //algorithm from google, http://www.cnblogs.com/sashow/archive/2009/05/21/1486316.html
	return week;
}
/*void GetWeekChar(UINT8 *p)
{
	UINT8 week = GetWeek();
	strcpy ((char*)p, (char *)Time_WeekList[week]);
}*/

uint8 GetMaxDayofMonth(UINT16 year, UINT16 mon)
{
	UINT8 max_day = day_list[mon];
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
		UINT16  yu_1      = (CurrentTime.sec%60);
		UINT16  shang_1 = (CurrentTime.sec/60);

		CurrentTime.sec   = yu_1;
		CurrentTime.min += shang_1;

		if(CurrentTime.min > 59)
		{
			UINT16  yu_2      = (CurrentTime.min%60);
			UINT16  shang_2 = (CurrentTime.min/60);

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
	UINT8 max_day = day_list[CurrentDate.mon];
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

static bool IsLeapYear(UINT16 year)
{
	return (year%400==0)||(year%4==0&&year%100!=0);
}

