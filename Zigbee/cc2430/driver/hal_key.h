/**************************************************************************************************
  Filename:       hal_key.h
  Revised:        $Date: 2010/04/03 22:33:39 $
  Revision:       $Revision: 1.8 $

  Description:    This file contains the interface to the KEY Service.

**************************************************************************************************/

#ifndef HAL_KEY_H
#define HAL_KEY_H

#ifdef __cplusplus
extern "C"
{
#endif

/**************************************************************************************************
 *                                             INCLUDES
 **************************************************************************************************/
#include "hal_board.h"
#include "hal_key_cfg.h"
#if ((defined HAL_AUDIO) && (HAL_AUDIO == TRUE)) || (defined MINE_CONTROLLER) || (defined GASMONITOR_PROJECT)
#include "key_interface.h"
#endif
/**************************************************************************************************
 * TYPEDEFS
 **************************************************************************************************/
typedef void (*halKeyCBack_t) (uint16 keys, uint8 state);

/**************************************************************************************************
 *                                             GLOBAL VARIABLES
 **************************************************************************************************/
extern bool Hal_KeyIntEnable;

/**************************************************************************************************
 *                                             FUNCTIONS - API
 **************************************************************************************************/

/*
 * Initialize the Key Service
 */
extern void HalKeyInit( void );

/*
 * Configure the Key Service
 */
extern void HalKeyConfig( bool interruptEnable, const halKeyCBack_t cback);

/*
 * Read the Key status
 */
extern uint8 HalKeyRead( void);

/*
 * Enter sleep mode, store important values
 */
extern void HalKeyEnterSleep ( void );

/*
 * Exit sleep mode, retore values
 */
extern uint8 HalKeyExitSleep ( void );

/*
 * This is for internal used by hal_driver
 */
extern void HalKeyPoll ( void );

/*
 * This is for internal used by hal_sleep
 */
extern bool HalKeyPressed( void );

#if (defined AUDIO_SERIAL)
uint8 Key2ASCII(uint8 key);
#else
#define HalResetNumber() ResetNumberBuf()

//Get the number in the buffer, and the caller need respond to free the memory.
uint8 HalGetNumbers(uint8** pNmbr);
#endif
/**************************************************************************************************
**************************************************************************************************/

#ifdef __cplusplus
}
#endif

#endif
