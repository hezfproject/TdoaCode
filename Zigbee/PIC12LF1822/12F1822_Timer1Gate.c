//***********************************************************************************
// Sample code to show how to use the Timer 1 Gate feature to determine the
// width of an incoming pulse.

// Device: PIC12F1822
// Compiler: Microchip XC8 v1.10
// IDE: MPLAB X v1.5
// Created: October 2012
//
// In this simple example we have a 5V pulse with a width that ranges from 1ms to 2ms
// coming into the Timer 1 Gate Pin (RA4).  This example uses the TMR1 Gate
// feature set up in 'Single Pulse Mode' to generate an interrupt when a pulse
// has been received.  Based on the width of the pulse, an LED (pin RA2) will
// be turned on or off

// **********************************************************************************

#include <htc.h>
#include <pic.h> 										// include standard header file
#include <stdlib.h>
#include <stdio.h>
#include <string.h>// include standard header file


// set Config bits
//#pragma config FOSC=INTOSC, WDTE=OFF, MCLRE=OFF, CP=OFF, CPD=OFF,BOREN=OFF
//#pragma config CLKOUTEN=OFF, IESO=OFF, FCMEN=OFF,FOSC = INTOSC, FCMEN=OFF
//#pragma config WRT=OFF,PLLEN=OFF,STVREN=ON,BORV=LO,LVP=OFF
__CONFIG ((FOSC_INTOSC  & WDTE_ON & MCLRE_OFF & CP_OFF & CPD_OFF & BOREN_OFF & CLKOUTEN_OFF & IESO_OFF &  FCMEN_OFF));
__CONFIG ((WRT_OFF & PLLEN_OFF & STVREN_ON & BORV_25 & LVP_OFF));
/****************************************************************/
// Definitions
#define _XTAL_FREQ  16000000        // this is used by the __delay_ms(xx) and __delay_us(xx) functions
#define PWM_In      PORTAbits.RA4   // RA4 is PWM input signal (this is T1G input)
#define LED         LATAbits.LATA2  // RA2 is LED for debugging purposes
#define PulseLimit  30000           // if pulse is longer than this value, turn the LED on else turn off.


// Global Variables   
volatile unsigned int PulseValue;   //this will contain our counter value
struct
{
  unsigned PulseFound :1;  // flag used to indicate that pulse value has been grabbed
}flags;


//*************************************************************************************

void interrupt Timer1_Gate_ISR()
{
    PulseValue = TMR1;  // read timer value
    flags.PulseFound=1; // set flag for main routine to use
    TMR1=0;             // clear timer value
    PIR1bits.TMR1GIF=0; // clear the the interrupt flag
    T1GCONbits.T1GGO=1; // set the T1GGO/Done bit so it starts looking for a pulse again
}
int sleepcnt=0;
//*************************************************************************************
void main ( )
{
    unsigned int Timer1Count;   // this will contain the Timer1 count value
    
    // set up oscillator control register
    OSCCONbits.SPLLEN=0;    // PLL is disabled
    OSCCONbits.IRCF=0x0F;   //set OSCCON IRCF bits to select OSC frequency=16Mhz
    OSCCONbits.SCS=0x02;    //set the SCS bits to select internal oscillator block
    // OSCON should be 0x7Ah now.

    // Set up I/O pins
    ANSELAbits.ANSELA=0;    // set all analog pins to digital I/O
    ADCON0bits.ADON=0;      // turn ADC off
    DACCON0bits.DACEN=0;    // turn DAC off
    
  
    //TRISAbits.TRISA0 = 0;	// RA0 = nc
    //TRISAbits.TRISA1 = 0;	// RA1 = nc
    //TRISAbits.TRISA2 = 0;	// RA2 = LED for debugging
    //TRISAbits.TRISA3 = 0;	// RA3 = nc (MCLR)
    TRISAbits.TRISA4 = 0;	// RA4 = PWM pulse input to gate
    //TRISAbits.TRISA5 = 0;	// RA5 = nc
    //TRISAbits.TRISA6 = 0;	// RA6 = no pin
    //TRISAbits.TRISA7 = 0;	// RA7 = no pin
	ANSELA=0x1F;		// all pins are digital I/O
	LATAbits.LATA4 = 1;

	
    // Set up the Timer1 Control Register (T1CON)
	while(1)
	{
		if(sleepcnt%2)
		{
	    	LATAbits.LATA4 = 1;
			WDTCON=0x0e;
		}
		else  // Timer 1 source is system clock (FOSC)
		{
			LATAbits.LATA4 = 0;
			WDTCON=0x13;
		}
		sleepcnt++;
	 		SLEEP();
		//CLRWDT();
	}
}
