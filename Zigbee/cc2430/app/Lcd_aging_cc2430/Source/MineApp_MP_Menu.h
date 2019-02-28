/**************************************************************************************************
  Filename:       MineApp_MP_Menu.h
  Revised:        $Date: 2009/08/13 03:44:13 $
  Revision:       $Revision: 1.13 $

  Description:    Menu Application of mobile phone.
  **************************************************************************************************/

#ifndef _MENULIB_H_
#define _MENULIB_H_
#ifdef __cplusplus
extern "C"
{
#endif

enum MP_Event
{
     INIT_NWK_MSG,
     INIT_MAIN_MSG,
     INCOMING_CALL_MSG,
     INCOMING_MESSAGE_MSG,
     DIALING_SUCCESS_MSG,
     MISSED_CALL_MSG,
     NO_POWER_MSG,
};

enum SIG_BAT_STRENTH
{
     SIGNAL_STRENTH=1,
     BATTERY_STRENTH
};

/*********************************************************************
 * CONSTANTS
 */
/*********************************************************************
 * MACROS
 */
 /*To identify calling, called or idle status, For a mp, must be one of the three.*/
/*#define STATUS_IDLE        (0x00)
#define STATUS_CALLING       (0x01)
#define STATUS_CALLIED      (0x02)

extern uint8 CallingStatus;

#define ON_CALLED() (CallingStatus == STATUS_CALLIED)
#define ON_CALLING() (CallingStatus == STATUS_CALLING)
#define ON_IDLE() (CallingStatus == STATUS_IDLE)
*/
/*********************************************************************
 * Function - API
 */
extern uint8 Init_Menu_NV(void);
extern void Get_Num_From_Menu(uint8*);
extern void Handle_Key(uint8);
extern void Display_Msg_Menu(uint8, uint8*);
extern void Update_Signal_Battery(uint8, uint8);
extern void Update_Time(void);
#ifdef __cplusplus
}
#endif
#endif
