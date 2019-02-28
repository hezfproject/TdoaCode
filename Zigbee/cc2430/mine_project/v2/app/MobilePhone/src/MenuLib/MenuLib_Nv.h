#ifndef MENULIB_NV_H
#define MENULIB_NV_H

#include "Hal_types.h"
#include "app_protocol.h"
#include "Menulib_global.h"

	#define MINEAPP_NV_DIALED        0x0206 
	#define MINEAPP_NV_MISSED        0x0207 
	#define MINEAPP_NV_ANSWERED   0x0208
#ifdef NEW_DOUBLE_NVID_OP
	#define MINEAPP_NV_CONTACT1      0x0210
	#define MINEAPP_NV_CONTACT2      0x0211
	#define MINEAPP_NV_SMS1                0x0212
	#define MINEAPP_NV_SMS2                0x0213
#endif

#ifdef MENU_RF_DEBUG
	#define ZCD_NV_SET_CHANLIST     0x020B
	#define ZCD_NV_SET_PANID           0x020C
#endif
#ifdef SMS_TEMPLATE
	#define MINEAPP_NV_SMS_TEMPLATE              0x020D
#endif
#ifdef SMS_SENDBOX
	#define MINEAPP_NV_SMS_SENDBOX                0x020F
#endif
	#define MINEAPP_NV_SET_INFORMATION        0x020E
	
extern uint8  menu_Contact_nv_init(void);
extern uint8 menu_Contact_ReadContactNum(uint8 * pContactNum);
extern uint8 menu_Contact_ReadContact(Contact_Node *pContactNode, uint8 idx);
extern uint8 menu_Contact_AddContact(const Contact_Node *pContactNode);
extern uint8 menu_Contact_DeleteContact(uint8 idx);
extern uint8 menu_Contact_SearchContactByNum(Contact_Node *pContactNode, uint8* pidx, const uint8* pNum);
#endif
