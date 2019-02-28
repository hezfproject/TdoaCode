/***********************************************

    Filename:       hal_audio.c 
    Revised:        $Date: 2011/01/12 00:33:13 $
    Revision:       $Revision: 1.45 $

Description:    This file contains the audio definitions.
************************************************/
/*********************************************************************
For yiri RRN board, we have two different terminals: one is mobile phone , and
the other is used to orientation and so on. 
For the tiri mobile phone, we need add audio driver in it. And this file is defined 
related audio interface used in yiri mobile phone.
**********************************************************************/

/*********************************************************************
* INCLUDES
*/
#include "ZcomDef.h"
#include "AppProtocolWrapper.h"
#include "OSAL.h"
//#ifndef NEW_MENU_LIB
#include "MineApp_global.h"
//#endif 
#include "string.h"
#include "hal_audio.h"
#include "hal_mcu.h"
#include "Mineapp_MenuLibChinese.h"
#include "audio_interface.c"
/*********************************************************************
* CONSTANTS
*/
/*********************************************************************
* MACROS
*/
#define AUDIODATATHR   ((AUDIO_IDX_THRESHOLD) * (AUDIO_PER_RAW_DATA_LEN))
#define IDX_MAX    ((AUDIO_IDX_THRESHOLD)*1)
#define READBUFSIZE  ((IDX_MAX) * (AUDIO_PER_RAW_DATA_LEN))
#define WRITEBUFSIZE  ((IDX_MAX )* (AUDIO_PER_RAW_DATA_LEN)*2)

// Task ID not initialized
#define NO_TASK_ID 0xFF


#define BELL_TIMEOUT           (30000/40)        //    max bell ring timeout
#define BUSY_TIMEOUT           (3000/40)        //    max bell ring timeout

#define SHAKE_ONPERIOD			(4000/40)
#define SHAKE_SLIENCEPERIOD	(2000/40)

/*********************************************************************
* TYPEDEFS
*/
typedef struct
{
	uint16  Node1;
	uint16  Node2;    
	uint16  Beat;
}RingPace_t;

#ifdef PACKAGE_INFORMATION
typedef struct 
{
	uint16 frameSilenceCnt;
	uint16 frameCnt;
}frame_info_t;
#endif
/*********************************************************************
* LOCAL VARIABLES
*/
#ifdef PACKAGE_INFORMATION
static frame_info_t  frameInfo;
#endif
static byte registeredAudioTaskID = NO_TASK_ID;
static int16 ReadIdx = 0;
static uint8 RelayBuf[AUDIO_PER_RAW_DATA_LEN]; //relay audio data from queue to AMBE.

static uint8 ReadAudioBuf[READBUFSIZE];//hold the audio data from AMBE.
static uint8 WriteAudioBuf[WRITEBUFSIZE]; //hold the audio data to rf.

static halAudioBufControl_t WriteBufCtrl;
static halAudioBufControl_t ReadBufCtrl;

static bool IsWriteEmpty = false; //trigger ambe to write interrupt.
static bool IsReadFull = false; //trigger ambe to read interrupt.
/*
static bool WriteAmbe = false;
static bool ReadAmbe = false;
*/
//Sound Stream Defination
static  RingPace_t const __code* RingHeader = NULL;
static uint16         RingLen = 0;
static uint8          RingToneType;

static uint8         Saved_Bell_Name = RING_BELL_1;
static uint8         Saved_SMS_Name = RING_BELL_4;
static bool          Saved_shakeen = FALSE;
static uint8         Ring_openflag;
static bool          Ring_playing = FALSE;

static uint8         Ring_BellGainIdx = MAX_GAIN_LEVEL-1;
static uint8         Ring_SoundGainIdx = MAX_GAIN_LEVEL-1;
static uint8         Ring_PadGainIdx = MAX_GAIN_LEVEL-1;
static uint16         Ring_PlayPaceCnt = 0;
static uint16         Ring_PlayBeatCnt = 0;
static uint16         Ring_PlayTotalCnt = 0;
static uint16 	     Ring_PlayShakeCnt = 0;

static  __code const RingPace_t Bell_1[] = {
	{0, 0, 24}, {MidLa, 0, 6}, {HiDo, 0, 3}, {MidLa, 0, 3}, {MidMi, 0, 6}, {MidLa, 0, 3},
	{MidMi, 0, 3}, {MidDo, 0, 6}, {MidMi, 0, 3}, {MidDo, 0, 3}, {LowLa, 0, 12}, {LowSi, 0, 3},
	{LowLa, 0, 3}, {LowSi, 0, 3}, {LowLa, 0, 3}, {LowSi, 0, 3}, {LowLa, 0, 3}, {LowSi, 0, 3},
	{LowLa, 0, 3}, {LowSi, 0, 6}, {MidRe, 0, 3}, {LowSi, 0, 3}, {MidDo, 0, 12}
};

static  __code const RingPace_t Bell_2[] = {
	{0, 0, 12}, {HiSo, HiSi, 1}, {HiSi, HiSi, 1}, {HiSo, HiSi, 1}, {HiSi, HiSi, 1}, 
	{HiSo, HiSi, 1}, {HiSi, HiSi, 1}, {HiSo, HiSi, 1}, {HiSi, HiSi, 1}, 
	{HiSo, HiSi, 1}, {HiSi, HiSi, 1}, {HiSo, HiSi, 1}, {HiSi, HiSi, 1}, {0, 0, 24}
};

static  __code const RingPace_t Bell_3[] = {
	{0, 0, 24}, 
	{HiDo, HiFa, 1}, {HiFa, HiFa, 1}, {HiDo, HiFa, 1}, {HiFa, HiFa, 1}, 
	{HiDo, HiFa, 1}, {HiFa, HiFa, 1}, {HiDo, HiFa, 1}, {HiFa, HiFa, 1}, 
	{HiDo, HiFa, 1}, {HiFa, HiFa, 1}, {HiDo, HiFa, 1}, {HiFa, HiFa, 1} 
	/*
	{0 , 0, 24}, {HiDo, HiFa, 1}, {HiFa, HiFa, 1}, {HiDo, HiFa, 1}, {HiFa, HiFa, 1}, 
	{HiDo, HiFa, 1}, {HiFa, HiFa, 1}, {HiDo, HiFa, 1}, {HiFa, HiFa, 1}, 
	{HiDo, HiFa, 1}, {HiFa, HiFa, 1}, {HiDo, HiFa, 1}, {HiFa, HiFa, 1}
	*/
};

static  __code const RingPace_t  Bell_4[] = {
	{0, 0, 24}, {HiDo, 0, 6}, {MidFa, 0, 6}, {MidLa, 0, 6}, {HiDo, 0, 6}
};

static __code const RingPace_t  PowerOn[] =  {{0, 0, 12}, {MidDo, 0, 3}, {MidMi, 0, 3}, {MidSo, 0, 3}, {HiDo, 0, 6}};

static __code const RingPace_t  PowerOff[] =  {{0, 0, 12}, {HiDo, 0, 3}, {MidSo, 0, 3}, {MidMi, 0, 3}, {MidDo, 0, 6}};

static __code const RingPace_t Ring[] = {{Ring_Note1, Ring_Note2, 24}, {0, 0, 24}};

static __code const RingPace_t  Busy[] = {{Digit0_Note2, Busy_Note2, 9}, {0, 0, 9}};	

static __code const RingPace_t  Dial[] = {{Dial_Note1, Dial_Note2, 1}};

/*
static __code const RingPace_t  Digit1[] = {{Digit1_Note1, Digit1_Note2, 3}};

static __code const RingPace_t  Digit2[] = {{Digit2_Note1, Digit2_Note2, 3}};

static __code const RingPace_t  Digit3[] = {{Digit3_Note1, Digit3_Note2, 3}};

static __code const RingPace_t  Digit4[] = {{Digit4_Note1, Digit4_Note2, 3}};

static __code const RingPace_t  Digit5[] = {{Digit5_Note1, Digit5_Note2, 3}};

static __code const RingPace_t  Digit6[] = {{Digit6_Note1, Digit6_Note2, 3}};

static __code const RingPace_t  Digit7[] = {{Digit7_Note1, Digit7_Note2, 3}};

static __code const RingPace_t  Digit8[] = {{Digit8_Note1, Digit8_Note2, 3}};

static __code const RingPace_t  Digit9[] = {{Digit9_Note1, Digit9_Note2, 3}};

static __code const RingPace_t  Digit0[] = {{Digit0_Note1, Digit0_Note2, 3}};

static __code const RingPace_t  DigitStar[] = {{DigitStar_Note1, DigitStar_Note2, 3}};

static __code const RingPace_t  DigitPound[] = {{DigitPound_Note1, DigitPound_Note2, 3}};
*/

static __code const  uint16 BellGainList[MAX_GAIN_LEVEL] = {
	GAIN_MIN,        
	GAIN_10000,
	GAIN_15000,
	GAIN_25000,
	GAIN_MAX       
};
static __code const uint16 SoundGainList[MAX_GAIN_LEVEL] = {
	GAIN_MIN,      
	GAIN_2500,         
	GAIN_5000,         
	GAIN_10000,         
	GAIN_15000         
};
/*
static __code const uint16 PadGainList[MAX_GAIN_LEVEL] = {
	GAIN_MIN,
	GAIN_20000,
	GAIN_20000,
	GAIN_20000,
	GAIN_20000,
};*/

static const RingPace_t __code* __code const Header_List[]=
{
	NULL,
	Bell_1,
	Bell_2,
	Bell_3,
	Bell_4,
	PowerOn,
	PowerOff,

	Ring,
	Busy,

	Dial,
	/*
	Digit1,
	Digit2,
	Digit3,
	Digit4,
	Digit5,
	Digit6,
	Digit7,
	Digit8,
	Digit9,
	Digit0,
	DigitStar,
	DigitPound,
	*/
};

static __code const uint16 Length_list[]=
{
	0,
	sizeof(Bell_1)/sizeof(Bell_1[0]),
	sizeof(Bell_2)/sizeof(Bell_2[0]),
	sizeof(Bell_3)/sizeof(Bell_3[0]),
	sizeof(Bell_4)/sizeof(Bell_4[0]),

	sizeof(PowerOn)/sizeof(PowerOn[0]),
	sizeof(PowerOff)/sizeof(PowerOff[0]),
	
	sizeof(Ring)/sizeof(Ring[0]),
	sizeof(Busy)/sizeof(Busy[0]),

	sizeof(Dial)/sizeof(Dial[0]),
	/*
	sizeof(Digit1)/sizeof(Digit1[0]),
	sizeof(Digit2)/sizeof(Digit2[0]),
	sizeof(Digit3)/sizeof(Digit3[0]),
	sizeof(Digit4)/sizeof(Digit4[0]),
	sizeof(Digit5)/sizeof(Digit5[0]),
	sizeof(Digit6)/sizeof(Digit6[0]),
	sizeof(Digit7)/sizeof(Digit7[0]),
	sizeof(Digit8)/sizeof(Digit8[0]),
	sizeof(Digit9)/sizeof(Digit9[0]),
	sizeof(Digit0)/sizeof(Digit0[0]),
	sizeof(DigitStar)/sizeof(DigitStar[0]),
	sizeof(DigitPound)/sizeof(DigitPound[0]),
	*/
};


/*********************************************************************
* GLOBAL VARIABLES
*/
char*  const __code  BellNameStr_list[RING_BELL_NUM]=
{
	RING1_CHINA,
	RING2_CHINA,
	RING3_CHINA,
	RING4_CHINA
};
/*********************************************************************
* LOCAL FUNCTIONS
*/
static void Audio_SendEvt( void* msg);
static void HalResetRingParam(void);
//static bool HalCheckAudioOpen(void);

void HalAudioStart(void)
{
	//to ensure the audio buffer , using global vairiables instead of dynamic memory.
	//uint8* ReadAudioBuf = osal_mem_alloc(AUDIOBUFSIZE);
	//uint8* WriteAudioBuf = osal_mem_alloc(AUDIOBUFSIZE);
	//if (ReadAudioBuf && WriteAudioBuf)
	{
		Dque_Init(&ReadBufCtrl, READBUFSIZE, ReadAudioBuf, AUDIO_PER_RAW_DATA_LEN);
		Dque_Init(&WriteBufCtrl, WRITEBUFSIZE, WriteAudioBuf, AUDIO_PER_RAW_DATA_LEN);
	}
	//else
	{
		//need reset the machine.
	}
#ifdef AUDIO_SERIAL
	PICTL &= ~0x02;		// rising edge trigger for P1
	IEN2 |= 0X10;   		// enable P1 interrupt   
#endif
}

/*********************************************************************
* @fn      FillAudioBuffer
*
* @brief   Write audio buffer when app recv a piece of audio data.
*
* @param  len - the length of the audio data from rf.
*
* @return  status

*********************************************************************/
uint8 FillAudioBuffer(uint8* pbuf, int16 len)
{
	halIntState_t   intState;
	HAL_ENTER_CRITICAL_SECTION(intState);
	if (DQUE_SUCCESS == Dque_AddDataToTail(&WriteBufCtrl, pbuf, len))
	{
		if (IsWriteEmpty)
		{
			WriteAudioData();
			IsWriteEmpty = false;
		}
		HAL_EXIT_CRITICAL_SECTION(intState);
		return AUDIO_SUCCESS;
	}
	HAL_EXIT_CRITICAL_SECTION(intState);
	return AUDIO_WRITE_ERR;
}

/*********************************************************************
* @fn      ReadAudioData
*
* @brief   Read Audio Data from AMBE to buf.
*
* @param  None.
*
* @return  status

*********************************************************************/
__near_func uint8 ReadAudioData(void)
{
	halIntState_t   intState;
	HAL_ENTER_CRITICAL_SECTION(intState);
	if (Dque_IsFull(&ReadBufCtrl))
	{
		IsReadFull = true;
		HAL_EXIT_CRITICAL_SECTION(intState);
		return AUDIO_EXCEED_THR;
	}

	ReadAudio(RelayBuf, AUDIO_PER_RAW_DATA_LEN);
	uint8 rv = Dque_AddItemToTail(&ReadBufCtrl, RelayBuf);
	if (rv == DQUE_SUCCESS)
	{
		ReadIdx++;
	}

	HAL_EXIT_CRITICAL_SECTION(intState);
	return AUDIO_READ_ERR;
}

/*********************************************************************
* @fn      WriteAudioData
*
* @brief   Write Audio Data from buf to AMBE.
*
* @param  None.
*
* @return  status

*********************************************************************/
__near_func uint8 WriteAudioData(void)
{
	halIntState_t   intState;
	HAL_ENTER_CRITICAL_SECTION(intState);
	if (Dque_IsEmpty(&WriteBufCtrl))
	{
		IsWriteEmpty = true;
#ifdef PACKAGE_INFORMATION
		frameInfo.frameSilenceCnt++;
#endif
		WriteSilence(); //Using silence frame to fill the blank.
		HAL_EXIT_CRITICAL_SECTION(intState);
		return AUDIO_INVALID_DATA;
	}
	uint8 rv = Dque_GetItemFromHead(&WriteBufCtrl, RelayBuf);
	if (rv == DQUE_SUCCESS)
	{
#ifdef PACKAGE_INFORMATION
		frameInfo.frameCnt++;
#endif
		WriteAudio(RelayBuf, AUDIO_PER_RAW_DATA_LEN);
	}

	HAL_EXIT_CRITICAL_SECTION(intState);
	return AUDIO_WRITE_ERR;
}

/******************************************************************************
* @fn      HalAudioPoll
*
* @brief   Poll the Audio Data in readbuf. Because every 20 ms an Audio interrupt occurs and 6 byte audio
*             data is producted. So we need cache the data to the readaudiobuf to until the threshold,
*             and build a system msg to notify OS to prepare to send out the audio data from rf. 
*
* @param   none
*
* @return  none
*****************************************************************************/
static uint16 blk;
//uint16 evenn = 0;
__near_func void HalAudioPoll( void )
{
	halIntState_t   intState;
	HAL_ENTER_CRITICAL_SECTION(intState);
	if ((ReadIdx >= AUDIO_IDX_THRESHOLD && !Dque_IsEmpty(&ReadBufCtrl))
		|| Dque_IsFull(&ReadBufCtrl))
	{
#ifndef SINGLE_AUDIO_TEST
		int16 datasize = Dque_GetDataSize(&ReadBufCtrl);
		byte* audiomsg = osal_msg_allocate(sizeof(spi_event_hdr_t) +sizeof(app_Voice_t)+ datasize);
		if (audiomsg)
		{
			AppPacket_t * pAudio = (AppPacket_t *)audiomsg;
			pAudio->hdr.event = APP_AUDIO;
			pAudio->data.app_Voice.msgtype = VOICE;
			pAudio->data.app_Voice.len = datasize;
			pAudio->data.app_Voice.blk = blk++;

			//skip spi_event_hdr_t + sizeof(app_Voice_t).
			uint8* voicedata = (byte *)(audiomsg + sizeof(spi_event_hdr_t) + sizeof(app_Voice_t));
			Dque_GetDataFromHead(&ReadBufCtrl, voicedata, datasize);
			Audio_SendEvt(audiomsg);

			ReadIdx = 0;
			if (IsReadFull)
			{
				ReadAudioData();
				IsReadFull = false;
			}
		}
#else//test audio on one node.
		int16 datasize = Dque_GetDataSize(&ReadBufCtrl);
		uint8* relaydata = osal_mem_alloc(datasize);
		if (relaydata)
		{
			//evenn++;
			Dque_GetDataFromHead(&ReadBufCtrl, relaydata, datasize);
			//if (evenn%2)
			FillAudioBuffer(relaydata, datasize);
			osal_mem_free(relaydata);
		}

		ReadIdx = 0;
		if (IsReadFull)
		{
			ReadAudioData();
			IsReadFull = false;
		}
#endif

	}
	HAL_EXIT_CRITICAL_SECTION(intState);
}

/*********************************************************************
* @fn      Audio_SendEvt
*
* @brief   Send "Audio" message to application.
*
* @param  msg - msg to send.
*
* @return  status
*********************************************************************/
void Audio_SendEvt( void* msg)
{
	if (registeredAudioTaskID != NO_TASK_ID)
		osal_msg_send(registeredAudioTaskID, (byte *)msg);	
}

/*********************************************************************
* @fn      RegisterForAudio
*
* @brief    The Audio handler is setup to send all audio events to
* 		   one task (if a task is registered)..
*
* @param  task_id  - the candidate task.
*
* @return  status
*********************************************************************/
byte RegisterForAudio( byte task_id )
{
	// Allow only the first task
	if ( registeredAudioTaskID == NO_TASK_ID )
	{
		registeredAudioTaskID = task_id;
		return true;
	}
	else
		return false;
}

/*********************************************************************
* @fn      HalRingOpen
*
* @brief   Open the Ring and play ring prepared.
*
* @param  Ringname, flag.
*
* @return  None.
*********************************************************************/
void HalRingOpen(RingName name, uint8 flag)
{
 	StartAudio();
      Ring_playing = TRUE;
	Ring_openflag = flag;
	if(name==RING_ANY)
	{
		if(flag == OPENFLAG_ASBELL)
		{
			RingHeader = Header_List[Saved_Bell_Name];
			RingLen = Length_list[Saved_Bell_Name];
		}
		else if(flag == OPENFLAG_ASSMS)
		{
			RingHeader = Header_List[Saved_SMS_Name];
			RingLen = Length_list[Saved_SMS_Name];
		}
	}   
	else
	{   
		RingHeader = Header_List[name];
		RingLen = Length_list[name];
	}
	if ( name <= RING_POWEROFF)
	{
		RingToneType = RING_TYPE_BELL;
#ifndef NEW_MENU_LIB
		SET_ON_CALLED();
#endif
	}
	else if (name >= RING_RING && name <= RING_BUSY)
	{
		RingToneType = RING_TYPE_SOUND;
#ifndef NEW_MENU_LIB
		SET_ON_CALLING();
#endif
	}
	else
		RingToneType = RING_TYPE_NONE;

	Ring_PlayPaceCnt = 0;
	Ring_PlayBeatCnt = 0;
	Ring_PlayTotalCnt = 0;
	Ring_PlayShakeCnt = 0;

	return;
}
/*********************************************************************
* @fn      HalRingSetGain
*
* @brief  Set Gain of Bell,Sound and pad
*
* @param  gain_type, gain_level.
*
* @return  none
*********************************************************************/
void HalRingSetGain(uint8 gain_type,  uint8 gain_level)
{
	if(gain_level < MAX_GAIN_LEVEL)
	{
		switch(gain_type)
		{
		case RING_TYPE_BELL:
			Ring_BellGainIdx = gain_level;
			break;
		case RING_TYPE_SOUND:
			Ring_SoundGainIdx = gain_level;
			break;
		case RING_TYPE_PAD:
			Ring_PadGainIdx = gain_level;
			break;            
		}
	}
	return;
}
/*********************************************************************
* @fn      HalRingGetGain
*
* @brief   Get Gain of Bell,Sound and pad
*
* @param   gain_type, point of gain_level.
*
* @return none
*********************************************************************/

void HalRingGetGain(uint8 gain_type,  uint8* pGain_level)
{
	switch(gain_type)
	{
	case RING_TYPE_BELL:
		*pGain_level = Ring_BellGainIdx;
		break;
	case RING_TYPE_SOUND:
		*pGain_level = Ring_SoundGainIdx;
		break;
	case RING_TYPE_PAD:
		*pGain_level = Ring_PadGainIdx;
		break;            
	}
	return;
}
/*********************************************************************
* @fn      HalRingGetType
*
* @brief   Get Tone Type,should be 
*
* @param   none
*
* @return   RING_TYPE_NONE,RING_TYPE_BELL,RING_TYPE_SOUND,RING_TYPE_PAD.
*********************************************************************/

//uint8 HalRingGetType(void)
//{
//	return RingToneType;
//}

uint8 HalRingGetOpenFlag(void)
{
    return Ring_openflag;
}

void  HalRingSetBellName(RingName name)
{   
	if (name >= RING_BELL_1 && name <= RING_BELL_4)
	{
		Saved_Bell_Name = name;
	}
}

uint8  HalRingGetBellName(void)
{
	return Saved_Bell_Name;
}
void  HalRingSetSMSBell(RingName name)
{   
	if (name >= RING_BELL_1 && name <= RING_BELL_4)
	{
		Saved_SMS_Name= name;
	}
}
uint8  HalRingGetSMSBell(void)
{
	return Saved_SMS_Name;
}

void  HalRingGetBellNameStr(uint8 index, uint8* p)
{
	strcpy((char *)p, (char *)BellNameStr_list[index]);
	return;
}

void  HalRingGetSMSNameStr(uint8 index, uint8* p)
{
	strcpy((char *)p, (char *)BellNameStr_list[index]);
	return;
}

void  HalRingSetShakeCtl(bool shakectl)
{
	Saved_shakeen = shakectl;
	if(!shakectl)
	{
		HALRING_SHAKEOFF();
	}
}

bool  HalRingGetShakeCtl(void)
{
	return Saved_shakeen;
}

/*********************************************************************
* @fn      HalRingPlay
*
* @brief   Play a pace of Ring.
*
* @param  None.
*
* @return  RING_SUCESS,RING_ENDED,RING_FAILED.
*********************************************************************/

uint8 HalRingPlay()
{  
	if(RingHeader!=NULL && ((Ring_PlayTotalCnt==0) ||(DPE==1)))
	{
		//P1IFG&= ~0x02;
		//while (DPE == 0);

		if(Ring_PlayPaceCnt < RingLen)
		{
			RingPace_t pace = RingHeader[Ring_PlayPaceCnt];

			uint16 gain;

			if(RingToneType == RING_TYPE_BELL)
			{
				gain = BellGainList[Ring_BellGainIdx];
			}
			else if(RingToneType == RING_TYPE_SOUND)
			{
				gain = SoundGainList[Ring_SoundGainIdx];
			}

			PlayNote(pace.Node1, pace.Node2,gain);

			/* situations that play until timeout */
			if(     (RingToneType==RING_TYPE_BELL && Ring_openflag==OPENFLAG_ASBELL && Ring_PlayTotalCnt > BELL_TIMEOUT) //called side
				|| (RingHeader == Ring&&  Ring_PlayTotalCnt > BELL_TIMEOUT) 
				|| (RingHeader == Busy&&  Ring_PlayTotalCnt > BUSY_TIMEOUT)
				//|| (Ring_Name== RING_RING&&  Ring_PlayTotalCnt > BELL_TIMEOUT) //calling side normal voice
				// || (Ring_Name == RING_BUSY&&  Ring_PlayTotalCnt > BUSY_TIMEOUT) //calling side busy voice.
				)
			{
				//HalRingClose();
				HalResetRingParam();
				return RING_ENDED;
			}

			Ring_PlayTotalCnt++;            
			if(Ring_PlayBeatCnt < pace.Beat-1)
			{  
				Ring_PlayBeatCnt++;
			}
			else
			{
				Ring_PlayPaceCnt++;
				Ring_PlayBeatCnt = 0;
			}
			return RING_SUCESS;
		}
		else
		{
			Ring_PlayPaceCnt = 0;
			Ring_PlayBeatCnt = 0;

			/*situations that play once */
			if(RingToneType==RING_TYPE_PAD ||
				(RingToneType==RING_TYPE_BELL && Ring_openflag==OPENFLAG_ASSMS)
				)
			{
				HalResetRingParam();
				return RING_ENDED;
			}
		}
	}
	return RING_FAILED;
}

bool HalRingIsPlaying(void)
{
    return Ring_playing;
}
void HalShakePlay(void)
{
	if(Saved_shakeen)
	{
		uint16 cnt = (Ring_PlayShakeCnt++) % (SHAKE_ONPERIOD + SHAKE_SLIENCEPERIOD);
		if(cnt < SHAKE_ONPERIOD)
		{
			HALRING_SHAKEON();
		}
		else
		{
			HALRING_SHAKEOFF();
		}
	}
}
 /*********************************************************************
 * @fn      HalRingClose
 *
 * @brief   Close Playing Ring.
 *
 * @param  None.
 *
 * @return  None.
 *********************************************************************/

void HalRingClose(void)
{
	StopAudio();
	HalResetRingParam();
	HALRING_SHAKEOFF();
}

 void HalResetRingParam(void)
 {
        Ring_playing = FALSE;
 	  RingHeader = NULL;
        RingLen = 0;
        RingToneType = RING_TYPE_NONE;
        Ring_PlayPaceCnt = 0;
        Ring_PlayBeatCnt = 0;
        Ring_PlayTotalCnt = 0;	
	 Ring_PlayShakeCnt = 0;
 }
 
 /*********************************************************************
 * @fn      HalAudioOpen
 *
 * @brief   Open the Audio setting and do some prepare work.
 *
 * @param  None.
 *
 * @return  None.
 *********************************************************************/
void HalAudioOpen(void)
{
#ifdef AUDIO_SERIAL
	//set MP status state-machine.

	SET_ON_AUDIO(); //FIXME: No way to driver state-machine for called party.

	if (EN_AUD == 0)
		StartAudio();	
		
	P1IEN |= 0x03;//P1.0 P1.1 interrupt enable
#else
	SET_ON_AUDIO();
	AUDIO_ON;
	StartAudio();	
	P0IFG = 0;
	P0IF = 0;
#endif
#ifdef PACKAGE_INFORMATION
	frameInfo.frameSilenceCnt = 0;
	frameInfo.frameCnt = 0;
#endif
}

/*********************************************************************
* @fn      HalAudioClose
*
* @brief   Close the audio setting and do some clear work.
*
* @param  None.
*
* @return  None.
*********************************************************************/
void HalAudioClose(void)
{
#ifdef AUDIO_SERIAL
	#ifndef NEW_MENU_LIB
	SET_ON_IDLE();
	#endif

	StopAudio();
	blk = 0;
	//WriteAmbe = false;
	//ReadAmbe = false;
#else
	SET_NUMBER_FLAG_OFF();
	SET_ON_LCDKEY();
	AUDIO_OFF;
	P0IFG = 0;
	P0IF = 0;
#endif
}

#ifdef PACKAGE_INFORMATION
void HalAudioGetFrameInfo(uint16* pFrameCnt, uint16* pFrameSilenceCnt)
{
	*pFrameCnt = frameInfo.frameCnt;
	*pFrameSilenceCnt = frameInfo.frameSilenceCnt;
	return;
}
#endif
/*
bool HalCheckAudioOpen(void)
{
	return (EN_AUD == 1) && (WriteAmbe == true) && (ReadAmbe == true);
}

void HalSetWriteAmbe(bool WriteInt)
{
	WriteAmbe = WriteInt;
}

void HalSetReadAmbe(bool ReadInt)
{
	ReadAmbe = ReadInt;
}
*/
