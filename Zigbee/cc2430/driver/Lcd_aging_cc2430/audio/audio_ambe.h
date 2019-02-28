#ifndef __AUDIO_AMBE__
#define __AUDIO_AMBE__

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


// Audio power on/off define
#define AUDIO_ON    (P0_4 = 1)
#define AUDIO_OFF   (P0_4 = 0)

// AMBE data frame length define
#define AMBE_RAWDATA_LEN_2400       (6)


// Initial audio(AMBE)
void InitialAudio(void);
// Start Audio
void StartAudio(void);

// Read encoder data from audio(for AMBE, it's 34bytes per read)
// pBuf: pointer to read buf assigned from high level
// nMaxLen: maximal length for one read, i.e. the length of assigned read buf.(here it must be equal or larger than 34)
// return value: -1: fail, others: data length of this read
__near_func int8 ReadAudio(uint8 *pBuf, uint8 nMaxLen);

// Write decoder data into audio(fro AMBE, it must be 34bytes per write)
// pBuf: pointer to write buf containing data to be decoded by AMBE
// nLen: data length to be written
// retrun value: -1 fail, others: data length of this write
__near_func int8 WriteAudio(uint8 *pBuf, uint8 nLen);

__near_func void StartWrite(void);

__near_func void StartRead(void);

__near_func int8 WriteSilence(void);

#endif  // __AUDIO_AMBE__
