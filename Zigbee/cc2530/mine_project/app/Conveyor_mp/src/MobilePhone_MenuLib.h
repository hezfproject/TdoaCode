#ifndef MINEAPP_MENULIB_H
#define MINEAPP_MENULIB_H

#include "hal_types.h"
#include "AppProtocol.h"
#include "MenuLib_Nv.h"   //MENULIB_NV_H

enum KeyStatus
{
    KEY_SHORT_PRESS,
    KEY_LONG_PRESS,
};
enum menu_msg
{
    MSG_INIT_NWK,
    MSG_INIT_MAIN,
    MSG_INCOMING_CALL,
    MSG_DIALING_SUCCESS,
    MSG_MISSED_CALL,
    MSG_NO_POWER,
    MSG_PAD_LOCK,
    MSG_VOICE_FINISH,
    MSG_SMS_SUCCESS,
    MSG_SMS_FAILED,
    MSG_SMS_INCOMING,
    MSG_POLL_END,
    MSG_POLL_START,
    MSG_REFRESH_SCREEN,
    MSG_POWERON_ANIM,
    MSG_POWEROFF_ANIM,
    MSG_CONVEYOR_INFO_INCOMING,
    MSG_CONVEYOR_FAILED,   
};

extern bool    Menu_Get_SMS_Full_Ring_Flag(void);
extern void    Menu_Set_SMS_Full_Ring_Flag(bool flag);
extern uint8* Menu_Get_SMS_Data(void);
extern uint8   Menu_Get_SMS_Len(void);
extern uint8 Menu_Init(void);
extern uint8* Menu_GetNumBuf(void);
extern uint8* Menu_GetDialNumBuf(void);
extern void Menu_handle_key(uint8 keys, uint8 status);
extern void Menu_handle_msg(uint8 MSG, const char *p, uint8 len);

extern void Menu_UpdateTime(void);
extern void Menu_UpdateSignal(uint8 level);
extern void Menu_UpdateBattery(uint8 level);
#if  1//def 	RSSI_INFORMATION
extern void Menu_UpdateRSSI(int8 rssi);
extern void Menu_UpdateLinkFlag(bool flag);
#endif
#if  1//def PACKAGE_INFORMATION
void Menu_UpdatePackage( uint16 recvpagenum, uint16 errpacknum);
#endif
extern uint8 Save_New_SMS( app_SMS_t*sms);
extern uint8 Get_SMS_Quantity(void);

extern void Menu_RefreshNwkDisp(void);
extern void Menu_ProcessMenuLibEvt(void);

#endif
