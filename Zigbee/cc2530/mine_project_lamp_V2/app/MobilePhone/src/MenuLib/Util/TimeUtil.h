#ifndef TIME_UTIL_H
#define TIME_UTIL_H

#include "hal_types.h"
#include  "hal_defs.h"
#include "ioCC2530.h"

typedef struct
{
    uint16 hour;
    uint16 min;
    uint16 sec;
}Time_t;

typedef struct
{
    uint16 year;
    uint16 mon;
    uint16 day;
}Date_t;

enum time_format_t
{
    TIME_FORMAT_24,
    TIME_FORMAT_12
};

#define TIME_MAX_YEAR   2099
#define TIME_MIN_YEAR   1970
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

//extern void  ResetTimeDate(void);
extern bool SetTime(Time_t time);
extern void  SyncTime(void);
extern Time_t  GetTime(void);
extern void GetCharFromTime(uint8* p,Time_t time);
extern void GetFullCharFromTime(uint8* p,Time_t time);
extern void  GetTimeChar(uint8* p);
extern void GetFullTimeChar(uint8* p);
extern bool SetDate(Date_t date);
extern Date_t   GetDate(void);
extern void GetCharFromDate(uint8* p, Date_t date);
extern void  GetDateChar(uint8* p);
extern uint8 GetWeek(void);
extern void GetWeekChar(uint8 *p);
extern uint8 GetMaxDayofMonth(uint16 year, uint16 mon);

extern void SetTimeFormat(uint8 val);
extern uint8 GetTimeFormat(void);

#ifdef 	MENU_TIMEUPDATE_CTL
extern void SetTimeAutoUpdate(bool);
extern bool GetTimeAutoUpdate(void);
#endif

#endif

