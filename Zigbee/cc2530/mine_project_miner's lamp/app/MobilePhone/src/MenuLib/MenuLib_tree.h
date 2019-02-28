#ifndef MENULIB_TREE_H
#define MENULIB_TREE_H

#include "MenuLib_global.h"
#include "hal_types.h"

extern Record_type s_recordtype;

extern void      menu_tree_nodeID_check(void);
extern void      menu_tree_display(void);
extern void      menu_tree_handle_key(uint8 keys, uint8 status);
extern  void    menu_steptochild(uint8 ID, uint8 sel_item);
extern  void     menu_steptoparent(void);
//extern  void     menu_tree_stackclear(void);
extern void  menu_tree_stack_init(void);
extern void  menu_tree_stack_clear(void);

#ifdef CFG_STATION_CHECK
extern void  menu_parse_station_signal(uint16 panid, uint16 shortaddr, uint8 *pInd, uint16 len, int8 rssi);
#endif

#endif
