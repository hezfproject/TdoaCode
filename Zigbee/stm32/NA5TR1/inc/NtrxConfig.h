/******************************************************************************
 Copyright 2010, Walasey Technologies
 Author: WangKun
******************************************************************************/

#ifndef _NTRXCHIP_H_
#define _NTRXCHIP_H_

#ifdef __cplusplus
extern "C" {
#endif

//=============================================================================

// #define CONFIG_22MHZ_DEF_ONLY

#define RfTxDataOutputPower         0x3F
#define RfTxReqOutputPower          0x3F

// Select the time between to nanoNET TRX chip recalibrations in ms, zero is off
#define CONFIG_NTRX_RECAL_DELAY     60000

// The maximal count value for packet retransmissions
#define NA_MAX_ARQ_CNT              0x01

// Timeout for ranging in [ms]
#define RANGING_TIMEOUT             5000

#ifdef CONFIG_22MHZ_DEF_ONLY
#define CONFIG_NTRX_22MHZ_500NS
#define CONFIG_NTRX_22MHZ_1000NS
#define CONFIG_NTRX_22MHZ_2000NS
#define CONFIG_NTRX_22MHZ_4000NS
#define CONFIG_NTRX_22MHZ_8000NS
#define CONFIG_NTRX_22MHZ_16000NS
#define CONFIG_NTRX_22MHZ_HR_4000NS
#define CONFIG_NTRX_DEFAULT_MODE    NtrxMode22MHzHR4us250k
#else
#define CONFIG_NTRX_80MHZ_500NS
#define CONFIG_NTRX_80MHZ_1000NS
#define CONFIG_NTRX_80MHZ_2000NS
#define CONFIG_NTRX_80MHZ_4000NS
#define CONFIG_NTRX_DEFAULT_MODE    NtrxMode80MHz1us1M
#endif

//=============================================================================

#ifdef __cplusplus
}
#endif

#endif /* _NTRXCHIP_H_ */
