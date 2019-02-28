#ifndef MENULIB_NV_H
#define MENULIB_NV_H

#include "Hal_types.h"
#include "AppProtocol.h"
#include "Menulib_global.h"

	#define MINEAPP_NV_DIALED        0x0206 
	#define MINEAPP_NV_MISSED        0x0207 
	#define MINEAPP_NV_ANSWERED   0x0208
	
	#define ZCD_NV_SET_CHANLIST     0x0209
	#define ZCD_NV_SET_PANID           0x020A
	#define MINEAPP_NV_SET_INFORMATION        0x020E //et information include volume, backlight and time
	#define MP_NV_R_OR_D_ITEM            		0x020C
	#define MP_STOREPARAM_ITEM                        0x020B
	#define MP_SMS_SEQNUM					0x0214
	#define MP_DISPLAY_PARAMETER		        0x0215
	
#ifdef NEW_DOUBLE_NVID_OP
	#define MINEAPP_NV_CONTACT1      0x0210
	#define MINEAPP_NV_CONTACT2      0x0211
	#define MINEAPP_NV_SMS1                0x0212
	#define MINEAPP_NV_SMS2                0x0213
#endif
#ifdef SMS_TEMPLATE
	#define MINEAPP_NV_SMS_TEMPLATE              0x020D
#endif
#ifdef SMS_SENDBOX
	#define MINEAPP_NV_SMS_SENDBOX                0x020F
#endif
	
extern uint8  menu_Contact_nv_init(void);
extern uint8 menu_Contact_ReadContactNum(uint8 * pContactNum);
extern uint8 menu_Contact_ReadContact(Contact_Node *pContactNode, uint8 idx);
extern uint8 menu_Contact_AddContact(const Contact_Node *pContactNode);
extern uint8 menu_Contact_DeleteContact(uint8 idx);
extern uint8 menu_Contact_SearchContactByNum(Contact_Node *pContactNode, uint8* pidx, const uint8* pNum);
#endif
