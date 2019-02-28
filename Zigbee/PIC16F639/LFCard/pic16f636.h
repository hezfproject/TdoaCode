
#ifndef	_HTC_H_
#warning Header file pic16f636.h included directly. Use #include <htc.h> instead.
#endif


 /* header file for the MICROCHIP PIC microcontrollers
  * PIC12F635
  * PIC16F636
  * PIC16F639
 */

#ifndef	__PIC16F636_H
#define	__PIC16F636_H

// Special function register definitions

volatile       unsigned char	INDF		@ 0x00;
volatile       unsigned char	TMR0		@ 0x01;
volatile       unsigned char	PCL		@ 0x02;
volatile       unsigned char	STATUS		@ 0x03;
volatile	unsigned char	FSR		@ 0x04;
#if defined(_16F636) || defined(_16F639)
volatile       unsigned char	PORTA		@ 0x05;
volatile       unsigned char	PORTC		@ 0x07;
#else
volatile       unsigned char	GPIO		@ 0x05;
#endif
volatile       unsigned char	PCLATH		@ 0x0A;
volatile       unsigned char	INTCON		@ 0x0B;
volatile       unsigned char	PIR1		@ 0x0C;
volatile       unsigned char	TMR1L		@ 0x0E;
volatile       unsigned char	TMR1H		@ 0x0F;
volatile       unsigned char	T1CON		@ 0x10;
volatile       unsigned char	WDTCON		@ 0x18;
volatile       unsigned char	CMCON0		@ 0x19;
volatile	unsigned char	CMCON1		@ 0x1A;
volatile	unsigned char	OPTION		@ 0x81;
#if defined(_16F636) || defined(_16F639)
volatile       unsigned char	TRISA		@ 0x85;
volatile       unsigned char	TRISC		@ 0x87;
#else
volatile       unsigned char	TRISIO		@ 0x85;
#endif
volatile	unsigned char	PIE1		@ 0x8C;
volatile       unsigned char	PCON		@ 0x8E;
volatile       unsigned char	OSCCON		@ 0x8F;
volatile	unsigned char	OSCTUNE		@ 0x90;
volatile       unsigned char	LVDCON		@ 0x94;
volatile	unsigned char	WPUDA		@ 0x95;
volatile	unsigned char	IOCA		@ 0x96;
volatile	unsigned char	WDA		@ 0x97;
volatile	unsigned char	VRCON		@ 0x99;
volatile       unsigned char	EEDAT		@ 0x9A;
/* Alternate definition */
volatile       unsigned char	EEDATA		@ 0x9A;
volatile       unsigned char	EEADR		@ 0x9B;
/* Alternate definition */
volatile       unsigned char	EEADRL		@ 0x9B;
volatile       unsigned char	EECON1		@ 0x9C;
volatile       unsigned char	EECON2		@ 0x9D;
volatile       unsigned char	CRCON		@ 0x110;
volatile       unsigned char	CRDAT0		@ 0x111;
volatile       unsigned char	CRDAT1		@ 0x112;
volatile       unsigned char	CRDAT2		@ 0x113;
volatile       unsigned char	CRDAT3		@ 0x114;


/* Definitions for STATUS register */
volatile       bit	CARRY		@ ((unsigned)&STATUS*8)+0;
volatile       bit	DC		@ ((unsigned)&STATUS*8)+1;
volatile       bit	ZERO		@ ((unsigned)&STATUS*8)+2;
volatile       bit	PD		@ ((unsigned)&STATUS*8)+3;
volatile       bit	TO		@ ((unsigned)&STATUS*8)+4;
volatile	bit	RP0		@ ((unsigned)&STATUS*8)+5;
volatile	bit	RP1		@ ((unsigned)&STATUS*8)+6;
volatile	bit	IRP		@ ((unsigned)&STATUS*8)+7;

#if defined(_16F636) || defined(_16F639)
/* Definitions for PORTA register */
volatile       bit	RA0		@ ((unsigned)&PORTA*8)+0;
volatile       bit	RA1		@ ((unsigned)&PORTA*8)+1;
volatile       bit	RA2		@ ((unsigned)&PORTA*8)+2;
volatile       bit	RA3		@ ((unsigned)&PORTA*8)+3;
volatile       bit	RA4		@ ((unsigned)&PORTA*8)+4;
volatile       bit	RA5		@ ((unsigned)&PORTA*8)+5;

/* Definitions for PORTC register */
volatile       bit	RC0		@ ((unsigned)&PORTC*8)+0;
volatile       bit	RC1		@ ((unsigned)&PORTC*8)+1;
volatile       bit	RC2		@ ((unsigned)&PORTC*8)+2;
volatile       bit	RC3		@ ((unsigned)&PORTC*8)+3;
volatile       bit	RC4		@ ((unsigned)&PORTC*8)+4;
volatile       bit	RC5		@ ((unsigned)&PORTC*8)+5;
#else
/* Definitions for GPIO register */
volatile       bit	GP0		@ ((unsigned)&GPIO*8)+0;
volatile       bit	GP1		@ ((unsigned)&GPIO*8)+1;
volatile       bit	GP2		@ ((unsigned)&GPIO*8)+2;
volatile       bit	GP3		@ ((unsigned)&GPIO*8)+3;
volatile       bit	GP4		@ ((unsigned)&GPIO*8)+4;
volatile       bit	GP5		@ ((unsigned)&GPIO*8)+5;
#endif

/* Definitions for INTCON register */
volatile       bit	RAIF		@ ((unsigned)&INTCON*8)+0;
volatile       bit	INTF		@ ((unsigned)&INTCON*8)+1;
volatile       bit	T0IF		@ ((unsigned)&INTCON*8)+2;
volatile	bit	RAIE		@ ((unsigned)&INTCON*8)+3;
volatile	bit	INTE		@ ((unsigned)&INTCON*8)+4;
volatile	bit	T0IE		@ ((unsigned)&INTCON*8)+5;
volatile	bit	PEIE		@ ((unsigned)&INTCON*8)+6;
volatile bit	GIE		@ ((unsigned)&INTCON*8)+7;

/* Definitions for PIR1 register */
volatile       bit	TMR1IF		@ ((unsigned)&PIR1*8)+0;
volatile       bit	OSFIF		@ ((unsigned)&PIR1*8)+2;
volatile       bit	C1IF		@ ((unsigned)&PIR1*8)+3;
#if defined(_16F636) || defined(_16F639)
volatile       bit	C2IF		@ ((unsigned)&PIR1*8)+4;
#endif
volatile       bit	CRIF		@ ((unsigned)&PIR1*8)+5;
volatile       bit	LVDIF		@ ((unsigned)&PIR1*8)+6;
volatile       bit	EEIF		@ ((unsigned)&PIR1*8)+7;

/* Definitions for T1CON register */
volatile	bit	TMR1ON		@ ((unsigned)&T1CON*8)+0;
volatile	bit	TMR1CS		@ ((unsigned)&T1CON*8)+1;
volatile	bit	T1SYNC		@ ((unsigned)&T1CON*8)+2;
volatile	bit	T1OSCEN		@ ((unsigned)&T1CON*8)+3;
volatile	bit	T1CKPS0		@ ((unsigned)&T1CON*8)+4;
volatile	bit	T1CKPS1		@ ((unsigned)&T1CON*8)+5;
volatile	bit	TMR1GE		@ ((unsigned)&T1CON*8)+6;
volatile	bit	T1GINV		@ ((unsigned)&T1CON*8)+7;

/* Definitions for WDTCON register */
volatile	bit	SWDTEN		@ ((unsigned)&WDTCON*8)+0;
volatile	bit	WDTPS0		@ ((unsigned)&WDTCON*8)+1;
volatile	bit	WDTPS1		@ ((unsigned)&WDTCON*8)+2;
volatile	bit	WDTPS2		@ ((unsigned)&WDTCON*8)+3;
volatile	bit	WDTPS3		@ ((unsigned)&WDTCON*8)+4;

/* Definitions for CMCON0 register */
volatile	bit	CM0		@ ((unsigned)&CMCON0*8)+0;
volatile	bit	CM1		@ ((unsigned)&CMCON0*8)+1;
volatile	bit	CM2		@ ((unsigned)&CMCON0*8)+2;
volatile	bit	CIS		@ ((unsigned)&CMCON0*8)+3;
volatile	bit	C1INV		@ ((unsigned)&CMCON0*8)+4;
#if defined(_16F636) || defined(_16F639)
volatile	bit	C2INV		@ ((unsigned)&CMCON0*8)+5;
#endif
volatile       bit	C1OUT		@ ((unsigned)&CMCON0*8)+6;
#if defined(_16F636) || defined(_16F639)
volatile       bit	C2OUT		@ ((unsigned)&CMCON0*8)+7;
#endif

/* Definitions for CMCON1 register */
volatile	bit	C2SYNC		@ ((unsigned)&CMCON1*8)+0;
volatile	bit	T1GSS		@ ((unsigned)&CMCON1*8)+1;

/* Definitions for OPTION register */
volatile	bit	PS0		@ ((unsigned)&OPTION*8)+0;
volatile	bit	PS1		@ ((unsigned)&OPTION*8)+1;
volatile	bit	PS2		@ ((unsigned)&OPTION*8)+2;
volatile	bit	PSA		@ ((unsigned)&OPTION*8)+3;
volatile	bit	T0SE		@ ((unsigned)&OPTION*8)+4;
volatile	bit	T0CS		@ ((unsigned)&OPTION*8)+5;
volatile	bit	INTEDG		@ ((unsigned)&OPTION*8)+6;
volatile	bit	RAPU		@ ((unsigned)&OPTION*8)+7;

#if defined(_16F636) || defined(_16F639)
/* Definitions for TRISA register */
volatile	bit	TRISA0		@ ((unsigned)&TRISA*8)+0;
volatile	bit	TRISA1		@ ((unsigned)&TRISA*8)+1;
volatile	bit	TRISA2		@ ((unsigned)&TRISA*8)+2;
volatile	bit	TRISA3		@ ((unsigned)&TRISA*8)+3;
volatile	bit	TRISA4		@ ((unsigned)&TRISA*8)+4;
volatile	bit	TRISA5		@ ((unsigned)&TRISA*8)+5;

/* Definitions for TRISC register */
volatile       bit	TRISC0		@ ((unsigned)&TRISC*8)+0;
volatile       bit	TRISC1		@ ((unsigned)&TRISC*8)+1;
volatile       bit	TRISC2		@ ((unsigned)&TRISC*8)+2;
volatile       bit	TRISC3		@ ((unsigned)&TRISC*8)+3;
volatile       bit	TRISC4		@ ((unsigned)&TRISC*8)+4;
volatile       bit	TRISC5		@ ((unsigned)&TRISC*8)+5;
#else
/* Definitions for TRISIO register */
volatile	bit	TRISIO0		@ ((unsigned)&TRISIO*8)+0;
volatile	bit	TRISIO1		@ ((unsigned)&TRISIO*8)+1;
volatile	bit	TRISIO2		@ ((unsigned)&TRISIO*8)+2;
volatile	bit	TRISIO3		@ ((unsigned)&TRISIO*8)+3;
volatile	bit	TRISIO4		@ ((unsigned)&TRISIO*8)+4;
volatile	bit	TRISIO5		@ ((unsigned)&TRISIO*8)+5;
#endif

/* Definitions for PIE1 register */
volatile	bit	TMR1IE		@ ((unsigned)&PIE1*8)+0;
volatile	bit	OSFIE		@ ((unsigned)&PIE1*8)+2;
volatile	bit	C1IE		@ ((unsigned)&PIE1*8)+3;
#if defined(_16F636) || defined(_16F639)
volatile	bit	C2IE		@ ((unsigned)&PIE1*8)+4;
#endif
volatile	bit	CRIE		@ ((unsigned)&PIE1*8)+5;
volatile	bit	LVDIE		@ ((unsigned)&PIE1*8)+6;
volatile	bit	EEIE		@ ((unsigned)&PIE1*8)+7;

/* Definitions for PCON register */
volatile       bit	BOD		@ ((unsigned)&PCON*8)+0;
volatile       bit	POR		@ ((unsigned)&PCON*8)+1;
volatile       bit	WUR		@ ((unsigned)&PCON*8)+3;
volatile	bit	SBODEN		@ ((unsigned)&PCON*8)+4;
volatile	bit	ULPWUE		@ ((unsigned)&PCON*8)+5;

/* Definitions for OSCCON register */
volatile	bit	SCS		@ ((unsigned)&OSCCON*8)+0;
volatile       bit	LTS		@ ((unsigned)&OSCCON*8)+1;
volatile       bit	HTS		@ ((unsigned)&OSCCON*8)+2;
volatile       bit	OSTS		@ ((unsigned)&OSCCON*8)+3;
volatile	bit	IRCF0		@ ((unsigned)&OSCCON*8)+4;
volatile	bit	IRCF1		@ ((unsigned)&OSCCON*8)+5;
volatile	bit	IRCF2		@ ((unsigned)&OSCCON*8)+6;

/* Definitions for OSCTUNE register */
volatile	bit	TUN0		@ ((unsigned)&OSCTUNE*8)+0;
volatile	bit	TUN1		@ ((unsigned)&OSCTUNE*8)+1;
volatile	bit	TUN2		@ ((unsigned)&OSCTUNE*8)+2;
volatile	bit	TUN3		@ ((unsigned)&OSCTUNE*8)+3;
volatile	bit	TUN4		@ ((unsigned)&OSCTUNE*8)+4;

/* Definitions for LVDCON register */
volatile	bit	LVDL0		@ ((unsigned)&LVDCON*8)+0;
volatile	bit	LVDL1		@ ((unsigned)&LVDCON*8)+1;
volatile	bit	LVDL2		@ ((unsigned)&LVDCON*8)+2;
volatile	bit	LVDEN		@ ((unsigned)&LVDCON*8)+4;
volatile       bit	IRVST		@ ((unsigned)&LVDCON*8)+5;

/* Definitions for WPUDA register */
volatile	bit	WPUDA0		@ ((unsigned)&WPUDA*8)+0;
volatile	bit	WPUDA1		@ ((unsigned)&WPUDA*8)+1;
volatile	bit	WPUDA2		@ ((unsigned)&WPUDA*8)+2;
volatile	bit	WPUDA4		@ ((unsigned)&WPUDA*8)+4;
volatile	bit	WPUDA5		@ ((unsigned)&WPUDA*8)+5;

/* Definitions for IOCA register */
volatile	bit	IOCA0		@ ((unsigned)&IOCA*8)+0;
volatile	bit	IOCA1		@ ((unsigned)&IOCA*8)+1;
volatile	bit	IOCA2		@ ((unsigned)&IOCA*8)+2;
volatile	bit	IOCA3		@ ((unsigned)&IOCA*8)+3;
volatile	bit	IOCA4		@ ((unsigned)&IOCA*8)+4;
volatile	bit	IOCA5		@ ((unsigned)&IOCA*8)+5;

/* Definitions for WDA register */
volatile	bit	WDA0		@ ((unsigned)&WDA*8)+0;
volatile	bit	WDA1		@ ((unsigned)&WDA*8)+1;
volatile	bit	WDA2		@ ((unsigned)&WDA*8)+2;
volatile	bit	WDA4		@ ((unsigned)&WDA*8)+4;
volatile	bit	WDA5		@ ((unsigned)&WDA*8)+5;

/* Definitions for VRCON register */
volatile	bit	VR0		@ ((unsigned)&VRCON*8)+0;
volatile	bit	VR1		@ ((unsigned)&VRCON*8)+1;
volatile	bit	VR2		@ ((unsigned)&VRCON*8)+2;
volatile	bit	VR3		@ ((unsigned)&VRCON*8)+3;
volatile	bit	VRR		@ ((unsigned)&VRCON*8)+5;
volatile	bit	VREN		@ ((unsigned)&VRCON*8)+7;

/* Definitions for EECON1 register */
volatile       bit	RD		@ ((unsigned)&EECON1*8)+0;
volatile       bit	WR		@ ((unsigned)&EECON1*8)+1;
volatile       bit	WREN		@ ((unsigned)&EECON1*8)+2;
volatile       bit	WRERR		@ ((unsigned)&EECON1*8)+3;

/* Definitions for CRCON register */
volatile       bit	CRREG0		@ ((unsigned)&CRCON*8)+0;
volatile       bit	CRREG1		@ ((unsigned)&CRCON*8)+1;
volatile       bit	ENCDEC		@ ((unsigned)&CRCON*8)+6;
volatile       bit	GODONE		@ ((unsigned)&CRCON*8)+7;


// Configuration Mask Definitions
#define CONFIG_ADDR	0x2007
// Oscillator 
#define EXTCLK		0x3FFF	// External RC Clockout
#define EXTIO		0x3FFE	// External RC No Clock
#define INTCLK		0x3FFD	// Internal RC Clockout
#define INTIO		0x3FFC	// Internal RC No Clock
#define EC		0x3FFB	// EC
#define HS		0x3FFA	// HS
#define XT		0x3FF9	// XT
#define LP		0x3FF8	// LP
// Watchdog Timer 
#define WDTEN		0x3FFF	// On
#define WDTDIS		0x3FF7	// Off
// Power Up Timer 
#define PWRTDIS		0x3FFF	// Off
#define PWRTEN		0x3FEF	// On
// Master Clear Enable 
#define MCLREN		0x3FFF	// MCLR function is enabled
#define MCLRDIS		0x3FDF	// MCLR functions as IO
// Code Protect 
#define UNPROTECT	0x3FFF	// Code is not protected
#define CP		0x3FBF	// Code is protected
#define PROTECT		CP	//alternate
// Data EE Read Protect 
#define UNPROTECT	0x3FFF	// Do not read protect EEPROM data
#define CPD		0x3F7F	// Read protect EEPROM data
// Brown Out Detect 
#define BORDIS		0x3CFF	// BOD and SBOREN disabled
#define SBOREN		0x3DFF	// SBOREN controls BOR function (Software control)
#define BORXSLP		0x3EFF	// BOD enabled in run, disabled in sleep, SBOREN disabled
#define BOREN		0x3FFF	// BOD Enabled, SBOREN Disabled
// Internal External Switch Over Mode 
#define IESOEN		0x3FFF	// Enabled
#define IESODIS		0x3BFF	// Disabled
// Monitor Clock Fail-safe 
#define FCMEN		0x3FFF	// Enabled
#define FCMDIS		0x37FF	// Disabled
// Wake-Up Reset 
#define WAKECNT		0x3FFF	// Continue upon wake-up
#define WAKERST		0x2FFF	// Reset upon wake-up

#endif
