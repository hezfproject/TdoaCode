#ifndef _BUTTON_H_
#define _BUTTON_H_

// button key bit mask
#define BTN_IDLE    0x00
#define BTN_HELP    0x01
#define BTN_CFRM    0x02

//void BUTTON_RCC_Configuration(void);

void BUTTON_Configuration(void);

uint_8 BUTTON_KeyCode_Read(void);

#endif

