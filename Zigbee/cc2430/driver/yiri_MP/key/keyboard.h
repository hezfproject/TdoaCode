/**************************************************************************************************
  Filename:       keyboard.h
  Description:     This file contains the interfaces to config keyboard of yiri RRN board for mobile phone.
  Porting from lcd_key.c of hardware group.
**************************************************************************************************/
#ifndef KEYBORAD_H
#define KEYBORAD_H

#include "hal_defs.h"
#include "hal_types.h"
#include "hal_key_cfg.h"
#include <iocc2430.h>

// Key name define
#define KEY_INVALID     (0x0000)
#define KEY_0           (HAL_KEY_0)
#define KEY_1           (HAL_KEY_1)
#define KEY_2           (HAL_KEY_2)
#define KEY_3           (HAL_KEY_3)
#define KEY_4           (HAL_KEY_4)
#define KEY_5           (HAL_KEY_5)
#define KEY_6           (HAL_KEY_6)
#define KEY_7           (HAL_KEY_7)
#define KEY_8           (HAL_KEY_8)
#define KEY_9           (HAL_KEY_9)
#define KEY_YES       (HAL_KEY_CALL)
#define KEY_NO        (HAL_KEY_BACKSPACE)

// Timer1 operation define
#define TIMER1_RESET        (T1CNTL = 0x00)
#define TIMER1_START        (T1CTL |= 0x02)
#define TIMER1_STOP         (T1CTL &= 0xFC)
#define TIMER1_INT_ENABLE   (IEN1 |= 0X02)
#define TIMER1_INT_CLEAR    (IRCON &= 0XFD)

extern uint8 Nmbr[]; //only access by hal_key.
// Initial Key
void InitialKey(void);

// Get key by adc sampling
uint16 GetKey(void);

//Get The numbers.
uint8 GetNumbers(void);
//
uint8 ResetNumberBuf(void);
#endif

