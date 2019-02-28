/******************************************************************************
 Copyright 2010, Walasey Technologies
 Author: WangKun
******************************************************************************/
#include "NtrxConfig.h"
#include "NtrxChip.h"
#include "Console.h"
#include <stdio.h>
//#include "stm32f10x.h"
//#include "NtrxChip.h"

//=============================================================================
// NtrxSetStaAddress() : sets the MAC address in the chip.
// Address: -input- 6 byte address field containing the MAC address
// This address is used for address matching when this feature is enabled.
void NtrxSetStaAddress( NtrxDevPtr Address )
{
	NtrxSetLongField( NA_RamStaAddr0, Address ) ;
}

//=============================================================================

const NtrxFlashCode NtrxInitShadowReg[] =
{
	  0,   0,   0,   0,   0,   0,   0,   6,		// 0x00
	  0,   0,   0,   0,   0,   0,   0,   0,		// 0x08
	  0,   0,   0,   0,   0,   0,  64,   0,		// 0x10
	 32,  64,   0,  32,   0,   0,   0,   3,		// 0x18
	  6, 152,  12,   0,   2,  63,  30,   6,		// 0x20
	  0,   0, 171,  44, 213, 146, 148, 202,		// 0x28
	105, 171,  48,   0,   0,   0,   0,   0,		// 0x30
	  0,   0,   0,   0, 224,   4,   0,   1,		// 0x38
	  3,   7,   0,   3,  63,  63,  15,  15,		// 0x40
	115,   0,  16,  16,  67,  20,  16,   0,		// 0x48
	  0, 127,   0,   0,   0,   0,   0,   0,		// 0x50
	  0,   0,  11,  95,   5,   7, 213,  98,		// 0x58
	  0,   0,   0,  12,  10,   0,   0,   0,		// 0x60
	  0,   0,   0,   0,   0,   0,   0,   0,		// 0x68
	  0,   0,   0,   0,   0,   0,   0,   0,		// 0x70
	  0,   0,   0,   0,   0,  80,   0,   0 		// 0x78
} ;

//-----------------------------------------------------------------------------

void NtrxReset( void )
{
	NtrxResetOn() ;
	Delay_us( 500 ) ;
	NtrxResetOff() ;
	Delay_us( 500 ) ;
}

Bool NtrxCheckVerRev( void )
{
	if( NtrxReadReg( NA_Version_O ) != NA_Version_I )	//default value is 0x5
		return False ;
	if( NtrxReadReg( NA_Revision_O ) != NA_Revision_I )	//default value is 0x1
		return False ;
	return True ;
}

void NtrxInitShadowRegister( void )
{
	uint8 i ;
	for( i = 0 ; i < sizeof( NtrxInitShadowReg ) ; i ++ )
		NtrxSetShadowReg( i, NtrxReadFlash( &NtrxInitShadowReg[i] ) ) ;
}

//-----------------------------------------------------------------------------

void NtrxRestart( void )
{
	// Clear All Interrupts
	NtrxSetField( NA_TxIntsReset, 0xFF ) ;
	NtrxSetField( NA_RxIntsReset, 0xFF ) ;

	// Allow some events in the receiver to trigger an interrupt
	if( NtrxGetField( NA_RxCrc2Mode ) )
		NtrxSetField( NA_RxIntsEn, (0x01<<NA_RxEnd_B) ) ;
	else
		NtrxSetField( NA_RxIntsEn, (0x01<<NA_RxEnd_B)|(0x01<<NA_RxOverflow_B) ) ;
	NtrxSetField( NA_TxIntsEn, (0x01<<NA_TxEnd_B) ) ;

	// Allow the the nanoNET TRX chip to drive the interrupt line
	NtrxSetField( NA_RxIrqEnable, True ) ;
	NtrxSetField( NA_TxIrqEnable, True ) ;

	// Start the receiver of the TRX chip
	NtrxSetField( NA_RxCmdStart, True ) ;

	// Select message type date for transmitted messages
	NtrxSetField( NA_TxPacketType, NA_TypeCodeData_VC_C ) ;

	// Enable retransmissions
	NtrxSetField( NA_TxArq, True ) ;

	// Init Variables
	BaseBandTimerIrq = False ;
	NtrxTxEndIrq = False ;
	NtrxRxEndIrq = False ;
	NtrxTransmitState = TxIDLE ;
	NtrxRangingState = RANGING_READY ;
}

//-----------------------------------------------------------------------------

void NtrxFctCal( void )
{
	uint8 CapsVal = 6 ;
	uint8 FctSum = 0 ;
	uint8 i ;

	NtrxWriteReg( NA_ChirpFilterCaps_O, (1<<NA_FctClockEn_B)|CapsVal ) ;
	Delay_us( 10 ) ;
	while( CapsVal < 16 )
	{
		FctSum = 0 ;
		for( i = 0 ; i < 4 ; i ++ )
		{
			NtrxWriteReg( NA_ChirpFilterCaps_O, (1<<NA_FctClockEn_B)|(1<<NA_StartFctMeasure_B)|CapsVal ) ;
			Delay_us( 15 ) ;
			FctSum += NtrxGetField( NA_FctPeriod ) ;
		}
		if( FctSum >= 174 )			// targetValMax
			CapsVal -- ;
		else if( FctSum <= 152 )	// targetValMin
			CapsVal ++ ;
		else
		{
			NtrxWriteReg( NA_ChirpFilterCaps_O, CapsVal ) ;
			return ;
		}
	}
	NtrxWriteReg( NA_ChirpFilterCaps_O, 6 ) ;
}

//-----------------------------------------------------------------------------

#ifdef CONFIG_22MHZ_DEF_ONLY

	const NtrxFlashCode NtrxInitTable1[] =
	{
		NA_RxCmdStop, True,
		NA_RxIntsReset, 0xFF,
		NA_RxBufferCmd, 3,

		NA_ResetBbRadioCtrl, True,
		NA_ResetBbRadioCtrl, False,
		NA_EnableBbClock, False,
		NA_ResetBbClockGate, True,
		NA_EnableBbCrystal, False,
		NA_ResetBbClockGate, True,
		NA_EnableBbCrystal, True,

		NA_DELAY, 5,

		NA_ResetBbClockGate, False,
		NA_EnableBbClock, True,
		NA_ResetBbRadioCtrl, True,
		NA_ResetBbRadioCtrl, False,
		NA_GateAdjFramesyncEn, False,
		NA_GateAdjBitsyncEn, False,

		// Set AGC Values
		NA_AgcHold, True,
		NA_AgcDefaultEn, True,
		NA_AgcValue, 35,
		NA_PulseDetDelay, 2,
		NA_GateSizeFramesync, NA_GateSize9Slots_VC_C,
	
		NA_UseFec, True,
		NA_TABLE_END
	} ;

	const NtrxFlashCode NtrxInitTxTable2[] =
	{
		NA_EnableExtPA, True,
		NA_CsqUsePhaseShift, False,
		NA_TABLE_END
	} ;

	void NtrxInitTRxMode( uint8 bw, uint8 sd, uint8 sr )
	{
		NtrxCurrentChannel = 1;
	
		NtrxSetIndexReg( 0x00 ) ;
		NtrxProcessTable( NtrxInitTable1 ) ;
		NtrxSetIndexReg( 0x00 ) ;
		NtrxSetCorrThreshold( bw, sd ) ;
		NtrxProcessTable( NtrxInitTxTable2 ) ;
		NtrxSetIndexReg( 0x00 ) ;
		NtrxFctCal() ;
		//NtrxAllCalibration() ;
		NtrxRestart() ;
	}
	
#else

	const NtrxFlashCode NtrxInitTable1[] =
	{
		NA_ResetBbClockGate, True,
		NA_EnableBbCrystal, True,
		NA_DELAY, 5,
		NA_ResetBbClockGate, False,
		NA_EnableBbClock, True,
		NA_ResetBbRadioCtrl, True,
		NA_ResetBbRadioCtrl, False,
		NA_TABLE_END
	} ;

	const NtrxFlashCode NtrxInitRxTable2[] =
	{
		NA_RxArqMode, NA_RxArqModeCrc2_VC_C,
		NA_RxTimeBCrc1Mode, False, 
		NA_RxCrc2Mode, True,
		NA_RfRxCompValueI, 14,
		NA_RfRxCompValueQ, 14,
		NA_TABLE_END
	} ;

	const NtrxFlashCode NtrxInitRxTable3[] =
	{
		NA_DeviceSelect, NA_DeviceSelectBbRam0_C,
		NA_HoldAgcInFrameSync, True,
		NA_HoldAgcInBitSync, 24,
		NA_UseAlternativeAgc, False,
		NA_ChirpFilterCaps, 0,
		NA_GateAdjFramesyncEn, False,
		NA_GateAdjBitsyncEn, False,
		NA_TABLE_END
	} ;

	const NtrxFlashCode NtrxInitTxTable2[] =
	{
		NA_TxArq, True,
		NA_TxArqMax, NA_MAX_ARQ_CNT,
		NA_TxScrambEn, True,
		NA_EnableExtPA, True,
		#ifdef NT_SNIFFER
		NA_RxAddrMode, False,
		#else
		NA_RxAddrMode, True,
		#endif
		NA_CsqUsePhaseShift, False,
		NA_EnableCsqClock, True,
		NA_CsqUseRam,  True,
		NA_CsqAsyMode, False,
		NA_TxOutputPower0, RfTxDataOutputPower,
		NA_TxOutputPower1, RfTxReqOutputPower,
		NA_TABLE_END
	} ;

	const NtrxFlashCode NtrxInitTxTable3[] =
	{
		NA_EnableLO, False,
		NA_EnableLOdiv10, False,
		NA_TxAddrSlct, 0,
		NA_TABLE_END
	} ;

	const NtrxFlashCode NtrxInitTable4[] =
	{
		NA_DeviceSelect, NA_DeviceSelectBbRam0_C,
		NA_DELAY, 2,
		NA_TABLE_END
	} ;

	void NtrxInitTRxMode( uint8 bw, uint8 sd, uint8 sr )
	{
		if( bw == NA_22MHz )
			NtrxCurrentChannel = 1;
		else
			NtrxCurrentChannel = 0;

		NtrxProcessTable( NtrxInitTable1 ) ;
		NtrxProcessTable( NtrxInitRxTable2 ) ;
		NtrxSetRxIQMatrix( bw, sd ) ;
		NtrxProcessTable( NtrxInitRxTable3 ) ;
		NtrxSetCorrThreshold( bw, sd ) ;
		NtrxProcessTable( NtrxInitTable4 ) ;
		NtrxProcessTable( NtrxInitTxTable2 ) ;
		NtrxSetTxIQMatrix( bw, sd ) ;
		if( bw != NA_22MHz_HR )
			NtrxFctCal ();
		else
			NtrxWriteReg( NA_ChirpFilterCaps_O, 15 ) ;
		NtrxProcessTable( NtrxInitTxTable3 ) ;
		NtrxAllCalibration() ;
		NtrxSetAgcValues( bw, sd, sr ) ;
		NtrxSetField( NA_UseFec, True) ;
		NtrxRestart() ;
		//NtrxAllCalibration() ;
	}

#endif /* CONFIG_22MHZ_DEF_ONLY */

//=============================================================================

uint8 NtrxSetupTRxMode( NtrxTRxMode Mode )
{
	uint8 bw, sd, sr ;

	NtrxRangeConst = 0 ;

	switch( Mode )
	{
		#ifdef CONFIG_NTRX_80MHZ_500NS
			case NtrxMode80MHz500ns125k :
				bw = NA_80MHz ; sd = NA_500ns ; sr = NA_125k_S ; break ;
			case NtrxMode80MHz500ns1M :
				bw = NA_80MHz ; sd = NA_500ns ; sr = NA_1M_S ; break ;
			case NtrxMode80MHz500ns2M :
				bw = NA_80MHz ; sd = NA_500ns ; sr = NA_2M_S ; NtrxRangeConst = 68.929336 ;	break ;
		#endif
		#ifdef CONFIG_NTRX_80MHZ_1000NS
			case NtrxMode80MHz1us500k :
				bw = NA_80MHz ; sd = NA_1us ; sr = NA_500k_S ; break ;
			case NtrxMode80MHz1us1M :
				bw = NA_80MHz ; sd = NA_1us ; sr = NA_1M_S ; NtrxRangeConst = 122.492363 ; break ;
		#endif
		#ifdef CONFIG_NTRX_80MHZ_2000NS
			case NtrxMode80MHz2us125k :
				bw = NA_80MHz ; sd = NA_2us ; sr = NA_125k_S ; break ;
			case NtrxMode80MHz2us250k :
				bw = NA_80MHz ; sd = NA_2us ; sr = NA_250k_S ; break ;
			case NtrxMode80MHz2us500k :
				bw = NA_80MHz ; sd = NA_2us ; sr = NA_500k_S ; NtrxRangeConst = 229.490053 ; break ;
		#endif
		#ifdef CONFIG_NTRX_80MHZ_4000NS
			case NtrxMode80MHz4us125k :
				bw = NA_80MHz ; sd = NA_4us ; sr = NA_125k_S ; break ;
			case NtrxMode80MHz4us250k :
				bw = NA_80MHz ; sd = NA_4us ; sr = NA_250k_S ; NtrxRangeConst = 445.584595 ; break ;
		#endif
		#ifdef CONFIG_NTRX_22MHZ_1000NS
			case NtrxMode22MHz1us1M :
				bw = NA_22MHz ; sd = NA_1us ; sr = NA_1M_S ; break ;
		#endif
		#ifdef CONFIG_NTRX_22MHZ_2000NS
			case NtrxMode22MHz2us125k :
				bw = NA_22MHz ; sd = NA_2us ; sr = NA_125k_S ; break ;
			case NtrxMode22MHz2us250k :
				bw = NA_22MHz ; sd = NA_2us ; sr = NA_250k_S ; break ;
			case NtrxMode22MHz2us500k :
				bw = NA_22MHz ; sd = NA_2us ; sr = NA_500k_S ; break ;
		#endif
		#ifdef CONFIG_NTRX_22MHZ_4000NS
	 		case NtrxMode22MHz4us125k :
	 			bw = NA_22MHz ; sd = NA_4us ; sr = NA_125k_S ; break ;
	 		case NtrxMode22MHz4us250k :
	 			bw = NA_22MHz ; sd = NA_4us ; sr = NA_250k_S ; break ;
		#endif
		#ifdef CONFIG_NTRX_22MHZ_8000NS
			case NtrxMode22MHz8us125k :
				bw = NA_22MHz ; sd = NA_8us ; sr = NA_125k_S ; break ;
		#endif
		#ifdef CONFIG_NTRX_22MHZ_HR_4000NS
			case NtrxMode22MHzHR4us125k :
				bw = NA_22MHz_HR ; sd = NA_4us ; sr = NA_125k_S ; break ;
			case NtrxMode22MHzHR4us250k :
				bw = NA_22MHz_HR ; sd = NA_4us ; sr = NA_250k_S ; NtrxRangeConst = 445.553589 ; break ;
		#endif
		default :
			return 0 ;
	}

	NtrxInitTRxMode( bw, sd, sr ) ;
	NtrxSetPollMode( True ) ;
	NtrxSetCalInterval( CONFIG_NTRX_RECAL_DELAY ) ;
	return 1 ;
}

void NtrxInit( uint8 reset )
{
	volatile uint8 rs = 0;
	uint32 i=0;

	if(reset){
		// Check Registers Table
		if( NtrxCheckTable() == False )
			ErrorHandler( 3 ) ;

		// Init CPU ctrl pin for NTRX
		NtrxCtrlInit() ;

		// Reset
		NtrxReset() ;

		// Initialize shadow registers
		NtrxInitShadowRegister() ;
	}

	NtrxSSN_Lo() ;

	//必须在SPI片选引脚拉低后延时一段时间，SPI才能正常工作。延时长度要注意。
	for(i=0;i<5000;i++);

	// Configure SPI output of chip LSB first / push pull
	NtrxWriteReg( 0x00, 0x42 ) ;	 //42
	 									 
	//测试Ntrx的SPI读写是否正常
 	if((rs = NtrxReadReg(0x00)) == 0x00)	 //0x00只有4 bits有用，读取0x42不管LSB还是MSB都应该是0x02
 		ErrorHandler( 7 ) ;

	//check!!
 	if( NtrxCheckVerRev() == False )
 		ErrorHandler( 1 ) ;

	if( NtrxSetupTRxMode( CONFIG_NTRX_DEFAULT_MODE ) == 0 )
		ErrorHandler( 4 ) ;

    printf("Ntrx initial successs!\n\n");
}




