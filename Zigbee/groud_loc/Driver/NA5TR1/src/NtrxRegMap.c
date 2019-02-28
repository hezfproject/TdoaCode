/******************************************************************************
 Copyright 2010, Walasey Technologies
 Author: WangKun
******************************************************************************/

#include "NtrxChip.h"

const NtrxFlashCode BitMaskTable[8] = { 0x01, 0x03, 0x07, 0x0F, 0x1F, 0x3F, 0x7F, 0xFF } ;

/*=============================================================================
  寄存器描述数据表
  每个寄存器区域数据:
  字节1: 地址
  字节2(Bit6~7): 寄存器读写类型
  字节2(Bit0~3): 起始Bit位(从0开始)
  字节3: 终止Bit位
*/

#define NTRX_TYPE_MASK 0xC0
#define NTRX_WT_TYPE 0x00		// Trigger Reg ( Can be write, but not backup in shadow reg )
#define NTRX_RO_TYPE 0x40		// Read Only
#define NTRX_WO_TYPE 0x80		// Write Only
#define NTRX_RW_TYPE 0xC0		// Read & Write

#define NTRX_FIELD_DES_SIZE     3

const NtrxFlashCode NtrxRegDesTable[] =
{
	// Address                     Type           LSB                              MSB
	//0x00
	NA_SpiBitOrder_O,              NTRX_RW_TYPE + NA_SpiBitOrder_B,                NA_SpiBitOrder_B,                // NA_SpiBitOrder
	NA_SpiTxDriver_O,              NTRX_RW_TYPE + NA_SpiTxDriver_B,                NA_SpiTxDriver_B,                // NA_SpiTxDriver
	NA_IrqPolarity_O,              NTRX_RW_TYPE + NA_IrqPolarity_B,                NA_IrqPolarity_B,                // NA_IrqPolarity
	NA_IrqDriver_O,                NTRX_RW_TYPE + NA_IrqDriver_B,                  NA_IrqDriver_B,                  // NA_IrqDriver
	//0x01
	NA_Version_O,                  NTRX_RO_TYPE + NA_Version_LSB,                  NA_Version_MSB,                  // NA_Version
	NA_WakeUpTimeByte_O,           NTRX_WT_TYPE + NA_WakeUpTimeByte_LSB,           NA_WakeUpTimeByte_MSB,           // NA_WakeUpTimeByte
	//0x02
	NA_Revision_O,                 NTRX_RO_TYPE + NA_Revision_LSB,                 NA_Revision_MSB,                 // NA_Revision
	NA_WakeUpTimeWe_O,             NTRX_WO_TYPE + NA_WakeUpTimeWe_LSB,             NA_WakeUpTimeWe_MSB,             // NA_WakeUpTimeWe
	//0x03
	NA_BattMgmtEnable_O,           NTRX_RW_TYPE + NA_BattMgmtEnable_B,             NA_BattMgmtEnable_B,             // NA_BattMgmtEnable
	NA_BattMgmtThreshold_O,        NTRX_RW_TYPE + NA_BattMgmtThreshold_LSB,        NA_BattMgmtThreshold_MSB,        // NA_BattMgmtThreshold
	NA_BattMgmtCompare_O,          NTRX_RO_TYPE + NA_BattMgmtCompare_B,            NA_BattMgmtCompare_B,            // NA_BattMgmtCompare
	//0x04
	NA_DioDirection_O,             NTRX_WO_TYPE + NA_DioDirection_B,               NA_DioDirection_B,               // NA_DioDirection
	NA_DioOutValueAlarmEnable_O,   NTRX_WO_TYPE + NA_DioOutValueAlarmEnable_B,     NA_DioOutValueAlarmEnable_B,     // NA_DioOutValueAlarmEnable
	NA_DioAlarmStart_O,            NTRX_WO_TYPE + NA_DioAlarmStart_B,              NA_DioAlarmStart_B,              // NA_DioAlarmStart
	NA_DioAlarmPolarity_O,         NTRX_WO_TYPE + NA_DioAlarmPolarity_B,           NA_DioAlarmPolarity_B,           // NA_DioAlarmPolarity
	NA_DioUsePullup_O,             NTRX_WO_TYPE + NA_DioUsePullup_B,               NA_DioUsePullup_B,               // NA_DioUsePullup
	NA_DioUsePulldown_O,           NTRX_WO_TYPE + NA_DioUsePulldown_B,             NA_DioUsePulldown_B,             // NA_DioUsePulldown
	NA_DioInValueAlarmStatus_O,    NTRX_RO_TYPE + NA_DioInValueAlarmStatus_LSB,    NA_DioInValueAlarmStatus_MSB,    // NA_DioInValueAlarmStatus
	//0x05
	NA_DioPortWe_O,                NTRX_WO_TYPE + NA_DioPortWe_LSB,                NA_DioPortWe_MSB,                // NA_DioPortWe
	//0x06
	NA_EnableWakeUpRtc_O,          NTRX_RW_TYPE + NA_EnableWakeUpRtc_B,            NA_EnableWakeUpRtc_B,            // NA_EnableWakeUpRtc
	NA_EnableWakeUpDio_O,          NTRX_RW_TYPE + NA_EnableWakeUpDio_B,            NA_EnableWakeUpDio_B,            // NA_EnableWakeUpDio
	NA_PowerUpTime_O,              NTRX_RW_TYPE + NA_PowerUpTime_LSB,              NA_PowerUpTime_MSB,              // NA_PowerUpTime
	NA_PowerDownMode_O,            NTRX_RW_TYPE + NA_PowerDownMode_B,              NA_PowerDownMode_B,              // NA_PowerDownMode
	//0x07
	NA_PowerDown_O,                NTRX_WO_TYPE + NA_PowerDown_B,                  NA_PowerDown_B,                  // NA_PowerDown
	NA_ResetBbClockGate_O,         NTRX_RW_TYPE + NA_ResetBbClockGate_B,           NA_ResetBbClockGate_B,           // NA_ResetBbClockGate
	NA_ResetBbRadioCtrl_O,         NTRX_RW_TYPE + NA_ResetBbRadioCtrl_B,           NA_ResetBbRadioCtrl_B,           // NA_ResetBbRadioCtrl
	NA_UsePullup4Test_O,           NTRX_WO_TYPE + NA_UsePullup4Test_B,             NA_UsePullup4Test_B,             // NA_UsePullup4Test
	NA_UsePulldown4Test_O,         NTRX_WO_TYPE + NA_UsePulldown4Test_B,           NA_UsePulldown4Test_B,           // NA_UsePulldown4Test
	//0x08
	NA_EnableBbCrystal_O,          NTRX_RW_TYPE + NA_EnableBbCrystal_B,            NA_EnableBbCrystal_B,            // NA_EnableBbCrystal
	NA_EnableBbClock_O,            NTRX_RW_TYPE + NA_EnableBbClock_B,              NA_EnableBbClock_B,              // NA_EnableBbClock
	NA_BypassBbCrystal_O,          NTRX_RW_TYPE + NA_BypassBbCrystal_B,            NA_BypassBbCrystal_B,            // NA_BypassBbCrystal
	NA_FeatureClockFreq_O,         NTRX_RW_TYPE + NA_FeatureClockFreq_LSB,         NA_FeatureClockFreq_MSB,         // NA_FeatureClockFreq
	NA_EnableFeatureClock_O,       NTRX_RW_TYPE + NA_EnableFeatureClock_B,         NA_EnableFeatureClock_B,         // NA_EnableFeatureClock
	//0x09
	NA_UsePullup4Spiclk_O,         NTRX_WO_TYPE + NA_UsePullup4Spiclk_B,           NA_UsePullup4Spiclk_B,           // NA_UsePullup4Spiclk
	NA_UsePulldown4Spiclk_O,       NTRX_WO_TYPE + NA_UsePulldown4Spiclk_B,         NA_UsePulldown4Spiclk_B,         // NA_UsePulldown4Spiclk
	NA_UsePullup4Spissn_O,         NTRX_WO_TYPE + NA_UsePullup4Spissn_B,           NA_UsePullup4Spissn_B,           // NA_UsePullup4Spissn
	NA_UsePulldown4Spissn_O,       NTRX_WO_TYPE + NA_UsePulldown4Spissn_B,         NA_UsePulldown4Spissn_B,         // NA_UsePulldown4Spissn
	NA_UsePullup4Spirxd_O,         NTRX_WO_TYPE + NA_UsePullup4Spirxd_B,           NA_UsePullup4Spirxd_B,           // NA_UsePullup4Spirxd
	NA_UsePulldown4Spirxd_O,       NTRX_WO_TYPE + NA_UsePulldown4Spirxd_B,         NA_UsePulldown4Spirxd_B,         // NA_UsePulldown4Spirxd
	NA_UsePullup4Spitxd_O,         NTRX_WO_TYPE + NA_UsePullup4Spitxd_B,           NA_UsePullup4Spitxd_B,           // NA_UsePullup4Spitxd
	NA_UsePulldown4Spitxd_O,       NTRX_WO_TYPE + NA_UsePulldown4Spitxd_B,         NA_UsePulldown4Spitxd_B,         // NA_UsePulldown4Spitxd
	//0x0a
	NA_UsePullup4Por_O,            NTRX_WO_TYPE + NA_UsePullup4Por_B,              NA_UsePullup4Por_B,              // NA_UsePullup4Por
	NA_UsePulldown4Por_O,          NTRX_WO_TYPE + NA_UsePulldown4Por_B,            NA_UsePulldown4Por_B,            // NA_UsePulldown4Por
	NA_UsePullup4Pamp_O,           NTRX_WO_TYPE + NA_UsePullup4Pamp_B,             NA_UsePullup4Pamp_B,             // NA_UsePullup4Pamp
	NA_UsePulldown4Pamp_O,         NTRX_WO_TYPE + NA_UsePulldown4Pamp_B,           NA_UsePulldown4Pamp_B,           // NA_UsePulldown4Pamp
	NA_UsePullup4Ucirq_O,          NTRX_WO_TYPE + NA_UsePullup4Ucirq_B,            NA_UsePullup4Ucirq_B,            // NA_UsePullup4Ucirq
	NA_UsePulldown4Ucirq_O,        NTRX_WO_TYPE + NA_UsePulldown4Ucirq_B,          NA_UsePulldown4Ucirq_B,          // NA_UsePulldown4Ucirq
	NA_UsePullup4Ucrst_O,          NTRX_WO_TYPE + NA_UsePullup4Ucrst_B,            NA_UsePullup4Ucrst_B,            // NA_UsePullup4Ucrst
	NA_UsePulldown4Ucrst_O,        NTRX_WO_TYPE + NA_UsePulldown4Ucrst_B,          NA_UsePulldown4Ucrst_B,          // NA_UsePulldown4Ucrst
	//0x0b
	NA_WritePulls4Spi_O,           NTRX_WO_TYPE + NA_WritePulls4Spi_B,             NA_WritePulls4Spi_B,             // NA_WritePulls4Spi
	NA_WritePulls4Pads_O,          NTRX_WO_TYPE + NA_WritePulls4Pads_B,            NA_WritePulls4Pads_B,            // NA_WritePulls4Pads
	//0x0e
	NA_RamIndex_O,                 NTRX_RW_TYPE + NA_RamIndex_LSB,                 NA_RamIndex_MSB,                 // NA_RamIndex
	NA_DeviceSelect_O,             NTRX_RW_TYPE + NA_DeviceSelect_LSB,             NA_DeviceSelect_MSB,             // NA_DeviceSelect
	//0x0f
	NA_TxIrqEnable_O,              NTRX_RW_TYPE + NA_TxIrqEnable_B,                NA_TxIrqEnable_B,                // NA_TxIrqEnable
	NA_RxIrqEnable_O,              NTRX_RW_TYPE + NA_RxIrqEnable_B,                NA_RxIrqEnable_B,                // NA_RxIrqEnable
	NA_BbTimerIrqEnable_O,         NTRX_RW_TYPE + NA_BbTimerIrqEnable_B,           NA_BbTimerIrqEnable_B,           // NA_BbTimerIrqEnable
	NA_LoIrqEnable_O,              NTRX_RW_TYPE + NA_LoIrqEnable_B,                NA_LoIrqEnable_B,                // NA_LoIrqEnable
	NA_TxIrqStatus_O,              NTRX_RO_TYPE + NA_TxIrqStatus_B,                NA_TxIrqStatus_B,                // NA_TxIrqStatus
	NA_RxIrqStatus_O,              NTRX_RO_TYPE + NA_RxIrqStatus_B,                NA_RxIrqStatus_B,                // NA_RxIrqStatus
	NA_BbTimerIrqStatus_O,         NTRX_RO_TYPE + NA_BbTimerIrqStatus_B,           NA_BbTimerIrqStatus_B,           // NA_BbTimerIrqStatus
	NA_LoIrqStatus_O,              NTRX_RO_TYPE + NA_LoIrqStatus_B,                NA_LoIrqStatus_B,                // NA_LoIrqStatus
	//0x10
	NA_TxIntsRawStat_O,            NTRX_RO_TYPE + NA_TxIntsRawStat_LSB,            NA_TxIntsRawStat_MSB,            // NA_TxIntsRawStat
	NA_TxIntsReset_O,              NTRX_WT_TYPE + NA_TxIntsReset_LSB,              NA_TxIntsReset_MSB,              // NA_TxIntsReset
	//0x11
	NA_RxIntsRawStat_O,            NTRX_RO_TYPE + NA_RxIntsRawStat_LSB,            NA_RxIntsRawStat_MSB,            // NA_RxIntsRawStat
	NA_RxIntsReset_O,              NTRX_WT_TYPE + NA_RxIntsReset_LSB,              NA_RxIntsReset_MSB,              // NA_RxIntsReset
	//0x12
	NA_LoIntsRawStat_O,            NTRX_RO_TYPE + NA_LoIntsRawStat_B,              NA_LoIntsRawStat_B,              // NA_LoIntsRawStat
	NA_LoIntsReset_O,              NTRX_WT_TYPE + NA_LoIntsReset_LSB,              NA_LoIntsReset_MSB,              // NA_LoIntsReset
	NA_ClearBasebandTimerInt_O,    NTRX_WT_TYPE + NA_ClearBasebandTimerInt_B,      NA_ClearBasebandTimerInt_B,      // NA_ClearBasebandTimerInt
	//0x13
	NA_TxIntsEn_O,                 NTRX_WO_TYPE + NA_TxIntsEn_LSB,                 NA_TxIntsEn_MSB,                 // NA_TxIntsEn
	//0x14
	NA_RxIntsEn_O,                 NTRX_WO_TYPE + NA_RxIntsEn_LSB,                 NA_RxIntsEn_MSB,                 // NA_RxIntsEn
	//0x15
	NA_LoIntsEn_O,                 NTRX_WO_TYPE + NA_LoIntsEn_B,                   NA_LoIntsEn_B,                   // NA_LoIntsEn

	// LO related registers
	//0x16
	NA_LoRxCapsValue_O,            NTRX_RW_TYPE + NA_LoRxCapsValue_LSB,            NA_LoRxCapsValue_MSB,            // NA_LoRxCapsValue
	//0x19
	NA_LoTxCapsValue_O,            NTRX_RW_TYPE + NA_LoTxCapsValue_LSB,            NA_LoTxCapsValue_MSB,            // NA_LoTxCapsValue
	//0x1c
	NA_LoEnableFastTuning_O,       NTRX_WO_TYPE + NA_LoEnableFastTuning_B,         NA_LoEnableFastTuning_B,         // NA_LoEnableFastTuning
	NA_LoFastTuningLevel_O,        NTRX_WO_TYPE + NA_LoFastTuningLevel_LSB,        NA_LoFastTuningLevel_MSB,        // NA_LoFastTuningLevel
	NA_LoEnableLsbNeg_O,           NTRX_WO_TYPE + NA_LoEnableLsbNeg_B,             NA_LoEnableLsbNeg_B,             // NA_LoEnableLsbNeg
	NA_UseLoRxCaps_O,              NTRX_WO_TYPE + NA_UseLoRxCaps_B,                NA_UseLoRxCaps_B,                // NA_UseLoRxCaps
	//0x1d
	NA_LoTargetValue_O,            NTRX_RW_TYPE + NA_LoTargetValue_LSB,            NA_LoTargetValue_MSB,            // NA_LoTargetValue
	//0x1f
	NA_AgcThresHold1_O,            NTRX_WO_TYPE + NA_AgcThresHold1_LSB,            NA_AgcThresHold1_MSB,            // NA_AgcThresHold1
	//0x20
	NA_AgcThresHold2_O,            NTRX_WO_TYPE + NA_AgcThresHold2_LSB,            NA_AgcThresHold2_MSB,            // NA_AgcThresHold2
	//0x21
	NA_HoldAgcInBitSync_O,         NTRX_WO_TYPE + NA_HoldAgcInBitSync_LSB,         NA_HoldAgcInBitSync_MSB,         // NA_HoldAgcInBitSync
	NA_HoldAgcInFrameSync_O,       NTRX_WO_TYPE + NA_HoldAgcInFrameSync_B,         NA_HoldAgcInFrameSync_B,         // NA_HoldAgcInFrameSync
	//0x22
	NA_AgcDeadTime_O,              NTRX_WO_TYPE + NA_AgcDeadTime_LSB,              NA_AgcDeadTime_MSB,              // NA_AgcDeadTime
	NA_AgcNregLength_O,            NTRX_WO_TYPE + NA_AgcNregLength_LSB,            NA_AgcNregLength_MSB,            // NA_AgcNregLength
	//0x23
	NA_AgcIntTime_O,               NTRX_WO_TYPE + NA_AgcIntTime_LSB,               NA_AgcIntTime_MSB,               // NA_AgcIntTime

	// AGC related registers
	//0x25
	NA_AgcValue_O,                 NTRX_WO_TYPE + NA_AgcValue_LSB,                 NA_AgcValue_MSB,                 // NA_AgcValue
	NA_AgcDefaultEn_O,             NTRX_WO_TYPE + NA_AgcDefaultEn_B,               NA_AgcDefaultEn_B,               // NA_AgcDefaultEn
	NA_AgcHold_O,                  NTRX_WO_TYPE + NA_AgcHold_B,                    NA_AgcHold_B,                    // NA_AgcHold
	//0x26
	NA_AgcRssiThres_O,             NTRX_WO_TYPE + NA_AgcRssiThres_LSB,             NA_AgcRssiThres_MSB,             // NA_AgcRssiThres
	NA_AgcGain_O,                  NTRX_RO_TYPE + NA_AgcGain_LSB,                  NA_AgcGain_MSB,                  // NA_AgcGain
	//0x27
	NA_ChirpFilterCaps_O,          NTRX_WO_TYPE + NA_ChirpFilterCaps_LSB,          NA_ChirpFilterCaps_MSB,          // NA_ChirpFilterCaps
	NA_FctClockEn_O,               NTRX_WO_TYPE + NA_FctClockEn_B,                 NA_FctClockEn_B,                 // NA_FctClockEn
	NA_StartFctMeasure_O,          NTRX_WO_TYPE + NA_StartFctMeasure_B,            NA_StartFctMeasure_B,            // NA_StartFctMeasure
	NA_EnableTx_O,                 NTRX_WO_TYPE + NA_EnableTx_B,                   NA_EnableTx_B,                   // NA_EnableTx
	NA_FctPeriod_O,                NTRX_RO_TYPE + NA_FctPeriod_LSB,                NA_FctPeriod_MSB,                // NA_FctPeriod
	//0x28
	NA_BasebandTimerStartValue_O,  NTRX_WO_TYPE + NA_BasebandTimerStartValue_LSB,  NA_BasebandTimerStartValue_MSB,	// NA_BasebandTimerStartValue
	//0x2a
	NA_SyncWord_O,                 NTRX_RW_TYPE + NA_SyncWord_LSB,                 NA_SyncWord_MSB,                 // NA_SyncWord
	NA_ToaOffsetMeanAck_O,         NTRX_RO_TYPE + NA_ToaOffsetMeanAck_LSB,         NA_ToaOffsetMeanAck_MSB,         // NA_ToaOffsetMeanAck
	//0x2b
	NA_ToaOffsetMeanAckValid_O,    NTRX_RO_TYPE + NA_ToaOffsetMeanAckValid_B,      NA_ToaOffsetMeanAckValid_B,      // NA_ToaOffsetMeanAckValid
	//0x2c
	NA_TxRespTime_O,               NTRX_RO_TYPE + NA_TxRespTime_LSB,               NA_TxRespTime_MSB,               // NA_TxRespTime
	//0x2e
	NA_PhaseOffsetData_O,          NTRX_RO_TYPE + NA_PhaseOffsetData_LSB,          NA_PhaseOffsetData_MSB,          // NA_PhaseOffsetData
	NA_PhaseOffsetAck_O,           NTRX_RO_TYPE + NA_PhaseOffsetAck_LSB,           NA_PhaseOffsetAck_MSB,           // NA_PhaseOffsetAck
	//0x2f
	NA_ToaOffsetMeanData_O,        NTRX_RO_TYPE + NA_ToaOffsetMeanData_LSB,        NA_ToaOffsetMeanData_MSB,        // NA_ToaOffsetMeanData
	//0x30
	NA_ToaOffsetMeanDataValid_O,   NTRX_RO_TYPE + NA_ToaOffsetMeanDataValid_B,     NA_ToaOffsetMeanDataValid_B,     // NA_ToaOffsetMeanDataValid
	//0x31
	NA_RxPacketType_O,             NTRX_RO_TYPE + NA_RxPacketType_LSB,             NA_RxPacketType_MSB,             // NA_RxPacketType
	NA_RxAddrMatch_O,              NTRX_RO_TYPE + NA_RxAddrMatch_LSB,              NA_RxAddrMatch_MSB,              // NA_RxAddrMatch
	NA_RxCrc1Stat_O,               NTRX_RO_TYPE + NA_RxCrc1Stat_B,                 NA_RxCrc1Stat_B,                 // NA_RxCrc1Stat
	NA_RxCrc2Stat_O,               NTRX_RO_TYPE + NA_RxCrc2Stat_B,                 NA_RxCrc2Stat_B,                 // NA_RxCrc2Stat
	//0x32
	NA_RxCorrBitErr_O,             NTRX_RO_TYPE + NA_RxCorrBitErr_LSB,             NA_RxCorrBitErr_MSB,             // NA_RxCorrBitErr
	NA_RxCorrErrThres_O,           NTRX_RO_TYPE + NA_RxCorrErrThres_LSB,           NA_RxCorrErrThres_MSB,           // NA_RxCorrErrThres
	//0x33
	NA_RxAddrSegEsMatch_O,         NTRX_RO_TYPE + NA_RxAddrSegEsMatch_B,           NA_RxAddrSegEsMatch_B,           // NA_RxAddrSegEsMatch
	NA_RxAddrSegIsMatch_O,         NTRX_RO_TYPE + NA_RxAddrSegIsMatch_B,           NA_RxAddrSegIsMatch_B,           // NA_RxAddrSegIsMatch
	NA_RxCryptEn_O,                NTRX_RO_TYPE + NA_RxCryptEn_B,                  NA_RxCryptEn_B,                  // NA_RxCryptEn
	NA_RxCryptId_O,                NTRX_RO_TYPE + NA_RxCryptId_LSB,                NA_RxCryptId_MSB,                // NA_RxCryptId
	NA_RxCryptSeqN_O,              NTRX_RO_TYPE + NA_RxCryptSeqN_B,                NA_RxCryptSeqN_B,                // NA_RxCryptSeqN
	//0x37
	NA_TxTimeSlotControl_O,        NTRX_WO_TYPE + NA_TxTimeSlotControl_B,          NA_TxTimeSlotControl_B,          // NA_TxTimeSlotControl
	NA_RxTimeSlotControl_O,        NTRX_RO_TYPE + NA_RxTimeSlotControl_B,          NA_RxTimeSlotControl_B,          // NA_RxTimeSlotControl
	//0x3c
	NA_TxArqCnt_O,                 NTRX_RO_TYPE + NA_TxArqCnt_LSB,                 NA_TxArqCnt_MSB,                 // NA_TxArqCnt
	NA_TxArqMax_O,                 NTRX_WO_TYPE + NA_TxArqMax_LSB,                 NA_TxArqMax_MSB,                 // NA_TxArqMax
	//0x3d
	NA_CsqDitherValue_O,           NTRX_WO_TYPE + NA_CsqDitherValue_LSB,           NA_CsqDitherValue_MSB,           // NA_CsqDitherValue
	NA_CsqUsePhaseShift_O,         NTRX_WO_TYPE + NA_CsqUsePhaseShift_B,           NA_CsqUsePhaseShift_B,           // NA_CsqUsePhaseShift
	NA_CsqUse4Phases_O,            NTRX_WO_TYPE + NA_CsqUse4Phases_B,              NA_CsqUse4Phases_B,              // NA_CsqUse4Phases
	NA_CsqAsyMode_O,               NTRX_WO_TYPE + NA_CsqAsyMode_B,                 NA_CsqAsyMode_B,                 // NA_CsqAsyMode
	NA_CsqMemAddrInit_O,           NTRX_WO_TYPE + NA_CsqMemAddrInit_B,             NA_CsqMemAddrInit_B,             // NA_CsqMemAddrInit
	NA_CsqUseRam_O,                NTRX_WO_TYPE + NA_CsqUseRam_B,                  NA_CsqUseRam_B,                  // NA_CsqUseRam
	//0x3f
	NA_D3lFixnMap_O,               NTRX_WO_TYPE + NA_D3lFixnMap_B,                 NA_D3lFixnMap_B,                 // NA_D3lFixnMap
	NA_D3lPomEn_O,                 NTRX_WO_TYPE + NA_D3lPomEn_B,                   NA_D3lPomEn_B,                   // NA_D3lPomEn
	NA_D3lPomLen_O,                NTRX_WO_TYPE + NA_D3lPomLen_LSB,                NA_D3lPomLen_MSB,                // NA_D3lPomLen
	NA_D3lUpDownEx_O,              NTRX_WO_TYPE + NA_D3lUpDownEx_B,                NA_D3lUpDownEx_B,                // NA_D3lUpDownEx
	//0x40
	NA_LeaveMapThresh1InBitsync_O, NTRX_WO_TYPE + NA_LeaveMapThresh1InBitsync_LSB, NA_LeaveMapThresh1InBitsync_MSB, // NA_LeaveMapThresh1InBitsync
	NA_UseMapThresh1InFramesync_O, NTRX_WO_TYPE + NA_UseMapThresh1InFramesync_B,   NA_UseMapThresh1InFramesync_B,   // NA_UseMapThresh1InFramesync
	//0x41
	NA_Go2MapThresh1InBitsync_O,   NTRX_WO_TYPE + NA_Go2MapThresh1InBitsync_LSB,   NA_Go2MapThresh1InBitsync_MSB,   // NA_Go2MapThresh1InBitsync
	//0x42
	NA_EnableLO_O,                 NTRX_WO_TYPE + NA_EnableLO_B,                   NA_EnableLO_B,                   // NA_EnableLO
	NA_EnableLOdiv10_O,            NTRX_WO_TYPE + NA_EnableLOdiv10_B,              NA_EnableLOdiv10_B,              // NA_EnableLOdiv10
	NA_EnableCsqClock_O,           NTRX_WO_TYPE + NA_EnableCsqClock_B,             NA_EnableCsqClock_B,             // NA_EnableCsqClock
	NA_InvertRxClock_O,            NTRX_WO_TYPE + NA_InvertRxClock_B,              NA_InvertRxClock_B,              // NA_InvertRxClock
	NA_EnableExtPA_O,              NTRX_WO_TYPE + NA_EnableExtPA_B,                NA_EnableExtPA_B,                // NA_EnableExtPA
	//0x43
	NA_LnaFreqAdjust_O,            NTRX_WO_TYPE + NA_LnaFreqAdjust_LSB,            NA_LnaFreqAdjust_MSB,            // NA_LnaFreqAdjust
	NA_TxPaBias_O,                 NTRX_WO_TYPE + NA_TxPaBias_LSB,                 NA_TxPaBias_MSB,                 // NA_TxPaBias
	//0x44
	NA_TxOutputPower0_O,           NTRX_WO_TYPE + NA_TxOutputPower0_LSB,           NA_TxOutputPower0_MSB,           // NA_TxOutputPower0
	//0x45
	NA_TxOutputPower1_O,           NTRX_WO_TYPE + NA_TxOutputPower1_LSB,           NA_TxOutputPower1_MSB,           // NA_TxOutputPower1
	//0x46
	NA_RfRxCompValueI_O,           NTRX_WO_TYPE + NA_RfRxCompValueI_LSB,           NA_RfRxCompValueI_MSB,           // NA_RfRxCompValueI
	//0x47
	NA_RfRxCompValueQ_O,           NTRX_WO_TYPE + NA_RfRxCompValueQ_LSB,           NA_RfRxCompValueQ_MSB,           // NA_RfRxCompValueQ
	//0x48
	NA_SymbolDur_O,                NTRX_WO_TYPE + NA_SymbolDur_LSB,                NA_SymbolDur_MSB,                // NA_SymbolDur
	NA_SymbolRate_O,               NTRX_WO_TYPE + NA_SymbolRate_LSB,               NA_SymbolRate_MSB,               // NA_SymbolRate
	NA_ModulationSystem_O,         NTRX_WO_TYPE + NA_ModulationSystem_B,           NA_ModulationSystem_B,           // NA_ModulationSystem
	//0x49
	NA_Crc2Type_O,                 NTRX_WO_TYPE + NA_Crc2Type_LSB,                 NA_Crc2Type_MSB,                 // NA_Crc2Type
	NA_UseFec_O,                   NTRX_WO_TYPE + NA_UseFec_B,                     NA_UseFec_B,                     // NA_UseFec
	NA_TxRxCryptCrc2Mode_O,        NTRX_WO_TYPE + NA_TxRxCryptCrc2Mode_B,          NA_TxRxCryptCrc2Mode_B,          // NA_TxRxCryptCrc2Mode
	NA_TxRxCryptClkMode_O,         NTRX_WO_TYPE + NA_TxRxCryptClkMode_LSB,         NA_TxRxCryptClkMode_MSB,         // NA_TxRxCryptClkMode
	//0x4a
	NA_SwapBbBuffers_O,            NTRX_RW_TYPE + NA_SwapBbBuffers_B,              NA_SwapBbBuffers_B,              // NA_SwapBbBuffers
	NA_TxRxBbBufferMode1_O,        NTRX_WO_TYPE + NA_TxRxBbBufferMode1_B,          NA_TxRxBbBufferMode1_B,          // NA_TxRxBbBufferMode1
	NA_TxRxBbBufferMode0_O,        NTRX_WO_TYPE + NA_TxRxBbBufferMode0_B,          NA_TxRxBbBufferMode0_B,          // NA_TxRxBbBufferMode0
	NA_FdmaEnable_O,               NTRX_WO_TYPE + NA_FdmaEnable_B,                 NA_FdmaEnable_B,                 // NA_FdmaEnable
	NA_TxRxMode_O,                 NTRX_WO_TYPE + NA_TxRxMode_B,                   NA_TxRxMode_B,                   // NA_TxRxMode
	//0x4b
	NA_ChirpMatrix0_O,             NTRX_WO_TYPE + NA_ChirpMatrix0_LSB,             NA_ChirpMatrix0_MSB,             // NA_ChirpMatrix0
	NA_ChirpMatrix1_O,             NTRX_WO_TYPE + NA_ChirpMatrix1_LSB,             NA_ChirpMatrix1_MSB,             // NA_ChirpMatrix1
	//0x4c
	NA_ChirpMatrix2_O,             NTRX_WO_TYPE + NA_ChirpMatrix2_LSB,             NA_ChirpMatrix2_MSB,             // NA_ChirpMatrix2
	NA_ChirpMatrix3_O,             NTRX_WO_TYPE + NA_ChirpMatrix3_LSB,             NA_ChirpMatrix3_MSB,             // NA_ChirpMatrix3
	//0x4d
	NA_TxPreTrailMatrix0_O,        NTRX_WO_TYPE + NA_TxPreTrailMatrix0_LSB,        NA_TxPreTrailMatrix0_MSB,        // NA_TxPreTrailMatrix0
	NA_TxPreTrailMatrix1_O,        NTRX_WO_TYPE + NA_TxPreTrailMatrix1_LSB,        NA_TxPreTrailMatrix1_MSB,        // NA_TxPreTrailMatrix1
	NA_TxUnderrunIgnore_O,         NTRX_WO_TYPE + NA_TxUnderrunIgnore_B,           NA_TxUnderrunIgnore_B,           // NA_TxUnderrunIgnore
	NA_TxMacCifsDis_O,             NTRX_WO_TYPE + NA_TxMacCifsDis_B,               NA_TxMacCifsDis_B,               // NA_TxMacCifsDis
	//0x4e
	NA_TxVCarrSens_O,              NTRX_WO_TYPE + NA_TxVCarrSens_B,                NA_TxVCarrSens_B,                // NA_TxVCarrSens
	NA_TxPhCarrSenseMode_O,        NTRX_WO_TYPE + NA_TxPhCarrSenseMode_LSB,        NA_TxPhCarrSenseMode_MSB,        // NA_TxPhCarrSenseMode
	NA_TxVCarrSensAck_O,           NTRX_WO_TYPE + NA_TxVCarrSensAck_B,             NA_TxVCarrSensAck_B,             // NA_TxVCarrSensAck
	NA_TxArq_O,                    NTRX_WO_TYPE + NA_TxArq_B,                      NA_TxArq_B,                      // NA_TxArq
	NA_Tx3Way_O,                   NTRX_WO_TYPE + NA_Tx3Way_B,                     NA_Tx3Way_B,                     // NA_Tx3Way
	NA_TxBackOffAlg_O,             NTRX_WO_TYPE + NA_TxBackOffAlg_B,               NA_TxBackOffAlg_B,               // NA_TxBackOffAlg
	NA_TxFragPrio_O,               NTRX_WO_TYPE + NA_TxFragPrio_B,                 NA_TxFragPrio_B,                 // NA_TxFragPrio
	//0x4f
	NA_TxBackOffSeed_O,            NTRX_WO_TYPE + NA_TxBackOffSeed_LSB,            NA_TxBackOffSeed_MSB,            // NA_TxBackOffSeed
	//0x50
	NA_TxCryptSeqReset_O,          NTRX_WO_TYPE + NA_TxCryptSeqReset_LSB,          NA_TxCryptSeqReset_MSB,          // NA_TxCryptSeqReset
	NA_TxCryptEn_O,                NTRX_WO_TYPE + NA_TxCryptEn_B,                  NA_TxCryptEn_B,                  // NA_TxCryptEn
	NA_TxCryptId_O,                NTRX_WO_TYPE + NA_TxCryptId_LSB,                NA_TxCryptId_MSB,                // NA_TxCryptId
	NA_TxCryptSeqN_O,              NTRX_WO_TYPE + NA_TxCryptSeqN_B,                NA_TxCryptSeqN_B,                // NA_TxCryptSeqN
	//0x51
	NA_TxScrambInit_O,             NTRX_WO_TYPE + NA_TxScrambInit_LSB,             NA_TxScrambInit_MSB,             // NA_TxScrambInit
	NA_TxScrambEn_O,               NTRX_WO_TYPE + NA_TxScrambEn_B,                 NA_TxScrambEn_B,                 // NA_TxScrambEn
	//0x54
	NA_TxPacketType_O,             NTRX_WO_TYPE + NA_TxPacketType_LSB,             NA_TxPacketType_MSB,             // NA_TxPacketType
	NA_TxAddrSlct_O,               NTRX_WO_TYPE + NA_TxAddrSlct_B,                 NA_TxAddrSlct_B,                 // NA_TxAddrSlct
	//0x55
	NA_TxCmdStop_O,                NTRX_WT_TYPE + NA_TxCmdStop_B,                  NA_TxCmdStop_B,                  // NA_TxCmdStop
	NA_TxCmdStart_O,               NTRX_WT_TYPE + NA_TxCmdStart_B,                 NA_TxCmdStart_B,                 // NA_TxCmdStart
	NA_TxBufferCmd_O,              NTRX_WT_TYPE + NA_TxBufferCmd_LSB,              NA_TxBufferCmd_MSB,              // NA_TxBufferCmd
	//0x56
	NA_RxCmdStop_O,                NTRX_WT_TYPE + NA_RxCmdStop_B,                  NA_RxCmdStop_B,                  // NA_RxCmdStop
	NA_RxCmdStart_O,               NTRX_WT_TYPE + NA_RxCmdStart_B,                 NA_RxCmdStart_B,                 // NA_RxCmdStart
	NA_RxBufferCmd_O,              NTRX_WT_TYPE + NA_RxBufferCmd_LSB,              NA_RxBufferCmd_MSB,              // NA_RxBufferCmd
	//0x57
	NA_RxCryptSeqReset_O,          NTRX_WO_TYPE + NA_RxCryptSeqReset_LSB,          NA_RxCryptSeqReset_MSB,          // NA_RxCryptSeqReset
	//0x58
	NA_RxTransBytes_O,             NTRX_WO_TYPE + NA_RxTransBytes_LSB,             NA_RxTransBytes_MSB,             // NA_RxTransBytes
	//0x5a
	NA_RxTimeBCrc1Mode_O,          NTRX_WO_TYPE + NA_RxTimeBCrc1Mode_B,            NA_RxTimeBCrc1Mode_B,            // NA_RxTimeBCrc1Mode
	NA_RxCrc2Mode_O,               NTRX_WO_TYPE + NA_RxCrc2Mode_B,                 NA_RxCrc2Mode_B,                 // NA_RxCrc2Mode
	NA_RxArqMode_O,                NTRX_WO_TYPE + NA_RxArqMode_LSB,                NA_RxArqMode_MSB,                // NA_RxArqMode
	NA_RxAddrSegEsMode_O,          NTRX_WO_TYPE + NA_RxAddrSegEsMode_B,            NA_RxAddrSegEsMode_B,            // NA_RxAddrSegEsMode
	NA_RxAddrSegIsMode_O,          NTRX_WO_TYPE + NA_RxAddrSegIsMode_B,            NA_RxAddrSegIsMode_B,            // NA_RxAddrSegIsMode
	NA_RxAddrSegDevIdL_O,          NTRX_WO_TYPE + NA_RxAddrSegDevIdL_LSB,          NA_RxAddrSegDevIdL_MSB,          // NA_RxAddrSegDevIdL
	//0x5b
	NA_RxDataEn_O,                 NTRX_WO_TYPE + NA_RxDataEn_B,                   NA_RxDataEn_B,                   // NA_RxDataEn
	NA_RxBrdcastEn_O,              NTRX_WO_TYPE + NA_RxBrdcastEn_B,                NA_RxBrdcastEn_B,                // NA_RxBrdcastEn
	NA_RxTimeBEn_O,                NTRX_WO_TYPE + NA_RxTimeBEn_B,                  NA_RxTimeBEn_B,                  // NA_RxTimeBEn
	NA_RxAddrMode_O,               NTRX_WO_TYPE + NA_RxAddrMode_B,                 NA_RxAddrMode_B,                 // NA_RxAddrMode
	NA_RangingPulses_O,            NTRX_WO_TYPE + NA_RangingPulses_LSB,            NA_RangingPulses_MSB,            // NA_RangingPulses
	//0x5c
	NA_PulseDetDelay_O,            NTRX_WO_TYPE + NA_PulseDetDelay_LSB,            NA_PulseDetDelay_MSB,            // NA_PulseDetDelay
	//0x5d
	NA_GateAdjThreshold_O,         NTRX_WO_TYPE + NA_GateAdjThreshold_LSB,         NA_GateAdjThreshold_MSB,         // NA_GateAdjThreshold
	NA_DownPulseDetectDis_O,       NTRX_WO_TYPE + NA_DownPulseDetectDis_B,         NA_DownPulseDetectDis_B,         // NA_DownPulseDetectDis
	NA_UpPulseDetectDis_O,         NTRX_WO_TYPE + NA_UpPulseDetectDis_B,           NA_UpPulseDetectDis_B,           // NA_UpPulseDetectDis
	//0x5e
	NA_GateSizeUnsync_O,           NTRX_WO_TYPE + NA_GateSizeUnsync_LSB,           NA_GateSizeUnsync_MSB,           // NA_GateSizeUnsync
	NA_GateSizeBitsync_O,          NTRX_WO_TYPE + NA_GateSizeBitsync_LSB,          NA_GateSizeBitsync_MSB,          // NA_GateSizeBitsync
	NA_GateSizeFramesync_O,        NTRX_WO_TYPE + NA_GateSizeFramesync_LSB,        NA_GateSizeFramesync_MSB,        // NA_GateSizeFramesync
	NA_GateAdjBitsyncEn_O,         NTRX_WO_TYPE + NA_GateAdjBitsyncEn_B,           NA_GateAdjBitsyncEn_B,           // NA_GateAdjBitsyncEn
	NA_GateAdjFramesyncEn_O,       NTRX_WO_TYPE + NA_GateAdjFramesyncEn_B,         NA_GateAdjFramesyncEn_B,         // NA_GateAdjFramesyncEn

	// Not needed for 22Mhz default start
	//0x5f
	NA_Go2BitsyncThreshold_O,      NTRX_WO_TYPE + NA_Go2BitsyncThreshold_LSB,      NA_Go2BitsyncThreshold_MSB,      // NA_Go2BitsyncThreshold
	NA_LeaveBitsyncThreshold_O,    NTRX_WO_TYPE + NA_LeaveBitsyncThreshold_LSB,    NA_LeaveBitsyncThreshold_MSB,    // NA_LeaveBitsyncThreshold
	//0x60
	NA_RtcTimeBTxAdj_O,            NTRX_WO_TYPE + NA_RtcTimeBTxAdj_LSB,            NA_RtcTimeBTxAdj_MSB,            // NA_RtcTimeBTxAdj
	//0x61
	NA_RtcTimeBRxAdj_O,            NTRX_WO_TYPE + NA_RtcTimeBRxAdj_LSB,            NA_RtcTimeBRxAdj_MSB,            // NA_RtcTimeBRxAdj
	//0x62
	NA_RtcCmdWr_O,                 NTRX_WO_TYPE + NA_RtcCmdWr_B,                   NA_RtcCmdWr_B,                   // NA_RtcCmdWr
	NA_RtcCmdRd_O,                 NTRX_WO_TYPE + NA_RtcCmdRd_B,                   NA_RtcCmdRd_B,                   // NA_RtcCmdRd
	NA_RtcTimeBAutoMode_O,         NTRX_WO_TYPE + NA_RtcTimeBAutoMode_B,           NA_RtcTimeBAutoMode_B,           // NA_RtcTimeBAutoMode
	//0x63
	NA_AgcAmplitude_O,             NTRX_WO_TYPE + NA_AgcAmplitude_LSB,             NA_AgcAmplitude_MSB,             // NA_AgcAmplitude
	//0x64
	NA_AgcRangeOffset_O,           NTRX_WO_TYPE + NA_AgcRangeOffset_LSB,           NA_AgcRangeOffset_MSB,           // NA_AgcRangeOffset
	NA_UseAlternativeAgc_O,        NTRX_WO_TYPE + NA_UseAlternativeAgc_B,          NA_UseAlternativeAgc_B,          // NA_UseAlternativeAgc

	NA_RamTxLength_O,              NTRX_RW_TYPE + NA_RamTxLength_LSB,              NA_RamTxLength_MSB,              // NA_RamTxLength
	NA_RamStaAddr0_O,              NTRX_RW_TYPE + NA_RamStaAddr0_LSB,              NA_RamStaAddr0_MSB,              // NA_RamStaAddr0
	NA_RamStaAddr1_O,              NTRX_RW_TYPE + NA_RamStaAddr1_LSB,              NA_RamStaAddr1_MSB,              // NA_RamStaAddr1
} ;

//=============================================================================

Bool NtrxCheckTable( void )
{
	uint16 n ;
	if( NA_TABLE_END > 255 )	// NtrxProcessTable函数处理的数据表都是单字节类型的,所以个数不超过256
		return False ;
	n = sizeof( NtrxRegDesTable ) ;
	if( n == NA_REG_FIELD_COUNT * NTRX_FIELD_DES_SIZE )
		return True ;
	return False ;
}

//=============================================================================

void NtrxSetIndexReg( uint8 Index )
{
	if( Index != NtrxGetShadowReg( NA_RamIndex_O ) )
		NtrxWriteReg( NA_RamIndex_O, Index ) ;
}

void NtrxSetRamIndex( uint8 Page )
{
	NtrxSetIndexReg( ( NtrxGetShadowReg( NA_RamIndex_O ) & 0x30 ) | ( Page & 3 ) ) ;
}

void NtrxSetField( NtrxRegField Field, uint8 Value )
{
	uint8 Addr, Lsb, Msb, Type ;
	uint16 Index = (uint16)Field * NTRX_FIELD_DES_SIZE ;

	Addr = NtrxReadFlash( &NtrxRegDesTable[Index++] ) ;
	Lsb  = NtrxReadFlash( &NtrxRegDesTable[Index++] ) ;	  //所在bit位
	Msb  = NtrxReadFlash( &NtrxRegDesTable[Index] ) ;
	Type = Lsb & NTRX_TYPE_MASK ;
	Lsb &= ~NTRX_TYPE_MASK ;

	if( Type == NTRX_RO_TYPE )	// Read Only
		ErrorHandler( 0 ) ;		// Error : Try to write register of readonly

	if( Msb > 7 )
		Msb = 7 ;
	Msb = NtrxReadFlash( &BitMaskTable[Msb-Lsb] ) ;
	if( Msb == 1 )
	{
		if( Value != 0 )
			Value = 1 ;
	}
	else
		Value &= Msb ;
	Msb <<= Lsb ;
	Value <<= Lsb ;

	Value = ( NtrxGetShadowReg( Addr ) & (~Msb) ) | Value ;
	NtrxSetRamIndex( 0 ) ;
	NtrxWriteSingleSPI( Addr, Value ) ;	   //Value = 0x03: NA_TypeCodeBrdcast_VC_C
	if( Type != NTRX_WT_TYPE )
		NtrxSetShadowReg( Addr, Value ) ;
}

void NtrxSetLongField( NtrxRegField Field, NtrxBufferPtr Buffer )
{
	uint8 Addr, Msb, Type ;
	uint16 Index = (uint16)Field * NTRX_FIELD_DES_SIZE ;

	Addr = NtrxReadFlash( &NtrxRegDesTable[Index++] ) ;
	Type = NtrxReadFlash( &NtrxRegDesTable[Index++] ) ;
	Msb  = NtrxReadFlash( &NtrxRegDesTable[Index] ) ;

	if( ( Type & (~NTRX_TYPE_MASK) ) != 0 )
		ErrorHandler( 0 ) ;		// LSB must be zero
	if( Type == NTRX_RO_TYPE )
		ErrorHandler( 0 ) ;		// Error : Try to write register of readonly

	NtrxSetRamIndex( 0 ) ;
	NtrxWriteSPI( Addr, Buffer, Msb/8+1 ) ;

	// Copy Value To Shadow Regs
	if( Type != NTRX_WT_TYPE )
		while( 1 )
		{
			NtrxSetShadowReg( Addr, *Buffer ) ;
			if( Msb < 8 )
				return ;
			Msb -= 8 ;
			Addr ++ ;
			Buffer ++ ;
		}
}

uint8 NtrxGetField( NtrxRegField Field )
{
	uint8 Addr, Lsb, Msb, Value ;
	uint16 Index = (uint16)Field * NTRX_FIELD_DES_SIZE ;

	Addr = NtrxReadFlash( &NtrxRegDesTable[Index++] ) ;
	Lsb  = NtrxReadFlash( &NtrxRegDesTable[Index++] ) ;
	Msb  = NtrxReadFlash( &NtrxRegDesTable[Index] ) ;

	if( ( Lsb & NTRX_TYPE_MASK ) == NTRX_WT_TYPE )
		ErrorHandler( 0 ) ;		// Error : Try to read trigger register

	NtrxSetRamIndex( 0 ) ;
	if( ( Lsb & NTRX_TYPE_MASK ) == NTRX_WO_TYPE )
		Value = NtrxGetShadowReg( Addr ) ;
	else
		Value = NtrxReadSingleSPI( Addr ) ;

	Lsb &= ~NTRX_TYPE_MASK ;
	if( Msb > 7 )
		Msb = 7 ;
	Value >>= Lsb ;
	Value &= NtrxReadFlash( &BitMaskTable[Msb-Lsb] ) ;
	return Value ;
}

void NtrxGetLongField( NtrxRegField Field, NtrxBufferPtr Buffer )
{
	uint8 Addr, Msb, Type ;
	//uint16 Index = (uint16)Field * NTRX_FIELD_DES_SIZE ;
	uint16 Index ;
	Index = Field ;
	Index *= NTRX_FIELD_DES_SIZE ;

	Addr = NtrxReadFlash( &NtrxRegDesTable[Index++] ) ;
	Type = NtrxReadFlash( &NtrxRegDesTable[Index++] ) ;
	Msb  = NtrxReadFlash( &NtrxRegDesTable[Index] ) ;

	if( ( Type & (~NTRX_TYPE_MASK) ) != 0 )
		ErrorHandler( 0 ) ;		// LSB must be zero
	if( Type == NTRX_WT_TYPE )
		ErrorHandler( 0 ) ;		// Error : Try to read trigger register

	NtrxSetRamIndex( 0 ) ;
	if( Type == NTRX_WO_TYPE )
	{
		while( 1 )
		{
			*Buffer = NtrxGetShadowReg( Addr ) ;
			if( Msb < 8 )
				break ;
			Msb -= 8 ;
			Addr ++ ;
			Buffer ++ ;
		}
	}
	else
	{
		NtrxReadSPI( Addr, Buffer, Msb/8+1 ) ;
		Buffer += (Msb/8) ;
		Msb &= 7 ;
	}
	*Buffer &= NtrxReadFlash( &BitMaskTable[Msb] ) ;
}

void NtrxProcessTable( const NtrxFlashCode *TablePtr )
{
	NtrxRegField c ;
	uint8 v ;
	while( 1 )
	{
		c = (NtrxRegField)NtrxReadFlash( TablePtr++ ) ;
		v = NtrxReadFlash( TablePtr++ ) ;
		if( c == NA_TABLE_END )
			return ;
		if( c == NA_DELAY )
			Delay_ms( v ) ;
		else
			NtrxSetField( c, v ) ;
	}
}

//=============================================================================
