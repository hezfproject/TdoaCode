/******************************************************************************
 Copyright 2010, Walasey Technologies
 Author: WangKun
******************************************************************************/

#include "NtrxChip.h"

void NtrxPowerdownByDio(void)
{
    uint8 val;

    NtrxSetCalInterval( 0 ) ;               // auto calibrate off
    NtrxSetPollMode( False ) ;
    NtrxSetField( NA_RxCmdStop, True ) ;    // Stop receiver

    // enable DIGIO for wake-up alarm, start the alarm function,
    // rising edge causes the alarm, configure as an input pin
    val = (1 << NA_DioOutValueAlarmEnable_B)
        | (1 << NA_DioAlarmStart_B)
        | (1 << NA_DioAlarmPolarity_B);
    NtrxWriteReg(NA_DioDirection_O, val);

    /* add wake up by dio*/
    NtrxSetField(NA_DioPortWe, 1);

    val = ((1 << NA_EnableWakeUpDio_B)
        | (NA_PowerUpTime1Ticks_C << NA_PowerUpTime_LSB)
        | ((NA_PowerDownModeFull_C & 0x01) << NA_PowerDownMode_B ));

    NtrxWriteReg(NA_EnableWakeUpDio_O, val);

    // turn off BB clock
    NtrxSetField(NA_EnableBbClock, 0);
    NtrxSetField(NA_ResetBbClockGate, 1);
    NtrxSetField(NA_EnableBbCrystal, 0);

    // go to power down mode
    NtrxWriteReg(NA_PowerDown_O, (1 << NA_ResetBbClockGate_B) | (1 << NA_PowerDown_B));
}

/*
 * NtrxPowerdownMode:
 *
 * @Seconds: -input- number of Seconds to stay in powerdown mode 0 means to timelimit
 * NTRXPowerdownMode() sets the RTC alarm time to the requested time in Seconds
 * and puts the transceiver into powerdown mode.
 * Clear-to-send.
 */

void NtrxPowerdownMode( uint32 ticks )
{
    uint8 i ;
    uint32  t ;
    NtrxBufferType rtc[6] ;

    NtrxSetCalInterval( 0 ) ;               // auto calibrate off
    NtrxSetPollMode( False ) ;
    NtrxSetField( NA_RxCmdStop, True ) ;    // Stop receiver

    ticks &= 0xFFFFFFul;

    if( ticks > 0 )
    {
        // Get RTC
        NtrxGetRTC( rtc ) ;

        // Calculate wake up time
        t = ((uint32)rtc[3]<<16) + ((uint16)rtc[2]<<8) + rtc[1] ;
        t += ticks ;
        rtc[0] = (uint8)t ;
        rtc[1] = (uint8)(t>>8) ;
        rtc[2] = (uint8)(t>>16) ;

        // Write wake up time
        for( i = 0 ; i <= NA_WakeUpTimeWe_MSB-NA_WakeUpTimeWe_LSB ; i++ )
        {
            NtrxSetField( NA_WakeUpTimeByte, rtc[i] ) ;
            NtrxSetField( NA_WakeUpTimeWe, (1<<i) ) ;
            NtrxSetField( NA_WakeUpTimeWe, 0 ) ;
        }
        i = (1<<NA_EnableWakeUpRtc_B)|(1<<NA_EnableWakeUpDio_B)|(1<<NA_PowerUpTime_LSB) ;
    }
    else
        i = (1<<NA_EnableWakeUpDio_B) ;

    /*add wake up by dio*/
    NtrxSetField( NA_DioDirection, 0);
    NtrxSetField( NA_DioOutValueAlarmEnable, 1);
    NtrxSetField( NA_DioAlarmPolarity, 1);
    NtrxSetField( NA_DioAlarmStart, 1);
    NtrxSetField( NA_DioPortWe, 1 );

    NtrxWriteReg(NA_EnableWakeUpDio_O, i);

    NtrxWriteReg( NA_PowerDown_O, (1<<NA_ResetBbClockGate_B) ) ;
    NtrxWriteReg( NA_EnableBbCrystal_O, 0x00 ) ;
    NtrxWriteReg( NA_EnableWakeUpRtc_O, i | NA_PowerDownModeFull_C ) ;
    NtrxWriteReg( NA_PowerDown_O, (1<<NA_ResetBbClockGate_B)|(1<<NA_PowerDown_B) ) ;
}
#if 0
void NtrxPowerdownMode( uint32 Seconds )
{
	uint8 i ;
	uint16  t ;
	NtrxBufferType rtc[6] ;

	NtrxSetCalInterval( 0 ) ;				// auto calibrate off
	NtrxSetPollMode( False ) ;
	NtrxSetField( NA_RxCmdStop, True ) ;	// Stop receiver

	if( Seconds > 0 )
	{
		// Get RTC
		NtrxGetRTC( rtc ) ;

		// Calculate wake up time
		if( Seconds > 0x1FFFF )
			Seconds = 0x1FFFF ;
		Seconds <<= 7 ;
		t = ((uint16)rtc[2]<<8)+rtc[1] ;
		Seconds += ( ((uint32)rtc[3]<<16) | t ) ;
		rtc[0] = (uint8)Seconds ;
		rtc[1] = (uint8)(Seconds>>8) ;
		rtc[2] = (uint8)(Seconds>>16) ;

		// Write wake up time
		for( i = 0 ; i <= NA_WakeUpTimeWe_MSB-NA_WakeUpTimeWe_LSB ; i++ )
		{
			NtrxSetField( NA_WakeUpTimeByte, rtc[i] ) ;
			NtrxSetField( NA_WakeUpTimeWe, (1<<i) ) ;
			NtrxSetField( NA_WakeUpTimeWe, 0 ) ;
		}
		i = (1<<NA_EnableWakeUpRtc_B)|(1<<NA_PowerUpTime_LSB) ;
	}
	else
		i = 0 ;

	NtrxWriteReg( NA_PowerDown_O, (1<<NA_ResetBbClockGate_B) ) ;
	NtrxWriteReg( NA_EnableBbCrystal_O, 0x00 ) ;
	NtrxWriteReg( NA_EnableWakeUpRtc_O, i ) ;
	NtrxWriteReg( NA_PowerDown_O, (1<<NA_ResetBbClockGate_B)|(1<<NA_PowerDown_B) ) ;
}
#endif
