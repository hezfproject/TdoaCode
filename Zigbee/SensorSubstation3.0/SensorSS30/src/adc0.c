//-----------------------------------------------------------------------------
// Includes
//-----------------------------------------------------------------------------
#include <c8051f350.h>                    // SFR declarations
#include <stdio.h>                        // Standard I/O Library
#include <math.h>
#include <intrins.h>
#include "c51f350.h"


//-----------------------------------------------------------------------------
// 16-bit SFR Definitions for 'F35x
//-----------------------------------------------------------------------------
sfr16 ADC0DEC  = 0x9a;

//-----------------------------------------------------------------------------
// Global CONSTANTS
//-----------------------------------------------------------------------------
#define MDCLK        2457600                // Modulator Clock (Hz)
#define OWR          10                     // desired Output Word Rate in Hz
#define VREF_3_V     (3.0)

//-----------------------------------------------------------------------------
// Function PROTOTYPES
//-----------------------------------------------------------------------------
static void ADC0_Init();

//-----------------------------------------------------------------------------
// MAIN Routine
//-----------------------------------------------------------------------------
typedef union LONGDATA{                 // Access LONGDATA as an
   UINT32 result;                       // unsigned long variable or
   UINT8 Byte[4];                       // 4 unsigned byte variables
}LONGDATA;

LONGDATA xdata GetData;

INT32 xdata sample_array[2];				// sample_array[0] MUX ONE  sample_array[1] MUX TWO
void ADC_GetValue()
{
    volatile INT32 ADC_OutputVal = 0;       // Concatenated ADC output value
    if (AD0INT)
    {
        ADC0MD  = 0x80;					//Set ADC to Idle mode
        ADC_OutputVal   = (UINT32)ADC0L + ((UINT32)ADC0M << 8) + ((UINT32)ADC0H << 16);

        if(ADC_OutputVal & 0x800000)
            ADC_OutputVal |= 0xFF000000;
            
        ADC_OutputVal = (ADC_OutputVal * VREF_3_V / 0x7FFFFF) * 1000;
        
        switch (ADC0MUX)
        {
        case 0x01:
            sample_array[0] = ADC_OutputVal * 10 / 15;
            ADC0MUX = 0x28;		 		// Set next Difference Input AIN+ => AIN0.0  AIN- => AIN0.1
            break;
            
        case 0x28:
            sample_array[1] = ADC_OutputVal * 1000 / 39;
            ADC0MUX = 0x01;		 		// Set next Difference Input AIN+ => AIN0.2  AIN- => AGND
            break;
            
        default:
            break;
        }
        
        AD0INT = 0;                        // clear AD0 interrupt flag    
        ADC0MD  = 0x82;                    // start Single Sample mode
    // 0.1mA
    }
}

// This function initializes the ADC to measure across AIN0.1 and AIN0.0
// on the Target Board (Differential measurements, Bipolar codes)
//
void ADC0_Init()
{
    UINT8 u8Wd;

    WATCHDOG_FEED();
    WATCHDOG_SAVE(u8Wd);
    WATCHDOG_OFF();

    REF0CN &= ~0x01;                     // (disable if using internal vref)
	ADC0CF |= 0x04;					     //External	Vref
    ADC0MD  = 0x80;					     //Set ADC to Idle mode
    ADC0CN = 0x10;                      // Bipolar output codes, GAIN=1,双极性输出

                                        // 使用SINC3的输出结束作为中断
                                        // and uses extern VREF 2.5v
    // Generate MDCLK for modulator.
    // Ideally MDCLK = 2.4576
    ADC0CLK = (SYSTEMCLOCK / MDCLK) - 1;

    // ADC0 输出字速率  = MDCLK / [128 * (D ECI[10:0] + 1)]
    // program decimation rate for desired OWR
	ADC0DEC = (UINT32)((MDCLK / OWR) >> 7) - 1;//program decimation rate for desired OWR
    ADC0BUF = 0x00;                        // Turn on Input Buffers

	ADC0MUX = 0x01;                      // Difference Input	   AIN+ => AIN0.0	AIN- => AIN0.1

    ADC0MD |= 0x01;                      // Full internal calibration
    while (!AD0CALC);                      // Wait for calibration complete

    ADC0MD &= ~0x07;                     // Set ADC to Idle mode
	ADC0MD  = 0x82;                      // Single Sample mode
	ADC_GetValue();
    WATCHDOG_SET(u8Wd);
}
