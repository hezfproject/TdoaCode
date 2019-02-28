/**************************************************************************************************
  Filename:       OSAL_Clock.h
**************************************************************************************************/

#ifndef OSAL_CLOCK_H
#define OSAL_CLOCK_H

#ifdef __cplusplus
extern "C"
{
#endif

/*********************************************************************
 * INCLUDES
 */

/*********************************************************************
 * MACROS
 */

/*********************************************************************
 * CONSTANTS
 */

/*********************************************************************
 * TYPEDEFS
 */

// number of seconds since 0 hrs, 0 minutes, 0 seconds, on the 
// 1st of January 2000 UTC
typedef uint32 UTCTime;

// To be used with 
typedef struct
{
  uint8 seconds;  // 0-59
  uint8 minutes;  // 0-59
  uint8 hour;     // 0-23
  uint8 day;      // 0-30
  uint8 month;    // 0-11
  uint16 year;    // 2000+
} UTCTimeStruct;

/*********************************************************************
 * GLOBAL VARIABLES
 */

/*********************************************************************
 * FUNCTIONS
 */

  /*
   * Updates the OSAL clock and Timers from the MAC 320us timer tick.
   */
  extern void osalTimeUpdate( void );
  
  /*
   * Set the new time.  This will only set the seconds portion
   * of time and doesn't change the factional second counter.
   *     newTime - number of seconds since 0 hrs, 0 minutes, 
   *               0 seconds, on the 1st of January 2000 UTC
   */
  extern void osal_setClock( UTCTime newTime );

  /*
   * Gets the current time.  This will only return the seconds 
   * portion of time and doesn't include the factional second counter.
   *     returns: number of seconds since 0 hrs, 0 minutes, 
   *              0 seconds, on the 1st of January 2000 UTC
   */
  extern UTCTime osal_getClock( void );
  
  /*
   * Converts UTCTime to UTCTimeStruct
   *
   * secTime - number of seconds since 0 hrs, 0 minutes, 
   *          0 seconds, on the 1st of January 2000 UTC
   * tm - pointer to breakdown struct
   */
  extern void osal_ConvertUTCTime( UTCTimeStruct * tm, UTCTime secTime );

/*********************************************************************
*********************************************************************/

#ifdef __cplusplus
}
#endif

#endif /* OSAL_CLOCK_H */
