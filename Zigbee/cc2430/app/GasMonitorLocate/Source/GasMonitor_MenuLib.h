#ifndef MINEAPP_MENULIB_H
#define MINEAPP_MENULIB_H

#include "hal_types.h"
#include "AppProtocolWrapper.h"

enum KeyStatus
{
	KEY_SHORT_PRESS,
	KEY_LONG_PRESS,
};
enum menu_msg
{
	MSG_INIT_MAIN,
	MSG_NO_POWER,
	MSG_POWERON_ANIM,
	MSG_POWEROFF_ANIM,
	MSG_SOS,
       MSG_SOSALARM,
#ifdef CFG_GAS_SHORTMESSAGE
       MSG_NEW_SHORTMSG,
#endif
};

extern uint8 Menu_Init(void);
extern void Menu_handle_key(uint8 keys, uint8 status);
extern void Menu_handle_msg(uint8 MSG, uint8 *p, uint8 len);

extern void Menu_UpdateTime(void);
extern void Menu_UpdateSignal(uint8 level);
extern void Menu_UpdateBattery(uint8 level);
extern bool  Menu_UpdateGasDensity(uint16 value);
#ifdef CFG_GAS_SHORTMESSAGE
extern void Menu_UpdateSmsIcon(void);
#endif
extern void menu_GasPrint(uint8 n1,uint8 n2, uint8 n3,bool forcePrint);
extern void menu_LocatePrint(uint8 n1,uint8 n2, uint8 n3,bool forcePrint);
#ifdef CFG_GAS_CARDCHECK
extern void menu_CardCheckPrint(uint16 cardnum);
#endif
extern void Menu_ProcessMenuLibEvt(void);
#endif
