/**************************************************************************************************
  Filename:       KeyAudioISR.h
  Revised:        $Date: 2009/08/13 03:38:30 $
  Revision:       $Revision: 1.5 $
  Description:     This file defines the interrupt service routine on P0 port to handle the switch between
  lcd/key and audio.
**************************************************************************************************/
#ifndef KEYAUDIOISR_H
#define KEYAUDIOISR_H

#if 0
#include "hal_types.h"
#include "hal_defs.h"

#define WORK_STATUS_IDLE        (0x00)
#define WORK_STATUS_AUDIO       (0x01)
#define WORK_STATUS_LCDKEY      (0x02)

extern uint8 nWorkStatus;

#define ON_LCDKEY() (nWorkStatus == WORK_STATUS_LCDKEY)
#define ON_AUDIO() (nWorkStatus == WORK_STATUS_AUDIO)
#define IS_IDLE() (nWorkStatus == WORK_STATUS_IDLE)

#define SET_STATUS(WorkStatus) \
st ( \
	nWorkStatus = WorkStatus;\
)

#define SET_ON_LCDKEY() SET_STATUS(WORK_STATUS_LCDKEY)
#define SET_ON_AUDIO() SET_STATUS(WORK_STATUS_AUDIO)
#define SET_ON_IDLE() SET_STATUS(WORK_STATUS_IDLE)


#ifndef AUDIO_SERIAL
extern uint8 IsNumberOn;

#define SET_NUMBER_FLAG(NumberStatus) \
st ( \
	IsNumberOn = NumberStatus;\
)
#define SET_NUMBER_FLAG_ON() SET_NUMBER_FLAG(TRUE)
#define SET_NUMBER_FLAG_OFF() SET_NUMBER_FLAG(FALSE)

#define IS_NUMBER_PRESSED() (IsNumberOn == TRUE)
#endif
#endif

#endif

