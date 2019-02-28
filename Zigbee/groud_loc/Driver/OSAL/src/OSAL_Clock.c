/**************************************************************************************************
  Filename:       OSAL_Clock.c
**************************************************************************************************/

/*********************************************************************
 * INCLUDES
 */

#include "comdef.h"
//#include "OnBoard.h"
#include "OSAL.h"
#include "OSAL_Clock.h"

/*********************************************************************
 * MACROS
 */

/*********************************************************************
 * CONSTANTS
 */

// (MAXCALCTICKS * 8) + (max remainder) must be <= (uint16 max), 
// so: (8188 * 8) + 24 <= 65535
#define MAXCALCTICKS  ((uint16)(8188))

#define	BEGYEAR	        2000    //  00:00:00 January 1, 2000
#define	DAY             86400UL // 24 hours * 60 minutes * 60 seconds
#define	IsLeapYear(yr)	(!((yr) % 4) && (((yr) % 100) || !((yr) % 400)))
#define	YearLength(yr)	(IsLeapYear(yr) ? 366 : 365)


/*********************************************************************
 * TYPEDEFS
 */

/*********************************************************************
 * GLOBAL VARIABLES
 */


/*********************************************************************
 * EXTERNAL VARIABLES
 */

/*********************************************************************
 * EXTERNAL FUNCTIONS
 */
extern uint32 GetSysClock(void);

/*********************************************************************
 * LOCAL VARIABLES
 */
static uint32 prevUpdateTime = 0;
static uint16 timeMSec = 0;

// number of seconds since 0 hrs, 0 minutes, 0 seconds, on the
// 1st of January 2000 UTC
UTCTime OSAL_timeSeconds = 0;

/*********************************************************************
 * LOCAL FUNCTION PROTOTYPES
 */
static uint8 monthLength( uint8 lpyr, uint8 mon );

static void osalClockUpdate( uint16 elapsedMSec );

/*********************************************************************
 * FUNCTIONS
 *********************************************************************/

/*********************************************************************
 * @fn      osalTimeUpdate
 *
 * @brief   Uses the free running rollover count of the MAC backoff timer;
 *          this timer runs freely with a constant 320 usec interval.  The
 *          count of 320-usec ticks is converted to msecs and used to update
 *          the OSAL clock and Timers by invoking osalClockUpdate() and
 *          osalTimerUpdate().  This function is intended to be invoked 
 *          from the background, not interrupt level.
 *
 * @param   None.
 *
 * @return  None.
 */
void osalTimeUpdate( void )
{
  uint16 elapsedMSec = 0;

  // Get the free-running count of 320us timer ticks
  uint32 tmp = GetSysClock();
  
  if ( tmp != prevUpdateTime )
  {
    // Calculate the elapsed MSecs of the free-running timer.
    elapsedMSec = (uint16)(tmp - prevUpdateTime);
  
    prevUpdateTime = tmp;
      
    // Update OSAL Clock and Timers
    if ( elapsedMSec )
    {
      osalClockUpdate( elapsedMSec );
      osalTimerUpdate( elapsedMSec );   //更新时间任务队列
    }
  }
}

/*********************************************************************
 * @fn      osalClockUpdate
 *
 * @brief   Updates the OSAL Clock time with elapsed milliseconds.
 *
 * @param   elapsedMSec - elapsed milliseconds
 *
 * @return  none
 */
static void osalClockUpdate( uint16 elapsedMSec )
{
  // Add elapsed milliseconds to the saved millisecond portion of time
  timeMSec += elapsedMSec;

  // Roll up milliseconds to the number of seconds
  if ( timeMSec > 1000 )
  {
    OSAL_timeSeconds += timeMSec / 1000;
    timeMSec = timeMSec % 1000;
  }
}

/*********************************************************************
 * @fn      osal_setClock
 *
 * @brief   Set the new time.  This will only set the seconds portion
 *          of time and doesn't change the factional second counter.
 *
 * @param   newTime - number of seconds since 0 hrs, 0 minutes,
 *                    0 seconds, on the 1st of January 2000 UTC
 *
 * @return  none
 */
void osal_setClock( UTCTime newTime )
{
  OSAL_timeSeconds = newTime;
}

/*********************************************************************
 * @fn      osal_getClock
 *
 * @brief   Gets the current time.  This will only return the seconds
 *          portion of time and doesn't include the factional second
 *          counter.
 *
 * @param   none
 *
 * @return  number of seconds since 0 hrs, 0 minutes, 0 seconds,
 *          on the 1st of January 2000 UTC
 */
UTCTime osal_getClock( void )
{
  return ( OSAL_timeSeconds );
}

/*********************************************************************
 * @fn      osal_ConvertUTCTime
 *
 * @brief   Converts UTCTime to UTCTimeStruct
 *
 * @param   tm - pointer to breakdown struct
 *
 * @param   secTime - number of seconds since 0 hrs, 0 minutes,
 *          0 seconds, on the 1st of January 2000 UTC
 *
 * @return  none
 */
void osal_ConvertUTCTime( UTCTimeStruct *tm, UTCTime secTime )
{
  // calculate the time less than a day - hours, minutes, seconds
  {
    uint32 day = secTime % DAY;
    tm->seconds = day % 60UL;
    tm->minutes = (day % 3600UL) / 60;
    tm->hour = day / 3600UL;
  }

  // Fill in the calendar - day, month, year
  {
    uint16 numDays = secTime / DAY;
    tm->year = BEGYEAR;
    while ( numDays >= YearLength( tm->year ) )
    {
      numDays -= YearLength( tm->year );
      tm->year++;
    }

    tm->month = 0;
    while ( numDays >= monthLength( IsLeapYear( tm->year ), tm->month ) )
    {
      numDays -= monthLength( IsLeapYear( tm->year ), tm->month );
      tm->month++;
    }

    tm->day = numDays;
  }
}

/*********************************************************************
 * @fn      monthLength
 *
 * @param   lpyr - 1 for leap year, 0 if not
 *
 * @param   mon - 0 - 11 (jan - dec)
 *
 * @return  returns the number of days in a month
 */
static uint8 monthLength( uint8 lpyr, uint8 mon )
{
  uint8 days = 31;

	if ( mon == 1 ) // feb
		days = ( 28 + lpyr );
  else
  {
    if ( mon > 6 )
      mon--;

    if ( (mon % 2) == 1 )
      days = 30;
  }

	return ( days );
}
