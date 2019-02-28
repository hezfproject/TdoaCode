#ifndef __KEY__
#define __KEY__

#include "hal_key_cfg.h"
#include "hal_defs.h"
#include "hal_types.h"
/*
// data type define
#ifndef uint8
#define uint8   unsigned char
#endif
#ifndef uint16
#define uint16  unsigned short
#endif
#ifndef uint32
#define uint32  unsigned int
#endif
#ifndef int8
#define int8    signed char
#endif
#ifndef int16
#define int16   short
#endif
#ifndef int32
#define int32   int
#endif
*/
// Key name define
#define KEY_INVALID             (0xFF)
#define KEY_NAME_SELECT         HAL_KEY_SELECT         // KEY_SEG0_DIG0
#define KEY_NAME_UP             HAL_KEY_UP         // KEY_SEG0_DIG1
#define KEY_NAME_BACKSPACE      HAL_KEY_BACKSPACE         // KEY_SEG0_DIG2
#define KEY_NAME_CALL         HAL_KEY_CALL         // KEY_SEG0_DIG3
#define KEY_NAME_LEFT         HAL_KEY_LEFT         // KEY_SEG0_DIG4
#define KEY_NAME_RIGHT      HAL_KEY_RIGHT         // KEY_SEG0_DIG5
#define KEY_NAME_POWER     HAL_KEY_POWER         // KEY_SEG0_DIG6
#define KEY_NAME_DOWN      HAL_KEY_DOWN         // KEY_SEG0_DIG7
#define KEY_NAME_1              HAL_KEY_1         // KEY_SEG1_DIG0
#define KEY_NAME_2              HAL_KEY_2         // KEY_SEG1_DIG1
#define KEY_NAME_3              HAL_KEY_3       // KEY_SEG1_DIG2
#define KEY_NAME_4              HAL_KEY_4        // KEY_SEG1_DIG3
#define KEY_NAME_5              HAL_KEY_5        // KEY_SEG1_DIG4
#define KEY_NAME_6              HAL_KEY_6        // KEY_SEG1_DIG5
#define KEY_NAME_7              HAL_KEY_7        // KEY_SEG1_DIG6
#define KEY_NAME_8              HAL_KEY_8       // KEY_SEG1_DIG7
#define KEY_NAME_9              HAL_KEY_9        // KEY_SEG2_DIG0
#define KEY_NAME_STAR        HAL_KEY_STAR        // KEY_SEG2_DIG1 "*"
#define KEY_NAME_0              HAL_KEY_0        // KEY_SEG2_DIG2
#define KEY_NAME_POUND     HAL_KEY_POUND       // KEY_SEG2_DIG3 "#"

//backlight define
#define BACKLIGHT_INIT     st(P0SEL &= ~0x40;P0DIR |= 0x40;)      		// set P0.6 to output
#define BACKLIGHT_ON    (P0_6 = 1)
#define BACKLIGHT_OFF   (P0_6 = 0)

#define BACKLIGHT_OPEN() \
st ( \
	BACKLIGHT_ON; \
)

#define BACKLIGHT_CLOSE() \
st ( \
	BACKLIGHT_OFF; \
)

// Timer1 operation define
#define TIMER1_RESET        (T1CNTL = 0x00)
#define TIMER1_START        (T1CTL |= 0x02)
#define TIMER1_STOP         (T1CTL &= 0xFC)
#define TIMER1_INT_ENABLE   (IEN1 |= 0X02)
#define TIMER1_INT_CLEAR    (IRCON &= 0XFD)

// Initial Key
void InitialKey(void);

// Get key by adc sampling
uint16 GetKey(void);

void KeyIntoSleep(void);

void WaitKeySleep(uint16 TimeOut);


 void KeyReset(void);

void KeyEnable(void);

#endif  // __KEY__
