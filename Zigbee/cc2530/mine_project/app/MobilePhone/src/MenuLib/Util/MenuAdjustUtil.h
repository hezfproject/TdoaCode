#ifndef MENU_ADJUST_UTIL_H
#define MENU_ADJUST_UTIL_H

enum padlock_stat
{
    PADLOCK_LOCKED,
    PADLOCK_MID,
    PADLOCK_UNLOCKED,
    PADLOCK_ALERT,
};

void   menu_setadjusttime(void);
void    menu_setadjustdate(void);
 void    menu_adjusttime_display(void);
 void    menu_adjusttime_onkey(uint8 keys, uint8 status);
 void    menu_adjustdate_display(void);
 void    menu_adjustdate_onkey(uint8 keys, uint8 status);
void    menu_adjustvol_display(void);
void     menu_adjustvol_onkey(uint8 keys, uint8 status);

void HalSetPadLockStat(uint8 val);
uint8  HalGetPadLockStat(void);

void HalSetPadLockEnable(bool val);
bool HalGetPadLockEnable(void );

#endif
