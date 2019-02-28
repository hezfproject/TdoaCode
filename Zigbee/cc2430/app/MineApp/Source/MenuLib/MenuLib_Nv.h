#ifndef MENULIB_NV_H
#define MENULIB_NV_H

#include "Hal_types.h"
#include "AppProtocol.h"
#include "Menulib_global.h"


extern uint8 menu_Contact_ReadContactNum(uint8 * pContactNum);
extern uint8 menu_Contact_ReadContact(Contact_Node *pContactNode, uint8 idx);
extern uint8 menu_Contact_AddContact(const Contact_Node *pContactNode);
extern uint8 menu_Contact_DeleteContact(uint8 idx);
extern uint8 menu_Contact_SearchContactByNum(Contact_Node *pContactNode, uint8* pidx, const uint8* pNum);
#endif
