/**************************************************************************************************
Filename:       hal_audio.h
Revised:        $Date: 2011/07/19 18:51:08 $
Revision:       $Revision: 1.4 $
Description:     This file contains the interface to config audio for yiri mobile phone.
**************************************************************************************************/

#ifndef HAL_AUDIO_H
#define HAL_AUDIO_H

/*********************************************************************
* INCLUDES
*/
#include "audio_ambe_serial.h"
#include "app_protocol.h"
#include "OSAL.h"
#include "MobilePhone_cfg.h"

/*********************************************************************
* CONSTANTS
*/
/*********************************************************************
* MACROS
*/
#define AUDIO_SUCCESS  0
#define AUDIO_INVALID_DATA 1
#define AUDIO_EXCEED_THR 2
#define AUDIO_READ_ERR    3
#define AUDIO_WRITE_ERR  4

/*********************************************************************
* TYPEDEFS
*/

//typedef DQUE_t halAudioBufControl_t;

/**************************************************************************************************
*DEFINES
*/
typedef enum
{
	RING_ANY = 0,  /* open the bell from customer setting*/

	/* Bells */
	RING_BELL_1,
	RING_BELL_2,
	RING_BELL_3,
	RING_BELL_4,

	/* Inside Bell */
	RING_POWERON,
	RING_POWEROFF,

	/* Sounds */
	RING_RING,
	RING_BUSY,



	/* Pads */
	RING_DIAL,
	RING_DIGIT1,
	RING_DIGIT2,
	RING_DIGIT3,
	RING_DIGIT4,
	RING_DIGIT5,
	RING_DIGIT6,
	RING_DIGIT7,
	RING_DIGIT8,
	RING_DIGIT9,
	RING_DIGIT0,
	RING_DIGITSTAR,
	RING_DIGITPOUND
} RingName;

typedef enum
{
	VOICEBELL_NULL=0,

	VOICEBELL_OUTOFREACH,
	VOICEBELL_BUSY,
	VOICEBELL_NOBODY,
	VOICEBELL_HELP,
	VOICEBELL_RETREAT,
	VOICEBELL_SPEED,
	VOICEBELL_HELP_ACK,
	VOICEBELL_SLOW_DOWM,
	VOICEBELL_AVOID,
	VOICEBELL_GIVE_WAY,
} VoiceBellName_t ;



#define GAIN_32768 0x7000
#define GAIN_25000 0x6CE1
#define GAIN_20000 0x6A4D
#define GAIN_15000 0x66FB
#define GAIN_10000 0x624D
#define GAIN_5000  0x5A4D
#define GAIN_2500  0x524D
#define GAIN_1000  0x47BA
#define GAIN_500   0x3FBA
#define GAIN_0     0xD800
#define GAIN_MAX  GAIN_32768   //, e.g. 3.17dBm0
#define GAIN_MIN  GAIN_0  //, e.g. -117.56dBm0(silence

#define RING_GAIN_LEVEL_0   0
#define RING_GAIN_LEVEL_1   1
#define RING_GAIN_LEVEL_2   2
#define RING_GAIN_LEVEL_3   3
#define RING_GAIN_LEVEL_4   4

#define RING_TYPE_NONE      0
#define RING_TYPE_BELL      1
#define RING_TYPE_SMS       2
#define RING_TYPE_SOUND     3
#define RING_TYPE_PAD       4

#define     RING_SUCESS 0
#define     RING_ENDED  1
#define     RING_FAILED 2

#define OPENFLAG_ASONCE  0
#define OPENFLAG_ASBELL   1
#define OPENFLAG_ASSMS_POW    2
#define OPENFLAG_ASLISTEN 3

#define MAX_GAIN_LEVEL      5

#define     RING_MAX_BELLNAMELEN    16
#define     RING_BELL_NUM                   4

#define VOICEBELL_SEND_ING               0
#define VOICEBELL_SEND_COMPLETE     1

/* voice buf definations */
#define VOICE_READBUFSIZE  ((VOICE_IDX_THRESHOLD) * (VOICE_PER_RAW_DATA_LEN))
#define VOICE_WRITEBUFSIZE  ((VOICE_IDX_THRESHOLD )* (VOICE_PER_RAW_DATA_LEN))

/**************************************************************************************************
*FUNCTIONS
*/
extern char*  const   BellNameStr_list[RING_BELL_NUM];
extern uint8   SilenceFrame[VOICE_PER_RAW_DATA_LEN];
/**************************************************************************************************
*FUNCTIONS
*/

void HalAudioInit ( void );
__near_func void HalAudioPoll ( void );
uint8 FillAudioBuffer ( uint8* pbuf, uint16 len );
void FlushAudioBuffer(void);
__near_func uint8 ReadAudioData ( void );
__near_func uint8 WriteAudioData ( void );
uint8 RegisterForAudio ( uint8 task_id );
void HalVoiceBellOpen ( void );
void HalAudioOpen ( void );
void HalAudioClose ( void );
void HalRingOpen ( RingName name, uint8 flag );
uint8 HalRingPlay ( void );
bool HalRingIsPlaying ( void );
void HalRingClose ( void );
void HalShakePlay ( void );
void HalRingSetGain ( uint8 gain_type,  uint8 gain_level );
void HalRingGetGain ( uint8 gain_type,  uint8* pGain_level );
uint8 HalRingGetOpenFlag ( void );
uint8 HalRingGetPlayingName ( void );
//uint8 HalRingGetType(void);
void HalRingSetBellName ( RingName );
uint8 HalRingGetBellName ( void );
void  HalRingGetBellNameStr ( uint8 index, uint8* p );
void  HalRingSetSMSBell ( RingName name );
uint8  HalRingGetSMSBell ( void );
void  HalRingGetSMSNameStr ( uint8 index, uint8* p );
void  HalRingSetShakeCtl ( bool shake );
bool  HalRingGetShakeCtl ( void );
uint8 HalFillVoiceBellbuf ( void );
bool  HalSetVoiceBellBuf ( VoiceBellName_t  name );
/*bool HalCheckAudioOpen(void);
void HalSetReadAmbe(bool ReadInt);
void HalSetWriteAmbe(bool WriteInt);*/
#endif

