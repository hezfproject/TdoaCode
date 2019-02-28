#include <iocc2430.h>
#include "audio_ambe.h"


/*******************************************************************************
// Local macro define
*******************************************************************************/
// ambe control signal define
#define AMBE_EPR                    P0_0
#define AMBE_DPE                    P0_1
#define AMBE_RDN                    P0_2
#define AMBE_WRN                    P0_3

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

#define Delay16us       wait(0x48)
#define Delay2Cycles  wait(0x02)

// Local subroutine define
__near_func void WriteAMBE(uint8 nData);
void wait( uint16 timeout );

/*******************************************************************************
// Delay
*******************************************************************************/
void wait( uint16 timeout )
{
	while (timeout--)
	{
		asm("NOP");//a machine cycle: 12/32 us.
		//asm("NOP");
		//asm("NOP");
	}
}

/*******************************************************************************
// Read encoder data from audio(for AMBE, it's 34bytes per read)
// pBuf: pointer to read buf assigned from high level
// nMaxLen: maximal length for one read, i.e. the length of assigned read buf.
            (here it must be equal or larger than 6, depending on coding rate)
// return value: -1: fail, others: data length of this read
*******************************************************************************/
uint8 __idata sign_p = 0; //FIXME: need a global idata to relay the data from port1 to xdata, and the reason is not clear now.
//uint8 __idata tmp[10];
__near_func int8 ReadAudio(uint8 *pBuf, uint8 nMaxLen)
{
    uint8 nI;
    if (nMaxLen < AMBE_RAWDATA_LEN_2400) {
        return -1;          // Read buf is not large enough
    }
    P1DIR = 0x00;          //P1 set to input
    for (nI=0; nI<AMBE_HEADER_LENGTH; nI++) {
        AMBE_RDN = 0;
	 Delay2Cycles;
        //tmp[nI] = P1;
        AMBE_RDN = 1;
        Delay16us;
    }        
    for (nI=0; nI<AMBE_RAWDATA_LEN_2400; nI++) {
        AMBE_RDN = 0;
	 Delay2Cycles;
	 sign_p = P1;
        pBuf[nI] = sign_p;
        AMBE_RDN = 1;
        Delay16us;
    }
    for (nI=0; nI<(AMBE_FRAME_LENGTH-AMBE_HEADER_LENGTH-AMBE_RAWDATA_LEN_2400); nI++) {
        AMBE_RDN = 0;
	 Delay2Cycles;
        AMBE_RDN = 1;
        Delay16us;
    }

    P0IFG &= ~0x04;
    return AMBE_RAWDATA_LEN_2400;
}


/*******************************************************************************
// Write one data into AMBE, be sure init P1 before use this function
*******************************************************************************/
//uint8 __idata sign_pp = 0;
__near_func void WriteAMBE(uint8 nData)
{
    P1= nData;
    //sign_pp = nData;
    //P1 = sign_pp;
    AMBE_WRN = 0;
    Delay2Cycles;
    AMBE_WRN = 1;
    Delay16us;    
}

/*******************************************************************************
// Write decoder data into audio(fro AMBE, it must be 6bytes per write)
// pBuf: pointer to write buf containing data to be decoded by AMBE
// nLen: data length to be written
// retrun value: -1 fail, others: data length of this write
*******************************************************************************/

__near_func int8 WriteAudio(uint8 *pBuf, uint8 nLen)
{
    uint8 nI;
    
    if (nLen != AMBE_RAWDATA_LEN_2400) {
        return -1;      // for AMBE, every writing must be 6 bytes for 2400bps)
    }
  
    P1DIR = 0xFF;       //P1 set to output
    // Write Header
    WriteAMBE(0x13);
    WriteAMBE(0xEC);
	
    WriteAMBE(AMBE_ID_VOICE);
    WriteAMBE(0x00);    // control0
    for (nI=0; nI<6; nI++) {
        WriteAMBE(0x00);    // control1--3
    }
    // Write encoding data
    for(nI=0;nI<AMBE_RAWDATA_LEN_2400;nI++)
    {
    	//sign_p = pBuf[nI];
       // WriteAMBE(sign_p);
       WriteAMBE(pBuf[nI]);
    }    
    // Write 0
    for(nI=0;nI<AMBE_FRAME_LENGTH-AMBE_HEADER_LENGTH-AMBE_RAWDATA_LEN_2400;nI++)
    {
        WriteAMBE(0);
    }

    P0IFG &= ~0x08;
    return AMBE_RAWDATA_LEN_2400;
}    

/*******************************************************************************
// Init Audio(AMBE)
*******************************************************************************/
void InitialAudio(void)
{
    P0SEL &= ~0x10;     //P0.4 gpio
    P0DIR |= 0x10;      //P0.4 output
    
    AUDIO_OFF;
    P0IFG = 0; //clear interrupt if possible.
}  
/*******************************************************************************
// Start Audio(AMBE), P1 for data bus, P0 for control signas and interrupts
*******************************************************************************/
void StartAudio(void)
{
    
    P0SEL &= ~0x0f;     //P0.0-P0.3 gpio
    P0DIR &= ~0x03;     //P0.0-P0.1 input
    P0DIR |= 0x0C;      //P0.2-P0.3 output
    
    P1SEL = 0x00;       //P1 general io
    P1DIR = 0x00;       //P1 all input
    
    PICTL |= 0x08;      //P0IENL=1, means P0.3 to P0.0 interrupt enable;
    PICTL &= 0xFE;      //把P0_0口的中断设为上升沿有效

    P0IFG = 0; //clear interrupt if possible.
    P0IF = 0;           //clear P0 interrupt flag
    P0IE = 1;           //P0 interrupt enable
   // P1DIR = 0x00;
    EA = 1;             //Global interrupt enable
    
    wait(0xff);        //Wait for AMBE reset
    
    AMBE_RDN = 0;
    //AMBE_WRN = 0;
    Delay16us;
    AMBE_RDN = 1;
    //AMBE_WRN = 1;       //Force RDN and WRN low to get first interrupt form EPR and/or DPE
    P0IFG &= ~0x04;
    //P0IFG &= ~0x08;
}


__near_func void StartWrite(void)
{
	AMBE_WRN = 0;
	Delay16us;
	AMBE_WRN = 1;
	P0IFG &= ~0x08;
}

__near_func void StartRead(void)
{
    AMBE_RDN = 0;
    Delay16us;
    AMBE_RDN = 1;
    P0IFG &= ~0x04;
}

/*******************************************************************************
// Write silence frame into audio.
*******************************************************************************/
__near_func int8 WriteSilence(void)
{
    uint8 nI;
    P1DIR = 0xFF;       //P1 set to output
    // Write Header
    WriteAMBE(0x13);
    WriteAMBE(0xEC);
    WriteAMBE(AMBE_ID_VOICE);
    WriteAMBE(0x02);    // control0 to silence.
    for (nI=0; nI<6; nI++) {
        WriteAMBE(0x00);    // control1--3
    }

    P0IFG &= ~0x08;
    return AMBE_RAWDATA_LEN_2400;
}

