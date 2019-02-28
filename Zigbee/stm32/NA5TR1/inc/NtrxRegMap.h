/******************************************************************************
 Copyright 2010, Walasey Technologies
 Author: WangKun
******************************************************************************/

#ifndef _NTRXREGMAP_H_
#define _NTRXREGMAP_H_

typedef enum
{
	//0x00
	NA_SpiBitOrder,
	NA_SpiTxDriver,
	NA_IrqPolarity,
	NA_IrqDriver,
	//0x01
	NA_Version,
	NA_WakeUpTimeByte,
	//0x02
	NA_Revision,
	NA_WakeUpTimeWe,
	//0x03
	NA_BattMgmtEnable,
	NA_BattMgmtThreshold,
	NA_BattMgmtCompare,
	//0x04
	NA_DioDirection,
	NA_DioOutValueAlarmEnable,
	NA_DioAlarmStart,
	NA_DioAlarmPolarity,
	NA_DioUsePullup,
	NA_DioUsePulldown,
	NA_DioInValueAlarmStatus,
	//0x05
	NA_DioPortWe,
	//0x06
	NA_EnableWakeUpRtc,
	NA_EnableWakeUpDio,
	NA_PowerUpTime,
	NA_PowerDownMode,
	//0x07
	NA_PowerDown,
	NA_ResetBbClockGate,
	NA_ResetBbRadioCtrl,
	NA_UsePullup4Test,
	NA_UsePulldown4Test,
	//0x08
	NA_EnableBbCrystal,
	NA_EnableBbClock,
	NA_BypassBbCrystal,
	NA_FeatureClockFreq,
	NA_EnableFeatureClock,
	//0x09
	NA_UsePullup4Spiclk,
	NA_UsePulldown4Spiclk,
	NA_UsePullup4Spissn,
	NA_UsePulldown4Spissn,
	NA_UsePullup4Spirxd,
	NA_UsePulldown4Spirxd,
	NA_UsePullup4Spitxd,
	NA_UsePulldown4Spitxd,
	//0x0a
	NA_UsePullup4Por,
	NA_UsePulldown4Por,
	NA_UsePullup4Pamp,
	NA_UsePulldown4Pamp,
	NA_UsePullup4Ucirq,
	NA_UsePulldown4Ucirq,
	NA_UsePullup4Ucrst,
	NA_UsePulldown4Ucrst,
	//0x0b
	NA_WritePulls4Spi,
	NA_WritePulls4Pads,
	//0x0e
	NA_RamIndex,
	NA_DeviceSelect,
	//0x0f
	NA_TxIrqEnable,
	NA_RxIrqEnable,
	NA_BbTimerIrqEnable,
	NA_LoIrqEnable,
	NA_TxIrqStatus,
	NA_RxIrqStatus,
	NA_BbTimerIrqStatus,
	NA_LoIrqStatus,
	//0x10
	NA_TxIntsRawStat,
	NA_TxIntsReset,
	//0x11
	NA_RxIntsRawStat,
	NA_RxIntsReset,
	//0x12
	NA_LoIntsRawStat,
	NA_LoIntsReset,
	NA_ClearBasebandTimerInt,
	//0x13
	NA_TxIntsEn,
	//0x14
	NA_RxIntsEn,
	//0x15
	NA_LoIntsEn,
	//0x16
	NA_LoRxCapsValue,
	//0x19
	NA_LoTxCapsValue,
	//0x1c
	NA_LoEnableFastTuning,
	NA_LoFastTuningLevel,
	NA_LoEnableLsbNeg,
	NA_UseLoRxCaps,
	//0x1d
	NA_LoTargetValue,
	//0x1f
	NA_AgcThresHold1,
	//0x20
	NA_AgcThresHold2,
	//0x21
	NA_HoldAgcInBitSync,
	NA_HoldAgcInFrameSync,
	//0x22
	NA_AgcDeadTime,
	NA_AgcNregLength,
	//0x23
	NA_AgcIntTime,
	//0x25
	NA_AgcValue,
	NA_AgcDefaultEn,
	NA_AgcHold,
	//0x26
	NA_AgcRssiThres,
	NA_AgcGain,
	//0x27
	NA_ChirpFilterCaps,
	NA_FctClockEn,
	NA_StartFctMeasure,
	NA_EnableTx,
	NA_FctPeriod,
	//0x28
	NA_BasebandTimerStartValue,
	//0x2a
	NA_SyncWord,
	//0x2a
	NA_ToaOffsetMeanAck,
	//0x2b
	NA_ToaOffsetMeanAckValid,
	//0x2c
	NA_TxRespTime,
	//0x2e
	NA_PhaseOffsetData,
	NA_PhaseOffsetAck,
	//0x2f
	NA_ToaOffsetMeanData,
	//0x30
	NA_ToaOffsetMeanDataValid,
	//0x31
	NA_RxPacketType,
	NA_RxAddrMatch,
	NA_RxCrc1Stat,
	NA_RxCrc2Stat,
	//0x32
	NA_RxCorrBitErr,
	NA_RxCorrErrThres,
	//0x33
	NA_RxAddrSegEsMatch,
	NA_RxAddrSegIsMatch,
	NA_RxCryptEn,
	NA_RxCryptId,
	NA_RxCryptSeqN,
	//0x37
	NA_TxTimeSlotControl,
	NA_RxTimeSlotControl,
	//0x3c
	NA_TxArqCnt,
	NA_TxArqMax,
	//0x3d
	NA_CsqDitherValue,
	NA_CsqUsePhaseShift,
	NA_CsqUse4Phases,
	NA_CsqAsyMode,
	NA_CsqMemAddrInit,
	NA_CsqUseRam,
	//0x3f
	NA_D3lFixnMap,
	NA_D3lPomEn,
	NA_D3lPomLen,
	NA_D3lUpDownEx,
	//0x40
	NA_LeaveMapThresh1InBitsync,
	NA_UseMapThresh1InFramesync,
	//0x41
	NA_Go2MapThresh1InBitsync,
	//0x42
	NA_EnableLO,
	NA_EnableLOdiv10,
	NA_EnableCsqClock,
	NA_InvertRxClock,
	NA_EnableExtPA,
	//0x43
	NA_LnaFreqAdjust,
	NA_TxPaBias,
	//0x44
	NA_TxOutputPower0,
	//0x45
	NA_TxOutputPower1,
	//0x46
	NA_RfRxCompValueI,
	//0x47
	NA_RfRxCompValueQ,
	//0x48
	NA_SymbolDur,
	NA_SymbolRate,
	NA_ModulationSystem,
	//0x49
	NA_Crc2Type,
	NA_UseFec,
	NA_TxRxCryptCrc2Mode,
	NA_TxRxCryptClkMode,
	//0x4a
	NA_SwapBbBuffers,
	NA_TxRxBbBufferMode1,
	NA_TxRxBbBufferMode0,
	NA_FdmaEnable,
	NA_TxRxMode,
	//0x4b
	NA_ChirpMatrix0,
	NA_ChirpMatrix1,
	//0x4c
	NA_ChirpMatrix2,
	NA_ChirpMatrix3,
	//0x4d
	NA_TxPreTrailMatrix0,
	NA_TxPreTrailMatrix1,
	NA_TxUnderrunIgnore,
	NA_TxMacCifsDis,
	//0x4e
	NA_TxVCarrSens,
	NA_TxPhCarrSenseMode,
	NA_TxVCarrSensAck,
	NA_TxArq,
	NA_Tx3Way,
	NA_TxBackOffAlg,
	NA_TxFragPrio,
	//0x4f
	NA_TxBackOffSeed,
	//0x50
	NA_TxCryptSeqReset,
	NA_TxCryptEn,
	NA_TxCryptId,
	NA_TxCryptSeqN,
	//0x51
	NA_TxScrambInit,
	NA_TxScrambEn,
	//0x54
	NA_TxPacketType,
	NA_TxAddrSlct,
	//0x55
	NA_TxCmdStop,
	NA_TxCmdStart,
	NA_TxBufferCmd,
	//0x56
	NA_RxCmdStop,
	NA_RxCmdStart,
	NA_RxBufferCmd,
	//0x57
	NA_RxCryptSeqReset,
	//0x58
	NA_RxTransBytes,
	//0x5a
	NA_RxTimeBCrc1Mode,
	NA_RxCrc2Mode,
	NA_RxArqMode,
	NA_RxAddrSegEsMode,
	NA_RxAddrSegIsMode,
	NA_RxAddrSegDevIdL,
	//0x5b
	NA_RxDataEn,
	NA_RxBrdcastEn,
	NA_RxTimeBEn,
	NA_RxAddrMode,
	NA_RangingPulses,
	//0x5c
	NA_PulseDetDelay,
	//0x5d
	NA_GateAdjThreshold,
	NA_DownPulseDetectDis,
	NA_UpPulseDetectDis,
	//0x5e
	NA_GateSizeUnsync,
	NA_GateSizeBitsync,
	NA_GateSizeFramesync,
	NA_GateAdjBitsyncEn,
	NA_GateAdjFramesyncEn,
	//0x5f
	NA_Go2BitsyncThreshold,
	NA_LeaveBitsyncThreshold,
	//0x60
	NA_RtcTimeBTxAdj,
	//0x61
	NA_RtcTimeBRxAdj,
	//0x62
	NA_RtcCmdWr,
	NA_RtcCmdRd,
	NA_RtcTimeBAutoMode,
	//0x63
	NA_AgcAmplitude,
	//0x64
	NA_AgcRangeOffset,
	NA_UseAlternativeAgc,
	//0x7f
	NA_RamTxLength,
	NA_RamStaAddr0,
	NA_RamStaAddr1,
	// Psydo Regs
	NA_REG_FIELD_COUNT,
	NA_DELAY,
	NA_TABLE_END
} NtrxRegField ;

//-------------------------------------------------------------------------------
// explanation of post_fixes:
//   _O    : offset; this is the address where the register is located.
//           if the register has more than 8 bits so that it is spread
//           over more than one address this offset is the address of the
//           8 LSBs. The next 8 bits are located at offset+1 and so on
//
//   _B    : for one-bit-registers this is the bit position inside the byte
//           the value range is 0 to 7 where 0 is the LSB and 7 the MSB
//
//   _LSB  : for registers with several bits this value gives the position
//           of the lowest bit inside the byte at the offset address
//
//   _MSB  : for registers with several bits this value gives the position
//           of the highest bit at the offset address. This is always related
//           to the offset address but if the value is 8 or higher the bit is
//           placed at address offset+X.
//
//   _I    : this is the initialization value of the register which is inside
//           after reset.
//-------------------------------------------------------------------------------


//-------------------------------------------------------------------------------
// registers located in serial interface (ON part: all registers;
// PWD part: registers for interrupt controller, battery management,
// featureClock, 32MHz clock
// to simplify the decoder all these registers should be located at
// addresses 0x00 - 0x0F
// Attention: if registers are moved to address 0x10 or higher,
//            changes have to be made in the spi register set
//-------------------------------------------------------------------------------

// pads configuration
// to be able to program the bit order in any case only halve of the register
// is used so that a mirrored byte will cause no conflicts. With this it is
// always possible to configure the spi independent of the direction
// program sequence spi: 0x81 0x00 0x81 (last could be also 0xC3, 0xE7, 0xFF, ..)
//RW
  #define NA_SpiBitOrder_O                              (0x0)
  #define NA_SpiBitOrder_B                              (0)
  #define NA_SpiBitOrder_I                              (0x0)
  #define NA_SpiBitOrderLsbFirst_C                      (0x0)
  #define NA_SpiBitOrderMsbFirst_C                      (0x1)

//RW
  #define NA_SpiTxDriver_O                              (0x0)
  #define NA_SpiTxDriver_B                              (1)
  #define NA_SpiTxDriver_I                              (0x0)
  #define NA_SpiTxDriverOpenDrain_C                     (0x0)
  #define NA_SpiTxDriverPushPull_C                      (0x1)

//RW
  #define NA_IrqPolarity_O                              (0x0)
  #define NA_IrqPolarity_B                              (2)
  #define NA_IrqPolarity_I                              (0x0)
  #define NA_IrqPolarityActiveLow_C                     (0x0)
  #define NA_IrqPolarityActiveHigh_C                    (0x1)

//RW
  #define NA_IrqDriver_O                                (0x0)
  #define NA_IrqDriver_B                                (3)
  #define NA_IrqDriver_I                                (0x0)
  #define NA_IrqDriverOpenDrain_C                       (0x0)
  #define NA_IrqDriverPushPull_C                        (0x1)

//------------------------------------------------------------
//RO
  #define NA_Version_O                                  (0x1)
  #define NA_Version_MSB                                (7)
  #define NA_Version_LSB                                (0)
  #define NA_Version_I                                  (0x5)

//WO
  #define NA_WakeUpTimeByte_O                           (0x1)
  #define NA_WakeUpTimeByte_MSB                         (7)
  #define NA_WakeUpTimeByte_LSB                         (0)
  #define NA_WakeUpTimeByte_I                           (0x0)

//------------------------------------------------------------
//RO
  #define NA_Revision_O                                 (0x2)
  #define NA_Revision_MSB                               (7)
  #define NA_Revision_LSB                               (0)
  #define NA_Revision_I                                 (0x1)

//WO
  #define NA_WakeUpTimeWe_O                             (0x2)
  #define NA_WakeUpTimeWe_MSB                           (3)
  #define NA_WakeUpTimeWe_LSB                           (1)
  #define NA_WakeUpTimeWe_I                             (0x0)

//------------------------------------------------------------
//RW
  #define NA_BattMgmtEnable_O                           (0x3)
  #define NA_BattMgmtEnable_B                           (7)
  #define NA_BattMgmtEnable_I                           (0x0)

//RW
  #define NA_BattMgmtThreshold_O                        (0x3)
  #define NA_BattMgmtThreshold_MSB                      (4)
  #define NA_BattMgmtThreshold_LSB                      (1)
  #define NA_BattMgmtThreshold_I                        (0x0)

//RO
  #define NA_BattMgmtCompare_O                          (0x3)
  #define NA_BattMgmtCompare_B                          (0)
  #define NA_BattMgmtCompare_I                          (0x0)

//------------------------------------------------------------
//WO
  #define NA_DioDirection_O                             (0x4)
  #define NA_DioDirection_B                             (0)
  #define NA_DioDirection_I                             (0x0)

//WO
  #define NA_DioOutValueAlarmEnable_O                   (0x4)
  #define NA_DioOutValueAlarmEnable_B                   (1)
  #define NA_DioOutValueAlarmEnable_I                   (0x0)

//WO
  #define NA_DioAlarmStart_O                            (0x4)
  #define NA_DioAlarmStart_B                            (2)
  #define NA_DioAlarmStart_I                            (0x0)

//WO
  #define NA_DioAlarmPolarity_O                         (0x4)
  #define NA_DioAlarmPolarity_B                         (3)
  #define NA_DioAlarmPolarity_I                         (0x0)

//WO
  #define NA_DioUsePullup_O                             (0x4)
  #define NA_DioUsePullup_B                             (4)
  #define NA_DioUsePullup_I                             (0x0)

//WO
  #define NA_DioUsePulldown_O                           (0x4)
  #define NA_DioUsePulldown_B                           (5)
  #define NA_DioUsePulldown_I                           (0x0)

//RO
  #define NA_DioInValueAlarmStatus_O                    (0x4)
  #define NA_DioInValueAlarmStatus_MSB                  (3)
  #define NA_DioInValueAlarmStatus_LSB                  (0)
  #define NA_DioInValueAlarmStatus_I                    (0x0)

//------------------------------------------------------------
//WO
  #define NA_DioPortWe_O                                (0x5)
  #define NA_DioPortWe_MSB                              (3)
  #define NA_DioPortWe_LSB                              (0)
  #define NA_DioPortWe_I                                (0x0)

//------------------------------------------------------------
//RW
  #define NA_EnableWakeUpRtc_O                          (0x6)
  #define NA_EnableWakeUpRtc_B                          (0)
  #define NA_EnableWakeUpRtc_I                          (0x0)

//RW
  #define NA_EnableWakeUpDio_O                          (0x6)
  #define NA_EnableWakeUpDio_B                          (1)
  #define NA_EnableWakeUpDio_I                          (0x0)

//RW
  #define NA_PowerUpTime_O                              (0x6)
  #define NA_PowerUpTime_MSB                            (6)
  #define NA_PowerUpTime_LSB                            (4)
  #define NA_PowerUpTime_I                              (0x0)
  #define NA_PowerUpTime1Ticks_C                        (0x1)
  #define NA_PowerUpTime2Ticks_C                        (0x2)
  #define NA_PowerUpTime4Ticks_C                        (0x3)
  #define NA_PowerUpTime8Ticks_C                        (0x4)
  #define NA_PowerUpTime16Ticks_C                       (0x5)
  #define NA_PowerUpTime32Ticks_C                       (0x6)
  #define NA_PowerUpTime64Ticks_C                       (0x7)
  #define NA_PowerUpTime128Ticks_C                      (0x0)

//RW
  #define NA_PowerDownMode_O                            (0x6)
  #define NA_PowerDownMode_B                            (7)
  #define NA_PowerDownMode_I                            (0x0)
  #define NA_PowerDownModeFull_C                        (0x0)
  #define NA_PowerDownModePad_C                         (0x1)

//------------------------------------------------------------
//WO
  #define NA_PowerDown_O                                (0x7)
  #define NA_PowerDown_B                                (0)
  #define NA_PowerDown_I                                (0x0)

//RW
  #define NA_ResetBbClockGate_O                         (0x7)
  #define NA_ResetBbClockGate_B                         (1)
  #define NA_ResetBbClockGate_I                         (0x1)

//RW
  #define NA_ResetBbRadioCtrl_O                         (0x7)
  #define NA_ResetBbRadioCtrl_B                         (2)
  #define NA_ResetBbRadioCtrl_I                         (0x1)

//WO
  #define NA_UsePullup4Test_O                           (0x7)
  #define NA_UsePullup4Test_B                           (6)
  #define NA_UsePullup4Test_I                           (0x0)

//WO
  #define NA_UsePulldown4Test_O                         (0x7)
  #define NA_UsePulldown4Test_B                         (7)
  #define NA_UsePulldown4Test_I                         (0x0)

//------------------------------------------------------------
//RW
  #define NA_EnableBbCrystal_O                          (0x8)
  #define NA_EnableBbCrystal_B                          (0)
  #define NA_EnableBbCrystal_I                          (0x0)

//RW
  #define NA_EnableBbClock_O                            (0x8)
  #define NA_EnableBbClock_B                            (1)
  #define NA_EnableBbClock_I                            (0x0)

//RW
  #define NA_BypassBbCrystal_O                          (0x8)
  #define NA_BypassBbCrystal_B                          (3)
  #define NA_BypassBbCrystal_I                          (0x0)

//RW
  #define NA_FeatureClockFreq_O                         (0x8)
  #define NA_FeatureClockFreq_MSB                       (6)
  #define NA_FeatureClockFreq_LSB                       (4)
  #define NA_FeatureClockFreq_I                         (0x0)

  #define NA_FeatureClockDiv1_C                         (0x7)
  #define NA_FeatureClockDiv2_C                         (0x6)
  #define NA_FeatureClockDiv4_C                         (0x5)
  #define NA_FeatureClockDiv8_C                         (0x4)
  #define NA_FeatureClockDiv16_C                        (0x3)
  #define NA_FeatureClockDiv32_C                        (0x2)
  #define NA_FeatureClockDiv64_C                        (0x1)
  #define NA_FeatureClockDiv128_C                       (0x0)

//RW
  #define NA_EnableFeatureClock_O                       (0x8)
  #define NA_EnableFeatureClock_B                       (7)
  #define NA_EnableFeatureClock_I                       (0x0)

//------------------------------------------------------------
//WO
  #define NA_UsePullup4Spiclk_O                         (0x9)
  #define NA_UsePullup4Spiclk_B                         (0)
  #define NA_UsePullup4Spiclk_I                         (0x0)

//WO
  #define NA_UsePulldown4Spiclk_O                       (0x9)
  #define NA_UsePulldown4Spiclk_B                       (1)
  #define NA_UsePulldown4Spiclk_I                       (0x0)

//WO
  #define NA_UsePullup4Spissn_O                         (0x9)
  #define NA_UsePullup4Spissn_B                         (2)
  #define NA_UsePullup4Spissn_I                         (0x0)

//WO
  #define NA_UsePulldown4Spissn_O                       (0x9)
  #define NA_UsePulldown4Spissn_B                       (3)
  #define NA_UsePulldown4Spissn_I                       (0x0)

//WO
  #define NA_UsePullup4Spirxd_O                         (0x9)
  #define NA_UsePullup4Spirxd_B                         (4)
  #define NA_UsePullup4Spirxd_I                         (0x0)

//WO
  #define NA_UsePulldown4Spirxd_O                       (0x9)
  #define NA_UsePulldown4Spirxd_B                       (5)
  #define NA_UsePulldown4Spirxd_I                       (0x0)

//WO
  #define NA_UsePullup4Spitxd_O                         (0x9)
  #define NA_UsePullup4Spitxd_B                         (6)
  #define NA_UsePullup4Spitxd_I                         (0x0)

//WO
  #define NA_UsePulldown4Spitxd_O                       (0x9)
  #define NA_UsePulldown4Spitxd_B                       (7)
  #define NA_UsePulldown4Spitxd_I                       (0x0)

//------------------------------------------------------------
//WO
  #define NA_UsePullup4Por_O                            (0xA)
  #define NA_UsePullup4Por_B                            (0)
  #define NA_UsePullup4Por_I                            (0x0)

//WO
  #define NA_UsePulldown4Por_O                          (0xA)
  #define NA_UsePulldown4Por_B                          (1)
  #define NA_UsePulldown4Por_I                          (0x0)

//WO
  #define NA_UsePullup4Pamp_O                           (0xA)
  #define NA_UsePullup4Pamp_B                           (2)
  #define NA_UsePullup4Pamp_I                           (0x0)

//WO
  #define NA_UsePulldown4Pamp_O                         (0xA)
  #define NA_UsePulldown4Pamp_B                         (3)
  #define NA_UsePulldown4Pamp_I                         (0x0)

//WO
  #define NA_UsePullup4Ucirq_O                          (0xA)
  #define NA_UsePullup4Ucirq_B                          (4)
  #define NA_UsePullup4Ucirq_I                          (0x0)

//WO
  #define NA_UsePulldown4Ucirq_O                        (0xA)
  #define NA_UsePulldown4Ucirq_B                        (5)
  #define NA_UsePulldown4Ucirq_I                        (0x0)

//WO
  #define NA_UsePullup4Ucrst_O                          (0xA)
  #define NA_UsePullup4Ucrst_B                          (6)
  #define NA_UsePullup4Ucrst_I                          (0x0)

//WO
  #define NA_UsePulldown4Ucrst_O                        (0xA)
  #define NA_UsePulldown4Ucrst_B                        (7)
  #define NA_UsePulldown4Ucrst_I                        (0x0)

//------------------------------------------------------------
//WO
  #define NA_WritePulls4Spi_O                           (0xB)
  #define NA_WritePulls4Spi_B                           (0)
  #define NA_WritePulls4Spi_I                           (0x0)

//WO
  #define NA_WritePulls4Pads_O                          (0xB)
  #define NA_WritePulls4Pads_B                          (1)
  #define NA_WritePulls4Pads_I                          (0x0)

//------------------------------------------------------------
//RW
  #define NA_RamIndex_O                                 (0xE)
  #define NA_RamIndex_MSB                               (1)
  #define NA_RamIndex_LSB                               (0)
  #define NA_RamIndex_I                                 (0x0)

//RW
  #define NA_DeviceSelect_O                             (0xE)
  #define NA_DeviceSelect_MSB                           (5)
  #define NA_DeviceSelect_LSB                           (4)
  #define NA_DeviceSelect_I                             (0x0)
  #define NA_DeviceSelectBbRam0_C                       (0x0)
  #define NA_DeviceSelectBbRam1_C                       (0x1)
  #define NA_DeviceSelectCsq_C                          (0x2)
  #define NA_DeviceSelectD3l_C                          (0x3)

//------------------------------------------------------------
//RW
  #define NA_TxIrqEnable_O                              (0xF)
  #define NA_TxIrqEnable_B                              (0)
  #define NA_TxIrqEnable_I                              (0x0)

//RW
  #define NA_RxIrqEnable_O                              (0xF)
  #define NA_RxIrqEnable_B                              (1)
  #define NA_RxIrqEnable_I                              (0x0)

//RW
  #define NA_BbTimerIrqEnable_O                         (0xF)
  #define NA_BbTimerIrqEnable_B                         (2)
  #define NA_BbTimerIrqEnable_I                         (0x0)

//RW
  #define NA_LoIrqEnable_O                              (0xF)
  #define NA_LoIrqEnable_B                              (3)
  #define NA_LoIrqEnable_I                              (0x0)

//RO
  #define NA_TxIrqStatus_O                              (0xF)
  #define NA_TxIrqStatus_B                              (4)
  #define NA_TxIrqStatus_I                              (0x0)

//RO
  #define NA_RxIrqStatus_O                              (0xF)
  #define NA_RxIrqStatus_B                              (5)
  #define NA_RxIrqStatus_I                              (0x0)

//RO
  #define NA_BbTimerIrqStatus_O                         (0xF)
  #define NA_BbTimerIrqStatus_B                         (6)
  #define NA_BbTimerIrqStatus_I                         (0x0)

//RO
  #define NA_LoIrqStatus_O                              (0xF)
  #define NA_LoIrqStatus_B                              (7)
  #define NA_LoIrqStatus_I                              (0x0)

//------------------------------------------------------------
// registers in ON part which are nor directly accessable
//------------------------------------------------------------
  #define NA_WakeUpTime_MSB                             (31)
  #define NA_WakeUpTime_LSB                             (8)
  #define NA_WakeUpTime_I                               (0x0)

//-------------------------------------------------------------------------------
// registers located in baseband radio controller
// to simplify the decoder all these registers must be located at
// addresses 0x10 - 0x7F
//-------------------------------------------------------------------------------

//------------------------------------------------------------
// IRQ related registers
//------------------------------------------------------------
//RO
  #define NA_TxIntsRawStat_O                            (0x10)
  #define NA_TxIntsRawStat_MSB                          (5)
  #define NA_TxIntsRawStat_LSB                          (0)
  #define NA_TxIntsRawStat_I                            (0x0)

//WO
  #define NA_TxIntsReset_O                              (0x10)
  #define NA_TxIntsReset_MSB                            (5)
  #define NA_TxIntsReset_LSB                            (0)
  #define NA_TxIntsReset_I                              (0x0)

  #define NA_TxTimeSlotEnd_B                            (5)
  #define NA_TxTimeSlotTOut_B                           (4)
  #define NA_TxUnderrun_B                               (3)
  #define NA_TxEnd_B                                    (2)
  #define NA_TxBufferRdy_MSB                            (1)
  #define NA_TxBufferRdy_LSB                            (0)

//------------------------------------------------------------
//RO
  #define NA_RxIntsRawStat_O                            (0x11)
  #define NA_RxIntsRawStat_MSB                          (6)
  #define NA_RxIntsRawStat_LSB                          (0)
  #define NA_RxIntsRawStat_I                            (0x0)

//WO
  #define NA_RxIntsReset_O                              (0x11)
  #define NA_RxIntsReset_MSB                            (6)
  #define NA_RxIntsReset_LSB                            (0)
  #define NA_RxIntsReset_I                              (0x0)

  #define NA_RxTimeSlotEnd_B                            (6)
  #define NA_RxTimeSlotTOut_B                           (5)
  #define NA_RxOverflow_B                               (4)
  #define NA_RxHeaderEnd_B                              (3)
  #define NA_RxEnd_B                                    (2)
  #define NA_RxBufferRdy_MSB                            (1)
  #define NA_RxBufferRdy_LSB                            (0)

//------------------------------------------------------------
//RO
  #define NA_LoIntsRawStat_O                            (0x12)
  #define NA_LoIntsRawStat_B                            (1)
  #define NA_LoIntsRawStat_I                            (0x0)

//WO
  #define NA_LoIntsReset_O                              (0x12)
  #define NA_LoIntsReset_B                              (1)
  #define NA_LoIntsReset_MSB                            (1)
  #define NA_LoIntsReset_LSB                            (0)
  #define NA_LoIntsReset_I                              (0x0)

  #define NA_LoTuningReady_B                            (1)

//WO
  #define NA_ClearBasebandTimerInt_O                    (0x12)
  #define NA_ClearBasebandTimerInt_B                    (7)
  #define NA_ClearBasebandTimerInt_I                    (0)

//------------------------------------------------------------
//WO
  #define NA_TxIntsEn_O                                 (0x13)
  #define NA_TxIntsEn_MSB                               (5)
  #define NA_TxIntsEn_LSB                               (0)
  #define NA_TxIntsEn_I                                 (0x0)

//------------------------------------------------------------
//WO
  #define NA_RxIntsEn_O                                 (0x14)
  #define NA_RxIntsEn_MSB                               (6)
  #define NA_RxIntsEn_LSB                               (0)
  #define NA_RxIntsEn_I                                 (0x0)

//------------------------------------------------------------
//WO
  #define NA_LoIntsEn_O                                 (0x15)
  #define NA_LoIntsEn_B                                 (1)
  #define NA_LoIntsEn_I                                 (0x0)

//------------------------------------------------------------
// LO related registers
//------------------------------------------------------------
//RW
  #define NA_LoRxCapsValue_O                            (0x16)
  #define NA_LoRxCapsValue_MSB                          (21)
  #define NA_LoRxCapsValue_LSB                          (0)
  #define NA_LoRxCapsValue_I                            (0x200040)

//------------------------------------------------------------
//RW
  #define NA_LoTxCapsValue_O                            (0x19)
  #define NA_LoTxCapsValue_MSB                          (21)
  #define NA_LoTxCapsValue_LSB                          (0)
  #define NA_LoTxCapsValue_I                            (0x200040)

//------------------------------------------------------------
//WO
  #define NA_LoEnableFastTuning_O                       (0x1C)
  #define NA_LoEnableFastTuning_B                       (0)
  #define NA_LoEnableFastTuning_I                       (0x0)

//WO
// if the expanded gray counter (neg edge ff) is used the max. value is 6
// if the small gray counter (no neg edge ff) is used the max. value is 2
  #define NA_LoFastTuningLevel_O                        (0x1C)
  #define NA_LoFastTuningLevel_MSB                      (3)
  #define NA_LoFastTuningLevel_LSB                      (1)
  #define NA_LoFastTuningLevel_I                        (0x0)

//WO
  #define NA_LoEnableLsbNeg_O                           (0x1C)
  #define NA_LoEnableLsbNeg_B                           (4)
  #define NA_LoEnableLsbNeg_I                           (0x0)

//WO; needed for lo tuning
  #define NA_UseLoRxCaps_O                              (0x1C)
  #define NA_UseLoRxCaps_B                              (7)
  #define NA_UseLoRxCaps_I                              (0x0)

//------------------------------------------------------------
//RW
  #define NA_LoTargetValue_O                            (0x1D)
  #define NA_LoTargetValue_MSB                          (15)
  #define NA_LoTargetValue_LSB                          (0)
  #define NA_LoTargetValue_I                            (0x0000)

//------------------------------------------------------------
// AGC related registers
//------------------------------------------------------------
//WO
  #define NA_AgcThresHold1_O                            (0x1F)
  #define NA_AgcThresHold1_MSB                          (7)
  #define NA_AgcThresHold1_LSB                          (0)
  #define NA_AgcThresHold1_I                            (0x3)

//------------------------------------------------------------
//WO
  #define NA_AgcThresHold2_O                            (0x20)
  #define NA_AgcThresHold2_MSB                          (7)
  #define NA_AgcThresHold2_LSB                          (0)
  #define NA_AgcThresHold2_I                            (0x6)

//------------------------------------------------------------
//WO; hold agc after this number of symbols in bitsync state
  #define NA_HoldAgcInBitSync_O                         (0x21)
  #define NA_HoldAgcInBitSync_MSB                       (6)
  #define NA_HoldAgcInBitSync_LSB                       (0)
  #define NA_HoldAgcInBitSync_I                         (0x18)

//WO; hold in framesync state
  #define NA_HoldAgcInFrameSync_O                       (0x21)
  #define NA_HoldAgcInFrameSync_B                       (7)
  #define NA_HoldAgcInFrameSync_I                       (0x1)

//------------------------------------------------------------
//WO
  #define NA_AgcDeadTime_O                              (0x22)
  #define NA_AgcDeadTime_MSB                            (5)
  #define NA_AgcDeadTime_LSB                            (0)
  #define NA_AgcDeadTime_I                              (0xC)

//WO
  #define NA_AgcNregLength_O                            (0x22)
  #define NA_AgcNregLength_MSB                          (7)
  #define NA_AgcNregLength_LSB                          (6)
  #define NA_AgcNregLength_I                            (0x0)

//------------------------------------------------------------
//WO
  #define NA_AgcIntTime_O                               (0x23)
  #define NA_AgcIntTime_MSB                             (11)
  #define NA_AgcIntTime_LSB                             (0)
  #define NA_AgcIntTime_I                               (0x200)

//------------------------------------------------------------
//WO;
  #define NA_AgcValue_O                                 (0x25)
  #define NA_AgcValue_MSB                               (5)
  #define NA_AgcValue_LSB                               (0)
  #define NA_AgcValue_I                                 (0x3F)

//WO;
  #define NA_AgcDefaultEn_O                             (0x25)
  #define NA_AgcDefaultEn_B                             (6)
  #define NA_AgcDefaultEn_I                             (0x0)

//WO;
  #define NA_AgcHold_O                                  (0x25)
  #define NA_AgcHold_B                                  (7)
  #define NA_AgcHold_I                                  (0x0)


//------------------------------------------------------------
//WO
  #define NA_AgcRssiThres_O                             (0x26)
  #define NA_AgcRssiThres_MSB                           (5)
  #define NA_AgcRssiThres_LSB                           (0)
  #define NA_AgcRssiThres_I                             (0x1E)

//RO
  #define NA_AgcGain_O                                  (0x26)
  #define NA_AgcGain_MSB                                (5)
  #define NA_AgcGain_LSB                                (0)
  #define NA_AgcGain_I                                  (0x0)

//------------------------------------------------------------
// RC oscillator related registers
//------------------------------------------------------------
//WO
  #define NA_ChirpFilterCaps_O                          (0x27)
  #define NA_ChirpFilterCaps_MSB                        (3)
  #define NA_ChirpFilterCaps_LSB                        (0)
  #define NA_ChirpFilterCaps_I                          (0x6)

//WO
  #define NA_FctClockEn_O                               (0x27)
  #define NA_FctClockEn_B                               (4)
  #define NA_FctClockEn_I                               (0x0)

//WO
  #define NA_StartFctMeasure_O                          (0x27)
  #define NA_StartFctMeasure_B                          (5)
  #define NA_StartFctMeasure_I                          (0x0)

//WO; fct tuning
  #define NA_EnableTx_O                                 (0x27)
  #define NA_EnableTx_B                                 (7)
  #define NA_EnableTx_I                                 (0x0)

//RO
  #define NA_FctPeriod_O                                (0x27)
  #define NA_FctPeriod_MSB                              (5)
  #define NA_FctPeriod_LSB                              (0)
  #define NA_FctPeriod_I                                (0x0)

//------------------------------------------------------------
// baseband timer related registers
//------------------------------------------------------------
//RW
  #define NA_BasebandTimerStartValue_O                  (0x28)
  #define NA_BasebandTimerStartValue_MSB                (15)
  #define NA_BasebandTimerStartValue_LSB                (0)
  #define NA_BasebandTimerStartValue_I                  (0x0)

//------------------------------------------------------------
// syncword (8 bytes at address 42 - 49)
//------------------------------------------------------------
//WO
  #define NA_SyncWord_O                                 (0x2A)
  #define NA_SyncWord_MSB                               (63)
  #define NA_SyncWord_LSB                               (0)
  #define NA_SyncWord_I                                 (0xAB69CA9492D52CAB)

//------------------------------------------------------------
// ranging related read registers
//------------------------------------------------------------
//RO
  #define NA_ToaOffsetMeanAck_O                         (0x2A)
  #define NA_ToaOffsetMeanAck_MSB                       (12)
  #define NA_ToaOffsetMeanAck_LSB                       (0)
  #define NA_ToaOffsetMeanAck_I                         (0x0)

//RO
  #define NA_ToaOffsetMeanAckValid_O                    (0x2B)
  #define NA_ToaOffsetMeanAckValid_B                    (7)
  #define NA_ToaOffsetMeanAckValid_I                    (0x0)

//------------------------------------------------------------
//RO
  #define NA_TxRespTime_O                               (0x2C)
  #define NA_TxRespTime_MSB                             (15)
  #define NA_TxRespTime_LSB                             (0)
  #define NA_TxRespTime_I                               (0x0)

//------------------------------------------------------------
//RO
  #define NA_PhaseOffsetAck_O                           (0x2E)
  #define NA_PhaseOffsetAck_MSB                         (2)
  #define NA_PhaseOffsetAck_LSB                         (0)
  #define NA_PhaseOffsetAck_I                           (0x0)

//RO
  #define NA_PhaseOffsetData_O                          (0x2E)
  #define NA_PhaseOffsetData_MSB                        (6)
  #define NA_PhaseOffsetData_LSB                        (4)
  #define NA_PhaseOffsetData_I                          (0x0)

//------------------------------------------------------------
//RO
  #define NA_ToaOffsetMeanData_O                        (0x2F)
  #define NA_ToaOffsetMeanData_MSB                      (12)
  #define NA_ToaOffsetMeanData_LSB                      (0)
  #define NA_ToaOffsetMeanData_I                        (0x0)

//RO
  #define NA_ToaOffsetMeanDataValid_O                   (0x30)
  #define NA_ToaOffsetMeanDataValid_B                   (7)
  #define NA_ToaOffsetMeanDataValid_I                   (0x0)

//------------------------------------------------------------
// rx status registers
//------------------------------------------------------------
//RO
  #define NA_RxPacketType_O                             (0x31)
  #define NA_RxPacketType_MSB                           (3)
  #define NA_RxPacketType_LSB                           (0)
  #define NA_RxPacketType_I                             (0x0)

//RO
  #define NA_RxAddrMatch_O                              (0x31)
  #define NA_RxAddrMatch_MSB                            (5)
  #define NA_RxAddrMatch_LSB                            (4)
  #define NA_RxAddrMatch_I                              (0x0)

//RO
  #define NA_RxCrc1Stat_O                               (0x31)
  #define NA_RxCrc1Stat_B                               (6)
  #define NA_RxCrc1Stat_I                               (0x0)

//RO
  #define NA_RxCrc2Stat_O                               (0x31)
  #define NA_RxCrc2Stat_B                               (7)
  #define NA_RxCrc2Stat_I                               (0x0)

//------------------------------------------------------------
//RO
  #define NA_RxCorrBitErr_O                             (0x32)
  #define NA_RxCorrBitErr_MSB                           (3)
  #define NA_RxCorrBitErr_LSB                           (0)
  #define NA_RxCorrBitErr_I                             (0x0)

//WO
  #define NA_RxCorrErrThres_O                           (0x32)
  #define NA_RxCorrErrThres_MSB                         (7)
  #define NA_RxCorrErrThres_LSB                         (4)
  #define NA_RxCorrErrThres_I                           (0x3)

//------------------------------------------------------------
//RO
  #define NA_RxAddrSegEsMatch_O                         (0x33)
  #define NA_RxAddrSegEsMatch_B                         (0)
  #define NA_RxAddrSegEsMatch_I                         (0x0)

//RO
  #define NA_RxAddrSegIsMatch_O                         (0x33)
  #define NA_RxAddrSegIsMatch_B                         (1)
  #define NA_RxAddrSegIsMatch_I                         (0x0)

//RO
  #define NA_RxCryptEn_O                                (0x33)
  #define NA_RxCryptEn_B                                (4)
  #define NA_RxCryptEn_I                                (0x0)

//RO
  #define NA_RxCryptId_O                                (0x33)
  #define NA_RxCryptId_MSB                              (6)
  #define NA_RxCryptId_LSB                              (5)
  #define NA_RxCryptId_I                                (0x0)

//RO
  #define NA_RxCryptSeqN_O                              (0x33)
  #define NA_RxCryptSeqN_B                              (7)
  #define NA_RxCryptSeqN_I                              (0x0)

//------------------------------------------------------------
//RO
  #define NA_RxFec1BitErr_O                             (0x34)
  #define NA_RxFec1BitErr_MSB                           (14)
  #define NA_RxFec1BitErr_LSB                           (0)
  #define NA_RxFec1BitErr_I                             (0x0)

//------------------------------------------------------------
// timeslot related registers
//------------------------------------------------------------
//WO
  #define NA_TxTimeSlotStart_O                          (0x33)
  #define NA_TxTimeSlotStart_MSB                        (15)
  #define NA_TxTimeSlotStart_LSB                        (0)
  #define NA_TxTimeSlotStart_I                          (0x0)

//------------------------------------------------------------
//WO
  #define NA_TxTimeSlotEnd_O                            (0x35)
  #define NA_TxTimeSlotEnd_MSB                          (15)
  #define NA_TxTimeSlotEnd_LSB                          (0)
  #define NA_TxTimeSlotEnd_I                            (0x0)

//------------------------------------------------------------
//WO
  #define NA_TxTimeSlotControl_O                        (0x37)
  #define NA_TxTimeSlotControl_B                        (0)
  #define NA_TxTimeSlotControl_I                        (0x0)

//WO
  #define NA_RxTimeSlotControl_O                        (0x37)
  #define NA_RxTimeSlotControl_B                        (1)
  #define NA_RxTimeSlotControl_I                        (0x0)

//------------------------------------------------------------
//RO
  #define NA_RxPacketSlot_O                             (0x38)
  #define NA_RxPacketSlot_MSB                           (15)
  #define NA_RxPacketSlot_LSB                           (0)
  #define NA_RxPacketSlot_I                             (0x0)

//WO
  #define NA_RxTimeSlotStart_O                          (0x38)
  #define NA_RxTimeSlotStart_MSB                        (15)
  #define NA_RxTimeSlotStart_LSB                        (0)
  #define NA_RxTimeSlotStart_I                          (0x0)

//------------------------------------------------------------
//WO
  #define NA_RxTimeSlotEnd_O                            (0x3A)
  #define NA_RxTimeSlotEnd_MSB                          (15)
  #define NA_RxTimeSlotEnd_LSB                          (0)
  #define NA_RxTimeSlotEnd_I                            (0x0)

//------------------------------------------------------------
// tx status registers
//------------------------------------------------------------
//RO
  #define NA_TxArqCnt_O                                 (0x3C)
  #define NA_TxArqCnt_MSB                               (3)
  #define NA_TxArqCnt_LSB                               (0)
  #define NA_TxArqCnt_I                                 (0x0)

//WO
  #define NA_TxArqMax_O                                 (0x3C)
  #define NA_TxArqMax_MSB                               (7)
  #define NA_TxArqMax_LSB                               (4)
  #define NA_TxArqMax_I                                 (0xE)

//------------------------------------------------------------
// chirpsequencer related registers
//------------------------------------------------------------
//WO
  #define NA_CsqDitherValue_O                           (0x3D)
  #define NA_CsqDitherValue_MSB                         (1)
  #define NA_CsqDitherValue_LSB                         (0)
  #define NA_CsqDitherValue_I                           (0x0)

//WO
  #define NA_CsqUsePhaseShift_O                         (0x3D)
  #define NA_CsqUsePhaseShift_B                         (2)
  #define NA_CsqUsePhaseShift_I                         (0x1)

//WO
  #define NA_CsqUse4Phases_O                            (0x3D)
  #define NA_CsqUse4Phases_B                            (3)
  #define NA_CsqUse4Phases_I                            (0x0)

//WO
  #define NA_CsqAsyMode_O                               (0x3D)
  #define NA_CsqAsyMode_B                               (4)
  #define NA_CsqAsyMode_I                               (0x0)

//WO
  #define NA_CsqMemAddrInit_O                           (0x3D)
  #define NA_CsqMemAddrInit_B                           (5)
  #define NA_CsqMemAddrInit_I                           (0x0)

//WO
  #define NA_CsqUseRam_O                                (0x3D)
  #define NA_CsqUseRam_B                                (6)
  #define NA_CsqUseRam_I                                (0x0)

//------------------------------------------------------------
//WO
#define NA_CsqSetValue_O                                (0x3E)
#define NA_CsqSetValue_MSB                              (5)
#define NA_CsqSetValue_LSB                              (0)
#define NA_CsqSetValue_I                                (0x0)

//WO
#define NA_CsqSetIValue_O                               (0x3E)
#define NA_CsqSetIValue_B                               (6)
#define NA_CsqSetIValue_I                               (0x0)

//WO
#define NA_CsqSetQValue_O                               (0x3E)
#define NA_CsqSetQValue_B                               (7)
#define NA_CsqSetQValue_I                               (0x0)

//------------------------------------------------------------
// Correlator related registers
//------------------------------------------------------------
//WO
  #define NA_D3lFixnMap_O                               (0x3F)
  #define NA_D3lFixnMap_B                               (0)
  #define NA_D3lFixnMap_I                               (0x1)

//WO
  #define NA_D3lPomEn_O                                 (0x3F)
  #define NA_D3lPomEn_B                                 (1)
  #define NA_D3lPomEn_I                                 (0x0)

//WO
  #define NA_D3lPomLen_O                                (0x3F)
  #define NA_D3lPomLen_MSB                              (3)
  #define NA_D3lPomLen_LSB                              (2)
  #define NA_D3lPomLen_I                                (0x0)

//WO; test
  #define NA_D3lUpDownEx_O                              (0x3F)
  #define NA_D3lUpDownEx_B                              (7)
  #define NA_D3lUpDownEx_I                              (0x0)

//------------------------------------------------------------
//WO
  #define NA_LeaveMapThresh1InBitsync_O                 (0x40)
  #define NA_LeaveMapThresh1InBitsync_MSB               (6)
  #define NA_LeaveMapThresh1InBitsync_LSB               (0)
  #define NA_LeaveMapThresh1InBitsync_I                 (0x3)

//WO
  #define NA_UseMapThresh1InFramesync_O                 (0x40)
  #define NA_UseMapThresh1InFramesync_B                 (7)
  #define NA_UseMapThresh1InFramesync_I                 (0x0)

//------------------------------------------------------------
//WO
  #define NA_Go2MapThresh1InBitsync_O                   (0x41)
  #define NA_Go2MapThresh1InBitsync_MSB                 (6)
  #define NA_Go2MapThresh1InBitsync_LSB                 (0)
  #define NA_Go2MapThresh1InBitsync_I                   (0x7)

//------------------------------------------------------------
// Enable and clock registers
//------------------------------------------------------------
//WO; needed for lo tuning and programming CSQ ram
  #define NA_EnableLO_O                                 (0x42)
  #define NA_EnableLO_B                                 (0)
  #define NA_EnableLO_I                                 (0x0)

//WO; needed for lo tuning and programming CSQ ram
  #define NA_EnableLOdiv10_O                            (0x42)
  #define NA_EnableLOdiv10_B                            (1)
  #define NA_EnableLOdiv10_I                            (0x0)

//WO; needed for programming CSQ ram
  #define NA_EnableCsqClock_O                           (0x42)
  #define NA_EnableCsqClock_B                           (2)
  #define NA_EnableCsqClock_I                           (0x0)

//WO; test
  #define NA_InvertRxClock_O                            (0x42)
  #define NA_InvertRxClock_B                            (3)
  #define NA_InvertRxClock_I                            (0x0)

//WO
  #define NA_EnableExtPA_O                              (0x42)
  #define NA_EnableExtPA_B                              (4)
  #define NA_EnableExtPA_I                              (0x0)

//------------------------------------------------------------
// analogue config registers
//------------------------------------------------------------
//WO
  #define NA_LnaFreqAdjust_O                            (0x43)
  #define NA_LnaFreqAdjust_MSB                          (2)
  #define NA_LnaFreqAdjust_LSB                          (0)
  #define NA_LnaFreqAdjust_I                            (0x3)

//WO
  #define NA_TxPaBias_O                                 (0x43)
  #define NA_TxPaBias_MSB                               (6)
  #define NA_TxPaBias_LSB                               (4)
  #define NA_TxPaBias_I                                 (0x0)

//------------------------------------------------------------
//WO
  #define NA_TxOutputPower0_O                           (0x44)
  #define NA_TxOutputPower0_MSB                         (5)
  #define NA_TxOutputPower0_LSB                         (0)
  #define NA_TxOutputPower0_I                           (0x3F)

//------------------------------------------------------------
//WO
  #define NA_TxOutputPower1_O                           (0x45)
  #define NA_TxOutputPower1_MSB                         (5)
  #define NA_TxOutputPower1_LSB                         (0)
  #define NA_TxOutputPower1_I                           (0x3F)

//------------------------------------------------------------
// quantization registers
//------------------------------------------------------------
//WO
  #define NA_RfRxCompValueI_O                           (0x46)
  #define NA_RfRxCompValueI_MSB                         (4)
  #define NA_RfRxCompValueI_LSB                         (0)
  #define NA_RfRxCompValueI_I                           (0xF)

//------------------------------------------------------------
//WO
  #define NA_RfRxCompValueQ_O                           (0x47)
  #define NA_RfRxCompValueQ_MSB                         (4)
  #define NA_RfRxCompValueQ_LSB                         (0)
  #define NA_RfRxCompValueQ_I                           (0xF)

//------------------------------------------------------------
// config registers for tx and rx
//------------------------------------------------------------
//WO
  #define NA_SymbolDur_O                                (0x48)
  #define NA_SymbolDur_MSB                              (2)
  #define NA_SymbolDur_LSB                              (0)
  #define NA_SymbolDur_I                                (0x3)

// 16000 ns and 8000 ns are just allowed for 22 MHz bndwidth !!!
  #define NA_SymbolDur16000ns_C                         (0x5)
  #define NA_SymbolDur8000ns_C                          (0x4)
  #define NA_SymbolDur4000ns_C                          (0x3)
  #define NA_SymbolDur2000ns_C                          (0x2)
  #define NA_SymbolDur1000ns_C                          (0x1)
  #define NA_SymbolDur500ns_C                           (0x0)

//WO
  #define NA_SymbolRate_O                               (0x48)
  #define NA_SymbolRate_MSB                             (6)
  #define NA_SymbolRate_LSB                             (4)
  #define NA_SymbolRate_I                               (0x7)

  #define NA_SymbolRate31k25Symbols_VC_C                (0x4)
  #define NA_SymbolRate62k5Symbols_VC_C                 (0x5)
  #define NA_SymbolRate125kSymbols_VC_C                 (0x6)
  #define NA_SymbolRate250kSymbols_VC_C                 (0x7)
  #define NA_SymbolRate500kSymbols_VC_C                 (0x0)
  #define NA_SymbolRate1MSymbols_VC_C                   (0x1)
  #define NA_SymbolRate2MSymbols_VC_C                   (0x2)

//WO
  #define NA_ModulationSystem_O                         (0x48)
  #define NA_ModulationSystem_B                         (7)
  #define NA_ModulationSystem_I                         (0x0)

  #define NA_ModulationSystem2ary_BC_C                  (0x0)
  #define NA_ModulationSystem4ary_BC_C                  (0x1)

//------------------------------------------------------------
//WO
  #define NA_Crc2Type_O                                 (0x49)
  #define NA_Crc2Type_MSB                               (1)
  #define NA_Crc2Type_LSB                               (0)
  #define NA_Crc2Type_I                                 (0x0)

  #define NA_Crc2Type1_VC_C                             (0x0)
  #define NA_Crc2Type2_VC_C                             (0x1)
  #define NA_Crc2Type3_VC_C                             (0x2)
  #define NA_Crc2Type1Bits_IC_C                         (16)
  #define NA_Crc2Type2Bits_IC_C                         (16)
  #define NA_Crc2Type3Bits_IC_C                         (32)

//WO
  #define NA_UseFec_O                                   (0x49)
  #define NA_UseFec_B                                   (2)
  #define NA_UseFec_I                                   (0x0)

  #define NA_UseFecOff_BC_C                             (0x0)
  #define NA_UseFecOn_BC_C                              (0x1)

//WO
  #define NA_TxRxCryptCrc2Mode_O                        (0x49)
  #define NA_TxRxCryptCrc2Mode_B                        (3)
  #define NA_TxRxCryptCrc2Mode_I                        (0x0)

  #define NA_TxRxCryptCrc2ModeUncrypted_BC_C            (0x0)
  #define NA_TxRxCryptCrc2ModeEncrypted_BC_C            (0x1)

//WO
  #define NA_TxRxCryptClkMode_O                         (0x49)
  #define NA_TxRxCryptClkMode_MSB                       (7)
  #define NA_TxRxCryptClkMode_LSB                       (4)
  #define NA_TxRxCryptClkMode_I                         (0x0)

  #define NA_TxRxCryptClkModeCryptClock_BC_C            (0x0)
  #define NA_TxRxCryptClkModeScrambInit_BC_C            (0x1)

//------------------------------------------------------------
//RW
  #define NA_SwapBbBuffers_O                            (0x4A)
  #define NA_SwapBbBuffers_B                            (0)
  #define NA_SwapBbBuffers_I                            (0x0)

//WO
  #define NA_TxRxBbBufferMode1_O                        (0x4A)
  #define NA_TxRxBbBufferMode1_B                        (1)
  #define NA_TxRxBbBufferMode1_I                        (0x0)

  #define NA_TxRxBufferMode1Duplex_BC_C                 (0x0)
  #define NA_TxRxBufferMode1Simplex_BC_C                (0x1)

//WO
  #define NA_TxRxBbBufferMode0_O                        (0x4A)
  #define NA_TxRxBbBufferMode0_B                        (2)
  #define NA_TxRxBbBufferMode0_I                        (0x0)

  #define NA_TxRxBufferMode0Auto_BC_C                   (0x0)
  #define NA_TxRxBufferMode0Transparent_BC_C            (0x1)

//WO
  #define NA_FdmaEnable_O                               (0x4A)
  #define NA_FdmaEnable_B                               (4)
  #define NA_FdmaEnable_I                               (0x1)

//WO
  #define NA_TxRxMode_O                                 (0x4A)
  #define NA_TxRxMode_B                                 (7)
  #define NA_TxRxMode_I                                 (0x0)

  #define NA_TxRxModeAuto_BC_C                          (0x0)
  #define NA_TxRxModeTransparent_BC_C                   (0x1)

//------------------------------------------------------------
//WO
  #define NA_ChirpMatrix0_O                             (0x4B)
  #define NA_ChirpMatrix0_MSB                           (2)
  #define NA_ChirpMatrix0_LSB                           (0)
  #define NA_ChirpMatrix0_I                             (0x0)

//WO
  #define NA_ChirpMatrix1_O                             (0x4B)
  #define NA_ChirpMatrix1_MSB                           (6)
  #define NA_ChirpMatrix1_LSB                           (4)
  #define NA_ChirpMatrix1_I                             (0x1)

//------------------------------------------------------------
//WO
  #define NA_ChirpMatrix2_O                             (0x4C)
  #define NA_ChirpMatrix2_MSB                           (2)
  #define NA_ChirpMatrix2_LSB                           (0)
  #define NA_ChirpMatrix2_I                             (0x3)

//WO
  #define NA_ChirpMatrix3_O                             (0x4C)
  #define NA_ChirpMatrix3_MSB                           (6)
  #define NA_ChirpMatrix3_LSB                           (4)
  #define NA_ChirpMatrix3_I                             (0x4)

  #define NA_ChirpDown_VC_C                             (0x0)
  #define NA_ChirpUp_VC_C                               (0x1)
  #define NA_ChirpFoldMinus_VC_C                        (0x2)
  #define NA_ChirpFoldPlus_VC_C                         (0x3)
  #define NA_ChirpOff_VC_C                              (0x4)

//------------------------------------------------------------
// config registers for tx only
//------------------------------------------------------------
//WO
  #define NA_TxPreTrailMatrix0_O                        (0x4D)
  #define NA_TxPreTrailMatrix0_MSB                      (1)
  #define NA_TxPreTrailMatrix0_LSB                      (0)
  #define NA_TxPreTrailMatrix0_I                        (0x0)

//WO
  #define NA_TxPreTrailMatrix1_O                        (0x4D)
  #define NA_TxPreTrailMatrix1_MSB                      (3)
  #define NA_TxPreTrailMatrix1_LSB                      (2)
  #define NA_TxPreTrailMatrix1_I                        (0x1)

//WO
  #define NA_TxUnderrunIgnore_O                         (0x4D)
  #define NA_TxUnderrunIgnore_B                         (4)
  #define NA_TxUnderrunIgnore_I                         (0x1)

  #define NA_TxUnderrunIgnoreOff_BC_C                   (0x0)
  #define NA_TxUnderrunIgnoreOn_BC_C                    (0x1)

//WO
  #define NA_TxMacCifsDis_O                             (0x4D)
  #define NA_TxMacCifsDis_B                             (7)
  #define NA_TxMacCifsDis_I                             (0x0)

//------------------------------------------------------------
//WO
  #define NA_TxVCarrSens_O                              (0x4E)
  #define NA_TxVCarrSens_B                              (0)
  #define NA_TxVCarrSens_I                              (0x0)

  #define NA_TxVCarrSensOff_BC_C                        (0x0)
  #define NA_TxVCarrSensOn_BC_C                         (0x1)

//WO
  #define NA_TxPhCarrSenseMode_O                        (0x4E)
  #define NA_TxPhCarrSenseMode_MSB                      (2)
  #define NA_TxPhCarrSenseMode_LSB                      (1)
  #define NA_TxPhCarrSenseMode_I                        (0x0)

  #define NA_TxPhCarrSensModeOff_VC_C                   (0x0)
  #define NA_TxPhCarrSensModeSymbols_VC_C               (0x1)
  #define NA_TxPhCarrSensModeRssi_VC_C                  (0x2)
  #define NA_TxPhCarrSensModeSymbolsRssi_VC_C           (0x3)

//WO
  #define NA_TxVCarrSensAck_O                           (0x4E)
  #define NA_TxVCarrSensAck_B                           (3)
  #define NA_TxVCarrSensAck_I                           (0x0)

  #define NA_TxVCarrSensAckOff_BC_C                     (0x0)
  #define NA_TxVCarrSensAckOn_BC_C                      (0x1)

//WO
  #define NA_TxArq_O                                    (0x4E)
  #define NA_TxArq_B                                    (4)
  #define NA_TxArq_I                                    (0x1)

  #define NA_TxArqOff_BC_C                              (0x0)
  #define NA_TxArqOn_BC_C                               (0x1)

//WO
  #define NA_Tx3Way_O                                   (0x4E)
  #define NA_Tx3Way_B                                   (5)
  #define NA_Tx3Way_I                                   (0x0)

  #define NA_Tx3WayOff_BC_C                             (0x0)
  #define NA_Tx3WayOn_BC_C                              (0x1)

//WO
  #define NA_TxBackOffAlg_O                             (0x4E)
  #define NA_TxBackOffAlg_B                             (6)
  #define NA_TxBackOffAlg_I                             (0x0)

  #define NA_TxBackOffAlgOff_BC_C                       (0x0)
  #define NA_TxBackOffAlgOn_BC_C                        (0x1)

//WO
  #define NA_TxFragPrio_O                               (0x4E)
  #define NA_TxFragPrio_B                               (7)
  #define NA_TxFragPrio_I                               (0x0)
//------------------------------------------------------------
//WO
  #define NA_TxBackOffSeed_O                            (0x4F)
  #define NA_TxBackOffSeed_MSB                          (7)
  #define NA_TxBackOffSeed_LSB                          (0)
  #define NA_TxBackOffSeed_I                            (0x0)

//------------------------------------------------------------
//WO
  #define NA_TxCryptSeqReset_O                          (0x50)
  #define NA_TxCryptSeqReset_MSB                        (3)
  #define NA_TxCryptSeqReset_LSB                        (0)
  #define NA_TxCryptSeqReset_I                          (0x0)

//WO
  #define NA_TxCryptEn_O                                (0x50)
  #define NA_TxCryptEn_B                                (4)
  #define NA_TxCryptEn_I                                (0x0)

//WO
  #define NA_TxCryptId_O                                (0x50)
  #define NA_TxCryptId_MSB                              (6)
  #define NA_TxCryptId_LSB                              (5)
  #define NA_TxCryptId_I                                (0x0)

//WO
  #define NA_TxCryptSeqN_O                              (0x50)
  #define NA_TxCryptSeqN_B                              (7)
  #define NA_TxCryptSeqN_I                              (0x0)

//------------------------------------------------------------
//WO
  #define NA_TxScrambInit_O                             (0x51)
  #define NA_TxScrambInit_MSB                           (6)
  #define NA_TxScrambInit_LSB                           (0)
  #define NA_TxScrambInit_I                             (0x7F)

//WO
  #define NA_TxScrambEn_O                               (0x51)
  #define NA_TxScrambEn_B                               (7)
  #define NA_TxScrambEn_I                               (0x1)

//------------------------------------------------------------
//WO
  #define NA_TxTransBytes_O                             (0x52)
  #define NA_TxTransBytes_MSB                           (12)
  #define NA_TxTransBytes_LSB                           (0)
  #define NA_TxTransBytes_I                             (0x0)

//------------------------------------------------------------
//WO
  #define NA_TxPacketType_O                             (0x54)
  #define NA_TxPacketType_MSB                           (3)
  #define NA_TxPacketType_LSB                           (0)
  #define NA_TxPacketType_I                             (0x0)

//WO
  #define NA_TxAddrSlct_O                               (0x54)
  #define NA_TxAddrSlct_B                               (7)
  #define NA_TxAddrSlct_I                               (0x0)

//------------------------------------------------------------
//WO
  #define NA_TxCmdStop_O                                (0x55)
  #define NA_TxCmdStop_B                                (0)
  #define NA_TxCmdStop_I                                (0x0)

//WO
  #define NA_TxCmdStart_O                               (0x55)
  #define NA_TxCmdStart_B                               (1)
  #define NA_TxCmdStart_I                               (0x0)

//WO
  #define NA_TxBufferCmd_O                              (0x55)
  #define NA_TxBufferCmd_MSB                            (3)
  #define NA_TxBufferCmd_LSB                            (2)
  #define NA_TxBufferCmd_I                              (0x0)

//------------------------------------------------------------
// config registers for rx only
//------------------------------------------------------------
//WO
  #define NA_RxCmdStop_O                                (0x56)
  #define NA_RxCmdStop_B                                (0)
  #define NA_RxCmdStop_I                                (0x0)

//WO
  #define NA_RxCmdStart_O                               (0x56)
  #define NA_RxCmdStart_B                               (1)
  #define NA_RxCmdStart_I                               (0x0)

//WO
  #define NA_RxBufferCmd_O                              (0x56)
  #define NA_RxBufferCmd_MSB                            (3)
  #define NA_RxBufferCmd_LSB                            (2)
  #define NA_RxBufferCmd_I                              (0x0)

//------------------------------------------------------------
//WO
  #define NA_RxCryptSeqReset_O                          (0x57)
  #define NA_RxCryptSeqReset_MSB                        (3)
  #define NA_RxCryptSeqReset_LSB                        (0)
  #define NA_RxCryptSeqReset_I                          (0x0)

//------------------------------------------------------------
//WO
  #define NA_RxTransBytes_O                             (0x58)
  #define NA_RxTransBytes_MSB                           (12)
  #define NA_RxTransBytes_LSB                           (0)
  #define NA_RxTransBytes_I                             (0x0)

//------------------------------------------------------------
//WO
  #define NA_RxTimeBCrc1Mode_O                          (0x5A)
  #define NA_RxTimeBCrc1Mode_B                          (0)
  #define NA_RxTimeBCrc1Mode_I                          (0x1)

  #define NA_RxTimeBCrc1ModeOff_BC_C                    (0x0)
  #define NA_RxTimeBCrc1ModeOn_BC_C                     (0x1)

//WO
  #define NA_RxCrc2Mode_O                               (0x5A)
  #define NA_RxCrc2Mode_B                               (1)
  #define NA_RxCrc2Mode_I                               (0x1)

  #define NA_RxCrc2ModeTrigOff_BC_C                     (0x0)
  #define NA_RxCrc2ModeTrigOn_BC_C                      (0x1)

//WO
  #define NA_RxArqMode_O                                (0x5A)
  #define NA_RxArqMode_MSB                              (3)
  #define NA_RxArqMode_LSB                              (2)
  #define NA_RxArqMode_I                                (0x2)

  #define NA_RxArqModeNone_VC_C                         (0x0)
  #define NA_RxArqModeCrc1_VC_C                         (0x1)
  #define NA_RxArqModeCrc2_VC_C                         (0x2)

//WO
  #define NA_RxAddrSegEsMode_O                          (0x5A)
  #define NA_RxAddrSegEsMode_B                          (4)
  #define NA_RxAddrSegEsMode_I                          (0x0)

//WO
  #define NA_RxAddrSegIsMode_O                          (0x5A)
  #define NA_RxAddrSegIsMode_B                          (5)
  #define NA_RxAddrSegIsMode_I                          (0x0)

//WO
  #define NA_RxAddrSegDevIdL_O                          (0x5A)
  #define NA_RxAddrSegDevIdL_MSB                        (7)
  #define NA_RxAddrSegDevIdL_LSB                        (6)
  #define NA_RxAddrSegDevIdL_I                          (0x0)

//------------------------------------------------------------
//WO
  #define NA_RxDataEn_O                                 (0x5B)
  #define NA_RxDataEn_B                                 (0)
  #define NA_RxDataEn_I                                 (0x1)
  #define NA_RxDataOff_BC_C                             (0x0)
  #define NA_RxDataOn_BC_C                              (0x1)

//WO
  #define NA_RxBrdcastEn_O                              (0x5B)
  #define NA_RxBrdcastEn_B                              (1)
  #define NA_RxBrdcastEn_I                              (0x1)

  #define NA_RxBrdcastOff_BC_C                          (0x0)
  #define NA_RxBrdcastOn_BC_C                           (0x1)

//WO
  #define NA_RxTimeBEn_O                                (0x5B)
  #define NA_RxTimeBEn_B                                (2)
  #define NA_RxTimeBEn_I                                (0x1)
  #define NA_RxTimeBOff_BC_C                            (0x0)
  #define NA_RxTimeBOn_BC_C                             (0x1)

//WO
  #define NA_RxAddrMode_O                               (0x5B)
  #define NA_RxAddrMode_B                               (3)
  #define NA_RxAddrMode_I                               (0x1)

  #define NA_RxAddrModeOff_BC_C                         (0x0)
  #define NA_RxAddrModeOn_BC_C                          (0x1)

//WO
  #define NA_RangingPulses_O                            (0x5B)
  #define NA_RangingPulses_MSB                          (7)
  #define NA_RangingPulses_LSB                          (4)
  #define NA_RangingPulses_I                            (0x5)

  #define NA_RangingPulses1_VC_C                        (0x0)
  #define NA_RangingPulses2_VC_C                        (0x1)
  #define NA_RangingPulses4_VC_C                        (0x2)
  #define NA_RangingPulses8_VC_C                        (0x3)
  #define NA_RangingPulses16_VC_C                       (0x4)
  #define NA_RangingPulses24_VC_C                       (0x5)
  #define NA_RangingPulses32_VC_C                       (0x6)
  #define NA_RangingPulses40_VC_C                       (0x7)
  #define NA_RangingPulses48_VC_C                       (0x8)
  #define NA_RangingPulses56_VC_C                       (0x9)
  #define NA_RangingPulses64_VC_C                       (0xA)

//------------------------------------------------------------
//WO
  #define NA_PulseDetDelay_O                            (0x5C)
  #define NA_PulseDetDelay_MSB                          (4)
  #define NA_PulseDetDelay_LSB                          (0)
  #define NA_PulseDetDelay_I                            (0x5)

//------------------------------------------------------------
//WO
  #define NA_GateAdjThreshold_O                         (0x5D)
  #define NA_GateAdjThreshold_MSB                       (2)
  #define NA_GateAdjThreshold_LSB                       (0)
  #define NA_GateAdjThreshold_I                         (0x7)

//WO
  #define NA_DownPulseDetectDis_O                       (0x5D)
  #define NA_DownPulseDetectDis_B                       (4)
  #define NA_DownPulseDetectDis_I                       (0x0)

//WO
  #define NA_UpPulseDetectDis_O                         (0x5D)
  #define NA_UpPulseDetectDis_B                         (5)
  #define NA_UpPulseDetectDis_I                         (0x0)

//------------------------------------------------------------
//WO
  #define NA_GateSizeUnsync_O                           (0x5E)
  #define NA_GateSizeUnsync_MSB                         (1)
  #define NA_GateSizeUnsync_LSB                         (0)
  #define NA_GateSizeUnsync_I                           (0x1)

//WO
  #define NA_GateSizeBitsync_O                          (0x5E)
  #define NA_GateSizeBitsync_MSB                        (3)
  #define NA_GateSizeBitsync_LSB                        (2)
  #define NA_GateSizeBitsync_I                          (0x1)

//WO
  #define NA_GateSizeFramesync_O                        (0x5E)
  #define NA_GateSizeFramesync_MSB                      (5)
  #define NA_GateSizeFramesync_LSB                      (4)
  #define NA_GateSizeFramesync_I                        (0x1)

  #define NA_GateSize3Slots_VC_C                        (0x0)
  #define NA_GateSize5Slots_VC_C                        (0x1)
  #define NA_GateSize7Slots_VC_C                        (0x2)
  #define NA_GateSize9Slots_VC_C                        (0x3)

//WO
  #define NA_GateAdjBitsyncEn_O                         (0x5E)
  #define NA_GateAdjBitsyncEn_B                         (6)
  #define NA_GateAdjBitsyncEn_I                         (0x1)

//WO
  #define NA_GateAdjFramesyncEn_O                       (0x5E)
  #define NA_GateAdjFramesyncEn_B                       (7)
  #define NA_GateAdjFramesyncEn_I                       (0x1)

//------------------------------------------------------------
//WO
  #define NA_Go2BitsyncThreshold_O                      (0x5F)
  #define NA_Go2BitsyncThreshold_MSB                    (2)
  #define NA_Go2BitsyncThreshold_LSB                    (0)
  #define NA_Go2BitsyncThreshold_I                      (0x2)

//WO
  #define NA_LeaveBitsyncThreshold_O                    (0x5F)
  #define NA_LeaveBitsyncThreshold_MSB                  (6)
  #define NA_LeaveBitsyncThreshold_LSB                  (4)
  #define NA_LeaveBitsyncThreshold_I                    (0x6)

//------------------------------------------------------------
// registers for real-time clock
//------------------------------------------------------------
//WO
  #define NA_RtcTimeBTxAdj_O                            (0x60)
  #define NA_RtcTimeBTxAdj_MSB                          (7)
  #define NA_RtcTimeBTxAdj_LSB                          (0)
  #define NA_RtcTimeBTxAdj_I                            (0x0)

  #define NA_MacRtcTimeBTxAdj1M2Ary_IC_C                (5)
  #define NA_MacRtcTimeBTxAdj1M4Ary_IC_C                (3)
  #define NA_MacRtcTimeBTxAdj1M2AryFec_IC_C             (7)
  #define NA_MacRtcTimeBTxAdj1M4AryFec_IC_C             (4)
  #define NA_MacRtcTimeBTxAdj500k2Ary_IC_C              (9)
  #define NA_MacRtcTimeBTxAdj500k4Ary_IC_C              (7)
  #define NA_MacRtcTimeBTxAdj500k2AryFec_IC_C           (15)
  #define NA_MacRtcTimeBTxAdj500k4AryFec_IC_C           (13)

//------------------------------------------------------------
//WO
  #define NA_RtcTimeBRxAdj_O                            (0x61)
  #define NA_RtcTimeBRxAdj_MSB                          (7)
  #define NA_RtcTimeBRxAdj_LSB                          (0)
  #define NA_RtcTimeBRxAdj_I                            (0x0)

  #define NA_MacRtcTimeBRxAdj1M2Ary_IC_C                (4)
  #define NA_MacRtcTimeBRxAdj1M4Ary_IC_C                (8)
  #define NA_MacRtcTimeBRxAdj1M2AryFec_IC_C             (9)
  #define NA_MacRtcTimeBRxAdj1M4AryFec_IC_C             (10)
  #define NA_MacRtcTimeBRxAdj500k2Ary_IC_C              (8)
  #define NA_MacRtcTimeBRxAdj500k4Ary_IC_C              (11)
  #define NA_MacRtcTimeBRxAdj500k2AryFec_IC_C           (12)
  #define NA_MacRtcTimeBRxAdj500k4AryFec_IC_C           (13)

//------------------------------------------------------------
//WO
  #define NA_RtcCmdWr_O                                 (0x62)
  #define NA_RtcCmdWr_B                                 (0)
  #define NA_RtcCmdWr_I                                 (0x0)

//WO
  #define NA_RtcCmdRd_O                                 (0x62)
  #define NA_RtcCmdRd_B                                 (1)
  #define NA_RtcCmdRd_I                                 (0x0)

//WO
  #define NA_RtcTimeBAutoMode_O                         (0x62)
  #define NA_RtcTimeBAutoMode_B                         (4)
  #define NA_RtcTimeBAutoMode_I                         (0x0)

  #define NA_RtcTimeBAutoModeOff_BC_C                   (0x0)
  #define NA_RtcTimeBAutoModeOn_BC_C                    (0x1)


//------------------------------------------------------------
// ALTERNATIVE AGC related registers
//------------------------------------------------------------
//WO
  #define NA_AgcAmplitude_O                             (0x63)
  #define NA_AgcAmplitude_MSB                           (4)
  #define NA_AgcAmplitude_LSB                           (0)
  #define NA_AgcAmplitude_I                             (0xC)

//------------------------------------------------------------
//WO
  #define NA_AgcRangeOffset_O                           (0x64)
  #define NA_AgcRangeOffset_MSB                         (3)
  #define NA_AgcRangeOffset_LSB                         (0)
  #define NA_AgcRangeOffset_I                           (0xA)

//WO
  #define NA_UseAlternativeAgc_O                        (0x64)
  #define NA_UseAlternativeAgc_B                        (7)
  #define NA_UseAlternativeAgc_I                        (0x0)

//-------------------------------------------------------------------------------
// RAM ADDRESSES
//-------------------------------------------------------------------------------


  #define NA_RamStaAddr0_O                              (0x80)
  #define NA_RamStaAddr0_MSB                            (47)
  #define NA_RamStaAddr0_LSB                            (0)

  #define NA_RamStaAddr1_O                              (0x88)
  #define NA_RamStaAddr1_MSB                            (47)
  #define NA_RamStaAddr1_LSB                            (0)

  #define NA_RamTxDstAddr_O                             (0x90)
  #define NA_RamTxDstAddr_MSB                           (47)
  #define NA_RamTxDstAddr_LSB                           (0)

// take care that TxRxPacketLength_MSB_c/.LSB_c have the same values than
// RamRxLength_MSB_c/..LSB_c and RamTxLength_MSB_c/..LSB_c
  #define NA_RamTxLength_O                              (0x98)
  #define NA_RamTxLength_MSB                            (12)
  #define NA_RamTxLength_LSB                            (0)

  #define NA_RamTxFragC_O                               (0x99)
  #define NA_RamTxFragC_B                               (5)

  #define NA_RamTxSeqN_O                                (0x99)
  #define NA_RamTxSeqN_B                                (6)

  #define NA_RamTxLCh_O                                 (0x99)
  #define NA_RamTxLCh_B                                 (7)

  #define NA_RamRxDstAddr_O                             (0xA8)
  #define NA_RamRxDstAddr_MSB                           (47)
  #define NA_RamRxDstAddr_LSB                           (0)

  #define NA_RamRxSrcAddr_O                             (0xB0)
  #define NA_RamRxSrcAddr_MSB                           (47)
  #define NA_RamRxSrcAddr_LSB                           (0)

// take care that TxRxPacketLength_MSB_c/.LSB_c have the same values than
// RamRxLength_MSB_c/..LSB_c and RamTxLength_MSB_c/..LSB_c
  #define NA_RamRxLength_O                              (0xB8)
  #define NA_RamRxLength_MSB                            (12)
  #define NA_RamRxLength_LSB                            (0)

  #define NA_RamRxFragC_O                               (0xB9)
  #define NA_RamRxFragC_B                               (5)

  #define NA_RamRxSeqN_O                                (0xB9)
  #define NA_RamRxSeqN_B                                (6)

  #define NA_RamRxLCh_O                                 (0xB9)
  #define NA_RamRxLCh_B                                 (7)

  #define NA_RamRtcTx_O                                 (0xE0)
  #define NA_RamRtcTx_MSB                               (47)
  #define NA_RamRtcTx_LSB                               (0)

  #define NA_RamRtcRx_O                                 (0xE8)
  #define NA_RamRtcRx_MSB                               (47)
  #define NA_RamRtcRx_LSB                               (0)

  #define NA_RamRtcReg_O                                (0xF0)
  #define NA_RamRtcReg_MSB                              (47)
  #define NA_RamRtcReg_LSB                              (0)

  #define NA_RamTxRxCryptKey_O                          (0x180)
  #define NA_RamTxRxCryptKey_MSB                        (127)
  #define NA_RamTxRxCryptKey_LSB                        (0)

  #define NA_RamTxCryptClock_O                          (0x1C0)
  #define NA_RamTxCryptClock_MSB                        (31)
  #define NA_RamTxCryptClock_LSB                        (0)

  #define NA_RamRxCryptClock_O                          (0x1E0)
  #define NA_RamRxCryptClock_MSB                        (31)
  #define NA_RamRxCryptClock_LSB                        (0)

  #define NA_RamRxBuffer_O                              (0x280)
  #define NA_RamRxBuffer_MSB                            (1023)
  #define NA_RamRxBuffer_LSB                            (0)

  #define NA_RamTxBuffer_O                              (0x380)
  #define NA_RamTxBuffer_MSB                            (1023)
  #define NA_RamTxBuffer_LSB                            (0)

  #define NA_RamTxRxBuffer_O                            (0x280)
  #define NA_RamTxRxBuffer_MSB                          (2047)
  #define NA_RamTxRxBuffer_LSB                          (0)

  #define NA_RamRxTransBuffer_O                         (0x80)
  #define NA_RamRxTransBuffer_MSB                       (2047)
  #define NA_RamRxTransBuffer_LSB                       (0)

  #define NA_RamTxTransBuffer_O                         (0x280)
  #define NA_RamTxTransBuffer_MSB                       (2047)
  #define NA_RamTxTransBuffer_LSB                       (0)

  #define NA_RamTxRxTransBuffer_O                       (0x80)
  #define NA_RamTxRxTransBuffer_MSB                     (4095)
  #define NA_RamTxRxTransBuffer_LSB                     (0)

  #define NA_RamCsqDataByte0_O                          (0x80)
  #define NA_RamCsqDataByte0_MSB                        (7)
  #define NA_RamCsqDataByte0_LSB                        (0)
  #define NA_RamCsqDataPage0_O                          (0x0)

  #define NA_RamCsqDataByte1_O                          (0x180)
  #define NA_RamCsqDataByte1_MSB                        (7)
  #define NA_RamCsqDataByte1_LSB                        (0)
  #define NA_RamCsqDataPage1_O                          (0x1)

  #define NA_RamCsqDataByte2_O                          (0x280)
  #define NA_RamCsqDataByte2_MSB                        (7)
  #define NA_RamCsqDataByte2_LSB                        (0)
  #define NA_RamCsqDataPage2_O                          (0x2)

  #define NA_RamD3lPatI_O                               (0x80)
  #define NA_RamD3lPatI_MSB                             (255)
  #define NA_RamD3lPatI_LSB                             (0)
  #define NA_RamD3lPatI_I                               (0x00000000000000000000000000000000B64CCCE30E1F807FFE01F870C733326D)
  #define NA_RamD3lPatIPage_O                           (0x0)

  #define NA_RamD3lPatQ_O                               (0x180)
  #define NA_RamD3lPatQ_MSB                             (255)
  #define NA_RamD3lPatQ_LSB                             (0)
  #define NA_RamD3lPatQ_I                               (0x000000000000000000000000000000006C9999CE387C0FFFFFF03E1C73999936)
  #define NA_RamD3lPatQPage_O                           (0x1)

  #define NA_RamD3lThreshold_O                          (0x280)
  #define NA_RamD3lThresholdPage_O                      (0x2)

  #define NA_RamD3lThresDown_MSB                        (17)
  #define NA_RamD3lThresDown_LSB                        (0)
  #define NA_RamD3lThresDown_I                          (0x6E37)

  #define NA_RamD3lThresUp_MSB                          (17)
  #define NA_RamD3lThresUp_LSB                          (0)
  #define NA_RamD3lThresUp_I                            (0x6E37)

//-------------------------------------------------------------------------------
// CONSTANTS
//-------------------------------------------------------------------------------

  #define NA_True_BC_C                                  (0x1)
  #define NA_False_BC_C                                 (0x0)

// take care that TxRxPacketLength_MSB_c/.LSB_c have the same values than
// RamRxLength_MSB_c/..LSB_c and RamTxLength_MSB_c/..LSB_c
  #define NA_TxRxPacketLength_MSB                       (12)
  #define NA_TxRxPacketLength_LSB                       (0)
  #define NA_TxRxPacketLength_INIT_C                    (0x0)

// used for RxCryptEn and TxCryptEn
  #define NA_TxRxCryptOff_BC_C                          (0x0)
  #define NA_TxRxCryptOn_BC_C                           (0x1)

// bus size for incoming rx value for quantization and agc
  #define NA_ADC_IQ_MSB                                 (4)
  #define NA_ADC_IQ_LSB                                 (0)

  #define NA_Crc1Bits_IC_C                              (16)

// the number of different keys
  #define NA_CryptIdCount_IC_C                          (4)

// the number of bits used for crypt bits
  #define NA_CryptBits_IC_C                             (4)

  #define NA_CryptFieldEn_B                             (0)
  #define NA_CryptFieldId_LSB                           (1)
  #define NA_CryptFieldId_MSB                           (2)
  #define NA_CryptFieldSeqN_B                           (3)

  #define NA_RtcTimeSlotLsbs_IC_C                       (16)

  #define NA_MacTypeBits_IC_C                           (2)

  #define NA_TypeCodeBitPos_IC_C                        (4)
  #define NA_TypeCodeBits_IC_C                          (4)

  #define NA_PwrMngmtClockFreq_IC_C                     (3)

  #define NA_RtcReg_MSB                                 (47)
  #define NA_RtcReg_LSB                                 (0)

  #define NA_TxRxPacketFragC_B                          (5)
  #define NA_TxRxPacketSeqN_B                           (6)
  #define NA_TxRxPacketLCh_B                            (7)

  #define NA_TailSymbols_IC_C                           (4)
  #define NA_SyncWordBits_IC_C                          (64)
  #define NA_ScrambInitBits_IC_C                        (8)
  #define NA_MacCodeBits_IC_C                           (8)
  #define NA_AddrBits_IC_C                              (48)
  #define NA_LengthBits_IC_C                            (16)

  #define NA_TypeCodeData_VC_C                          (0x0)
  #define NA_TypeCodeAck_VC_C                           (0x1)
  #define NA_TypeCodeTimeB_VC_C                         (0x2)
  #define NA_TypeCodeBrdcast_VC_C                       (0x3)
  #define NA_TypeCodeReq2s_VC_C                         (0x4)
  #define NA_TypeCodeClr2s_VC_C                         (0x5)
  #define NA_TypeCodeUndef_VC_C                         (0xFF)

  #define NA_MacType_VC_C                               (0x0)

  #define NA_FeatClockDefaultDio_IC_C                   (3)

  #define NA_MacPreambleSymbols500ns_IC_C               (50)
  #define NA_MacPreambleSymbols1000ns_IC_C              (30)
  #define NA_MacPreambleSymbols2000ns_IC_C              (20)
  #define NA_MacPreambleSymbols4000ns_IC_C              (16)
  #define NA_MacPreambleSymbols8000ns_IC_C              (14)
  #define NA_MacPreambleSymbols16000ns_IC_C             (12)

  #define NA_CrystalFrequency_IC_C                      (32)

#endif	// _NTRXREGMAP_H_
