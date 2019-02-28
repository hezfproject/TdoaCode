#ifndef __KEY__
#define __KEY__

#include "hal_defs.h"
#include "hal_types.h"

/* Interrupt option - Enable or disable */
#define HAL_KEY_INTERRUPT_DISABLE    0x00
#define HAL_KEY_INTERRUPT_ENABLE     0x01

/* Key state - ADC, nornal and so on.
*/
#define HAL_KEY_STATE_NORMAL          0x00
#define HAL_KEY_STATE_SHIFT           0x01

#define HAL_KEY_INVALID        (0xFF)
#define HAL_KEY_SELECT         (1)
#define HAL_KEY_UP             (2)
#define HAL_KEY_BACKSPACE      (3)
#define HAL_KEY_CALL           (4)
#define HAL_KEY_LEFT           (5)
#define HAL_KEY_RIGHT          (6)
#define HAL_KEY_POWER          (7)
#define HAL_KEY_CANCEL          HAL_KEY_POWER
#define HAL_KEY_DOWN           (8)
#define HAL_KEY_1              (9)
#define HAL_KEY_2              (10)
#define HAL_KEY_3              (11)
#define HAL_KEY_4              (12)
#define HAL_KEY_5              (13)
#define HAL_KEY_6              (14)
#define HAL_KEY_7              (15)
#define HAL_KEY_8              (16)
#define HAL_KEY_9              (17)
#define HAL_KEY_STAR           (18)
#define HAL_KEY_0              (19)
#define HAL_KEY_POUND          (20)


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
