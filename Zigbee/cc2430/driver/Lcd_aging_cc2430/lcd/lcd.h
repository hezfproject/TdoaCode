/**************************************************************************************************
  Filename:       lcd.h
  Description:     This file contains the interfaces to config lcd of yiri RRN board for mobile phone.
  Porting from lcd_key.c of hardware group.
**************************************************************************************************/
#ifndef LCD_H
#define LCD_H

#include "hal_defs.h"
#include "hal_types.h"
#include <iocc2430.h>

// LCD backlight define
#define BACKLIGHT_ON            (P0_5 = 0)
#define BACKLIGHT_OFF           (P0_5 = 1)

#define BACKLIGHT_OPEN() \
st ( \
	P0IFG &= ~0x20; \
	BACKLIGHT_ON; \
	P0IFG &= ~0x20; \
)

#define BACKLIGHT_CLOSE() \
st ( \
	P0IFG &= ~0x20; \
	BACKLIGHT_OFF; \
	P0IFG &= ~0x20; \
)

// Timer1 operation define
#define TIMER1_RESET        (T1CNTL = 0x00)
#define TIMER1_START        (T1CTL |= 0x02)
#define TIMER1_STOP         (T1CTL &= 0xFC)
#define TIMER1_INT_ENABLE   (IEN1 |= 0X02)
#define TIMER1_INT_CLEAR    (IRCON &= 0XFD)


// Dispaly key on LCD
void LCDDisplay(int16 key);

// Initial LCD
void InitialLCD(void);
// Start LCD
void StartLCD(void);

#endif
