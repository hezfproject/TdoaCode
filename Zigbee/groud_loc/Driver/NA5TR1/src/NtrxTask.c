/******************************************************************************
 Copyright 2010, Walasey Technologies
 Author: WangKun
******************************************************************************/

#include "NtrxChip.h"
#include "Console.h"
#include "cpu.h"

#include "stm32f10x.h"

//#include "stm32f10x_systick.h" 
#include <stdio.h>

static Bool NtrxPollMode ;
volatile static uint8 TransFailCount ;
volatile static uint16 AutoCaliDelay ;
static uint32 LastCaliTime ;


//=============================================================================
// Set interrupt service on or off
void NtrxSetPollMode( Bool mode )
{
	NtrxPollMode = mode ;
}

//=============================================================================
// Set the time between to nanoNET TRX chip recalibrations in ms
// 0 : Auto calibration off
void NtrxSetCalInterval( uint16 ci )
{
	AutoCaliDelay = ci ;
}

//=============================================================================
// Ranging destination address backup and match function

static NtrxDevAddr RangingDstAddr ;

void NtrxRangeAddrBackup( NtrxDevPtr DstAddr )
{
	uint8 i ;
	for( i = 0 ; i < NTRX_DEV_ADDR_SIZE ; i ++ )
		RangingDstAddr[i] = DstAddr[i] ;
}

uint8 NtrxDevAddrMatch( NtrxDevPtr SrcAddr, NtrxDevPtr DstAddr )
{
	uint8 i ;
	for( i = 0 ; i < NTRX_DEV_ADDR_SIZE ; i ++ )
		if( DstAddr[i] != SrcAddr[i] )
			return False ;
	return True ;
}

//=============================================================================
/// Stops the baseband-counter

void NtrxStopBasebandTimer( void )
{
	NtrxSetField( NA_ClearBasebandTimerInt, True ) ;
}

//=============================================================================
// NtrxAllCalibration() calibrates the local RF oscillators

const NtrxFlashCode logCh[6][2] = {
	{0x68,0x5A}, // CH 0: 2.44175 GHz, center of the band, shall be used for 80 MHz mode
	{0x62,0x67}, // CH 1: 2.412 GHz, Center frequency of nonoverlapping channel no. 1 (Europe or USA)
	{0x68,0x67}, // CH 2: 2.442 GHz, Center frequency of nonoverlapping channel no. 2 (Europe)
	{0x6E,0x67}, // CH 3: 2.472 GHz, Center frequency of nonoverlapping channel no. 3 (Europe)
	{0x67,0x67}, // CH 4: 2.437 GHz, Center frequency of nonoverlapping channel no. 2 (USA)
	{0x6C,0x67}  // CH 5: 2.462 GHz, Center frequency of nonoverlapping channel no. 3 (USA)
} ;

static NtrxBufferType rxcap[3];
static NtrxBufferType txcap[3];
static Bool trxlocalibrated = False;
void NtrxRxLoCalibration( void )
{
	NtrxBufferType cdata[2] ;

	if(trxlocalibrated){
		NtrxSetLongField(NA_LoRxCapsValue, rxcap);
		return;
	}

	NtrxSetField( NA_EnableLO, True ) ;
	NtrxSetField( NA_EnableLOdiv10, True ) ;
	NtrxSetField( NA_UseLoRxCaps, True ) ;

	NtrxWriteReg( NA_LoIntsReset_O, 0x03 ) ;
	cdata[0] = NtrxReadFlash( &logCh[NtrxCurrentChannel][1] ) ; 
	cdata[1] = NtrxReadFlash( &logCh[NtrxCurrentChannel][0] ) ; 
	NtrxSetLongField( NA_LoTargetValue, cdata ) ;

	while( 1 )
	{
		if( NtrxGetField( NA_LoIntsRawStat ) != 0 )
			break ;
	}

	NtrxSetField( NA_UseLoRxCaps, False ) ;
	NtrxSetField( NA_EnableLOdiv10, False ) ;
	NtrxSetField( NA_EnableLO, False ) ;

	NtrxGetLongField(NA_LoRxCapsValue, rxcap);
}

void NtrxTxLoCalibration (void)
{
	NtrxBufferType cdata[2] ;

	if(trxlocalibrated){
		NtrxSetLongField(NA_LoTxCapsValue, txcap);
		return;
	}

	NtrxSetField( NA_EnableLO, True ) ;
	NtrxSetField( NA_EnableLOdiv10, True ) ;
	NtrxSetField( NA_UseLoRxCaps, False ) ;

	NtrxWriteReg( NA_LoIntsReset_O, 0x03 ) ;
	
	cdata[0] = NtrxReadFlash( &logCh[NtrxCurrentChannel][1] ) ;
	cdata[1] = NtrxReadFlash( &logCh[NtrxCurrentChannel][0] ) ;
	NtrxSetLongField( NA_LoTargetValue, cdata ) ;

	while( 1 )
	{
		if( NtrxGetField( NA_LoIntsRawStat ) != 0 ) //表示自动调整结束
			break ;
	}

	NtrxSetField( NA_EnableLOdiv10, False ) ;
	NtrxSetField( NA_EnableLO, False ) ;

	NtrxGetLongField(NA_LoTxCapsValue, txcap);
}

void NtrxAllCalibration (void)
{
	if( NtrxTransmitState != TxIDLE )
	{
		NtrxCalibration = True ;
		return ;
	}

    //校准过一次后，过较长时间再校准，直接读出
	if(!trxlocalibrated) LastCaliTime = GetSysClock() ;

	NtrxWriteReg( NA_LoEnableFastTuning_O, (1<<NA_LoEnableFastTuning_B)|(1<<NA_LoEnableLsbNeg_B)|(4<<NA_LoFastTuningLevel_LSB) ) ;
	NtrxSetField( NA_RxCmdStop, True ) ;
	NtrxSetField( NA_RxIntsReset, 0xFF ) ;
	NtrxSetField( NA_RxBufferCmd, 3 ) ;

	NtrxRxLoCalibration() ;
	NtrxTxLoCalibration() ;
	trxlocalibrated = True;

	NtrxSetField( NA_RxCmdStart, True ) ;

	NtrxCalibration = False ;
	TransFailCount = 0 ;
}

//=============================================================================
// NtrxSendMessage() writes len bytes of payload to transmit buffer
// and set MAC header and start transmission
//   Payload : -input- data to be transmitted to destination
//   Len:      -input- length of payload to be written into transmit buffer

Bool NtrxSendMessage( NtrxPacketType Type, NtrxDevPtr Dest, NtrxBufferPtr Payload, uint8 Len )
{
	volatile uint8 rs = 0;
	//NtrxBufferType rv[100] = {0} ;

    //csma,添加代码
	NtrxSetField(NA_TxPhCarrSenseMode, NA_TxPhCarrSensModeSymbols_VC_C);
	NtrxSetField(NA_TxBackOffAlg, NA_TxBackOffAlgOn_BC_C);
	//NtrxSetField(NA_TxBackOffSeed, 7);
	
    //PacketTypeData(数据包)、PacketTypeTimeB(时钟包)、PacketTypeBrdcast(广播包)。

	// Select message type date for transmitted messages
	NtrxSetField( NA_TxPacketType, Type ) ;

	// Disable retransmissions for Broadcast & TimeB packet
	if( ( Type == PacketTypeTimeB ) || ( Type == PacketTypeBrdcast ) )
		NtrxSetField( NA_TxArq, False ) ;

	// Write user data to transmit buffer in ntrx chip
	NtrxSetIndexReg( MSB(NA_RamTxBuffer_O) ) ;		//MSB: get high byte?	 
	NtrxWriteSPI( LSB(NA_RamTxBuffer_O), Payload, Len ) ;

	// Copy the destination address to temp buffer
	NtrxSetIndexReg( 0 ) ;	
	NtrxWriteSPI( NA_RamTxDstAddr_O, Dest, 6 ) ;

	// Copy the length of data & additionl bits
	NtrxWriteSingleSPI( NA_RamTxLength_O, Len ) ;					
	NtrxWriteSingleSPI( NA_RamTxLength_O+1, NtrxRangingState<<5 ) ;	

	// If RtcTimeBAutoMode = 0, Payload is time message in TimeB packet
	if( Type == PacketTypeTimeB )
	{
		if( NtrxGetField( NA_RtcTimeBAutoMode ) == 0 )	 //do not automatically read RTC timer
			NtrxWriteSPI( NA_RamRtcTx_O, Payload, 6 ) ;
	}

	// Start the transmission
	NtrxTransmitState = TxSEND ;
	//Tx buffer is ready, and start transmission.	 write only.   //0x0E
	NtrxWriteSingleSPI( NA_TxCmdStart_O, (1<<NA_TxCmdStart_B)|(1<<NA_TxBufferCmd_LSB)|(1<<NA_TxBufferCmd_MSB) ) ;	

	if( Type != 1 )		// PacketTypeAck
	{
		while( NtrxTransmitState != TxWAIT )
			NtrxInterruptService() ;						
		if( Type != PacketTypeData )	//4 types packets: PacketTypeData,PacketTypeTimeB,PacketTypeBrdcast,PacketTypeRanging
			NtrxInterruptService() ;   
	}

	// Enable retransmissions  ,The packet is retransmitted automatically if TxArqMax > 0
	NtrxSetField( NA_TxArq, True ) ;	//waits for an Ack packet as a response

	return NtrxTxEnd() ;									
}

//=============================================================================
// NtrxRangingMode() handles the start and continue of ranging at all.

Bool NtrxRangingMode( NtrxDevPtr DstAddr )
{
	NtrxLRV[ACTTXID] ++ ;
	return NtrxSendMessage( PacketTypeData, DstAddr, NtrxLRV, RANGE_ARRYLEN ) ;
}

//=============================================================================

void NtrxClearLocalRangeValue( void )
{
	uint8 i ;
	for( i = 0 ; i < RANGE_ARRYLEN ; i ++ )
		NtrxLRV[i] = 0 ;
}


Bool TOAGET(uint16* pto, uint8* ppo)
{
	if( NtrxGetField( NA_ToaOffsetMeanDataValid ) )
	{
		uint8 tmp[2];
		*ppo = NtrxGetField( NA_PhaseOffsetData ) ;
		NtrxGetLongField( NA_ToaOffsetMeanData, &tmp[0] ) ;
		*pto = (((uint16)tmp[1] << 8) | (uint16)tmp[0]);
		return True;
	}
	return False;
}
//=============================================================================
// RangingCallback_Rx() collects rangingdata after receiving an rangingpacket
// par Payload : data of received packet
// par Len : length of Payload
// This functions is automaticly called in NTRXReceive

void RangingCallback_Rx( NtrxBufferPtr Payload, uint8 Len , uint8 Rssi)
{
	NtrxLRV[ACTTXID] = 0 ; 

	if( NtrxGetField( NA_ToaOffsetMeanDataValid ) == 0 )
	{
		NtrxClearLocalRangeValue() ;
	}
	else if( Len == RANGE_ARRYLEN )
	{
		switch( Payload[ACTTXID] )
		{
			// 远程第一次RangingMode()函数(在NtrxRxReceive函数中)发出的数据
			case 1 :
				// 将本地Note2程序部分读到的数据拷贝到NtrxSet1中
				NtrxSet1[PHASEOFFSETACK]      = NtrxLRV[PHASEOFFSETACK] ;
				NtrxSet1[TOAOFFSETMEANACK_H]  = NtrxLRV[TOAOFFSETMEANACK_H] ;
				NtrxSet1[TOAOFFSETMEANACK_L]  = NtrxLRV[TOAOFFSETMEANACK_L] ;
				NtrxSet1[TXRESPTIME_H]        = NtrxLRV[TXRESPTIME_H] ;
				NtrxSet1[TXRESPTIME_L]        = NtrxLRV[TXRESPTIME_L] ;

				// 远程Note1程序部分读到的数据通过本次传输到本地(Payload中)
				// 将这部分数据拷贝到NtrxSet1
				NtrxSet1[PHASEOFFSETDATA]     = Payload[PHASEOFFSETDATA] ;
				NtrxSet1[TOAOFFSETMEANDATA_H] = Payload[TOAOFFSETMEANDATA_H] ;
				NtrxSet1[TOAOFFSETMEANDATA_L] = Payload[TOAOFFSETMEANDATA_L] ;
				break ;

			// 远程第二次RangingMode()函数(在NtrxRxReceive函数中)发出的数据
			case 2 :
				// 远程Note2程序部分读到的数据通过最后一次传输到本地(Payload中)
				// 将这部分数据拷贝到NtrxSet2
				NtrxSet2[PHASEOFFSETACK]      = Payload[PHASEOFFSETACK] ;
				NtrxSet2[TOAOFFSETMEANACK_H]  = Payload[TOAOFFSETMEANACK_H] ;
				NtrxSet2[TOAOFFSETMEANACK_L]  = Payload[TOAOFFSETMEANACK_L] ;
				NtrxSet2[TXRESPTIME_H]        = Payload[TXRESPTIME_H] ;
				NtrxSet2[TXRESPTIME_L]        = Payload[TXRESPTIME_L] ;

				// 将本地Note1程序部分读到的数据拷贝到NtrxSet2中
				NtrxSet2[PHASEOFFSETDATA]     = NtrxLRV[PHASEOFFSETDATA] ;
				NtrxSet2[TOAOFFSETMEANDATA_H] = NtrxLRV[TOAOFFSETMEANDATA_H] ;
				NtrxSet2[TOAOFFSETMEANDATA_L] = NtrxLRV[TOAOFFSETMEANDATA_L] ;
				break ;

			default :
				break ;
		}

		// Now we can update our local ranging values
		NtrxLRV[PREVRXID] = Payload[ACTTXID] ;

		// Note1
		NtrxLRV[PHASEOFFSETDATA] = NtrxGetField( NA_PhaseOffsetData ) ;             //32M
		NtrxGetLongField( NA_ToaOffsetMeanData, &NtrxLRV[TOAOFFSETMEANDATA_L] ) ;   //2.4G

		RangingRssi = Rssi;
	}
}

//=============================================================================
// RangingCallback_Ack() collects rangingdata after sending an rangingpacket
// This functions is automaticly called in NtrxTxEnd

void RangingCallback_Ack( void )
{
	if( NtrxGetField( NA_ToaOffsetMeanAckValid ) == 0 )
	{
		NtrxClearLocalRangeValue() ;
	}
	else
	{
		// Note2
		NtrxLRV[PHASEOFFSETACK] = NtrxGetField( NA_PhaseOffsetAck ) ;   //32M
		NtrxGetLongField( NA_ToaOffsetMeanAck, &NtrxLRV[TOAOFFSETMEANACK_L] ) ;//2.4G
		NtrxGetLongField( NA_TxRespTime, &NtrxLRV[TXRESPTIME_L] ) ;     //4M
	}
	NtrxLRV[PREVTXID] = NtrxLRV[ACTTXID] ;
}

//=============================================================================
// NtrxInterruptService is an interrupt service routine of the nanochip.
// It updates the TX, RX and BaseTimer status flags.

void NtrxInterruptService( void )
{
	uint8 IrqStatus ;

	// Transmit
	IrqStatus = NtrxReadReg( NA_TxIntsRawStat_O ) ;	//TX raw interrupt status
	if( IrqStatus != 0 )
	{
		NtrxSetField( NA_TxIntsReset, IrqStatus ) ;	// Clear Interrupts
		if( IrqStatus & ((0x01<<NA_TxBufferRdy_LSB)|(0x01<<NA_TxBufferRdy_MSB)) )
		{
			if( NtrxGetField( NA_TxPacketType ) != PacketTypeData )
				NtrxTxEndIrq = True ;
		}
		if( IrqStatus & (0x01<<NA_TxEnd_B) )  	//TxEnd == true, send success. IrqStatus = 0x04
			NtrxTxEndIrq = True ;
		if( NtrxTxEndIrq )
			NtrxTransmitState = TxWAIT ;
	}

	// Receive
	IrqStatus = NtrxReadReg( NA_RxIntsRawStat_O ) ;
	if( IrqStatus != 0 )
	{
		if( IrqStatus & (0x01<<NA_RxEnd_B) ){
			NtrxRxEndIrq = True ;
			AppRecordTime();    //mcu记录接收中断到达的时间
		}
		NtrxSetField( NA_RxIntsReset, IrqStatus ) ;	// Clear Interrupts
	}

	// Baseband Timer
	if( NtrxGetField( NA_BbTimerIrqStatus ) )
	{
		NtrxStopBasebandTimer() ;   //Ntrx上的计时器，到时产生中断
		BaseBandTimerIrq = True ;
	}	
}

//=============================================================================
// NtrxTxEnd() finish transmission and reset internal state.

Bool NtrxTxEnd( void )
{
	Bool r ;

	NtrxTxEndIrq = False ;

    //// NA_TxArqMax: The maximal count value for packet retransmissions
	if( NtrxGetField( NA_TxArqCnt ) > NtrxGetField( NA_TxArqMax ) )	//max retransmission value and retransmission counter value  
		r = False ;
	else
		r = True ;

	if( ( r == False ) && ( NtrxTransmitState == TxWAIT ) ) //发送未完成
	{
	    //发送失败，且处于发送等待状态
		//TransFailCount ++ ;
		NtrxTransmitState = TxIDLE ;
		//if( NtrxCalibration )
		//{
		//	NtrxAllCalibration() ;  //不用每次都校准
		//}
		NtrxRangingState = RANGING_READY ;
	}
	else
	{
		NtrxTransmitState = TxIDLE ;
		if( NtrxRangingState != RANGING_READY )
		{
			RangingCallback_Ack() ;		//读取保存返回的测距信息
			NtrxRangingState = RANGING_ANSWER1 ;
		}
	}

	return r ;
}

//=============================================================================
// NtrxRxReceive() read out the payload of a received message and
// calls the upper layer/application

NtrxBufferType NtrxReceiveBuffer[128] ;

void NtrxRxReceive( void )
{
	NtrxDevAddr DstAddr ;
	NtrxDevAddr SrcAddr ;
	uint8 RxMsgStatus ;
	uint8 RxLen = 0 ;
	uint8 DstState = RANGING_READY ;
	uint8 Rssi = 0xFF ;

	RxMsgStatus = NtrxReadReg( NA_RxPacketType_O ) ;
	if( RxMsgStatus & (0x01<<NA_RxCrc2Stat_B) )		// Check if data is valid
	{
		NtrxSetIndexReg( 0 ) ;
		// Read destination address
		NtrxReadSPI( NA_RamRxDstAddr_O, DstAddr, NTRX_DEV_ADDR_SIZE ) ;
		// Read source address
		NtrxReadSPI( NA_RamRxSrcAddr_O, SrcAddr, NTRX_DEV_ADDR_SIZE ) ;
		// Read length & additionl bits
		RxLen = NtrxReadReg( NA_RamRxLength_O ) ;
		DstState = NtrxReadReg( NA_RamRxLength_O+1 ) ;
		// Read rssi
		Rssi = NtrxReadReg( NA_AgcGain_O ) & 0x3F;  //比原始代码增加rssi

		if( ( ( DstState & 0x1F ) != 0 ) || ( RxLen > 128 ) )//不在READY状态
			RxLen = 0 ;
		else
		{
			DstState >>= 5 ;
			#ifndef NT_SNIFFER
			// Read user data
			NtrxSetIndexReg( MSB(NA_RamRxBuffer_O) ) ;
			NtrxReadSPI( LSB(NA_RamRxBuffer_O), NtrxReceiveBuffer, RxLen ) ;
			#endif
		}
	}

	// Restart receiver
	NtrxWriteSingleSPI( NA_RxCmdStart_O, (1<<NA_RxCmdStart_B)|(1<<NA_RxBufferCmd_LSB)|(1<<NA_RxBufferCmd_MSB) ) ;
	NtrxRxEndIrq = False ;

	if( RxLen > 0 )
	{
	#if 0
		#ifdef NT_SNIFFER
		AppCallback( SrcAddr, DstAddr, RxMsgStatus, NtrxReceiveBuffer, RxLen , Rssi, DstState) ;
		return;
		#endif
	#endif

		//收到的是时间包或广播
		if( ( ( RxMsgStatus & NtrxRxPacketTypeMask ) == PacketTypeTimeB ) ||
			( ( RxMsgStatus & NtrxRxPacketTypeMask ) == PacketTypeBrdcast ) )
		{   //if 0_
			if(TOACB(NtrxReceiveBuffer, RxLen) == True) //增加的语句，组网
                return;
			// Broadcast or TimeB message, Call application with received data
			AppCallback( SrcAddr, DstAddr, RxMsgStatus, NtrxReceiveBuffer, RxLen , Rssi, DstState) ;
		}
		else if( ( NtrxRangingState == RANGING_READY ) && ( DstState == RANGING_START ) )
		{   //if 1_ 被动卡上的处理过程,
		
			NtrxRangeAddrBackup( SrcAddr ) ;

			// Received ranging data to RangingCallback_Rx ( without protocol header stuff ) 
			RangingCallback_Rx( NtrxReceiveBuffer, RxLen , Rssi) ;
		
			// Send ranging packet
			NtrxRangingState = RANGING_ANSWER1 ;
			NtrxRangingMode( SrcAddr ) ;
				
			// Send ranging packet with TxEnd information from NtrxRangingMode before
			NtrxRangingState = RANGING_ANSWER2 ;
			NtrxRangingMode( SrcAddr ) ;

			NtrxRangingState = RANGING_READY ;
			AppCallback( SrcAddr, DstAddr, ( RxMsgStatus & 0xF0 ) | PacketTypeRanging, NULL, 0 , Rssi, DstState) ;
		}
		else if( ( NtrxRangingState == RANGING_ANSWER1 ) && ( DstState == RANGING_ANSWER1 ) )
		{   //if 2_
			if( NtrxDevAddrMatch( SrcAddr, RangingDstAddr ) == True )
			{
				// Received ranging data to RangingCallback_Rx ( without protocol header stuff )
				RangingCallback_Rx( NtrxReceiveBuffer, RxLen , Rssi) ;
				NtrxRangingState = RANGING_ANSWER2 ;
			}
		}
		else if( ( NtrxRangingState == RANGING_ANSWER2 ) && ( DstState == RANGING_ANSWER2 ) )
		{   //if 3_
			if( NtrxDevAddrMatch( SrcAddr, RangingDstAddr ) == True )
			{
				// Received ranging data to RangingCallback_Rx ( without protocol header stuff )
				RangingCallback_Rx( NtrxReceiveBuffer, RxLen , Rssi) ;
				NtrxRangingState = RANGING_SUCCESSFULL ;
			}
		}
		else if( ( NtrxRangingState == RANGING_READY ) && ( DstState == RANGING_READY ) )
		{   //if 4_
			// Normal message, Call application with received data
			AppCallback( SrcAddr, DstAddr, RxMsgStatus, NtrxReceiveBuffer, RxLen , Rssi, DstState) ;
		}
	}
}

//=============================================================================
// NtrxUpdate operates the complete receive part of the driver. It serves
// receiver interrupt flags and picks up the received frame.

#define RECALIBRATE_NTRX_LO_PERIOD	(1000ul * 60 * 1)	//1 minutes

void NtrxUpdate( void )
{
	if( NtrxPollMode == True )  //处理接收和发送中断
		NtrxInterruptService() ;

	if( NtrxTransmitState == TxWAIT )   //处理未完成的发送
		NtrxTxEnd() ;

	if( NtrxTransmitState == TxIDLE )   //处理未完成的接收中断
		if( NtrxRxEndIrq )
			NtrxRxReceive() ;

#if 0
	if( AutoCaliDelay != 0 )
		if( ( TransFailCount > 3 ) || ( GetSysClock() - LastCaliTime > AutoCaliDelay ) )
			NtrxAllCalibration() ;
#endif

    //每分钟校准一次
	if(GetSysClock() - LastCaliTime > RECALIBRATE_NTRX_LO_PERIOD){
		trxlocalibrated = False;
		NtrxAllCalibration();
	}
}

//=============================================================================
