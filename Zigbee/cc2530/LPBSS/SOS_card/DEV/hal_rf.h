/*******************************************************************************
  Filename:     hal_rf.h
  Revised:      $Date: 18:21 2012Äê5ÔÂ7ÈÕ
  Revision:     $Revision: 1.0 $
  
  Description:  HAL radio interface header file

*******************************************************************************/
#ifndef _HAL_RF_H
#define _HAL_RF_H

/*******************************************************************************
* INCLUDES
*/
#include <types.h>

/*******************************************************************************
* TYPEDEFS
*/
// CC2590/91 gain modes
typedef enum
{
    HAL_RF_GAIN_LOW     = 0,
    HAL_RF_GAIN_HIGH    = 1
}HAL_RF_GAIN_MODE_E;

// TX power lookup index
typedef enum
{
    HAL_RF_TXPOWER_4P5_DBM  =    0,
    HAL_RF_TXPOWER_2P5_DBM  =    1,
    HAL_RF_TXPOWER_1_DBM    =    2,
    HAL_RF_TXPOWER_M0P5_DBM =    3,
    HAL_RF_TXPOWER_M1P5_DBM =    4,
    HAL_RF_TXPOWER_M3_DBM   =    5,
    HAL_RF_TXPOWER_M6_DBM   =    6,
    HAL_RF_TXPOWER_M18_DBM  =    7
}HAL_RF_TXPOWER_INDEX_E;

typedef struct
{
    HAL_RF_GAIN_MODE_E      emGainMode;
    HAL_RF_TXPOWER_INDEX_E  emTxPowIndex;
}HAL_RF_DEV_T;

/*******************************************************************************
* CONSTANTS AND DEFINES
*/

// Chip ID's
#define HAL_RF_CHIP_ID_CC1100               0x00
#define HAL_RF_CHIP_ID_CC1110               0x01
#define HAL_RF_CHIP_ID_CC1111               0x11
#define HAL_RF_CHIP_ID_CC2420               0x02
#define HAL_RF_CHIP_ID_CC2500               0x80
#define HAL_RF_CHIP_ID_CC2510               0x81
#define HAL_RF_CHIP_ID_CC2511               0x91
#define HAL_RF_CHIP_ID_CC2550               0x82
#define HAL_RF_CHIP_ID_CC2520               0x84
#define HAL_RF_CHIP_ID_CC2430               0x85
#define HAL_RF_CHIP_ID_CC2431               0x89
#define HAL_RF_CHIP_ID_CC2530               0xA5
#define HAL_RF_CHIP_ID_CC2531               0xB5
#define HAL_RF_CHIP_ID_CC2540               0x8D

// IEEE 802.15.4 defined constants (2.4 GHz logical channels)
#define MIN_CHANNEL                         11    // 2405 MHz
#define MAX_CHANNEL                         26    // 2480 MHz
#define CHANNEL_SPACING                     5     // MHz


/*******************************************************************************
* GLOBAL FUNCTIONS
*/

// Generic RF interface
UINT8 HAL_RF_Init(HAL_RF_DEV_T*);

UINT8 HAL_RF_SetTxPower(HAL_RF_TXPOWER_INDEX_E emIndex);

UINT8 HAL_RF_Transmit(VOID);

// With CC2590/91 only
VOID  HAL_RF_SetGain(HAL_RF_GAIN_MODE_E emGainMode);

UINT8 HAL_RF_GetChipId(VOID);

UINT8 HAL_RF_GetChipVer(VOID);

UINT8 HAL_RF_GetRandomByte(VOID);

UINT8 HAL_RF_GetRssiOffset(VOID);

// data buffer functions
VOID  HAL_RF_AppendTxBuf(UINT8* pu8Data, UINT8 u8Length);

VOID  HAL_RF_WriteTxBuf(UINT8* pu8Data, UINT8 u8Length);

VOID  HAL_RF_ReadRxBuf(UINT8* pu8Data, UINT8 u8Length);

VOID  HAL_RF_WaitTransceiverReady(VOID);

UINT8 HAL_RF_ReadMemory(UINT16 u16Addr, UINT8* pu8Data, UINT8 u8Length);

UINT8 HAL_RF_WriteMemory(UINT16 u16Addr, UINT8* pu8Data, UINT8 u8Length);

// rf running config functions
VOID  HAL_RF_ReceiveOn(VOID);

VOID  HAL_RF_ReceiveOff(VOID);

VOID  HAL_RF_DisableRxInterrupt(VOID);

VOID  HAL_RF_EnableRxInterrupt(VOID);

VOID  HAL_RF_RxInterruptConfig(ISR_PFN pfnISR);

// IEEE 802.15.4 specific interface
VOID  HAL_RF_SetChannel(UINT8 u8Channel);

VOID  HAL_RF_SetShortAddr(UINT16 u16ShortAddr);

VOID  HAL_RF_SetPanId(UINT16 u16PanId);

#endif
