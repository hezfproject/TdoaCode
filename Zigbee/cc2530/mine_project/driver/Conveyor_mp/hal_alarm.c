#include "hal_Alarm.h"
#include "Comdef.h"

#include "MobilePhone_global.h"
#include "MobilePhone_Menulib.h"
#include "OSAL_Timers.h"
#include "hal_drivers.h"
#include "MenuAdjustUtil.h"
/**************************************************************************************************
*                                                typedef 
**************************************************************************************************/
typedef struct
{
	uint16   timeout[MP_MAX_ALARMNUM];
	uint8     bitmap;
} HAL_alarm_t;

static HAL_alarm_t  Hal_alarm;

/**************************************************************************************************
*                                            function proto 
**************************************************************************************************/
static void HAL_AlarmProcessTimeout ( uint8 alarmtype );

/**************************************************************************************************
*                                          functions 
**************************************************************************************************/

uint8 HAL_AlarmSet ( uint8 alarmtype, uint16 timeout )
{
	if ( alarmtype >= MP_MAX_ALARMNUM )
	{
		return FAILURE;
	}
	Hal_alarm.timeout[alarmtype] = timeout;
	Hal_alarm.bitmap |= BV ( alarmtype );

	return SUCCESS;
}

bool HAL_AlarmIsSeting ( uint8 alarmtype, uint16 timeout )
{
	if ( alarmtype >= MP_MAX_ALARMNUM )
	{
		return false;
	}
	return ( Hal_alarm.bitmap>>alarmtype & 0x01 ) ? true:false;
}

void HAL_AlarmUnSet ( uint8 alarmtype )
{
	if ( alarmtype >= MP_MAX_ALARMNUM )
	{
		return ;
	}
	Hal_alarm.timeout[alarmtype] = 0;
	Hal_alarm.bitmap &= ~ ( BV ( alarmtype ) );
}

void HAL_AlarmPoll ( void )
{
	static uint32 lasttick;
	uint32 tick = osal_GetSystemClock();

	uint16 tickadd = ( uint16 ) ( tick - lasttick );

	for ( uint8 i=0; i< MP_MAX_ALARMNUM; i++ )
	{
		if ( Hal_alarm.bitmap>>i & 0x01 )
		{
			if ( Hal_alarm.timeout[i] < tickadd )
			{
				HAL_AlarmProcessTimeout ( i );
				Hal_alarm.bitmap &= ~BV ( i );
			}
			else
			{
				Hal_alarm.timeout[i] -= tickadd;
			}
		}
	}

	lasttick = tick;
}
void HAL_AlarmProcessTimeout ( uint8 alarmtype )
{
	switch ( alarmtype )
	{
	case MP_ALARM_INITNWK:
	{
               /* if still searching nwk, display no nwk */ 
              HalSetPadLockStat(PADLOCK_UNLOCKED);
              Menu_RefreshNwkDisp();
		break;
	}
	case MP_ALARM_JOIN:
	{
            if(MP_NwkInfo.nwkState == NWK_DETAIL_JOINASSOCING)
           {
                 MP_NwkInfo.nwkState = NWK_DETAIL_INIT;
                MP_DevInfo.CoordPanID = 0xFFFF;
                MP_DevInfo.currentRssi = MP_MIN_RSSI;
           }
            else if(MP_NwkInfo.nwkState == NWK_DETAIL_CELLSWITCHING)
               {
                 MP_NwkInfo.nwkState = NWK_DETAIL_ENDDEVICE;
               }
              Menu_RefreshNwkDisp();
		break;
	}
	case MP_ALARM_WAKE:
	{
		Menu_handle_msg ( MSG_POLL_END, NULL, 0 );
		break;
	}
	}
}

