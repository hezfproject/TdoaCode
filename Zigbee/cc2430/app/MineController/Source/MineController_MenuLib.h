#ifndef MINECONTROLLER_MENULIB_H
#define MINECONTROLLER_MENULIB_H

#include "hal_types.h"
#include "AppProtocolWrapper.h"

//#define SMS_MAX_NUM 6
#define MAX_CALL_NUM     10
#define MAX_CONTACT_NUM     40
#define MAX_SMS_NUM                       10
#define MAX_SMS_NUM_TEMPLATE     5

enum KeyStatus
{
	KEY_SHORT_PRESS,
      KEY_LONG_PRESS,
};
enum menu_msg
{
     MSG_INIT_NWK,
     MSG_INIT_MAIN,
     MSG_NO_POWER,
     MSG_SET_SUCCESSFUL,
     /*
     MSG_INCOMING_CALL,
     MSG_DIALING_SUCCESS,
     MSG_MISSED_CALL,
     MSG_PAD_LOCK,
     MSG_VOICE_FINISH,
     MSG_SMS_SUCCESS,
     MSG_SMS_FAILED,
     MSG_SMS_INCOMING,
     MSG_POLL_END,
     MSG_POLL_START,
     */
};


extern uint8 Menu_Init(void);
extern void Get_Sleeptime_data(app_Sleep_t* app_data);
extern void Menu_handle_key(uint8 keys, uint8 status);
extern void Menu_handle_msg(uint8 MSG, uint8 *p, uint8 len);

extern void Menu_UpdateTime(void);
extern void Menu_UpdateSignal(uint8 level);
extern void Menu_UpdateBattery(uint8 level);

//extern uint8* Menu_Get_SMS_Data(void);
//extern uint8   Menu_Get_SMS_Len(void);
//extern void Get_Num_From_Menu(uint8*);

///extern uint8 Save_New_SMS(APPWrapper_t*sms);
//extern uint8 Get_SMS_Quantity(void);

//extern void Menu_UpdateNwkLogo(void);
//extern void Menu_SearchNwkFinish(void);
extern void Menu_ProcessMenuLibEvt(void);

//extern void Menu_SetNwkStat(bool val);
//extern bool Menu_IsNwkOn(void);
#endif
