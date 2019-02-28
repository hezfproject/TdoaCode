#include <iocc2430.h>
#include "key.h"
#include "delay.h"
#include "WatchDogUtil.h"

/***************************
// Local macro define
****************************/
// Key-chip interface define
#define KEY_SCL			P1_3
#define KEY_SDA			P1_4


#define KEY_INT_ENABLE()   (P1IEN |= 0x10)
#define KEY_INT_DISABLE() (P1IEN &= ~0x10)
/***************************
// Local subroutine define
****************************/
void ReadKeyChip(uint8 cmd, uint8 *pdata);
void WriteKeyChip(uint8 cmd, uint8 data);

// Initial BackLight
//void InitialBackLight(void);
/****************************************************************
*** Read key-scan chip
*****************************************************************/

void ReadKeyChip(uint8 cmd, uint8 *pdata)
{
	int8 i = 0;
	uint8 data = 0;
	uint8 intState;
	intState = EA;
	EA = 0;
	// Change SDA(P1.4) back to output
	P1DIR |= 0x10;	
	// disable interrupt on sda(p1.4)
	P1IEN &= ~0x10;		
	DelayUs(5);

	KEY_SDA = 1;
	KEY_SCL = 1;
	DelayUs(5);
	KEY_SDA = 0;	// send start signal
	DelayUs(5);
	KEY_SCL = 0;
	DelayUs(5);

	for (i=7; i>=0; i--) {
		KEY_SDA = (cmd >> i) & 1;	// send command
		DelayUs(5);
		KEY_SCL = 1;
		DelayUs(5);
		KEY_SCL = 0;
		DelayUs(5);
	}

	// Change SDA(P1.4) to input
	P1DIR &= ~0x10;			

	KEY_SDA = 1;	// send "1", wait acknoledge 
	DelayUs(5);
	KEY_SCL = 1;
	DelayUs(5);
	KEY_SCL = 0;
	DelayUs(5);



	for (i=7; i>=0; i--) {
		KEY_SCL = 1;
		DelayUs(5);		
		data |= KEY_SDA << i;
		KEY_SCL = 0;
		DelayUs(5);
	}	

	// Change SDA(P1.4) back to output
	P1DIR |= 0x10;	

	KEY_SCL = 1;
	KEY_SDA = 1;
	DelayUs(5);

	// enable interrupt on sda(p1.4)
	P1DIR &= ~0x10;			// set P1.4 to input
	P1IFG &= ~0x10;			// clear ifg of P1.4
	P1IEN |= 0x10;		    

	*pdata = data;		
	EA = intState;
}


/****************************************************************
*** Get key from key-scan chip
*****************************************************************/
/****************************************************************
*** Get key from key-scan chip
*****************************************************************/

uint16 GetKey(void)
{
	uint8 key_data;

	ReadKeyChip(0x6f, &key_data);

	switch (key_data) {
		case (0x40):
			return  HAL_KEY_SELECT;     // KEY_SEG0_DIG0;
		case (0x41):
			return  HAL_KEY_UP;         // KEY_SEG0_DIG1;
		case (0x42):
			return  HAL_KEY_BACKSPACE;  // KEY_SEG0_DIG2;
		case (0x43):
			return  HAL_KEY_CALL;       // KEY_SEG0_DIG3;
		case (0x44):
			return  HAL_KEY_LEFT;       // KEY_SEG0_DIG4;
		case (0x45):
			return  HAL_KEY_RIGHT;      // KEY_SEG0_DIG5;
		case (0x46):
			return  HAL_KEY_POWER;      // KEY_SEG0_DIG6;
		case (0x47):
			return  HAL_KEY_DOWN;       // KEY_SEG0_DIG7;
		case (0x48):
			return  HAL_KEY_1;          // KEY_SEG1_DIG0;
		case (0x49):
			return  HAL_KEY_2;          // KEY_SEG1_DIG1;
		case (0x4a):
			return  HAL_KEY_3;          // KEY_SEG1_DIG2;
		case (0x4b):
			return  HAL_KEY_4;          // KEY_SEG1_DIG3;
		case (0x4c):
			return  HAL_KEY_5;          // KEY_SEG1_DIG4;
		case (0x4d):
			return  HAL_KEY_6;          // KEY_SEG1_DIG5;
		case (0x4e):
			return  HAL_KEY_7;          // KEY_SEG1_DIG6;
		case (0x4f):
			return  HAL_KEY_8;          // KEY_SEG1_DIG7;
		case (0x50):
			return  HAL_KEY_9;          // KEY_SEG2_DIG0;
		case (0x51):
			return  HAL_KEY_STAR;       // KEY_SEG2_DIG1;
		case (0x52):
			return  HAL_KEY_0;          // KEY_SEG2_DIG2;
		case (0x53):
			return  HAL_KEY_POUND;      // KEY_SEG2_DIG3;
		default:
			return  HAL_KEY_INVALID;
	}
}

/****************************************************************
*** Write key-scan chip
*****************************************************************/
void WriteKeyChip(uint8 cmd, uint8 data)
{
	int8 i = 0;

	uint8 intState = EA;
	EA = 0;

	// disable interrupt on sda(p1.4)
	P1IEN &= ~0x10;		
	DelayMs(1);	
	// Change SDA(P1.4) back to output
	P1DIR |= 0x10;	

	KEY_SDA = 1;
	KEY_SCL = 1;
	DelayUs(5);
	KEY_SDA = 0;	// send start signal
	DelayUs(5);
	KEY_SCL = 0;
	DelayUs(5);

	for (i=7; i>=0; i--) {
		KEY_SDA = (cmd >> i) & 1;	// send command
		DelayUs(5);
		KEY_SCL = 1;
		DelayUs(5);
		KEY_SCL = 0;
		DelayUs(5);
	}

	// Change SDA(P1.4) to input
	P1DIR &= ~0x10;			

	KEY_SDA = 1;	// send "1", wait acknoledge 
	DelayUs(5);
	KEY_SCL = 1;
	DelayUs(5);
	KEY_SCL = 0;
	DelayUs(5);

	// Change SDA(P1.4) back to output
	P1DIR |= 0x10;	

	for (i=7; i>=0; i--) {
		KEY_SDA = (data >> i) & 1;	// send data
		DelayUs(5);
		KEY_SCL = 1;
		DelayUs(5);
		KEY_SCL = 0;
		DelayUs(5);
	}	

	KEY_SDA = 0;
	DelayUs(5);
	KEY_SCL = 1;
	DelayUs(5);
	KEY_SDA = 1;		
	DelayUs(5);

	// enable interrupt on sda(p1.4)
	//    P1INP |= 0x10;                     //add by yhh go to three-trible  
	P1DIR &= ~0x10;			// set P1.4 to input    
	P1IFG &= ~0x10;			// clear ifg of P1.4
	P1IEN |= 0x10;	
	EA = intState;
}

/*******************************************************************************
// Initial Keyboard
*******************************************************************************/
/****************************************************************
*** Initial key-scan chip and key interrupt
*****************************************************************/
void InitialKey(void)
{

	// init P1.4 to interrupt, rising edge trigger
	P1SEL &= ~0x10;			// set P1.4 to general io
	P1DIR &= ~0x10;			// set P1.4 to input
	//	PICTL &= ~0x02;			// rising edge trigger for P1
	P1IFG &= ~0x10;			// clear ifg of P1.4
	//	IRCON2 &= ~0x08;		// clear ifg of P1    
	//	P1IEN |= 0x10;			// enable P1.4 interrupt
	//	IEN2 |= 0X10;   		// enable P1 interrupt
	//	EA = 1;					// global interrupt enable


	// init P1.3 to output, used as SCL
	P1SEL &= ~0x08;			// set P1.3 to general io
	P1DIR |= 0x08;			// set P1.3 to output

	// init P1.4 to output, used as SDA
	P1SEL &= ~0x10;			// set P1.4 to general io
	P1DIR |= 0x10;			// set P1.4 to output

	// init SCL and SDA to 1
	KEY_SCL = 1;
	KEY_SDA = 1;

	KeyReset();
	// init key-scan chip
	//1, set key-scan chip parameters
	WriteKeyChip(0x68, 0x22);   // 0x22);	// enable key-scan, INTM set to falling pulse
	//    CH452_Write(CH452_SYSON2);
	//2, put key-scan chip into sleep mode
	WriteKeyChip(0x64, 0x02);	

	P1DIR |= 0x01<<2;       // initial shake controller
	P1_2 = 0;
}


void KeyIntoSleep(void)
{
	WriteKeyChip(0x64, 0x02);	
}

void WaitKeySleep(uint16 TimeOut)
{
	uint16 testInterval = 100;  
	uint16 testnum = TimeOut/testInterval;
	for(uint16 i=0; i<testnum;i++)
	{
		// DelayMs(testInterval);
		uint8 key_tmp = GetKey();

#if(defined WATCHDOG) &&(WATCHDOG == TRUE)
		FeedWatchDog();
#endif
		if(key_tmp == HAL_KEY_INVALID)
			break;
	}
	KeyIntoSleep();
}
void KeyReset(void)
{
	WriteKeyChip(0x64, 0x01);	
	//DelayMs(100);
	DelayUs(500);
}


/* enable key by time */
void KeyEnable(void)
{
	KEY_INT_DISABLE();
	// init key-scan chip
	//1, set key-scan chip parameters
	WriteKeyChip(0x68, 0x22);   // 0x22);	// enable key-scan, INTM set to falling pulse
	//    CH452_Write(CH452_SYSON2);
	//2, put key-scan chip into sleep mode
	WriteKeyChip(0x64, 0x02);	
	KEY_INT_ENABLE();
}

/*
void InitialBackLight(void)   	
{

// init P0_5 as backlight control
P0SEL &= ~0x40;   			// set P0.6 as general io, used as backlight control
P0DIR |= 0x40;      		// set P0.6 to output

#if 0
// init timer1
PERCFG |= 0x40;         	// Timer1 use alt2
CLKCON |= 0x38;         	// use 32M/128=1/4M for timer ticket
T1CTL = 0x0c;           	// Tick freq/128=1/512M=2k
//    T1CCTL0 = 0x54;         	// 8'b01010100
T1CC0L = 0x00;      
T1CC0H = 0x14;          	// expire time = 3s

// init timer1 interrupt
IEN1 |= 0X02;           	// timer1 interrupt enable
#endif
}
*/

