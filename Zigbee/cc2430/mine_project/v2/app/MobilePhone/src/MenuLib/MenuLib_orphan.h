#ifndef MENULIB_ORPHAN_H
#define MENULIB_ORPHAN_H

#include "hal_types.h"

extern  void   menu_orphan_nodeID_check(void);
extern  void   menu_orphan_handle_key(uint8 keys, uint8 status);
extern  void  menu_orphan_display(void);
extern  void  menu_set_signal(uint8 idx);
extern  void  menu_set_battery(uint8 idx);
#endif
