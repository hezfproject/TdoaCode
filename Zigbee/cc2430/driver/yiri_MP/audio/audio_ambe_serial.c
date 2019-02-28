#include <iocc2430.h>
#include "audio_ambe_serial.h"
#include "../comm/delay.h"
#include "codec_fm11ge300.h"


/*******************************************************************************
// Local macro define
*******************************************************************************/
// ambe control signal define
// Audio interface define
#define EPR             P1_0
#define DPE             P1_1
#define CHS_DI			P0_4
#define CHS_I_CLK		P0_2
#define CHS_I_STRB		P0_0
#define CHS_DO			P0_5
#define CHS_O_CLK		P0_3
#define CHS_O_STRB		P0_1
#define EN_AUD          P2_0

// AMBE frame length
#define AMBE_FRAME_LENGTH           (34)
#define AMBE_HEADER_LENGTH          (10)
// AMBE header define
#define AMBE_HEADER                 (0x13EC)
// AMBE input ID define
#define AMBE_ID_VOICE               (0x00)
#define AMBE_ID_RATECHANGE          (0x01)
#define AMBE_ID_GAINCHANGE          (0x02)
#define AMBE_ID_AD                  (0x03)
#define AMBE_ID_SLEEP               (0x04)
#define AMBE_ID_DTMF                (0x06)
#define AMBE_ID_WAKEUP              (0xFE)

// AMBE rate selection define
#define CONTROL1_RATE2400_FEC0      (0x4130)
#define CONTROL2_RATE2400_FEC0      (0x0000)
#define CONTROL1_RATE2350_FEC50     (0x4130)
#define CONTROL2_RATE2350_FEC50     (0x0001)

#define CONTROL1_RATE3600_FEC0      (0x6148)
#define CONTROL2_RATE3600_FEC0      (0x0000)
#define CONTROL1_RATE3350_FEC250    (0x5148)
#define CONTROL2_RATE3350_FEC250    (0x0020)
// Rate = 4000bps
#define CONTROL1_RATE3750_FEC250    (0x6150)
#define CONTROL2_RATE3750_FEC250    (0x0020)
// Rate = 4800bps
#define CONTROL1_RATE4800_FEC0      (0xA360)
#define CONTROL2_RATE4800_FEC0      (0x0000)
#define CONTROL1_RATE4550_FEC250    (0xA360)
#define CONTROL2_RATE4550_FEC250    (0x0020)
#define CONTROL1_RATE3600_FEC1200   (0x6160)
#define CONTROL2_RATE3600_FEC1200   (0x9006)
#define CONTROL1_RATE2550_FEC2250   (0x5160)
#define CONTROL2_RATE2550_FEC2250   (0x9400)
// Rate = 6400bps
#define CONTROL1_RATE4150_FEC2250   (0xA380)
#define CONTROL2_RATE4150_FEC2250   (0x9600)
// Rate = 7200bps
#define CONTROL1_RATE4400_FEC2800   (0xA390)
#define CONTROL2_RATE4400_FEC2800   (0x9800)
// Rate = 8000bps
#define CONTROL1_RATE7750_FEC250    (0xE4A0)
#define CONTROL2_RATE7750_FEC250    (0x0020)
#define CONTROL1_RATE4650_FEC3350   (0xA3A0)
#define CONTROL2_RATE4650_FEC3350   (0x9A00)
// Rate = 9600bps
#define CONTROL1_RATE9600_FEC0      (0xE4C0)
#define CONTROL2_RATE9600_FEC0      (0x0000)
#define CONTROL1_RATE4850_FEC4750   (0xA3C0)
#define CONTROL2_RATE4850_FEC4750   (0xF200)

/*******************************************************************************
// Local structure and variable define
*******************************************************************************/
typedef struct _TAudioHandle {
    uint8       nRate;
    uint8       nOutGain;
    uint8       nInGain;
    uint8       nWorkMode;
    uint8       nForceSilence;
}TAUDIOHANDLE, *PAUDIOHANDLE;

static TAUDIOHANDLE tAudio;

static uint8 BitArray[8] = {0x01, 0x02, 0x04, 0x08, 0x10, 0x20, 0x40,0x80};
//uint8 BitArray[8] = {0x80, 0x40, 0x20, 0x10, 0x08, 0x04, 0x02,0x01};

static uint8 OutGainIdx = 4;
static uint8 OutGainArray[] = {0x00,0x40,0x80,0xC0,0xFF};
/*******************************************************************************
// Local subroutine define
*******************************************************************************/
static void ReadSerial(uint8 *pDataH, uint8 *pDataL);
static void WriteSerial(uint8 DataH, uint8 DataL);

/*******************************************************************************
// Read two data(16 bits) from AMBE,  using serial
*******************************************************************************/
void ReadSerial(uint8 *pDataH, uint8 *pDataL)
{
	int8 nI = 0;
	
	*pDataH = 0;
	*pDataL = 0;
	
	CHS_O_STRB = 0;
    asm("nop");
    asm("nop");
    
    CHS_O_CLK = 1;
    asm("nop");
    asm("nop");
    CHS_O_CLK = 0;
    asm("nop");
    asm("nop");
    
	for (nI=7; nI>=0; nI--) {
		if (CHS_DO == 1) {
			*pDataH |= BitArray[nI];
		}      
		CHS_O_CLK = 1;
        asm("nop");      
        asm("nop");
        CHS_O_CLK = 0;
        asm("nop");
        asm("nop");

	}
	CHS_O_STRB = 1;
	for (nI=7; nI>=0; nI--) {
		if (CHS_DO == 1) {
			*pDataL |= BitArray[nI];   
		}
		CHS_O_CLK = 1;
        asm("nop");
        asm("nop");
		CHS_O_CLK = 0;
        asm("nop");
        asm("nop");
	}
}
/*******************************************************************************
// Read encoder data from audio(for AMBE, it's 34bytes per read)
// pBuf: pointer to read buf assigned from high level
// nMaxLen: maximal length for one read, i.e. the length of assigned read buf.
            (here it must be equal or larger than 6, depending on coding rate)
// return value: -1: fail, others: data length of this read
*******************************************************************************/

//uint8 tmp[34];

int8 ReadAudio(uint8 *pBuf, uint8 nMaxLen)
{
    uint8 nI = 0;
    uint8 data1 = 0;
    uint8 data2 = 0;
    
    if (nMaxLen < AMBE_RAWDATA_LEN_2400) {
        return -1;          // Read buf is not large enough
    }
    
    for (nI=0; nI<AMBE_HEADER_LENGTH; nI+=2) {
        ReadSerial(&data1, &data2);
        //tmp[nI] = data1;
        //tmp[nI+1] = data2;
    }        
    for (nI=0; nI<AMBE_RAWDATA_LEN_2400; nI+=2) {
        ReadSerial(&pBuf[nI], &pBuf[nI+1]);
    }
    for (nI=0; nI<(AMBE_FRAME_LENGTH-AMBE_HEADER_LENGTH-AMBE_RAWDATA_LEN_2400); nI+=2) {
        ReadSerial(&data1, &data2);
    }        
    
    return AMBE_RAWDATA_LEN_2400;
}


/*******************************************************************************
// Write 2 data(16 bits) into AMBE, using serial
*******************************************************************************/
void WriteSerial(uint8 DataH, uint8 DataL)
{
	int8 nI = 0;
	
	CHS_I_STRB = 0;
//    asm("nop");
	CHS_I_CLK = 0;
// 	asm("nop");
//    asm("nop");
	CHS_I_CLK = 1;
    
	for (nI=7; nI>=0; nI--) {
		CHS_I_CLK = 0;   
        if (DataH & BitArray[nI]) {
            CHS_DI = 1;
        } else {
            CHS_DI = 0;
        }
//        asm("nop");
		CHS_I_CLK = 1;
//        asm("nop");
//        asm("nop");
	}
	CHS_I_STRB = 1;
	for (nI=7; nI>=0; nI--) {
		CHS_I_CLK = 0;  
        if (DataL & BitArray[nI]) {
            CHS_DI = 1;
        } else {
            CHS_DI = 0;
        }        
//        asm("nop");
		CHS_I_CLK = 1;
//        asm("nop");
//        asm("nop");
	}
}

/*******************************************************************************
// Write decoder data into audio(fro AMBE, it must be 6bytes per write for 2400bps bitrate)
// pBuf: pointer to write buf containing data to be decoded by AMBE
// nLen: data length to be written
// retrun value: -1 fail, others: data length of this write
*******************************************************************************/

int8 WriteAudio(uint8 *pBuf, uint8 nLen)
{
    uint8 nI;
    
    if (nLen != AMBE_RAWDATA_LEN_2400) {
        return -1;      // for AMBE, every writing must be 6 bytes for 2400bps)
    }
  
    // Write Header, always wirte gain control frame
    WriteSerial(0x13, 0xEC);
    WriteSerial(AMBE_ID_GAINCHANGE, tAudio.nForceSilence << 1);
    WriteSerial(tAudio.nInGain, tAudio.nOutGain);
    WriteSerial(0x01, 0xf4);
    WriteSerial(0x00, 0x00);
    // Write encoding data
    for(nI=0;nI<AMBE_RAWDATA_LEN_2400;nI+=2)
    {
        WriteSerial(pBuf[nI], pBuf[nI+1]);
    }    
    // Write 0
    for(nI=0;nI<AMBE_FRAME_LENGTH-AMBE_HEADER_LENGTH-AMBE_RAWDATA_LEN_2400;nI+=2)
    {
        WriteSerial(0, 0);
    }
    
    return AMBE_RAWDATA_LEN_2400;
}    

/*******************************************************************************
// Turn off Audio(AMBE)
*******************************************************************************/
void StopAudio(void)
{
/*
//    P2SEL &= ~0x01;      //P2.0 gpio
//    P2DIR |= 0x01;       //p2.0 output
//    P0DIR |= 0x3f;      //P0.0~P0.5 output
//    P1DIR |= 0x03;      //P1.0 P1.1 output
//    P1IEN &= ~0x03;			// disable P1.0 P1.1 interrupt
//    P1IFG &= ~0x03;			// clear ifg of P1.0 P1.1
//  	P1IEN &= ~0x03;			// disable P1.0 P1.1 interrupt
//    P1IFG &= ~0x03;			// clear ifg of P1.0 P1.1
    P0DIR &= ~0x1f;      //P0.0~P0.4 input
//    CHS_I_STRB = 0;
//    CHS_I_CLK = 0;
//    CHS_O_STRB = 0;
//    CHS_O_CLK = 0;
//    CHS_DI = 0;
//    CHS_DO = 0;
//    EPR = 0;
//    DPE = 0;
    
    EN_AUD = 0;
*/
    P2SEL &= ~0x01;      //P2.0 gpio
    P2DIR |= 0x01;       //p2.0 output
    
    P0DIR &= ~0x1f;      //P0.0~P0.4 iutput
    
    P1IEN &= ~0x03;      //P1.0 P1.1 interrupt disable
    P1IFG &= ~0x03;      //clear int flag on P1.0 P1.1

    CHS_I_STRB = 0;
    CHS_I_CLK = 0;
    CHS_O_STRB = 0;
    CHS_O_CLK = 0;
    CHS_DI = 0;
    CloseCodec();
    EN_AUD = 0;

}
/*******************************************************************************
// Turn on Audio(AMBE)
*******************************************************************************/
void StartAudio(void)
{
/*  
//    P1IEN |= 0x03;			// enable P1.0 P1.1 interrupt
//    P1IFG &= ~0x03;			// clear ifg of P1.0 P1.1
    
    EN_AUD = 1;
//    DelayMs(10);
    P0DIR |= 0x1f;      //P0.0~P0.4 output
    CHS_I_STRB = 1;
    CHS_I_CLK = 0;
    CHS_O_STRB = 1;
    CHS_O_CLK = 0;
*/
    StopAudio();
    DelayMs(10);
    EN_AUD = 1;
    
    P0SEL &= ~0x3f;     //P0.0~P0.5 gpio //~0x0f;     //P0.0-P0.3 gpio
    P0DIR &= ~0x20;     //P0.5 input //~0x03;     //P0.0-P0.1 input
    P0DIR |= 0x1f;      //P0.0~P0.4 output   //0x0C;      //P0.2-P0.3 output
    
    P1SEL &= ~0x03;     //P1.0 P1.1  gpio       //~0x0f;		//P1.0-P1.3 gpio
//    P1DIR |= 0x07;		//P1.0-P1.2 output
    P1DIR &= ~0x03;
	//P1IEN |= 0x03;//P1.0 P1.1 interrupt enable
    //P1IFG = 0;
    P1IFG &= ~0x03;
  
    CHS_I_STRB = 1;
    CHS_I_CLK = 0;
    CHS_O_STRB = 1;
    CHS_O_CLK = 0;
    InitialCodec();
} 
/*******************************************************************************
// Init Audio(AMBE), just turn off audio
*******************************************************************************/
void InitialAudio(void)
{
/*
//    P2INP |= 0x60;        //Set P0 P1 input pull down
    
    P2SEL &= ~0x01;      //P2.0 gpio
    P2DIR |= 0x01;       //p2.0 output
    
    P0SEL &= ~0x3f;     //P0.0~P0.5 gpio 
    P0DIR &= ~0x20;     //P0.5 input 
    P0DIR &= ~0x1f;      //P0.0~P0.4 input   
    
    P1SEL &= ~0x03;     //P1.0 P1.1  gpio       
    P1DIR &= ~0x03;		//P1.0 P1.1 input

//	IRCON2 &= ~0x08;		// clear ifg of P1
    
//	P1IEN &= ~0x03;			// disable P1.0 P1.1 interrupt
//	P1IFG &= ~0x03;			// clear ifg of P1.0 P1.1
    
    EN_AUD = 0;
*/
    StopAudio();

    // Init audio structure
    tAudio.nRate = AMBE_BITRATE_2400;
    tAudio.nOutGain = 0xff;     // 0x80;
    //tAudio.nInGain = 0xfc; 
    tAudio.nInGain = INGAIN_PHONE2PHONE; 
    //tAudio.nInGain = 0x00; 
    tAudio.nWorkMode = 0;
    tAudio.nForceSilence = 0;

//    P1IF = 0;           // clear interrupt flag on P1     
}  
    

/*******************************************************************************
// Control Audio(AMBE)
// nCtlID: control id
// pBuf: pointer to buf containing control data
// nLen: control data length
*******************************************************************************/
int ControlAudio(uint8 nCtlID, uint8* pBuf, uint8 nLen)
{
    switch (nCtlID) {
    case AMBE_SET_BITRATE:
        if (pBuf[0] > AMBE_BITRATE_MAX) {
            return -1;
        }
        tAudio.nRate = pBuf[0];
        break;
    case AMBE_SET_OUTPUTGAIN:
        tAudio.nOutGain = pBuf[0];
        break;
    case AMBE_SET_INPUTGAIN:
        tAudio.nInGain = pBuf[0];
        break;
    case AMBE_SET_SLEEPMODE:
        break;
    case AMBE_SET_WAKEUP:
        break;
    case AMBE_SET_SLIENCE:
        tAudio.nForceSilence = (pBuf[0] & 1);
        break;
    default:
        return -1;
    }
        
    return 0;
}

/*******************************************************************************
// Write silence frame into audio.
*******************************************************************************/
__near_func int8 WriteSilence(void)
{
	uint8 nI;
    // Write Header, always wirte gain control frame
    WriteSerial(0x13, 0xEC);
    WriteSerial(AMBE_ID_GAINCHANGE, 0x02);
    WriteSerial(tAudio.nInGain, tAudio.nOutGain);
    WriteSerial(0x01, 0xf4);
    WriteSerial(0x00, 0x00);
    // Write encoding data
    for(nI=0;nI<AMBE_RAWDATA_LEN_2400;nI+=2)
    {
        WriteSerial(0x00, 0x00);
    }    
    // Write 0
    for(nI=0;nI<AMBE_FRAME_LENGTH-AMBE_HEADER_LENGTH-AMBE_RAWDATA_LEN_2400;nI+=2)
    {
        WriteSerial(0, 0);
    }
    return AMBE_RAWDATA_LEN_2400;
}

/*******************************************************************************
// Play perticular sound
*******************************************************************************/
void PlayNote (uint16 Note1, uint16 Note2, uint16 Gain)
{
//Gain Max = 0x7000, e.g. 3.17dBm0
//Gain Min = 0xD800, e.g. -117.56dBm0(silence)
    uint8 j;
        WriteSerial(0x13, 0xEC);
        WriteSerial(0x06, 0x00);
        WriteSerial((Gain>>8)&0xff, Gain);
        WriteSerial((Gain>>8)&0xff, Gain);
        WriteSerial((Note1>>8)&0xff, Note1);
        WriteSerial((Note2>>8)&0xff, Note2);

        for(j=0;j<11;j++) WriteSerial(0, 0);
}

/*******************************************************************************
// Audio Control
*******************************************************************************/
void AudioSetOutputGain(uint8 idx)
{
	if(idx < sizeof(OutGainArray)/sizeof(OutGainArray[0]))
	{
	    OutGainIdx = idx;
	    tAudio.nOutGain = OutGainArray[OutGainIdx];
	}
}
uint8  AudioGetOutputGain(void)
{
    return OutGainIdx;
}

void AudioSetInputGain(uint8 InGain)
{
	tAudio.nInGain = InGain;
}

uint8  AudioGetInputGain(void)
{
    return tAudio.nInGain;
}

void AudioIntoSleep()
{
    StopAudio();
    P0DIR |= 0x1f;           //change port P0.0~P0.4 to  output to save power;
    P0INP |=(0x01<<5);   // p0_5 to tristate
	
    CHS_I_STRB = 0;
    CHS_I_CLK = 0;
    CHS_O_STRB = 0;
    CHS_O_CLK = 0;
    CHS_DI = 0;
    EN_AUD = 0;
	
}

