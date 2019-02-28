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
#ifdef CFG_GAS_FIND_APPOINT_CARD
void	 GasMonitor_stationID_fromdensity(uint8* pNum10000,uint8* pNum1000,uint8* pNum100, uint8* pNum10,uint8* pNum1,uint16 density);
void menu_find_appoint_card_onkey(uint8 keys, uint8 status);
uint16 Get_appoint_card_number(void);
uint8 Get_adjustgas_invpos(void);
void Clear_appoint_card_number(void);
#endif
void    menu_adjustdate_display(void);
void    menu_adjustdate_onkey(uint8 keys, uint8 status);
//void    menu_adjustvol_display(void);
//void     menu_adjustvol_onkey(uint8 keys, uint8 status);
void HalSetPadLockStat(uint8 val);
uint8  HalGetPadLockStat(void);
void HalSetPadLockEnable(bool val);
bool HalGetPadLockEnable(void );

#ifdef CFG_GAS_DETECTOR
bool    menu_get_calibration_result(void);
void    menu_calibration_setdensity_display(void);
void    menu_calibration_setdensity_onkey(uint8 keys, uint8 status);
void    menu_settings_overdensity_display(void);
void    menu_settings_overdensity_onkey(uint8 keys, uint8 status);
#endif

uint16   menu_get_overalert_density(void);
uint8   menu_set_overalert_density(uint16 density);

void menu_set_adjustdensity(uint16 adjustdensity);
uint16 menu_get_adjustdensity(void);

#ifdef CFG_GAS_TEMPERATURE
void    menu_settings_temperature_display(void);
void    menu_settings_temperature_onkey(uint8 keys, uint8 status);
#endif
bool    menu_set_temperature(int16 temper);

void menu_CardNumAdjust_display(void);
void menu_CardNumAdjust_onkey(uint8 keys, uint8 status);
#endif
