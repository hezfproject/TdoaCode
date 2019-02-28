#ifndef MENU_ADJUST_UTIL_H
#define MENU_ADJUST_UTIL_H

#define OVERDENSITY_MIN  (1*50)
#define OVERDENSITY_MAX (5*50)
#define OVERDENSITY_INIT (2*50)

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
//void    menu_adjustvol_display(void);
//void     menu_adjustvol_onkey(uint8 keys, uint8 status); 
void HalSetPadLockStat(uint8 val);
uint8  HalGetPadLockStat(void);
void HalSetPadLockEnable(bool val);
bool HalGetPadLockEnable(void );

bool    menu_get_calibration_result(void);
void    menu_calibration_setdensity_display(void);
void    menu_calibration_setdensity_onkey(uint8 keys, uint8 status);
void    menu_settings_overdensity_display(void);
void    menu_settings_overdensity_onkey(uint8 keys, uint8 status);

uint16   menu_get_overalert_density(void);
uint8   menu_set_overalert_density(uint16 density);

void menu_set_adjustdensity(uint16 adjustdensity);
uint16 menu_get_adjustdensity(void);

void    menu_settings_temperature_display(void);
void    menu_settings_temperature_onkey(uint8 keys, uint8 status);
bool    menu_set_temperature(int16 temper);

#endif
