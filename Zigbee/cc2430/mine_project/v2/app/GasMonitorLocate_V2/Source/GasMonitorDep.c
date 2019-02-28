/**************************************************************************************************
Filename:       GasMonitorDep.c
Revised:        $Date: 2011/06/17 02:52:12 $
Revision:       $Revision: 1.2 $

Description:   Dependend interface for Gas Monitor.
**************************************************************************************************/
/*********************************************************************
* INCLUDES
*/
#include "GasMonitorDep.h"
#include "ZComDef.h"
#include "WatchDogUtil.h"

#include "hal_mcu.h"
#include "hal_adc.h"
#include "hal_key.h"
#include "mac_radio_defs.h"
#include "GasMenulib_global.h"
#include "Lcd_serial.h"
#include "ch4.h"
#include "MenuAdjustUtil.h"
#include "OSAL_Nv.h"
#include "TimeUtil.h"


#include "App_cfg.h"
#include "Delay.h"
#include "WatchDogUtil.h"
#include "hal_types.h"
#include "lcd_interface.h"
#include "ch4.h"
#include "beeper.h"
#include "Drivers.h"
#include "SleepUtil.h"

/*************************************************************************************************
*CONSTANTS
*/
/*************************************************************************************************
*MACROS
*/
#define MAX_VDD_SAMPLES       3
#define VDD_LIMIT   HAL_ADC_VDD_LIMIT_0			/* 3.3v */

#define GASMONITOR_RAM_ADDR			0xFEF0		//0xFEF0-0xFEFF are used to store paramters
/*************************************************************************************************
*MACROS
*/
/* sleep and external interrupt port masks */
#define STIE_BV                             BV(5)
#define P0IE_BV                             BV(5)
#define P1IE_BV                             BV(4)
#define P2IE_BV                             BV(1)
#define IsNumberEqual(termNbr_t1, termNbr_t2) (osal_strcmp(termNbr_t1, termNbr_t2) == 0)

/*********************************************************************
* GLOBAL VARIABLES
*/
typedef struct
{
	Time_t  Time;
	Date_t   Date;
} Gasmonitor_Info_t;



/*********************************************************************
* LOCAL VARIABLES
*/
static __idata bool GasMonitor_PowerOn = TRUE;

static Gasmonitor_Info_t Gasmonitor_Info;

static inline bool GasMonitor_InTestLongPress ( uint16 keys,uint16 TimeOut );
static inline bool GasMonitor_InPowerTestLongPress ( uint16 keys,uint16 TimeOut );

void GasMonitor_PowerOFF ( void )
{
	HAL_DISABLE_INTERRUPTS();
#if (defined WATCHDOG) &&(WATCHDOG == TRUE)
	FeedWatchDog();
#endif
	MAC_RADIO_TURN_OFF_POWER();
	BACKLIGHT_CLOSE();
	LCDIntoSleep();
	ch4_LdoCtrl ( false );
	HalStopBeeper ( 0,true );
	MOTOR_CLOSE();
	GasMonitor_WriteInfoToFlash();
	Gasmonitor_SaveDevDateInfo();
	GasMonitor_ShutDown();
}

void GasMonitor_ShutDown ( void )
{
	GasMonitor_PowerOn = FALSE;
	volatile static __idata bool longpress = FALSE;

	while ( 1 )
	{
		HAL_ENABLE_INTERRUPTS();
		
		UtilSleep( CC2430_PM1, 500000);
		Gasmonitor_UpdateSleepTime();

		longpress = GasMonitor_InPowerTestLongPress ( HAL_KEY_POWER,GASMONITOR_POWER_LONGPRESS_TIMEOUT );

		if ( longpress )
		{
			STARTWATCHDOG ( DOGTIMER_INTERVAL_2MS );
			DelayMs ( 100 );
		}		

		HAL_DISABLE_INTERRUPTS();
	}

}

bool GasMonitor_InPowerTestLongPress ( uint16 keys,uint16 TimeOut )
{
	uint16 testInterval = 100;
	uint16 testnum = TimeOut/testInterval;
	for ( uint16 i=0; i<testnum; i++ )
	{
		uint8 key_tmp = ( ~P1 ) &KEY_MASK;

		if ( ( key_tmp & keys ) == 0 )
		{
			return false;
		}
		DelayMs ( testInterval );
	}
	return true;
}

bool GasMonitor_InTestLongPress ( uint16 keys,uint16 TimeOut )
{
	uint16 testInterval = 300;
	uint16 testnum = TimeOut/testInterval;
	HalHaltBeeper();
	for ( uint16 i=0; i<testnum; i++ )
	{
		uint8 key_tmp = ( ~P1 ) &KEY_MASK;

#if(defined WATCHDOG) &&(WATCHDOG == TRUE)
		FEEDWATCHDOG();
#endif
		if ( ( key_tmp & keys ) == 0 )
		{
			HalResumeBeeper();
			return false;
		}
		DelayMs ( testInterval );
	}

	HalResumeBeeper();
	return true;
}
bool GasMonitor_TestLongPress ( uint16 keys,uint16 TimeOut )
{
	return  GasMonitor_InTestLongPress ( keys, TimeOut );
}

bool GasMonitor_vdd_check ( void )
{
	for ( uint8 i=0; i < MAX_VDD_SAMPLES; i++ )
	{
		if ( HalAdcCheckVdd ( VDD_LIMIT ) )
			return TRUE;
		DelayMs ( 10 );
	}
	return  FALSE;
}

void GasMonitor_LongDelay ( uint16 timeout, uint8 cnt )
{
	for ( uint8 i=0; i<cnt; i++ )
	{
#if (defined WATCHDOG) &&(WATCHDOG==TRUE)
		FeedWatchDog();
#endif
		DelayMs ( timeout );
	}
}

bool GasMonitor_IsPowerOn ( void )
{
	return GasMonitor_PowerOn;
}

void GasMonitor_SetPowerOn ( bool powerOn )
{
	GasMonitor_PowerOn = powerOn;
}

uint8 GasMonitor_InitFlashInfo ( void )
{
	uint8 flag;
	set_info_t info;
	info.backlight_ctl = BACKLIGHT_CTL_10S;
	info.over_density = OVERDENSITY_INIT;
	info.ch4_ctl.slope_a = SLOP_A_INIT;
	info.ch4_ctl.zero_val = ZERO_VAL_INIT;
	info.ch4_ctl.cal_value = CAL_VALUE_INIT;
	info.tyrTemper.temp = TEM_CALIBTEMP_INIT;
	info.tyrTemper.value = TEM_CALIBVAL_INIT;

	flag = osal_nv_write ( GASMONITOR_NV_SETTINGS, 0, sizeof ( info ), &info );

	LCDSetBackLightCtl ( info.backlight_ctl );
	menu_set_overalert_density ( info.over_density );
	ch4_set_ch4ctl ( info.ch4_ctl );
	menu_set_adjustdensity ( info.ch4_ctl.cal_value );
	tem_set_tempCtrl ( info.tyrTemper );
	menu_set_temperature ( info.tyrTemper.temp );
	return flag;
}
uint8 GasMonitor_WriteInfoToFlash ( void )
{
	set_info_t info;
	uint8 flag;

	info.ch4_ctl = ch4_get_ch4ctl();
	info.backlight_ctl = LCDGetBackLightCtl();
	info.over_density = menu_get_overalert_density();
	info.tyrTemper = tem_get_tempCtrl();

	flag = osal_nv_write ( GASMONITOR_NV_SETTINGS, 0, sizeof ( info ), &info );
	return flag;
}

uint8 GasMonitor_ReadInfoFromFlash ( void )
{
	set_info_t info;
	uint8 flag;
	bool isDataNormal = true;
	flag = osal_nv_read ( GASMONITOR_NV_SETTINGS, 0, sizeof ( info ), &info );

	if ( flag == ZSUCCESS )
	{
		if ( info.backlight_ctl > BACKLIGHT_CTL_30S )
		{
			info.backlight_ctl = BACKLIGHT_CTL_10S;
			isDataNormal = false;
		}
		LCDSetBackLightCtl ( info.backlight_ctl );

		if ( info.over_density< OVERDENSITY_MIN ||  info.over_density> OVERDENSITY_MAX )
		{
			info.over_density = OVERDENSITY_INIT;
			isDataNormal = false;
		}
		menu_set_overalert_density ( info.over_density );

		if ( info.ch4_ctl.slope_a < SLOP_A_MIN ||  info.ch4_ctl.slope_a> SLOP_A_MAX )
		{
			info.ch4_ctl.slope_a = SLOP_A_INIT;
			isDataNormal = false;
		}
		if ( info.ch4_ctl.zero_val< ZERO_VAL_MIN || info.ch4_ctl.zero_val> ZERO_VAL_MAX )
		{
			info.ch4_ctl.zero_val = ZERO_VAL_INIT;
			isDataNormal = false;
		}
		if ( info.ch4_ctl.cal_value < CAL_VALUE_MIN || info.ch4_ctl.cal_value> CAL_VALUE_MAX )
		{
			info.ch4_ctl.cal_value = CAL_VALUE_INIT;
			isDataNormal = false;
		}
		ch4_set_ch4ctl ( info.ch4_ctl );
		menu_set_adjustdensity ( info.ch4_ctl.cal_value );

		if ( info.tyrTemper.temp < TEM_CALIBTEMP_MIN || info.tyrTemper.temp > TEM_CALIBTEMP_MAX )
		{
			info.tyrTemper.temp = TEM_CALIBTEMP_INIT;
			isDataNormal = false;
		}
		if ( info.tyrTemper.value< TEM_CALIBVAL_MIN || info.tyrTemper.value > TEM_CALIBVAL_MAX )
		{
			info.tyrTemper.value = TEM_CALIBVAL_INIT;
			isDataNormal = false;
		}
		tem_set_tempCtrl ( info.tyrTemper );
		menu_set_temperature ( info.tyrTemper.temp );

		if ( !isDataNormal )
		{
			flag = osal_nv_write ( GASMONITOR_NV_SETTINGS, 0, sizeof ( info ), &info );
		}

	}
	else // read failed
	{
		GasMonitor_InitFlashInfo();
	}
	return flag;
}


void Gasmonitor_SaveParam2RAM ( void )
{
	* ( ( Gasmonitor_Info_t* ) ( GASMONITOR_RAM_ADDR ) ) = Gasmonitor_Info;
}

void Gasmonitor_ReadParamFromRAM ( void )
{
	Gasmonitor_Info = * ( ( Gasmonitor_Info_t* ) ( GASMONITOR_RAM_ADDR ) );
}

void Gasmonitor_ReadDevDateInfo ( void )
{
	Gasmonitor_ReadParamFromRAM();

	SetTime ( Gasmonitor_Info.Time );
	SetDate ( Gasmonitor_Info.Date );
}

void Gasmonitor_SaveDevDateInfo ( void )
{
       Gasmonitor_Info.Time=GetTime();
	Gasmonitor_Info.Date=GetDate();
       
	Gasmonitor_SaveParam2RAM();
}

void Gasmonitor_UpdateSleepTime ( void )
{
	Gasmonitor_ReadParamFromRAM();

	SyncTime();

       Gasmonitor_SaveDevDateInfo();

}
