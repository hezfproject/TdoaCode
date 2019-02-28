/*******************************************************************************
  Filename:       hal_timer.h
  Revised:        $Date: 18:32 2012年5月7日
  Revision:       $Revision: 1.0 $

  Description:    定时器2模块头文件

*******************************************************************************/
#ifndef _HAL_TIMER_H
#define _HAL_TIMER_H

/*******************************************************************************
* INCLUDES
*/
#include <types.h>

/*******************************************************************************
* TYPEDEFS
*/
typedef VOID (*TIMER_CALLBACK_PFN)(VOID);

/*******************************************************************************
* GLOBAL FUNCTIONS DECLARATION
*/

/*******************************************************************************
* @fn          HAL_TIMER2_Start
*
* @brief       Initialise TIMER-2.
*
*
* @param    INPUT   pfnT2 -User timer process callback,
*                                       this is a os tick timer
*
* @return      none
*/
VOID HAL_TIMER2_Start(TIMER_CALLBACK_PFN pfnT2);


typedef struct
{
    UINT16 hour;
    UINT16 min;
    UINT16 sec;
}Time_t;

typedef struct
{
    UINT16 year;
    UINT16 mon;
    UINT16 day;
}Date_t;

enum time_format_t
{
    TIME_FORMAT_24,
    TIME_FORMAT_12
};

#define TIME_MAX_YEAR   2099
#define TIME_MIN_YEAR   2000
#define TIME_INIT_YEAR  TIME_MIN_YEAR
#define TIME_INIT_MONTH  1
#define TIME_INIT_DAY       1


#define    WEEK_MONDAY          1
#define    WEEK_TUESDAY         2
#define    WEEK_WEDNESDAY       3
#define    WEEK_THURSDAY        4
#define    WEEK_FRIDAY          5
#define    WEEK_SATURDAY        6
#define    WEEK_SUNDAY          7

 #define TIME_MAX_OSCILLATOR       0xFFFF7F

//extern void  ResetTimeDate(void);
extern bool SetTime(Time_t time);
extern void  SyncTime(void);
extern Time_t  GetTime(void);
extern void GetCharFromTime(UINT8* p,Time_t time);
extern void GetFullCharFromTime(UINT8* p,Time_t time);
extern void  GetTimeChar(UINT8* p);
extern void GetFullTimeChar(UINT8* p);
extern bool SetDate(Date_t date);
extern Date_t   GetDate(void);
extern void GetCharFromDate(UINT8* p, Date_t date);
extern void  GetDateChar(UINT8* p);
extern UINT8 GetWeek(void);
//extern void GetWeekChar(UINT8 *p);
extern UINT8 GetMaxDayofMonth(UINT16 year, UINT16 mon);

#ifdef MENU_CLOCKFORMAT
extern void SetTimeFormat(UINT8 val);
extern UINT8 GetTimeFormat(void);
#endif

#ifdef 	MENU_TIMEUPDATE_CTL
extern void SetTimeAutoUpdate(bool);
extern bool GetTimeAutoUpdate(void);
#endif


#endif
