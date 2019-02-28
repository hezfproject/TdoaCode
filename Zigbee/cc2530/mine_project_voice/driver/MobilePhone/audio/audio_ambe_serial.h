#ifndef __AUDIO_AMBE__
#define __AUDIO_AMBE__

#include "hal_types.h"
/*
// data type define
#ifndef uint8
#define uint8   unsigned char
#endif
#ifndef uint16
#define uint16  unsigned short
#endif
#ifndef uint32
#define uint32  unsigned int
#endif
#ifndef int8
#define int8    signed char
#endif
#ifndef int16
#define int16   short
#endif
#ifndef int32
#define int32   int
#endif
*/
//Sound defination
#define LowDo       0x10BC
#define LowRe       0x12C9
#define LowMi       0x1516
#define LowFa       0x1656
#define LowSo       0x1917
#define LowLa       0x1B75
#define LowSi       0x1F96
#define MidDo       0x2179
#define MidRe       0x259A
#define MidMi       0x2A2D
#define MidFa       0x2CB4
#define MidSo       0x322D
#define MidLa       0x3852
#define MidSi       0x3F3B
#define HiDo        0x42F2
#define HiRe        0x4B2B
#define HiMi        0x5462
#define HiFa        0x5960
#define HiSo        0x645A
#define HiLa        0x70A3
#define HiSi        0x7E66


#define Digit1_Note1    0x26B0
#define Digit1_Note2    0x164D
#define Digit2_Note1    0x2AC0
#define Digit2_Note2    0x164D
#define Digit3_Note1    0x2F43
#define Digit3_Note2    0x164D

#define Digit4_Note1    0x26B0
#define Digit4_Note2    0x18A3
#define Digit5_Note1    0x2AC0
#define Digit5_Note2    0x18A3
#define Digit6_Note1    0x2F43
#define Digit6_Note2    0x18A3

#define Digit7_Note1    0x26B0
#define Digit7_Note2    0x1B43
#define Digit8_Note1    0x2AC0
#define Digit8_Note2    0x1B43
#define Digit9_Note1    0x2F43
#define Digit9_Note2    0x1B43

#define DigitStar_Note1     0x26B0
#define DigitStar_Note2     0x1E1C
#define Digit0_Note1        0x2AC0
#define Digit0_Note2        0x1E1C
#define DigitPound_Note1    0x2F43
#define DigitPound_Note2    0x1E1C

#define Dial_Note1      0x0B33
#define Dial_Note2      0x0E14
#define Busy_Note1      0x0F5C
#define Busy_Note2      0x13D7
#define Ring_Note1      0x0E14
#define Ring_Note2      0x0F5C

// Audio control id define
#define AMBE_SET_BITRATE            (0)
#define AMBE_SET_OUTPUTGAIN         (1)
#define AMBE_SET_INPUTGAIN          (2)
#define AMBE_SET_SLEEPMODE          (3)
#define AMBE_SET_WAKEUP             (4)
#define AMBE_SET_SLIENCE            (5)

// AMBE data rate define
#define AMBE_BITRATE_2400           (0)
#define AMBE_BITRATE_2350_50        (1)
#define AMBE_BITRATE_3600           (2)
#define AMBE_BITRATE_3350_250       (3)
#define AMBE_BITRATE_3750_250       (4)
#define AMBE_BITRATE_4800           (5)
#define AMBE_BITRATE_4550_250       (6)
#define AMBE_BITRATE_3600_1200      (7)
#define AMBE_BITRATE_2550_2250      (8)
#define AMBE_BITRATE_4150_2250      (9)
#define AMBE_BITRATE_4400_2800      (10)
#define AMBE_BITRATE_7750_250       (11)
#define AMBE_BITRATE_4650_3350      (12)
#define AMBE_BITRATE_9600           (13)
#define AMBE_BITRATE_4850_4750      (14)
#define AMBE_BITRATE_MAX            (AMBE_BITRATE_4850_4750)


// AMBE data frame length define
#define AMBE_RAWDATA_LEN_2400       (6)
#define AMBE_RAWDATA_LEN_3600       (9)
#define AMBE_RAWDATA_LEN_4000       (10)
#define AMBE_RAWDATA_LEN_4800       (12)
#define AMBE_RAWDATA_LEN_6400       (16)
#define AMBE_RAWDATA_LEN_7200       (18)
#define AMBE_RAWDATA_LEN_8000       (20)
#define AMBE_RAWDATA_LEN_9600       (24)

#define INGAIN_PHONE2PHONE 	0x7f
#define INGAIN_PHONE2GATEWAY  0x00

#define DEFAULT_OUTGAIN_LEVEL 2

// Initial audio(AMBE)
void InitialAudio(void);
// Start Audio
void StartAudio(void);
// Stop Audio
void StopAudio(void);

// Read encoder data from audio(for AMBE, it's 34bytes per read)
// pBuf: pointer to read buf assigned from high level
// nMaxLen: maximal length for one read, i.e. the length of assigned read buf.(here it must be equal or larger than 34)
// return value: -1: fail, others: data length of this read
int8 ReadAudio(uint8 *hBuf, uint8 *pBuf, uint8 nMaxLen);

// Write decoder data into audio(fro AMBE, it must be 34bytes per write)
// pBuf: pointer to write buf containing data to be decoded by AMBE
// nLen: data length to be written
// retrun value: -1 fail, others: data length of this write
int8 WriteAudio(uint8 *pBuf, uint8 nLen);

// Control Audio(AMBE)
// nCtlID: control id
// pBuf: pointer to buf containing control data
// nLen: control data length
int ControlAudio(uint8 nCtlID, uint8* pBuf, uint8 nLen);

__near_func int8 WriteSilence(void);

// Play Perticular Sound
void PlayNote (uint16 Note1, uint16 Note2, uint16 Gain);

// Output Gain Control
void   AudioSetOutputGain(uint8 idx);
uint8  AudioGetOutputGain(void);

void AudioSetInputGain(uint8 InGain);
uint8 AudioGetInputGain(void);

void AudioIntoSleep(void);
bool AudioIsEnabled(void);
bool AudioIsPackEmpty(void);

#endif  // __AUDIO_AMBE__
