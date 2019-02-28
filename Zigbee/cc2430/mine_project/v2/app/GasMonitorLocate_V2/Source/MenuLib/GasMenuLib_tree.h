#ifndef GASMENULIB_TREE_H
#define GASMENULIB_TREE_H

#include "GasMenuLib_global.h"
#include "hal_types.h"




extern void 	menu_tree_init(void);
extern void      menu_tree_nodeID_check(void);
extern void      menu_tree_display(void);
extern void      menu_tree_handle_key(uint8 keys, uint8 status);
extern  void    menu_steptochild(uint8 ID, uint8 sel_item);
extern  void     menu_steptoparent(void);
extern  void  menu_tree_stack_clear(void);
extern  void menu_TemperPrint(int16 temper, bool forcePrint);
extern void menu_CardInfo_receive(uint16 CardID, int8 rssi);
extern void menu_CardSearch_start(void);
extern void menu_CardSearch_end(void);
extern void menu_shortmessage_delete(uint8 smsSelNum);
#endif
